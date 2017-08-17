/*
*  C Implementation: locks.c
*
* Description: Syncronisation and stuff ...
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/


#include	"ke.h"
#include	<inc/assert.h>

#include	<inc/interlocked.h>

#if 0
LONG 
InterlockedExchange(
							IN OUT PLONG  Target,
							IN LONG  Value
							)
{
	LONG ret;


	
//	__asm (".intel_syntax noprefix");
//	__asm __volatile ("xchg [ecx], eax":"=a" (ret) : "a" (Value),  "c" (Target));
//	__asm  (".att_syntax noprefix");

	return ret;
}


LONG 
InterlockedExchangeAdd(
							IN OUT PLONG  Addend,
							IN LONG  Value
							)
{
	LONG ret;

//	__asm (".intel_syntax noprefix");
//	__asm __volatile ("lock xadd [ecx], eax": "=a" (ret): "c" (Addend), "a" (Value));
//	__asm  (".att_syntax noprefix");
	
	return ret;
}


LONG 
InterlockedIncrement(
						IN PLONG  Addend
						)
{
	return(InterlockedExchangeAdd(Addend, 1));
}

LONG 
InterlockedDecrement(
						IN PLONG  Addend
						)
{
	return (InterlockedExchangeAdd(Addend, -1));
}

#endif

VOID
KeInitializeSpinLock(IN PKSPIN_LOCK SpinLock)
{
	memset(&SpinLock, 0, sizeof(KSPIN_LOCK));
}



KSPIN_LOCK DispatcherDataBaseLock;


ULONG
KeQuerySpinLock(
				IN PKSPIN_LOCK SpinLock, 
				OUT PKTHREAD *Thread
				)
{

	*Thread = SpinLock->Thread;
	return(SpinLock->Lock);
}



VOID
KeAcquireSpinLock(
					IN PKSPIN_LOCK SpinLock,
					OUT PKIRQL OldIrql
					)
{

	ULONG Result = 1;

#ifdef DEBUG
{
PKTHREAD DThread;
// Assert if the IRQL is Greater Then DISPATCH_LEVEL
	ASSERT1(KeGetCurrentIrql() <= DISPATCH_LEVEL, KeGetCurrentIrql());



//Assert if this lock is already acquired by the current Thread
	if (SpinLock->Thread) {
		DThread = KeGetCurrentThread();
		ASSERT2(SpinLock->Thread != DThread, SpinLock->Thread, DThread);
	}

}
#endif



//Raise the IRQL
	KeRaiseIrql(DISPATCH_LEVEL,OldIrql);



/*
	For a uniproc machine raising the IRQL will suffice our requirement.

	As we are already preparing ourself to run on Multiproc let add the
	xchg also.

*/

#if defined(__KOS_MP__)
	while (Result != 0) {
		Result = InterlockedExchange( &SpinLock->Lock, 1);
	}
#endif

// store some debugging info
	SpinLock->OldIrql = *OldIrql;
	SpinLock->Thread = KeGetCurrentThread();



}


VOID 
KeReleaseSpinLock(
		IN PKSPIN_LOCK  SpinLock,
		IN KIRQL  OldIrql
		)

{
	ULONG Result;


#ifdef DEBUG
{
	PKTHREAD DThread;
	KIRQL DIRQL;

//Some debugging
	DIRQL = KeGetCurrentIrql();
	DThread = KeGetCurrentThread();
	ASSERT1(DIRQL == DISPATCH_LEVEL, DIRQL);


/*
	Check if the OldIRQL was the one we started from


	This will not be true for DispatcherDataBaseLock, well soon it will not be 
	true for most of the scenarios and we have to remove this assert.
	Till then we can use it for some more testing
*/

	if( SpinLock != &DispatcherDataBaseLock) {
		ASSERT2(SpinLock->OldIrql == OldIrql,SpinLock->OldIrql, OldIrql );

		ASSERT2(SpinLock->Thread == DThread, 
						SpinLock->Thread, DThread);


	}
	

}
#endif


#if defined( __KOS_MP__)
			Result = InterlockedExchange( &SpinLock->Lock, 0);
#endif


// If lock wasn't already acquired assert
	ASSERT1(Result == 1, Result);

	SpinLock->OldIrql = 0;
	SpinLock->Thread = 0;

	KeLowerIrql(OldIrql);

}





VOID
KeAcquireSpinLockAtDpcLevel(
								IN PKSPIN_LOCK  SpinLock
								)
/*
	The only differnce in KeAcquireSpinLock and KeAcquireSpinLockAtDpcLevel
	is that we do not raise the IRQL in the later.

	And yeah asserts are also different ;-)
	
*/



{

	ULONG Result = 1;

#ifdef DEBUG
{
	PKTHREAD DThread;
	KIRQL DIrql;

	DIrql = KeGetCurrentIrql();
// Assert if the IRQL is Greater Then DISPATCH_LEVEL
	ASSERT1(DIrql >= DISPATCH_LEVEL, DIrql);

//Assert if this lock is already acquired by the current Thread
	if (SpinLock->Thread) {
		DThread = KeGetCurrentThread();
		ASSERT2(SpinLock->Thread != DThread, SpinLock->Thread, DThread);
	}

}
#endif



/*
	For a uniproc machine raising the IRQL will suffice our requirement, and if we are already 
	at Dispatch level don't do anything.

	As we are already preparing ourself to run on Multiproc let add the
	xchg also.

*/

#if defined(__KOS_MP__)
	while (Result != 0) {
		Result = InterlockedExchange( &SpinLock->Lock, 1);
	}
#endif

// store some debugging info
	SpinLock->OldIrql = DISPATCH_LEVEL;
	SpinLock->Thread = KeGetCurrentThread();



}




VOID 
  KeReleaseSpinLockFromDpcLevel(
								IN PKSPIN_LOCK  SpinLock
								)

{
	ULONG Result;
#ifdef DEBUG
//Some debugging	

	KIRQL DIrql;
	PKTHREAD DThread;

	DIrql = KeGetCurrentIrql();
	DThread = KeGetCurrentThread();
	ASSERT1(DIrql >= DISPATCH_LEVEL, DIrql);
	ASSERT2(SpinLock->Thread == DThread, 
						SpinLock->Thread, DThread);
#endif
	
#if defined( __KOS_MP__)
			Result = InterlockedExchange( &SpinLock->Lock, 0);

	// If lock wasn't already acquired assert
			ASSERT1(Result == 1, Result);
#endif



	SpinLock->OldIrql = 0;
	SpinLock->Thread = 0;

/*
	In KeReleaseSpinLockFromDpcLevel we do not need to lower the IRQL
*/

//	KeLowerIrql(OldIrql);

}
















/*********************************
	Dispatcher Database
*********************************/







VOID INLINE
KiLockDispatcherDataBase(OUT PKIRQL OldIrql)
{
	KeAcquireSpinLock( &DispatcherDataBaseLock,OldIrql);
}


VOID INLINE
KiUnLockDispatcherDataBase(IN KIRQL OldIrql)
{
	KeReleaseSpinLock( &DispatcherDataBaseLock,OldIrql);
}



VOID INLINE
KiLockDispatcherDataBaseAtDpcLevel()
{
	KeAcquireSpinLockAtDpcLevel( &DispatcherDataBaseLock);
	kprintf("L Dis\n");
}


VOID INLINE
KiUnLockDispatcherDataBaseFromDpcLevel()
{
	kprintf("UL Dis\n");
	KeReleaseSpinLockFromDpcLevel( &DispatcherDataBaseLock);
}

ULONG
KiQueryDispatcherDatabase(
							OUT PKTHREAD * Thread
							)
{
	return(KeQuerySpinLock( &DispatcherDataBaseLock,Thread));
}

void
LockTesting()
{

	int a = 5;
	int b = 10;



	b = InterlockedExchange(&a,b);
 	ASSERT(b == 5);
	ASSERT(a == 10);

	b = InterlockedExchangeAdd(&a, b);
	ASSERT(a == 15);
	ASSERT(b == 10);

	b = InterlockedDecrement(&a);
	ASSERT(a == 14);
	ASSERT(b == 15);

	b = InterlockedIncrement(&a);	
	ASSERT(a == 15);
	ASSERT(b == 14);




}



