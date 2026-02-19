#!/bin/bash
###########################################################################
#   Copyright (C) 2024-2024  by John McKisson - john.mckisson@gmail.com   #
#   Copyright (C) 2023-2025  by Stephen Lyons - slysven@virginmedia.com   #
#                                                                         #
#   This program is free software; you can redistribute it and/or modify  #
#   it under the terms of the GNU General Public License as published by  #
#   the Free Software Foundation; either version 2 of the License, or     #
#   (at your option) any later version.                                   #
#                                                                         #
#   This program is distributed in the hope that it will be useful,       #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#   GNU General Public License for more details.                          #
#                                                                         #
#   You should have received a copy of the GNU General Public License     #
#   along with this program; if not, write to the                         #
#   Free Software Foundation, Inc.,                                       #
#   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             #
###########################################################################

# Version: 2.1.0    Remove MINGW32 since upstream no longer supports it
#          2.0.0    Rework to build on an MSYS2 MINGW64 Github workflow
#          1.5.0    Change BUILD_TYPE to BUILD_CONFIG to avoid clash with
#                   CI/CB system using same variable
#          1.4.0    No change
#          1.3.0    Remove used of the no longer supported/used by us QT5
#                   Gamepad stuff (since PR #6787 was merged into
#                   the development branch)
#          1.2.0    No changes
#          1.1.0    Updated to bail out if there isn't a mudlet.exe file to
#                   work with
#          1.0.0    Original version

# Script to each time to package all the files needed to run Mudlet on
# Windows in a archive file that will be deployed from a github workflow

# To be used AFTER setup-windows-sdk.sh and build-mudlet-for-windows.sh
# have been run.

# Exit codes:
# 0 - Everything is fine. 8-)
# 1 - Failure to change to a directory
# 2 - Unsupported MSYS2/MINGW shell type
# 3 - Unsupported build type
# 4 - Directory to be used to assemble the package is NOT empty
# 8 - No Mudlet.exe file found to work with

if [ "${MSYSTEM}" = "MSYS" ]; then
  echo "Please run this script from a MINGW64 type bash terminal as the MSYS one"
  echo "does not supported what is needed."
  exit 2
elif [ "${MSYSTEM}" != "MINGW64" ]; then
  echo "This script is not set up to handle systems of type ${MSYSTEM}, only"
  echo "MINGW64 is currently supported. Please rerun this in a bash terminal of"
  echo "that type."
  exit 2
fi

# These should be set by the build-mudlet-for-windows script:
if [ "${BUILD_ACTION}" != "Release" ]; then
  if [ -z "${BUILD_COMMIT}" ]; then
    # Error out if not set:
    echo "BUILD_COMMIT not set in environment and this isn't a \"Release\" build, check preceding steps!"
    exit 10
  fi
  if [ -z "${MUDLET_VERSION_BUILD}" ]; then
    echo "MUDLET_VERSION_BUILD not set in environment and this isn't a \"Release\" build, check preceding steps!"
    exit 10
  fi
fi

if [ -z "${BUILD_CONFIG}" ]; then
  # Error out if not set:
  echo "BUILD_CONFIG not set in environment, check preceding steps!"
  exit 10
fi

if [ -z "${VERSION}" ]; then
  # Error out if not set:
  echo "VERSION not set in environment, check preceding steps!"
  exit 10
fi

GITHUB_WORKSPACE_UNIX_PATH="$(cygpath -au "${GITHUB_WORKSPACE}")"

if [ ! -f "${GITHUB_WORKSPACE_UNIX_PATH}/build-${MSYSTEM}/${BUILD_CONFIG}/mudlet.exe" ]; then
  echo "ERROR: no Mudlet executable found - did the previous build"
  echo "complete sucessfully?"
  exit 6
fi

PACKAGE_DIR="${GITHUB_WORKSPACE_UNIX_PATH}/package-${MSYSTEM}-${BUILD_CONFIG}"
export PACKAGE_DIR

echo "MSYSTEM is: ${MSYSTEM}"
echo ""

cd "${GITHUB_WORKSPACE_UNIX_PATH}" || exit 1

if [ -d "${PACKAGE_DIR}" ]; then
  # The wanted packaging dir exists - as is wanted
  echo ""
  echo "Checking for an empty ${PACKAGE_DIR} in which to assemble files..."
  echo ""
  if [ -n "$(ls -A "${PACKAGE_DIR}")" ]; then
    # But it isn't empty...
    echo "${PACKAGE_DIR} does not appear to be empty, please"
    echo "erase everything there and try again."
    exit 4
  fi
else
  echo ""
  echo "Creating ${PACKAGE_DIR} in which to assemble files..."
  echo ""
  # This will create the directory if it doesn't exist but won't moan if it does
  mkdir -p "${PACKAGE_DIR}"
fi
cd "${PACKAGE_DIR}" || exit 1
echo ""

echo "Copying wanted compiled files from ${GITHUB_WORKSPACE_UNIX_PATH}/build-${MSYSTEM} to ${PACKAGE_DIR} ..."
echo ""

if [ ! -f "${GITHUB_WORKSPACE_UNIX_PATH}/build-${MSYSTEM}/${BUILD_CONFIG}/mudlet.exe" ]; then
  echo "ERROR: no Mudlet executable found - did the previous build"
  echo "complete sucessfully?"
  exit 6
fi


FILES_TO_COPY=("mudlet.exe")

if [ -f "${GITHUB_WORKSPACE_UNIX_PATH}/build-${MSYSTEM}/${BUILD_CONFIG}/mudlet.exe.debug" ]; then
    FILES_TO_COPY+=("mudlet.exe.debug")
fi

if [ "${WITH_SENTRY}" = "ON" ]; then
    FILES_TO_COPY+=("crashpad_handler.exe" "crashpad_wer.dll" "MudletCrashReporter.exe")
fi

for f in "${FILES_TO_COPY[@]}"; do
    cp "${GITHUB_WORKSPACE_UNIX_PATH}/build-${MSYSTEM}/${BUILD_CONFIG}/$f" "${PACKAGE_DIR}/"
done

# The location that windeployqt6 puts the Qt translation files by default is
# "./translations" unfortunately "QLibraryInfo::path(QLibraryInfo::TranslationsPath)"
# in the calls to "QString mudlet::getMudletPath(const enums::mudletPathType, const QString&, const QString&)"
# with "enums::qtTranslationsPath" as the first argument returns:
# "./share/Qt6/translations" - which means the Qt translations were not getting
# loaded for our Windows builds:
# Also the --debug / --release flags don't work or are not needed for Qt6
# (or even Qt5) as the debug information is shipped separatly rather than
# being included in the Qt6 dll files.

WINDEPLOY_ARGS=( \
  "--translationdir" \
  "./share/qt6/translations" \
  "--compiler-runtime" \
  "--no-system-dxc-compiler" \
  "--force-openssl")

echo "Running ${MINGW_INTERNAL_BASE_DIR}/bin/windeployqt-qt6.exe..."
echo "  With options: \"" "${WINDEPLOY_ARGS[@]}" "\""
echo ""

"${MINGW_INTERNAL_BASE_DIR}/bin/windeployqt6" "${WINDEPLOY_ARGS[@]}" "./mudlet.exe"

# Copy in all the other known to be needed .dlls BEFORE we analyse the WHOLE lot
# for any dependencies - otherwise we'd have to add any of the dependencies for
# those others manually after dealing with the ones we can detect from the
# Mudlet executable and the Qt plugins...
echo ""
echo "Copying discord-rpc library in..."
cp -v -p "${GITHUB_WORKSPACE_UNIX_PATH}/3rdparty/discord/rpc/lib/discord-rpc64.dll"  .


# Lua libraries:
# If there is a demand for other rocks in the Windows installer because of
# revisions to the mappers or geyser framework or popular demand otherwise then
# the rock for those will also have to be installed and their C(.dll)/Lua (.lua)
# files included here:
echo ""
echo "Copying lua C libraries in..."
cp -v -p -t . \
    "${MINGW_INTERNAL_BASE_DIR}/lib/lua/5.1/lfs.dll" \
    "${MINGW_INTERNAL_BASE_DIR}/lib/lua/5.1/lpeg.dll" \
    "${MINGW_INTERNAL_BASE_DIR}/lib/lua/5.1/lsqlite3.dll" \
    "${MINGW_INTERNAL_BASE_DIR}/lib/lua/5.1/lua-utf8.dll" \
    "${MINGW_INTERNAL_BASE_DIR}/lib/lua/5.1/rex_pcre2.dll" \
    "${MINGW_INTERNAL_BASE_DIR}/lib/lua/5.1/yajl.dll"

mkdir ./luasql
cp -v -p "${MINGW_PREFIX}/lib/lua/5.1/luasql/sqlite3.dll" ./luasql/sqlite3.dll
mkdir ./brimworks
cp -v -p "${MINGW_PREFIX}/lib/lua/5.1/brimworks/zip.dll" ./brimworks/zip.dll
echo ""

echo "Copying OpenSSL libraries in..."
# The openSSL libraries has a different name depending on the bitness - but we
# only do 64-bits now:
cp -v -p -t . \
    "${MINGW_INTERNAL_BASE_DIR}/bin/libcrypto-3-x64.dll" \
    "${MINGW_INTERNAL_BASE_DIR}/bin/libssl-3-x64.dll"

echo ""
echo "Examining the Mudlet application and all the libraries and Qt plugins to identify other needed libraries..."

# The greps filter means we only get paths that:
# * do not contain "Qt6"
# * include the "root" directory of the particular bash terminal in use (to
#   capture those with "mingw64", "ucrt64" or "clang64" in them)
# * include "bin" for the path where the above keep their main library files
# The cuts ensures we only get the file and path to the library after the =>
# in the lines that match:
case "${MSYSTEM}" in
  *MINGW64*)
    NEEDED_LIBS_ARG=mingw64
    ;;
  *CLANG64*)
    NEEDED_LIBS_ARG=clang64
    ;;
  *UCRT64*)
    NEEDED_LIBS_ARG=ucrt64
    ;;
  *)
    echo "Uh, oh! Failed to work out what to use to identify the libraries we need to bundle!"
    exit 2
    ;;
esac

mapfile -t NEEDED_LIBS < <(${MINGW_INTERNAL_BASE_DIR}/bin/ntldd --recursive \
  ./mudlet.exe \
  ./*.dll \
  ./*/*.dll \
  | /usr/bin/grep -v 'Qt6' \
  | /usr/bin/grep "${NEEDED_LIBS_ARG}" \
  | /usr/bin/grep 'bin' \
  | /usr/bin/cut -d '>' -f2 \
  | /usr/bin/cut -d '(' -f1 \
  | /usr/bin/sort -u)

# echo ""
# echo "  In summary, the needed libraries are:"
# echo "${NEEDED_LIBS[@]}"

echo ""
echo "Copying identified libraries from Mudlet executable and plugins..."
# Note: DON'T double quote the array here - somehow extra leading or trailing
# spaces get into the call of `cygpath -au ${LIB}` which breaks things.
for LIB in ${NEEDED_LIBS[@]}; do
  # The ntldd above returns "Windows style pathFileNames"
  cp -p -v -t . "$(/usr/bin/cygpath -au "${LIB}")"
done
echo "    ... done copying identified libraries."

echo ""
echo "Copying Mudlet & Geyser Lua files and the Generic Mapper in..."
# Using the '/./' notation provides the point at which rsync reproduces the
# directory structure from the source into the target and avoids the need
# to change directory before and after the rsync call:

# As written it copies every file but it should be polished up to skip unneeded
# ones:
rsync -avR "${GITHUB_WORKSPACE_UNIX_PATH}"/src/mudlet-lua/./* ./mudlet-lua/
echo ""

echo "Copying Lua code formatter Lua files in..."
# As written it copies every file but it should be polished up to skip unneeded
# ones:
rsync -avR "${GITHUB_WORKSPACE_UNIX_PATH}"/3rdparty/lcf/./* ./lcf/
echo ""

echo "Copying Lua translation files in..."
mkdir -p ./translations/lua/translated
cp -v -p -t ./translations/lua/translated \
    "${GITHUB_WORKSPACE_UNIX_PATH}"/translations/lua/translated/mudlet-lua_??_??.json
cp -v -p -t ./translations/lua "${GITHUB_WORKSPACE_UNIX_PATH}/translations/lua/mudlet-lua.json"
echo ""

echo "Copying Hunspell dictionaries in..."
cp -v -p -t . \
    "${GITHUB_WORKSPACE_UNIX_PATH}"/src/*.aff \
    "${GITHUB_WORKSPACE_UNIX_PATH}"/src/*.dic
echo ""

cd "${GITHUB_WORKSPACE_UNIX_PATH}" || exit 1

# For debugging purposes:
# echo "The recursive contents of the Project build sub-directory $(cygpath --windows "~/src/mudlet/package"):"
# /usr/bin/ls -laR
# echo ""

echo "$(cygpath --windows "${PACKAGE_DIR}") should contain everything needed to run Mudlet!"
echo ""
echo "   ... package-mudlet-for-windows.sh shell script finished."
echo ""
cd ~ || exit 1

exit 0
