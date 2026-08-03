#!/bin/bash
# Fails if any release binary lacks a SHA256SUMS.txt entry.
#
# Mudlet's updater refuses to install a download it cannot verify, so a release
# binary without a checksum line is not installable - see
# assemble-release-checksums.sh for how that used to happen. This is the gate that
# keeps it from being published.
#
# Usage: verify-release-checksums.sh <sums-file> <asset-names-file>
#   asset-names-file lists one asset filename per line: every release binary among
#   them must have an entry in sums-file.

set -euo pipefail

SUMS_FILE="${1:?SHA256SUMS.txt path required}"
ASSET_NAMES_FILE="${2:?asset names file required}"

readonly BINARY_SUFFIXES=('.AppImage.tar' '.dmg' '.exe')

if [[ ! -f "${SUMS_FILE}" ]]; then
  echo "::error::${SUMS_FILE} does not exist - cannot verify release checksum coverage"
  exit 1
fi

is_release_binary() {
  local name="$1"
  local suffix
  for suffix in "${BINARY_SUFFIXES[@]}"; do
    if [[ "${name}" == *"${suffix}" ]]; then
      return 0
    fi
  done
  return 1
}

# Filenames covered by the checksum file, one per line
COVERED="$(mktemp)"
trap 'rm -f "${COVERED}"' EXIT
while IFS= read -r line || [[ -n "${line}" ]]; do
  if [[ "${line}" =~ ^([0-9a-fA-F]{64})[[:space:]]+\*?(.+)$ ]]; then
    printf '%s\n' "${BASH_REMATCH[2]}" >> "${COVERED}"
  fi
done < "${SUMS_FILE}"

UNCOVERED=()
CHECKED=0
while IFS= read -r asset_name || [[ -n "${asset_name}" ]]; do
  if [[ -z "${asset_name}" ]] || ! is_release_binary "${asset_name}"; then
    continue
  fi
  CHECKED=$((CHECKED + 1))
  if ! grep -qxF "${asset_name}" "${COVERED}"; then
    UNCOVERED+=("${asset_name}")
  fi
done < "${ASSET_NAMES_FILE}"

if [[ ${#UNCOVERED[@]} -gt 0 ]]; then
  echo "::error::${#UNCOVERED[@]} release binary/binaries have no SHA256SUMS.txt entry, so Mudlet's updater cannot install them: ${UNCOVERED[*]}"
  echo "SHA256SUMS.txt covers:"
  sort "${COVERED}"
  exit 1
fi

if [[ ${CHECKED} -eq 0 ]]; then
  echo "::error::No release binaries found to verify - expected at least one"
  exit 1
fi

echo "All ${CHECKED} release binary/binaries have a SHA256SUMS.txt entry"
