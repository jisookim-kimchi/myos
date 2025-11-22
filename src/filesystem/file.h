#ifndef FILE_H
#define FILE_H

#include "pathparser.h"

typedef unsigned int FILE_SEEK_MODE;

enum
{
    FILE_SEEK_SET = 0,
    FILE_SEEK_CUR = 1,
    FILE_SEEK_END = 2
};

typedef unsigned int FILE_MODE;

enum
{
    FILE_MODE_READ,
    FILE_MODE_WRITE,
    FILE_MODE_APPEND,
    FILE_MODE_INVALID,
};

struct disk;
typedef void*(*FS_OPEN_FUNCTION)(struct disk* disk, struct path_part* path, FILE_MODE mode);
typedef int (*FS_RESOLVE_FUNCTION)(struct disk* disk); //check if the disk validates this filesystem

typedef struct filesystem
{
    // Filesystem should return zero from resolve if the provided disk is using its filesystem
    FS_RESOLVE_FUNCTION resolve;
    FS_OPEN_FUNCTION open;
    char name[20];
} filesystem_t;

typedef struct file_descriptor
{
    struct filesystem* filesystem;
    int index;
    void* private;
    struct disk* disk;
} file_descriptor_t;

void file_system_init();
int fopen(const char* filename, const char* mode);
void file_system_insert(struct filesystem* fs);
struct filesystem* file_system_resolve(struct disk* disk);
FILE_MODE get_filemode(const char *str);

#endif