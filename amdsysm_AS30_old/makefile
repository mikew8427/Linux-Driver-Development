CC=gcc
MODCFLAGS := -Wall -DMODULE -D__KERNEL__ -DLINUX -DMODVERSION 

driver.o:  	syscall.o
		ld -m elf_i386 -r -o driver.o syscall.o
syscall.o:	syscall.c /usr/src/linux-2.4/include/linux/version.h
		$(CC) $(MODCFLAGS) -I/usr/src/linux-2.4.21-4.EL/include -c syscall.c

