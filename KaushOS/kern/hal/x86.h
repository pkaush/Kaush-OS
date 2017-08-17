/*
*  C Interface:	x86.h
*
* Description: C interface for x86 instructions and Data
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/



#ifndef KOS_KERN_HAL_X86_H
#define KOS_KERN_HAL_X86_H


#ifndef __ASSEMBLER__

#include <inc/assert.h>
#include <inc/stdio.h>
#include <inc/types.h>

#include <kern/ke/ke.h>



// Current IRQ mask.
// Initial IRQ mask has interrupt 2 enabled (for slave 8259A).
#define DEFAULT_PIC_MASK			(0xFFFF &  ~(1<<IRQ_SLAVE))

static __inline void breakpoint(void) __attribute__((always_inline));
static __inline UCHAR inb(int port) __attribute__((always_inline));
static __inline void insb(int port, void *addr, int cnt) __attribute__((always_inline));
static __inline USHORT inw(int port) __attribute__((always_inline));
static __inline void insw(int port, void *addr, int cnt) __attribute__((always_inline));
static __inline ULONG inl(int port) __attribute__((always_inline));
static __inline void insl(int port, void *addr, int cnt) __attribute__((always_inline));
static __inline void outb(int port, UCHAR data) __attribute__((always_inline));
static __inline void outsb(int port, const void *addr, int cnt) __attribute__((always_inline));
static __inline void outw(int port, USHORT data) __attribute__((always_inline));
static __inline void outsw(int port, const void *addr, int cnt) __attribute__((always_inline));
static __inline void outsl(int port, const void *addr, int cnt) __attribute__((always_inline));
static __inline void outl(int port, ULONG data) __attribute__((always_inline));
static __inline void invlpg(void *addr) __attribute__((always_inline));
static __inline void lidt(void *p) __attribute__((always_inline));
static __inline void lldt(USHORT sel) __attribute__((always_inline));
static __inline void ltr(USHORT sel) __attribute__((always_inline));
static __inline void lcr0(ULONG val) __attribute__((always_inline));
static __inline ULONG rcr0(void) __attribute__((always_inline));
static __inline ULONG rcr2(void) __attribute__((always_inline));
static __inline void lcr3(ULONG val) __attribute__((always_inline));
static __inline ULONG rcr3(void) __attribute__((always_inline));
static __inline void lcr4(ULONG val) __attribute__((always_inline));
static __inline ULONG rcr4(void) __attribute__((always_inline));
static __inline void tlbflush(void) __attribute__((always_inline));
static __inline ULONG read_eflags(void) __attribute__((always_inline));
static __inline void write_eflags(ULONG eflags) __attribute__((always_inline));
static __inline ULONG read_ebp(void) __attribute__((always_inline));
static __inline ULONG read_esp(void) __attribute__((always_inline));
static __inline void cpuid(ULONG info, ULONG *eaxp, ULONG *ebxp, ULONG *ecxp, ULONG *edxp);
static __inline uint64_t read_tsc(void) __attribute__((always_inline));
static __inline void sti() __attribute__((always_inline));


static __inline void
sti(void)
{
	__asm __volatile("sti");
}


static __inline void
cli(void)
{
	__asm __volatile("cli");
}




static __inline void
breakpoint(void)
{
	TRACEPRINT("breakpoint");

	__asm __volatile("int3");
}

static __inline UCHAR
inb(int port)
{
	UCHAR data;
	__asm __volatile("inb %w1,%0" : "=a" (data) : "d" (port));
	return data;
}

static __inline void
insb(int port, void *addr, int cnt)
{
	__asm __volatile("cld\n\trepne\n\tinsb"			:
			 "=D" (addr), "=c" (cnt)		:
			 "d" (port), "0" (addr), "1" (cnt)	:
			 "memory", "cc");
}

static __inline USHORT
inw(int port)
{
	USHORT data;
	__asm __volatile("inw %w1,%0" : "=a" (data) : "d" (port));
	return data;
}

static __inline void
insw(int port, void *addr, int cnt)
{
	__asm __volatile("cld\n\trepne\n\tinsw"			:
			 "=D" (addr), "=c" (cnt)		:
			 "d" (port), "0" (addr), "1" (cnt)	:
			 "memory", "cc");
}

static __inline ULONG
inl(int port)
{
	ULONG data;
	__asm __volatile("inl %w1,%0" : "=a" (data) : "d" (port));
	return data;
}

static __inline void
insl(int port, void *addr, int cnt)
{
	__asm __volatile("cld\n\trepne\n\tinsl"			:
			 "=D" (addr), "=c" (cnt)		:
			 "d" (port), "0" (addr), "1" (cnt)	:
			 "memory", "cc");
}

static __inline void
outb(int port, UCHAR data)
{
	__asm __volatile("outb %0,%w1" : : "a" (data), "d" (port));
}

static __inline void
outsb(int port, const void *addr, int cnt)
{
	__asm __volatile("cld\n\trepne\n\toutsb"		:
			 "=S" (addr), "=c" (cnt)		:
			 "d" (port), "0" (addr), "1" (cnt)	:
			 "cc");
}

static __inline void
outw(int port, USHORT data)
{
	__asm __volatile("outw %0,%w1" : : "a" (data), "d" (port));
}

static __inline void
outsw(int port, const void *addr, int cnt)
{
	__asm __volatile("cld\n\trepne\n\toutsw"		:
			 "=S" (addr), "=c" (cnt)		:
			 "d" (port), "0" (addr), "1" (cnt)	:
			 "cc");
}

static __inline void
outsl(int port, const void *addr, int cnt)
{
	__asm __volatile("cld\n\trepne\n\toutsl"		:
			 "=S" (addr), "=c" (cnt)		:
			 "d" (port), "0" (addr), "1" (cnt)	:
			 "cc");
}

static __inline void
outl(int port, ULONG data)
{
	__asm __volatile("outl %0,%w1" : : "a" (data), "d" (port));
}

static __inline void 
invlpg(void *addr)
{ 
	__asm __volatile("invlpg (%0)" : : "r" (addr) : "memory");
}  

static __inline void
lidt(void *p)
{
	__asm __volatile("lidt (%0)" : : "r" (p));
}

static __inline void
lldt(USHORT sel)
{
	__asm __volatile("lldt %0" : : "r" (sel));
}

static __inline void
ltr(USHORT sel)
{
	__asm __volatile("ltr %0" : : "r" (sel));
}

static __inline void
lcr0(ULONG val)
{
	__asm __volatile("movl %0,%%cr0" : : "r" (val));
}

static __inline ULONG
rcr0(void)
{
	ULONG val;
	__asm __volatile("movl %%cr0,%0" : "=r" (val));
	return val;
}

static __inline ULONG
rcr2(void)
{
	ULONG val;
	__asm __volatile("movl %%cr2,%0" : "=r" (val));
	return val;
}

static __inline void
lcr3(ULONG val)
{
	__asm __volatile("movl %0,%%cr3" : : "r" (val));
}

static __inline ULONG
rcr3(void)
{
	ULONG val;
	__asm __volatile("movl %%cr3,%0" : "=r" (val));
	return val;
}

static __inline void
lcr4(ULONG val)
{
	__asm __volatile("movl %0,%%cr4" : : "r" (val));
}

static __inline ULONG
rcr4(void)
{
	ULONG cr4;
	__asm __volatile("movl %%cr4,%0" : "=r" (cr4));
	return cr4;
}

static __inline void
tlbflush(void)
{
	ULONG cr3;
	__asm __volatile("movl %%cr3,%0" : "=r" (cr3));
	__asm __volatile("movl %0,%%cr3" : : "r" (cr3));
}

static __inline ULONG
read_eflags(void)
{
        ULONG eflags;
        __asm __volatile("pushfl; popl %0" : "=r" (eflags));
        return eflags;
}

static __inline void
write_eflags(ULONG eflags)
{
        __asm __volatile("pushl %0; popfl" : : "r" (eflags));
}

static __inline ULONG
read_ebp(void)
{
        ULONG ebp;
        __asm __volatile("movl %%ebp,%0" : "=r" (ebp));
        return ebp;
}

static __inline ULONG
read_esp(void)
{
        ULONG esp;
        __asm __volatile("movl %%esp,%0" : "=r" (esp));
        return esp;
}

static __inline void
cpuid(ULONG info, ULONG *eaxp, ULONG *ebxp, ULONG *ecxp, ULONG *edxp)
{
	ULONG eax, ebx, ecx, edx;

	TRACEPRINT(" cpuid \n");
	
	asm volatile("cpuid" 
		: "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx)
		: "a" (info));
	if (eaxp)
		*eaxp = eax;
	if (ebxp)
		*ebxp = ebx;
	if (ecxp)
		*ecxp = ecx;
	if (edxp)
		*edxp = edx;
}

static __inline uint64_t
read_tsc(void)
{
        uint64_t tsc;
        __asm __volatile("rdtsc" : "=A" (tsc));
        return tsc;
}

#endif



/*

	M E M O R Y  M A N A G E R

*/

// Global descriptor numbers
#define GD_KT     0x08     // kernel text
#define GD_KD     0x10     // kernel data
#define GD_UT     0x18     // user text
#define GD_UD     0x20     // user data
#define GD_TSS    0x28     // Task segment selector

/* KERNEL MODE*/

// 1 MB 0x100000
/* Using 100 MB VA for kernel divide between image and data*/

#define	KERNSPACE_START	0x80000000
#define	KERNSPACE_END		0xfffff000 // address of the last page


/*
	This contains:
		Kernel Image 
		PFN database
		Kernel Default Stack(will be used for user to kernel transistion)
*/
#define	KERNBASE				KERNSPACE_START
#define	KERNSIZE				(PTSIZE * 2)		// 8 MB


// POOLS
#define	MMPAGEPOOL_START			(KERNBASE + KERNSIZE)
#define	MMPAGEPOOL_SIZE			(PTSIZE * 10) // 40 MB should be enough
		//else will move it to(PTSIZE * 20)  // 80 MB
#define	MMPAGEPOOL_END			(MMPAGEPOOL_START + MMPAGEPOOL_SIZE)

#define	MMNONPAGEPOOL_START	(MMPAGEPOOL_END)
#define	MMNONPAGEPOOL_SIZE		(PTSIZE * 10) // 40 MB should be enough
		//else will move it to(PTSIZE * 20)  // 80 MB
#define	MMNONPAGEPOOL_END		(MMNONPAGEPOOL_START + MMNONPAGEPOOL_SIZE)


#define	MMSYSPTE_START			(MMNONPAGEPOOL_END)
#define	MMSYSPTE_SIZE				(PTSIZE * 64) // 256 MB
#define	MMSYSPTE_END				(MMSYSPTE_START + MMSYSPTE_SIZE)


/*
	Keep this less than 3GB.
	At 3 GB we have our page directories and tables mapped
*/

#define	MMPAGETABLEAREA_START		0xC0000000										
#define	MM_PDE_ADDRESS				0xC0300000
#define	MMPAGETABLEAREA_SIZE		(PGSIZE * 1024)	// 4MB
#define	MMPAGETABLEAREA_END			(MMPAGETABLEAREA_START 	\
													+ MMPAGETABLEAREA_SIZE)




//Page Mapping Permissions for different Mem types

#define KPOOL_PERM PTE_W | PTE_P


// At IOPHYSMEM (640K) there is a 384K hole for I/O.  From the kernel,
// IOPHYSMEM can be addressed at KERNBASE + IOPHYSMEM.  The hole ends
// at physical address EXTPHYSMEM.
#define IOPHYSMEM	0x0A0000
#define EXTPHYSMEM	0x100000
#define MAXPHYMEM	0x10000000 //256MB


/* USER MODE*/

#define USERTOP	(KERNBASE -PTSIZE)	// Lets keep some margin.


/*
For keeping track of free pages in user mode we will use BITMAP.

One page of bitmap can point 128MB of pages,
but because we also need to put the Bitmap structure 
in same space so we will keep the size some where less than 128MB

In actual we will use less but here we will book 128MB(0x8000000)

*/
#define USERPTEPOOL_SIZE		0x8000000 //128MB
#define USERPTEPOOL_START		((USERTOP) - USERPTEPOOL_SIZE)
#define USERPTEPOOL_END			USERTOP




// Where user programs generally begin
#define USERTEXT		(2*PTSIZE) //from 8 MB Virtual memory 0x800000

// Used for temporary page mappings.
#define USERTEMP		( PTSIZE) //0x400000
#define USERTEMPSIZE	(PTSIZE)
#define USERTEMPEND	(USERTEMP + USERTEMPSIZE)



/*******************************************/

/*
 * x86 memory management unit (MMU)
 */

/*
 *
 *	Part 1.  Paging data structures and constants.
 *
 */



// page number field of address
#define PPN(la)		(((ULONG_PTR) (la)) >> PTXSHIFT)
#define VPN(la)		PPN(la)		// used to index into vpt[]

// page directory index
#define PDX(la)		((((ULONG_PTR) (la)) >> PDXSHIFT) & 0x3FF)
#define VPD(la)		PDX(la)		// used to index into vpd[]

// page table index
#define PTX(la)		((((ULONG_PTR) (la)) >> PTXSHIFT) & 0x3FF)


//

// offset in page
#define PGOFF(la)	(((ULONG_PTR) (la)) & 0xFFF)

#define PGMASK  0xFFF


// construct linear address from indexes and offset
#define PGADDR(d, t, o)	((void*) ((d) << PDXSHIFT | (t) << PTXSHIFT | (o)))

// Page directory and page table constants.
#define NPDENTRIES	1024		// page directory entries per page directory
#define NPTENTRIES	1024		// page table entries per page table

#define PGSIZE		4096		// bytes mapped by a page
#define PGSHIFT		12		// log2(PGSIZE)

#define PTSIZE		(PGSIZE*NPTENTRIES) // bytes mapped by a page directory entry
#define PTSHIFT		22		// log2(PTSIZE)

#define PTXSHIFT	12		// offset of PTX in a linear address
#define PDXSHIFT	22		// offset of PDX in a linear address

// Page table/directory entry flags.
#define PTE_P		0x001	// Present
#define PTE_W		0x002	// Writeable
#define PTE_U		0x004	// User
#define PTE_PWT		0x008	// Write-Through
#define PTE_PCD		0x010	// Cache-Disable
#define PTE_A		0x020	// Accessed
#define PTE_D		0x040	// Dirty
#define PTE_PS		0x080	// Page Size
#define PTE_MBZ		0x180	// Bits must be zero

#define PTE_FLAGS_MASK 0x3FF


// The PTE_AVAIL bits aren't used by the kernel or interpreted by the
// hardware, so user processes are allowed to set them arbitrarily.
#define PTE_AVAIL	0xE00	// Available for software use

// Only flags in PTE_USER
#define PTE_USER	(PTE_AVAIL | PTE_P | PTE_W | PTE_U)


// Control Register flags
#define CR0_PE		0x00000001	// Protection Enable
#define CR0_MP		0x00000002	// Monitor coProcessor
#define CR0_EM		0x00000004	// Emulation
#define CR0_TS		0x00000008	// Task Switched
#define CR0_ET		0x00000010	// Extension Type
#define CR0_NE		0x00000020	// Numeric Errror
#define CR0_WP		0x00010000	// Write Protect
#define CR0_AM		0x00040000	// Alignment Mask
#define CR0_NW		0x20000000	// Not Writethrough
#define CR0_CD		0x40000000	// Cache Disable
#define CR0_PG		0x80000000	// Paging

#define CR4_PCE		0x00000100	// Performance counter enable
#define CR4_MCE		0x00000040	// Machine Check Enable
#define CR4_PSE		0x00000010	// Page Size Extensions
#define CR4_DE		0x00000008	// Debugging Extensions
#define CR4_TSD		0x00000004	// Time Stamp Disable
#define CR4_PVI		0x00000002	// Protected-Mode Virtual Interrupts
#define CR4_VME		0x00000001	// V86 Mode Extensions

// Eflags register
#define FL_CF		0x00000001	// Carry Flag
#define FL_MBS 	0x00000002	// Must be set
#define FL_PF		0x00000004	// Parity Flag
#define FL_AF		0x00000010	// Auxiliary carry Flag
#define FL_ZF		0x00000040	// Zero Flag
#define FL_SF		0x00000080	// Sign Flag
#define FL_TF		0x00000100	// Trap Flag
#define FL_IF		0x00000200	// Interrupt Flag
#define FL_DF		0x00000400	// Direction Flag
#define FL_OF		0x00000800	// Overflow Flag
#define FL_IOPL_MASK	0x00003000	// I/O Privilege Level bitmask
#define FL_IOPL_0	0x00000000	//   IOPL == 0
#define FL_IOPL_1	0x00001000	//   IOPL == 1
#define FL_IOPL_2	0x00002000	//   IOPL == 2
#define FL_IOPL_3	0x00003000	//   IOPL == 3
#define FL_NT		0x00004000	// Nested Task
#define FL_RF		0x00010000	// Resume Flag
#define FL_VM		0x00020000	// Virtual 8086 mode
#define FL_AC		0x00040000	// Alignment Check
#define FL_VIF		0x00080000	// Virtual Interrupt Flag
#define FL_VIP		0x00100000	// Virtual Interrupt Pending
#define FL_ID		0x00200000	// ID flag

// Page fault error codes
#define FEC_PR		0x1	// Page fault caused by protection violation
#define FEC_WR		0x2	// Page fault caused by a write
#define FEC_U		0x4	// Page fault occured while in user mode


/*
 *
 *	Part 2.  Segmentation data structures and constants.
 *
 */

#ifdef __ASSEMBLER__

/*
 * Macros to build GDT entries in assembly.
 */
#define SEG_NULL						\
	.word 0, 0;						\
	.byte 0, 0, 0, 0
#define SEG(type,base,lim)					\
	.word (((lim) >> 12) & 0xffff), ((base) & 0xffff);	\
	.byte (((base) >> 16) & 0xff), (0x90 | (type)),		\
		(0xC0 | (((lim) >> 28) & 0xf)), (((base) >> 24) & 0xff)

#else	// not __ASSEMBLER__

#include <inc/types.h>

// Segment Descriptors
typedef struct _GDTENTRY {
	unsigned sd_lim_15_0 : 16;  // Low bits of segment limit
	unsigned sd_base_15_0 : 16; // Low bits of segment base address
	unsigned sd_base_23_16 : 8; // Middle bits of segment base address
	unsigned sd_type : 4;       // Segment type (see STS_ constants)
	unsigned sd_s : 1;          // 0 = system, 1 = application
	unsigned sd_dpl : 2;        // Descriptor Privilege Level
	unsigned sd_p : 1;          // Present
	unsigned sd_lim_19_16 : 4;  // High bits of segment limit
	unsigned sd_avl : 1;        // Unused (available for software use)
	unsigned sd_rsv1 : 1;       // Reserved
	unsigned sd_db : 1;         // 0 = 16-bit segment, 1 = 32-bit segment
	unsigned sd_g : 1;          // Granularity: limit scaled by 4K when set
	unsigned sd_base_31_24 : 8; // High bits of segment base address
}GDTENTRY, *PGDTENTRY;



// Null segment
#define SEG_NULL	(GDTENTRY){ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
// Segment that is loadable but faults when used
#define SEG_FAULT	(GDTENTRY){ 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0 }
// Normal segment
#define SEG(type, base, lim, dpl) (GDTENTRY)			\
{ ((lim) >> 12) & 0xffff, (base) & 0xffff, ((base) >> 16) & 0xff,	\
    type, 1, dpl, 1, (unsigned) (lim) >> 28, 0, 0, 1, 1,		\
    (unsigned) (base) >> 24 }
#define SEG16(type, base, lim, dpl) (GDTENTRY)			\
{ (lim) & 0xffff, (base) & 0xffff, ((base) >> 16) & 0xff,		\
    type, 1, dpl, 1, (unsigned) (lim) >> 16, 0, 0, 1, 0,		\
    (unsigned) (base) >> 24 }

#endif /* !__ASSEMBLER__ */

// Application segment type bits
#define STA_X		0x8	    // Executable segment
#define STA_E		0x4	    // Expand down (non-executable segments)
#define STA_C		0x4	    // Conforming code segment (executable only)
#define STA_W		0x2	    // Writeable (non-executable segments)
#define STA_R		0x2	    // Readable (executable segments)
#define STA_A		0x1	    // Accessed

// System segment type bits
#define STS_T16A	0x1	    // Available 16-bit TSS
#define STS_LDT		0x2	    // Local Descriptor Table
#define STS_T16B	0x3	    // Busy 16-bit TSS
#define STS_CG16	0x4	    // 16-bit Call Gate
#define STS_TG		0x5	    // Task Gate / Coum Transmitions
#define STS_IG16	0x6	    // 16-bit Interrupt Gate
#define STS_TG16	0x7	    // 16-bit Trap Gate
#define STS_T32A	0x9	    // Available 32-bit TSS
#define STS_T32B	0xB	    // Busy 32-bit TSS
#define STS_CG32	0xC	    // 32-bit Call Gate
#define STS_IG32	0xE	    // 32-bit Interrupt Gate
#define STS_TG32	0xF	    // 32-bit Trap Gate


/*
 *
 *	Part 3.  Traps.
 *
 */

#ifndef __ASSEMBLER__

// Task state segment format (as described by the Pentium architecture book)
typedef struct _KTASKSTATE {
	ULONG TS_link;	// Old TS selector
	ULONG_PTR TS_esp0;	// Stack pointers and segment selectors
	USHORT TS_ss0;	//   after an increase in privilege level
	USHORT TS_padding1;
	ULONG_PTR TS_esp1;
	USHORT TS_ss1;
	USHORT TS_padding2;
	ULONG_PTR TS_esp2;
	USHORT TS_ss2;
	USHORT TS_padding3;
	PHYSICAL_ADDRESS TS_cr3;	// Page directory base
	ULONG_PTR TS_eip;	// Saved state from last task switch
	ULONG TS_eflags;
	ULONG TS_eax;	// More saved state (registers)
	ULONG TS_ecx;
	ULONG TS_edx;
	ULONG TS_ebx;
	ULONG_PTR TS_esp;
	ULONG_PTR TS_ebp;
	ULONG TS_esi;
	ULONG TS_edi;
	USHORT TS_es;		// Even more saved state (segment selectors)
	USHORT TS_padding4;
	USHORT TS_cs;
	USHORT TS_padding5;
	USHORT TS_ss;
	USHORT TS_padding6;
	USHORT TS_ds;
	USHORT TS_padding7;
	USHORT TS_fs;
	USHORT TS_padding8;
	USHORT TS_gs;
	USHORT TS_padding9;
	USHORT TS_ldt;
	USHORT TS_padding10;
	USHORT TS_t;		// Trap on task switch
	USHORT TS_iomb;	// I/O map base address
}KTASKSTATE, *PKTASKSTATE;

// Gate descriptors for interrupts and traps
typedef struct _IDTENTRY {
	unsigned gd_off_15_0 : 16;   // low 16 bits of offset in segment
	unsigned gd_ss : 16;         // segment selector
	unsigned gd_args : 5;        // # args, 0 for interrupt/trap gates
	unsigned gd_rsv1 : 3;        // reserved(should be zero I guess)
	unsigned gd_type : 4;        // type(STS_{TG,IG32,TG32})
	unsigned gd_s : 1;           // must be 0 (system)
	unsigned gd_dpl : 2;         // descriptor(meaning new) privilege level
	unsigned gd_p : 1;           // Present
	unsigned gd_off_31_16 : 16;  // high bits of offset in segment
}IDTENTRY, *PIDTENTRY;

/*
	Set up a normal interrupt/trap gate descriptor.
		- istrap: 1 for a trap (= exception) gate, 0 for an interrupt gate.
			The difference is that entering an interrupt gate
	   		disables interrupts, but entering a trap gate does not.
		- sel: Code segment selector for interrupt/trap handler
		- off: Offset in code segment for interrupt/trap handler
		- dpl: Descriptor Privilege Level -
	the privilege level required for software to invoke
	this interrupt/trap gate explicitly using an int instruction.
*/

#define SETGATE(gate, istrap, sel, off, dpl)			\
{								\
	(gate).gd_off_15_0 = (ULONG) (off) & 0xffff;		\
	(gate).gd_ss = (sel);					\
	(gate).gd_args = 0;					\
	(gate).gd_rsv1 = 0;					\
	(gate).gd_type = (istrap) ? STS_TG32 : STS_IG32;	\
	(gate).gd_s = 0;					\
	(gate).gd_dpl = (dpl);					\
	(gate).gd_p = 1;					\
	(gate).gd_off_31_16 = (ULONG) (off) >> 16;		\
}

// Set up a call gate descriptor.
#define SETCALLGATE(gate, ss, off, dpl)           	        \
{								\
	(gate).gd_off_15_0 = (ULONG) (off) & 0xffff;		\
	(gate).gd_ss = (ss);					\
	(gate).gd_args = 0;					\
	(gate).gd_rsv1 = 0;					\
	(gate).gd_type = STS_CG32;				\
	(gate).gd_s = 0;					\
	(gate).gd_dpl = (dpl);					\
	(gate).gd_p = 1;					\
	(gate).gd_off_31_16 = (ULONG) (off) >> 16;		\
}

// Pseudo-descriptors used for LGDT, LLDT and LIDT instructions.
typedef struct _PSEUDODESC {
	USHORT pd__garbage;         // LGDT supposed to be from address 4N+2
	USHORT pd_lim;              // Limit
	ULONG pd_base __attribute__ ((packed));       // Base address
} PSEUDODESC;
#define PD_ADDR(desc)	(&(desc).pd_lim)


VOID
HalRaiseIrql(
				IN KIRQL NewIrql,
				IN KIRQL CurrentIrql
				);

VOID
HalLowerIrql(
			IN KIRQL NewIrql,
			IN KIRQL CurrentIrql
			);

VOID
HalRegisterVectortoIrql(
			IN KIRQL Irql,
			IN ULONG Vector
			);


#endif /* !__ASSEMBLER__ */





#endif //End of Header

