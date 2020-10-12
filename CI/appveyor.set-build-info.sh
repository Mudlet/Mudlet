#!/bin/sh

# Commented out things only needed for failure post-mortems:
#echo "Initial MSYSTEM is: ${MSYSTEM}"
#echo "Initial PATH is:"
#echo ${PATH}
#echo ""
#echo "Fixing things for ${BUILD_BITNESS}-bit builds:"
#echo ""
export MSYSTEM=MINGW${BUILD_BITNESS}
export MINGW_BASE_DIR=C:/msys64/mingw${BUILD_BITNESS}
export MINGW_INTERNAL_BASE_DIR=/mingw${BUILD_BITNESS}
export PATH=${MINGW_INTERNAL_BASE_DIR}/usr/local/bin:${MINGW_INTERNAL_BASE_DIR}/bin:/usr/bin:${PATH}
#echo "PATH is now:"
#echo ${PATH}
#echo ""
#echo "MSYSTEM is now: ${MSYSTEM}"

# Remove the following once we have the infrastructure for 64 Bit window builds sorted:
if [ "${BUILD_BITNESS}" = "64" ]; then
    export WITH_UPDATER="NO"
fi

VERSION=""
if [ "${Q_OR_C_MAKE}" = "cmake" ]; then
    VERSION=$(/usr/bin/perl -lne 'print $1 if /^set\(APP_VERSION (.+)\)/' < "${APPVEYOR_BUILD_FOLDER}/CMakeLists.txt")
elif [ "${Q_OR_C_MAKE}" = "qmake" ]; then
    VERSION=$(/usr/bin/perl -lne 'print $1 if /^VERSION = (.+)/' < "${APPVEYOR_BUILD_FOLDER}/src/mudlet.pro")
fi

MUDLET_VERSION_BUILD=""
if [ "${APPVEYOR_REPO_TAG}" = "false" ]; then
    # Build was NOT initiated by pushing a tag to the repository - which is required for a release build
    if [ "${APPVEYOR_SCHEDULED_BUILD}" = "true" ]; then
        export BUILD_TYPE="public_test"
        COMMIT_DATE=$(git show -s --format=%cs | /usr/bin/tr -d '-')
        YESTERDAYS_DATE=$(date -v-1d '+%F' | /usr/bin/tr -d '-')
        if [ "${COMMIT_DATE}" -lt "${YESTERDAYS_DATE}" ]; then
            # There hasn't been any changes in last 24 hours
            export ABORT_PT_BUILDS="true"
        fi
        # add a short commit to version for changelog generation so it knows what was last released
        COMMIT=$(echo "${COMMIT}" | cut -c 1-5)
        DATE=$(date +'%Y%m%d')
        MUDLET_VERSION_BUILD="-ptb-${DATE}+${COMMIT}"
        if [ "${BUILD_BITNESS}" = "64" ]; then
            export NUPKG_FILE="$(/usr/bin/cygpath --windows "/c/projects/squirrel-packaging-prep/Mudlet_64_-PublicTestBuild.${VERSION}-${DATE}+${COMMIT}.nupkg")"
        else
            export NUPKG_FILE="$(/usr/bin/cygpath --windows "/c/projects/squirrel-packaging-prep/Mudlet-PublicTestBuild.${VERSION}-${DATE}+${COMMIT}.nupkg")"
        fi
    else
        # -n is test for non-zero length string - so building for a PR
        # Shorten the Commit SHA1 produced (so that it is only 8 hex digits) so that
        # it matches the length used in the filename for the Linux/MacOs builds:
        if [ -n "${APPVEYOR_PULL_REQUEST_NUMBER}" ]; then
            export BUILD_TYPE="pull_request"
            # APPVEYOR_PULL_REQUEST_HEAD_COMMIT is the Pull (Merge) Request
            # source commit ID (SHA):
            COMMIT=$(git rev-parse --short "${APPVEYOR_PULL_REQUEST_HEAD_COMMIT}" | cut -c 1-8)
            MUDLET_VERSION_BUILD="-testing-pr${APPVEYOR_PULL_REQUEST_NUMBER}-${COMMIT}"
        else
            # This is a build of the development branch after a PR has been
            # merged - consider changing the "-testing" to "-dev"
            # The commit number has been shortened from 7 to 6 digits to match
            # the number used for the other platforms:
            export BUILD_TYPE="development"
            COMMIT=$(git rev-parse --short HEAD | cut -c 1-6)
            MUDLET_VERSION_BUILD="-testing-${COMMIT}"
        fi
    fi
else
    # Build was initiated by pushing a tag (maybe this is the case for a release build?)
    if [ "${BUILD_BITNESS}" = "64" ]; then
        export NUPKG_FILE="$(/usr/bin/cygpath --windows "/c/projects/squirrel-packaging-prep/Mudlet_64_.${VERSION}.nupkg")"
    else
        export NUPKG_FILE="$(/usr/bin/cygpath --windows "/c/projects/squirrel-packaging-prep/Mudlet.${VERSION}.nupkg")"
    fi
    export BUILD_TYPE="release"
    export COMMIT=""
    export NUPKG_FILE=""
fi
#MUDLET_VERSION_BUILD=-ptb20200930+fake1
#DATE=20200930
#COMMIT=fake1
#export APPVEYOR_REPO_TAG="true"
#export APPVEYOR_SCHEDULED_BUILD="true"

# For proper Semantic versioning a version with a suffix after the number proper
# beginning with a hyphen (and period separated sections thereafter) is
# considered EARLIER than the same version number without such a suffix; such
# sections are orderable though. We do not currently conform to this definition!

# Also an additional plus-sign prefixed piece of meta information, like a git
# commit is NOT orderable or considered.

if [ -n "${DATE}" ]; then
    export DATE
fi
# not all systems we deal with allow uppercase ascii characters
export COMMIT=$(echo "${COMMIT}" | /usr/bin/tr '[:upper:]' '[:lower:]')
export MUDLET_VERSION_BUILD=$(echo "${MUDLET_VERSION_BUILD}" | /usr/bin/tr '[:upper:]' '[:lower:]')
export VERSION=$(echo "${VERSION}" | /usr/bin/tr '[:upper:]' '[:lower:]')
