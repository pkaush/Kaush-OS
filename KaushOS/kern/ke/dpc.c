/*
*  C Implementation: Dpc.c
*
* Description:  Dpc Implementation
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/


#include "ke.h"
#include <inc/types.h>



#define ASSERT_DPC(Object)                                                   \
ASSERT(((Object)->Type == 0) ||                                          \
           ((Object)->Type == DpcObject) ||                                  \
           ((Object)->Type == ThreadedDpcObject))





VOID
KeSetImportanceDpc (
		IN OUT PRKDPC Dpc,
		IN KDPC_IMPORTANCE Importance
		)

{
	ASSERT_DPC(Dpc);
	Dpc->Importance = Importance;
}



VOID
KeSetTargetProcessorDpc (
		IN OUT PRKDPC Dpc,
		IN ULONG Number
    		)
{
	ASSERT_DPC(Dpc);
	Dpc->Number = Number;
}


VOID 
KeInitializeDpc(
	IN PRKDPC  Dpc,
	IN PKDEFERRED_ROUTINE  DeferredRoutine,
	IN PVOID  DeferredContext
	)
{
	memset(Dpc, 0, sizeof(PKDPC));
	Dpc->Type = DpcObject;
	Dpc->DeferredRoutine = DeferredRoutine;
	Dpc->DeferredContext = DeferredContext;
	Dpc->Importance = LowImportance;
	Dpc->Number = KeGetCurrentProcessorNumber();
}


BOOLEAN 
KeInsertQueueDpc(
	IN PRKDPC  Dpc,
	IN PVOID  SystemArgument1,
	IN PVOID  SystemArgument2
	)
{
	PKDPC_LIST DpcList;
	KIRQL OldIrql;
	PKDPC	DpcElement;
	BOOLEAN Ret = TRUE;

	
#ifdef DEBUG
{
	KIRQL DCurrentIrql;

	ASSERT_DPC(Dpc);
	DCurrentIrql = KeGetCurrentIrql();
	ASSERT1(DCurrentIrql > DISPATCH_LEVEL, DCurrentIrql);
}
#endif	
	Dpc->SystemArgument1 = SystemArgument1;
	Dpc->SystemArgument2 = SystemArgument2;


	DpcList = KiGetDpcList(Dpc->Number);
	ASSERT(DpcList);

/*
	If this element is already in this list return FALSE

	If the next entry of the element is NULL (which will mostly be the case)
	it is sure not inside the list so we don't even need to traverse the list
*/

	KiLockProcessorRaisetoHighLevel(Dpc->Number,  &OldIrql);

	if (LIST_NEXT_INTERNAL(Dpc,DpcListEntry) != NULL) {
		LIST_FOREACH(DpcElement,DpcList,DpcListEntry){
			if(Dpc == DpcElement) {
				Ret = FALSE;
				goto End;
			}
		}
	}


	if (Dpc->Importance == 	HighImportance) {
		LIST_INSERT_HEAD(DpcList, Dpc, DpcListEntry);
	} else {
		LIST_INSERT_TAIL(DpcList, Dpc, DpcListEntry);
	}



End:	
	KiUnLockProcessorFromHighLevel(Dpc->Number,  OldIrql);
	return Ret;
}




BOOLEAN
KeRemoveQueueDpc (
		OUT PRKDPC Dpc
		)
{
	PKDPC_LIST DpcList;
	KIRQL OldIrql;
	PKDPC	DpcElement;
	BOOLEAN Ret = FALSE;
		
#ifdef DEBUG
{
	KIRQL DCurrentIrql;

	ASSERT_DPC(Dpc);
	DCurrentIrql = KeGetCurrentIrql();
	
	ASSERT1(DCurrentIrql == DISPATCH_LEVEL, DCurrentIrql);
}
#endif
	
	
	DpcList = KiGetDpcList(Dpc->Number);
	ASSERT(DpcList);


	KiLockProcessorRaisetoHighLevel(Dpc->Number,  &OldIrql);

	if (LIST_NEXT_INTERNAL(Dpc,DpcListEntry) != NULL) {
		LIST_FOREACH(DpcElement,DpcList,DpcListEntry){
			if(Dpc == DpcElement) {
				LIST_REMOVE(DpcList,Dpc,DpcListEntry);
				Ret = TRUE;
				goto End;
			}
		}
	}


End:

	KiUnLockProcessorFromHighLevel(Dpc->Number,  OldIrql);
	return Ret;
}





VOID
KiProcessDpcQueue(IN KIRQL PreviousIrql)
{
	KIRQL	OldIrql;
	ULONG ProcessorNumber;
	PKDPC_LIST DpcList;

	PKDPC DpcElement;
	
#ifdef DEBUG
{
	KIRQL DIrql = KeGetCurrentIrql();
	ASSERT1(DIrql ==DISPATCH_LEVEL, DIrql);

}
#endif

/*
	This Routine will execute at DISPATCH_LEVEL.
	so we can safely expect that we will not get reshedule
	or the processor won't change
*/


	ProcessorNumber = KeGetCurrentProcessorNumber();
	DpcList = KiGetDpcList(ProcessorNumber);
	ASSERT(DpcList);


	while(!LIST_EMPTY(DpcList)) {

		KiLockProcessorRaisetoHighLevel(ProcessorNumber,
					&OldIrql);


#ifdef DEBUG
{
		KIRQL DIrql = KeGetCurrentIrql();
		ASSERT1(DIrql ==HIGH_LEVEL, DIrql);		
}
#endif


		LIST_REMOVE_HEAD(DpcList, DpcElement, DpcListEntry);



		KiUnLockProcessorFromHighLevel(ProcessorNumber,
					OldIrql);

#ifdef DEBUG
{	// Back to Dispatch Level
		KIRQL DIrql = KeGetCurrentIrql();
		ASSERT1(DIrql ==DISPATCH_LEVEL, DIrql); 	
}
#endif


/*

	Call the DPC Routine

*/



		DpcElement->DeferredRoutine (DpcElement,
						DpcElement->DeferredContext,
						DpcElement->SystemArgument1,
						DpcElement->SystemArgument2
						);

		
		
	}

	
}
