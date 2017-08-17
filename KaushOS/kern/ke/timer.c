/*
*  C Implementation: 
*
* Description: Implemetation of 8254 PIT
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/

#include "interrupt.h"

#define TIMER_INTERRUPTS	100 	//per sec

#define TIMER_FREQ	1193182




VOID
TimerISRoutine(KTRAPFRAME TrapFrame)
{
	KiIncrementCurrentTimerTicks();	
	KiThreadTimerISR();
}





VOID
KiTimerInit () 
{
  /* 8254 input frequency divided by TIMER_FREQ, rounded to
     nearest. */
  USHORT Count = ( TIMER_INTERRUPTS + TIMER_FREQ / 2) / TIMER_FREQ;

  outb (0x43, 0x34);    /* CW: counter 0, LSB then MSB, mode 2, binary. */
  outb (0x40, Count & 0xff);
  outb (0x40, Count >> 8);


  KiCreateInterruptObject(
  			IrqTimer,
  			0x0,
  			&TimerISRoutine,
  			"Timer");
  
}













KSTATUS
KeDelayExecutionThread(
							IN KPROCESSOR_MODE WaitMode,
							IN BOOLEAN Alertable,
							IN PLARGE_INTEGER Interval
							)
{
//TODO: Interval currently is treated as ticks count. we need to convert it to (nanosec *100)
	
	ULONG64 StartTicks = KiGetCurrentTimerTicks();
	ULONG LocalInterval = 0;

	ASSERT(HalEnableInterrupts());
	
	if (*Interval < 0) {

		LocalInterval = -1 *(*Interval);
		while (KiGetCurrentTimerTicks() - StartTicks < LocalInterval) {
			
			Schedule(NULL, NULL, NULL, NULL);
		}
	

	} else {

		LocalInterval = -1 *(*Interval);
		while (KiGetCurrentTimerTicks()  < LocalInterval) {
		
			Schedule(NULL, NULL, NULL, NULL);
		}
		
	}
}



