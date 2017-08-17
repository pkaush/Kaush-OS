/*
*  C Implementation:  Interrupt.c
*
* Description: Implementation of system interrupts
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/

#include <kern/hal/x86.h>
#include <inc/assert.h>
#include <inc/string.h>


#include <kern/mm/mm.h>
#include "interrupt.h"
#include <kern/ps/process.h>



// Common IDT for all processors


IDTENTRY IDT[256] = { {0}, };

PSEUDODESC IDT_REG_DESC =
{
	0, sizeof(IDT) - 1, (ULONG_PTR) IDT,
};




/*  
	This is our Interrupt  Object which will can also be 
	create by IoConnectInterrupt
*/

typedef struct _KINTERRUPT {
	
	PINTERRUPT_HANDLER	KISRRoutine;
	KIRQL	Irql;
	KSPIN_LOCK		ISR_Spinlock;
	PCHAR InterruptName;
	ULONG Vector;

} KINTERRUPT, PKINTERRUPT;



//Interrupt Handlers
extern PVOID _TrapFunctionList[INTR_CNT];
static KINTERRUPT KiInterruptObjects[INTR_CNT];



static const PCHAR
GetTrapname(ULONG vector)
{
	return (KiInterruptObjects[vector].InterruptName);
}


VOID
KiCreateCurrentIDT();








VOID
PrintGeneralRegs(PGENERALREGISTERS Regs)
{


	kprintf("  edi  0x%08x\n", Regs->Reg_edi);
	kprintf("  esi  0x%08x\n", Regs->Reg_esi);
	kprintf("  ebp  0x%08x\n", Regs->Reg_ebp);
	kprintf("  oesp 0x%08x\n", Regs->Reg_oesp);
	kprintf("  ebx  0x%08x\n", Regs->Reg_ebx);
	kprintf("  edx  0x%08x\n", Regs->Reg_edx);
	kprintf("  ecx  0x%08x\n", Regs->Reg_ecx);
	kprintf("  eax  0x%08x\n", Regs->Reg_eax);


}


VOID
PrintKTrapframe(PKTRAPFRAME Trapframe)
{

	kprintf("TRAP frame at %p\n", Trapframe);
	PrintGeneralRegs(&Trapframe->TrapFrame_regs);
	kprintf("  es   0x----%04x\n", Trapframe->TrapFrame_es);
	kprintf("  ds   0x----%04x\n", Trapframe->TrapFrame_ds);
	kprintf("  trap 0x%08x %s\n", Trapframe->TrapFrame_trapno, 
			GetTrapname(Trapframe->TrapFrame_trapno));
	kprintf("  err  0x%08x\n", Trapframe->TrapFrame_err);
	kprintf("  eip  0x%08x\n", Trapframe->TrapFrame_eip);
	kprintf("  cs   0x----%04x\n", Trapframe->TrapFrame_cs);
	kprintf("  flag 0x%08x\n", Trapframe->TrapFrame_eflags);
	kprintf("  esp  0x%08x\n", Trapframe->TrapFrame_esp);
	kprintf("  ss   0x----%04x\n", Trapframe->TrapFrame_ss);

}




static VOID
KiAcknowledgeInterrupt(ULONG Irq)
{

	ASSERT( (Irq >= IRQ_START) && (Irq <= IRQ_END));

	/* Acknowledge master PIC. */
	outb (0x20, 0x20);

	/* Acknowledge slave PIC if this is a slave interrupt. */
	if (Irq >= 0x28) {
		outb (0xa0, 0x20);
	}

}




static VOID
KiInterruptDispatch( PKTRAPFRAME Trapframe)
{
	BOOLEAN IntStatus;
	ULONG Vector;
	KIRQL OldIrql;
	
	IntStatus = HalAreInterruptsEnabled();

	ASSERT(IntStatus == FALSE);
	
	Vector = Trapframe->TrapFrame_trapno;

/*

I think The interrupts should be handled in this order

	- Raise the IRQL
	- Acquire the Lock
	- Enable the Interrupts
	- Run the ISR
	- Acknowledge the Interrupts

*/

	KeRaiseIrql(KiInterruptObjects[Vector].Irql, &OldIrql);
	KeAcquireSpinLockAtDpcLevel(&KiInterruptObjects[Vector].ISR_Spinlock);

	HalEnableInterrupts();



	KiInterruptObjects[Vector].KISRRoutine(Trapframe);



/*
	We have to Acknowledge the interrupt before Lowering the IRQL
	as new thread might start running as soon as we lower the interrupt.
*/
	KiAcknowledgeInterrupt(Vector);


/*
	- Release the Lock
	- Lower the Irql 
*/

	KeReleaseSpinLockFromDpcLevel( &KiInterruptObjects[Vector].ISR_Spinlock);
	KeLowerIrql(OldIrql);




	IntStatus = HalAreInterruptsEnabled();
	ASSERT(IntStatus == TRUE);
}




static VOID
KiExceptionDispatch(PKTRAPFRAME Trapframe)
{



	KiInterruptObjects[Trapframe->TrapFrame_trapno].KISRRoutine(Trapframe);

	



//iInterruptObjects[Trapframe->TrapFrame_trapno].KISRRoutine(Trapframe);

}


static VOID
KiSystemCallDispatch(PKTRAPFRAME Trapframe)
{
	ULONG Vector;
	ULONG RetStatus;
	// Interrupts should be enabled
	ASSERT(HalAreInterruptsEnabled() != TRUE);

	//Enable Interrupts
	HalEnableInterrupts();

	//Get the vector
	Vector = Trapframe->TrapFrame_trapno;
	ASSERT1(IsSystemCallIntr(Vector), Vector);


	RetStatus = ExpSysCallEntry(Trapframe->TrapFrame_esp);
	//KiInterruptObjects[Vector].KISRRoutine(Trapframe);


	// Return the status to usermode
	Trapframe->TrapFrame_regs.Reg_eax = RetStatus;

}





// Called from int.S
VOID
_KiTrap( PKTRAPFRAME Trapframe)
{

	if (IsDeviceInterrupt(Trapframe->TrapFrame_trapno)) {
		KiInterruptDispatch(Trapframe);
	} else if (IsSystemException(Trapframe->TrapFrame_trapno)){
		KiExceptionDispatch(Trapframe);
	} else if (IsSystemCallIntr(Trapframe->TrapFrame_trapno)) {
		KiSystemCallDispatch(Trapframe);
	}
	else {
		PrintKTrapframe(Trapframe);
		KeBugCheck("Unknown Exception\n");
	}	




}




VOID INLINE
KiRegisterVectortoIrql(
							IN KIRQL Irql,
							IN ULONG Vector
							);

KIRQL
KiGetIrqlForVector(ULONG Vector)

{
	return( CLOCK1_LEVEL - (Vector - IRQ_START));
}


VOID
KiCreateInterruptObject(
					IN ULONG Vector,
					IN ULONG DPL,
					IN PINTERRUPT_HANDLER Routine,
					IN const PCHAR InterruptName
					)
{

	KIRQL Irql;

/*
	Later we will use this condition to check if we need to create chained 
	Interrupt objects. For now one interrupt per objcet should be ok.

	Right now as we do not support the chained objects so assert
*/


	ASSERT(IsDeviceInterrupt(Vector) || IsSystemCallIntr(Vector));
	ASSERT(KiInterruptObjects[Vector].KISRRoutine == NULL);

/*
	 We need to recreate the IDTENTRY to change the DPL

	We will create trap gate to keep the interrupts disable and
	will enable tham manully after setting the proper mask as 
	per the IRQL assigned

*/

	SETGATE(IDT[Vector], 0, GD_KT, _TrapFunctionList[Vector], DPL);

/*
	Get the device IRQL only if this is a hardware device related Vector. 
	The syscall init will also call this function but that should assign an IRQL
*/

	Irql = IsDeviceInterrupt(Vector)?KiGetIrqlForVector(Vector) : PASSIVE_LEVEL;

	KiInterruptObjects[Vector].InterruptName = InterruptName;
	KiInterruptObjects[Vector].KISRRoutine = Routine;
	KiInterruptObjects[Vector].Vector = Vector;
	KiInterruptObjects[Vector].Irql = Irql;

	if (Irql != PASSIVE_LEVEL) {
		//Create a IRQL Mapping for the vector
		KiRegisterVectortoIrql(Irql,Vector);
	}
	
	Log(LOG_INFO, "KiCreateInterruptObject: Vector %ld, DPL %ld, Routine 0x%x,InterruptName %s\n Irql 0x%x\n",
					Vector, DPL, Routine, InterruptName, Irql);

	

	KeInitializeSpinLock( &KiInterruptObjects[Vector].ISR_Spinlock);



}


VOID
KiRegisterException(
					IN ULONG Vector,
					IN ULONG DPL,
					IN BOOL InterruptsOn,
					IN PINTERRUPT_HANDLER Routine,
					IN const PCHAR InterruptName
					)
{

	ASSERT (IsSystemException(Vector));
	ASSERT(KiInterruptObjects[Vector].KISRRoutine == NULL);

// We need to recreate the IDTENTRY to change the DPL
	SETGATE(IDT[Vector], InterruptsOn, GD_KT, 
							_TrapFunctionList[Vector], DPL);

	KiInterruptObjects[Vector].InterruptName = InterruptName;
	KiInterruptObjects[Vector].KISRRoutine = Routine;
	KiInterruptObjects[Vector].Vector = Vector;

// we do not need this as we should not get any exception in exception handler
//	KeInitializeSpinLock( &KiInterruptObjects[Vector].ISR_Spinlock);



}


VOID
KiInitSyscallInterface()

// Register the syscall interface

{

	KiCreateInterruptObject(
						SYSCALL_INTR,
						3,
						NULL,
						"SysCall");

}



VOID
KiTrapInitialize(void)
{
	ULONG count;

	

	for (count = 0; count < INTR_CNT; count++) {

		SETGATE(IDT[count], 0, GD_KT, _TrapFunctionList[count], 0);
	//Initialize all the names to UNKNOWN
		KiInterruptObjects[count].InterruptName = "UNKNOWN";
		KiInterruptObjects[count].KISRRoutine = NULL;
		KeInitializeSpinLock( &KiInterruptObjects[count].ISR_Spinlock);
		KiInterruptObjects[count].Vector = count;
	}


	KiCreateCurrentIDT();

	KeInitializeExceptions();

	// Setup the syscall interrupt interface	
	KiInitSyscallInterface();

}

