/*
*  C Interface:	io.h
*
* Description:
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/


#ifndef	__KOS_IO_IO_H__
#define	__KOS_IO_IO_H__

#include	<inc/list.h>
#include	<inc/file.h>

#include	<kern/ob/ob.h>




#define IRP_MJ_CREATE                   0x00
#define IRP_MJ_CREATE_NAMED_PIPE        0x01
#define IRP_MJ_CLOSE                    0x02
#define IRP_MJ_READ                     0x03
#define IRP_MJ_WRITE                    0x04
#define IRP_MJ_QUERY_INFORMATION        0x05
#define IRP_MJ_SET_INFORMATION          0x06
#define IRP_MJ_FLUSH_BUFFERS            0x09
#define IRP_MJ_QUERY_VOLUME_INFORMATION 0x0a
#define IRP_MJ_SET_VOLUME_INFORMATION   0x0b
#define IRP_MJ_DIRECTORY_CONTROL        0x0c
#define IRP_MJ_FILE_SYSTEM_CONTROL      0x0d
#define IRP_MJ_DEVICE_CONTROL           0x0e
#define IRP_MJ_INTERNAL_DEVICE_CONTROL  0x0f
#define IRP_MJ_SHUTDOWN                 0x10
#define IRP_MJ_LOCK_CONTROL             0x11
#define IRP_MJ_CLEANUP                  0x12
#define IRP_MJ_CREATE_MAILSLOT          0x13
#define IRP_MJ_QUERY_SECURITY           0x14
#define IRP_MJ_SET_SECURITY             0x15
#define IRP_MJ_POWER                    0x16
#define IRP_MJ_SYSTEM_CONTROL           0x17
#define IRP_MJ_DEVICE_CHANGE            0x18
#define IRP_MJ_QUERY_QUOTA              0x19
#define IRP_MJ_SET_QUOTA                0x1a
#define IRP_MJ_PNP                      0x1b
#define IRP_MJ_MAXIMUM_FUNCTION         0x1b


#define IRP_MN_NULL	0x0
#define IRP_MN_QUERY_STATS	0x1



typedef struct _IO_STACK_LOCATION IO_STACK_LOCATION, *PIO_STACK_LOCATION;
typedef struct _IO_STATUS_BLOCK IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;
typedef struct _IRP IRP, *PIRP;
typedef struct _DRIVER_OBJECT DRIVER_OBJECT, *PDRIVER_OBJECT;
typedef struct _DEVICE_OBJECT DEVICE_OBJECT, *PDEVICE_OBJECT;
typedef struct _FILE_OBJECT FILE_OBJECT, *PFILE_OBJECT;
typedef enum _DEVICE_TYPE DEVICE_TYPE, *PDEVICE_TYPE;



typedef
KSTATUS
IO_COMPLETION(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP  pIrp,
	IN PVOID  Context
	);

typedef IO_COMPLETION *PIO_COMPLETION;


typedef
BOOLEAN
DRIVER_CANCEL(
	IN PIRP  Irp
	);

typedef DRIVER_CANCEL *PDRIVER_CANCEL;


//PIO_STACK_LOCATION 
//IoGetCurrentIrpStackLocation(
//    IN PIRP  Irp
//    );


#define IO_NO_INCREMENT	0x0


#define IoGetCurrentIrpStackLocation(pIrp)	(&(pIrp)->IoStack[pIrp->CurrentStackLocation])

#define IoGetNextIrpStackLocation(pIrp)		(&(pIrp)->IoStack[pIrp->CurrentStackLocation +1])

#define IoSkipCurrentIrpStackLocation(pIrp)	((pIrp)->CurrentStackLocation) --

#define IoAdvanceIrpStackPointer(pIrp)	((pIrp)->CurrentStackLocation) ++

 #define IoCompleteRequest(pIrp,INCREMENT) ((pIrp)->Completed = TRUE)



//
// I/O Request Packet (IRP) definition
//


// maximum size for a device stack 
#define IO_MAX_STACKSIZE_ALLOWED 4



//struct for an Irp stack loacation
struct _IO_STACK_LOCATION { 
	UCHAR MajorFunction;
	UCHAR MinorFunction;
	UCHAR Flags;
	PIO_COMPLETION IoCompletion;
	PVOID	Context;
};

struct _IO_STATUS_BLOCK {
	KSTATUS Status;
	ULONG Information;
};

struct _IRP {
	PVOID Buffer;
	size_t	BufferSize;
	PFILE_OBJECT pFileObj;
	LIST_ENTRY ListEntry;
	ULONG	Offset;

	IO_STACK_LOCATION IoStack[IO_MAX_STACKSIZE_ALLOWED];
	LONG CurrentStackLocation;


	
	BOOLEAN  Cancel;
	KIRQL  CancelIrql;
	PDRIVER_CANCEL	IrpCancelRoutine;


	BOOLEAN	Completed;
	IO_STATUS_BLOCK IoStatus;
};


PIRP
IoCreateIrp( 	IN 	PFILE_OBJECT pFileObj,
					IN UCHAR MajorFunction,
					IN UCHAR MinorFunction,
					IN PVOID ReadBuffer,
					IN size_t	ReadBufferSize,
					IN PVOID WriteBuffer,
					IN size_t	WriteBufferSize,
					IN ULONG	Offset
					);


VOID
IoFreeIRP(PIRP pIrp);


#define DEVICE_NAME_LENGTH 64
#define DRIVER_NAME_LENGTH 64



enum _DEVICE_TYPE {
		IO_INVALID_DEVICE,
		IO_DEVICE_FILESYSTEM,
		IO_MAX_DEVICE
};




struct _DEVICE_OBJECT {
	ULONG DeviceType;
	CHAR DeviceName[DEVICE_NAME_LENGTH];
	USHORT Size;
	ULONG ReferenceCount;
	struct _DRIVER_OBJECT *DriverObject;
	struct _DEVICE_OBJECT *NextDevice;
	struct _DEVICE_OBJECT *AttachedDevice;
	struct _IRP *CurrentIrp;

	LONG Flags;			
	ULONG Characteristics; 
	LONG StackSize;
	PVOID DeviceExtension;

	PVOID	Reserved;

};



typedef
VOID 
DRIVER_UNLOAD ( 
    IN PDRIVER_OBJECT  DriverObject 
    ); 
typedef DRIVER_UNLOAD *PDRIVER_UNLOAD;

typedef
VOID
  DRIVER_STARTIO (
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp 
    );

typedef DRIVER_STARTIO *PDRIVER_STARTIO;

typedef
KSTATUS 
DRIVER_INITIALIZE ( 
    IN PDRIVER_OBJECT  DriverObject, 
    IN PUNICODE_STRING  RegistryPath 
    ); 
typedef DRIVER_INITIALIZE *PDRIVER_INITIALIZE;

typedef
KSTATUS
DRIVER_DISPATCH (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
typedef DRIVER_DISPATCH *PDRIVER_DISPATCH;



typedef
KSTATUS
DRIVER_AddDevice (
    IN PDRIVER_OBJECT  DriverObject,
    IN PDEVICE_OBJECT  PhysicalDeviceObject 
    );
typedef DRIVER_AddDevice *PDRIVER_AddDevice;

struct	_DRIVER_OBJECT {

	ULONG Flags;
	PDEVICE_OBJECT DeviceObject;


	PVOID DriverStart;
	ULONG DriverSize;

	PDRIVER_INITIALIZE DriverInit;
	PDRIVER_STARTIO DriverStartIo;
	PDRIVER_UNLOAD DriverUnload;
	PDRIVER_DISPATCH MajorFunction[IRP_MJ_MAXIMUM_FUNCTION + 1];

};



struct	_FILE_OBJECT {
	PDEVICE_OBJECT DeviceObject;
	ACCESS_MASK DesiredAccess; //file permissions on which it is opened
	FILE_MODE FileMode;
	LONG_PTR FileOffset;		// offset inside the file
	char filename[MAXPATHLEN];
	PVOID FsContext;
	PVOID FsContext2;
};


PIRP
IoBuildIrp( 	IN 	PFILE_OBJECT pFileObj,
					IN UCHAR MajorFunction,
					IN UCHAR MinorFunction,
					IN PVOID Buffer,
					IN size_t  BufferSize,
					IN ULONG	Offset
    					);




KSTATUS
KCreateFile( IN PCHAR Filename,
					IN FILE_MODE  FileMode,
					OUT PHANDLE  pHandle);



KSTATUS
KReadFile(IN HANDLE Handle,
					IN PVOID Buffer,
					IN ULONG NumberOfBytes);


KSTATUS
KWriteFile(IN HANDLE Handle,
					IN PVOID Buffer,
					IN ULONG NumberOfBytes);


VOID
KCloseFile(IN HANDLE Handle);


KSTATUS
KQueryFileStats(IN HANDLE Handle,
					IN PVOID Buffer,
					IN ULONG NumberOfBytes);


KSTATUS
KQueryStats(IN PCHAR Filename,
					IN PVOID Buffer,
					ULONG NumberOfBytes);



//KSetInformationFile()




//Init function will be called from kos_start
void
IoInit();


#endif
