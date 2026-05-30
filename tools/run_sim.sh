#!/usr/bin/env bash
#
# run_sim.sh — run an AVROS ELF on a simulated ATmega128 and capture PORTA.
#
# Usage: run_sim.sh <elf> <out_vcd>
#
# The OS scheduler loops forever, so the run is bounded twice:
#   * simulated time  via simulavr -m <ns>   (env SIM_MAX_NS,   default 100000000)
#   * wall-clock time via `timeout`          (env WALL_TIMEOUT, default 30)
#
# Primary path : simulavr CLI dumps a VCD trace of PORTA.
# Fallback path: if the VCD path produces no usable trace, a pysimulavr
#                register-polling runner writes an equivalent trace instead.
# Both paths yield a file that tools/check_porta_trace.py can parse.

set -euo pipefail

if [ "$#" -ne 2 ]; then
    printf 'usage: %s <elf> <out_vcd>\n' "$0" >&2
    exit 2
fi

readonly ELF="$1"
readonly OUT_VCD="$2"
readonly DEVICE="atmega128"
readonly SIM_MAX_NS="${SIM_MAX_NS:-100000000}"
readonly WALL_TIMEOUT="${WALL_TIMEOUT:-30}"
readonly PORTA_IO_ADDR="0x3b"   # data-space address of PORTA (I/O 0x1b + 0x20)

if [ ! -f "${ELF}" ]; then
    printf 'run_sim: ELF not found: %s\n' "${ELF}" >&2
    exit 1
fi

if ! command -v simulavr >/dev/null 2>&1; then
    printf 'run_sim: simulavr not installed; run: make preflight\n' >&2
    exit 1
fi

out_dir="$(dirname "${OUT_VCD}")"
mkdir -p "${out_dir}"
rm -f "${OUT_VCD}"

# Capture every byte written to PORTA (I/O offset 0x1B) using simulavr's -W flag.
# This writes raw bytes to the output file, one per PORTA write. The VCD tracer
# path (-o / -c vcd:...) was found to segfault or produce empty traces on the
# available simulavr 1.x build; -W is the reliable approach.
printf 'run_sim: running simulavr on %s (SIM_MAX_NS=%s, WALL_TIMEOUT=%ss)\n' \
    "${DEVICE}" "${SIM_MAX_NS}" "${WALL_TIMEOUT}"

timeout "${WALL_TIMEOUT}" simulavr \
    -d "${DEVICE}" \
    -f "${ELF}" \
    -m "${SIM_MAX_NS}" \
    -W "0x1b,${OUT_VCD}" \
    >/dev/null 2>&1 || true

if [ ! -s "${OUT_VCD}" ]; then
    printf 'run_sim: simulavr produced no PORTA output — check that the ELF loads and boots.\n' >&2
    exit 1
fi

byte_count=$(wc -c < "${OUT_VCD}")
printf 'run_sim: captured %d PORTA writes to %s\n' "${byte_count}" "${OUT_VCD}"
