#include "fat16.h"
#include "../memory/memory.h"
#include "../disk/disk.h"
#include "../disk/streamer.h"
#include "../status.h"
#include "../kernel_print.h"


int fat16_free_cluster_chain(struct disk* disk, uint32_t cluster)
{
    while (cluster != 0 && cluster < 0xFFF8)
    {
        uint32_t next = fat16_get_next_cluster(disk, cluster);
        fat16_set_next_cluster(disk, cluster, FAT16_UNUSED);
        cluster = next;
    }
    return 0;
}


// small debug helper: print a single byte as two hex chars
void fat16_print_hex_byte(unsigned char b)
{
    char h[3] = {0};
    const char *hex = "0123456789ABCDEF";
    h[0] = hex[(b >> 4) & 0xF];
    h[1] = hex[b & 0xF];
    print(h);
}

int fat16_get_first_cluster(struct fat_directory_item* item)
{
    return item->low_16_bits_first_cluster | (item->high_16_bits_first_cluster << 16);
}

int fat16_get_cluster_size(struct disk* disk, struct fat_private* private)
{
    return disk->sector_size * private->header.primary_header.sectors_per_cluster;
}

int fat16_get_sector_from_cluster(struct fat_private* private, int cluster)
{
    return private->root_directory.ending_sector_pos + ((cluster - 2) * private->header.primary_header.sectors_per_cluster);
}

int fat16_sector_to_absolute(struct disk *disk, int root_dir_sector_pos)
{
    return root_dir_sector_pos * (disk->sector_size);
}

int fat16_get_next_cluster(struct disk* disk, int cur_cluster)
{
    struct fat_private* fat_private = disk->fs_private_data;
    struct disk_streamer* stream = fat_private->fat_read_stream; 
    
    uint32_t fat_start_sector = fat_private->header.primary_header.reserved_sectors;
    uint32_t fat_entry_pos = fat_start_sector * disk->sector_size + (cur_cluster * 2);
    
    int res = disk_stream_seek(stream, fat_entry_pos);
    if (res < 0)
        return res;

    uint16_t next_cluster_val;
    res = disk_stream_read(stream, &next_cluster_val, sizeof(next_cluster_val));
    if (res < 0)
        return res;
    
    return (int)next_cluster_val;
}

int fat16_set_next_cluster(struct disk *disk, int cluster, int next_cluster)
{
    struct fat_private* fat_private = disk->fs_private_data;
    struct disk_streamer* stream = fat_private->fat_read_stream; 

    int fat_start_sector = fat_private->header.primary_header.reserved_sectors;
    int fat_entry_pos = fat_start_sector * disk->sector_size + (cluster * 2);

    if (disk_stream_seek(stream, fat_entry_pos) < 0)
        return -MYOS_IO_ERROR;

    uint16_t val = (uint16_t)next_cluster;
    if (disk_stream_write(stream, &val, sizeof(val)) < 0)
        return -MYOS_IO_ERROR;
    return 0;
}

int fat16_get_cluster_chain_link(struct disk* disk, int cur_cluster, int cluster_offset)
{
    int next_cluster = cur_cluster;
    for (int i = 0; i < cluster_offset; i++)
    {
        next_cluster = fat16_get_next_cluster(disk, next_cluster);
        if (next_cluster < 0)
            return next_cluster;
        if (next_cluster >= FAT_EOF_MARKER)
            return next_cluster;
    }
    return next_cluster;
}

int fat16_find_free_cluster(struct disk* disk)
{
    struct fat_private* fat_private = disk->fs_private_data;
    int sectors_per_fat = fat_private->header.primary_header.sectors_per_fat;
    int total_clusters = (sectors_per_fat * disk->sector_size) / 2;
    for (int i = 2; i < total_clusters; i++)
    {
        int entry_val = fat16_get_next_cluster(disk, i);
        if (entry_val == 0x0000)
        {
            return i;
        }
    }
    return -MYOS_IO_ERROR;
}

void fat16_to_dos_filename(const char* fname, uint8_t* out_name, uint8_t* out_ext)
{
    ft_memset(out_name, ' ', 8);
    ft_memset(out_ext, ' ', 3);
    const char* p = fname;
    int i = 0;
    for (i = 0; i < 8 && *p && *p != '.'; i++, p++)
    {
        char c = *p;
        if (c >= 'a' && c <= 'z') c -= 32;
        out_name[i] = (uint8_t)c;
    }
    if (*p == '.')
    {
        p++;
        for (i = 0; i < 3 && *p; i++, p++)
        {
            char c = *p;
            if (c >= 'a' && c <= 'z') c -= 32;
            out_ext[i] = (uint8_t)c;
        }
    }
}

int fat16_get_total_cluster_counts(struct disk* disk, struct fat_directory_item* item)
{
    int res = 0;
    int cluster = fat16_get_first_cluster(item);
    int cur_cluster = cluster;
    int total_clusters = 0;

    while (cur_cluster < FAT_EOF_MARKER)
    {
        total_clusters++;
        cur_cluster = fat16_get_next_cluster(disk, cur_cluster);
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