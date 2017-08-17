/*
*  C Implementation: Apc.c
*
* Description:  APC Implementation
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/





typedef struct _KAPC {
    UCHAR Type;
    UCHAR SpareByte0;
    UCHAR Size;
    UCHAR SpareByte1;
    ULONG SpareLong0;
    struct _KTHREAD *Thread;
    LIST_ENTRY ApcListEntry;
    PKKERNEL_ROUTINE KernelRoutine;
    PKRUNDOWN_ROUTINE RundownRoutine;
    PKNORMAL_ROUTINE NormalRoutine;
    PVOID NormalContext;

    //
    // N.B. The following two members MUST be together.
    //

    PVOID SystemArgument1;
    PVOID SystemArgument2;
    CCHAR ApcStateIndex;
    KPROCESSOR_MODE ApcMode;
    BOOLEAN Inserted;
} KAPC, *PKAPC, *PRKAPC;




VOID 
  KeEnterCriticalRegion()
{
/*
We will implement this with the APCs till then just keep the blank def here
*/
		ASSERT1(KeGetCurrentIrql() <= APC_LEVEL, KeGetCurrentIrql());



}

VOID 
KeLeaveCriticalRegion()
{
	ASSERT1(KeGetCurrentIrql() <= APC_LEVEL, KeGetCurrentIrql());

}

BOOLEAN
KeAreApcsDisabled()
{
		ASSERT1(KeGetCurrentIrql() <= APC_LEVEL, KeGetCurrentIrql());

}

BOOLEAN
KeAreAllApcsDisabled ()
{
	ASSERT1(KeGetCurrentIrql() <= APC_LEVEL, KeGetCurrentIrql());

}

