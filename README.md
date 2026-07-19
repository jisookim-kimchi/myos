# MyOS: 32-bit x86 Operating System

### Bootloader
BIOS ROM runs immediately upon turning on the PC -> loads the bootloader by searching for the boot signature "0x55AA".
Copies this sector (511 and 512 bytes) to RAM address 0x0000:0x7C00.
Jumps to 0x7C00 to execute the bootloader!

1. Transitioning from the bootloader to the kernel was very difficult because I had to write it in assembly.
In the process of switching to protected mode: loading the GDT, setting the CR0 PE bit, and performing a Far Jump.
The GDT is a structure defining segment registers, which can be thought of as a map showing where segments start and end.

CR0 is a control register where the 0th bit is the Protection Enable bit.
Changing 0 -> 1 switches the mode to 32-bit Protected Mode. However, we only changed the mode; we don't know how to jump to the kernel yet.

Far Jump: Changing the mode is not enough since the CPU's CS register remains in 16-bit mode. The far jump completely transitions to running 32-bit code.

2. A20 Gate: Enabled to allow memory access beyond 1MB.
3. ATA LBA Loading: Bypassed BIOS interrupts and directly used the hard disk controller's I/O ports to read data, delivering 100 kernel sectors to the 1MB mark.

4. Entering Protected Mode:
Called `ata_lba_read` under `[BITS 32]`, read data from the hard disk to memory, loaded the kernel to memory (1MB), and executed it.

#### Kernel Initialization
##### kernel.asm
1. Initializes all segment registers to the kernel segment (0x10).
2. Sets up the kernel stack (0x200000).
3. Remaps the PIC (IRQ 0-15 -> 32-47).

##### kernel.c
1. Redefines GDT and initializes the kernel heap.
2. Turns on IDT and I/O interrupt timer drivers.
3. Defines TSS (Task State Segment) and configures the kernel stack (0x600000) to return to during interrupts.
4. Searches the file system disk to recognize and prepare FAT16.
5. Enables paging to manage memory in 4KB blocks.

Why redefine the GDT? The bootloader's GDT is a minimal map, while the kernel's GDT is a complete map containing all features needed by the kernel.

#### TSS and TASK
TSS: Informs the CPU of the kernel stack (esp0) to return to kernel space when an interrupt occurs in user space.

struct task: A software data sheet that saves register states (EAX, EBX, etc.) used by the current process, so that it can be restored exactly when executed again later.
Explanation of TASK continues below...

#### FAT 16Disk
- Reserved Region: The area containing the boot sector (BPB) and our kernel. Think of it as the main gate.
Since `reserved sectors = 200` is configured, sector 0 has the bootloader, and sectors 1 to 199 load and wait for `kernel.bin`.
- FAT Region: A map showing how clusters are connected (FAT table). Think of it as apartment buildings.
Usually, there are 2 FAT tables, which exist for recovery purposes.
- Root Directory: A list of files in the root directory (/). Think of it as room numbers in an apartment.
Contains info like file name, extension, size, cluster position, etc. We parse this to find the name and locate the cluster number for the data region.
- Data Region: The area where actual file contents (text, binary, etc.) are stored. Think of it as actual residents.
Starts from cluster 2; clusters 0 and 1 are already reserved. The actual contents of the files we read and write are stored here.

#### File System
Let's look at the structure of FAT16.
Address (Byte) :  00    01  |  02    03  |  04    05  |  06    07  | ...
            [ Byte 0 ] [ Byte 2 ] [ Byte 4 ] [ Byte 6 ]
-------------------------------------------------------
Contents (Value) : [  0xFFF8  ] [  0xFFFF  ] [    10    ] [    11    ] ...
            (Cluster 0)  (Cluster 1)  (Cluster 2)  (Cluster 3)

A file system is just a rule.
From the OS's perspective, a disk is just a chunk of data composed of 0s and 1s.
The file system driver interprets this as a FAT16 file system and provides an interface.
I used arrays for the file system for extensibility, although I only implemented FAT16.

Keep this structure in mind!
FAT16 physical structure:
Reserved (Boot area) -> FAT1
FAT1 -> FAT2
FAT2 -> RootDir
RootDir -> Data

1. Implemented reading data from the disk byte-by-byte using streamers.
2. fat_header (BPB): If the disk is a book, think of the BPB as the table of contents. It is a structure mapping 1:1 to the hard disk binary data (referenced OSDev Wiki).
3. FAT Table: Linked list.
Core concept: Where are the files scattered?
`fat_entry_pos = (current cluster number) * 2`. Why * 2? Because it is FAT16, so each entry is 16 bits (2 bytes).
4. Address Translation:
The CPU wants to know the address, but FAT16 talks in cluster numbers.
`Sector = (Cluster - 2) * Sectors_per_cluster + root_directory.ending_sector_pos (Data area start)`
If the cluster is 3, then 3 - 2 = 1. Multiply that by the sectors per cluster and add the data area start sector.
Why Cluster - 2? Because clusters 0 and 1 are reserved.
5. Dedicated Streamers:
If you look at our `struct fat_private`, there are multiple streamers (e.g., `cluster_read_stream`, `fat_read_stream`) instead of just one.
Why: When reading a file, you occasionally need to search the FAT table address to find the next cluster. Using only one streamer would corrupt the file read offset. Therefore, I separated the FAT table streamer and the actual data streamer.
6. Directory Structure:
* Root Directory: Located right after the FAT table (fixed position). The size is fixed based on the number defined in the boot sector (Root Entries) and cannot be expanded.
* Sub-directories: Can be located anywhere in the data region (just like a normal file). The size can expand infinitely since it is connected by a cluster chain. It is just a file with a special directory attribute (`0x10`).

#### Paging
* Virtual Memory: Even if physical RAM is small, each program is allocated a virtual memory space of 4GB.
* Memory Isolation: Separates user space from the kernel. This is critical.
CR3 Register: Holds the physical address of the page directory currently in use.
This is critical for context switching.
The moment you swap the directory address from process A's to process B's, the memory map observed by the OS changes.
A technology mapping virtual addresses to physical addresses.
Structure: Page Directory -> Page Table -> Physical Frame.
Each process gets its own 4GB virtual memory space.
The memory layout for my OS is configured in `config.h`.
0~256MB: Kernel space.
256~512MB: User space.
The reason for separating kernel and user space at the 256MB boundary is security; otherwise, user-allocated memory could bleed into kernel space.

Paging structure:
Note that we manage memory in 4KB blocks per page.
Total is 1024 * 1024 * 4096 = 4GB.
1. Page Directory: Contains 1024 entries, each holding the physical address of a Page Table (like a country).
2. Page Table: Address index (like a city).
3. Page Table Entry: Page index (4 bytes) holding the physical address of the Page Frame (like a neighborhood/district).
4. Page Frame (Physical Frame, 4KB): Physical memory page (like an actual street address).

#### Heap
Heap Table:
Remember we manage memory in 4KB blocks.
An array managing the kernel heap status.
Each entry represents a 4KB memory block.
`[ 7(NEXT)][ 6(FIRST)][ 5 ][ 4 ][ 3 ][ 2 ][ 1 ][ 0(TAKEN/FREE)]` (Other bits are currently unused, but can be added).

When `malloc` is called -> calculates the required blocks via `heap_calculate_required_blocks(size)` -> searches for contiguous free space that satisfies the requested size starting from the beginning of the table (Index 0) -> calls `heap_mark_blocks_as_taken()` to mark them as "taken" -> returns the address.

#### Interrupt
When ring3 calls `int 0x80` ->
The CPU looks at the TSS, shifts the stack pointer to `ESP0` (kernel stack), and backs up the original states (user stack, address, code location) onto the kernel stack.
Then it restores states (see Restore state in `idt.asm`) via `popad` and returns via `iret`.
In `idt.asm`:
Note that `pop` increments esp by 4, which is different from `push`. Restoring `esp` into register was new to me.
; Restore state
    popad          ; Restores saved EAX, EBX... all 8 registers at once!
    pop gs         ; Pop segment registers one by one
    pop fs         ;
    pop es         ;
    pop ds         ;
    add esp, 8     ;
    iret           ; Return to Ring 3

1. IDT (Interrupt Descriptor Table):
Entries from 0 to 255 containing information for each interrupt.
Jumps to ISR when a specific signal number triggers!
Initialized with `idt_init()`.
2. ISR (Interrupt Service Routine):
The function that executes the actual interrupt.
This was a bit complicated. I referenced assembly in `idt.asm`, processed common routines, and called the C function.
nasm assembly syntax was quite challenging, and I relied heavily on OSDev Wiki.
Also, the first member of the structure actually points to the top of the stack (the lowest address). I encountered errors initially because I didn't know this.
3. ISR80h:
Puts the system call number in EAX and jumps to `interrupt_128`.
Jumps to `interrupt_handler` after executing `pushad`.
In `interrupt_handler`, checks EAX to process the system call (verifies `isr80h_handle_command`).

How is data passed?
* User program pushes arguments (string pointer) onto the stack.
* Kernel reads the user stack contents using `task_get_stack_item`.
* Then copies data from user to kernel using `copy_from_task`.
Note: The kernel temporarily switches paging (`paging_switch`) to the user's page directory, reads that memory, and returns to the kernel.
4. Hardware Interrupt:
Reads data from ports using the I/O bus.
Pressing a key triggers IRQ 1 -> jumps to IDT 0x21 -> reads data from port 0x60 (keyboard data port).
`io.asm` acts as the driver.

#### TASK
A task is similar to a thread.
How does a task use process shared resources?
1. Create a process first.
2. When calling `init_task`, creates a 4GB memory directory. Here, `task->process = process`.
3. Load file data -> load into physical memory.
4. Important point:
`process_map_memory` maps virtual memory to physical memory.
Each task has a page directory. `process_map_binary` maps the process physical address to the page directory. This means multiple tasks can share the process physical memory space.

#### Scheduling (Context Switching)
1. Scheduling:
Selects the next task to run by following the list's next pointer in a Round-Robin fashion.
Uses timer interrupts to perform preemptive scheduling every 10ms.
The OS forces a switch even if the task doesn't yield.
Why preemptive?
Because even if a program gets stuck in a bug, the OS forces a switch every 10ms so it doesn't freeze.
Implementing a priority queue to schedule based on priorities could be a good improvement.
Currently, I just implemented the basic logic.
How is priority distinguished? -> If a task runs for 100ms continuously, its priority is lowered to keep the system responsive.

Alternative approaches:
Assigning fixed priorities to each task (important in embedded systems), or reservation schemes.
2. Context Switching:
`task_switch` and `task_return`.
3. Paging Switch (CR3 Switch):
`paging_switch(task->page_directory)`.
Puts the page directory address of the new task into the CR3 register.
At this moment, the virtual memory space observed by the CPU instantly changes from Task A's world to Task B's world (code region and 16KB stack are swapped).
4. Privilege Recovery (TSS Update):
`tss.esp0 = new_task->kstack + 4096`.
TSS (Task State Segment): Informs the CPU of the kernel stack (Ring 0) to use when an interrupt occurs in user mode (Ring 3).
Since each task has its own kernel stack, the TSS must be updated on every switch!

#### PROCESS
A container (house) that includes memory, files, etc.
An active worker that occupies the CPU and executes code (the target of Context Switching).
A process has one or more tasks (currently 1:1, but can be expanded for multitasking).
Creation flow: `process_load_for_slot -> process_load_data -> process_map_virtual_memory -> process_setup_arguments -> iret`.

#### ELF
ELF is like a map showing where code and data belong.
Flow: `process_load_data -> elfloader_load_elf`.

#### Sbrk (Memory Allocation)
If a user program runs out of heap space during malloc, it requests the `sbrk` system call (defined in malloc functions).
Increases the process's `cur_end_heap` and maps actual physical pages for the expanded space.

#### Execution Method
1. Run `./build.sh` to compile the kernel and user programs.
2. Run `qemu-system-i386 -hda ./bin/myos.bin` to execute the OS.
3. Enter commands in the shell: `run 0:/bf.bin 0:/hello.bf`
4. `run 0:/blank.bin` (Test running an empty program)
5. `run 0:/waiter.bin` (Parent-child process wait test)
6. `run 0:/shell.bin` (Test running the shell)

##### References
I referenced OSDev Wiki whenever I got stuck.
- OSDev Wiki (https://wiki.osdev.org): Essential for understanding hardware specifications and structuring structs.
- Simhs93's Blog (https://m.blog.naver.com/simhs93/): An excellent lecture series in Korean.
I referenced these two the most.

Personal Reflection:
There was a lot to learn, including assembly. Although I am still not fluent in writing it, I can now read the code, follow the execution flow, and implement necessary features.
I had some understanding of interrupts, but I learned how the kernel transition works by implementing the TSS.
I also learned to separate the kernel stack and user stack while implementing the task struct.
Assembly felt like a foreign language at first, but analyzing `boot.asm` through `kernel.asm` line-by-line was an invaluable experience in learning how to communicate with hardware.
Debugging the kernel using GDB was also extremely helpful, though writing the Makefile was quite complex and difficult.

#### PCI Bus (Peripheral Component Interconnect)
Located on the motherboard.
The CPU requests Bus 0, Slot 1 from port 0xCF8 (PCI controller address port) using `outl`.
The motherboard responds to port 0xCFC (PCI controller data port).
Then the CPU reads the response using `inl`.
Essentially, it is a system that scans and connects internal devices.

NICs and GPU addresses change frequently.
Why?
Plug and Play: Once plugged in, the BIOS/OS finds and assigns a vacant address.
This is highly flexible when adding devices.
If you plug a NIC into slot 1 it becomes address 101, and into slot 2 it becomes 201.
Therefore, the OS must query the PCI controller (0xCF8) to use the device.
Is room 101 occupied? -> None (0xFFFF)
Is room 102 occupied? -> Realtek NIC (0x10EC)

Address Acquisition:
How do I communicate with it? -> Go to Base Address 0xC000 (BAR0).
Communication Start:
Sending data to 0xC000 goes to the NIC.
Our NIC is RTL8139.

#### rtl8139
RTL8139 network driver.
Detects devices via the PCI bus and handles packet transmission/reception. See `rtl8139.h`.
Packet Transmission (TX): Uses 4 slots in a round-robin format.
Packet Reception (RX): Uses WRAP mode.
WRAP Mode: Saves packets continuously without truncation. That's why I added `+1500` bytes. It is simple to implement but requires more memory.
To test packets, Wireshark is required.
Command: `qemu-system-i386 -hda ./myos.bin -netdev user,id=net0 -device rtl8139,netdev=net0 -object filter-dump,id=f1,netdev=net0,file=dump.pcap`

#### Ethernet Frame
Ethernet is a protocol for transmitting data over LAN cables (Local Area Network).
Local, not wide area. Different from Wi-Fi!
Format:
[Dest MAC Addr (6 bytes)] [Src MAC Addr (6 bytes)] [Frame Type (2 bytes)] [Payload (Max 1500 bytes)]

##### ARP (Address Resolution Protocol)
Structure under Ethernet Frame:
Ethernet Header (14 bytes) -> ARP Packet (28 bytes)
ARP: Resolves IP address to MAC address. Why? Ethernet communicates using MAC addresses (Layer 2).
Think of it like a contacts app. We only know the IP address, so we query to find the MAC address.
Debugging tip: I forgot to swap between network byte order and host byte order, causing it to fail initially.

##### IP (Internet Protocol)
Structure:
Ethernet Header (14 bytes) -> IP Header (20 bytes) -> Payload
IP: Transmits actual data (Layer 3).
- Source IP
- Destination IP
- Protocol (TCP, UDP, ICMP...)
- Data (Payload)
- TTL (Time to Live): Expiration of the packet.

##### ICMP (Internet Control Message Protocol)
Ping protocol: A signal sent to verify connection.
ICMP: Relays diagnostic information when IP packet transmission, reception, or routing fails.
In `bits.h`:
Network: Big-endian
CPU: Little-endian
Because of the different bit orders, `ntohs` and `ntohl` must be used.
I missed this frequently during testing, so packets sometimes didn't show up in Wireshark.

##### TCP (Transmission Control Protocol)
In little-endian systems, bits are filled right-to-left.
The struct declaration order is reversed compared to the actual memory layout!
Layers: Ethernet | IP | TCP | Data (14 + 20 + 20 + payload)
Segment = TCP Header + Data
Packet = IP Header + Segment
Frame = Ethernet Header + Packet

Connection:
3-way-handshake (SYN -> SYN-ACK -> ACK) without data payload to verify connection.

Debugging:
Used Wireshark to track SEQ and ACK numbers to verify delivery.
When connecting with HTTP, I encountered errors because the buffer size was set too small.
Also, because host (little-endian) and network (big-endian) byte orders differ, I had to ensure ntohs and ntohl were used consistently, which was a common source of bugs.

---


### 부트로더
pc를 키자마자 bios ROM 실행 -> 부트로더 로드 함 by searching boot signature "0x55AA"
511과 512 byte  -> 이섹터를 RAM의 주소 0x0000:0x7c00에 복사합니다. 
0x7c00로 점프해서 부트로더 실행! 

1. 부트로더에서 커널로 들어가는 부분에서 정말 어려웠음, 이유는 어셈블리로 짜야했기때문인데...
보호모드로 전환하는 과정에서 GDT로드 하는법, CR0 PE비트 설정하는법, Far jump하는법
gdt 에는 세그먼트 레지스터를 정의하는 구조체가 있고 지도로 생각하면됨. 어디서부터 어디까지가 정의되어있는지 확인가능.

CR0는 컨트롤 레지스터인데 0번째 비트가 Protection Enable 비트,
0 -> 1 로 바꾸면 32비트 보호모드로 동작한다! 라고 모드를 바꿈. 모드만 바꾼상태. 아직 커널로 전환하는 방법모름.

Far Jump : 모드만 바꿨을뿐 아직 CPU의 cs는 16비트 모드로 남아있기때문에 완전히 32비트 코드로 바꾸는역할.

2. A20 Gate : 1Mb이상의 메모리 접근을 가능하기 위해 이것도 활성화.
3. ATA LBA 로딩 BIOS 인터럽트 사용하지 않고 직접 하드디스크 컨트롤러의 I/O 포트를 사용하여 데이터를 읽어오고, 커널 100 섹터를 1MB지점으로 배달함

4. 보호모드로 들어가서 
[BITS 32]
ata_lba_read 호출, 하드디스크로부터 메모리로 데이터를 읽어오고. 커널을 메모리(1MB)로 옮겨놓고 실행.

#### 커널 초기화
##### kernel.asm
1. 모든 세그먼트 레지스터를 커널용 (0x10)으로 초기화.
2. 커널 스택 설정 (0x200000).
3. PIC 재매핑(IRQ 0-15 -> 32-47).

##### kernel.c
1. GDT다시 정의, 커널 힙을 초기화.
2. IDT & 입출력 인터럽트 타이머 드라이브 킴.
3. TSS (Task State Segment) 정의, 인터럽트 시 돌아올 커널 스택(0x600000)을 설정.
4. 파일시스템 디스크를 뒤져서 FAT16을 인식하고 사용할 준비를 마침.
5. 페이징 : 메모리를 4KB 단위로 쪼개어 관리하는 페이징을 킴.

GDT를 왜 다시 정의 ? : 부트로더 GDT는 최소한의 지도, 커널 GDT는 커널이 필요로 하는 모든 기능을 담은 완성된 지도.

#### TSS와 TASK
TSS : 유저 영역서 인터럽트 발생시 커널 영역으로 돌아오기 위해 커널 스택(esp0)을 알려줌.

struct task: 현재 프로세스가 사용하던 레지스터(EAX, EBX 등) 상태를 저장하여, 나중에 다시 실행될 때 그 상태 그대로 복원할 수 있게 해주는 소프트웨어 데이터 시트입니다.
TASK에 대한 설명은 뒤이어서...

#### FAT 16디스크

-Reserved Region	: 부트 섹터(BPB)와 우리 커널이 들어있는 구역.	정문.
0번섹터엔 부트로더, 1번에서 199번섹터 kernel.bin 이 로드 되어 대기하는 곳,,,
reserved sectors = 200으로 설정했기때문에 ..

-FAT Region	: 클러스터들이 어떻게 연결되었는지 적힌 지도(FAT 테이블).	아파트 동.
보통 2개의 FAT 테이블이 있는데, 이는 복구를 위한거임.

-Root Directory	: 최상위 폴더(/)에 있는 파일들의 리스트.	아파트 호수
파일 이름, 확장자, 크기, 클러스터 위치 , 등등 정보가있는데, 여기서 parsing해서 이름을 찾아서 데이터 영역으로 들어갈 클러스터 번호를 찾는거죠. 

-Data Region	: 실제 파일 내용(텍스트, 바이너리 등)이 저장되는 구역.	실제 거주민.
2번 클러스터 부터 시작해서 0,1번은 이미 예약됨. 우리가 실제로 읽고 쓰는 파일의 알맹이들은 이곳에 저장됨.

#### 파일시스템 :

FAT16의 구조를 보자.
주소(Byte) :  00    01  |  02    03  |  04    05  |  06    07  | ...
            [ 바이트 0 ] [ 바이트 2 ] [ 바이트 4 ] [ 바이트 6 ]
-------------------------------------------------------
내용(값)   : [  0xFFF8  ] [  0xFFFF  ] [    10    ] [    11    ] ...
            (Cluster 0)  (Cluster 1)  (Cluster 2)  (Cluster 3)

파일시스템은 규칙일뿐입니다.
OS입장에서는 디스크는 그냥 0과 1 로 이루어진 데이터 덩어리입니다.
이때 파일 시스템 드라이버가 등장해서 아 이것은 FAT16 파일시스템이야 하고 해석해주는 인터페이스를 제공하는거죠.
그래서 파일시스템을 배열로 한겁니다 확장성을 위해서...머근데 저는 FAT16만 구현하긴함.

이구조 기억!
  FAT16 physical structure :
  (부트영역)Reserved -> FAT1
  FAT1 -> FAT2
  FAT2 -> RootDir
  RootDir -> Data

1. 스트리머이용해서 바이트 단위로 디스크로부터 데이터를 읽어올수 잇도록했음.

2. fat_header (BPB) : 디스크가 책이라면 BPB는 목차라고 생각하면됨. 하드디스크 바이너리 데이터를 1:1로 매핑하는 구조체죠 wiki os dev 참조.

3. FAT 테이블 : 연결형 리스트
핵심 : 파일이 어디에 흩어져 있는가?
fat_entry_pos = (현재 클러스터 번호) * 2, 왜 * 2? : FAT16이기 때문에 한 칸이 16비트니까.

4. 주소의 변환
CPU는 주소를 알고싶어하지만, FAT16은 클러스터 번호로 말함.
Sector = (Cluster - 2) * Sectors_per_cluster + root_directory.ending_sector_pos(Data area start) 만약 클러스터가 3이라면 3-2 는 1 이죠? 거기에 섹터수 곱하고 + data_area_start를해주면됨.
왜 Cluster - 2? : 0번과 1번은 예약되어있기때문.

5. 전용 스트리머 (Streamers)
우리 struct fat_private를 보면 스트리머이 하나가 아니라 여러 개(cluster_read_stream, fat_read_stream 등)입니다.

이유: 파일을 읽는 도중에 다음 클러스터를 찾으려고 FAT 테이블 주소를 뒤져야 할 때가 있는데, 이때 스트리머 하나만 쓰면 주소가 꼬여버리죠.
그래서 FAT 테이블을 읽는 스트리머와 실제 데이터를 읽는 스트리머를 분리했음.

6. 디렉터리 구조
* 루트 디렉터리
  위치: FAT 테이블 바로 뒤 (고정 위치).
  크기: 부트 섹터에 정의된 개수(Root Entries)만큼 고정됨. 더 늘릴 수 X.
* 서브 디렉터리
  위치: 데이터 영역 아무 데나 (파일이랑 똑같음).
  크기: 파일처럼 클러스터 체인으로 연결되므로 무한히 늘어날 수 있음.
  특수 속성(Directory Attribute, 0x10)을 가진 파일임니다~.


#### 페이징 :

** 가상메모리 (Virtual Memory) : 실제 물리 RAM은 작아도, 각 프로그램에게 4GB의 가상 메모리 할당.

** 메모리 보호(Isolation) : 유저 영역과 커널 분리 >> 이거중요함.

  cr3레지스터 : 현재 사용중인 페이지 디렉토리의 물리주소를 담고잇음.
  이거 중요함 ! context switching의 핵심임 .
  A프로세스의 디렉터리 주소에서 B프로세스 주소로 바꾸는순간,OS가 바라보는 메모리가 바뀌는거임.
  가상 주소 -> 물리 주소로 매핑하는 기술.
  Page Directory -> Page Table -> Physical Frame. 구조임.
  프로세스마다 4GB라는 가상 메모리를 가지게 됨.
  일단 내 OS 메모리구조는...config.h 참조하셈.
  0~256MB : 커널 영역.
  256~512MB : 유저 영역.
  256MB기준으로 커널 영역 유저영역을 나눈이유는 보안위해서임, 안그러면 유저에서 할당된 메모리가 커널영역으로 가버릴수도있음.

  페이징의 구조는...
  아 참고로 우리는 한페이지당 4kb블록 단위로 관리됨.
  총 1024 * 1024 * 4096 = 4gb 이다.
  1. Page Directory : 1024개 Entry가있는데 각 Entry가 Page Table의 물리주소를 담고있음.(대한민국)
  
  2. Page Table : 주소의 인덱스, (서울시)

  3. Page Table Entry : 페이지 인덱스(4byte) -> Page Frame의 물리주소를 담고있음. (거주지 동/구.)

  4. Page Frame(Physical Frame)(4kb) : 물리 메모리의 페이지. (실제 집 주소.)


#### 힙(HEAP)
  
  Heap Table :
  명심해라 우리는 4KB블록 단위로 관리한다.
  커널 힙 상태를 관리하는 배열.
  각 Entry가 4byte 크기의 메모리 블록을 나타냄.
  [ 7(NEXT)][ 6(FIRST)][ 5 ][ 4 ][ 3 ][ 2 ][ 1 ][ 0(TAKEN/FREE)] 나머지 비트들은 일단 사용안함, 추가가능함.
  
  malloc 호출 -> heap_calculate_required_blocks(size)로 필요한 블록 수 계산 -> 테이블 처음(Index 0)부터 시작해서 요청된 사이즈를 충족하는 연속된 빈 공간 찾음 -> 찾으면 heap_mark_blocks_as_taken() 호출해서 "taken"으로 마킹 -> 주소반환.

#### 인터럽트(Interrupt)

ring3 이 int0x80을 호출하면 ->
cpu는 TSS를 보고 ESP0(커널스택)으로 스택 포인터를 옮기고.
기존 위치(유저 스택, 주소 ,코드위치)를 커널 스택에 백업함.
그리고나서 (idt.asm 에서 Restore state참조) popad를 하고 iret하면 복구.
idt.asm보면,
참고로 pop은 esp를 +4 push랑은 다름 그리고 esp를 레지스터에 넣어서 복구시키는건데, 사실 이것도 처음알았다...
; Restore state
    popad          ;저장했던 EAX, EBX.. 등 8개 레지스터 한 방에 복구!
    pop gs         ;세그먼트 레지스터들도 하나씩꺼내
    pop fs         ;
    pop es         ;
    pop ds         ;
    add esp, 8     ;
    iret           ;Ring 3로 귀환

  1. IDT(Interrupt Descriptor Table):
  0~255 번까지 있는데 각 인터럽트에 대한 정보를 담고있음.
  몇 번 신호가 오면(ISR)로 점프!
  idt_init()으로 초기화.

  2. ISR(Interrupt Service Routine):
  실제 Interrupt를 실행하는 함수.
  이거 좀 복잡... 어셈블리 idt.asm참조하고 공통 처리를 한 뒤에 C함수 호출하는건데,,,
  일단 이것 짤대 nasm 어셈블리 문법 완전,,하 ㅠㅠ..눈물났음,아직도 잘 못함 그냥 os wiki짱.
  또 구조체 첫번째 가 사실은 스택의 최상위 주소를 가리키는거.. 즉 구조체 첫번째 멤버가 가장 낮은 주소임. 이부분도 몰라서 에러났었음.

  3. ISR80h
  eax레지스터에게 시스템콜 번호를 넣고 interrupt_128로 점프함.
  그리고 레지스터 pushad후 interrupt_handler로 점프함.
  interrupt_handler에서 eax 값 확인후 시스템골 처리 isr80h_handle_command확인.
  
 어떻게 데이터를 전달?
  - 유저 프로그램이 인자(문자열 포인터)를 스택에 push
  - 커널은 task_get_stack_item 함수를 사용해 유저 스택의 내용을 읽어옵니다.
  - 그리고 copy_from_task 를 사용해서 유저 -> 커널로 데이터 복사.
  중요! 커널이 잠시 유저의 페이지 디렉토리로 paging_switch하여 해당 메모리를 읽어온 후 다시 커널로 복귀하는거임. 잠깐 훔쳐보는...

  4. Hardware Interrupt
  I/O 버스(고속도로같은것)를 통해 포트에서 데이터를 읽어옴.
  키보드 누르면 IRQ 1번, -> IDT 0x21으로 점프하고
  포트 0x60(키보드 데이터포트)에서 데이터를 읽어옴.
  io.asm이 driver역할을함.
 
#### TASK
task는 thread같은 개념이라 생각하면됨.
그럼 어떻게 task가 process의 공유자원을 사용하게되나 이게 중요하다고 보거든요?
1. process 를 먼저 만들고.
2. init_task를 호출할때 4gb 메모리 디렉터리를 만듦. 이때 task->process = process;
3. 파일 데이터 로드 -> 물리 메모리에 로드.
4. 중요한것 :
process_map_memory는 가상메모리를 물리메모리에 매핑해서 물리메모리를 연결하는것.
(각 TASK의 개인 사유지?)
task 에는 페이지 디렉터리가 있음 그리고 process_map_binary 함수에서 페이지의 디렉터리에 process의 물리 주소를 매핑해버림 이 의미는 task가 여러개여도 process의 물리메모리공간을 공유할수있음.

#### Scheduling (Context Switching)

1. Scheduling
라운드로빈 방식으로 리스트의 next를 따라가며 다음 실행할 Task를 선택.
timer interrupt를 사용해서 10ms마다 preemptive(선점형)스케쥴링.
task가 양보하지않아도 os가 강제로 바꿈.
왜 선점형으로 구현했나?
어떤 프로그램이 버그에 빠져도 10ms 마다 강제로 바꾸니 안멈춤.
우선순위큐 구현해서 우선순위에 따라 스케줄링하는 것도 좋은 방법일듯.
지금은 그냥 로직만 구현해버림.
어떻게 priority를 구분하냐? -> 혼자 100ms 쓰면 priority 내려버림.
왜? 즉각 반응하게 하기위해서!

생각해볼 방식:
테스크 마다 고정적인 우선순위를 주어서 하는 방식도 생각해봄 -> 이런건 임베디드에서 중요할듯.
미리 예약하는것도 있을테고...

2. Context Switching
task_switch 와 task_return

3. 바꿔치기 CR3 Switch
paging_switch(task->page_directory);
CR3 레지스터에 새로운 Task의 페이지 디렉토리 주소를 넣습니다.
이 순간 CPU가 바라보는 가상 메모리 공간이 Task A의 세상 -> Task B의 세상으로 확 바뀝니다. (코드 영역과 16KB 스택이 교체됨)

4. 권한 복구 준비 TSS Update
tss.esp0 = new_task->kstack + 4096;
TSS (Task State Segment): CPU에게 "유저 모드(Ring 3)에서 인터럽트 걸리면, 커널 모드(Ring 0) 
* 각 Task마다 별도의 커널 스택을 가지므로, 스위칭할 때마다 TSS도 갱신해줘야  함!

#### PROCESS
   (메모리, 파일 등)을 포함하는 하우스(house)
   실제로 CPU를 차지하고 코드를 실행하는 일꾼입니다. (Context Switching의 대상)
   프로세스는 하나 이상의 태스크를 가집니다.
   지금은 1:1 관계...늘리면 멀티태스킹...
생성흐름은 process_load_for_slot -> process_load_data -> process_map_virtual_memory -> process_setup_arguments -> iret 이렇게.


#### ELF
ELF는 약간 지도라고 생각하면됨 코드는 여기 데이터는 저기 이런식으로 알려주는 것.
process_load_data -> elfloader_load_elf 흐름.

#### Sbrk (Memory Allocation)
유저 프로그램이 malloc을 하다가 힙 공간이 부족하면 `sbrk` 시스템 콜을 요청.
malloc 함수보면 알수있음.
프로세스의 `cur_end_heap`을 늘리고, 늘어난 공간만큼 실제 물리 페이지를 할당해서 매핑해줌.

#### 실행 방법
1. `./build.sh`를 실행하여 커널과 유저 프로그램을 컴파일.
2. `qemu-system-i386 -hda ./bin/myos.bin`을 실행하여 운영체제 구동.
3. 쉘에서 명령어 입력: `run 0:/bf.bin 0:/hello.bf`
아 hello.bf 는 
4. `run 0:/blank.bin` (빈 프로그램 실행 테스트)
5. `run 0:/waiter.bin` (부모-자식 프로세스 대기 테스트)
6. `run 0:/shell.bin` (shell 실행 테스트)

##### references
그냥 모를때마다 OsDev Wiki를 참고했고, 사실 구글링도 많이했는데,,, OSDev Wiki가 젤 편했고,
구글에 소스는 무진장 많음 대표적인 두곳만 소개하자면,,,
- **OSDev Wiki (https://wiki.osdev.org)**: OS 개발의 백과사전 같은 곳으로, 하드웨어 명세를 이해하는 데 필수적이고 구조체 짜는데 있어서 참조 많이함.
- **(https://m.blog.naver.com/simhs93/)**: 한글로 된 훌륭한 강의.
그외 더많은데 두가지를 가장 많이 참조했습니다.

느낀점...
정말 많지만 어셈블리를 배워야 했고 솔직히 아직까지도 잘 못짜지만, 이제는 코드를 읽고 흐름을 파악하며 필요한 기능을 구현할 수 있게 되었습니다.
특히 인터럽트라는 개념은 어느정도 알고있었는데, 어떻게 커널로 돌아가는지 tss를 구현하면서 배웠고.
또 task 구조체를 구현하면서 커널스택과 유저스택을 구분하는 부분도 배웠고.
어셈블리가 처음엔 외계어 같았지만, `boot.asm`부터 `kernel.asm`까지 한 줄씩 분석하며 하드웨어와 대화하는 법을 익힐 수 있었던 가장 소중한 경험이었습니다.
그리고 디버깅 gdb를 사용해서 커널을 디버깅하는 것도 정말 좋았다.
근데 makefile짜는건 솔직히 복잡하고 어려웠음.


#### PCI 버스 (Peripheral Component Interconnect)
메인보드에있음.
cpu가 0xCF8(PCI컨트롤러 주소 포트) 데스크에 0번 버스(101동) 1번 슬롯(1001호) 요청! -> outl
그리고 메인보드가 신호를 듣고 1번 0xCFC(PCI컨트롤러 데이터포트)에 응답을 보냄.
그러면 cpu가 메세지확인(inl)
즉, 컴퓨터 내부의 장치를 검색하고 연결하는 시스템.

랜카드, 그래픽카드... 얘네는 매일 주소가 바뀝니다.
왜?
plug and play 방식 일단 꽃으면 자리는 (BIOS/OS)가 비어있는 곳 찾아서 정해줄게! 이방식임.
장치가 추가 되도 유연한 방식임.
랜카드를 1번 슬롯에 꽂으면 101호, 2번 슬롯에 꽂으면 201호가 되는 거죠.
그래서 OS가 이 장치를 쓰려면 먼저 PCI 컨트롤러(0xCF8)에 물어봐야 합니다.

101호는 어떤방이죠? -> 없음(0xFFFF)
102호는 어떤방이죠? -> 리얼텍 랜카드(0x10EC)방입니다.

주소 획득:
그럼 개랑 대화하려면 어디로 가면 됩니까? -> Base Address0xC000 (BAR0) 으로 가세요~
통신 시작:
이제 0xC000번지로 데이터를 보내면 랜카드가 받음.
참고로 우리랜카드는 rtl8139씀.

#### rtl8139

rtl8139 네트워크 드라이버.
PCI버스를 통해 장치를 탐지하고 패킷 송수신 가능.
rtl8139.h 참조바람.
패킷 전송 (TX) - 4개 슬롯. 라운드로빈
패킷 수신 (RX) - WRAP모드 사용.
WRAP모드 : 패킷을 끊지 않고 연속으로 저장하는거 그래서 + 1500 했음. 구현이 간단하기때문에, 물론 메모리 더필요...

패킷 테스트하기 위해 wireshark 필요.
qemu-system-i386 -hda ./myos.bin -netdev user,id=net0 -device rtl8139,netdev=net0 -object filter-dump,id=f1,netdev=net0,file=dump.pcap

#### Ethernet 프레임 - 편지.
이더넷은 랜선으로 데이터를 전송하는 프로토콜이라 생각하면됨 그러니까,,,
LAN(Local Area Network) 내에서 데이터를 전송하는 프로토콜.
광역 X 지역.
와이파이랑 다름!
받는이 MAC Addr    보내는이 MAC Addr    편지종류 프레임 타입    데이터
 6byte +          6byte +             2byte +              maximum.1500byte

##### ARP(Address Resolution Protocol)인 경우 이구조.
 ------Ethernet Frame-------------
|                                |  
|    Ethernet Header(14bytes)    |
|    [dest 6bytes] [src 6bytes]  |
|    [type(0x0806) 2bytes]       |
|--------------------------------|  
|                                |
|    ARP Packet(28bytes)         |
|    [hardware type 2bytes]      |
|    [protocol type 2bytes]      |
|    [hardware len 1byte]        |
|    [protocol len 1byte]        |
|    [operation 2bytes]          |
|    [sender mac 6bytes]         |
|    [sender ip 4bytes]          |
|    [target mac 6bytes]         |
|    [target ip 4bytes]          |
|--------------------------------|

ARP : IP주소 -> MAC 주소변환. 왜? 이더넷은 MAC주소로 통신함. 즉 L2(layer)
연락처 앱으로 생각해 (우리는 무조건 이걸로  전화할때나 문자할때나 모든 통신에서 공유한다.)
근데 우리는 IP주소만 알아... ->MAC주소 알아내야 함!
아이거할때 리틀엔디안에서 빅엔디안으로 안바꿔서 작동안됨...


##### ip(Internet Protocol) 인경우
------Ethernet Frame---------------------
|    Ethernet Header(14bytes)           |
|    [dest 6] [src 6] [type 2(0x0800)]  |
----------------------------------------
|     IP header(20bytes)                |
     [version] [len] [ TTL] [protocol]  |  TTL : time to live 패킷의 유효기간!
|    [src ip4] [dest ip4]               |
|-------------------------------------- |
|     DATA (TCP/UDP/ICMP packet)        |
|    (크기 다양)                         |
----------------------------------------

IP :  실제 데이터 전송! L3(layer)
-출발지 ip
-도착지 ip
-프로토콜 (TCP, UDP, ICMP...)
-데이터(페이로드)


##### ICMP(Internet Control Message Protocol)
ping 프로토콜 : 연결이 되었는지 보내는 신호! ICMP패킷을 보냄 
ICMP : IP 데이터그램이 전송, 수신, 또는 라우팅에 실패했을 때의 정보를 전달하는 프로토콜.

bits.h :
네트워크 : 빅엔디안
cpu : 리틀엔디안
비트 순서가 달라서 ntohs, ntohl 함수를 사용합니다.
근데 이거 좀 테스트할때 많이 놓침... 그래서 wireshark로 확인할때 안뜨는 경우 종종 있었음.

##### TCP(Telecommunication Protocol)
little-endian 시스템에선 비트가 오른쪽에서 왼쪽으로 채워짐.
구조체 선언 순서와 실제 메모리 배치가반대!
ethernet | ip | tcp | data
14         20    20   max 1500


------ethernet frame------
|                        |
|     ----packet-----    |
|     |             |    |
|     |   segments  |    |
|     |             |    |
|     ---------------    |
|-------------------------

segments = TCP 헤더 + 데이터
패킷 = IP 헤더 + 세그먼트 
프레임 = Ethernet 헤더 + 패킷

###### 어떻게 연결?

###### 3-way-handshake
데이터 없음.
연결만 확인.

###### Debugging
wireshark 이용.
seq 와 ack 를 보며 잘 전달이 되었는지 확인가능.

난 HTTP와 연결할때 buffersize를 너무 작게 설정해서 에러가 났었음.
그리고 또한 호스트(리틀엔디안)와 네트워크(빅엔디안) 비트 순서가 다르기때문에 ntohs, ntohl 함수를 사용해야 하는데 이것도 꼼꼼하게 확인해야함 여기서 많이 틀렸음.
