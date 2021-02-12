#include <sbi.h>

#define VERSION_BUF 50
int version = 1; // version must between 0 and 9
char buf[VERSION_BUF];

int bss_check()
{
    for (int i = 0; i < VERSION_BUF; ++i) {
        if (buf[i] != 0) return 0;
    } 
    return 1;
}

int getch()
{
	int ch;
	//Waits for a keyboard input and then returns. 
	while(1){
		ch = sbi_console_getchar();
		if(ch != -1)
			return ch;
	}  
	return ch;
}

int main(void)
{
    int check = bss_check();
	int ch;
    char output_str[] = "bss check: _ version: _\n\r";
    char output_val[2] = {0};
    int i, output_val_pos = 0;
	char hello[] = "Hello OS\n\r";

    output_val[0] = check ? 't' : 'f';
    output_val[1] = version + '0';
    for (i = 0; i < sizeof(output_str); ++i) {
        buf[i] = output_str[i];
		if (buf[i] == '_') {
			buf[i] = output_val[output_val_pos++];
		}
    } 

	//print "Hello OS!"
    sbi_console_putstr(hello); 
	//print array buf which is expected to be "Version: 1"
	sbi_console_putstr(buf);
	sbi_console_putchar(0);
    //fill function getch, and call getch here to receive keyboard input.
	//print it out at the same time 
	while(1){
		ch = getch();
		sbi_console_putchar(ch);	
	}  
    return 0;
}
