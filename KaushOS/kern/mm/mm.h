/*
*  C Interface: mm.h
*
* Description: Memory Manager Interface
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/


#ifndef KOS_KERN_MM_H
#define KOS_KERN_MM_H

#include <kern/ke/ke.h>
#include <kern/hal/x86.h>
#include <inc/assert.h>
#include <inc/error.h>
#include <inc/types.h>


#define IsKernelAddress(x)	((ULONG_PTR)(x) > KERNBASE)
#define IsUserAddress(x)	((ULONG_PTR)(x) < USERTOP)




VOID
MiBootInitMemoryLayout();


KSTATUS
MmVirtualAlloc(IN PVOID VirtualAddress,
						IN size_t size,
						IN ULONG perm);

void
MmVirtualFree(
						IN PVOID VirtualAddress,
						IN size_t size
						);

PHYSICAL_ADDRESS
MmGetPhysicalAddress(
						IN PVOID Address
						);

KSTATUS
MmMapSegment(
						IN PVOID PageDirectory,
						IN PVOID Address,
						IN size_t Length,
						IN ULONG Perm
						);

KSTATUS
MmInitializeUserPtes(
			OUT PVOID * ProcessBitmap
			);

PVOID
MmAllocateSystemPtes(
			IN ULONG NumberOfPages
			);

PVOID
MmAllocateUserPtes(IN PVOID Bitmap,IN ULONG NumberOfPages);



//Pool Related prototypes

typedef enum _POOL_TYPE {
	FirstPool	 		=	0x0,
	PagedPool		=	FirstPool,
	NonPagedPool,
	LastPool

} POOL_TYPE;


PVOID
MmAllocatePoolWithTag(IN POOL_TYPE pooltype,IN size_t NumberOfBytes,IN ULONG Tag);

VOID
MmFreePoolWithTag(IN PVOID P,IN ULONG Tag);






#endif //End of header

