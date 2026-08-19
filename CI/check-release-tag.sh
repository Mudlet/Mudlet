#!/bin/bash
# Usage: check-release-tag.sh <version> [tag]
#
# The updater offers the version from the release tag, not from APP_VERSION
# (src/updater/Release.cpp), and SemVer needs three components - so "Mudlet-5.0" is
# never offered to anyone, silently. CI/prepare-release-assets.sh cannot see it: that
# check matches the tag as a prefix of the asset names, and "Mudlet-5.0.0-linux-x64"
# does start with "Mudlet-5.0". Only the opposite mistake fails there.
#
# "Mudlet-5.0.0-rc1" is rejected as well - SemVer would accept it, but APP_VERSION
# cannot carry a suffix, so the assets named after it would not match the tag.
#
# A PTB passes no tag, its tag being generated rather than pushed.

set -euo pipefail

VERSION="${1:-}"
TAG="${2:-}"

if [ $# -lt 1 ] || [ $# -gt 2 ] || [ -z "${VERSION}" ]; then
  echo "usage: $(basename "$0") <version> [tag]" >&2
  exit 2
fi

# A multi-line message cannot become a GitHub annotation, so summarise in one line
annotate() {
  if [ "${GITHUB_ACTIONS:-}" = "true" ]; then
    echo "::error::$1"
  fi
}

# Same shape SemVer::getRegExp() accepts, leading zeros and all
if ! [[ "${VERSION}" =~ ^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$ ]]; then
  annotate "APP_VERSION '${VERSION}' is not a three-component version - nothing built from it would ever be offered as an update to anyone who already has Mudlet installed"
  cat >&2 <<EOF
error: APP_VERSION '${VERSION}' is not a three-component version like 5.0.0.

Mudlet's updater only recognises three-component semantic versions, so a build
carrying this version is never offered to anyone who already has Mudlet installed -
including public test builds. Set a three-component version in
set(APP_VERSION ...) in CMakeLists.txt.
EOF
  exit 1
fi

if [ -n "${TAG}" ] && [ "${TAG}" != "Mudlet-${VERSION}" ]; then
  annotate "Release tag '${TAG}' does not match APP_VERSION '${VERSION}' - it has to be exactly 'Mudlet-${VERSION}', or auto-update stops working for every existing user without saying so"
  cat >&2 <<EOF
error: release tag '${TAG}' does not match APP_VERSION '${VERSION}'.
The tag has to be exactly 'Mudlet-${VERSION}'.

Publishing under a mismatched tag breaks auto-update, without saying so. The
updater reads the version it offers from the tag rather than from the binary, so a
tag like 'Mudlet-5.0' offers version '5.0' - not a three-component semantic
version, therefore never newer than the installed 4.22.0, therefore never offered.
No error is shown and the update check logs "0 update(s) available", the same line
it logs when there is genuinely nothing new.

It also desynchronises macOS: create-github-release.yml puts the tag's version into
<sparkle:version> while the app reports APP_VERSION as its CFBundleVersion, so a
tag ahead of APP_VERSION leaves Sparkle re-offering an update the installed app can
never satisfy.

Delete the tag and push it again as 'Mudlet-${VERSION}'. If '${VERSION}' is not the
version you meant to release, change set(APP_VERSION ...) in CMakeLists.txt first.
EOF
  exit 1
fi

# A release log with no line here reads the same whether the tag was compared or the
# guard was never reached
if [ -n "${TAG}" ]; then
  echo "Release tag '${TAG}' matches APP_VERSION '${VERSION}'."
fi
