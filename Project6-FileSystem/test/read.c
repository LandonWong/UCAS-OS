#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>

#include <os.h>

#define MX 4096

static char buff[4096];

int main(int argc, char *argv[])
{
    int i, j;
    int fd = sys_fopen(argv[1], O_RDWR);
    sys_move_cursor(1, 1);
    memset(buff, 0, 4096);
    int begin = atoi(argv[2]);
    int end = atoi(argv[3]);
    int cnt = begin / 4096;
    for (i = 0; i < cnt; i++)
        sys_fread(fd, buff, 4096);
    begin -= cnt * 4096;
    end -= cnt * 4096;
    sys_fread(fd, buff, 4096);
    for (i = begin; i < end; i++){
        printf("%02x ", buff[i]);
        if ((i - begin) && (i - begin) % 20 == 0)
            printf("\n");
    }
    sys_fclose(fd);
    return 0;
}
