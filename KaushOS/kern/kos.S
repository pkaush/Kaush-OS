/*
*  Asm Implementation: KOS.S
*
* Description: Entry point to KOS.
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/

#include <inc/asm.h>
#include <kern/hal/x86.h>


#define	RELOC(x) ((x) - KERNBASE)

.intel_syntax noprefix
.text
.align 4
.globl		_start
_start:
//	mov 0x472, 0x1234	
	lgdt	RELOC(newgdtdesc)

//Load the Segment reg as per the new segmentation policy
	mov	eax, 0x10
	mov	ds, ax
	mov	es, ax
	mov	ss, ax

.att_syntax noprefix
/*
	There is a know issue with gas version that I am using
	which gives an error for 'jmp cs:eip'. so lets use
	at&t syntax for this for now. Will correct when we ugrade to
	new version.
*/
	ljmp $0x8,$relocated
	
//	jmp 0x8:relocated
	
	
relocated:

//Leave a few words on the stack(just for precaution)
//	mov	esp, (bootstacktop - 8)
/*
	Looks like GNU Assembler doesn't support intel syntax very well
	This also didn't work out so making this also in at&t syntax
*/
	movl	$(bootstacktop - 8),%esp


.intel_syntax noprefix

//	For gdb 'bt' to end corretly 
	mov	ebp, 0x0
	

//	now to C code
	call	KOS_start

//Should never get here, but in case we do, just spin.
spin:	jmp	spin

.att_syntax noprefix


###################################################################
# boot stack
###################################################################
	.p2align	PGSHIFT		# force page alignment
	.globl		bootstack
bootstack:
	.space		(PGSIZE * 3)	# default stack
	.globl		bootstacktop   
bootstacktop:

###################################################################
# setup the GDT	
###################################################################
.p2align	2				# force 4 byte alignment
newgdt:
	SEG_NULL				# null seg
	SEG(STA_X|STA_R, -KERNBASE, 0xffffffff)	# code seg
	SEG(STA_W, -KERNBASE, 0xffffffff)	# data seg
newgdtdesc:
	.word	0x17			# sizeof(mygdt) - 1
	.long	RELOC(newgdt)		# address mygdt
