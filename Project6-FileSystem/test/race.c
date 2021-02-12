#include <stdio.h>
#include <string.h>
#include <mthread.h>
#include <sys/syscall.h>
#include <stdlib.h>
#define NUM_TB 4
#define KEY 66

mthread_barrier_t barrier;
mthread_barrier_t end;
mthread_mutex_t   lock;
mthread_mutex_t   lock_file;
mthread_mutex_t   lock_signal;
int rank = 0;
int round = -1;
static char blank[]  = {"                         "};
static char plane2[] = {"| __--_--______/_|       "};
static char plane3[] = {"<[___\\_\\_______|       "};
static char plane4[] = {"|  o'o                   "};
int pid[NUM_TB];
typedef struct shm {
    int id;
    int key;
    uint8_t rank[NUM_TB];
}shm_t;

void write_file(int id, int myrank)
{
    mthread_mutex_lock(&lock_file);
    char name[7] = "plane";
    char num[2];
    itoa(num, id);
    strcat(name, num);
    if (!round)
        sys_mkdir(name);
    sys_cgdir(name);
    if (!round)
        sys_touch("rank.txt");
    int fd = sys_fopen("rank.txt");
    itoa(num, myrank);
    sys_fwrite(fd, num, 2);
    sys_fclose(fd);
    sys_cgdir("/");
    mthread_mutex_unlock(&lock_file);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        sys_move_cursor(1, 1);
        printf("> Error: no place!\n");
        return 0;
    }

    int i = 22;
    int myrank = 0;
    int j = atoi(argv[1]) * 3 + 1;
    int step = atoi(argv[2]);
    int first = 0;
    if (*argv[0] != 0)
    {
        mthread_mutex_init(&lock_signal);
    }
    shm_t *s = (shm_t *)shmpageget(KEY);
    s->id = lock_signal.id;
    s->key = lock_signal.key;
    // first threads
    while (1)
    {
        if (first || *argv[0] != 0)
        {
            mthread_mutex_lock(&lock_signal);
            sys_sleep(1);
	        srand(clock());
            *argv[0] = 0;
            first = 1;
            round++;
            sys_move_cursor(1, 1);
            printf("Round %d.\n", round);
            mthread_barrier_init(&barrier, NUM_TB + 1);
            mthread_barrier_init(&end, NUM_TB + 1);
            mthread_mutex_init(&lock);
            mthread_mutex_init(&lock_file);
            *argv[0] = 100;
            *argv[1] = 0;
            if (!round)
                sys_run("back", 2, argv, 67712);
            *argv[0] = 0;
            for (int i = 0; i < NUM_TB ; i++){
                argv[1] = itoa(argv[1], i);
                argv[2] = itoa(argv[2], (rand() % 3) + 1);
                pid[i] = sys_spawn(&main, argc, argv, ENTER_ZOMBIE_ON_EXIT);
            }
        }
        mthread_barrier_wait(&barrier);
        if (!first)
        {
            for (i = 60; i > 0; i -= step)
            {
                /* move */
                sys_move_cursor(i, j + 1);
                printf("%s", plane2);

                sys_move_cursor(i, j + 2);
                printf("%s", plane3);

                sys_move_cursor(i, j + 3);
                printf("%s", plane4);
            }


            sys_move_cursor(1, j + 1);
            printf("%s", blank);

            sys_move_cursor(1, j + 2);
            printf("%s", blank);

            sys_move_cursor(1, j + 3);
            printf("%s", blank);
    
            // record rank
            mthread_mutex_lock(&lock);
            myrank = rank++;
            mthread_mutex_unlock(&lock);

            write_file((j - 1) / 3, myrank);
            s->rank[(j - 1) / 3] = myrank;
        }
        mthread_barrier_wait(&end);
        if (first)
        {
            mthread_barrier_destroy(&barrier);
            mthread_barrier_destroy(&end);
            mthread_mutex_destroy(&lock);
            mthread_mutex_destroy(&lock_file);
            rank = 0;
            mthread_mutex_unlock(&lock_signal);
        }
        else
        {
            sys_exit();
        }
    }
    return 0;
}
