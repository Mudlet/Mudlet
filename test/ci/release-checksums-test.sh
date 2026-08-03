#!/bin/bash
# Tests the release checksum scripts against the asset sets that made Windows
# auto-update fail with "Could not verify the integrity of the download".
#
# The failure: the 2026-08-01 PTB ended up with five binaries but a
# SHA256SUMS.txt covering only four. The uncovered one was
# Mudlet-4.22.0-ptb-2026-08-01-dfdcb137-windows-64.exe - the very file the
# updater downloads on Windows - so it refused to install anything.
#
# The 2026-08-01 and 2026-08-03 filenames and hashes below are the real published
# ones; the stable-release and rebuild-counter cases further down are synthetic.

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
readonly INTEL_MAC_ASSET="${TAG}-x86_64.dmg"
readonly WINDOWS_ASSET="${TAG}-windows-64.exe"
# Only the Windows build appends a rebuild counter (CI/deploy-mudlet-for-windows.sh);
# a re-run also restamps the PTB date, which is what changes the tag prefix
readonly REBUILD_WINDOWS_ASSET='Mudlet-4.22.0-ptb-2026-08-02-dfdcb137rebuild2-windows-64.exe'

readonly LINUX_HASH='72a239146b07fc3b94f9e96955d2d6d52a6f0f40c8a5b7ee314b0bf875a55611'
readonly ARM64_HASH='7a7a75723e15a3443c4e8937fe2a85cee90b9dc8938e1e0580887e5c5e1fabbb'
readonly INTEL_MAC_HASH='d2426d7f799b619b0bfcb1e51e373f776288f77584bc9f08a79cc288cc58278c'
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
  if [[ ! -s "${file}" ]]; then
    fail "${file##*/} is empty or missing, so 'does not contain' proves nothing"
    return
  fi
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

run_prepare() {
  local case_dir="$1" tag="$2" type="$3"
  bash "${CI_DIR}/prepare-release-assets.sh" "${case_dir}/assets" "${tag}" "${type}" > "${case_dir}/prepare.log" 2>&1
}

run_assemble() {
  local case_dir="$1" published="$2"
  bash "${CI_DIR}/assemble-release-checksums.sh" "${case_dir}/assets" "${published}" > "${case_dir}/assemble.log" 2>&1
}

run_verify() {
  local case_dir="$1"
  shift
  bash "${CI_DIR}/verify-release-checksums.sh" "$@" > "${case_dir}/verify.log" 2>&1
}

expect_ok() {
  local what="$1" case_dir="$2" log="$3"
  if [[ "$4" -eq 0 ]]; then
    pass "${what} succeeded"
  else
    fail "${what} exited non-zero"
    cat "${case_dir}/${log}" >&2
  fi
}

expect_failure() {
  local what="$1" status="$2"
  if [[ "${status}" -ne 0 ]]; then
    pass "${what} rejected it"
  else
    fail "${what} accepted it"
  fi
}

# macOS has shasum rather than coreutils' sha256sum
sha256_of() {
  if command -v sha256sum > /dev/null 2>&1; then
    sha256sum "$1" | cut -d ' ' -f 1
  else
    shasum -a 256 "$1" | cut -d ' ' -f 1
  fi
}

# Creates a case directory with an assets/ subdirectory holding placeholder
# binaries and their .sha256 sidecars. Arguments are "name:hash" pairs; a hash of
# "real" is computed from the placeholder's actual bytes.
make_assets() {
  local case_dir="$1"
  shift
  mkdir -p "${case_dir}/assets"
  local pair name hash
  for pair in "$@"; do
    name="${pair%%:*}"
    hash="${pair##*:}"
    echo "placeholder for ${name}" > "${case_dir}/assets/${name}"
    if [[ "${hash}" == "real" ]]; then
      hash="$(sha256_of "${case_dir}/assets/${name}")"
    fi
    printf '%s *%s\n' "${hash}" "${name}" > "${case_dir}/assets/${name}.sha256"
  done
}

published_sums_from_the_complete_run() {
  # What run 30687252829 published: all four platforms of ${TAG}
  cat <<EOF
${LINUX_HASH}  ${LINUX_ASSET}
${INTEL_MAC_HASH}  ${INTEL_MAC_ASSET}
${ARM64_HASH}  ${ARM64_ASSET}
${WINDOWS_HASH} *${WINDOWS_ASSET}
EOF
}

#-----------------------------------------------------------------------------
start_test "the run that broke the 2026-08-01 PTB now keeps every binary covered"
# The 2026-08-02 run downloaded the Linux/macOS artifacts of ${TAG} but, because
# create-github-release.yml picks the newest successful run of each platform, the
# Windows installer of a later rebuild. It regenerated SHA256SUMS.txt from those
# four sidecars and clobbered the published file, dropping the entry for the
# ${WINDOWS_ASSET} that stayed on the release.
CASE="${WORK_DIR}/clobber"
make_assets "${CASE}" \
  "${REBUILD_WINDOWS_ASSET}:${REBUILD_WINDOWS_HASH}" \
  "${LINUX_ASSET}:${LINUX_HASH}" \
  "${ARM64_ASSET}:${ARM64_HASH}" \
  "${INTEL_MAC_ASSET}:${INTEL_MAC_HASH}"
published_sums_from_the_complete_run > "${CASE}/published-SHA256SUMS.txt"

run_prepare "${CASE}" "${TAG}" ptb
expect_ok "prepare-release-assets.sh" "${CASE}" prepare.log $?
assert_contains "${CASE}/prepare.log" "::warning::Ignoring 2 asset(s) from a different build"
if [[ -e "${CASE}/assets/${REBUILD_WINDOWS_ASSET}" ]]; then
  fail "the other build's installer was left in assets/"
else
  pass "the other build's installer was set aside"
fi

run_assemble "${CASE}" "${CASE}/published-SHA256SUMS.txt"
expect_ok "assemble-release-checksums.sh" "${CASE}" assemble.log $?
SUMS="${CASE}/assets/SHA256SUMS.txt"
# The regression: without merging, this file lost the ${WINDOWS_ASSET} entry
assert_contains "${SUMS}" "${WINDOWS_HASH}"
assert_contains "${SUMS}" "${WINDOWS_ASSET}"
assert_contains "${SUMS}" "${LINUX_ASSET}"
assert_contains "${SUMS}" "${ARM64_ASSET}"
assert_contains "${SUMS}" "${INTEL_MAC_ASSET}"
assert_absent "${SUMS}" "${REBUILD_WINDOWS_ASSET}"
assert_line_count "${SUMS}" 4

# The release afterwards holds what was already published plus what we upload
cat > "${CASE}/final-assets.txt" <<EOF
${ARM64_ASSET}
${LINUX_ASSET}
${WINDOWS_ASSET}
${INTEL_MAC_ASSET}
SHA256SUMS.txt
EOF
run_verify "${CASE}" "${SUMS}" "${CASE}/final-assets.txt"
expect_ok "verify-release-checksums.sh" "${CASE}" verify.log $?

#-----------------------------------------------------------------------------
start_test "an uncovered release binary is rejected"
# The exact state the 2026-08-01 PTB shipped in: SHA256SUMS.txt regenerated from
# one run's sidecars while an earlier run's Windows installer stayed published
CASE="${WORK_DIR}/uncovered"
mkdir -p "${CASE}"
cat > "${CASE}/SHA256SUMS.txt" <<EOF
${LINUX_HASH}  ${LINUX_ASSET}
${INTEL_MAC_HASH}  ${INTEL_MAC_ASSET}
${REBUILD_WINDOWS_HASH} *${REBUILD_WINDOWS_ASSET}
${ARM64_HASH}  ${ARM64_ASSET}
EOF
cat > "${CASE}/assets.txt" <<EOF
${ARM64_ASSET}
${LINUX_ASSET}
${WINDOWS_ASSET}
${INTEL_MAC_ASSET}
${REBUILD_WINDOWS_ASSET}
SHA256SUMS.txt
EOF
run_verify "${CASE}" "${CASE}/SHA256SUMS.txt" "${CASE}/assets.txt"
expect_failure "verify-release-checksums.sh" $?
assert_contains "${CASE}/verify.log" "${WINDOWS_ASSET}"
assert_contains "${CASE}/verify.log" "::error::"
# a blocked release cannot be repaired by re-running, so say what to do
assert_contains "${CASE}/verify.log" "Delete the stale asset from the release"

#-----------------------------------------------------------------------------
start_test "a binary whose bytes do not match its entry is rejected"
# The Windows job recomputes the sidecar after signing changes the bytes
# (build-mudlet-win.yml); if that ever stops happening, coverage alone would not
# notice and every user's updater would report a corrupt download
CASE="${WORK_DIR}/mismatch"
make_assets "${CASE}" "${WINDOWS_ASSET}:real"
run_assemble "${CASE}" ""
expect_ok "assemble-release-checksums.sh" "${CASE}" assemble.log $?
printf '%s\nSHA256SUMS.txt\n' "${WINDOWS_ASSET}" > "${CASE}/assets.txt"

run_verify "${CASE}" "${CASE}/assets/SHA256SUMS.txt" "${CASE}/assets.txt" "${CASE}/assets"
expect_ok "verify-release-checksums.sh on matching bytes" "${CASE}" verify.log $?

echo "tampered" > "${CASE}/assets/${WINDOWS_ASSET}"
run_verify "${CASE}" "${CASE}/assets/SHA256SUMS.txt" "${CASE}/assets.txt" "${CASE}/assets"
expect_failure "verify-release-checksums.sh on altered bytes" $?
assert_contains "${CASE}/verify.log" "do not match their SHA256SUMS.txt entry"

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
run_prepare "${CASE}" "${WINDOWS_ONLY_TAG}" ptb
expect_ok "prepare-release-assets.sh on a partial PTB" "${CASE}" prepare.log $?
assert_contains "${CASE}/prepare.log" "::warning::Missing release assets for: Linux (.AppImage.tar) macOS (arm64 .dmg) macOS (x86_64 .dmg)"
# the workflow always passes published/SHA256SUMS.txt, which does not exist on a
# release's first run
run_assemble "${CASE}" "${CASE}/does-not-exist-SHA256SUMS.txt"
expect_ok "assemble-release-checksums.sh with no published file" "${CASE}" assemble.log $?
assert_line_count "${CASE}/assets/SHA256SUMS.txt" 1
printf '%s\nSHA256SUMS.txt\n' "${WINDOWS_ONLY_ASSET}" > "${CASE}/assets.txt"
run_verify "${CASE}" "${CASE}/assets/SHA256SUMS.txt" "${CASE}/assets.txt"
expect_ok "verify-release-checksums.sh on the Windows-only PTB" "${CASE}" verify.log $?

#-----------------------------------------------------------------------------
start_test "a stable release missing a platform is rejected"
CASE="${WORK_DIR}/partial-stable"
STABLE_TAG='Mudlet-4.22.0'
make_assets "${CASE}" \
  "${STABLE_TAG}-windows-64-installer.exe:c22435c7ff0d36a5dbe5936bcfbbc173f656c2c33583dccbf17a7ad4ade3e350"
run_prepare "${CASE}" "${STABLE_TAG}" release
expect_failure "prepare-release-assets.sh on an incomplete stable release" $?
assert_contains "${CASE}/prepare.log" "::error::Stable release is missing assets for:"

#-----------------------------------------------------------------------------
start_test "a rebuild counter on this build's own installer is kept"
# When the tag comes from the Windows re-run itself, the counter is part of that
# build's own filename and must not make the installer look foreign
CASE="${WORK_DIR}/own-rebuild"
REBUILD_TAG='Mudlet-4.22.0-ptb-2026-08-02-dfdcb137'
make_assets "${CASE}" \
  "${REBUILD_TAG}rebuild2-windows-64.exe:${REBUILD_WINDOWS_HASH}" \
  "${REBUILD_TAG}-linux-x64.AppImage.tar:${LINUX_HASH}" \
  "${REBUILD_TAG}-arm64.dmg:${ARM64_HASH}" \
  "${REBUILD_TAG}-x86_64.dmg:${INTEL_MAC_HASH}"
run_prepare "${CASE}" "${REBUILD_TAG}" ptb
expect_ok "prepare-release-assets.sh" "${CASE}" prepare.log $?
assert_absent "${CASE}/prepare.log" "::warning::Ignoring"
if [[ -e "${CASE}/assets/${REBUILD_TAG}rebuild2-windows-64.exe" ]]; then
  pass "this build's own rebuilt installer was kept"
else
  fail "this build's own rebuilt installer was set aside"
fi

#-----------------------------------------------------------------------------
start_test "assets are matched to the tag case-insensitively"
# CI/set-build-info.sh lowercases VERSION, while a pushed git tag keeps its case
CASE="${WORK_DIR}/tag-case"
MIXED_CASE_TAG='Mudlet-4.23.0-RC1'
make_assets "${CASE}" \
  "mudlet-4.23.0-rc1-windows-64-installer.exe:${WINDOWS_HASH}" \
  "mudlet-4.23.0-rc1-linux-x64.AppImage.tar:${LINUX_HASH}" \
  "mudlet-4.23.0-rc1-arm64.dmg:${ARM64_HASH}" \
  "mudlet-4.23.0-rc1-x86_64.dmg:${INTEL_MAC_HASH}"
run_prepare "${CASE}" "${MIXED_CASE_TAG}" release
expect_ok "prepare-release-assets.sh on a mixed-case tag" "${CASE}" prepare.log $?
assert_absent "${CASE}/prepare.log" "::warning::Ignoring"

#-----------------------------------------------------------------------------
start_test "merging keeps the freshly built hash when a filename repeats"
CASE="${WORK_DIR}/rebuilt-same-name"
make_assets "${CASE}" "${WINDOWS_ASSET}:${REBUILD_WINDOWS_HASH}"
printf '%s *%s\n' "${WINDOWS_HASH}" "${WINDOWS_ASSET}" > "${CASE}/published-SHA256SUMS.txt"
run_assemble "${CASE}" "${CASE}/published-SHA256SUMS.txt"
expect_ok "assemble-release-checksums.sh" "${CASE}" assemble.log $?
assert_line_count "${CASE}/assets/SHA256SUMS.txt" 1
assert_contains "${CASE}/assets/SHA256SUMS.txt" "${REBUILD_WINDOWS_HASH}"
assert_absent "${CASE}/assets/SHA256SUMS.txt" "${WINDOWS_HASH}"

#-----------------------------------------------------------------------------
start_test "two different hashes for one filename are not resolved by guessing"
CASE="${WORK_DIR}/conflict"
mkdir -p "${CASE}/assets"
printf '%s *%s\n%s *%s\n' "${WINDOWS_HASH}" "${WINDOWS_ASSET}" "${REBUILD_WINDOWS_HASH}" "${WINDOWS_ASSET}" \
  > "${CASE}/published-SHA256SUMS.txt"
make_assets "${CASE}" "${LINUX_ASSET}:${LINUX_HASH}"
run_assemble "${CASE}" "${CASE}/published-SHA256SUMS.txt"
expect_failure "assemble-release-checksums.sh on conflicting entries" $?
assert_contains "${CASE}/assemble.log" "::error::Conflicting checksums for ${WINDOWS_ASSET}"

#-----------------------------------------------------------------------------
start_test "checksum lines survive tabs, CRLF and a missing trailing newline"
# None of our generators produce these, but each used to turn a cosmetic quirk into
# a release that could not be published
CASE="${WORK_DIR}/odd-formatting"
mkdir -p "${CASE}/assets"
printf '%s\t%s' "${WINDOWS_HASH}" "${WINDOWS_ASSET}" > "${CASE}/assets/${WINDOWS_ASSET}.sha256"
printf '%s *%s\r\n' "${LINUX_HASH}" "${LINUX_ASSET}" > "${CASE}/assets/${LINUX_ASSET}.sha256"
printf '%s  %s' "${ARM64_HASH}" "${ARM64_ASSET}" > "${CASE}/assets/${ARM64_ASSET}.sha256"
run_assemble "${CASE}" ""
expect_ok "assemble-release-checksums.sh on oddly formatted sidecars" "${CASE}" assemble.log $?
assert_line_count "${CASE}/assets/SHA256SUMS.txt" 3
assert_contains "${CASE}/assets/SHA256SUMS.txt" "${WINDOWS_HASH}  ${WINDOWS_ASSET}"
assert_contains "${CASE}/assets/SHA256SUMS.txt" "${LINUX_HASH} *${LINUX_ASSET}"
assert_contains "${CASE}/assets/SHA256SUMS.txt" "${ARM64_HASH}  ${ARM64_ASSET}"
printf '%s\n%s\n%s\nSHA256SUMS.txt\n' "${WINDOWS_ASSET}" "${LINUX_ASSET}" "${ARM64_ASSET}" > "${CASE}/assets.txt"
run_verify "${CASE}" "${CASE}/assets/SHA256SUMS.txt" "${CASE}/assets.txt"
expect_ok "verify-release-checksums.sh on the merged file" "${CASE}" verify.log $?

#-----------------------------------------------------------------------------
start_test "a sidecar that contributes nothing is an error"
CASE="${WORK_DIR}/empty-sidecar"
mkdir -p "${CASE}/assets"
echo "placeholder" > "${CASE}/assets/${WINDOWS_ASSET}"
: > "${CASE}/assets/${WINDOWS_ASSET}.sha256"
run_assemble "${CASE}" ""
expect_failure "assemble-release-checksums.sh on an empty sidecar" $?
assert_contains "${CASE}/assemble.log" "contributed no checksum entry"

#-----------------------------------------------------------------------------
start_test "an unreadable checksum file is reported as such, not as missing entries"
CASE="${WORK_DIR}/unreadable-sums"
mkdir -p "${CASE}"
echo "<html>503 Service Unavailable</html>" > "${CASE}/SHA256SUMS.txt"
printf '%s\nSHA256SUMS.txt\n' "${WINDOWS_ASSET}" > "${CASE}/assets.txt"
run_verify "${CASE}" "${CASE}/SHA256SUMS.txt" "${CASE}/assets.txt"
expect_failure "verify-release-checksums.sh on an unreadable checksum file" $?
assert_contains "${CASE}/verify.log" "contains no usable checksum entries"

#-----------------------------------------------------------------------------
start_test "an unrecognised asset type still has to be covered"
# The gate must not exempt a file just because its suffix is new
CASE="${WORK_DIR}/new-asset-type"
mkdir -p "${CASE}"
printf '%s *%s\n' "${WINDOWS_HASH}" "${WINDOWS_ASSET}" > "${CASE}/SHA256SUMS.txt"
printf '%s\nMudlet-4.22.0-linux-x64-portable.tar.gz\nSHA256SUMS.txt\n' "${WINDOWS_ASSET}" > "${CASE}/assets.txt"
run_verify "${CASE}" "${CASE}/SHA256SUMS.txt" "${CASE}/assets.txt"
expect_failure "verify-release-checksums.sh on an uncovered new asset type" $?
assert_contains "${CASE}/verify.log" "Mudlet-4.22.0-linux-x64-portable.tar.gz"

#-----------------------------------------------------------------------------
start_test "no sidecars at all is an error"
CASE="${WORK_DIR}/no-sidecars"
mkdir -p "${CASE}/assets"
run_assemble "${CASE}" ""
expect_failure "assemble-release-checksums.sh on an empty assets directory" $?
assert_contains "${CASE}/assemble.log" "::error::No .sha256 checksum files found"

#-----------------------------------------------------------------------------
echo
if [[ ${FAILURES} -gt 0 ]]; then
  echo "${FAILURES} check(s) FAILED"
  exit 1
fi
echo "All checks passed"
