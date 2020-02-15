nasm -f elf32 kernel.asm -o obj/kasm.o                                          # Compiling the assembly language to an boject.
gcc -m32 -c kernel.c -o obj/kc.o -ffreestanding                                 # Compiling the C code to an object.
gcc -m32 -c include/system.c -o obj/system.o -ffreestanding                     # Compiling the C code to an object.
gcc -m32 -c include/string.c -o obj/string.o -ffreestanding                     # Compiling the C code to an object.
gcc -m32 -c include/screen.c -o obj/screen.o -ffreestanding                     # Compiling the C code to an object.
gcc -m32 -c include/kb.c -o obj/kb.o -ffreestanding                             # Compiling the C code to an object.
# Linker for linking everything together and creating the bin file to run.
ld -m elf_i386 -T link.ld -o build-iso/boot/kernel.bin obj/kasm.o obj/kc.o obj/system.o obj/string.o obj/screen.o obj/kb.o
qemu-system-i386 -kernel build-iso/boot/kernel.bin
#grub-mkrescue -o fullISO.iso build-iso/

read a
