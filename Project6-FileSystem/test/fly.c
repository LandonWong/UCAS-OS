#include <stdio.h>
#include <sys/syscall.h>
#include <os.h>

/*
static char blank[] = {"                   "};
static char plane1[] = {"    ___         _  "};
static char plane2[] = {"| __\\_\\______/_| "};
static char plane3[] = {"<[___\\_\\_______| "};
static char plane4[] = {"|  o'o             "};
*/
static char program[80000];

int main(int argc, char *argv[])
{
    // read race
    int fd = sys_fopen("race", O_RDWR);
    sys_move_cursor(1, 1);
    int len = 70656;
    printf("successfully open race(%d)\n", len);
    sys_reflush();
    char *buff = program;
    while(len > 0)
    {
        sys_move_cursor(1, 2);
        printf("reading remain %d/%d\n",len, 70656);
        sys_reflush();
        int sz = len > 512 ? 512 : len;
        sys_fread(fd, buff, sz);
        buff += sz;
        len -= sz;
    }
    sys_fclose(fd);
    // spawn race
    argc = 3;
    strcpy(argv[0], "foo");
    sys_run(program1, argc, argv, 70656);
}
