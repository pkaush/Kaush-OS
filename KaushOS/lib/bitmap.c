/*
*  C Implementation: printf.c
*
* Description: Simple implementation of Bitmap
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/


#include <inc/bitmap.h>
#include <inc/assert.h>


/* Yields X divided by STEP, rounded up.
   For X >= 0, STEP >= 1 only. */
#define DIV_ROUND_UP(X, STEP) (((X) + (STEP) - 1) / (STEP))



// Number of bits in an element.
#define	ELEM_BITS (sizeof (ElementType) * 8)

/* 
	Returns the index of the element that contains the bit
	numbered BIT_IDX. 
*/
static INLINE size_t
BmElementIndex( IN size_t BitIndex) 
{
	return (BitIndex / ELEM_BITS);
}

/* 
	Returns an ElementType where only the bit corresponding to
	BitIndex is turned on.
*/
static INLINE ElementType
BmBitMask (IN size_t BitIndex) 
{
	return ((ElementType) 1 << (BitIndex % ELEM_BITS));
}

/* Returns the number of elements required for BIT_CNT bits. */
static inline size_t
BmElementCount (IN size_t BitCount)
{
	return DIV_ROUND_UP (BitCount, ELEM_BITS);
}

/* Returns the number of bytes required for BIT_CNT bits. */
static INLINE size_t
BmByteCount (IN size_t BitCount)
{
	return( sizeof (ElementType) * BmElementCount (BitCount));
}

/* 
	Returns a bit mask in which the bits actually used in the last
	element of B's bits are set to 1 and the rest are set to 0. 
*/
static INLINE ElementType
BmLastMask (const PBITMAP bitmap) 
{
	ULONG LastBits = (bitmap->BitCount % ELEM_BITS);
	return (LastBits ? ((ElementType) 1 << LastBits) - 1 : (ElementType) -1);
}

/* Creation and destruction. */


#if 0
/* Initializes B to be a bitmap of BIT_CNT bits
   and sets all of its bits to FALSE.
   Returns TRUE if success, FALSE if memory allocation
   failed. */
PBITMAP 
BmBitmapCreate (size_t BitCount) 
{
  PBITMAP b = malloc (sizeof *b);
  if (b != NULL)
    {
      b->BitCount = BitCount;
      b->bits = malloc (BmByteCount (BitCount));
      if (b->bits != NULL || BitCount == 0)
        {
          BmBitmapSetAll (b, FALSE);
          return b;
        }
      free (b);
    }
  return NULL;
}
#endif
/* 
	Creates and returns a bitmap with BIT_CNT bits in the
	BLOCK_SIZE bytes of storage preallocated at BLOCK.
	BLOCK_SIZE must be at least bitmap_needed_bytes(BIT_CNT). 
*/
PBITMAP 
BmBitmapCreateInBuffer (size_t BitCount, PVOID block, size_t block_size)
{
	PBITMAP bitmap = block;

	if (BitCount == 0) {
/*
	Hack to calculate bitcount.

	This will be used in case we pass the buffer and size 
	but not the number of bits. So we will figure out the 
	max number of bits we can support in the given buffer
	and set the bit count accordingly
*/
		ASSERT(block_size >  sizeof(BITMAP));

		size_t Size = block_size - sizeof(BITMAP);
		BitCount = Size * 8; //8 bits per byte :-)
	}
  
	ASSERT (block_size >= BmBitmapBufferSize (BitCount));

	bitmap->BitCount = BitCount;
	bitmap->bits = (ElementType *) (bitmap + 1);
	BmBitmapSetAll (bitmap, FALSE);
	return bitmap;
}

/*
	Returns the number of bytes required to accomodate a bitmap
	with BIT_CNT bits (for use with BmBitmapCreateInBuffer()).
*/
size_t
BmBitmapBufferSize (size_t BitCount) 
{
	return (sizeof (BITMAP) + BmByteCount (BitCount));
}


#if 0
/*
	Destroys bitmap B, freeing its storage.
	Not for use on bitmaps created by
	BmBitmapCreate_preallocated().
*/
void
BmBitmapDestroy (PBITMAP bitmap) 
{
	if (bitmap != NULL) {
		free (bitmap->bits);
		free (bitmap);
	}
}

#endif

/* Bitmap size. */

/* Returns the number of bits in B. */
size_t
BmBitmapSize (const PBITMAP bitmap)
{
	return (bitmap->BitCount);
}

/* Setting and testing single bits. */

/* Atomically sets the bit numbered IDX in B to VALUE. */
void
BmBitmapSet (PBITMAP bitmap, size_t idx, BOOLEAN value) 
{
	ASSERT (bitmap != NULL);
	ASSERT (idx < bitmap->BitCount);
	if (value) {
		BmBitmapMark (bitmap, idx);
	}  else {
		BmBitmapReset (bitmap, idx);
	}
}

/* Atomically sets the bit numbered BIT_IDX in Bitmap to TRUE. */
void
BmBitmapMark (PBITMAP bitmap, size_t BitIndex) 
{
	size_t idx = BmElementIndex (BitIndex);
	ElementType mask = BmBitMask (BitIndex);

/*
	This is equivalent to `bitmap->bits[idx] |= mask' except that it
	is guaranteed to be atomic on a uniprocessor machine.  See
	the description of the OR instruction in [IA32-v2b].
*/
	asm ("orl %1, %0" : "=m" (bitmap->bits[idx]) : "r" (mask) : "cc");
}

/* Atomically sets the bit numbered BIT_IDX in B to FALSE. */
void
BmBitmapReset (PBITMAP bitmap, size_t BitIndex) 
{
	size_t idx = BmElementIndex (BitIndex);
	ElementType mask = BmBitMask (BitIndex);

/*
	This is equivalent to `b->bits[idx] &= ~mask' except that it
	is guaranteed to be atomic on a uniprocessor machine.  See
	the description of the AND instruction in [IA32-v2a].
*/
	asm ("andl %1, %0" : "=m" (bitmap->bits[idx]) : "r" (~mask) : "cc");
}

/* 
	Atomically toggles the bit numbered IDX in B;
	that is, if it is TRUE, makes it FALSE,
	and if it is FALSE, makes it TRUE.
*/
void
BmBitmapFlip (PBITMAP bitmap, size_t BitIndex) 
{
  size_t idx = BmElementIndex (BitIndex);
  ElementType mask = BmBitMask (BitIndex);

/* 
	This is equivalent to `b->bits[idx] ^= mask' except that it
	is guaranteed to be atomic on a uniprocessor machine.  See
	the description of the XOR instruction in [IA32-v2b].
*/
	asm ("xorl %1, %0" : "=m" (bitmap->bits[idx]) : "r" (mask) : "cc");
}

/* Returns the value of the bit numbered IDX in Bitmap. */
BOOLEAN
BmBitmapTest (const PBITMAP bitmap, size_t idx) 
{
	ASSERT(bitmap != NULL);
	ASSERT(idx < bitmap->BitCount);
	return ((bitmap->bits[BmElementIndex (idx)] & BmBitMask (idx)) != 0);
}

/* Setting and testing multiple bits. */

/* Sets all bits in B to VALUE. */
void
BmBitmapSetAll (PBITMAP bitmap, BOOLEAN value) 
{
	ASSERT(bitmap != NULL);
	BmBitmapSetMultiple (bitmap, 0, BmBitmapSize (bitmap), value);
}

/* Sets the CNT bits starting at START in B to VALUE. */
void
BmBitmapSetMultiple (PBITMAP bitmap, size_t start, size_t cnt, BOOLEAN value) 
{
	size_t i;
  
	ASSERT(bitmap != NULL);
	ASSERT(start <= bitmap->BitCount);
	ASSERT(start + cnt <= bitmap->BitCount);

	for (i = 0; i < cnt; i++) {
		BmBitmapSet (bitmap, start + i, value);
	}
}

/* 
	Returns the number of bits in B between START and START + CNT,
	exclusive, that are set to VALUE. 
*/

size_t
BmBitmapCount (const PBITMAP bitmap, size_t start, size_t cnt, BOOLEAN value) 
{
	size_t i, value_cnt;

	ASSERT(bitmap != NULL);
	ASSERT(start <= bitmap->BitCount);
	ASSERT(start + cnt <= bitmap->BitCount);

	value_cnt = 0;
	
	for (i = 0; i < cnt; i++) {
		if (BmBitmapTest (bitmap, start + i) == value) {
			value_cnt++;
		}
	}
	
	return value_cnt;
}

/* Returns TRUE if any bits in B between START and START + CNT,
   exclusive, are set to VALUE, and FALSE otherwise. */
BOOLEAN
BmBitmapContains (const PBITMAP bitmap, size_t start, size_t cnt, BOOLEAN value) 
{
	size_t i;
  
	ASSERT (bitmap != NULL);
	ASSERT (start <= bitmap->BitCount);
	ASSERT (start + cnt <= bitmap->BitCount);

	for (i = 0; i < cnt; i++) {
		if (BmBitmapTest (bitmap, start + i) == value) {
			return TRUE;
		}
	}

	return FALSE;
}

/* 
	Returns TRUE if any bits in B between START and START + CNT,
	exclusive, are set to TRUE, and FALSE otherwise.
*/

BOOLEAN
BmBitmapAny (const PBITMAP bitmap, size_t start, size_t cnt) 
{
	return (BmBitmapContains (bitmap, start, cnt, TRUE));
}

/* 
	Returns TRUE if no bits in B between START and START + CNT,
	exclusive, are set to TRUE, and FALSE otherwise.
*/
BOOLEAN
BmBitmapNone (const PBITMAP b, size_t start, size_t cnt) 
{
	return (!BmBitmapContains (b, start, cnt, TRUE));
}

/* 
	Returns TRUE if every bit in B between START and START + CNT,
	exclusive, is set to TRUE, and FALSE otherwise. 
*/
BOOLEAN
BmBitmapAll (const PBITMAP b, size_t start, size_t cnt) 
{
  return (!BmBitmapContains (b, start, cnt, FALSE));
}

/* Finding set or unset bits. */

/* 
	Finds and returns the starting index of the first group of CNT
	consecutive bits in B at or after START that are all set to
	VALUE.
	If there is no such group, returns BITMAP_ERROR. 
*/
size_t
BmBitmapScan (const PBITMAP bitmap, 
							size_t start, 
							size_t cnt, 
							BOOLEAN value) 
{
	ASSERT (bitmap != NULL);
	ASSERT (start <= bitmap->BitCount);

	if (cnt <= bitmap->BitCount) {
		size_t last = bitmap->BitCount - cnt;
		size_t i;
		for (i = start; i <= last; i++) {
			if (!BmBitmapContains (bitmap, i, cnt, !value)) {
				return i; 
			}
		}
	}
	return BITMAP_ERROR;
}

/* 
	Finds the first group of CNT consecutive bits in B at or after
	START that are all set to VALUE, flips them all to !VALUE,
	and returns the index of the first bit in the group.
	If there is no such group, returns BITMAP_ERROR.
	If CNT is zero, returns 0.
	Bits are set atomically, but testing bits is not atomic with
	setting them. 
*/
size_t
BmBitmapScanAndFlip (PBITMAP bitmap, size_t start, size_t cnt, BOOLEAN value)
{
	size_t idx = BmBitmapScan (bitmap, start, cnt, value);

	if (idx != BITMAP_ERROR) {
		BmBitmapSetMultiple (bitmap, idx, cnt, !value);
	}

	return idx;
}

