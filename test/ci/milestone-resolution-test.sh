#!/bin/bash
# Tests .github/scripts/resolve-milestone.sh, whose whole job is to not fail
# quietly.
#
# It replaces a jq filter that matched a milestone title exactly. The real title
# had picked up a suffix, so it matched nothing, produced an empty milestone
# number and exited 0 - every pull request in the repository went unassigned for
# months without one red check (#9671).
#
# gh is stubbed from GH_STUB_DIR, so this needs no network and no token.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SCRIPTS_DIR="$(cd "${SCRIPT_DIR}/../../.github/scripts" && pwd)"

WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

FAILURES=0

start_test() {
  echo "=== $1"
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

# A stand-in for the gh CLI that answers from files, so the script can be handed
# a milestone listing without a network call or a token
new_stub() {
  local dir="${WORK_DIR}/$1"
  rm -rf "${dir}"
  mkdir -p "${dir}/bin"
  cat > "${dir}/bin/gh" <<'STUB'
#!/usr/bin/env bash
set -u

key=unknown
for arg in "$@"; do
  case "${arg}" in
    *milestones*) key=milestones ;;
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
  STUB_DIR="${dir}"
}

# The script takes every input from the environment, so the environment is built
# from nothing rather than inherited: `env -i`, and not a bare assignment prefix,
# so that nothing the surrounding CI job exported can reach it.
run_resolve_milestone() {
  env -i \
    PATH="${STUB_DIR}/bin:${PATH}" \
    HOME="${WORK_DIR}" \
    GH_STUB_DIR="${STUB_DIR}" \
    REPO=Mudlet/Mudlet \
    NEXT_MILESTONE="$1" \
    bash "${SCRIPTS_DIR}/resolve-milestone.sh" > "${STUB_DIR}/out.log" 2>&1
}

#-----------------------------------------------------------------------------
# The repository's real open milestones. "5.0 beginner-friendly" sitting next to
# "5.0.0 next release" is the reason the prefix match is anchored on a following
# space rather than being a bare startswith
MILESTONES='[{"number":18,"title":"5.0 beginner-friendly"},
             {"number":62,"title":"future release"},
             {"number":64,"title":"5.0.0 next release"}]'

start_test "a suffixed milestone title still matches the bare version (#9671)"
new_stub milestone-prefix
printf '%s\n' "${MILESTONES}" > "${STUB_DIR}/milestones.out"
run_resolve_milestone 5.0.0
assert_status 0 $?
assert_contains "${STUB_DIR}/out.log" '64 5.0.0 next release'

#-----------------------------------------------------------------------------
start_test "a shorter milestone that shares a prefix is not swept in"
new_stub milestone-shared-prefix
printf '%s\n' "${MILESTONES}" > "${STUB_DIR}/milestones.out"
run_resolve_milestone 5.0
assert_status 0 $?
assert_contains "${STUB_DIR}/out.log" '18 5.0 beginner-friendly'
assert_absent "${STUB_DIR}/out.log" '5.0.0 next release'

#-----------------------------------------------------------------------------
start_test "an exact title wins over a longer one that also starts with it"
new_stub milestone-exact
printf '%s\n' '[{"number":64,"title":"5.0.0 next release"},{"number":70,"title":"5.0.0"}]' \
  > "${STUB_DIR}/milestones.out"
run_resolve_milestone 5.0.0
assert_status 0 $?
assert_contains "${STUB_DIR}/out.log" '70 5.0.0'

#-----------------------------------------------------------------------------
start_test "a version that matches no milestone fails and says what is open"
new_stub milestone-none
printf '%s\n' "${MILESTONES}" > "${STUB_DIR}/milestones.out"
run_resolve_milestone 4.99.0
assert_status 1 $?
assert_contains "${STUB_DIR}/out.log" '::error::No open milestone matches'
assert_contains "${STUB_DIR}/out.log" '5.0.0 next release'

#-----------------------------------------------------------------------------
start_test "a version that matches several milestones is refused, not guessed"
new_stub milestone-ambiguous
printf '%s\n' '[{"number":64,"title":"5.0.0 next release"},{"number":71,"title":"5.0.0 stretch goals"}]' \
  > "${STUB_DIR}/milestones.out"
run_resolve_milestone 5.0.0
assert_status 1 $?
assert_contains "${STUB_DIR}/out.log" 'matches several open milestones'

#-----------------------------------------------------------------------------
start_test "a near miss on the version does not match"
new_stub milestone-nearmiss
printf '%s\n' "${MILESTONES}" > "${STUB_DIR}/milestones.out"
run_resolve_milestone 5.0.1
assert_status 1 $?
assert_contains "${STUB_DIR}/out.log" '::error::No open milestone matches'

#-----------------------------------------------------------------------------
start_test "a failed milestone read is loud"
new_stub milestone-fails
touch "${STUB_DIR}/milestones.fail"
run_resolve_milestone 5.0.0
assert_status 1 $?
assert_contains "${STUB_DIR}/out.log" '::error::Could not read the open milestones'

#-----------------------------------------------------------------------------
echo
if [ "${FAILURES}" -gt 0 ]; then
  echo "${FAILURES} check(s) FAILED"
  exit 1
fi
echo "All checks passed"
