#include "fat16.h"
#include "../string/string.h"
#include "../disk/disk.h"
#include "../disk/streamer.h"
#include "../status.h"
#include "../memory/heap/kernel_heap.h"
#include "../memory/memory.h"

//gloabl struct resolve, open, name
filesystem_t fat16_filesystem = 
{
    .resolve = fat16_resolve,
    .open = fat16_open,
    .read = fat16_read,
    .write = fat16_write,
    .seek = fat16_seek,
    .stat = fat16_stat,
    .close = fat16_close,
    .unresolve = fat16_unresolve,
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

int fat16_resolve(struct disk* disk)
{   
    int res = 0;
    struct fat_private* fat_private = kernel_zero_alloc(sizeof(struct fat_private));
    fat16_private_init(disk, fat_private);

    disk->fs_private_data = fat_private;
    struct disk_streamer* stream = create_disk_streamer(disk->id);
    if (disk_stream_read(stream, &fat_private->header, sizeof(struct fat_h)) < 0)
    {
        res = -MYOS_IO_ERROR;
        goto out;
    }
    //if it doesn't match the signature, we don't support it
    //weather no Extended Header or wrong FAT Volume
    if (fat_private->header.shared.extended_header.signature != 0x29)
    {
        res = -MYOS_ERROR_FILESYSTEM_NOT_SUPPORTED;
        goto out;
    }

    if (fat16_get_root_directory(disk, fat_private, &fat_private->root_directory) < 0)
    {
        res = -MYOS_IO_ERROR;
        goto out;
    }
    disk->filesystem = &fat16_filesystem;

out:
    if (res < 0)
    {
        fat16_unresolve(disk);
    }
    return res;
}


int fat16_unresolve(struct disk *disk)
{
    if (!disk || !disk->fs_private_data)
        return -MYOS_INVALID_ARG;
    struct fat_private *priv = disk->fs_private_data;
    /* ① 스트리머 해제 */
    if (priv->cluster_read_stream)
        destroy_disk_streamer(priv->cluster_read_stream);
    if (priv->fat_read_stream)
        destroy_disk_streamer(priv->fat_read_stream);
    if (priv->directory_stream)
        destroy_disk_streamer(priv->directory_stream);
    /* ② 루트 디렉터리 버퍼 해제 */
    if (priv->root_directory.item)
        kernel_free(priv->root_directory.item);
    /* ③ 구조체 자체 해제 */
    kernel_free(priv);
    disk->fs_private_data = NULL;
    return 0;   // 성공
}
