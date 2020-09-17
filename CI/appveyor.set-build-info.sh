#!/bin/sh

# Commented out things only needed for failure post-mortems:
# echo "Initial MSYSTEM is: ${MSYSTEM}"
# echo "Initial PATH is:"
# echo ${PATH}
# echo " "
# echo "Fixing things for ${BUILD_BITNESS}-bit builds:"
export MSYSTEM=MINGW${BUILD_BITNESS}
export MINGW_BASE_DIR=C:/msys64/mingw${BUILD_BITNESS}
export MINGW_INTERNAL_BASE_DIR=/mingw${BUILD_BITNESS}
export PATH=${MINGW_INTERNAL_BASE_DIR}/bin:/usr/bin:${PATH}
# echo " "
# echo "PATH is now:"
# echo ${PATH}
# echo " "
# echo "MSYSTEM is now: ${MSYSTEM}"

MUDLET_VERSION_BUILD=""

if [ -z "${TRAVIS_TAG}" ]; then
    if [ "$TRAVIS_EVENT_TYPE" = "cron" ]; then
        MUDLET_VERSION_BUILD="-ptb"
    else
        MUDLET_VERSION_BUILD="-testing"
    fi

    if [ "${TRAVIS_PULL_REQUEST}" != "false" ]; then # building for a PR
        COMMIT=$(git rev-parse --short "${TRAVIS_PULL_REQUEST_SHA}")
        MUDLET_VERSION_BUILD="${MUDLET_VERSION_BUILD}-PR${TRAVIS_PULL_REQUEST}-${COMMIT}"
    else
        COMMIT=$(git rev-parse --short HEAD)

        if [ "${MUDLET_VERSION_BUILD}" = "-ptb" ]; then
            DATE=$(date +'%Y-%m-%d')
            # add a short commit to version for changelog generation know what was last released
            SHORT_COMMIT=$(echo "${COMMIT}" | cut -c1-5)
            MUDLET_VERSION_BUILD="${MUDLET_VERSION_BUILD}-${DATE}-${SHORT_COMMIT}"
        else
            MUDLET_VERSION_BUILD="${MUDLET_VERSION_BUILD}-${COMMIT}"
        fi
    fi
fi

VERSION=""

if [ "${Q_OR_C_MAKE}" = "cmake" ]; then
    VERSION=$(perl -lne 'print $1 if /^set\(APP_VERSION (.+)\)/' < "${TRAVIS_BUILD_DIR}/CMakeLists.txt")
elif [ "${Q_OR_C_MAKE}" = "qmake" ]; then
    VERSION=$(perl -lne 'print $1 if /^VERSION = (.+)/' < "${TRAVIS_BUILD_DIR}/src/mudlet.pro")
fi

# not all systems we deal with allow uppercase ascii characters
export MUDLET_VERSION_BUILD=$(echo "$MUDLET_VERSION_BUILD" | tr '[:upper:]' '[:lower:]')
export VERSION=$(echo "$VERSION" | tr '[:upper:]' '[:lower:]')
