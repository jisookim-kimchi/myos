ORG 0x7c00
BITS 16

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

jmp short start
nop

;FAT16 Header
OEMIdentifier           db 'MYOS    '
BytesPerSector          dw 0x200
SectorPerCluster        db 0x80
ReservedSectorCount     dw 200 ;why not 1? 부트로더가 할수있는건 섹터읽긴데 커널을 저장할 공간을 확보하려고 200으로 설정, 커널을 파일로 두지않고 섹터에 직접 넣는다.커널을 파일로 넣는건 복잡하기때문 부트로더가 파일시스템을 알아야해
FATCopies               db 0x02
RootDirEntries          dw 0x40
NumSectors              dw 0x00
MediaType               db 0xF8
SectorsPerFat           dw 0x100
SectorsPerTrack         dw 0x20
NumberOfHeads           dw 0x40
HiddenSectors           dd 0x00
SectorsBig              dd 0x773594

; Extended BPB (Dos 4.0)
DriveNumber             db 0x80
WinNTBit                db 0x00
Signature               db 0x29
VolumeID                dd 0xD105
VolumeIDString          db 'MYOS BOOT  '
SystemIDString          db 'FAT16   '


start:
jmp 0:step2

step2:
    cli                    ; 인터럽트 비활성화
    mov ax, 0x00           ; ax = 0x00
    mov ds, ax             ; 데이터 세그먼트 = 0x00
    mov es, ax             ; 확장 세그먼트 = 0x00
    mov ss, ax             ; 스택 세그먼트 = 0x00
    mov sp, 0x7c00         ; 스택 포인터 설정
    sti                    ; 인터럽트 활성화

.load_protected:
    cli
    lgdt[gdt_descriptor]  ; load GDT on CPU's GDTR register
    mov eax, cr0          ; CRO's role : cpu mode setting 
    or eax, 1             ; PE 비트 설정 (Protected Mode Enable)
    mov cr0, eax            
    jmp CODE_SEG:load32
    jmp $
    
;GDT in protected mode define memory segments
gdt_start:


gdt_null:
    ; GDT entries would go here
    dd 0x0 ;4bytes
    dd 0x0 ;4bytes

;offset 0x08
gdt_code:
    dw 0xFFFF       ; Limit of Low Bits 0 - 15  //2bytes
    dw 0            ; Base of Low Bits 0 - 15   //2bytes
    db 0            ; Base of Middle Bits 16 - 23
    db 0x9a         ; Access Byte // 누가 어떻게 이 세그먼트에 접근가능? 10011010b
    db 11001111b    ; Granularity Byte
    db 0            ; Base of High Bits 24 - 31

;offset 0x10
gdt_data:           ;DS , ES, SS FS GS
    dw 0xFFFF              ; Limit of Low Bits 0 - 15
    dw 0             ; Base of Low Bits 0 - 15
    db 0            ; Base of Middle Bits 16 - 23
    db 0x92              ; Access Byte 
    db 11001111b    ; Granularity Byte
    db 0            ; Base of High Bits 24 - 31

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1 ; Size of GDT
    dd gdt_start               ; Address of GDT

[BITS 32]
load32:
    mov eax, 1
    mov ecx, 100
    mov edi, 0x0100000 ;kernel load address
    call ata_lba_read
    jmp CODE_SEG:0x0100000 ; Jump to kernel entry point at 1MB

ata_lba_read:
    mov ebx, eax ; Backup the LBA 

    ;send the high 8 bits of the lba to hard disk controller
    shr eax, 24
    or eax, 0xE0 ;setting for lba mode and master drive
    mov dx, 0x1F6
    out dx, al
    ;finished sending high 8 bits of lba
    
    ;send total sector to read
    mov eax, ecx
    mov dx, 0x1F2
    out dx, al
    ;finished sending total sector to read

    ;send more bits of the LBA
    mov eax, ebx ;restore the lba
    mov dx, 0x1F3
    out dx, al ;send bits 0-7
    ;finished sending bits 0-7 of lba

    mov dx, 0x1F4
    mov eax, ebx
    shr eax, 8
    out dx, al
    ;finished sending more bits of the lba

    ;send upper 16 bits of the lba
    mov dx, 0x1F5
    mov eax, ebx
    shr eax, 16
    out dx, al
    ;finished sending upper 16 bits of the lba

    mov dx, 0x1F7
    mov al, 0x20
    out dx, al ;send the read command
    ;finished sending the read command

    ;Read all sectors into memory
.next_sector:
    push ecx

    ;Check if we need to read
.try_again:
    mov dx, 0x1F7
    in al, dx
    test al, 8
    jz .try_again

;we need to read 256 words at a time
    mov ecx, 256
    mov dx, 0x1F0
    rep insw ;ECX 레지스터의 값만큼 다음 명령어를 반복 실행 0이 될때까지 계속 
    pop ecx
    loop .next_sector
;End of reading all sectors
    ret

times 510-($ -$$) db 0      ; fill remaining bytes with 0 up to 510
dw 0xAA55                   ; define word store 0xaa55


