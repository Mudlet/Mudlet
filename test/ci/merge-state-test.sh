#!/bin/bash
# Tests the two CI scripts whose whole job is to not fail quietly.
#
# .github/scripts/resolve-milestone.sh replaces a jq filter that matched a
# milestone title exactly. The real title had picked up a suffix, so it matched
# nothing, produced an empty milestone number and exited 0 - every pull request
# in the repository went unassigned for months without one red check (#9671).
#
# .github/scripts/report-merge-state.sh is what makes a conflicted pull request
# visible at all (#9619), so an API failure that it swallowed would put back the
# same false all-clear one level up.
#
# gh is stubbed from GH_STUB_DIR, so this needs no network and no token.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SCRIPTS_DIR="$(cd "${SCRIPT_DIR}/../../.github/scripts" && pwd)"

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

assert_status() {
  local expected="$1" actual="$2"
  if [ "${actual}" -ne "${expected}" ]; then
    fail "expected exit ${expected}, got ${actual}"
  fi
}

assert_contains() {
  local file="$1" needle="$2"
  if ! grep -qF -- "${needle}" "${file}"; then
    fail "expected '${needle}' in ${file}:"
    sed 's/^/        /' "${file}" >&2
  fi
}

assert_absent() {
  local file="$1" needle="$2"
  if grep -qF -- "${needle}" "${file}"; then
    fail "did not expect '${needle}' in ${file}:"
    sed 's/^/        /' "${file}" >&2
  fi
}

# A stand-in for the gh CLI that answers from files and records what it was
# asked, so a test can assert on the statuses that were posted
new_stub() {
  local dir="${WORK_DIR}/$1"
  rm -rf "${dir}"
  mkdir -p "${dir}/bin"
  cat > "${dir}/bin/gh" <<'STUB'
#!/usr/bin/env bash
set -u
printf '%s\n' "$*" >> "${GH_STUB_DIR}/calls.log"

key=unknown
for arg in "$@"; do
  case "${arg}" in
    *"/pulls?state=open"*) key=list ;;
    */pulls/*)             key="pull-${arg##*/}" ;;
    */statuses/*)          key=status ;;
    *milestones*)          key=milestones ;;
  esac
done

if [ -f "${GH_STUB_DIR}/${key}.fail" ]; then
  echo "gh: simulated API failure for ${key}" >&2
  exit 1
fi
if [ -f "${GH_STUB_DIR}/${key}.out" ]; then
  cat "${GH_STUB_DIR}/${key}.out"
fi
exit 0
STUB
  chmod +x "${dir}/bin/gh"
  : > "${dir}/calls.log"
  STUB_DIR="${dir}"
}

# env, not a bare assignment prefix: any extra VAR=VALUE arguments arrive by
# expansion, and bash only treats literal words as assignments
run_merge_state() {
  env GH_STUB_DIR="${STUB_DIR}" \
    PATH="${STUB_DIR}/bin:${PATH}" \
    REPO=Mudlet/Mudlet \
    BASE_REF=development \
    MERGE_STATE_MAX_ROUNDS=2 \
    MERGE_STATE_ROUND_DELAY_SECONDS=0 \
    "$@" \
    bash "${SCRIPTS_DIR}/report-merge-state.sh" > "${STUB_DIR}/out.log" 2>&1
}

run_resolve_milestone() {
  env GH_STUB_DIR="${STUB_DIR}" \
    PATH="${STUB_DIR}/bin:${PATH}" \
    REPO=Mudlet/Mudlet \
    NEXT_MILESTONE="$1" \
    bash "${SCRIPTS_DIR}/resolve-milestone.sh" > "${STUB_DIR}/out.log" 2>&1
}

#-----------------------------------------------------------------------------
start_test "a conflicting pull request is reported as a failed status"
new_stub conflicting
printf '9603\n' > "${STUB_DIR}/list.out"
printf 'open false bcc2b3e\n' > "${STUB_DIR}/pull-9603.out"
run_merge_state
assert_status 0 $?
assert_contains "${STUB_DIR}/calls.log" 'statuses/bcc2b3e'
assert_contains "${STUB_DIR}/calls.log" 'state=failure'
assert_contains "${STUB_DIR}/out.log" 'Conflicts with development'

#-----------------------------------------------------------------------------
start_test "a clean pull request is reported as a successful status"
new_stub clean
printf '9628\n' > "${STUB_DIR}/list.out"
printf 'open true f524d96\n' > "${STUB_DIR}/pull-9628.out"
run_merge_state
assert_status 0 $?
assert_contains "${STUB_DIR}/calls.log" 'state=success'
assert_absent "${STUB_DIR}/calls.log" 'state=failure'

#-----------------------------------------------------------------------------
start_test "a closed pull request gets no status at all"
new_stub closed
printf '9665\n' > "${STUB_DIR}/list.out"
printf 'closed null 0440ef5\n' > "${STUB_DIR}/pull-9665.out"
run_merge_state
assert_status 0 $?
assert_absent "${STUB_DIR}/calls.log" 'statuses/'
assert_contains "${STUB_DIR}/out.log" 'is closed'

#-----------------------------------------------------------------------------
start_test "an undecided merge state is left pending rather than guessed at"
new_stub undecided
printf '9675\n' > "${STUB_DIR}/list.out"
printf 'open null 23c1189\n' > "${STUB_DIR}/pull-9675.out"
run_merge_state
assert_status 0 $?
assert_contains "${STUB_DIR}/calls.log" 'state=pending'
assert_contains "${STUB_DIR}/out.log" '::warning::'

#-----------------------------------------------------------------------------
# The regression this file exists for: mapfile in a process substitution hides
# the exit status of gh, so a failed listing used to read as "nothing is open",
# post nothing, and exit 0 - leaving a stale success on every conflicted PR
start_test "a failed pull request listing is loud, not an empty repository"
new_stub list-fails
touch "${STUB_DIR}/list.fail"
run_merge_state
assert_status 1 $?
assert_contains "${STUB_DIR}/out.log" '::error::Could not list'
assert_absent "${STUB_DIR}/out.log" 'No open pull requests'

#-----------------------------------------------------------------------------
start_test "a pull request that never reads fails the job and does not block the others"
new_stub read-fails
printf '9603\n9628\n' > "${STUB_DIR}/list.out"
touch "${STUB_DIR}/pull-9603.fail"
printf 'open true f524d96\n' > "${STUB_DIR}/pull-9628.out"
run_merge_state
assert_status 1 $?
assert_contains "${STUB_DIR}/out.log" 'Never managed to read pull request #9603'
assert_contains "${STUB_DIR}/calls.log" 'statuses/f524d96'

#-----------------------------------------------------------------------------
start_test "a status that cannot be posted fails the job"
new_stub post-fails
printf '9628\n' > "${STUB_DIR}/list.out"
printf 'open true f524d96\n' > "${STUB_DIR}/pull-9628.out"
touch "${STUB_DIR}/status.fail"
run_merge_state
assert_status 1 $?
assert_contains "${STUB_DIR}/out.log" 'Could not post'
assert_contains "${STUB_DIR}/out.log" 'Reported the merge state of 0 of 1'

#-----------------------------------------------------------------------------
start_test "a single pull request is reported without listing anything"
new_stub single
printf 'open false bcc2b3e\n' > "${STUB_DIR}/pull-9603.out"
run_merge_state PR_NUMBER=9603
assert_status 0 $?
assert_absent "${STUB_DIR}/calls.log" 'state=open'
assert_contains "${STUB_DIR}/calls.log" 'state=failure'

#-----------------------------------------------------------------------------
MILESTONES='[{"number":18,"title":"5.0 beginner-friendly"},
             {"number":62,"title":"future release"},
             {"number":64,"title":"4.23.0 next release"}]'

start_test "a suffixed milestone title still matches the bare version (#9671)"
new_stub milestone-prefix
printf '%s\n' "${MILESTONES}" > "${STUB_DIR}/milestones.out"
run_resolve_milestone 4.23.0
assert_status 0 $?
assert_contains "${STUB_DIR}/out.log" '64 4.23.0 next release'

#-----------------------------------------------------------------------------
start_test "an exact title wins over a longer one that also starts with it"
new_stub milestone-exact
printf '%s\n' '[{"number":64,"title":"4.23.0 next release"},{"number":70,"title":"4.23.0"}]' \
  > "${STUB_DIR}/milestones.out"
run_resolve_milestone 4.23.0
assert_status 0 $?
assert_contains "${STUB_DIR}/out.log" '70 4.23.0'

#-----------------------------------------------------------------------------
start_test "a version that matches no milestone fails and says what is open"
new_stub milestone-none
printf '%s\n' "${MILESTONES}" > "${STUB_DIR}/milestones.out"
run_resolve_milestone 4.99.0
assert_status 1 $?
assert_contains "${STUB_DIR}/out.log" '::error::No open milestone matches'
assert_contains "${STUB_DIR}/out.log" '4.23.0 next release'

#-----------------------------------------------------------------------------
start_test "a version that matches several milestones is refused, not guessed"
new_stub milestone-ambiguous
printf '%s\n' '[{"number":64,"title":"4.23.0 next release"},{"number":71,"title":"4.23.0 stretch goals"}]' \
  > "${STUB_DIR}/milestones.out"
run_resolve_milestone 4.23.0
assert_status 1 $?
assert_contains "${STUB_DIR}/out.log" 'matches several open milestones'

#-----------------------------------------------------------------------------
start_test "a near miss on the version does not match"
new_stub milestone-nearmiss
printf '%s\n' "${MILESTONES}" > "${STUB_DIR}/milestones.out"
run_resolve_milestone 4.2
assert_status 1 $?
assert_contains "${STUB_DIR}/out.log" '::error::No open milestone matches'

#-----------------------------------------------------------------------------
start_test "a failed milestone read is loud"
new_stub milestone-fails
touch "${STUB_DIR}/milestones.fail"
run_resolve_milestone 4.23.0
assert_status 1 $?
assert_contains "${STUB_DIR}/out.log" '::error::Could not read the open milestones'

#-----------------------------------------------------------------------------
echo
if [ "${FAILURES}" -gt 0 ]; then
  echo "${FAILURES} check(s) FAILED"
  exit 1
fi
echo "All checks passed"
