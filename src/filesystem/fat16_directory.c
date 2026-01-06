#include "fat16.h"
#include "../string/string.h"
#include "../disk/disk.h"
#include "../disk/streamer.h"
#include "../status.h"
#include "../memory/heap/kernel_heap.h"
#include "../memory/memory.h"
#include "../config.h"


int fat16_update_directory_entry(struct disk *disk, struct fat_file_descriptor *file_descriptor, int bytes_written)
{
    struct fat_item* item = file_descriptor->item;
    if (!item)
        return -MYOS_IO_ERROR;

    struct fat_private* private = disk->fs_private_data;
    struct disk_streamer* stream = private->directory_stream;
    if (!stream)
        return -MYOS_IO_ERROR;
    
    // 이전에 저장해둔 위치(섹터 번호 * 섹터 크기 + 오프셋)로 이동
    unsigned int pos = item->item_entry_sector * disk->sector_size + item->item_entry_offset;
    int res = disk_stream_seek(stream, pos);
    if (res < 0)
        return res;

    // 메모리 상의 수정된 디렉터리 아이템(32바이트)을 디스크에 덮어씀
    res = disk_stream_write(stream, item->dir_item, sizeof(struct fat_directory_item));
    return res;
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
            // We are done
            break;
        }

        // Is the item unused
        if (item.filename[0] == 0xE5)
        {
            continue;
        }

        i++;
    }

    res = i;
    
out:
    return res;
}

/*
    disk->sector_size = 512bytes
*/
int fat16_get_root_directory(struct disk* disk, struct fat_private* fat_private, struct fat_directory* out_directory)
{
    int res = 0;

    struct fat_header* primary_header = &fat_private->header.primary_header;
    int root_dir_sector_pos = (primary_header->number_of_fats * primary_header->sectors_per_fat) + primary_header->reserved_sectors;
    int root_dir_entires = fat_private->header.primary_header.root_dir_entries;
    int root_dir_size = root_dir_entires * sizeof(struct fat_directory_item);
    int total_sectors = root_dir_size / disk->sector_size;
    
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
    out_directory->ending_sector_pos = root_dir_sector_pos + total_sectors;

out:
    return res;
}

struct fat_item *fat16_get_dir_entry(struct disk *disk, struct path_part *path)
{
    if (!disk || !path)
    {
        return ERROR(-MYOS_ERROR_NO_MEMORY);
    }
    struct fat_private* fat_private = disk->fs_private_data;
    struct fat_item* cur_item = 0;
    
    if (!(&fat_private->root_directory))
    {
        goto out;
    }
    struct fat_item* root_item = fat16_find_item_in_dir(disk, &fat_private->root_directory, path->part);
    if (root_item == NULL)
    {
        goto out;
    }
    struct path_part* next_part = path->next;
    cur_item = root_item;
    while (next_part != NULL)
    {
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

struct fat_item *fat16_find_item_in_dir(struct disk *disk, struct fat_directory *directory, const char *name)
{
    int i = 0;
    struct fat_item *item = NULL;
    char tempfile[MYOS_MAX_PATH_LENGTH] = {0};
    
    struct fat_private* fat_private = disk->fs_private_data;
    int max_entries = fat_private->header.primary_header.root_dir_entries;
    
    while (i < max_entries)
    {   
        if (directory->item[i].filename[0] == 0x00)
        {
            break;
        }
        
        if (directory->item[i].attribute == 0x0F)
        {
            i++;
            continue;
        }
        
        if (directory->item[i].filename[0] == 0xE5)
        {
            i++;
            continue;
        }
        
        format_83_to_string(&directory->item[i], tempfile, sizeof(tempfile));

        if (ft_istrncmp(tempfile, name, ft_strlen(name)) == 0)
        {
            item = fat16_new_item_from_directory_item(disk, &directory->item[i]);
            item->item_entry_sector = directory->sector_pos + (i * sizeof(struct fat_directory_item)) / disk->sector_size;
            item->item_entry_offset = (i * sizeof(struct fat_directory_item)) % disk->sector_size;
            break;
        }
        i++;
    }
    return item;
}

struct fat_item *fat16_new_item_from_directory_item(struct disk *disk, struct fat_directory_item *directory_item)
{
    struct fat_item* f_item = kernel_zero_alloc(sizeof(struct fat_item));
    if (!f_item)
    {
        return NULL;
    }
    if (directory_item->attribute & FAT_FILE_SUBDIRECTORY)
    {
        f_item->type = FAT_ITEM_TYPE_DIRECTORY;
        f_item->directory = fat16_load_for_sub_directory(disk, directory_item);
    }
    else
    {
        f_item->type = FAT_ITEM_TYPE_FILE;
        f_item->dir_item = fat16_clone_dir_item(directory_item, sizeof(struct fat_directory_item));
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
    for (i = 0; i < 8 && item->filename[i] != ' ' && j < max - 1; i++)
    {
        out[j++] = item->filename[i];
    }
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
        kernel_free(item->dir_item);
    }

    kernel_free(item);
}