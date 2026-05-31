# AVROS — ATmega128 Operating System

A preemptive, priority-based multitasking operating system written from scratch in C
and AVR assembly for the Atmel ATmega128 microcontroller. Originally a thesis project
(CSC499, University of Toronto, 2004) under Professor Steven Mann, inspired by the
microprocessor systems architecture course taught by Professor Michael Stumm.

---

## What it is

AVROS is a complete, self-contained operating system kernel running on bare metal — no
host OS, no RTOS library, no HAL. On a microcontroller with 4 KB of on-chip SRAM and
no memory management unit, it provides:

- **Preemptive multitasking** via Timer0 overflow interrupts at 10 Hz. The scheduler
  runs entirely inside the interrupt handler, context-switching without any cooperation
  from user processes.
- **Priority-based round-robin scheduling** across up to 6 concurrent processes, with
  three priority levels (low/normal/high) and O(n) selection.
- **Full context save and restore** in AVR assembly — all 32 general-purpose registers,
  the status register, and the program counter are saved to the active process's stack
  on every context switch and interrupt.
- **Software syscall trap** via external interrupt INT4 (pin PE4). User processes
  trigger a kernel mode transition by pulling the pin low, passing an opcode and up to
  five parameters through fixed kernel SRAM locations.
- **14 system calls**: process creation, termination (kill/quit), yield, sleep,
  suspend/wakeup, priority management, parent/child PID query, and integer message
  passing between processes.
- **Inter-process messaging** with per-process mailboxes holding up to 10 pending
  messages, encoded as packed integers carrying sender PID and payload.
- **An idle task** that holds the scheduler's ready set non-empty, preventing the CPU
  from freezing when all user processes are sleeping or blocked.

The kernel is split across two layers: a hand-written AVR assembly core (`kernel.s`)
that owns the interrupt vector table, bootstrap sequence, and all context-switch
mechanics; and a C layer (`kernelC.c`, `ksyscalls.c`) that implements the scheduler,
dispatcher, and individual system call logic.

---

## Significance of OS design

In 2004, the ATmega128 was a capable but severely constrained device: 128 KB of flash,
4 KB of on-chip SRAM, and a single-issue 8-bit ALU running at a few MHz. There was no
hardware divide, no hardware multiply beyond unsigned 8-bit, no barrel shifter, and no
stack protection of any kind. The C runtime provided by avr-libc was thin. There was no
FreeRTOS port for the AVR5 family with a stable ABI, and commercial RTOS products for
8-bit AVR were rare, expensive, or required proprietary toolchains.

Writing a preemptive kernel on this hardware meant solving problems that modern
platforms handle in silicon or in mature libraries:

**Manual stack partitioning.** With 4 KB of SRAM shared between the kernel stack, six
process stacks, a heap, and the process descriptor block, every byte was budgeted by
hand in `SRAM_map.h`. The kernel stack, user stack region, and heap have hard-coded
absolute boundaries — there is no `malloc`, no virtual address space, and no MMU to
enforce them.

**Context switching in pure assembly.** The C compiler cannot save and restore all
32 AVR registers atomically across an interrupt boundary in a portable way. Every context
switch is written in assembly: push all 32 registers onto the preempted process's stack,
save SP and SREG to kernel memory, swap to the kernel stack, run the C scheduler, load
the chosen process's SP and SREG, pop all 32 registers, and `reti`. Both the syscall
trap (INT4) and the Timer0 preemption handler are written this way.

**Two independent interrupt paths.** Cooperative yields happen through the INT4 software
trap (a voluntary kernel entry). Preemption happens through Timer0 overflow (an
involuntary kernel entry fired by hardware). Both paths must save and restore context
identically, because after a syscall the kernel may dispatch a *different* process than
the one that called — the caller does not return from the syscall until it is next
scheduled.

**Return value delivery across a context switch.** When a process makes a syscall and
the kernel switches to another process, the original caller's return value must survive
until the caller is rescheduled. AVROS delivers it through the AVR C ABI: the ASM trap
handler loads the return value into r24:r25 (the 16-bit return register pair) before
`reti`, so the value is present in registers when the caller resumes — no shared global
mailbox is needed at user space.

**No operating system primitives to build on.** The bootstrap sequence — setting the
stack pointer, initializing the process descriptor block, wiring the interrupt vectors,
arming Timer0, and jumping to the first user process — is written entirely by hand in
nine instructions of assembly before a single C function is called.

---

## Architecture overview

```
Flash (128 KB)
  [0x0000] Interrupt vector table  — 35 entries, 4 bytes each
           K_init (bootstrap)       — sets SP, wires Timer0 + INT4, creates PID 0
           K_trap (INT4 ISR)        — syscall entry: save context → kernel → restore
           tc0_ovf (Timer0 ISR)     — preempt entry: same mechanics, calls timer handler
           K_panic                  — writes 0xFF to PORTA, halts

SRAM (4 KB: 0x0100–0x10FF)
  [0x0100] Kernel static data       — KSP, ACTIVE index, syscall params, scratch
  [0x011D] Process descriptor block — 6 × struct PD (34 bytes each)
  [0x04E9] Heap
  [0x08D2] User stack space         — divided equally across MAX_PROCS
  [0x1037] Kernel stack
  [0x10FF] RAMEND (initial SP)
```

The scheduler selects the highest-priority S_READY process starting from (active+1),
round-robining within a priority level. Sleeping processes have their wait times
decremented on every Timer0 tick; they transition to S_READY when their countdown
expires.

---

## System calls

| Call | Description |
|------|-------------|
| `create_process(pc, prio)` | Spawn a new process at the given function pointer |
| `quit()` | Terminate self and free the process slot |
| `kill(pid)` | Terminate another process |
| `yield()` | Voluntarily relinquish the CPU |
| `sleep(ms)` | Sleep for approximately N milliseconds (100 ms granularity) |
| `suspend(pid)` | Block another process indefinitely |
| `wakeup(pid)` | Unblock a suspended process |
| `get_pid()` | Return the calling process's PID |
| `get_ppid()` | Return the parent process's PID |
| `get_prio()` / `set_prio(pid, prio)` | Read or write a process's priority |
| `snd_msg(pid, byte)` | Send an 8-bit message to another process's mailbox |
| `check_msg()` | Non-blocking check for pending messages |
| `get_msg()` | Retrieve the most recent pending message |

---

## Build and test

Requires: `gcc-avr`, `binutils-avr`, `avr-libc`, `simavr`, `avr-gdb`

```
sudo apt install gcc-avr binutils-avr avr-libc simavr
```

```
make preflight   # verify toolchain
make elf         # build build/flashImage.elf + build/flashImage.hex
make sim-test    # run on simulated ATmega128 and assert correctness
make clean
```

`make sim-test` runs the kernel on a simulated ATmega128 via simavr, observing PORTA
status codes through a GDB breakpoint harness. It passes when the full bootstrap
sequence completes (codes 0x01–0x05) and the preemptive scheduler is confirmed running,
with no kernel panic (PORTA never reaches 0xFF).

Flash `build/flashImage.hex` to real hardware with avrdude.

---

## Debugging

The kernel writes single-byte status codes to PORTA (I/O address 0x1B) at key execution
milestones. On hardware, read these with an oscilloscope or LEDs connected to PORTA
pins. Code 0xFF indicates an unrecoverable kernel panic. See `PORTA-stat_codes.txt` for
the full table.

---

*Written in C and AVR assembly. (c) gamykla 2004. Use freely.*
