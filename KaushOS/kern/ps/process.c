/*
*  C Implementation: Process.c
*
* Description:  Implements the User mode processes.
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010.
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/



#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/elf.h>
#include <inc/file.h>


#include <kern/ps/process.h>
#include <kern/mm/mm.h>
#include <kern/ke/interrupt.h>

#include <kern/hal/x86.h>

#include <kern/fs/ff.h>

#include <kern/ob/ob.h>

POBJECT_TYPE PsProcessType;


// Process Flags
#define PROCESS_FLAG_ALLOCATED	0x01
#define PROCESS_FLAG_INSERTED	0x02
#define PROCESS_FLAG_FIRST_THREAD_CREATED		0x04

#define PROCESS_FLAG_CLEANUP_MASK	0x0f


#define ProcessGetCleanupFlags(Process) ((Process)->Flags & PROCESS_FLAG_CLEANUP_MASK)
#define ProcessSetCleanupFlags(Process, Flag) ((Process)->Flags |= ((Flag) & PROCESS_FLAG_CLEANUP_MASK))
#define ProcessResetCleanupFlags(Process, Flag) ((Process)->Flags &= ~((Flag) & PROCESS_FLAG_CLEANUP_MASK))


// Process ID Table: To lookup Process from PID
HANDLE_TABLE PidTable;

typedef LIST_HEAD(_EPROCESS_LIST, _EPROCESS) EPROCESS_LIST,
													*PEPROCESS_LIST;

static EPROCESS_LIST PsActiveProcessHead;
static KSPIN_LOCK PsActiveProcessHeadLock;



PEPROCESS
PsGetCurrentProcess()
{
	PKTHREAD Thread;
	Thread = KeGetCurrentThread();

	if (Thread) {
		return ((PEPROCESS)Thread->ThreadProcess);
	}

	return NULL;
	
}


PEPROCESS
PsGetProcessFromPID( IN HANDLE ProcessId)
{
	PEPROCESS Process;
	KSTATUS Status;

	Status = ObLookupObjectByHandle(ProcessId, &PidTable,  &Process);

	if (!K_SUCCESS(Status)) {
		ASSERT(0); // remove later
		Process = NULL;
	}


	return NULL;
}



VOID
PsIncProcessRundownProtect(IN PEPROCESS Process)
{
	ObIsValidObject(Process);
	
	InterlockedIncrement( &Process->RundownProtect);
}


VOID
PsDecProcessRundownProtect(PEPROCESS Process)
{
	LONG Ret = 0;
	
	ASSERT(Process->RundownProtect);
	ObIsValidObject(Process);
	
	
	Ret = InterlockedDecrement( &Process->RundownProtect);
	if (Ret == 0) {
		ASSERT1(Process->ActiveThreads == 0, Process->ActiveThreads);
		ObDereferenceObject(Process);
	}
}



static INLINE
HANDLE
PsGetCurrentProcessId()
{
	PEPROCESS Process;

	Process = PsGetCurrentProcess();
	ASSERT(Process);
	return (Process->UniqueProcessId);
}



PVOID
MmGetGlobalPgdir();


INLINE
PVOID
PsGetCurrentProcessPageDirectory()
{
	PEPROCESS Process;

	Process = PsGetCurrentProcess();


	if (Process) {
		return((PVOID)Process->pcb.DirectoryTableBase);
	}

	return (MmGetGlobalPgdir());
}


ULONG_PTR
MmGetGlobalCr3();

VOID
PspAttachProcess(
				IN PEPROCESS NewProcess,
				OUT PRKAPC_STATE  ApcState
				)
/*
	Description: 


	Shift to new process address space

	This will mainly be used for creating a new process
	user address space

	This Should be called at IRQL >= DISPATCH_LEVEL
	so that we do not get rescheduled

	Environment:

	This Function should always be called with DispatcherLock acquire.


*/


{

#ifdef DEBUG
{
	PKTHREAD DThread;
	ULONG Result = KiQueryDispatcherDatabase( &DThread);

	ASSERT(Result == 1);
	ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
}
#endif


	KeStackAttachProcess((PKPROCESS)NewProcess, ApcState);
}




VOID
PspDetachProcess(
				IN PRKAPC_STATE  ApcState
				)
/*
	Description: 


	Shift to new process address space

	This will mainly be used for creating a new process
	user address space

	This Should be called at IRQL >= DISPATCH_LEVEL
	so that we do not get rescheduled

	Environment:

	This Function should always be called with DispatcherLock acquire.


*/


{

#ifdef DEBUG
{
	PKTHREAD DThread;
	ULONG Result = KiQueryDispatcherDatabase( &DThread);

	ASSERT(Result == 1);
	ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
}
#endif


/*


*/

	KeUnstackDetachProcess(ApcState);
}

///////////////////////////////
// Insert and Remove
static INLINE
KSTATUS
PspRemoveProcess(IN PEPROCESS Process)
{
	KIRQL OldIrql;

	ASSERT(Process);
	ASSERT1(	Process->pcb.State == PROCESS_STATE_TERMINATED, Process->pcb.State);

	KeAcquireSpinLock( &PsActiveProcessHeadLock, &OldIrql);
	LIST_REMOVE( &PsActiveProcessHead, Process,ProcessLink);

	KeReleaseSpinLock( &PsActiveProcessHeadLock, OldIrql);


	return STATUS_SUCCESS;

}


static INLINE
KSTATUS
PspInsertProcess(IN PEPROCESS Process)
{
	KIRQL OldIrql;

	ASSERT(Process);
	

	KeAcquireSpinLock( &PsActiveProcessHeadLock, &OldIrql);
	LIST_INSERT_TAIL( &PsActiveProcessHead,Process,ProcessLink);


	// Make the Process Active
	Process->pcb.State = PROCESS_STATE_ACTIVE;

	
	KeReleaseSpinLock( &PsActiveProcessHeadLock, OldIrql);


	return STATUS_SUCCESS;
}


// keep this prototype here
KSTATUS
MmCreateProcessAddressSpace(PVOID *PageDirectory);


/////////////////
// Allocate and Free
static INLINE
KSTATUS
PspFreeProcess(
				IN PEPROCESS Process
				)
{
	ASSERT(Process);

	// Only do this clean up if allocation went through

	if (ProcessGetCleanupFlags(Process) & PROCESS_FLAG_ALLOCATED) {



		if (Process->Parent) {
			PsDecProcessRundownProtect( Process->Parent);
		}

///////////////////////
// Remove the process
		ObRemoveHandle( Process->UniqueProcessId, &PidTable, Process);



////////////////////////
// Clean-up the Process Address Space

		MmDeleteProcessAddressSpace( (PVOID)Process->pcb.DirectoryTableBase);



	}
	return STATUS_SUCCESS;
}


#define PspReleaseBinary(Buffer) ExFreePoolWithTag(Buffer,'ELIF')


static INLINE
KSTATUS
PspReadBinary(PCHAR FileName, PVOID *Buffer, PLONG BinarySize)
{

	KSTATUS Status;
	FIL FilePointer, *fp;
	LONG FileSize;
	PVOID LocalBuff;
	LONG BytesRead;

	ASSERT(Buffer);
	ASSERT(BinarySize);

	fp = &FilePointer;


// Open the File as read only
	Status =f_open(fp, FileName, FA_READ);

	if ( !K_SUCCESS(Status)) {
		return Status;
	}

	FileSize = fp->fsize;
//Check if the file is not size zero
	if (FileSize <= 0) {
		return STATUS_INVALID_FILE_FOR_SECTION;
	}


//Allocate the buffer to read the file in mem
	LocalBuff = ExAllocatePoolWithTag( NonPagedPool, FileSize+20,'ELIF');

	if (!LocalBuff) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}

//Read the file
	Status = f_read( fp, LocalBuff,FileSize, &BytesRead);
	
	if ( (!K_SUCCESS(Status)) || (BytesRead < FileSize)) {

		ExFreePoolWithTag(LocalBuff,'ELIF');
		return Status;
	}


	f_close(fp);
	*BinarySize = FileSize;
	*Buffer = LocalBuff;
	
	return STATUS_SUCCESS;
}


static INLINE
KSTATUS
PspAllocateProcess(
				IN PEPROCESS Parent,
				IN PCHAR Binary,
				IN size_t size,
				IN POBJECT_ATTRIBUTES ObjectAttributes,
				IN KPROCESSOR_MODE ProbeMode,
				OUT PEPROCESS *NewProcess
				)
{

	KSTATUS Status;

	PEPROCESS Process;
	PKPROCESS KProcessBlock;
	PVOID EntryPoint = NULL;
	PVOID PageDirectory;
	KAPC_STATE ApcState;
	HANDLE ProcessId;
	BOOL IsCloneProcess = FALSE;
	PHANDLE_TABLE ParentHandleTable = NULL;
	KIRQL OldIrql;

	PVOID BinData = NULL;
	LONG BinSize = 0;
	
	

	//Lets verify our binary file first, because if we fail here 
	//we do not need to do anything else and we can safely return from here.
	if (Binary != NULL) {

		Status = PspReadBinary(Binary, &BinData, &BinSize);

		if (!K_SUCCESS(Status)) {
			kprintf("Failed to Read the Binary for the Process: 0x %x", Status);
			return Status;
		}

	}


	Process = NULL;
	Status = ObCreateObject(	ProbeMode, 
									PsProcessType, 
									ObjectAttributes,
									sizeof(EPROCESS),
									&Process);

	if (!K_SUCCESS(Status)) {

		ASSERT(0);
		if (BinData) {
			PspReleaseBinary(BinData);
		}
		return Status;
	}


// We do not need to memset here, CreateObject would have already done that for us.
//	memset(Process, 0, sizeof(EPROCESS));



/////////////////////////////////////////////
// Initialize Executive Process First


// Get the Unique Process Id for this Process
	Status = ObInsertHandle( Process, &PidTable, &ProcessId);

	if (!K_SUCCESS(Status)) {
// This should only happen if we have reached the Max Process
		ASSERT(0);

		if (BinData) {
			PspReleaseBinary(BinData);
		}
		
		ObDereferenceObject(Process);
		return Status;
	}

//If there is no binary and it has a parent process, it means we want to fork the process

	if ((Binary == NULL) && (Parent)) {
		IsCloneProcess = TRUE;
	}



	if (Parent) {
		ParentHandleTable = &Parent->HandleTable;
	}


	Status = ObAllocateHandleTable(&Process->HandleTable,  ParentHandleTable, IsCloneProcess);

	ASSERT(Status == STATUS_SUCCESS); // For now

	if (Binary) {
		strncpy(Process->ImageFileName,  Binary, MAX_NAME_LENGTH);

	}
	Process->UniqueProcessId = ProcessId;
	if (Parent) {
		Process->ParentProcessId = Parent->UniqueProcessId;
		
		/*
			Reference the parent structure so that parent doesn't go away
			while the child is there.
		
			Decrement this in terminate process.
		
			Don't terminate a process untill its Rundownprotection is not 0
		*/
			PsIncProcessRundownProtect(Parent);
	}

	Process->Parent = Parent;
	LIST_INIT( &Process->ThreadListHead);


///////////////////////////////////////////////
//Initialize Process Control Block

	KProcessBlock = &Process->pcb;

	KeInitializeSpinLock(&KProcessBlock->ProcessLock);
	KProcessBlock->BasePriority =PROCESS_PRIORITY_NORMAL;
	KProcessBlock->State = PROCESS_STATE_CREATION;
	KProcessBlock->Quantum = DEFAULT_PROCESS_QUANTUM;
	KeInitDispatcherHeader( &KProcessBlock->Header, ProcessObject, 0);

// Create Process Address Space
	Status = MmCreateProcessAddressSpace( &PageDirectory);
	if (!K_SUCCESS(Status)) {

		ObFreeHandleTable( &Process->HandleTable);
		
		if (Parent) {
			PsDecProcessRundownProtect(Parent);
		}


		if (BinData) {
			PspReleaseBinary(BinData);
		}
		
		ObDereferenceObject(Process);
		
		KeBugCheck("error:%d\n", Status);
		return Status;
	}
	
	KProcessBlock->DirectoryTableBase = (ULONG_PTR)PageDirectory;
//Load our binary. Soon we will be loading it from FS using IO manager

	if (IsCloneProcess) {

		ASSERT(Parent);

//While Clonning a process, we should not read a binary.
		ASSERT(BinData == NULL);
		ASSERT(BinSize == 0);
		
		KiLockDispatcherDataBase(&OldIrql);
		Status = MmCloneProcessAddressSpace( 
									Parent->pcb.DirectoryTableBase, 
									PageDirectory
									);

		KiUnLockDispatcherDataBase( OldIrql);


		if (!K_SUCCESS(Status)) {

			ObFreeHandleTable( &Process->HandleTable);
			
			if (Parent) {
				PsDecProcessRundownProtect(Parent);
			}

			MmDeleteProcessAddressSpace(PageDirectory);

			ObDereferenceObject(Process);
			
			KeBugCheck("error:%d\n", Status);
			return Status;
		}

		Process->pcb.ProcessEntryPoint = NULL;


	} else if (Binary) {

//While Creating a new process, we should have already read the binary.
		ASSERT(BinData != NULL);
		ASSERT(BinSize != 0);

		
		KiLockDispatcherDataBase(&OldIrql);
		PspAttachProcess(Process, &ApcState);

		ELFBinaryLoader(Process, BinData, BinSize);


		PspDetachProcess( &ApcState);
		KiUnLockDispatcherDataBase( OldIrql);

// Release the Binary we are done with it.
		PspReleaseBinary(BinData);
	}


/*
	Allocate one page to keep track of the user mode page pool
	And Create the bitmap in that.

	Now everything in MmInitializeUserPtes
*/


	if (IsCloneProcess) {
		Status = MmCloneUserPtes(&KProcessBlock->UserPtePool, Parent->pcb.UserPtePool);
	} else {
		Status = MmInitializeUserPtes(&KProcessBlock->UserPtePool);
	}
	
	if ( !K_SUCCESS(Status)) {

		MmDeleteProcessAddressSpace( (PVOID)KProcessBlock->DirectoryTableBase);

		ObFreeHandleTable( &Process->HandleTable);
		
		if (Parent) {
			PsDecProcessRundownProtect(Parent);
		}
		ObDereferenceObject(Process);
		KeBugCheck("error:%d\n", Status);
		return Status;

	}


	*NewProcess = Process;
	return STATUS_SUCCESS;
}



KSTATUS
PsCreateProcess(
						IN KPROCESSOR_MODE ProbeMode,
						IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
						IN PCHAR Binary,
						IN size_t size,
						IN PHANDLE ChildProcessid,
						IN PEPROCESS *CreatedProcess
						)

/*++

In Clone Process we will:

1. Clone Handle table
2. Clone Process Address Space
3. Create a new thread (Only One)
4. Clone the thread's, who called fork, Trapframe to the new thread

--*/


{
	KSTATUS Status;
	PEPROCESS NewProcess = NULL;
	KIRQL OldIrql;

	Status = PspAllocateProcess(
					PsGetCurrentProcess(),
					Binary,
					size,
					ObjectAttributes,
					ProbeMode,
					&NewProcess
					);


	if (!K_SUCCESS(Status)) {
		KeBugCheck("Wasn't able to allocate a Process Object error:%x\n", Status);
		return Status;
	}


	ASSERT1(ProcessGetCleanupFlags(NewProcess) == 0, ProcessGetCleanupFlags(NewProcess));

	ProcessSetCleanupFlags(NewProcess, PROCESS_FLAG_ALLOCATED);

// Insert our new process into Active list
	Status = PspInsertProcess(NewProcess);
	ASSERT1(ProcessGetCleanupFlags(NewProcess) == PROCESS_FLAG_ALLOCATED,
								ProcessGetCleanupFlags(NewProcess));

	if (!K_SUCCESS(Status)) {
		goto ErrorExit;
	}

	ProcessSetCleanupFlags(NewProcess, PROCESS_FLAG_INSERTED);



// now Create one thread to run in the process


// If the thread get created successfully it might start executing before this function returns success
// so lets set the cleanup flag before, and reset in case of failure
// We can raise IRQL but that will become a bug on multiproc

	ProcessSetCleanupFlags(NewProcess, PROCESS_FLAG_FIRST_THREAD_CREATED);


	Status = PspCreateThread(
					NewProcess,
					"Test ROutine",
					NewProcess->pcb.ProcessEntryPoint,
					NULL,
					NULL);

	if (!K_SUCCESS(Status)) {
		ProcessResetCleanupFlags(NewProcess, PROCESS_FLAG_FIRST_THREAD_CREATED);
		goto ErrorExit;
	}


//	ASSERT1(ProcessGetCleanupFlags(NewProcess) == (PROCESS_FLAG_ALLOCATED | 
//							PROCESS_FLAG_INSERTED|PROCESS_FLAG_FIRST_THREAD_CREATED),
//									ProcessGetCleanupFlags(NewProcess));





	Log(LOG_DEBUG,"[%x] Process Created by Parent [%x]\n", NewProcess->UniqueProcessId,
							NewProcess->ParentProcessId);

	if (CreatedProcess) {
		*CreatedProcess = NewProcess;
	}

	if (ChildProcessid) {
		*ChildProcessid = NewProcess->UniqueProcessId;
	}

	
	return STATUS_SUCCESS;


ErrorExit:
		Log(LOG_DEBUG,"[%x] Process Creation Failed. Parent [%x]\n", NewProcess->UniqueProcessId,
									NewProcess->ParentProcessId);
		ObDereferenceObject(NewProcess);
		return Status;

}


static INLINE
KSTATUS
PspExitProcess(
						IN PEPROCESS Process
						)

{
	PETHREAD CurrentThread;
	PETHREAD LastThread = NULL, NextThread = NULL;
	KIRQL Irql;
	KSTATUS Status;

	CurrentThread = (PETHREAD) KeGetCurrentThread();


	ASSERT(ObIsValidObject(Process));
	ASSERT1(Process->pcb.State != PROCESS_STATE_TERMINATED, Process->pcb.State);

	// we should not have reached here if the process creation was not completed
	ASSERT1(ProcessGetCleanupFlags(Process) & PROCESS_FLAG_FIRST_THREAD_CREATED,ProcessGetCleanupFlags(Process));
	ASSERT1(ProcessGetCleanupFlags(Process) & PROCESS_FLAG_INSERTED, ProcessGetCleanupFlags(Process));
	ASSERT1(ProcessGetCleanupFlags(Process) & PROCESS_FLAG_ALLOCATED,ProcessGetCleanupFlags(Process));


	// Mark the process as terminated
	Process->pcb.State = PROCESS_STATE_TERMINATED;
	KeReleaseNotificationDHeader( &Process->pcb.Header,0);


	//Take a ref to the object
	ObReferenceObject(Process);
	
	if (ProcessGetCleanupFlags(Process) & PROCESS_FLAG_FIRST_THREAD_CREATED) {

		// Acquire the spin lock
		KeAcquireSpinLock( &Process->pcb.ProcessLock, &Irql);

		//Exit all the threads
		while(!LIST_EMPTY( &Process->ThreadListHead)) {
			PKTHREAD ExitedThread;
			
			LIST_REMOVE_HEAD( &Process->ThreadListHead, NextThread, Process_link);

			// If the current thread belongs to the same process then,
			//don't exit it or the whole process will be stalled. Mark it and exit it last
			if (NextThread != CurrentThread) {
				PsExitThread( (PVOID) ExitedThread);
			} else {

				LastThread = CurrentThread;
			}
		}

		KeReleaseSpinLock( &Process->pcb.ProcessLock, Irql);


		ProcessResetCleanupFlags(Process, PROCESS_FLAG_FIRST_THREAD_CREATED);
	}


	if (ProcessGetCleanupFlags(Process) & PROCESS_FLAG_INSERTED) {

		Status = PspRemoveProcess( Process);
		ASSERT(K_SUCCESS(Status));
		ProcessResetCleanupFlags(Process, PROCESS_FLAG_INSERTED);
	}



	ObDereferenceObject(Process);

	if (LastThread) {
		ASSERT(LastThread == CurrentThread);
		PsExitThread(  LastThread);
		KeBugCheck("Should not come back here\n");
	}


}





HANDLE
KCloneProcess()
{
	HANDLE ChildProcessid;
	KSTATUS Status;
	
	Status = PsCreateProcess(UserMode, NULL, NULL, 0, &ChildProcessid, NULL);
	if (!K_SUCCESS(Status)) {
		return Status;
	}
	
	return ChildProcessid;
}




KSTATUS
KExitProcess(
				IN HANDLE ProcessId,
				IN ULONG ExitStatus
				)
{
	HANDLE Pid;
	PEPROCESS CurrentProcess;
	PEPROCESS TargetProcess;
	KSTATUS Status;

	CurrentProcess = PsGetCurrentProcess();

	kprintf(" KExitProcess: [%d]\n", ProcessId);

//We will only allow the exit of a process if:

// A process is exiting itself

	if (ProcessId == 0) {
			// ProcessId == 0 means a process exiting itself
		Status = PspExitProcess( CurrentProcess);
		goto KExitProcessExit;

	}
		
	if (CurrentProcess->UniqueProcessId == ProcessId) {

		Status = PspExitProcess(CurrentProcess);
		goto KExitProcessExit;

	}

	TargetProcess = PsGetProcessFromPID( ProcessId);

//if the target process is the child of the current process
	if (TargetProcess->Parent == CurrentProcess) {

		Status = PspExitProcess(TargetProcess);
		goto KExitProcessExit;


	} else {

		ASSERT2(0, CurrentProcess,  TargetProcess);
	}



KExitProcessExit:
	return Status;
	
	
}

HANDLE
KGetCurrentProcessId()
{
	return (PsGetCurrentProcessId());
}


static INLINE
KSTATUS
PspCloseProcess(
						IN PEPROCESS Process
						)
{
	KSTATUS Status;
	PETHREAD Thread;

	
	ASSERT(ObIsValidObject(Process));

	Thread = (PETHREAD) KeGetCurrentThread();

	// Cannot get called in process thread context, because all the threads,
	//which belong to this process should have already exited.
	ASSERT(Thread->ThreadsProcess != Process);
	ASSERT(Process->RundownProtect == 0);

	return (PspFreeProcess(Process));
}













#if 0

//PKSTART_ROUTINE SystemProcessInitalThread;


VOID
SystemProcessInitalThread (
				    	IN PVOID StartContext
    					)
{
	int i;
	for(i = 0;;i++) {
		kprintf("System Process %d\n", i);
	}

	
	KeBugCheck("System Process");

}


VOID
IdleThread (
				    	IN PVOID StartContext
    					)
{

	int i;

	
	for(i = 0;;i++) {
		kprintf("This is Idle Process %d\n", i);
		Schedule();
	}
	
	KeBugCheck("Idle Process");

}


#endif


VOID
PsInitPhase0()
{
	
	KSTATUS Status;
	OBJECT_TYPE ObjectType;
	
	
	LIST_INIT(&PsActiveProcessHead);

	Status = ObAllocateHandleTable( &PidTable, NULL, FALSE);

	if (!success(Status)) {
		KeBugCheck("Unable to Allocate Process ID Table\n");
	}
	
	memset( &ObjectType, 0, sizeof (OBJECT_TYPE));
	ObjectType.TypeIndex = OBJECT_TYPE_PROCESS;
	ObjectType.PoolType = NonPagedPool;
	ObjectType.ObjectSize = sizeof(EPROCESS);
	ObjectType.CloseProcedure = PspCloseProcess;

	
	Status = ObCreateObjectType( &ObjectType, &PsProcessType);
	

	ASSERT(success(Status));


// Initialize Thread
	PsThreadInit();	

}







/************************************************
			S Y S T E M  P R O C E S S
************************************************/



static PEPROCESS PsInitialSystemProcess;



//#define TEST_MUTANT


#ifdef TEST_MUTANT
static KMUTANT TestWaitLock;
#else //Test Semaphore
static KSEMAPHORE TestWaitLock;
#endif


VOID
PspIdleThread(
				IN PVOID StartContext
				)


{
	KIRQL OldIrql;


	while(1);

	while(1) {
		KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

		kprintf("Thread Idle %d\n", KeGetCurrentIrql());
		KeLowerIrql(OldIrql);

	}
	KeBugCheck("End");

}

///////////////////////////////////////////////////////////
//////////T E S T   C O D E ///////////////////////////////
//////////////////////////////////////////////////////////



#include <kern/fs/ff.h>

void fstry()
{

		kprintf("fstry\n");

		{
			
		FATFS filesys1;
		FIL fp;
		CHAR buff[1024];
		int num = 0;
		int ret = 0;
		
		

		ret = f_mount(1, &filesys1);


		kprintf("ret1 ->%d\n", ret);



#if 0
		kprintf("ret4 ->%d\n",f_mkfs(1, 0, 512));
#endif


#if 0

		ret = f_open( &fp,"1:abc1.txt", FA_WRITE | FA_CREATE_NEW	);
		kprintf("ret2 ->%d\n", ret);
		strcpy(buff, "My Name is Puneet Kaushik\n");
		ret = f_write( &fp, buff, 20, &num);
		f_sync(&fp);
		f_close(&fp);
		kprintf("ret3 ->%d\n", ret);
		kprintf("bytes writen ->%d\n", num);

#endif
#if 1

		ret = f_open( &fp,"1:abc1.txt", FA_READ);
		kprintf("ret2 ->%d\n", ret);

		ret = f_read( &fp, buff, 20, &num);

		kprintf("ret3 ->%d\n", ret);
		kprintf("bytes read ->%d\n", num);
		kprintf("buff ->%s\n", buff);


#endif


		}


		kprintf("fstry done\n");

		while (1);

}




VOID
PspTestThread (PVOID StartContext)
{

	ULONG num = (ULONG) StartContext;
	KIRQL OldIrql;
	ULONG count = 0;
	
	kprintf("Thread number %d %d Blocking on Mutant\n", num, KeGetCurrentIrql());

	KeWaitForSingleObject(
		&TestWaitLock, Executive, KernelMode, FALSE,
			NULL);

	kprintf("Thread number %d %d Acquired\n", num, KeGetCurrentIrql());



	for (count = 0; count < 200; count++) {
		KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
		kprintf("Thread number %d %d %d\n", num, KeGetCurrentIrql(), count);
		KeLowerIrql(OldIrql);
	}


	kprintf("Thread number %d %dReleasing Mutant\n", num, KeGetCurrentIrql());

#ifdef TEST_MUTANT
	KeReleaseMutant( &TestWaitLock,
							0,
							FALSE,FALSE);

#else
	KeReleaseSemaphore(
	   &TestWaitLock, 0, 1, FALSE);
#endif

	for (count = 0; count < 5; count++) {
		KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
		kprintf("Thread number %d %d\n", num, KeGetCurrentIrql());
		KeLowerIrql(OldIrql);
	}


	while(1);



KeBugCheck("End");

}




static VOID
ProcessThreadLocksTests();





static VOID
ProcessThreadLocksTests()
{
	ULONG count;
	PETHREAD Thread;
	KIRQL OldIrql;

/*
	Initialize Mutant or Semaphore for testing
*/


#ifdef TEST_MUTANT
	KeInitializeMutant(	&TestWaitLock, FALSE);
#else
	KeInitializeSemaphore(&TestWaitLock,3, 3);
#endif




/*
	Create 10 threads
*/

	
	for (count = 0; count < 10; count++) {


		PspCreateThread(PsInitialSystemProcess,
					"TestThread",
					&PspTestThread , (PVOID)count, &Thread);

	}





KeWaitForSingleObject(
	&TestWaitLock, Executive, KernelMode, FALSE,
		NULL);


for (count = 0; count < 200; count++) {


	KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
	kprintf("Thread System %d %d\n", KeGetCurrentIrql(), count);
	KeLowerIrql(OldIrql);

}



#ifdef TEST_MUTANT
	 KeReleaseMutant( &TestWaitLock,
					 0, FALSE,FALSE);
#else
	 KeReleaseSemaphore(
 		&TestWaitLock, 0, 1, FALSE);
#endif

while(1);



}


//////////////////////////////////////////////
////  E N D   T E S T C O D E /////////////////
//////////////////////////////////////////////


VOID
PsSystemProcessStart(
				IN PVOID StartContext
				)


{
	KMUTANT Mutant;


	KIRQL OldIrql;

	ULONG NumProcs;
	ULONG count;
	PETHREAD Thread;
	PETHREAD CurrentThread;
	BOOL IntsEnabled = FALSE;

	IntsEnabled = HalDisableInterrupts();

	kprintf("Inside SystemProcess");

	kprintf("Start %x\n", StartContext);
// Set the Current Thread
	KiSetCurrentThread((PKTHREAD) StartContext);


	CurrentThread =KeGetCurrentThread();

	ASSERT(CurrentThread);


//Set the Current Thread State
	CurrentThread->tcb.State = THREAD_STATE_RUNNING;




	ASSERT(PsInitialSystemProcess);



/*
	Create Idle Threads - One per processor

*/

	

	NumProcs = KeGetNumberOfProcessors();

	for (count = 0; count < NumProcs; count++) {


		PspCreateThread(PsInitialSystemProcess,
					"IdleThread",
					PspIdleThread , NULL, &Thread);


		PsSetTheadPriority(Thread, PROCESS_PRIORITY_IDLE);
		if (count > 0) {


/*
	We need to Implment Processor Affinity in the Scheduler before going further with Multiproc

	We will need that to tell the scheduler that these are threads that are only meant to be run on
	that specific processors
*/
			KeBugCheck("Affinity is not yet implemented");

			
		}



		KiSetIdleThread((PKTHREAD)Thread, count);


	}


/*

	We cannot or should not acquire any lock 
	before this point except SpinLock

*/

	if (IntsEnabled) {
		HalEnableInterrupts();
	}

	
	KosInit();


	while(1);

	KeBugCheck("End of system thread");


}





static VOID INLINE
SyspCreateFirstSystemThread(
						IN PEPROCESS Process,
						OUT PETHREAD *Thread
						)
{

	PETHREAD NewThread;
	PKTHREAD Tcb;
	PKPROCESS Pcb = &Process->pcb;
	
	NewThread = MmAllocatePoolWithTag(
			NonPagedPool, sizeof(ETHREAD), 'RHTE');
	
	if (NewThread == NULL) {
		KeBugCheck("First Thread Mem Allocation Failed\n");
	}
	
	memset(NewThread, 0, sizeof(ETHREAD));
	
//Initialize Thread Control Block
		Tcb = &NewThread->tcb;
		Tcb->KernelStack = (ULONG_PTR) MmAllocateSystemPtes(THREAD_STACK_SIZE);
	
		if ( Tcb->KernelStack == 0) {
/*
	 Do not Bother about freeing mem here as we will eithway
	 BugCheck for this
			MmFreePoolWithTag(NewThread, 'RHTE');
*/
			KeBugCheck("Stack Allocation failed\n");
		}
	
	
		Tcb->PreviousMode = KernelMode;
	
		Tcb->Priority = Pcb->BasePriority;
		Tcb->ThreadProcess = (PKPROCESS) Process;
		Tcb->State = THREAD_STATE_INITIALIZED;
		KeInitializeSpinLock(&Tcb->ThreadLock);
	
	//TODO: Put something here
		NewThread->Cid = 0;
	
	//TODO:
	//	NewThread->CreateTime = get current time
	
		NewThread->ThreadsProcess = Process;
	
		*Thread = NewThread;
	
	}



VOID
PsCreateSystemProcess()
{
	PEPROCESS NewProcess;
	KSTATUS Status;
	PETHREAD Thread;
	KIRQL OldIrql;
	ULONG_PTR StackPointer;
	OBJECT_ATTRIBUTES ObjectAttributes;


	InitializeObjectAttributes(	&ObjectAttributes,
						NULL,
						OBJ_PERMANENT | OBJ_KERNEL_HANDLE,
						NULL,
						NULL);


	Status = PspAllocateProcess(
					NULL,
					NULL,
					0,
					&ObjectAttributes,
					KernelMode,
					&NewProcess
					);


	if(!success(Status)) {
		KeBugCheck("Creation of system process failed\n");
	}

	PsInitialSystemProcess = NewProcess;
	
	Log(LOG_DEBUG, "Created System Process %x\n", NewProcess);

	Status = PspInsertProcess(NewProcess);
	if(!success(Status)) {
		KeBugCheck("Insertion of system process failed\n");
	}

	SyspCreateFirstSystemThread(NewProcess, &Thread);

	ASSERT(Thread);


	strncpy(Thread->Name, "FIrst System Thread", MAX_NAME_LENGTH);

	LIST_INSERT_TAIL(&NewProcess->ThreadListHead, 
							Thread, 
							Process_link);

#if 1
	Status = KiCreateThreadContext(
					&Thread->tcb,
					NULL,
					PsSystemProcessStart,
					Thread
					);
#else

	Status = SyspCreateThreadContext(
				&Thread->tcb,
				PsSystemProcessStart,
				"Starting  SystemProcess"
				);



#endif


/*
	Jump to new process and thread

*/
	KiLockDispatcherDataBase( &OldIrql);
	KiAttachProcess( &NewProcess->pcb);

	KiSetCurrentThread( Thread);
	
	KiContextSwitch(Thread->tcb.StackPointer,
			&StackPointer);


	KeBugCheck("We Should Never Reach here\n");
}





