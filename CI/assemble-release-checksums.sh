#!/bin/bash
# Assembles SHA256SUMS.txt for a GitHub Release from the per-platform .sha256
# sidecar files, merged over the SHA256SUMS.txt already published on the release.
#
# Merging matters because create-github-release.yml runs once per platform build
# workflow and uploads with --clobber. A run that sees fewer platforms than an
# earlier run - or a re-run of one platform whose binary is named differently -
# used to regenerate SHA256SUMS.txt from its own subset of sidecars and overwrite
# the complete file, while the binaries from the earlier run stayed published. That
# left release binaries with no checksum line, which the updater's download path
# refuses to install (see Feed::findChecksum and UpdateDialog::startDownload).
# Entries are keyed by filename and the freshly built sidecars win, so the
# published file only ever gains coverage.
#
# This is a read-modify-write of a file shared by both platform triggers; it is
# only safe because create-github-release.yml serialises them with a `concurrency`
# group keyed on the build's head_sha.
#
# Usage: assemble-release-checksums.sh <assets-dir> [published-sums-file]

set -euo pipefail

ASSETS_DIR="${1:?assets directory required}"
PUBLISHED_SUMS="${2:-}"

OUTPUT="${ASSETS_DIR%/}/SHA256SUMS.txt"

# no mapfile: macOS ships bash 3.2, and test/ci/release-checksums-test.sh runs
# these scripts under ctest there
SHA_FILES=()
while IFS= read -r sha_file; do
  SHA_FILES+=("${sha_file}")
done < <(find "${ASSETS_DIR}" -name '*.sha256' -type f | sort)

if [[ ${#SHA_FILES[@]} -eq 0 ]]; then
  echo "::error::No .sha256 checksum files found in ${ASSETS_DIR}"
  exit 1
fi

RECORDS="$(mktemp)"
trap 'rm -f "${RECORDS}"' EXIT

# Appends "<filename>\t<priority>\t<hash>\t<binary marker>" to ${RECORDS} for
# every checksum line on stdin, so the lines can be deduplicated by filename with
# the lowest priority number winning. The output line is rebuilt from these fields
# rather than carried through verbatim, because a checksum line may itself contain
# a tab and would then be truncated by the tab-delimited dedupe.
collect() {
  local priority="$1"
  local source_label="$2"
  local line
  while IFS= read -r line || [[ -n "${line}" ]]; do
    line="${line%$'\r'}"
    if [[ -z "${line}" ]]; then
      continue
    fi
    # sha256sum writes "<hash>  <name>" in text mode and "<hash> *<name>" in
    # binary mode; MSYS2's defaults to binary, so the Windows entry uses " *"
    if [[ ! "${line}" =~ ^([0-9a-fA-F]{64})[[:space:]]+(\*?)(.+)$ ]]; then
      echo "::warning::Ignoring unparseable checksum line from ${source_label}: ${line}"
      continue
    fi
    printf '%s\t%s\t%s\t%s\n' "${BASH_REMATCH[3]}" "${priority}" "${BASH_REMATCH[1]}" "${BASH_REMATCH[2]}" >> "${RECORDS}"
  done
}

record_count() {
  wc -l < "${RECORDS}" | tr -d ' '
}

if [[ -n "${PUBLISHED_SUMS}" && -f "${PUBLISHED_SUMS}" ]]; then
  echo "Merging over the SHA256SUMS.txt already published on the release"
  collect 2 "the published SHA256SUMS.txt" < "${PUBLISHED_SUMS}"
fi

# one sidecar at a time: concatenating them would join two records whenever a
# sidecar lacks a trailing newline
for sha_file in "${SHA_FILES[@]}"; do
  before="$(record_count)"
  sidecar_name="$(basename "${sha_file}")"
  collect 1 "${sidecar_name}" < "${sha_file}"
  if [[ "$(record_count)" -eq "${before}" ]]; then
    echo "::error::${sha_file} contributed no checksum entry, so the binary it describes would be published uncovered"
    exit 1
  fi
done

if [[ ! -s "${RECORDS}" ]]; then
  echo "::error::No valid checksum lines found in ${SHA_FILES[*]} ${PUBLISHED_SUMS}"
  exit 1
fi

# First record per filename wins after sorting by filename then priority, so a
# freshly built sidecar (1) beats an already published entry (2). Two different
# hashes for one filename at the winning priority mean the inputs disagree, which
# must not be resolved by picking one arbitrarily.
if ! sort -t $'\t' -k1,1 -k2,2n "${RECORDS}" | awk -F '\t' '
{
  filename = $1; priority = $2 + 0; hash = $3; marker = $4
  if (!(filename in winningPriority)) {
    winningPriority[filename] = priority
    winningHash[filename] = hash
    winningMarker[filename] = marker
    order[++count] = filename
  } else if (priority == winningPriority[filename] && hash != winningHash[filename]) {
    conflicting[filename] = 1
  }
}
END {
  failed = 0
  for (i = 1; i <= count; i++) {
    filename = order[i]
    if (filename in conflicting) {
      printf "::error::Conflicting checksums for %s - refusing to guess which is current\n", filename > "/dev/stderr"
      failed = 1
      continue
    }
    separator = (winningMarker[filename] == "*") ? " *" : "  "
    printf "%s%s%s\n", winningHash[filename], separator, filename
  }
  exit failed
}' > "${OUTPUT}"; then
  exit 1
fi

echo "Generated SHA256SUMS.txt ($(wc -l < "${OUTPUT}" | tr -d ' ') entries):"
cat "${OUTPUT}"
