#include <time.h>
#include <mthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <mailbox.h>
#include <sys/syscall.h>

#define MAXLEN 512
#define KEY    70

uint8_t buffer[MAXLEN];

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        sys_move_cursor(1, 1);
        printf("> No file name assigned.\n");
        return 0;
    }
    int len = atoi(argv[2]);
    sys_move_cursor(1, 1);
    printf("[FILE] Write to file %s (%d).\n", argv[1], len);
    mailbox_t ftp = mbox_open("ucas");
    printf("[FILE] Begin write to file!\n");
    sys_cgdir("/");
    sys_touch(argv[1]);
    int fd = sys_fopen(argv[1]);
    while (len)
    {
        mbox_recv(ftp, buffer, MAXLEN);
        sys_fwrite(fd, buffer, MAXLEN);
        sys_move_cursor(1, 5);
        printf("Remain %d packets.(%d)\n", len,strlen(buffer));
        len--;
    }
    sys_fclose(fd);
}
