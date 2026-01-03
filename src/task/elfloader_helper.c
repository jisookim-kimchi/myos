#include "elfloader.h"
#include "elf.h"
#include "../status.h"
#include <stdbool.h>


bool elfloader_validate_elf_magic(struct elf_header* elf_header)
{
    return elf_header->e_magic[0] == ELFMAG0 &&
           elf_header->e_magic[1] == ELFMAG1 &&
           elf_header->e_magic[2] == ELFMAG2 &&
           elf_header->e_magic[3] == ELFMAG3;
}

//32bit check
bool elf_valid_class(struct elf_header* elf_header)
{
    //return elf_header->e_magic[EI_CLASS] == ELFCLASSNONE ||
    return elf_header->e_magic[EI_CLASS] == ELFCLASS32;
}

//endian 체크
//ELFDATANONE : 엔디안이 정의되지 않았음
//ELFDATA2LSB : 엔디안이 Little Endian
bool elf_valid_encoding(struct elf_header* elf_header)
{
    // return elf_header->e_magic[EI_DATA] == ELFDATANONE ||
    return elf_header->e_magic[EI_DATA] == ELFDATA2LSB;
}

bool elf_is_executable(struct elf_header* elf_header)
{
    return elf_header->e_file_type == ET_EXEC &&
             elf_header->e_entry >= MYOS_PROGRAM_VIRTUAL_ADDRESS;
}

bool elf_has_program_header(struct elf_header* elf_header)
{
    return elf_header->e_program_header_offset > 0;
}

void* elf_get_memory(struct elf_file* elf_file)
{
    return elf_file->elf_memory;
}

//file 0x0 is elf header
struct elf_header* elf_get_header(struct elf_file* elf_file)
{
    return (struct elf_header*)elf_file->elf_memory;
}

struct elf32_shdr* elf_get_section_header(struct elf_header* elf_header)
{
    return (struct elf32_shdr*)((uint32_t)elf_header+elf_header->e_section_header_offset);
}

struct elf_program_header* elf_get_program_header(struct elf_header* elf_header)
{
    if (elf_header->e_program_header_offset == 0)
        return NULL;
    
    return (struct elf_program_header*)((uint32_t)elf_header + elf_header->e_program_header_offset);
}

/*
    0번 짐: 실행 코드 (.text)
    1번 짐: 데이터 (.data)
    2번 짐: 읽기 전용 데이터 (.rodata)
*/
struct elf_program_header* elf_program_header(struct elf_header* header, int index)
{
    return &elf_get_program_header(header)[index];
}

struct elf32_shdr* elf_section(struct elf_header* elf_header, int index)
{
    return &elf_get_section_header(elf_header)[index];
}

char* elf_get_str_table(struct elf_header* elf_header)
{
    return (char*) elf_header + elf_section(elf_header, elf_header->e_section_header_string_table_index)->sh_offset;
}

void* elf_get_virtual_base(struct elf_file* elf_file)
{
    return elf_file->virtual_base_address;
}

void* elf_get_virtual_end(struct elf_file* elf_file)
{
    return elf_file->virtual_end_address;
}

void* elf_get_phys_base(struct elf_file* elf_file)
{
    return elf_file->physical_base_address;
}

void* elf_get_phys_end(struct elf_file* elf_file)
{
    return elf_file->physical_end_address;
}

int elf_validate(struct elf_header* elf_header)
{
    return (elfloader_validate_elf_magic(elf_header) && elf_valid_class(elf_header) && elf_valid_encoding(elf_header) && elf_has_program_header(elf_header)) ? MYOS_ALL_OK : -MYOS_INVALID_ARG;
}