/*
*  C Implementation: Io.c
*
* Description: IO Manager
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/

#include	"io.h"
//#include	<kern/fs/fsinterface.h>




POBJECT_TYPE IoFileObjectType = NULL;




KSTATUS
IoCallDriver( IN PDEVICE_OBJECT pDeviceObject, 
					IN PIRP pIrp)
{
	PDRIVER_OBJECT pDriverObject;
	PIO_STACK_LOCATION pIosp;

	IoAdvanceIrpStackPointer(pIrp);

	if (pIrp->CurrentStackLocation >= pDeviceObject->StackSize) {
		KeBugCheck("Crossed Irp stack size;  current Irp stack index %d, total stack size %d\n",
				pIrp->CurrentStackLocation, pDeviceObject->StackSize);
	}

	pDriverObject = pDeviceObject->DriverObject;

	pIosp = IoGetCurrentIrpStackLocation(pIrp);

	return ((pDriverObject->MajorFunction[pIosp->MajorFunction]) (
				pDeviceObject,
				pIrp
				));

}



void
IoInit()
{

	// initlize the device list
	kprintf("Initializing IoManager\n");

	IoCreateDeviceObjectType();
	IoCreateFileObjectType();
	IoCreateDriverObjectType();

	
}


KSTATUS
IoFileOperations( IN PFILE_OBJECT FileObject,
				IN PVOID Buffer,
				IN ULONG NumberOfBytes,
				IN ULONG MajorFunction,
				IN ULONG MinorFunction
				)

{

	KSTATUS status;
	PIRP pIrp = NULL;

	
	if (FileObject == NULL) {
		ASSERT(0); // will remove this later
		return STATUS_INVALID_PARAMETER;
	}
	

	pIrp = IoBuildIrp(FileObject,
			MajorFunction,
			MinorFunction, 
			Buffer, 
			NumberOfBytes, 
			FileObject->FileOffset);
	

	ASSERT(pIrp);
	
	if (pIrp == NULL) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	
	status = IoCallDriver(FileObject->DeviceObject,pIrp);

	IoFreeIRP(pIrp);
	return status;

}




KSTATUS
IoMountFileSystem(
					IN PUNICODE_STRING MountPoint,
					IN PUNICODE_STRING DeviceName
					)
{

	return(ObMountFileSystem( MountPoint, DeviceName));

}


VOID
IoFreeIRP(PIRP pIrp)
{
	ExFreePoolWithTag(pIrp,'IprI');
}



PIRP
IoBuildIrp( 	IN 	PFILE_OBJECT pFileObj,
					IN UCHAR MajorFunction,
					IN UCHAR MinorFunction,
					IN PVOID Buffer,
					IN size_t  BufferSize,
					IN ULONG	Offset
    					)
{

	PDEVICE_OBJECT DeviceObject = pFileObj->DeviceObject;
	PIO_STACK_LOCATION pIosp = NULL;

	PIRP pIrp = (PIRP) MmAllocatePoolWithTag(
							PagedPool, sizeof(IRP), 'IprI');

	if(pIrp == NULL) {
		return NULL;
	}

	pIrp->CurrentStackLocation = -1;
	pIrp->pFileObj = pFileObj;
	pIrp->Offset = Offset;
	pIrp->Buffer = Buffer;
	pIrp->BufferSize = BufferSize;
	pIrp->IrpCancelRoutine = NULL;
	pIrp->Cancel = FALSE;

	pIrp->Completed = FALSE;
	
	pIosp  = IoGetNextIrpStackLocation(pIrp);
	pIosp->MajorFunction = MajorFunction;
	pIosp->MinorFunction = MinorFunction;

	return pIrp;
}



