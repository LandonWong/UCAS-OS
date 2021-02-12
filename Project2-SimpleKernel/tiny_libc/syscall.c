#include <sys/syscall.h>
#include <stdint.h>

void sys_sleep(uint32_t time)
{
    invoke_syscall(SYSCALL_SLEEP, time, IGNORE, IGNORE);
}

void sys_scheduler()
{
    invoke_syscall(SYSCALL_SCHED, IGNORE, IGNORE, IGNORE);
}

void sys_write(char *buff)
{
    invoke_syscall(SYSCALL_WRITE, (uintptr_t)buff, IGNORE, IGNORE);
}

void sys_reflush()
{
    invoke_syscall(SYSCALL_REFLUSH, IGNORE, IGNORE, IGNORE);
}

void sys_move_cursor(int x, int y)
{
    invoke_syscall(SYSCALL_CURSOR, x, y, IGNORE);
}

int binsemget(int key)
{
    return invoke_syscall(SYSCALL_BINSEMGET, key, IGNORE, IGNORE);
}

int binsemop(int binsem_id, int op)
{
    return invoke_syscall(SYSCALL_BINSEMOP, binsem_id, op, IGNORE);
}

long sys_get_timebase()
{
    return invoke_syscall(SYSCALL_GET_TIMEBASE, IGNORE, IGNORE, IGNORE);
}

long sys_get_tick()
{
    return invoke_syscall(SYSCALL_GET_TICK, IGNORE, IGNORE, IGNORE);
}
