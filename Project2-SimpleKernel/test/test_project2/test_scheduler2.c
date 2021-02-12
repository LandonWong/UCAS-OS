#include <stdio.h>
#include <sys/syscall.h>

static char blank[] = {"                    "};
static char plane1[] = {"    ___         _   "};
static char plane2[] = {"| __\\_\\______/_|  "};
static char plane3[] = {"<[___\\_\\_______|  "};
static char plane4[] = {"|  o'o              "};

void printf_task1(void)
{
    int i;
    int print_location = 5;

    for (i = 0;; i++)
    { 
        sys_move_cursor(1, print_location);
        printf("> [TASK1] This task is to test scheduler. (%d)", i);
    }
}

void printf_task2(void)
{  
    int i;
    int print_location = 6;

    for (i = 0;; i++)
     {
        sys_move_cursor(1, print_location);
        printf("> [TASK2] This task is to test scheduler. (%d)", i);
     }
}

void drawing_task2(void)
{  
    int i, j = 22;

    while (1)
     {
        for (i = 55; i > 0; i--)
          {
            sys_move_cursor(i, j + 0);
            printf("%s", plane1);

            sys_move_cursor(i, j + 1);
            printf("%s", plane2);

            sys_move_cursor(i, j + 2);
            printf("%s", plane3);

            sys_move_cursor(i, j + 3);
            printf("%s", plane4);
        }
        sys_move_cursor(1, j + 0);
        printf("%s", blank);

        sys_move_cursor(1, j + 1);
        printf("%s", blank);

        sys_move_cursor(1, j + 2);
        printf("%s", blank);

        sys_move_cursor(1, j + 3);
        printf("%s", blank);
     }
}
void priority_task1(void)
{
    int i;
    int print_location = 2;
    for (i = 0;; i++)
    {
        sys_move_cursor(1, print_location);
        printf("> [TASK1] This task is to test priority P_1. (%d)", i);
    }
} 

void priority_task2(void)
{    
    int i;
    int print_location = 3;

    for (i = 0;; i++)
     {
        sys_move_cursor(1, print_location);
        printf("> [TASK2] This task is to test priority P_2. (%d)", i);
     }
}   
void priority_task3(void)
{ 
    int i;
    int print_location = 4;
    
    for (i = 0;; i++)
    {
        sys_move_cursor(1, print_location);
        printf("> [TASK3] This task is to test priority P_3. (%d)", i);
    }
}      

void priority_task4(void)
{       
    int i;
    int print_location = 5;

    for (i = 0;; i++)
     { 
        sys_move_cursor(1, print_location);
        printf("> [TASK4] This task is to test priority P_4. (%d)", i);
     }
}

void do_sch_timer(void)
{
    uint64_t *sch_timer = 0x52000000;
    sys_move_cursor(1, 1);
    printf("> Last do_scheduler costs %d ticks \n", get_ticks() - *sch_timer);
    while(1);
}
