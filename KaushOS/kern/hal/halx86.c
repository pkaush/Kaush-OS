/*
*  C Implementation: halx86.c
*
* Description: Halinit
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/

#include "x86.h"
#include "hal.h"
void
Pic8259Init(void);



INLINE BOOL
HalAreInterruptsEnabled()
{
	ULONG_PTR Flags;
	
	asm volatile ("pushfl; popl %0" : "=g" (Flags));
	 return ((Flags & FL_IF) ? TRUE :  FALSE);

}


INLINE BOOL
HalDisableInterrupts()
{
	BOOL intstate;
	intstate = HalAreInterruptsEnabled();
	cli();
	return (intstate);
}



INLINE BOOL
HalEnableInterrupts()
{
	BOOL intstate;
	intstate = HalAreInterruptsEnabled();
	sti();
	return (intstate);
}


//Mask Table for IRQL mapping with 8259 PIC
USHORT Hal8259MaskTable[MAX_IRQL_LEVEL + 1];

VOID
HalRegisterVectortoIrql(
						IN KIRQL Irql,
						IN ULONG	Vector
						)
{
	ULONG irql;
	USHORT mask;



	ASSERT(Irql <=MAX_IRQL_LEVEL);


// Get the irq number from vector
	Vector -= IRQ_START;


/*
	UnMask this interrupt vector for all the < IRQLs.

	We will use this mask for setting the IRQL Levels 
	instead of depending on the pic priotization.
*/
	mask = ~(1 << Vector);
	for(irql = 0; irql < Irql; irql++) {
		Hal8259MaskTable[irql] &= mask;
	}
		
	Pic8259SetMask(Hal8259MaskTable[KeGetCurrentIrql()]);

}

/*
	Kernel Will maintain the value of Current IRQL

	HAL will only mask and unmask the IRQs as per
	New Irql.

	There is no difference in Lower and Raise IRQL
	as per the HAL perspective (except an assert :-))
*/


VOID
HalRaiseIrql(
				IN KIRQL NewIrql,
				IN KIRQL CurrentIrql
				)
{

	ASSERT2(Hal8259MaskTable[CurrentIrql] == Pic8259GetMask(),
				Hal8259MaskTable[CurrentIrql],
				Pic8259GetMask());
	
	ASSERT2(NewIrql >= CurrentIrql, NewIrql, CurrentIrql);

	
	Pic8259SetMask(Hal8259MaskTable[NewIrql]);
}


VOID
HalLowerIrql(
				IN KIRQL NewIrql, 
				IN KIRQL CurrentIrql 
				)
{

	ASSERT2(Hal8259MaskTable[CurrentIrql] == Pic8259GetMask(),
				Hal8259MaskTable[CurrentIrql],
				Pic8259GetMask());
	
	ASSERT2(NewIrql <= CurrentIrql, NewIrql, CurrentIrql);

	Pic8259SetMask(Hal8259MaskTable[NewIrql]);
}









VOID
Halinit(VOID)
{
	int i;
	Pic8259Init();
	
	for (i = 0; i <= MAX_IRQL_LEVEL; i++) {
/*
	Mask All the interrupts
	
	Every device driver with register its handler and unmask its interrupts


	By this time it should give us default mask
*/		


		ASSERT(Pic8259GetMask() == (0xFFFF &  ~(1<<2)));
		Hal8259MaskTable[i] = Pic8259GetMask();
	}
}


