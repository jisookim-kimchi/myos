#include "file.h"
#include "../config.h"
#include "../kernel.h"
#include "../memory/heap/kernel_heap.h"
#include "../memory/memory.h"
#include "../status.h"
#include "../string/string.h"
#include "disk/disk.h"
#include "fat16.h"

struct filesystem *filesystems[MYOS_MAX_FILESYSTEMS];
struct file_descriptor *file_descriptors[MYOS_MAX_FILE_DESCRIPTORS];

static struct filesystem **get_free_filesystem()
{
  int i = 0;

  while (i < MYOS_MAX_FILESYSTEMS)
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
    return;
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

static int make_new_fd(struct file_descriptor **out_fd)
{
  int res = -MYOS_NO_FREE_FILE_DESCRIPTOR;
  for (int i = 0; i < MYOS_MAX_FILE_DESCRIPTORS; i++)
  {
    if (file_descriptors[i] == NULL)
    {
      struct file_descriptor *fd =
          kernel_zero_alloc(sizeof(struct file_descriptor));
      // Descriptors start at 1
      fd->index = i + 1;
      file_descriptors[i] = fd;
      *out_fd = fd;
      res = 0;
      break;
    }
  }
  return res;
}

static struct file_descriptor *get_file_descriptor(int fd)
{
  if (fd < 0 || fd > MYOS_MAX_FILE_DESCRIPTORS)
  {
    print("Invalid file descriptor in file.c get_file_descriptor()\n");
    return NULL;
  }

  // Descriptors start at 1
  int i = fd - 1;
  return file_descriptors[i];
}

struct filesystem *file_system_resolve(struct disk *disk)
{
  struct filesystem *fs = NULL;
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

int file_system_unresolve(struct disk *disk)
{
  return disk->filesystem->unresolve(disk);
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
    mode = FILE_MODE_READ;
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

int fopen(const char *filename, const char *mode)
{
  int res = 0;
  struct path_root *root_path = parse_path(filename, NULL);
  if (!root_path)
  {
    res = -MYOS_INVALID_ARG;
    goto out;
  }

  // 0:/test.txt
  if (!root_path->first)
  {
    res = -MYOS_INVALID_ARG;
    goto out;
  }
  struct disk *d = get_disk(root_path->drive_number);
  if (!d)
  {
    res = -MYOS_IO_ERROR;
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
    goto out;
  }

  void *fd_private_data = d->filesystem->open(d, root_path->first, filemode);
  if (ISERR(fd_private_data))
  {
    res = ERROR_I(fd_private_data);
    goto out;
  }
  struct file_descriptor *desc = NULL;

  res = make_new_fd(&desc);
  if (res < 0)
  {
    goto out;
  }
  desc->filesystem = d->filesystem;
  desc->private = fd_private_data;
  desc->disk = d;
  desc->pos = 0;
  res = desc->index;

out:
  return res;
}

// fd : file descriptor, buffer : output buffer, size : size of each element,
// nmemb :  how many times to read
int fread(int fd, void *buffer, unsigned int size, unsigned int nmemb)
{
  if (size == 0 || nmemb == 0 || fd < 1)
  {
    return -MYOS_INVALID_ARG;
  }

  struct file_descriptor *desc = get_file_descriptor(fd);
  if (!desc)
  {
    return -MYOS_INVALID_ARG;
  }

  int bytes = desc->filesystem->read(desc->disk, desc->pos, desc->private, size, nmemb, buffer);
  if (bytes > 0)
  {
    desc->pos += bytes;
    return bytes / size;
  }
  return bytes;
}

int fwrite(void *ptr, uint32_t size, uint32_t nmemb, int fd)
{
  int res = 0;
  if (size == 0 || nmemb == 0 || fd < 1)
  {
    return -MYOS_INVALID_ARG;
  }

  struct file_descriptor *desc = get_file_descriptor(fd);
  if (!desc)
  {
    return -MYOS_INVALID_ARG;
  }

  res = desc->filesystem->write(desc->disk, desc->private, size, nmemb, (char *)ptr);
  if (res < 0)
  {
    return res;
  }

  desc->pos += res;
  return res / size;
}

int fseek(int fd, int offset, FILE_SEEK_MODE mode)
{
  int res = 0;
  struct file_descriptor *desc = get_file_descriptor(fd);
  if (!desc)
  {
    res = -MYOS_INVALID_ARG;
    goto out;
  }

  res = desc->filesystem->seek(desc->private, offset, mode);
  if (res >= 0)
  {
    desc->pos = res;
    res = 0;
  }
out:
  return res;
}

int fstat(int fd, struct file_stat *stat)
{
  int res = 0;
  struct file_descriptor *desc = get_file_descriptor(fd);
  if (!desc)
  {
    print("fstat: invalid fd in file.c\n");
    res = -MYOS_IO_ERROR;
    goto out;
  }

  res = desc->filesystem->stat(desc->private, stat);
  if (res < 0)
  {
    print("fstat: stat failed in file.c\n");
    goto out;
  }
out:
  return res;
}

int fclose(int fd)
{
  int res = 0;
  struct file_descriptor *desc = get_file_descriptor(fd);
  if (!desc)
  {
    return -MYOS_INVALID_ARG;
  }
  res = desc->filesystem->close(desc->private);

  if (file_descriptors[fd - 1] == desc)
  {
    file_descriptors[fd - 1] = 0;
  }

  kernel_free(desc);
  return res;
}