/*
*  C Implementation:  FsInterface.c
*
* Description: File Systems interface to IoManager
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/




//#include <kern/mm/mm.h>
#include "ff.h"
#include <kern/io/Io.h>

#define KOSFSLOGS 1



#if KOSFSLOGS
#define KosFsLog(fmt, args...) 		kprintf("[%s][%d]:", __FUNCTION__, __LINE__);kprintf(fmt, ## args)
#else
#define KosFsLog(fmt, args...)
#endif



VOID 
KfsUnload( IN PDRIVER_OBJECT  DriverObject )
{
	KeBugCheck("Not Yet Implement\n");

}




KSTATUS
KfsRead (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
	KSTATUS Status;
	PIO_STACK_LOCATION pIosp;
	PFILE_OBJECT pFileObject;
	FIL *Fcb;
	ULONG BytesRead = 0;


	pFileObject = Irp->pFileObj;
	Fcb = (FIL *) pFileObject->FsContext;

	ASSERT(Fcb);
	
	pIosp = IoGetCurrentIrpStackLocation(Irp);
	
	ASSERT(pIosp->MajorFunction == IRP_MJ_READ);
	ASSERT(Fcb != NULL);

	Status = f_read(Fcb, Irp->Buffer, Irp->BufferSize, &BytesRead);	
	ASSERT1(Status == FR_OK, Status);

	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = BytesRead;


	IoCompleteRequest(Irp, IO_NO_INCREMENT);	
	return Status;

}



KSTATUS
KfsWrite (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
	PIO_STACK_LOCATION pIosp;
	KSTATUS Status;
	PFILE_OBJECT pFileObject = Irp->pFileObj;
	FIL *Fcb;
	ULONG BytesWritten = 0;

	pIosp = IoGetCurrentIrpStackLocation(Irp);
	
	ASSERT(pIosp->MajorFunction == IRP_MJ_WRITE);
	Fcb = (FIL*) pFileObject->FsContext;


	ASSERT(Fcb != NULL);
	Status = f_write(Fcb, Irp->Buffer, Irp->BufferSize, &BytesWritten);
	//ASSERT1(Status == FR_OK, Status);

	
	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = BytesWritten;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;


}

BYTE
FileModeToFSMode(FILE_MODE FileMode)
{

	BYTE FsMode = 0;




	if (FileMode == O_RDONLY) {
			FsMode = FA_READ;
	}
	if (FileMode & O_WRONLY) {
			FsMode |= FA_WRITE | FA_OPEN_ALWAYS;
	}
	if (FileMode & O_RDWR) {
			FsMode |= FA_READ | FA_WRITE | FA_OPEN_ALWAYS;
	}
	if (FileMode & O_CREATE) {
			FsMode |= FA_CREATE_NEW;
	}
	if (FileMode & O_TRUNC) {
			FsMode |= FA_READ | FA_OPEN_ALWAYS;
	}
	if (FileMode & O_EXCL) {
			FsMode |= FA_CREATE_NEW;
	}
	if (FileMode & O_MKDIR) {
		KeBugCheck("Not Yet done!!");
			FsMode |= FA_READ;
	}

	printf("FileModeToFSMode: %x\n ", FileMode);


	return FsMode;

}



#define SYSTEM_DRIVE 1

KSTATUS
KfsCreate (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
	KSTATUS Status = 0;
	FIL *Fcb = NULL;
	PFILE_OBJECT pFileObject = Irp->pFileObj;
	FIL * fp = NULL;
	ASSERT(pFileObject->FsContext == NULL);


	// Right Now We are creating a new FCB for each open which is not correct
	// We should maintain some data struct to keep track and when a open request
	// to same file comes in we should point to the same FCB.

	Fcb = ExAllocatePoolWithTag(NonPagedPool, sizeof (FIL), ' BCF');
	if (Fcb == NULL) {

		return STATUS_INSUFFICIENT_RESOURCES;
	}


	f_chdrive(SYSTEM_DRIVE);
	KosFsLog("Opening File %s\n",pFileObject->filename);

	Status = f_open(Fcb, pFileObject->filename, FileModeToFSMode(pFileObject->FileMode));

//	ASSERT1(Status == FR_OK, Status);
	if(!K_SUCCESS(Status)) {
		ASSERT1(0, Status);
		ExFreePool(Fcb);
		Irp->pFileObj->FsContext = NULL;
		return Status;
	}

	Irp->pFileObj->FsContext = Fcb;


	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}



KSTATUS
KfsClose (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp
    )
{
	KSTATUS Status;

	Status = f_close(pIrp->pFileObj->FsContext);

	ASSERT(Status == STATUS_SUCCESS);

	MmFreePoolWithTag(pIrp->pFileObj->FsContext, ' BCF');

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);	
	return Status;
}


KSTATUS
KfsQueryInfo (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp
    )
{


	KeBugCheck("Not Yet Implemented");
	return STATUS_SUCCESS;
#if 0
	PIO_STACK_LOCATION pIosp;
	PFILE_OBJECT pFileObject = pIrp->pFileObj;
	struct File *file = (struct File *)pFileObject->FsContext;
	PFILE_STATS pFileStats;

	pIosp =  IoGetCurrentIrpStackLocation(pIrp);
	ASSERT(pIosp->MajorFunction == IRP_MJ_QUERY_INFORMATION);
	
	switch (pIosp->MinorFunction) {

		case IRP_MN_QUERY_STATS:
			pFileStats = (PFILE_STATS) pIrp->Buffer;	
			if (pIrp->BufferSize < sizeof(FILE_STATS)) {
				KeBugCheck("Buffer size less than expected; buffer size %d  Stat size %d\n", 
							pIrp->BufferSize, sizeof(FILE_STATS));
			}
		
		
			strncpy(pFileStats->StatName, file->f_name, MAXNAMELEN);
			pFileStats->StatSize = file->f_size;
			pFileStats->Stat_IsDir = (file->f_type == FTYPE_DIR);
			break;

			default:
				KeBugCheck("Received umimplemented Minor function %d", pIosp->MinorFunction);

	}

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	pIrp->status  = SUCCESS;
	return SUCCESS;	
#endif
}

KSTATUS
KfsSetInfo (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

{
	KeBugCheck("Not Yet Implement\n");


}


KSTATUS
KfsPassThrough (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp
    )
{

	KeBugCheck("KePassThrough: Passthroughs are not yet supported\n");
	return STATUS_SUCCESS;

}


	
KSTATUS 
KFSDriverEntry( 
		IN PDRIVER_OBJECT  pDriverObject, 
		IN PUNICODE_STRING	RegistryPath 
		)
	
{
		KSTATUS Status = 0;
		int function;
		PDEVICE_OBJECT	pDeviceObject;
		FATFS *filesys1;
		UNICODE_STRING DeviceName;
		UNICODE_STRING SymbolicLinkName;
		UNICODE_STRING MountPoint;

		kprintf("KFSDriverEntry\n");
		pDriverObject->DriverUnload = KfsUnload;

		for (function = 0; function <= IRP_MJ_MAXIMUM_FUNCTION; function++ ) {

			pDriverObject->MajorFunction[function] = KfsPassThrough;

		}

	
		pDriverObject->MajorFunction[IRP_MJ_READ] = KfsRead;
		pDriverObject->MajorFunction[IRP_MJ_WRITE] = KfsWrite;
		pDriverObject->MajorFunction[IRP_MJ_CREATE] = KfsCreate;
		pDriverObject->MajorFunction[IRP_MJ_CLOSE] = KfsClose;
		pDriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] = KfsQueryInfo;
		pDriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] = KfsSetInfo;

		RtlInitUnicodeString(
			&DeviceName, "\\Device\\Kfsdisk1");

		Status = IoCreateDevice( pDriverObject,
							0,
							&DeviceName,
							IO_DEVICE_FILESYSTEM,
							0,
							FALSE,
							&pDeviceObject);


		ASSERT1(Status == STATUS_SUCCESS, Status);

		KosFsLog("Created Device \\Device\\KfsDisk");


		RtlInitUnicodeString(&SymbolicLinkName,"\\GLOBAL??\\c:");

		Status = IoCreateSymbolicLink(&SymbolicLinkName, &DeviceName);

		ASSERT1(Status == STATUS_SUCCESS, Status);
		KosFsLog("Created Symlink c:");


		RtlInitUnicodeString( &MountPoint, "\\GLOBAL??");

		Status = IoMountFileSystem(&MountPoint, &DeviceName);
		KosFsLog("MountPoint Created for KOSFS");
		ASSERT1(Status == STATUS_SUCCESS, Status);


		// mount the FS
		filesys1 = ExAllocatePoolWithTag(NonPagedPool, sizeof (FATFS), 'FTAF');
		if (filesys1 == NULL) {
			KeBugCheck("FS Mount Failed\n");

		}
		
		f_mount(1, filesys1);
	return STATUS_SUCCESS;
}



void
KFSinit()
{

	DRIVER_OBJECT DriverObject;
	KSTATUS status;
	UNICODE_STRING DriverName;

	kprintf("Initializing Kaush FS\n");


	RtlInitUnicodeString( &DriverName, "\\Driver\\KFSDriver");

	kprintf("DriverName %s %d\n",DriverName.Buffer, DriverName.Length);

	status = IoCreateDriver( &DriverName,
						&KFSDriverEntry,
						NULL,
						&DriverObject);

	if (!K_SUCCESS(status)) {
		KeBugCheck("Create Driver failed status %x", status);

	}

		KosFsLog("Created Driver \\Driver\\KFSDriver");
}


