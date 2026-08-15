#!/bin/bash
# Tests test/lsan-tracer-retry.cmake, the launcher that reruns a leak-checked
# test when LeakSanitizer's tracer crashed on the way out (#9809).
#
# The flake it exists for is intermittent and CI-only, so the launcher is driven
# here against a stub command instead: the stub prints a log and exits non-zero,
# and each case asserts both the exit status and how many times the stub ran.
#
# One case per guard in the matcher. Rerunning a run that really failed would
# hide that failure, so all but three of them are about NOT rerunning - remove
# any single condition from tracer_crashed_after_a_clean_run and at least one
# case below goes red.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
LAUNCHER="${REPO_DIR}/test/lsan-tracer-retry.cmake"

WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

STUB="${WORK_DIR}/stub"
ATTEMPTS="${WORK_DIR}/attempts"
OUT="${WORK_DIR}/out"
FAILURES=0

# Every rerun below would otherwise append a line naming a stub to the real job
# summary, which is the one place the genuine flake rate is readable
SUMMARY="${WORK_DIR}/step-summary.md"
: > "${SUMMARY}"
export GITHUB_STEP_SUMMARY="${SUMMARY}"

# run-tests.xml writes here in preference to the path the workflow passes the
# launcher, so a value leaking in from the environment would change what the
# marker cases are testing
unset MUDLET_TEST_FAILURE_MARKER

start_test() {
  echo "=== $1"
}

fail() {
  echo "    FAIL: $*" >&2
  FAILURES=$((FAILURES + 1))
}

# Verbatim from job 93585398259 of run 31428353403, the failure #9809 was filed
# for, with the QDEBUG/QWARN lines dropped: TelnetTextDisplayedTest reported
# itself green and only then did LeakSanitizer die, with no leak report at all
cat > "${WORK_DIR}/qtest-passed.out" <<'EOF'
********* Start testing of TelnetTextDisplayedTest *********
Config: Using QtTest library 6.9.0, Qt 6.9.0 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 10.3.1 20210422 (Red Hat 10.3.1-1)), ubuntu 22.04
PASS   : TelnetTextDisplayedTest::initTestCase()
PASS   : TelnetTextDisplayedTest::test_TelnetTextDisplayed()
PASS   : TelnetTextDisplayedTest::test_MalformedEntityKeepsNonAsciiBytes()
PASS   : TelnetTextDisplayedTest::test_CustomEntityKeepsNonAsciiValue()
PASS   : TelnetTextDisplayedTest::cleanupTestCase()
Totals: 5 passed, 0 failed, 0 skipped, 0 blacklisted, 4773ms
********* Finished testing of TelnetTextDisplayedTest *********
EOF

# The leak-checked legs run with LSAN_OPTIONS=log_threads=1, so the tracer names
# each thread's ranges on its way through and the crash lands mid-list - the last
# Stack line is the range it died on. Kept in the fixture because that traffic is
# what the matcher now has to read past.
cat > "${WORK_DIR}/tracer-crash.err" <<'EOF'
==17930==Processing thread 17923.
==17930==Stack at 0x7ffd1e4a2000-0x7ffd1eca2000 (SP = 0x7ffd1ec9de10).
==17930==TLS at 0x7fbee8a1bb00-0x7fbee8a1cdc0.
==17930==Processing thread 17926.
==17930==Stack at 0x29800000-0x29ffed40 (SP = 0x29ffe9f0).
Tracer caught signal 11: addr=0x29800018 pc=0x7fbee80e17e8 sp=0x7fbedd3ffd20
==17923==LeakSanitizer has encountered a fatal error.
==17923==HINT: For debugging, try setting environment variable LSAN_OPTIONS=verbosity=1:log_threads=1
==17923==HINT: LeakSanitizer does not work under ptrace (strace, gdb, etc)
EOF

cat > "${WORK_DIR}/leak-report.err" <<'EOF'

=================================================================
==18044==ERROR: LeakSanitizer: detected memory leaks

Direct leak of 1032 byte(s) in 1 object(s) allocated from:
    #0 0x7f4c9e367357 in operator new(unsigned long)
    #1 0x55d3f1a2b4c1 in TConsole::TConsole(Host*, TConsole::ConsoleType, QWidget*)

SUMMARY: AddressSanitizer: 1032 byte(s) leaked in 1 allocation(s).
EOF
cat "${WORK_DIR}/leak-report.err" "${WORK_DIR}/tracer-crash.err" > "${WORK_DIR}/leak-and-tracer.err"

# The same run with one slot failing, and with it cut short before QTest got to
# print any verdict at all
cat > "${WORK_DIR}/qtest-failed.out" <<'EOF'
********* Start testing of TelnetTextDisplayedTest *********
PASS   : TelnetTextDisplayedTest::initTestCase()
FAIL!  : TelnetTextDisplayedTest::test_TelnetTextDisplayed() Compared values are not the same
   Actual   (displayedText): "hello"
   Expected (expectedText) : "world"
   Loc: [/home/runner/work/Mudlet/Mudlet/test/functional_tests/TelnetTextDisplayedTest.cpp(78)]
PASS   : TelnetTextDisplayedTest::cleanupTestCase()
Totals: 4 passed, 1 failed, 0 skipped, 0 blacklisted, 4692ms
********* Finished testing of TelnetTextDisplayedTest *********
EOF

cat > "${WORK_DIR}/qtest-no-verdict.out" <<'EOF'
********* Start testing of TelnetTextDisplayedTest *********
PASS   : TelnetTextDisplayedTest::initTestCase()
PASS   : TelnetTextDisplayedTest::test_TelnetTextDisplayed()
EOF

cat > "${WORK_DIR}/tracer-line-only.err" <<'EOF'
Tracer caught signal 11: addr=0x29800018 pc=0x7fbee80e17e8 sp=0x7fbedd3ffd20
EOF

cat > "${WORK_DIR}/fatal-error-line-only.err" <<'EOF'
==17923==LeakSanitizer has encountered a fatal error.
==17923==HINT: For debugging, try setting environment variable LSAN_OPTIONS=verbosity=1:log_threads=1
==17923==HINT: LeakSanitizer does not work under ptrace (strace, gdb, etc)
EOF

cat > "${WORK_DIR}/qtest-all-skipped.out" <<'EOF'
********* Start testing of TelnetTextDisplayedTest *********
SKIP   : TelnetTextDisplayedTest::test_TelnetTextDisplayed() no display available
Totals: 0 passed, 0 failed, 1 skipped, 0 blacklisted, 3ms
********* Finished testing of TelnetTextDisplayedTest *********
EOF

cat > "${WORK_DIR}/other-signal.err" <<'EOF'
Tracer caught signal 7: addr=0x29800018 pc=0x7fbee80e17e8 sp=0x7fbedd3ffd20
==17923==LeakSanitizer has encountered a fatal error.
==17923==HINT: LeakSanitizer does not work under ptrace (strace, gdb, etc)
EOF

cat > "${WORK_DIR}/lua-suite-passed.out" <<'EOF'
Running Lua tests from /home/runner/work/Mudlet/Mudlet/src/mudlet-lua/tests
mudlet::~mudlet() INFO - uninstalling translation...
EOF

: > "${WORK_DIR}/empty"

# $1..$3 describe the first run, $4..$6 every run after it: stdout file,
# stderr file, exit status
write_stub() {
  cat > "${STUB}" <<EOF
#!/bin/bash
attempt=\$(( \$(cat "${ATTEMPTS}") + 1 ))
echo "\${attempt}" > "${ATTEMPTS}"
if [ "\${attempt}" -eq 1 ]; then
  cat "$1"
  cat "$2" >&2
  exit $3
fi
cat "$4"
cat "$5" >&2
exit $6
EOF
  chmod +x "${STUB}"
  echo 0 > "${ATTEMPTS}"
}

# Piped into the log rather than redirected onto it: where both cmake and ctest
# are snaps, the inner cmake's writes to a file the outer one handed it are
# dropped, and every assertion below about output then fails for a reason that
# has nothing to do with the launcher
capture() {
  local status
  "$@" 2>&1 | tee "${OUT}" > /dev/null
  status="${PIPESTATUS[0]}"
  return "${status}"
}

run_launcher() {
  # ${1+...} rather than a bare "$@": bash 3.2, which is what macOS
  # runners still ship, calls that an unbound variable under set -u
  capture cmake -P "${LAUNCHER}" -- ${1+"$@"} "${STUB}"
  echo $?
}

assert_status() {
  local expected="$1" actual="$2"
  # -ne on an empty status errors out and takes the false branch, turning every
  # assertion into a pass
  case "${actual}" in
    ''|*[!0-9]*)
      fail "expected exit ${expected}, got a non-numeric status '${actual}'"
      return
      ;;
  esac
  if [ "${actual}" -ne "${expected}" ]; then
    fail "expected exit ${expected}, got ${actual}, output:"
    sed 's/^/        /' "${OUT}" >&2
  fi
}

assert_attempts() {
  local expected="$1" actual
  actual="$(cat "${ATTEMPTS}")"
  if [ "${actual}" != "${expected}" ]; then
    fail "expected the command to run ${expected} time(s), it ran ${actual}, output:"
    sed 's/^/        /' "${OUT}" >&2
  fi
}

assert_contains() {
  if ! grep -qF -- "$1" "${OUT}"; then
    fail "expected '$1' in the launcher output:"
    sed 's/^/        /' "${OUT}" >&2
  fi
}

assert_lacks() {
  if grep -qF -- "$1" "${OUT}"; then
    fail "did not expect '$1' in the launcher output:"
    sed 's/^/        /' "${OUT}" >&2
  fi
}

assert_summary_contains() {
  if ! grep -qF -- "$1" "${SUMMARY}"; then
    fail "expected '$1' in the job summary:"
    sed 's/^/        /' "${SUMMARY}" >&2
  fi
}

# --- the rerun fires, exactly once, and stays visible -------------------------

start_test "the tracer crash after a green QTest run is retried, and passes"
write_stub "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/tracer-crash.err" 1 \
           "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/empty" 0
assert_status 0 "$(run_launcher)"
assert_attempts 2
assert_contains "lsan-tracer-retry: rerunning"
# ctest hides the output of a test that ends up passing, so the job summary is
# the only place the flake stays countable
assert_summary_contains "lsan-tracer-retry: rerunning"

start_test "the crashed run's tracer trace survives into the job summary"
# The rerun passes, so ctest shows nothing at all - if the thread ranges and the
# faulting address do not reach the summary here, #9809 leaves no evidence to
# diagnose from and log_threads=1 in the test environment buys nothing
assert_summary_contains "Stack at 0x29800000-0x29ffed40"
assert_summary_contains "Tracer caught signal 11: addr=0x29800018"

start_test "the retry is bounded: a second tracer crash fails the run"
write_stub "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/tracer-crash.err" 1 \
           "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/tracer-crash.err" 1
assert_status 1 "$(run_launcher)"
assert_attempts 2

start_test "a command that succeeds runs once"
write_stub "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/empty" 0 \
           "${WORK_DIR}/empty" "${WORK_DIR}/empty" 0
assert_status 0 "$(run_launcher)"
assert_attempts 1
assert_lacks "lsan-tracer-retry: rerunning"

# --- one case per guard: none of these may rerun ------------------------------

start_test "guard: a leak report is never retried, even alongside the tracer crash"
write_stub "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/leak-and-tracer.err" 1 \
           "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/empty" 0
assert_status 1 "$(run_launcher)"
assert_attempts 1

start_test "guard: a failing test is never retried, even alongside the tracer crash"
write_stub "${WORK_DIR}/qtest-failed.out" "${WORK_DIR}/tracer-crash.err" 1 \
           "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/empty" 0
assert_status 1 "$(run_launcher)"
assert_attempts 1

start_test "guard: a test that died before reporting a verdict is never retried"
write_stub "${WORK_DIR}/qtest-no-verdict.out" "${WORK_DIR}/tracer-crash.err" 1 \
           "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/empty" 0
assert_status 1 "$(run_launcher)"
assert_attempts 1

start_test "guard: a run that only skipped is not a clean run"
write_stub "${WORK_DIR}/qtest-all-skipped.out" "${WORK_DIR}/tracer-crash.err" 1 \
           "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/empty" 0
assert_status 1 "$(run_launcher)"
assert_attempts 1

start_test "guard: the tracer line alone is not the signature"
write_stub "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/tracer-line-only.err" 1 \
           "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/empty" 0
assert_status 1 "$(run_launcher)"
assert_attempts 1

start_test "guard: LeakSanitizer's other fatal errors are not the signature"
write_stub "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/fatal-error-line-only.err" 1 \
           "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/empty" 0
assert_status 1 "$(run_launcher)"
assert_attempts 1

start_test "guard: a tracer death on another signal is not the signature"
write_stub "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/other-signal.err" 1 \
           "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/empty" 0
assert_status 1 "$(run_launcher)"
assert_attempts 1

start_test "guard: an ordinary non-zero exit is never retried"
write_stub "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/empty" 3 \
           "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/empty" 0
assert_status 1 "$(run_launcher)"
assert_attempts 1

start_test "guard: a run that already exited cleanly is left alone"
# LeakSanitizer only fails the run when LSAN_OPTIONS sets exitcode; without it
# the signature can be printed by a run ctest is about to call a pass
write_stub "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/tracer-crash.err" 0 \
           "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/empty" 0
assert_status 0 "$(run_launcher)"
assert_attempts 1

# --- marker mode, which the Lua suite uses because busted prints no summary ---

MARKER="${WORK_DIR}/busted-tests-failed"

start_test "the Lua suite is retried when it left no failure marker"
rm -f "${MARKER}"
write_stub "${WORK_DIR}/lua-suite-passed.out" "${WORK_DIR}/tracer-crash.err" 1 \
           "${WORK_DIR}/lua-suite-passed.out" "${WORK_DIR}/empty" 0
assert_status 0 "$(run_launcher "--failure-marker=${MARKER}")"
assert_attempts 2

start_test "guard: the Lua suite is not retried when it wrote a failure marker"
echo "Lua busted tests failed" > "${MARKER}"
write_stub "${WORK_DIR}/lua-suite-passed.out" "${WORK_DIR}/tracer-crash.err" 1 \
           "${WORK_DIR}/lua-suite-passed.out" "${WORK_DIR}/empty" 0
assert_status 1 "$(run_launcher "--failure-marker=${MARKER}")"
assert_attempts 1

start_test "guard: the Lua suite is not retried when run-tests wrote its marker elsewhere"
# run-tests.xml prefers MUDLET_TEST_FAILURE_MARKER and only falls back to the
# path the workflow passes here, so checking one path alone is not enough
rm -f "${MARKER}"
echo "Lua busted tests failed" > "${WORK_DIR}/elsewhere-marker"
write_stub "${WORK_DIR}/lua-suite-passed.out" "${WORK_DIR}/tracer-crash.err" 1 \
           "${WORK_DIR}/lua-suite-passed.out" "${WORK_DIR}/empty" 0
assert_status 1 "$(MUDLET_TEST_FAILURE_MARKER="${WORK_DIR}/elsewhere-marker" run_launcher "--failure-marker=${MARKER}")"
assert_attempts 1
rm -f "${WORK_DIR}/elsewhere-marker"

if [ "${FAILURES}" -ne 0 ]; then
  echo "${FAILURES} assertion(s) failed" >&2
  exit 1
fi

echo "all lsan-tracer-retry checks passed"
