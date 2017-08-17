/*
*  C Implementation: Ke.c
*
* Description: Main file of the kernel
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/

#include "ke.h"



VOID
KiInitProcessorPhase0();


VOID
KiTrapInitialize();


VOID
KeInitPhase0()
{
	//Initialize Processor

	KiInitProcessorPhase0();


	//Initialize IDT
	KiTrapInitialize();

	KiSchedInitPhase();



}



VOID
KeInitPhase1()
{

	KiTimerInit();

}

