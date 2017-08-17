/*
*  C Implementation:Obname.c
*
* Description: Object name handling
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/


#include "ob.h"


#define ObNameLogsEnable 1



#if ObNameLogsEnable
#define ObNameLog(fmt, args...) 		kprintf("[%s][%d]:", __FUNCTION__, __LINE__);kprintf(fmt, ## args)
#else
#define ObNameLog(fmt, args...)
#endif


static INLINE
BOOLEAN
ObParseDirectoryPath(IN PCHAR Name, 
										OUT PUNICODE_STRING DirName, 
										OUT PCHAR *ParsedTill)
										
{
	PCHAR StartPtr, EndPtr;
	ULONG Length;
	StartPtr = Name;


//	ObNameLog("ObParseDirectoryPath: Name %s %d\n", Name, StartPtr[0]);

	while ( (*StartPtr != '\0') && (IsPathSeperator(*StartPtr)) ) {
		StartPtr++;
//		ObNameLog("s%s\n",StartPtr);
	}


	if (*StartPtr == '\0') {
		return FALSE;
	}
	
	EndPtr = StartPtr+1;

	
	while ( (*EndPtr != '\0') && (!IsPathSeperator(*EndPtr)) ) {
			EndPtr++;
//			ObNameLog("e%s\n",EndPtr);
	}

	Length = EndPtr - StartPtr;

	ASSERT2(Length <= DirName->MaximumLength, Length, DirName->MaximumLength);


	strncpy(DirName->Buffer, StartPtr, min(Length, DirName->MaximumLength));

// Null Terminate the string here, if the EndPtr has hit the '\0' then it will be NULL terminated anyways
// else this location might contain a '\'

	DirName->Buffer[Length] = '\0'; 
	DirName->Length = Length;
//	ObNameLog("ObParseDirectoryPath: DirName %s\n", DirName);


	if (ParsedTill) {
		*ParsedTill = EndPtr;
	}


	return TRUE;
}




KSTATUS
ObpLookupObjectByName(
								IN POBJECT_DIRECTORY Parent,
								IN PUNICODE_STRING Name,
								IN POBP_LOOKUP_CONTEXT Context,
								IN POBJECT_HEADER InsertObjectHeader
								)
{
	PCHAR NamePtr;
	PCHAR ParsedTill;
	UNICODE_STRING DirName;
	KSTATUS Status;
	BOOLEAN IsNext;
	POBJECT_DIRECTORY Directory;
	OBP_LOOKUP_CONTEXT LocalContext;


	if (ObRootDirectory == NULL)  {

			KeBugCheck("Performing named Object Operation before initailizing Root Directory\n");			
	} 


	ASSERT(Parent);

	ObNameLog("Starting With Parent %s\n", OBJECT_TO_OBJECT_HEADER(Parent)->ObjectName.Buffer);
	ObNameLog("Name: %s\n", Name->Buffer);


	if (Context == NULL) {

		Context = &LocalContext;
		ObInitializeLookupContext(Context);
	}


	Directory = Parent;

	Status =  ObpAllocateObjectNameBuffer(
					MAX_OBJECT_NAME_SIZE,
					TRUE, &DirName);



	if (!K_SUCCESS(Status)) {
		return Status;
	}


	NamePtr = Name->Buffer;
	ParsedTill = NamePtr;

	do {

		Context->ParsedTill = ParsedTill;

		if ( !ObIsDirectoryObject(Directory)) {

			ObNameLog("Not Dir Object %s\n", OBJECT_TO_OBJECT_HEADER(Directory)->ObjectName.Buffer);


			Context->Directory = Directory; //Dir till where we were able to parse
			Context->Object = NULL;

// We need to Put Symlink support in here

			Status = STATUS_OBJECT_PATH_NOT_FOUND;
			goto ExitObpLookupObjectByName;
		}



		IsNext = ObParseDirectoryPath(NamePtr, 
							&DirName, 
							&ParsedTill);



		ObNameLog("DirName %s %d\n", DirName.Buffer, IsNext);


// If a Name does not end with a Path seperator
// then ParsedTill will point to '\0' at the end.

		NamePtr = ParsedTill;

		if (IsNext) {
		
			Status = ObpLookupEntryDirectory(
										Directory,
										&DirName,
										Context);

			if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {


//We should copy the ParsedTill before checking, else the ObParseDirectoryPath will modify it.
//				Context->ParsedTill = ParsedTill;
				Context->Directory = Directory; //Dir where the object should be
				Context->Object = NULL;

// If this Object, which is not found, if it is *not* the last entry in the ObjectPath Given
// Then the return value should be PATH NOT FOUND
				IsNext = ObParseDirectoryPath(NamePtr, 
								&DirName, 
								&ParsedTill);

				ObNameLog("Object Not Found - parsed to see if last; DirName %s %d\n", DirName.Buffer, IsNext);

				if (IsNext) {

					ObNameLog("Pathnotfound DirName %s %d\n", DirName.Buffer, IsNext);

					Status = STATUS_OBJECT_PATH_NOT_FOUND;
					goto ExitObpLookupObjectByName;
				}
// Else it was the last entry set the context accordingly


				if (InsertObjectHeader) {
					// Caller wants us to insert the Object

					//Set the object name. It should be the last entry in the Path
					InsertObjectHeader->ObjectName.Buffer = DirName.Buffer;
					InsertObjectHeader->ObjectName.MaximumLength = DirName.MaximumLength;
					InsertObjectHeader->ObjectName.Length = DirName.Length;

					ObNameLog("Inserting in Directory DirName %s\n", DirName.Buffer);	
					ObpInsertEntryDirectory( Directory, Context, InsertObjectHeader);

					Status = STATUS_SUCCESS;

				}

			}

// For any other error scenario
			if ( !K_SUCCESS(Status)) {
				goto ExitObpLookupObjectByName;
			} 


			Directory = Context->Object;
		}
		

	} while((IsNext) && (*NamePtr));


ExitObpLookupObjectByName:
	
// Free the Buffer if we have not used it for Object Name
	if ((!InsertObjectHeader) || (Status != STATUS_SUCCESS)){
		ObpFreeObjectNameBuffer(&DirName);
	}

	return Status;
}





KSTATUS
ObGetDeviceObject(PCHAR Path,
								PCHAR *PathRemaining,
								PVOID *DeviceObject)
{

	OBP_LOOKUP_CONTEXT Context;
	KSTATUS Status;
	POBJECT_DIRECTORY DirectoryObject;
	POBJECT_SYMBOLIC_LINK SymLink;
	PVOID Object;
	UNICODE_STRING PathName;
	ObInitializeLookupContext( &Context);

	PathName.Buffer = Path;
	PathName.Length = strlen(Path);
	PathName.MaximumLength = PathName.Length;

	
	Status = ObpLookupObjectByName(ObGlobalDirectory,
							&PathName, &Context,NULL);

	ObNameLog("Path %s, Status 0x%x Dir 0x%x Object 0x%x parsedtill %s\n", Path, Status, 
														Context.Directory, Context.Object, Context.ParsedTill);

	if (Status == STATUS_SUCCESS) {
		Object = Context.Object;

	} else if ( (Status == STATUS_OBJECT_NAME_NOT_FOUND) 
						||
			(Status == STATUS_OBJECT_PATH_NOT_FOUND) ) {

		Object = Context.Directory;

	}



/*
			Is Object a Directory
				Check if it is a mountpoint
					return the mountpoint Device
*/
		if ( ObIsDirectoryObject(Object)) {
			ObNameLog("Object 0x%x is DirObject\n", Object);

			if (ObIsMountpointObject(Object)) {
				ObNameLog("Object 0x%x is MountPOint\n", Object);
				DirectoryObject = Object;
				*DeviceObject =  DirectoryObject->DeviceObject;
				*PathRemaining = Context.ParsedTill;
				return STATUS_SUCCESS;
			}
			ObpPrintDirectoryTree( ObRootDirectory, 0);
			ASSERT(0);
		}

/*
		Is Object a SYMLINK 
			Check if the symlink is to a device
				return the device
*/
		else if (ObIsSymbolicLinkObject(Object)) {
			ObNameLog("Object 0x%x is SymLink\n", Object);

			SymLink = Object;
			if (ObIsDeviceObject(SymLink->LinkTargetObject)) {
				ObNameLog("Object 0x%x is Symlink to Device\n", Object);


				*DeviceObject = SymLink->LinkTargetObject;
				*PathRemaining = Context.ParsedTill;
				return STATUS_SUCCESS;
			}

			
			ASSERT(0);
		}

}



