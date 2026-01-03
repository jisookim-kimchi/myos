#ifndef ELF_H
#define ELF_H

#include <stdint.h>
#include <stddef.h>

//0x7f is not a normal ASCII character. the system interpreted as : i am a binary file, not a text file
//kind of a sign.

#define PF_X 0x01
#define PF_W 0x02
#define PF_R 0x04

#define ELFMAG0 0x7f
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'

#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3
#define PT_NOTE 4
#define PT_SHLIB 5
#define PT_PHDR 6

#define SHT_NULL 0
#define SHT_PROGBITS 1
#define SHT_SYMTAB 2
#define SHT_STRTAB 3
#define SHT_RELA 4
#define SHT_HASH 5
#define SHT_DYNAMIC 6
#define SHT_NOTE 7
#define SHT_NOBITS 8
#define SHT_REL 9
#define SHT_SHLIB 10
#define SHT_DYNSYM 11
#define SHT_LOPROC 12
#define SHT_HIPROC 13
#define SHT_LOUSER 14
#define SHT_HIUSER 15

#define ET_NONE 0
#define ET_REL 1
#define ET_EXEC 2
#define ET_DYN 3
#define ET_CORE 4

#define EI_NIDENT 16
#define EI_CLASS 4
#define EI_DATA 5

#define ELFCLASSNONE 0
#define ELFCLASS32 1
#define ELFCLASS64 2

#define ELFDATANONE 0
#define ELFDATA2LSB 1
#define ELFDATA2MSB 2

#define SHN_UNDEF 0

//ELF Header : master map
/*
Position (32 bit)	Position (64 bit)	Value
0-3	0-3	Magic number - 0x7F, then 'ELF' in ASCII
4	4	1 = 32 bit, 2 = 64 bit
5	5	1 = little endian, 2 = big endian
6	6	ELF header version
7	7	OS ABI - usually 0 for System V
8-15	8-15	Unused/padding
16-17	16-17	Type (1 = relocatable, 2 = executable, 3 = shared, 4 = core)
18-19	18-19	Instruction set - see table below
20-23	20-23	ELF Version (currently 1)
24-27	24-31	Program entry offset
28-31	32-39	Program header table offset
32-35	40-47	Section header table offset
36-39	48-51	Flags - architecture dependent; see note below
40-41	52-53	ELF Header size
42-43	54-55	Size of an entry in the program header table
44-45	56-57	Number of entries in the program header table
46-47	58-59	Size of an entry in the section header table
48-49	60-61	Number of entries in the section header table
50-51	62-63	Section index to the section header string table
*/
struct elf_header
{
  unsigned char e_magic[16];                    // 0-15 : magic number and basic info
  uint16_t e_file_type;                         // 16-17 : file type
  uint16_t e_machine;                           // 18-19 : machine type cpu architecture
  uint32_t e_version;                           // 20-23 : elf version
  uint32_t e_entry;                             // 24-27 : program starting point entry point
  uint32_t e_program_header_offset;             // 28-31 : program header offset
  uint32_t e_section_header_offset;             // 32-35 : section header offset
  uint32_t e_flags;                             // 36-39 : flags
  uint16_t e_elf_header_size;                   // 40-43 : elf header size
  uint16_t e_program_header_size;               // 44-47 : program header size
  uint16_t e_program_header_count;              // 48-51 : program header count
  uint16_t e_section_header_size;               // 52-55 : section header size
  uint16_t e_section_header_count;              // 56-59 : section header count
  uint16_t e_section_header_string_table_index; // 60-63 : section header string
} __attribute__((packed));

//실제 코드를 메모리에 어디에 복사해야하는지...?
/*
Position	Value
0-3	Type of segment (see below)
4-7	The offset in the file that the data for this segment can be found
(p_offset) 8-11	Where you should start to put this segment in virtual memory
(p_vaddr) 12-15	Reserved for segment's physical address (p_paddr) 16-19	Size of
the segment in the file (p_filesz) 20-23	Size of the segment in memory
(p_memsz, at least as big as p_filesz) 24-27	Flags (see below)  //
Permissions  (read:4, write:2, execute:1) 28-31	The required alignment for this
section (usually a power of 2)
*/
struct elf_program_header
{
  uint32_t ep_type;   // if it is 1, it means Load its contents into memory
  uint32_t ep_offset;
  uint32_t ep_vaddr;
  uint32_t ep_paddr;
  uint32_t ep_filesize;
  uint32_t ep_memsize;
  uint32_t ep_flags;
  uint32_t ep_align;
} __attribute__((packed));

/* Standard ELF 32-bit Types */
typedef uint16_t elf32_half;
typedef uint32_t elf32_word;
typedef int32_t elf32_sword;
typedef uint32_t elf32_addr;
typedef int32_t elf32_off;

//여기부터 여기까지는 함수들 (.text이고) 요기는 변수들(.data) 저기는 .bss 이야.
//파일 내부를 논리적으로 쪼개서 설명. 인덱스탭
/* Standard ELF 32-bit Section Header */
struct elf32_shdr
{
  elf32_word sh_name;
  elf32_word sh_type;
  elf32_word sh_flags;
  elf32_addr sh_addr;
  elf32_off sh_offset;
  elf32_word sh_size;
  elf32_word sh_link;
  elf32_word sh_info;
  elf32_word sh_addralign;
  elf32_word sh_entsize;
} __attribute__((packed));

// 함수 이름, 함수의 주소, 함수의 크기, 함수의 정보, 함수의 기타 정보, 함수의 인덱스
/* Standard ELF 32-bit Symbol Table Entry */
struct elf32_sym
{
  elf32_word st_name;
  elf32_addr st_value;
  elf32_word st_size;
  unsigned char st_info;
  unsigned char st_other;
  elf32_half st_shndx;
} __attribute__((packed));

//외부 라이브러리 주문서 외부 협력사 연락처 같은것. 동적링크 기능을 만들때 필수 표준 라이브러리같은것.
/* Standard ELF 32-bit Dynamic Entry */
struct elf32_dyn
{
  elf32_sword d_tag;
  union
  {
    elf32_word d_val;
    elf32_addr d_ptr;
  } d_un;

} __attribute__((packed));


void* elf_get_entry_ptr(struct elf_header* elf_header);
uint32_t elf_get_entry(struct elf_header* elf_header);

#endif