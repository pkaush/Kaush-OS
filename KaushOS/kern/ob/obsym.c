/*
*  C Implementation:Obsym.c
*
* Description: Object Symbolink link implementation
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/


#include "ob.h"


#define ObSymLogsEnable 0



#if ObSymLogsEnable
#define ObSymLog(fmt, args...) 		kprintf("[%s][%d]:", __FUNCTION__, __LINE__);kprintf(fmt, ## args)
#else
#define ObSymLog(fmt, args...)
#endif

POBJECT_TYPE ObSymbolicLinkObjectType;





PVOID
ObGetTargetFromSymbolicLink(
				IN POBJECT_SYMBOLIC_LINK SymbolicLinkObject
				)
{

	return(SymbolicLinkObject->LinkTargetObject);


}



KSTATUS
ObpCreateSymbolicLinkObject(
								OUT PHANDLE Handle,
								OUT POBJECT_SYMBOLIC_LINK *SymbolicLinkObject,
								IN PVOID TargetObject,
								IN ACCESS_MASK DesiredAccess,
								IN POBJECT_ATTRIBUTES ObjectAttributes
								)
{

	PETHREAD Thread;
	POBJECT_SYMBOLIC_LINK Object;
	KSTATUS Status;
	UNICODE_STRING LocalDirName;
	KPROCESSOR_MODE PreviousMode;


	// A SymLink Object Will always be named
	ASSERT( (ObjectAttributes) && (ObjectAttributes->ObjectName));



	if (!ObRootDirectory) {
			KeBugCheck("Creating Symbolic Link Object before Creating Root Object\n");
		
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
						ObSymbolicLinkObjectType,
						ObjectAttributes,
						sizeof (OBJECT_DIRECTORY),
						&Object);
				
	if (!K_SUCCESS(Status)) {
		goto KCreateDirectoryObjectExit;
	}


// Initialize the Symlink Object

//Take a reference of the target before assigning

	ObReferenceObject(TargetObject);
	Object->LinkTargetObject = TargetObject;





	if (SymbolicLinkObject) {

		*SymbolicLinkObject = Object;
	}

// If the Caller has requested a handle then Insert it.

	if (Handle) {

		Status = ObInsertObject(Object, DesiredAccess,
										0, Handle);

		if ( !K_SUCCESS(Status)) {
			ObDereferenceObject(Object);
		}
	}
	
KCreateDirectoryObjectExit:

	return Status;
}



KSTATUS
ObCreateSymbolicLink(
					IN PUNICODE_STRING SymbolicLinkName,
					IN PUNICODE_STRING TargetName
					)
{
	PVOID Object;
	OBP_LOOKUP_CONTEXT Context;
	KSTATUS Status;
	OBJECT_ATTRIBUTES ObjectAttributes;


	ObInitializeLookupContext( &Context);

	Status = ObpLookupObjectByName(
				ObRootDirectory, TargetName, &Context,NULL);

	if (!K_SUCCESS(Status)) {
		ObpPrintDirectoryTree(ObRootDirectory,0);
		ASSERT1(0, Status);
		return Status;
	}


	InitializeObjectAttributes( &ObjectAttributes, SymbolicLinkName, 0, NULL,NULL);

	Status = ObpCreateSymbolicLinkObject(NULL,NULL,Context.Object, 0,&ObjectAttributes);



	return Status;
}





VOID
ObpCreateSymbolicLinkObjectType()
{

	OBJECT_TYPE SymLinkType;

	memset(&SymLinkType, 0, sizeof (OBJECT_TYPE));

	SymLinkType.ObjectSize = sizeof(OBJECT_SYMBOLIC_LINK);
	SymLinkType.PoolTag = NonPagedPool;
	SymLinkType.TypeIndex = OBJECT_TYPE_SYMLINK;
	KeInitializeSpinLock( &SymLinkType.SpinLock);
	LIST_INIT( &SymLinkType.Object_List_Head);

	ObCreateObjectType( &SymLinkType,
						&ObSymbolicLinkObjectType);

}





