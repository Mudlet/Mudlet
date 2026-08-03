#!/bin/bash
# Validates the assets downloaded from the platform build runs before they are
# published to a GitHub Release, and sets aside any that do not belong.
#
# Release assets are named "<release tag>[<rebuild counter>]-<platform>.<ext>",
# because both the tag and the filenames are built from the same
# VERSION/MUDLET_VERSION_BUILD/BUILD_COMMIT triple. The two platform build
# workflows can be re-run independently, and a re-run restamps the PTB build date
# and appends a rebuild counter - so its artifacts carry a different tag prefix.
# create-github-release.yml always picks the newest successful run per platform,
# so a re-run of one platform can pair its artifacts with another platform's
# older artifacts. Publishing those would put a binary from build B into the
# release for build A, whose in-app version does not match the release tag.
#
# Usage: prepare-release-assets.sh <assets-dir> <release-tag> <release-type>

set -euo pipefail

ASSETS_DIR="${1:?assets directory required}"
RELEASE_TAG="${2:?release tag required}"
RELEASE_TYPE="${3:?release type required}"

# Suffixes of files published as release assets, as opposed to the .sha256
# sidecars that only feed SHA256SUMS.txt
readonly BINARY_SUFFIXES=('.AppImage.tar' '.dmg' '.exe')

mkdir -p "${ASSETS_DIR}"

echo "Downloaded assets:"
find "${ASSETS_DIR}" -type f | sort

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

# Set aside assets belonging to a different build. A .sha256 sidecar is judged by
# the binary it describes, so a rejected binary takes its sidecar with it.
REJECTED_DIR="${ASSETS_DIR%/}-rejected"
REJECTED=()
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

if [[ ${#REJECTED[@]} -gt 0 ]]; then
  echo "::warning::Ignoring ${#REJECTED[@]} asset(s) from a different build than ${RELEASE_TAG}: ${REJECTED[*]}"
fi

MISSING=()
if ! find "${ASSETS_DIR}" -name '*.AppImage.tar' -type f | grep -q .; then
  MISSING+=("Linux (.AppImage.tar)")
fi
if ! find "${ASSETS_DIR}" -name '*.dmg' -type f | grep -q .; then
  MISSING+=("macOS (.dmg)")
fi
if ! find "${ASSETS_DIR}" -name '*.exe' -type f | grep -q .; then
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

if [[ ${#MISSING[@]} -eq 3 ]]; then
  echo "::error::No release assets found for any platform"
  exit 1
fi

echo "Assets to publish for ${RELEASE_TAG}:"
find "${ASSETS_DIR}" -type f | sort
