#!/bin/bash

MUDLET_VERSION_BUILD=""

if [ -z "${TRAVIS_TAG}" ] && ! [[ "$GITHUB_REF" =~ ^"refs/tags/" ]]; then
  if [ "$TRAVIS_EVENT_TYPE" = "cron" ] || [[ "$GITHUB_EVENT_NAME" = "schedule" ]]; then
    MUDLET_VERSION_BUILD="-ptb"
  else
    MUDLET_VERSION_BUILD="-testing"
  fi

  if [ -n "$TRAVIS_PULL_REQUEST" ]; then # building for a PR
    BUILD_COMMIT=$(git rev-parse --short "${TRAVIS_PULL_REQUEST_SHA}")
    MUDLET_VERSION_BUILD="${MUDLET_VERSION_BUILD}-PR${TRAVIS_PULL_REQUEST}-${BUILD_COMMIT}"
    PR_NUMBER=${TRAVIS_PULL_REQUEST}
    export PR_NUMBER
  elif [ "${GITHUB_EVENT_NAME}" = "pull_request" ]; then
    BUILD_COMMIT=$(git rev-parse --short "${GITHUB_SHA}^2")
    PR_NUMBER=$(echo "$GITHUB_REF" | sed 's/refs\///' |sed 's/pull\///' | sed 's/\/merge//')
    MUDLET_VERSION_BUILD="${MUDLET_VERSION_BUILD}-PR${PR_NUMBER}-${BUILD_COMMIT}"
    echo "PR_NUMBER=$PR_NUMBER" >> "$GITHUB_ENV"
  else
    BUILD_COMMIT=$(git rev-parse --short HEAD)

    if [ "${MUDLET_VERSION_BUILD}" = "-ptb" ]; then
      DATE=$(date +'%Y-%m-%d')
      # add a short commit to version for changelog generation know what was last released
      SHORT_COMMIT=$(echo "${BUILD_COMMIT}" | cut -c1-5)
      MUDLET_VERSION_BUILD="${MUDLET_VERSION_BUILD}-${DATE}-${SHORT_COMMIT}"
    else
      MUDLET_VERSION_BUILD="${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}"
    fi
  fi
fi

VERSION=""

if [ "${Q_OR_C_MAKE}" = "cmake" ]; then
  VERSION=$(perl -lne 'print $1 if /^set\(APP_VERSION (.+)\)/' < "${TRAVIS_BUILD_DIR}/CMakeLists.txt")
elif [ "${Q_OR_C_MAKE}" = "qmake" ]; then
  VERSION=$(perl -lne 'print $1 if /^VERSION = (.+)/' < "${TRAVIS_BUILD_DIR}/src/mudlet.pro")
elif [ -n "$GITHUB_REPOSITORY" ]; then
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
