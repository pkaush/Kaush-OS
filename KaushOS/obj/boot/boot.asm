
obj/boot/boot.out:     file format elf32-i386


Disassembly of section .text:

00007c00 <start>:
    7c00:	fc                   	cld    
    7c01:	fa                   	cli    
    7c02:	31 c0                	xor    %eax,%eax
    7c04:	8e d8                	mov    %eax,%ds
    7c06:	8e c0                	mov    %eax,%es
    7c08:	8e d0                	mov    %eax,%ss
    7c0a:	bc                   	.byte 0xbc
    7c0b:	00                   	.byte 0x0
    7c0c:	70                   	.byte 0x70

00007c0d <enable_A20>:
    7c0d:	e4 64                	in     $0x64,%al
    7c0f:	a8 02                	test   $0x2,%al
    7c11:	75 fa                	jne    7c0d <enable_A20>
    7c13:	b0 d1                	mov    $0xd1,%al
    7c15:	e6 64                	out    %al,$0x64

00007c17 <enable_A20.enable2>:
    7c17:	e4 64                	in     $0x64,%al
    7c19:	a8 02                	test   $0x2,%al
    7c1b:	75 fa                	jne    7c17 <enable_A20.enable2>
    7c1d:	b0 df                	mov    $0xdf,%al
    7c1f:	e6 60                	out    %al,$0x60
    7c21:	fa                   	cli    
    7c22:	0f 01 16             	lgdtl  (%esi)
    7c25:	63 7c 0f 20          	arpl   %di,0x20(%edi,%ecx,1)
    7c29:	c0 66 83 c8          	shlb   $0xc8,-0x7d(%esi)
    7c2d:	01 0f                	add    %ecx,(%edi)
    7c2f:	22 c0                	and    %al,%al
    7c31:	ea                   	.byte 0xea
    7c32:	36 7c 08             	ss jl  7c3d <protcseg+0x7>
	...

00007c36 <protcseg>:
    7c36:	66 b8 10 00          	mov    $0x10,%ax
    7c3a:	8e d8                	mov    %eax,%ds
    7c3c:	8e c0                	mov    %eax,%es
    7c3e:	8e e0                	mov    %eax,%fs
    7c40:	8e e8                	mov    %eax,%gs
    7c42:	8e d0                	mov    %eax,%ss
    7c44:	e8 c7 00 00 00       	call   7d10 <load_kern>
    7c49:	eb fe                	jmp    7c49 <protcseg+0x13>

00007c4b <gdti>:
	...
    7c53:	ff 07                	incl   (%edi)
    7c55:	00 00                	add    %al,(%eax)
    7c57:	00 9a c0 00 ff 07    	add    %bl,0x7ff00c0(%edx)
    7c5d:	00 00                	add    %al,(%eax)
    7c5f:	00                   	.byte 0x0
    7c60:	92                   	xchg   %eax,%edx
    7c61:	c0                   	.byte 0xc0
	...

00007c63 <pGDT>:
    7c63:	17                   	pop    %ss
    7c64:	00 4b 7c             	add    %cl,0x7c(%ebx)
	...

00007c69 <waitdisk>:
}

static __inline UCHAR inb(int port)
{
    UCHAR data;
    __asm __volatile("inb %w1,%0" : "=a"(data) : "d"(port));
    7c69:	ba f7 01 00 00       	mov    $0x1f7,%edx
    7c6e:	ec                   	in     (%dx),%al
*******************************************************************************/

void waitdisk(void)
{
    // wait for disk reaady
    while ((inb(0x1F7) & 0xC0) != 0x40)
    7c6f:	83 e0 c0             	and    $0xffffffc0,%eax
    7c72:	3c 40                	cmp    $0x40,%al
    7c74:	75 f8                	jne    7c6e <waitdisk+0x5>
        /* do nothing */;
}
    7c76:	c3                   	ret    

00007c77 <readsect>:

void readsect(void *dst, ULONG offset)
{
    7c77:	57                   	push   %edi
    7c78:	8b 4c 24 0c          	mov    0xc(%esp),%ecx
    // wait for disk to be ready
    waitdisk();
    7c7c:	e8 e8 ff ff ff       	call   7c69 <waitdisk>
    __asm __volatile("cld\n\trepne\n\tinsl" : "=D"(addr), "=c"(cnt) : "d"(port), "0"(addr), "1"(cnt) : "memory", "cc");
}

static __inline void outb(int port, UCHAR data)
{
    __asm __volatile("outb %0,%w1" : : "a"(data), "d"(port));
    7c81:	b0 01                	mov    $0x1,%al
    7c83:	ba f2 01 00 00       	mov    $0x1f2,%edx
    7c88:	ee                   	out    %al,(%dx)
    7c89:	ba f3 01 00 00       	mov    $0x1f3,%edx
    7c8e:	88 c8                	mov    %cl,%al
    7c90:	ee                   	out    %al,(%dx)

    outb(0x1F2, 1); // count = 1
    outb(0x1F3, offset);
    outb(0x1F4, offset >> 8);
    7c91:	89 c8                	mov    %ecx,%eax
    7c93:	c1 e8 08             	shr    $0x8,%eax
    7c96:	ba f4 01 00 00       	mov    $0x1f4,%edx
    7c9b:	ee                   	out    %al,(%dx)
    outb(0x1F5, offset >> 16);
    7c9c:	89 c8                	mov    %ecx,%eax
    7c9e:	c1 e8 10             	shr    $0x10,%eax
    7ca1:	ba f5 01 00 00       	mov    $0x1f5,%edx
    7ca6:	ee                   	out    %al,(%dx)
    outb(0x1F6, (offset >> 24) | 0xE0);
    7ca7:	89 c8                	mov    %ecx,%eax
    7ca9:	c1 e8 18             	shr    $0x18,%eax
    7cac:	83 c8 e0             	or     $0xffffffe0,%eax
    7caf:	ba f6 01 00 00       	mov    $0x1f6,%edx
    7cb4:	ee                   	out    %al,(%dx)
    7cb5:	b0 20                	mov    $0x20,%al
    7cb7:	ba f7 01 00 00       	mov    $0x1f7,%edx
    7cbc:	ee                   	out    %al,(%dx)
    outb(0x1F7, 0x20); // cmd 0x20 - read sectors

    // wait for disk to be ready
    waitdisk();
    7cbd:	e8 a7 ff ff ff       	call   7c69 <waitdisk>
    __asm __volatile("cld\n\trepne\n\tinsl" : "=D"(addr), "=c"(cnt) : "d"(port), "0"(addr), "1"(cnt) : "memory", "cc");
    7cc2:	8b 7c 24 08          	mov    0x8(%esp),%edi
    7cc6:	b9 80 00 00 00       	mov    $0x80,%ecx
    7ccb:	ba f0 01 00 00       	mov    $0x1f0,%edx
    7cd0:	fc                   	cld    
    7cd1:	f2 6d                	repnz insl (%dx),%es:(%edi)

    // read a sector
    // count = SECTSIZE/4 bcos ins internally ends up using insd which loads 4 bytes(one word)at a time
    insl(0x1F0, dst, SECTSIZE / 4);
}
    7cd3:	5f                   	pop    %edi
    7cd4:	c3                   	ret    

00007cd5 <readseg>:
{
    7cd5:	57                   	push   %edi
    7cd6:	56                   	push   %esi
    7cd7:	53                   	push   %ebx
    7cd8:	8b 5c 24 10          	mov    0x10(%esp),%ebx
    va &= 0xFFFFFF;
    7cdc:	89 df                	mov    %ebx,%edi
    7cde:	81 e7 ff ff ff 00    	and    $0xffffff,%edi
    end_va = va + count;
    7ce4:	03 7c 24 14          	add    0x14(%esp),%edi
    va &= ~(SECTSIZE - 1);
    7ce8:	81 e3 00 fe ff 00    	and    $0xfffe00,%ebx
    offset = (offset / SECTSIZE) + 1;
    7cee:	8b 74 24 18          	mov    0x18(%esp),%esi
    7cf2:	c1 ee 09             	shr    $0x9,%esi
    7cf5:	46                   	inc    %esi
    while (va < end_va)
    7cf6:	39 fb                	cmp    %edi,%ebx
    7cf8:	73 12                	jae    7d0c <readseg+0x37>
        readsect((UCHAR *)va, offset);
    7cfa:	56                   	push   %esi
    7cfb:	53                   	push   %ebx
    7cfc:	e8 76 ff ff ff       	call   7c77 <readsect>
        va += SECTSIZE;
    7d01:	81 c3 00 02 00 00    	add    $0x200,%ebx
        offset++;
    7d07:	46                   	inc    %esi
    7d08:	58                   	pop    %eax
    7d09:	5a                   	pop    %edx
    7d0a:	eb ea                	jmp    7cf6 <readseg+0x21>
}
    7d0c:	5b                   	pop    %ebx
    7d0d:	5e                   	pop    %esi
    7d0e:	5f                   	pop    %edi
    7d0f:	c3                   	ret    

00007d10 <load_kern>:
{
    7d10:	56                   	push   %esi
    7d11:	53                   	push   %ebx
    7d12:	50                   	push   %eax
    readseg((ULONG)ELFHDR, SECTSIZE * 8, 0);
    7d13:	6a 00                	push   $0x0
    7d15:	68 00 10 00 00       	push   $0x1000
    7d1a:	68 00 00 01 00       	push   $0x10000
    7d1f:	e8 b1 ff ff ff       	call   7cd5 <readseg>
    if (ELFHDR->e_magic != ELF_MAGIC)
    7d24:	83 c4 0c             	add    $0xc,%esp
    7d27:	81 3d 00 00 01 00 7f 	cmpl   $0x464c457f,0x10000
    7d2e:	45 4c 46 
    7d31:	75 3d                	jne    7d70 <load_kern+0x60>
    ph = (struct Proghdr *)((UCHAR *)ELFHDR + ELFHDR->e_phoff);
    7d33:	a1 1c 00 01 00       	mov    0x1001c,%eax
    7d38:	8d 98 00 00 01 00    	lea    0x10000(%eax),%ebx
    eph = ph + ELFHDR->e_phnum;
    7d3e:	0f b7 35 2c 00 01 00 	movzwl 0x1002c,%esi
    7d45:	c1 e6 05             	shl    $0x5,%esi
    7d48:	01 de                	add    %ebx,%esi
    for (; ph < eph; ph++)
    7d4a:	39 f3                	cmp    %esi,%ebx
    7d4c:	73 16                	jae    7d64 <load_kern+0x54>
        readseg(ph->p_va, ph->p_memsz, ph->p_offset);
    7d4e:	ff 73 04             	push   0x4(%ebx)
    7d51:	ff 73 14             	push   0x14(%ebx)
    7d54:	ff 73 08             	push   0x8(%ebx)
    7d57:	e8 79 ff ff ff       	call   7cd5 <readseg>
    for (; ph < eph; ph++)
    7d5c:	83 c3 20             	add    $0x20,%ebx
    7d5f:	83 c4 0c             	add    $0xc,%esp
    7d62:	eb e6                	jmp    7d4a <load_kern+0x3a>
    ((void (*)(void))(ELFHDR->e_entry & 0xFFFFFF))();
    7d64:	a1 18 00 01 00       	mov    0x10018,%eax
    7d69:	25 ff ff ff 00       	and    $0xffffff,%eax
    7d6e:	ff d0                	call   *%eax
    __asm __volatile("cld\n\trepne\n\toutsb" : "=S"(addr), "=c"(cnt) : "d"(port), "0"(addr), "1"(cnt) : "cc");
}

static __inline void outw(int port, USHORT data)
{
    __asm __volatile("outw %0,%w1" : : "a"(data), "d"(port));
    7d70:	ba 00 8a 00 00       	mov    $0x8a00,%edx
    7d75:	b8 00 8a ff ff       	mov    $0xffff8a00,%eax
    7d7a:	66 ef                	out    %ax,(%dx)
    7d7c:	b8 00 8e ff ff       	mov    $0xffff8e00,%eax
    7d81:	66 ef                	out    %ax,(%dx)
    7d83:	eb fe                	jmp    7d83 <load_kern+0x73>
