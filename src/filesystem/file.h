#ifndef FILE_H
#define FILE_H

#include <stdint.h>
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

typedef unsigned int FILE_STAT_MODE;

#define FILE_STAT_READ_ONLY 0x01
#define FILE_STAT_HIDDEN 0x02
#define FILE_STAT_SYSTEM 0x04
#define FILE_STAT_VOLUME_LABEL 0x08
#define FILE_STAT_DIRECTORY 0x10
#define FILE_STAT_ARCHIVE 0x20

typedef struct file_stat
{
    uint32_t size;
    FILE_STAT_MODE mode;
    // uint32_t atime;
    // uint32_t mtime;
    // uint32_t ctime;
}file_stat_t;

typedef struct file_descriptor
{
    struct filesystem* filesystem;
    int index;
    void* private;
    struct disk* disk;
    uint32_t pos;
} file_descriptor_t;


struct disk;
typedef void*(*FS_OPEN_FUNCTION)(struct disk* disk, struct path_part* path, FILE_MODE mode);
typedef int (*FS_READ_FUNCTION)(struct disk* disk, uint32_t offset, void *private_data, uint32_t read_size, uint32_t nmemb, char *out);
typedef int (*FS_WRITE_FUNCTION)(struct disk* disk, void* private_data, uint32_t size, uint32_t nmemb, char *in);
typedef int (*FS_RESOLVE_FUNCTION)(struct disk* disk); //check if the disk validates this filesystem
typedef int (*FS_UNRESOLVE_FUNCTION)(struct disk* disk); //unresolve the filesystem
typedef int (*FS_SEEK_FUNCTION)(void *private, int offset, FILE_SEEK_MODE mode);
typedef int (*FS_STAT_FUNCTION)(void *private, struct file_stat *stat);
typedef int (*FS_CLOSE_FUNCTION)(void *private);

typedef struct filesystem
{
    // Filesystem should return zero from resolve if the provided disk is using its filesystem
    FS_RESOLVE_FUNCTION resolve;
    FS_OPEN_FUNCTION open;
    FS_READ_FUNCTION read;
    FS_WRITE_FUNCTION write;
    FS_UNRESOLVE_FUNCTION unresolve;
    FS_SEEK_FUNCTION seek;
    FS_STAT_FUNCTION stat;
    FS_CLOSE_FUNCTION close;
    char name[20];
} filesystem_t;

void file_system_init();
int fopen(const char* filename, const char* mode);
int fread(int fd, void *buffer, unsigned int size, unsigned int nmemb);
int fwrite(void *ptr, uint32_t size, uint32_t nmemb, int fd);
void file_system_insert(struct filesystem* fs);
struct filesystem* file_system_resolve(struct disk* disk);
FILE_MODE get_filemode(const char *str);
int file_system_unresolve(struct disk* disk);
int fseek(int fd, int offset, FILE_SEEK_MODE mode);
int fstat(int fd,  struct file_stat* stat);
int fclose(int fd);

#endif