#!/bin/bash

MUDLET_VERSION_BUILD=""

# The travis case is now dead in the water as we stopped using it years ago so
# we can elminated testing for its values/variables!

if ! [[ "$GITHUB_REF" =~ ^"refs/tags/" ]]; then
  # This is NOT a "release" build!
  if [[ "$GITHUB_EVENT_NAME" = "schedule" ]] || [[ "$GITHUB_EVENT_INPUTS_SCHEDULED" = "true" ]]; then
    MUDLET_VERSION_BUILD="-ptb"
  elif [[ "${GITHUB_EVENT_NAME}" != "pull_request" ]]; then
    # We don't put "-testing" as a prefix to PR builds as the "-pr####" is
    # enough to identify the latter; so a "-testing" build now is only the
    # development branch after a PR is merged into it:
    MUDLET_VERSION_BUILD="-testing"
  fi

  if [ "${GITHUB_EVENT_NAME}" = "pull_request" ]; then
    # GITHUB_SHA identifies the commitish that results from merging the PR's
    # state onto the development branch and the ^2 to that returns the HEAD
    # of the PR before that happened.
    BUILD_COMMIT=$(git rev-parse --short "${GITHUB_SHA}^2")
    PR_NUMBER=$(echo "$GITHUB_REF" | sed 's/refs\///' | sed 's/pull\///' | sed 's/\/merge//')
    MUDLET_VERSION_BUILD="${MUDLET_VERSION_BUILD}-PR${PR_NUMBER}"
    echo "PR_NUMBER=$PR_NUMBER" >> "$GITHUB_ENV"
  else
    BUILD_COMMIT=$(git rev-parse --short HEAD)
  fi

  if [ "${MUDLET_VERSION_BUILD}" = "-ptb" ]; then
    DATE=$(date +'%Y-%m-%d')
    MUDLET_VERSION_BUILD="${MUDLET_VERSION_BUILD}-${DATE}"
  fi
fi

VERSION=""

if [ -n "$GITHUB_REPOSITORY" ]; then
  VERSION=$(perl -lne 'print $1 if /^set\(APP_VERSION (.+)\)/' < "${GITHUB_WORKSPACE}/CMakeLists.txt")
fi

# not all systems we deal with allow uppercase ascii characters
MUDLET_VERSION_BUILD=$(echo "$MUDLET_VERSION_BUILD" | tr '[:upper:]' '[:lower:]')
VERSION=$(echo "$VERSION" | tr '[:upper:]' '[:lower:]')

if [ -n "$GITHUB_REPOSITORY" ]; then
  {
    echo "VERSION=$VERSION"
    echo "MUDLET_VERSION_BUILD=$MUDLET_VERSION_BUILD"
    echo "BUILD_COMMIT=$BUILD_COMMIT"
  } >> "$GITHUB_ENV"
fi

export VERSION
export MUDLET_VERSION_BUILD
export BUILD_COMMIT
