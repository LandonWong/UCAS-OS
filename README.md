# Operating System Lab in UCAS (B0911011Y), Fall 2020
## wangsongyue18@mails.ucas.ac.cn
## Introduction
* Lab Environment: Xilinx PYNQ-Z2 (PL: two [Rocket-chips](https://github.com/chipsalliance/rocket-chip))
* Architecture: RISCV64
## Notice
本仓库为中国科学院大学：操作系统（研讨课，RISC-V版本）的开源代码。本仓库完成了从Project #1到#6的所有C-core设计。\
请注意：抄袭代码会对您的成绩造成影响。\
项目托管在[Gitee仓库](https://gitee.com/landonwong/UCAS_OS)。
## Project6: File System
**For more description, please click [Project6.readme](https://gitee.com/landonwong/UCAS_OS/blob/master/Project6-FileSystem/README.md)**
### Schedule
(2020-12-22) Finish S core design.\
(2020-12-26) Finish A core design.\
(2021-01-05) Finish C core design ^ _ ^
## Project5: Device Driver (C-core)
**For more description, please click [Project5.readme](https://gitee.com/landonwong/UCAS_OS/blob/master/Project5-DeviceDriver/README.md)**
### Schedule
(2020-12-08) Transplant drivers from the Xilinx official driver.\
(2020-12-10) Finish polling send and recv design.\
(2020-12-11) Finish blocked send and recv and parallelization design.
### Contents
* Phase 1: Driver transplant
* Phase 2: Polling send and recv design
* Phase 3: Blocked send and recv and parallelization design
## Project4: Virtual Memory Management (C-core)
**For more description, please click [Project4.readme](https://gitee.com/landonwong/UCAS_OS/blob/master/Project4-VirtualMemoryManagement/README.md)**
### Schedule
(2020-11-28) Finish phase1 design.\
(2020-12-01) Finish phase2 design.\
(2020-12-02) Finish all tasks design.
### Contents
* Phase 1: Kernel vPage Management
    * Task1: Setting up kernel virtual memory mapping
    * Task2: Start kernel process
* Phase 2: User Process vPage Management
    * Task3: User pgdir management and switch when scheduling
    * Task4: Page missing handling
    * Task5: Page allocation on demand
* Phase 3: Advanced page management & Dual-core Support
    * Task6: Share Memory
    * Task7: Page replacement
    * Task8: Dual-core support
## Project3: Interactive OS and Process Management (C-core)
**For more description, please click [Project3.readme](https://gitee.com/landonwong/UCAS_OS/blob/master/Project3-InteractiveOS_ProcessManagement/README.md)**
### Schedule
(2020-11-05) Finish phase1 design.\
(2020-11-07) Finish phase2 design.\
(2020-11-12) Finish all tasks design.
### Contents
* Phase 1: Interactive shell design
    * Task1: Shell interaction
    * Task2: Process related method : `exec`, `ps`, `kill`, `exit`
    * Task3: Process mutex for user
* Phase 2: Synchronization primitives
    * Task4: Semaphore
    * Task5: Barrier
    * Task6: Mailbox
* Phase 3: Multi-core support
## Project2: A Simple kernel (C-core)
**For more description, please click [Project2.readme](https://gitee.com/landonwong/UCAS_OS/tree/master/Project2-SimpleKernel/README.md)**
### Schedule
(2020-10-12) Finish part1 design.\
(2020-10-27) Finish part2 design.
### Contents
* Phase 1: Non-Preemptive OS
    * Task 1: Task scheduler
    * Task 2: Process lock
* Phase 2: Preemptive OS with interrupt handler
    * Task 3: Interrupt helper and env call
    * Task 4: Preemptive OS design
## Project 1: Boot Loader & Image Creator (C-core)
**For more description, please click [Project1.readme](https://gitee.com/landonwong/UCAS_OS/tree/master/Project1-Bootloader/README.md)**
### Schedule
(2020-09-20) Already finished **C-core** design.
### Contents
All codes and docs are put in subdirectory `UCAS_OS/Project1-Bootloader`.
* Phase 1: Try to add instructions in bootblock.
* Phase 2: Finish bootblock and relocate the kernel.
* Phase 3: Realize the process of merging two ELF files into one image.


