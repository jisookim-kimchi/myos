#include "file.h"
#include "fat16.h"
#include "../config.h"
#include "../memory/memory.h"
#include "../kernel.h"
#include "disk/disk.h"


struct filesystem* filesystems[MYOS_MAX_FILESYSTEMS];
struct file_descriptor* file_descriptors[MYOS_MAX_FILE_DESCRIPTORS];

static struct filesystem** get_free_filesystem()
{
    int i = 0;

    while(i < MYOS_MAX_FILESYSTEMS)
    {
        if (filesystems[i] == NULL)
        {
            return &filesystems[i];
        }
        i++;
    }
    return NULL;
}

void file_system_insert(struct filesystem *fs)
{
    struct filesystem **filesystems = get_free_filesystem();
    if (filesystems == NULL)
    {
        print("No space to insert filesystem!\n");
        return ;
    }
    *filesystems = fs;
}

static void file_system_static_load()
{
    file_system_insert(fat16_init());
}

void file_system_load()
{
    ft_memset(filesystems, 0, sizeof(filesystems));
    file_system_static_load();
}

void file_system_init()
{
    ft_memset(file_descriptors, 0, sizeof(file_descriptors));
    file_system_load();
}

static int make_new_fd(struct file_descriptor** out_fd)
{
    int res = -MYOS_NO_FREE_FILE_DESCRIPTOR;
    for (int i = 0; i < MYOS_MAX_FILE_DESCRIPTORS; i++)
    {
        if (file_descriptors[i] == NULL)
        {
            struct file_descriptor* fd = kernel_zero_alloc(sizeof(struct file_descriptor));
            //Descriptors start at 1
            fd->index = i + 1;
            file_descriptors[i] = fd;
            *out_fd = fd;
            res = 0;
            break;
        }
    }
    return res;
}

static struct file_descriptor* get_file_descriptor(int fd)
{
    if (fd <= 0 || fd > MYOS_MAX_FILE_DESCRIPTORS)
    {
        return NULL;
    }

    // Descriptors start at 1
    int i = fd - 1;
    return file_descriptors[i];
}

struct filesystem* file_system_resolve(struct disk* disk)
{
    struct filesystem* fs = NULL;
    for (int i = 0; i < MYOS_MAX_FILESYSTEMS; i++)
    {
        if (filesystems[i] != NULL && filesystems[i]->resolve(disk) == 0)
        {
            fs = filesystems[i];
            break;
        }
    }
    return fs;
}

FILE_MODE get_filemode(const char *str)
{
    FILE_MODE mode = FILE_MODE_INVALID;
    if (!str)
    {
        mode = FILE_MODE_INVALID;
        return mode;
    }
    if (ft_strcmp(str, "r") == 0)
    {
        mode =FILE_MODE_READ;
    }
    else if (ft_strcmp(str, "w") == 0)
    {
        mode = FILE_MODE_WRITE;
    }
    else if (ft_strcmp(str, "a") == 0)
    {
        mode = FILE_MODE_APPEND;
    }

    return mode;
}

int fopen(const char* filename, const char* mode)
{
    int res = 0;
    struct path_root* root_path = parse_path(filename, NULL);
    if (!root_path)
    {
        res = -MYOS_INVALID_ARG;
        print("0\n");
        goto out;
    }

    // 0:/test.txt
    if (!root_path->first)
    {
        res = -MYOS_INVALID_ARG;
        print("1\n");
        goto out;
    }
    struct disk *d = get_disk(root_path->drive_number);
    if (!d)
    {
        res = -MYOS_IO_ERROR;
        print("2\n");
        goto out;
    }
    if (!d->filesystem)
    {
        res = -MYOS_IO_ERROR;
        goto out;
    }

    FILE_MODE filemode = get_filemode(mode);
    if (filemode == FILE_MODE_INVALID)
    {
        res = -MYOS_IO_ERROR;
        print("3\n");
        goto out;
    }
    
    void *fd_private_data = d->filesystem->open(d, root_path->first, filemode);
    if (ISERR(fd_private_data))
    {
        res = ERROR_I(fd_private_data);
        print("4\n");
        goto out;
    }
    struct file_descriptor* desc = 0;

    res = make_new_fd(&desc);
    if (res < 0)
    {
        print("5\n");
        goto out;

    }
    desc->filesystem = d->filesystem;
    desc->private = fd_private_data;
    desc->disk = d;
    desc->pos = 0;
    res = desc->index;

out:
    // fopen shouldnt return negative values
    if (res < 0)
    {
        res = 0;
    }

    return res;
}

int fread(int fd, void *buffer, unsigned int size, unsigned int nmemb)
{
    int res = 0;
    if (size == 0 || nmemb == 0 || fd < 1)
    {
        res = MYOS_INVALID_ARG;
        goto out;
    }
    struct file_descriptor* desc = get_file_descriptor(fd);
    if (!desc)
    {
        res = -MYOS_INVALID_ARG;
        goto out;
    }

    res = desc->filesystem->read(desc->disk, desc->pos, desc->private, size, nmemb, buffer);

out:
    return res;
}