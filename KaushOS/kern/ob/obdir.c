/*
*  C Implementation:Obdir.c
*
* Description: Directory Object Implementation
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/

#include "ob.h"

POBJECT_TYPE ObDirectoryObjectType;
POBJECT_DIRECTORY ObRootDirectory;
POBJECT_DIRECTORY ObGlobalDirectory;






#define ObDirLogsEnable 1


#if ObDirLogsEnable
#define ObDirLog(fmt, args...) 		kprintf("[%s][%d]:", __FUNCTION__, __LINE__);kprintf(fmt, ## args)
#else
#define ObDirLog(fmt, args...)
#endif



FORCEINLINE
VOID
ObpAcquireDirectoryLockExclusive(IN POBJECT_DIRECTORY Directory,
                                  IN POBP_LOOKUP_CONTEXT Context)
{

	KeAcquireSpinLock( &Directory->SpinLock, &Context->OldIrql);
	Context->DirectoryLocked = TRUE;



}

FORCEINLINE
VOID
ObpAcquireDirectoryLockShared(IN POBJECT_DIRECTORY Directory,
                               IN POBP_LOOKUP_CONTEXT Context)
{

	KeAcquireSpinLock( &Directory->SpinLock, &Context->OldIrql);
	Context->DirectoryLocked = TRUE;


}


FORCEINLINE
VOID
ObpReleaseDirectoryLock(IN POBJECT_DIRECTORY Directory,
                         IN POBP_LOOKUP_CONTEXT Context)
{

	Context->DirectoryLocked = FALSE;
	KeReleaseSpinLock(&Directory->SpinLock, Context->OldIrql);


}

LONG
RtlCompareUnicodeString(
								IN PCUNICODE_STRING Str1,
								IN PCUNICODE_STRING Str2,
								IN BOOLEAN  CaseInsensitive);



KSTATUS
ObpLookupEntryDirectory(IN POBJECT_DIRECTORY Directory,
                        IN PUNICODE_STRING Name,
                        IN POBP_LOOKUP_CONTEXT Context)
{

	POBJECT_DIRECTORY_ENTRY DirEntry;
	POBJECT_HEADER ObjectHeader;
	PLIST_ENTRY ListPtr;
	DirEntry = &Directory->Head;
	LONG Result = -1;
	BOOL UnLock = FALSE;
	

	if (!Context->DirectoryLocked) {
		UnLock = TRUE;
		ObpAcquireDirectoryLockShared(Directory, Context);
	}

	ObDirLog("Parsing through the children of %s looking for %s\n", 
							OBJECT_TO_OBJECT_HEADER(Directory)->ObjectName.Buffer,
							Name->Buffer);

	ObDirLog("0x%x 0x%x\n", DirEntry->DirEntryLink.Flink, &DirEntry->DirEntryLink);
// Loop through all the objects in current Directory
	for(ListPtr = DirEntry->DirEntryLink.Flink; ListPtr != &DirEntry->DirEntryLink; ListPtr = ListPtr->Flink) {

		ObjectHeader = CONTAINING_RECORD(ListPtr, OBJECT_HEADER, DirectoryEntry.DirEntryLink);

		ObDirLog("%s\n", ObjectHeader->ObjectName.Buffer);

	ObDirLog("Length %s %d & %s %d\n", Name->Buffer, 
				Name->Length, ObjectHeader->ObjectName.Buffer, ObjectHeader->ObjectName.Length);

//Check if it is the object we are looking for
		Result = RtlCompareUnicodeString(Name, &ObjectHeader->ObjectName, FALSE);
//		Result = strncmp(Name->Buffer, ObjectHeader->ObjectName.Buffer, Name->Length);
		ObDirLog("Comparing %s & %s result %d\n", Name->Buffer, ObjectHeader->ObjectName.Buffer, Result);

		if (Result == 0) {
// If it is then set the context
			Context->Directory = Directory;
			Context->Object = &ObjectHeader->Body;

			if (ObIsMountpointObject(&ObjectHeader->Body)) {
				Context->LastMountPoint = &ObjectHeader->Body;
			}


			break;
		}
	}

	if (UnLock) {
//Only Unlock if we have locked it
		ObpReleaseDirectoryLock( Directory, Context);
	}


	if (Result != 0) {
		return STATUS_OBJECT_NAME_NOT_FOUND;
	}

	return STATUS_SUCCESS;

}



BOOLEAN
ObpInsertEntryDirectory(IN POBJECT_DIRECTORY Parent,
                        IN POBP_LOOKUP_CONTEXT Context,
                        IN POBJECT_HEADER ObjectHeader)
{

	BOOLEAN UnLock = FALSE;
	
	ASSERT(ObIsDirectoryObject(Parent));
	ASSERT(ObjectHeader->DirectoryEntry.DirEntryLink.Flink == NULL);
	ASSERT(Context);
	ASSERT(Context->Directory == Parent);

	if ( !Context->DirectoryLocked) {

		ObpAcquireDirectoryLockExclusive(Parent,	Context);
		UnLock = TRUE;
	}


	ObDirLog("Inserting DirEntry Parent %s Obj %s\n", OBJECT_TO_OBJECT_HEADER(Parent)->ObjectName.Buffer,
																				ObjectHeader->ObjectName.Buffer);

	InsertTailList(
				   &Parent->Head.DirEntryLink,
				   &ObjectHeader->DirectoryEntry.DirEntryLink
				   );


	if (UnLock) {

		ObpReleaseDirectoryLock( Parent, Context);
	}


	return TRUE;
}


KSTATUS
ObCreateDirectoryObject(
								OUT PHANDLE DirectoryHandle,
								OUT POBJECT_DIRECTORY *DirectoryObject,
								IN ACCESS_MASK DesiredAccess,
								IN POBJECT_ATTRIBUTES ObjectAttributes
								)
{

	PETHREAD Thread;
	POBJECT_DIRECTORY Object;
	KSTATUS Status;
	UNICODE_STRING LocalDirName;
	BOOLEAN CreatingRootObject = FALSE;
	KPROCESSOR_MODE PreviousMode;


	// A directory Object Will always be named
	ASSERT( (ObjectAttributes) && (ObjectAttributes->ObjectName));



	if (!ObRootDirectory) {

		if ( (ObjectAttributes) && (ObjectAttributes->ObjectName)) {
			RtlInitUnicodeString( &LocalDirName, "\\");
			if (!RtlCompareUnicodeString(ObjectAttributes->ObjectName, &LocalDirName, FALSE)) {


			//While creating the Root Object, create a unname Object 
			//so that ObCreateObject doesn't try to push us somewhere

				CreatingRootObject = TRUE;
				ObjectAttributes->ObjectName = NULL;



			} else {
				KeBugCheck("Creating Directory Object before Creating Root Object\n");
			}
		} else {
			KeBugCheck("Creating Directory Object before Creating Root Object 1\n");
		}
		
	}

//Some early objects will be created before current thread is set

	Thread = KeGetCurrentThread();
	if (Thread == NULL) {
		PreviousMode = KernelMode;
	} else {
		PreviousMode = Thread->tcb.PreviousMode;
	}


//Create the Object	
	Status = ObCreateObject(
						PreviousMode,
						ObDirectoryObjectType,
						ObjectAttributes,
						sizeof (OBJECT_DIRECTORY),
						&Object);
				
	if (!K_SUCCESS(Status)) {
		goto KCreateDirectoryObjectExit;
	}


// Initialize the Directory Object

	Object->Flags = 0;
	InitializeListHead (&Object->Head.DirEntryLink);
	KeInitializeSpinLock( &Object->SpinLock);





	if (CreatingRootObject) {
		POBJECT_HEADER ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);


		
		Status = ObpAllocateObjectNameBuffer(
							LocalDirName.Length,
							FALSE,
							&ObjectHeader->ObjectName);

		if (!K_SUCCESS(Status)) {

			KeBugCheck("Name Buffer Allocation failed while creating RootDirectory");
		}

		RtlCopyUnicodeString(&ObjectHeader->ObjectName, &LocalDirName);

	}



	if (DirectoryObject) {

		*DirectoryObject = Object;
	}

// If the Caller has requested a handle then Insert it.

	if (DirectoryHandle) {

		Status = ObInsertObject(Object, DesiredAccess,
										0, DirectoryHandle);

		if ( !K_SUCCESS(Status)) {
			ObDereferenceObject(Object);
		}
		
	}
	
	

KCreateDirectoryObjectExit:


	return Status;

}





VOID
ObpPrintDirectoryTree(POBJECT_DIRECTORY Root, INT Level)
{

	PLIST_ENTRY ListPtr;
	POBJECT_DIRECTORY_ENTRY DirEntry;
	POBJECT_HEADER ObjectHeader;
	INT i;


	ASSERT(ObIsDirectoryObject(Root));
	
	DirEntry = &Root->Head;


	if (Level == 0) {
		kprintf("%s\n", OBJECT_TO_OBJECT_HEADER(Root)->ObjectName.Buffer);
	}

// Loop through all the objects in current Directory
	for(ListPtr = DirEntry->DirEntryLink.Flink; ListPtr != &DirEntry->DirEntryLink; ListPtr = ListPtr->Flink) {

		ObjectHeader = CONTAINING_RECORD(ListPtr, OBJECT_HEADER, DirectoryEntry.DirEntryLink);

		for (i = 0; i <= Level; i++) {
			kprintf("\t");
		}

		kprintf("|________%s\n", ObjectHeader->ObjectName.Buffer);
		if (ObIsDirectoryObject(&ObjectHeader->Body)) {

			ObpPrintDirectoryTree(&ObjectHeader->Body, Level+1);
		}
	}

}




KSTATUS
ObMountFileSystem(
					IN PUNICODE_STRING MountPoint,
					IN PUNICODE_STRING TargetName
					)
{
	PVOID Object;
	OBP_LOOKUP_CONTEXT Context;
	KSTATUS Status;
	OBJECT_ATTRIBUTES ObjectAttributes;
	PVOID TargetObject;
	POBJECT_DIRECTORY DirObject;

	
	// Initialize our context
	ObInitializeLookupContext(&Context);


	// Lookup the Taret objet first
	Status = ObpLookupObjectByName(
				ObRootDirectory, TargetName, &Context,NULL);

	if (!K_SUCCESS(Status)) {
		ObDirLog("LookUp Failed for TargetName %s\n", TargetName->Buffer);
		ObpPrintDirectoryTree(ObRootDirectory,0);
		ASSERT1(0, Status);
		return Status;
	}

	TargetObject = Context.Object;

//To create a mount point target object has to be a device object
	if (!ObIsDeviceObject(TargetObject)) {

		return STATUS_INVALID_DEVICE_OBJECT_PARAMETER;
	}


	// ReInitialize our context to search for mount Directory
	ObInitializeLookupContext(&Context);


	// Lookup the MountPoint
	Status = ObpLookupObjectByName(
				ObRootDirectory, MountPoint, &Context,NULL);

	if (!K_SUCCESS(Status)) {
		ObDirLog("LookUp Failed for MountPoint %s\n", MountPoint->Buffer);
		ObpPrintDirectoryTree(ObRootDirectory,0);
		ASSERT1(0, Status);
		return Status;
	}
	DirObject = Context.Object;


	// Check if some FS is already mounted on this
	if (ObIsMountpointObject(DirObject)) {
		ObDirLog("MountPoint is already Mounted %s with Device 0x%x\n",
						MountPoint->Buffer, DirObject->DeviceObject);

		return STATUS_OBJECT_NAME_EXISTS;
	}


	DirObject->Flags |= OB_DIRECTORY_MOUNTPOINT;
	DirObject->DeviceObject = TargetObject;

	return Status;
}






VOID
ObpCreateDirectoryObjectType()
{

	OBJECT_TYPE DirectoryType;

	memset(&DirectoryType, 0, sizeof (OBJECT_TYPE));

	DirectoryType.ObjectSize = sizeof(OBJECT_DIRECTORY);
	DirectoryType.PoolTag = NonPagedPool;
	DirectoryType.TypeIndex = OBJECT_TYPE_DIRECTORY;
	KeInitializeSpinLock( &DirectoryType.SpinLock);
	LIST_INIT( &DirectoryType.Object_List_Head);

	ObCreateObjectType( &DirectoryType,
						&ObDirectoryObjectType);

}





