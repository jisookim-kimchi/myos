#include "../io/io.h"
#include "disk.h"
#include "../memory/memory.h"
#include "../config.h"
#include "../status.h"

disk_t disk;

/*
0x1F7 : ATA port status register
disk Controller need ~400 nanosecond delay to update status register
0x1F7 port read delay ~100 nanosecond
so 4 times is enough.
*/
static void disk_400ns_delay()
{
  for (int i = 0; i < 4; i++)
    insb(0x1F7);
}

/*
0x1F7 : Port Status Register
0x80 : BSY (Busy) bit
to prevent the OS down if the disk controller fails
or the hardware is disconnected.
*/
static int disk_wait_for_ready()
{
  int timeout = 1000000;
  while ((insb(0x1F7) & 0x80) && --timeout > 0)
    ;
  
  if (timeout <= 0)
    return -MYOS_IO_TIMEOUT;
  return 0;
}

/*
0x1F7 : Port Status register
0x80 : BSY (Busy) bit
0x08 : Data Request DRQ if 1 then ready to read and write
0x01 : Error
*/
static int disk_wait_for_drq()
{
  unsigned char status;
  int timeout = 1000000;
  while (--timeout > 0)
  {
    status = insb(0x1F7);
    if (!(status & 0x80) && (status & 0x08))
      break;
    if (status & 0x01)
      return -MYOS_IO_ERROR;
  }
  if (timeout <= 0)
    return -MYOS_IO_TIMEOUT;
  return 0;
}

/*
    LBA : Logical Block Addressing give number of sector to read and write for the disk
    reads sectors from the disk using LBA, accessing it as a linear sequence of 512-byte blocks.
    cpu->ATA port (0x1F0 ~ 0x1F7) use. 
    lba sector number, total number of sectors to read, buffer to store data

*/
int disk_read_sectors(int lba, int total, void *buffer)
{
    int res = disk_wait_for_ready();
    if (res < 0)
        return res;

    outsb(0x1F6, (lba >> 24) | 0x40 | 0xE0); // 0x40 : lba mode, 0xE0 : Master drive selecting
    outsb(0x1F2, total); // total number of sectors to read
    outsb(0x1F3, (unsigned char)(lba & 0xFF)); // 24 low bits of lba
    outsb(0x1F4, (unsigned char)((lba >> 8))); // 24 low bits of lba
    outsb(0x1F5, (unsigned char)((lba >> 16))); // 24 low bits of lba
    outsb(0x1F7, 0x20); // read command

    disk_400ns_delay();

    unsigned short *ptr = (unsigned short *)buffer;
    for (int i = 0; i < total; i++)
    {
        res = disk_wait_for_drq();
        if (res < 0)
            return res;
        for (int j = 0; j < 256; j++)
        {
            *ptr = insw(0x1F0); // 0x1F0 is the Data Register, used to read the actual data stored on the ard disk.
            ptr++;
        }
    }
    return 0;
}

int disk_write_sectors(int lba, int total, void *buffer)
{
    int res = disk_wait_for_ready();
    if (res < 0)
        return res;

    outsb(0x1F6, (lba >> 24) | 0x40 | 0xE0); // 0x40 for LBA bit
    outsb(0x1F2, total);
    outsb(0x1F3, (unsigned char)(lba & 0xFF));
    outsb(0x1F4, (unsigned char)((lba >> 8)));
    outsb(0x1F5, (unsigned char)((lba >> 16)));
    outsb(0x1F7, 0x30);

    disk_400ns_delay();

    unsigned short *ptr = (unsigned short *)buffer;
    for (int i = 0; i < total; i++)
    {
        res = disk_wait_for_drq();
        if (res < 0)
            return res;
        for (int j = 0; j < 256; j++)
        {
            outsw(0x1F0, *ptr);
            ptr++;
        }
    }
    return 0;
}

void disk_search_and_init()
{
    ft_memset(&disk, 0, sizeof(disk_t));
    disk.type = REAL_DISK_TYPE;
    disk.sector_size = MYOS_SECTOR_SIZE;
    disk.filesystem = file_system_resolve(&disk);
    disk.id = 0;
}

disk_t* get_disk(int index)
{
    if (index != 0)
    {
        return NULL;
    }
    return &disk;
}

/*
    Abstraction, Wrapper function for reading blocks from disk
*/
int disk_read_block(disk_t *idisk, unsigned int lba, unsigned int total, void *buffer)
{
    if (idisk != &disk)
    {
        return -MYOS_IO_ERROR;
    }
    return disk_read_sectors(lba, total, buffer);
}

/*
    Abstraction, Wrapper function for writing blocks to disk
*/
int disk_write_block(disk_t *idisk, unsigned int lba, unsigned int total, void *buffer)
{
    if (idisk != &disk)
    {
        return -MYOS_IO_ERROR;
    }
    return disk_write_sectors(lba, total, buffer);
}