/*
*  C Interface:	IRQL.h
*
* Description:
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/

#ifndef	__KOS_LIB_BITMAP_H__
#define	__KOS_LIB_BITMAP_H__


#include	<inc/types.h>


/* Bitmap abstract data type. */

typedef ULONG ElementType;

/* 
	From the outside, a bitmap is an array of bits.  From the
	inside, it's an array of ElementType (defined above) that
	simulates an array of bits.
*/
typedef struct _BITMAP {
	size_t BitCount;     /* Number of bits. */
	ElementType *bits;    /* Elements that represent bits. */
} BITMAP, *PBITMAP;



/* Creation and destruction. */
PBITMAP
BmBitmapCreateInBuffer(size_t BitCount,PVOID block,size_t block_size);

size_t
BmBitmapBufferSize(size_t BitCount);

void
BmBitmapDestroy(PBITMAP bitmap);


/* Bitmap size. */

size_t
BmBitmapSize(const PBITMAP bitmap);


/* Setting and testing single bits. */

VOID
BmBitmapSet(PBITMAP bitmap,size_t idx,BOOLEAN value);

VOID
BmBitmapMark(PBITMAP bitmap,size_t BitIndex);

VOID
BmBitmapReset(PBITMAP bitmap,size_t BitIndex);

VOID
BmBitmapReset(PBITMAP bitmap,size_t BitIndex);


VOID
BmBitmapFlip(PBITMAP bitmap,size_t BitIndex);

BOOL
BmBitmapTest(const PBITMAP bitmap,size_t idx);


/* Setting and testing multiple bits. */
VOID
BmBitmapSetAll(PBITMAP bitmap,BOOLEAN value);

VOID
BmBitmapSetMultiple(PBITMAP bitmap,size_t start,size_t cnt,BOOLEAN value);

size_t
BmBitmapCount(const PBITMAP bitmap,size_t start,size_t cnt,BOOLEAN value);

BOOL
BmBitmapContains(const PBITMAP bitmap,size_t start,size_t cnt,BOOLEAN value);

BOOL
BmBitmapAny(const PBITMAP bitmap,size_t start,size_t cnt);

BOOL
BmBitmapAll(const PBITMAP bitmap,size_t start,size_t cnt);

BOOL
BmBitmapNone(const PBITMAP bitmap,size_t start,size_t cnt);

BOOL
BmBitmapAny(const PBITMAP bitmap,size_t start,size_t cnt);



/* Finding set or unset bits. */
#define BITMAP_ERROR	-100 // some rendom error no, will correct it later

size_t
BmBitmapScan(const PBITMAP bitmap,size_t start,size_t cnt,BOOLEAN value);

size_t
BmBitmapScanAndFlip(PBITMAP bitmap,size_t start,size_t cnt,BOOLEAN value);




#endif

