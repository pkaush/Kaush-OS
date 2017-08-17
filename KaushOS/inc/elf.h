/*
*  C Interface: elf.h
*
* Description: ELF specification
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/

#ifndef KOS_INC_ELF_H
#define KOS_INC_ELF_H

#define ELF_MAGIC 0x464C457FU	/* "\x7FELF" in little endian */

struct Elf {
	ULONG e_magic;	// must equal ELF_MAGIC
	UCHAR e_elf[12];
	USHORT e_type;
	USHORT e_machine;
	ULONG e_version;
	ULONG e_entry;
	ULONG e_phoff;
	ULONG e_shoff;
	ULONG e_flags;
	USHORT e_ehsize;
	USHORT e_phentsize;
	USHORT e_phnum;
	USHORT e_shentsize;
	USHORT e_shnum;
	USHORT e_shstrndx;
};

struct Proghdr {
	ULONG p_type;
	ULONG p_offset;
	ULONG p_va;
	ULONG p_pa;
	ULONG p_filesz;
	ULONG p_memsz;
	ULONG p_flags;
	ULONG p_align;
};

// Values for Proghdr::p_type
#define ELF_PROG_LOAD		1

// Flag bits for Proghdr::p_flags
#define ELF_PROG_FLAG_EXEC	1
#define ELF_PROG_FLAG_WRITE	2
#define ELF_PROG_FLAG_READ	4

#endif //End of Header
