ASM				= nasm
BOOTOBJPATH		= ..
BOOTOBJ			= asmboot.obj
BOOTASMFILES	= firstboot.s secondboot.s
BOOTOBJS		= firstboot.obj secondboot.obj

.SUFFIXES: .obj

$(BOOTOBJ): $(BOOTOBJS)
	cat $(BOOTOBJS) > $(BOOTOBJPATH)/$(BOOTOBJ)

.s.obj:
	$(ASM) $< -o $@ -f bin -l $@.lst.asm

secondboot.obj: secondboot.s
firstboot.obj: firstboot.s


.PHONY: clean
clean: 
		rm -f *.obj
	   	rm -f *.o
		rm -f *.BAK
		rm -f *.asm

