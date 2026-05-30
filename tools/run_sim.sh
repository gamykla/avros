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

# A VCD with at least one value-change line under a PORTA-named variable counts
# as usable. Header-only output means the CLI tracing path did not capture PORTA.
vcd_is_usable() {
    local f="$1"
    [ -s "${f}" ] && grep -qiE '(porta|0x3b|0x1b)' "${f}"
}

run_via_cli() {
    printf 'run_sim: trying simulavr CLI VCD path (SIM_MAX_NS=%s, WALL_TIMEOUT=%ss)\n' \
        "${SIM_MAX_NS}" "${WALL_TIMEOUT}"
    # simulavr 1.x: -d device, -f elf, -m max-ns, -o vcd, -t traced-signal.
    # PORTA is requested by both common spellings; unknown signals are tolerated
    # because failure is detected by inspecting the resulting file, not the rc.
    timeout "${WALL_TIMEOUT}" simulavr \
        -d "${DEVICE}" \
        -f "${ELF}" \
        -m "${SIM_MAX_NS}" \
        -o "${OUT_VCD}" \
        -t "PORTA" \
        -t "${DEVICE}.PORTA" \
        >/dev/null 2>&1 || true
}

run_via_pysimulavr() {
    printf 'run_sim: CLI VCD path unusable; falling back to pysimulavr polling.\n' >&2
    SIM_MAX_NS="${SIM_MAX_NS}" \
    PORTA_IO_ADDR="${PORTA_IO_ADDR}" \
    timeout "${WALL_TIMEOUT}" python3 tools/check_porta_trace.py \
        --pysim-run "${ELF}" "${DEVICE}" "${OUT_VCD}"
}

run_via_cli

if vcd_is_usable "${OUT_VCD}"; then
    printf 'run_sim: VCD trace captured at %s\n' "${OUT_VCD}"
    exit 0
fi

run_via_pysimulavr

if vcd_is_usable "${OUT_VCD}"; then
    printf 'run_sim: pysimulavr trace captured at %s\n' "${OUT_VCD}"
    exit 0
fi

printf 'run_sim: failed to capture a usable PORTA trace via either path.\n' >&2
printf 'run_sim: see tools/README.md for simulavr signal-naming guidance.\n' >&2
exit 1
