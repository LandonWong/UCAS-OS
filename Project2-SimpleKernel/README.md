# Project #2 (P2) A Simple Kernel Design in Operating System Seminar, UCAS

## wangsongyue18@mails.ucas.ac.cn

## Introduction

In this repo, I designed the operating system kernels (**C-core**) with the following functions in turn, and their locations are also given below:

| Task | Schedule method | Schedule Algorithm                           | Support Process/Thread Type | Lock         | Block Support         | Internal Exception Handle | External Interrupt Handle |
| ---- | --------------- | -------------------------------------------- | --------------------------- | ------------ | --------------------- | ------------------------- | ------------------------- |
| #1   | Non-Preemptive  | Round Robin                                  | Kernel                      | -            | -                     | -                         | -                         |
| #2   | Non-Preemptive  | Round Robin                                  | Kernel                      | Kernel Lock  | Lock blocked          | -                         | -                         |
| #3   | Non-Preemptive  | Round Robin                                  | Kernel & User               | Kernel Lock  | Lock  & sleep blocked | Syscall                   | -                         |
| #4   | Preemptive      | Priority Scheduling<br>(Based on time slice) | Kernel & User               | Process Lock | Lock  & sleep blocked | Syscall                   | Timer Interrupt           |

Task #1 and #2 was put in branch `p2-part1`, task #3 was put in branch `p2-task3`, and task #4 was put in `master`.
## Implementation
#### Memory Layout
This time we just use direct memory mapping. Memory we used can be divided into parts shown below:
```

0x50200000      0x50500000                         0x52000000        0x600000000
     +---------------+----------------------------------+------------------+
     |     Kernel    |                                  |                  |
     |      Code     |        Process Stack zone        |       Heap       | 
     |       and     |   each stack contains 4K space   |   For kmalloc()  |
     |      data     |                                  |                  |
     +---------------+----------------------------------+------------------+

```
* Kernel Code and Data: This is where kernel preserved and it will not be able to access in user process.
* Process Stack
  * We provide two stacks (one user stack and one kernel stack) for each process including kernel thread. Kernel size was fixed as one page (4KB).
  * The spatial order of different process stacks is as follows
    ```

      0x5050   0x5050   0x5050   0x5050   0x5050   0x5050
        0000     1000     2000     3000     4000     5000
         +--------+--------+--------+--------+--------+-----+--------+
         | Kernel | Proc#1 | Proc#1 | Proc#2 | Proc#2 |     |        |
         | Thread | KERNEL | USER   | KERNEL | USER   | ... |        |
         | Stack  | Stack  | Stack  | Stack  | Stack  |     |        |
         +--------+--------+--------+--------+--------+-----+--------+

    ```
  * In each kernel stack, the first (highest) 288B was trapframe (`reg_context_t`), which was used to save all 32 GPRs and CSRs. And  the 112B below was to save switch_to context (14 callee preserved registers)
* Heap: Kernel thread can use `kmalloc()` to acquire space in heap zone. In task 4, process binsems were dramatically allocated, and they will be stored at here.
#### Non-Preemptive Kernel
* **Process Scheduler** (`/kernel/sched/sched.c`)
  This kernel maintained a `ready_queue` which contains all process (`PCB`) in order. This time we use RR algorithm, each time choose the header of this queue and switch to it
* **Process Switch (Kernel Process/Thread)** (`/arch/riscv/kernel/entry.S`)
  We only saved 14 callee preserved registers because the control is handed over voluntarily by the process.
* **Kernel Spin & Mutex Lock ** (`/kernel/locking/lock.c`)
  We use atomic instructions to realize spin lock. And for mutex lock, each lock has a `block_queue`, when acquire lock failed, the process control block which current running will be added in `block_queue` through `do_block()` method. And when other processes release this lock, it will call `do_unblock()` method to re-add the block PCB to `ready_queue`.
* **Environment Call Setup**
  * Supported types of  env-call were shown below (`init/main.c`)
    | Syscall Number | Description            |
    | -------------- | ---------------------- |
    | 2              | sleep                  |
    | 10             | get binsem             |
    | 11             | lock or unlock binsem  |
    | 20             | write data to console  |
    | 21             | read data from console |
    | 22             | move cursor            |
    | 23             | screen reflush         |
    | 24             | call do scheduler      |
    | 30             | get timebase           |
    | 31             | get tick now           |
  * Their corresponding functions for user was defined in `/tiny_libc/syscall.c`.
#### Preemptive Kernel
* **Sleep** (`/kernel/sched/sched.c`)
  The kernel maintains a `sleep_queue`. When user process call sleep env call, it will be put in `sleep_queue` and re-do_schduler.
* **Timer interrupt** (`/kernel/irq/irq.c`)
  * We use `sbi_set_timer()` in SBI (Supervisor binary interface) to control interrupt.
  * When timer interrupt occurs, we do things below
    * Change current running (do_scheduler)
    * Refresh serial port tx buffer
    * Check all sleep tasks
    * Reset timer
## Usage
* Compile and run
  * For compiling all, please execute `make all` in the top directory.
  * For write image into SD-card, please execute `make floppy` and remember to modify `DISK` macro in `Makefile` to your own SD-card symbol.
  * For running in QEMU, please execute `make qemu` and remember to change the `QEMU_PATH` option in `Makefile`.
  * For running in QEMU and debugs with gdb, please execute `make qemu-gdb` and remember to change the `QEMU_PATH` option in `Makefile`.
* **Test & Check Shortcut**
  * I provide 4 testbench in `/init/main.c`, you just need to uncomment one define.
  * I provide 2 scheduler algorithm (RR and Priority) in `/kernel/sched/sched.c` , you just need to uncomment one define. 
## Acknowledgements & References
* Acknowledgements
  * Thanks to TA Luming Wang, he helps me fix a lot of bugs and support some critical information and thoughts to me.
  * Thanks to many classmates, they gave me so much peer-pressure and make me finished it quickly.
* References 
  * [Raspberry Pi-OS Lesson: Scheduler](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/docs/lesson04/rpi-os.md)
  * 三十天自制操作系统 [日] 川和秀实
  * [ICS2019, Nanjing University](https://nju-projectn.github.io/ics-pa-gitbook/ics2019/)
  * [Xv6-public, MIT](https://github.com/mit-pdos/xv6-public)
  * RISC-V Chinese Reader. CRVA, ICT (中国开放指令集联盟，中科院计算所)
  * RISCV-Privileged-ISA  Handbook, Berkeley