#****************************************************************
#   AVROS: ATmega128 Operating System
#   (c) gamykla
#
#   Build and simulation-test harness.
#****************************************************************

CC      = avr-gcc
LD      = avr-gcc
OBJCOPY = avr-objcopy

MCU     = atmega128
CFLAGS  = -mmcu=$(MCU) -O2 -g
# -nostartfiles: kernel provides its own vector table and main: label; skip crt0.
# Using avr-gcc as linker driver (not avr-ld directly) pulls in libgcc automatically,
# which provides __tablejump2__ needed by the switch dispatch in K_syscall().
LDFLAGS = -mmcu=$(MCU) -nostartfiles

BUILD = build

# Explicit object set. The original Makefile listed these loose objects in its
# `all` target in an order that did not reliably build ksyscalls.o / user_init.o
# before the link; they are now pinned and every object is a link prerequisite.
OBJECTS = \
	$(BUILD)/kernel.o \
	$(BUILD)/kernelC.o \
	$(BUILD)/ksyscalls.o \
	$(BUILD)/syscall_interface.o \
	$(BUILD)/user_init.o

ELF = $(BUILD)/flashImage.elf
HEX = $(BUILD)/flashImage.hex

# Default target builds the flash artifact.
all: $(HEX)

$(BUILD):
	mkdir -p $(BUILD)

# kernel.s pulls in the SRAM map, timer, and vector table via .include.
$(BUILD)/kernel.o: kernel.s SRAM_map.s timer.s vector_table.s ATmega128defs.s | $(BUILD)
	$(CC) $(CFLAGS) -Wall -c kernel.s -o $@

$(BUILD)/kernelC.o: kernelC.c kernelC.h SRAM_map.h errors.h timer.h ksyscalls.h | $(BUILD)
	$(CC) $(CFLAGS) -c kernelC.c -o $@

$(BUILD)/ksyscalls.o: ksyscalls.c ksyscalls.h | $(BUILD)
	$(CC) $(CFLAGS) -c ksyscalls.c -o $@

$(BUILD)/syscall_interface.o: syscall_interface.c syscall_interface.h | $(BUILD)
	$(CC) $(CFLAGS) -c syscall_interface.c -o $@

$(BUILD)/user_init.o: user_init.c user_init.h | $(BUILD)
	$(CC) $(CFLAGS) -c user_init.c -o $@

# Link to ELF (consumed by simulavr), then emit the real flash HEX.
$(ELF): $(OBJECTS) | $(BUILD)
	$(LD) $(LDFLAGS) $(OBJECTS) -o $(ELF)

$(HEX): $(ELF)
	$(OBJCOPY) -j .text -j .data -O ihex $(ELF) $(HEX)

# Verify the toolchain is present before attempting a build or sim run.
preflight:
	./tools/preflight.sh

# Build the ELF (and HEX by-product).
elf: $(ELF)

# Build + run simulavr, dump build/porta.vcd, print observed PORTA codes. No assertion.
sim-run: $(ELF)
	./tools/run_sim.sh $(ELF) $(BUILD)/porta.vcd
	python3 tools/check_porta_trace.py --observe $(BUILD)/porta.vcd

# Build + run + assert. Exits non-zero on failure. This is the regression gate.
sim-test: $(ELF)
	./tools/run_sim.sh $(ELF) $(BUILD)/porta.vcd
	python3 tools/check_porta_trace.py $(BUILD)/porta.vcd

clean:
	rm -rf $(BUILD)

.PHONY: all preflight elf sim-run sim-test clean
