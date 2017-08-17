/*
*  C Implementation: loadk.c
*
* Description: 
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/

#include <kern/hal/x86.h>
#include <inc/elf.h>

#define SECTSIZE	512
#define ELFHDR		((struct Elf *) 0x10000) // scratch space

void readsect(void*, ULONG);
void readseg(ULONG, ULONG, ULONG);


/*
void
print(char *str)
{
static unsigned int i = 0;
int8_t *vram = (int8_t *)0xb8000;

        for(; *str != 'z'; str++, i+=2)
        {
                *(vram + i) = *str;
                *(vram + i + 1) = 0x20;
        }
}
*/

/* & every Virtual Address with 0xffffff to convert it to physical address */

void
load_kern(void)
{
	struct Proghdr *ph, *eph;

	// read 1st page off disk
	readseg((ULONG) ELFHDR, SECTSIZE*8, 0);

	// is this a valid ELF?
	if (ELFHDR->e_magic != ELF_MAGIC)
		goto bad;

	// load each program segment (ignores ph flags)
	ph = (struct Proghdr *) ((UCHAR *) ELFHDR + ELFHDR->e_phoff);
	eph = ph + ELFHDR->e_phnum;

	for (; ph < eph; ph++) {
		readseg(ph->p_va, ph->p_memsz, ph->p_offset);
	}

	// call the entry point from the ELF header
	// note: does not return!
	((void (*)(void)) (ELFHDR->e_entry & 0xFFFFFF))();

bad:
	outw(0x8A00, 0x8A00);
	outw(0x8A00, 0x8E00);
	while (1)
		/* do nothing */;
}

// Read 'count' bytes at 'offset' from kernel into virtual address 'va'.
// Might copy more than asked
void
readseg(ULONG va, ULONG count, ULONG offset)
{
	ULONG end_va;

	va &= 0xFFFFFF;
	end_va = va + count;
	
	// round down to sector boundary
	va &= ~(SECTSIZE - 1);

	// translate from bytes to sectors, and kernel starts at sector 1
	offset = (offset / SECTSIZE) + 1;

	// If this is too slow, we could read lots of sectors at a time.
	// We'd write more to memory than asked, but it doesn't matter --
	// we load in increasing order.
	while (va < end_va) {
		readsect((UCHAR*) va, offset);
		va += SECTSIZE;
		offset++;
	}
}



/******************************************************************************

	WaitDisk()

To check the disk is ready or not read 0x1f7 and & it with 0xc0 (11000000)
Bcos 7th bit should be 0, and 6th bit should be 1, we don't care about the rest

7th bit ===> Drive is executing a command
6th bit ===> Drive is ready

 0 1 0 0 0 0 0 0
 1 1 0 0 0 0 0 0 &
-----------------
 0 1 0 0 0 0 0 0

*******************************************************************************/

void
waitdisk(void)
{
	// wait for disk reaady
	while ((inb(0x1F7) & 0xC0) != 0x40)
		/* do nothing */;
}

void
readsect(void *dst, ULONG offset)
{
	// wait for disk to be ready
	waitdisk();

	outb(0x1F2, 1);		// count = 1
	outb(0x1F3, offset);
	outb(0x1F4, offset >> 8);
	outb(0x1F5, offset >> 16);
	outb(0x1F6, (offset >> 24) | 0xE0);
	outb(0x1F7, 0x20);	// cmd 0x20 - read sectors

	// wait for disk to be ready
	waitdisk();

	// read a sector
//count = SECTSIZE/4 bcos ins internally ends up using insd which loads 4 bytes(one word)at a time
	insl(0x1F0, dst, SECTSIZE/4);
}

