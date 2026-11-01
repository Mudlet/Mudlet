#!/usr/bin/env python3
"""Self-test for check-headless-lua-failures.py.

A checker that has quietly stopped reading the log passes a broken run, so CI
runs this first: each way a run can go wrong must fail the check, and a run
that matches the list must pass it.
"""

import os
import subprocess
import sys
import tempfile
import unittest

CHECKER = os.path.join(os.path.dirname(os.path.abspath(__file__)), "check-headless-lua-failures.py")
SPEC = ".../run.abc123/tests/Example_spec.lua"
HOOKED = ".../run.abc123/tests/Hooked_spec.lua"
OTHER = "Example_spec.lua: Example group the other test"
ONE = "Example_spec.lua: Example group one test"


def report(kind, where, name, message="Expected objects to be equal."):
    return f"\n{kind} -> {where}\n{name}\n{message}\n"


def log(successes, failures=(), errors=(), pending=0, after=""):
    reports = "".join(report("Failure", f"{SPEC} @ {10 + i}", name) for i, name in enumerate(failures))
    reports += "".join(report("Error", where, name) for where, name in errors)
    reports += "".join(report("Pending", f"{SPEC} @ {90 + i}", "Example group a pended test", "") for i in range(pending))
    summary = f"{successes} successes / {len(failures)} failures / {len(errors)} errors / {pending} pending : 1.234 seconds"
    return f"Setup to auto-run tests - commencing.\n++++-\n{summary}\n{reports}{after}All tests complete, shutting down Mudlet\n"


KNOWN = """# Specs that fail in mudlet-headless
minimum successes: 10
maximum pending: 1

# Need a main window
Failure Example_spec.lua: Example group one test
"""


class CheckHeadlessLuaFailures(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.log_path = os.path.join(self.directory.name, "headless-lua.log")
        self.known_path = os.path.join(self.directory.name, "known.txt")

    def tearDown(self):
        self.directory.cleanup()

    def check(self, log_text, known_text=KNOWN, write=False):
        with open(self.log_path, "w", encoding="utf-8") as out:
            out.write(log_text)
        with open(self.known_path, "w", encoding="utf-8") as out:
            out.write(known_text)
        command = [sys.executable, "-I", CHECKER, self.log_path, self.known_path] + (["--write"] if write else [])
        environment = {key: value for key, value in os.environ.items() if key != "GITHUB_ACTIONS"}
        result = subprocess.run(command, capture_output=True, text=True, env=environment)
        return result.returncode, result.stdout

    def known(self):
        with open(self.known_path, encoding="utf-8") as known:
            return known.read()

    def test_passes_a_run_that_fails_exactly_the_listed_tests(self):
        status, output = self.check(log(10, ["Example group one test"], pending=1))
        self.assertEqual(status, 0, output)

    def test_fails_a_failure_that_is_not_listed(self):
        status, output = self.check(log(10, ["Example group one test", "Example group the other test"]))
        self.assertEqual(status, 1, output)
        self.assertIn(f"Failure {OTHER}", output)

    def test_fails_a_listed_test_that_now_passes(self):
        status, output = self.check(log(11))
        self.assertEqual(status, 1, output)
        self.assertIn("did not fail", output)

    def test_counts_a_test_name_that_fails_twice(self):
        twice = log(10, ["Example group one test", "Example group one test"])
        status, output = self.check(twice)
        self.assertEqual(status, 1, output)
        status, output = self.check(twice, KNOWN + f"Failure {ONE}\n")
        self.assertEqual(status, 0, output)

    def test_fails_a_failed_hook_even_when_listed(self):
        hook = (f"{HOOKED} @ 5", "Hooked group before_each")
        status, output = self.check(log(10, ["Example group one test"], [hook]), KNOWN + "Error Hooked_spec.lua: Hooked group before_each\n")
        self.assertEqual(status, 1, output)
        self.assertIn("Hooks that failed", output)

    def test_fails_a_run_with_no_summary(self):
        status, output = self.check("Setup to auto-run tests - commencing.\n++++\n")
        self.assertEqual(status, 1, output)
        self.assertIn("did not finish", output)

    def test_fails_a_spec_file_that_does_not_load(self):
        broken = (".../run.abc123/tests/Broken_spec.lua:3: unexpected symbol near ')'", "")
        status, output = self.check(log(10, ["Example group one test"], [broken]))
        self.assertEqual(status, 1, output)
        self.assertIn("could not place", output)

    def test_fails_a_log_cut_off_after_its_summary(self):
        status, output = self.check(log(10, ["Example group one test"]).split("\nFailure ->")[0] + "\n")
        self.assertEqual(status, 1, output)
        self.assertIn("summary does not match", output)

    def test_does_not_take_a_message_line_for_a_report(self):
        message = report("Failure", f"{SPEC} @ 10", "Example group one test", f"Failure -> {SPEC} @ 99\nExample group the other test")
        status, output = self.check(log(10).replace("0 failures", "1 failures").replace("\nAll tests", "\n" + message + "All tests"))
        self.assertEqual(status, 0, output)

    def test_fails_a_run_that_passed_fewer_tests_than_recorded(self):
        status, output = self.check(log(9, ["Example group one test"]))
        self.assertEqual(status, 1, output)
        self.assertIn("fewer than the 10 recorded", output)

    def test_fails_a_run_that_pended_more_tests_than_recorded(self):
        status, output = self.check(log(10, ["Example group one test"], pending=2))
        self.assertEqual(status, 1, output)
        self.assertIn("more than the 1 recorded", output)

    def test_fails_a_list_line_that_is_not_an_entry(self):
        status, output = self.check(log(10, ["Example group one test"]), KNOWN + "Example_spec.lua: no kind given\n")
        self.assertEqual(status, 1, output)
        self.assertIn("not an entry", output)

    def test_write_keeps_comments_drops_stale_entries_and_adds_new_ones(self):
        status, output = self.check(log(12, ["Example group the other test"], pending=0), write=True)
        self.assertEqual(status, 0, output)
        self.assertEqual(self.known(), KNOWN.replace("10", "12").replace("pending: 1", "pending: 0").replace(f"Failure {ONE}\n", "") + f"Failure {OTHER}\n")
        status, output = self.check(log(12, ["Example group the other test"]), self.known())
        self.assertEqual(status, 0, output)

    def test_write_adds_the_counts_to_a_list_without_them(self):
        status, output = self.check(log(12, ["Example group one test"], pending=3), f"# comment\nFailure {ONE}\n", write=True)
        self.assertEqual(status, 0, output)
        self.assertEqual(self.known(), f"# comment\nminimum successes: 12\nmaximum pending: 3\nFailure {ONE}\n")

    def test_write_leaves_the_list_alone_when_the_run_has_problems(self):
        hook = (f"{HOOKED} @ 5", "Hooked group after_each")
        status, output = self.check(log(12, [], [hook]), write=True)
        self.assertEqual(status, 1, output)
        self.assertEqual(self.known(), KNOWN)


if __name__ == "__main__":
    unittest.main()
