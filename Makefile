MAKE			= make
CC				= gcc
LD				= ld
LDFLAGS			= 
LDMAPFILENAME	= gfw_map.txt
LDENTRYPOINT	= startup_32
LDTEXTSECVAL	= 0x800
ASM				= nasm
CFLAGS			= -nostdlib -ffreestanding -Wall -Wshadow -O0
BOOTFILEPATH	= ./boot_files
BOOTFILENAME	= boot.bin
BOOTFDIMAGE		= fdimage.bin
BOOTASMPATH		= ./bootasm
BOOTOBJ			= asmboot.obj
INCLUDES		= -I. -I./includes
SRC				= start.c main_init.c io.c idt.c vga.c system.c page.c keyboard.c memory.c  \
				ATAdrv.c i8259A.c gdebug.c tss.c BIOSemu.c CPUemu.c FDCdrv.c device.c ATAemu.c \
				vm8086.c pci-main.c cpuseg_emu.c dp8390.c ctrl_reg_emu.c vm.c io_emu.c gdt.c irq.c \
				packet.c ethernet.c interrupt_emu.c tss_emu.c page_emu.c ldt_emu.c clock.c irq_emu.c \
				i8259A_emu.c eflags_emu.c ignore_int.c task.c mm.c misc.c
COBJS    		= $(patsubst %.c,%.o,$(SRC))
DEPENDS 		= $(patsubst %.c,%.d,$(SRC))
CPACKEDOBJ		= c.obj
ASMOBJS			= idts.o
FINALOBJS		= $(BOOTOBJ) $(CPACKEDOBJ)
QEMUPATH		= qemu-system-x86_64

$(BOOTFILENAME): $(FINALOBJS)
	cat $(FINALOBJS) > $(BOOTFILEPATH)/$(BOOTFILENAME)
	dd if=/dev/zero of=$(BOOTFILEPATH)/$(BOOTFDIMAGE) bs=1KiB count=1440
	dd if=$(BOOTFILEPATH)/$(BOOTFILENAME) of=$(BOOTFILEPATH)/$(BOOTFDIMAGE) conv=notrunc

$(BOOTOBJ):
	$(MAKE) -C $(BOOTASMPATH)

.SUFFIXES: .s

.s.o:
	$(ASM) $< -f elf -l $@.lst.asm

.c.o:
	$(CC) $(INCLUDES) $(CFLAGS) -c $<

%.d: %.c
	@set -e; $(CC) -MM $(CPPFLAGS) $(INCLUDES) $< \
	| sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
	[ -s $@ ] || rm -f $@

c_tmp.o: $(COBJS) $(ASMOBJS)
	$(LD) -o $@ -Map $(LDMAPFILENAME) -e $(LDENTRYPOINT) -Ttext $(LDTEXTSECVAL) $(LDFLAGS) $(COBJS) $(ASMOBJS)

$(CPACKEDOBJ): c_tmp.o
	objcopy -S -O binary $< $@

.PHONY: clean test depend
clean:
	$(MAKE) -C $(BOOTASMPATH) clean
	rm -f *.obj
	rm -f *.o
	rm -f *.BAK
	rm -f *.asm
	rm -f *.d

test:
	$(QEMUPATH) -m 256 -boot a -fda $(BOOTFILEPATH)/$(BOOTFILENAME)

-include $(DEPENDS)
