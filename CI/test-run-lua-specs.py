#!/usr/bin/env python3
"""Self-test for run-lua-specs.py.

A runner that has quietly stopped checking its processes passes a broken run, so
CI runs this first. A stub stands in for Mudlet: it runs the spec file named by
TESTS_DIRECTORY, prints a busted summary, records what it was given and exits.
It misbehaves when that file is named for it (MarkFails, Errors, Exits3,
NoSummary, Sleeps, Stubborn, Holds). Every stub holds a file under running/ while
it runs, so a count of that folder never exceeds the processes alive at the time.

The self-test is a POSIX program: its stubs are shell scripts and it sends
signals. The runner's Windows branches are tested here too, on POSIX, by loading
the runner in-process with its WINDOWS switch set and subprocess and os.killpg
replaced.
"""

import importlib.util
import json
import os
import queue
import re
import signal
import subprocess
import sys
import tempfile
import threading
import time
import unittest
from unittest import mock

RUNNER = os.path.join(os.path.dirname(os.path.abspath(__file__)), "run-lua-specs.py")

# Runs the runner with XVFB_READY_SECONDS lowered, so a test does not wait the full time
LOWERED_WAIT = (
    "import importlib.util, sys\n"
    "spec = importlib.util.spec_from_file_location('runner', sys.argv[1])\n"
    "runner = importlib.util.module_from_spec(spec)\n"
    "spec.loader.exec_module(runner)\n"
    "runner.XVFB_READY_SECONDS = float(sys.argv[2])\n"
    "sys.exit(runner.main(sys.argv[3:]))\n"
)

STUB = r'''
import json
import os
import signal
import sys
import time

started = time.time()
spec = os.environ["TESTS_DIRECTORY"]
name = os.path.basename(spec)
records = os.environ["STUB_RECORD_DIR"]
with open(spec, "rb") as source:
    fixture = b"MUDLET_TEST_" in source.read()
if "Stubborn" in name:
    signal.signal(signal.SIGTERM, signal.SIG_IGN)

running = os.path.join(records, "running")
os.makedirs(running, exist_ok=True)
mine = os.path.join(running, str(os.getpid()))
open(mine, "w").close()
most = len(os.listdir(running))

if "Stubborn" in name:
    time.sleep(60)
release = os.environ.get("STUB_RELEASE")
if release and "Holds" in name:
    give_up = time.time() + 15
    while not os.path.exists(release) and time.time() < give_up:
        time.sleep(0.02)

# Wait* files hold on until STUB_BARRIER of them have been seen running at once
barrier = int(os.environ.get("STUB_BARRIER", "0"))
if barrier and name.startswith("Wait"):
    released = os.path.join(records, "released")
    give_up = time.time() + 10
    while not os.path.exists(released) and time.time() < give_up:
        count = len(os.listdir(running))
        most = max(most, count)
        if count >= barrier:
            open(released, "w").close()
        time.sleep(0.02)

fixtures_most = 0
if fixture:
    fixtures_running = os.path.join(records, "fixtures-running")
    os.makedirs(fixtures_running, exist_ok=True)
    held = os.path.join(fixtures_running, str(os.getpid()))
    open(held, "w").close()
    give_up = time.time() + float(os.environ.get("STUB_FIXTURE_HOLD", "0"))
    while True:
        fixtures_most = max(fixtures_most, len(os.listdir(fixtures_running)))
        if time.time() >= give_up:
            break
        time.sleep(0.02)
    os.remove(held)

out = sys.stdout.buffer
out.write(b"Setup to auto-run tests - commencing.\n" + f"output of {name}\n".encode() + b"noise \xff\xfe that is not UTF-8\n")
out.flush()
if "Sleeps" in name:
    time.sleep(60)

failures = 1 if "MarkFails" in name else 0
errors = 1 if "Errors" in name else 0
if failures:
    with open(os.environ["MUDLET_TEST_FAILURE_MARKER"], "w", encoding="utf-8") as marker:
        marker.write("Lua busted tests failed")
successes = 1 - failures - errors
if "NoSummary" not in name:
    print(f"{successes} {'success' if successes == 1 else 'successes'} / {failures} {'failure' if failures == 1 else 'failures'} / {errors} {'error' if errors == 1 else 'errors'} / 0 pending : 0.012 seconds")
print("All tests complete, shutting down Mudlet", flush=True)

config = os.environ.get("XDG_CONFIG_HOME", "")
record = {
    "spec": spec,
    "name": name,
    "pid": os.getpid(),
    "config": config,
    "profiles": os.path.isdir(os.path.join(config, "mudlet", "profiles")),
    "probe": os.environ.get("PROBE"),
    "display": os.environ.get("DISPLAY"),
    "xauthority": os.environ.get("XAUTHORITY"),
    "most": most,
    "fixtures_most": fixtures_most,
    "started": started,
}
os.remove(mine)
record["ended"] = time.time()
with open(os.path.join(records, f"{os.getpid()}.json"), "w", encoding="utf-8") as out:
    json.dump(record, out)
sys.exit(3 if "Exits3" in name else 0)
'''

FAKE_XVFB_RUN = """#!/bin/sh
echo "$$ $2" >> "$XVFB_RECORD"
number="${2#--server-num=}"
DISPLAY=":$number"
XAUTHORITY="fake-xauthority-$number$XVFB_RUN_TAG"
export DISPLAY XAUTHORITY
shift 2
exec "$@"
"""

# Late enough that the runner looks for the slot file before this run has written it
SLOW_XVFB_RUN = FAKE_XVFB_RUN.replace('exec "$@"', 'sleep 0.3\nexec "$@"')

SILENT_XVFB_RUN = """#!/bin/sh
echo "$$ $2" >> "$XVFB_RECORD"
exec sleep 60
"""

FIXTURE_TEXT = 'local port = os.getenv("MUDLET_TEST_HTTP_PORT")\n'
SUMMARY_LINE = re.compile(r"^(\d+) success(?:es)? / (\d+) failures? / (\d+) errors? / (\d+) pending : \d+\.\d seconds$")


def alive(pid):
    try:
        os.kill(pid, 0)
    except ProcessLookupError:
        return False
    return True


def load_runner():
    spec = importlib.util.spec_from_file_location("run_lua_specs", RUNNER)
    runner = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(runner)
    return runner


class FakeGroup:
    """A process and its group: both go on SIGKILL, and on SIGTERM unless it ignores that."""

    def __init__(self, pid, ignores_sigterm):
        self.pid = pid
        self.ignores_sigterm = ignores_sigterm
        self.alive = True
        self.waited = False

    def poll(self):
        return None if self.alive else 0

    def wait(self):
        self.waited = True
        return 0


class FakeTree:
    """A Windows process and what it started: taskkill ends them unless they hold on, kill() always does."""

    def __init__(self, pid, running=True, holds_on=False):
        self.pid = pid
        self.alive = running
        self.holds_on = holds_on
        self.killed = False

    def poll(self):
        return None if self.alive else 0

    def wait(self, timeout=None):
        if self.alive:
            if timeout is None:
                raise AssertionError(f"waited without a time limit for {self.pid}, which is still running")
            raise subprocess.TimeoutExpired(str(self.pid), timeout)
        return 0

    def kill(self):
        self.alive = False
        self.killed = True


@unittest.skipIf(os.name == "nt", "the self-test is a POSIX program (shell script stubs, signals); CI runs it on Linux")
class RunLuaSpecs(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        # Registered first so it runs last, after the cleanups that read the records in it
        self.addCleanup(self.directory.cleanup)
        self.tests = os.path.join(self.directory.name, "tests")
        self.records = os.path.join(self.directory.name, "records")
        self.stub = os.path.join(self.directory.name, "stub.py")
        self.marker = os.path.join(self.directory.name, "busted-tests-failed")
        os.makedirs(self.tests)
        os.makedirs(self.records)
        with open(self.stub, "w", encoding="utf-8") as out:
            out.write(STUB)

    def spec(self, name, text='describe("a spec", function() end)\n'):
        path = os.path.join(self.tests, name)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "w", encoding="utf-8") as out:
            out.write(text)
        return path

    def specs(self, *names):
        return [self.spec(name) for name in names]

    def runner_arguments(self, options, command=True, tests_dir=True, xvfb_wait=None):
        if xvfb_wait is None:
            arguments = [sys.executable, "-I", RUNNER]
        else:
            arguments = [sys.executable, "-I", "-c", LOWERED_WAIT, RUNNER, str(xvfb_wait)]
        if tests_dir:
            arguments += ["--tests-dir", self.tests]
        arguments += list(options)
        if command:
            arguments += ["--", sys.executable, "-I", self.stub]
        return arguments

    def runner_environment(self, environment):
        env = {key: value for key, value in os.environ.items() if key not in ("GITHUB_ACTIONS", "PROBE", "STUB_BARRIER", "STUB_FIXTURE_HOLD", "STUB_RELEASE", "TESTS_DIRECTORY")}
        env["STUB_RECORD_DIR"] = self.records
        env.update(environment or {})
        return env

    def start_runner(self, *options, environment=None):
        process = subprocess.Popen(self.runner_arguments(options), stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, env=self.runner_environment(environment))
        self.addCleanup(self.end_quietly, process)
        return process

    def end_quietly(self, process):
        if process.poll() is None:
            process.kill()
        process.wait()
        process.stdout.close()

    def wait_for_running_stubs(self, count):
        running = os.path.join(self.records, "running")
        give_up = time.monotonic() + 10
        while time.monotonic() < give_up:
            pids = [int(name) for name in os.listdir(running)] if os.path.isdir(running) else []
            if len(pids) >= count:
                return pids
            time.sleep(0.02)
        self.fail(f"fewer than {count} stubs started")

    def survivors(self, pids):
        give_up = time.monotonic() + 5
        while time.monotonic() < give_up and any(alive(pid) for pid in pids):
            time.sleep(0.05)
        left = [pid for pid in pids if alive(pid)]
        for pid in left:
            os.kill(pid, signal.SIGKILL)
        return left

    def run_runner(self, *options, environment=None, command=True, tests_dir=True, xvfb_wait=None):
        arguments = self.runner_arguments(options, command, tests_dir, xvfb_wait)
        began = time.monotonic()
        result = subprocess.run(arguments, capture_output=True, env=self.runner_environment(environment), timeout=90)
        self.elapsed = time.monotonic() - began
        self.raw = result.stdout
        self.errors = result.stderr.decode("utf-8", "replace")
        return result.returncode, result.stdout.decode("utf-8", "replace")

    def stub_records(self):
        records = []
        for name in sorted(os.listdir(self.records)):
            if name.endswith(".json"):
                with open(os.path.join(self.records, name), encoding="utf-8") as record:
                    records.append(json.load(record))
        return sorted(records, key=lambda record: record["started"])

    def last_line(self, output):
        return output.rstrip("\n").splitlines()[-1]

    def fake_xvfb_run(self, script):
        folder = os.path.join(self.directory.name, "bin")
        os.makedirs(folder, exist_ok=True)
        path = os.path.join(folder, "xvfb-run")
        with open(path, "w", encoding="utf-8") as out:
            out.write(script)
        os.chmod(path, 0o700)
        self.xvfb_record = os.path.join(self.directory.name, "xvfb-run.txt")
        # A runner that fails to stop its servers would otherwise leave them running for good
        self.addCleanup(self.kill_recorded_servers)
        return {"PATH": folder + os.pathsep + os.environ.get("PATH", ""), "XVFB_RECORD": self.xvfb_record}

    def kill_recorded_servers(self):
        if os.path.exists(self.xvfb_record):
            with open(self.xvfb_record, encoding="utf-8") as calls:
                self.kill_alive([int(line.split()[0]) for line in calls])

    def kill_alive(self, pids):
        for pid in pids:
            if alive(pid):
                os.kill(pid, signal.SIGKILL)

    def fake_servers(self):
        with open(self.xvfb_record, encoding="utf-8") as calls:
            servers = [line.split() for line in calls.read().splitlines()]
        for pid, _ in servers:
            if alive(int(pid)):
                os.kill(int(pid), signal.SIGKILL)
                self.fail(f"the fake X server {pid} was left running")
        return sorted(argument for _, argument in servers)

    def test_runs_every_spec_file_once_in_a_process_of_its_own(self):
        expected = self.specs("Alpha_spec.lua", "Beta_spec.lua", "sub/Nested_spec.lua")
        self.specs("helper.lua", "Notes_spec.txt", ".hidden/Hidden_spec.lua", ".Dot_spec.lua")
        status, output = self.run_runner("--jobs", "2")
        self.assertEqual(status, 0, output)
        records = self.stub_records()
        self.assertEqual(sorted(record["spec"] for record in records), sorted(expected))
        self.assertEqual(len({record["pid"] for record in records}), len(expected))
        self.assertIn("3 spec files, 2 at a time; 0 of them talk to a fixture", output)

    def test_runs_at_most_jobs_processes_and_more_than_one_at_a_time(self):
        self.specs(*(f"Wait{number}_spec.lua" for number in range(1, 7)))
        status, output = self.run_runner("--jobs", "3", environment={"STUB_BARRIER": "3"})
        self.assertEqual(status, 0, output)
        self.assertTrue(os.path.exists(os.path.join(self.records, "released")), "three never ran at once")
        self.assertEqual(max(record["most"] for record in self.stub_records()), 3)

    def test_runs_the_fixture_specs_one_at_a_time_and_first(self):
        self.specs("Alpha_spec.lua", "Beta_spec.lua")
        for name in ("Fix1_spec.lua", "Fix2_spec.lua", "Fix3_spec.lua"):
            self.spec(name, FIXTURE_TEXT)
        status, output = self.run_runner("--jobs", "3", environment={"STUB_FIXTURE_HOLD": "0.4"})
        self.assertEqual(status, 0, output)
        records = self.stub_records()
        fixtures = [record for record in records if record["name"].startswith("Fix")]
        self.assertEqual(len(fixtures), 3)
        self.assertEqual([record["fixtures_most"] for record in fixtures], [1, 1, 1])
        self.assertIn("3 of them talk to a fixture and run one at a time", output)
        status, output = self.run_runner("--jobs", "1")
        self.assertEqual(status, 0, output)
        order = [record["name"] for record in self.stub_records()[len(records):]]
        self.assertEqual(order, ["Fix1_spec.lua", "Fix2_spec.lua", "Fix3_spec.lua", "Alpha_spec.lua", "Beta_spec.lua"])

    def test_gives_the_fixture_variables_to_fixture_specs_only(self):
        self.specs("Alpha_spec.lua", "Beta_spec.lua")
        self.spec("Fix_spec.lua", FIXTURE_TEXT)
        status, output = self.run_runner("--jobs", "2", "--fixture-env", "PROBE=x")
        self.assertEqual(status, 0, output)
        records = self.stub_records()
        self.assertEqual(len(records), 3)
        for record in records:
            self.assertEqual(record["probe"], "x" if record["name"] == "Fix_spec.lua" else None, record)

    def test_gives_every_spec_file_a_config_root_of_its_own(self):
        self.specs("Alpha_spec.lua", "Beta_spec.lua", "Gamma_spec.lua")
        work = os.path.join(self.directory.name, "work")
        status, output = self.run_runner("--jobs", "3", "--work-dir", work)
        self.assertEqual(status, 0, output)
        records = self.stub_records()
        self.assertEqual(len({record["config"] for record in records}), 3)
        for record in records:
            self.assertTrue(record["profiles"], record)
            self.assertFalse(os.path.exists(record["config"]), "a config root was left behind without --keep")
            self.assertTrue(os.path.exists(os.path.join(os.path.dirname(record["config"]), "output.log")))

        kept = os.path.join(self.directory.name, "kept")
        status, output = self.run_runner("--keep", "--work-dir", kept)
        self.assertEqual(status, 0, output)
        for record in self.stub_records()[3:]:
            self.assertTrue(os.path.isdir(os.path.join(record["config"], "mudlet", "profiles")), "--keep did not keep the config root")

        status, output = self.run_runner()
        self.assertEqual(status, 0, output)
        for record in self.stub_records()[6:]:
            self.assertFalse(os.path.exists(os.path.dirname(os.path.dirname(record["config"]))), "the temporary work directory was left behind")

    def test_passes_when_every_spec_file_passes(self):
        self.specs("Alpha_spec.lua", "Beta_spec.lua", "Gamma_spec.lua")
        status, output = self.run_runner("--jobs", "2", "--failure-marker", self.marker)
        self.assertEqual(status, 0, output)
        self.assertEqual(SUMMARY_LINE.match(self.last_line(output)).groups(), ("3", "0", "0", "0"), output)
        self.assertRegex(output, r"\n3 spec files: 3 passed, 0 failed; \d+\.\d s, \d+\.\d s summed over the processes\n")
        self.assertIn("The 3 slowest spec files:", output)
        self.assertNotIn("failed:", output)
        self.assertFalse(os.path.exists(self.marker))

    def test_uses_busted_wording_for_a_count_of_one(self):
        self.specs("Alpha_spec.lua")
        status, output = self.run_runner()
        self.assertEqual(status, 0, output)
        self.assertRegex(self.last_line(output), r"^1 success / 0 failures / 0 errors / 0 pending : \d+\.\d seconds$")

    def test_fails_each_spec_file_that_goes_wrong_and_names_it(self):
        self.specs("Alpha_spec.lua", "MarkFails_spec.lua", "Errors_spec.lua", "Exits3_spec.lua", "NoSummary_spec.lua")
        status, output = self.run_runner("--jobs", "3", "--failure-marker", self.marker)
        self.assertEqual(status, 1, output)
        self.assertIn("MarkFails_spec.lua failed: left its failure marker", output)
        self.assertIn("Errors_spec.lua failed: busted counted 0 failures and 1 error", output)
        self.assertNotIn("Errors_spec.lua failed: left its failure marker", output)
        self.assertIn("Exits3_spec.lua failed: exited with status 3", output)
        self.assertIn("NoSummary_spec.lua failed: its log has no busted summary line", output)
        self.assertNotIn("Alpha_spec.lua failed", output)
        self.assertIn("5 spec files: 1 passed, 4 failed;", output)
        self.assertRegex(self.last_line(output), r"^2 successes / 1 failure / 1 error / 0 pending : ")
        self.assertTrue(os.path.exists(self.marker))

    def test_stops_at_the_end_of_the_budget(self):
        self.specs("Sleeps_spec.lua", "Zulu_spec.lua")
        status, output = self.run_runner("--budget", "1")
        self.assertEqual(status, 1, output)
        self.assertLess(self.elapsed, 10)
        self.assertIn("Sleeps_spec.lua failed: still running after 1 s", output)
        self.assertIn("Zulu_spec.lua failed: not started: the budget ran out", output)
        self.assertEqual(self.stub_records(), [])

    def test_a_second_signal_kills_every_process_it_started(self):
        # The stub ignores SIGTERM, so the first signal leaves the runner waiting out its grace period
        self.specs("Stubborn_spec.lua")
        process = self.start_runner("--xvfb", environment=self.fake_xvfb_run(FAKE_XVFB_RUN))
        stubs = self.wait_for_running_stubs(1)
        self.addCleanup(self.kill_alive, stubs)
        process.send_signal(signal.SIGINT)
        time.sleep(0.3)
        process.send_signal(signal.SIGINT)
        self.assertNotEqual(process.wait(timeout=5), 0)
        with open(self.xvfb_record, encoding="utf-8") as calls:
            servers = [int(line.split()[0]) for line in calls]
        self.assertEqual(len(servers), 1)
        self.assertEqual(self.survivors(stubs + servers), [], "a process the runner started outlived it")

    def test_kills_only_the_groups_still_there_at_the_end_of_the_grace_period(self):
        runner = load_runner()
        runner.STOP_GRACE_SECONDS = 0.2
        groups = {101: FakeGroup(101, ignores_sigterm=False), 102: FakeGroup(102, ignores_sigterm=True)}
        sent = []

        def killpg(pid, signum):
            sent.append((pid, signum))
            group = groups[pid]
            if not group.alive:
                raise ProcessLookupError
            if signum == signal.SIGKILL or (signum == signal.SIGTERM and not group.ignores_sigterm):
                group.alive = False

        with mock.patch.object(runner.os, "killpg", killpg):
            runner.stop_groups(list(groups.values()))
        self.assertIn((102, signal.SIGKILL), sent)
        self.assertNotIn((101, signal.SIGKILL), sent)
        self.assertTrue(all(group.waited for group in groups.values()))

    def windows_seam(self, processes):
        runner = load_runner()
        runner.WINDOWS = True
        runner.STOP_GRACE_SECONDS = 0.2
        taskkills = []
        signalled = []
        by_pid = {process.pid: process for process in processes}

        def run(arguments, **options):
            taskkills.append(list(arguments))
            process = by_pid.get(int(arguments[-1]))
            if process and not process.holds_on:
                process.alive = False
            return subprocess.CompletedProcess(arguments, 0)

        for patch in (mock.patch.object(runner.subprocess, "run", run), mock.patch.object(runner.os, "killpg", lambda pid, signum: signalled.append((pid, signum)))):
            patch.start()
            self.addCleanup(patch.stop)
        return runner, taskkills, signalled

    def test_starts_a_windows_process_in_a_process_group_of_its_own(self):
        runner = load_runner()
        given = []

        def popen(command, **options):
            given.append(options)
            return FakeTree(4242)

        spec = runner.Spec(1, self.spec("Alpha_spec.lua"), "Alpha_spec.lua", False)
        work = os.path.join(self.directory.name, "work")
        with mock.patch.object(runner.subprocess, "Popen", popen):
            runner.WINDOWS = True
            runner.start(spec, 1, ["mudlet"], work, None, [])
            runner.WINDOWS = False
            runner.start(spec, 1, ["mudlet"], work, None, [])
        on_windows, on_posix = given
        self.assertTrue(on_windows["creationflags"] & 0x00000200, on_windows)
        self.assertNotIn("start_new_session", on_windows)
        self.assertIs(on_posix["start_new_session"], True)
        self.assertNotIn("creationflags", on_posix)

    def test_stops_a_windows_process_and_what_it_started_with_taskkill(self):
        running = FakeTree(301)
        ended = FakeTree(302, running=False)
        holding_on = FakeTree(303, holds_on=True)
        processes = [running, ended, holding_on]
        runner, taskkills, signalled = self.windows_seam(processes)
        runner.LIVE.update(processes)
        runner.stop_groups(processes)
        self.assertEqual(sorted(call[1:] for call in taskkills), [["/F", "/T", "/PID", "301"], ["/F", "/T", "/PID", "303"]])
        self.assertEqual(signalled, [])
        self.assertTrue(holding_on.killed, "a process taskkill did not end was not killed after the grace period")
        self.assertFalse(running.killed)
        self.assertEqual(runner.LIVE, set())

    def test_runs_taskkill_from_system_root_when_it_is_there(self):
        runner, taskkills, _ = self.windows_seam([])
        root = os.path.join(self.directory.name, "Windows")
        os.makedirs(os.path.join(root, "System32"))
        open(os.path.join(root, "System32", "taskkill.exe"), "w").close()
        with mock.patch.dict(os.environ, {"SystemRoot": root}):
            runner.taskkill(FakeTree(401))
        with mock.patch.dict(os.environ, {"SystemRoot": os.path.join(self.directory.name, "nowhere")}):
            runner.taskkill(FakeTree(402))
        self.assertEqual([call[0] for call in taskkills], [os.path.join(root, "System32", "taskkill.exe"), "taskkill"])

    def test_a_second_signal_on_windows_taskkills_every_running_process(self):
        running = FakeTree(501)
        ended = FakeTree(502, running=False)
        runner, taskkills, signalled = self.windows_seam([running, ended])
        runner.LIVE.update([running, ended])
        runner.kill_all_started()
        self.assertEqual([call[1:] for call in taskkills], [["/F", "/T", "/PID", "501"]])
        self.assertEqual(signalled, [])

    def test_names_the_folder_by_index_alone_on_windows(self):
        runner = load_runner()
        work = os.path.join(self.directory.name, "work")
        spec = runner.Spec(37, self.spec("Alpha_spec.lua"), "Alpha_spec.lua", False)
        names = []
        for windows in (True, False):
            runner.WINDOWS = windows
            runner.prepare(spec, work)
            names.append(os.path.basename(spec.folder))
            self.assertTrue(os.path.isdir(os.path.join(spec.folder, "config", "mudlet", "profiles")))
        self.assertEqual(names, ["037", "037-Alpha_spec"])

    def test_prints_the_logs_asked_for(self):
        self.specs("Alpha_spec.lua", "MarkFails_spec.lua")
        shown = {}
        for which in ("all", "failed", "none"):
            status, output = self.run_runner("--logs", which)
            self.assertEqual(status, 1, output)
            shown[which] = ["output of Alpha_spec.lua" in output, "output of MarkFails_spec.lua" in output]
        self.assertEqual(shown, {"all": [True, True], "failed": [False, True], "none": [False, False]})
        self.assertIn("=== Lua specs: MarkFails_spec.lua (", self.run_runner("--logs", "all")[1])
        self.assertIn(b"noise \xff\xfe that is not UTF-8", self.raw)

        status, output = self.run_runner()
        self.assertEqual(["output of Alpha_spec.lua" in output, "output of MarkFails_spec.lua" in output], [False, True])
        status, output = self.run_runner(environment={"GITHUB_ACTIONS": "true"})
        self.assertEqual(["output of Alpha_spec.lua" in output, "output of MarkFails_spec.lua" in output], [True, True])
        self.assertRegex(output, r"::group::Lua specs: Alpha_spec\.lua \(\d+\.\d s, passed\)\n")
        self.assertRegex(output, r"::group::Lua specs: MarkFails_spec\.lua \(\d+\.\d s, FAILED\)\n")
        self.assertIn("::endgroup::", output)
        self.assertIn("::error::Lua specs, MarkFails_spec.lua: left its failure marker", output)

    def test_prints_each_log_as_its_spec_file_ends(self):
        self.specs("Holds_spec.lua", "Quick_spec.lua")
        release = os.path.join(self.records, "release")
        process = self.start_runner("--jobs", "2", "--logs", "all", environment={"STUB_RELEASE": release})
        lines = queue.Queue()
        threading.Thread(target=lambda: [lines.put(line) for line in process.stdout], daemon=True).start()
        seen = []
        give_up = time.monotonic() + 8
        try:
            while time.monotonic() < give_up:
                try:
                    seen.append(lines.get(timeout=max(0.01, give_up - time.monotonic())))
                except queue.Empty:
                    break
                if seen[-1].startswith(b"=== end of Quick_spec.lua ==="):
                    break
        finally:
            # Holds_spec.lua runs until this file exists
            open(release, "w").close()
        self.assertEqual(process.wait(timeout=30), 0)
        self.assertTrue(seen and seen[-1].startswith(b"=== end of Quick_spec.lua ==="), "Quick_spec.lua's log did not come while Holds_spec.lua still ran")
        headers = [index for index, line in enumerate(seen) if line.startswith(b"=== Lua specs: Quick_spec.lua (")]
        self.assertEqual(len(headers), 1, seen)
        self.assertLess(headers[0], seen.index(b"output of Quick_spec.lua\n"))

    def test_gives_every_job_slot_an_x_server_of_its_own(self):
        self.specs(*(f"Spec{number}_spec.lua" for number in range(1, 7)))
        status, output = self.run_runner("--jobs", "2", "--xvfb", environment=self.fake_xvfb_run(FAKE_XVFB_RUN))
        self.assertEqual(status, 0, output + self.errors)
        self.assertEqual(self.fake_servers(), ["--server-num=210", "--server-num=220"])
        records = self.stub_records()
        self.assertEqual(len(records), 6)
        self.assertEqual({record["display"] for record in records}, {":210", ":220"})
        for record in records:
            self.assertEqual(record["xauthority"], "fake-xauthority-" + record["display"][1:])
        for one in records:
            for other in records:
                if one is not other and one["started"] < other["ended"] and other["started"] < one["ended"]:
                    self.assertNotEqual(one["display"], other["display"], "two processes shared a display at once")

    def test_takes_the_lowest_free_job_slot(self):
        self.specs("Alpha_spec.lua")
        status, output = self.run_runner("--jobs", "3", "--xvfb", environment=self.fake_xvfb_run(FAKE_XVFB_RUN))
        self.assertEqual(status, 0, output + self.errors)
        self.assertEqual(len(self.fake_servers()), 3)
        self.assertEqual([record["display"] for record in self.stub_records()], [":210"])

    def test_a_work_directory_used_before_gets_the_new_x_server(self):
        self.specs("Alpha_spec.lua")
        work = os.path.join(self.directory.name, "work")
        environment = self.fake_xvfb_run(SLOW_XVFB_RUN)
        for tag in ("-first", "-second"):
            status, output = self.run_runner("--jobs", "1", "--xvfb", "--work-dir", work, environment=dict(environment, XVFB_RUN_TAG=tag))
            self.assertEqual(status, 0, output + self.errors)
            self.assertEqual(self.stub_records()[-1]["xauthority"], "fake-xauthority-210" + tag)
        self.fake_servers()

    def test_gives_up_quickly_when_an_x_server_does_not_come_up(self):
        self.specs("Alpha_spec.lua")
        status, output = self.run_runner("--jobs", "2", "--xvfb", environment=self.fake_xvfb_run(SILENT_XVFB_RUN), xvfb_wait=1)
        self.assertEqual(status, 2, output + self.errors)
        self.assertLess(self.elapsed, 10)
        self.assertIn("the X server of job slot 1 did not come up", self.errors)
        self.assertEqual(self.fake_servers(), ["--server-num=210", "--server-num=220"])
        self.assertEqual(self.stub_records(), [])

    def test_list_starts_nothing(self):
        self.specs("Alpha_spec.lua", "Beta_spec.lua")
        self.spec("Fix_spec.lua", FIXTURE_TEXT)
        status, output = self.run_runner("--list", "--jobs", "2")
        self.assertEqual(status, 0, output)
        self.assertEqual(output.splitlines()[1:], ["  Alpha_spec.lua", "  Beta_spec.lua", "  Fix_spec.lua  (fixture)"])
        self.assertEqual(os.listdir(self.records), [])

    def test_takes_the_folder_from_tests_directory_only_when_it_is_one(self):
        alpha = self.spec("Alpha_spec.lua")
        status, output = self.run_runner("--list", tests_dir=False, environment={"TESTS_DIRECTORY": self.tests})
        self.assertEqual((status, output.splitlines()[1:]), (0, ["  Alpha_spec.lua"]))
        status, output = self.run_runner("--list", tests_dir=False, environment={"TESTS_DIRECTORY": alpha})
        self.assertNotIn("  Alpha_spec.lua", output)
        # A copy of the runner outside the repository finds no src/mudlet-lua/tests beside it
        if status != 0:
            self.assertEqual(status, 2, output)
            self.assertIn(os.path.join("src", "mudlet-lua", "tests"), self.errors)

    def test_refuses_to_run_without_spec_files(self):
        self.spec("helper.lua")
        status, output = self.run_runner()
        self.assertEqual(status, 2, output)
        self.assertIn("no *_spec*.lua files", self.errors)
        self.assertEqual(os.listdir(self.records), [])

    def test_refuses_other_setup_problems(self):
        self.specs("Alpha_spec.lua")
        for options, command, environment, said in (
            ((), False, None, "no Mudlet command"),
            (("--jobs", "0"), True, None, "--jobs"),
            (("--budget", "0"), True, None, "--budget"),
            (("--xvfb",), True, {"PATH": self.records}, "xvfb-run is not on PATH"),
        ):
            status, _ = self.run_runner(*options, command=command, environment=environment)
            self.assertEqual(status, 2, said)
            self.assertIn(said, self.errors)
        self.assertEqual(os.listdir(self.records), [])


if __name__ == "__main__":
    unittest.main()
