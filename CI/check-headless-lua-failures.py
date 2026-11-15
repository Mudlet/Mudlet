#!/usr/bin/env python3
"""Checks a busted run in mudlet-headless against the specs known to fail there.

Many specs drive a main window that a run with no main window does not have, so
the run fails on those whatever the change under test. This passes only when the
failures and errors the log reports are exactly the listed ones. It fails on a
log with no busted summary, as after a crash or a hang; on a failure busted
cannot place in a test, as when a spec file does not load; and on an error in a
setup or teardown hook, which stops every test under it from running.

Usage: check-headless-lua-failures.py <busted log> <known failures file>... [--write]

Each entry in a known failures file is "Failure <spec file>: <full test name>"
or "Error <spec file>: <full test name>", once for each test of that name that
fails. Blank lines and lines starting with '#' are ignored. Given several files,
the run must fail what they list between them, less any entry a line of "Not "
followed by that entry takes off: a browser run lists only its own failures, and
takes off desktop ones its tests do not get as far as. --write updates the last
file from the log: it keeps the comments and the lines that still hold where
they are, drops the rest and adds what is missing at the end.
"""

import collections
import os
import re
import sys

SUMMARY = re.compile(r"^(\d+) success(?:es)? / (\d+) failures? / (\d+) errors? / (\d+) pending : [0-9.]+ seconds\s*$")
HEADER = re.compile(r"^(Pending|Failure|Error) -> (.*)$")
TRACED = re.compile(r"^(?:.*[/\\])?([^/\\]+_spec\.lua) @ \d+\s*$")
ENTRY = re.compile(r"^(Not )?((?:Failure|Error) [^/\\]+_spec\.lua: .+)$")
HOOK = re.compile(r" (?:lazy_|strict_)?(?:setup|teardown)$| (?:before|after)_each$")


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


class Known:
    def __init__(self, path):
        with open(path, encoding="utf-8") as known:
            self.lines = known.read().splitlines()
        self.entries = collections.Counter()
        self.taken_off = collections.Counter()
        self.malformed = []
        for line in self.lines:
            text = line.strip()
            if not text or text.startswith("#"):
                continue
            match = ENTRY.match(text)
            if not match:
                self.malformed.append(line)
            elif match.group(1):
                self.taken_off[match.group(2)] += 1
            else:
                self.entries[text] += 1


def write_known(path, known, others, run):
    remaining = run.entries - others
    remaining_off = others - run.entries
    kept = []
    for line in known.lines:
        text = line.strip()
        match = ENTRY.match(text)
        if match:
            left = remaining_off if match.group(1) else remaining
            if left[match.group(2)] <= 0:
                continue
            left[match.group(2)] -= 1
        kept.append(line)
    added = sorted(remaining.elements()) + [f"Not {entry}" for entry in sorted(remaining_off.elements())]
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
    write = argv[-1:] == ["--write"]
    paths = argv[2:-1] if write else argv[2:]
    if len(argv) < 3 or not paths or "--write" in paths:
        print(__doc__.strip().split("\n\n")[2])
        return 2
    log_path = argv[1]
    run = read_log(log_path)
    if run is None:
        print(f"The busted run in {log_path} did not finish - it crashed, hung or never started.")
        return 1
    lists = [Known(path) for path in paths]
    listed = sum((known.entries for known in lists), collections.Counter())
    taken_off = sum((known.taken_off for known in lists), collections.Counter())
    known = listed - taken_off
    named = " or ".join(paths)

    problems = []
    for path, each in zip(paths, lists):
        report(problems, f"Lines in {path} that are not an entry", each.malformed)
    if not write:
        # --write drops them
        report(problems, f"Lines that take off an entry {named} does not list", sorted(f"Not {entry}" for entry in (taken_off - listed).elements()))
    report(problems, "Failures busted could not place in a test, as when a spec file does not load", run.unplaced)
    counts = {"Failure": run.failures, "Error": run.errors, "Pending": run.pending}
    mismatched = [f"{kind}: the summary says {count}, the log reports {run.counted[kind]}" for kind, count in counts.items() if run.counted[kind] != count]
    report(problems, "The summary does not match the reports under it, so the log cannot be read", mismatched)
    hooks = sorted(entry for entry in run.entries if entry.startswith("Error ") and HOOK.search(entry))
    report(problems, "Hooks that failed, so none of the tests under them ran; guard them in the spec rather than list them", hooks)

    if write:
        if problems:
            print(f"{paths[-1]} was not written.")
            return 1
        others = sum((each.entries for each in lists[:-1]), collections.Counter()) - sum((each.taken_off for each in lists[:-1]), collections.Counter())
        added = write_known(paths[-1], lists[-1], others, run)
        print(f"Wrote {paths[-1]} for {sum(run.entries.values())} known failures, {len(added)} lines of it new.")
        return 0

    new = sorted((run.entries - known).elements())
    report(problems, f"Failed but not listed in {named}", new)
    stale = sorted((known - run.entries).elements())
    report(problems, f"Listed in {named} but did not fail; take them off the list with --write", stale)

    if problems:
        if os.environ.get("GITHUB_ACTIONS") == "true":
            for problem in problems:
                print(f"::error file={paths[-1]}::{problem}")
        return 1
    print(f"The {sum(known.values())} listed failures, and only those, failed; {run.successes} passed and {run.pending} pended.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
