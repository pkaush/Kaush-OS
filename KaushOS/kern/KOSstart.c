/*
*  C Implementation: KOSstart.c
*
* Description: The very first intialization file called from the assembly routine. It initializes all the sub systems 
*		and run the very first process.
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/


#include <inc/asm.h>
#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <kern/hal/x86.h>
#include <inc/file.h>

#include <kern/mm/mm.h>
#include <kern/ke/ke.h>

#include <kern/ps/process.h>



/*
Declare some of the prototypes here as these will only be
called from here, so we do not need to put them in header file
*/

void printf_KOS();

void
GeneralTesting();

VOID
IoAndFsTesting();

VOID
Halinit(VOID);

VOID
KeInitPhase0();

VOID
MmInitPhase0();

VOID
PsInitPhase0();

VOID
PsCreateSystemProcess();


#include <kern/fs/ff.h>
void
KOS_start(void)
{



/*
	Clear out the uninitialized global data (BSS) section of our program.
	We have defined these symbols in kern/kernel.ld
*/
	extern char edata[], end[];
	memset(edata, 0, end - edata);
	VgaInit();

	printf_KOS();
	kprintf("Loading Kaush Operating System\n");

	Log(LOG_CRIT,"KOS Logging Start.\n");

#ifdef DEBUG
//	PrintMemSig();
//	GeneralTesting();
#endif

/*
	Do this First as this will initialize the processor structures
	Irqls, Spinlock etc
	 
*/
	KeInitPhase0();

	kprintf("Irql -->%d\n", KeGetCurrentIrql());
	// Memory Manager initialization functions
	MmInitPhase0();

	Ob_init();

	PsInitPhase0();

// Initialize HAL related datastructure
	Halinit();


	KeInitPhase1();

		
			
	IoInit();
	KFSinit();

	




		// No need to enable interrupts here. Our first thread whens starts running it will load new value in EFLAGS
		// Which will have interrupts enabled.


		//Enable Interrupts 
	//	HalEnableInterrupts();



	PsCreateSystemProcess();



		KeBugCheck("We Should never reach here\n");
}



#if 1
VOID
CopyProcess(char *Buff,size_t size, char *filename) {

	KSTATUS ret;
	FIL fp;
	UINT bdone;

	f_chdrive(1);

	ret = f_open( &fp,filename, FA_WRITE| FA_CREATE_NEW);
	kprintf("fopen %d\n", ret);
	ret = f_write( &fp, Buff, size, &bdone);
	kprintf("fwrite %d bytes written %d\n", ret, bdone);

	f_close(&fp);


}



#define COPY_PROCESS(x, y) {			\
	extern uint8_t _binary_obj_##x##_start[],	\
	_binary_obj_##x##_size[];					\
													\
	CopyProcess( _binary_obj_##x##_start, _binary_obj_##x##_size, y);	\
}
#endif

VOID
KosInit()
{

	KMUTANT mutex;
	KIRQL OldIrql;
	BOOL Interrupts;
	FIL fp;
	int ret;

	UINT bdone;

	kprintf("Initialize Disk driver\n");
	disk_init();

	ASSERT(KeGetCurrentThread());


#if 1
{
	KSTATUS Status;
	HANDLE Handle;
	CHAR buff [1024];


	
	f_chdrive(1);
	Status = KCreateFile("/abc.txt", O_RDONLY,&Handle);

	ASSERT1(K_SUCCESS(Status), Status);
	Status = KReadFile(Handle, buff, 30);

	kprintf("fread %s\n", buff);
	ASSERT1(K_SUCCESS(Status), Status);
//	KCloseFile(Handle);
}
#endif




#if 0
{
	FATFS Filesys1;

	f_mount(1, &Filesys1);

//	f_mkfs(1, 1, 512);
//	f_mount(1,NULL);
CHAR buff [1024];



//	ret = f_open( &fp,"1:abc.txt", FA_WRITE| FA_CREATE_NEW);
//	kprintf("fopen %d\n", ret);
//	f_write( &fp, "Let's see this should work         \n", 30, &bdone);
//	kprintf("fwrite %d\n", ret);

	f_chdrive(1);
	ret = f_open( &fp,"/abc.txt", FA_READ);
	kprintf("fopen %d\n", ret);

	
	f_read( &fp, buff, 30, &bdone);
	kprintf("fread %d  : %s\n", ret, buff);



	f_close( &fp);



}
#endif





#if 0
	
		
//	ProcessThreadLocksTests();




	{

	KSTATUS Status;
	HANDLE Handle;
	CHAR Buffer[64];



	printf("format -> %d",f_mkfs( 1, 1, 512));


	Status = KCreateFile("/abc.txt",O_RDWR, &Handle);
	ASSERT1(Status == STATUS_SUCCESS, Status);



	strcpy(Buffer, "My Name is Puneet Kaushik\n");
	Status = KWriteFile( Handle, Buffer, 21);

//	Status = KReadFile( Handle, Buffer, 21);
	ASSERT1(Status == STATUS_SUCCESS, Status);

	printf("String Read :%s\n",Buffer);

	KCloseFile(Handle);

	}

#endif





//	CREATE_PROCESS(user_prog_test);
//	COPY_PROCESS(user_prog_test, "testprog.exe");


	PsCreateProcess(
	UserMode,
	NULL, "testprog.exe", 0, NULL,NULL);


	ObpPrintDirectoryTree(ObRootDirectory,0);
	while(1) {		

	}

}



void
printf_KOS()
{


kprintf("\n	");
kprintf("          #     #                  # #                             ######  \n");
kprintf("          #   #                #           #                       #           #  \n");
kprintf("          # #               #                 #                    #                \n");
kprintf("          ##             #                       #                  ###### \n");
kprintf("          # #               #                 #                                  #  \n");
kprintf("          #   #                #           #                        #          #  \n");
kprintf("          #     #                  # #                             ######  \n");
kprintf("\n	");
kprintf("\n	Loading .........................................\n");

}

/*
 * Variable KeBugCheckstr contains argument to first call to KeBugCheck; used as flag
 * to indicate that the kernel has already called KeBugCheck.
 */
static const char *KeBugCheckstr;
KIRQL BugCheckIrql;

/*
 * Panic is called on unresolvable fatal errors.
 * It prints "KeBugCheck: mesg", and then enters the kernel monitor.
 */
void
_KeBugCheck(const char *file, int line, const char *fmt,...)
{

	va_list ap;
	KIRQL Irql;

	BugCheckIrql = KeGetCurrentIrql();

	KeRaiseIrql(HIGH_LEVEL, &Irql);

	if (KeBugCheckstr) {
/*
	In case we have already bugchecked.
	Though I do not expect this should happen on single proc.
*/		
		goto dead;
	}	
	KeBugCheckstr = fmt;

	va_start(ap, fmt);
	kprintf("kernel KeBugCheck at %s:%d: ", file, line);
	vprintf(fmt, ap);
	kprintf("\n");
	kprintf(" At Irql: %d", Irql);
	va_end(ap);

dead:
	/* break into the kernel monitor */
	while (1);

}


#if 0
void
GeneralTesting()
{

	// tesing locks
	LockTesting();
	// Testing out list functions
	List_testing();

}



VOID
WriteProcessTODisk(PVOID ProcStart, size_t ProcSize)
{



	HANDLE Handle;
	KSTATUS status;


	kprintf(" %s got called for Process size %d", ProcSize);

	status = KCreateFile("/hello", O_CREATE,&Handle);

	if (!success(status)) {
		KeBugCheck(" KcreateFile failed	status %d\n", status);
	}

	status = KWriteFile(Handle,ProcStart, ProcSize);

	if (!success(status)) {
		KeBugCheck(" KWriteFile failed	status %d\n", status);
	}

	KCloseFile(Handle); 

}


VOID
IoAndFsTesting()
{

	KSTATUS status;
	HANDLE Handle;
	char Buffer[128];
	char Buffer1[128];

	status = KCreateFile("/Disclamer", O_RDWR,&Handle);

	if (!success(status)) {
		KeBugCheck(" KcreateFile failed  status %d\n", status);
	}

	status = KReadFile(Handle,Buffer, 124);

	if (!success(status)) {
		KeBugCheck(" KcreateFile failed  status %d\n", status);
	}



	kprintf("Buffer ===> \n %s\n", Buffer);

	KCloseFile(Handle);

// ====== create a file ============

	status = KCreateFile("/kaush0", O_CREATE,&Handle);
	
	if (!success(status)) {
		KeBugCheck(" KcreateFile failed  status %d\n", status);
	}

	strncpy(Buffer, " my name in Kaush \n\n sacchi!!\n", 124);
	status = KWriteFile(Handle,Buffer, 124);

	if (!success(status)) {
		KeBugCheck(" KcreateFile failed  status %d\n", status);
	}

	KCloseFile(Handle);	

	// read it back

	status = KCreateFile("/kaush0", O_RDWR,&Handle);

	if (!success(status)) {
		KeBugCheck(" KcreateFile failed	status %d", status);
	}

	status = KReadFile(Handle,Buffer1, 124);

	if (!success(status)) {
		KeBugCheck(" KcreateFile failed	status %d", status);
	}

	kprintf("Buffer ===> \n %s\n", Buffer1);
	

	
	KCloseFile(Handle);

}







#endif
