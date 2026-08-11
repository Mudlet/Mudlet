#!/bin/bash
# Tests test/lsan-tracer-retry.cmake, the launcher that reruns a leak-checked
# test when LeakSanitizer's tracer crashed on the way out (#9809).
#
# The flake it exists for is intermittent and CI-only, so the launcher is driven
# here against a stub command instead: the stub prints a log and exits non-zero,
# and each case asserts both the exit status and how many times the stub ran.
# Rerunning a run that really failed would hide the failure, so most of the
# cases below are about NOT rerunning.

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
REAL_STEP_SUMMARY="${GITHUB_STEP_SUMMARY:-}"
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

cat > "${WORK_DIR}/tracer-crash.err" <<'EOF'
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
# ctest hides the output of a test that ends up passing, so the job summary is
# the only place the flake stays countable
assert_summary_contains "lsan-tracer-retry: rerunning"

start_test "the retry is bounded: a second tracer crash fails the run"
write_stub "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/tracer-crash.err" 1 \
           "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/tracer-crash.err" 1
assert_status 1 "$(run_launcher)"
assert_attempts 2

start_test "the output of both runs still reaches the caller"
write_stub "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/tracer-crash.err" 1 \
           "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/empty" 0
assert_status 0 "$(run_launcher)"
assert_contains "PASS   : TelnetTextDisplayedTest::test_TelnetTextDisplayed()"
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

start_test "a test that died before reporting a verdict is never retried"
write_stub "${WORK_DIR}/qtest-no-verdict.out" "${WORK_DIR}/tracer-crash.err" 1 \
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
cat > "${STUB}" <<EOF
#!/bin/bash
echo 1 > "${ATTEMPTS}"
kill -SEGV \$\$
EOF
chmod +x "${STUB}"
echo 0 > "${ATTEMPTS}"
capture cmake -P "${LAUNCHER}" -- "${STUB}"
assert_status 1 $?
assert_attempts 1
assert_contains "Segmentation fault"

start_test "an ordinary non-zero exit is never retried"
write_stub "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/empty" 3 \
           "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/empty" 0
assert_status 1 "$(run_launcher)"
assert_attempts 1

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
capture cmake -P "${LAUNCHER}" -- "${STUB}" --profile "Mudlet self-test" --mirror
assert_status 0 $?
assert_contains "[Mudlet self-test]"
assert_contains "[--mirror]"

start_test "a run that already exited cleanly is left alone"
# LeakSanitizer only fails the run when LSAN_OPTIONS sets exitcode; without it
# the signature can be printed by a run ctest is about to call a pass
write_stub "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/tracer-crash.err" 0 \
           "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/empty" 0
assert_status 0 "$(run_launcher)"
assert_attempts 1

start_test "a tracer death on another signal is not the signature"
write_stub "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/other-signal.err" 1 \
           "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/empty" 0
assert_status 1 "$(run_launcher)"
assert_attempts 1

start_test "a run that only skipped is not a clean run"
write_stub "${WORK_DIR}/qtest-all-skipped.out" "${WORK_DIR}/tracer-crash.err" 1 \
           "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/empty" 0
assert_status 1 "$(run_launcher)"
assert_attempts 1

start_test "the Lua suite is not retried when run-tests wrote its marker elsewhere"
# run-tests.xml prefers MUDLET_TEST_FAILURE_MARKER and only falls back to the
# path the workflow passes here, so checking one path alone is not enough
rm -f "${MARKER}"
echo "Lua busted tests failed" > "${WORK_DIR}/elsewhere-marker"
write_stub "${WORK_DIR}/lua-suite-passed.out" "${WORK_DIR}/tracer-crash.err" 1 \
           "${WORK_DIR}/lua-suite-passed.out" "${WORK_DIR}/empty" 0
assert_status 1 "$(MUDLET_TEST_FAILURE_MARKER="${WORK_DIR}/elsewhere-marker" run_launcher "--failure-marker=${MARKER}")"
assert_attempts 1
rm -f "${WORK_DIR}/elsewhere-marker"

start_test "options after the command belong to the command"
cat > "${STUB}" <<EOF
#!/bin/bash
printf '[%s]\n' "\$@"
EOF
chmod +x "${STUB}"
capture cmake -P "${LAUNCHER}" -- "${STUB}" --failure-marker=/tmp/not-mine --
assert_status 0 $?
assert_contains "[--failure-marker=/tmp/not-mine]"
assert_contains "[--]"

start_test "an option the launcher does not know is rejected, not run as the command"
if capture cmake -P "${LAUNCHER}" -- --failure-marker/tmp/typo "${STUB}"; then
  fail "expected the launcher to reject a mistyped option"
fi
assert_contains "unknown option"

start_test "--failure-marker with no path is rejected rather than changing mode"
if capture cmake -P "${LAUNCHER}" -- --failure-marker= "${STUB}"; then
  fail "expected the launcher to reject an empty failure marker"
fi
assert_contains "needs a path"

start_test "a command that cannot be launched fails and says which one"
if capture cmake -P "${LAUNCHER}" -- "${WORK_DIR}/no-such-command" --with --args; then
  fail "expected the launcher to fail on a command it cannot launch"
fi
assert_contains "no-such-command --with --args"

start_test "a job summary that cannot be appended to does not cost the rerun"
write_stub "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/tracer-crash.err" 1 \
           "${WORK_DIR}/qtest-passed.out" "${WORK_DIR}/empty" 0
assert_status 0 "$(GITHUB_STEP_SUMMARY="${WORK_DIR}" run_launcher)"
assert_attempts 2

start_test "a launcher given no command fails rather than reporting success"
if capture cmake -P "${LAUNCHER}" --; then
  fail "expected the launcher to reject an empty command line"
fi
assert_contains "no command given"

# The launcher runs inside one ctest test command, so both of its runs share one
# TIMEOUT. functional_tests/CMakeLists.txt doubles the budget of every wrapped
# test from a deferred call; if that ever stops reaching them, the rerun is
# killed halfway and the job goes red looking like a timeout rather than #9809
start_test "every wrapped test has ctest budget for the rerun"
testfile="${MUDLET_FUNCTIONAL_TEST_BINARY_DIR:-}/CTestTestfile.cmake"
if [ ! -e "${testfile}" ]; then
  echo "    skipped: no configured build tree to read"
else
  wrapped="$(awk -F'[][]' '/^add_test\(/ && /lsan-tracer-retry\.cmake/ { print $3 }' "${testfile}")"
  if [ -z "${wrapped}" ]; then
    echo "    skipped: this build tree wraps no tests, so the leak check is off"
  else
    for name in ${wrapped}; do
      budget="$(awk -v n="[=[${name}]=]" '
        index($0, "set_tests_properties(" n) == 1 {
          if (match($0, /TIMEOUT "[0-9]+"/)) print substr($0, RSTART + 9, RLENGTH - 10)
        }' "${testfile}")"
      case "${budget}" in
        ''|*[!0-9]*) fail "${name} is wrapped but has no numeric TIMEOUT" ;;
        *) [ "${budget}" -ge 120 ] || fail "${name} is wrapped but its TIMEOUT is only ${budget}s, too tight for a rerun" ;;
      esac
    done
    echo "    checked $(echo "${wrapped}" | wc -w | tr -d ' ') wrapped tests"
  fi
fi

start_test "the real job summary was left alone"
if [ -n "${REAL_STEP_SUMMARY}" ] && grep -qF "lsan-tracer-retry" "${REAL_STEP_SUMMARY}" 2>/dev/null; then
  fail "this test wrote its own rerun notices into ${REAL_STEP_SUMMARY}"
fi

if [ "${FAILURES}" -ne 0 ]; then
  echo "${FAILURES} assertion(s) failed" >&2
  exit 1
fi

echo "all lsan-tracer-retry checks passed"
