/*
*  C Implementation: Iodevice.c
*
* Description: IO Manager Device Implementation
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/

#include	"io.h"



POBJECT_TYPE IoDeviceObjectType;





KSTATUS
IoCreateSymbolicLink(
					IN PUNICODE_STRING SymbolicLinkName,
					IN PUNICODE_STRING DeviceName
					)
{
	return (ObCreateSymbolicLink( SymbolicLinkName, DeviceName));
}







KSTATUS
IoCreateDevice(
						IN PDRIVER_OBJECT  DriverObject,
						IN ULONG  DeviceExtensionSize,
						IN PUNICODE_STRING  DeviceName  OPTIONAL,
						IN DEVICE_TYPE  DeviceType,
						IN ULONG  DeviceCharacteristics,
						IN BOOLEAN  Exclusive,
						OUT PDEVICE_OBJECT  *DeviceObject
)


{

	PETHREAD Thread;
	KPROCESSOR_MODE PreviousMode;
	OBJECT_ATTRIBUTES ObjectAttributes;
	PDEVICE_OBJECT LocalDeviceObject;
	UNICODE_STRING LocalDeviceName;
	CHAR LocalBuffer[128];
	KSTATUS Status;

	
	static ULONG DeviceNumber = 1;


	Thread = KeGetCurrentThread();

	if (Thread) {

		PreviousMode = Thread->tcb.PreviousMode;
	}  else {

		PreviousMode = KernelMode;
	}


	if (DeviceName == NULL) {

		DeviceName = &LocalDeviceName;
		DeviceName->Buffer = &LocalBuffer;
		snprintf(LocalBuffer, 21,"\\Device\\KosDev-%08lx",DeviceNumber);

		DeviceNumber++;
	}


	InitializeObjectAttributes( &ObjectAttributes,
						DeviceName,
						0, NULL,NULL);

	

	Status = ObCreateObject(
				PreviousMode,
				IoDeviceObjectType,
				&ObjectAttributes,
				sizeof (DEVICE_OBJECT) + DeviceExtensionSize,
				&LocalDeviceObject);

	if (!K_SUCCESS(Status)) {

		return Status;
	}



	LocalDeviceObject->Characteristics = DeviceCharacteristics;
	LocalDeviceObject->DeviceExtension = LocalDeviceObject + 1;
	LocalDeviceObject->DriverObject = DriverObject;
	LocalDeviceObject->StackSize = 1;
	LocalDeviceObject->Size = sizeof (DEVICE_OBJECT) + DeviceExtensionSize;


//Insert the device in the Driver List
//	LocalDeviceObject->



	return STATUS_SUCCESS;
}


VOID
IoCreateDeviceObjectType()
{
	OBJECT_TYPE LocalObType;

	memset(&LocalObType, 0, sizeof (OBJECT_TYPE));
	
	LocalObType.ObjectSize = sizeof(DEVICE_OBJECT);
	LocalObType.PoolTag = NonPagedPool;
	LocalObType.TypeIndex = OBJECT_TYPE_DEVICE;
	KeInitializeSpinLock( &LocalObType.SpinLock);
	LIST_INIT( &LocalObType.Object_List_Head);
	
	ObCreateObjectType( &LocalObType,
						&IoDeviceObjectType);


}
