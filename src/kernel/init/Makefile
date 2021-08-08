include file_size.mk

INIT_AS       := i686-elf-as
INIT_GCC      := i686-elf-gcc
INIT_READELF  := i686-elf-readelf
INIT_OBJCOPY  := i686-elf-objcopy
# Disable 'schedule-insns2' because it was causing incorrect behavior when writing to the VGA text
# buffer
INIT_CFLAGS   := -std=c99 -ffreestanding -O2 -Wall -Wextra -Werror -pedantic -fno-schedule-insns2
INIT_LFLAGS   := -T init.ld -ffreestanding -O2 -nostdlib -lgcc

IMGS := init.bin

OBJDIR := $(OBJDIR)/$(notdir $(CURDIR))
VPATH  := $(OBJDIR)

# This environment variable is intended to be used by the sort utility run from
# this Makefile when sorting hexadecimal numbers of the same length. C locale
# collating sequence should place numbers [0-9] before letters [a-f].
LC_COLLATE := C
export LC_COLLATE

.PHONY: all $(IMGS) clean

all: $(IMGS)

boot.o: boot.asm | $(OBJDIR)
	$(INIT_AS) -o $(OBJDIR)/$@ $<

%.o: %.c assert.h | $(OBJDIR)
	$(INIT_GCC) $(INIT_CFLAGS) -o $(OBJDIR)/$@ -c $<

init.bin: boot.o init.o io_port.o multiboot.o pci.o terminal.o
	# Link to ELF format
	$(INIT_GCC) $(INIT_LFLAGS) -o $(OBJDIR)/$(@:%.bin=%.elf) $(addprefix $(OBJDIR)/,$(notdir $^))
	# Convert ELF to binary file for easy loading with the secondary stage bootloader
	$(INIT_OBJCOPY) -O binary $(OBJDIR)/$(@:%.bin=%.elf) $(OBJDIR)/$@
	# Get the lowest physical address of the loadable sections in the ELF file
	#
	#  readelf - Print information about sections in the .ELF file
	#  awk     - Get physical addresses from the lines describing loadable sections
	#  sort    - Sort the hexadecimal addresses in ascending order (LC_COLLATE=C)
	#  head    - Get the first address
	echo KERNEL_INIT_LOAD_OFFSET equ $$( \
	  $(INIT_READELF) -l $(OBJDIR)/$(@:%.bin=%.elf) | \
	  awk '/^ *LOAD/ { print $$4 }' | \
	  sort | \
	  head -n 1 \
	  ) \
	  > $(OBJDIR)/info_kernel_init.inc
	# Get an address of the entry point
	echo KERNEL_INIT_ENTRY_OFFSET equ $$( \
	  $(INIT_READELF) -h $(OBJDIR)/$(@:%.bin=%.elf) | \
	  grep 'Entry point address' | \
	  head -n 1 | \
          awk '{ print $$4 }' \
	  ) \
	  >> $(OBJDIR)/info_kernel_init.inc
	# Get a size of the binary image in sectors
	echo KERNEL_INIT_SECTOR_COUNT equ \
	   $(call file_size_in_sectors,$(OBJDIR)/$@) \
	  >> $(OBJDIR)/info_kernel_init.inc

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR)
