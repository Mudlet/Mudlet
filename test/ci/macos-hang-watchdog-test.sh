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
assert_contains "${run_dir}/outcome.txt" "outcome: trigger fired"
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
assert_contains "${run_dir}/outcome.txt" "watchdog killed by a signal"
assert_contains "${WORK_DIR}/killed.log" "::error::"

#-----------------------------------------------------------------------------
start_test "a slow start is not mistaken for a hang"
run_dir="${WORK_DIR}/slow-start"
"${WATCHDOG}" --capture-dir "${run_dir}" --stall-seconds 2 --max-seconds 60 \
  -- bash -c 'sleep 5; echo finally; exit 3' > "${WORK_DIR}/slow-start.log" 2>&1
assert_status 3 $? "a command silent for longer than the stall window before its first line"
assert_contains "${run_dir}/outcome.txt" "command exited with status 3"

#-----------------------------------------------------------------------------
start_test "a command that exits as the trigger fires keeps its own status"
run_dir="${WORK_DIR}/raced"
"${WATCHDOG}" --capture-dir "${run_dir}" --stall-seconds 3 --max-seconds 60 \
  -- bash -c 'echo started; sleep 3; exit 5' > "${WORK_DIR}/raced.log" 2>&1
raced_status=$?
# Either the command won the race (5) or the watchdog did (90/91) - the failure
# to avoid is 91 with the command's real status thrown away when it had exited
if [[ "${raced_status}" == "5" ]]; then
  pass "the command's own status survived the race"
  assert_contains "${run_dir}/outcome.txt" "command exited with status 5"
elif [[ "${raced_status}" == "90" || "${raced_status}" == "91" ]]; then
  pass "the watchdog won the race and reported the trigger (exit ${raced_status})"
else
  fail "a command exiting as the trigger fires gave ${raced_status}, expected 5, 90 or 91"
fi

#-----------------------------------------------------------------------------
# The only successful outcome, exit 90, needs the macOS capture tools. Stub them
# so the branch is covered everywhere rather than only on a Mac.
start_test "a capture that works reports success and does not cry wolf"
run_dir="${WORK_DIR}/captured"
stub_dir="${WORK_DIR}/stubs"
mkdir -p "${stub_dir}"
cat > "${stub_dir}/sample" <<'STUB'
#!/bin/bash
# mimics: sample <pid> <seconds> -file <out>
echo "stub sample of $1" > "$4"
STUB
cat > "${stub_dir}/spindump" <<'STUB'
#!/bin/bash
echo "stub spindump" > "$4"
STUB
cat > "${stub_dir}/lldb" <<'STUB'
#!/bin/bash
echo "  thread #1"
echo "    frame #0: stub"
STUB
chmod +x "${stub_dir}/sample" "${stub_dir}/spindump" "${stub_dir}/lldb"
PATH="${stub_dir}:${PATH}" "${WATCHDOG}" --capture-dir "${run_dir}" --stall-seconds 3 --max-seconds 60 \
  -- bash -c 'echo started; exec sleep 60' > "${WORK_DIR}/captured.log" 2>&1
assert_status 90 $? "a stall with working capture tools"
assert_contains "${run_dir}/outcome.txt" "stacks captured: yes"
assert_contains "${run_dir}/sample.txt" "stub sample"
assert_contains "${run_dir}/lldb.txt" "thread #1"
if [[ -e "${run_dir}/CAPTURE-FAILED" ]]; then
  fail "a successful capture still left a CAPTURE-FAILED marker"
else
  pass "a successful capture leaves no CAPTURE-FAILED marker"
fi
if grep -qF "::error::" "${WORK_DIR}/captured.log"; then
  fail "a successful capture emitted an error annotation"
else
  pass "a successful capture emits no error annotation"
fi

#-----------------------------------------------------------------------------
start_test "a signal after the stacks are captured does not erase them"
run_dir="${WORK_DIR}/kill-after-capture"
PATH="${stub_dir}:${PATH}" "${WATCHDOG}" --capture-dir "${run_dir}" --stall-seconds 3 --max-seconds 60 \
  --hold-seconds 60 -- bash -c 'echo started; exec sleep 120' > "${WORK_DIR}/kill-after-capture.log" 2>&1 &
watchdog_pid=$!
# Past the 3 second trigger and into the hold, where a cancelled job would land
sleep 8
kill -TERM "${watchdog_pid}" 2>/dev/null
held_status=0
wait "${watchdog_pid}" || held_status=$?
assert_status 90 "${held_status}" "a watchdog signalled during the hold"
assert_contains "${run_dir}/outcome.txt" "stacks captured: yes"
assert_contains "${run_dir}/outcome.txt" "the hold was cut short by a signal"
assert_exists "${run_dir}/sample.txt"

#-----------------------------------------------------------------------------
start_test "leftovers from an earlier run are not passed off as this run's evidence"
run_dir="${WORK_DIR}/stale"
mkdir -p "${run_dir}"
echo "backtraces from some other day" > "${run_dir}/sample.txt"
"${WATCHDOG}" --capture-dir "${run_dir}" --stall-seconds 3 --max-seconds 60 \
  -- bash -c 'echo started; exec sleep 60' > "${WORK_DIR}/stale.log" 2>&1
stale_status=$?
if [[ "${stale_status}" == "91" ]]; then
  pass "the stale sample.txt did not count as a capture"
  if grep -qF "some other day" "${run_dir}/sample.txt" 2>/dev/null; then
    fail "the stale sample.txt is still there and would be uploaded as evidence"
  else
    pass "the stale sample.txt was cleared"
  fi
elif [[ "${stale_status}" == "90" ]]; then
  # Only reachable where the real tools exist and genuinely captured something
  assert_contains "${run_dir}/outcome.txt" "stacks captured: yes"
  if grep -qF "some other day" "${run_dir}/sample.txt" 2>/dev/null; then
    fail "sample.txt still holds the earlier run's contents"
  else
    pass "sample.txt was replaced by this run's capture"
  fi
else
  fail "a stalled run over a stale capture directory exited ${stale_status}"
fi

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
