# Project #3 (P3) An Interactive OS Design and Process Management in Operating System Seminar, UCAS
## wangsongyue18@mails.ucas.ac.cn
## Introduction
In this repo or edition of UCAS OS, I realized an os kernel with following characteristics:
* (S-core) **Interactive user shell design**
* (A-core) **Synchronization and communication**
    * Synchronization among threads (Condition variables and Barrier)
    * Inter process communication (IPC) (Mailbox)
* (C-core) **Dual core support** (Dual rocket chip)
    * Synchronization and communication on DUAL CORE
    * `taskset`: Supported binding one task to a certain core
## Supported shell command
* **help**
    * Description: Display all support shell commands with description and usage.
    * Usage: `help`
    * Note: HELP command has no args.
* **exec**
    * Description: Execute one task and set its exit mode.
    * Usage: `exec <task_id> <exit_mode>`
    * Note: <exit_mode> has two options: 0 for `EXIT-ON-ZOMBIE`, 1 for `AUTO-CLEAN-UP`.
* **ps**
    * Description: Display all alive processes, with some infomations (PID, cpu-mask, status).
    * Usage: `ps`
    * Note: PS command has no args, and shell's pid is always 1.
* **kill**
    * Description: Kill one process.
    * Usage: `kill <pid>`
    * Note: Release it's lock and recover stack space.
* **clean**
    * Description: Clean and reflush the screen.
    * Usage: `clean`
    * Note: CLEAN command has no args.
## Usage
* **Compile and floppy**: `make all && make floppy`\
    Please modify corresponding SD card label in `./Makefile`.
* **Run on PYNQ-Z2**\
    Set your board to `SD-card` boot mode. And set micro-usb function to `UART`.
* **Boot**
    * Boot with singal core `loadboot` (For both u-boot and Berkeley Boot Loader(BBL))
    * Boot with dual core `loadbootm` (For both u-boot and Berkeley Boot Loader(BBL))
* **Using**\
    When display following command prompt, please execute shell command for free.
    ```
    
    (This part of screen is designed for user processes/threads' display)

    ............................................................
    [root@UCAS_OS / #] 

    (This part of screen is designed for shell interaction)

    ```
    **Note**: Shell supports **auto-scrolling** but it is a waste of time. You can use `clear` command for fast scrolling.