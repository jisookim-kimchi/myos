#include "elfloader.h"
#include "../filesystem/file.h"
#include "../memory/heap/kernel_heap.h"
#include "../memory/memory.h"
#include "../status.h"
#include "elf.h"
#include <stdbool.h>

/**
 * ELF 로더 구현 순서:
 * 1. 파일을 열고 파일 전체를 elf_file->elf_memory (커널 버퍼)에 읽어옴
 * 2. elfloader_validate()를 사용하여 헤더가 올바른지 검사
 * 3. 프로그램 헤더들을 훑으며 필요한 가상 메모리의 시작과 끝 주소를 계산
 * (in_memory_size 파악)
 * 4. 실제 프로그램이 배치될 physical_base_address 메모리를 할당 (kzalloc 등)
 * 5. PT_LOAD 세그먼트들을 순회하며 elf_memory에서 physical_base_address로
 * 데이터 복사
 * 6. (중요) p_memsz가 p_filesz보다 큰 경우, 남은 부분을 0으로 채움 (BSS 처리)
 */

// 1단계: 파일을 열어 전체 데이터를 elf_file->elf_memory (커널 버퍼)에 읽어옴
int elfloader_read_file(struct elf_file* elf_file)
{
    int res = 0;
    int fd = 0;
    struct file_stat stat;

    fd = fopen(elf_file->filename, "r");
    if (fd <= 0)
        return -MYOS_IO_ERROR;

    res = fstat(fd, &stat);
    if (res < 0)
        goto out;

    elf_file->elf_memory = kernel_zero_alloc(stat.size);
    if (!elf_file->elf_memory)
    {
        res = -MYOS_ERROR_NO_MEMORY;
        goto out;
    }

    res = fread(fd, elf_file->elf_memory, stat.size, 1);
    if (res < 0)
        goto out;

    res = MYOS_ALL_OK;

out:

    fclose(fd);
    return res;
}

int elfloader_calculate_vmem_boundaries(struct elf_file *elf_file)
{
    int res = 0;
    struct elf_header* header = elf_get_header(elf_file);
    uint32_t min_vaddr = 0xFFFFFFFF;
    uint32_t max_vaddr = 0;
    for (int i = 0; i < header->e_program_header_count; i++)
    {
        struct elf_program_header *phdr = elf_program_header(header, i);
        if (phdr->ep_type != PT_LOAD)
            continue;

        if (phdr->ep_vaddr < min_vaddr)
            min_vaddr = phdr->ep_vaddr;
        if (phdr->ep_vaddr + phdr->ep_memsize > max_vaddr)
            max_vaddr = phdr->ep_vaddr + phdr->ep_memsize;
    }
    if (max_vaddr <= min_vaddr)
    {
        return -MYOS_INVALID_ARG;
    }

    elf_file->virtual_base_address = (void *)min_vaddr;
    elf_file->virtual_end_address = (void *)max_vaddr;
    elf_file->in_memory_size = max_vaddr - min_vaddr;

    return res;
}

// 4. 실제 프로그램이 배치될 physical_base_address 메모리를 할당 (kzalloc 등)
int elfloader_allocate_phy_memory(struct elf_file *elf_file)
{
    int res = 0;
    elf_file->physical_base_address = kernel_zero_alloc(elf_file->in_memory_size);
    if (!elf_file->physical_base_address)
    {
        res = -MYOS_ERROR_NO_MEMORY;
        goto out;
    }

    res = MYOS_ALL_OK;

out:
    return res;
}

// 5. PT_LOAD 세그먼트들을 순회하며 elf_memory에서 physical_base_address로 데이터 복사
int elfloader_load_segments(struct elf_file* elf_file)
{
    int res = 0;
    struct elf_header* header = elf_get_header(elf_file);
    for (int i = 0; i < header->e_program_header_count; i++)
    {
        struct elf_program_header* phdr = elf_program_header(header, i);
        if (phdr->ep_type != PT_LOAD)
            continue;

        void* dest = (void*)((uint32_t)elf_file->physical_base_address + (phdr->ep_vaddr - (uint32_t)elf_file->virtual_base_address));
        void* src = (void*)((uint32_t)elf_file->elf_memory + phdr->ep_offset);
        ft_memcpy(dest, src, phdr->ep_filesize);
    }

    return res;
}

int elfloader_load_elf(struct elf_file *elf_file)
{
  int res = 0;

  // 1. 파일 읽기
  res = elfloader_read_file(elf_file);
  if (res < 0)
    return res;

  // 2. elfloader_validate()를 사용하여 헤더가 올바른지 검사
  struct elf_header* header = elf_get_header(elf_file);
  res = elf_validate(header);
  if (res < 0)
    return res;

  // 3. 프로그램 헤더들을 훑으며 필요한 가상 메모리의 시작과 끝 주소를 계산
  res = elfloader_calculate_vmem_boundaries(elf_file);
  if (res < 0)
    return res;

  // 4. 실제 프로그램이 배치될 physical_base_address 메모리를 할당 (kzalloc 등)
  res = elfloader_allocate_phy_memory(elf_file);
  if (res < 0)
    return res;

  // 5. 데이터 복사
  res = elfloader_load_segments(elf_file);
  if (res < 0)
      return res;

  return res;
}

