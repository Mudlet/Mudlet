#!/bin/bash
# Tests CI/macos-hang-watchdog.sh, the watchdog that is supposed to capture
# thread backtraces of Mudlet when it stops responding on a macOS runner
# (issue #9670).
#
# What is being protected here is the harness's honesty rather than its stacks.
# The capture tools it drives - sample, spindump, lldb - only exist on macOS, so
# on the machines that run this test every capture fails, and that is precisely
# the interesting case: a watchdog that stays quiet when it captured nothing is
# worse than no watchdog, because the empty artifact reads as a clean run. So
# every check below is about what the watchdog *says and returns*: does it pass
# an ordinary exit status through untouched, does it notice silence, does it
# notice a run that overruns its budget, does it admit when it captured nothing,
# and does it leave the process it was watching dead rather than stranded.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WATCHDOG="$(cd "${SCRIPT_DIR}/../../CI" && pwd)/macos-hang-watchdog.sh"

WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

FAILURES=0
CURRENT_TEST=""

start_test() {
  CURRENT_TEST="$1"
  echo "=== ${CURRENT_TEST}"
}

fail() {
  echo "    FAIL: $*" >&2
  FAILURES=$((FAILURES + 1))
}

pass() {
  echo "    ok: $*"
}

assert_status() {
  local expected="$1" actual="$2" what="$3"
  if [[ "${actual}" == "${expected}" ]]; then
    pass "${what} exited ${expected}"
  else
    fail "${what} exited ${actual}, expected ${expected}"
  fi
}

assert_contains() {
  local file="$1" needle="$2"
  if [[ ! -f "${file}" ]]; then
    fail "${file##*/} does not exist, so it cannot contain '${needle}'"
    return
  fi
  if grep -qF -- "${needle}" "${file}"; then
    pass "${file##*/} contains '${needle}'"
  else
    fail "${file##*/} is missing '${needle}'"
    echo "--- ${file} ---" >&2
    cat "${file}" >&2
  fi
}

assert_exists() {
  if [[ -e "$1" ]]; then
    pass "${1##*/} was written"
  else
    fail "${1##*/} was not written"
  fi
}

assert_gone() {
  local pid="$1"
  # A pid the watchdog reaped is either absent or a zombie of this shell, and
  # neither can still be running Mudlet.
  local state
  state="$(ps -p "${pid}" -o stat= 2>/dev/null | tr -d ' ')"
  if [[ -z "${state}" || "${state#Z}" != "${state}" ]]; then
    pass "the watched process is gone"
  else
    fail "pid ${pid} is still alive in state '${state}' after the watchdog finished"
  fi
}

if [[ ! -x "${WATCHDOG}" ]]; then
  echo "FAIL: ${WATCHDOG} is missing or not executable" >&2
  exit 1
fi

#-----------------------------------------------------------------------------
start_test "a command that finishes on its own passes its status through"
run_dir="${WORK_DIR}/clean"
"${WATCHDOG}" --capture-dir "${run_dir}" --stall-seconds 20 --max-seconds 60 \
  -- bash -c 'echo working; sleep 1; echo done' > "${WORK_DIR}/clean.log" 2>&1
assert_status 0 $? "a command that exits 0"
assert_contains "${run_dir}/outcome.txt" "command exited with status 0"
assert_contains "${run_dir}/command-output.log" "working"
if [[ -e "${run_dir}/CAPTURE-FAILED" ]]; then
  fail "a clean run left a CAPTURE-FAILED marker behind"
else
  pass "a clean run leaves no CAPTURE-FAILED marker"
fi

#-----------------------------------------------------------------------------
start_test "a failing command's own exit status is not swallowed"
run_dir="${WORK_DIR}/failing"
"${WATCHDOG}" --capture-dir "${run_dir}" --stall-seconds 20 --max-seconds 60 \
  -- bash -c 'echo nope; exit 7' > "${WORK_DIR}/failing.log" 2>&1
assert_status 7 $? "a command that exits 7"
assert_contains "${run_dir}/outcome.txt" "command exited with status 7"

#-----------------------------------------------------------------------------
start_test "silence trips the stall trigger and the failure to capture is loud"
run_dir="${WORK_DIR}/stalled"
"${WATCHDOG}" --capture-dir "${run_dir}" --stall-seconds 3 --max-seconds 120 \
  -- bash -c 'echo started; exec sleep 120' > "${WORK_DIR}/stalled.log" 2>&1
stall_status=$?
# 90 would mean a capture tool worked, which can only happen on macOS.
if [[ "${stall_status}" == "90" || "${stall_status}" == "91" ]]; then
  pass "the stall was caught (exit ${stall_status})"
else
  fail "a silent command exited ${stall_status}, expected 90 or 91"
  cat "${WORK_DIR}/stalled.log" >&2
fi
assert_contains "${run_dir}/outcome.txt" "outcome: hang caught"
assert_contains "${run_dir}/outcome.txt" "no output for"
assert_exists "${run_dir}/processes.txt"
if [[ "${stall_status}" == "91" ]]; then
  assert_exists "${run_dir}/CAPTURE-FAILED"
  assert_contains "${run_dir}/capture-failures.txt" "sample"
  # The GitHub annotation is what stops this being a silent failure in a job log
  assert_contains "${WORK_DIR}/stalled.log" "::error::"
  assert_contains "${run_dir}/outcome.txt" "stacks captured: no"
else
  assert_contains "${run_dir}/outcome.txt" "stacks captured: yes"
fi

#-----------------------------------------------------------------------------
start_test "a command that never stops talking still trips the budget"
run_dir="${WORK_DIR}/overrun"
"${WATCHDOG}" --capture-dir "${run_dir}" --stall-seconds 0 --max-seconds 4 \
  -- bash -c 'while :; do echo tick; sleep 1; done' > "${WORK_DIR}/overrun.log" 2>&1
overrun_status=$?
if [[ "${overrun_status}" == "90" || "${overrun_status}" == "91" ]]; then
  pass "the overrun was caught (exit ${overrun_status})"
else
  fail "an overrunning command exited ${overrun_status}, expected 90 or 91"
  cat "${WORK_DIR}/overrun.log" >&2
fi
assert_contains "${run_dir}/outcome.txt" "still running after"

#-----------------------------------------------------------------------------
start_test "the watched process is not left behind"
run_dir="${WORK_DIR}/leftover"
pid_file="${WORK_DIR}/leftover.pid"
# $$ and $0 belong to the inner bash, which is why they are not expanded here
# shellcheck disable=SC2016
"${WATCHDOG}" --capture-dir "${run_dir}" --stall-seconds 3 --max-seconds 120 \
  -- bash -c 'echo $$ > "$0"; echo started; exec sleep 120' "${pid_file}" \
  > "${WORK_DIR}/leftover.log" 2>&1
watched_pid="$(cat "${pid_file}" 2>/dev/null)"
if [[ -n "${watched_pid}" ]]; then
  assert_gone "${watched_pid}"
else
  fail "the test command never reported its pid"
fi

#-----------------------------------------------------------------------------
start_test "being killed itself is reported rather than passed off as a clean run"
run_dir="${WORK_DIR}/killed"
"${WATCHDOG}" --capture-dir "${run_dir}" --stall-seconds 60 --max-seconds 120 \
  -- bash -c 'echo started; exec sleep 120' > "${WORK_DIR}/killed.log" 2>&1 &
watchdog_pid=$!
# Long enough for the watchdog to have started the command and installed its trap
sleep 3
kill -TERM "${watchdog_pid}" 2>/dev/null
killed_status=0
wait "${watchdog_pid}" || killed_status=$?
assert_status 92 "${killed_status}" "a watchdog killed by its caller"
assert_contains "${run_dir}/outcome.txt" "watchdog killed externally"
assert_contains "${WORK_DIR}/killed.log" "::error::"

#-----------------------------------------------------------------------------
start_test "misuse is refused rather than quietly watching nothing"
"${WATCHDOG}" --stall-seconds 10 -- true > /dev/null 2>&1
assert_status 2 $? "a run with no --capture-dir"
"${WATCHDOG}" --capture-dir "${WORK_DIR}/misuse" -- > /dev/null 2>&1
assert_status 2 $? "a run with no command"
"${WATCHDOG}" --capture-dir "${WORK_DIR}/misuse" --max-seconds soon -- true > /dev/null 2>&1
assert_status 2 $? "a run with a non-numeric --max-seconds"
"${WATCHDOG}" --capture-dir "${WORK_DIR}/misuse" --stall-seconds 0 --max-seconds 0 -- true > /dev/null 2>&1
assert_status 2 $? "a run with both triggers disabled"

#-----------------------------------------------------------------------------
echo
if [[ ${FAILURES} -gt 0 ]]; then
  echo "${FAILURES} check(s) FAILED"
  exit 1
fi
echo "All checks passed"
