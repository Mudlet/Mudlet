#!/bin/bash

# fail on error
set -e

BUILD=""

if [ -z "${TRAVIS_TAG}" ]; then
  BUILD="-testing"
  if [ "${TRAVIS_PULL_REQUEST}" != "false" ]; then # building for a PR
    COMMIT=$(git parse-rev --short "${TRAVIS_PULL_REQUEST_SHA}")
    BUILD="${BUILD}-CI-${COMMIT}"
  else
    COMMIT=$(git parse-rev --short HEAD)
    BUILD="${BUILD}-${COMMIT}"
  fi
fi

if [ "${Q_OR_C_MAKE}" = "cmake" ]; then
  perl -pi -e "s/SET\(APP_BUILD \"-dev\"\)/SET(APP_BUILD \"${BUILD}\")/" ${TRAVIS_BUILD_DIR}/CMakeLists.txt
elif [ "${Q_OR_C_MAKE}" = "qmake" ]; then
  perl -pi -e "s/BUILD = -dev/BUILD = ${BUILD}/" ${TRAVIS_BUILD_DIR}/src/src.pro
fi
