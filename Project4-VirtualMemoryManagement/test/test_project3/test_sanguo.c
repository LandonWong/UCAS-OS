#include <time.h>
#include <mthread.h>
#include <test_project3/test3.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <mailbox.h>
#include <sys/syscall.h>

static struct task_info sq_task = {(uintptr_t)&SunQuan, USER_PROCESS};
static struct task_info lb_task = {(uintptr_t)&LiuBei, USER_PROCESS};
static struct task_info cc_task = {(uintptr_t)&CaoCao, USER_PROCESS};

void SunQuan(void)
{
    mailbox_t pub = mbox_open("SunQuan-Publish-PID");
    pid_t myPid = sys_getpid();
    sys_move_cursor(1, 1);
    /* Send PID twice, once for LiuBei,
     * and once for the CaoCao */

    //sys_move_cursor(1, 1);
    //printf("SunQuan(%d): Hello, I am SunQuan          ", myPid);

    mbox_send(pub, &myPid, sizeof(pid_t));
    mbox_send(pub, &myPid, sizeof(pid_t));

    /* Find LiuBei's PID */
    mailbox_t sub = mbox_open("LiuBei-Publish-PID");

    for (int i = 0;; i++)
    {
        pid_t liubei;

        sys_move_cursor(1, 1);
        printf("(%d)[SunQuan](%d): Where are you Liubei?                ",i, myPid);
        mbox_recv(sub, &liubei, sizeof(pid_t));

        sys_move_cursor(1, 1);
        printf("(%d)[SunQuan](%d): I'm waiting for Liubei (%d)          ",i, myPid, liubei);
        sys_waitpid(liubei);

        sys_move_cursor(1, 1);
        printf("(%d)[SunQuan](%d): I'm coming to save you, LiuBei!",i, myPid);

        sys_sleep(1);
        sys_spawn(&lb_task, NULL, ENTER_ZOMBIE_ON_EXIT, 0x3);
        mbox_send(pub, &myPid, sizeof(pid_t));
    }
}

void LiuBei(void)
{
    mailbox_t pub = mbox_open("LiuBei-Publish-PID");
    pid_t myPid = sys_getpid();

    /* Send PID twice, once for sunquan Hood,
     * and once  for the CaoCao */
    mbox_send(pub, &myPid, sizeof(pid_t));
    mbox_send(pub, &myPid, sizeof(pid_t));

    /* Find sunquan's PID */
    mailbox_t sub = mbox_open("SunQuan-Publish-PID");

    // sys_move_cursor(1, 2);
    // printf("LiuBei(%d): Hello, I am Liubei          ", myPid);

    for (int i = 0;; i++)
    { 
        pid_t aramis;

        sys_move_cursor(1, 2);
        printf("(%d)[LiuBei](%d): Where are you SunQuan?          ",i, myPid);
        mbox_recv(sub, &aramis, sizeof(pid_t));

        sys_move_cursor(1, 2);
        printf("(%d)[LiuBei](%d): I'm waiting for SunQuan (%d)    ",i, myPid, aramis);
        sys_waitpid(aramis);

        sys_move_cursor(1, 2);
        printf("(%d)[LiuBei](%d): I'm coming to save you, SunQuan!",i, myPid);

        sys_sleep(1);
        sys_spawn(&sq_task, NULL, ENTER_ZOMBIE_ON_EXIT, 0x3);
        mbox_send(pub, &myPid, sizeof(pid_t));
    }
} 

void CaoCao(void)
{   
    pid_t myPid = sys_getpid();

    mailbox_t subSunQuan = mbox_open("SunQuan-Publish-PID");
    mailbox_t subLiuBei = mbox_open("LiuBei-Publish-PID");

    printf("[CaoCao](%d): I am waiting for your pids", myPid);
    int i;
    pid_t sunquan, liubei;
    mbox_recv(subSunQuan, &sunquan, sizeof(pid_t));
    mbox_recv(subLiuBei, &liubei, sizeof(pid_t));

    for (i = 0;;i++)
     { 
        sys_move_cursor(1, 3);
        printf("(%d)[CaoCao](%d): I am working... muahaha ",i, myPid);

        sys_sleep(1);

        sys_move_cursor(1, 4);
        printf("(%d)[CaoCao](%d): I have my decision! ",i, myPid);

        switch (rand() % 2)
         {
        case 0:
            sys_move_cursor(1, 5);
            printf("(%d)[CaoCao](%d): I will kill SunQuan (%d)!  ",i, myPid, sunquan);
            sys_sleep(1);

            sys_move_cursor(1, 6);
            printf("(%d)[CaoCao]biu biu biu ~~~~~~ AAAAAAAA SunQuan is dead QAQ.",i);
            sys_kill(sunquan);
            mbox_recv(subSunQuan, &sunquan, sizeof(pid_t));

            sys_move_cursor(1, 7);
            printf("(%d)[CaoCao](%d): Oops! SunQuan(%d) lives!                 ",i, myPid, sunquan);
            break;
        case 1:
            sys_move_cursor(1, 5);
            printf("(%d)[CaoCao](%d): I will kill LiuBei(%d)! ",i, myPid, liubei);
            sys_sleep(1);

            sys_move_cursor(1, 6);
            printf("(%d)[CaoCao]biu biu biu ~~~~~~ AAAAAAAA Liubei is dead QAQ.",i);
            sys_kill(liubei);

            sys_move_cursor(1, 7);
            mbox_recv(subLiuBei, &liubei, sizeof(pid_t));
            printf("(%d)[CaoCao](%d): F**k! LiuBei(%d) is alive again! ",i, myPid, liubei);
            break;
         }

        sys_sleep(1);
    }
} 
