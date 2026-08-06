#!/bin/bash
# Tests CI/check-release-tag.sh, the guard that keeps a release from being
# published under a tag that does not spell out APP_VERSION.
#
# The mistake it catches is invisible without it. Tagging "Mudlet-5.0" while
# APP_VERSION is 5.0.0 passes every existing check - CI/prepare-release-assets.sh
# only matches the tag as a prefix of the asset names, and
# "Mudlet-5.0.0-linux-x64.AppImage.tar" does start with "Mudlet-5.0" - yet the
# updater offers version "5.0", which is not a three-component semantic version and
# is therefore never treated as newer than an installed 4.22.0. Every existing user
# stops being offered updates and nothing anywhere says so.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
CHECK="${REPO_DIR}/CI/check-release-tag.sh"

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

# Runs the guard and leaves its combined output in ${WORK_DIR}/out
run_check() {
  bash "${CHECK}" "$1" "$2" > "${WORK_DIR}/out" 2>&1
  echo $?
}

assert_status() {
  local expected="$1" actual="$2"
  if [ "${actual}" -ne "${expected}" ]; then
    fail "expected exit ${expected}, got ${actual}, output:"
    sed 's/^/        /' "${WORK_DIR}/out" >&2
  fi
}

assert_contains() {
  local needle="$1"
  if ! grep -qF -- "${needle}" "${WORK_DIR}/out"; then
    fail "expected '${needle}' in the output:"
    sed 's/^/        /' "${WORK_DIR}/out" >&2
  fi
}

start_test "the tag a 5.0 release has to carry is accepted"
assert_status 0 "$(run_check "Mudlet-5.0.0" "5.0.0")"

start_test "every tag Mudlet has released under so far is accepted"
for version in 4.19.0 4.20.1 4.21.0 4.22.0 5.0.0 10.11.12; do
  assert_status 0 "$(run_check "Mudlet-${version}" "${version}")"
done

start_test "the two-component tag that silently disables auto-update is rejected"
assert_status 1 "$(run_check "Mudlet-5.0" "5.0.0")"
assert_contains "Mudlet-5.0.0"
assert_contains "auto-update"

start_test "a stale tag against a bumped APP_VERSION is rejected"
assert_status 1 "$(run_check "Mudlet-4.22.0" "5.0.0")"

start_test "a stale APP_VERSION against a bumped tag is rejected"
assert_status 1 "$(run_check "Mudlet-5.0.0" "4.22.0")"

start_test "a two-component APP_VERSION is rejected even though its tag matches"
assert_status 1 "$(run_check "Mudlet-5.0" "5.0")"
assert_contains "three-component"

start_test "a tag missing the Mudlet- prefix is rejected"
assert_status 1 "$(run_check "5.0.0" "5.0.0")"

start_test "a suffixed tag is rejected - releases are tagged clean"
assert_status 1 "$(run_check "Mudlet-5.0.0-rc1" "5.0.0")"

start_test "a missing argument is a usage error, not a silent pass"
assert_status 2 "$(run_check "Mudlet-5.0.0" "")"
assert_status 2 "$(run_check "" "5.0.0")"

start_test "the failure is annotated for GitHub Actions, not only printed"
GITHUB_ACTIONS=true bash "${CHECK}" "Mudlet-5.0" "5.0.0" > "${WORK_DIR}/out" 2>&1
assert_contains "::error::"

# The guard is only useful if the release scripts actually reach it
start_test "the tag-push build validation calls the guard"
if command -v pcre2grep > /dev/null; then
  mkdir -p "${WORK_DIR}/repo"
  printf 'set(APP_VERSION 5.0.0)\n' > "${WORK_DIR}/repo/CMakeLists.txt"

  for script in validate_deployment.sh validate-deployment-for-windows.sh; do
    (cd "${WORK_DIR}/repo" && GITHUB_REF="refs/tags/Mudlet-5.0.0" GITHUB_REPO_TAG=true WITH_UPDATER=YES \
      bash "${REPO_DIR}/CI/${script}") > "${WORK_DIR}/out" 2>&1
    assert_status 0 "$?"

    (cd "${WORK_DIR}/repo" && GITHUB_REF="refs/tags/Mudlet-5.0" GITHUB_REPO_TAG=true WITH_UPDATER=YES \
      bash "${REPO_DIR}/CI/${script}") > "${WORK_DIR}/out" 2>&1
    assert_status 1 "$?"
    assert_contains "does not match APP_VERSION"
  done

  # A development build has no tag to check and must not be blocked by one
  (cd "${WORK_DIR}/repo" && GITHUB_REF="refs/heads/development" GITHUB_REPO_TAG=false WITH_UPDATER=YES \
    bash "${REPO_DIR}/CI/validate_deployment.sh") > "${WORK_DIR}/out" 2>&1
  assert_status 0 "$?"
  assert_contains "skipping release validation"
else
  echo "    skipped: pcre2grep is not installed"
fi

start_test "the release workflow calls the guard before it publishes anything"
if ! grep -qF 'CI/check-release-tag.sh' "${REPO_DIR}/.github/workflows/create-github-release.yml"; then
  fail "create-github-release.yml no longer calls CI/check-release-tag.sh"
fi

if [ "${FAILURES}" -gt 0 ]; then
  echo "${FAILURES} check(s) failed"
  exit 1
fi

echo "All checks passed"
