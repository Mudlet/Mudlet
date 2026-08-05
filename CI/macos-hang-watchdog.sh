#!/bin/bash
#
# Runs a command and, if it stops producing output or outstays its budget,
# captures full thread backtraces of it *before* anything kills it.
#
# Written for "Mudlet stops responding on macOS part way through the package
# specs, after a burst of profile saves" (issue #9670): the hang only happens on
# the macOS CI runners, so the stacks have to be taken on the runner, by
# something that does not need a human at the keyboard.
#
# Two independent triggers, because either one alone can be fooled:
#   --stall-seconds  the command has written nothing for this long. This is the
#                    one that matches the bug, which is total silence. It is not
#                    armed until the command has written something, so a slow
#                    start is not mistaken for a hang.
#   --max-seconds    the command has simply run too long. Catches a wedge that
#                    still dribbles output, which the stall trigger would miss.
# Both capture. Nothing is killed before a capture has been attempted.
#
# Neither trigger can tell a hang from something merely slow - only the stacks
# can - so what is reported is which trigger fired, not a diagnosis.
#
# Deliberately not "set -e": a capture tool that fails must be *reported*, not
# allowed to abort the script before it has tried the other tools or written its
# outcome file. Every command whose failure matters is checked by hand.
set -uo pipefail

# Exit codes above the range a sane command uses, so the caller can tell a hang
# apart from an ordinary test failure.
readonly EXIT_HANG_CAPTURED=90
readonly EXIT_HANG_NOT_CAPTURED=91
readonly EXIT_KILLED_BEFORE_CAPTURE=92
readonly EXIT_USAGE=2

usage() {
  cat >&2 <<'USAGE'
Usage: macos-hang-watchdog.sh --capture-dir DIR [options] -- command [args...]

  --capture-dir DIR    directory for the output log, the stacks and the outcome
                       file (created if missing, required)
  --stall-seconds N    capture once the command has gone N seconds without
                       adding to its output, counting only from its first byte
                       (default 30, 0 disables this trigger)
  --max-seconds N      capture once the command has run for N seconds whatever
                       it is printing (default 240, 0 disables this trigger)
  --hold-seconds N     after capturing, leave the process running for up to N
                       seconds so someone on an interactive session can look at
                       it live (default 0, capped at 1800)

Exit codes: the command's own status if it finished on its own, 90 if it was
caught hanging and stacks were captured, 91 if it was caught hanging and every
capture tool failed, 92 if the watchdog was itself killed before it could
capture, 2 for a usage error.
USAGE
}

readonly HOLD_SECONDS_CAP=1800

capture_dir=""
stall_seconds=30
max_seconds=240
hold_seconds=0

require_value() {
  # $1 option name, $2 the value as passed (may be unset)
  if [ -z "${2:-}" ]; then
    echo "macos-hang-watchdog: $1 needs a value" >&2
    usage
    exit "${EXIT_USAGE}"
  fi
}

require_number() {
  # $1 option name, $2 value
  case "$2" in
    '' | *[!0-9]*)
      echo "macos-hang-watchdog: $1 needs a whole number of seconds, got '$2'" >&2
      exit "${EXIT_USAGE}"
      ;;
  esac
}

while [ $# -gt 0 ]; do
  case "$1" in
    --capture-dir)
      require_value "$1" "${2:-}"
      capture_dir="$2"
      shift 2
      ;;
    --stall-seconds)
      require_value "$1" "${2:-}"
      require_number "$1" "$2"
      stall_seconds="$2"
      shift 2
      ;;
    --max-seconds)
      require_value "$1" "${2:-}"
      require_number "$1" "$2"
      max_seconds="$2"
      shift 2
      ;;
    --hold-seconds)
      require_value "$1" "${2:-}"
      require_number "$1" "$2"
      hold_seconds="$2"
      shift 2
      ;;
    --help | -h)
      usage
      exit 0
      ;;
    --)
      shift
      break
      ;;
    *)
      echo "macos-hang-watchdog: unknown option '$1'" >&2
      usage
      exit "${EXIT_USAGE}"
      ;;
  esac
done

if [ -z "${capture_dir}" ]; then
  echo "macos-hang-watchdog: --capture-dir is required" >&2
  usage
  exit "${EXIT_USAGE}"
fi

if [ $# -eq 0 ]; then
  echo "macos-hang-watchdog: no command given after --" >&2
  usage
  exit "${EXIT_USAGE}"
fi

if [ "${stall_seconds}" -eq 0 ] && [ "${max_seconds}" -eq 0 ]; then
  echo "macos-hang-watchdog: both triggers are disabled, so nothing would ever be captured" >&2
  exit "${EXIT_USAGE}"
fi

if [ "${hold_seconds}" -gt "${HOLD_SECONDS_CAP}" ]; then
  echo "macos-hang-watchdog: --hold-seconds ${hold_seconds} is above the ${HOLD_SECONDS_CAP}s cap, using the cap"
  hold_seconds="${HOLD_SECONDS_CAP}"
fi

if ! mkdir -p "${capture_dir}"; then
  echo "macos-hang-watchdog: cannot create the capture directory '${capture_dir}'" >&2
  exit "${EXIT_USAGE}"
fi

log_file="${capture_dir}/command-output.log"
outcome_file="${capture_dir}/outcome.txt"
failures_file="${capture_dir}/capture-failures.txt"

if ! : > "${log_file}"; then
  echo "macos-hang-watchdog: cannot write to '${log_file}'" >&2
  exit "${EXIT_USAGE}"
fi
# Anything an earlier run left here would otherwise be counted as this run's
# evidence - record_tool_result only looks at whether a file is non-empty
rm -f "${failures_file}" "${capture_dir}/CAPTURE-FAILED" \
  "${capture_dir}/sample.txt" "${capture_dir}/spindump.txt" \
  "${capture_dir}/lldb.txt" "${capture_dir}/lldb-sudo.txt" \
  "${capture_dir}/processes.txt" "${outcome_file}"

# A process that has exited but not been reaped is still signalable, so
# "kill -0" would say it is alive forever. Ask for its state instead: gone means
# no output at all, reaped-pending means a state starting with Z.
process_running() {
  local state
  state="$(ps -p "$1" -o stat= 2>/dev/null | tr -d ' ')"
  [ -n "${state}" ] && [ "${state#Z}" = "${state}" ]
}

# spindump and the lldb retry run under sudo, so their process is root-owned and
# an ordinary kill from this shell is refused. Escalate rather than let a wedged
# capture tool outlive the limit that is supposed to bound it.
signal_tool() {
  local signal="$1" pid="$2"
  kill "-${signal}" "${pid}" 2>/dev/null && return 0
  sudo -n kill "-${signal}" "${pid}" 2>/dev/null
}

# Runs a command with a hard limit, so a capture tool that wedges cannot take
# the watchdog down with it. Returns 124 if it had to be killed.
run_with_limit() {
  local limit="$1"
  shift
  "$@" &
  local tool_pid=$!
  local waited=0
  while [ "${waited}" -lt "${limit}" ] && process_running "${tool_pid}"; do
    sleep 1
    waited=$((waited + 1))
  done
  if process_running "${tool_pid}"; then
    signal_tool TERM "${tool_pid}"
    sleep 2
    signal_tool KILL "${tool_pid}"
    # Only reap once it has actually gone: waiting on a process this shell
    # cannot signal would block for as long as the tool decides to run, which is
    # the very thing the limit exists to prevent.
    waited=0
    while [ "${waited}" -lt 10 ] && process_running "${tool_pid}"; do
      sleep 1
      waited=$((waited + 1))
    done
    if process_running "${tool_pid}"; then
      note_capture_failure "could not kill $1 (pid ${tool_pid}), leaving it to finish on its own"
      return 124
    fi
    wait "${tool_pid}" 2>/dev/null
    return 124
  fi
  local status=0
  wait "${tool_pid}" || status=$?
  return "${status}"
}

captured_anything=0

note_capture_failure() {
  echo "$1" >> "${failures_file}"
  echo "macos-hang-watchdog: $1" >&2
}

# $1 tool name, $2 exit status, $3 file it should have filled
record_tool_result() {
  local tool="$1" status="$2" output="$3"
  if [ "${status}" -ne 0 ]; then
    if [ "${status}" -eq 124 ]; then
      note_capture_failure "${tool} did not finish in time and was killed"
    else
      note_capture_failure "${tool} exited ${status}"
    fi
    # A tool that died part way through may still have written something worth
    # reading, so say so - but do not let it count as a successful capture
    if [ -s "${output}" ]; then
      note_capture_failure "${tool} did leave $(wc -c < "${output}" | tr -d ' ') bytes in ${output}, which may still be readable"
    fi
    return
  fi
  if [ ! -s "${output}" ]; then
    note_capture_failure "${tool} succeeded but wrote nothing to ${output}"
    return
  fi
  captured_anything=1
  echo "macos-hang-watchdog: ${tool} wrote $(wc -c < "${output}" | tr -d ' ') bytes to ${output}"
}

capture_stacks() {
  local reason="$1" pid="$2"

  echo "=== macos-hang-watchdog: ${reason} - capturing stacks of pid ${pid} ==="

  {
    echo "--- the watched process ---"
    ps -p "${pid}" -o pid,ppid,stat,etime,command 2>&1 || echo "pid ${pid} is already gone"
    echo "--- every mudlet process on the runner ---"
    # pgrep cannot report elapsed time, which is what says whether a stray
    # process has been sitting there since before the run started
    # shellcheck disable=SC2009
    ps ax -o pid,ppid,stat,etime,command 2>&1 | grep -i '[m]udlet' \
      || echo "no process with 'mudlet' in its command line - the target had already gone"
  } > "${capture_dir}/processes.txt" 2>&1

  # sample first: it does not stop the target, so the stacks it takes are of a
  # process still in the state the hang left it in.
  if command -v sample > /dev/null 2>&1; then
    run_with_limit 90 sample "${pid}" 5 -file "${capture_dir}/sample.txt" > "${capture_dir}/sample-tool.log" 2>&1
    record_tool_result "sample" $? "${capture_dir}/sample.txt"
  else
    note_capture_failure "sample is not on this runner (is this macOS?)"
  fi

  # spindump adds what each thread is blocked *on*, which is the part that tells
  # a thread-pool deadlock apart from a thread that is merely idle. "sudo -n" so
  # that a runner without passwordless sudo fails immediately instead of sitting
  # on a password prompt until the watchdog's own limit kills it.
  if command -v spindump > /dev/null 2>&1; then
    run_with_limit 120 sudo -n spindump "${pid}" 5 -file "${capture_dir}/spindump.txt" -noProcessingWhileSampling > "${capture_dir}/spindump-tool.log" 2>&1
    record_tool_result "spindump" $? "${capture_dir}/spindump.txt"
  else
    note_capture_failure "spindump is not on this runner (is this macOS?)"
  fi

  # lldb last: attaching suspends the target, so anything that wants to see the
  # process live has already had its turn.
  if command -v lldb > /dev/null 2>&1; then
    run_with_limit 120 lldb -p "${pid}" --batch -o "thread backtrace all" -o "process detach" > "${capture_dir}/lldb.txt" 2>&1
    local lldb_status=$?
    if [ "${lldb_status}" -eq 0 ] && grep -q "thread #" "${capture_dir}/lldb.txt"; then
      record_tool_result "lldb" 0 "${capture_dir}/lldb.txt"
    else
      # Attaching without privileges fails on some macOS configurations, and
      # lldb reports that either as a non-zero status or as a zero status with
      # no frames, so retry on both. The retry writes its own file: overwriting
      # lldb.txt would destroy the reason the first attempt came up empty.
      echo "macos-hang-watchdog: lldb produced no backtraces (exit ${lldb_status}), retrying under sudo"
      note_capture_failure "lldb without sudo produced no thread backtraces (exit ${lldb_status}) - see lldb.txt"
      run_with_limit 120 sudo -n lldb -p "${pid}" --batch -o "thread backtrace all" -o "process detach" > "${capture_dir}/lldb-sudo.txt" 2>&1
      local lldb_sudo_status=$?
      if [ "${lldb_sudo_status}" -eq 0 ] && ! grep -q "thread #" "${capture_dir}/lldb-sudo.txt"; then
        note_capture_failure "lldb under sudo ran but captured no thread backtraces - see lldb-sudo.txt"
      else
        record_tool_result "lldb (sudo)" "${lldb_sudo_status}" "${capture_dir}/lldb-sudo.txt"
      fi
    fi
  else
    note_capture_failure "lldb is not on this runner (is this macOS?)"
  fi

  if [ "${captured_anything}" -eq 1 ]; then
    echo "macos-hang-watchdog: stacks are in ${capture_dir}"
    return 0
  fi

  : > "${capture_dir}/CAPTURE-FAILED"
  echo "::error::macos-hang-watchdog caught the hang but every stack capture tool failed - see capture-failures.txt in the uploaded artifact"
  return 1
}

terminate_child() {
  local pid="$1"
  process_running "${pid}" || return 0
  kill -TERM "${pid}" 2>/dev/null
  local waited=0
  while [ "${waited}" -lt 15 ] && process_running "${pid}"; do
    sleep 1
    waited=$((waited + 1))
  done
  if process_running "${pid}"; then
    echo "macos-hang-watchdog: pid ${pid} ignored SIGTERM, sending SIGKILL"
    kill -KILL "${pid}" 2>/dev/null
  fi
}

echo "macos-hang-watchdog: running: $*"
echo "macos-hang-watchdog: stall trigger ${stall_seconds}s, budget ${max_seconds}s, hold after capture ${hold_seconds}s"
echo "macos-hang-watchdog: capture directory ${capture_dir}"

"$@" >> "${log_file}" 2>&1 &
child_pid=$!

# Mirror the log into the caller's own output, so a job watching live sees the
# command's progress (and its silence) as it happens rather than at the end.
tail -f "${log_file}" &
tail_pid=$!

# shellcheck disable=SC2317  # reached through the traps below
cleanup() {
  kill "${tail_pid}" 2>/dev/null
  wait "${tail_pid}" 2>/dev/null
}
trap cleanup EXIT

# Being signalled from outside means the caller gave up on us: a step timeout, a
# cancelled job, the job's own cap, a preempted runner. Which of those it was is
# not knowable from in here, so list them rather than assert one - but do say
# loudly that nothing was sampled, because the alternative is an artifact that
# looks like a clean run.
# shellcheck disable=SC2317  # reached through the TERM/INT trap below
on_external_kill() {
  trap - TERM INT
  echo "::error::macos-hang-watchdog was killed by a signal after ${SECONDS}s without capturing anything (its own budget was ${max_seconds}s). A step timeout shorter than that budget, a cancelled job or a preempted runner would all look like this."
  {
    echo "outcome: watchdog killed by a signal after ${SECONDS}s"
    echo "stacks captured: no"
    echo "watchdog budget: ${max_seconds}s"
    echo "log bytes at that point: $(wc -c < "${log_file}" 2>/dev/null | tr -d ' ')"
  } > "${outcome_file}"
  terminate_child "${child_pid}"
  cleanup
  exit "${EXIT_KILLED_BEFORE_CAPTURE}"
}
trap on_external_kill TERM INT

start_time="$(date +%s)"
last_size=-1
last_change="${start_time}"
last_heartbeat="${start_time}"
hang_reason=""

while process_running "${child_pid}"; do
  now="$(date +%s)"
  size="$(wc -c < "${log_file}" 2>/dev/null | tr -d ' ')"
  [ -n "${size}" ] || size=0
  if [ "${size}" != "${last_size}" ]; then
    last_size="${size}"
    last_change="${now}"
  fi

  elapsed=$((now - start_time))
  stalled=$((now - last_change))

  # Only armed once the command has written something. A cold start that takes
  # longer than the stall window to produce its first line is a slow launch, not
  # the silence-after-progress this is looking for, and the budget still covers
  # a command that never says anything at all.
  if [ "${stall_seconds}" -gt 0 ] && [ "${last_size}" -gt 0 ] && [ "${stalled}" -ge "${stall_seconds}" ]; then
    hang_reason="no output for ${stalled}s (stall trigger ${stall_seconds}s, ${elapsed}s into the run)"
    break
  fi
  if [ "${max_seconds}" -gt 0 ] && [ "${elapsed}" -ge "${max_seconds}" ]; then
    hang_reason="still running after ${elapsed}s (budget ${max_seconds}s)"
    break
  fi

  if [ $((now - last_heartbeat)) -ge 15 ]; then
    last_heartbeat="${now}"
    echo "macos-hang-watchdog: ${elapsed}s elapsed, ${stalled}s since the last output, ${size} bytes logged"
  fi

  sleep 1
done

# A command that went quiet and then exited on the very tick the trigger fired
# finished normally, and reporting that as a hang would throw away its real
# status - so the trigger only stands if the process is still there.
if [ -n "${hang_reason}" ] && ! process_running "${child_pid}"; then
  echo "macos-hang-watchdog: ${hang_reason}, but the command exited as the trigger fired - treating it as a normal exit"
  hang_reason=""
fi

if [ -z "${hang_reason}" ]; then
  command_status=0
  wait "${child_pid}" || command_status=$?
  # Let tail catch up with whatever the command wrote as it exited.
  sleep 1
  end_time="$(date +%s)"
  {
    echo "outcome: command exited with status ${command_status} after $((end_time - start_time))s"
    echo "log bytes: $(wc -c < "${log_file}" 2>/dev/null | tr -d ' ')"
  } > "${outcome_file}"
  echo "macos-hang-watchdog: command exited with status ${command_status}"
  exit "${command_status}"
fi

echo "macos-hang-watchdog: ${hang_reason}"
capture_status=0
capture_stacks "${hang_reason}" "${child_pid}" || capture_status=$?

{
  # "trigger fired" rather than "hang": neither trigger can tell a wedge from
  # something slow, and only the stacks can settle it
  echo "outcome: trigger fired, process sampled while still running"
  echo "reason: ${hang_reason}"
  echo "stacks captured: $([ "${captured_anything}" -eq 1 ] && echo yes || echo no)"
  echo "log bytes at capture: $(wc -c < "${log_file}" 2>/dev/null | tr -d ' ')"
  echo "command: $*"
} > "${outcome_file}"

# The result is on disk now, so a signal from here on must not overwrite it with
# "captured nothing" - which is exactly what ending a tmate session by
# cancelling the job would otherwise do during the hold below. Swap the trap for
# one that still tidies the child up but keeps the outcome that was written.
# shellcheck disable=SC2317  # reached through the TERM/INT trap below
on_kill_after_capture() {
  trap - TERM INT
  echo "macos-hang-watchdog: signalled after the stacks were already captured, keeping the outcome as it stands"
  echo "note: the hold was cut short by a signal" >> "${outcome_file}"
  terminate_child "${child_pid}"
  cleanup
  if [ "${capture_status}" -ne 0 ]; then
    exit "${EXIT_HANG_NOT_CAPTURED}"
  fi
  exit "${EXIT_HANG_CAPTURED}"
}
trap on_kill_after_capture TERM INT

if [ "${hold_seconds}" -gt 0 ]; then
  echo "macos-hang-watchdog: holding pid ${child_pid} for up to ${hold_seconds}s so it can be inspected live"
  held=0
  while [ "${held}" -lt "${hold_seconds}" ] && process_running "${child_pid}"; do
    sleep 5
    held=$((held + 5))
  done
fi

terminate_child "${child_pid}"
wait "${child_pid}" 2>/dev/null

if [ "${capture_status}" -ne 0 ]; then
  exit "${EXIT_HANG_NOT_CAPTURED}"
fi
exit "${EXIT_HANG_CAPTURED}"
