/*
*  C Implementation: Loader.c
*
* Description:  This is a Loader for user mode processes
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/

#include <inc/types.h>
#include <inc/elf.h>
#include "process.h"
#include "kern/fs/ff.h"

/* Values for p_type.  See [ELF1] 2-3. */
#define PT_NULL    0            /* Ignore. */
#define PT_LOAD    1            /* Loadable segment. */
#define PT_DYNAMIC 2            /* Dynamic linking info. */
#define PT_INTERP  3            /* Name of dynamic loader. */
#define PT_NOTE    4            /* Auxiliary info. */
#define PT_SHLIB   5            /* Reserved. */
#define PT_PHDR    6            /* Program header table. */
#define PT_STACK   0x6474e551   /* Stack segment. */

/* Flags for p_flags.  See [ELF3] 2-3 and 2-4. */
#define PF_X 1          /* Executable. */
#define PF_W 2          /* Writable. */
#define PF_R 4          /* Readable. */















#if 1



KSTATUS
ELFBinaryLoader(IN PEPROCESS Process,
					IN PCHAR Binary,
					size_t size)
/*++
	This function should be called after attaching to the new process
--*/
{

	

	ULONG_PTR i = 0;
	struct Proghdr *ph, *eph;

	struct Elf * elf_head = (struct Elf *) Binary;

	ASSERT(Process);
	

	if (elf_head->e_magic != ELF_MAGIC) {
		KeBugCheck("Not an ELF binary\n");
	}

	if ( elf_head->e_type != 2
		      || elf_head->e_machine != 3
		      || elf_head->e_version != 1
		      || elf_head->e_phentsize != sizeof (struct Proghdr)
		      || elf_head->e_phnum > 1024) {


		KeBugCheck("Not an ELF binary 2\n");

	}

	kprintf("elf_head->e_phnum 0x%x\n", elf_head->e_phnum);

	ph = (struct Proghdr *) ((uint8_t *) elf_head + elf_head->e_phoff);
	eph = ph + elf_head->e_phnum;

	for (; ph < eph; ph++){
		kprintf("map_segment %x %x\n", ph->p_va, ph->p_memsz);
		MmMapSegment( Process->pcb.DirectoryTableBase,
			ph->p_va,
			ph->p_memsz,
			PTE_W|PTE_U);

		if(ph->p_filesz > ph->p_memsz){
			KeBugCheck("Bad ELF binary");
		}

		
		memcpy((void *)(ph->p_va), Binary + ph->p_offset, ph->p_filesz);	//copy to va

		for(i = ph->p_va + ph->p_filesz; i < ph->p_memsz; i++){ // zero out rest of the segment memory
		
			*(uintptr_t *)i = 0;
		}
	}
	// sets eip to the entrypoint

	Process->pcb.ProcessEntryPoint = (ULONG_PTR) elf_head->e_entry;
	
	return STATUS_SUCCESS;
}
#else


static
BOOL
ELFValidateSegment (const struct Proghdr *phdr, size_t FileSize) 
{


/* p_offset and p_va must have the same page offset. */
	if ((phdr->p_offset & PGMASK) != (phdr->p_va & PGMASK)) {
		return FALSE;
	}

/* p_offset must point within FILE. */
	if (phdr->p_offset > FileSize) {
		return FALSE;
	}

/* p_memsz must be at least as big as p_filesz. */
	if (phdr->p_memsz < phdr->p_filesz) {
		return FALSE; 
	}

/* The segment must not be empty. */
	if (phdr->p_memsz == 0) {
		return FALSE;
	}

/* The virtual memory region must both start and end within the
user address space range. */
	if (!IsUserAddress((void *) phdr->p_va)) {
		return FALSE;
	}

	if (!IsUserAddress((void *) (phdr->p_va + phdr->p_memsz))) {
		return FALSE;
	}
/* The region cannot "wrap around" across the kernel virtual
address space. */
	if (phdr->p_va + phdr->p_memsz < phdr->p_va) {
		return FALSE;
	}

// Disallow mapping page 0.

	if (phdr->p_va < PGSIZE) {
		return FALSE;
	}


	return TRUE;
}




KSTATUS
ELFBinaryLoader(IN PEPROCESS Process,
					IN PCHAR Binary,
					size_t size)
/*++
	This function should be called after attaching to the new process
--*/
{

	

	ULONG hnum = 0;
	ULONG i = 0;
	struct Proghdr phdr;
	FIL * fp, FilePointerBuff;
	off_t Offset;
	KSTATUS Status = STATUS_SUCCESS;
	struct Elf elf_head;
	UINT BytesRead;
	ASSERT(Process);


	fp = &FilePointerBuff;
	Status = f_open(fp, Binary, FA_READ);

	if (!K_SUCCESS(Status)) {
		goto ELFBinaryLoaderExit;
	}

	Status = f_read(fp,&elf_head, sizeof(struct Elf), &BytesRead);

	if(!K_SUCCESS(Status)) {
		goto ELFBinaryLoaderExit;
	}
	
	if (elf_head.e_magic != ELF_MAGIC) {
		KeBugCheck("Not an ELF binary\n");
	}

	if ( elf_head.e_type != 2
		      || elf_head.e_machine != 3
		      || elf_head.e_version != 1
		      || elf_head.e_phentsize != sizeof (struct Proghdr)
		      || elf_head.e_phnum > 1024) {


		KeBugCheck("Not an ELF binary 2\n");

	}

	Offset = elf_head.e_phoff;

	for (hnum = 0; hnum < elf_head.e_phnum; hnum++) {

		if (Offset < 0 || Offset  > fp->fsize) {
			goto ELFBinaryLoaderExit;
		}

		Status = f_lseek( fp, Offset);

		if (!K_SUCCESS(Status)) {

			goto ELFBinaryLoaderExit;
		}


		Status = f_read( fp, &phdr, sizeof(struct Proghdr), &BytesRead);

		if ((!K_SUCCESS(Status)) || (BytesRead < sizeof(struct Proghdr))) {
			goto ELFBinaryLoaderExit;
		}


		Offset += sizeof(struct Proghdr);
		
		switch (phdr.p_type) 
		  {
		  case PT_NULL:
		  case PT_NOTE:
		  case PT_PHDR:
		  case PT_STACK:
		  default:
			/* Ignore this segment. */
			break;
		  case PT_DYNAMIC:
		  case PT_INTERP:
		  case PT_SHLIB:
			goto ELFBinaryLoaderExit;
		  case PT_LOAD:

				if (ELFValidateSegment( &phdr, fp->fsize)) {

					Status = f_lseek( fp, phdr.p_offset);

					if (!K_SUCCESS(Status)) {
						goto ELFBinaryLoaderExit;
					}

					
					kprintf("map_segment %x %x\n", phdr.p_va, phdr.p_memsz);
					MmMapSegment( Process->pcb.DirectoryTableBase,
						phdr.p_va,
						phdr.p_memsz,
						PTE_W|PTE_U);

					
					Status = f_read( fp, phdr.p_va, phdr.p_filesz, &BytesRead);

					if ( (!K_SUCCESS(Status)) || (BytesRead < sizeof(struct Proghdr)) ) {
						goto ELFBinaryLoaderExit;
					} 

					
					for(i = phdr.p_va + phdr.p_filesz; i < phdr.p_memsz; i++){ 
						// zero out rest of the segment memory
					
						*(uintptr_t *)i = 0;
					}

				}



		}

	}
	
	// sets eip to the entrypoint
	Process->pcb.ProcessEntryPoint = (ULONG_PTR) elf_head.e_entry;

ELFBinaryLoaderExit:


// No need to free the allocated Pages here. Failure to load the binary will lead to 
// process address space destruction, which will free the allocated pages here.

	f_close(fp);

	return Status;
}





#endif



