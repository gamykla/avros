#!/usr/bin/env python3
"""check_porta_trace.py — assert AVROS boot/runtime progress from a PORTA trace.

The AVROS kernel writes single-byte status codes to PORTA (I/O port 0x1B) at key
milestones. This tool parses a VCD trace of PORTA and applies a pass/fail
assertion contract:

  * FAIL if a forbidden code (default 0xFF = K_panic) appears ANYWHERE in the
    trace, including after valid boot codes — the intermittent syscall race
    manifests mid-run.
  * PASS only if every expected code appears at least once. The default expected
    set is the bootstrap codes 0x01..0x05, the user_init heartbeat 0x00, and at
    least one scheduler code (28/0x1C or 29/0x1D).

Expected and forbidden code sets are CLI-configurable so the follow-on defect
regression test can tighten them without editing this file.

Modes:
  check_porta_trace.py <vcd>                 assert (exit 0 pass / non-zero fail)
  check_porta_trace.py --observe <vcd>       print observed codes, always exit 0
  check_porta_trace.py --pysim-run E D OUT   run ELF E on device D via pysimulavr,
                                             write a VCD-style trace to OUT
  check_porta_trace.py --self-test           run synthetic-fixture unit tests
"""

import argparse
import os
import sys

# Defaults (overridable via CLI). "at least one scheduler code" is expressed as a
# group: the assertion passes if any member of a group is present.
DEFAULT_EXPECTED_SINGLE = [0x01, 0x02, 0x03, 0x04, 0x05, 0x00]
DEFAULT_EXPECTED_GROUPS = [[0x1C, 0x1D]]  # 28 / 29 scheduler codes
DEFAULT_FORBIDDEN = [0xFF]                 # K_panic


def parse_int(token):
    """Parse a code given as decimal or 0x-hex."""
    return int(token, 0)


def parse_vcd_porta_values(text):
    """Extract the ordered sequence of PORTA byte values from VCD text.

    Supports two trace shapes so both the simulavr CLI VCD path and the
    pysimulavr fallback (which emits the same format) parse identically:

      * Standard VCD: `$var ... <id> PORTA[7:0] $end` declarations followed by
        `b<bits> <id>` value-change lines, scoped by `#<time>` timestamps.
      * Simple trace: `#<time> PORTA b<bits>` one-line records.

    Returns a list of (time, value) tuples in trace order.
    """
    id_to_porta = {}
    samples = []
    current_time = 0

    for raw in text.splitlines():
        line = raw.strip()
        if not line:
            continue

        if line.startswith("$var"):
            parts = line.split()
            # $var wire 8 <id> PORTA[7:0] $end  -> id at index 3, name at 4
            if len(parts) >= 5 and parts[4].split("[")[0].upper() == "PORTA":
                id_to_porta[parts[3]] = True
            continue

        if line.startswith("#"):
            # Timestamp, optionally followed by an inline simple record.
            body = line[1:]
            head = body.split(None, 1)
            try:
                current_time = int(head[0])
            except ValueError:
                continue
            if len(head) == 2:
                rest = head[1].split()
                # `#<time> PORTA b1010`
                if len(rest) >= 2 and rest[0].upper().startswith("PORTA"):
                    val = _bits_to_int(rest[1])
                    if val is not None:
                        samples.append((current_time, val))
            continue

        if line[0] == "b":
            # Binary value change: `b<bits> <id>`
            parts = line.split()
            if len(parts) == 2 and parts[1] in id_to_porta:
                val = _bits_to_int(parts[0])
                if val is not None:
                    samples.append((current_time, val))
            continue

    return samples


def _bits_to_int(token):
    """Convert a VCD binary value token (optionally `b`-prefixed) to an int."""
    bits = token[1:] if token[:1] == "b" else token
    if not bits or any(c not in "01" for c in bits):
        return None
    return int(bits, 2)


def evaluate(values, expected_single, expected_groups, forbidden):
    """Apply the assertion contract. Returns (passed, reasons, observed_set)."""
    observed = [v for (_, v) in values]
    observed_set = set(observed)
    reasons = []

    hit_forbidden = sorted(observed_set & set(forbidden))
    if hit_forbidden:
        reasons.append(
            "forbidden code(s) present: "
            + ", ".join("0x%02X" % c for c in hit_forbidden)
        )

    missing = [c for c in expected_single if c not in observed_set]
    if missing:
        reasons.append(
            "expected code(s) missing: "
            + ", ".join("0x%02X" % c for c in missing)
        )

    for group in expected_groups:
        if not (observed_set & set(group)):
            reasons.append(
                "none of expected group present: "
                + ", ".join("0x%02X" % c for c in group)
            )

    passed = not reasons
    return passed, reasons, observed_set


def load_values(path):
    # Auto-detect format: VCD is UTF-8 text starting with '$'; the raw binary
    # produced by `simulavr -W 0x1b,file` is not valid VCD text.
    try:
        with open(path, "r", encoding="utf-8", errors="strict") as handle:
            text = handle.read()
        stripped = text.lstrip()
        if stripped.startswith("$") or stripped.startswith("#"):
            return parse_vcd_porta_values(text)
    except UnicodeDecodeError:
        pass
    # Binary format: one raw byte per PORTA write, from simulavr -W.
    with open(path, "rb") as handle:
        data = handle.read()
    return [(i, b) for i, b in enumerate(data)]


def cmd_assert(args):
    values = load_values(args.vcd)
    expected_single = (
        [parse_int(t) for t in args.expected]
        if args.expected is not None
        else DEFAULT_EXPECTED_SINGLE
    )
    expected_groups = (
        [[parse_int(t) for t in g.split(",")] for g in args.expect_any]
        if args.expect_any is not None
        else DEFAULT_EXPECTED_GROUPS
    )
    forbidden = (
        [parse_int(t) for t in args.forbidden]
        if args.forbidden is not None
        else DEFAULT_FORBIDDEN
    )

    passed, reasons, observed = evaluate(
        values, expected_single, expected_groups, forbidden
    )

    print("observed PORTA codes: " + format_codes(observed))
    if passed:
        print("PASS: boot + scheduler progress confirmed; no forbidden codes.")
        return 0
    for reason in reasons:
        print("FAIL: " + reason, file=sys.stderr)
    return 1


def cmd_observe(args):
    values = load_values(args.vcd)
    observed = {v for (_, v) in values}
    print("observed PORTA codes: " + format_codes(observed))
    print("total transitions: %d" % len(values))
    return 0


def format_codes(code_set):
    if not code_set:
        return "(none)"
    return ", ".join("0x%02X" % c for c in sorted(code_set))


def cmd_pysim_run(args):
    """Run the ELF via pysimulavr, polling PORTA, and write a simple VCD trace."""
    try:
        import pysimulavr
    except ImportError:
        print(
            "pysim-run: pysimulavr not available; install python3-simulavr "
            "or use the simulavr CLI path.",
            file=sys.stderr,
        )
        return 1

    elf, device, out = args.pysim_run
    sim_max_ns = int(os.environ.get("SIM_MAX_NS", "100000000"))
    porta_addr = int(os.environ.get("PORTA_IO_ADDR", "0x3b"), 0)

    dev = pysimulavr.AvrFactory.instance().makeDevice(device)
    dev.Load(elf)
    dev.SetClockFreq(1)  # 1 ns per clock tick keeps step math simple
    sim = pysimulavr.SystemClock.Instance()
    sim.Add(dev)

    samples = []
    last = None
    while sim.GetCurrentTime() < sim_max_ns:
        sim.Step()
        val = dev.getRWMem(porta_addr) & 0xFF
        if val != last:
            samples.append((sim.GetCurrentTime(), val))
            last = val

    write_simple_vcd(out, samples)
    print("pysim-run: wrote %d PORTA transitions to %s" % (len(samples), out))
    return 0


def write_simple_vcd(path, samples):
    """Write a minimal VCD that parse_vcd_porta_values can read back."""
    with open(path, "w", encoding="utf-8") as handle:
        handle.write("$timescale 1ns $end\n")
        handle.write("$scope module top $end\n")
        handle.write("$var wire 8 ! PORTA[7:0] $end\n")
        handle.write("$upscope $end\n")
        handle.write("$enddefinitions $end\n")
        for time, val in samples:
            handle.write("#%d\n" % time)
            handle.write("b%s !\n" % format(val, "08b"))


def cmd_self_test(_args):
    """Synthetic-fixture unit tests for the parser and assertion contract."""
    failures = []

    def check(name, condition):
        status = "ok  " if condition else "FAIL"
        print("  %s %s" % (status, name))
        if not condition:
            failures.append(name)

    # Standard VCD with a full healthy boot + scheduler sequence.
    healthy = (
        "$var wire 8 ! PORTA[7:0] $end\n"
        "$enddefinitions $end\n"
        "#0\nb00000001 !\n"
        "#1\nb00000010 !\n"
        "#2\nb00000011 !\n"
        "#3\nb00000100 !\n"
        "#4\nb00000101 !\n"
        "#5\nb00011100 !\n"  # 0x1C scheduler
        "#6\nb00000000 !\n"  # user_init
    )
    vals = parse_vcd_porta_values(healthy)
    passed, reasons, observed = evaluate(
        vals, DEFAULT_EXPECTED_SINGLE, DEFAULT_EXPECTED_GROUPS, DEFAULT_FORBIDDEN
    )
    check("healthy boot parses 7 transitions", len(vals) == 7)
    check("healthy boot passes assertion", passed and not reasons)
    check("healthy observed includes 0x1C", 0x1C in observed)

    # Panic appears mid-run after valid boot codes -> must FAIL.
    panicked = healthy + "#7\nb11111111 !\n"
    vals = parse_vcd_porta_values(panicked)
    passed, reasons, _ = evaluate(
        vals, DEFAULT_EXPECTED_SINGLE, DEFAULT_EXPECTED_GROUPS, DEFAULT_FORBIDDEN
    )
    check("mid-run panic fails assertion", not passed)
    check("panic reason mentions forbidden", any("forbidden" in r for r in reasons))

    # Missing scheduler code -> must FAIL on the group requirement.
    no_sched = (
        "$var wire 8 ! PORTA[7:0] $end\n$enddefinitions $end\n"
        "#0\nb00000001 !\n#1\nb00000010 !\n#2\nb00000011 !\n"
        "#3\nb00000100 !\n#4\nb00000101 !\n#5\nb00000000 !\n"
    )
    vals = parse_vcd_porta_values(no_sched)
    passed, reasons, _ = evaluate(
        vals, DEFAULT_EXPECTED_SINGLE, DEFAULT_EXPECTED_GROUPS, DEFAULT_FORBIDDEN
    )
    check("missing scheduler fails assertion", not passed)
    check("missing scheduler reason mentions group", any("group" in r for r in reasons))

    # Simple inline trace format parses identically.
    simple = "#0 PORTA b00000001\n#1 PORTA b11111111\n"
    vals = parse_vcd_porta_values(simple)
    check("simple format parses 2 transitions", len(vals) == 2)
    check("simple format reads 0xFF", any(v == 0xFF for (_, v) in vals))

    # Round-trip: write_simple_vcd output is parseable.
    import tempfile

    handle = tempfile.NamedTemporaryFile(
        mode="w", suffix=".vcd", delete=False, encoding="utf-8"
    )
    handle.close()
    write_simple_vcd(handle.name, [(0, 0x01), (1, 0x05), (2, 0x1D)])
    with open(handle.name, encoding="utf-8") as fh:
        vals = parse_vcd_porta_values(fh.read())
    os.unlink(handle.name)
    check("round-trip writes/reads 3 transitions", len(vals) == 3)

    print()
    if failures:
        print("SELF-TEST FAILED: %d case(s)" % len(failures), file=sys.stderr)
        return 1
    print("SELF-TEST PASSED")
    return 0


def build_parser():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("vcd", nargs="?", help="path to the PORTA VCD trace")
    parser.add_argument(
        "--observe", action="store_true",
        help="print observed codes only; always exit 0",
    )
    parser.add_argument(
        "--expected", nargs="*", metavar="CODE",
        help="codes that must each appear (decimal or 0x-hex)",
    )
    parser.add_argument(
        "--expect-any", nargs="*", metavar="A,B",
        help="comma-separated group where at least one member must appear",
    )
    parser.add_argument(
        "--forbidden", nargs="*", metavar="CODE",
        help="codes that must NOT appear anywhere (default 0xFF)",
    )
    parser.add_argument(
        "--pysim-run", nargs=3, metavar=("ELF", "DEVICE", "OUT"),
        help="run ELF on DEVICE via pysimulavr; write trace to OUT",
    )
    parser.add_argument(
        "--self-test", action="store_true",
        help="run synthetic-fixture unit tests",
    )
    return parser


def main(argv):
    args = build_parser().parse_args(argv)

    if args.self_test:
        return cmd_self_test(args)
    if args.pysim_run:
        return cmd_pysim_run(args)
    if not args.vcd:
        print("error: a VCD path is required", file=sys.stderr)
        return 2
    if args.observe:
        return cmd_observe(args)
    return cmd_assert(args)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
