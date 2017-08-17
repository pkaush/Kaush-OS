;
;  Asm Implementation: boot.S
;
; Description: The first file of boot loader
;
;
; Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2006
;
; Copyright: See COPYRIGHT file that comes with this distribution
;



%define PROT_MODE_CSEG	0x8
%define PROT_MODE_DSEG	0x10

extern load_kern

[BITS 16]
;[ORG 0x7C00]

global	start

start:
	cld			; Clear Direction Flag
	cli			; Clear interrupts

	xor	ax, ax		; Lets start coding
	mov	ds, ax
	mov	es, ax
	mov	ss, ax
	mov	sp, 0x7000	; Set the Stack Segment to the 0x7000
				; Page Aligned so that we can move it later

enable_A20:			; A pain from the past
.enable1
	in	al, 0x64	; Get the status from port 0x64
	test	al, 0x2		; Test if busy
	jnz	.enable1		; YES
	mov 	al, 0xd1		; Command Write
	out	0x64, al	; Output to port 0x64

.enable2
	in	al, 0x64	; Get status again
	test	al, 0x2		; Test if busy
	jnz	.enable2	; YES
	mov	al, 0xdf	; Command Enable
	out	0x60, al	; Output

; lets jmp to protected mode
	cli			; Block Interrupts(mandatory as we are not setting IDT)
	lgdt	[pGDT]		; Load GDT
	mov	eax, cr0	; Turn on protected protected mode
	or	eax, 0x1
	mov	cr0, eax	
	jmp	PROT_MODE_CSEG:protcseg


[BITS 32]
protcseg:
	
	mov	ax, PROT_MODE_DSEG
	mov	ds, ax
	mov	es, ax
	mov	fs, ax
	mov	gs, ax
	mov	ss, ax

	call load_kern

	jmp	$


gdti:
	dw   0,0,0,0         ; dummy

	dw   0x07FF          ; 8Mb - limit=2047 (2048*4096=8Mb)
	dw   0x0000          ; base address=0
	dw   0x9A00          ; code read/exec
	dw   0x00C0          ; granularity=4096, 386

	dw   0x07FF          ; 8Mb - limit=2047 (2048*4096=8Mb)
	dw   0x0000          ; base address=0
	dw   0x9200          ; data read/write
	dw   0x00C0          ; granularity=4096, 386




pGDT	dw 0x17        ; sizeof(GDT) - 1
	dd gdti        ; address GDT


;times 510-($-$$) db 0
;dw 0xAA55
