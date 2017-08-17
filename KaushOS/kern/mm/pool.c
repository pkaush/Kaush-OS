/*
*  C Implementation:  Pool.c
*
* Description: Implementation of Kernel Pool
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/



#include	"kern/mm/mm.h"




//TAG types

#define NULLTAG			0x0

// Pool Block Status 
#define POOL_STATUS_FREE			0x00
#define POOL_STATUS_ALLOCATED	0x01


#define MINIMUMFREEPOOLSIZE	512

// put some pad at the end of every pool
// to reduce the chances of overwriting next pool header.
#define POOLPAD	56	

static KSPIN_LOCK PoolLock;


typedef struct	_POOLHEADER {
	POOL_TYPE PoolType;	// we are going to use PoolType as Magic for this
	size_t	size;
	KLIST_ENTRY(_POOLHEADER)	link;
	ULONG Tag;
	ULONG	status;
	char mem[1];

} POOLHEADER, *PPOOLHEADER;


typedef LIST_HEAD(_FREEPOOLLIST, _POOLHEADER) FREEPOOLLIST, *PFREEPOOLLIST;

#define NUMBEROFPOOLS	(LastPool - FirstPool)

FREEPOOLLIST KernelPools[NUMBEROFPOOLS+2];


KSTATUS
MmInitializePool (IN PVOID va,
							IN size_t size,
							IN POOL_TYPE pooltype);


void
Test_Pool(POOL_TYPE pooltype);


VOID
MmInitializeAllPools()
{
	int i = 0;

	for (i = 0; i < NUMBEROFPOOLS; i++) {
		memset (&KernelPools[i], 0, sizeof(struct _FREEPOOLLIST));
	}

	// initialize Paged Pool
	MmInitializePool((PVOID)MMPAGEPOOL_START, 
						MMPAGEPOOL_SIZE, PagedPool);

	// Initialize Non Paged Pool
	MmInitializePool((PVOID)MMNONPAGEPOOL_START, 
						MMNONPAGEPOOL_SIZE, NonPagedPool);	
#ifdef DEBUG
	Test_Pool(PagedPool);
	Test_Pool(NonPagedPool); 
	// actually we do not nee to test both as the implementation is same
#endif


// Right now we have any paged pool, soon we will also be initailizing non paged pool here.
// But before we do that we need to start the swapping out pages 
//because right now everything is non paged

}



KSTATUS
MmInitializePool (IN PVOID va,
							IN size_t size,
							IN POOL_TYPE pooltype)

{
	KSTATUS status;
	PPOOLHEADER pPoolHeader = NULL;
	PFREEPOOLLIST pFreePoolList = &KernelPools[pooltype];
	LIST_INIT(pFreePoolList);

	status = MmVirtualAlloc(va, size,KPOOL_PERM);
	if (!success(status)) {
		KeBugCheck("Pool Alocation Failed Pooltype %d VA 0x%x size %d\n", pooltype, va, size);
	}

	pPoolHeader = (PPOOLHEADER) va;
	pPoolHeader->Tag = NULLTAG;
	LIST_INIT_ENTRY(pPoolHeader,link);
	pPoolHeader->PoolType = pooltype;
	pPoolHeader->size = size - sizeof(POOLHEADER); // header will also be stored in the same space
	pPoolHeader->mem[0] = 0; 	// NULL terminating the free mem.
	pPoolHeader->status = POOL_STATUS_FREE;

	LIST_INSERT_HEAD(pFreePoolList,pPoolHeader,link);
	return STATUS_SUCCESS;
}


PCHAR
GetPoolTagString(ULONG Tag) {

	return NULL;
/*	
	static char buf[8];
	PCHAR tmp = (PCHAR) Tag;
	snprintf(buf,sizeof(ULONG),"%c%c%c%c\0",tmp[0],tmp[1],tmp[2],tmp[3]);
	return buf;
*/	
}


PVOID
MmAllocatePoolWithTag (
					IN POOL_TYPE pooltype,
					IN size_t NumberOfBytes,
					IN ULONG Tag
					)
{

	PFREEPOOLLIST pFreePoolList = &KernelPools[pooltype];
	PPOOLHEADER pPoolBlock = NULL;
	KIRQL OldIrql;

#ifdef DEBUG
{
	KIRQL CurrentIrql;
	CurrentIrql = KeGetCurrentIrql();
	ASSERT1(CurrentIrql <= DISPATCH_LEVEL, CurrentIrql);

	if(CurrentIrql == DISPATCH_LEVEL) {
		ASSERT1(pooltype == NonPagedPool, pooltype);
	}

}
#endif	



	// put some padding at the end of the pool
	NumberOfBytes += POOLPAD; 

	// Round up to 4 byte boundry
	NumberOfBytes = ROUNDUP(NumberOfBytes , 4);

	
	// If there is no element in the list that means it is not yet initailized
	// return NULL if thats the case or Panic.
	if (LIST_EMPTY(pFreePoolList)) {
		KeBugCheck("allocating pool before init pooltype %d Tag %s\n", pooltype, GetPoolTagString(Tag));
		return NULL; // if we decide to remove KeBugCheck some day
	}

	// Acquire the write lock on the list as we might be changing few stuff on it
	KeAcquireSpinLock( &PoolLock, &OldIrql);

	// loop on the all the elements of the list and
	// find an empty pool block that suites out requirement.
	LIST_FOREACH(pPoolBlock, pFreePoolList, link) {
			if (!(pPoolBlock->status & POOL_STATUS_ALLOCATED) 
					&& (pPoolBlock->size > NumberOfBytes)) {
				break;
			}
	}
				
	// if we didn't find anything big enough then return NULL
	if (pPoolBlock == NULL) {
		KeReleaseSpinLock( &PoolLock, OldIrql);
		return NULL;
	}
	
	// Panic If we are trying to allocate an already allocated pool
	// if we continue this will corrupt pool.
	if  (pPoolBlock->status & POOL_STATUS_ALLOCATED) {
		KeReleaseSpinLock( &PoolLock, OldIrql);
		KeBugCheck("Free pool block already allocated pool status 0x%x, pooltype 0x%x ", 
					pPoolBlock->status, pPoolBlock->PoolType);
	}

	
// lets not divide memory if the next block will be smaller than Minimum
	if (NumberOfBytes < pPoolBlock->size - MINIMUMFREEPOOLSIZE)  {
		//create a new poolheader after this pool memory
		PPOOLHEADER pNewPoolBlock = (PPOOLHEADER) 
								((ULONG_PTR)pPoolBlock + sizeof(POOLHEADER) 
								+ NumberOfBytes);


		// Initialize the new block and mark it free
		pNewPoolBlock->Tag = NULLTAG;
		LIST_INIT_ENTRY(pNewPoolBlock,link);
		pNewPoolBlock->PoolType = pooltype;
		pNewPoolBlock->size =  // header will also be stored in the same space
			pPoolBlock->size - sizeof(POOLHEADER) - NumberOfBytes; 
		
		pNewPoolBlock->mem[0] = 0;	// NULL terminating the free mem.
		pNewPoolBlock->status = POOL_STATUS_FREE;
		LIST_INSERT_AFTER(pFreePoolList, pPoolBlock,pNewPoolBlock,link);

		//readject the old size
		pPoolBlock->size = NumberOfBytes;
	}
	pPoolBlock->Tag = Tag;
	pPoolBlock->status |= POOL_STATUS_ALLOCATED;
	KeReleaseSpinLock( &PoolLock, OldIrql);

	return (&pPoolBlock->mem[0]);
}


VOID
MmFreePoolWithTag(
				IN PVOID  P,
				IN ULONG  Tag 
				)
{

	PPOOLHEADER pPoolBlock = CONTAINING_RECORD(P, POOLHEADER, mem);
	PPOOLHEADER pPrevPoolBlock = NULL, pNextPoolBlock = NULL;
	POOL_TYPE pooltype = pPoolBlock->PoolType;
	PFREEPOOLLIST pFreePoolList = NULL;
	KIRQL OldIrql;

	
	//Lets do some verification, if anything fails lets KeBugCheck
	if ( !(pPoolBlock->status & POOL_STATUS_ALLOCATED) ||
		(pooltype >= NUMBEROFPOOLS) ||
		Tag != 0?(pPoolBlock->Tag != Tag):0) {

		KeBugCheck(" Verification Failed Status 0x%x pooltype 0x%x oldtag %s newtag %s\n",
					pPoolBlock->status, pooltype, 
					GetPoolTagString(pPoolBlock->Tag), GetPoolTagString(Tag));
	}

	pFreePoolList = &KernelPools[pooltype];

	//Free the current Block
	pPoolBlock->Tag = NULLTAG;	
	pPoolBlock->mem[0] = 0; // NULL terminating the free mem.
	pPoolBlock->status = POOL_STATUS_FREE;

	KeAcquireSpinLock( &PoolLock, &OldIrql);


	// check the previous and the next block.
	// if they are also free then merge the blocks.

	pNextPoolBlock = LIST_NEXT(pFreePoolList, pPoolBlock,link);
	if ((pNextPoolBlock)  && !(pNextPoolBlock->status & POOL_STATUS_ALLOCATED)) {
		// merge both the pool blocks
		pPoolBlock->size += pNextPoolBlock->size + sizeof(POOLHEADER);
		LIST_REMOVE(pFreePoolList, pNextPoolBlock,link);
	}

	pPrevPoolBlock = LIST_PREV(pFreePoolList, pPoolBlock,link);
	if ((pPrevPoolBlock)  && !(pPrevPoolBlock->status & POOL_STATUS_ALLOCATED)) {
		// merge both the pool blocks
		pPrevPoolBlock->size += pPoolBlock->size + sizeof(POOLHEADER);
		LIST_REMOVE(pFreePoolList, pPoolBlock,link);
	}

	KeReleaseSpinLock( &PoolLock, OldIrql);

}


void
Test_Pool(POOL_TYPE pooltype)
{
	PVOID	ptr1, ptr2, ptr3;
	
	PFREEPOOLLIST pFreePoolList = NULL;
	pFreePoolList = &KernelPools[pooltype];
	PPOOLHEADER pPoolBlock = NULL;
	size_t	NumberOfBytes = 0;
	size_t size = 0;
	NumberOfBytes = 12;
	// allocate 12 bytes
	pPoolBlock = LIST_FIRST(pFreePoolList);
	size = pPoolBlock->size;
	ptr1 = MmAllocatePoolWithTag(pooltype, NumberOfBytes, 'test');
	ASSERT(ptr1); // check if the memory is indeed allocated.

//check if the pool header is set right
	ASSERT(&pPoolBlock->mem[0] == ptr1);
	ASSERT(pPoolBlock->PoolType == pooltype);
	ASSERT(pPoolBlock->size == (NumberOfBytes + POOLPAD));
	ASSERT(pPoolBlock->status & POOL_STATUS_ALLOCATED);

	pPoolBlock = LIST_NEXT( pFreePoolList, pPoolBlock,link);

	// pPoolBlock should not be NULL, when first block is allocated, it should have created the next.
	ASSERT(pPoolBlock);
	ASSERT(pPoolBlock->PoolType == pooltype);
	ASSERT(pPoolBlock->size == (size - (NumberOfBytes + POOLPAD) - sizeof(POOLHEADER)));
	ASSERT((pPoolBlock->status & POOL_STATUS_ALLOCATED) == 0);

	//second mem allocation
	ptr2 = MmAllocatePoolWithTag(pooltype, NumberOfBytes, 'test');
	ASSERT(ptr2); // check if the memory is indeed allocated.

	// the same (second) pool block should have been used to allocate this request.
	//check if the pool header is set right
	ASSERT(&pPoolBlock->mem[0] == ptr2);
	ASSERT(pPoolBlock->PoolType == pooltype);
	ASSERT(pPoolBlock->size == (NumberOfBytes + POOLPAD));
	ASSERT(pPoolBlock->status & POOL_STATUS_ALLOCATED);

	// verify: next should be free	
	pPoolBlock = LIST_NEXT( pFreePoolList, pPoolBlock,link);

	// pPoolBlock should not be NULL, when first block is allocated, it should have created the next.
	ASSERT(pPoolBlock);
	ASSERT(pPoolBlock->PoolType == pooltype);
//	ASSERT(pPoolBlock->size == size - (NumberOfBytes + POOLPAD));
	ASSERT(pPoolBlock->size == (size - ((NumberOfBytes + POOLPAD) + sizeof(POOLHEADER)) * 2));
	ASSERT((pPoolBlock->status & POOL_STATUS_ALLOCATED) == 0);

	// third mem allocation
	ptr3 = MmAllocatePoolWithTag(pooltype, NumberOfBytes, 'test');
	ASSERT(ptr3); // check if the memory is indeed allocated.

	// the same (third) pool block should have been used to allocate this request.
	//check if the pool header is set right
	ASSERT(&pPoolBlock->mem[0] == ptr3);
	ASSERT(pPoolBlock->PoolType == pooltype);
	ASSERT(pPoolBlock->size == (NumberOfBytes + POOLPAD));
	ASSERT(pPoolBlock->status & POOL_STATUS_ALLOCATED);

	// verify: next should be free	
	pPoolBlock = LIST_NEXT( pFreePoolList, pPoolBlock,link);

	// pPoolBlock should not be NULL, when first block is allocated, it should have created the next.
	ASSERT(pPoolBlock);
	ASSERT(pPoolBlock->PoolType == pooltype);
//	ASSERT(pPoolBlock->size == size - (NumberOfBytes + POOLPAD));
	ASSERT(pPoolBlock->size == (size - ((NumberOfBytes + POOLPAD) + sizeof(POOLHEADER)) * 3));
	ASSERT((pPoolBlock->status & POOL_STATUS_ALLOCATED) == 0);

	// start freeing
	MmFreePoolWithTag(ptr1,'test');
	//the first block should be marked free!!
	pPoolBlock = LIST_FIRST(pFreePoolList);
	ASSERT((pPoolBlock->status & POOL_STATUS_ALLOCATED) == 0);

	MmFreePoolWithTag(ptr3,'test');
	//go to third block by calling LIST_NEXT twice.
	pPoolBlock = LIST_NEXT(pFreePoolList, pPoolBlock,link);
	pPoolBlock = LIST_NEXT(pFreePoolList, pPoolBlock,link);
	// this should be free now
	ASSERT((pPoolBlock->status & POOL_STATUS_ALLOCATED) == 0);

	MmFreePoolWithTag(ptr2,'test');
	// now all the memory should be free
	pPoolBlock = LIST_FIRST(pFreePoolList);
	ASSERT(pPoolBlock);
	ASSERT(pPoolBlock->PoolType == pooltype);
	ASSERT(pPoolBlock->size == size);
	ASSERT((pPoolBlock->status & POOL_STATUS_ALLOCATED) == 0);

	// there should be no next element now
	ASSERT(NULL == LIST_NEXT(pFreePoolList, pPoolBlock,link));

	kprintf("PoolTest passed!!\n");
}

