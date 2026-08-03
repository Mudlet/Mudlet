#!/bin/bash
# Validates the assets downloaded from the platform build runs before they are
# published to a GitHub Release, and sets aside any that do not belong.
#
# A PTB tag is "Mudlet-<VERSION><-ptb-DATE>-<COMMIT>" and its asset filenames are
# built from the same values, so they share that prefix; a stable release tag is
# the pushed git tag, which spells "Mudlet-<VERSION>" and so shares the prefix
# too. Anything else came from a different build: the two platform build workflows
# can be re-run independently, a re-run recomputes the PTB date and so carries a
# newer prefix than the tag, and create-github-release.yml always pairs the newest
# successful run of each platform. Publishing such a file would put a binary from
# build B into the release for build A, whose in-app version does not match the
# release tag - and would leave the release holding a binary that no SHA256SUMS.txt
# line can cover, because its checksum sidecar belongs to the other build.
#
# Setting an asset aside does not remove anything already published: a binary from
# another build that an earlier run uploaded stays on the release, and stays
# covered because assemble-release-checksums.sh merges its published entry
# forward.
#
# Usage: prepare-release-assets.sh <assets-dir> <release-tag> <release-type>

set -euo pipefail

ASSETS_DIR="${1:?assets directory required}"
RELEASE_TAG="${2:?release tag required}"
RELEASE_TYPE="${3:?release type required}"

mkdir -p "${ASSETS_DIR}"

echo "Downloaded assets:"
find "${ASSETS_DIR}" -type f | sort

# Everything published alongside the binaries, rather than a list of binary
# suffixes: an unrecognised suffix has to be treated as a binary that needs
# checking, or adding an asset type would silently exempt it
is_release_binary() {
  local name="$1"
  [[ "${name}" != "SHA256SUMS.txt" && "${name}" != *.sha256 ]]
}

# Whether any file matching the pattern exists. `find | grep -q .` would be
# simpler but exits non-zero under `set -o pipefail` when find is still writing as
# grep leaves, which would report a present asset as missing.
have_asset() {
  [[ -n "$(find "${ASSETS_DIR}" -name "$1" -type f -print -quit)" ]]
}

# Set aside assets belonging to a different build. A .sha256 sidecar is judged by
# the binary it describes, so a rejected binary takes its sidecar with it.
# CI/set-build-info.sh lowercases VERSION while a git tag keeps its case, so the
# prefix has to be compared case-insensitively.
REJECTED_DIR="${ASSETS_DIR%/}-rejected"
REJECTED=()
shopt -s nocasematch
while IFS= read -r asset_path; do
  asset_name="$(basename "${asset_path}")"
  binary_name="${asset_name%.sha256}"
  if ! is_release_binary "${binary_name}"; then
    continue
  fi
  if [[ "${binary_name}" == "${RELEASE_TAG}"* ]]; then
    continue
  fi
  mkdir -p "${REJECTED_DIR}"
  mv "${asset_path}" "${REJECTED_DIR}/"
  REJECTED+=("${asset_name}")
done < <(find "${ASSETS_DIR}" -type f | sort)
shopt -u nocasematch

if [[ ${#REJECTED[@]} -gt 0 ]]; then
  echo "::warning::Ignoring ${#REJECTED[@]} asset(s) from a different build than ${RELEASE_TAG}: ${REJECTED[*]}"
fi

MISSING=()
if ! have_asset '*.AppImage.tar'; then
  MISSING+=("Linux (.AppImage.tar)")
fi
if ! have_asset '*-arm64.dmg'; then
  MISSING+=("macOS (arm64 .dmg)")
fi
if ! have_asset '*-x86_64.dmg'; then
  MISSING+=("macOS (x86_64 .dmg)")
fi
if ! have_asset '*.exe'; then
  MISSING+=("Windows (.exe)")
fi

if [[ ${#MISSING[@]} -gt 0 ]]; then
  echo "::warning::Missing release assets for: ${MISSING[*]}"
fi

# Stable releases must have all platforms; PTB tolerates partial
if [[ "${RELEASE_TYPE}" == "release" && ${#MISSING[@]} -gt 0 ]]; then
  echo "::error::Stable release is missing assets for: ${MISSING[*]}"
  exit 1
fi

if ! have_asset '*.AppImage.tar' && ! have_asset '*.dmg' && ! have_asset '*.exe'; then
  echo "::error::No release assets found for any platform"
  exit 1
fi

echo "Assets to publish for ${RELEASE_TAG}:"
find "${ASSETS_DIR}" -type f | sort
