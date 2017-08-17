
/*
*  C Implementation: pic.c
*
* Description: Reprogramming PIC
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/


#include <inc/assert.h>
#include <inc/stdio.h>
#include <inc/types.h>

#include "X86.h"
#include "hal.h"

#define PIC0_CTRL	0x20    /* Master PIC control register address. */
#define PIC0_DATA	0x21    /* Master PIC data register address. */
#define PIC1_CTRL	0xa0    /* Slave PIC control register address. */
#define PIC1_DATA	0xa1    /* Slave PIC data register address. */

#define IRQ_SLAVE	2	// IRQ at which slave connects to master
#define IRQ_OFFSET	32	// IRQ 0 corresponds to int IRQ_OFFSET


// Current IRQ mask.
// Initial IRQ mask has interrupt 2 enabled (for slave 8259A).
static USHORT Pic8259Mask = 0;

void
Pic8259SetMask(USHORT mask);



#define MAX_IRQS	16	// Number of IRQs
static ULONG Picinitdone = 0;





/* Initialize the 8259A interrupt controllers. */
void
Pic8259Init(void)
{

	/* Mask all interrupts on both PICs. */
	outb (PIC0_DATA, 0xff);
	outb (PIC1_DATA, 0xff);

	 /* Initialize master. */
	outb (PIC0_CTRL, 0x11); /* ICW1: single mode, edge triggered, expect ICW4. */
	outb (PIC0_DATA, IRQ_OFFSET); /* ICW2: line IR0...7 -> irq 0x20...0x27. */
	outb (PIC0_DATA, 0x04); /* ICW3: slave PIC on line IR2. */
	outb (PIC0_DATA, 0x01); /* ICW4: 8086 mode, normal EOI, non-buffered. */

	/* Initialize slave. */
	outb (PIC1_CTRL, 0x11); /* ICW1: single mode, edge triggered, expect ICW4. */
	outb (PIC1_DATA, (IRQ_OFFSET + 8)); /* ICW2: line IR0...7 -> irq 0x28...0x2f. */
	outb (PIC1_DATA, 0x02); /* ICW3: slave ID is 2. */
	outb (PIC1_DATA, 0x01); /* ICW4: 8086 mode, normal EOI, non-buffered. */

// Set that we are done with init
	Picinitdone = 1;

	/* Unmask the interrupts*/
	Pic8259SetMask(DEFAULT_PIC_MASK);
}



USHORT
Pic8259GetMask()
{
	return(Pic8259Mask);
}


INLINE BOOL
HalAreInterruptsEnabled();


VOID
Pic8259SetMask(USHORT mask)
{

	int i;
	
/*
	If we haven't yet programmed the pic then at least interrupts
	should be disable
*/
	
	if (Picinitdone == 0) {
		ASSERT(mask == Pic8259Mask);
		ASSERT(HalAreInterruptsEnabled() == FALSE);
		return;
	}

/*
	There is no way to query the interrupt mask from the pic
	so keep a local copy.
*/	
	Pic8259Mask = mask;

	
	outb(PIC0_DATA, (char)mask);
	outb(PIC1_DATA, (char)(mask >> 8));

#if 0 //DEBUG
	Log( LOG_INFO,"enabled interrupts:");
	for (i = 0; i < 16; i++)
		if (~mask & (1<<i))
			Log(LOG_INFO, " %d", i);
	Log(LOG_INFO, "\n");
#endif

}

