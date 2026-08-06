#!/bin/bash
# Tests CI/check-release-tag.sh, the guard that keeps a release from being
# published under a version nobody can be offered.
#
# The mistake it catches leaves no trace. Tagging "Mudlet-5.0" while APP_VERSION is
# 5.0.0 passes every existing check - CI/prepare-release-assets.sh only matches the
# tag as a prefix of the asset names, and "Mudlet-5.0.0-linux-x64.AppImage.tar" does
# start with "Mudlet-5.0" - yet the updater then offers version "5.0", which is not
# a three-component semantic version and is therefore never treated as newer than an
# installed 4.22.0. Every existing user stops being offered updates and the update
# check goes on logging "0 update(s) available".

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
CHECK="${REPO_DIR}/CI/check-release-tag.sh"
RELEASE_WORKFLOW="${REPO_DIR}/.github/workflows/create-github-release.yml"

WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

OUT="${WORK_DIR}/out"
FAILURES=0

start_test() {
  echo "=== $1"
}

fail() {
  echo "    FAIL: $*" >&2
  FAILURES=$((FAILURES + 1))
}

# Runs the guard, leaving its combined output in ${OUT} and echoing its status
run_check() {
  bash "${CHECK}" "$@" > "${OUT}" 2>&1
  echo $?
}

assert_status() {
  local expected="$1" actual="$2"
  # An empty or non-numeric status would make the comparison below error out and
  # take the false branch, quietly turning every assertion into a pass
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

assert_contains() {
  local file="$1" needle="$2"
  if ! grep -qF -- "${needle}" "${file}"; then
    fail "expected '${needle}' in ${file}:"
    sed 's/^/        /' "${file}" >&2
  fi
}

start_test "the tag a 5.0 release has to carry is accepted"
assert_status 0 "$(run_check "5.0.0" "Mudlet-5.0.0")"

start_test "the tags of every release since 4.19 are accepted, and plausible future ones"
for version in 4.19.0 4.20.1 4.21.0 4.22.0 5.0.0 10.11.12; do
  assert_status 0 "$(run_check "${version}" "Mudlet-${version}")"
done

start_test "the two-component tag that silently disables auto-update is rejected"
assert_status 1 "$(run_check "5.0.0" "Mudlet-5.0")"
assert_contains "${OUT}" "Mudlet-5.0.0"
assert_contains "${OUT}" "auto-update"

start_test "a stale tag against a bumped APP_VERSION is rejected"
assert_status 1 "$(run_check "5.0.0" "Mudlet-4.22.0")"

start_test "a stale APP_VERSION against a bumped tag is rejected"
assert_status 1 "$(run_check "4.22.0" "Mudlet-5.0.0")"

start_test "a two-component APP_VERSION is rejected even though its tag matches"
assert_status 1 "$(run_check "5.0" "Mudlet-5.0")"
assert_contains "${OUT}" "three-component"

start_test "a version SemVer would reject for its leading zeros is rejected"
assert_status 1 "$(run_check "5.01.0" "Mudlet-5.01.0")"

# Release.cpp only strips a capitalised "Mudlet-", while the asset check matches
# case-insensitively and set-build-info.sh lowercases the version - so a lowercase
# tag is a mistake the surrounding tooling invites, and it fails the same silent way
start_test "a lowercase tag is rejected"
assert_status 1 "$(run_check "4.22.0" "mudlet-4.22.0")"

start_test "a tag missing the Mudlet- prefix is rejected"
assert_status 1 "$(run_check "5.0.0" "5.0.0")"

# Not because the updater could not parse "5.0.0-rc1", but because APP_VERSION
# cannot carry the suffix, so the tag and the assets named after APP_VERSION would
# disagree. Supporting release candidates means changing both together
start_test "a suffixed tag is rejected while APP_VERSION cannot carry a suffix"
assert_status 1 "$(run_check "5.0.0" "Mudlet-5.0.0-rc1")"

start_test "a PTB checks its version without a tag to compare against"
assert_status 0 "$(run_check "5.0.0")"
assert_status 1 "$(run_check "5.0")"
assert_contains "${OUT}" "three-component"

start_test "a missing or surplus argument is a usage error, not a silent pass"
assert_status 2 "$(run_check "")"
assert_status 2 "$(run_check)"
assert_status 2 "$(run_check "5.0.0" "Mudlet-5.0.0" "extra")"

start_test "the failure is annotated for GitHub Actions, not only printed"
GITHUB_ACTIONS=true bash "${CHECK}" "5.0.0" "Mudlet-5.0" > "${OUT}" 2>&1
assert_contains "${OUT}" "::error::"

# The guard is only worth anything if the release scripts actually reach it
start_test "the tag-push build validation calls the guard"
if ! command -v pcre2grep > /dev/null; then
  # Silence here would hide the only assertions that prove the wiring, and CI has
  # pcre2grep, so a missing one there is a broken runner rather than a local quirk
  if [ -n "${CI:-}${GITHUB_ACTIONS:-}" ]; then
    fail "pcre2grep is missing, so the build validation scripts cannot be exercised"
  else
    echo "    skipped: pcre2grep is not installed"
  fi
else
  mkdir -p "${WORK_DIR}/repo"
  printf 'set(APP_VERSION 5.0.0)\n' > "${WORK_DIR}/repo/CMakeLists.txt"

  for script in validate_deployment.sh validate-deployment-for-windows.sh; do
    (cd "${WORK_DIR}/repo" && GITHUB_REF="refs/tags/Mudlet-5.0.0" GITHUB_REPO_TAG=true WITH_UPDATER=YES \
      bash "${REPO_DIR}/CI/${script}") > "${OUT}" 2>&1
    assert_status 0 "$?"

    (cd "${WORK_DIR}/repo" && GITHUB_REF="refs/tags/Mudlet-5.0" GITHUB_REPO_TAG=true WITH_UPDATER=YES \
      bash "${REPO_DIR}/CI/${script}") > "${OUT}" 2>&1
    assert_status 1 "$?"
    assert_contains "${OUT}" "does not match APP_VERSION"
  done

  # The Windows script decides it is a release build from GITHUB_REPO_TAG but takes
  # the tag from GITHUB_REF, so the two can disagree. That is still a hard failure,
  # but it has to say what could not be determined rather than print a usage line
  (cd "${WORK_DIR}/repo" && GITHUB_REF="refs/heads/development" GITHUB_REPO_TAG=true WITH_UPDATER=YES \
    bash "${REPO_DIR}/CI/validate-deployment-for-windows.sh") > "${OUT}" 2>&1
  assert_status 1 "$?"
  assert_contains "${OUT}" "could not be determined"

  # A development build has no tag to check and must not be blocked by one. The two
  # scripts decide that from different variables, so both are exercised
  (cd "${WORK_DIR}/repo" && GITHUB_REF="refs/heads/development" WITH_UPDATER=YES \
    bash "${REPO_DIR}/CI/validate_deployment.sh") > "${OUT}" 2>&1
  assert_status 0 "$?"
  assert_contains "${OUT}" "skipping release validation"

  (cd "${WORK_DIR}/repo" && GITHUB_REF="refs/heads/development" GITHUB_REPO_TAG=false WITH_UPDATER=YES \
    bash "${REPO_DIR}/CI/validate-deployment-for-windows.sh") > "${OUT}" 2>&1
  assert_status 0 "$?"
  assert_contains "${OUT}" "skipping release validation"
fi

start_test "the release workflow calls the guard before it publishes anything"
# Blank out comments while keeping the line numbering, so a call that has merely
# been commented out cannot satisfy any of this
UNCOMMENTED="${WORK_DIR}/workflow-without-comments"
sed 's/^[[:space:]]*#.*$//' "${RELEASE_WORKFLOW}" > "${UNCOMMENTED}"

# The release-scripts/ prefix matters: the checkout of the tagged commit may predate
# the guard, so the copy taken from this workflow's own ref is the one to run
release_call='^[[:space:]]*bash release-scripts/CI/check-release-tag\.sh "\$\{VERSION\}" "\$\{REF#refs/tags/\}"[[:space:]]*$'
ptb_call='^[[:space:]]*bash release-scripts/CI/check-release-tag\.sh "\$\{VERSION\}"[[:space:]]*$'
publish_line=$(grep -n 'gh release create' "${UNCOMMENTED}" | head -1 | cut -d: -f1)

for description in "release:${release_call}" "PTB:${ptb_call}"; do
  guard_line=$(grep -nE "${description#*:}" "${UNCOMMENTED}" | head -1 | cut -d: -f1)
  if [ -z "${guard_line}" ]; then
    fail "create-github-release.yml no longer runs the guard on the ${description%%:*} path"
  elif [ -z "${publish_line}" ]; then
    fail "could not find the publishing step in create-github-release.yml"
  elif [ "${guard_line}" -ge "${publish_line}" ]; then
    fail "the ${description%%:*} guard at line ${guard_line} runs after the release is published at line ${publish_line}"
  fi
done

if [ "${FAILURES}" -gt 0 ]; then
  echo "${FAILURES} check(s) failed"
  exit 1
fi

echo "All checks passed"
