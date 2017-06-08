#!/bin/bash

BUILD=""

if [ -z "${TRAVIS_TAG}" ]; then
  BUILD="-testing"
  if [ "${TRAVIS_PULL_REQUEST}" != "false" ]; then # building for a PR
    COMMIT=$(git rev-parse --short "${TRAVIS_PULL_REQUEST_SHA}")
    BUILD="${BUILD}-CI-${COMMIT}"
  else
    COMMIT=$(git rev-parse --short HEAD)
    BUILD="${BUILD}-${COMMIT}"
  fi
fi

VERSION=""

if [ "${Q_OR_C_MAKE}" = "cmake" ]; then
  VERSION=$(perl -lne 'print $1 if /^SET\(APP_VERSION (.+)\)/' < ${TRAVIS_BUILD_DIR}/CMakeLists.txt)
  perl -pi -e "s/SET\(APP_BUILD \"-dev\"\)/SET(APP_BUILD \"${BUILD}\")/" ${TRAVIS_BUILD_DIR}/CMakeLists.txt
elif [ "${Q_OR_C_MAKE}" = "qmake" ]; then
  VERSION=$(perl -lne 'print $1 if /^VERSION = (.+)/' < ${TRAVIS_BUILD_DIR}/src/src.pro)
  perl -pi -e "s/BUILD = -dev/BUILD = ${BUILD}/" ${TRAVIS_BUILD_DIR}/src/src.pro
fi

export VERSION
export BUILD
