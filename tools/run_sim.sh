#!/usr/bin/env bash
#
# run_sim.sh — run AVROS ELF on simavr (ATmega128) via GDB and record PORTA events.
#
# Usage: run_sim.sh <elf> <out_trace>
#
# Uses simavr's built-in GDB server + avr-gdb to observe PORTA writes.
# Breakpoints are placed at the OUT 0x1B instructions in kernel.s (byte addresses
# from the ELF disassembly).  When a breakpoint fires, the corresponding PORTA
# value is appended to the binary trace file; check_porta_trace.py parses it.
#
# The GDB session runs until:
#   * K_panic is detected  (0xFF appended, session ends immediately)
#   * schedule() is confirmed running N times (success, session ends)
#   * WALL_TIMEOUT seconds elapse (timeout, session is killed)
#
# Env vars:
#   CPU_FREQ     AVR CPU frequency in Hz  (default 3690000)
#   WALL_TIMEOUT Wall-clock seconds       (default 20)
#   GDB_PORT     TCP port for simavr GDB  (default 12340)

set -euo pipefail

if [ "$#" -ne 2 ]; then
    printf 'usage: %s <elf> <out_trace>\n' "$0" >&2
    exit 2
fi

readonly ELF="$1"
readonly OUT_TRACE="$2"
readonly DEVICE="atmega128"
readonly CPU_FREQ="${CPU_FREQ:-3690000}"
readonly WALL_TIMEOUT="${WALL_TIMEOUT:-20}"
readonly GDB_PORT="${GDB_PORT:-1234}"   # simavr hardcodes port 1234 (no -p flag in 1.6)

if [ ! -f "${ELF}" ]; then
    printf 'run_sim: ELF not found: %s\n' "${ELF}" >&2
    exit 1
fi

for tool in simavr avr-gdb python3; do
    if ! command -v "${tool}" >/dev/null 2>&1; then
        printf 'run_sim: %s not installed; run: make preflight\n' "${tool}" >&2
        exit 1
    fi
done

out_dir="$(dirname "${OUT_TRACE}")"
mkdir -p "${out_dir}"
rm -f "${OUT_TRACE}"

# ── Start simavr with GDB server ─────────────────────────────────────────────
printf 'run_sim: starting simavr (device=%s, freq=%s Hz, GDB port=%s)\n' \
    "${DEVICE}" "${CPU_FREQ}" "${GDB_PORT}"

# simavr exits when the GDB connection drops; the timeout wrapper is the
# outer bound.
simavr -g -m "${DEVICE}" -f "${CPU_FREQ}" "${ELF}" \
    2>/dev/null &
SIM_PID=$!

# Give simavr time to open the GDB socket before connecting
sleep 0.4

# ── Write GDB script ─────────────────────────────────────────────────────────
# Byte addresses of OUT 0x1B instructions in the ELF (from avr-objdump -d).
# These are the assembly-level PORTA writes; they are reliable under -O2.
# If the ELF is rebuilt the addresses may change — `avr-objdump -d <elf>`
# then grep for 'out.*0x1b' to verify.
#
#   0x00b2  K_init writes 0x01 (bootstrap: stack + SP set up)
#   0x00b8  K_init writes 0x02 (after K_init_processDescriptorBlock)
#   0x00c8  K_init writes 0x03 (after Timer0 configure)
#   0x00dc  K_init writes 0x04 (before create_initial_process)
#   0x00e2  K_init writes 0x05 (after create_initial_process)
#   0x0035a K_panic writes 0xFF — stop immediately on panic
#   0x0037e schedule() entry  — confirms preemptive scheduling is live
#
GDB_SCRIPT="${out_dir}/avros_test.gdb"
cat > "${GDB_SCRIPT}" << GDBEOF
set pagination off
set confirm off
set \$sched = 0
target remote :${GDB_PORT}

break *0xb2
commands 1
  printf "HIT:0x01\\n"
  continue
end

break *0xb8
commands 2
  printf "HIT:0x02\\n"
  continue
end

break *0xc8
commands 3
  printf "HIT:0x03\\n"
  continue
end

break *0xdc
commands 4
  printf "HIT:0x04\\n"
  continue
end

break *0xe2
commands 5
  printf "HIT:0x05\\n"
  continue
end

break *0x35a
commands 6
  printf "HIT:0xff\\n"
  quit 0
end

break *0x37e
commands 7
  printf "HIT:0x1c\\n"
  set \$sched = \$sched + 1
  if \$sched >= 5
    quit 0
  end
  continue
end

continue
GDBEOF

# ── Run avr-gdb ───────────────────────────────────────────────────────────────
GDB_LOG="${out_dir}/gdb_out.txt"
timeout "${WALL_TIMEOUT}" avr-gdb -batch -x "${GDB_SCRIPT}" "${ELF}" \
    > "${GDB_LOG}" 2>&1 || true

# ── Kill simavr ───────────────────────────────────────────────────────────────
kill "${SIM_PID}" 2>/dev/null || true
wait "${SIM_PID}" 2>/dev/null || true

# ── Convert GDB hit log to binary trace ───────────────────────────────────────
python3 - "${GDB_LOG}" "${OUT_TRACE}" << 'PYEOF'
import sys, re

gdb_log, trace_path = sys.argv[1], sys.argv[2]
with open(gdb_log) as fh:
    content = fh.read()

hits = re.findall(r'HIT:(0x[0-9a-fA-F]+)', content)
data = bytes(int(h, 16) for h in hits)

with open(trace_path, 'wb') as fh:
    fh.write(data)

observed = sorted(set(data))
print(f'run_sim: {len(data)} PORTA events -> {trace_path}')
if observed:
    print(f'run_sim: distinct codes: {", ".join(f"0x{b:02x}" for b in observed)}')
else:
    print('run_sim: no PORTA events captured — simavr may not have started correctly')
PYEOF
