#ifndef ELFLOADER_H
#define ELFLOADER_H

#include <stdint.h>
#include <stddef.h>

#include "elf.h"
#include "../config.h"

/*
    elf_file : elf 파일의 정보를 저장하는 구조체
    운영체제가 실행하면서 실제로 부여한 동적인 정보
    elf_memory : elf 파일이 메모리에 로드된 물리 주소
    virtual_base_address : elf 파일의 가상 주소
    virtual_end_address : elf 파일의 가상 주소의 끝
    physical_base_address : elf 파일의 물리 주소
    physical_end_address : elf 파일의 물리 주소의 끝
*/
struct elf_file
{
    char filename[MYOS_MAX_PATH_LENGTH];

    int in_memory_size;
    void* elf_memory;
    void* virtual_base_address;
    void* virtual_end_address;
    void* physical_base_address;
    void* physical_end_address;
};

int elfloader_load_elf(struct elf_file* elf_file);
int elf_validate(struct elf_header* elf_header);

void* elf_get_memory(struct elf_file* elf_file);
struct elf_header* elf_get_header(struct elf_file* elf_file);
struct elf32_shdr* elf_get_section_header(struct elf_header* elf_header);
struct elf_program_header* elf_get_program_header(struct elf_header* elf_header);
struct elf_program_header* elf_program_header(struct elf_header* header, int index);
struct elf32_shdr* elf_section(struct elf_header* elf_header, int index);
char* elf_get_str_table(struct elf_header* elf_header);

void* elf_get_virtual_base(struct elf_file* elf_file);
void* elf_get_virtual_end(struct elf_file* elf_file);
void* elf_get_phys_base(struct elf_file* elf_file);
void* elf_get_phys_end(struct elf_file* elf_file);

#endif
