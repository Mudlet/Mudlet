#!/bin/bash
# Fails if any release binary lacks a SHA256SUMS.txt entry, or if a binary we still
# have on disk does not hash to the value listed for it.
#
# The updater's download path refuses a download it cannot verify (see
# UpdateDialog::startDownload, which passes requireChecksums), so a release binary
# without a checksum line cannot be installed through the update dialog - see
# assemble-release-checksums.sh for how that used to happen. This is the gate that
# keeps it from being published.
#
# The check covers assets already on the release, not just the ones being uploaded,
# because those are what a user's updater will see. A release that is *already*
# missing a checksum therefore fails every later run of the publishing job and
# cannot be repaired by re-running it: the sidecar for an older run's binary is no
# longer downloadable, so the stale asset has to be deleted from the release (or
# its platform build re-run) by hand.
#
# Usage: verify-release-checksums.sh <sums-file> <asset-names-file> [assets-dir]
#   asset-names-file lists one asset filename per line: every release binary among
#     them must have an entry in sums-file.
#   assets-dir, when given, is searched for each of those binaries, and any that is
#     found has its actual SHA256 compared against the listed one.

set -euo pipefail

SUMS_FILE="${1:?SHA256SUMS.txt path required}"
ASSET_NAMES_FILE="${2:?asset names file required}"
ASSETS_DIR="${3:-}"

if [[ ! -f "${SUMS_FILE}" ]]; then
  echo "::error::${SUMS_FILE} does not exist - cannot verify release checksum coverage"
  exit 1
fi

# Everything published alongside the binaries, rather than a list of binary
# suffixes: an unrecognised suffix has to be treated as a binary that needs
# checking, or adding an asset type would silently exempt it
is_release_binary() {
  local name="$1"
  [[ "${name}" != "SHA256SUMS.txt" && "${name}" != *.sha256 ]]
}

sha256_of() {
  if command -v sha256sum > /dev/null 2>&1; then
    sha256sum "$1" | cut -d ' ' -f 1
  else
    shasum -a 256 "$1" | cut -d ' ' -f 1
  fi
}

# "<filename>\t<hash>" for every entry in the checksum file
COVERED="$(mktemp)"
trap 'rm -f "${COVERED}"' EXIT
while IFS= read -r line || [[ -n "${line}" ]]; do
  line="${line%$'\r'}"
  if [[ "${line}" =~ ^([0-9a-fA-F]{64})[[:space:]]+\*?(.+)$ ]]; then
    printf '%s\t%s\n' "${BASH_REMATCH[2]}" "${BASH_REMATCH[1]}" >> "${COVERED}"
  fi
done < "${SUMS_FILE}"

if [[ ! -s "${COVERED}" ]]; then
  echo "::error::${SUMS_FILE} contains no usable checksum entries - it is empty or was not downloaded correctly"
  exit 1
fi

UNCOVERED=()
MISMATCHED=()
CHECKED=0
while IFS= read -r asset_name || [[ -n "${asset_name}" ]]; do
  asset_name="${asset_name%$'\r'}"
  if [[ -z "${asset_name}" ]] || ! is_release_binary "${asset_name}"; then
    continue
  fi
  CHECKED=$((CHECKED + 1))

  expected="$(awk -F '\t' -v name="${asset_name}" '$1 == name { print $2; exit }' "${COVERED}")"
  if [[ -z "${expected}" ]]; then
    UNCOVERED+=("${asset_name}")
    continue
  fi

  if [[ -z "${ASSETS_DIR}" ]]; then
    continue
  fi
  asset_path="$(find "${ASSETS_DIR}" -name "${asset_name}" -type f -print -quit 2> /dev/null || true)"
  if [[ -z "${asset_path}" ]]; then
    continue
  fi
  actual="$(sha256_of "${asset_path}")"
  if [[ "${actual}" != "${expected}" ]]; then
    MISMATCHED+=("${asset_name} (listed ${expected}, actual ${actual})")
  fi
done < "${ASSET_NAMES_FILE}"

if [[ ${#UNCOVERED[@]} -gt 0 ]]; then
  echo "::error::${#UNCOVERED[@]} release binary/binaries have no SHA256SUMS.txt entry, so Mudlet's updater cannot install them: ${UNCOVERED[*]}"
  echo "Delete the stale asset from the release, or re-run the platform build that produced it, then re-run this job."
  echo "SHA256SUMS.txt covers:"
  cut -f 1 "${COVERED}" | sort
  exit 1
fi

if [[ ${#MISMATCHED[@]} -gt 0 ]]; then
  echo "::error::${#MISMATCHED[@]} release binary/binaries do not match their SHA256SUMS.txt entry, so Mudlet's updater would reject the download: ${MISMATCHED[*]}"
  exit 1
fi

if [[ ${CHECKED} -eq 0 ]]; then
  echo "::error::No release binaries found to verify - expected at least one"
  exit 1
fi

echo "All ${CHECKED} release binary/binaries have a SHA256SUMS.txt entry"
