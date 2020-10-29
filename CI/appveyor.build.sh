#!/bin/sh

echo "Running appveyor.build.sh shell script..."

# Source/setup some variables (including PATH):
. "$(/usr/bin/cygpath --unix "${APPVEYOR_BUILD_FOLDER}/CI/appveyor.set-build-info.sh")"

echo ""
echo "Project directory is: $(/usr/bin/cygpath --windows "${APPVEYOR_BUILD_FOLDER}")"
cd "$(/usr/bin/cygpath --unix "${APPVEYOR_BUILD_FOLDER}")" || appveyor AddMessage "ERROR: ${APPVEYOR_BUILD_FOLDER} directory not found! Build aborted." -Category Error; exit 1
# echo "  which contains:"
# /usr/bin/ls -l
echo "  creating './build' sub-directory and moving to it"
/usr/bin/mkdir ./build
cd ./build || appveyor AddMessage "ERROR: ${APPVEYOR_BUILD_FOLDER}/build directory not found! Build aborted." -Category Error; exit 1
# echo "  it contains:"
# /usr/bin/ls -l

echo ""
echo "Now building a ${BUILD_BITNESS} bit Mudlet ${VERSION}${MUDLET_VERSION_BUILD}..."
if [ -n "${APPVEYOR_PULL_REQUEST_HEAD_COMMIT}" ]; then
    echo "Head commit SHA1 is: \"${APPVEYOR_PULL_REQUEST_HEAD_COMMIT}\"."
else
    echo "APPVEYOR_PULL_REQUEST_HEAD_COMMIT is empty."
fi

# We could support debug builds in the future by adding as an argument to the qmake call:
# CONFIG+=debug and changing references to "release" sub-directories to "debug"...

# THIS IS REQUIRED AS IT SWITCHES SOME THINGS IN THE QMAKE (AND CMAKE) BUILDS TO
# WORK IN A MSYS2/MINGW-W64 BUILD ENVIRONMENT - without it the lua.h and related
# headers won't be found!
export WITH_MAIN_BUILD_SYSTEM=NO

echo ""
echo "Running qmake in release + debug_info mode:"
# We do not use CONFIG+=separate_debug_info because we may be renaming the
# built executable and that is likely to break the:
# objcopy --add-gnu-debuglink=foo.debug foo
# linkage that qmake would do prior to us renaming the executable
"${MINGW_INTERNAL_BASE_DIR}/bin/qmake" CONFIG+=release CONFIG-=qml_debug CONFIG-=qtquickcompiler CONFIG-=separate_debug_info CONFIG+=force_debug_info ../src/mudlet.pro
exit_status=$?
if [ ${exit_status} -ne 0 ]; then
    exit ${exit_status}
fi
echo ""
echo "Running mingw32-make with 'keep-going' option for a dual core VM:"
"${MINGW_INTERNAL_BASE_DIR}/bin/mingw32-make" -k -j 3
exit_status=$?
if [ ${exit_status} -ne 0 ]; then
    exit ${exit_status}
fi
echo ""
echo "mingw32-make finished!"
echo ""

# Follow section commented out as only needed for post mortem checks:
# echo "Project directory: ${APPVEYOR_BUILD_FOLDER}"
# echo "  now contains:"
# /usr/bin/ls -al ${APPVEYOR_BUILD_FOLDER}
# echo ""

# # Note that the APPVEYOR_BUILD_FOLDER variable uses '\' (a single backslash)
# # as the directory separator but that is not usable in commands (it needs doubling)
# # or changing to forward slashes to work for them:
# echo "Project build directory: $(/usr/bin/cygpath --windows "/c/projects/mudlet/build")"
# echo "  now contains:"
# /usr/bin/ls -al /c/projects/mudlet/build
# echo ""
# echo "Project build sub-directory: $(/usr/bin/cygpath --windows "/c/projects/mudlet/build/release")"
# echo "  now contains:"
# /usr/bin/ls -al /c/projects/mudlet/build/release

# Will only get here if the build was successful
# Copy the executable to a separate location

echo "Creating packaging directory: $(/usr/bin/cygpath --windows "/c/projects/mudlet/package")"
mkdir "/c/projects/mudlet/package"
echo ""
echo "Copying mudlet executable to it:"
cp "/c/projects/mudlet/build/release/mudlet.exe" "/c/projects/mudlet/package"
echo ""

echo "   ... appveyor.build.sh shell script finished!"
echo ""
