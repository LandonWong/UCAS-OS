# Project #6 (P6) File System in Operating System Seminar, UCAS
## wangsongyue18@mails.ucas.ac.cn
## C-Core测试说明
本仓库是UCAS_OS的最终版本，可以支持P6 C-Core的测试。
### 测试说明
P6 C-Core测试过程主要分为网络传输ELF文件并写入文件系统，测试程序从文件形式的启动，线程的支持，以及完整进程间通信和共享内存的支持。
### 与测试相关的进程
| 测试进程名 | 说明 |
|:--:|:--|
| ftprecv | 进程接收网络传来的数据，通过Mailbox发送给ftpfile进程 |
| ftpfile | 将Mailbox的内容写入文件系统 |
| race | （网传至文件系统）从右向左飞的进程，随机步长，顺序写入文件和共享内存 |
| back | （网传至文件系统）从左向右飞的进程，根据共享内存中的顺序，确定不同优先级 |
### 测试启动过程
#### UCAS_CORE接收网传用户进程ELF文件
可以通过`ftprecv`进程接收网络传来的数据，并通过mailbox发送至`ftpfile`进程，写入文件系统。
*   启动进程\
    在shell中执行`exec ftprecv <file_name> <packet_number>`来接收文件。这个操作会先启动`ftprecv`进程，再启动`ftpfile`进程。
*   启动`pktRxTx`程序\
    两个进程将会显示剩余接收包的数目。

注意：race测试程序有140个包（每个数据包512字节），back测试程序有136个包（每个数据包512字节）。
因此，您只需输入以下命令：
*   接收`race`\
    `exec ftprecv race 140`
*   接收`back`\
    `exec ftprecv back 136`
*   可通过`ls`命令查看这两个文件
#### 网络传输用户进程ELF文件
可以通过`pktRxTx`小程序传输我们需要的进程的ELF文件。
*   桥接网卡\
    请为虚拟机添加一张桥接至物理机物理LAN的网卡。
*   启动`pktRxTx`程序\
    进入`./pktRxTx`目录，执行脚本`./autorun`。脚本会申请`root`权限，开启桥接网卡（请将脚本中`enp0s8`换成本机桥接网卡名）并自动进入发送模式。
*   发送ELF文件\
    选择桥接的网卡。输入`send <packet_number>`，按提示输入文件名即可。

注意：race测试程序有140个包（每个数据包512字节），back测试程序有136个包（每个数据包512字节）。
#### 运行测试程序
我为运行文件系统中的ELF文件提供了`sys_run`接口（对应shell命令`run`）
*   `run <file_name> <elf_length> <argv[1]> ...`
*   运行`race`：在根目录执行 `run race 70816 1 1`
