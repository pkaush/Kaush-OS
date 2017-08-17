/*
*  C Interface:	Process.h
*
* Description: Interface for user environment
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/


#ifndef KOS_KERN_PROC_H
#define KOS_KERN_PROC_H

#include <inc/types.h>
#include <inc/list.h>
#include <kern/mm/mm.h>

#include <kern/ke/ke.h>
#include <kern/hal/x86.h>
#include <kern/ex/ex.h>
#include <kern/ob/ob.h>





typedef ULONG	PID;


typedef struct _EPROCESS {

// Process Control Block
	struct _KPROCESS pcb;

	LARGE_INTEGER CreateTime;
	LARGE_INTEGER ExitTime;

	ULONG Flags;

	HANDLE_TABLE HandleTable;
/*
	This will be a refcount to protect removal of a process 
	while we are on it
*/
	LONG RundownProtect;

	HANDLE UniqueProcessId;
	HANDLE ParentProcessId;
// Pointer to parent Structure
	struct _EPROCESS	*Parent;


	KLIST_ENTRY(_EPROCESS) ProcessLink;

//	PHANDLE_TABLE ObjectTable;

// Name of the binary
	CHAR ImageFileName[ MAX_NAME_LENGTH];

	LIST_HEAD(_ThreadListHead, _ETHREAD) ThreadListHead;
	ULONG ActiveThreads;
	KSTATUS ExitStatus;

} EPROCESS, *PEPROCESS;




typedef struct _ETHREAD {

	KTHREAD tcb;

	LARGE_INTEGER CreateTime;
       LARGE_INTEGER ExitTime;
    
	KSTATUS ExitStatus;
	ULONG Flags;

//Process to which this thread belongs
	PEPROCESS ThreadsProcess;
	KLIST_ENTRY(_ETHREAD) Process_link;


	KSPIN_LOCK ThreadLock;
	ULONG RundownProtect;
	ULONG CrossThreadFlags;



// Thread Id
	PID Cid;
// Some name will be useful for debugging
	CHAR	Name[MAX_NAME_LENGTH];


}ETHREAD, *PETHREAD;


//Number of pages for thread stack (Both kernel and user)
#define THREAD_STACK_SIZE			3
#define THREAD_USER_STACK_SIZE		3




KSTATUS
PspCreateThread(
	IN PEPROCESS Process,
	IN PCHAR ThreadName,
	IN PKSTART_ROUTINE StartRoutine,
	IN PVOID StartContext,
	IN PETHREAD * Thread
	);


VOID
PsThreadInit();


PEPROCESS
PsGetCurrentProcess();



VOID
PspAttachProcess(
						IN PEPROCESS Process,
						OUT PRKAPC_STATE ApcState
						);


VOID
PspDetachProcess(
						IN PRKAPC_STATE ApcState
						);



// System Process
VOID
PsSystemProcessStart(
				IN PVOID StartContext
				);


#endif //End of Header

