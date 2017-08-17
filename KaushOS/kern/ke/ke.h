/*
*  C Interface:	Interrupt.h
*
* Description: Interface to Interrupts, Exceptions, IRQLS
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/


#ifndef	__KOS_KE_KE_H__
#define	__KOS_KE_KE_H__

#include	<inc/types.h>
#include	<inc/list.h>
#include	<inc/error.h>
#include	<inc/string.h>
#include	<inc/bitmap.h>
#include	<inc/interlocked.h>

#include	"interrupt.h"






/*	SOME P R O T O T Y P E S		*/


typedef struct _KPROCESS  KPROCESS, *PKPROCESS;
typedef struct _KTHREAD KTHREAD, *PKTHREAD;



/******************************************
		I  R  Q  L
******************************************/

typedef ULONG	KIRQL;
typedef ULONG*	PKIRQL;


/* 
	Include hal header here because it needs the Definition 
	of IRQLS;
*/

#include	<kern/hal/X86.h>

/*
	EIther of Them, lets keep it here for now so IDE can give 
	me name when needed.

	Also it will help to test spinlock before going to multiproc.
	
*/
#define __KOS_UP__	1

#define __KOS_MP__	1



/*
  *	Locks
  */




typedef struct	_KSPIN_LOCK {
	ULONG		Lock;
	struct _KTHREAD	*Thread;
	KIRQL		OldIrql;
	
}KSPIN_LOCK, *PKSPIN_LOCK;




typedef enum _KPROCESSOR_MODE {
    KernelMode,
    UserMode,
    MaximumMode
}  KPROCESSOR_MODE;


VOID 
KeAcquireSpinLock(
		IN PKSPIN_LOCK  SpinLock,
		OUT PKIRQL  OldIrql
		);

VOID 
KeInitializeSpinLock(
		IN PKSPIN_LOCK  SpinLock
		);

VOID 
KeReleaseSpinLock(
		IN PKSPIN_LOCK  SpinLock,
		IN KIRQL  NewIrql
		);



ULONG
KeQuerySpinLock(
			IN PKSPIN_LOCK SpinLock,
			OUT PKTHREAD * Thread
			);







/*
  *	IRQL
  */



#define PASSIVE_LEVEL 0             // Passive release level
#define LOW_LEVEL 0                 // Lowest interrupt level
#define APC_LEVEL 1                 // APC interrupt level
#define DISPATCH_LEVEL 2            // Dispatcher level

#define PROFILE_LEVEL 27            // timer used for profiling.
#define CLOCK1_LEVEL 28             // Interval clock 1 level - Not used on x86
#define CLOCK2_LEVEL 28             // Interval clock 2 level
#define IPI_LEVEL 29                // Interprocessor interrupt level
#define POWER_LEVEL 30              // Power failure level
#define HIGH_LEVEL 31               // Highest interrupt level
#define MAX_IRQL_LEVEL	HIGH_LEVEL



KIRQL
KeGetCurrentIrql(
		VOID
		);

VOID 
KeLowerIrql(
		IN KIRQL  NewIrql
		);

VOID 
KeRaiseIrql(
		IN KIRQL  NewIrql,
		OUT PKIRQL  OldIrql
		);


VOID 
KeLowerIrql(
		IN KIRQL  NewIrql
		);



VOID
KiSetCurrentIrql(
				IN KIRQL NewIrql
				);



/* Perf counters */
extern ULONG TotalPhysicalPages;
extern ULONG PhysicalPagesInUse;
extern ULONG PhysicalPagesFree;




/****************************************************
					DPC & APC
****************************************************/






typedef struct _KAPC_STATE {
//		LIST_ENTRY(type) ApcListHead[MaximumMode];
//		struct _KPROCESS *Process;
//		BOOLEAN KernelApcInProgress;
//		BOOLEAN KernelApcPending;
		BOOLEAN UserApcPending;
} KAPC_STATE, *PKAPC_STATE, *PRKAPC_STATE;





typedef enum _KDPC_IMPORTANCE {
	LowImportance,
	MediumImportance,
	HighImportance,
	MediumHighImportance
} KDPC_IMPORTANCE;



typedef struct _KDPC *PKDPC;

typedef
VOID
(*PKDEFERRED_ROUTINE) (
	IN PKDPC Dpc,
	IN PVOID DeferredContext,
	IN PVOID SystemArgument1,
	IN PVOID SystemArgument2
	);



typedef struct _KDPC {
	UCHAR Type;
	UCHAR Importance;
	USHORT Number;
	KLIST_ENTRY(_KDPC) DpcListEntry;
	PKDEFERRED_ROUTINE DeferredRoutine;
	PVOID DeferredContext;
	PVOID SystemArgument1;
	PVOID SystemArgument2;
	__volatile PVOID DpcData;
} KDPC, *PRKDPC;



typedef LIST_HEAD(_DPC_LIST, _KDPC) KDPC_LIST, *PKDPC_LIST;


PKDPC_LIST
KiGetDpcList(IN ULONG ProcessorNumber);


/*************************************************
			 SCHEDULER & WAIT
*************************************************/


VOID
KiLockDispatcherDataBase(OUT PKIRQL OldIrql);


VOID
KiUnLockDispatcherDataBase(IN KIRQL OldIrql);


VOID
KiLockDispatcherDataBaseAtDpcLevel();


VOID
KiUnLockDispatcherDataBaseFromDpcLevel();

ULONG
KiQueryDispatcherDatabase(
					OUT PKTHREAD *Thread
					);



typedef enum _WAIT_TYPE {
	WAIT_TYPE_ANY,
	WAIT_TYPE_ALL

} WAIT_TYPE;




#define MAX_WAITS_ALLOWED 2

typedef struct _KWAIT_BLOCK {

	struct _KTHREAD *Thread;
	PVOID Object;
	WAIT_TYPE Type;
	KLIST_ENTRY(_KWAIT_BLOCK) WaitLink;
	KLIST_ENTRY(_KWAIT_BLOCK) ThreadLink;
} KWAIT_BLOCK, *PKWAIT_BLOCK;





typedef LIST_HEAD(_KWAIT_BLOCK_LIST,_KWAIT_BLOCK) 
			KWAIT_BLOCK_LIST, *PKWAIT_BLOCK_LIST;


/*
http://www.nirsoft.net/kernel_struct/vista/KOBJECTS.html

Oh nirsoft Praji tussi Great ho!!
*/


typedef enum _KOBJECTS
{
		 EventNotificationObject = 0,
		 EventSynchronizationObject = 1,
		 MutantObject = 2,
		 ProcessObject = 3,
		 QueueObject = 4,
		 SemaphoreObject = 5,
		 ThreadObject = 6,
		 GateObject = 7,
		 TimerNotificationObject = 8,
		 TimerSynchronizationObject = 9,
		 Spare2Object = 10,
		 Spare3Object = 11,
		 Spare4Object = 12,
		 Spare5Object = 13,
		 Spare6Object = 14,
		 Spare7Object = 15,
		 Spare8Object = 16,
		 Spare9Object = 17,
		 ApcObject = 18,
		 DpcObject = 19,
		 DeviceQueueObject = 20,
		 EventPairObject = 21,
		 InterruptObject = 22,
		 ProfileObject = 23,
		 ThreadedDpcObject = 24,
		 MaximumKernelObject = 25
} KOBJECTS;










typedef enum _KWAIT_REASON {
    Executive,
    FreePage,
    PageIn,
    PoolAllocation,
    DelayExecution,
    Suspended,
    UserRequest,
    WrExecutive,
    WrFreePage,
    WrPageIn,
    WrPoolAllocation,
    WrDelayExecution,
    WrSuspended,
    WrUserRequest,
    WrEventPair,
    WrQueue,
    WrLpcReceive,
    WrLpcReply,
    WrVirtualMemory,
    WrPageOut,
    WrRendezvous,
    Spare2,
    Spare3,
    Spare4,
    Spare5,
    WrCalloutStack,
    WrKernel,
    WrResource,
    WrPushLock,
    WrMutex,
    WrQuantumEnd,
    WrDispatchInt,
    WrPreempted,
    WrYieldExecution,
    WrFastMutex,
    WrGuardedMutex,
    WrRundown,
    MaximumWaitReason
} KWAIT_REASON;



typedef struct _DISPATCHER_HEADER {
		KWAIT_BLOCK_LIST WaitListHead;
		KOBJECTS Type;
/*
	Keep the Signal state as signed long so that we can have check 
	to see if the value goes to negative.
	
*/
		
		LONG SignalState;
			
} DISPATCHER_HEADER, *PDISPATCHER_HEADER;
			

// DDK

typedef struct _KMUTANT {
    DISPATCHER_HEADER Header;
//    KLIST_ENTRY(_KMUTANT) MutantListEntry;
    struct _KTHREAD *OwnerThread;
    BOOLEAN Abandoned;
    UCHAR ApcDisable;
} KMUTANT, *PKMUTANT, *PRKMUTANT, KMUTEX, *PKMUTEX, *PRKMUTEX;



typedef struct _KSEMAPHORE {
    DISPATCHER_HEADER Header;
    LONG Limit;
} KSEMAPHORE, *PKSEMAPHORE, *PRKSEMAPHORE;



/*
		E V E N T (Notification or Syncronisation)

*/

typedef enum _EVENT_TYPE {
		NotificationEvent,
		SynchronizationEvent
} EVENT_TYPE;




typedef struct _KEVENT {
    DISPATCHER_HEADER Header;
} KEVENT, *PKEVENT, *PRKEVENT;





VOID
KeInitializeEvent(
						IN OUT PRKEVENT Event,
						IN EVENT_TYPE Type,
						IN BOOLEAN State
						);

VOID
KeInitializeMutant(
						IN OUT PKMUTANT Mutant,
						IN BOOL InitialOwner
						);

VOID
KeInitializeSemaphore(
						IN OUT PRKSEMAPHORE Semaphore,
						IN LONG Count,
						IN LONG Limit
						);

KSTATUS
KeWaitForSingleObject(
					IN PVOID Object,
					IN KWAIT_REASON WaitReason,
					IN KPROCESSOR_MODE WaitMode,
					IN BOOLEAN Alertable,
					IN PLARGE_INTEGER Timeout OPTIONAL
					);


/*****************************************
			Process & Thread Section
*****************************************/


typedef enum _PROCESS_PRIORITY {
		PROCESS_PRIORITY_BELOWIDLE,	// Nothing Should run here
		PROCESS_PRIORITY_IDLE,
		PROCESS_PRIORITY_BELOW_NORMAL,
		PROCESS_PRIORITY_NORMAL,
		PROCESS_PRIORITY_ABOVE_NORMAL,
		PROCESS_PRIORITY_HIGH,
		PROCESS_PRIORITY_MAX,

} PROCESS_PRIORITY, *PPROCESS_PRIORITY, KPRIORITY;

typedef enum _PROCESS_STATE {
		PROCESS_STATE_IDLE,
		PROCESS_STATE_ACTIVE,
		PROCESS_STATE_CREATION,
		PROCESS_STATE_TERMINATED

}PROCESS_STATE, *PPROCESS_STATE;



typedef enum _THREAD_STATE {
		THREAD_STATE_INITIALIZED,
		THREAD_STATE_READY,
		THREAD_STATE_STANDBY,
		THREAD_STATE_RUNNING,
		THREAD_STATE_WAITING,
		THREAD_STATE_TERMINATED
} THREAD_STATE, *PTHREAD_STATE;


#define DEFAULT_PROCESS_QUANTUM	3

struct _KPROCESS  {
		// syncronisation with process Object
	DISPATCHER_HEADER Header;

	ULONG_PTR DirectoryTableBase;

// May be later	
//	KGDTENTRY LdtDescriptor
//	KIDTENTRY Int21Descriptor


// IO privilage Level: this will have IOPL bit mask
	UCHAR Iopl;



// Value of Quantum used by thread
	LONG Quantum;



//	volatile KAFFINITY ActiveProcessors;

// Time spend
	ULONG KernelTime;
	ULONG UserTime;

//List heads 	
//	LIST_HEAD( _ReadyListHead, struct _KPROCESS) ReadyListHead;
//	LIST_HEAD( _ThreadListHead, _KTHREAD) ThreadListHead;
	
	KSPIN_LOCK ProcessLock;

//Allowed Processors
	KAFFINITY Affinity;
// No need
//	USHORT StackCount;

// Initial prority of the process
	PROCESS_PRIORITY BasePriority;

	// Current Process state
	PROCESS_STATE State;
/*
	 This will be the Entry point for the first thread to start execution
	 mostly the main() of the user process
*/
	PVOID UserPtePool;
	PVOID ProcessEntryPoint;
};



struct _KTHREAD {

	DISPATCHER_HEADER Header;
	
// Virtual address of user and kernel stack
	ULONG_PTR UserStack, StackLimit;
	ULONG_PTR KernelStack;
	
	KSPIN_LOCK ThreadLock;


	PKPROCESS ThreadProcess;


// Number of context switches
	ULONG ContextSwitches;
	LONG Quantum;


//Current state of the thread
	THREAD_STATE State;

	KIRQL WaitIrql;


//	KAPC_STATE ApcState
//	KSPIN_LOCK ApcQueueLock

	LONG_PTR WaitStatus;
	KWAIT_BLOCK_LIST WaitBlockList;
	KWAIT_BLOCK WaitBlock[MAX_WAITS_ALLOWED];
	UCHAR WaitReason;
	PROCESS_PRIORITY Priority;
	UCHAR PreviousMode;

	KLIST_ENTRY(_KTHREAD) Scheduler_link;

// Will be used to start the execution first time or after context switch
	ULONG_PTR StackPointer;

};



typedef VOID
(*PKSTART_ROUTINE) (
    IN PVOID StartContext
    );






typedef LIST_HEAD(_KTHREAD_LIST, _KTHREAD) KTHREAD_LIST,
														*PKTHREAD_LIST;


typedef LIST_HEAD(_KPROCESS_LIST, _KPROCESS) KPROCESS_LIST,
														*PKPROCESS_LIST;







PKTHREAD
KeGetCurrentThread();


VOID
KiBlockThread();


VOID
KiReadyThread(
					IN PKTHREAD Tcb
					);


VOID
KiSetCurrentThread( 
						IN PKTHREAD Thread
						);



VOID
KiCreateCurrentGDT();



KSTATUS
KiCreateThreadContext(
					IN PKTHREAD Tcb,
					IN PKTHREAD CloneTcb,
					IN PKSTART_ROUTINE StartRoutine,
					IN PVOID StartContext
					);



ULONG
KiContextSwitch(
					IN ULONG_PTR NextStackPointer,
					IN ULONG_PTR *CurrentStackPointer
					);



VOID
KiAttachProcess(
				IN PKPROCESS NewProcess
				);

VOID
  KeStackAttachProcess (
			IN PKPROCESS  Process,
			OUT PRKAPC_STATE  ApcState
			);


VOID
  KeUnstackDetachProcess(
    IN PRKAPC_STATE  ApcState
    );




#define MAX_ALLOWED_NUM_PROCESSORS		1



ULONG
KeGetNumberOfProcessors();














#endif
