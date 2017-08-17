/*
*  C Interface:	Syscall.h
*
* Description: Syscalls
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/


#ifndef KOS_INC_SYSCALL_H
#define KOS_INC_SYSCALL_H


enum 
{
	SYS_GETPID,			/*Get Current Process PID*/
	SYS_EXIT,                   /* Terminate this process. */
	SYS_FORK,
	SYS_EXEC,                   /* Start another process. */
	SYS_WAIT,                   /* Wait for a child process to die. */
	SYS_CREATE,                 /* Create a file. */
	SYS_REMOVE,                 /* Delete a file. */
	SYS_OPEN,                   /* Open a file. */
	SYS_FILESIZE,               /* Obtain a file's size. */
	SYS_READ,                   /* Read from a file. */
	SYS_WRITE,                  /* Write to a file. */
	SYS_SEEK,                   /* Change position in a file. */
	SYS_TELL,                   /* Report current position in a file. */
	SYS_CLOSE,                  /* Close a file. */


	SYS_MMAP,                   /* Map a file into memory. */
	SYS_MUNMAP,                 /* Remove a memory mapping. */


	SYS_CHDIR,                  /* Change the current directory. */
	SYS_MKDIR,                  /* Create a directory. */
	SYS_READDIR,                /* Reads a directory entry. */
	SYS_ISDIR,                  /* Tests if a fd represents a directory. */
	SYS_INUMBER,                 /* Returns the inode number for a fd. */

	SYS_TESTING		
};


#endif 
