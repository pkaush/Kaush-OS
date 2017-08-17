/*
*  C Implementation:  Exception.c
*
* Description: Exception Handling
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/

#include "ke.h"
#include "interrupt.h"

VOID
PrintKTrapframe(PKTRAPFRAME Trapframe);


static VOID 
KiExceptionHandler (PKTRAPFRAME TrapFrame)
{

	PrintKTrapframe(TrapFrame);

	KeBugCheck("EXCEPTION");
}



static VOID 
KiExceptionPFHandler (PKTRAPFRAME TrapFrame)
{

	ULONG Cr2;

	Cr2 = rcr2();

	// For Page fault we have disable interrupts to protect the value in CR2
	HalEnableInterrupts();

	kprintf("KiExceptionPFHandler\n");

	MmPfHandler( Cr2,TrapFrame);
}





VOID
KeInitializeExceptions()
{

	KiRegisterException(T_DIVIDE, 0, TRUE, &KiExceptionHandler, "Divide Error");
	KiRegisterException(T_DEBUG, 0, TRUE, &KiExceptionHandler, "Reserved");
//I do not think we can actually have NMI this way, but lets have the handler
	KiRegisterException(T_NMI, 0, TRUE, &KiExceptionHandler, "NMI");

/*
	Usermode can raise these exceptions, so keep the DPL as 3
	Rest will have DPL 0 so that usermode can't raise them using INT instruction
*/
	KiRegisterException(T_BRKPT, 3, TRUE, &KiExceptionHandler, "BreakPoint");
	KiRegisterException(T_OFLOW, 3, TRUE, &KiExceptionHandler, "OverFlow");
	KiRegisterException(T_BOUND, 3, TRUE, &KiExceptionHandler, "Bound RangeExceeded");


	KiRegisterException(T_INVOP, 0, TRUE, &KiExceptionHandler, "Invalid Opcode");
	KiRegisterException(T_DEVICE, 0, TRUE, &KiExceptionHandler, "Device Not Available");
		

	KiRegisterException(T_DBLFLT, 0, TRUE, &KiExceptionHandler, "Double Fault");
	KiRegisterException(T_COPROC, 0, TRUE, &KiExceptionHandler, "Coprocessor");
	KiRegisterException(T_INVTSS, 0, TRUE, &KiExceptionHandler, "Invalid TSS");
	KiRegisterException(T_SEGNP, 0, TRUE, &KiExceptionHandler, "Segment Not Present");
	KiRegisterException(T_STACK, 0, TRUE, &KiExceptionHandler, "Stack Segment Fault");
	KiRegisterException(T_GPFLT, 0, TRUE, &KiExceptionHandler, "General Protection Fault");

	//Interrupts should be disable for Page Fault, so we do not overwrite CR2
	KiRegisterException(T_PGFLT, 0, FALSE, &KiExceptionPFHandler, "Page Fault");
	KiRegisterException(T_RES, 0, TRUE, &KiExceptionHandler, "Reserved");
	KiRegisterException(T_FPERR, 0, TRUE, &KiExceptionHandler, "x87 FPU floating-point");
	KiRegisterException(T_ALIGN, 0, TRUE, &KiExceptionHandler, "Alignment check");
	KiRegisterException(T_MCHK, 0, TRUE, &KiExceptionHandler, "Machine check");
	KiRegisterException(T_SIMDERR, 0, TRUE, &KiExceptionHandler, "SIMD floating-point exception");

}


