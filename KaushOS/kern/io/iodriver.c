/*
*  C Implementation: Iodriver.c
*
* Description: IO Manager Driver Implementation
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/

#include	"io.h"



POBJECT_TYPE IoDriverObjectType;


KSTATUS
IoCreateDriver(IN PUNICODE_STRING DriverName,
						IN PDRIVER_INITIALIZE DriverInit,
						IN PUNICODE_STRING	RegPath,
						OUT PDRIVER_OBJECT  *ppDriverObject
						)
{
	int i = 0;
	PDRIVER_OBJECT DriverObject;
	KSTATUS Status;
	OBJECT_ATTRIBUTES ObjectAttributes;


	
	ASSERT(IoDriverObjectType);


	InitializeObjectAttributes( &ObjectAttributes,
		DriverName, 0, NULL, NULL);


	Status = ObCreateObject(
					KernelMode,
					IoDriverObjectType,
					&ObjectAttributes,
					sizeof(DRIVER_OBJECT),
					&DriverObject);

	if (!K_SUCCESS(Status)) {
		return Status;
	}


	DriverObject->DriverInit = DriverInit;
	DriverObject->DriverSize = sizeof(DRIVER_OBJECT);

	
	Status = (*DriverInit) (DriverObject, RegPath);

	if (!K_SUCCESS(Status)) {

		KeBugCheck("DriverEntry Failed for %s\n", DriverName);
	}

	*ppDriverObject = DriverObject;

	return STATUS_SUCCESS;
}



VOID
IoCreateDriverObjectType()
{
	OBJECT_TYPE DrvObjectType;

	memset(&DrvObjectType, 0, sizeof (OBJECT_TYPE));
	
	DrvObjectType.ObjectSize = sizeof(OBJECT_DIRECTORY);
	DrvObjectType.PoolTag = NonPagedPool;
	DrvObjectType.TypeIndex = OBJECT_TYPE_DRIVER;
	KeInitializeSpinLock( &DrvObjectType.SpinLock);
	LIST_INIT( &DrvObjectType.Object_List_Head);

	
	ObCreateObjectType( &DrvObjectType,
						&IoDriverObjectType);





}


