#!/usr/bin/env python3
"""Checks a busted run in mudlet-headless against the specs known to fail there.

Many specs drive a main window that a run with no main window does not have, so
the run fails on those whatever the change under test. This passes only when the
failures and errors the log reports are exactly the listed ones, and the run
passed and pended no fewer and no more tests than the list records. It fails on
a log with no busted summary, as after a crash or a hang; on a failure busted
cannot place in a test, as when a spec file does not load; and on an error in a
setup or teardown hook, which stops every test under it from running.

Usage: check-headless-lua-failures.py <busted log> <known failures file> [--write]

Each entry in the known failures file is "Failure <spec file>: <full test name>"
or "Error <spec file>: <full test name>", once for each test of that name that
fails. "minimum successes: <n>" and "maximum pending: <n>" record the rest of the
run. Blank lines and lines starting with '#' are ignored. --write updates the
file from the log: it keeps the comments and the entries that still fail where
they are, drops the rest and adds new failures at the end.
"""

import collections
import os
import re
import sys

SUMMARY = re.compile(r"^(\d+) success(?:es)? / (\d+) failures? / (\d+) errors? / (\d+) pending : [0-9.]+ seconds\s*$")
HEADER = re.compile(r"^(Pending|Failure|Error) -> (.*)$")
TRACED = re.compile(r"^(?:.*[/\\])?([^/\\]+_spec\.lua) @ \d+\s*$")
ENTRY = re.compile(r"^(Failure|Error) [^/\\]+_spec\.lua: .+$")
HOOK = re.compile(r" (?:lazy_|strict_)?(?:setup|teardown)$| (?:before|after)_each$")
SUCCESSES = "minimum successes"
PENDING = "maximum pending"


class Run:
    def __init__(self):
        self.successes = self.failures = self.errors = self.pending = 0
        self.entries = collections.Counter()
        self.unplaced = []
        self.counted = collections.Counter()


def read_log(path):
    with open(path, encoding="utf-8", errors="replace") as log:
        lines = log.read().splitlines()
    starts = [index for index, line in enumerate(lines) if SUMMARY.match(line)]
    if not starts:
        return None
    run = Run()
    start = starts[-1]
    run.successes, run.failures, run.errors, run.pending = (int(count) for count in SUMMARY.match(lines[start]).groups())
    # busted puts a blank line before each report, so a message line that looks like a header is not taken for one
    for index in range(start + 1, len(lines)):
        match = HEADER.match(lines[index])
        if not match or lines[index - 1].strip():
            continue
        kind, rest = match.groups()
        run.counted[kind] += 1
        if kind == "Pending":
            continue
        traced = TRACED.match(rest)
        name = lines[index + 1].strip() if index + 1 < len(lines) else ""
        if traced:
            run.entries[f"{kind} {traced.group(1)}: {name}"] += 1
        else:
            run.unplaced.append(f"{kind} -> {rest} ({name})")
    return run


def read_known(path):
    with open(path, encoding="utf-8") as known:
        lines = known.read().splitlines()
    entries = collections.Counter()
    limits = {}
    malformed = []
    for line in lines:
        text = line.strip()
        if not text or text.startswith("#"):
            continue
        key, _, value = text.partition(":")
        if key in (SUCCESSES, PENDING) and value.strip().isdigit():
            limits[key] = int(value)
        elif ENTRY.match(text):
            entries[text] += 1
        else:
            malformed.append(line)
    return lines, entries, limits, malformed


def write_known(path, lines, run):
    remaining = collections.Counter(run.entries)
    kept = []
    for line in lines:
        text = line.strip()
        key = text.partition(":")[0]
        if key == SUCCESSES:
            line = f"{SUCCESSES}: {run.successes}"
        elif key == PENDING:
            line = f"{PENDING}: {run.pending}"
        elif text and not text.startswith("#"):
            if remaining[text] <= 0:
                continue
            remaining[text] -= 1
        kept.append(line)
    keys = {line.strip().partition(":")[0] for line in kept}
    for key, value in ((PENDING, run.pending), (SUCCESSES, run.successes)):
        if key not in keys:
            kept.insert(next((i for i, line in enumerate(kept) if line.strip() and not line.startswith("#")), len(kept)), f"{key}: {value}")
    added = sorted(remaining.elements())
    with open(path, "w", encoding="utf-8") as out:
        out.write("\n".join(kept + added) + "\n")
    return added


def report(problems, title, items):
    if not items:
        return
    problems.append(title)
    print(f"{title}:")
    for item in items:
        print(f"  {item}")


def main(argv):
    if len(argv) not in (3, 4) or (len(argv) == 4 and argv[3] != "--write"):
        print(__doc__.strip().split("\n\n")[2])
        return 2
    log_path, known_path = argv[1], argv[2]
    run = read_log(log_path)
    if run is None:
        print(f"The busted run in {log_path} did not finish - it crashed, hung or never started.")
        return 1
    lines, known, limits, malformed = read_known(known_path)

    problems = []
    report(problems, f"Lines in {known_path} that are not an entry", malformed)
    report(problems, "Failures busted could not place in a test, as when a spec file does not load", run.unplaced)
    counts = {"Failure": run.failures, "Error": run.errors, "Pending": run.pending}
    mismatched = [f"{kind}: the summary says {count}, the log reports {run.counted[kind]}" for kind, count in counts.items() if run.counted[kind] != count]
    report(problems, "The summary does not match the reports under it, so the log cannot be read", mismatched)
    hooks = sorted(entry for entry in run.entries if entry.startswith("Error ") and HOOK.search(entry))
    report(problems, "Hooks that failed, so none of the tests under them ran; guard them in the spec rather than list them", hooks)

    if len(argv) == 4:
        if problems:
            print(f"{known_path} was not written.")
            return 1
        added = write_known(known_path, lines, run)
        print(f"Wrote {sum(run.entries.values())} known failures to {known_path}, {len(added)} of them new.")
        return 0

    new = sorted((run.entries - known).elements())
    report(problems, f"Failed with no main window but not listed in {known_path}", new)
    stale = sorted((known - run.entries).elements())
    report(problems, f"Listed in {known_path} but did not fail; take them off the list with --write", stale)
    shortfall = []
    if run.successes < limits.get(SUCCESSES, 0):
        shortfall.append(f"{run.successes} tests passed, fewer than the {limits[SUCCESSES]} recorded")
    if run.pending > limits.get(PENDING, 0):
        shortfall.append(f"{run.pending} tests pended, more than the {limits.get(PENDING, 0)} recorded")
    report(problems, "Fewer tests ran than the list records; if that was meant, update it with --write", shortfall)

    if problems:
        if os.environ.get("GITHUB_ACTIONS") == "true":
            for problem in problems:
                print(f"::error file=CI/headless-lua-known-failures.txt::{problem}")
        return 1
    print(f"The {sum(known.values())} listed failures, and only those, failed; {run.successes} passed and {run.pending} pended.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
