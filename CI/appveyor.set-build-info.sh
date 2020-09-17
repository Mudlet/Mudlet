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
${APPVEYOR_REPO_TAG}
if [ "${APPVEYOR_REPO_TAG}" = "false" ]; then
    if [ "${APPVEYOR_SCHEDULED_BUILD}" = "true" ]; then
        MUDLET_VERSION_BUILD="-ptb"
    else
        MUDLET_VERSION_BUILD="-testing"
    fi

    if [ -p ${APPVEYOR_PULL_REQUEST_NUMBER} ] ; then # building for a PR
        COMMIT=$(git rev-parse --short "${APPVEYOR_PULL_REQUEST_HEAD_COMMIT})"
        MUDLET_VERSION_BUILD="${MUDLET_VERSION_BUILD}-PR${APPVEYOR_PULL_REQUEST_NUMBER}-${COMMIT}"
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
    VERSION=$(perl -lne 'print $1 if /^set\(APP_VERSION (.+)\)/' < "${APPVEYOR_BUILD_FOLDER}/CMakeLists.txt")
elif [ "${Q_OR_C_MAKE}" = "qmake" ]; then
    VERSION=$(perl -lne 'print $1 if /^VERSION = (.+)/' < "${APPVEYOR_BUILD_FOLDER}/src/mudlet.pro")
fi

# not all systems we deal with allow uppercase ascii characters
export MUDLET_VERSION_BUILD=$(echo "$MUDLET_VERSION_BUILD" | tr '[:upper:]' '[:lower:]')
export VERSION=$(echo "$VERSION" | tr '[:upper:]' '[:lower:]')
