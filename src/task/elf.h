#ifndef ELF_H
#define ELF_H

#include <stdint.h>

/**
 * Standard ELF32 Data Types
 */
typedef uint32_t Elf32_Addr;
typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Off;
typedef int32_t Elf32_Sword;
typedef uint32_t Elf32_Word;

#define EI_NIDENT 16

/**
 * ELF32 Execution Header
 */
typedef struct {
  unsigned char e_ident[EI_NIDENT];
  Elf32_Half e_type;
  Elf32_Half e_machine;
  Elf32_Word e_version;
  Elf32_Addr e_entry; /* 진짜 프로그램 시작 주소! */
  Elf32_Off e_phoff;  /* 프로그램 헤더 테이블의 오프셋 */
  Elf32_Off e_shoff;
  Elf32_Word e_flags;
  Elf32_Half e_ehsize;
  Elf32_Half e_phentsize;
  Elf32_Half e_phnum; /* 프로그램 헤더의 개수 */
  Elf32_Half e_shentsize;
  Elf32_Half e_shnum;
  Elf32_Half e_shstrndx;
} Elf32_Ehdr;

/**
 * ELF32 Program Header (Segment Header)
 */
typedef struct {
  Elf32_Word p_type;  /* PT_LOAD 등을 확인 */
  Elf32_Off p_offset; /* 파일 내 오프셋 */
  Elf32_Addr p_vaddr; /* 매핑될 가상 주소 */
  Elf32_Addr p_paddr;
  Elf32_Word p_filesz; /* 파일에서의 크기 */
  Elf32_Word p_memsz;  /* 메모리에서의 크기 (BSS 포함) */
  Elf32_Word p_flags;
  Elf32_Word p_align;
} Elf32_Phdr;

/* ELF Magic Numbers */
#define ELFMAG0 0x7f
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'

/* Segment Types */
#define PT_NULL 0
#define PT_LOAD 1 /* 메모리에 로드해야 하는 세그먼트 */
#define PT_DYNAMIC 2
#define PT_INTERP 3
#define PT_NOTE 4
#define PT_SHLIB 5
#define PT_PHDR 6

#endif
