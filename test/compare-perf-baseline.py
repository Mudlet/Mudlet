#!/usr/bin/env python3
"""Compare two PipelineBenchmark runs and gate on throughput regressions.

The libmudlet refactor (issue #9011) carries a "no more than 10% throughput
loss" gate. Absolute benchmark numbers are meaningless across machines, so this
tool does the only comparison that is meaningful: an OLDER Mudlet vs a NEWER
Mudlet built and run on the SAME machine.

It reads the `METRIC <name> <value>` lines that
test/functional_tests/PipelineBenchmark.cpp prints, computes the per-metric
delta between a "before" and an "after" run, and exits non-zero if any gated
metric regressed by more than the threshold (default 10%).

Two ways to use it:

  # 1. Run two already-built binaries from two build trees:
  test/compare-perf-baseline.py --run \\
      before-build/test/functional_tests/PipelineBenchmark \\
      after-build/test/functional_tests/PipelineBenchmark

  # 2. Compare two captured METRIC outputs (`... PipelineBenchmark > before.txt`):
  test/compare-perf-baseline.py before.txt after.txt

Options:
  --threshold 0.10   max tolerated fractional regression on a gated metric
  --gate a,b,c       override which metrics fail the run (default:
                     text_lines_per_sec,trigger_lines_per_sec,trigger_overhead_ms)

Exit codes: 0 = all gated metrics within threshold, 1 = a gated metric
regressed, 2 = usage error or the two runs are not comparable.
"""

import argparse
import os
import re
import subprocess
import sys

# Metrics whose absolute value is a fixed property of the corpus/trigger set.
# If they differ between the two runs the runs used different harnesses and the
# comparison is invalid, so we abort rather than report a bogus delta.
INVARIANTS = ("text_corpus_lines", "text_corpus_bytes", "trigger_count")

# Which metrics fail the run by default. The task's focus: throughput
# (lines/sec) and the isolated matcher cost (trigger_overhead_ms).
DEFAULT_GATE = ("text_lines_per_sec", "trigger_lines_per_sec", "trigger_overhead_ms")

METRIC_RE = re.compile(r"^METRIC\s+(\S+)\s+(-?[\d.]+)\s*$")


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


def parse_metrics(text):
    metrics = {}
    for line in text.splitlines():
        match = METRIC_RE.match(line.strip())
        if match:
            metrics[match.group(1)] = float(match.group(2))
    return metrics


def run_binary(path):
    if not os.access(path, os.X_OK):
        fail(f"{path} is not an executable benchmark binary")
    env = dict(os.environ)
    env.setdefault("QT_QPA_PLATFORM", "offscreen")
    env.setdefault("ASAN_OPTIONS", "detect_leaks=0")
    print(f"running {path} ...", file=sys.stderr)
    result = subprocess.run([os.path.abspath(path)], capture_output=True, text=True, env=env)
    if result.returncode != 0:
        sys.stderr.write(result.stdout)
        sys.stderr.write(result.stderr)
        fail(f"{path} exited with {result.returncode}")
    return result.stdout


def load(source, run):
    if run:
        return parse_metrics(run_binary(source))
    try:
        with open(source, encoding="utf-8") as handle:
            return parse_metrics(handle.read())
    except OSError as error:
        fail(f"cannot read {source}: {error}")


def check_invariants(before, after):
    for name in INVARIANTS:
        if name in before and name in after and before[name] != after[name]:
            fail(
                f"{name} differs ({before[name]:g} vs {after[name]:g}) - the two runs used "
                "different corpora or trigger sets and cannot be compared. Rebuild both trees "
                "from the same PipelineBenchmark harness."
            )


def compare(before, after, threshold, gate):
    rows = []
    failed = False
    for name in sorted(set(before) | set(after)):
        kind = classify(name)
        if kind == "invariant":
            continue
        if name not in before or name not in after:
            rows.append((name, "-", "MISSING", ""))
            continue

        old, new = before[name], after[name]
        if old == 0:
            rows.append((name, f"{old:g} -> {new:g}", "SKIP", "before is zero"))
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

        gated = name in gate
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
    parser.add_argument("--threshold", type=float, default=0.10, help="max tolerated fractional regression (default 0.10)")
    parser.add_argument("--gate", default=",".join(DEFAULT_GATE), help="comma-separated metrics that fail the run")
    args = parser.parse_args()

    gate = {name.strip() for name in args.gate.split(",") if name.strip()}

    before = load(args.before, args.run)
    after = load(args.after, args.run)
    if not before or not after:
        fail("no METRIC lines found in one of the runs")

    check_invariants(before, after)
    rows, failed = compare(before, after, args.threshold, gate)

    name_width = max([len("metric")] + [len(row[0]) for row in rows])
    value_width = max([len("before -> after")] + [len(row[1]) for row in rows])
    print(f"Regression gate: {args.threshold * 100:.0f}%   gated metrics: {', '.join(sorted(gate))}\n")
    print(f"{'metric'.ljust(name_width)}  {'before -> after'.ljust(value_width)}  status  delta")
    print(f"{'-' * name_width}  {'-' * value_width}  ------  -----")
    for name, value, status, delta in rows:
        print(f"{name.ljust(name_width)}  {value.ljust(value_width)}  {status:<6}  {delta}")

    print()
    if failed:
        print(f"FAIL: at least one gated metric lost more than {args.threshold * 100:.0f}%.")
        return 1
    print(f"PASS: all gated metrics stayed within {args.threshold * 100:.0f}%.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
