/*
*  C Implementation: wait.c
*
* Description: Implementation of wait queues and wait blocks
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/



#include "ke.h"





/*
	Some random number to keep check if Max threads allowed to take a 
	resource.

*/
#define MAX_DISPATCH_SIGNAL_STATE	100

static VOID INLINE
KiInitWaitBlock(
				IN OUT PKWAIT_BLOCK WaitBlock,
				IN PVOID Object
				)
{


//It should always be used for the current Thread
	ASSERT((ULONG_PTR)KeGetCurrentThread() < 
				(ULONG_PTR)WaitBlock);
// And
	ASSERT((ULONG_PTR)KeGetCurrentThread() + sizeof(KTHREAD) > 
					(ULONG_PTR)WaitBlock);



	WaitBlock->Thread = KeGetCurrentThread();
	WaitBlock->Object = Object;
	WaitBlock->Type = WAIT_TYPE_ANY;

	LIST_INIT_ENTRY( WaitBlock, ThreadLink);
	LIST_INIT_ENTRY( WaitBlock, WaitLink);	

}


VOID
KeInitDispatcherHeader (
						IN PDISPATCHER_HEADER Dispatcher,
						IN KOBJECTS Type,
						IN ULONG SignalState
						)
/*
	Initialize all the elements of the Dispatcher Header

*/


{
	Dispatcher->SignalState = SignalState;
	Dispatcher->Type = Type;
	LIST_INIT( &Dispatcher->WaitListHead);
}




static VOID INLINE
KiDHeaderAcquire(
				IN PDISPATCHER_HEADER Dispatcher
				)
{

	PKTHREAD Thread;
	PKWAIT_BLOCK WaitBlock;
	
	ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

	if (Dispatcher->SignalState == 0) {
		Thread = KeGetCurrentThread();
/*
	Always use Block 0 of Wait block here. KeWaitforMultipleObject 
	should not call into this function.
*/
		WaitBlock = &Thread->WaitBlock[0];
		KiInitWaitBlock( WaitBlock, Dispatcher);
		LIST_INSERT_TAIL( &Dispatcher->WaitListHead, WaitBlock, WaitLink);

// Block the current thread and start a new thread.		
		KiBlockThread();
	} else {

/*
	If this ASSERT fails that means after we have released the lock 
	some other thread has acquired it before we got scheduled for
	execution.

	The way to fix this would be to keep the upper check in a while
	instead of "	if (Dispatcher->SignalState == 0)"
	
*/
	
		ASSERT(Dispatcher->SignalState > 0);

/*
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
*/


// Only decrement SignalState for the Syncronization object not for Notification objects
// Like for process and thread once the signalstate is set, it should never be reset.
// And like EventNotificationObject once set, should be reset manually.


		if ( (EventSynchronizationObject)
					||
			(MutantObject)
					||
			(QueueObject)
					||
			(TimerSynchronizationObject)
					||
			(SemaphoreObject) ) {
			
				InterlockedDecrement( &Dispatcher->SignalState);
		}
	
	}

}



static BOOL INLINE
KiDHeaderTryAcquire(
						IN PDISPATCHER_HEADER Dispatcher
						)


{
	
	ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

	if (Dispatcher->SignalState > 0) {
		InterlockedDecrement( &Dispatcher->SignalState);
		return TRUE;
	}

	return FALSE;		
		
}



static VOID INLINE
KiDHeaderRelease(
				IN PDISPATCHER_HEADER Dispatcher
				)
{

	PKTHREAD Thread;
	PKWAIT_BLOCK WaitBlock;
	
	ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

	ASSERT(Dispatcher->SignalState < MAX_DISPATCH_SIGNAL_STATE);
	

// If the signal state is not 0 The list should be empty
	ASSERT1((Dispatcher->SignalState == 0) || 
			(LIST_EMPTY( &Dispatcher->WaitListHead)),
			(Dispatcher->SignalState));

/*
	If Waiting list is Not Empty Release one Thread from the queue
	and put it in the ready queue.

//TODO:	We should boost the thread priority also
	
*/
	if (!LIST_EMPTY( &Dispatcher->WaitListHead)) {

/*
	Because we will not increment the signalstate in this case
	so we can take it as this Thread has acquired the lock.

	We need to do this to follow the FIFO order for the locks.
*/


		LIST_REMOVE_HEAD( &Dispatcher->WaitListHead,  WaitBlock,WaitLink);

//verify that this block was waiting on this Object
		ASSERT2(WaitBlock->Object == Dispatcher, 
					WaitBlock->Object, Dispatcher);

//Get the thread
		Thread = WaitBlock->Thread;
		if (LIST_EMPTY( &Thread->WaitBlockList)) {
			KiReadyThread(Thread);
		} else {

/*
 	Because we will not increment the signalstate in this case
	so we can take it as this Thread has acquired the lock
*/
			LIST_REMOVE( &Thread->WaitBlockList, WaitBlock,ThreadLink);
			if (WaitBlock->Type == WAIT_TYPE_ANY) {
/*
	If Wait Type is Any remove the thread wait blocks from all the wait lists

	This will only come into picture when a thread is waiting for multiple Objects

*/
				while(!LIST_EMPTY( &Thread->WaitBlockList)) {
					PDISPATCHER_HEADER DObject;
					LIST_REMOVE( &Thread->WaitBlockList, WaitBlock,ThreadLink);
					DObject = WaitBlock->Object;
					
					//verify that this block was waiting on this Object
							ASSERT(WaitBlock->Object == DObject);
					LIST_REMOVE( &DObject->WaitListHead,  WaitBlock,WaitLink);
					
				}
				
			} else { //WAIT_TYPE_ALL

			}

		}
		
//		KiReadyThread(PKTHREAD Tcb)
	}else {
		InterlockedIncrement( &Dispatcher->SignalState);
	}
}








static VOID INLINE
KiDHeaderReleaseAll(
				IN PDISPATCHER_HEADER Dispatcher
				)
{

	PKTHREAD Thread;
	PKWAIT_BLOCK WaitBlock;
	
	ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

	ASSERT(Dispatcher->SignalState < MAX_DISPATCH_SIGNAL_STATE);
	

// If the signal state is not 0 The list should be empty
	ASSERT((Dispatcher->SignalState == 0) || 
			(LIST_EMPTY( &Dispatcher->WaitListHead)));

/*
	While Waiting list is Not Empty Release All the Threads from the queue
	and put them in the ready queue.

//TODO:	We should boost the thread priority also
	
*/
	while (!LIST_EMPTY( &Dispatcher->WaitListHead)) {

		LIST_REMOVE_HEAD( &Dispatcher->WaitListHead,  WaitBlock,WaitLink);

//verify that this block was waiting on this Object
		ASSERT(WaitBlock->Object == Dispatcher);

//Get the thread
		Thread = WaitBlock->Thread;
		if (LIST_EMPTY( &Thread->WaitBlockList)) {
			KiReadyThread(Thread);
		} else {

/*
 	Because we will not increment the signalstate in this case
	so we can take it as this Thread has acquired the lock.
*/
			LIST_REMOVE( &Thread->WaitBlockList, WaitBlock,ThreadLink);
			if (WaitBlock->Type == WAIT_TYPE_ANY) {
/*
	If Wait Type is Any remove the thread wait blocks from all the wait lists

	This will only come into picture when a thread is waiting for multiple Objects

*/
				while(!LIST_EMPTY( &Thread->WaitBlockList)) {
					PDISPATCHER_HEADER DObject;
					LIST_REMOVE( &Thread->WaitBlockList, WaitBlock,ThreadLink);
					DObject = WaitBlock->Object;
					
					//verify that this block was waiting on this Object
							ASSERT(WaitBlock->Object == DObject);
					LIST_REMOVE( &DObject->WaitListHead,  WaitBlock,WaitLink);
					
				}
				KiReadyThread(Thread);
				
			} else { //WAIT_TYPE_ALL

			}

		}
		
//		KiReadyThread(PKTHREAD Tcb)
	}


/*
	For the Notifications locks (like Process, Thread and notification event) always
	put the signal state to 1 and NOT increment it.
*/
	
	Dispatcher->SignalState = 1;
	
}



LONG
KeReleaseNotificationDHeader (
								IN OUT PDISPATCHER_HEADER DHeader,
								OUT PROCESS_PRIORITY Increment
								)
{

	KIRQL OldIrql;


	KiLockDispatcherDataBase( &OldIrql);
	KiDHeaderReleaseAll( DHeader);
	KiUnLockDispatcherDataBase(OldIrql);
	
	return 0;
}









/*
	M U T A N T (Super natural Big brother or Mutex ;-))

*/
VOID
KeInitializeMutant (
			IN OUT PKMUTANT Mutant,
			IN BOOLEAN InitialOwner
		)
{

	if (InitialOwner == FALSE) {

		KeInitDispatcherHeader(
				&Mutant->Header,
				MutantObject,
				1);
		
		Mutant->OwnerThread = NULL;
	} else {

		KeInitDispatcherHeader(
				&Mutant->Header,
				MutantObject,
				0);

		Mutant->OwnerThread = KeGetCurrentThread();

	}

}

LONG
KeReleaseMutant (
	IN OUT PRKMUTANT Mutant,
	OUT PROCESS_PRIORITY Increment,
	IN BOOLEAN Abandoned,
	IN BOOLEAN Wait
    )
{

	KIRQL OldIrql;

/*
	The Thread who has aquired the tread should be the one
	releasing it.
*/
	ASSERT(Mutant->OwnerThread == KeGetCurrentThread());

/*
	Set the owner thread to NULL here.
	If some thread is waiting for it, the OwnerThread value will get set in KeWaitSingleObject.
*/

	Mutant->OwnerThread = NULL;
	KiLockDispatcherDataBase( &OldIrql);
	KiDHeaderRelease( &Mutant->Header);


	KiUnLockDispatcherDataBase(OldIrql);
	
	return 0;
}




/*
		S E M A P H O R E (Counting Lock)

*/



VOID
KeInitializeSemaphore (
			IN PRKSEMAPHORE Semaphore,
			IN LONG Count,  
			IN LONG Limit
			)
/*++

KeInitializeSemaphore

	The KeInitializeSemaphore routine initializes a semaphore object with a given count 
	and specifies an upper limit that the count can attain.

VOID 
  KeInitializeSemaphore(
    IN PRKSEMAPHORE  Semaphore,
    IN LONG  Count,
    IN LONG  Limit
    );

Parameters

Semaphore
    Pointer to a dispatcher object of type semaphore, for which the caller provides the storage.
    
Count
    Specifies the initial count value to be assigned to the semaphore.
    This value must be positive. A nonzero value sets the initial state of the semaphore to signaled.
    
Limit
    Specifies the maximum count value that the semaphore can attain.
    This value must be positive. It determines how many waiting threads become eligible 
    for execution when the semaphore is set to the signaled state and can therefore 
    access the resource that the semaphore protects. 

Return Value

None



--*/

{

	ASSERT(Count <= Limit);
	KeInitDispatcherHeader(
			&Semaphore->Header,
			SemaphoreObject,
			Count);

	Semaphore->Limit = Limit;
		
}





LONG
KeReleaseSemaphore (
			IN OUT PRKSEMAPHORE Semaphore,
			IN PROCESS_PRIORITY  Increment,
			IN LONG Adjustment,
			IN BOOLEAN Wait
    			)

{
	KIRQL OldIrql;
	LONG OldState;
	ASSERT( Semaphore->Header.SignalState < Semaphore->Limit);
	ASSERT(Increment == 0);
	ASSERT(Adjustment == 1);
	
	OldState = Semaphore->Header.SignalState;
	
	KiLockDispatcherDataBase( &OldIrql);
/*
	For Adjestments I think we can call the KiDHeaderRelease
	that many number of times.
*/
		KiDHeaderRelease( &Semaphore->Header);

	KiUnLockDispatcherDataBase(OldIrql);
	
	return(OldState);
}









VOID
KeInitializeEvent (
						IN OUT PRKEVENT Event,
						IN EVENT_TYPE Type,
						IN BOOLEAN State
						)
{

	Event->Header.SignalState = State;
	
	if (Type == NotificationEvent) {
		Event->Header.Type = EventNotificationObject;
	} else {
		Event->Header.Type = EventSynchronizationObject;
	}

	LIST_INIT( &Event->Header.WaitListHead);


}




VOID
KeClearEvent (
			IN OUT PRKEVENT Event
			)
{
		ASSERT(	(Event->Header.Type == EventNotificationObject) ||
				(Event->Header.Type == EventSynchronizationObject));

		
		Event->Header.SignalState = 0;

}



LONG
KeResetEvent (
				IN OUT PRKEVENT Event
				)
{
	LONG	State;
	ASSERT( (Event->Header.Type == EventNotificationObject) ||
			(Event->Header.Type == EventSynchronizationObject));

	State = InterlockedExchange( &Event->Header.SignalState, 0);
	return State;
}




LONG
KeSetEvent (
		IN OUT PRKEVENT Event,
		IN KPRIORITY Increment,
		IN BOOLEAN Wait
		)
{

	LONG	State;
	KIRQL OldIrql;


	
	ASSERT( (Event->Header.Type == EventNotificationObject) ||
			(Event->Header.Type == EventSynchronizationObject));

	KiLockDispatcherDataBase( &OldIrql);

	if (Event->Header.Type == EventNotificationObject) {
		KiDHeaderReleaseAll( &Event->Header);
	} else {
		KiDHeaderRelease( &Event->Header);
	}


	State = InterlockedExchange( &Event->Header.SignalState, 0);

	KiUnLockDispatcherDataBase(OldIrql);
	return State;
	

}




KSTATUS 
KeWaitForSingleObject(
								IN PVOID  Object,
								IN KWAIT_REASON  WaitReason,
								IN KPROCESSOR_MODE  WaitMode,
								IN BOOLEAN  Alertable,
								IN PLARGE_INTEGER  Timeout  OPTIONAL
    								)
	
/*
Desription: 
	This should only be called at PASSIVE_LEVEL or APC_LEVEL

	
	
*/


{
	KIRQL OldIrql;
	PDISPATCHER_HEADER DHeader;
	PKMUTANT Mutant;
	KSTATUS status;

	ASSERT(KeGetCurrentIrql()  <= APC_LEVEL);

//	USED(WaitMode);
//	USED(WaitReason);



	KiLockDispatcherDataBase( &OldIrql);



	DHeader = (PDISPATCHER_HEADER) Object;

	switch(DHeader->Type) {

		case MutantObject:
			
				Mutant = (PKMUTANT) DHeader;			
/*
	Right now we do not support recursive aquisition.

	Though it should be simple and a must to do.
	
*/
				ASSERT2(Mutant->OwnerThread != KeGetCurrentThread(),  
							Mutant->OwnerThread,
							KeGetCurrentThread());

				if (Mutant->Header.SignalState == 0) {
					kprintf("Locking Mutant Owned by 0x%x, 0x%x", Mutant->OwnerThread, KeGetCurrentThread());
					ASSERT(0);
				}
				
				KiDHeaderAcquire(DHeader);

	// Got Mutant, put the OwnerThread
				Mutant->OwnerThread = KeGetCurrentThread();	
				status = STATUS_SUCCESS;
			break;


		case ThreadObject:
		case ProcessObject:		
		case SemaphoreObject:
		case EventNotificationObject:
		case EventSynchronizationObject:
				KiDHeaderAcquire(DHeader);
				status = STATUS_SUCCESS;
			break;

	default:
		KeBugCheck("Not Yet Implemented\n");

	}


	KiUnLockDispatcherDataBase( OldIrql);

	return status;
}
