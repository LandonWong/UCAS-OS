# Project #4 (P4) Virtual Memory Management in Operating System Seminar, UCAS
## wangsongyue18@mails.ucas.ac.cn
## Introduction
(**C-core**) \
In this version of UCAS OS, I added virtual memory management to the kernel. For the operating system kernel, I use sv39 virtual address and use secondary page table mapping. For user processes, I use sv39 virtual address and three-level page table mapping.\
In this kernel, each process has its own page directory, which can realize the isolation of address space between processes, which makes UCAS OS more secure. At the same time, I also implemented some advanced memory management methods, such as inter process shared memory, page recycling and page replacement based on clock algorithm.\
In order to get closer to the real operating system and make up for the lack of a file system, I convert the user process program to ELF format. Each time I start a process, it is realized by parsing elf strings. Instead of being like before, a process is a function.
## Usage
### Testing
* User Process Testing: `fly`
* Auto-allocpage Testing: `rw`
* Share Memory Testing: `consensus`
* Page swap/replace Testing: `swap`
### Booting
* **Booting** \
    `loadboot` for single-core and `loadbootm` for dual-core.
### Supported shell command
* **help**
    * Description: Display all support shell commands with description and usage.
    * Usage: `help`
    * Note: HELP command has no args.
* **ls**
    * Description: Display all executable ELF program.
    * Usage: `ls`
    * Note: PS command has no args.
* **exec**
    * Description: Execute one task and set its exit mode.
    * Usage: `exec <proc_name (argv[0])> <argv[1]> <argv[2]> <argv[3]>`
    * Note: `<proc_name>` can be `swap`, `fly`, `rw` and `consensus`.
* **ps**
    * Description: Display all alive processes, with some infomations (PID, cpu-mask, status).
    * Usage: `ps`
    * Note: PS command has no args, and shell's pid is always 1.\
* **kill**
    * Description: Kill one process.
    * Usage: `kill <pid>`
    * Note: Release it's lock and recover stack space.
* **clean**
    * Description: Clean and reflush the screen.
    * Usage: `clean`
    * Note: CLEAN command has no args.
* **taskset**
    * Description: Set one process always execute at some core
    * Usage: `taskset -p <mask> <pid>`
    * Note: mask can be 0x1, 0x2 or 0x3
## Virtual Memory Configuration
### kernel Process
* **Space Allocation**\
 For kernel process, we use linear memory mapping to map high address to all usable address space (`0x50000000`~`0x60000000`), and the offset is `0xffffffc0000000000`.\
 Kernel page directory and PTEs are put at phyaddr `0x5e000000`.
    ``` 
     Phy addr    |   Memory    |     Kernel vaddr

    0x5f000000   +-------------+  0xffffffc05f000000
                 | Kernel PTEs | 
    0x5e000000   +-------------+  0xffffffc05e000000
                 | Kernel Heap | 
    0x5d000000   +-------------+  0xffffffc05d000000
                 |   Free Mem  |
                 |     Pages   | 
                 |  for alloc  | 
    0x51000000   +-------------+  0xffffffc051000000
                 |     ...     | 
    0x50500000   +-------------+  0xffffffc050500000
                 |   Kernel    |
                 |   Segments  |
    0x50400000   +-------------+  0xffffffc050400000
                 | vBoot Setup |
                 |   (boot.c)  |
    0x50300000   +-------------+  0xffffffc050300000
                 |     ...     |
    0x50201000   +-------------+  N/A
                 |  Bootblock  |
    0x50200000   +-------------+  N/A
    ```
* **Page Mapping methods**\
    I use riscv sv39 virtual address and secondary page table to map the kernel space. The page size is 2MB. The format of the virtual address is as follows.
    ```
       +---------+---------+--------------------------+
       |   VPN1  |   VPN0  |          OFFSET          |
       +---------+---------+--------------------------+
     38        30 29     21 20                       0 
    ```
    Note that we do not set the `USER` bit of PTE, which means when processor are at U-level does not have access to this address space.
### User Process
* **Kernel Mapping**\
    In order to allow user processes to access the full kernel when they are stuck in the kernel, I also copied the above mapping to each user's page directory.
* **User Page**
    * ELF Segments\
    We use `load_elf()` method with `alloc_page_helper()`. Every time we resolve the virtual address of a segment of ELF file, we use `alloc_page_helper()` to allocate a page in physical memory and map the virtual address to it.
    * User Stack is fixed at `0xf00010000`.
    So for user process, it sees the virtual memory layout as follows.
    ``` 
    0xffffffc000000000   +-------------+ 
                         |     ...     |
           0xf00011000   +-------------+ 
                         |  User Stack | 
           0xf00010000   +-------------+  
                         |     ...     | 
               0x20000   +-------------+ 
                         |   Program   |
                         |   Segments  |
               0x10000   +-------------+ 
                         |     ...     |
                   0x0   +-------------+ 
    ```
### Page Replacement
 We limit each process to a maximum of four physical page frames.\
 When the physical page frame is equal to 4, and then apply for the page frame, we will use the **clock** algorithm to select a page and write it to the SD card for saving.\
 When this page is accessed again, we will recopy it from SD card to memory.\
 You can use test ELF file `swap` for testing.
