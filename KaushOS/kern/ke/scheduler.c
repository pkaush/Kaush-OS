/*
*  C Implementation: Scheduler.c
*
* Description:  Scheduler
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/

#include "ke.h"

#include <kern/mm/mm.h>
#include <kern/ps/process.h>


/*
	I really really want to thank

	David B. Probert, Ph.D.
	Windows Kernel Development
	Microsoft Corporation

	His slides, documents, has helped me understand and design a lot of things


*/




typedef struct _SCHEDULER_TRAPFRAME {
	GENERALREGISTERS TrapFrame_regs;
	ULONG_PTR Eflags;
	ULONG_PTR	Eip;

/*
	These Fields will only be used when starting a new Thread
	these will be passed as arguments to the KiStartThread

*/
	ULONG_PTR	ReturnAddress;
	ULONG_PTR	StartRoutine;
	ULONG_PTR	StartContext;

} SCHEDULER_TRAPFRAME, *PSCHEDULER_TRAPFRAME;


PKTHREAD_LIST
KiGetCurrentSchedulerList(PROCESS_PRIORITY Priority);


KSPIN_LOCK ExitThreadLock;
KTHREAD_LIST ExitThreadList;



PKTHREAD
KiSelectNextThread( 
				IN PROCESS_PRIORITY CurrentPriority
				)
{
	PKTHREAD Thread = NULL;
	PKTHREAD_LIST SchedulerListHead;
	ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
	PROCESS_PRIORITY Priority;

	ASSERT1(CurrentPriority <= PROCESS_PRIORITY_MAX, CurrentPriority);

	ASSERT1(CurrentPriority >= PROCESS_PRIORITY_IDLE, CurrentPriority);

	Priority = PROCESS_PRIORITY_MAX;
	do {

		if (Priority < CurrentPriority) {

			return NULL;
		}


		SchedulerListHead = KiGetCurrentSchedulerList(Priority);
		Priority --;
		ASSERT(Priority >= 0);
	} while(LIST_EMPTY(SchedulerListHead));

	ASSERT(SchedulerListHead);
	ASSERT(!LIST_EMPTY(SchedulerListHead));


	LIST_REMOVE_HEAD( SchedulerListHead,
			Thread, Scheduler_link);
	ASSERT(Thread != NULL);

	return(Thread);

}


VOID
KiReadyThread(
					IN PKTHREAD Tcb
					);



VOID
KiSwapThread( 
			IN PKTHREAD CurrentThread,
			IN PKTHREAD NextThread
			)
{


#ifdef DEBUG
{
	
	KIRQL debugIrql;
	PKTHREAD Thread, CurrentThread1;
	ULONG Result;
	debugIrql = KeGetCurrentIrql();
	
// Check if we are running on DISPATCH_LEVEL
	ASSERT1(debugIrql == DISPATCH_LEVEL, debugIrql);
	
// Check if Dispatcherdatebase is Locked
	Result = KiQueryDispatcherDatabase( &Thread);
	ASSERT1(Result == 1, Result);
	
// Check if it is Locked by us	
	CurrentThread1 = KeGetCurrentThread();
	ASSERT2(Thread == CurrentThread1, Thread, CurrentThread1);

}
#endif
	
	
	ASSERT(KeGetCurrentThread()== CurrentThread);
	ASSERT(CurrentThread != NULL);

	ASSERT1( (CurrentThread->State == THREAD_STATE_READY) 
		|| (CurrentThread->State == THREAD_STATE_TERMINATED)
		|| (CurrentThread->State == THREAD_STATE_WAITING), CurrentThread->State);

	ASSERT1(	(NextThread->State == THREAD_STATE_READY), NextThread->State);

	NextThread->State = THREAD_STATE_RUNNING;
// Set the next thread as the current thread
	KiSetCurrentThread(NextThread);
// attach to new process address space
	KiAttachProcess( NextThread->ThreadProcess);


// Update TSS
	KiUpdateTSS(NextThread->KernelStack + (THREAD_STACK_SIZE * PGSIZE));


/*
	This check is for starting the first thread for execution
*/


//	if(CurrentThread) {
		KiContextSwitch(NextThread->StackPointer,
			&CurrentThread->StackPointer);
//	} else {
//		KiContextSwitch(NextThread->StackPointer,
//			&StackPointer);
//	}


}


VOID
KiRunUserThread(IN PKTRAPFRAME Trapframe);


VOID
KiStartThread(IN PKSTART_ROUTINE StartRoutine,IN PVOID StartContext);


#if 1 // Defined in ctxswitch.S
VOID
KiRunUserThread(IN PKTRAPFRAME Trapframe)

{
	PKTHREAD Thread = KeGetCurrentThread();
	PKPROCESS Process = KeGetCurrentProcess();



	ASSERT(Process == Thread->ThreadProcess);
	ASSERT(IsCurrentPageDirectory(Process->DirectoryTableBase));


	HalDisableInterrupts();

	KiUpdateTSS(Thread->KernelStack + (THREAD_STACK_SIZE * PGSIZE));
	

#if 0
// This code has been added for some debugging, and should be removed later
	{

		
		typedef struct _MMPTE_HARDWARE {
			ULONG Valid:1;
			ULONG Writable:1;
			ULONG UserPriv:1;
			ULONG WriteThrough:1;
			ULONG CacheDisabled:1;
			ULONG Accessed:1;
			ULONG Dirty:1;			//Reseved for PDE
			ULONG SuperPage:1;	// Pagetable attribute Index
			ULONG Global:1;
			ULONG Reserved:2;
			ULONG CopyOnWrite:1;			//Copy on Write
			ULONG PageFrameNumber:20;
		} MMPTE_HARDWARE, *PMMPTE_HARDWARE;
		
		typedef struct _MMPTE {
			union {
				ULONG Long;
				MMPTE_HARDWARE Hard;
			} u;
		} MMPTE, *PMMPTE;
		
		
#define MmGetPdeAddress(x) (PMMPTE)(((((ULONG_PTR)(x)) >> 22) << 2) + MM_PDE_ADDRESS)
#define MmGetPteAddress(x) (PMMPTE)(((((ULONG_PTR)(x)) >> 12) << 2) + MMPAGETABLEAREA_START)
#define MiGetVitualAddressFromPTE(x) ((PVOID)((ULONG)(x) << 10))
		
		typedef struct _MMPFN {
			PMMPTE PteAddress;
			KLIST_ENTRY(_MMPFN) link;
			ULONG RefCount;
		}MMPFN, *PMMPFN;
		

		PULONG DirTable;
		PULONG PageTable;
		PMMPTE Pde;
		PMMPTE Pte;
		PVOID Va, Va1 ;


		kprintf("=====KiRunUserThread Thread 0x%x, UserStack 0x%x,  Pgdir 0x%x val 0x%x\n\n", 
				Thread, Thread->UserStack, Process->DirectoryTableBase, *(ULONG *)(Thread->UserStack));

		memset(Thread->UserStack, 0xdeedbeef, THREAD_USER_STACK_SIZE * PGSIZE);

		DirTable = (PULONG) Process->DirectoryTableBase;
		Va= (PVOID)Thread->UserStack;

		Pde = MmGetPdeAddress(Thread->UserStack);


		kprintf("Pde->u.Long 0x%x dir0x%x\n", Pde->u.Long, DirTable[PDX(Va)]);


		Pte = MmGetPteAddress(Va);

		PageTable = MmGetVirtualForPhysical(DirTable[PDX(Va)] & ~0xfff);

		Va1 = MmGetVirtualForPhysical(PageTable[PTX(Va)] & ~0xfff); 	

		kprintf("Pte->u.Long 0x%x page0x%x Va1 0x%x\n", Pte->u.Long, PageTable[PTX(Va)], Va1);

		

		
	PrintKTrapframe(Trapframe);

	}

#endif

		__asm __volatile("movl %0,%%esp\n"
		"\tpopal\n"
		"\tpopl %%es\n"
		"\tpopl %%ds\n"
		"\taddl $0x8,%%esp\n" /* skip tf_trapno and tf_errcode */
		"\tiret"
		: : "g" (Trapframe) : "memory");
	KeBugCheck("iret failed");

}

#endif

//TODO: Will move it to trap section ... later



KSTATUS
KiCreateThreadContext(
					IN PKTHREAD Tcb,
					IN PKTHREAD CloneTcb,
					IN PKSTART_ROUTINE StartRoutine,
					IN PVOID StartContext
					)
{
	PKTRAPFRAME pTrapFrame = NULL;
	PKTRAPFRAME pCloneTrapFrame = NULL;
	PSCHEDULER_TRAPFRAME pSchedulerTrapframe;
	ULONG_PTR StackPtr;

	if ((CloneTcb) && (StartRoutine)) {
		ASSERT2(0, CloneTcb, StartRoutine);
	}
	
	// X86 Stack grows towards lower memory
	StackPtr = Tcb->KernelStack + (THREAD_STACK_SIZE * PGSIZE);
/*
	In case of User Thread Create the trapframe on the stack
	to transtion to user mode

*/
	if (IsUserAddress((PVOID)StartRoutine)) {
		//Allocate the trapframe on the stack	
			StackPtr -= sizeof(KTRAPFRAME);
			memset( (PVOID)StackPtr, 0, sizeof(KTRAPFRAME));
			pTrapFrame = (PKTRAPFRAME) StackPtr;
	}


//Create Scheduler Trapframe, it will be used by the Context Switch
	StackPtr -= sizeof(SCHEDULER_TRAPFRAME);
	memset( (PVOID)StackPtr, 0, sizeof(SCHEDULER_TRAPFRAME));
	pSchedulerTrapframe = (PVOID)StackPtr;




	if (IsKernelAddress(StartRoutine)) {


		// We dont clone the kernel thread trapframe
		ASSERT(CloneTcb == NULL);
		ASSERT(StartRoutine);

		pSchedulerTrapframe ->Eip = (ULONG_PTR) &KiStartThread;
		pSchedulerTrapframe->Eflags = FL_MBS | FL_IF;
		pSchedulerTrapframe ->StartRoutine = (ULONG_PTR)StartRoutine;
		pSchedulerTrapframe ->StartContext = (ULONG_PTR)StartContext;

		Tcb->StackPointer = StackPtr;
		Tcb->PreviousMode = KernelMode;

	} else {


		if (CloneTcb) {


			ASSERT1(StartRoutine == NULL, StartRoutine);
			pCloneTrapFrame = 	((CloneTcb->KernelStack + (THREAD_STACK_SIZE * PGSIZE)) - (sizeof(KTRAPFRAME)));
			PrintKTrapframe(pCloneTrapFrame);

			ASSERT1(pCloneTrapFrame->TrapFrame_cs == GD_UT | 3, pCloneTrapFrame->TrapFrame_cs);
			ASSERT1(pCloneTrapFrame->TrapFrame_ss == GD_UD | 3, pCloneTrapFrame->TrapFrame_ss);
			memcpy(pTrapFrame, pCloneTrapFrame, sizeof(KTRAPFRAME));

			// Status for clone child will be 0
			pTrapFrame->TrapFrame_regs.Reg_eax = 0;
		} else {



			ASSERT(StartRoutine != NULL);
			pTrapFrame->TrapFrame_cs = GD_UT | 3;
			pTrapFrame->TrapFrame_ds = GD_UD | 3;
			pTrapFrame->TrapFrame_es = GD_UD | 3;
			pTrapFrame->TrapFrame_ss = GD_UD| 3;

			pTrapFrame->TrapFrame_eflags = FL_MBS | FL_IF;
			pTrapFrame->TrapFrame_eip =  (ULONG_PTR) StartRoutine;

			pTrapFrame->TrapFrame_esp = Tcb->UserStack + (THREAD_USER_STACK_SIZE * PGSIZE) ;
		}
		
		pSchedulerTrapframe->Eip = (ULONG_PTR) &KiStartThread;
		pSchedulerTrapframe->Eflags = FL_MBS;
		pSchedulerTrapframe->StartRoutine = (ULONG_PTR) &KiRunUserThread;
		pSchedulerTrapframe->StartContext = (ULONG_PTR)pTrapFrame;

		Tcb->StackPointer = StackPtr;
		Tcb->PreviousMode = UserMode;


	}


	return STATUS_SUCCESS;
}



VOID
Schedule(
	IN KDPC *Dpc,
	IN PVOID DeferredContext,
	IN PVOID SystemArgument1,
	IN PVOID SystemArgument2
	)




/*

	This function will be called at DISPATCH_LEVEL
	mostly from a DPC context, which timer interrupt
	will queue on the completion of current thread quantum

*/

{
	PKTHREAD NextThread = NULL, CurrentThread = NULL;

	KIRQL OldIrql;

#ifdef DEBUG
{	// Check the Current IRQL is DISPATCH_LEVEL
//	KIRQL DCurrentIrql;
//	DCurrentIrql = KeGetCurrentIrql();
//	ASSERT1(DCurrentIrql == DISPATCH_LEVEL,DCurrentIrql);
}
#endif

/* 
	Lock the Dispatcherdatabase. 
	It should be unlocked when the new thread starts its execution
	
*/
	KiLockDispatcherDataBase( &OldIrql);

	


/*
	Put the current thread in the ready queue before selecting the 
	next thread so that if nothing else is ready this thread will get
	one more chance.

	Will definitly be protected by Dipatcher Lock against this thread getting 
	queued on some other processor(atleast until we have a common dispatcher
	lock for all processors).

*/

	CurrentThread = KeGetCurrentThread();

	ASSERT(CurrentThread);
	CurrentThread->WaitIrql = OldIrql;

	if (CurrentThread->State == THREAD_STATE_TERMINATED) {

	// If the current Thread is Terminating then we are ok with selecting anything 
	// greater than equal to idle priority.
		NextThread = KiSelectNextThread(PROCESS_PRIORITY_IDLE);
	
		ASSERT(NextThread != NULL);
		ASSERT(CurrentThread != NextThread);

		KeAcquireSpinLockAtDpcLevel( &ExitThreadLock);
		LIST_INSERT_TAIL( &ExitThreadList, CurrentThread, Scheduler_link);
		KeReleaseSpinLockFromDpcLevel( &ExitThreadLock);

		KiSwapThread(CurrentThread, NextThread);
		KeBugCheck("The Thread is Terminated. Should not execute here\n");

	} else {

		NextThread = KiSelectNextThread(CurrentThread->Priority );

		ASSERT2(CurrentThread != NextThread, CurrentThread, NextThread);
		
		if (NextThread) {
			// Put the current thread in the ready queue
			KiReadyThread(CurrentThread);
			// Start the next thread after saving the current context
			KiSwapThread(CurrentThread, NextThread);

		}
	}

/*

	Now as we are running on the next thread's stack so "CurrentThread"
	will be used to point to it.

	Lower the IRQL to what it was before it went into waiting state
	It should always be either PASSIVE_LEVEL or APC_LEVEL
*/


	ASSERT1(CurrentThread->WaitIrql <= DISPATCH_LEVEL, CurrentThread->WaitIrql);
	KiUnLockDispatcherDataBase(CurrentThread->WaitIrql);



	do { // Clean all the terminated threads
			KIRQL OldIrql;
			KeAcquireSpinLock( &ExitThreadLock, &OldIrql);

			while(!LIST_EMPTY( &ExitThreadList)) {
				PKTHREAD ExitedThread;
				
				LIST_REMOVE_HEAD( &ExitThreadList, ExitedThread, Scheduler_link);
				PsExitThread( (PVOID) ExitedThread);
			}

			KeReleaseSpinLock( &ExitThreadLock, OldIrql);

	} while (0);

}



/*

	We will keep only one DPC for scheduling on one processor.
	If it is already there in that queue then we will get False.

*/
KDPC	KiSchedulerDPC[MAX_ALLOWED_NUM_PROCESSORS];




VOID INLINE
KiThreadTimerISR()
/*
	This code will in the context of the timer ISR
	so keep it minimal. Keep this function as Inline
	as it will be only called at on place and thts ISR
*/
{
	ULONG ProcessorNumber;
	BOOLEAN Queued = FALSE;
	PKTHREAD CurrentThread;

#ifdef DEBUG
{
	KIRQL DCurrentIrql;
	DCurrentIrql = KeGetCurrentIrql();
	ASSERT1(DCurrentIrql == CLOCK1_LEVEL, DCurrentIrql);
	
}
#endif



	ProcessorNumber = KeGetCurrentProcessorNumber();
	CurrentThread = KeGetCurrentThread();

	if ( --CurrentThread->Quantum <= 0) {
		Queued = KeInsertQueueDpc(
						&KiSchedulerDPC[ProcessorNumber],
						NULL,NULL);

	}



}




VOID
KiStandbyThread(
					IN PKTHREAD Tcb
					)

/*
	This function will remove a thread from scheduler list


	This function should be called with DispatcherDatabase Locked.
	And will return with the Lock held.
*/

{
	PKTHREAD_LIST SchedulerListHead;

#ifdef DEBUG
{

	KIRQL debugIrql;
	PKTHREAD Thread, CurrentThread;
	ULONG Result;
	debugIrql = KeGetCurrentIrql();

// Check if we are running on DISPATCH_LEVEL
	ASSERT1(debugIrql == DISPATCH_LEVEL, debugIrql);

// Check if Dispatcherdatebase is Locked
	Result = KiQueryDispatcherDatabase( &Thread);
	ASSERT1(Result == 1, Result);

// Check if it is Locked by us	
	CurrentThread = KeGetCurrentThread();
	ASSERT2(Thread == CurrentThread, Thread, CurrentThread);
	
}
#endif

	if ( (Tcb->State != THREAD_STATE_READY)
			||
		(Tcb->State != THREAD_STATE_TERMINATED) ) {

		KeBugCheck(" Incorrect Thread State\n");
	}
	
	// Set the Thread state
	if (Tcb->State != THREAD_STATE_TERMINATED) {
		Tcb->State = THREAD_STATE_STANDBY;
	}
	
	SchedulerListHead = KiGetCurrentSchedulerList(Tcb->Priority);

/*
	This list is protected by Dispatcher database.
*/
	LIST_REMOVE(SchedulerListHead,Tcb,Scheduler_link);

}



VOID
KiReadyThread(
					IN PKTHREAD Tcb
					)

/*

	This function should be called with DispatcherDatabase Locked.
	And will return with the Lock held.

*/

{
	PKTHREAD_LIST SchedulerListHead;

#ifdef DEBUG
{

	KIRQL debugIrql;
	PKTHREAD DThread, DCurrentThread;
	ULONG Result;
	debugIrql = KeGetCurrentIrql();

// Check if we are running on DISPATCH_LEVEL
	ASSERT1(debugIrql == DISPATCH_LEVEL, debugIrql);

// Check if Dispatcherdatebase is Locked
	Result = KiQueryDispatcherDatabase( &DThread);
	ASSERT1(Result == 1, Result);

// Check if it is Locked by us	
	DCurrentThread = KeGetCurrentThread();
	ASSERT2(DThread == DCurrentThread, DThread, DCurrentThread);
	
}
#endif

	if ((Tcb->State != THREAD_STATE_INITIALIZED) 
			&&
		(Tcb->State != THREAD_STATE_WAITING)
			&&
		(Tcb->State !=THREAD_STATE_RUNNING)) {

		KeBugCheck(" Incorrect Thread State %d\n", Tcb->State);
	}
// Set the Thread state
	Tcb->State = THREAD_STATE_READY;

//Assign the time slice

	Tcb->Quantum = Tcb->ThreadProcess->Quantum;
/*
	Get the Scheduler list for the current processor
	Will soon start using AFFINITY and priority for this.
*/
	SchedulerListHead = KiGetCurrentSchedulerList(Tcb->Priority);

/*
	This list is protected by Dispatcher database.
*/
	LIST_INSERT_TAIL( SchedulerListHead,Tcb, Scheduler_link);

}


VOID
KiStartThread(
					IN PKSTART_ROUTINE StartRoutine,
					IN PVOID StartContext
						)
{

	KIRQL OldIrql;


#ifdef DEBUG
	{
	
		KIRQL debugIrql;
		PKTHREAD Thread, CurrentThread;
		ULONG Result;
		debugIrql = KeGetCurrentIrql();
	
	// Check if we are running on DISPATCH_LEVEL
		ASSERT1(debugIrql == DISPATCH_LEVEL, debugIrql);
	
	// Check if Dispatcherdatebase is Locked
		Result = KiQueryDispatcherDatabase(&Thread);
		ASSERT1(Result == 1, Result);



	}
#endif


//Unlock the DispathcherDatabase and Start the Thread at PASSIVE_LEVEL
	KiUnLockDispatcherDataBase(PASSIVE_LEVEL);


	StartRoutine(StartContext);

	PsExitThread(KeGetCurrentThread());
#if 0
#ifdef DEBUG
{
/*
	A Thread Should always finish its execution at PASSIVE_LEVEL

*/

	KIRQL CurrentIrql = KeGetCurrentIrql();
	ASSERT1(CurrentIrql == PASSIVE_LEVEL, CurrentIrql);
}
#endif	

	KiLockDispatcherDataBase( &OldIrql);

/*
	ASSERT(0);
	Delete the thread here.


*/
	Log(LOG_DEBUG,"Thread finished  %x\n", KeGetCurrentThread());
	Schedule(NULL, NULL, NULL, NULL);

#endif
}



VOID
KiBlockThread()
{


	PKTHREAD NextThread, CurrentThread;


#ifdef DEBUG
{

	KIRQL debugIrql;
	PKTHREAD DThread, DCurrentThread;
	ULONG Result;
	debugIrql = KeGetCurrentIrql();

// Check if we are running on DISPATCH_LEVEL
	ASSERT1(debugIrql == DISPATCH_LEVEL, debugIrql);

// Check if Dispatcherdatebase is Locked
	Result = KiQueryDispatcherDatabase(&DThread);
	ASSERT1(Result == 1, Result);

// Check if it is Locked by us	
	DCurrentThread = KeGetCurrentThread();
	ASSERT2((DThread == DCurrentThread) , DThread, DCurrentThread);
	
}
#endif


	CurrentThread = KeGetCurrentThread();

	ASSERT1(CurrentThread->State == THREAD_STATE_RUNNING,	CurrentThread->State);

	CurrentThread->State = THREAD_STATE_WAITING;


//Select Next Thread for execution
	NextThread = KiSelectNextThread(PROCESS_PRIORITY_IDLE);


	ASSERT(NextThread != NULL);

//	if (NextThread == NULL) {
//		NextThread = KiGetIdleThread();
//	}


// Start the next thread after saving the current context
	KiSwapThread(CurrentThread, NextThread);


}








KSTATUS
SyspCreateThreadContext(
					IN PKTHREAD Tcb,
					IN PKSTART_ROUTINE StartRoutine,
					IN PVOID StartContext
					)
{
//	PKTRAPFRAME pTrapFrame;
	PSCHEDULER_TRAPFRAME pSchedulerTrapframe;
	ULONG_PTR StackPtr;


	StackPtr = Tcb->KernelStack + (THREAD_STACK_SIZE * PGSIZE);


//Create Scheduler Trapframe, it will be used by the Context Switch
	StackPtr -= sizeof(SCHEDULER_TRAPFRAME);
	memset((PVOID)StackPtr, 0, sizeof(SCHEDULER_TRAPFRAME));
	pSchedulerTrapframe = (PVOID)StackPtr;


	ASSERT1(IsKernelAddress(StartRoutine), StartRoutine);

	pSchedulerTrapframe ->Eip = (ULONG_PTR) &KiStartThread;
	pSchedulerTrapframe ->StartRoutine = (ULONG_PTR)StartRoutine;
	pSchedulerTrapframe ->StartContext = (ULONG_PTR)StartContext;

	Tcb->StackPointer = StackPtr;
	Tcb->PreviousMode = KernelMode;

	return STATUS_SUCCESS;
}








VOID
KiAttachProcess(
				IN PKPROCESS Process
				)
{
	ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

	ASSERT(Process);

	MmAttachToAddressSpace((PVOID)Process->DirectoryTableBase);
}





VOID
KeStackAttachProcess (
				IN PKPROCESS  Process,
				OUT PRKAPC_STATE  ApcState
				)
{


	ASSERT2(KeGetCurrentProcess() != Process, Process, KeGetCurrentProcess());
	ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
	KiAttachProcess(Process);
}





VOID
KeUnstackDetachProcess(
				IN PRKAPC_STATE  ApcState
				)
{
	PKTHREAD Thread;
	ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

	Thread = KeGetCurrentThread();
	ASSERT(Thread);
	
	KiAttachProcess(Thread->ThreadProcess);

}







VOID
KiSchedInitPhase()
{
	ULONG ProcessorNumber;


	for(ProcessorNumber = 0; 
			ProcessorNumber < MAX_ALLOWED_NUM_PROCESSORS;
			ProcessorNumber ++) {

			KeInitializeDpc(
				&KiSchedulerDPC[ProcessorNumber],
				Schedule,
				NULL
				);

			KeSetTargetProcessorDpc( &KiSchedulerDPC[ProcessorNumber],
				ProcessorNumber);


	}

	KeInitializeSpinLock( &ExitThreadLock);
	LIST_INIT	( &ExitThreadList);

	
}




