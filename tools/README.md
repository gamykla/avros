# AVROS Build & Simulation Test Harness

A reproducible `make`-driven harness that builds the AVROS image, runs it on a
simulated ATmega128 (simulavr), observes the PORTA status codes the kernel emits,
and asserts boot + scheduler progress. It is the regression gate the upcoming
syscall return-value-race fix will consume.

## Prerequisites

Install the AVR GNU toolchain and the simulator:

```
sudo apt install gcc-avr binutils-avr avr-libc simulavr
```

`python3` (standard library only) is required for the trace checker. Run
`make preflight` to verify everything is present — it prints the exact install
command if anything is missing.

## Targets

| Target          | What it does                                                        |
|-----------------|--------------------------------------------------------------------|
| `make preflight`| Verify `avr-gcc`, `simulavr`, `python3` are installed.              |
| `make elf`      | Build `build/flashImage.elf` (and `build/flashImage.hex`).         |
| `make sim-run`  | Build + run sim, write `build/porta.vcd`, print observed codes.     |
| `make sim-test` | Build + run + assert. Exit 0 = pass, non-zero = fail. The gate.     |
| `make clean`    | Remove `build/`.                                                    |

## Configuration (environment variables)

| Variable       | Default      | Meaning                                              |
|----------------|--------------|------------------------------------------------------|
| `SIM_MAX_NS`   | `100000000`  | Simulated-time bound (ns). The OS loops forever.     |
| `WALL_TIMEOUT` | `30`         | Wall-clock guard (seconds) that kills a stuck sim.   |

Example: `SIM_MAX_NS=20000000 WALL_TIMEOUT=10 make sim-test`.

## PORTA status codes

The kernel writes single-byte codes to PORTA (I/O port 0x1B, data-space 0x3B).
See `../PORTA-stat_codes.txt` for the full list. Key ones:

- `0x01`–`0x05` — bootstrap milestones (`kernel.s`, `K_init`)
- `0x00` — user_init heartbeat (`user_init.c`)
- `28`/`0x1C` — `schedule()`, `29`/`0x1D` — `dispatch()` (`kernelC.c`)
- `0xFF` — `K_panic` (unrecoverable, `kernel.s`). Its presence anywhere = failure.

## Assertion contract (`check_porta_trace.py`)

- FAIL if any **forbidden** code (default `0xFF`) appears anywhere — the
  intermittent race manifests mid-run, so a panic after valid boot codes is
  still a failure.
- PASS only if every **expected** code appears: defaults are `0x01`–`0x05`,
  `0x00`, and at least one scheduler code (`0x1C` or `0x1D`).

The expected/forbidden sets are CLI-configurable so the defect regression test
can tighten them without editing the script:

```
python3 tools/check_porta_trace.py build/porta.vcd \
    --expected 0x01 0x02 0x03 0x04 0x05 0x00 0x20 0x22 0x23 \
    --expect-any 0x1C,0x1D \
    --forbidden 0xFF
```

Observe-only (no assertion): `python3 tools/check_porta_trace.py --observe build/porta.vcd`.

Run the checker's own unit tests: `python3 tools/check_porta_trace.py --self-test`.

## Two observation paths

**Primary — simulavr CLI VCD.** `run_sim.sh` invokes `simulavr -d atmega128 -f
<elf> -m <ns> -o build/porta.vcd -t PORTA`, producing a VCD trace of PORTA. This
is preferred when the installed simulavr build exposes a stable PORTA signal name.

**Fallback — pysimulavr register polling.** If the CLI VCD output contains no
usable PORTA value changes (signal naming varies between simulavr builds),
`run_sim.sh` automatically falls back to a `pysimulavr` runner inside
`check_porta_trace.py --pysim-run`. It steps the simulated device and polls the
PORTA data-space address (`0x3B`), writing an equivalent VCD that the same
assertion logic parses.

> Note: the exact simulavr 1.0 signal name and CLI flags for VCD PORTA tracing
> were not verifiable in the authoring environment (toolchain not installed). If
> the CLI path yields an empty trace, the pysimulavr fallback is used; if neither
> path works, `run_sim.sh` exits non-zero and points back here. Confirm the
> correct `-t` signal name on first run against an installed simulavr.

## Build wiring note

The translation units linked into the image are `kernel.o`, `kernelC.o`,
`ksyscalls.o`, `syscall_interface.o`, and `user_init.o`. The Makefile pins this
set explicitly and makes the ELF link depend on every object, replacing the
original loose `all`-target ordering that did not guarantee all objects were
built before the link. PORTA never has its data-direction register (DDRA) set to
output by the kernel; simulavr traces register writes regardless, which is what
the harness relies on.
