/*
*  Asm Implementation: ctxswitch.S
*
* Description: Context Switch - It will switch the current Thread with the new Thread
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/


.intel_syntax noprefix

#include <inc/asm.h>




/*
 *	KSTATUS
 *	KiContextSwitch(ULONG_PTR NextStackPointer,
 *				ULONG_PTR *CurrentStackPointer);
 */

ENTRY(KiContextSwitch)

	mov	eax, esp // we will use eax as frame pointer here
	pushf
	pusha			// Push all the general purpose registers


	
	mov	ebx, [eax + 0x8]
	mov	[ebx], esp

		// new context
	mov	esp, [eax + 0x4]


	popa
	popf	
	mov	eax, 0x0	//SUCCESS :-)
	ret









/*
 *	VOID
 *	KiRunUserThread(IN PKTRAPFRAME Trapframe)
 


ENTRY(KiRunUserThread)

	pop eax
	mov esp, eax

	popa

	pop es
	pop ds
	add esp, 0x8

	iretd


*/


