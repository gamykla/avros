---
id: 1
name: simulavr-build-test-harness
type: DevOpsRefinement
tier: Standard
status: Draft
spec: N/A — devops refinement
---

# TDD — Simulavr Build & Test Harness for AVROS

## Goal

Provide a reproducible `make`-driven build and simulation-test harness for AVROS on
simulavr / ATmega128, enabling automated PORTA-based regression assertions for the
upcoming syscall return-value-race defect fix. The harness compiles the OS image,
runs it on a simulated ATmega128, observes the single-byte status codes the kernel
writes to PORTA (I/O address 0x1B), and asserts boot + scheduler progress while
failing hard if the kernel ever signals a panic (0xFF).

## Non-goals

- No defect fix in this work item (the syscall return-value race is fixed in a
  follow-on item that consumes this harness).
- No toolchain installation. The harness performs preflight checks and fails with an
  explicit install command if a required tool is missing.
- No CI integration yet. The harness is local/CLI only; CI wiring is a later item.
- No source-code changes to the OS kernel. Only the Makefile build wiring is touched.

## Approach

1. **Build target.** Compile all `.c`/`.s` sources with
   `avr-gcc -mmcu=atmega128 -O2 -g`, link with `avr-ld -arch=atmega128`, and produce
   both `build/flashImage.elf` (loaded natively by simulavr) and `build/flashImage.hex`
   (the real flash artifact via `avr-objcopy -j .text -j .data -O ihex`). The
   translation units are: `kernel.o` (boot/trap/timer entry; includes vector_table.s,
   SRAM_map.s, timer.s), `kernelC.o`, `ksyscalls.o`, `syscall_interface.o`,
   `user_init.o`. The original Makefile listed these loose objects in its `all` target
   in an order that did not reliably build `ksyscalls.o` / `user_init.o` before the
   link, and it built into the repo root. The harness pins an explicit `OBJECTS` set,
   makes the link depend on every object, and builds into `build/`.

2. **Simulator.** Run `simulavr -d atmega128` loading the ELF. Produce a VCD trace of
   PORTA. The run is bounded two ways because the OS scheduler loops forever:
   - simulated time via `-m <ns>` (env `SIM_MAX_NS`, default 100000000 = 100 ms),
   - a wall-clock `timeout` guard (env `WALL_TIMEOUT`, default 30 s).

3. **Observation.** The kernel writes single-byte status codes to PORTA at key
   milestones (see `PORTA-stat_codes.txt`). Code `0xFF` = `K_panic` (unrecoverable
   crash; see `K_panic:` in `kernel.s`). PORTA is I/O port 0x1B (data-space address
   0x3B per `SRAM_map.h` / ATmega128 I/O offset of 0x20).

4. **Assertion** (`tools/check_porta_trace.py`). Parse the VCD trace. FAIL if PORTA
   ever reaches `0xFF` **anywhere** in the trace — including after valid boot codes,
   because the intermittent race manifests mid-run. PASS only if every code in a
   configurable expected-codes set appears at least once. Default expected set:
   bootstrap codes `0x01`–`0x05` (kernel.s), `0x00` (user_init heartbeat), and at
   least one scheduler code (`28`/`0x1C` from `schedule()` or `29`/`0x1D` from
   `dispatch()` in kernelC.c). Expected and forbidden lists are passed as CLI args so
   the defect regression test can tighten them without code changes. Exit 0 = pass,
   non-zero = fail with a human-readable reason.

5. **Fallback.** If the simulavr CLI VCD PORTA signal naming proves unreliable in the
   installed simulavr build, `tools/run_sim.sh` falls back to a `pysimulavr`
   register-polling runner that polls the PORTA I/O address and emits an equivalent
   trace file. The same `check_porta_trace.py` assertion logic applies to both. Both
   paths are documented in `tools/README.md`.

## Makefile targets

- `preflight` — verify `avr-gcc`, `simulavr`, `python3` are present; on any miss, fail
  with the exact message `sudo apt install gcc-avr binutils-avr avr-libc simulavr`.
- `elf` — build `build/flashImage.elf` (and `build/flashImage.hex` as a by-product).
- `sim-run` — build + run simulavr, produce `build/porta.vcd`, print the PORTA codes
  observed. Observe-only; no assertion.
- `sim-test` — build + run + assert; exits 0 on pass, non-zero on fail. This is the
  regression gate the defect fix will consume.
- `clean` — remove `build/`.

## File-level change list

- MODIFY `Makefile` — pin explicit `OBJECTS` (`kernel`, `kernelC`, `ksyscalls`,
  `syscall_interface`, `user_init`), make the link depend on all of them, build into
  `build/`, add `preflight` / `elf` / `sim-run` / `sim-test` / `clean` targets.
- ADD `tools/preflight.sh` — standalone tool-presence check (invoked by `make preflight`).
- ADD `tools/run_sim.sh` — runs simulavr with bounded sim + wall time, dumps PORTA VCD;
  falls back to pysimulavr polling when the VCD path fails.
- ADD `tools/check_porta_trace.py` — parses VCD (or polling log), implements the
  pass/fail assertion contract; includes a `--self-test` with synthetic fixtures.
- ADD `tools/README.md` — prerequisites, how to run the harness, how to read results,
  and both observation paths.
- ADD `.gitignore` — none existed; ignore `build/`, `*.o`, `*.hex`, `*.elf`, `*.aps`,
  `*.vcd`, `__pycache__/`.
- ADD `CLAUDE.md` — none existed; create the project CLAUDE.md with a short
  architecture overview, a Build section, and the "Build and test harness" section
  (DoD item 9).

## Test / verification plan

1. `shellcheck tools/preflight.sh tools/run_sim.sh` — must be clean.
2. `python3 -m py_compile tools/check_porta_trace.py` — syntax check.
3. `python3 tools/check_porta_trace.py --self-test` — synthetic-fixture unit tests pass.
4. `make preflight` — prints a clear, exact install error if tools are not installed.
5. `make sim-test` — end-to-end: builds ELF, runs simulavr, asserts boot + scheduler
   progress and absence of 0xFF. (Runs only when the AVR toolchain + simulavr are
   installed; otherwise reported as SKIPPED.)

## Infra self-review (Mode 1A)

- Scripts shellcheck-clean: yes — `shellcheck` is available in this environment and is
  run against both scripts (`set -euo pipefail`, all expansions quoted, no unquoted
  user input).
- Make targets idempotent: yes — `build/` is created with `mkdir -p`; rerunning
  `sim-test` is safe and side-effect-free outside `build/`.
- No hardcoded absolute paths: yes — all paths are relative to the repo root or come
  from environment variables.
- Clear failure modes: yes — preflight fails loudly with the exact apt command, the
  sim run is killed by `timeout`, and the checker exits non-zero with a printed reason.
- No secrets/credentials: yes — none introduced anywhere.
- Verdict: **Approved.**

## Risks

- **R1 (medium):** simulavr 1.0 CLI flag / signal name for the PORTA→VCD dump is
  unknown until tested on the installed build. Mitigated by the documented pysimulavr
  register-polling fallback in `run_sim.sh`.
- **R2 (low/medium):** the kernel writes to the PORTA register but never sets DDRA to
  output. simulavr should still trace register writes regardless of direction; verify
  at impl time and prefer register-write tracing over pin-state tracing.
- **R3 (low):** ELF is produced via direct `avr-ld -arch=atmega128` with no crt0.
  `avr-ld` still emits a valid ELF and simulavr loads ELF images natively. The reset
  vector (`vector_table.s`, `.org 0x0000 -> jmp K_init`) provides the entry point.
- **R4 (low):** ATmega128 model support in the installed simulavr build. `preflight`
  asserts the device is usable; `run_sim.sh` passes `-d atmega128`.
- **R5 (low):** the Makefile wiring change could alter link order. `sim-test`
  validates that boot codes `0x01`–`0x05` appear, catching a broken link.

## CLAUDE.md update (DoD item 9)

No project `CLAUDE.md` existed. Create one with a short architecture overview, a
Build section, and a "Build and test harness" section documenting the new
`preflight` / `elf` / `sim-run` / `sim-test` / `clean` targets, the
`SIM_MAX_NS` / `WALL_TIMEOUT` env vars, and how to interpret PORTA results.

## Implementation log

| DoD item | Status |
|---|---|
| 1 — Spec conformance | N/A — devops refinement |
| 2 — TDD conformance | ✓ — all targets and files implemented per TDD; object set corrected to the actual units (kernel, kernelC, ksyscalls, syscall_interface, user_init) during impl |
| 3 — Tests pass (feature branch) | ✓ — shellcheck clean; py_compile clean; `--self-test` 11/11 pass; bash -n clean; preflight runs and fails loudly with the exact install command. `make sim-test`: SKIPPED — AVR toolchain + simulavr not installed in this environment |
| 4 — Lint/static analysis | ✓ — `shellcheck tools/preflight.sh tools/run_sim.sh` CLEAN; `python3 -m py_compile` CLEAN |
| 5 — Clean build | ◐ — `make elf` SKIPPED (avr-gcc not installed); Makefile graph validated via `make -n elf` (all 5 objects incl. ksyscalls.o compile, link via `avr-ld -arch=atmega128`, HEX via objcopy) |
| 6 — SDLC followed | ✓ — TDD → Mode 1A self-review (Approved) → implementation → log |
| 7 — Merged to main | ⏳ pending Justin's PR approval |
| 8 — Tests pass on main | ⏳ pending merge |
| 9 — CLAUDE.md reviewed | ✓ — no project CLAUDE.md existed; created one with architecture overview, Build, and "Build and test harness" sections |
| 10 — Issue closed | ⏳ pending merge |

### Verification run summary
- `shellcheck tools/preflight.sh tools/run_sim.sh` → CLEAN
- `python3 -m py_compile tools/check_porta_trace.py` → OK
- `python3 tools/check_porta_trace.py --self-test` → 11/11 cases passed
- `bash -n tools/preflight.sh tools/run_sim.sh` → OK
- `make preflight` → correctly reports missing avr-gcc/avr-ld/avr-objcopy/simulavr with the exact `sudo apt install gcc-avr binutils-avr avr-libc simulavr` message; exits non-zero
- `make -n elf` / `make -n sim-test` → Makefile graph correct; ksyscalls.o now reliably linked
- `make elf` / `make sim-test` → SKIPPED (AVR toolchain + simulavr not installed in the authoring environment)

### Deviations from TDD
- TDD draft initially assumed objects `boot.o/user.o/klib.o`; those files do not exist
  in the repo. Corrected before implementation to the real units
  (`kernel/kernelC/ksyscalls/syscall_interface/user_init`) and the link to
  `avr-ld -arch=atmega128`. The TDD text above reflects the corrected set.
- `CLAUDE.md` and `.gitignore` did not exist (TDD says ADD, not MODIFY) — both created.
- R1 (simulavr PORTA→VCD signal naming) remains unverified until run against an
  installed simulavr; the pysimulavr fallback and a first-run note are in place.
