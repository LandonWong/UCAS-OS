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
#include <user_programs.h>
#include <os.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdint.h>

#define SHELL_BEGIN 15
#define CMD_NUM 19
#define CMD_MAX_LENGTH 20

char path[100] = "/";
char history[CMD_MAX_LENGTH];
typedef void (*function)(void *arg0, void *arg1, void *arg2, void *arg3, void *arg4);

static void shell_help();
static pid_t shell_exec(char *proc, char *arg1, char *arg2, char* arg3, char *arg4);
static pid_t shell_run(char *proc, char *arg1, char *arg2, char* arg3, char *arg4);
static void shell_taskset();
static void shell_kill(char *);
static void shell_exit();
static void shell_clear();
static void shell_ps();
static void shell_ln();
static void shell_pwd();
static void shell_ts();
static void shell_ls();
static void shell_mkfs();
static void shell_statfs();
static void shell_mkdir(char *name);
static void shell_rmdir(char *name);
static void shell_rm(char *name);
static void shell_cd(char *name);
static void shell_touch(char *name);
static void shell_cat(char *name);
static struct {
    char *name;
    char *description;
    char *usage;
    function func;
    int arg_num;
} cmd_table [] = {
    {"help", "Display informations about all supported commands", "help [NO ARGS]", &shell_help, 0},
    {"exec", "Execute task n in testbench (mode: 0 parent, 1 auto)", "exec [n] [mode]", &shell_exec, 1},
    {"run", "Execute task n from file system", "run [name] [len] <argv ...>", &shell_run, 2},
    {"taskset", "Set process or task(default) only run on a certain core", "taskset -p [pid] [mask]",&shell_taskset, 2},
    {"kill", "Kill process n (pid)", "kill [n]",&shell_kill, 1},
    {"clear", "Clear the screen", "clear", &shell_clear, 0},
    {"ps", "Show all process", "ps", &shell_ps, 0},
    {"mkfs", "Initial file system", "mkfs", &shell_mkfs, 0},
    {"statfs", "Show info of file system", "statfs", &shell_statfs, 0},
    {"mkdir", "Create a directory", "mkdir [name]", &shell_mkdir, 1},
    {"rmdir", "Remove a directory", "rmdir [name]", &shell_rmdir, 1},
    {"rm", "Remove a file", "rm [name]", &shell_rm, 1},
    {"cd", "Change a directory", "cd [name]", &shell_cd, 1},
    {"touch", "Create a file", "touch [name]", &shell_touch, 1},
    {"cat", "Print content of file to console", "cat [name]", &shell_cat, 1},
    {"ts", "Show all test programs", "ts", &shell_ts, 0},
    {"pwd", "Show current path", "pwd", &shell_pwd, 0},
    {"ls", "Show content of this directory", "ls", &shell_ls, 0},
    {"ln", "Link file to another directory", "ln [-s] <path1> <path2>", &shell_ln, 2}
};

static void shell_ln(char *str1, char *str2, char *str3)
{
    if (!strcmp(str1, "-s"))
    {
        sys_links(str2, str3);
    }else{
        sys_linkh(str1, str2);
    }
}
static void shell_ls(char *name)
{
    sys_read_dir(name);
}
static void shell_mkfs()
{
    sys_mkfs();
}
static void shell_statfs()
{
    sys_statfs();
}
static void shell_mkdir(char *name)
{
    sys_mkdir(name);
}
static void shell_rmdir(char *name)
{
    sys_rmdir(name);
}
static void shell_rm(char *name)
{
    sys_rmdir(name);
}

static void shell_pwd()
{
    printf("%s\n", path);
}

static void path_ret()
{
    int len = strlen(path);
    int i;
    for (i = len; i > 0; i--)
        if (path[i] != '/')
            path[i] = 0;
        else
            break;
        if (i)
            path[i] = 0;
}

static void path_change(char *name)
{
    int len = strlen(name);
    int i;
    if (name[0] == '/')
    {
        strcpy(path, name);
        return ;
    }
    for (i = 0; i < len; i++)
        if (name[i] == '/')
            break;
    if (i == len)
    {
        if (!strcmp(name,"."))
        {
        }
        else if(!strcmp(name, ".."))
        {
            path_ret();
        }
        else
        {
            if (strcmp(path, "/"))
            strcat(path, "/");
            strcat(path, name);
        }
    }else{
        char cur_name[16];
        char nxt_name[16];
        memset(cur_name, 0, 16);
        memset(nxt_name, 0, 16);
        memcpy(cur_name, name, i);
        memcpy(nxt_name, &name[i + 1], len - i - 1);
        if (!strcmp(cur_name,"."))
        {
        }
        else if(!strcmp(cur_name, ".."))
        {
            path_ret();
        }
        else
        {
            if (strcmp(path, "/"))
            strcat(path, "/");
            strcat(path, cur_name);
        }
        path_change(nxt_name);
    }
}

static void shell_cd(char *name)
{
    if (sys_cgdir(name)){
        path_change(name);
    }
}
static void shell_touch(char *name)
{
    sys_touch(name);
}
static void shell_cat(char *name)
{
    sys_cat(name);
}
static void shell_ts()
{
    sys_show_exec();
}

static void shell_help()
{
    printf("LandonWong's Shell. These shell commands are defined internally.\n");
    for (int i = 1; i < CMD_NUM; i++){
        printf("* %s:\t%s\t%s\n",cmd_table[i].name,cmd_table[i].usage,cmd_table[i].description);
    }
}

static pid_t shell_exec(char *proc, char *arg1, char *arg2, char* arg3, char *arg4)
{
    int argc = 1;
    if (strlen(arg1) != 0)
        argc ++;
    if (strlen(arg2) != 0)
        argc ++;
    if (strlen(arg3) != 0)
        argc ++;
    if (strlen(arg4) != 0)
        argc ++;
    char *argv[] = {proc, arg1, arg2, arg3, arg4};
    pid_t pid = sys_exec(proc, argc, argv, 1);
    if (pid > 0)
        printf("> Successfully exec in pid %d.\n", pid);
    return pid;
};

static pid_t shell_run(char *proc, char *arg1, char *arg2, char* arg3, char *arg4)
{
    int argc = 1;
    if (strlen(arg2) != 0)
        argc ++;
    if (strlen(arg3) != 0)
        argc ++;
    if (strlen(arg4) != 0)
        argc ++;
    char *argv[] = {proc, arg2, arg3, arg4};
    pid_t pid = sys_run(proc, argc, argv, atoi(arg1));
    if (pid > 0)
        printf("> Successfully run in pid %d.\n", pid);
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
   //     if (pid = sys_spawn(test_tasks[id], NULL, 0, mask)){
   //         printf("> Successfully spawn task %d, pid is %d, mode is %d, mask is %d.\n",id ,pid ,0 ,mask);
   //     }
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
int main()
{
    sys_clear();
    sys_move_cursor(1, SHELL_BEGIN);
    printf("......................................................................\n");
    printf("[root@UCAS_OS %s #] ", path);
    sys_reflush();
    int ch = 0, i;
    int length = 0;
    while (1)
    { 
        length = 0;
        char cmd[64] = {0};
        char arg[10][20] = {0};
        int man_id;
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
        strcpy(history, cmd);
        if (length  == 0){
            printf("[root@UCAS_OS %s #] ",path);
            sys_reflush();
            continue;
         }
        char *parse = cmd;
        i = 0;
        while ((parse = strtok(arg[i++], parse, ' ')) != NULL);
        man_id = man_match(arg[0], i);
        if (man_id < 0){
            printf("%s: command not found\n", arg[0]);
            printf("[root@UCAS_OS %s #] ",path);
            sys_reflush();
            continue;
        } 
        if (cmd_table[man_id].arg_num > i - 1){
            printf("Usage: %s\n",cmd_table[man_id].usage);
            printf("Your input has %d parameters, but %s needs %d.\n", i - 1,cmd_table[man_id].name , cmd_table[man_id].arg_num);
            printf("[root@UCAS_OS %s #] ",path);
            sys_reflush();
            continue;
        } 
        cmd_table[man_id].func(arg[1], arg[2], arg[3], arg[4], arg[5]);
        printf("[root@UCAS_OS %s #] ",path);
        sys_reflush();
    }    
}
