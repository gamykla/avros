# AVROS — ATmega128 Operating System

Thesis project (CSC499, University of Toronto, 2004) under Professor Steven Mann.
A preemptive, priority-based multi-tasking OS written from scratch for the Atmel ATmega128 microcontroller.
Languages: C and AVR Assembly. Archived/research status — not actively developed.

---

## Toolchain

```
avr-gcc   — compile C and ASM files (-mmcu=atmega128 -O2)
avr-ld    — link object files (-arch=atmega128)
avr-objcopy — produce Intel HEX output from the linked ELF
```

`make all` → `flashImage.hex` (flash this to the chip)
`make clean` → removes `.o`, `.hex`, `.aps`

Note: `TerminalDriver.c` was originally written for IAR EWAAVR; it uses raw pointer
casts for UART registers instead of avr-libc macros because that was the original
target compiler.

---

## Memory Map (ATmega128 on-chip SRAM: 0x0100–0x10FF, 4 KB)

| Region | Range | Size | Purpose |
|---|---|---|---|
| Kernel static data | 0x0100–0x04E8 | ~1 KB | KSP, ACTIVE, syscall params, PDBLOCK |
| Heap | 0x04E9–0x08D1 | 1 KB | Dynamic allocation |
| User stack space | 0x08D2–0x1036 | ~2.3 KB | Per-process stacks (divided by MAX_PROCS) |
| Kernel stack | 0x1037–0x10FF | 201 B | Kernel execution stack (SP init'd at RAMEND) |

Key fixed addresses (defined identically in `SRAM_map.h` for C and `SRAM_map.s` for ASM):

| Address | Symbol | Purpose |
|---|---|---|
| 0x0100–0x0101 | KSP_HIGH/LOW | Kernel stack pointer saved here on syscall/interrupt |
| 0x0102 | ACTIVE | Index of the currently running process |
| 0x0103–0x010D | OPCODE, P1–P5 | Syscall opcode and up to 5 parameters |
| 0x010E–0x010F | A_SPL/A_SPH | Active process stack pointer (saved on entry to kernel) |
| 0x0110 | RETCODEI | 0x01 = return value follows; 0x00 = no return value |
| 0x0112 | A_SREG | Active process status register |
| 0x0113–0x0114 | RETURN_L/H | Return value from syscall to user process |
| 0x0115–0x011C | TMP_1–TMP_8 | Temporary kernel scratch bytes |
| 0x011D+ | PD_BLOCK_START | Process descriptor array (`struct PD[MAX_PROCS]`) |

---

## Process Descriptor (`struct PD` in `kernelC.h`)

```c
struct PD {
    int returnCode;        // NO_RETURN_CODE (-100) means no pending retval
    unsigned char pch, pcl;  // program counter (high, low)
    unsigned char sph, spl;  // stack pointer (high, low)
    unsigned char sreg;       // status register
    unsigned char ppid, pid;
    unsigned char priority;   // PRIO_LOW=0, PRIO_NORMAL=3, PRIO_HIGH=6
    unsigned char state;      // S_ACTIVE=0, S_DEAD=1, S_BLOCKED=2, S_SLEEPING=3, S_READY=4
    char msgi;                // top-of-message-stack index (-1 = empty)
    int msgbuf[MAX_MESSAGES]; // message buffer (max 10 messages)
    unsigned int waitTime;    // sleep countdown in milliseconds
};
```

MAX_PROCS = 6 (comment says "do not exceed 6" due to on-chip RAM constraints).
Each process gets `STACK_SIZE = (USER_STACKSPACE_END - USER_STACKSPACE_START) / MAX_PROCS` bytes of stack.

---

## Boot Sequence (`kernel.s`: `K_init`)

1. Set SP to RAMEND (top of kernel stack area).
2. Write PORTA status 0x01–0x05 at each bootstrap milestone (hardware debug).
3. Call `K_init_processDescriptorBlock()` — zero/init all PD slots, assign per-process stack pointers.
4. Configure Timer0 with prescaler 1024 and overflow interrupt enabled (10 Hz preemptive scheduling).
5. Configure INT4 on PE4 as software syscall trap (low-level triggered).
6. Call `create_initial_process()` — sets up PID 0 (user_init) as S_ACTIVE.
7. Save kernel SP, load PID 0's SP, enable interrupts (`sei`), jump to `user_init`.

---

## Interrupt Handlers (`kernel.s`, `vector_table.s`)

**All unhandled vectors → `K_panic`** (writes 0xFF to PORTA, infinite loop).

**`K_trap` (INT4) — system call entry:**
1. Push all 32 registers onto the *user* stack.
2. Save user SP and SREG to kernel memory (A_SPH, A_SPL, A_SREG).
3. Switch to kernel stack (load KSP).
4. Call `K_syscall()` (C) → dispatches to the appropriate `K_*` implementation.
5. Save kernel SP back, restore chosen process's SP/SREG.
6. If RETCODEI == 1, load RETURN_L/H into r24:r25 before restoring registers.
7. `reti` — returns to whichever process `dispatch()` selected.

**`tc0_ovf` (Timer0 overflow) — preemptive scheduling:**
Same push/save/switch-to-kernel pattern as `K_trap`, but calls `K_timer_handler()`
which: decrements sleeping processes' wait times, wakes any that hit zero, then
calls `schedule()` + `dispatch()`. Reloads INITIAL_TIMER0_COUNT on every tick.

---

## Scheduling (`kernelC.c`)

`schedule()` — O(n) priority + round-robin:
- Marks current ACTIVE as S_READY.
- Finds the maximum priority among all S_READY processes.
- Starting from (active+1) % MAX_PROCS, picks the first S_READY process at that priority.
- Updates `ACTIVE`.

`dispatch()` — prepares the chosen process:
- Writes its SP/SREG to A_SPL, A_SPH, A_SREG (the interrupt handler reads these back).
- If the process has a pending returnCode, sets RETCODEI=1 and writes it to RETURN_L/H.
- Sets the process state to S_ACTIVE.

---

## Syscall Mechanism

User code calls a wrapper in `syscall_interface.c` (e.g., `create_process()`) which:
1. Stops Timer0 (disables preemption during parameter setup — critical section).
2. Writes opcode + up to 5 parameters to fixed kernel memory addresses.
3. Pulls PE4 low → triggers INT4 → `K_trap` fires.
4. After the kernel returns, reads the result from RETURN_L/H.

The kernel-side implementations live in `ksyscalls.c`. The opcode dispatch table is
in `K_syscall()` (`kernelC.c`).

### System Calls

| Opcode | User API | Kernel impl | Notes |
|---|---|---|---|
| GET_PPID=0 | `get_ppid()` | `K_get_ppid()` | |
| GET_PID=1 | `get_pid()` | `K_get_pid()` | **BUG: always returns 1** — `return 1; PDBLOCK[*active].pid;` unreachable |
| GET_PRIO=2 | `get_prio()` | `K_get_prio()` | |
| SET_PRIO=3 | `set_prio(pid, prio)` | `K_set_prio()` | |
| SLEEP=4 | `sleep(ms)` | `K_sleep()` | Sleep granularity = SLEEP_PERIOD (100 ms) |
| KILL=5 | `kill(pid)` | `K_kill()` | Cannot kill self; use quit() |
| YIELD=6 | `yield()` | `K_yield()` | Just returns 0; schedule() naturally skips |
| SUSPEND=7 | `suspend(pid)` | `K_suspend()` | Sets state → S_BLOCKED |
| WAKEUP=8 | `wakeup(pid)` | `K_wakeup()` | Sets state → S_READY; must already be S_BLOCKED |
| CREATE_PROCESS=9 | `create_process(pc, prio)` | `K_create_process()` | Returns new PID; pushes 32 zero regs + PC onto process stack |
| SND_MSG=10 | `snd_msg(pid, msg)` | `K_snd_msg()` | msg=uint8, packed with sender PID into int |
| CHK_MSG=11 | `check_msg()` | `K_check_msg()` | Returns 1 if messages pending, 0 if not |
| GET_MSG=12 | `get_msg()` | `K_get_msg()` | Returns top-of-stack message; LIFO |
| QUIT=13 | `quit()` | `K_quit()` | Kills self; no return |

---

## Message Format

Each message is an `int` (2 bytes): low byte = sender PID, high byte = message content byte.
`msgbuf` is a fixed stack per process (max 10 messages). `get_msg()` is LIFO — newest first.
A comment in `ksyscalls.c` notes that retrieving the *oldest* message (FIFO) was low priority and not implemented.

---

## Timer Configuration

- Prescaler: 1024, initial count: INITIAL_TIMER0_COUNT (defined in `timer.s`)
- Target: 10 interrupts/sec (SLEEP_PERIOD = 100 ms)
- Clock assumed: ~3.69 MHz (based on `TerminalDriver.c` baud rate comments)

---

## Files

| File | Role |
|---|---|
| `vector_table.s` | Interrupt vector table — must be at flash address 0x0000 |
| `ATmega128defs.s` | Register and I/O alias definitions for the assembler |
| `SRAM_map.s` | Memory map constants for ASM |
| `SRAM_map.h` | Memory map constants for C (must stay in sync with `.s`) |
| `timer.s` | Timer prescaler/count constants for ASM |
| `timer.h` | Timer constants for C (must stay in sync with `.s`) |
| `kernel.s` | Bootstrap (`K_init`), INT4 trap handler, Timer0 handler, `K_panic` |
| `kernelC.c/h` | Core kernel in C: PDBLOCK init, scheduler, dispatcher, timer handler, syscall dispatcher |
| `ksyscalls.c/h` | Kernel-side implementations of all 14 syscalls |
| `syscall_interface.c/h` | User-facing syscall wrappers; triggers INT4 trap |
| `user_init.c/h` | Initial user program (PID 0); creates child processes |
| `TerminalDriver.c/h` | UART/VT100 terminal driver (originally from Atmel, IAR compiler) |
| `errors.h` | Error code constants (-1 through -12) |
| `PORTA-stat_codes.txt` | Debug status code reference for PORTA hardware output |
| `Makefile` | Build rules |

---

## Hardware Debug (PORTA)

The kernel writes single-byte status codes to PORTA (I/O address 0x1B) at key execution
points. PORTA pins are read with an oscilloscope or LEDs to trace execution without UART.
See `PORTA-stat_codes.txt` for the full code table (0x00–0xFF, with 0xFF = kernel panic).

---

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
`WALL_TIMEOUT` (wall-clock seconds, default 30). If simulavr's CLI PORTA→VCD signal
name doesn't produce a usable trace, the harness falls back to a `pysimulavr`
register-polling runner (see `tools/README.md`).

Install prerequisites:
```
sudo apt install gcc-avr binutils-avr avr-libc simulavr
```

---

## Known Issues

- **`K_get_pid()` bug** (`ksyscalls.c:55`): `return 1; PDBLOCK[*active].pid;` — the actual PID lookup is dead code; the function always returns 1.
- **Leftover debug `asm` statements** throughout C files write status codes to PORTA; these are harmless but chatty.
- **`TerminalDriver.c`**: `Term_Draw_Frame` and `Term_Handle_Menu` are commented out — they used `__flash` pointer types specific to IAR that avr-gcc does not support.
- **SRAM_map.h / SRAM_map.s must be kept in sync** — both define the same addresses for their respective languages. There is no automated check for drift.
