# Project #5 (P5) Device Driver in Operating System Seminar, UCAS
## wangsongyue18@mails.ucas.ac.cn
## Introduction
In this repo, I transplanted the Gigabit Ethernet Controller (GEM) on the PS of the Xilinx PYNQ board to UCAS OS. It supports the sending and receiving of TCP/IP packets, polling and network interruption (SEIE). At the same time UCAS OS also supports synchronous full-duplex transmission and reception.
## File structure
```
    .
    ├── drivers
    │     ├── emacps                # Drivers of GEM
    │     │     ├── xemacps_main.c  # GEM: RX TX main driver
    │     │     └── ...
    │     ├── net.c                 # GEM Drivers-to-kernel interface
    │     ├── plic.c                # Platform-Level Interrupt Controller Driver
    │     └── ...
    └── ...    
```
## kernel Infomation
### Memory Layout
Since RISC-V uses a unified memory mapping, I mapped the I/O to the high bits of the kernel address `0xffffffe000000000`.\
Note that because RISC-V cannot set a certain segment of memory to be uncached, d-cache must be refreshed for each I/O operation.
``` 
     Phy addr    |   Memory    |     Kernel vaddr

        N/A      +-------------+  0xffffffe002000000
                 | I/O  Mapped | 
        N/A      +-------------+  0xffffffe000000000
                 |     ...     | 
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
### Function Interface
* `EmacPsSetupRxBD`: reset rx' BD ring 
* `EmacPsSetupTxBD`: reset tx' BD ring 
* `EmacPsSend`: send Ethernet frame to HW
* `EmacPsWaitSend`: wait response of sending Ethernet frame to HW 
* `EmacPsRecv`: recv Ethernet frame to HW
* `EmacPsWaitRecv`: wait response of recieving Ethernet frame to HW 
* `ReadRXSR`: read GEM register RXSR
* `ClearRXSR`: clear GEM register RXSR' complete bit
* `ReadTXSR`: read GEM register TXSR
* `ClearTXSR`: clear GEM register TXSR' complete bit
## Reference
* [Xilinx embeddedsw official driver: emacps](https://github.com/Xilinx/embeddedsw/blob/master/XilinxProcessorIPLib/drivers/emacps/)
* [ug585-Zynq-7000-TRM](https://www.xilinx.com/support/documentation/user_guides/ug585-Zynq-7000-TRM.pdf)
* RISC-V Spec Vol.2 Privileged Arch v1.10