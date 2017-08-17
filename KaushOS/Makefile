OBJDIR := obj


SETUP_TOOLS_DIR := /usr/local/cross

COMPILER := i386-kosj-elf


USER_LIBC_INCLUDE := -I$(SETUP_TOOLS_DIR)/$(COMPILER)/include -I$(SETUP_TOOLS_DIR)/lib/gcc/$(COMPILER)/4.5.0/include

#GCCPREFIX=i386-kos-elf-
#GCCPREFIX=/cygdrive/c/mine/projects/newtools/BUILD/bin/bin/i686-kjaguar-elf-
#GCCPREFIX=/cygdrive/c/mine/projects/newtools/BUILD_i386/bin/bin/i386-kjaguar-elf-
GCCPREFIX=$(SETUP_TOOLS_DIR)/bin/$(COMPILER)-

#V = @


TOP = .

CC	:= $(GCCPREFIX)gcc -pipe
GCC_LIB := $(shell $(CC) -print-libgcc-file-name)
AS	:= $(GCCPREFIX)as
AR	:= $(GCCPREFIX)ar
LD	:= $(GCCPREFIX)ld
OBJCOPY	:= $(GCCPREFIX)objcopy
OBJDUMP	:= $(GCCPREFIX)objdump
NM	:= $(GCCPREFIX)nm

# Native commands
NCC	:= gcc $(CC_VER) -pipe
TAR	:= gtar
PERL	:= perl

# Compiler flags
# Note that -O2 is required for the boot loader to fit within 512 bytes;
# -fno-builtin is required to avoid refs to undefined functions in the kernel.
CFLAGS	:= $(CFLAGS) $(DEFS) -O2 -fno-builtin -I$(TOP) -MD -Wall -Wno-format

# Linker flags for kos user programs
ULDFLAGS := 

SRCDIRS :=
OBJDIRS :=


KERN_CFLAGS := $(CFLAGS) -DKOS_KERNEL -gstabs -Wno-multichar -Wno-implicit-function-declaration -march=i486
USER_CFLAGS := $(CFLAGS) -DKOS_USER -gstabs



# Make sure that 'all' is the first target
all:




# Include Makefrags for subdirectories

include user/Lib/Makefrag
include user/prog/Makefrag
include boot/Makefrag
include kern/Makefrag

#include user/Makefrag

images: $(OBJDIR)/kern/disk.img

bochs: images
	bochs 'display_library: nogui'

# For deleting the build
clean:
	rm -rf $(OBJDIR)

# This magic automatically generates makefile dependencies
# for header files included from C source files we compile,
# and keeps those dependencies up-to-date every time we recompile.
# See 'mergedep.pl' for more information.
$(OBJDIR)/.deps: $(foreach dir, $(OBJDIRS), $(wildcard $(OBJDIR)/$(dir)/*.d))
	@mkdir -p $(@D)
	@$(PERL) mergedep.pl $@ $^

-include $(OBJDIR)/.deps
