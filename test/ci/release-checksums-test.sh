#!/bin/bash
# Tests the release checksum scripts against the asset sets that made Windows
# auto-update fail with "Could not verify the integrity of the download".
#
# The failure: the 2026-08-01 PTB ended up with five binaries but a
# SHA256SUMS.txt covering only four. The uncovered one was
# Mudlet-4.22.0-ptb-2026-08-01-dfdcb137-windows-64.exe - the very file the
# updater downloads on Windows - so it refused to install anything. Hashes and
# filenames below are the real ones from that release and from the
# create-github-release runs that produced it.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CI_DIR="$(cd "${SCRIPT_DIR}/../../CI" && pwd)"

WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

FAILURES=0
CURRENT_TEST=""

readonly TAG='Mudlet-4.22.0-ptb-2026-08-01-dfdcb137'
readonly LINUX_ASSET="${TAG}-linux-x64.AppImage.tar"
readonly ARM64_ASSET="${TAG}-arm64.dmg"
readonly X86_64_ASSET="${TAG}-x86_64.dmg"
readonly WINDOWS_ASSET="${TAG}-windows-64.exe"
# A Windows re-run restamps the build date and appends a rebuild counter, so its
# installer is published under a name that does not belong to ${TAG}
readonly REBUILD_WINDOWS_ASSET='Mudlet-4.22.0-ptb-2026-08-02-dfdcb137rebuild2-windows-64.exe'

readonly LINUX_HASH='72a239146b07fc3b94f9e96955d2d6d52a6f0f40c8a5b7ee314b0bf875a55611'
readonly ARM64_HASH='7a7a75723e15a3443c4e8937fe2a85cee90b9dc8938e1e0580887e5c5e1fabbb'
readonly X86_64_HASH='d2426d7f799b619b0bfcb1e51e373f776288f77584bc9f08a79cc288cc58278c'
readonly WINDOWS_HASH='72dba076741a245a994b553b1cf88dba5d7f5bb07f0d6887ecd58361402764e1'
readonly REBUILD_WINDOWS_HASH='9b3895247937d37f645dd31e7ded739e3126ccad6bfa4188cdd3b78f735c2f6f'

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

assert_contains() {
  local file="$1" needle="$2"
  if grep -qF -- "${needle}" "${file}"; then
    pass "${file##*/} contains ${needle}"
  else
    fail "${file##*/} is missing ${needle}"
    echo "--- ${file} ---" >&2
    cat "${file}" >&2
  fi
}

assert_absent() {
  local file="$1" needle="$2"
  if grep -qF -- "${needle}" "${file}"; then
    fail "${file##*/} unexpectedly contains ${needle}"
  else
    pass "${file##*/} does not contain ${needle}"
  fi
}

assert_line_count() {
  local file="$1" expected="$2" actual
  actual="$(wc -l < "${file}" | tr -d ' ')"
  if [[ "${actual}" == "${expected}" ]]; then
    pass "${file##*/} has ${expected} entries"
  else
    fail "${file##*/} has ${actual} entries, expected ${expected}"
    cat "${file}" >&2
  fi
}

# Creates a case directory with an assets/ subdirectory holding placeholder
# binaries and their .sha256 sidecars. Arguments are "name:hash" pairs.
make_assets() {
  local case_dir="$1"
  shift
  mkdir -p "${case_dir}/assets"
  local pair name hash
  for pair in "$@"; do
    name="${pair%%:*}"
    hash="${pair##*:}"
    echo "placeholder for ${name}" > "${case_dir}/assets/${name}"
    printf '%s *%s\n' "${hash}" "${name}" > "${case_dir}/assets/${name}.sha256"
  done
}

published_sums_from_the_complete_run() {
  # What run 30687252829 published: all four platforms of ${TAG}
  cat <<EOF
${LINUX_HASH}  ${LINUX_ASSET}
${X86_64_HASH}  ${X86_64_ASSET}
${ARM64_HASH}  ${ARM64_ASSET}
${WINDOWS_HASH} *${WINDOWS_ASSET}
EOF
}

#-----------------------------------------------------------------------------
start_test "the run that broke the 2026-08-01 PTB now keeps every binary covered"
# Run 30734870982 downloaded the Linux/macOS artifacts of ${TAG} but, because it
# picks the newest successful Windows run, the Windows installer of a later
# rebuild. It regenerated SHA256SUMS.txt from those four sidecars and clobbered
# the published file, dropping the entry for the ${WINDOWS_ASSET} that stayed on
# the release.
CASE="${WORK_DIR}/clobber"
make_assets "${CASE}" \
  "${REBUILD_WINDOWS_ASSET}:${REBUILD_WINDOWS_HASH}" \
  "${LINUX_ASSET}:${LINUX_HASH}" \
  "${ARM64_ASSET}:${ARM64_HASH}" \
  "${X86_64_ASSET}:${X86_64_HASH}"
published_sums_from_the_complete_run > "${CASE}/published-SHA256SUMS.txt"

if ! bash "${CI_DIR}/prepare-release-assets.sh" "${CASE}/assets" "${TAG}" ptb > "${CASE}/prepare.log" 2>&1; then
  fail "prepare-release-assets.sh exited non-zero"
  cat "${CASE}/prepare.log" >&2
else
  pass "prepare-release-assets.sh succeeded"
fi
assert_contains "${CASE}/prepare.log" "::warning::Ignoring 2 asset(s) from a different build"
if [[ -e "${CASE}/assets/${REBUILD_WINDOWS_ASSET}" ]]; then
  fail "the other build's installer was left in assets/"
else
  pass "the other build's installer was set aside"
fi

if ! bash "${CI_DIR}/assemble-release-checksums.sh" "${CASE}/assets" "${CASE}/published-SHA256SUMS.txt" > "${CASE}/assemble.log" 2>&1; then
  fail "assemble-release-checksums.sh exited non-zero"
  cat "${CASE}/assemble.log" >&2
else
  pass "assemble-release-checksums.sh succeeded"
fi
SUMS="${CASE}/assets/SHA256SUMS.txt"
# The regression: without merging, this file lost the ${WINDOWS_ASSET} entry
assert_contains "${SUMS}" "${WINDOWS_HASH}"
assert_contains "${SUMS}" "${WINDOWS_ASSET}"
assert_contains "${SUMS}" "${LINUX_ASSET}"
assert_contains "${SUMS}" "${ARM64_ASSET}"
assert_contains "${SUMS}" "${X86_64_ASSET}"
assert_absent "${SUMS}" "${REBUILD_WINDOWS_ASSET}"
assert_line_count "${SUMS}" 4

# The release afterwards holds what was already published plus what we upload
cat > "${CASE}/final-assets.txt" <<EOF
${ARM64_ASSET}
${LINUX_ASSET}
${WINDOWS_ASSET}
${X86_64_ASSET}
SHA256SUMS.txt
EOF
if bash "${CI_DIR}/verify-release-checksums.sh" "${SUMS}" "${CASE}/final-assets.txt" > "${CASE}/verify.log" 2>&1; then
  pass "verify-release-checksums.sh accepts the merged release"
else
  fail "verify-release-checksums.sh rejected a fully covered release"
  cat "${CASE}/verify.log" >&2
fi

#-----------------------------------------------------------------------------
start_test "an uncovered release binary is rejected"
# The exact state the 2026-08-01 PTB shipped in: SHA256SUMS.txt regenerated from
# one run's sidecars while an earlier run's Windows installer stayed published
CASE="${WORK_DIR}/uncovered"
mkdir -p "${CASE}"
cat > "${CASE}/SHA256SUMS.txt" <<EOF
${LINUX_HASH}  ${LINUX_ASSET}
${X86_64_HASH}  ${X86_64_ASSET}
${REBUILD_WINDOWS_HASH} *${REBUILD_WINDOWS_ASSET}
${ARM64_HASH}  ${ARM64_ASSET}
EOF
cat > "${CASE}/assets.txt" <<EOF
${ARM64_ASSET}
${LINUX_ASSET}
${WINDOWS_ASSET}
${X86_64_ASSET}
${REBUILD_WINDOWS_ASSET}
SHA256SUMS.txt
EOF
if bash "${CI_DIR}/verify-release-checksums.sh" "${CASE}/SHA256SUMS.txt" "${CASE}/assets.txt" > "${CASE}/verify.log" 2>&1; then
  fail "verify-release-checksums.sh accepted a release whose Windows installer has no checksum"
  cat "${CASE}/verify.log" >&2
else
  pass "verify-release-checksums.sh rejected the release"
fi
assert_contains "${CASE}/verify.log" "${WINDOWS_ASSET}"
assert_contains "${CASE}/verify.log" "::error::"

#-----------------------------------------------------------------------------
start_test "a Windows-only PTB is published with its one checksum"
# The 2026-08-03 PTB: the Linux/macOS build had not succeeded, so only the
# Windows installer was available. Partial PTBs are allowed, but the installer
# still has to be covered.
CASE="${WORK_DIR}/windows-only"
WINDOWS_ONLY_TAG='Mudlet-4.22.0-ptb-2026-08-03-3474cb58'
WINDOWS_ONLY_ASSET="${WINDOWS_ONLY_TAG}-windows-64.exe"
make_assets "${CASE}" \
  "${WINDOWS_ONLY_ASSET}:c22435c7ff0d36a5dbe5936bcfbbc173f656c2c33583dccbf17a7ad4ade3e350"
if bash "${CI_DIR}/prepare-release-assets.sh" "${CASE}/assets" "${WINDOWS_ONLY_TAG}" ptb > "${CASE}/prepare.log" 2>&1; then
  pass "prepare-release-assets.sh allows a partial PTB"
else
  fail "prepare-release-assets.sh rejected a partial PTB"
  cat "${CASE}/prepare.log" >&2
fi
assert_contains "${CASE}/prepare.log" "::warning::Missing release assets for: Linux (.AppImage.tar) macOS (.dmg)"
bash "${CI_DIR}/assemble-release-checksums.sh" "${CASE}/assets" "" > "${CASE}/assemble.log" 2>&1
assert_line_count "${CASE}/assets/SHA256SUMS.txt" 1
printf '%s\nSHA256SUMS.txt\n' "${WINDOWS_ONLY_ASSET}" > "${CASE}/assets.txt"
if bash "${CI_DIR}/verify-release-checksums.sh" "${CASE}/assets/SHA256SUMS.txt" "${CASE}/assets.txt" > "${CASE}/verify.log" 2>&1; then
  pass "the Windows-only PTB passes verification"
else
  fail "the Windows-only PTB was rejected"
  cat "${CASE}/verify.log" >&2
fi

#-----------------------------------------------------------------------------
start_test "a stable release missing a platform is rejected"
CASE="${WORK_DIR}/partial-stable"
STABLE_TAG='Mudlet-4.22.0'
make_assets "${CASE}" \
  "${STABLE_TAG}-windows-64-installer.exe:c22435c7ff0d36a5dbe5936bcfbbc173f656c2c33583dccbf17a7ad4ade3e350"
if bash "${CI_DIR}/prepare-release-assets.sh" "${CASE}/assets" "${STABLE_TAG}" release > "${CASE}/prepare.log" 2>&1; then
  fail "prepare-release-assets.sh published an incomplete stable release"
else
  pass "prepare-release-assets.sh rejected the incomplete stable release"
fi
assert_contains "${CASE}/prepare.log" "::error::Stable release is missing assets for:"

#-----------------------------------------------------------------------------
start_test "a rebuild counter on this build's own assets is kept"
# A re-run of both platforms tags itself with the rebuild counter too, so the
# counter alone must not make an asset look foreign
CASE="${WORK_DIR}/own-rebuild"
REBUILD_TAG='Mudlet-4.22.0-ptb-2026-08-02-dfdcb137'
make_assets "${CASE}" \
  "${REBUILD_TAG}rebuild2-windows-64.exe:${REBUILD_WINDOWS_HASH}" \
  "${REBUILD_TAG}-linux-x64.AppImage.tar:${LINUX_HASH}" \
  "${REBUILD_TAG}-arm64.dmg:${ARM64_HASH}"
if bash "${CI_DIR}/prepare-release-assets.sh" "${CASE}/assets" "${REBUILD_TAG}" ptb > "${CASE}/prepare.log" 2>&1; then
  pass "prepare-release-assets.sh succeeded"
else
  fail "prepare-release-assets.sh exited non-zero"
  cat "${CASE}/prepare.log" >&2
fi
assert_absent "${CASE}/prepare.log" "::warning::Ignoring"
if [[ -e "${CASE}/assets/${REBUILD_TAG}rebuild2-windows-64.exe" ]]; then
  pass "this build's own rebuilt installer was kept"
else
  fail "this build's own rebuilt installer was set aside"
fi

#-----------------------------------------------------------------------------
start_test "merging keeps the freshly built hash when a filename repeats"
CASE="${WORK_DIR}/rebuilt-same-name"
make_assets "${CASE}" "${WINDOWS_ASSET}:${REBUILD_WINDOWS_HASH}"
printf '%s *%s\n' "${WINDOWS_HASH}" "${WINDOWS_ASSET}" > "${CASE}/published-SHA256SUMS.txt"
bash "${CI_DIR}/assemble-release-checksums.sh" "${CASE}/assets" "${CASE}/published-SHA256SUMS.txt" > "${CASE}/assemble.log" 2>&1
assert_line_count "${CASE}/assets/SHA256SUMS.txt" 1
assert_contains "${CASE}/assets/SHA256SUMS.txt" "${REBUILD_WINDOWS_HASH}"
assert_absent "${CASE}/assets/SHA256SUMS.txt" "${WINDOWS_HASH}"

#-----------------------------------------------------------------------------
start_test "no sidecars at all is an error"
CASE="${WORK_DIR}/no-sidecars"
mkdir -p "${CASE}/assets"
if bash "${CI_DIR}/assemble-release-checksums.sh" "${CASE}/assets" "" > "${CASE}/assemble.log" 2>&1; then
  fail "assemble-release-checksums.sh accepted an empty assets directory"
else
  pass "assemble-release-checksums.sh rejected an empty assets directory"
fi
assert_contains "${CASE}/assemble.log" "::error::No .sha256 checksum files found"

#-----------------------------------------------------------------------------
echo
if [[ ${FAILURES} -gt 0 ]]; then
  echo "${FAILURES} check(s) FAILED"
  exit 1
fi
echo "All checks passed"
