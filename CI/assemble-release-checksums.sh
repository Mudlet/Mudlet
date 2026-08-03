#!/bin/bash
# Assembles SHA256SUMS.txt for a GitHub Release from the per-platform .sha256
# sidecar files, merged over the SHA256SUMS.txt already published on the release.
#
# Merging matters because create-github-release.yml runs once per platform build
# workflow and uploads with --clobber. A run that sees fewer platforms than an
# earlier run - or a re-run of one platform whose binary is named differently -
# used to regenerate SHA256SUMS.txt from its own subset of sidecars and overwrite
# the complete file, while the binaries from the earlier run stayed published.
# That left release binaries with no checksum line, which Mudlet's updater rejects
# with "Could not verify the integrity of the download". Entries are keyed by
# filename and the freshly built sidecars win, so the published file only ever
# gains coverage.
#
# Usage: assemble-release-checksums.sh <assets-dir> [published-sums-file]

set -euo pipefail

ASSETS_DIR="${1:?assets directory required}"
PUBLISHED_SUMS="${2:-}"

OUTPUT="${ASSETS_DIR%/}/SHA256SUMS.txt"

mapfile -t SHA_FILES < <(find "${ASSETS_DIR}" -name '*.sha256' -type f | sort)

if [[ ${#SHA_FILES[@]} -eq 0 ]]; then
  echo "::error::No .sha256 checksum files found in ${ASSETS_DIR}"
  exit 1
fi

RECORDS="$(mktemp)"
trap 'rm -f "${RECORDS}"' EXIT

# Emits "<filename>\t<priority>\t<line>" for every checksum line on stdin, so the
# lines can be deduplicated by filename with the lowest priority winning.
collect() {
  local priority="$1"
  local line hash filename
  while IFS= read -r line || [[ -n "${line}" ]]; do
    # sha256sum writes "<hash>  <name>" in text mode and "<hash> *<name>" in binary mode
    if [[ ! "${line}" =~ ^([0-9a-fA-F]{64})[[:space:]]+\*?(.+)$ ]]; then
      continue
    fi
    hash="${BASH_REMATCH[1]}"
    filename="${BASH_REMATCH[2]}"
    printf '%s\t%s\t%s\n' "${filename}" "${priority}" "${line}" >> "${RECORDS}"
  done
}

if [[ -n "${PUBLISHED_SUMS}" && -f "${PUBLISHED_SUMS}" ]]; then
  echo "Merging over the SHA256SUMS.txt already published on the release"
  collect 2 < "${PUBLISHED_SUMS}"
fi

cat "${SHA_FILES[@]}" | collect 1

if [[ ! -s "${RECORDS}" ]]; then
  echo "::error::No valid checksum lines found in ${SHA_FILES[*]} ${PUBLISHED_SUMS}"
  exit 1
fi

sort -t $'\t' -k1,1 -k2,2n "${RECORDS}" | awk -F '\t' '!seen[$1]++ { print $3 }' > "${OUTPUT}"

echo "Generated SHA256SUMS.txt ($(wc -l < "${OUTPUT}") entries):"
cat "${OUTPUT}"
