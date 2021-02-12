#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>

#include <os.h>

#define MX 10
static char buff[64];

int main(void)
{
    int i, j;
    int fd = sys_fopen("1.txt", O_RDWR);

    // write 'hello world!' * 10
    for (i = 0; i < MX; i++)
    {
        sys_fwrite(fd, "hello world!\n", 13);
    }
    // read
    for (i = 0; i < MX; i++)
    {
        sys_fread(fd, buff, 13);
        for (j = 0; j < 13; j++)
        {
            printf("%c", buff[j]);
        }
    }
    sys_fclose(fd);
}
