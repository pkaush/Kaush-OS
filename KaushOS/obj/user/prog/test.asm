
obj/user/prog/test:     file format elf32-i386


Disassembly of section .text:

08049000 <main>:
 8049000:	55                   	push   %ebp
 8049001:	c7 05 08 b0 04 08 05 	movl   $0x5,0x804b008
 8049008:	00 00 00 
 804900b:	89 e5                	mov    %esp,%ebp
 804900d:	83 e4 f0             	and    $0xfffffff0,%esp
 8049010:	e8 8b 00 00 00       	call   80490a0 <fork>
 8049015:	e8 86 00 00 00       	call   80490a0 <fork>
 804901a:	eb fe                	jmp    804901a <main+0x1a>
 804901c:	66 90                	xchg   %ax,%ax
 804901e:	66 90                	xchg   %ax,%ax

08049020 <_start>:

    char *i;
    int ret = 0;

    // Initialize BSS section
    for (i = &__bss_start; i < &_end; i++)
 8049020:	b8 04 b0 04 08       	mov    $0x804b004,%eax
{
 8049025:	83 ec 0c             	sub    $0xc,%esp
    for (i = &__bss_start; i < &_end; i++)
 8049028:	3d 10 b0 04 08       	cmp    $0x804b010,%eax
 804902d:	73 0e                	jae    804903d <_start+0x1d>
 804902f:	90                   	nop
    {
        *i = 0;
 8049030:	c6 00 00             	movb   $0x0,(%eax)
    for (i = &__bss_start; i < &_end; i++)
 8049033:	83 c0 01             	add    $0x1,%eax
 8049036:	3d 10 b0 04 08       	cmp    $0x804b010,%eax
 804903b:	75 f3                	jne    8049030 <_start+0x10>
    }

    ret = main(0, 0, __env);
 804903d:	83 ec 04             	sub    $0x4,%esp
 8049040:	68 04 b0 04 08       	push   $0x804b004
 8049045:	6a 00                	push   $0x0
 8049047:	6a 00                	push   $0x0
 8049049:	e8 b2 ff ff ff       	call   8049000 <main>

    exit(ret);
 804904e:	89 04 24             	mov    %eax,(%esp)
 8049051:	e8 1a 00 00 00       	call   8049070 <exit>
}
 8049056:	83 c4 1c             	add    $0x1c,%esp
 8049059:	c3                   	ret    
 804905a:	66 90                	xchg   %ax,%ax
 804905c:	66 90                	xchg   %ax,%ax
 804905e:	66 90                	xchg   %ax,%ax

08049060 <_exit>:

void _exit(int val)
{

    int ret = 0;
    ret = syscall1(SYS_EXIT, val);
 8049060:	ff 74 24 04          	push   0x4(%esp)
 8049064:	6a 01                	push   $0x1
 8049066:	cd 40                	int    $0x40
 8049068:	83 c4 08             	add    $0x8,%esp
}
 804906b:	c3                   	ret    
 804906c:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi

08049070 <exit>:
 8049070:	ff 74 24 04          	push   0x4(%esp)
 8049074:	6a 01                	push   $0x1
 8049076:	cd 40                	int    $0x40
 8049078:	83 c4 08             	add    $0x8,%esp
 804907b:	c3                   	ret    
 804907c:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi

08049080 <execve>:
{

    // Not Implemented Yet
    //	errno = -1;
    return -1;
}
 8049080:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
 8049085:	c3                   	ret    
 8049086:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
 804908d:	8d 76 00             	lea    0x0(%esi),%esi

08049090 <getpid>:
int getpid()
{

    int pid;

    pid = syscall0(SYS_GETPID);
 8049090:	6a 00                	push   $0x0
 8049092:	cd 40                	int    $0x40
 8049094:	83 c4 04             	add    $0x4,%esp
    return pid;
}
 8049097:	c3                   	ret    
 8049098:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
 804909f:	90                   	nop

080490a0 <fork>:

int fork(void)
{
    return (syscall0(SYS_FORK));
 80490a0:	6a 02                	push   $0x2
 80490a2:	cd 40                	int    $0x40
 80490a4:	83 c4 04             	add    $0x4,%esp
}
 80490a7:	c3                   	ret    
 80490a8:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
 80490af:	90                   	nop

080490b0 <kill>:
    pid = syscall0(SYS_GETPID);
 80490b0:	6a 00                	push   $0x0
 80490b2:	cd 40                	int    $0x40
 80490b4:	83 c4 04             	add    $0x4,%esp
{

    // Not Implemented Yet
    // Can Only remove if the requested process is the current process.

    if (pid == getpid())
 80490b7:	39 44 24 04          	cmp    %eax,0x4(%esp)
 80490bb:	75 0b                	jne    80490c8 <kill+0x18>
    ret = syscall1(SYS_EXIT, val);
 80490bd:	ff 74 24 08          	push   0x8(%esp)
 80490c1:	6a 01                	push   $0x1
 80490c3:	cd 40                	int    $0x40
 80490c5:	83 c4 08             	add    $0x8,%esp
        _exit(sig);

    return -1;
}
 80490c8:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
 80490cd:	c3                   	ret    
 80490ce:	66 90                	xchg   %ax,%ax

080490d0 <sbrk>:

int sbrk(int nbytes)
{

    return -1;
}
 80490d0:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
 80490d5:	c3                   	ret    
 80490d6:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
 80490dd:	8d 76 00             	lea    0x0(%esi),%esi

080490e0 <isatty>:
int isatty(int fd)

{

    return 0;
}
 80490e0:	31 c0                	xor    %eax,%eax
 80490e2:	c3                   	ret    
 80490e3:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
 80490ea:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi

080490f0 <close>:
 80490f0:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
 80490f5:	c3                   	ret    
 80490f6:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
 80490fd:	8d 76 00             	lea    0x0(%esi),%esi

08049100 <link>:
}

int link(char *old, char *new)
{
    return -1;
}
 8049100:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
 8049105:	c3                   	ret    
 8049106:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
 804910d:	8d 76 00             	lea    0x0(%esi),%esi

08049110 <lseek>:

int lseek(int file, int ptr, int dir)
{
    return 0;
}
 8049110:	31 c0                	xor    %eax,%eax
 8049112:	c3                   	ret    
 8049113:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
 804911a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi

08049120 <open>:

int open(const char *name, int flags, ...)
{
    return -1;
}
 8049120:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
 8049125:	c3                   	ret    
 8049126:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
 804912d:	8d 76 00             	lea    0x0(%esi),%esi

08049130 <read>:

int read(int file, char *ptr, int len)
{
    return 0;
}
 8049130:	31 c0                	xor    %eax,%eax
 8049132:	c3                   	ret    
 8049133:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
 804913a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi

08049140 <fstat>:

int fstat(int file, void *st)
{
    return 0;
}
 8049140:	31 c0                	xor    %eax,%eax
 8049142:	c3                   	ret    
 8049143:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
 804914a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi

08049150 <stat>:

int stat(const char *file, void *st)
{
    return 0;
}
 8049150:	31 c0                	xor    %eax,%eax
 8049152:	c3                   	ret    
 8049153:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
 804915a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi

08049160 <unlink>:

int unlink(char *name)
{
    return -1;
}
 8049160:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
 8049165:	c3                   	ret    
 8049166:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
 804916d:	8d 76 00             	lea    0x0(%esi),%esi

08049170 <wait>:
 8049170:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
 8049175:	c3                   	ret    
 8049176:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
 804917d:	8d 76 00             	lea    0x0(%esi),%esi

08049180 <gettimeofday>:
 8049180:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
 8049185:	c3                   	ret    
 8049186:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
 804918d:	8d 76 00             	lea    0x0(%esi),%esi

08049190 <UTestSysCall>:

// Syscall for Testing
int UTestSysCall(int a, int b, int c, int d)
{
    int ret = 0;
    ret = syscall3(SYS_TESTING, b, c, d);
 8049190:	ff 74 24 10          	push   0x10(%esp)
 8049194:	ff 74 24 0c          	push   0xc(%esp)
 8049198:	ff 74 24 08          	push   0x8(%esp)
 804919c:	6a 15                	push   $0x15
 804919e:	cd 40                	int    $0x40
 80491a0:	83 c4 10             	add    $0x10,%esp
    return ret;
}
 80491a3:	c3                   	ret    
