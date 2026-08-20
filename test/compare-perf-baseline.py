#!/usr/bin/env python3
"""Compare two PipelineBenchmark runs and gate on throughput regressions.

Absolute benchmark numbers are meaningless across machines, so the only valid
comparison is an OLDER vs a NEWER Mudlet built and run on the SAME machine (the
libmudlet refactor's 10% throughput-loss gate, issue #9011). This reads the
`METRIC <name> <value>` lines PipelineBenchmark prints, computes per-metric
deltas, and exits non-zero if any gated metric regressed past the threshold.

Usage:
  # run two already-built binaries:
  test/compare-perf-baseline.py --run before-build/.../PipelineBenchmark \\
      after-build/.../PipelineBenchmark
  # or compare two captured METRIC dumps:
  test/compare-perf-baseline.py before.txt after.txt

Exit codes: 0 = within threshold, 1 = a gated metric regressed, 2 = usage error
or the two runs are not comparable. This script is the arbiter of the gate, so it
fails loud (exit 2) rather than silently passing on anything it cannot trust: a
missing or unparseable gated metric, a missing invariant, a non-positive
baseline, or a --gate name that matches no metric.
"""

import argparse
import math
import os
import subprocess
import sys

# Fixed properties of the corpus/trigger set plus the build flavour; if any
# differ, the two runs used different harnesses or build configurations and the
# comparison is invalid - so we abort. build_asan guards against comparing an
# ASan build to a release build, whose absolute numbers are incomparable;
# corpus_version guards against comparing across a retune of the generated
# corpus, which moves every absolute number the benchmark reports.
# display_rows/cols_per_paint describe the display bench's workload the way
# text_corpus_* describe the text bench's. They move with the font metrics and
# the surrounding layout, and a build that draws a differently-sized screen did
# not do the same work - which is a reason to refuse the comparison, not to
# report the difference as a throughput change.
INVARIANTS = (
    "text_corpus_lines",
    "text_corpus_bytes",
    "trigger_count",
    "build_asan",
    "corpus_version",
    "display_rows_per_paint",
    "display_cols_per_paint",
)

# Gated by default: throughput (lines/sec) for the text and trigger pipelines,
# plus the shipped default packages on the same corpus - the pipeline metrics run
# on a bare profile, so only defaults_text_lines_per_sec can see a package
# costing every new user throughput.
# trigger_overhead_ms is intentionally NOT here - it is a difference of two noisy
# best-passes (up to ~16% run-to-run worst case, wider than the 10% gate), so it
# would fire on noise. It stays emitted and reportable, and can be gated
# explicitly with --gate trigger_overhead_ms when a change targets matching.
DEFAULT_GATE = ("text_lines_per_sec", "trigger_lines_per_sec", "defaults_text_lines_per_sec")

# Wall-clock ceiling for a single benchmark run under --run. The ASan/offscreen
# functional-test build feeds a huge corpus several times, so this is generous.
RUN_TIMEOUT_SECONDS = 1200


def fail(message):
    """Abort with exit code 2: a usage error or two runs that cannot be compared."""
    sys.stderr.write(f"error: {message}\n")
    sys.exit(2)


def classify(name):
    """Return 'higher', 'lower', or 'invariant' for how to read a metric."""
    if name in INVARIANTS:
        return "invariant"
    if name.endswith("_per_sec"):
        return "higher"  # throughput: bigger is better
    if name.endswith("_ms") or name.endswith("_kb"):
        return "lower"  # time / memory: smaller is better
    return "info"


def parse_metrics(text, source):
    """Parse `METRIC <name> <value>` lines, failing hard on anything malformed.

    Any line whose first whitespace-token is exactly `METRIC` must parse fully:
    exactly three tokens, a finite numeric value, and no duplicate name.
    Silently dropping such a line (a NaN/Inf value, a comma decimal, a
    concatenated capture) would let a gated metric vanish and the gate pass by
    default - the exact failure mode this arbiter must never have.
    """
    metrics = {}
    for raw in text.splitlines():
        line = raw.strip()
        if not line.startswith("METRIC"):
            continue
        parts = line.split()
        if parts[0] != "METRIC":
            continue  # e.g. a "METRICS ..." log line, not one of ours
        if len(parts) != 3:
            fail(f"{source}: malformed METRIC line {raw!r} (expected 'METRIC <name> <value>')")
        name, raw_value = parts[1], parts[2]
        try:
            value = float(raw_value)
        except ValueError:
            fail(f"{source}: METRIC {name} has a non-numeric value {raw_value!r}")
        if not math.isfinite(value):
            fail(f"{source}: METRIC {name} value {raw_value!r} is not a finite number")
        if name in metrics:
            fail(f"{source}: METRIC {name} appears more than once")
        metrics[name] = value
    return metrics


def run_binary(path):
    if not os.path.isfile(path):
        fail(f"{path} is not a file")
    if not os.access(path, os.X_OK):
        fail(f"{path} is not an executable benchmark binary")
    env = dict(os.environ)
    env.setdefault("QT_QPA_PLATFORM", "offscreen")
    env.setdefault("ASAN_OPTIONS", "detect_leaks=0")
    # Same environment ctest registers the benchmark with, so a run from here
    # measures the profile a run from there does. Without it mpkg comes back,
    # and with it the package listing download and the self-upgrade it can
    # trigger - which is both noise in the numbers and a different profile.
    env.setdefault("MUDLET_TEST_MODE", "1")
    print(f"running {path} ...", file=sys.stderr)
    try:
        result = subprocess.run(
            [os.path.abspath(path)],
            capture_output=True,
            text=True,
            env=env,
            timeout=RUN_TIMEOUT_SECONDS,
        )
    except subprocess.TimeoutExpired:
        fail(f"{path} did not finish within {RUN_TIMEOUT_SECONDS}s")
    if result.returncode != 0:
        sys.stderr.write(result.stdout)
        sys.stderr.write(result.stderr)
        fail(f"{path} exited with {result.returncode}")
    return result.stdout


def load(source, run):
    if run:
        return parse_metrics(run_binary(source), source)
    try:
        with open(source, encoding="utf-8") as handle:
            return parse_metrics(handle.read(), source)
    except OSError as error:
        fail(f"cannot read {source}: {error}")


def check_invariants(before, after):
    for name in INVARIANTS:
        in_before = name in before
        in_after = name in after
        if not in_before or not in_after:
            missing = "before" if not in_before else "after"
            fail(
                f"invariant {name} is missing from the {missing} run - the two runs are not from "
                "the same PipelineBenchmark harness/build and cannot be compared."
            )
        if before[name] != after[name]:
            fail(
                f"{name} differs ({before[name]:g} vs {after[name]:g}) - the two runs used "
                "different corpora, trigger sets or build configurations and cannot be compared. "
                "Rebuild both trees from the same PipelineBenchmark harness, built the same way."
            )


def compare(before, after, threshold, gate):
    rows = []
    failed = False
    for name in sorted(set(before) | set(after)):
        kind = classify(name)
        if kind == "invariant":
            continue

        gated = name in gate
        if name not in before or name not in after:
            if gated:
                missing = "before" if name not in before else "after"
                fail(f"gated metric {name} is missing from the {missing} run - cannot evaluate the gate.")
            rows.append((name, "-", "MISSING", ""))
            continue

        old, new = before[name], after[name]
        if old <= 0:
            if gated:
                fail(
                    f"gated metric {name} has a non-positive 'before' value ({old:g}); a valid "
                    "throughput/time baseline must be greater than zero, so the runs are not comparable."
                )
            rows.append((name, f"{old:g} -> {new:g}", "SKIP", "before <= 0"))
            continue

        change = (new / old) - 1.0  # signed fractional change, after vs before
        if kind == "higher":
            regressed = change < -threshold
            delta = f"{change * 100:+.1f}%"
        elif kind == "lower":
            regressed = change > threshold
            delta = f"{change * 100:+.1f}% (lower is better)"
        else:
            regressed = False
            delta = f"{change * 100:+.1f}%"

        if gated and regressed:
            status = "FAIL"
            failed = True
        elif gated:
            status = "PASS"
        elif regressed:
            status = "warn"
        else:
            status = "info"
        rows.append((name, f"{old:g} -> {new:g}", status, delta))
    return rows, failed


def main():
    parser = argparse.ArgumentParser(
        description="Compare two PipelineBenchmark runs (older vs newer Mudlet, same machine).",
        epilog="See docs/libmudlet-perf-baseline.md for the full before/after workflow.",
    )
    parser.add_argument("before", help="'before' METRIC file, or benchmark binary with --run")
    parser.add_argument("after", help="'after' METRIC file, or benchmark binary with --run")
    parser.add_argument("--run", action="store_true", help="treat the two arguments as benchmark binaries to run")
    parser.add_argument("--threshold", type=float, default=0.10, help="max tolerated fractional regression (default 0.10); a value >= 1 is read as a percentage")
    parser.add_argument("--gate", default=",".join(DEFAULT_GATE), help="comma-separated metrics that fail the run")
    args = parser.parse_args()

    threshold = args.threshold
    if threshold <= 0:
        fail("--threshold must be greater than 0")
    if threshold >= 1:
        sys.stderr.write(f"note: --threshold {threshold:g} looks like a percentage; reading it as {threshold / 100:g} ({threshold:g}%).\n")
        threshold /= 100.0

    gate = {name.strip() for name in args.gate.split(",") if name.strip()}

    before = load(args.before, args.run)
    after = load(args.after, args.run)
    if not before or not after:
        fail("no METRIC lines found in one of the runs")

    known = set(before) | set(after)
    unknown_gates = sorted(name for name in gate if name not in known)
    if unknown_gates:
        fail(f"--gate names not found in either run: {', '.join(unknown_gates)} - check for a typo.")

    check_invariants(before, after)
    rows, failed = compare(before, after, threshold, gate)

    name_width = max([len("metric")] + [len(row[0]) for row in rows])
    value_width = max([len("before -> after")] + [len(row[1]) for row in rows])
    print(f"Regression gate: {threshold * 100:.0f}%   gated metrics: {', '.join(sorted(gate))}\n")
    print(f"{'metric'.ljust(name_width)}  {'before -> after'.ljust(value_width)}  status  delta")
    print(f"{'-' * name_width}  {'-' * value_width}  ------  -----")
    for name, value, status, delta in rows:
        print(f"{name.ljust(name_width)}  {value.ljust(value_width)}  {status:<6}  {delta}")

    print()
    if failed:
        print(f"FAIL: at least one gated metric lost more than {threshold * 100:.0f}%.")
        return 1
    print(f"PASS: all gated metrics stayed within {threshold * 100:.0f}%.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
