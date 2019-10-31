#!/bin/sh

if [[ ${BUILD_BITNESS} != "32" -a ${BUILD_BITNESS} != "64" ]]; then
    echo "Requires environmental variable BUILD_BITNESS to exist and be set to \"32\" or \"64\" to specify bitness of target to be built."
    exit -1
fi

cd ${APPVEYOR_BUILD_FOLDER}

if [[ ${APPVEYOR_REPO_TAG} == "false" ]] ; then
    MUDLET_VERSION_BUILD="-${BUILD_BITNESS}bit-testing"
    if [[ -p ${APPVEYOR_PULL_REQUEST_NUMBER} ]] ; then
        COMMIT="$(git rev-parse --short ${APPVEYOR_PULL_REQUEST_HEAD_COMMIT})"
        MUDLET_VERSION_BUILD="${MUDLET_VERSION_BUILD}-PR${APPVEYOR_PULL_REQUEST_NUMBER}-${COMMIT}"
    else
        COMMIT="$(git rev-parse --short HEAD)"
        MUDLET_VERSION_BUILD="${MUDLET_VERSION_BUILD}-${COMMIT}"
    fi
fi

echo "BUILDING MUDLET ${MUDLET_VERSION_BUILD} ..."

# We could support debug builds in the future
if [[ ${BUILD_BITNESS} == "32"]]; then
    C:\msys64\mingw32\bin\qmake.exe CONFIG+=release "${APPVEYOR_BUILD_FOLDER}/src/mudlet.pro"
    C:\msys64\mingw32\bin\mingw32-make-make.exe -f
else
    C:\msys64\mingw64\bin\qmake.exe CONFIG+=release "${APPVEYOR_BUILD_FOLDER}/src/mudlet.pro"
    C:\msys64\mingw64\bin\mingw32-make-make.exe -f
fi
