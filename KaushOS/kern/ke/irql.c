/*
*  C Implementation: IRQL.c
*
* Description:
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/


#include	<kern/ke/ke.h>




VOID 
KeRaiseIrql(
		IN KIRQL  NewIrql,
		OUT PKIRQL  OldIrql
		)
{

	KIRQL CurrentIrql;
	CurrentIrql = KeGetCurrentIrql();

	ASSERT(NewIrql >= CurrentIrql);
	ASSERT(NewIrql !=3);

	if (NewIrql != CurrentIrql) {
// Set the new IRQL in HAL
		HalRaiseIrql(NewIrql,CurrentIrql);

// Set the new IRQL in PRCB
		KiSetCurrentIrql(NewIrql);
	}

	
	*OldIrql = CurrentIrql;
}




VOID 
KeLowerIrql(
		IN KIRQL  OldIrql
		)
{
	KIRQL CurrentIrql;
	BOOL ints;
	static int count = 0;

//	ints = HalDisableInterrupts();

	CurrentIrql = KeGetCurrentIrql();
	
	ASSERT2(OldIrql <= CurrentIrql, OldIrql, CurrentIrql);
	

/*
	Take Care of pending DPC and APC
*/


	if ((OldIrql < DISPATCH_LEVEL) &&
		(CurrentIrql >= DISPATCH_LEVEL)) {

		KiSetCurrentIrql(DISPATCH_LEVEL);
		HalLowerIrql(DISPATCH_LEVEL,CurrentIrql);

// Update the current Irql here only instead of quering again.
		CurrentIrql = DISPATCH_LEVEL;

		KiProcessDpcQueue();

	}

	

	if (OldIrql != CurrentIrql) {
		KiSetCurrentIrql(OldIrql);
		HalLowerIrql(OldIrql,CurrentIrql);
	}
	
//	if (ints == TRUE) {
		
//		HalEnableInterrupts();
//	}

}





VOID INLINE
KiRegisterVectortoIrql(
							IN KIRQL Irql,
							IN ULONG Vector
							)
{

	HalRegisterVectortoIrql(Irql,Vector);

}




