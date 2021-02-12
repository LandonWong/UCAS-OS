#include <stdio.h>
#include <sys/syscall.h>
#define MAGIC 250

/*
 * We only allow a process has 4 phy page frames (besides code and data segs).
 * but include kernel stack and user stack.
 *
 * The test first load and alloc 2 page frames, till now the process already
 * have 4 pfs
 *
 * Then test will write one pf, which makes this situation:
 *    
 *    kernel stack: [ A  D ]
 *      user stack: [ A  D ]
 *             pf1: [ A    ]
 *             pf2: [ A  D ] (which write to just now)
 *
 *  Then we write another pf, so pf1 will be stored to SD-card.
 *
 *    kernel stack: [ A  D ]
 *      user stack: [ A  D ]
 *             pf3: [ A  D ]
 *             pf2: [ A  D ]
 *
 *             pf1: [SDcard]
 *
 * Then we write to pf1, so pf1 will recopy to memory and pf3
 * will exchange to SD card.
 *
 *    kernel stack: [ A  D ]
 *      user stack: [ A  D ]
 *             pf1: [ A  D ]
 *             pf3: [ A  D ]
 *
 *             pf2: [SDcard]
 *
 * Every copy between memory and SD card will have a feedback to screen.
 *
 */

int main()
{
    sys_move_cursor(1, 1);
    printf("Begin page swap testing. Please note swap infomation.\n");
    uint64_t *pf1 = 0x1000;
    uint64_t *pf2 = 0x1000000;
    uint64_t *pf3 = 0x2000000;
    uint64_t tmp;
    printf("> 1: Read from (pf1) 0x1000\n");
    tmp = *pf1;
    sys_sleep(1);
    printf("> 2: Read from (pf2) 0x1000000\n");
    tmp = *pf2;
    sys_sleep(1);
    printf("> 3: Write to  (pf2) 0x1000000\n");
    *pf2 = MAGIC;
    sys_sleep(1);
    // PAGE SWAP
    printf("> 4: Write to  (pf3) 0x2000000 (swap)\n");
    *pf3 = MAGIC; // This will cause exchange at pf1
    sys_sleep(1);
    // PAGE SWAP
    printf("> 5: Write to  (pf1) 0x1008 (swap)\n");
    *(pf1 + 1) = 100; // This will cause exchange at pf3
    sys_sleep(1);
    printf("> 6: Check we write on step 4 (maybe swap)\n");
    if (*pf3 == MAGIC && *pf2 == MAGIC) 
        printf("> Data raw success!\n");
    else
        printf("> Data raw failed!\n");
    sys_sleep(1);
    printf("End testing.\n");
    return 0;
}
