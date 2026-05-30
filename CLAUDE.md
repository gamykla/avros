# AVROS — Project Notes for Agents

AVROS is a small multi-tasking operating system for the ATmega128 microcontroller,
written in C and AVR assembly (originally a 2004 thesis project). It boots, sets up
a timer-driven pre-emptive round-robin scheduler, traps system calls via INT4, and
multiplexes several user processes on a single core.

## Architecture overview

- `kernel.s` — bootstrap (`K_init`), the INT4 trap handler (`K_trap`), the timer0
  overflow handler (`tc0_ovf`), and `K_panic`. Includes `vector_table.s`,
  `SRAM_map.s`, `timer.s`, and `ATmega128defs.s`.
- `kernelC.c` — C kernel core: `create_initial_process`, `K_syscall`, the
  `schedule()` / `dispatch()` scheduler.
- `ksyscalls.c` — kernel-side syscall implementations (`K_*`).
- `syscall_interface.c` — user-side syscall stubs that trap into the kernel.
- `user_init.c` — the first user process (PID 0) plus demo child processes.
- `SRAM_map.h` / `SRAM_map.s` — fixed SRAM layout (PD block, syscall params, etc.).

### PORTA status codes

The kernel reports execution milestones by writing single-byte codes to PORTA
(I/O port 0x1B, data-space address 0x3B). The full list is in
`PORTA-stat_codes.txt`. `0xFF` is `K_panic` — an unrecoverable crash.

## Build

The flash image is built with the AVR GNU toolchain into `build/`:

```
make elf         # build build/flashImage.elf and build/flashImage.hex
```

Objects: `kernel.o`, `kernelC.o`, `ksyscalls.o`, `syscall_interface.o`,
`user_init.o`. Compiled with `avr-gcc -mmcu=atmega128 -O2 -g`, linked with
`avr-ld -arch=atmega128`, HEX produced with `avr-objcopy -j .text -j .data -O ihex`.

## Build and test harness

A reproducible simulavr-based harness builds the image and runs it on a simulated
ATmega128, observing the PORTA status codes to assert boot and scheduler progress.
Full details and the assertion contract live in `tools/README.md`.

```
make preflight   # verify avr-gcc, simulavr, python3 are installed
make elf         # build build/flashImage.elf (+ .hex)
make sim-run     # run sim, dump build/porta.vcd, print observed PORTA codes
make sim-test    # run sim + assert boot/scheduler progress; exit non-zero on fail
make clean       # remove build/
```

`sim-test` is the regression gate: it FAILS if PORTA ever reaches `0xFF` (K_panic)
anywhere in the trace, and PASSES only if the expected boot, user_init, and
scheduler codes all appear. Expected/forbidden code sets are CLI-configurable on
`tools/check_porta_trace.py` so the syscall-race regression test can tighten them
without code changes.

The simulation is bounded by `SIM_MAX_NS` (simulated time, default 100 ms) and
`WALL_TIMEOUT` (wall-clock seconds, default 30) because the scheduler loops forever.
simulavr's PORTA→VCD signal name varies by build; if the CLI path yields no usable
trace, the harness falls back to a pysimulavr register-polling runner (see
`tools/README.md`).

## Toolchain

Install prerequisites with:

```
sudo apt install gcc-avr binutils-avr avr-libc simulavr
```
