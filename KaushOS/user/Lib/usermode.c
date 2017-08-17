//int main (int, char *[]);
//void _start (int argc, char *argv[]);

extern int main(int argc, char **argv, char **environ);

extern char __bss_start, _end; // As per our usermode linker script BSS is in between these 2 symbols

char *__env[1] = { 0 };
char **environ = __env;

_start()
{

	char *i;
	int ret = 0;


	// Initialize BSS section
	for(i = &__bss_start; i < &_end; i++){
		*i = 0;
	}


	ret = main(0,0, __env);

	exit(ret);
}



