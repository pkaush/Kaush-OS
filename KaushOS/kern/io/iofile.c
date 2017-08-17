/*
*  C Implementation: IoFile.c
*
* Description: IO Manager's File Implementation
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/





#include "io.h"


#define IoFileLogsEnable	1



#if IoFileLogsEnable	
#define IoFileLog(fmt, args...) 		kprintf("[%s][%d]:", __FUNCTION__, __LINE__);kprintf(fmt, ## args)
#else
#define IoFileLog(fmt, args...)
#endif


KSTATUS
IoCreateFile(IN PCHAR Filename,
					IN FILE_MODE  FileMode,
					OUT PHANDLE pHandle)
{
	PDEVICE_OBJECT DeviceObject = NULL;
	KSTATUS Status;
	PIRP pIrp;
	PFILE_OBJECT pFileObj = NULL;
	OBJECT_ATTRIBUTES ObjectAttributes;
	PCHAR PathRemaining;


	
	Status = ObGetDeviceObject(Filename, &PathRemaining, &DeviceObject);

	if (!K_SUCCESS(Status)) {
		ASSERT1(0, Status);
		return Status;
	}

	IoFileLog("GOt the Device Object for file %s, rem path %s objct 0x%x\n", Filename, PathRemaining, DeviceObject);
	ASSERT(IoFileObjectType);


	InitializeObjectAttributes(&ObjectAttributes,
					NULL,OBJ_INHERIT,NULL, NULL);

	Status = ObCreateObject(	UserMode,
						IoFileObjectType,
						&ObjectAttributes,
						sizeof(FILE_OBJECT),
						&pFileObj);

	if (!K_SUCCESS(Status)) {
		ASSERT(0);
		return Status;
	}

	ASSERT (pFileObj);


	pFileObj->DeviceObject = DeviceObject;
	pFileObj->FileOffset = 0;
	pFileObj->FileMode = FileMode;
	strncpy(pFileObj->filename, PathRemaining, MAXPATHLEN);
	
	pIrp = IoBuildIrp(pFileObj,
					IRP_MJ_CREATE,
					IRP_MN_NULL,
					NULL, 0, 0);

		
	if (pIrp == NULL) {
		ObDereferenceObject( pFileObj);
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	

	Status = IoCallDriver(DeviceObject, pIrp);
		
	if (!K_SUCCESS(Status)) {	
		ObDereferenceObject( pFileObj);
		return Status;
	}



	Status = ObInsertObject(pFileObj, 0,1, pHandle);
		
	if (!K_SUCCESS(Status)) {	
		ObDereferenceObject( pFileObj);
		return Status;
	}

	IoFreeIRP(pIrp);
	return Status;
}












KSTATUS
KCreateFile( IN PCHAR Filename,
					IN FILE_MODE  FileMode,
					OUT PHANDLE pHandle)
/*
  * Filename - Filename with complete path
  *
  */


{
	return (IoCreateFile(Filename, FileMode, pHandle));
}



KSTATUS
IoCloseFile(IN PFILE_OBJECT FileObject)
{

	if (FileObject->FsContext== 0) {
		// Dereferensing the object where file hasn't been created will bring us here
		// so we will just return SUCCESS from Here.
		return STATUS_SUCCESS;
	}

	// need to clean up FileObj created in 
	return (IoFileOperations( FileObject, NULL, 0, 
					IRP_MJ_CLOSE, IRP_MN_NULL));


}


VOID
KCloseFile(IN HANDLE Handle)
{
	ObClose(Handle);
}






KSTATUS
KReadFile( IN HANDLE Handle,
					IN PVOID Buffer,
					IN ULONG NumberOfBytes
					)
{

	KSTATUS Status;
	PFILE_OBJECT FileObject;
	Status = ObReferenceObjectByHandle(Handle, 0, IoFileObjectType, UserMode, &FileObject, NULL);

	if (!K_SUCCESS(Status)) {
		ASSERT1(0, FileObject);

	}


	Status = IoFileOperations(FileObject, Buffer, 
							NumberOfBytes, 
							IRP_MJ_READ, 
							IRP_MN_NULL);


	ObDereferenceObject(FileObject);
	return Status;
}


KSTATUS
KWriteFile( IN HANDLE Handle,
					IN PVOID Buffer,
					IN ULONG NumberOfBytes
					)
{
	KSTATUS Status;
	PFILE_OBJECT FileObject;
	Status = ObReferenceObjectByHandle(Handle, 0, IoFileObjectType, UserMode, &FileObject, NULL);

	if (!K_SUCCESS(Status)) {
		ASSERT1(0, FileObject);

	}

	IoFileOperations(FileObject, Buffer, 
							NumberOfBytes, 
							IRP_MJ_WRITE, 
							IRP_MN_NULL);
	
	ObDereferenceObject(FileObject);
	return Status;
}



#if 0

KSTATUS
KQueryFileStats( IN HANDLE Handle,
							IN PVOID Buffer,
							IN ULONG NumberOfBytes
							)
{
	return (IoFileOperations(Handle, Buffer, 
							NumberOfBytes, 
							IRP_MJ_QUERY_INFORMATION, 
							IRP_MN_QUERY_STATS));
	
	ObDereferenceObject(FileObject);
	return Status;
}

KSTATUS
KQueryStats(IN PCHAR Filename, IN PVOID Buffer, ULONG NumberOfBytes)
{
	KSTATUS status;
	HANDLE Handle;

	if (NumberOfBytes < sizeof(FILE_STATS)) {
		KeBugCheck(" Buffer size less than expected\n");

	}
	
	status = KCreateFile(Filename, O_RDONLY, &Handle);
	if (!success(status)) {
		KeBugCheck("Wasn't able to open file %e\n", status);
		return status;
	}

	status = KQueryFileStats(Handle, Buffer, NumberOfBytes);
	if (!success(status)) {
		KeBugCheck("KQueryFileStats failed: %e\n", status);
		return status;
	}

	//close the file
	KCloseFile(Handle);

}


#endif





KSTATUS
IoCreateFileObjectType()
{


	OBJECT_TYPE ObjectType;

	memset (&ObjectType, 0, sizeof(OBJECT_TYPE));

	
	ObjectType.PoolType = NonPagedPool;
	ObjectType.FLAGS = 0;
	ObjectType.ObjectSize = sizeof(FILE_OBJECT);
	ObjectType.TypeIndex = OBJECT_TYPE_FILE;
	ObjectType.PoolTag = 'eliF';


	ObjectType.CloseProcedure = IoCloseFile;



	
	ObCreateObjectType(&ObjectType, &IoFileObjectType);

}


