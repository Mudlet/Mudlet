#!/bin/sh

# Commented out things only needed for failure post-mortems:
#echo "Initial MSYSTEM is: ${MSYSTEM}"
#echo "Initial PATH is:"
#echo ${PATH}
#echo ""
echo "Fixing things for ${BUILD_BITNESS}-bit builds:"
export MSYSTEM=MINGW${BUILD_BITNESS}
export MINGW_BASE_DIR=C:/msys64/mingw${BUILD_BITNESS}
export MINGW_INTERNAL_BASE_DIR=/mingw${BUILD_BITNESS}
export PATH=${MINGW_INTERNAL_BASE_DIR}/usr/local/bin:${MINGW_INTERNAL_BASE_DIR}/bin:/usr/bin:${PATH}
echo ""
#echo "PATH is now:"
#echo ${PATH}
#echo ""
#echo "MSYSTEM is now: ${MSYSTEM}"

# Remove the following once we have the infrastructure for 64 Bit window builds sorted:
if [ "${BUILD_BITNESS}" = "64" ] ; then
    export WITH_UPDATER="NO"
fi

# TEMPORARILY BODGE THINGS FOR TESTING (1 of 2):
#MUDLET_VERSION_BUILD=""
#if [ "${APPVEYOR_REPO_TAG}" = "false" ]; then
#    if [ "${APPVEYOR_SCHEDULED_BUILD}" = "true" ]; then
#        MUDLET_VERSION_BUILD="-ptb"
#        export PUBLIC_TEST_BUILD="true"
#    else
#        MUDLET_VERSION_BUILD="-testing"
#        export PUBLIC_TEST_BUILD="false"
#    fi
#
#    # -n is test for non-zero length string - so building for a PR
#    # Shorten the Commit SHA1 produced (so that it is only 7 hex digits) so that
#    # it matches the length used in the filename for the Linux/MacOs builds:
#    if [ -n "${APPVEYOR_PULL_REQUEST_NUMBER}" ]; then
#        export COMMIT=$(git rev-parse --short "${APPVEYOR_PULL_REQUEST_HEAD_COMMIT}" | cut -c-7)
#        MUDLET_VERSION_BUILD="${MUDLET_VERSION_BUILD}-PR${APPVEYOR_PULL_REQUEST_NUMBER}-${COMMIT}"
#    else
#        export COMMIT=$(git rev-parse --short HEAD | cut -c-7)
#        if [ "${MUDLET_VERSION_BUILD}" = "-ptb" ]; then
#            DATE=$(date +'%Y-%m-%d')
#            # add a short commit to version for changelog generation know what was last released
#            SHORT_COMMIT=$(echo "${COMMIT}" | cut -c1-5)
#            MUDLET_VERSION_BUILD="${MUDLET_VERSION_BUILD}-${DATE}-${SHORT_COMMIT}"
#        else
#            MUDLET_VERSION_BUILD="${MUDLET_VERSION_BUILD}-${COMMIT}"
#        fi
#    fi
#else
#    export COMMIT=""
#    export PUBLIC_TEST_BUILD="false"
#fi
MUDLET_VERSION_BUILD="-ptb-20200919-fake1"
COMMIT="fake1"
export APPVEYOR_REPO_TAG="true"
export APPVEYOR_SCHEDULED_BUILD="true"
export PUBLIC_TEST_BUILD="true"

VERSION=""

if [ "${Q_OR_C_MAKE}" = "cmake" ]; then
    VERSION=$(/usr/bin/perl -lne 'print $1 if /^set\(APP_VERSION (.+)\)/' < "${APPVEYOR_BUILD_FOLDER}/CMakeLists.txt")
elif [ "${Q_OR_C_MAKE}" = "qmake" ]; then
    VERSION=$(/usr/bin/perl -lne 'print $1 if /^VERSION = (.+)/' < "${APPVEYOR_BUILD_FOLDER}/src/mudlet.pro")
fi

# not all systems we deal with allow uppercase ascii characters
export MUDLET_VERSION_BUILD=$(echo "$MUDLET_VERSION_BUILD" | /usr/bin/tr '[:upper:]' '[:lower:]')
export VERSION=$(echo "$VERSION" | /usr/bin/tr '[:upper:]' '[:lower:]')
