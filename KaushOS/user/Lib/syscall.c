/*
*  C Implementation: Syscall.c
*
* Description:  Implementation of User mode part of syscall
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/

#include <inc/syscall.h>
#include <inc/stdarg.h>

//Sys call with 0 args
#define syscall0(NUMBER)                                        \
        ({                                                      \
          int retval;                                           \
          asm volatile                                          \
            ("pushl %[number]; int $0x40; addl $4, %%esp"       \
               : "=a" (retval)                                  \
               : [number] "i" (NUMBER)                          \
               : "memory");                                     \
          retval;                                               \
        })



//Sys call with 1 arg
#define syscall1(NUMBER, ARG0)                                           \
        ({                                                               \
          int retval;                                                    \
          asm volatile                                                   \
            ("pushl %[arg0]; pushl %[number]; int $0x40; addl $8, %%esp" \
               : "=a" (retval)                                           \
               : [number] "i" (NUMBER),                                  \
                 [arg0] "g" (ARG0)                                       \
               : "memory");                                              \
          retval;                                                        \
        })


#define syscall2(NUMBER, ARG0, ARG1)                            \
        ({                                                      \
          int retval;                                           \
          asm volatile                                          \
            ("pushl %[arg1]; pushl %[arg0]; "                   \
             "pushl %[number]; int $0x40; addl $12, %%esp"      \
               : "=a" (retval)                                  \
               : [number] "i" (NUMBER),                         \
                 [arg0] "g" (ARG0),                             \
                 [arg1] "g" (ARG1)                              \
               : "memory");                                     \
          retval;                                               \
        })


#define syscall3(NUMBER, ARG0, ARG1, ARG2)                      \
        ({                                                      \
          int retval;                                           \
          asm volatile                                          \
            ("pushl %[arg2]; pushl %[arg1]; pushl %[arg0]; "    \
             "pushl %[number]; int $0x40; addl $16, %%esp"      \
               : "=a" (retval)                                  \
               : [number] "i" (NUMBER),                         \
                 [arg0] "g" (ARG0),                             \
                 [arg1] "g" (ARG1),                             \
                 [arg2] "g" (ARG2)                              \
               : "memory");                                     \
	          retval;                                               \
        })

	
void
_exit(int val)
{

	int ret = 0;
	ret = syscall1(SYS_EXIT,val);
}


void
exit(int val)
{

	int ret = 0;
	ret = syscall1(SYS_EXIT,val);
}

int
execve(char *name, char **argv, char **env)
{

	// Not Implemented Yet
//	errno = -1;
	return -1;
}


int
getpid()
{

	int pid;
	
	pid = syscall0(SYS_GETPID);
	return pid;

}


int
fork(void) 
{
	return(syscall0(SYS_FORK));

}


int
kill(int pid, int sig)
{


  // Not Implemented Yet
  // Can Only remove if the requested process is the current process.

  if(pid == getpid())
    _exit(sig);

  return -1;


}

int 
sbrk(int nbytes){

	return -1;

}



int
isatty(int fd)

{

		return 0;
}


int
close(int file) 
{
	return -1;
}

int
link(char *old, char *new) 
{
	return -1;
}

int
lseek(int file, int ptr, int dir) 
{
	return 0;
}

int
open(const char *name, int flags, ...) 
{
	return -1;
}

int
read(int file, char *ptr, int len) 
{
	return 0;
}

int fstat(int file, void *st) 
{
	return 0;
}

int
stat(const char* file, void *st) 
{
	return 0;
}


int
unlink(char *name) 
{
	return -1;
}


int
wait(int *status) 
{
	return -1;
}


int gettimeofday(void *p, void *z)
{
	return -1;
}




// Syscall for Testing
int
UTestSysCall(int a, int b, int c, int d)
{
	int ret = 0;
	ret = syscall3(SYS_TESTING, b, c, d);
	return ret;
}

