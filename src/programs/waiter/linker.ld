ENTRY(_start)
OUTPUT_FORMAT("binary")
SECTIONS
{
    . = 0x10000000;
    .text : ALIGN(4096)
    {
        *(.asm)
        *(.text)
    }
    .rodata : ALIGN(4096)
    {
        *(.rodata)
    }

    .data : ALIGN(4096)
    {
        *(.data)
    }
    
    .bss : ALIGN(4096)
    {
        *(COMMON)
        *(.bss)
    }
}
