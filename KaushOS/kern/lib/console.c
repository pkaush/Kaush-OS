/*
*  C Implementation: Console.c
*
* Description: This has a set of functions which will be used
*		to print on the console
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/



// Well these Locks will be used if we feel there is a need for them
// Till then keep the empty def
#include	<inc/stdarg.h>
#include	<inc/types.h>
#include	<inc/assert.h>

#include	<kern/hal/X86.h>


#define 	KiInitializeConsoleLock()
#define	KiAcquireConsole()
#define	KiReleaseConsole()

#ifdef DEBUG
static void
pport_putc(int c);
#endif




static void
PutcharWithoutLock (UCHAR ch) 
{
	Vga_putc(ch);

#ifdef DEBUG
	pport_putc(ch);
#endif
}

static void
VprintfWithoutLock (char ch, void *char_cnt_) 
{
	int *char_cnt = char_cnt_;
	(*char_cnt)++;
	PutcharWithoutLock (ch);
}



void
ConsoleInit (void) 
{
	KiInitializeConsoleLock();
}


/* The standard vprintf() function,
   which is like printf() but uses a va_list.
   Writes its output to both vga display and serial port. */
int
vprintf (const char *format, va_list args) 
{
	int char_cnt = 0;

	KiAcquireConsole();
	__vprintf (format, args, VprintfWithoutLock, &char_cnt);
	KiReleaseConsole();

	return char_cnt;
}

/* Writes string S to the console, followed by a new-line
   character. */
int
puts (const char *str) 
{
	KiAcquireConsole ();

	while (*str != '\0') {
		PutcharWithoutLock (*str++);
		PutcharWithoutLock ('\n');
	}

	KiReleaseConsole ();

	return 0;
}

/* Writes the N characters in BUFFER to the console. */
void
putbuf (const char *buffer, size_t n) 
{
	KiAcquireConsole();
		while (n-- > 0) {
			PutcharWithoutLock (*buffer++);
		}
		
	KiReleaseConsole();
}

/* Writes C to the vga display and serial port. */
int
putchar (int ch) 
{
	KiAcquireConsole ();

	PutcharWithoutLock (ch);

	KiReleaseConsole ();
  
  return ch;
}

/* Helper function for vprintf(). */



/***** Parallel port output code *****/
/*
		For debugging

*/

// Stupid I/O delay routine necessitated by historical PC design flaws
#ifdef DEBUG

static void
ppdelay(void)
{

	inb(0x84);
	inb(0x84);
	inb(0x84);
	inb(0x84);

}

static void
pport_putc(int c)
{


	int i;

	for (i = 0; !(inb(0x378+1) & 0x80) && i < 12800; i++)
		ppdelay();
	outb(0x378+0, c);
	outb(0x378+2, 0x08|0x04|0x01);
	outb(0x378+2, 0x08);

}

#endif


