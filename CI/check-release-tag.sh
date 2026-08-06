#!/bin/bash
# Asserts that a release tag is spelled exactly "Mudlet-<APP_VERSION>", with a
# three-component APP_VERSION.
#
# This exists because getting the tag wrong fails silently. The updater takes the
# version it offers from the release tag, not from APP_VERSION
# (Release::Release() in src/updater/Release.cpp strips the "Mudlet-" prefix), and
# SemVer::getRegExp() in src/updater/SemVer.cpp only accepts three components. Tag
# "Mudlet-5.0" and every installed copy of Mudlet decides the release is not newer
# than what it already has, so nobody is ever offered the update - with no error,
# no warning and no log line anywhere.
#
# CI/prepare-release-assets.sh cannot catch this: its check is a tag *prefix* match
# against the asset names, and "Mudlet-5.0.0-linux-x64.AppImage.tar" does start with
# "Mudlet-5.0". The opposite mistake - a stale APP_VERSION with a correct tag - is
# caught there, which is exactly why this one is worth a check of its own.
#
# Usage: check-release-tag.sh <tag> <version>

set -uo pipefail

TAG="${1:-}"
VERSION="${2:-}"

if [ -z "${TAG}" ] || [ -z "${VERSION}" ]; then
  echo "usage: $(basename "$0") <tag> <version>" >&2
  exit 2
fi

# The explanation below is worth reading in full, but it will not be turned into a
# GitHub annotation, so leave a one-line summary where a red X is looked for first
annotate() {
  if [ "${GITHUB_ACTIONS:-}" = "true" ]; then
    echo "::error::$1"
  fi
}

# A two-component APP_VERSION is just as fatal, and it would sail past the equality
# check below because the tag built from it would match
if ! [[ "${VERSION}" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
  annotate "APP_VERSION '${VERSION}' is not a three-component version - a release built from it would never be offered to anyone who already has Mudlet installed"
  cat >&2 <<EOF
error: APP_VERSION '${VERSION}' is not a three-component version like 5.0.0.

Mudlet's updater only recognises three-component semantic versions, so a release
built from this version is never offered to anyone who already has Mudlet
installed. Set a three-component version in set(APP_VERSION ...) in CMakeLists.txt.
EOF
  exit 1
fi

if [ "${TAG}" != "Mudlet-${VERSION}" ]; then
  annotate "Release tag '${TAG}' does not match APP_VERSION '${VERSION}' - it has to be exactly 'Mudlet-${VERSION}', or auto-update silently stops working for every existing user"
  cat >&2 <<EOF
error: release tag '${TAG}' does not match APP_VERSION '${VERSION}'.
The tag has to be exactly 'Mudlet-${VERSION}'.

Publishing under a mismatched tag breaks auto-update for everyone, silently. The
updater reads the version it offers from the tag rather than from the binary, so a
tag like 'Mudlet-5.0' offers version '5.0' - which is not a three-component
semantic version, is therefore never treated as newer than the installed 4.22.0,
and is never offered. No error is shown and nothing is logged: every existing user
simply stops receiving updates, on every platform. macOS breaks the same way by a
different route, because create-github-release.yml puts the tag's version into
<sparkle:version> while the app itself reports APP_VERSION as its CFBundleVersion.

Delete the tag and push it again as 'Mudlet-${VERSION}'. If '${VERSION}' is not the
version you meant to release, change set(APP_VERSION ...) in CMakeLists.txt first.
EOF
  exit 1
fi

echo "Release tag '${TAG}' matches APP_VERSION '${VERSION}'."
