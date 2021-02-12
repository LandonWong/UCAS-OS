/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                  The shell acts as a task running in user mode.
 *       The main function is to make system calls through the user's output.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#include <test.h>
#include <string.h>
#include <os.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdint.h>

#define SHELL_BEGIN 15
#define CMD_NUM 6
#define CMD_MAX_LENGTH 10
struct task_info task_test_waitpid = {
    (uintptr_t)&wait_exit_task, USER_PROCESS};
struct task_info task_test_semaphore = {
    (uintptr_t)&test_affinity, USER_PROCESS};
struct task_info task_test_condition = {
    (uintptr_t)&test_condition, USER_PROCESS};
struct task_info task_test_barrier = {
    (uintptr_t)&test_barrier, USER_PROCESS};

struct task_info task13 = {(uintptr_t)&SunQuan, USER_PROCESS};
struct task_info task14 = {(uintptr_t)&LiuBei, USER_PROCESS};
struct task_info task15 = {(uintptr_t)&CaoCao, USER_PROCESS};
struct task_info task_test_multicore = {(uintptr_t)&test_multicore, USER_PROCESS};

static struct task_info *test_tasks[16] = {&task_test_waitpid,
                                           &task_test_semaphore,
                                           &task_test_condition,
                                           &task_test_barrier,
                                           &task13, &task14, &task15,
                                           &task_test_multicore};
static int num_test_tasks = 8;
int curr_linenum;
typedef void (*function)(void *arg0, void *arg1, void *arg2);

static void shell_help();
static pid_t shell_exec(char *, char *);
static void shell_taskset();
static void shell_kill(char *);
static void shell_exit();
static void shell_clear();
static void shell_ps();
static struct {
    char *name;
    char *description;
    char *usage;
    function func;
    int arg_num;
} cmd_table [] = {
    {"help", "Display informations about all supported commands", "help [NO ARGS]", &shell_help, 0},
    {"exec", "Execute task n in testbench (mode: 0 parent, 1 auto)", "exec [n] [mode]", &shell_exec, 2},
    {"taskset", "Set process or task(default) only runs on a certain core", "taskset [task id] [mask]\n           taskset  -p [pid] [mask]",&shell_taskset, 2},
    {"kill", "Kill process n (pid)", "kill [n]",&shell_kill, 1},
    {"clear", "Clear the screen", "clear", &shell_clear, 0},
    {"ps", "Show all process", "ps", &shell_ps, 0}
};

static void shell_help()
{
    printf("LandonWong's Shell. These shell commands are defined internally.\n");
    for (int i = 1; i < CMD_NUM; i++){
        printf("* %s:\t%s\t%s\n",cmd_table[i].name,cmd_table[i].usage,cmd_table[i].description);
    }
}

static pid_t shell_exec(char *task_id, char *mode)
{
    int id = atoi(task_id);
    int m = atoi(m);
    int pid = -1;
    if (id >= num_test_tasks){
       printf("> Error: No such task.\n");
       return ;
    }
    printf("> Exec task %d\n",id);
    if (pid = sys_spawn(test_tasks[id], NULL, m, 0x3)){
        printf("> Successfully spawn task %d, pid is %d, mode is %d.\n",id,pid,m);
    }
    return pid;
};

static void shell_taskset(char *arg0, char *arg1, char *arg2)
{
    if(!(arg0[0] == '-' && arg0[1] == 'p')){
        uint8_t mask = atoi(arg0);
        uint8_t id = atoi(arg1);
        int pid;
        if (mask != 1 && mask != 2 && mask != 3){
            printf("Usage: Mask must be 0x1, 0x2 or 0x3\n");
            return ;
        }
        if (pid = sys_spawn(test_tasks[id], NULL, 0, mask)){
            printf("> Successfully spawn task %d, pid is %d, mode is %d, mask is %d.\n",id ,pid ,0 ,mask);
        }
    }else{
        uint8_t mask = atoi(arg1);
        uint8_t pid  = atoi(arg2);
        sys_taskset(mask, pid);
    }
};

static void shell_kill(char *pid)
{
    pid_t pid_num = atoi(pid);
    if(sys_kill(pid_num)){
        printf("> Successfully killed process pid[%d].\n",pid_num);
    }else{
        printf("> Error: Process pid[%d] not found.\n",pid_num);
    }
};

static void shell_exit()
{
};

static void shell_ps()
{
    sys_ps();
};

static void shell_clear()
{
    curr_linenum = 0;
    sys_clear();
    sys_move_cursor(1, SHELL_BEGIN);
    printf("......................................................................\n");
};
static int man_match(char *arg, int arg_num){
    int i;
    for (i = 0; i < CMD_NUM; i++)
    {
        if (strcmp(arg, cmd_table[i].name) == 0)
            return i;
    }
    return -1; 
}
void test_shell()
{
    sys_clear();
    sys_move_cursor(1, SHELL_BEGIN);
    printf("......................................................................\n");
    printf("[root@UCAS_OS / #] ");
    sys_reflush();
    int ch = 0, i;
    int length = 0;
    while (1)
    { 
        length = 0;
        char cmd[10] = {0};
        char arg[10][10] = {0};
        int man_id;
        // TODO: call syscall to read UART port
         while(1){
            ch = getchar();
             if(ch == '\r'){
                printf("\n");
                break;
             }
              if(ch == 8 || ch == 127){
                  if (length > 0){
                    putchar(ch);
                    sys_reflush();
                    cmd[--length] = 0;;
                }
            }else{
                putchar(ch);
                sys_reflush();
                cmd[length++] = ch;
            }
          } 
        cmd[length] = 0;
        if (length  == 0){
            printf("[root@UCAS_OS / #] ");
            sys_reflush();
            continue;
         }
        char *parse = cmd;
        // TODO: parse input
        i = 0;
        while ((parse = strtok(arg[i++], parse, ' ')) != NULL);
        man_id = man_match(arg[0], i);
        if (man_id < 0){
            printf("%s: command not found\n", arg[0]);
            printf("[root@UCAS_OS / #] ");
            sys_reflush();
            continue;
        } 
        if (cmd_table[man_id].arg_num > i - 1){
            printf("Usage: %s\n",cmd_table[man_id].usage);
            printf("Your input has %d parameters, but %s needs %d.\n", i - 1,cmd_table[man_id].name , cmd_table[man_id].arg_num);
            printf("[root@UCAS_OS / #] ");
            sys_reflush();
            continue;
        } 
        //if (cmd_table[man_id].arg_num == 0)
            //parameter = NULL;
          //  cmd_table[man_id].func();
        //else if (cmd_table[man_id].arg_num == 1)
            //parameter = arg[1];
          //  cmd_table[man_id].func(arg[1]);
       // else
            //parameter = para;
            cmd_table[man_id].func(arg[1], arg[2], arg[3]);
        // note: backspace maybe 8('\b') or 127(delete)

        // TODO: ps, exec, kill, clear

        printf("[root@UCAS_OS / #] ");
        sys_reflush();
    }    
}
