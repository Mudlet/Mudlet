#!/bin/sh

echo "Running appveyor.build.sh shell script..."

if [ ${BUILD_BITNESS} != "32" -a ${BUILD_BITNESS} != "64" ] ; then
    echo "Requires environmental variable BUILD_BITNESS to exist and be set to \"32\" or \"64\" to specify bitness of target to be built."
    exit -1
fi

echo "Initial PATH is:"
echo ${PATH}
if [ ${BUILD_BITNESS} == "32" ] ; then
    echo "Fixing it for 32-bit builds:"
    export PATH=/mingw32/bin:/usr/bin:${PATH}
else
    echo "Fixing it for 64-bit builds:"
    export PATH=/mingw64/bin:/usr/bin:${PATH}
fi
echo "It is now:"
echo ${PATH}

echo " "
echo "Moving to project directory: ${APPVEYOR_BUILD_FOLDER}"
cd ${APPVEYOR_BUILD_FOLDER}
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
if [ ${BUILD_BITNESS} == "32" ] ; then
    # Should be already defined in environment: MINGW_BASE_DIR=C:/msys64/mingw32
    echo " "
    echo "Running qmake:"
    /mingw32/bin/qmake CONFIG+=release ../src/mudlet.pro
    echo " "
    echo "Running mingw32-make with 'keep-going' option:"
    /mingw32/bin/mingw32-make -k
else
    # Should be already defined in environment: MINGW_BASE_DIR=C:/msys64/mingw64
    # Remove the following once we have the infrastructure for 64 Bit window builds sorted:
    WITH_UPDATER=NO
    echo " "
    echo "Running qmake:"
    /mingw64/bin/qmake CONFIG+=release ../src/mudlet.pro
    echo " "
    echo "Running mingw32-make with 'keep-going' option:"
    /mingw64/bin/mingw32-make -k
fi

echo " "
echo "mingw32-make finished!"
