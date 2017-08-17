/*
*  C Implementation: Kperfcount.c
*
* Description:
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/



#include "ke.h"



ULONG NumberOfProcessors = 1;



KSPIN_LOCK SystemLock;


#define NUM_GDT_ENTRIES		6

/*
	I would really like to thanks:
	Mark and David for this
	
*/
typedef struct _KPRCB {

	
	KIRQL			CurrentIrql;
	PKTHREAD		CurrentThread;

//This will be one per processor Thread
	PKTHREAD		IdleThread;

	
// Scheduler Queue
	KTHREAD_LIST Scheduler_list[PROCESS_PRIORITY_MAX +1];


// DPC List

	KDPC_LIST		DpcListHead;


// Time spent in perticular context.
	uint64_t	TimerTicks;
	uint64_t	IdleTime;
	uint64_t	DpcTime;
	uint64_t	IsrTime;

/*
	0 - For APC
	1 - For DPC

	Not Using it right Now!!

	
*/

	ULONG SoftwareInterrupt[2];
	

} KPRCB, *PKPRCB;

typedef struct _KPCR {


	struct _KPRCB Prcb;
	
	// DPCList should also be here
	GDTENTRY GDT[NUM_GDT_ENTRIES];
	PIDTENTRY IDT;
	KTASKSTATE TSS;


	KSPIN_LOCK ProcessorLock;
	
}KPCR, *PKPCR;





KPCR KiProcessorBlock[MAX_ALLOWED_NUM_PROCESSORS];




/*
 Confused :-( How to make it get the current processor Block 
different on different processor

I guess the best solution will be to use APIC Id to find out which
processor we are running on.

We will do it soon :-)

*/
//ULONG CurrentProcessorNumber = 0;


ULONG
KeGetCurrentProcessorNumber()
{



#ifdef __KOS_UP__
	return 0;
#else
	KeBugCheck(" MP is not Yet Implemented");
	return 0;
#endif



}



ULONG
KeGetNumberOfProcessors()
{

#ifdef __KOS_UP__
	return 1;
#else
	KeBugCheck(" MP is not Yet Implemented");
#endif


}












#define KiGetPcr(x) (&KiProcessorBlock[(x)])



#if 0
PKPCR
KiGetPcr(
			IN	ULONG ProcessorNumber
			)
{
//Only single proc right now, MP coming soon ;-)
	ASSERT(ProcessorNumber == 0);

	ASSERT(ProcessorNumber < MAX_ALLOWED_NUM_PROCESSORS);
	return( &KiProcessorBlock[ProcessorNumber]);
}
#endif

PKPCR
KiGetCurrentPcr()
{
	return(KiGetPcr(KeGetCurrentProcessorNumber()));
}



PKPRCB
KiGetPrcb(
				IN	ULONG ProcessorNumber
				)
{
	PKPCR Pcr;
	Pcr = KiGetPcr(ProcessorNumber);
	ASSERT(Pcr);
	ASSERT(&Pcr->Prcb != 0);

	return( &Pcr->Prcb);

}

#if 0

PKPRCB
KiGetCurrentPrcb()
{
	PKPRCB Prcb = KiGetPrcb(KeGetCurrentProcessorNumber());
//	kprintf("KiGetCurrentPrcb: &Pcr->Prcb 0x%x\n", Prcb);
	ASSERT(Prcb);
	return(Prcb);
}

#endif


#define KiGetCurrentPrcb() KiGetPrcb(KeGetCurrentProcessorNumber())

















VOID
KiLockProcessorRaisetoHighLevel(
								IN ULONG ProcessorNumber,
								OUT PKIRQL OldIrql
								)
{

	PKPCR Pcr;

	Pcr = KiGetPcr(ProcessorNumber);

	KeRaiseIrql( HIGH_LEVEL, OldIrql);
	KeAcquireSpinLockAtDpcLevel(&Pcr->ProcessorLock);

}


VOID
KiUnLockProcessorFromHighLevel(
								IN ULONG ProcessorNumber,
								IN KIRQL OldIrql
								)
{

	PKPCR Pcr;

	ASSERT(KeGetCurrentIrql() == HIGH_LEVEL);
	Pcr = KiGetPcr(ProcessorNumber);

	KeReleaseSpinLockFromDpcLevel(&Pcr->ProcessorLock);
	KeLowerIrql(OldIrql);
}


ULONG64
KiGetCurrentTimerTicks()
{
	PKPRCB Prcb = KiGetCurrentPrcb();
	return(Prcb->TimerTicks);
}


VOID INLINE
KiIncrementCurrentTimerTicks()
{
	PKPRCB Prcb = KiGetCurrentPrcb();
	Prcb->TimerTicks++;
}




PKTHREAD
KeGetCurrentThread()
{

	PKPRCB Prcb = KiGetPrcb(KeGetCurrentProcessorNumber());

	ASSERT(Prcb);
	return(Prcb->CurrentThread);
}


PKPROCESS
KeGetCurrentProcess()
{
	PKTHREAD Thread;
	Thread = KeGetCurrentThread();

	if (Thread) {
		return ((PKPROCESS)Thread->ThreadProcess);
	}

	return NULL;
}



VOID
KiSetCurrentThread(
					IN PKTHREAD Thread
					)
{
	PKPRCB Prcb;
	Prcb = KiGetCurrentPrcb();
	ASSERT(Thread);

	Prcb->CurrentThread = Thread;
}


PKTHREAD
KiGetIdleThread()
{
	
	PKPRCB Prcb;
	
	Prcb = KiGetCurrentPrcb();
	return (Prcb->IdleThread);
}


VOID
KiSetIdleThread(
				IN PKTHREAD Thread,
				IN LONG ProcessorNumber
				)
{
	
	PKPRCB Prcb;

//Check Thread AFFINITY here


	if (ProcessorNumber == -1) {
		Prcb = KiGetCurrentPrcb();

	} else {
		Prcb = KiGetPrcb(ProcessorNumber);

	}

	Prcb->IdleThread = Thread;	
}


VOID
KiRequestSoftwareInterrupt(KIRQL Irql)
{
	PKPRCB Prcb;
	ASSERT1((Irql == DISPATCH_LEVEL) ||
			(Irql == APC_LEVEL), Irql);


	Prcb = KiGetCurrentPrcb();

	Prcb->SoftwareInterrupt[Irql-1] = 1;
}

PKTHREAD_LIST
KiGetCurrentSchedulerList(ULONG Priority)
{
	PKPRCB Prcb;
	
	Prcb = KiGetCurrentPrcb();
	return ( &Prcb->Scheduler_list[Priority]);
}



PKDPC_LIST
KiGetDpcList(
						IN ULONG ProcessorNumber
						)
{
	PKPRCB Prcb;
	
	Prcb = KiGetPrcb(ProcessorNumber);

	return(  &Prcb->DpcListHead);
}



VOID
KiIncrementCurrentProcessorsIdleTime()
{

	PKPRCB Prcb;
	Prcb = KiGetCurrentPrcb();

	Prcb->IdleTime ++;
}

VOID
KiIncrementCurrentProcessorsDpcTime()
{

	PKPRCB Prcb;
	Prcb = KiGetCurrentPrcb();

	Prcb->DpcTime ++;
}



VOID
KiIncrementCurrentProcessorsIsrTime()
{

	PKPRCB Prcb;
	Prcb = KiGetCurrentPrcb();

	Prcb->IsrTime ++;
}


KIRQL
KeGetCurrentIrql()

{

	PKPRCB Prcb;

	Prcb = KiGetCurrentPrcb();
	ASSERT1(Prcb->CurrentIrql  <= MAX_IRQL_LEVEL, Prcb->CurrentIrql);
	return (Prcb->CurrentIrql);
}

VOID
KiSetCurrentIrql( 
				IN KIRQL NewIrql
				)
{

	PKPRCB Prcb;

	Prcb = KiGetCurrentPrcb();
	ASSERT1(NewIrql  <= MAX_IRQL_LEVEL, NewIrql);
	Prcb->CurrentIrql = NewIrql;

}



// Our Model GDT
static GDTENTRY MODELGDT[] =
{
	// 0x0 - unused (always faults -- for trapping NULL far pointers)
	SEG_NULL,

	// 0x8 - kernel code segment
	[GD_KT >> 3] = SEG(STA_X | STA_R, 0x0, 0xffffffff, 0),

	// 0x10 - kernel data segment
	[GD_KD >> 3] = SEG(STA_W, 0x0, 0xffffffff, 0),

	// 0x18 - user code segment
	[GD_UT >> 3] = SEG(STA_X | STA_R, 0x0, 0xffffffff, 3),

	// 0x20 - user data segment
	[GD_UD >> 3] = SEG(STA_W, 0x0, 0xffffffff, 3),

	// 0x28 - tss;
	[GD_TSS >> 3] = SEG_NULL
};



//always get system lock before using this
PSEUDODESC Desc;

VOID
KiUpdateTSS(PVOID Stack)
{

	PKPCR Pcr;
//	KIRQL OldIrql;

//	PNumber = KeGetCurrentProcessorNumber();

	Pcr = KiGetCurrentPcr();
	Pcr->TSS.TS_esp0 = (ULONG_PTR)Stack;

}

VOID
KiCreateCurrentGDT()
/*

	This function needs to executed on all the processors


*/

{
	PKPCR Pcr;
	KIRQL OldIrql;

//	PNumber = KeGetCurrentProcessorNumber();

	Pcr = KiGetCurrentPcr();

	KiLockProcessorRaisetoHighLevel(
		KeGetCurrentProcessorNumber(), &OldIrql);
/*
	Copy our MODEL GDT
*/
	memset( Pcr->GDT, 0, sizeof(GDTENTRY)  *NUM_GDT_ENTRIES);

	memcpy( Pcr->GDT, MODELGDT, sizeof(GDTENTRY)  *NUM_GDT_ENTRIES);



/*
	Initialize the TSS field of the GDT.

	Task Register will point to an entry in GDT, 
	which should have the Task State Segment selector.
	
	So also load GD_TSS in Task Register
*/

	Pcr->TSS.TS_ss0 = GD_KD;
	Pcr->GDT[GD_TSS >> 3] = SEG16(STS_T32A, 
					(ULONG_PTR) (&Pcr->TSS),
					sizeof(KTASKSTATE), 0);

	Pcr->GDT[GD_TSS >> 3].sd_s = 0;


	Desc.pd__garbage = 0;
	Desc.pd_lim = (sizeof(MODELGDT)) - 1;
	Desc.pd_base = (ULONG_PTR) Pcr->GDT;


	// Load the GDT
	asm volatile("lgdt Desc+2");
	asm volatile("movw %%ax,%%gs" :: "a" (GD_KD));
	asm volatile("movw %%ax,%%fs" :: "a" (GD_KD));
	asm volatile("movw %%ax,%%es" :: "a" (GD_KD));
	asm volatile("movw %%ax,%%ds" :: "a" (GD_KD));
	asm volatile("movw %%ax,%%ss" :: "a" (GD_KD));
	asm volatile("ljmp %0,$1f\n 1:\n" :: "i" (GD_KT));	// reload cs
	asm volatile("lldt %%ax" :: "a" (0));





	// Load the TSS selector in Task Register
	ltr(GD_TSS);



	KiUnLockProcessorFromHighLevel(KeGetCurrentProcessorNumber(), OldIrql);


}


PVOID
KiGetCurrentGDT()
{

 	KeBugCheck("Not Yet Implemented\n");




}


extern PIDTENTRY IDT;
extern PSEUDODESC IDT_REG_DESC;

VOID
KiCreateCurrentIDT()
{
	PKPCR Pcr;
	ULONG PNumber;


	PNumber = KeGetCurrentProcessorNumber();

	Pcr = KiGetPcr(PNumber);


	// Load the IDT
	asm volatile("lidt IDT_REG_DESC+2");

	ASSERT(Pcr->IDT == NULL);

// Set This GDT in the PCR

	Pcr->IDT = IDT;
	


}


PKTASKSTATE
KiGetCurrentTSS()
{
	PKPCR Pcr;

	Pcr = KiGetCurrentPcr();

	return( &Pcr->TSS);
}


VOID
KiUpdateCurrentTSS(PVOID Stack)
{
	PKPCR Pcr;

	Pcr = KiGetCurrentPcr();
	Pcr->TSS.TS_esp0 = (ULONG_PTR)Stack;
}


VOID
KiInitializeAllProcessors()
{




}

VOID
KiInitProcessorPhase0()
{
	ULONG count, num;

	KeInitializeSpinLock( &SystemLock);
//set the PCR to 0
	for (count = 0; count < MAX_ALLOWED_NUM_PROCESSORS; count++) {
		memset( &KiProcessorBlock[count], 0,
								sizeof(KPCR) );


	//Initialize Scheduler List for all the priorities
		for (num = 0; num < PROCESS_PRIORITY_MAX; num++) {

			LIST_INIT( &KiProcessorBlock[count].Prcb.Scheduler_list[num]);

		}	
//	Initialize the DPC Head
	LIST_INIT( &KiProcessorBlock[count].Prcb.DpcListHead);			
		
	}


}


