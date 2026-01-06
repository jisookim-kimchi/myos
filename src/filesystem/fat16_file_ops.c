#include "fat16.h"
#include "../disk/disk.h"
#include "../disk/streamer.h"
#include "../memory/memory.h"
#include "../memory/heap/kernel_heap.h"
#include "../status.h"


int fat16_create_file(struct disk* disk, struct path_part* path)
{
    struct fat_private *f_private = disk->fs_private_data;
    struct disk_streamer* streamer = f_private->directory_stream;
    struct fat_directory *root = &f_private->root_directory;
    int max_entries = f_private->header.primary_header.root_dir_entries;
    for (int i = 0; i < max_entries; i++)
    {
        if (root->item[i].filename[0] == 0x00 || root->item[i].filename[0] == 0xE5)
        {
            ft_memset(&root->item[i], 0, sizeof(struct fat_directory_item));
            fat16_to_dos_filename(path->part, root->item[i].filename, root->item[i].ext);
            root->item[i].attribute = 0x00;
            
            // Write to disk
            int root_dir_sector = f_private->header.primary_header.reserved_sectors + (f_private->header.primary_header.number_of_fats * f_private->header.primary_header.sectors_per_fat);
            int entry_pos = (root_dir_sector * disk->sector_size) + (i * sizeof(struct fat_directory_item));
            
            disk_stream_seek(streamer, entry_pos);
            if(disk_stream_write(streamer, &root->item[i], sizeof(struct fat_directory_item)) < 0)
            {
                return -MYOS_IO_ERROR;
            }
            return 0;
        }
    }
    return -MYOS_IO_ERROR;
}

int fat16_seek(void *private, int offset, FILE_SEEK_MODE seek_mode)
{
    struct fat_file_descriptor *desc = private;
    struct fat_item* desc_item = desc->item;

    if (!desc || !desc_item || desc_item->type != FAT_ITEM_TYPE_FILE)
        return -MYOS_INVALID_ARG;

    int32_t new_pos = (int32_t)desc->pos;

    switch (seek_mode)
    {
        case FILE_SEEK_SET:
            new_pos = offset;
            break;
        case FILE_SEEK_END:
            new_pos = (int32_t)desc_item->dir_item->filesize + offset; // Relative to the end of the file
            break;
        case FILE_SEEK_CUR:
            new_pos += offset;
            break;
        default:
            return -MYOS_INVALID_ARG;
    }

    if (new_pos < 0 || new_pos > (int32_t)desc_item->dir_item->filesize)
    {
        return -MYOS_IO_ERROR;
    }

    desc->pos = (uint32_t)new_pos;
    return desc->pos;
}

int fat16_stat(void *private, struct file_stat *stat)
{
    struct fat_file_descriptor *desc = private;
    struct fat_item* desc_item = desc->item;

    if (!desc || !desc_item || desc_item->type != FAT_ITEM_TYPE_FILE)
    {
        return -MYOS_INVALID_ARG;
    }

    stat->size = desc_item->dir_item->filesize;
    stat->mode = 0x00;

    if (desc_item->dir_item->attribute & FAT_FILE_READ_ONLY)
    {
        stat->mode |= FILE_STAT_READ_ONLY;
    }
    if (desc_item->dir_item->attribute & FAT_FILE_SUBDIRECTORY)
    {
        stat->mode |= FILE_STAT_DIRECTORY;
    }
    // if (desc_item->dir_item->attribute & FAT_FILE_HIDDEN)
    // {
    //     stat->mode |= FILE_STAT_HIDDEN;
    // }
    // if (desc_item->dir_item->attribute & FAT_FILE_SYSTEM)
    // {
    //     stat->mode |= FILE_STAT_SYSTEM;
    // }
    // if (desc_item->dir_item->attribute & FAT_FILE_VOLUME_LABEL)
    // {
    //     stat->mode |= FILE_STAT_VOLUME_LABEL;
    //}
    // if (desc_item->dir_item->attribute & FAT_FILE_ARCHIVED)
    // {
    //     stat->mode |= FILE_STAT_ARCHIVE;
    // }
    return 0;
}

int fat16_close(void *private)
{
    struct fat_file_descriptor *desc = private;
    struct fat_item* desc_item = desc->item;

    if (!desc || !desc_item || desc_item->type != FAT_ITEM_TYPE_FILE)
        return -MYOS_INVALID_ARG;

    fat16_item_free(desc_item);
    kernel_free(desc);
    return 0;
}

//we consider just only for READONLY in moment
void* fat16_open(struct disk* disk, struct path_part* path, FILE_MODE mode)
{
    struct fat_file_descriptor *desc = kernel_zero_alloc(sizeof(struct fat_file_descriptor));
    if (!desc)
    {
        return ERROR(-MYOS_ERROR_NO_MEMORY);
    }
    desc->item = fat16_get_dir_entry(disk, path);
    if (desc->item == NULL)
    {
        if (mode == FILE_MODE_WRITE)
        {
            int res = fat16_create_file(disk, path);
            if (res < 0)
            {
                kernel_free(desc);
                return ERROR(res);
            }
            desc->item = fat16_get_dir_entry(disk, path);
        }
    }
    if (desc->item == NULL)
    {
        kernel_free(desc);
        return ERROR(-MYOS_FILE_NOT_FOUND);
    }
    desc->pos = 0;

    if (mode == FILE_MODE_WRITE)
    {
        struct fat_directory_item *item = desc->item->dir_item;
        int first_cluster = fat16_get_first_cluster(item);
        if (first_cluster != 0)
        {
            fat16_free_cluster_chain(disk, first_cluster);
            item->low_16_bits_first_cluster = 0;
            item->high_16_bits_first_cluster = 0;
            item->filesize = 0;
            fat16_update_directory_entry(disk, desc, 0);
        }
    }

  return desc;
}

int fat16_write_internal(struct disk* disk, int cluster, uint32_t offset, uint32_t total_bytes, void* data)
{
    struct fat_private *f_private = disk->fs_private_data;
    struct disk_streamer *streamer = f_private->cluster_read_stream;
    int cluster_size = fat16_get_cluster_size(disk, f_private);
    int cur_cluster = cluster;

    if (offset >= cluster_size)
    {
        int cluster_offset = offset / cluster_size;
        cur_cluster = fat16_get_cluster_chain_link(disk, cluster, cluster_offset); 
        offset = offset % cluster_size;
        if (cur_cluster == FAT_EOF_MARKER)
        {
            return -MYOS_EOF;
        }
    }

    int bytes_written = 0;
    while (total_bytes > 0)
    {
        int res = 0;
        int starting_sector = fat16_get_sector_from_cluster(f_private, cur_cluster);
        int offset_int_cluster = offset % cluster_size;

        uint32_t cluster_absolute_pos = starting_sector * disk->sector_size + offset_int_cluster;
        
        disk_stream_seek(streamer, cluster_absolute_pos);
       
        //calculate how many bytes to write
        int bytes_to_write = total_bytes;
        if (bytes_to_write > cluster_size - offset_int_cluster)
        {
            bytes_to_write = cluster_size - offset_int_cluster;
        }

        res = disk_stream_write(streamer, (unsigned char*)data + bytes_written, bytes_to_write);
        if (res < 0)
        {
            return res;
        }
        total_bytes -= bytes_to_write;
        bytes_written += bytes_to_write;
        offset += bytes_to_write;

        if (total_bytes > 0)
        {
            int next_cluster = fat16_get_next_cluster(disk, cur_cluster);
            if (next_cluster == FAT_EOF_MARKER)
            {
                int free_cluster = fat16_find_free_cluster(disk);
                if (free_cluster < 0)
                {
                    return MYOS_ERROR_NO_MEMORY;
                }
                fat16_set_next_cluster(disk, cur_cluster, free_cluster);
                fat16_set_next_cluster(disk, free_cluster, FAT_EOF_MARKER);
                cur_cluster = free_cluster;
            }
            else
            {
                cur_cluster = next_cluster;
            }
        }
    }
    return bytes_written;
}

int fat16_write(struct disk *disk, void *private, uint32_t size, uint32_t nmemb, char *in)
{
    struct fat_file_descriptor* descriptor = (struct fat_file_descriptor*) private;
    struct fat_directory_item* item = descriptor->item->dir_item;

    int offset = descriptor->pos;
    uint32_t total = size * nmemb;

    // 1. 첫 클러스터 번호 가져오기
    int cluster = item->low_16_bits_first_cluster | (item->high_16_bits_first_cluster << 16);
    
    //첫 클러스터가 0이면 새로 할당 (새 파일인 경우)
    if (cluster == 0)
    {
        int free_cluster = fat16_find_free_cluster(disk);
        if (free_cluster < 0)
        {
            return -MYOS_IO_ERROR;
        }
        
        // 1-1. FAT 테이블에 할당 표시 (EOF)
        fat16_set_next_cluster(disk, free_cluster, FAT_EOF_MARKER);
        
        // 1-2. 디렉토리 엔트리에 첫 클러스터 정보 업데이트
        item->low_16_bits_first_cluster = (uint16_t)(free_cluster & 0xFFFF);
        item->high_16_bits_first_cluster = (uint16_t)((free_cluster >> 16) & 0xFFFF);
        
        // 1-3. 디렉토리 엔트리를 디스크에 저장 (중요!)
        // 여기서 업데이트 안 하면 나중에 파일 목록 다시 읽을 때 클러스터 정보가 날아감
        fat16_update_directory_entry(disk, descriptor, item->filesize);
        
        cluster = free_cluster;
    }
    int res = fat16_write_internal(disk, cluster, offset, total, in);
    if (res >= 0)
    {
        descriptor->pos += res;
        if (descriptor->pos > item->filesize)
        {
            item->filesize = descriptor->pos;
            fat16_update_directory_entry(disk, descriptor, item->filesize);
        }
    }
    
    return res;
}

int fat16_read_internal(struct disk* disk, int cluster, int offset, int size, void* out_buffer)
{
    int bytes_read = 0;
    // int bytes_to_read_this_cluster = 0;
    int res = 0;
    struct fat_private* fat_private = disk->fs_private_data;
    struct disk_streamer* streamer = fat_private->cluster_read_stream;

    int cluster_size = fat16_get_cluster_size(disk, fat_private);
    int cur_cluster = cluster;

    if (offset >= cluster_size)
    {
        int cluster_offset = offset / cluster_size;
        // FAT 테이블을 조회하여 다음 클러스터 번호를 찾고 업데이트
        cur_cluster = fat16_get_cluster_chain_link(disk, cur_cluster, cluster_offset); 
        // 오프셋을 현재 클러스터 내의 위치로 재설정
        offset = offset % cluster_size;
        // 만약 클러스터 체인 끝(EOF)에 도달했다면 에러 또는 종료 처리
        if (cur_cluster >= FAT_EOF_MARKER)
        {
            res = MYOS_EOF;
            goto destroyed;
        }
    }
    while (size > 0)
    {
        int starting_sector = fat16_get_sector_from_cluster(fat_private, cur_cluster);
        uint32_t cluster_absolute_pos = (uint32_t)starting_sector * disk->sector_size + offset;
        // 2-2. 스트리머 위치 설정 (Seek)
        disk_stream_seek(streamer, cluster_absolute_pos);
        int remaining_in_cluster = cluster_size - offset;
        int bytes_to_read = remaining_in_cluster;
        
        if (bytes_to_read > size)
        {
            bytes_to_read = size;
        }
        res = disk_stream_read(streamer, (unsigned char*)out_buffer + bytes_read, bytes_to_read);
        if (res < 0)
        {
            goto destroyed;
        }
        //update status
        size -= res;
        bytes_read += res;
        offset += res;
        if (offset >= cluster_size)
        {
            //move to next cluster
            cur_cluster = fat16_get_next_cluster(disk, cur_cluster);
            offset = 0;
            if (cur_cluster == FAT_EOF_MARKER)
            {
                break;
            }
        }
    }
    res = bytes_read;

destroyed:
    //TODO : probably we dont have to get rid of it here
    //destroy_disk_streamer(streamer);

    //maybe we have to do it in fat16_unresolve
    //fat16_unresolve(fat_private);
    return res;
}

int fat16_read(struct disk *disk, uint32_t offset, void *private_data, uint32_t read_size, uint32_t nmemb, char *out)
{
  struct fat_file_descriptor *ffd = private_data;

  if (!ffd || !ffd->item)
    return -MYOS_INVALID_ARG;

  struct fat_directory_item *item = ffd->item->dir_item;
  uint32_t total_bytes = read_size * nmemb;

  // Respect file size
  if (offset >= item->filesize)
    return 0;

  if (offset + total_bytes > item->filesize)
    total_bytes = item->filesize - offset;

  uint32_t first_cluster = fat16_get_first_cluster(item);

  return fat16_read_internal(disk, first_cluster, offset, total_bytes, out);
}