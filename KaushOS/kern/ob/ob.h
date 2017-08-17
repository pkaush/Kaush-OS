/*
*  C Interface:	Ob.h
*
* Description: Handle table and stuff also known as VFS in Linux
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/


#ifndef _KOS_KERN_OB_OB_H_
#define _KOS_KERN_OB_OB_H_

//#include <kern/kern.h>
#include <inc/types.h>
#include <inc/file.h>
#include <kern/ke/ke.h>
#include <kern/mm/mm.h>
#include <kern/ps/process.h>






#define MAX_OBJECT_NAME_SIZE	128
#define MAX_OBJECT_PATH		1024
#define IsPathSeperator(x) (((x) == '\\') ||((x) == '/'))





//
// Valid values for the Attributes field
//

#define OBJ_INHERIT             0x00000002L
#define OBJ_PERMANENT           0x00000010L
#define OBJ_EXCLUSIVE           0x00000020L
#define OBJ_CASE_INSENSITIVE    0x00000040L
#define OBJ_OPENIF              0x00000080L
#define OBJ_OPENLINK            0x00000100L
#define OBJ_KERNEL_HANDLE       0x00000200L
#define OBJ_FORCE_ACCESS_CHECK  0x00000400L
#define OBJ_VALID_ATTRIBUTES    0x000007F2L


typedef struct _OBJECT_ATTRIBUTES {
    ULONG Length;
    HANDLE RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;        // Points to type SECURITY_DESCRIPTOR
    PVOID SecurityQualityOfService;  // Points to type SECURITY_QUALITY_OF_SERVICE
} OBJECT_ATTRIBUTES;
typedef OBJECT_ATTRIBUTES *POBJECT_ATTRIBUTES;




//++
//
// VOID
// InitializeObjectAttributes(
//     OUT POBJECT_ATTRIBUTES p,
//     IN PUNICODE_STRING n,
//     IN ULONG a,
//     IN HANDLE r,
//     IN PSECURITY_DESCRIPTOR s
//     )
//
//--

#define InitializeObjectAttributes( p, n, a, r, s ) { \
    (p)->Length = sizeof( OBJECT_ATTRIBUTES );          \
    (p)->RootDirectory = r;                             \
    (p)->Attributes = a;                                \
    (p)->ObjectName = n;                                \
    (p)->SecurityDescriptor = s;                        \
    (p)->SecurityQualityOfService = NULL;               \
    }



//typedef struct _OBJECT_HEADER_NAME_INFO
//{
//	POBJECT_DIRECTORY Directory;
//	UNICODE_STRING Name;
//	ULONG QueryReferences;
//} OBJECT_HEADER_NAME_INFO, *POBJECT_HEADER_NAME_INFO;





//
// Object Directory Structures
//
typedef struct _OBJECT_DIRECTORY_ENTRY
{
	LIST_ENTRY DirEntryLink;
} OBJECT_DIRECTORY_ENTRY, *POBJECT_DIRECTORY_ENTRY;




typedef struct _OBJECT_DIRECTORY
{
	struct _OBJECT_DIRECTORY_ENTRY Head;
	ULONG Flags;
	PVOID DeviceObject;

	// We will use spinlock here as the time taken under the lock 
	//will be less then what it takes to block and unblock
	KSPIN_LOCK SpinLock; 
} OBJECT_DIRECTORY, *POBJECT_DIRECTORY;


#define OB_DIRECTORY_MOUNTPOINT		0x1



typedef struct _OBJECT_SYMBOLIC_LINK
{
	PVOID LinkTargetObject;
	ULONG DosDeviceDriveIndex;
} OBJECT_SYMBOLIC_LINK, *POBJECT_SYMBOLIC_LINK;




typedef
KSTATUS
OBJECT_METHODS(
				IN PVOID Object
				);

typedef OBJECT_METHODS *POBJECT_METHODS;



//list of the objects from Object type
typedef LIST_HEAD(Object_Type_List, _OBJECT_HEADER)		OBJECT_TYPE_LIST,
																	*POBJECT_TYPE_LIST; 


#define	OB_OBJECT_TYPE_INFO_FLAGS_OBJECT_SYNCRONIZABLE	0x1


typedef enum _OBJECT_TYPE_INDEX {
	OBJECT_TYPE_FILE,
	OBJECT_TYPE_DIRECTORY,
	OBJECT_TYPE_SYMLINK,
	OBJECT_TYPE_DEVICE,
	OBJECT_TYPE_DRIVER,
	OBJECT_TYPE_PROCESS,
	OBJECT_TYPE_THREAD,
	OBJECT_TYPE_SECTION,
	OBJECT_TYPE_EVENT,
	OBJECT_TYPE_MUTEX,
	OBJECT_TYPE_SEMAPHORE,
	OBJECT_TYPE_TIMER,
	OBJECT_TYPE_LAST

}OBJECT_TYPE_INDEX, *POBJECT_TYPE_INDEX;


#define OBJECT_HEADER_MAGIC 0x12121212




typedef struct _OBJECT_TYPE {

	
	POOL_TYPE PoolType;
	ULONG FLAGS;
	ULONG ObjectSize;
	KSPIN_LOCK SpinLock;
	ULONG TypeIndex;
	ULONG PoolTag;

	OBJECT_TYPE_LIST	Object_List_Head;
	
	POBJECT_METHODS OpenProcedure;
	POBJECT_METHODS CloseProcedure;
	POBJECT_METHODS DeleteProcedure;
	POBJECT_METHODS ParseProcedure;
	POBJECT_METHODS SecurityProcedure;
	POBJECT_METHODS QueryNameProcedure;
	POBJECT_METHODS OkayToCloseProcedure;

} OBJECT_TYPE, * POBJECT_TYPE;


typedef struct _OBJECT_HANDLE_INFORMATION {
	ULONG HandleAttributes;
	ACCESS_MASK GrantedAccess;
} OBJECT_HANDLE_INFORMATION, *POBJECT_HANDLE_INFORMATION;







typedef struct _OBJECT_HEADER {

	ULONG Magic;//;-)
		//used to list in Object type list
	KLIST_ENTRY(_OBJECT_HEADER) ObjectTypeListLink;

	OBJECT_DIRECTORY_ENTRY DirectoryEntry;
	POBJECT_TYPE ObjectType;

	LONG OpenHandleCount;
	LONG RefCount;						// Reference Count
	POBJECT_DIRECTORY DirectoryObject;  // Pointer to the Directory Object if we are inserted, else NULL
	UNICODE_STRING ObjectName;		// Name of the object if Named Object
	ULONG Attributes;
	
	ULONG Body;

} OBJECT_HEADER, *POBJECT_HEADER;




//#define OBJECT_TYPE_INFO_FLAGS_SYNCRONIZABLE_OBJECT	0X1


extern POBJECT_TYPE ObDirectoryObjectType;
extern POBJECT_TYPE ObSymbolicLinkObjectType;
extern POBJECT_TYPE IoDeviceObjectType;
extern POBJECT_TYPE IoDriverObjectType;
extern POBJECT_TYPE IoFileObjectType;



extern POBJECT_DIRECTORY ObRootDirectory;
extern POBJECT_DIRECTORY ObGlobalDirectory;


#define OBJECT_TO_OBJECT_HEADER(o)                          \
    CONTAINING_RECORD((o), OBJECT_HEADER, Body)






FORCEINLINE
BOOL
ObIsValidObject(IN PVOID Object)
{

	POBJECT_HEADER ObjectHeader;

	ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);
	return(ObjectHeader->Magic == OBJECT_HEADER_MAGIC);
}

FORCEINLINE
BOOLEAN
ObIsObjectInheritable(PVOID Object)
{
	POBJECT_HEADER Header;
	ASSERT(ObIsValidObject(Object));
	Header = OBJECT_TO_OBJECT_HEADER(Object);
	return (Header->Attributes & OBJ_INHERIT);
}


FORCEINLINE
BOOL
ObIsDirectoryObject( IN PVOID Object)
{
	POBJECT_HEADER ObjectHeader = (POBJECT_HEADER) OBJECT_TO_OBJECT_HEADER(Object);
	ASSERT(ObIsValidObject(Object));
	ASSERT(ObDirectoryObjectType);
	return (ObjectHeader->ObjectType == ObDirectoryObjectType);
}


FORCEINLINE
BOOL
ObIsMountpointObject( IN PVOID Object)
{
	POBJECT_HEADER ObjectHeader = (POBJECT_HEADER) OBJECT_TO_OBJECT_HEADER(Object);
	ASSERT(ObIsValidObject(Object));
	ASSERT(ObDirectoryObjectType);
	
	return ((ObjectHeader->ObjectType == ObDirectoryObjectType) &&
		(((POBJECT_DIRECTORY)Object)->Flags & OB_DIRECTORY_MOUNTPOINT));
}




FORCEINLINE
BOOL
ObIsSymbolicLinkObject( IN PVOID Object)
{
	POBJECT_HEADER ObjectHeader = (POBJECT_HEADER) OBJECT_TO_OBJECT_HEADER(Object);
	ASSERT(ObIsValidObject(Object));
	ASSERT(ObSymbolicLinkObjectType);
	return (ObjectHeader->ObjectType == ObSymbolicLinkObjectType);
}


FORCEINLINE
BOOL
ObIsDeviceObject( IN PVOID Object)
{
	POBJECT_HEADER ObjectHeader = (POBJECT_HEADER) OBJECT_TO_OBJECT_HEADER(Object);
	ASSERT(ObIsValidObject(Object));
	ASSERT(IoDeviceObjectType);
	return (ObjectHeader->ObjectType == IoDeviceObjectType);
}

typedef struct _OBP_LOOKUP_CONTEXT
{
	POBJECT_DIRECTORY Directory;
	PVOID Object;
	POBJECT_DIRECTORY LastMountPoint;
	PCHAR ParsedTill;
	BOOLEAN DirectoryLocked;
	KIRQL OldIrql;

} OBP_LOOKUP_CONTEXT, *POBP_LOOKUP_CONTEXT;


FORCEINLINE
VOID
ObInitializeLookupContext(POBP_LOOKUP_CONTEXT Context)
{

	memset(Context, 0, sizeof(OBP_LOOKUP_CONTEXT));

}










/////////////////////////
/// Object Name

FORCEINLINE
KSTATUS
ObpAllocateObjectNameBuffer(IN ULONG Length,
                            IN BOOLEAN UseLookaside,
                            IN OUT PUNICODE_STRING ObjectName)

{

	ASSERT(Length <= MAX_OBJECT_NAME_SIZE);

	
	ObjectName->Buffer = ExAllocatePoolWithTag(NonPagedPool,
									MAX_OBJECT_NAME_SIZE, 'mNbO');

	if ( !ObjectName->Buffer) {
		ObjectName->Length = 0;
		ObjectName->MaximumLength = 0;
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	ObjectName->Length = 0;
	ObjectName->MaximumLength = MAX_OBJECT_NAME_SIZE;


	return  STATUS_SUCCESS;
}




FORCEINLINE
VOID
ObpFreeObjectNameBuffer(IN PUNICODE_STRING ObjectName)
{
	ExFreePoolWithTag(ObjectName->Buffer, 'mNbO');
	ObjectName->Buffer = NULL;
	ObjectName->MaximumLength = 0;
	ObjectName->Length = 0;
}




FORCEINLINE
KSTATUS
ObpAllocateObjectPathNameBuffer(
				IN ULONG Length,
                            IN BOOLEAN UseLookaside,
                            IN OUT PUNICODE_STRING ObjectName)

{

	ASSERT(Length <= MAX_OBJECT_PATH);
	ASSERT(UseLookaside == TRUE);

	
	ObjectName->Buffer = ExAllocatePoolWithTag(NonPagedPool,
									MAX_OBJECT_PATH, 'hPbO');

	if ( !ObjectName->Buffer) {
		ObjectName->Length = 0;
		ObjectName->MaximumLength = 0;
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	ObjectName->Length = Length;
	ObjectName->MaximumLength = MAX_OBJECT_PATH;


	return  STATUS_SUCCESS;
}




FORCEINLINE
VOID
ObpFreeObjectPathNameBuffer(IN PUNICODE_STRING ObjectName)
{
	ExFreePoolWithTag(ObjectName->Buffer, 'hPbO');
	ObjectName->Buffer = NULL;
	ObjectName->MaximumLength = 0;
	ObjectName->Length = 0;
}




#endif
