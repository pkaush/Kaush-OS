/*
*  C Implementation: mm.c
*
* Description: This modules initializes the Virtual and Physical mappings and provides the APIs to work on memory.
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/



#include "mm.h"


typedef struct _MMPTE_HARDWARE {
	ULONG Valid:1;
	ULONG Writable:1;
	ULONG UserPriv:1;
	ULONG WriteThrough:1;
	ULONG CacheDisabled:1;
	ULONG Accessed:1;
	ULONG Dirty:1;	 		//Reseved for PDE
	ULONG SuperPage:1;	// Pagetable attribute Index
	ULONG Global:1;
	ULONG Reserved:2;
	ULONG CopyOnWrite:1;			//Copy on Write
	ULONG PageFrameNumber:20;
} MMPTE_HARDWARE, *PMMPTE_HARDWARE;

typedef struct _MMPTE {
    union {
        ULONG Long;
        MMPTE_HARDWARE Hard;
    } u;
} MMPTE, *PMMPTE;


#define MmIsPageEntryValid(pde)	(pde->u.Hard.Valid)
#define MmIsPageEntryWritable(pde)	(pde->u.Hard.Valid)

BOOL MemManagerTestingON;


typedef struct _MMPFN {
	PMMPTE PteAddress;
	KLIST_ENTRY(_MMPFN) link;
	ULONG RefCount;
}MMPFN, *PMMPFN;



typedef LIST_HEAD(_PFNList, _MMPFN) PFNLIST;

// lock for Free page List
#define MmLockFreePageList() //TODO:
#define MmUnlockFreePageList()

static PFNLIST PfnFreeList; // Free list of physical pages
static PMMPFN PfnDataBase;		// Virtual address of PFN Database

// Boot stack - memory allocated in kos.S
extern char bootstacktop[], bootstack[];

static PHYSICAL_ADDRESS MaxPA;	// Maximum physical address
static ULONG MMNumberOfPhysicalPages;			// Amount of physical memory (in pages)
static size_t basemem;		// Amount of base memory (in bytes)


static PHYSICAL_ADDRESS GlobalCr3;
static PMMPTE GlobalPgdir;
static PCHAR PtrToBootFreemem;	// Pointer to next byte of free mem

static PBITMAP	MiSystemPteBitmap;

PHYSICAL_ADDRESS INLINE
MmGetGlobalCr3()
{		// Tmp function for use in Attach Process
	return (GlobalCr3);

}


PVOID INLINE
MmGetGlobalPgdir()
{
	return (GlobalPgdir);


}

static VOID
MmInitializeSystemPtes();




#define MmGetPdeAddress(x) (PMMPTE)(((((ULONG_PTR)(x)) >> 22) << 2) + MM_PDE_ADDRESS)
#define MmGetPteAddress(x) (PMMPTE)(((((ULONG_PTR)(x)) >> 12) << 2) + MMPAGETABLEAREA_START)
#define MiGetVitualAddressFromPTE(x) ((PVOID)((ULONG)(x) << 10))

/* Prototypes */

static void MiTest();


/*
This is only for boot time so we do not expect to use it for more than 4MB
*/

#define MiBootVA2PA(VA) 		\
	({								\
		PHYSICAL_ADDRESS __VA = (PHYSICAL_ADDRESS) (VA);		\
		if (__VA < KERNBASE)					\
			KeBugCheck("Invalid %08lx\n",__VA);\
		__VA - KERNBASE;					\
	})


#define MiBootPA2VA(PA)		\
	({											\
		PHYSICAL_ADDRESS __PA = (PA);				\
		ULONG __PPN = PPN(__PA); 			\
		if (__PPN >= MMNumberOfPhysicalPages)					\
			KeBugCheck("Invalid PA %08lx", __PA);				\
		(PVOID) (__PA + KERNBASE);				\
	})



/********************************************
		PFN Management
********************************************/
	
static VOID
x86CalcRam(VOID)
{
		// Hardcoding the amount of RAM in my test system for now
		basemem = IOPHYSMEM;
		MaxPA= MAXPHYMEM; //256 MB
		
		MMNumberOfPhysicalPages= MaxPA/ PGSIZE;
		kprintf("MaxPA %x MmNumPages:%x\n", MaxPA, MMNumberOfPhysicalPages);
}



static VOID
MmInitPFNDataBase()
{	
	ULONG PgCount;

	LIST_INIT(&PfnFreeList);

//	 Marking page 0 as in use to protect realmode IVT. 
	//Will need this in V8086 mode
	PfnDataBase[0].RefCount = 1;


	//Base memory, marking in use as it has our Kernel stack
	for (PgCount = 1; PgCount < (basemem/PGSIZE); PgCount++) {
		PfnDataBase[PgCount].RefCount = 1;
	}


// IO hole should never be allocated (Mark it in use)
	for(PgCount= (IOPHYSMEM/PGSIZE); 
					PgCount < (EXTPHYSMEM/PGSIZE); PgCount++) {
		PfnDataBase[PgCount].RefCount = 1; // mark this mem in use
	}

/*
	As per our current implementation we expect that all the 
	kernel and boot related data before turning on pagining
	is in single Page Directory Entry.

	If we hit this assert then we need to map more Page Directory Entries
	while enabling Paging.

*/
	ASSERT((ULONG_PTR)PtrToBootFreemem < (KERNBASE + PTSIZE));


//mark in use by kernel, stack  and PFN data structures
	for(PgCount = (EXTPHYSMEM/PGSIZE); 
			PgCount < ((ULONG) MiBootVA2PA(PtrToBootFreemem))/PGSIZE; 
				PgCount++) {

		PfnDataBase[PgCount].RefCount = 1;
	}

// rest all free
	for(PgCount = ((ULONG) MiBootVA2PA(PtrToBootFreemem))/PGSIZE; 
				PgCount < (MaxPA/PGSIZE); PgCount++){
		PfnDataBase[PgCount].RefCount = 0;
	// No need to Lock the List here, we will be the only user
		LIST_INSERT_TAIL(&PfnFreeList, &PfnDataBase[PgCount], link);
	}

}



/*
	This allocated page is not zeroed out
	The caller is reponsible to do that.
*/

static KSTATUS
MmAllocatePFN(PMMPFN *pfn, PMMPTE PteAddress)
{
	PMMPFN Page;


	MmLockFreePageList();
	LIST_REMOVE_HEAD(&PfnFreeList, Page, link);
	MmUnlockFreePageList();

	if (Page == NULL) {
		return STATUS_NO_MEMORY;
	}


	Page->PteAddress = PteAddress;
	*pfn = Page;

	return STATUS_SUCCESS;
}


static
VOID
MiFreePFN(PMMPFN pfn)
{
	ASSERT(pfn->RefCount == 0); //check that no page ref is there.

	MmLockFreePageList();
	LIST_INSERT_HEAD(&PfnFreeList, pfn, link);
	MmUnlockFreePageList();
}


#define MiIncReferencePFN(x)	((x)->RefCount)++

/*
	Decrement the Ref count of a PFN
	Free it if the Refcount reaches 0;

*/
VOID static
MiDecReferencePFN(PMMPFN pfn)
{
	if (--(pfn->RefCount) == 0)
		MiFreePFN(pfn);
}

#define MiGetRefCount(x) ((x)->RefCount)


static INLINE ULONG
MiPFNtoFrameNumber(PMMPFN pfn)
{
	return (pfn - PfnDataBase);
}

static INLINE PHYSICAL_ADDRESS
MiPFNtoPhysicalAddress(PMMPFN pfn)
{
	PHYSICAL_ADDRESS FrameNumber = MiPFNtoFrameNumber(pfn);
	return(FrameNumber << PGSHIFT);
}

static INLINE PMMPFN
MiPhysicalAddresstoPFN(PHYSICAL_ADDRESS PAddress)
{
	return (&PfnDataBase[PAddress >> PTXSHIFT]);
}



static INLINE
PMMPFN
MmGetPfnFromPte(PMMPTE Pte)
{
	return (MiPhysicalAddresstoPFN(Pte->u.Long & ~PTE_FLAGS_MASK));
}

/**************************************/




/*
  * Takes Kernel virtual add and gives Physical address
  */

PHYSICAL_ADDRESS
MmGetPhysicalAddress(IN PVOID Address)
{
	PMMPTE pPte;
	PHYSICAL_ADDRESS PhyAddress;
	
	pPte = (PMMPTE) MmGetPteAddress(Address);
	if (pPte->u.Hard.Valid) {
		PhyAddress = ((pPte->u.Hard.PageFrameNumber << PGSHIFT) + (PGOFF(Address)));
		return PhyAddress;
	}

	return STATUS_INVALID_PARAMETER;
	
}


PVOID
MmGetVirtualForPhysical(IN PHYSICAL_ADDRESS PhysicalAddress)
{
	PMMPTE pPte;

	pPte = PfnDataBase[PPN(PhysicalAddress)].PteAddress;
	return (MiGetVitualAddressFromPTE(pPte) + (PGOFF(PhysicalAddress)));
}

#define IsPageAlignedAddress(x)	PGOFF(x)?0:1


/*********************************************
		BOOT TIME INIT
*********************************************/

	
	
	/*
		Allocate NumBytes of physical memory aligned on an 
		align-byte boundary.  Align must be a power of two.
		Return kernel virtual address.	Returned memory is uninitialized.
	*/
static PVOID
MiInitBootAlloc(IN ULONG NumBytes, IN ULONG align)
{
		extern char end[];
		PVOID VirtAddress;


// do not use this function after the initializing PFNDATABASE
//		ASSERT(PfnFreeList == NULL);
	
		if (PtrToBootFreemem == 0)
			PtrToBootFreemem = end;
	
		//round Free mem up to be aligned properly
		PtrToBootFreemem = ROUNDUP(PtrToBootFreemem, align);
		
		//save current value of PtrToBootFreemem as allocated chunk
		VirtAddress= PtrToBootFreemem;
	
		//increase PtrToBootFreemem to record allocation
		PtrToBootFreemem += NumBytes;
	
		//return allocated chunk
		return VirtAddress;
	
}



/*
This is a boot time function to map memory segments.

It will only work before relocating kernel to KERNBASE using paging.
So it is goot enough on before paging is on

*/
static INLINE void
MiBootMapMemory( IN OUT PMMPTE PgDir, 
								IN ULONG_PTR  Address, 
								IN size_t size, 
								IN PHYSICAL_ADDRESS PhyAddress, 
								ULONG Perm)
{
	PMMPTE page_table;
	int i, j;

	DBGPRINT("Mapping memory at va:%x to %x\n", Address, Address+size);

	ASSERT(IsPageAlignedAddress(Address));
	ASSERT(IsPageAlignedAddress(PhyAddress));

	for ( i = 0; i < size; i += PTSIZE) {
		if (PgDir[PDX(Address+i)].u.Long == 0) {		
			ASSERT(0); // right now we are not supporting Creation of PTEs here so assert
			
			page_table = (PMMPTE)MiInitBootAlloc(PGSIZE, PGSIZE);
			memset(page_table, 0, PGSIZE);

			PgDir[PDX((ULONG_PTR)Address + i)].u.Long = MiBootVA2PA(page_table)|Perm;
		} else {
			page_table = MiBootPA2VA((PgDir[PDX(Address+i)].u.Long) & ~0x3FF);
		}

		for (j = 0; ((i + j) < size) && (j < PTSIZE); j += PGSIZE) {
			page_table[PTX(Address+i+j)].u.Long = (ULONG_PTR)PhyAddress | Perm;
			PhyAddress += PGSIZE;
		}
	}

}



//Prototype
VOID
MiInitializeUserPtesLock();



VOID
MiBootInitMemoryLayout()
{
	PMMPFN pfn;
	PMMPTE FirstPTE;
	ULONG_PTR CR0;
	ULONG_PTR counter = 0;
	KSTATUS status;
	
	/*
	Calculate the RAM in this system.
	If this fails it will bugcheck so no need to check return value
	*/
	x86CalcRam();


	
	PfnDataBase = (PMMPFN) MiInitBootAlloc(sizeof(MMPFN) * MMNumberOfPhysicalPages, PGSIZE);

	GlobalPgdir = (PMMPTE) MiInitBootAlloc(PGSIZE, PGSIZE); //Allocate 1 Page
	FirstPTE = (PMMPTE) MiInitBootAlloc(PGSIZE, PGSIZE); //Allocate 1 Page
	
	// Initilize both the pages
	memset(GlobalPgdir, 0, PGSIZE);
	memset(FirstPTE, 0,PGSIZE);

	// Put PTE into Pgdir
	GlobalPgdir[PDX(KERNBASE)].u.Long =  MiBootVA2PA(FirstPTE) |PTE_W |PTE_P;

/*
	Map Physical address 0 -4MB at KERNBASE
*/
	MiBootMapMemory(GlobalPgdir, KERNBASE,PTSIZE, 0, PTE_P|PTE_W);



/* 
	Please don't change the location.

	It uses MiInitBootAlloc function for memory Allocation,
	so should be called before 	MmInitPFNDataBase()
*/
	MmInitializeSystemPtes();


	//Initialize User mode pte spinlock
	MiInitializeUserPtesLock();


/*
	Initialize PFN database
		After here we cannot use "MiInitBootAlloc"
*/
	MmInitPFNDataBase();	

/*
	Segmentation To Paging
	Here we will use the flat 4GB address space
*/

	kprintf("PtrToBootFreemem %x\n" , PtrToBootFreemem);
		  
	GlobalPgdir[0].u.Long = GlobalPgdir[PDX(KERNBASE)].u.Long;
	GlobalCr3 = (PHYSICAL_ADDRESS) MiBootVA2PA(GlobalPgdir);

{ 	
	// some debugging info
	kprintf("GlobalPgdir %x GlobalCr3 %x\n", GlobalPgdir,GlobalCr3);
	kprintf("&GlobalPgdir[PDX(KERNBASE)] %x\n", &GlobalPgdir[PDX(KERNBASE)]);
	kprintf("&GlobalPgdir[0] %x\n", &GlobalPgdir[0]);
	kprintf("GlobalPgdir[PDX(KERNBASE)] %x\n", GlobalPgdir[PDX(KERNBASE)].u.Long);
	kprintf("GlobalPgdir[0] %x\n", GlobalPgdir[0].u.Long);
	kprintf("MiBootVA2PA(FirstPTE) %x\n", MiBootVA2PA(FirstPTE));
	kprintf("FirstPTE %x \n", FirstPTE);
}

	// Install page Directory.
	lcr3(GlobalCr3);
	
	// Turn on paging.
	CR0 = rcr0();
	//	cr0 |= CR0_PE|CR0_PG|CR0_AM|CR0_WP|CR0_NE|CR0_TS|CR0_EM|CR0_MP;
	//	cr0 &= ~(CR0_TS|CR0_EM);
	CR0 |= CR0_PG;
	
	lcr0(CR0);
	
		// Current mapping: KERNBASE+x => x => x.
		// (x < 4MB so uses paging pgdir[0])

	// Reload all segment registers.
//asm volatile("lgdt GDT_DESC+2");

	KiCreateCurrentGDT();

	
	// Final mapping: KERNBASE+x => KERNBASE+x => x.
	
	// This mapping was only used after paging was turned on but
	// before the segment registers were reloaded. Now we can remove this.
	GlobalPgdir[0].u.Long = 0;
	
	// Flush TLB to kill the pgdir[0] mapping.
	lcr3(GlobalCr3);

/*
	Now we can Use PFN functions as we have already created and initialized 
	the PfnDataBase
*/

	//Recursively insert page Directory as a page table
	GlobalPgdir[PDX(MMPAGETABLEAREA_START)].u.Long = MiBootVA2PA(GlobalPgdir) | PTE_W |PTE_P;


/*
Now what we want to do is allocate all the Page Tables for the entire Kernel Space
For that we will create Page Tables. Because the PTEs in these pagetables will 
map pages so the physical address will go into Page Directory. 
And as these page tables are in itself physical page so there will be a PTE for each
page table.
*/


	// Now Allocate Memory for all the Kernel PTEs
	for(counter = (KERNBASE + PTSIZE);  // the values will rollover to 0 
		((counter <= KERNSPACE_END) && (counter >= KERNBASE)); 
				counter += PTSIZE) {

		PMMPTE pPte;
		pPte = MmGetPteAddress(counter);

/*
GlobalPgdir[PDX(MM_PAGETABLE_AREA)].u.Long = MiBootVA2PA(SecondPTE) | PTE_W |PTE_P;
*/
		if ( (ULONG_PTR)pPte == MM_PDE_ADDRESS) {
			continue;
		}


//Allocate a Physical page		
		status = MmAllocatePFN(&pfn, pPte);
		ASSERT(success(status));	

// Map the physical address of these pages in
		ASSERT(GlobalPgdir[PDX(counter)].u.Long == 0);
		GlobalPgdir[PDX(counter)].u.Long = MiPFNtoPhysicalAddress(pfn) |PTE_W |PTE_P;
		
// Fill the new PTEs with 0s
		memset((PCHAR) pPte, 0, PGSIZE);

/*
		kprintf("GLobalPgdir[%d] %x \t", PDX(counter), GlobalPgdir[PDX(counter)].u.Long);
		kprintf("pPte %x \n", (PCHAR)pPte);
*/

	}


	//Lets do some testing now
//		MiTest();


}




/**************************************
	Virtual Memory Management Functions
**************************************/

//ULONG_PTR
PMMPTE
PsGetCurrentProcessPageDirectory();


//static INLINE
BOOL
IsCurrentPageDirectory(PVOID PageDirectory)
{

/*
	PHYSICAL_ADDRESS Cr3;
	Cr3 = rcr3();
	ASSERT2(Cr3 == MmGetPhysicalAddress(PageDirectory), MmGetPhysicalAddress(PageDirectory), Cr3);
*/
	return (MmGetPhysicalAddress(PageDirectory) 
		== MmGetPhysicalAddress((PVOID)MM_PDE_ADDRESS));
}



	
VOID
InvalidateTLB(PMMPTE PageDirectory, PVOID Address)
{
/*
	Only do it if we are inserting a page into current process context
*/
	if ((PageDirectory != NULL ) &&
//		(PsGetCurrentProcessPageDirectory()  !=  PageDirectory)
		!IsCurrentPageDirectory(PageDirectory)) {
		return;
	}

	invlpg(Address);
}
	
	
KSTATUS
MiRemovePage(PMMPTE PageDirectory, PVOID Address)
{
	
	PMMPTE pPte = NULL;
	PMMPTE pPde = NULL;
	PMMPFN pPfn = NULL;

	ASSERT(PageDirectory != NULL);
	ASSERT1(IsPageAlignedAddress(Address), Address);

	if ((PageDirectory != NULL) 
					&&
		(!IsCurrentPageDirectory(PageDirectory))) {


			if(IsUserAddress(Address)) {
	/*
		As per our current design a Kernel Space layout will be copied 
		for the new process, but to map or remove pages in user mode, it has to 
		do it in its own context.
	*/
				if (!MemManagerTestingON) {
					KeBugCheck("MiRemovePage: Removing User Page for other process context\n");
				}
			}
	}

	pPde = MmGetPdeAddress(Address);
	pPte = MmGetPteAddress(Address);
	
	// First check if the PDE is valid then check if the PTE is valid
	if ( (pPde->u.Hard.Valid  == 0) || (pPte->u.Hard.Valid == 0) ) {
		KeBugCheck("Trying to remove an unmapped page\n");
		return STATUS_SUCCESS; // Incase if we remove KeBugCheck later
	}	

// remove the page reference 
	pPfn = MiPhysicalAddresstoPFN(pPte->u.Hard.PageFrameNumber << PTXSHIFT);
	MiDecReferencePFN(pPfn);

/*
	Mark the page as invalid. Put the complete PTE to 0 
*/
	pPte->u.Long = 0;

	return STATUS_SUCCESS;
}
	
	


KSTATUS
MiInsertPage( IN PMMPTE PageDirectory, 
						IN PVOID Address, 
						IN PMMPFN pPfn,
						IN ULONG Perm)
{
	PMMPTE Pte;
	PMMPTE Pde;
	KSTATUS status;


	if ((PageDirectory != NULL) && 
//		((PMMPTE)PsGetCurrentProcessPageDirectory()  !=  PageDirectory)
		!IsCurrentPageDirectory(PageDirectory)) {
			if(IsUserAddress(Address)) {
/*
	As per our current design a Kernel Space layout will be copied 
	for the new process, but to map pages in user mode, it has to 
	do it in its own context.
*/
				KeBugCheck("MmInsertPage: Inserting User Page for other process context\n");
			}
	}

	Pte = (PMMPTE) MmGetPteAddress(Address);
	Pde = (PMMPTE) MmGetPdeAddress(Address);

/*
	If the Pde is invalid, Map a page table for this entry.

	We are gonna do it only for the use space, for kernel all the page tables are
	already mapped.
*/
	if (!MmIsPageEntryValid(Pde)) {
		PMMPFN pPfn1;


		ASSERT1(IsUserAddress(Address), Address);

	// PTE for a page table is PDE :-) And please don't go into recursion!!
		status = MmAllocatePFN(&pPfn1, Pde);
		if (!success(status)) {
			return status;
		}
		MiIncReferencePFN(pPfn1);
		if (IsUserAddress(Address)) {
			PageDirectory[PDX(Address)].u.Long =  
					MiPFNtoPhysicalAddress(pPfn1)| PTE_W |PTE_U|PTE_P;
		} else {
			PageDirectory[PDX(Address)].u.Long =  
					MiPFNtoPhysicalAddress(pPfn1)| PTE_W |PTE_P;


		}
		memset(ROUNDDOWN(Pte, PGSIZE),   0, PGSIZE);
	}
	
	if (pPfn == NULL) {
		status = MmAllocatePFN(&pPfn,Pte);
		
		if (!success(status)) {
			return status;
		}
	}

	if ((Pte->u.Hard.PageFrameNumber) == MiPFNtoFrameNumber(pPfn)) {
	// Already Mapped
//		ASSERT(0);
		return STATUS_SUCCESS;
	}

	if (Pte->u.Long != 0) {
		status = MiRemovePage(PageDirectory, Address);
		if (!success(status)) {
			return status;
		}
	}
	
	Pte->u.Long = MiPFNtoPhysicalAddress(pPfn) |Perm |PTE_P;

// Increament the reference on the frame and invalidate the TLB
	MiIncReferencePFN(pPfn);	
	InvalidateTLB(PageDirectory, Address);

	return STATUS_SUCCESS;

}




KSTATUS
MmMapSegment(	
				IN PVOID PageDirectory, 
				IN PVOID Address, 
				IN size_t Length, 
				IN ULONG Perm
				)
{
	KSTATUS status = STATUS_SUCCESS;

	PMMPFN pfn = NULL;
	PVOID VAddr = 0;
	PVOID VaStart, VaEnd;

	VaStart = ROUNDDOWN(Address, PGSIZE);
	VaEnd = ROUNDUP(Address + Length, PGSIZE);
	
	for(VAddr = VaStart;  VAddr < VaEnd;  VAddr += PGSIZE)
	{
		status = MmAllocatePFN(&pfn, (PMMPTE)MmGetPteAddress(VAddr));
		if (!success(status)) {
			KeBugCheck("page allocation Failed\n");
		}

		status = MiInsertPage(PageDirectory, VAddr, pfn,Perm);
		if (!success(status)) {
			// Will start handling these failures in part 2 of Development
			KeBugCheck("MiInsertPage Failed\n");
		}
		
	}
	return status;
}




/*
	V I R T U A L  ALLOC AND FREE
*/


VOID
MmVirtualFree(IN PVOID VirtualAddress, IN size_t size)
{
	size_t i;
	size = ROUNDUP(size, PGSIZE);

	for ( i = 0; i < size; i+= PGSIZE) {
	// boot_pgdir might work well for kernel mode as same address
		MiRemovePage((PMMPTE)MM_PDE_ADDRESS, VirtualAddress + i);
	}
}







KSTATUS
MmVirtualAlloc( IN PVOID VirtualAddress, 
							IN size_t size, 
							IN ULONG perm)
{
	size_t i;
	PMMPFN Page;
	KSTATUS status = STATUS_SUCCESS;
		
	size = ROUNDUP(size, PGSIZE);

	for(i = 0; i < size; i += PGSIZE) {
		status = MmAllocatePFN(&Page, (PMMPTE)
					MmGetPteAddress(VirtualAddress + i));
		ASSERT(success(status));

		if (!success(status)) {
				MmVirtualFree(VirtualAddress,i); //lets cleanup what we have already allocated
				return status;
		}
		status  = MiInsertPage(	(PVOID)MM_PDE_ADDRESS, 
				VirtualAddress+ i, Page, perm |PTE_P);

		if (!success(status)) {
			ASSERT(0);
			MmVirtualFree(VirtualAddress,i); 
			return status;
		}
		
	}
	return status;
}







/* S Y S P T E */

static KSPIN_LOCK SystemPteSpinLock;


/*
	This function is called while boot initialization so we have to use BootAlloc
	functions to allocate memory
*/

static VOID
MmInitializeSystemPtes()
{
	size_t BuffSize;
	PVOID Buffer;
	ULONG BitCount = (MMSYSPTE_SIZE/ PGSIZE);

	STATIC_ASSERT(MMSYSPTE_END < MMPAGETABLEAREA_START);


	BuffSize = BmBitmapBufferSize(BitCount);
	Buffer = MiInitBootAlloc(BuffSize,PGSIZE);
	kprintf("MiSystemPteBitmap : %d", BuffSize);

	MiSystemPteBitmap = BmBitmapCreateInBuffer(BitCount, Buffer, BuffSize);

	KeInitializeSpinLock(&SystemPteSpinLock);
}
/*
This will get you the free pages from the memory allocated 
for sysptes.

This will work in First fit method.
*/


PVOID
MmAllocateSystemPtes(IN ULONG NumberOfPages)
{
	KIRQL OldIrql;
	KSTATUS status;
	ULONG PageIndex = 0;
	PVOID Memory = NULL;

/*
	We do not want to get Context Switched with the bitmap lock
	so lets use spinlock.

	We only need the spinlock across Bitmap operations 
	as rest of the allocations have there own locks.
	
*/

	KeAcquireSpinLock( &SystemPteSpinLock, &OldIrql);

	PageIndex = BmBitmapScanAndFlip(MiSystemPteBitmap, 0,
										NumberOfPages, 
										FALSE);
	
	KeReleaseSpinLock( &SystemPteSpinLock,OldIrql);

	if (PageIndex == BITMAP_ERROR) {
		return NULL;		
	}

	Memory = (PVOID) (MMSYSPTE_START + (PGSIZE * PageIndex));

// map these pages
	status = MmVirtualAlloc(Memory, 
							NumberOfPages * PGSIZE,
								PTE_W);
	if (!success(status)) {
		return NULL;
	}

	return Memory;
}

VOID
MmFreeSystemPtes(IN PVOID Address, IN ULONG NumberOfPages)
{
	size_t	start;
	KIRQL OldIrql;
	
	ASSERT((ULONG_PTR)Address >= (MMSYSPTE_START));
	ASSERT((ULONG_PTR)Address < (MMSYSPTE_END));


	KeAcquireSpinLock( &SystemPteSpinLock, &OldIrql);


	//Syspte addresses should always be page aligned
	ASSERT(IsPageAlignedAddress(Address));

	start = (((ULONG_PTR)Address - MMSYSPTE_START) / PGSIZE);


// clear the bit map
	BmBitmapSetMultiple(MiSystemPteBitmap, start,
				NumberOfPages, FALSE);

// And free the memory
	MmVirtualFree(Address, (NumberOfPages * PGSIZE));
	
	KeReleaseSpinLock( &SystemPteSpinLock,OldIrql);
}




/*
Usermode PTE pool

This will only work for the current process context.

*/



static KSPIN_LOCK UserPteSpinLock;

VOID
MiInitializeUserPtesLock()
{
		KeInitializeSpinLock(&UserPteSpinLock);
}


KSTATUS
MmCloneUserPtes(OUT PVOID *ProcessBitmap, PVOID CloneProcessBitmap)
{

	PVOID Buffer;
	PBITMAP Bitmap;

	// One Page can address upto 128 MB Virtual mem
	Buffer = MmAllocateSystemPtes(1);

	if (Buffer == NULL) {
		*ProcessBitmap = NULL;
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	Bitmap = BmBitmapCreateInBuffer( 0, 
										(PVOID)Buffer, 
										PGSIZE);

	memcpy( Buffer, CloneProcessBitmap, PGSIZE);


	*ProcessBitmap = Buffer;

	return STATUS_SUCCESS;
}



KSTATUS
MmInitializeUserPtes(OUT PVOID *ProcessBitmap)
{

	PVOID Buffer;
	PBITMAP Bitmap;

	// One Page can address upto 128 MB Virtual mem
	Buffer = MmAllocateSystemPtes(1);

	if (Buffer == NULL) {
		*ProcessBitmap = NULL;
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	Bitmap = BmBitmapCreateInBuffer( 0, 
										(PVOID)Buffer, 
										PGSIZE);

	*ProcessBitmap = Bitmap;

	return STATUS_SUCCESS;
}
/*
This will get you the free pages from the memory allocated 
for user mode ptes. This is per process.

This will work in First fit method.
*/


PVOID
MmAllocateUserPtes(IN PVOID Bitmap,IN ULONG NumberOfPages)
{
	KIRQL OldIrql;
	KSTATUS status;
	ULONG PageIndex = 0;
	PVOID Memory = NULL;

/*
	We do not want to get Context Switched with the bitmap lock
	so lets use spinlock.

	We only need the spinlock across Bitmap operations 
	as rest of the allocations have there own locks.
	
*/

	KeAcquireSpinLock( &UserPteSpinLock, &OldIrql);

	PageIndex = BmBitmapScanAndFlip(Bitmap, 0,
										NumberOfPages, 
										FALSE);
	
	KeReleaseSpinLock( &UserPteSpinLock,OldIrql);

	if (PageIndex == BITMAP_ERROR) {
		return NULL;		
	}

	Memory = (PVOID) (USERPTEPOOL_START+ (PGSIZE * PageIndex));

// map these pages as writable and user
	status = MmVirtualAlloc(Memory, 
							NumberOfPages * PGSIZE,
								PTE_W| PTE_U);
	if (!success(status)) {
		return NULL;
	}

	return Memory;
}

VOID
MmFreeUserPtes( IN PVOID Address, 
							IN PBITMAP Bitmap, 
							IN ULONG NumberOfPages)
{
	size_t	start;
	KIRQL OldIrql;
	int i;

	
	ASSERT((ULONG_PTR)Address >= (USERPTEPOOL_START));
	ASSERT((ULONG_PTR)Address < (USERPTEPOOL_END));


	KeAcquireSpinLock( &UserPteSpinLock, &OldIrql);


	//userpte addresses should always be page aligned
	ASSERT(IsPageAlignedAddress(Address));

	start = (((ULONG_PTR)Address - USERPTEPOOL_START) / PGSIZE);

// ASSERT


	for( i = 0; i < NumberOfPages; i++) {
		ASSERT(BmBitmapTest(Bitmap, start + i) == TRUE);
	}

// clear the bit map
	BmBitmapSetMultiple(Bitmap, start,
				NumberOfPages, FALSE);

// And free the memory
	MmVirtualFree(Address, (NumberOfPages * PGSIZE));
	
	KeReleaseSpinLock( &UserPteSpinLock,OldIrql);
}



/* T E S T I N G */


static void
PrintMemSig();


static void
MiTest()
{
	PMMPFN Page0, Page1, Page2, Page;

	PFNLIST freelist;

	char *ptr;
	ptr = (char *) 0xF000000;
	KSTATUS status;
	int j = 0;

	MemManagerTestingON = TRUE;

	// should be able to allocate three pages
	Page0 = Page1 = Page2 = NULL;

	ASSERT(MmAllocatePFN(&Page0, NULL) == STATUS_SUCCESS);
	ASSERT(MmAllocatePFN(&Page1, NULL) == STATUS_SUCCESS);
	ASSERT(MmAllocatePFN(&Page2, NULL) == STATUS_SUCCESS);

	ASSERT(Page0);
	ASSERT(Page1 && Page1 != Page0);
	ASSERT(Page2 && Page2 != Page1 && Page2 != Page0);

	// temporarily steal the rest of the free pages
	freelist = PfnFreeList;
	LIST_INIT(&PfnFreeList);


	// should be no free memory
	ASSERT(MmAllocatePFN(&Page, NULL) == STATUS_NO_MEMORY);
	// there is no free memory, so we can't allocate a page table 
	ASSERT(!success(MiInsertPage(GlobalPgdir, 0x0, Page1, 0)));

	// free Page0 and try again: Page0 should be used for page table
	MiFreePFN(Page0);
	ASSERT(success(MiInsertPage(GlobalPgdir, 0x0, Page1, 0)));
	ASSERT((GlobalPgdir[0].u.Hard.PageFrameNumber << PTXSHIFT) == MiPFNtoPhysicalAddress(Page0));
	ASSERT((((MmGetPdeAddress(0x0))->u.Hard.PageFrameNumber) << PTXSHIFT)== MiPFNtoPhysicalAddress(Page0));
	ASSERT(MiPhysicalAddresstoPFN(MmGetPhysicalAddress((PVOID)0x0)) == Page1);

	ASSERT(Page1->RefCount == 1);
	ASSERT(Page0->RefCount == 1);

// should be able to map Page2 at PGSIZE because Page0 is already allocated for page table
	ASSERT(success(MiInsertPage(GlobalPgdir, (PVOID) PGSIZE, Page2,  0)));
	ASSERT(MmGetPhysicalAddress((PVOID) PGSIZE) == MiPFNtoPhysicalAddress(Page2));

	ASSERT(Page2->RefCount == 1);

	// should be no free memory
	ASSERT(MmAllocatePFN(&Page, NULL) == STATUS_NO_MEMORY);

	// should be able to map Page2 at PGSIZE because it's already there
	ASSERT(success(MiInsertPage(GlobalPgdir, (PVOID) PGSIZE, Page2,  0) ));
	ASSERT(MmGetPhysicalAddress((PVOID) PGSIZE) == MiPFNtoPhysicalAddress(Page2));
	ASSERT(Page2->RefCount == 1);

	// Page2 should NOT be on the free list
	// could haPageen in ref counts are handled sloPageily in mem_pg_insert
	ASSERT(MmAllocatePFN(&Page, NULL) == STATUS_NO_MEMORY);

	// should not be able to map at PTSIZE because need free page for page table
	ASSERT(!success(MiInsertPage(GlobalPgdir, (PVOID) PTSIZE, Page0,  0) ));


	// insert Page1 at PGSIZE (replacing Page2)
	ASSERT(success(MiInsertPage(GlobalPgdir,(PVOID) PGSIZE, Page1,  0) ));

	// should have Page1 at both 0 and PGSIZE, Page2 nowhere, ...
	ASSERT(MmGetPhysicalAddress((PVOID)0x0) == MiPFNtoPhysicalAddress(Page1));
	ASSERT(MmGetPhysicalAddress((PVOID)PGSIZE) == MiPFNtoPhysicalAddress(Page1));

	// ... and ref counts should reflect this
	ASSERT(Page1->RefCount == 2);
	ASSERT(Page2->RefCount == 0);

	// Page2 should be returned by MmAllocatePFN
	ASSERT(MmAllocatePFN(&Page, NULL) == 0 && Page == Page2);


	// unmapping Page1 at 0 should keep Page1 at PGSIZE
	MiRemovePage(GlobalPgdir, 0x0);
	ASSERT(MmGetPhysicalAddress((PVOID)0x0) == STATUS_INVALID_PARAMETER);
	ASSERT(MmGetPhysicalAddress((PVOID)PGSIZE) == MiPFNtoPhysicalAddress(Page1));
	ASSERT(Page1->RefCount == 1);
	ASSERT(Page2->RefCount == 0);


	// unmapping Page1 at PGSIZE should free it
	MiRemovePage(GlobalPgdir, (PVOID) PGSIZE);
	ASSERT(MmGetPhysicalAddress((PVOID)0x0) == STATUS_INVALID_PARAMETER);
	ASSERT(MmGetPhysicalAddress((PVOID)PGSIZE) == STATUS_INVALID_PARAMETER);
	ASSERT(Page1->RefCount == 0);
	ASSERT(Page2->RefCount == 0);

	// so it should be returned by MmAllocatePFN
	ASSERT(MmAllocatePFN(&Page, NULL) == 0 && Page == Page1);


	// should be no free memory
	ASSERT(MmAllocatePFN(&Page, NULL) == STATUS_NO_MEMORY);

	// forcibly take Page0 back
	ASSERT(((MmGetPdeAddress(0x0))->u.Hard.PageFrameNumber << PTXSHIFT) == MiPFNtoPhysicalAddress(Page0));
	GlobalPgdir[0].u.Long  = 0;
	ASSERT(Page0->RefCount == 1);
	Page0->RefCount = 0;

	// give free list back
	PfnFreeList = freelist;

	// free the pages we took
	MiFreePFN(Page0);
	MiFreePFN(Page1);
	MiFreePFN(Page2);

	//Test MmVirtualAlloc And MmVirtualfree


	kprintf("[%s:%d]If it fails After here it failed in VirtualAlloc test\n", __FUNCTION__, __LINE__);


	// +10 to verify that it allocates one more page
	status = MmVirtualAlloc((PVOID)ptr, PTSIZE + 10, PTE_W );
	ASSERT(	success(status) ==  TRUE);
		//PGSIZE because we have allocated one more page by using +10

		// fill the memory by 1
		for(j = 0; j <PTSIZE  + PGSIZE; j++) {
			*(ptr+j) = 1;
		}

		// read all the ones
		for( j = 0; j < 	PTSIZE	+ PGSIZE; j++) {
			ASSERT(*(ptr+j) == 1);
		}
		kprintf("	VirtualFree(%x,PTSIZE + 10);\n", ptr);

	MmVirtualFree(ptr,PTSIZE + 10);


	PrintMemSig();

	MemManagerTestingON = FALSE;
	kprintf("Memory Manager Test Succeeded!\n");
}


VOID
PrintMemSig(VOID)
{

		kprintf(" ======================================== \n");

		ASSERT(IsPageAlignedAddress(KERNSPACE_START));
		ASSERT(IsPageAlignedAddress(KERNSPACE_END));
		
		ASSERT(IsPageAlignedAddress(KERNBASE));
		ASSERT(IsPageAlignedAddress(KERNSIZE));
		
		ASSERT(IsPageAlignedAddress(MMPAGEPOOL_START));
		ASSERT(IsPageAlignedAddress(MMPAGEPOOL_SIZE));
		ASSERT(IsPageAlignedAddress(MMPAGEPOOL_END));
		
		ASSERT(IsPageAlignedAddress(MMNONPAGEPOOL_START));
		ASSERT(IsPageAlignedAddress(MMNONPAGEPOOL_SIZE));
		ASSERT(IsPageAlignedAddress(MMNONPAGEPOOL_END));
		
		
		ASSERT(IsPageAlignedAddress(MMSYSPTE_START));
		ASSERT(IsPageAlignedAddress(MMSYSPTE_SIZE));
		ASSERT(IsPageAlignedAddress(MMSYSPTE_END));
		
		
		/*
			Keep this less than 3GB.
			At 3 GB we have our page directories and tables mapped
		*/
		
		ASSERT(IsPageAlignedAddress(MMPAGETABLEAREA_START));
		ASSERT(IsPageAlignedAddress(MM_PDE_ADDRESS));
		ASSERT(IsPageAlignedAddress(MMPAGETABLEAREA_SIZE));
//		ASSERT(IsPageAlignedAddress(MMPAGETABLEAREA_END));
		
		
	

		kprintf("KERNSPACE_START ============== %x\n",KERNSPACE_START);
		kprintf("KERNSPACE_END ============== %x\n",KERNSPACE_END);

		kprintf("KERNBASE ============== %x\n",KERNBASE);
		kprintf("KERNSIZE ============== %x\n",KERNSIZE);

		kprintf("MMPAGEPOOL_START ============== %x\n",MMPAGEPOOL_START);
		kprintf("MMPAGEPOOL_SIZE ============== %x\n",MMPAGEPOOL_SIZE);
		kprintf("MMPAGEPOOL_END ============== %x\n",MMPAGEPOOL_END);

		kprintf("MMNONPAGEPOOL_START ============== %x\n",MMNONPAGEPOOL_START);
		kprintf("MMNONPAGEPOOL_SIZE ============== %x\n",MMNONPAGEPOOL_SIZE);
		kprintf("MMNONPAGEPOOL_END ============== %x\n",MMNONPAGEPOOL_END);


		kprintf("MMSYSPTE_START ============== %x\n",MMSYSPTE_START);
		kprintf("MMSYSPTE_SIZE ============== %x\n",MMSYSPTE_SIZE);
		kprintf("MMSYSPTE_END ============== %x\n",MMSYSPTE_END);


		/*
			Keep this less than 3GB.
			At 3 GB we have our page directories and tables mapped
		*/

		kprintf("MMPAGETABLEAREA_START ============== %x\n",MMPAGETABLEAREA_START);
		kprintf("MM_PDE_ADDRESS ============== %x\n",MM_PDE_ADDRESS);
		kprintf("MMPAGETABLEAREA_SIZE ============== %x\n",MMPAGETABLEAREA_SIZE);
		kprintf("MMPAGETABLEAREA_END ============== %x\n",MMPAGETABLEAREA_END);


		kprintf(" ======================================== \n");





}




VOID
MmUnMapPageTemp(PVOID VAddr, PMMPFN Page)
{


	ASSERT1(VAddr >= (PVOID)USERTEMP, VAddr);
	ASSERT1(VAddr < (PVOID)USERTEMPEND, VAddr);



	MiRemovePage((PMMPTE) MM_PDE_ADDRESS, VAddr);

	ASSERT(Page->RefCount != 0);

}


PVOID
MmMapPageTemp(PMMPFN Page)
{


	ULONG_PTR VAddr;
	KSTATUS Status = STATUS_NO_MEMORY;
	PMMPTE Pde, Pte;


	ASSERT((USERTEMPEND - USERTEMP) <= PTSIZE);

	Pde = MmGetPdeAddress(USERTEMP);


	if (!MmIsPageEntryValid(Pde)) {
		// map the page at USERTEMP. It will also map the PDE and allocate a Pagetable

		Status  = MiInsertPage( (PVOID)MM_PDE_ADDRESS, 
				(PVOID)USERTEMP, Page, PTE_W );

				VAddr = USERTEMP;
				goto EndMapPageTemp;
	}

	for (VAddr = USERTEMP; VAddr < USERTEMPEND; VAddr +=PGSIZE) {

		Pte = MmGetPteAddress(VAddr);

		if (!MmIsPageEntryValid(Pte)) {

			Status	= MiInsertPage( (PVOID)MM_PDE_ADDRESS, 
						(PVOID)VAddr, Page, PTE_W );

			goto EndMapPageTemp;


		}
	}


EndMapPageTemp:

		if (!K_SUCCESS(Status)) {
			return NULL;
		}

		return((PVOID) VAddr);
}


VOID
MmAttachToAddressSpace(PVOID PageDirectory)
{

	PHYSICAL_ADDRESS Cr3;
	
	Cr3 = MmGetPhysicalAddress(PageDirectory);
	lcr3(Cr3);

}



VOID
MmDeleteProcessAddressSpace(PVOID PageDirectory)
/*++

	We cannot afford to context switch in between, 
	So this Function should be called at IRQL == DISPATH_LEVEL

--*/

{

	PMMPTE Pte;
	PMMPTE Pde;
	PMMPTE TestPde;
	LONG_PTR DirAddress = 0;
	LONG_PTR PageAddress = 0;
	PHYSICAL_ADDRESS OldCr3;

	ASSERT1(KeGetCurrentIrql() == DISPATCH_LEVEL, KeGetCurrentIrql());
//store the old page directory
	OldCr3 = rcr3();


// We should not be running in the context of the PageDirectory
	ASSERT1(!IsCurrentPageDirectory(PageDirectory), PageDirectory);

// Load the new Page directory, 
// Remember PageDirectory is a virtual address so we cannot directly do lcr3 with it
	MmAttachToAddressSpace(PageDirectory);

	TestPde = (PMMPTE) PageDirectory;

	for (DirAddress = 0; DirAddress < USERTOP; DirAddress += PTSIZE) {

		Pde = MmGetPdeAddress(DirAddress);




	ASSERT( TestPde[PDX(DirAddress)].u.Long == Pde->u.Long);

	//First check if the PDE is valid
	
		if (Pde->u.Hard.Valid != 0) {




		//	Remove all the pages mapped from by this pde
			for (PageAddress = DirAddress; PageAddress < DirAddress + PTSIZE; PageAddress += PGSIZE) {
		
				Pte = MmGetPteAddress(PageAddress);
				if ( Pte->u.Hard.Valid != 0) {
					MiRemovePage(PageDirectory, (PVOID)PageAddress);
				}
			}

			// Now remove this page table
			MiRemovePage(PageDirectory, (PVOID) MmGetPteAddress(DirAddress));

			
		} else {
			// Debugging
			ASSERT1(TestPde[PDX(DirAddress)].u.Long == 0, TestPde[PDX(DirAddress)].u.Long );
		}

	}

// Go back to the old page directory
	lcr3(OldCr3);


// Now Free the Page Directory
	MmFreeSystemPtes(PageDirectory, 1);


}




KSTATUS
MmCloneProcessAddressSpace( 
							IN PMMPTE ClonePageDirectory, 
							IN PMMPTE NewPageDirectory
							)
/*++

Function: MmCloneProcessAddressSpace

Args:
	ClonePageDirectory - Page Directory to Clone
	NewPageDirectory -New Address space


1. Parse through all the pages in the usermode (i.e 0 to USERTOP) in ClonePageDirectory
2. If the page is read only - map it as read only in the NewPageDirectory
3. If the page is RW or COW -take a ref to Page and map it as (Read Only | COW) in both PageDirectories.
4. In Page fault handler if the pagefault is write and the page is COW then
5. If the PageFrame->ref is 2 or more allocate a new page, copy data and 
	Map the new page as write. Reduce the ref count on the original page.
6. If the PageFrame->ref is 1 just change the page perm from (RO | COW) to RW.

.... or something like that :-)



Return:
	STATUS_INSUFFICIENT_RESOURCES
	STATUS_NO_MEMORY

	If this function fails the process still need to send the pagedir to MmDeleteProcessAddressSpace

--*/





{

	PMMPTE ClonePte, NewPte;
	PMMPTE ClonePde, NewPde;
	PMMPTE CloneTestPde, NewTestPde;
	LONG_PTR DirAddress = 0;
	LONG_PTR PageAddress = 0;
	PHYSICAL_ADDRESS OldCr3;
	PMMPFN Pfn = NULL;
	PMMPFN NewPfn = NULL;
	PMMPTE TempPageAddr = NULL;
	KSTATUS Status;

	ASSERT1(KeGetCurrentIrql() == DISPATCH_LEVEL, KeGetCurrentIrql());
//store the old page directory
	OldCr3 = rcr3();


// We should not be running in the context of the NewPageDirectory
	ASSERT1(!IsCurrentPageDirectory(NewPageDirectory), NewPageDirectory);



//And should be running in the context of ClonePageDirectory
	ASSERT1(IsCurrentPageDirectory(ClonePageDirectory), ClonePageDirectory);



	CloneTestPde = (PMMPTE) ClonePageDirectory;
	NewTestPde = (PMMPTE) NewPageDirectory;

	for (DirAddress = 0; DirAddress < USERTOP; DirAddress += PTSIZE) {

		ClonePde = MmGetPdeAddress(DirAddress);
		NewPde = &NewTestPde[PDX(DirAddress)];


		
		ASSERT( CloneTestPde[PDX(DirAddress)].u.Long == ClonePde->u.Long);
		ASSERT2( NewTestPde[PDX(DirAddress)].u.Long == 0, NewTestPde[PDX(DirAddress)].u.Long, DirAddress);

		
	//First check if the PDE is valid
		if (ClonePde->u.Hard.Valid != 0) {


		// Allocate a PFN and take the ref before using it
		// We need to manually take the ref here because we are not using InsertPage

				Status = MmAllocatePFN(&NewPfn, NewPde);
				if (!K_SUCCESS(Status)) {
					return Status;
				}
				
				MiIncReferencePFN( NewPfn);

				NewPageDirectory[PDX(DirAddress)].u.Long =  
							MiPFNtoPhysicalAddress(NewPfn)| PTE_W |PTE_U|PTE_P;


				// Map temporary this page and memset
				ASSERT(TempPageAddr == NULL);
				TempPageAddr = MmMapPageTemp(NewPfn);

				if (TempPageAddr == NULL) {
					ASSERT(0);
				}
				
				memset(ROUNDDOWN(TempPageAddr, PGSIZE),   0, PGSIZE);




			for (PageAddress = DirAddress; PageAddress < DirAddress + PTSIZE; PageAddress += PGSIZE) {
		
				ClonePte = MmGetPteAddress(PageAddress);
				NewPte = &TempPageAddr[PTX(PageAddress)];
				if ( ClonePte->u.Hard.Valid != 0) {



				//Get the PFN struct from physical address
//					Pfn = MiPhysicalAddresstoPFN(ClonePte->u.Long & ~PTE_FLAGS_MASK);
					Pfn = MmGetPfnFromPte(ClonePte);
				
				//Ref the page
					MiIncReferencePFN(Pfn);


				// If the page is already mapped COW-> map it the same as COW.
					if (ClonePte->u.Hard.CopyOnWrite) {
						
						// A page marked COW should not be writable
						ASSERT1(ClonePte->u.Hard.Writable  == 0, ClonePte->u.Long);
						
						NewPte->u.Long = ClonePte->u.Long;

					} else  if (ClonePte->u.Hard.Writable)	{

				//If Page writable -> map it RO and COW


						ClonePte->u.Hard.Writable = 0;
						ClonePte->u.Hard.CopyOnWrite = 1;
						NewPte->u.Long = ClonePte->u.Long;
					} else {
					
				// If a page is RO, we will just map it as it is
						NewPte->u.Long = ClonePte->u.Long;

					}

				}
			}

			MmUnMapPageTemp(TempPageAddr, NewPfn);
			TempPageAddr = NULL;
			
		}

	}

	return STATUS_SUCCESS;
}

KSTATUS
MmCreateProcessAddressSpace(PVOID *PageDirectory)
{
	PMMPTE page;
	ULONG_PTR count;

	// allocate a page for the new process page directory
	page = (PMMPTE) MmAllocateSystemPtes(1);

	if (page == NULL) {
		ASSERT(0);
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	
/*
	Copy our global page directory into the new one.

	We will always copy GlobalPgdir, as the lower half will be 0
	so no need to memset it.

*/
//	for(count = KERNBASE; count >= KERNBASE; count+= ) {

//	}

	memcpy( page, GlobalPgdir, PGSIZE);
	kprintf("MmCreateProcessAddressSpace: phyadd :%x\n", MmGetPhysicalAddress(page));


	//Recursively insert page Directory as a page table
	page[PDX(MMPAGETABLEAREA_START)].u.Long = 
			MmGetPhysicalAddress(page) | PTE_W |PTE_P;

	*PageDirectory = page;

	return STATUS_SUCCESS;
}







VOID
MmPfHandler(PVOID FaultAddress, PVOID TrapFrame)
{
	PMMPTE Pte, Pde;
	PMMPFN Pfn;
	PVOID PageAddress;
	PVOID VirtualAddress;
	KSTATUS Status;

	kprintf("MmPfHandler 0x%x\n", FaultAddress);

	if (!IsUserAddress(FaultAddress)) {

		PrintKTrapframe(TrapFrame);
		KeBugCheck("Kernel PageFault: 0x%x\n", FaultAddress);
	}


	Pde = MmGetPdeAddress(FaultAddress);
	Pte = MmGetPteAddress(FaultAddress);


	if (Pde->u.Hard.Valid == 0) {

		PrintKTrapframe(TrapFrame);
		KeBugCheck("Invalid PDE  0x%x Faulting Page: 0x%x\n",Pde->u.Long, FaultAddress);
	}


	if (Pte->u.Hard.Valid == 0) {

		PrintKTrapframe(TrapFrame);
		KeBugCheck("Invalid PTE  0x%x Faulting Page: 0x%x\n",Pte->u.Long, FaultAddress);
	}


	//Right now we only handle Page Faults for COW
	if (Pte->u.Hard.CopyOnWrite == 0) {

		PrintKTrapframe(TrapFrame);
		KeBugCheck("Not COW PTE  0x%x Faulting Page: 0x%x\n",Pte->u.Long, FaultAddress);
	}


	
	PageAddress = (PVOID) ROUNDDOWN(FaultAddress, PGSIZE);

	if (Pte->u.Hard.CopyOnWrite) {

// If the current Page Ref count is 1 map it writable
// Else
// Take a ref of the Page and remove it from it location.
// map this Old page to tmp location.
// Allocate a new page and insert it at the Fault Address
// Copy the contents from old page to the new page
// Unmap the old page and dec reference.

		Pfn = MmGetPfnFromPte(Pte);

		ASSERT(MiGetRefCount(Pfn) != 0);	
		ASSERT1(Pte->u.Hard.Writable == 0, Pte->u.Long);
		
		Pfn = MmGetPfnFromPte(Pte);


		if (MiGetRefCount(Pfn) == 1) {
			Pte->u.Hard.CopyOnWrite = 0;
			Pte->u.Hard.Writable = 1;
		} else {


			MiIncReferencePFN(Pfn);
			MiRemovePage((PVOID)MM_PDE_ADDRESS, PageAddress);

			VirtualAddress = MmMapPageTemp( Pfn);
			if (VirtualAddress == NULL) {
				PrintKTrapframe(TrapFrame);
				KeBugCheck("Failed to Handle Page Fault 0x%x Faulting Page: 0x%x\n",Pte->u.Long, FaultAddress);
			}


			Status  = MmMapSegment( (PVOID)MM_PDE_ADDRESS,  PageAddress, PGSIZE, PTE_U|PTE_W);

			if (!K_SUCCESS(Status)) {
				PrintKTrapframe(TrapFrame);
				KeBugCheck("Failed to Handle Page Fault MapSeg Failed 0x%x Faulting Page: 0x%x\n",Pte->u.Long, FaultAddress);
			}

			memcpy(PageAddress, VirtualAddress, PGSIZE);
			MmUnMapPageTemp(VirtualAddress, Pfn);
			MiDecReferencePFN(Pfn);

		}


		return;
	}

	PrintKTrapframe(TrapFrame);
	KeBugCheck("Failed to Handle Page Fault 0x%x Faulting Page: 0x%x\n",Pte->u.Long, FaultAddress);


}







VOID
MmInitializeAllPools();


VOID
MmInitPhase0()
{
	Log(LOG_INFO, "Initailizing Memory Phase 0.\n");
	MiBootInitMemoryLayout();
	MmInitializeAllPools();

}


VOID
MmInitPhase1()
{


}



