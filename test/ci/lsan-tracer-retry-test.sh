#!/bin/bash
# Tests test/lsan-tracer-retry.cmake, the launcher that reruns a leak-checked
# test when LeakSanitizer's tracer crashed on the way out (#9809).
#
# The flake it exists for is intermittent and CI-only, so the launcher is driven
# here against captured output instead: a stub command prints a recorded log and
# exits non-zero, and each case asserts both the exit status and how many times
# the stub ran. Retrying a run that really failed would hide the failure, so
# most of the cases below are about NOT retrying.

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

start_test() {
  echo "=== $1"
}

fail() {
  echo "    FAIL: $*" >&2
  FAILURES=$((FAILURES + 1))
}

# The tracer crash as it reached the log of run 31428353403, on
# TelnetTextDisplayedTest: the test body reported itself green and only then did
# LeakSanitizer die, with no leak report at all
cat > "${WORK_DIR}/qtest-passed.out" <<'EOF'
********* Start testing of TelnetTextDisplayedTest *********
Config: Using QtTest library 6.8.2, Qt 6.8.2
PASS   : TelnetTextDisplayedTest::initTestCase()
PASS   : TelnetTextDisplayedTest::textIsDisplayed()
PASS   : TelnetTextDisplayedTest::textIsDisplayedWithAnsi()
PASS   : TelnetTextDisplayedTest::promptIsDisplayed()
PASS   : TelnetTextDisplayedTest::cleanupTestCase()
Totals: 5 passed, 0 failed, 0 skipped, 0 blacklisted, 812ms
********* Finished testing of TelnetTextDisplayedTest *********
EOF

cat > "${WORK_DIR}/tracer-crash.err" <<'EOF'
Tracer caught signal 11: addr=0x29800018 pc=0x7f4c9e2e17e8 sp=0x7f4c99afeb30
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

cat > "${WORK_DIR}/qtest-failed.out" <<'EOF'
********* Start testing of TelnetTextDisplayedTest *********
Config: Using QtTest library 6.8.2, Qt 6.8.2
PASS   : TelnetTextDisplayedTest::initTestCase()
FAIL!  : TelnetTextDisplayedTest::textIsDisplayed() Compared values are not the same
   Actual   (displayedText): "hello"
   Expected (expectedText) : "world"
   Loc: [/home/runner/work/Mudlet/Mudlet/test/functional_tests/TelnetTextDisplayedTest.cpp(212)]
PASS   : TelnetTextDisplayedTest::cleanupTestCase()
Totals: 4 passed, 1 failed, 0 skipped, 0 blacklisted, 790ms
********* Finished testing of TelnetTextDisplayedTest *********
EOF

cat > "${WORK_DIR}/qtest-crashed-early.out" <<'EOF'
********* Start testing of TelnetTextDisplayedTest *********
Config: Using QtTest library 6.8.2, Qt 6.8.2
PASS   : TelnetTextDisplayedTest::initTestCase()
QFATAL : TelnetTextDisplayedTest::textIsDisplayed() ASSERT: "mpHost" in file ctelnet.cpp, line 812
EOF

# Both phrases have to be there, so neither half of the signature is enough on
# its own. LeakSanitizer announces its other fatal errors - a thread it cannot
# read registers from, a world it cannot stop - with the same second line, and
# those are a different problem that has to stay visible
cat > "${WORK_DIR}/tracer-line-only.err" <<'EOF'
Tracer caught signal 11: addr=0x29800018 pc=0x7f4c9e2e17e8 sp=0x7f4c99afeb30
EOF

cat > "${WORK_DIR}/fatal-error-line-only.err" <<'EOF'
==17923==Unable to get registers from thread 17925.
==17923==LeakSanitizer has encountered a fatal error.
==17923==HINT: For debugging, try setting environment variable LSAN_OPTIONS=verbosity=1:log_threads=1
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

run_launcher() {
  cmake -P "${LAUNCHER}" -- "$@" "${STUB}" > "${OUT}" 2>&1
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

start_test "a command that succeeds runs once"
write_stub "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/empty" 0 \
           "${WORK_DIR}/empty" "${WORK_DIR}/empty" 0
assert_status 0 "$(run_launcher)"
assert_attempts 1
assert_lacks "lsan-tracer-retry: rerunning"

start_test "the tracer crash after a green QTest run is retried, and passes"
write_stub "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/tracer-crash.err" 1 \
           "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/empty" 0
assert_status 0 "$(run_launcher)"
assert_attempts 2
assert_contains "lsan-tracer-retry: rerunning"

start_test "the retry is bounded: a second tracer crash fails the run"
write_stub "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/tracer-crash.err" 1 \
           "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/tracer-crash.err" 1
assert_status 1 "$(run_launcher)"
assert_attempts 2

start_test "the output of both runs still reaches the caller"
write_stub "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/tracer-crash.err" 1 \
           "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/empty" 0
assert_status 0 "$(run_launcher)"
assert_contains "PASS   : TelnetTextDisplayedTest::textIsDisplayed()"
assert_contains "Tracer caught signal 11"

start_test "a leak report is never retried, even alongside the tracer crash"
cat "${WORK_DIR}/leak-report.err" "${WORK_DIR}/tracer-crash.err" > "${WORK_DIR}/leak-and-tracer.err"
write_stub "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/leak-and-tracer.err" 1 \
           "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/empty" 0
assert_status 1 "$(run_launcher)"
assert_attempts 1

start_test "a plain leak report, with no tracer crash at all, is never retried"
write_stub "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/leak-report.err" 1 \
           "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/empty" 0
assert_status 1 "$(run_launcher)"
assert_attempts 1

start_test "a failing test is never retried, even alongside the tracer crash"
write_stub "${WORK_DIR}/qtest-failed.out" "${WORK_DIR}/tracer-crash.err" 1 \
           "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/empty" 0
assert_status 1 "$(run_launcher)"
assert_attempts 1

start_test "a test that died before reporting anything is never retried"
write_stub "${WORK_DIR}/qtest-crashed-early.out" "${WORK_DIR}/tracer-crash.err" 1 \
           "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/empty" 0
assert_status 1 "$(run_launcher)"
assert_attempts 1

start_test "the tracer line alone is not the signature"
write_stub "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/tracer-line-only.err" 1 \
           "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/empty" 0
assert_status 1 "$(run_launcher)"
assert_attempts 1

start_test "LeakSanitizer's other fatal errors are not the signature"
write_stub "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/fatal-error-line-only.err" 1 \
           "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/empty" 0
assert_status 1 "$(run_launcher)"
assert_attempts 1

start_test "a command killed by a signal fails rather than being read as success"
# CMake hands back a description rather than a number for those, so a status
# comparison that assumed a number would let them through
cat > "${STUB}" <<'EOF'
#!/bin/bash
kill -SEGV $$
EOF
chmod +x "${STUB}"
echo 0 > "${ATTEMPTS}"
cmake -P "${LAUNCHER}" -- "${STUB}" > "${OUT}" 2>&1
assert_status 1 $?
assert_contains "Segmentation fault"

start_test "an ordinary non-zero exit is never retried"
write_stub "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/empty" 3 \
           "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/empty" 0
assert_status 1 "$(run_launcher)"
assert_attempts 1

# The Lua suite prints no QTest summary, and LeakSanitizer's Die() eats its own
# summary line along with the rest of the unflushed stdio, so the evidence that
# nothing failed is the marker file the suite writes when something did
MARKER="${WORK_DIR}/busted-tests-failed"

start_test "the Lua suite is retried when it left no failure marker"
rm -f "${MARKER}"
write_stub "${WORK_DIR}/lua-suite-passed.out" "${WORK_DIR}/tracer-crash.err" 1 \
           "${WORK_DIR}/lua-suite-passed.out" "${WORK_DIR}/empty" 0
assert_status 0 "$(run_launcher "--failure-marker=${MARKER}")"
assert_attempts 2

start_test "the Lua suite is not retried when it wrote a failure marker"
echo "Lua busted tests failed" > "${MARKER}"
write_stub "${WORK_DIR}/lua-suite-passed.out" "${WORK_DIR}/tracer-crash.err" 1 \
           "${WORK_DIR}/lua-suite-passed.out" "${WORK_DIR}/empty" 0
assert_status 1 "$(run_launcher "--failure-marker=${MARKER}")"
assert_attempts 1

start_test "the marker mode still refuses to retry a leak report"
rm -f "${MARKER}"
write_stub "${WORK_DIR}/lua-suite-passed.out" "${WORK_DIR}/leak-and-tracer.err" 1 \
           "${WORK_DIR}/lua-suite-passed.out" "${WORK_DIR}/empty" 0
assert_status 1 "$(run_launcher "--failure-marker=${MARKER}")"
assert_attempts 1

start_test "without a marker, a run with no QTest summary is not retried either"
write_stub "${WORK_DIR}/lua-suite-passed.out" "${WORK_DIR}/tracer-crash.err" 1 \
           "${WORK_DIR}/lua-suite-passed.out" "${WORK_DIR}/empty" 0
assert_status 1 "$(run_launcher)"
assert_attempts 1

start_test "arguments reach the command unsplit"
# The Lua leg passes --profile "Mudlet self-test", so an argument containing a
# space has to survive both the launcher's own parsing and execute_process
cat > "${STUB}" <<EOF
#!/bin/bash
printf '[%s]\n' "\$@"
EOF
chmod +x "${STUB}"
cmake -P "${LAUNCHER}" -- "${STUB}" --profile "Mudlet self-test" --mirror > "${OUT}" 2>&1
assert_status 0 $?
assert_contains "[Mudlet self-test]"
assert_contains "[--mirror]"

start_test "a launcher given no command fails rather than reporting success"
if cmake -P "${LAUNCHER}" -- > "${OUT}" 2>&1; then
  fail "expected the launcher to reject an empty command line"
fi

if [ "${FAILURES}" -ne 0 ]; then
  echo "${FAILURES} assertion(s) failed" >&2
  exit 1
fi

echo "all lsan-tracer-retry checks passed"
