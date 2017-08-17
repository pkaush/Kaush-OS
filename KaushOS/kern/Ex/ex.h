/*
*  C Interface:	Ex.h
*
* Description:
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/


#ifndef	__KOS_EX_EX_H__
#define	__KOS_EX_EX_H__


#include	<inc/types.h>
#include	<kern/ke/ke.h>
#include	<inc/error.h>
#include	<inc/file.h>



#define EX_MAX_HANDLE	1024 // entries for one page


typedef struct _HANDLE_TABLE_ENTRY
{
	PVOID Object;
} HANDLE_TABLE_ENTRY, *PHANDLE_TABLE_ENTRY;



typedef struct _HANDLE_TABLE
{
	PHANDLE_TABLE_ENTRY Handle;

//I am not yet going into PushLocks
	KMUTANT	HandleLock;

} HANDLE_TABLE, *PHANDLE_TABLE;











typedef struct _ERESOURCE {

// I don't think this will work

	ULONG	Shared;
	ULONG	Exclusive;
	ULONG	ExclusiveWaiting;
	KSPIN_LOCK	Spinlock;
}ERESOURCE, *PERESOURCE;


KSTATUS
ExInitializeResourceLite(
	IN PERESOURCE  Resource
	);

BOOLEAN 
ExAcquireResourceExclusiveLite(
	IN PERESOURCE  Resource,
	IN BOOLEAN  Wait
	);

VOID 
ExReleaseResourceLite(
	IN PERESOURCE  Resource
	);



#define ExAllocatePoolWithTag(P,N,T) MmAllocatePoolWithTag(P,N,T)

#define ExFreePoolWithTag(P,T) MmFreePoolWithTag(P,T) 

#define ExFreePool(P) MmFreePoolWithTag(P,0) 




/***************************************
				LOOKASIDE LIST
***************************************/

#if 0
//enum PP_NPAGED_LOOKASIDE_NUMBER

typedef enum _PP_NPAGED_LOOKASIDE_NUMBER
{
	LookasideSmallIrpList = 0,
	LookasideLargeIrpList = 1,
	LookasideMdlList = 2,
	LookasideCreateInfoList = 3,
	LookasideNameBufferList = 4,
	LookasideTwilightList = 5,
	LookasideCompletionList = 6,
	LookasideScratchBufferList = 7,
	LookasideMaximumList = 8
} PP_NPAGED_LOOKASIDE_NUMBER;



VOID
KAPI
ExInitializeSystemLookasideList(IN PGENERAL_LOOKASIDE List,
                                IN POOL_TYPE Type,
                                IN ULONG Size,
                                IN ULONG Tag,
                                IN USHORT MaximumDepth,
                                IN PLIST_ENTRY ListHead);





PVOID
ExAllocateFromPPLookasideList(
					PP_NPAGED_LOOKASIDE_NUMBER LookAsideListIndex
					);



typedef
PVOID
ALLOCATE_FUNCTION (
	IN POOL_TYPE PoolType,
	IN SIZE_T  NumberOfBytes,
	IN ULONG  Tag
	);


typedef ALLOCATE_FUNCTION *PALLOCATE_FUNCTION;


typedef
VOID
FREE_FUNCTION(
	PVOID  Buffer
	);

typedef FREE_FUNCTION *PFREE_FUNCTION;




typedef struct _GENERAL_LOOKASIDE
{
	union
	{
		SLIST_HEADER ListHead;
		SINGLE_LIST_ENTRY SingleListHead;
	};
	WORD Depth;
	WORD MaximumDepth;
	ULONG TotalAllocates;
	union
	{
		ULONG AllocateMisses;
		ULONG AllocateHits;
	};
	ULONG TotalFrees;
	union
	{
		ULONG FreeMisses;
		ULONG FreeHits;
	};
	POOL_TYPE Type;
	ULONG Tag;
	ULONG Size;
	union
	{
		PVOID * AllocateEx;
		PVOID * Allocate;
	};
	union
	{
		PVOID FreeEx;
		PVOID Free;
	};
	LIST_ENTRY ListEntry;
	ULONG LastTotalAllocates;
	union
	{
		ULONG LastAllocateMisses;
		ULONG LastAllocateHits;
	};
	ULONG Future[2];
} GENERAL_LOOKASIDE,
	*PGENERAL_LOOKASIDE,
	GENERAL_LOOKASIDE_POOL, 
	*PGENERAL_LOOKASIDE_POOL;






typedef struct _PAGED_LOOKASIDE_LIST{
	GENERAL_LOOKASIDE L;
	ULONG Lock__ObsoleteButDoNotDelete;
}	PAGED_LOOKASIDE_LIST, 
	*PPAGED_LOOKASIDE_LIST,
	NPAGED_LOOKASIDE_LIST, 
	*PNPAGED_LOOKASIDE_LIST;



//
//Non-Paged Lookaside List
//


VOID 
ExInitializeNPagedLookasideList(
			IN PNPAGED_LOOKASIDE_LIST  Lookaside,
			IN PALLOCATE_FUNCTION  Allocate  OPTIONAL,
			IN PFREE_FUNCTION  Free  OPTIONAL,
			IN ULONG  Flags,
			IN SIZE_T  Size,
			IN ULONG  Tag,
			IN USHORT  Depth
			);

PVOID 
ExAllocateFromNPagedLookasideList(
			IN PNPAGED_LOOKASIDE_LIST  Lookaside
			);


VOID 
ExFreeToNPagedLookasideList(
			IN PNPAGED_LOOKASIDE_LIST  Lookaside,
			IN PVOID  Entry
			);




//
//Paged Lookaside List
//



VOID 
ExInitializePagedLookasideList(
			IN PPAGED_LOOKASIDE_LIST  Lookaside,
			IN PALLOCATE_FUNCTION  Allocate  OPTIONAL,
			IN PFREE_FUNCTION  Free  OPTIONAL,
			IN ULONG  Flags,
			IN SIZE_T  Size,
			IN ULONG  Tag,
			IN USHORT  Depth
			);


PVOID 
ExAllocateFromPagedLookasideList(
			IN PPAGED_LOOKASIDE_LIST  Lookaside
			);


VOID 
ExFreeToPagedLookasideList(
			IN PPAGED_LOOKASIDE_LIST  Lookaside,
			IN PVOID  Entry
			);

#endif
#endif
