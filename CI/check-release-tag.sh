#!/bin/bash
# Asserts that the version a release will be published under can actually be
# offered as an update: APP_VERSION has to be a three-component semantic version,
# and the release tag - when there is one - has to be exactly "Mudlet-<APP_VERSION>".
#
# This exists because getting it wrong fails silently. The updater takes the version
# it offers from the release tag, not from APP_VERSION (Release::Release() in
# src/updater/Release.cpp strips the "Mudlet-" prefix), and SemVer::getRegExp() in
# src/updater/SemVer.cpp needs three components. Tag "Mudlet-5.0" and every
# installed copy of Mudlet decides the release is not newer than what it already
# has. Nothing is raised and the update check logs "0 update(s) available", which is
# exactly what it logs in a week when there genuinely is no release - so there is no
# signal to notice.
#
# CI/prepare-release-assets.sh cannot catch this: its check is a tag *prefix* match
# against the asset names, and "Mudlet-5.0.0-linux-x64.AppImage.tar" does start with
# "Mudlet-5.0". The opposite mistake - a stale APP_VERSION with a correct tag - is
# caught there, which is exactly why this one is worth a check of its own.
#
# Suffixed release tags such as "Mudlet-5.0.0-rc1" are rejected too. Not because the
# updater could not parse them - SemVer does accept a prerelease component - but
# because APP_VERSION cannot carry the suffix (CI/validate_deployment.sh requires
# three plain components), so tag and binary would disagree and the assets, which
# are named after APP_VERSION, would all fail the prefix match above. Supporting a
# release candidate means teaching APP_VERSION and this guard about it together.
#
# Usage: check-release-tag.sh <version> [tag]
#   With no tag - a PTB, whose tag is generated rather than pushed - only the shape
#   of the version is checked, which is the half that breaks a PTB the same way.

set -euo pipefail

VERSION="${1:-}"
TAG="${2:-}"

if [ $# -lt 1 ] || [ $# -gt 2 ] || [ -z "${VERSION}" ]; then
  echo "usage: $(basename "$0") <version> [tag]" >&2
  exit 2
fi

# The explanations below are worth reading in full, but a multi-line message cannot
# become a GitHub annotation, so leave a one-line summary where a red X is looked
# for first
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
it logs when there is genuinely nothing new, so nobody notices until the download
numbers do not move.

It also desynchronises macOS: create-github-release.yml puts the tag's version into
<sparkle:version> while the app reports APP_VERSION as its CFBundleVersion, so a
tag ahead of APP_VERSION leaves Sparkle re-offering an update the installed app can
never satisfy.

Delete the tag and push it again as 'Mudlet-${VERSION}'. If '${VERSION}' is not the
version you meant to release, change set(APP_VERSION ...) in CMakeLists.txt first.
EOF
  exit 1
fi

if [ -n "${TAG}" ]; then
  echo "Release tag '${TAG}' matches APP_VERSION '${VERSION}'."
else
  echo "APP_VERSION '${VERSION}' can be offered as an update."
fi
