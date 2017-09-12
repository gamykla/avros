#****************************************************************
#   AVROS: ATmega128 Operating System
#   (c) gamykla
#
#  $Source: /u/csc02g/gamykla/AVROS-CVS/avros/Makefile,v $
#  $Author: gamykla $
#  $Date: 2004/04/17 00:39:43 $
#  $Revision: 1.1 $
#
#****************************************************************

all: kernel.o kernelC.o syscall_interface.o flashImage.o hex

kernel.o: kernel.s SRAM_map.s timer.s vector_table.s
	avr-gcc -O2 -g -Wall -mmcu=atmega128 -c kernel.s

kernelC.o: kernelC.c kernelC.h SRAM_map.h errors.h timer.h
	avr-gcc  -O2 -mmcu=atmega128 -c kernelC.c

syscall_interface.o: syscall_interface.c syscall_interface.h
	avr-gcc -O2 -mmcu=atmega128 -c syscall_interface.c

ksyscalls.o: ksyscalls.h ksyscalls.c
	avr-gcc -O2 -mmcu=atmega128 -c ksyscalls.c

flashImage.o: kernel.o syscall_interface.o user_init.o kernelC.o ksyscalls.o
	avr-ld -arch=atmega128 kernel.o kernelC.o ksyscalls.o syscall_interface.o user_init.o -o flashImage.o


hex: flashImage.o
	avr-objcopy -j .text -j .data -O ihex flashImage.o flashImage.hex

user_init.o: user_init.c user_init.h
	avr-gcc  -O2 -mmcu=atmega128 -c user_init.c


clean:
	rm *.o *.hex *.aps

