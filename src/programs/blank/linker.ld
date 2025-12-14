ENTRY(_start)
OUTPUT_FORMAT("binary")
SECTIONS
{
    . = 0x400000;
    .text : ALIGN(4096) /* c언어로 짠 코드들이 컴파일되어 기계어로 변한것, */
    {
        *(.text) 
    }
    .asm : ALIGN(4096) /* 어셈블리코드 */
    {
        *(.asm)
    }
    .rodata : ALIGN(4096) /* 데이터 */
    {
        *(.rodata)
    }

    .data : ALIGN(4096) /* 변수창고 */
    {
        *(.data)
    }
    
    .bss : ALIGN(4096) /* 빈 박스창고 */
    {
        *(COMMON)
        *(.bss)
    }
}