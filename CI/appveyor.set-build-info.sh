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

# Pull the VERSION number from the relevant project file:
VERSION=""
if [ "${Q_OR_C_MAKE}" = "cmake" ]; then
    VERSION=$(/usr/bin/perl -lne 'print $1 if /^set\(APP_VERSION (.+)\)/' < "/c/projects/mudlet/CMakeLists.txt")
elif [ "${Q_OR_C_MAKE}" = "qmake" ]; then
    VERSION=$(/usr/bin/perl -lne 'print $1 if /^VERSION = (.+)/' < "/c/projects/mudlet/src/mudlet.pro")
else
    appveyor AddMessage "ERROR: Q_OR_C_MAKE environmental variable not set - cannot determine which project build system and where to get version x.y.z number from! Build abort." -Category Error
fi

MUDLET_VERSION_BUILD=""

# Fake stuff to emulate PTB
export BUILD_TYPE="public_test"
COMMIT=$(git rev-parse --short HEAD | cut -c 1-5)
DATE="19700103"
PTB_DATE="1970-01-03"
MUDLET_VERSION_BUILD="-ptb-${PTB_DATE}-${COMMIT}"

#if [ "${APPVEYOR_REPO_TAG}" = "false" ]; then
#    # Build was NOT initiated by pushing a tag to the repository - which is required for a release build
#    if [ "${APPVEYOR_SCHEDULED_BUILD}" = "true" ]; then
#        export BUILD_TYPE="public_test"
#        COMMIT_DATE=$(git show -s --format="%cs" | /usr/bin/tr -d '-')
#        YESTERDAYS_DATE=$(date -v-1d '+%F' | /usr/bin/tr -d '-')
#        if [ "${COMMIT_DATE}" -lt "${YESTERDAYS_DATE}" ]; then
#            # There hasn't been any changes in last 24 hours
#            export ABORT_PT_BUILDS="true"
#        fi
#        # add a short commit to version for changelog generation so it knows
#        # what was last released
#        COMMIT=$(git rev-parse --short HEAD | cut -c 1-5)
#        DATE=$(date +'%Y%m%d')
#        PTB_DATE=$(date +'%Y-%m-%d')
         export PTB_DATE
#        # The date used here is hyphen separated but all the other usages have
#        # no separator:
#        MUDLET_VERSION_BUILD="-ptb-$(date +'%Y-%m-%d')-${COMMIT}"
#        # As the nuget/squirrel stuff is built on a NET 4.5 framework the stuff
#        # we want to include in the project must be in a ./lib/Net45
#        # sub-directory - the name of the generated file is tied to the "id"
#        # "version" (and "version-suffix") elements within the nuspec file
#        # with the last two of them as overridden by the nuget pack command
#        # arguments. The "id" for the 32 Bit release and ptb builds must remain
#        # the same even after 64 Bit versions start to be created so that the
#        # they continue to be updated. The "id" must also NOT end with a number
#        # (or any sort of braces) which is why the "64" is surrounded by '_'s
#        # instead:
        if [ "${BUILD_BITNESS}" = "64" ]; then
            export NUPKG_FILE="Mudlet_x64_-PublicTestBuild.${VERSION}-ptb${DATE}.nupkg"
            export EXPORT_NUSPEC_FILE="mudlet64-ptb.nuspec"
            export SQUIRREL_FULL_NUPKG_FILE="Mudlet_x64_-PublicTestBuild-${VERSION}-ptb${DATE}-full.nupkg"
            export SQUIRREL_FULL_RENAMED_NUPKG_FILE="Mudletx64-${VERSION}-ptb${DATE}-full.nupkg"
        else
            export NUPKG_FILE="Mudlet-PublicTestBuild.${VERSION}-ptb${DATE}.nupkg"
            export EXPORT_NUSPEC_FILE="mudlet-ptb.nuspec"
            export SQUIRREL_FULL_NUPKG_FILE="Mudlet-PublicTestBuild-${VERSION}-ptb${DATE}-full.nupkg"
            export SQUIRREL_FULL_RENAMED_NUPKG_FILE="Mudlet-${VERSION}-ptb${DATE}-full.nupkg"
        fi
        export SUFFIX_FOR_NUGET="ptb${DATE}"
        export LOADING_GIF_PATHFILE=/c/projects/installers/windows/splash-installing-ptb-2x.png
        export SETUP_ICON_PATHFILE=/c/projects/mudlet/src/icons/mudlet_ptb.ico
#    else
#        # -n is test for non-zero length string - so building for a PR
#        # Shorten the Commit SHA1 produced (so that it is only 8 hex digits) so that
#        # it matches the length used in the filename for the Linux/MacOs builds:
#        if [ -n "${APPVEYOR_PULL_REQUEST_NUMBER}" ]; then
#            export BUILD_TYPE="pull_request"
#            # APPVEYOR_PULL_REQUEST_HEAD_COMMIT is the Pull (Merge) Request
#            # source commit ID (SHA):
#            COMMIT=$(git rev-parse --short "${APPVEYOR_PULL_REQUEST_HEAD_COMMIT}" | cut -c 1-8)
#            MUDLET_VERSION_BUILD="-testing-pr${APPVEYOR_PULL_REQUEST_NUMBER}-${COMMIT}"
#        else
#            # This is a build of the development branch after a PR has been
#            # merged - consider changing the "-testing" to "-dev"
#            # The commit number has been shortened from 7 to 6 digits to match
#            # the number used for the other platforms:
#            export BUILD_TYPE="development"
#            COMMIT=$(git rev-parse --short HEAD | cut -c 1-6)
#            MUDLET_VERSION_BUILD="-testing-${COMMIT}"
#        fi
#        # Now append an "-x32" or "-x64" suffix to the "windows" to match the linux
#        # snapshot file:
#        export ZIP_FILE_NAME=Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-windows-x${BUILD_BITNESS}.zip
#    fi
#else
#    export BUILD_TYPE="release"
#    export COMMIT=""
#    # As the nuget/squirrel stuff is built on a NET 4.5 framework the stuff
#    # we want to include in the project must be in a ./lib/Net45
#    # sub-directory:
#    # Build was initiated by pushing a tag (this should be the case for a
#    # release build)
#    if [ "${BUILD_BITNESS}" = "64" ]; then
#        export NUPKG_FILE="Mudlet_x64_.${VERSION}.nupkg"
#        export EXPORT_NUSPEC_FILE="mudlet64.nuspec"
#        export SQUIRREL_FULL_NUPKG_FILE="Mudlet_x64_-${VERSION}-full.nupkg"
#        export SQUIRREL_FULL_RENAMED_NUPKG_FILE="Mudletx64-${VERSION}-full.nupkg"
#    else
#        export NUPKG_FILE="Mudlet.${VERSION}.nupkg"
#        export EXPORT_NUSPEC_FILE="mudlet.nuspec"
#        export SQUIRREL_FULL_NUPKG_FILE="Mudlet-${VERSION}-full.nupkg"
#        export SQUIRREL_FULL_RENAMED_NUPKG_FILE="Mudlet-${VERSION}-full.nupkg"
#    fi
#    export SUFFIX_FOR_NUGET=""
#    export LOADING_GIF_PATHFILE="/c/projects/installers/windows/splash-installing-2x.png"
#    export SETUP_ICON_PATHFILE="/c/projects/mudlet/src/icons/mudlet.ico"
#fi

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
COMMIT=$(echo "${COMMIT}" | /usr/bin/tr '[:upper:]' '[:lower:]')
MUDLET_VERSION_BUILD=$(echo "${MUDLET_VERSION_BUILD}" | /usr/bin/tr '[:upper:]' '[:lower:]')
VERSION=$(echo "${VERSION}" | /usr/bin/tr '[:upper:]' '[:lower:]')
export COMMIT
export MUDLET_VERSION_BUILD
export VERSION
