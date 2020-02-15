COMPILER = gcc																		# Setting the compiler for C to var.
LINKER = ld																				# Setting the linker.
ASSEMBLER = nasm																	# Setting the compiler for C to var.
CFLAGS = -m32 -c -ffreestanding										# Adding the flags of C for the terminal code.
ASFLAGS = -f elf32																# Adding the flags for the terminal code in assemble language.
LDFLAGS = -m elf_i386 -T src/link.ld							# Adding the flags for the linker.
EMULATOR = qemu-system-i386												# Setting the emulator for the OS, pre-ISO.
EMULATOR_FLAGS = -kernel													# Adding the flags for the emulator.

OBJS = obj/kasm.o obj/kc.o obj/idt.o obj/isr.o obj/kb.o obj/screen.o obj/string.o obj/system.o obj/util.o obj/shell.o
OUTPUT = kernel/boot/kernel.bin

run: all
	$(EMULATOR) $(EMULATOR_FLAGS) $(OUTPUT)

all:$(OBJS)
	mkdir kernel/ -p																# Making the directory for kernel/
	mkdir kernel/boot/ -p														# Making the directory under kernel/boot/
	$(LINKER) $(LDFLAGS) -o $(OUTPUT) $(OBJS)

# Compiling the assembli code.
obj/kasm.o:src/kernel.asm
	$(ASSEMBLER) $(ASFLAGS) -o obj/kasm.o src/kernel.asm

# Compiling the C code.
obj/kc.o:src/kernel.c
	$(COMPILER) $(CFLAGS) src/kernel.c -o obj/kc.o

obj/idt.o:src/idt.c
	$(COMPILER) $(CFLAGS) src/idt.c -o obj/idt.o

obj/kb.o:src/kb.c
	$(COMPILER) $(CFLAGS) src/kb.c -o obj/kb.o

obj/isr.o:src/isr.c
	$(COMPILER) $(CFLAGS) src/isr.c -o obj/isr.o

obj/screen.o:src/screen.c
	$(COMPILER) $(CFLAGS) src/screen.c -o obj/screen.o

obj/string.o:src/string.c
	$(COMPILER) $(CFLAGS) src/string.c -o obj/string.o

obj/system.o:src/system.c
	$(COMPILER) $(CFLAGS) src/system.c -o obj/system.o

obj/util.o:src/util.c
	$(COMPILER) $(CFLAGS) src/util.c -o obj/util.o

obj/shell.o:src/shell.c
	$(COMPILER) $(CFLAGS) src/shell.c -o obj/shell.o

# Building the rest of the objects into a .bin emulated file & the ISO.
build:all
	#Activate the install xorr if you do not have it already installed
	#sudo apt-get install xorriso
	rm kernel/boot/grub/ -r -f					# Removing this dir.
	mkdir kernel/boot/grub/							#	Making a new dir under kernel/boot/grub/.
	echo set default=0 >> kernel/boot/grub/grub.cfg
	echo set timeout=0 >> kernel/boot/grub/grub.cfg
	echo menuentry "Testing" { >> kernel/boot/grub/grub.cfg
	echo         set root='(hd96)' >> kernel/boot/grub/grub.cfg
	echo         multiboot /boot/kernel.bin >> kernel/boot/grub/grub.cfg
	echo } >> kernel/boot/grub/grub.cfg

	grub-mkrescue -o kernel.iso kernel/		# Creating the ISO for the OS.

clear:
	rm -f obj/*.o							# Remove all the files that are under obj & have .o at the end.
	rm -r -f kernel/					# Remove everything under the folder kernel.
