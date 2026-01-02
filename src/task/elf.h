#ifndef ELF_H
#define ELF_H

#include <stdint.h>

//0x7f is not a normal ASCII character. the system interpreted as : i am a binary file, not a text file
//kind of a sign.
#define ELFMAG0 0x7f
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'

#define PT_LOAD 1

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
4-7	The offset in the file that the data for this segment can be found (p_offset)
8-11	Where you should start to put this segment in virtual memory (p_vaddr)
12-15	Reserved for segment's physical address (p_paddr)
16-19	Size of the segment in the file (p_filesz)
20-23	Size of the segment in memory (p_memsz, at least as big as p_filesz)
24-27	Flags (see below)  // Permissions  (read:4, write:2, execute:1)
28-31	The required alignment for this section (usually a power of 2)
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

#endif