#!/usr/bin/env python3
"""Runs every busted Lua spec file in a Mudlet process of its own, several at a time.

Spec files are not independent: one can pass only because an earlier one left a
window open or a global set. Run alone in a fresh process each, with a config
root of its own, nothing one spec file leaves behind can reach another, and the
run passes only when every spec file passes on its own. A spec file is handed
to Mudlet as TESTS_DIRECTORY, which the run-tests package accepts as a file.

The spec files that talk to a CI fixture run one at a time, because each fixture
serves one client, and only they get the --fixture-env variables.

Usage: run-lua-specs.py [options] -- <mudlet command> [arguments...]

  --jobs N               Mudlet processes running at a time (default 1)
  --tests-dir DIR        folder with the *_spec.lua files (default: $TESTS_DIRECTORY when it
                         names a folder, else src/mudlet-lua/tests in this repository)
  --work-dir DIR         where each spec file's config root, log and failure marker go; a
                         folder named here is never deleted (default: a fresh temporary
                         directory, deleted at the end unless --keep)
  --keep                 keep every config root, and the temporary work directory
  --budget SECONDS       stop processes still running after this long (default 900)
  --xvfb                 give every job slot an X server of its own
  --fixture-env K=V      variable given only to the spec files that talk to a fixture
                         (repeatable)
  --failure-marker FILE  create this file when the run fails
  --logs WHICH           whose output to print as each spec file ends: all, failed or none
                         (default: all on GitHub Actions, failed elsewhere)
  --list                 print the spec files in order, fixture specs marked; start nothing

Each spec file's output is printed as soon as it has ended, so a runner killed
part way still leaves what ran. The slowest spec files, every failure with its
reasons and the totals follow at the end, and the last line printed is always a
summary in busted's own wording. Exits 0 when every spec file passed, 1 when any
failed, and 2 on a setup problem. A second SIGINT or SIGTERM kills every process
the runner started and ends it at once.
"""

import argparse
import collections
import os
import re
import shutil
import signal
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))

# Each of these fixtures serves one client: the HTTP server is single-threaded, the
# telnet fixture and the MMCP peer keep one connection and one capture, and the fake
# Discord puts every client's frames into one capture file
FIXTURE_VARIABLES = ("MUDLET_TEST_HTTP_PORT", "MUDLET_TEST_TELNET_DIR", "MUDLET_TEST_MMCP_DIR", "MUDLET_TEST_DISCORD")

SUMMARY = re.compile(rb"(\d+) success(?:es)? / (\d+) failures? / (\d+) errors? / (\d+) pending")

PROGRESS_SECONDS = 30
STOP_GRACE_SECONDS = 10
XVFB_READY_SECONDS = 30
SLOWEST_SHOWN = 15

# Prints DISPLAY and XAUTHORITY to the slot file, then holds the server open until it is stopped
XVFB_HOLDER = 'printf "%s\\n%s\\n" "$DISPLAY" "$XAUTHORITY" > "$1"; exec sleep 2147483647'

# Every process started and not yet seen to end with its group: each runs in a session of its
# own, so if the runner exits without stopping them, nothing else will
LIVE = set()


class SetupProblem(Exception):
    pass


class Spec:
    def __init__(self, index, path, name, fixture):
        self.index = index
        self.path = path
        self.name = name
        self.fixture = fixture
        self.folder = None
        self.process = None
        self.slot = None
        self.started = None
        self.ended = None
        self.stopped = None
        self.not_started = None
        self.start_error = None
        self.returncode = None
        self.summary = None
        self.reasons = []

    def file(self, name):
        return os.path.join(self.folder, name)

    def seconds(self):
        if self.started is None or self.ended is None:
            return 0.0
        return self.ended - self.started

    def running(self):
        return self.process is not None and self.returncode is None


class XServer:
    def __init__(self, slot, process, slot_file):
        self.slot = slot
        self.process = process
        self.slot_file = slot_file
        self.display = None
        self.xauthority = None


def plural(count, one, many):
    return one if count == 1 else many


def counted(count, one, many):
    return f"{count} {plural(count, one, many)}"


def parse_arguments(argv):
    if "--" in argv:
        split = argv.index("--")
        options, command = argv[:split], argv[split + 1:]
    else:
        options, command = argv, []

    def assignment(text):
        key, equals, _ = text.partition("=")
        if not equals or not key:
            raise argparse.ArgumentTypeError(f"expected K=V, got '{text}'")
        return text

    parser = argparse.ArgumentParser(
        prog="run-lua-specs.py",
        usage="%(prog)s [options] -- <mudlet command> [arguments...]",
        description="Runs every busted Lua spec file in a Mudlet process of its own, several at a time.",
    )
    parser.add_argument("--jobs", type=int, default=1, metavar="N", help="Mudlet processes running at a time (default 1)")
    parser.add_argument("--tests-dir", metavar="DIR", help="folder with the *_spec.lua files (default: $TESTS_DIRECTORY when it names a folder, else src/mudlet-lua/tests in this repository)")
    parser.add_argument("--work-dir", metavar="DIR", help="where each spec file's config root, log and failure marker go; a folder named here is never deleted (default: a fresh temporary directory, deleted at the end unless --keep)")
    parser.add_argument("--keep", action="store_true", help="keep every config root, and the temporary work directory")
    parser.add_argument("--budget", type=float, default=900, metavar="SECONDS", help="stop processes still running after this long (default 900)")
    parser.add_argument("--xvfb", action="store_true", help="give every job slot an X server of its own")
    parser.add_argument("--fixture-env", type=assignment, action="append", default=[], metavar="K=V", help="variable given only to the spec files that talk to a fixture (repeatable)")
    parser.add_argument("--failure-marker", metavar="FILE", help="create this file when the run fails")
    parser.add_argument("--logs", choices=("all", "failed", "none"), help="whose output to print as each spec file ends (default: all on GitHub Actions, failed elsewhere)")
    parser.add_argument("--list", action="store_true", help="print the spec files in order, fixture specs marked; start nothing")
    arguments = parser.parse_args(options)
    arguments.command = command
    return arguments


def default_tests_dir():
    configured = os.environ.get("TESTS_DIRECTORY")
    if configured and os.path.isdir(configured):
        return configured
    return os.path.join(os.path.dirname(HERE), "src", "mudlet-lua", "tests")


def find_specs(tests_dir):
    paths = []
    for root, folders, names in os.walk(tests_dir):
        folders[:] = [folder for folder in folders if not folder.startswith(".")]
        for name in names:
            if not name.startswith(".") and "_spec" in name and name.endswith(".lua"):
                paths.append(os.path.join(root, name))
    specs = []
    for index, path in enumerate(sorted(paths), 1):
        with open(path, "rb") as source:
            text = source.read()
        fixture = any(variable.encode() in text for variable in FIXTURE_VARIABLES)
        specs.append(Spec(index, path, os.path.relpath(path, tests_dir), fixture))
    return specs


def check_setup(arguments):
    if arguments.jobs < 1:
        raise SetupProblem(f"--jobs must be 1 or more, not {arguments.jobs}")
    if arguments.budget <= 0:
        raise SetupProblem(f"--budget must be more than 0 seconds, not {arguments.budget:g}")
    if not arguments.list:
        if not arguments.command:
            raise SetupProblem("no Mudlet command after --")
        if arguments.xvfb and not shutil.which("xvfb-run"):
            raise SetupProblem("--xvfb was given, but xvfb-run is not on PATH")
    tests_dir = os.path.abspath(arguments.tests_dir or default_tests_dir())
    specs = find_specs(tests_dir)
    if not specs:
        raise SetupProblem(f"no *_spec*.lua files under {tests_dir}")
    return tests_dir, specs


def signal_group(process, signum):
    try:
        os.killpg(process.pid, signum)
    except (ProcessLookupError, PermissionError):
        pass


def group_alive(process):
    try:
        os.killpg(process.pid, 0)
    except (ProcessLookupError, PermissionError):
        return False
    return True


def stop_groups(processes):
    for process in processes:
        signal_group(process, signal.SIGTERM)
    give_up = time.monotonic() + STOP_GRACE_SECONDS
    # The whole group: Xvfb, or a process Mudlet started, can outlive the leader and
    # still be removing its lock file
    while time.monotonic() < give_up and any(process.poll() is None or group_alive(process) for process in processes):
        time.sleep(0.05)
    for process in processes:
        # Only a group still there: the id of one that went away may belong to another by now
        if group_alive(process):
            signal_group(process, signal.SIGKILL)
        process.wait()
        LIVE.discard(process)


def start_x_servers(jobs, work_dir, interrupted):
    servers = []
    for slot in range(1, jobs + 1):
        slot_file = os.path.join(work_dir, f"slot-{slot}.display")
        # A --work-dir used before still holds the file of a server that has stopped
        try:
            os.remove(slot_file)
        except FileNotFoundError:
            pass
        with open(os.path.join(work_dir, f"slot-{slot}.log"), "wb") as log:
            # A server number of its own for each slot, so the xvfb-run calls do not race for one
            command = ["xvfb-run", "--auto-servernum", f"--server-num={200 + 10 * slot}", "sh", "-c", XVFB_HOLDER, "sh", slot_file]
            try:
                process = subprocess.Popen(command, stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
            except OSError as error:
                stop_groups([server.process for server in servers])
                raise SetupProblem(f"the X server of job slot {slot} did not come up: {error}")
        LIVE.add(process)
        servers.append(XServer(slot, process, slot_file))

    give_up = time.monotonic() + XVFB_READY_SECONDS
    waiting = list(servers)
    while waiting and not interrupted:
        for server in list(waiting):
            try:
                with open(server.slot_file, encoding="utf-8") as lines:
                    text = lines.read()
            except OSError:
                text = ""
            if text.count("\n") >= 2:
                server.display, server.xauthority = text.split("\n")[:2]
                waiting.remove(server)
        if not waiting:
            break
        ended = [server for server in waiting if server.process.poll() is not None]
        if ended or time.monotonic() >= give_up:
            stop_groups([server.process for server in servers])
            server = (ended or waiting)[0]
            try:
                with open(os.path.join(work_dir, f"slot-{server.slot}.log"), encoding="utf-8", errors="replace") as log:
                    said = log.read().strip()
            except OSError:
                said = ""
            why = f"ended with status {server.process.returncode}" if ended else f"was not ready after {XVFB_READY_SECONDS:g} s"
            raise SetupProblem(f"the X server of job slot {server.slot} did not come up: xvfb-run {why}" + (f"; it said: {said}" if said else ""))
        time.sleep(0.05)
    return servers


def prepare(spec, work_dir):
    spec.folder = os.path.join(work_dir, f"{spec.index:03d}-{os.path.basename(spec.path)[:-len('.lua')]}")
    if os.path.lexists(spec.folder):
        shutil.rmtree(spec.folder)
    # Mudlet only adopts $XDG_CONFIG_HOME/mudlet when it already holds a profiles folder
    os.makedirs(spec.file(os.path.join("config", "mudlet", "profiles")))
    os.makedirs(spec.file("cache"))
    os.makedirs(spec.file("home"))


def environment(spec, server, fixture_env):
    env = dict(os.environ)
    env["XDG_CONFIG_HOME"] = spec.file("config")
    env["XDG_CACHE_HOME"] = spec.file("cache")
    if sys.platform == "darwin":
        # Moves the native settings and caches into the spec file's folder, as the ctest sandbox does
        env["CFFIXED_USER_HOME"] = spec.file("home")
    env["TESTS_DIRECTORY"] = spec.path
    env["MUDLET_TEST_FAILURE_MARKER"] = spec.file("failed")
    for key, value in (("AUTORUN_BUSTED_TESTS", "true"), ("QUIT_MUDLET_AFTER_TESTS", "true"), ("MUDLET_TEST_MODE", "1")):
        env.setdefault(key, value)
    if server:
        env["DISPLAY"] = server.display
        env["XAUTHORITY"] = server.xauthority
    if spec.fixture:
        for pair in fixture_env:
            key, _, value = pair.partition("=")
            env[key] = value
    return env


def start(spec, slot, command, work_dir, server, fixture_env):
    spec.slot = slot
    prepare(spec, work_dir)
    spec.started = time.monotonic()
    with open(spec.file("output.log"), "wb") as log:
        try:
            spec.process = subprocess.Popen(
                command,
                env=environment(spec, server, fixture_env),
                stdin=subprocess.DEVNULL,
                stdout=log,
                stderr=subprocess.STDOUT,
                start_new_session=True,
            )
            LIVE.add(spec.process)
        except OSError as error:
            spec.start_error = str(error)
            spec.ended = time.monotonic()


def judge(spec, keep):
    reasons = spec.reasons
    if spec.not_started:
        reasons.append(spec.not_started)
        return
    if spec.start_error:
        reasons.append(f"could not be started: {spec.start_error}")
    elif spec.stopped:
        reasons.append(spec.stopped)
    elif spec.returncode < 0:
        try:
            name = signal.Signals(-spec.returncode).name
        except ValueError:
            name = f"signal {-spec.returncode}"
        reasons.append(f"was killed by {name}")
    elif spec.returncode != 0:
        reasons.append(f"exited with status {spec.returncode}")

    if os.path.exists(spec.file("failed")):
        reasons.append("left its failure marker, so busted reported a failure or an error")

    try:
        with open(spec.file("output.log"), "rb") as log:
            matches = SUMMARY.findall(log.read())
    except OSError:
        matches = []
    if matches:
        spec.summary = tuple(int(count) for count in matches[-1])
        _, failures, errors, _ = spec.summary
        if failures or errors:
            reasons.append(f"busted counted {counted(failures, 'failure', 'failures')} and {counted(errors, 'error', 'errors')}")
    else:
        reasons.append("its log has no busted summary line, so it crashed, hung or never got to its specs")

    if not keep:
        for folder in ("config", "cache", "home"):
            shutil.rmtree(spec.file(folder), ignore_errors=True)


def schedule(specs, arguments, work_dir, servers, interrupted, finish):
    fixtures = collections.deque(spec for spec in specs if spec.fixture)
    others = collections.deque(spec for spec in specs if not spec.fixture)
    running = {}
    done = 0
    began = time.monotonic()
    deadline = began + arguments.budget
    next_progress = began + PROGRESS_SECONDS

    def leave_unstarted(why):
        for spec in list(fixtures) + list(others):
            spec.not_started = why
            finish(spec)
        fixtures.clear()
        others.clear()

    def stop_running(why):
        stopping = list(running.values())
        for spec in stopping:
            spec.stopped = why
        stop_groups([spec.process for spec in stopping])
        for spec in stopping:
            spec.returncode = spec.process.returncode
            spec.ended = time.monotonic()
            finish(spec)
        running.clear()

    while fixtures or others or running:
        for slot, spec in list(running.items()):
            code = spec.process.poll()
            if code is not None:
                LIVE.discard(spec.process)
                spec.returncode = code
                spec.ended = time.monotonic()
                finish(spec)
                del running[slot]
                done += 1

        if interrupted:
            stop_running(f"stopped when the runner got {interrupted[0]}")
            leave_unstarted("not started: the runner was stopped")
            break
        now = time.monotonic()
        if now >= deadline:
            stop_running(f"still running after {arguments.budget:g} s, when the budget ran out, so it was stopped")
            leave_unstarted("not started: the budget ran out")
            break

        while len(running) < arguments.jobs:
            if fixtures and not any(spec.fixture for spec in running.values()):
                spec = fixtures.popleft()
            elif others:
                spec = others.popleft()
            else:
                break
            slot = min(set(range(1, arguments.jobs + 1)) - set(running))
            start(spec, slot, arguments.command, work_dir, servers.get(slot), arguments.fixture_env)
            if spec.start_error:
                finish(spec)
                done += 1
            else:
                running[slot] = spec

        if now >= next_progress:
            busy = ", ".join(f"{spec.name} ({time.monotonic() - spec.started:.0f} s)" for _, spec in sorted(running.items()))
            print(f"{done} of {len(specs)} done; running: {busy or 'nothing'}", flush=True)
            next_progress += PROGRESS_SECONDS
        time.sleep(0.05)
    return time.monotonic() - began


def show_log(spec, which, on_github):
    if spec.folder is None or which == "none" or (which == "failed" and not spec.reasons):
        return
    title = f"Lua specs: {spec.name} ({spec.seconds():.1f} s, {'FAILED' if spec.reasons else 'passed'})"
    print(f"::group::{title}" if on_github else f"=== {title} ===", flush=True)
    try:
        with open(spec.file("output.log"), "rb") as log:
            text = log.read()
    except OSError:
        text = b""
    if text and not text.endswith(b"\n"):
        text += b"\n"
    sys.stdout.buffer.write(text)
    sys.stdout.buffer.flush()
    print("::endgroup::" if on_github else f"=== end of {spec.name} ===", flush=True)


def show_slowest(specs):
    ran = sorted((spec for spec in specs if spec.started is not None and spec.ended is not None), key=lambda spec: (-spec.seconds(), spec.name))
    if not ran:
        return
    shown = min(SLOWEST_SHOWN, len(ran))
    print("The slowest spec file:" if shown == 1 else f"The {shown} slowest spec files:")
    for spec in ran[:SLOWEST_SHOWN]:
        print(f"  {spec.seconds():8.1f} s  {spec.name}")


def summary_line(specs, wall):
    totals = [0, 0, 0, 0]
    for spec in specs:
        if spec.summary:
            totals = [total + count for total, count in zip(totals, spec.summary)]
    successes, failures, errors, pending = totals
    return (f"{successes} {plural(successes, 'success', 'successes')} / {failures} {plural(failures, 'failure', 'failures')} / "
            f"{errors} {plural(errors, 'error', 'errors')} / {pending} pending : {wall:.1f} seconds")


def run(arguments, specs, work_dir):
    interrupted = []

    def note(signum, frame):
        if interrupted:
            for process in list(LIVE):
                if group_alive(process):
                    signal_group(process, signal.SIGKILL)
            os.write(2, f"run-lua-specs.py: got {signal.Signals(signum).name} again, so every process it started was killed\n".encode())
            os._exit(128 + signum)
        interrupted.append(signal.Signals(signum).name)

    signal.signal(signal.SIGINT, note)
    signal.signal(signal.SIGTERM, note)

    on_github = os.environ.get("GITHUB_ACTIONS") == "true"
    which = arguments.logs or ("all" if on_github else "failed")

    def finish(spec):
        judge(spec, arguments.keep)
        show_log(spec, which, on_github)

    servers = []
    try:
        if arguments.xvfb:
            servers = start_x_servers(arguments.jobs, work_dir, interrupted)
        wall = schedule(specs, arguments, work_dir, {server.slot: server for server in servers}, interrupted, finish)
    finally:
        stop_groups([spec.process for spec in specs if spec.running()] + [server.process for server in servers])

    show_slowest(specs)
    failed = [spec for spec in specs if spec.reasons]
    for spec in failed:
        for reason in spec.reasons:
            print(f"::error::Lua specs, {spec.name}: {reason}" if on_github else f"{spec.name} failed: {reason}")
    summed = sum(spec.seconds() for spec in specs)
    print(f"{counted(len(specs), 'spec file', 'spec files')}: {len(specs) - len(failed)} passed, {len(failed)} failed; {wall:.1f} s, {summed:.1f} s summed over the processes")

    if failed and arguments.failure_marker:
        try:
            with open(arguments.failure_marker, "w", encoding="utf-8") as marker:
                marker.write("Lua busted tests failed\n")
        except OSError as error:
            print(f"Could not write the failure marker {arguments.failure_marker}: {error}", file=sys.stderr)
    print(summary_line(specs, wall), flush=True)
    return 1 if failed else 0


def main(argv):
    arguments = parse_arguments(argv)
    try:
        tests_dir, specs = check_setup(arguments)
    except SetupProblem as problem:
        print(f"run-lua-specs.py: {problem}", file=sys.stderr)
        return 2

    fixtures = sum(1 for spec in specs if spec.fixture)
    print(f"{counted(len(specs), 'spec file', 'spec files')}, {arguments.jobs} at a time; {fixtures} of them talk to a fixture and run one at a time")
    if arguments.list:
        for spec in specs:
            print(f"  {spec.name}{'  (fixture)' if spec.fixture else ''}")
        return 0

    keep = arguments.keep or arguments.work_dir is not None
    if arguments.work_dir:
        work_dir = os.path.abspath(arguments.work_dir)
        os.makedirs(work_dir, exist_ok=True)
    else:
        work_dir = tempfile.mkdtemp(prefix="mudlet-lua-specs-")
    if keep:
        print(f"work directory: {work_dir}")
    sys.stdout.flush()
    try:
        return run(arguments, specs, work_dir)
    except SetupProblem as problem:
        print(f"run-lua-specs.py: {problem}", file=sys.stderr)
        return 2
    finally:
        if not keep:
            shutil.rmtree(work_dir, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
