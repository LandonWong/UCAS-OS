#include <sys/syscall.h>
#include <stdint.h>
#include <mailbox.h>

long sys_net_recv(uintptr_t addr, size_t length, int num_packet, size_t* frLength)
{
    return invoke_syscall(SYSCALL_NET_RECV, addr, length, num_packet, frLength);
}
void sys_net_send(uintptr_t addr, size_t length)
{
    invoke_syscall(SYSCALL_NET_SEND, addr, length, IGNORE, IGNORE);
}
void sys_net_irq_mode(int mode)
{
    invoke_syscall(SYSCALL_NET_IRQ_MODE, mode, IGNORE, IGNORE, IGNORE);
}

pid_t sys_exec(const char *file_name, int argc, char* argv[], spawn_mode_t mode)
{
        return invoke_syscall(SYSCALL_EXEC, (uintptr_t)file_name, argc, (uintptr_t)argv, mode);
}

void sys_show_exec()
{
        invoke_syscall(SYSCALL_SHOW_EXEC, IGNORE, IGNORE, IGNORE, IGNORE);
}

void* shmpageget(int key)
{
        return invoke_syscall(SYSCALL_SHMPGET, key, IGNORE, IGNORE, IGNORE);
}

void shmpagedt(void *addr)
{
        invoke_syscall(SYSCALL_SHMPDT, (uintptr_t)addr, IGNORE, IGNORE, IGNORE);
}

void sys_sleep(uint32_t time)
{
    invoke_syscall(SYSCALL_SLEEP, time, IGNORE, IGNORE,IGNORE);
}

void sys_scheduler()
{
    invoke_syscall(SYSCALL_YIELD, IGNORE, IGNORE, IGNORE,IGNORE);
}

void sys_write(char *buff)
{
    invoke_syscall(SYSCALL_WRITE, (uintptr_t)buff, IGNORE, IGNORE,IGNORE);
}

void sys_reflush()
{
    invoke_syscall(SYSCALL_REFLUSH, IGNORE, IGNORE, IGNORE,IGNORE);
}

void sys_move_cursor(int x, int y)
{
    invoke_syscall(SYSCALL_CURSOR, x, y, IGNORE,IGNORE);
}

int binsemget(int key)
{
    return invoke_syscall(SYSCALL_SEMGET, key, IGNORE, IGNORE,IGNORE);
}

int binsemop(int binsem_id, int op)
{
    return invoke_syscall(SYSCALL_BINSEMOP, binsem_id, op, IGNORE,IGNORE);
}

long sys_get_timebase()
{
    return invoke_syscall(SYSCALL_GET_TIMEBASE, IGNORE, IGNORE, IGNORE,IGNORE);
}

long sys_get_tick()
{
    return invoke_syscall(SYSCALL_GET_TICK, IGNORE, IGNORE, IGNORE,IGNORE);
}

pid_t sys_spawn(task_info_t *info, void* arg, spawn_mode_t mode,uint8_t mask){
    return invoke_syscall(SYSCALL_SPAWN, info, arg, mode,mask);
}

pid_t sys_getpid()
{
    return invoke_syscall(SYSCALL_GETPID,IGNORE,IGNORE,IGNORE,IGNORE);
}

void sys_clear(){
    invoke_syscall(SYSCALL_SCREEN_CLEAR,IGNORE,IGNORE,IGNORE,IGNORE);   
}

void sys_ps(){
    invoke_syscall(SYSCALL_PS,IGNORE,IGNORE,IGNORE,IGNORE);   
}
void sys_exit()
{
    return invoke_syscall(SYSCALL_EXIT,IGNORE,IGNORE,IGNORE,IGNORE);    
}

int sys_waitpid(pid_t pid)
{
    return invoke_syscall(SYSCALL_WAITPID, pid,IGNORE,IGNORE,IGNORE);
}

int sys_kill(pid_t pid)
{
    return invoke_syscall(SYSCALL_KILL, pid,IGNORE,IGNORE,IGNORE);    
}

int getchar(){
    int ch;   
    while(1)
    {
        ch = invoke_syscall(SYSCALL_SERIAL_READ, IGNORE,IGNORE,IGNORE,IGNORE);
        if (ch != -1)
            return ch;
    }
    return ch;
}

void putchar(int ch)
{
    invoke_syscall(SYSCALL_SERIAL_WRITE, ch, IGNORE,IGNORE,IGNORE);
}

void sys_block(pcb_t *pcb_node, list_head *queue)
{
    invoke_syscall(SYSCALL_DOBLOCK, pcb_node, queue, IGNORE,IGNORE);
}

void sys_unblock(list_node_t *item)
{
    invoke_syscall(SYSCALL_DOUNBLOCK, item, IGNORE, IGNORE,IGNORE);
}
void sem_signal(int id)
{
    invoke_syscall(SYSCALL_SEMSIGNAL, id, IGNORE, IGNORE,IGNORE);
}
void sem_wait(int id, int lock_id)
{
    invoke_syscall(SYSCALL_SEMWAIT, id, lock_id, IGNORE,IGNORE);
}
void sem_broadcast(int id)
{
    invoke_syscall(SYSCALL_SEMBROADCAST, id, IGNORE, IGNORE,IGNORE);
}

void barrier_wait(int id, int count)
{
    invoke_syscall(SYSCALL_BARRIERWAIT, id, count, IGNORE,IGNORE);
}

mailbox_t sys_mbox_open(char *name)
{
    return invoke_syscall(SYSCALL_MBOX_OPEN, name, IGNORE, IGNORE,IGNORE);
}

void sys_mbox_close(mailbox_t mailbox)
{
    invoke_syscall(SYSCALL_MBOX_CLOSE, mailbox, IGNORE, IGNORE,IGNORE);
}
void sys_mbox_send(mailbox_t mailbox, void *msg, int msg_length)
{
    invoke_syscall(SYSCALL_MBOX_SEND, mailbox, msg, msg_length,IGNORE);
}
void sys_mbox_recv(mailbox_t mailbox, void *msg, int msg_length)
{
    invoke_syscall(SYSCALL_MBOX_RECV, mailbox, msg, msg_length,IGNORE);
}
void sys_scroll()
{
    invoke_syscall(SYSCALL_SCREENSCROLL, IGNORE, IGNORE, IGNORE,IGNORE);
}
void sys_taskset(uint8_t mask, pid_t pid)
{
    invoke_syscall(SYSCALL_TASKSET, mask,pid, IGNORE,IGNORE);    
}
