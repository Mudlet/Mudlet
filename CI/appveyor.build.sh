#!/bin/sh

echo "Running appveyor.build.sh shell script..."

# Probably not required as already tested for in appveyor.install.sh
# if [ ${BUILD_BITNESS} != "32" ] && [ ${BUILD_BITNESS} != "64" ] ; then
#    echo "Requires environmental variable BUILD_BITNESS to exist and be set to \"32\" or \"64\" to specify bitness of target to be built."
#    exit 1
# fi

# Commenting out things only needed for failure post-mortems
# echo "Initial MSYSTEM is: ${MSYSTEM}"
# echo "Initial PATH is:"
# echo ${PATH}
echo " "
echo "Fixing things for ${BUILD_BITNESS}-bit builds:"
export MSYSTEM=MINGW${BUILD_BITNESS}
export MINGW_BASE_DIR=C:/msys64/mingw${BUILD_BITNESS}
export MINGW_INTERNAL_BASE_DIR=/mingw${BUILD_BITNESS}
export PATH=${MINGW_INTERNAL_BASE_DIR}/bin:/usr/bin:${PATH}
echo " "
# echo "PATH is now:"
# echo ${PATH}
# echo " "
# echo "MSYSTEM is now: ${MSYSTEM}"

echo " "
echo "Project directory is: $(/usr/bin/cygpath --windows ${APPVEYOR_BUILD_FOLDER})"
cd $(/usr/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER})
# echo "  which contains:"
# /usr/bin/ls -l
echo "  creating './build' sub-directory and moving to it"
/usr/bin/mkdir ./build
cd ./build
# echo "  it contains:"
# /usr/bin/ls -l

if [ ${APPVEYOR_REPO_TAG} = "false" ] ; then
    MUDLET_VERSION_BUILD="-${BUILD_BITNESS}bit-testing"
    if [ -p ${APPVEYOR_PULL_REQUEST_NUMBER} ] ; then
        COMMIT="$(git rev-parse --short ${APPVEYOR_PULL_REQUEST_HEAD_COMMIT})"
        MUDLET_VERSION_BUILD="${MUDLET_VERSION_BUILD}-PR${APPVEYOR_PULL_REQUEST_NUMBER}-${COMMIT}"
    else
        COMMIT="$(git rev-parse --short HEAD)"
        MUDLET_VERSION_BUILD="${MUDLET_VERSION_BUILD}-${COMMIT}"
    fi
fi

echo " "
echo "Now building a ${BUILD_BITNESS} bit MUDLET${MUDLET_VERSION_BUILD} ..."

# We could support debug builds in the future by adding as an argument to the qmake call:
# CONFIG+=debug and changing references to "release" sub-directories to "debug"...
# Remove the following once we have the infrastructure for 64 Bit window builds sorted:
if [ ${BUILD_BITNESS} = "64" ] ; then
    export WITH_UPDATER=NO
fi

echo " "
echo "Running qmake:"
${MINGW_INTERNAL_BASE_DIR}/bin/qmake CONFIG+=release ../src/mudlet.pro
exit_status=$?
if [ ${exit_status} -ne 0 ]; then
    exit ${exit_status}
fi
echo " "
echo "Running mingw32-make on individual oversized qrc_mudlet_fonts_windows file first:"
# Change the referred to makefile if we switch to a debug build:
${MINGW_INTERNAL_BASE_DIR}/bin/mingw32-make -f Makefile.Release release/qrc_mudlet_fonts_windows.o
exit_status=$?
if [ ${exit_status} -ne 0 ]; then
    exit ${exit_status}
fi
echo " "
echo "Running mingw32-make with 'keep-going' option for a dual core VM:"
${MINGW_INTERNAL_BASE_DIR}/bin/mingw32-make -k -j 3
exit_status=$?
if [ ${exit_status} -ne 0 ]; then
    exit ${exit_status}
fi
echo " "
echo "mingw32-make finished!"
echo " "

# Follow section commented out as only needed for post mortem checks:
# echo "Project directory: ${APPVEYOR_BUILD_FOLDER}"
# echo "  now contains:"
# /usr/bin/ls -al ${APPVEYOR_BUILD_FOLDER}
# echo " "

# # Note that the APPVEYOR_BUILD_FOLDER variable uses '\' (a single backslash)
# # as the directory separator but that is not usable in commands (it needs doubling)
# # or changing to forward slashes to work for them:
# echo "Project build directory: $(/usr/bin/cygpath --windows ${APPVEYOR_BUILD_FOLDER}/build)"
# echo "  now contains:"
# /usr/bin/ls -al $(/usr/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER}/build)
# echo " "
# echo "Project build sub-directory: $(/usr/bin/cygpath --windows ${APPVEYOR_BUILD_FOLDER}/build/release)"
# echo "  now contains:"
# /usr/bin/ls -al $(/usr/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER}/build/release)

# Will only get here if the build was successful
# Copy the executable (and a record of it's name) to a separate location

echo "Creating packaging directory: $(/usr/bin/cygpath --windows ${APPVEYOR_BUILD_FOLDER}/package)"
mkdir $(/usr/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER}/package)
echo " "
echo "Copying mudlet executable to it:"
cp $(/usr/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER}/build/release/mudlet.exe) $(/usr/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER}/package)
echo " "

cd $(/usr/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER}/package)

echo "Creating 'name' file:"
if [ -n "${MUDLET_VERSION_BUILD}" ] ; then
    echo "${MUDLET_VERSION_BUILD}" > name
else
    # Create an empty file if there are no name details (for an official Mudlet release)
    touch "name"
fi
echo " "

echo "   ... appveyor.build.sh shell script finished!"
echo " "
