/*
*  C Implementation: Process.c
*
* Description:  Implements the User mode processes.
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/




#include "process.h"



// Thread Flags
#define THREAD_FLAG_ALLOCATED	0x01
#define THREAD_FLAG_INSERTED	0x02
#define THREAD_FLAG_CONTEXT_CREATED	0x04

#define THREAD_FLAG_EXEC_STATE_MASK	0x0f





POBJECT_TYPE	ThreadObjectType;

PETHREAD
PsGetCurrentThread()
{

	return ((PETHREAD)KeGetCurrentThread());
}


VOID
PsSetCurrentThread(
					PETHREAD Thread
				)
{
	KiSetCurrentThread((PKTHREAD)Thread);
}


VOID
PsSetTheadPriority(PETHREAD Thread, PROCESS_PRIORITY Priority)
{

	Thread->tcb.Priority = Priority;

}




static
BOOL
PsIsLastThread(IN PETHREAD Thread)
{
	PEPROCESS Process;

	Process = Thread->ThreadsProcess;
	return(Process->ActiveThreads == 1);
}



VOID
PsExitThread(IN PETHREAD Thread)
{
	PETHREAD CurrentThread;

	
	CurrentThread = (PETHREAD) KeGetCurrentThread();

	if (Thread == NULL)  {

		Thread = CurrentThread;
	}


	if (Thread == CurrentThread) {

		//If the thread is trying to remove itself, we mark it as terminated
		//and reschedule. The new thread will first do the thread cleanup for this.

		ASSERT1(Thread->tcb.State == THREAD_STATE_RUNNING, Thread->tcb.State);

		//Mark the Thread as terminated and release anyone waiting for its terminatrion
		Thread->tcb.State = THREAD_STATE_TERMINATED;
		KeReleaseNotificationDHeader( &Thread->tcb.Header,0);

		
		Schedule(NULL, NULL, NULL, NULL);
		KeBugCheck("Should Never Return here\n");

	} else {

		// The Thread we are trying to terminate should not be running.
		ASSERT1(Thread->tcb.State != THREAD_STATE_RUNNING, Thread->tcb.State);

		// Pull it out of the scheduler queue, if it is there

		if (Thread->tcb.State == THREAD_STATE_READY) {
			KiStandbyThread( &Thread->tcb);
		}

		if (Thread->tcb.State != THREAD_STATE_TERMINATED) {

			
			Thread->tcb.State = THREAD_STATE_TERMINATED;
			KeReleaseNotificationDHeader( &Thread->tcb.Header,0);
		}

		// dereference the Object
		ObDereferenceObject( Thread);
	}


}

VOID
PsCloseThread(IN PETHREAD Thread)
{

	PKTHREAD Tcb;
	PEPROCESS Process;
	KIRQL OldIrql;
	BOOL AttachedToProcess = FALSE;

	// Well, we are assuming that we will never remove a thread in its own context

	ASSERT((PETHREAD)KeGetCurrentThread() != Thread);

	Tcb = &Thread->tcb;
	// Thread should have been already terminated
	ASSERT1(Tcb->State == THREAD_STATE_TERMINATED, Tcb->State);

	// Flag Should always say that the Object is allocated, unless PsCloseThread is not the function.
	ASSERT1(Thread->Flags & THREAD_FLAG_ALLOCATED, Thread->Flags);

	//Get the Thread's Process
	Process = Thread->ThreadsProcess;
	ASSERT(Process);
	ASSERT1(Process->ActiveThreads, Process->ActiveThreads);


// Its allowed now, please remove this when hits	
	if  ((PEPROCESS)KeGetCurrentProcess() == Process) {

		KeBugCheck("A Process is not allowed to close a thread which belongs to different Process\n");
	}

	// Get the Process Lock before while terminating a thread
	KeAcquireSpinLock( &Process->pcb.ProcessLock, &OldIrql);

	Tcb = &Thread->tcb;


	// Check if the Thread Context was created
	if (Thread->Flags & THREAD_FLAG_CONTEXT_CREATED) {

		// Remove the flag
		Thread->Flags &=  ~THREAD_FLAG_CONTEXT_CREATED;

		ASSERT1(Thread->Flags == (THREAD_FLAG_ALLOCATED | THREAD_FLAG_INSERTED), Thread->Flags);

	}
	
	
	if (Thread->Flags & THREAD_FLAG_INSERTED) {

		// Remove the flag
		Thread->Flags &=  ~THREAD_FLAG_INSERTED;

		ASSERT1(Thread->Flags == THREAD_FLAG_ALLOCATED, Thread->Flags);


		// Remove from the list
		LIST_REMOVE(&Process->ThreadListHead, 
										Thread, 
										Process_link);
		
	}

		// This should always be true
	if (Thread->Flags & THREAD_FLAG_ALLOCATED) {

		// Remove the flag
		Thread->Flags &=  ~THREAD_FLAG_ALLOCATED;

		// Now all the flags should have got removed
		ASSERT(Thread->Flags  == 0);
	}



	if ( (PEPROCESS)KeGetCurrentProcess() != Process) {

		/*
			Lock the dispatcher database before attaching to a new process
			address space. Just to be on the safer side
		
		*/
		KiLockDispatcherDataBase( &OldIrql);
		
		PspAttachProcess(Process, NULL);
		AttachedToProcess = TRUE;
	}

	// check it again
	ASSERT ( (PEPROCESS)KeGetCurrentProcess() != Process);

	// Free the User stack we do not need it now
	MmFreeUserPtes(Tcb->UserStack, Process->pcb.UserPtePool, THREAD_USER_STACK_SIZE);		
	{
		PKPROCESS Process = Tcb->ThreadProcess; 
		
//		kprintf("=====Free Stack Thread 0x%x, UserStack 0x%x, Pgdir 0x%x\n\n", 
//			Thread, Thread->tcb.UserStack, Process->DirectoryTableBase);
	}

	// Go to the old process address space

	if (AttachedToProcess) {
		PspDetachProcess(NULL);
		KiUnLockDispatcherDataBase(OldIrql);
	}



	// We Cannot Free the kernel stack if it is for the current thread.
	// We need it to execute right now. A ref to this should have been taken before 
	// and will be freed with context switch
	if (KeGetCurrentThread() != Thread) {

		MmFreeSystemPtes( Tcb->KernelStack, THREAD_STACK_SIZE);
	}

	InterlockedDecrement( &Process->ActiveThreads);
	PsDecProcessRundownProtect(Process);
	KeReleaseSpinLock(&Process->pcb.ProcessLock, OldIrql);

}






KSTATUS
PspAllocateThread(
					IN PEPROCESS Process,
					OUT PETHREAD *Thread,
					BOOL IsClone
					)

/*

	PspAllocateThread Should be called with the
		&Process->pcb.ProcessLock spinlock acquired.

*/

{



	PETHREAD NewThread;
	PETHREAD CurrentThread;
	PKTHREAD Tcb;
	PEPROCESS OldProcess;
	KIRQL OldIrql;
	PKPROCESS Pcb = &Process->pcb;
	KSTATUS Status;
	BOOL AttachedToProcess = FALSE;

#ifdef DEBUG
{

	KIRQL DebugIrql;
	PKTHREAD DebugThread, DebugThread1;
	ULONG Result;



	DebugIrql = KeGetCurrentIrql();
	ASSERT1(DebugIrql == DISPATCH_LEVEL, DebugIrql);

	
	/*
		Check if the ProcessLock is acquired by the current Thread
	*/
		Result = KeQuerySpinLock(&Process->pcb.ProcessLock, &DebugThread);
		ASSERT1(Result == 1, Result);

		DebugThread1 = KeGetCurrentThread();
		ASSERT2(DebugThread1 == DebugThread, DebugThread, DebugThread1);


		ASSERT1(Process->pcb.State == PROCESS_STATE_ACTIVE, Process->pcb.State);

}
#endif


	Status = ObCreateObject(
					KernelMode,
					ThreadObjectType,
					NULL,
					sizeof(ETHREAD),
					&NewThread);

	if (!K_SUCCESS(Status)) {

		ASSERT(0);
		return Status;
	}

//No need to memset to 0 here. ObCreateObject will do it for us.
//	memset(NewThread, 0, sizeof(ETHREAD));

//Initialize Thread Control Block
	Tcb = &NewThread->tcb;
	Tcb->KernelStack = (ULONG_PTR)	MmAllocateSystemPtes(THREAD_STACK_SIZE);

//
// PsCloseThread
//


	if (Tcb->KernelStack == 0) {
		
		ObDereferenceObject(NewThread);
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	


//I need to use it to tell where to start execution
//	Tcb->PreviousMode =

	Tcb->Priority = Pcb->BasePriority;
	Tcb->ThreadProcess = (PKPROCESS) Process;
	Tcb->State = THREAD_STATE_INITIALIZED;
	Tcb->Quantum = Process->pcb.Quantum;
	KeInitializeSpinLock(&Tcb->ThreadLock);

	Tcb->ContextSwitches = 0;
	KeInitDispatcherHeader( &Tcb->Header);
	

/*
	Soon we do not need to PspAttachProcess as we will
	just map the addresses and not the physical page.

	Actual page allocation will be done in page fault handler which 
	will eitherway run in the current process context.
*/	


	if (IsClone) {

		CurrentThread = KeGetCurrentThread();
		Tcb->UserStack = CurrentThread->tcb.UserStack;

	} else {

		if (KeGetCurrentProcess() != Process) {

			/*
				Lock the dispatcher database before attaching to a new process
				address space. Just to be on the safer side
			
			*/
			KiLockDispatcherDataBase( &OldIrql);
			
			PspAttachProcess(Process, NULL);
			AttachedToProcess = TRUE;
		}

		Tcb->UserStack = (ULONG_PTR)
					MmAllocateUserPtes(Process->pcb.UserPtePool,
											THREAD_USER_STACK_SIZE+1);

		if (Tcb->UserStack == 0) {
			KeBugCheck("User Ptes exhausted!! \n");
		}
		

	// Go to the old process address space

		if (AttachedToProcess) {
			PspDetachProcess(NULL);
			KiUnLockDispatcherDataBase(OldIrql);
		}
		

	}



//Initialize ETHREAD block


//TODO: Put something here
	NewThread->Cid = 0;

//TODO:
//	NewThread->CreateTime = get current time

	NewThread->ThreadsProcess = Process;

	*Thread = NewThread;
	
	return STATUS_SUCCESS;
}


//Call this process with the Process Lock and return with the lock



KSTATUS
PspInsertThread(
				IN PEPROCESS Process,
				IN PETHREAD Thread
				)

/*

	PspInsertThread Should be called with the
		&Process->pcb.ProcessLock spinlock acquired.

*/



{



	ULONG Result;

	ASSERT1(KeGetCurrentIrql() == DISPATCH_LEVEL, KeGetCurrentIrql());
#ifdef DEBUG
{
	PKTHREAD DThread, DThread1;

/*
	Check if the ProcessLock is acquired by the current Thread
*/
	Result = KeQuerySpinLock(&Process->pcb.ProcessLock, &DThread);
	DThread1 = KeGetCurrentThread();

	ASSERT2(DThread == DThread1, DThread, DThread1);
	ASSERT1(Result == 1, Result);
}
#endif

	LIST_INSERT_TAIL(&Process->ThreadListHead, 
							Thread, 
							Process_link);


// Insert in the scheduler list
//	LIST_INSERT_TAIL();

	return STATUS_SUCCESS;
}





KSTATUS
KiCreateThreadContext(
						IN PKTHREAD Tcb,
						IN PKTHREAD CloneTcb,
						IN PKSTART_ROUTINE StartRoutine,
						IN PVOID StartContext);



KSTATUS
PspCreateThread(
						IN PEPROCESS Process,
						IN PCHAR ThreadName,
						IN PKSTART_ROUTINE StartRoutine,
						IN PVOID StartContext,
						IN PETHREAD *Thread
						)
{
	PETHREAD NewThread;
	PETHREAD CurrentThread;
	KSTATUS Status;
	KIRQL OldIrql;
	BOOL IsClone = FALSE;

	if (Process->pcb.State != PROCESS_STATE_ACTIVE) {
		Log(LOG_ERROR,"Trying to Create a thread in an inactive Process %x\n", Process);

	}

	if (StartRoutine == NULL) {

		IsClone = TRUE;
	}

	Log(LOG_INFO,"Creating Thread for Process %x\n", Process);
		
// Get the Process Lock before creating a thread
	KeAcquireSpinLock( &Process->pcb.ProcessLock, &OldIrql);

	Status = PspAllocateThread(Process,&NewThread, IsClone);

	if (!K_SUCCESS(Status)) {
		Log(LOG_ERROR,"PspAllocateThread Failed for Process %x\n", Process);
		KeReleaseSpinLock( &Process->pcb.ProcessLock, OldIrql);
		return Status;
	}

	ASSERT1((NewThread->Flags  & THREAD_FLAG_EXEC_STATE_MASK) == 0, NewThread->Flags);

	NewThread->Flags |= THREAD_FLAG_ALLOCATED;

	if (ThreadName) {
		
		strncpy(NewThread->Name, ThreadName, MAX_NAME_LENGTH);
	} else {

		strncpy(NewThread->Name, "ThreadName", MAX_NAME_LENGTH);
	}

//
// Failures after this point need the dereference of the object, which will also call the close thread
// and undo any initiallization done over it.
//

	Status  = PspInsertThread(Process, NewThread);

	if (!K_SUCCESS (Status)) {

		Log(LOG_ERROR,"PspInsertThread Failed for Process %x\n", Process);
		goto ErrorExit;
		
	}

	ASSERT1((NewThread->Flags &THREAD_FLAG_EXEC_STATE_MASK) == THREAD_FLAG_ALLOCATED, NewThread->Flags);

	NewThread->Flags |= THREAD_FLAG_INSERTED;


	if (IsClone) {
		CurrentThread = KeGetCurrentThread();
		Status = KiCreateThreadContext( &NewThread->tcb, &CurrentThread->tcb,
							NULL, NULL);
		
	} else {
		Status = KiCreateThreadContext( &NewThread->tcb, NULL,
								StartRoutine, StartContext);
	}



	if (!K_SUCCESS(Status)) {
		Log(LOG_ERROR,"KiCreateThreadContext Failed for Process %x\n", Process);
		goto ErrorExit;
	}

	ASSERT1(NewThread->Flags == THREAD_FLAG_ALLOCATED | THREAD_FLAG_INSERTED, NewThread->Flags);

	NewThread->Flags |= THREAD_FLAG_CONTEXT_CREATED;


/*
	Lock the Dispatcher database before Putting the thread in the ready queue.
*/

	KiLockDispatcherDataBaseAtDpcLevel();

	KiReadyThread(&NewThread->tcb);
	KiUnLockDispatcherDataBaseFromDpcLevel();

	InterlockedIncrement( &Process->ActiveThreads);

	// Increase the rundown Protection
	PsIncProcessRundownProtect(Process);

	
	KeReleaseSpinLock( &Process->pcb.ProcessLock, OldIrql);

	if (Thread) {
		*Thread = NewThread;
	}
	
	return STATUS_SUCCESS;

ErrorExit:
	KeReleaseSpinLock( &Process->pcb.ProcessLock, OldIrql);
	ObDereferenceObject(NewThread);
	
	return Status;
}



VOID
PsThreadInit()
{
	KSTATUS Status;
	OBJECT_TYPE TObjectType;


	memset (&TObjectType, 0, sizeof (OBJECT_TYPE));

	TObjectType.CloseProcedure = PsCloseThread;
	TObjectType.ObjectSize = sizeof(ETHREAD);
	TObjectType.PoolTag = 'RHTE';
	TObjectType.PoolType = NonPagedPool;
	TObjectType.TypeIndex = OBJECT_TYPE_THREAD;

	Status = ObCreateObjectType(
					&TObjectType,
						&ThreadObjectType);


	ASSERT(K_SUCCESS(Status));

}



