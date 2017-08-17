/*
*  C Implementation: vga.c
*
* Description: Implemetation of Vga display driver 
*				"http://www.osdever.net/FreeVGA/home.htm"
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/


#include <inc/stdio.h>
#include <inc/types.h>
#include <kern/hal/X86.h>



#define COL_CNT 80
#define ROW_CNT 25

static size_t XPos, YPos;

#define GRAY_ON_BLACK 0x07


static UCHAR (*VgaBuffer)[COL_CNT][2];

static void ClearRow (size_t y);
static void Clear (void);
static void Newline (void);
static void MoveCursor (void);
static void FindCursor (size_t *x, size_t *y);

/* Initializes the VGA text display. */
void
VgaInit(void)
{
  /* Already initialized? */
  static BOOLEAN Initilized = 0;


//We do not want to initialize it multiple times
ASSERT(Initilized == 0);
  
//Get the Virtual address for the VGA Buffer for physical address.
// This didn't work out as we are initilizing this even before MM
//		VgaBuffer = MmGetVirtualForPhysical(0xb8000);
// We know that this is our virtual address for VGA buffer
// so use it for now. We should expose some APIs in MM to do this.
		VgaBuffer = 0x800b8000;				
		FindCursor( &XPos, &YPos);
		Initilized = 1; 
}

void
Vga_putc (int ch)
{
/*
	I guess we should run on some elevated IRQL here

	Will do it later as per the need.
*/
	switch (ch) {


		case '\n':
			Newline ();
		break;

		case '\f':
			Clear ();
		break;

		case '\b':
			if (XPos > 0) {
				XPos--;
			}
		break;
      
		case '\r':
			XPos = 0;
		break;

		case '\t':
			XPos = ROUNDUP (XPos + 1, 8);
			if (XPos >= COL_CNT) {
				Newline ();
			}
		break;
      
		default:
			VgaBuffer[YPos][XPos][0] = ch;
			VgaBuffer[YPos][XPos][1] = GRAY_ON_BLACK;
			if (++XPos >= COL_CNT) {
				Newline ();
			}
		break;
    }

  /* Update cursor position. */
	MoveCursor ();

}

/* Clears the screen and moves the cursor to the upper left. */
static void
Clear (void)
{
	size_t y;

	for (y = 0; y < ROW_CNT; y++) {
		ClearRow (y);
	}
	
	XPos = YPos = 0;
	MoveCursor ();
}

/* Clears row Y to spaces. */
static void
ClearRow (size_t y) 
{
	size_t x;

	for (x = 0; x < COL_CNT; x++) {
		VgaBuffer[y][x][0] = ' ';
		VgaBuffer[y][x][1] = GRAY_ON_BLACK;
	}
}

/* 
	Advances the cursor to the first column in the next line on
	the screen.  If the cursor is already on the last line on the
	screen, scrolls the screen upward one line.
*/
static void
Newline (void)
{
	XPos = 0;
	YPos++;

	if (YPos >= ROW_CNT) {
		YPos = ROW_CNT - 1;
		memmove (&VgaBuffer[0], &VgaBuffer[1], sizeof VgaBuffer[0] * (ROW_CNT - 1));
		ClearRow (ROW_CNT - 1);
	}
}

/* Moves the hardware cursor to (XPos,YPos). */
static void
MoveCursor (void) 
{
  /* See [FREEVGA] under "Manipulating the Text-mode Cursor". */
	USHORT cp = XPos + COL_CNT * YPos;
	outw (0x3d4, 0x0e | (cp & 0xff00));
	outw (0x3d4, 0x0f | (cp << 8));
}

/* Reads the current hardware cursor position into (*X,*Y). */
static void
FindCursor (size_t *x, size_t *y) 
{
	/* See [FREEVGA] under "Manipulating the Text-mode Cursor". */
	USHORT cp;

	outb (0x3d4, 0x0e);
	cp = inb (0x3d5) << 8;

	outb (0x3d4, 0x0f);
	cp |= inb (0x3d5);

	*x = cp % COL_CNT;
	*y = cp / COL_CNT;
}







