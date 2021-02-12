#include <stdio.h>
#include <sys/syscall.h>
#include <string.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <mthread.h>

#define HDR_OFFSET 54
#define SHMP_KEY 0x42
#define MAGIC 0xbeefbeefbeefbeeflu
#define FIFO_BUF_MAX 2048

#define MAX_RECV_CNT 256
char recv_buffer[MAX_RECV_CNT * sizeof(EthernetFrame)];
size_t recv_length[MAX_RECV_CNT] = {0};

const char response[] = "Response: ";

/* this should not exceed a page */
typedef struct echo_shm_vars {
    atomic_long magic_number;
    atomic_long available;
    char fifo_buffer[FIFO_BUF_MAX];
} echo_shm_vars_t;

int main(int argc, char *argv[])
{}
