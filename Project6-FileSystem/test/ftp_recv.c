#include <time.h>
#include <mthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <mailbox.h>
#include <sys/syscall.h>
#include <os.h>

#define MAX_RECV_CNT 64
#define MAXLEN 64
#define KEY    70
#define HDR_LEN 54
#define STEP 4

uint8_t buffer[MAXLEN];
char recv_buffer[MAX_RECV_CNT * sizeof(EthernetFrame)];
size_t recv_length[MAX_RECV_CNT];

int main(int argc, char *argv[])
{
    mailbox_t ftp = mbox_open("ucas");
    if (argc != 3)
    {
        sys_move_cursor(30, 1);
        printf("> No file name assigned.\n");
        return 0;
    }
    sys_exec("ftpfile", argc, argv,
            AUTO_CLEANUP_ON_EXIT);
    int len = atoi(argv[2]);
    sys_net_irq_mode(1);
    sys_move_cursor(50, 1);
    printf("[RECV] Begin transfer(%d) \n",len);
    sys_reflush();
    while (len)
    {
        int size = (len > STEP) ? STEP : size;
        int ret = sys_net_recv(recv_buffer, size * sizeof(EthernetFrame), size, recv_length);
        sys_move_cursor(50, 3);
        printf("get %d packets.\n",size);
        EthernetFrame *curr = recv_buffer;
        int pos = 0;
        for (int i = 0; i < size; i++)
        {
            sys_move_cursor(1, 1);
            mbox_send(ftp, (uintptr_t)recv_buffer + pos + HDR_LEN, 512);
            //mbox_send(ftp, buffer, 512);
            pos += recv_length[i];
        }
        sys_move_cursor(50, 4);
        len -= size;
        printf("Remain %d packets.\n", len);
        sys_reflush();
    }
}
