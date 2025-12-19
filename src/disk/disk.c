#include "../io/io.h"
#include "disk.h"
#include "../memory/memory.h"
#include "../config.h"
#include "../status.h"

disk_t disk;
//LBA : Logical Block Addressing give number of sector to read and write for the disk
//logical block addressing 방식으로 디스크에서 섹터를 읽는 함수 연속적인 비트 512바이트 단위 번호로 접근.
// cpu->ATA 포트 (0x1F0 ~ 0x1F7) 사용. -> 디스크 컨트롤러 
//lba sector number, total number of sectors to read, buffer to store data
int disk_read_sectors(int lba, int total, void *buffer)
{
    outsb(0x1F6, (lba >> 24) | 0xE0); // 드라이브 및 LBA 상위 4비트 설정
    outsb(0x1F2, total);              // 섹터 수 설정
    outsb(0x1F3, (unsigned char)(lba & 0xFF));         // LBA 하위 8비트 설정
    outsb(0x1F4, (unsigned char)((lba >> 8)));  // LBA 중간 8비트 설정
    outsb(0x1F5, (unsigned char)((lba >> 16)));  // LBA 중간 8비트 설정
    outsb(0x1F7, 0x20);               // 읽기 명령 전송

    unsigned short* ptr = (unsigned short*)buffer; //read word(2 bytes)
    for (int i = 0; i < total; i++)
    {
        //wait for the buffer to be ready.
        char c = insb(0x1F7);
        while(!(c & 0x08)) // bit3 : DRQ(Data Request) 비트가 설정될 때까지 대기
        {
            c = insb(0x1F7);
        }
        for(int i = 0; i < 256; i++) //512 bytes = 256 words
        {
            *ptr = insw(0x1F0); // 데이터 포트에서 2바이트 읽기
            ptr++;
        }
    }
    return 0; // 성공 시 0 반환
}

int disk_write_sectors(int lba, int total, void *buffer)
{
    outsb(0x1F6, (lba >> 24) | 0xE0); // 드라이브 및 LBA 상위 4비트 설정
    outsb(0x1F2, total);              // 섹터 수 설정
    outsb(0x1F3, (unsigned char)(lba & 0xFF));         // LBA 하위 8비트 설정
    outsb(0x1F4, (unsigned char)((lba >> 8)));  // LBA 중간 8비트 설정
    outsb(0x1F5, (unsigned char)((lba >> 16)));  // LBA 중간 8비트 설정
    outsb(0x1F7, 0x30);               // 쓰기 명령 전송

    unsigned short* ptr = (unsigned short*)buffer; //read word(2 bytes)
    for (int i = 0; i < total; i++)
    {
        //wait for the buffer to be ready.
        char c = insb(0x1F7);
        while(!(c & 0x08)) // bit3 : DRQ(Data Request) 비트가 설정될 때까지 대기
        {
            c = insb(0x1F7);
        }
        for(int i = 0; i < 256; i++) //512 bytes = 256 words
        {
            outsw(0x1F0, *ptr);
            ptr++;
        }
    }
    return 0; // 성공 시 0 반환
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