#!/usr/bin/env bash
#
# preflight.sh — verify the AVROS build/sim toolchain is installed.
#
# Exits 0 when every required tool is present, otherwise prints the exact
# install command and exits 1. Called by `make preflight`.

set -euo pipefail

readonly REQUIRED_TOOLS=(avr-gcc avr-ld avr-objcopy simulavr python3)
readonly INSTALL_HINT="sudo apt install gcc-avr binutils-avr avr-libc simulavr"

missing=()

for tool in "${REQUIRED_TOOLS[@]}"; do
    if command -v "${tool}" >/dev/null 2>&1; then
        printf '  ok   %s -> %s\n' "${tool}" "$(command -v "${tool}")"
    else
        printf '  MISS %s\n' "${tool}"
        missing+=("${tool}")
    fi
done

if [ "${#missing[@]}" -ne 0 ]; then
    printf '\npreflight FAILED: missing tool(s): %s\n' "${missing[*]}" >&2
    printf 'Install the AVR toolchain and simulator with:\n\n    %s\n\n' "${INSTALL_HINT}" >&2
    exit 1
fi

# Confirm simulavr accepts the ATmega128 device model (R4). A bad device list
# means sim-run/sim-test would fail later with a less obvious error.
if ! simulavr --help 2>&1 | grep -qi 'atmega128'; then
    printf '\npreflight WARNING: simulavr --help did not advertise atmega128.\n' >&2
    printf 'The installed simulavr build may not support this device model.\n' >&2
fi

printf '\npreflight OK: toolchain ready.\n'
