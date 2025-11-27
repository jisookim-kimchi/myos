#include "fat16.h"
#include "../string/string.h"
#include "disk/disk.h"
#include "disk/streamer.h"
#include "stdint.h"
#include "status.h"
#include "kernel.h"


static int fat16_get_cluster_size(struct disk* disk,struct fat_private* private);
static struct fat_directory* fat16_load_for_sub_directory(struct disk* disk, struct fat_directory_item* item);
static struct fat_item *fat16_new_item_from_directory_item(struct disk *disk, struct fat_directory_item *directory_item);
static int fat16_get_next_cluster(struct disk* disk, int cur_cluster);

// small debug helper: print a single byte as two hex chars
static void fat16_print_hex_byte(unsigned char b)
{
    char h[3] = {0};
    const char *hex = "0123456789ABCDEF";
    h[0] = hex[(b >> 4) & 0xF];
    h[1] = hex[b & 0xF];
    print(h);
}



//gloabl struct resolve, open, name
filesystem_t fat16_filesystem = 
{
    .resolve = fat16_resolve,
    .open = fat16_open,
    //.read = fat16_read
};

filesystem_t *fat16_init()
{
    ft_strcpy(fat16_filesystem.name, "FAT16");
    return &fat16_filesystem;
}

static void fat16_private_init(struct disk* disk, struct fat_private* fat_private)
{
    ft_memset(fat_private, 0, sizeof(struct fat_private));
    fat_private->cluster_read_stream = create_disk_streamer(disk->id);
    fat_private->fat_read_stream = create_disk_streamer(disk->id);
    fat_private->directory_stream = create_disk_streamer(disk->id);
}

int fat16_sector_to_absolute(struct disk *disk, int root_dir_sector_pos)
{
    return root_dir_sector_pos * (disk->sector_size);
}

int fat16_get_total_items_for_dir(struct disk* disk, int root_dir_sector_pos)
{
    struct fat_directory_item item;
    struct fat_directory_item empty;
    ft_memset(&empty, 0, sizeof(struct fat_directory_item));

    struct fat_private* fat_private = disk->fs_private_data;

    int res = 0;
    int i = 0;
    int directory_start_pos = root_dir_sector_pos * disk->sector_size;
    struct disk_streamer* stream = fat_private->directory_stream;
    if(disk_stream_seek(stream, directory_start_pos) < 0)
    {
        res = -MYOS_IO_ERROR;
        goto out;
    }
    while (1)
    {
        if (disk_stream_read(stream, &item, sizeof(struct fat_directory_item)) < 0)
        {
            res = -MYOS_IO_ERROR;
            goto out;
        }
        if (item.filename[0] == 0x00)
        {
            break;
        }
        if (item.filename[0] != 0xE5)
        {
            i++;
        }
    }
    res = i;

out:
    return res;
}

int fat16_get_root_directory(struct disk* disk, struct fat_private* fat_private, struct fat_directory* out_directory)
{
    int res = 0;

    struct fat_header* primary_header = &fat_private->header.primary_header;
    int root_dir_sector_pos = (primary_header->fat_copies * primary_header->sectors_per_fat) + primary_header->reserved_sectors;
    int root_dir_entires = fat_private->header.primary_header.root_dir_entries;
    int root_dir_size = root_dir_entires * sizeof(struct fat_directory_item);
    int total_sectors = root_dir_size / disk->sector_size;;
    
    if ((root_dir_size % disk->sector_size) > 0)
    {
        total_sectors += 1;
    }
    int total_items = fat16_get_total_items_for_dir(disk, root_dir_sector_pos);

    struct fat_directory_item* dir = kernel_zero_alloc(root_dir_size);
    if (!dir)
    {
        res = -MYOS_ERROR_NO_MEMORY;
        goto out;
    }

    struct disk_streamer* stream = fat_private->directory_stream;
    if (disk_stream_seek(stream, fat16_sector_to_absolute(disk, root_dir_sector_pos)) < 0)
    {
        res = -MYOS_IO_ERROR;
        goto out;
    }

    if (disk_stream_read(stream, dir, root_dir_size) < 0)
    {
        res = -MYOS_IO_ERROR;
        goto out;
    }
    out_directory->item = dir;
    out_directory->total = total_items;
    out_directory->sector_pos = root_dir_sector_pos;
    //out_directory->ending_sector_pos = root_dir_sector_pos + (root_dir_size / disk->sector_size);
    out_directory->ending_sector_pos = root_dir_sector_pos + total_sectors;

out:
    return res;
}

int fat16_resolve(struct disk* disk)
{   
    int res = 0;
    struct fat_private* fat_private = kernel_zero_alloc(sizeof(struct fat_private));
    fat16_private_init(disk, fat_private);

    disk->fs_private_data = fat_private;
    struct disk_streamer* stream = create_disk_streamer(disk->id);
    if (disk_stream_read(stream, &fat_private->header, sizeof(struct fat_h)) < 0)
    {
        print("Error reading FAT header\n");
        res = -MYOS_IO_ERROR;
        goto out;
    }
    //if it doesn't match the signature, we don't support it
    if (fat_private->header.shared.extended_header.signature != 0x29)
    {
        print("Unsupported filesystem signature\n");
        res = -MYOS_ERROR_FILESYSTEM_NOT_SUPPORTED;
        goto out;
    }

    if (fat16_get_root_directory(disk, fat_private, &fat_private->root_directory) < 0)
    {
        print("Error loading root directory\n");
        res = -MYOS_IO_ERROR;
        goto out;
    }
    disk->filesystem = &fat16_filesystem;

out:
    if (stream)
    {
        destroy_disk_streamer(stream);
    }

    if (res < 0)
    {
        kernel_free(fat_private);
        disk->fs_private_data = NULL;
    }
    return res;
}

//we consider just only for READONLY in moment
void* fat16_open(struct disk* disk, struct path_part* path, FILE_MODE mode)
{
    if (mode != FILE_MODE_READ)
    {
        return NULL;
    }
    struct fat_file_descriptor *desc = kernel_zero_alloc(sizeof(struct fat_file_descriptor));
    if (!desc)
    {
        return ERROR(-MYOS_ERROR_NO_MEMORY);
    }
    desc->item = fat16_get_dir_entry(disk, path);
    if (desc->item == NULL)
    {
        kernel_free(desc);
        print("Error finiding....\n");
        return ERROR(-MYOS_FILE_NOT_FOUND);
    }
    desc->pos = 0;
    return desc;
}

struct fat_item *fat16_get_dir_entry(struct disk *disk, struct path_part *path)
{
    if (!disk || !path)
    {
        return ERROR(-MYOS_ERROR_NO_MEMORY);
    }
    struct fat_private* fat_private = disk->fs_private_data;
    struct fat_item* cur_item = 0;
    
    // 0:/test.txt -> fat16_find_item_in_dir will be returend test.txt
    // 0:/abc/test.txt -> it will be returned abc 
    // error !
    if (!(&fat_private->root_directory))
    {
        goto out;
    }
    struct fat_item* root_item = fat16_find_item_in_dir(disk, &fat_private->root_directory, path->part);
    if (root_item == NULL)
    {
        print("Error finding root item\n");
        goto out;
    }
    struct path_part* next_part = path->next;
    cur_item = root_item;
    while (next_part != NULL)
    {
        //directorty일경우 계속 loop.
        if (cur_item->type != FAT_ITEM_TYPE_DIRECTORY)
        {
            cur_item = NULL;
            break;
        }
        struct fat_item* next_item = fat16_find_item_in_dir(disk, cur_item->directory, next_part->part);
        fat16_item_free(cur_item);
        cur_item = next_item;
        next_part = next_part->next;
    }
out:
    return cur_item;
}

// find specific item in directory.
// item could be file or directory....
struct fat_item *fat16_find_item_in_dir(struct disk *disk, struct fat_directory *directory, const char *name)
{
    int i = 0;
    struct fat_item *item = NULL;
    char tempfile[MYOS_MAX_PATH_LENGTH] = {0};
    while (i < directory->total)
    {
        // Convert 8.3 entry to string
        format_83_to_string(&directory->item[i], tempfile, sizeof(tempfile));


        if (ft_istrncmp(tempfile, name, ft_strlen(name)) == 0)
        {
            print("Found matching item: ");
            item = fat16_new_item_from_directory_item(disk, &directory->item[i]);
            break;
        }
        i++;
    }
    return item;
}

static struct fat_item *fat16_new_item_from_directory_item(struct disk *disk, struct fat_directory_item *directory_item)
{
    struct fat_item* f_item = kernel_zero_alloc(sizeof(struct fat_item));
    if (!f_item)
    {
        return NULL;
    }
    //determine type
    if (directory_item->attribute & FAT_FILE_SUBDIRECTORY)
    {
        f_item->type = FAT_ITEM_TYPE_DIRECTORY;
        f_item->directory = fat16_load_for_sub_directory(disk, directory_item);
    }
    else
    {
        f_item->type = FAT_ITEM_TYPE_FILE;
        f_item->item = fat16_clone_dir_item(directory_item, sizeof(struct fat_directory_item));
    }
    return f_item; 
}

struct fat_directory_item *fat16_clone_dir_item(struct fat_directory_item *src, size_t size)
{
    struct fat_directory_item* new_item_copy = kernel_zero_alloc(size);
    if (!new_item_copy)
    {
        return NULL;
    }
    ft_memcpy(new_item_copy, src, size);
    return new_item_copy;
}

void format_83_to_string(struct fat_directory_item* item, char *out, int max)
{
    int i = 0;
    int j = 0;
    //filename
    for (i = 0; i < 8 && item->filename[i] != ' ' && j < max - 1; i++)
    {
        out[j++] = item->filename[i];
    }
    //extension
    if (item->ext[0] != ' ')
    {
        if (j < max - 1)
        {
            out[j++] = '.';
        }
        for (i = 0; i < 3 && item->ext[i] != ' ' && j < max - 1; i++)
        {
            out[j++] = item->ext[i];
        }
    }
    out[j] = '\0';
}

//[FAT 테이블] - [루트 디렉터리 영역] - [데이터 영역 (클러스터 2)]

static int fat16_get_cluster_chain_link(struct disk* disk, int cur_cluster, int cluster_offset)
{
    int next_cluster = cur_cluster;

    for (int i = 0; i < cluster_offset; i++)
    {
        // 1. 기본 함수(단일 조회)를 호출하여 다음 클러스터 번호를 얻어옴
        next_cluster = fat16_get_next_cluster(disk, next_cluster);

        // 2. 에러 또는 EOF 체크
        // 음수 값은 I/O 에러를 의미합니다.
        if (next_cluster < 0)
        {
            return next_cluster; // I/O 에러 반환
        }
        // 목표 횟수를 채우기 전에 파일의 끝을 만나면 즉시 반환
        if (next_cluster >= FAT_EOF_MARKER)
        {
            return next_cluster; // EOF 마커 반환
        }
    }
    // cluster_offset만큼 이동하여 찾은 최종 클러스터 번호를 반환
    return next_cluster;
}

static int fat16_get_next_cluster(struct disk* disk, int cur_cluster)
{
    struct fat_private* fat_private = disk->fs_private_data;
    struct disk_streamer* stream = fat_private->fat_read_stream; 
    
    // 1. FAT 테이블 내에서 해당 클러스터 항목의 절대 위치 계산 (바이트 단위)
    // FAT 테이블 시작 섹터 * 섹터 크기 + (현재 클러스터 번호 * FAT16 항목 크기 2바이트)
    // fat_start_sector_pos 변수를 사용한다고 가정
    uint32_t fat_start_sector = fat_private->header.primary_header.reserved_sectors;
    uint32_t fat_entry_pos = fat_start_sector * disk->sector_size + (cur_cluster * 2);
    
    // 2. 스트리머 위치 설정 (Seek)
    int res = disk_stream_seek(stream, fat_entry_pos);
    if (res < 0)
        return res;

    uint16_t next_cluster_val;
    
    // 3. 16비트(2바이트) 값 읽기 (Read)
    res = disk_stream_read(stream, &next_cluster_val, sizeof(next_cluster_val));
    if (res < 0)
        return res;
    
    // 4. FAT 항목 값 반환
    return (int)next_cluster_val;
    
}

static int fat16_get_first_cluster(struct fat_directory_item* item)
{
    return item->low_16_bits_first_cluster | (item->high_16_bits_first_cluster << 16);
}

//ending sector_pos + (cluster -2 * secctors per cluster) why?
//because data area starts from ending sector of root dir area 
//but why cluster -2? : because cluster 0,1 are reserved for FAT16 specification.
static int fat16_get_sector_from_cluster(struct fat_private* private, int cluster)
{
    return private->root_directory.ending_sector_pos + ((cluster - 2) * private->header.primary_header.sectors_per_cluster);
}

//get total cluster counts;
static int fat16_get_total_cluster_counts(struct disk* disk, struct fat_directory_item* item)
{
    int res = 0;
    //struct fat_private* fat_private = disk->fs_private_data;
    int cluster = fat16_get_first_cluster(item);
    int cur_cluster = cluster;
    int total_clusters = 0;
    //int cluster_size = fat16_get_cluster_size(disk, fat_private);

    while (cur_cluster < FAT_EOF_MARKER)
    {
        total_clusters++;
        cur_cluster = fat16_get_next_cluster(disk, cur_cluster);
        //if it is - and not EOF Marker
        if (cur_cluster < 0 && cur_cluster != FAT_EOF_MARKER) 
        {
            res = -MYOS_IO_ERROR;
            goto out;
        }
    }
    res = total_clusters;

out:
    return res;
}

//read from specific cluster with offset and size
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

static int fat16_get_cluster_size(struct disk* disk,struct fat_private* private)
{
    // 섹터당 바이트 수 * 클러스터당 섹터 수
    return disk->sector_size * private->header.primary_header.sectors_per_cluster;
}

struct fat_directory* fat16_load_for_sub_directory(struct disk* disk, struct fat_directory_item* item)
{
    int res = 0;
    struct fat_directory *dir;
    struct fat_private* fat_private = disk->fs_private_data;
    if (!(item->attribute & FAT_FILE_SUBDIRECTORY))
    {
        return ERROR(-MYOS_ERROR_NO_MEMORY);
    }

    dir = kernel_zero_alloc(sizeof(struct fat_directory));
    if (!dir)
    {
        res = -MYOS_ERROR_NO_MEMORY;
        goto out;
    }
    int cluster = fat16_get_first_cluster(item);
    int total_cluster_counts = fat16_get_total_cluster_counts(disk, item);
    int dir_size = total_cluster_counts * fat16_get_cluster_size(disk, fat_private);

    dir->item = kernel_zero_alloc(dir_size);
    if (!dir->item)
    {
        res = -MYOS_ERROR_NO_MEMORY;
        goto out;
    }
    res = fat16_read_internal(disk, cluster, 0x00, dir_size, dir->item);
    if (res < 0)
    {
        // 오류 발생 시 할당된 메모리 해제
        goto out;
    }
    int valid_items = 0;
    int item_capacity = dir_size / sizeof(struct fat_directory_item);
    for (int i = 0; i < item_capacity; i++)
    {
        if (dir->item[i].filename[0] == 0x00)
        {
            break;
        }
        if (dir->item[i].filename[0] != 0xE5)
        {
            valid_items++;
        }
    }
    dir->total = valid_items;

out:
    if (ISERR(res))
    {
        if (dir)
            fat16_free_dir(dir);
        return ERROR(res);
    }
    return dir;
}

void fat16_free_dir(struct fat_directory* directory)
{
    if (!directory)
    {
        return;
    }

    if (directory->item)
    {
        kernel_free(directory->item);
    }

    kernel_free(directory);
}

void fat16_item_free(struct fat_item* item)
{
    if (item->type == FAT_ITEM_TYPE_DIRECTORY)
    {
        fat16_free_dir(item->directory);
    }
    else if(item->type == FAT_ITEM_TYPE_FILE)
    {
        kernel_free(item->item);
    }

    kernel_free(item);
}