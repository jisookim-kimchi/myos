#include "../io/io.h"
#include "disk.h"
#include "../memory/memory.h"
#include "../config.h"
#include "../status.h"

disk_t disk;


static void disk_400ns_delay()
{
  for (int i = 0; i < 4; i++)
    insb(0x1F7);
}

static int disk_wait_for_ready()
{
  int timeout = 1000000;
  while ((insb(0x1F7) & 0x80) && --timeout > 0)
    ;
  
  if (timeout <= 0)
    return -MYOS_IO_TIMEOUT;
  return 0;
}

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

//LBA : Logical Block Addressing give number of sector to read and write for the disk
//logical block addressing 방식으로 디스크에서 섹터를 읽는 함수 연속적인 비트 512바이트 단위 번호로 접근.
// cpu->ATA 포트 (0x1F0 ~ 0x1F7) 사용. -> 디스크 컨트롤러 
//lba sector number, total number of sectors to read, buffer to store data
int disk_read_sectors(int lba, int total, void *buffer)
{
    int res = disk_wait_for_ready();
    if (res < 0)
        return res;

    outsb(0x1F6, (lba >> 24) | 0x40 | 0xE0); // 0x40 for LBA bit
    outsb(0x1F2, total);
    outsb(0x1F3, (unsigned char)(lba & 0xFF));
    outsb(0x1F4, (unsigned char)((lba >> 8)));
    outsb(0x1F5, (unsigned char)((lba >> 16)));
    outsb(0x1F7, 0x20);

    disk_400ns_delay();

    unsigned short *ptr = (unsigned short *)buffer;
    for (int i = 0; i < total; i++)
    {
        res = disk_wait_for_drq();
        if (res < 0)
            return res;
        for (int j = 0; j < 256; j++)
        {
            *ptr = insw(0x1F0);
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
    disk.sector_size = MYOS_SECTOR_SIZE; // 일반적인 섹터 크기 설정
    disk.filesystem = file_system_resolve(&disk); //file_system_resolve()가 호출되어 디스크에 어떤 파일시스템이 있는지 확인합니다.
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

int disk_read_block(disk_t *idisk, unsigned int lba, unsigned int total, void *buffer)
{
    if (idisk != &disk)
    {
        return -MYOS_IO_ERROR; // 디스크가 NULL인 경우 오류 반환
    }
    return disk_read_sectors(lba, total, buffer);
}

int disk_write_block(disk_t *idisk, unsigned int lba, unsigned int total, void *buffer)
{
    if (idisk != &disk)
    {
        return -MYOS_IO_ERROR;
    }
    return disk_write_sectors(lba, total, buffer);
}