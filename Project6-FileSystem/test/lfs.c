#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>

#include <os.h>

#define MX (1024 * 8)

static char buff[1024];
static char cont[1024];

int main(void)
{
    int i, j;
    int fd = sys_fopen("test", O_RDWR);
    memset(cont, 2, 1023);
    // write 'hello world!' * 10
    for (i = 0; i < MX; i++)
    {
        sys_move_cursor(1, 1);
        printf("Successfully write %d KiB.\n", i);
        sys_reflush();
        sys_fwrite(fd, cont, 1024);
    }
    // read
    for (i = 0; i < MX; i++)
    {
        sys_move_cursor(1, 2);
        printf("Successfully read %d KiB.\n", i);
        sys_reflush();
        sys_fread(fd, buff, 1024);
        if (strcmp(buff, cont))
        {
            sys_fclose(fd);
            printf("ERROR! at %dKB.", i);
            return 0;
        }
    }
    sys_fclose(fd);
}
