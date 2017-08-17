/*
*  C Implementation:Obhandle.c
*
* Description: Object Manager Handle Table implementations
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/

#include "ob.h"

//InterlockedCompareExchange(volatile PLONG const Destination,const LONG Exchange,const LONG Comperand)



#define OB_INVALID_HANDLE	-1





VOID
ObDerefAllHandleTableObjects(IN PHANDLE_TABLE HandleTable)
{

	int i = 0;
	PVOID Object;
	
	for (i = 0; i < EX_MAX_HANDLE; i++) {

			if (HandleTable->Handle[i].Object != OB_INVALID_HANDLE) {

				Object = HandleTable->Handle[i].Object;
				ObDereferenceObject(Object);
				
				HandleTable->Handle[i].Object = OB_INVALID_HANDLE;

			}
	}


}

VOID
ObFreeHandleTable(IN PHANDLE_TABLE HandleTable)
{
	PVOID Memory;
	ULONG NumPages = 0;
	ULONG MemSize = 0;
	int i;

	MemSize = EX_MAX_HANDLE * sizeof(HANDLE_TABLE_ENTRY) ;
	NumPages = MemSize/PGSIZE;

	ASSERT(MemSize	== PGSIZE);

//Well in case we remove the previous assert
// It should always be multiples of page size
	ASSERT((MemSize % PGSIZE)  == 0);

// We should never enter here, and we have an assert for this.
	if (MemSize % PGSIZE) {
		// If we enter here that means our number of handle are not rounded up to page size
		// so get one more
		
		NumPages++;
	}

	// Deref All the Object that we have
	ObDerefAllHandleTableObjects( HandleTable);

	Memory = HandleTable->Handle;



	MmFreeSystemPtes(Memory, NumPages);

}

KSTATUS
ObAllocateHandleTable(IN PHANDLE_TABLE HandleTable,
										IN PHANDLE_TABLE InheritHandleTable,
										IN BOOL IsClone)
{

	PVOID Memory;
	ULONG NumPages = 0;
	ULONG MemSize = 0;
	int i;
	PVOID Object;

	

	
	MemSize = EX_MAX_HANDLE * sizeof(HANDLE_TABLE_ENTRY) ;
	NumPages = MemSize/PGSIZE;

	ASSERT(MemSize  == PGSIZE);

//Well in case we remove the previous assert
	ASSERT((MemSize % PGSIZE)  == 0);

	if (IsClone) {
		ASSERT(InheritHandleTable);
	}

// We should never enter here, and we have an assert for this.
	if (MemSize % PGSIZE) {
		// If we enter here that means our number of handle are not rounded up to page size
		// so get one more
		
		NumPages++;
	}

	Memory = MmAllocateSystemPtes(NumPages);

	if (Memory == NULL) {

		return STATUS_INSUFFICIENT_RESOURCES;
	}

	HandleTable->Handle = (PHANDLE_TABLE_ENTRY) Memory;


	if (InheritHandleTable) {
		for (i = 0; i < EX_MAX_HANDLE; i++) {

			Object = InheritHandleTable->Handle[i].Object;

			if ( (Object == OB_INVALID_HANDLE) || (Object == NULL)) {

				continue;
			}

// If fork is set inherit all the objects, else copy only inheritable objects			
			if( (IsClone) || (ObIsObjectInheritable(Object))) {


// If we are inheriting an object then the handle should remain same, 
// so push the object at the same location (i)

				ObReferenceObject( Object);			
				HandleTable->Handle[i].Object = Object;

			} else {
			
				HandleTable->Handle[i].Object = OB_INVALID_HANDLE;

			}
		}

	} else {

		for (i = 0; i < EX_MAX_HANDLE; i++) {
			
			HandleTable->Handle[i].Object = OB_INVALID_HANDLE;
		}

	}
	return STATUS_SUCCESS;
}


KSTATUS
ObInsertHandle(
				IN PVOID Object,
				IN PHANDLE_TABLE HandleTable,
				OUT PHANDLE Handle
				)
// This function should not be call Outside OB as OB will take care of incrementing handle count
// One exception to this is Process, where we are reusing this function to get the PID.


{
	int i;
	ULONG RetValue = 0;
	BOOL Found = FALSE;
	KSTATUS Status;
	POBJECT_HEADER ObjectHeader;


	ASSERT( Object );
	ASSERT( HandleTable );
	ASSERT( Handle );

	ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);

	if (ObjectHeader->Attributes & OBJ_EXCLUSIVE) {

		//TODO: A Object marked as exclusive can only have single handle to it.
	}

	
	for (i = 0; i < EX_MAX_HANDLE; i++) {

		// InterlockedCompareExchangePointer saves us from using other locks here

		RetValue = InterlockedCompareExchangePointer( &HandleTable->Handle[i].Object,
													Object,
													(PVOID) OB_INVALID_HANDLE);

		if (RetValue == OB_INVALID_HANDLE) {
			ASSERT(HandleTable->Handle[i].Object == Object);
			Found = TRUE;
			break;
		}

	}

	if (Found) {
		
		ASSERT1(i < EX_MAX_HANDLE, i);
		ASSERT(HandleTable->Handle[i].Object == Object);

		*Handle = i;
		Status = STATUS_SUCCESS;
	} else {
		Log(LOG_ERROR, "Failed to Insert Handle \n");
		Status = STATUS_TOO_MANY_OPENED_FILES;
		*Handle = OB_INVALID_HANDLE;

	}

	
	return Status;
}




VOID
ObRemoveHandle(
				IN HANDLE Handle,				
				IN PHANDLE_TABLE HandleTable,
				IN PVOID Object
				)

{
	ASSERT(Handle < EX_MAX_HANDLE);
	ASSERT(Handle  != OB_INVALID_HANDLE);
	ASSERT(HandleTable->Handle[Handle].Object == Object);
	
	HandleTable->Handle[Handle].Object = (PVOID)OB_INVALID_HANDLE;
}

KSTATUS
ObLookupObjectByHandle(
				IN HANDLE Handle,
				IN PHANDLE_TABLE HandleTable,
				OUT PVOID *Object
				)
{


	ASSERT(Handle < EX_MAX_HANDLE);
	ASSERT(Handle  != OB_INVALID_HANDLE);

	*Object = HandleTable->Handle[Handle].Object; 
	return STATUS_SUCCESS;
}

