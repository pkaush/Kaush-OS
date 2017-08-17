/*
*  C Interface:	File.H
*
* Description: Interface of user mode filesystem
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/


#ifndef __KOS_INC_FILE_H__
#define __KOS_INC_FILE_H__
#include <inc/types.h>
#include <inc/error.h>


// Maximum size of a filename (a single path component), including null
// Must be a multiple of 4
#define MAXNAMELEN	128

// Maximum size of a complete pathname, including null
#define MAXPATHLEN		1024

typedef ULONG HANDLE, *PHANDLE;


//Function Prototype
typedef  ULONG ACCESS_MASK;


#define FILE_READ_DATA 0x1 	//Read data from the file. 
#define FILE_READ_ATTRIBUTES 0x2  //Read the file's attributes.
//Read the file's extended attributes (EAs).
//This flag is irrelevant for device and intermediate drivers. 
#define FILE_READ_EA 0x4
#define FILE_WRITE_DATA 0x8 //Write data to the file. 
#define FILE_WRITE_ATTRIBUTES 0x10//Write the file's attributes.
#define FILE_WRITE_EA  0x20//Change the file's extended attributes (EAs). This flag is irrelevant for device and intermediate drivers. 
#define FILE_APPEND_DATA 0x40//Append data to the file. 
#define FILE_EXECUTE 0x80//Use system paging I/O to read data from the file into memory. This flag is irrelevant for device and intermediate drivers. 

typedef ULONG	FILE_MODE;

/* File open modes */
#define	O_RDONLY	0x0000		/* open for reading only */
#define	O_WRONLY	0x0001		/* open for writing only */
#define	O_RDWR		0x0002		/* open for reading and writing */
#define	O_ACCMODE	0x0003		/* mask for above modes */

#define	O_CREATE		0x0100		/* create if nonexistent */
#define	O_TRUNC		0x0200		/* truncate to zero length */
#define	O_EXCL		0x0400		/* error if already exists */
#define	O_MKDIR		0x0800		/* create directory, not regular file */


typedef struct _FILE_STATS
{
	char StatName[MAXNAMELEN];
	off_t StatSize;
	int Stat_IsDir;
}FILE_STATS, *PFILE_STATS;





//user mode protoypes for file operations
KSTATUS
CreateFile(IN PCHAR Filename,IN FILE_MODE FileMode,OUT PHANDLE pHandle);

KSTATUS
ReadFile(IN HANDLE Handle,IN PVOID Buffer,IN ULONG NumberOfBytes);

KSTATUS
WriteFile(IN HANDLE Handle,IN PVOID Buffer,IN ULONG NumberOfBytes);


//this function will open the file collect the stats and return
KSTATUS
QueryStats(IN PCHAR Filename,IN PVOID Buffer,IN ULONG NumberOfBytes);

KSTATUS
QueryFileStats(IN HANDLE Handle,IN PVOID Buffer,IN ULONG NumberOfBytes);

KSTATUS
CloseFile(IN HANDLE Handle);

#endif
