#include <stdio.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <time.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
	srand(clock());
	long mem2 = 0;
	uintptr_t mem1 = 0;
	int curs = 0;
	int i;
	sys_move_cursor(2, 2);
    /*
	printf("argc = %d, argv at 0x%lx\n", argc, argv);
	for (i = 1; i < argc; ++i) 
	 	printf("argv[%d] = %s, at 0x%lx\n", i, argv[i], argv[i]);
    sys_reflush();
    printf("___________________\n");*/
	for (i = 1; i < argc; i++)
	{
		mem1 = atol(argv[i]);
		// sys_move_cursor(2, curs+i);
		mem2 = rand();
		*(long*)mem1 = mem2;
		printf("test %d: 0x%lx, write %ld\n",i , mem1, mem2);
		if (*(long*)mem1 != mem2) {
			printf("Error!\n");
		}
	}
	//Only input address.
	//Achieving input r/w command is recommended but not required.
	printf("Success!\n");
	// while(1);
	return 0;
}
