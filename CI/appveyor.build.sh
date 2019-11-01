#!/bin/sh

echo "Running appveyor.build.sh shell script..."

if [ ${BUILD_BITNESS} != "32" -a ${BUILD_BITNESS} != "64" ] ; then
    echo "Requires environmental variable BUILD_BITNESS to exist and be set to \"32\" or \"64\" to specify bitness of target to be built."
    exit -1
fi

echo "Initial MSYSTEM is: ${MSYSTEM}"
echo "Initial PATH is:"
echo ${PATH}
echo " "
echo "Fixing things for ${BUILD_BITNESS}-bit builds:"
export MSYSTEM=MINGW${BUILD_BITNESS}
export MINGW_BASE_DIR=C:/msys64/mingw${BUILD_BITNESS}
export MINGW_INTERNAL_BASE_DIR=/mingw${BUILD_BITNESS}
export PATH=/${MINGW_INTERNAL_BASE_DIR}/bin:/usr/bin:${PATH}
echo " "
echo "PATH is now:"
echo ${PATH}
echo " "
echo "MSYSTEM is now: ${MSYSTEM}"

echo " "
echo "Moving to project directory: $(/bin/cygpath --windows ${APPVEYOR_BUILD_FOLDER})"
cd $(/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER})
echo "  containing:"
ls -l
echo " creating './build' directory"
mkdir ./build
cd ./build

if [ ${APPVEYOR_REPO_TAG} == "false" ] ; then
    MUDLET_VERSION_BUILD="-${BUILD_BITNESS}bit-testing"
    if [ -p ${APPVEYOR_PULL_REQUEST_NUMBER} ] ; then
        COMMIT="$(git rev-parse --short ${APPVEYOR_PULL_REQUEST_HEAD_COMMIT})"
        MUDLET_VERSION_BUILD="${MUDLET_VERSION_BUILD}-PR${APPVEYOR_PULL_REQUEST_NUMBER}-${COMMIT}"
    else
        COMMIT="$(git rev-parse --short HEAD)"
        MUDLET_VERSION_BUILD="${MUDLET_VERSION_BUILD}-${COMMIT}"
    fi
fi

echo "Building MUDLET${MUDLET_VERSION_BUILD} ..."

# We could support debug builds in the future by adding as an argument to the qmake call:
# CONFIG+=debug
# Remove the following once we have the infrastructure for 64 Bit window builds sorted:
if [ ${BUILD_BITNESS} == "64" ] ; then
    WITH_UPDATER=NO
fi

echo " "
echo "Running qmake:"
${MINGW_INTERNAL_BASE_DIR}/bin/qmake CONFIG+=release ../src/mudlet.pro
echo " "
echo "Running mingw${BUILD_BITNESS}-make on individual oversized qrc_mudlet_fonts_windows file first:"
# Change the referred to makefile if we switch to a debug build:
${MINGW_INTERNAL_BASE_DIR}/bin/mingw32-make -f Makefile.Release release/qrc_mudlet_fonts_windows.o
echo " "
echo "Running mingw${BUILD_BITNESS}-make with 'keep-going' option for a dual core VM:"
${MINGW_INTERNAL_BASE_DIR}/bin/mingw32-make -k -j 3
echo " "
echo "mingw32-make finished!"
echo " "
echo "Project directory: ${APPVEYOR_BUILD_FOLDER}"
echo "  now contains:"
ls -al ${APPVEYOR_BUILD_FOLDER}
echo " "

# Note that the APPVEYOR_BUILD_FOLDER variable ùses '\' (a single backslash)
# as the directory separator but that is not usable in commands (it needs doubling)
# or changing to forward slashes to work for them:
echo "Project build directory: ${APPVEYOR_BUILD_FOLDER}\build"
echo "  now contains:"
ls -al $(/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER}/build)
echo " "
echo "Project build sub-directory: ${APPVEYOR_BUILD_FOLDER}\build\release"
echo "  now contains:"
ls -al $(/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER}/build/release)
