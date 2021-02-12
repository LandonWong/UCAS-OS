#include <screen.h>
#include <common.h>
#include <stdio.h>
#include <os/string.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/mm.h>
#include <os/irq.h>

#define SCREEN_WIDTH    80
#define SCREEN_HEIGHT   50

/* screen buffer */
char new_screen[SCREEN_HEIGHT * SCREEN_WIDTH] = {0};
char old_screen[SCREEN_HEIGHT * SCREEN_WIDTH] = {0};

/* cursor position */
void vt100_move_cursor(int x, int y)
{
    // \033[y;xH
    current_running = (get_current_cpu_id() == 0) ? &current_running_core0 : &current_running_core1;
    disable_preempt();
    printk("%c[%d;%dH", 27, y, x);
    ((pcb_t *)*current_running)->cursor_x = x;
    ((pcb_t *)*current_running)->cursor_y = y;
    enable_preempt();
}

/* clear screen */
static void vt100_clear()
{
    // \033[2J
    printk("%c[2J", 27);
}

/* hidden cursor */
static void vt100_hidden_cursor()
{
    // \033[?25l
    printk("%c[?25l", 27);
}

/* write a char */
void screen_write_ch(char ch)
{
    current_running = (get_current_cpu_id() == 0) ? &current_running_core0 : &current_running_core1;
    if (ch == '\n')
    {
        ((pcb_t *)*current_running)->cursor_x = 1;
        ((pcb_t *)*current_running)->cursor_y++;
    }
    if (ch == '\b' || ch == 127)
    {
        new_screen[(((pcb_t *)*current_running)->cursor_y - 1) * SCREEN_WIDTH + (--(((pcb_t *)*current_running)->cursor_x) - 1)] = ' ';
    }
    else
    {
        new_screen[(((pcb_t *)*current_running)->cursor_y - 1) * SCREEN_WIDTH + (((pcb_t *)*current_running)->cursor_x - 1)] = ch;
        (((pcb_t *)*current_running)->cursor_x)++;
    }
}

void init_screen(void)
{
    vt100_hidden_cursor();
    vt100_clear();
}

void screen_clear(void)
{
    current_running = (get_current_cpu_id() == 0) ? &current_running_core0 : &current_running_core1;
    int i, j;
    for (i = 0; i < SCREEN_HEIGHT; i++)
    {
        for (j = 0; j < SCREEN_WIDTH; j++)
        {
            new_screen[i * SCREEN_WIDTH + j] = ' ';
        }
    }
    ((pcb_t *)*current_running)->cursor_x = 1;
    ((pcb_t *)*current_running)->cursor_y = 1;
    screen_reflush();
}

void screen_move_cursor(int x, int y)
{
    current_running = (get_current_cpu_id() == 0) ? &current_running_core0 : &current_running_core1;
    ((pcb_t *) *current_running)->cursor_x = x;
    ((pcb_t *) *current_running)->cursor_y = y;
}

void screen_write(char *buff)
{
    int i = 0;
    int l = kstrlen(buff);
    for (i = 0; i < l; i++)
    {
        screen_write_ch(buff[i]);
     }
} 

int serial_getchar()
{
    return sbi_console_getchar();
}


void serial_putchar(int ch)
{
    sbi_console_putchar(ch);    
}
/*
 * This function is used to print the serial port when the clock
 * interrupt is triggered. However, we need to pay attention to
 * the fact that in order to speed up printing, we only refresh
 * the characters that have been modified since this time.
 */
void screen_reflush(void)
{
    int i, j;

    current_running = (get_current_cpu_id() == 0) ? &current_running_core0 : &current_running_core1;
    int save_cursor_x = ((pcb_t *)*current_running)->cursor_x;
    int save_cursor_y = ((pcb_t *)*current_running)->cursor_y;

    /* here to reflush screen buffer to serial port */
    for (i = 0; i < SCREEN_HEIGHT; i++)
    {
        for (j = 0; j < SCREEN_WIDTH; j++)
        {
            /* We only print the data of the modified location. */
            if (new_screen[i * SCREEN_WIDTH + j] != old_screen[i * SCREEN_WIDTH + j])
            {
                vt100_move_cursor(j + 1, i + 1);
                port_write_ch(new_screen[i * SCREEN_WIDTH + j]);
                old_screen[i * SCREEN_WIDTH + j] = new_screen[i * SCREEN_WIDTH + j];
            }
        }
    }
    ((pcb_t *)*current_running)->cursor_x = save_cursor_x;
    ((pcb_t *)*current_running)->cursor_y = save_cursor_y;

    /* recover cursor position */
    vt100_move_cursor(((pcb_t *)*current_running)->cursor_x, ((pcb_t *)*current_running)->cursor_y);
}



void screen_scroll(int line1, int line2)
{
    int i, j;

    current_running = (get_current_cpu_id() == 0) ? &current_running_core0 : &current_running_core1;
    for (i = line1; i <= line2; i++)
    {
        for (j = 0; j < SCREEN_WIDTH; j++)
        {
            old_screen[i * SCREEN_WIDTH + j] = 0;
        }
    }

    for (i = line1; i <= line2; i++)
    {
        for (j = 0; j < SCREEN_WIDTH; j++)
        {
            if (i == line2)
            {
                new_screen[i * SCREEN_WIDTH + j] = ' ';
            }
            else
            {
                new_screen[i * SCREEN_WIDTH + j] = new_screen[(i + 1) * SCREEN_WIDTH + j];
            }
        }
    }
    ((pcb_t *)*current_running)->cursor_y --;
    screen_reflush();
}

void kernel_screen_scroll_check()
{
     current_running = (get_current_cpu_id() == 0) ? &current_running_core0 : &current_running_core1;
     if (((pcb_t *)*current_running)->cursor_y > 30)
     {
         screen_scroll(19, ((pcb_t *)*current_running)->cursor_y - 1);
     }
}
