/*
*  C Implementation:Ob.c
*
* Description: Object Manager
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/


#include	"ob.h"

#define OB_OBJECT_NAME_LENGTH	64

typedef ULONG SECURITY_DESCRIPTOR;


#define ObLogsEnable	1

#if ObLogsEnable
#define ObLog(fmt, args...) 		kprintf("[%s][%d]:", __FUNCTION__, __LINE__); kprintf(fmt, ## args)
#else
#define ObLog(fmt, args...)
#endif



// this will store the info of a type of Object
// this will be just created once at the init
OBJECT_TYPE ObjectTypeList[OBJECT_TYPE_LAST];



KSTATUS
ObCreateObjectType(
					IN POBJECT_TYPE ObjectType,
					OUT POBJECT_TYPE *SavedObjectType
					)

{

	POBJECT_TYPE LocalObjectType;

	ASSERT1(ObjectType->TypeIndex < OBJECT_TYPE_LAST, ObjectType->TypeIndex);
	ASSERT(SavedObjectType);
	ASSERT(*SavedObjectType == NULL);

	if (ObjectType->TypeIndex >= OBJECT_TYPE_LAST) {
		KeBugCheck("Unknown Object type\n");
	}


// Take the pointer to the memory reserved for this object type	
	LocalObjectType = &ObjectTypeList[ObjectType->TypeIndex];

// Copy the passed Object type into our storage for the object type
	memcpy(LocalObjectType, ObjectType,	sizeof(OBJECT_TYPE));

	LIST_INIT(&LocalObjectType->Object_List_Head);
	KeInitializeSpinLock(&LocalObjectType->SpinLock);

	if (SavedObjectType) {

		*SavedObjectType = LocalObjectType;
	}

	return STATUS_SUCCESS;
}


VOID
ObRemoveObject( IN PVOID Object)
{


	POBJECT_TYPE ObjectType;
	KIRQL OldIrql;
	ULONG PoolTag;
	POBJECT_HEADER ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);

	ASSERT(ObjectHeader->RefCount == 0);
	ASSERT(ObjectHeader->OpenHandleCount == 0);
	ASSERT( (ObjectHeader->Attributes & OBJ_PERMANENT) == 0);

	ObjectType = ObjectHeader->ObjectType;


	if (ObjectType->CloseProcedure) {
		(ObjectType->CloseProcedure)(Object);

	}

	
		
	KeAcquireSpinLock( &ObjectType->SpinLock, &OldIrql);
	LIST_REMOVE( &ObjectType->Object_List_Head, 
					ObjectHeader, 
					ObjectTypeListLink);


	KeReleaseSpinLock(&ObjectType->SpinLock, OldIrql);

	PoolTag = ObjectType->PoolTag;
	
	MmFreePoolWithTag(ObjectHeader, PoolTag);

}


VOID
ObCloseObject(
							IN PVOID Object,
							IN ULONG HandleCount
							)

{
	KSTATUS status;
	POBJECT_HEADER ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);
	POBJECT_TYPE ObjectType;


	ASSERT1(ObjectHeader->RefCount >= 1, ObjectHeader->RefCount);


	// if Object is permanent, don't remove it. Just return here.
	if (ObjectHeader->Attributes & OBJ_PERMANENT) {

	// Only a named Object can be defined permanent
		ASSERT(ObjectHeader->ObjectName.Buffer != NULL);

		return;
	}


	// Before Creating the object handle, the object should be referenced first,
	// so we will not increment the Object->Refcount while Inserting the Handle
	//But while closing a handle the RefCount and handleCount both should be decremented


	if (HandleCount) {
		InterlockedExchangeAdd( &ObjectHeader->OpenHandleCount, -HandleCount);
		InterlockedExchangeAdd( &ObjectHeader->RefCount, -HandleCount);

	} else {

		InterlockedDecrement( &ObjectHeader->RefCount);
	}
	
	if (ObjectHeader->RefCount > 0) {
		return;
	}

	//if ref count has reched 0 free the Object
	ObRemoveObject(Object);

}



VOID
ObClose(HANDLE Handle)
{
	KSTATUS Status;
	PVOID Object;
	Status = ObReferenceObjectByHandle(Handle, 0, NULL, UserMode, &Object, NULL);


	ASSERT(K_SUCCESS(Status));


	ObDereferenceObject( Object); //Because we just took a ref

	if (!K_SUCCESS(Status)) {

		return;
	}


	ObCloseObject(Object,1);
}


VOID
ObIncrementHandleCount( 
							IN PVOID Object)
{

	ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

	POBJECT_HEADER ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);
	ASSERT(ObjectHeader->RefCount > 0);
	

	InterlockedIncrement(&ObjectHeader->OpenHandleCount);	

	// Before Creating the object handle, the object should be referenced first,
	// so we will not increment the Object->Refcount while Inserting the Handle
	//But while closing a handle the RefCount and handleCount both should be decremented

}


VOID
ObReferenceObject(
						IN PVOID  Object
						)
{
	
	POBJECT_HEADER ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);
	ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
	ASSERT(ObjectHeader->RefCount != 0);
	ASSERT(ObIsValidObject(Object));

	
	InterlockedIncrement(&ObjectHeader->RefCount);
}


KSTATUS 
ObReferenceObjectByPointer(
						IN PVOID  Object,
						IN ACCESS_MASK  DesiredAccess,
						IN POBJECT_TYPE  ObjectType,
						IN KPROCESSOR_MODE  AccessMode
						)

{

	POBJECT_HEADER ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);
	
	ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
//	ASSERT(AccessMode == KernelMode);
// Check the Object Type
	ASSERT(ObjectHeader->ObjectType == ObjectType);
// Assert here if we are referencing an Object where the refcount has already gone to zero.
	ASSERT(ObjectHeader->RefCount);
	
	ObReferenceObject( Object);

	return STATUS_SUCCESS;
}


KSTATUS 
ObReferenceObjectByHandle(
								IN HANDLE  Handle,
								IN ACCESS_MASK  DesiredAccess,
								IN POBJECT_TYPE  ObjectType  OPTIONAL,
								IN KPROCESSOR_MODE  AccessMode,
								OUT PVOID  *Object
								)
{


	KSTATUS Status;
	POBJECT_HEADER ObjectHeader;
	PVOID LocalObject;
	PEPROCESS CurrentProcess = PsGetCurrentProcess();

//	ASSERT(AccessMode == KernelMode);
	ASSERT(CurrentProcess);
	
	Status = ObLookupObjectByHandle(Handle,
						&CurrentProcess->HandleTable, &LocalObject);

	if (!K_SUCCESS(Status)) {
		KeBugCheck("Failed to get the object from handle\n");
		return Status; // Well if we remove the bug check some day
	}

// Get the Object header and increment the ref count
	ObjectHeader = OBJECT_TO_OBJECT_HEADER(LocalObject);

	
	Status = ObReferenceObjectByPointer(LocalObject,
									DesiredAccess,
									ObjectType,
									AccessMode);
	
	if (!K_SUCCESS(Status)) {
		ASSERT(0);
		return Status;
	}


	if (Object) {
		*Object = LocalObject;
	}
	
	return STATUS_SUCCESS;
}



VOID 
ObDereferenceObject(
						IN PVOID  Object
						)
{

	ASSERT(ObIsValidObject( Object));
	ObCloseObject(Object, 0);
}

KSTATUS
ObInsertObject(IN PVOID Object,
					IN ACCESS_MASK DesiredAccess,
					IN ULONG ObjectPointerBias,
					OUT PHANDLE Handle)
{

	KSTATUS Status;
	PEPROCESS Process = NULL;

	


	Process = PsGetCurrentProcess();

	ASSERT(Process);
	
	Status = ObInsertHandle( Object, &Process->HandleTable,Handle);

	if (!K_SUCCESS(Status)) {

		return Status;
	}
	
	ObIncrementHandleCount(Object);

	return STATUS_SUCCESS;

}

/*
KSTATUS
KAPI
ObCreateObject(IN KPROCESSOR_MODE ProbeMode OPTIONAL,
               IN POBJECT_TYPE Type,
               IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
               IN KPROCESSOR_MODE AccessMode,
               IN OUT PVOID ParseContext OPTIONAL,
               IN ULONG ObjectSize,
               IN ULONG PagedPoolCharge OPTIONAL,
               IN ULONG NonPagedPoolCharge OPTIONAL,
               OUT PVOID *Object)
*/

KSTATUS
ObCreateObject( 
					IN KPROCESSOR_MODE ProbeMode, 
					IN POBJECT_TYPE ObjectType,
					IN POBJECT_ATTRIBUTES ObjectAttributes,
					IN ULONG ObjectSize,
					OUT PVOID *Object
					)
//ObCreateObject will set the refcount 1.

// If we just want to create the Handle to the Object and not use the object after that,
// then we need to dereference after the ObInsertObject.




{

	KSTATUS Status = STATUS_SUCCESS;
	POBJECT_HEADER ObjectHeader = NULL;
	PVOID LocalObject;
	size_t ObjectMemSize = 0;
	KIRQL OldIrql;
	PKTHREAD Thread;

	POBJECT_DIRECTORY Root;
	OBP_LOOKUP_CONTEXT Context;
	BOOLEAN ObjectHasName = FALSE;



	Thread = KeGetCurrentThread();

	ASSERT(Object != NULL);
	ASSERT(ObjectSize > 0);

	// For user mode we need to probe and verify the user addresses
//	ASSERT(ProbeMode == KernelMode);
	ASSERT(ObjectType);
	ASSERT(ObjectType == &ObjectTypeList[ObjectType->TypeIndex]);
//	ASSERT(ObjectAttributes);

	if (ObjectAttributes) {
		ASSERT1( (ObjectAttributes->Attributes & ~OBJ_VALID_ATTRIBUTES) == 0, ObjectAttributes->Attributes);
	}

	ObLog("Creating Object  Type %d 0x%x\n", ObjectType->TypeIndex, ObjectType); 


	ObInitializeLookupContext( &Context);

// Check if the Object has a name
	if ( (ObjectAttributes) && (ObjectAttributes->ObjectName)) {

		ObjectHasName = TRUE;
		ObLog("Name Object %s\n", ObjectAttributes->ObjectName->Buffer);

		if (!ObjectAttributes->RootDirectory) {
			// If RootDirectory is 0, it means ObjectName is fully qualified, so start from ObRootDirectory
			ASSERT(ObRootDirectory);

			
			Root = ObRootDirectory;	

		} else {

			Status = ObReferenceObjectByHandle(
							ObjectAttributes->RootDirectory,
							0,
							ObDirectoryObjectType,
							Thread->PreviousMode,
							&Root);

			
			if (!K_SUCCESS(Status)) {
				return Status; // Well if we remove the bug check some day
			}
		}

		
	// Check if the Object Exists	
		Status = ObpLookupObjectByName(
							Root,
							ObjectAttributes->ObjectName,
							&Context,
							NULL);

		if ( K_SUCCESS(Status)) {

		// We have found the Object, check if caller wanted to open it.
			if ( ObjectAttributes->Attributes & OBJ_OPENIF) {				

				ASSERT(ObIsValidObject( Context.Object));
				
				if ( Object != NULL) {
					*Object = Context.Object;
					ObReferenceObject( Object);
				}

				return Status;			
			}
		// Else Return Name Collision
		
			ObLog("Name Collision for Object %s\n", ObjectAttributes->ObjectName->Buffer);
			return STATUS_OBJECT_NAME_COLLISION;
		}

// If the Status is *NOT* STATUS_OBJECT_NAME_NOT_FOUND we will return the Status
		if(Status != STATUS_OBJECT_NAME_NOT_FOUND) {
				ObLog("Error for Object %s err: 0x%x\n", ObjectAttributes->ObjectName->Buffer, Status);
				return Status;
		}

		

	}


	
// The Object does not exist. Lets Create the object now.

//Size of Object plus header
	ObjectMemSize = sizeof(OBJECT_HEADER) + ObjectSize;

	ObjectHeader = (POBJECT_HEADER)	
							ExAllocatePoolWithTag( 
								ObjectType->PoolType, 
								ObjectMemSize, 
								ObjectType->PoolTag);

	if (ObjectHeader == NULL) {
		ObLog("Pool Allocation Failed for object\n");
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	// init the mem
	memset(ObjectHeader, 0, ObjectMemSize);

	ObjectHeader->Magic = OBJECT_HEADER_MAGIC;
//Object starts at ObjectHeader->Body
	LocalObject = &ObjectHeader->Body;

	if (ObjectHasName) {	

			ASSERT1(ObjectAttributes->ObjectName->Length <= MAX_OBJECT_PATH, 
				ObjectAttributes->ObjectName->Length);

			
			if (ObjectAttributes->ObjectName->Length > MAX_OBJECT_PATH) {
				ExFreePoolWithTag(ObjectHeader, 'tjbo');
				ObLog("Object Path too big %d\n",ObjectAttributes->ObjectName->Length);
				return STATUS_INVALID_PARAMETER;
			}


		ObLog("Inserting Object %s len %d\n",ObjectAttributes->ObjectName->Buffer, 
													ObjectAttributes->ObjectName->Length);

		// Insert the Object
		Status = ObpLookupObjectByName(Root,
							ObjectAttributes->ObjectName,
							&Context,
							ObjectHeader);

		if (!K_SUCCESS(Status)) {

			ObLog("Insertion Failed for Object %s err: 0x%x\n", ObjectAttributes->ObjectName->Buffer, Status);
			ExFreePoolWithTag(ObjectHeader, 'tjbo');
			return Status;
		}


		
		
	}

	if (ObjectAttributes ) {
		ObjectHeader->Attributes = ObjectAttributes->Attributes;
	}

	
	// set what type of object this is.
	ObjectHeader->ObjectType = ObjectType;

// Handle count will be incremented when we will create a handle for it
	ObjectHeader->OpenHandleCount = 0;

// Set the initial ref count to 1
	ObjectHeader->RefCount = 1;


// We might not be using it, buts still lets keep it right now.
	if (ObjectType->OpenProcedure) {
		Status = (ObjectType->OpenProcedure)(LocalObject);			

		if(!K_SUCCESS(Status)) {

			ExFreePoolWithTag(ObjectHeader, 'tjbo');
			return Status;
		}
	}

	//Insert the object in object type list
		KeAcquireSpinLock( &ObjectType->SpinLock, &OldIrql);
	
		LIST_INSERT_HEAD( &ObjectType->Object_List_Head, 
								ObjectHeader, 
								ObjectTypeListLink);
	
	
		
		KeReleaseSpinLock( &ObjectType->SpinLock, OldIrql);
	


	if(Object != NULL) {	
		*Object = LocalObject;
	}
	
	return STATUS_SUCCESS;

ErrorExit:



	return Status;
}



#if 0


KSTATUS
ObCreateObject( 
					IN KPROCESSOR_MODE ProbeMode, 
					IN POBJECT_TYPE ObjectType,
					IN POBJECT_ATTRIBUTES ObjectAttributes,
					IN ULONG ObjectSize,
					OUT PVOID *Object
					)
//ObCreateObject will set the refcount 1.

// If we just want to create the Handle to the Object and not use the object after that,
// then we need to dereference after the ObInsertObject.




{

	KSTATUS status = STATUS_SUCCESS;
	POBJECT_HEADER ObjectHeader = NULL;
	PVOID LocalObject;
	size_t ObjectMemSize = 0;
	KIRQL OldIrql;


	ASSERT(Object != NULL);
	ASSERT(ObjectSize > 0);

	// For user mode we need to probe and verify the user addresses
//	ASSERT(ProbeMode == KernelMode);

	ASSERT(ObjectType == &ObjectTypeList[ObjectType->TypeIndex]);
//	ASSERT(ObjectAttributes);


//Size of Object plus header
	ObjectMemSize = sizeof(OBJECT_HEADER) + ObjectSize;

	ObjectHeader = (POBJECT_HEADER)	
							ExAllocatePoolWithTag( 
								ObjectType->PoolType, 
								ObjectMemSize, 
								ObjectType->PoolTag);

	if (ObjectHeader == NULL) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	// init the mem
	memset(ObjectHeader, 0, ObjectMemSize);

	ObjectHeader->Magic = OBJECT_HEADER_MAGIC;
//Object starts at ObjectHeader->Body
	LocalObject = &ObjectHeader->Body;


	if (ObjectAttributes) {
		ASSERT( (ObjectAttributes->Attributes & ~OBJ_VALID_ATTRIBUTES) == 0);

		if (ObjectAttributes->ObjectName) {

			ASSERT1(ObjectAttributes->ObjectName->Length > OBJECT_NAME_STRING_SIZE, 
				ObjectAttributes->ObjectName->Length);

			
			if (ObjectAttributes->ObjectName->Length > OBJECT_NAME_STRING_SIZE) {
				ExFreePoolWithTag(ObjectHeader, 'tjbo');
				return STATUS_INVALID_PARAMETER;
			}


// Need to move this to lookaside list soon.
			ObjectHeader->ObjectName.Buffer = ObjectHeader->ObjNameString;
			ObjectHeader->ObjectName.Length = ObjectAttributes->ObjectName->Length;
			ObjectHeader->ObjectName.MaximumLength = MAX_OBJECT_NAME_SIZE;

			
			memcpy(ObjectHeader->ObjectName.Buffer,
						ObjectAttributes->ObjectName->Buffer,
						ObjectAttributes->ObjectName->Length);

		}

		ObjectHeader->Attributes = ObjectAttributes->Attributes;
		
		
	}


	
	// set what type of object this is.
	ObjectHeader->ObjectType = ObjectType;

// Handle count will be incremented when we will create a handle for it
	ObjectHeader->OpenHandleCount = 0;

// Set the initial ref count to 1
	ObjectHeader->RefCount = 1;


//Insert the object in object type list
	KeAcquireSpinLock( &ObjectType->SpinLock, &OldIrql);

	LIST_INSERT_HEAD( &ObjectType->Object_List_Head, 
							ObjectHeader, 
							ObjectTypeListLink);


	
	KeReleaseSpinLock( &ObjectType->SpinLock, OldIrql);





// We might not be using it, buts still lets keep it right now.
	if (ObjectType->OpenProcedure) {
		status = (ObjectType->OpenProcedure)(LocalObject);			

		if(!K_SUCCESS(status)) {
			ExFreePoolWithTag(ObjectHeader, 'tjbo');
			return status;
		}
	}



	if(Object != NULL) {	
		*Object = LocalObject;
	}
	
	return STATUS_SUCCESS;
}


#endif


VOID
Ob_init()
{


	OBJECT_ATTRIBUTES ObjectAttributes;
	UNICODE_STRING Name;
	KSTATUS Status;
	POBJECT_DIRECTORY  DirectoryObject;
	HANDLE DirHandle;

	ObpCreateDirectoryObjectType();
	ObpCreateSymbolicLinkObjectType();

//Create Root Directory Object
	RtlInitUnicodeString( &Name,"\\");
	InitializeObjectAttributes( &ObjectAttributes,
							&Name,OBJ_PERMANENT | OBJ_KERNEL_HANDLE, NULL, NULL);

	Status = ObCreateDirectoryObject(
				NULL,
				&DirectoryObject,
				0,
				&ObjectAttributes);
			

	if (!K_SUCCESS(Status)) {
		KeBugCheck("Root ObjectCreation Failed!!0x%x\n", Status);
	}

	ObRootDirectory = DirectoryObject;


// Create Global??
	RtlInitUnicodeString( &Name,"\\GLOBAL??");
	InitializeObjectAttributes( &ObjectAttributes,
						&Name,OBJ_PERMANENT | OBJ_KERNEL_HANDLE, NULL, NULL);

	Status = ObCreateDirectoryObject(
				NULL,
				&DirectoryObject,
				0,
				&ObjectAttributes);
			

	if (!K_SUCCESS(Status)) {
		KeBugCheck("GLOBAL?? ObjectCreation Failed!!0x%x\n", Status);
	}

	ObGlobalDirectory = DirectoryObject;



	// Create 
		RtlInitUnicodeString( &Name,"\\Driver");
		InitializeObjectAttributes( &ObjectAttributes,
							&Name,OBJ_PERMANENT | OBJ_KERNEL_HANDLE, DirHandle, NULL);
	
		Status = ObCreateDirectoryObject(
					NULL,
					&DirectoryObject,
					0,
					&ObjectAttributes);
				
	
		if (!K_SUCCESS(Status)) {
			KeBugCheck("Driver ObjectCreation Failed!!0x%x\n", Status);
		}
	


	// Create 
		RtlInitUnicodeString( &Name,"\\Device");
		InitializeObjectAttributes( &ObjectAttributes,
							&Name,OBJ_PERMANENT | OBJ_KERNEL_HANDLE, DirHandle, NULL);
	
		Status = ObCreateDirectoryObject(
					NULL,
					&DirectoryObject,
					0,
					&ObjectAttributes);
				
	
		if (!K_SUCCESS(Status)) {
			KeBugCheck("Driver ObjectCreation Failed!!0x%x\n", Status);
		}
	


	ObpPrintDirectoryTree(ObRootDirectory, 0);


}





