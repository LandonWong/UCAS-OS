#include <stdio.h>
#include <string.h>
#include <mthread.h>
#include <sys/syscall.h>
#include <stdlib.h>
#define NUM_TB 4
#define KEY 66

mthread_barrier_t barrier;
mthread_barrier_t end;
mthread_mutex_t   lock_signal;
static char blank[]  = {"                          "};
static char plane2[] = {"        |_\___--______ |"};
static char plane3[] = {"        |___//_//_____]>"};
static char plane4[] = {"                  o'o  | "};
int pid[NUM_TB];
typedef struct shm {
    int id;
    int key;
    uint8_t rank[NUM_TB];
}shm_t;

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        sys_move_cursor(1, 1);
        printf("> Error: no place!\n");
        return 0;
    }

    int i = 22;
    int j = atoi(argv[1]) * 3 + 1;
    int first = 0;
    // first threads
    shm_t *s = (shm_t *)shmpageget(KEY);
    lock_signal.id = s->id;
    lock_signal.key = s->key;
    while (1)
    {
        if (first || *argv[0] != 0)
        {
            mthread_mutex_lock(&lock_signal);
            sys_sleep(1);
	        srand(clock());
            *argv[0] = 0;
            first = 1;
            mthread_barrier_init(&barrier, NUM_TB + 1);
            mthread_barrier_init(&end, NUM_TB + 1);
            for (int i = 0; i < NUM_TB ; i++){
                argv[1] = itoa(argv[1], i);
                pid[i] = sys_spawn(&main, argc, argv, AUTO_CLEANUP_ON_EXIT);
                sys_setpriority(pid[i], s->rank[i]);
            }
        }
        mthread_barrier_wait(&barrier);
        if (!first)
        {
            for (i = 0; i < 60; i++)
            {
                /* move */
                sys_move_cursor(i, j + 1);
                printf("%s", plane2);

                sys_move_cursor(i, j + 2);
                printf("%s", plane3);

                sys_move_cursor(i, j + 3);
                printf("%s", plane4);
            }
            sys_move_cursor(60, j + 1);
            printf("%s", blank);

            sys_move_cursor(60, j + 2);
            printf("%s", blank);

            sys_move_cursor(60, j + 3);
            printf("%s", blank);
    
        }
        mthread_barrier_wait(&end);
        if (first)
        {
            mthread_barrier_destroy(&barrier);
            mthread_barrier_destroy(&end);
            mthread_mutex_unlock(&lock_signal);
        }
        else
        {
            sys_exit();
        }
    }
    return 0;
}
