/*
*  C Implementation: Syscall.c
*
* Description: Kernel SYscall interface
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/


#include <inc/syscall.h>
#include "ex.h"


static int mutex_inited = 0;
static KMUTANT Mutex;


ULONG
ExpSysCallEntry(PULONG Args)
{

	ULONG SyscallIndex;
	ULONG Arg1, Arg2, Arg3;
	ULONG RetStatus = 0;
	KSTATUS Status;
	SyscallIndex = *Args;



	switch(SyscallIndex) {


		case SYS_EXIT:
//Exit the Current Process
			Arg1 = *(Args +1);			
			RetStatus = KExitProcess(0, Arg1);
			return RetStatus;
			break;




		case SYS_FORK:
			return (KCloneProcess());

		case SYS_GETPID:

// Get The current Process Id
			RetStatus = KGetCurrentProcessId();
			return RetStatus;

			break;



	case SYS_TESTING:

			Arg1 = *(Args +1);
			Arg2 = *(Args +2);
			Arg3 = *(Args +3);

			
			kprintf("Inside ExpSysCallEntry %x %x %x %x\n",SyscallIndex, Arg1, Arg2, Arg3);
			{
			if (mutex_inited == 0) {

				KeInitializeMutant( &Mutex, FALSE);
			}

			KeWaitForSingleObject( &Mutex, Executive, UserMode, FALSE, 0);

			}

			return 1;
		break;


	}





}



