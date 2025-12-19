#ifndef DISK_H
#define DISK_H

typedef unsigned int MYOS_DISK_TYPE;

#define REAL_DISK_TYPE 0

#include "../filesystem/file.h"

typedef struct disk
{
    MYOS_DISK_TYPE type;
    int sector_size;
    struct filesystem* filesystem;
    uint32_t id;

    //private data for each disk implementation
    void *fs_private_data;
}   disk_t;

int disk_read_sectors(int lba, int total, void *buffer);
int disk_read_block(disk_t *idisk, unsigned int lba, unsigned int total, void *buffer);
int disk_write_sectors(int lba, int total, void *buffer);
int disk_write_block(disk_t *idisk, unsigned int lba, unsigned int total, void *buffer);
disk_t* get_disk(int index);
void disk_search_and_init();

#endif // DISK_H