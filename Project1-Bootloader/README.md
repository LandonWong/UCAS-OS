# Project #1 (P1) Bootloader and Image Creator Design in Operating System Seminar, UCAS
## wangsongyue18@mails.ucas.ac.cn
## Introduction
In this repo, I implemented a simple bootloader and image merge tool. The bootloader can copy the system kernel of any size from the second partition of the SD card and thereafter, and put the kernel to the memory starting address through bootloader relocation. The mirror merge tool can merge multiple ELF files of any size (segment) into an image file, which can be written to an SD card for testing.
## File Structure
```
 Project 1
    ├── Makefile                 # Makefile: all compile, disk-flashing commands 
    ├── P1_RISCV_Guidebook.pdf   # Guidebook of Project 1
    ├── Project1-report.pdf      # Report of Project 1
    ├── bootblock.S              # Phase 1: Bootloader
    ├── core
    ├── createimage.c            # Phase 3: Image merge tool
    ├── elf2char
    ├── generateMapping
    ├── head.S                   # Phase 2: Prepare environment for kernel operation
    ├── riscv.lds                # Linker script
    ├── kernel.c                 # Phase 3: A simple OS kernel for testing
    ├── README.md                # Documentation
    └── include                  # SBI_CALL Function Package
        ├── asm
        │   ├── sbiasm.h         # Marco ecall instructions
        │   └── sbidef.h         # Ecall number definition
        └── sbi.h                # SBI Function for C
```
## Usage
* Compile & make image: `make all`
* Write image to SD card: `make floppy`
## Component
### Bootblock (Phase 1)
* File: `bootblock.S`
* Position in SD card: Sector 0
* Function
   * (BIOS(BBL) copy the Sector 0(Bootblock) to memory `0x50200000`)
   * **Relocation**: Re-copy sector 0 of the SD card to `0x5a000000`
   * **Relocation**: Let PC jump to `0x5a000032` (first instruction after relocation)
   * Copy kernel to `0x50200000`
   * Let PC jump to `0x50200000`, running kernel
* Final memory layout
   ```
   0x50200000        0x50200202        0x5a000000    0x5a000082   0x60000000
        +-----------------+-----------------+-------------+------------+
    ... |      Kernel     |       ...       |  bootblock  |            | ...
        +-----------------+-----------------+-------------+------------+
   ```
### Kernel (Phase 2)
* File: `head.S` & `kernel.c`
* Position in SD card: Sector 1 & 2
* Function:
   * `head.S`
      * Clear the section `.bss`
      * Setup stack point(register `sp`) as `0x50500000`
      * Jump to main at `kernel.c`
   * `kernel.c`
      * Check whether `.bss` section has been clear (through checking global variables)
      * Get char from console and echo ic
### Createimage (Phase 3)
* File: `createimage.c`
* Usage: `./createimage --extended <file1> <file2> ...`
* Function:
   * Foreach ELF binary files
      * Get ELF header and find all program segments
      * Write all segments to `image`
   * Write `#sector(kernel)` to `<image> + 0x1fc` 
   * Print infos to terminal
## Acknowledgement & References
* **Acknowledgement**\
Thanks to TA Luming Wang for giving me a deeper understanding of the experimental framework and the hardware environment.\
Thanks to my roommate Ao Liu and Tingyu Liu, who help me found some details & bugs in this lab.
And also thanks to my partner Yunlong Zhao, who helps me prepare the design review and slides a lot.
* **References**
   * P1_RISCV_Guidebook, ICT & UCAS
   * [RISC-V-Reader-Chinese-v2](http://crva.ict.ac.cn/documents/RISC-V-Reader-Chinese-v2p1.pdf), CRVA, ICT
   * [ELF文件格式解析](https://zhuanlan.zhihu.com/p/52571826), zhihu