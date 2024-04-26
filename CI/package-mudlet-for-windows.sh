#!/bin/bash
###########################################################################
#   Copyright (C) 2023-2024  by Stephen Lyons - slysven@virginmedia.com   #
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

# Version: 1.5.0    Change BUILD_TYPE to BUILD_CONFIG to avoid clash with
#                   CI/CB system using same variable
#        : 1.4.0    No change
#          1.3.0    Remove used of the no longer supported/used by us QT5
#                   Gamepad stuff (since PR #6787 was merged into
#                   the development branch)
#          1.2.0    No changes
#          1.1.0    Updated to bail out if there isn't a mudlet.exe file to
#                   work with
#          1.0.0    Original version

# Script to each time to package all the files needed to run Mudlet on
# Windows in a archive file that can be used both on this PC and others.

# To be used AFTER setup-windows-sdk.sh and build-mudlet-for-windows.sh
# have been run.

# Exit codes:
# 0 - Everything is fine. 8-)
# 1 - Failure to change to a directory
# 2 - Unsupported MSYS2/MINGGW shell type
# 3 - Unsupported build type
# 4 - Directory to be used to assemble the package is NOT empty
# 6 - No Mudlet.exe file found to work with

if [ "${BUILD_CONFIG}" != "release" ] && [ "${BUILD_CONFIG}" != "debug" ]; then
  echo "Please set the environmental variable BUILD_CONFIG to one of \"release\" or"
  echo "\"debug\" to specify which type of build you wish this to be."
  exit 3
fi

if [ "${MSYSTEM}" = "MSYS" ]; then
  echo "Please run this script from an MINGW32 or MINGW64 type bash terminal appropriate"
  echo "to the bitness you want to work on. You may do this once for each of them should"
  echo "you wish to do both."
  exit 2
elif [ "${MSYSTEM}" = "MINGW32" ]; then
  export BUILD_BITNESS="32"
  export BUILDCOMPONENT="i686"
elif [ "${MSYSTEM}" = "MINGW64" ]; then
  export BUILD_BITNESS="64"
  export BUILDCOMPONENT="x86_64"
else
  echo "This script is not set up to handle systems of type ${MSYSTEM}, only MINGW32 or"
  echo "MINGW64 are currently supported. Please rerun this in a bash terminal of one"
  echo "of those two types."
  exit 2
fi

MINGW_BASE_DIR="C:/msys64/mingw${BUILD_BITNESS}"
export MINGW_BASE_DIR
MINGW_INTERNAL_BASE_DIR="/mingw${BUILD_BITNESS}"
export MINGW_INTERNAL_BASE_DIR
PATH="${MINGW_INTERNAL_BASE_DIR}/usr/local/bin:${MINGW_INTERNAL_BASE_DIR}/bin:/usr/bin:${PATH}"
export PATH
GITHUB_WORKSPACE_UNIX_PATH=$(echo ${GITHUB_WORKSPACE} | sed 's|\\|/|g' | sed 's|D:|/d|g')
PACKAGE_DIR="${GITHUB_WORKSPACE_UNIX_PATH}/package-${MSYSTEM}-${BUILD_CONFIG}"

echo "MSYSTEM is: ${MSYSTEM}"
echo "PATH is now:"
echo "${PATH}"
echo ""

cd $GITHUB_WORKSPACE_UNIX_PATH || exit 1

if [ -d "${PACKAGE_DIR}" ]; then
  # The wanted packaging dir exists - as is wanted
  echo ""
  echo "Checking for an empty ${PACKAGE_DIR} in which to assemble files..."
  echo ""
  if [ -n "$(ls -A ${PACKAGE_DIR})" ]; then
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

echo "Copying wanted compiled files from ${GITHUB_WORKSPACE}/build-${MSYSTEM} to ${GITHUB_WORKSPACE}/package-${MSYSTEM} ..."
echo ""

if [ ! -f "${GITHUB_WORKSPACE_UNIX_PATH}/build-${MSYSTEM}/${BUILD_CONFIG}/mudlet.exe" ]; then
  echo "ERROR: no Mudlet executable found - did the previous build"
  echo "complete sucessfully?"
  exit 6
fi

cp "${GITHUB_WORKSPACE_UNIX_PATH}/build-${MSYSTEM}/${BUILD_CONFIG}/mudlet.exe" "${PACKAGE_DIR}/"
if [ -f "${GITHUB_WORKSPACE_UNIX_PATH}/build-${MSYSTEM}/${BUILD_CONFIG}/mudlet.exe.debug" ]; then
  cp "${GITHUB_WORKSPACE_UNIX_PATH}/build-${MSYSTEM}/${BUILD_CONFIG}/mudlet.exe.debug" "${PACKAGE_DIR}/"
fi

# Since Qt 5.14 using the --release switch is broken (it now seems to be
# assumed), --debug still seems to work - sort of - it doesn't copy the
# MINGW .debug files for the Qt libraries.
# https://bugreports.qt.io/browse/QTBUG-80806 seems relevant.
echo "Running windeployqt..."
if [ "${BUILD_CONFIG}" = "debug" ]; then
  "${MINGW_INTERNAL_BASE_DIR}/bin/windeployqt" --debug --no-virtualkeyboard ./mudlet.exe
  ZIP_FILE_NAME="Mudlet-${MSYSTEM}-debug"
  # Stupidly windeployqt does not copy the .debug files that actually contains
  # the debug information for the Qt library files - so copy them manually:
  # They have been deduced by looking for matching 'Xxxx.dll.debug' files for
  # each 'Xxxx.dll' one so A) they may not be complete and B) are only for
  # the Qt libraries - other third party ones are not necessarily covered
  # so far:
  libnames_core=("Qt5Core.dll" "Qt5Gui.dll" "Qt5Multimedia.dll" "Qt5Network.dll" "Qt5Svg.dll" "Qt5Widgets.dll" "Qt5TextToSpeech.dll")
  libnames_plugins_audio=("qtaudio_windows.dll")
  libnames_plugins_bearer=("qgenericbearer.dll")
  libnames_plugins_iconengines=("qsvgicon.dll")
  if [ "${MSYSTEM}" = "MINGW64" ]; then
    libnames_plugins_imageformats=("qgif.dll" "qicns.dll" "qico.dll" "qjp2.dll" "qjpeg.dll" "qmng.dll" "qsvg.dll" "qtga.dll" "qtiff.dll" "qwbmp.dll" "qwebp.dll")
  elif [ "${MSYSTEM}" = "MINGW32" ]; then
    # The library for the "multiple network graphics" file format is not
    # included in the MINGW32 environment!
    libnames_plugins_imageformats=("qgif.dll" "qicns.dll" "qico.dll" "qjp2.dll" "qjpeg.dll" "qsvg.dll" "qtga.dll" "qtiff.dll" "qwbmp.dll" "qwebp.dll")
  fi
  libnames_plugins_mediaservice=("dsengine.dll" "qtmedia_audioengine.dll" "wmfengine.dll")
  libnames_plugins_platforms=("qwindows.dll")
  libnames_plugins_playlistformats=("qtmultimedia_m3u.dll")
  libnames_plugins_styles=("qwindowsvistastyle.dll")
  libnames_plugins_texttospeech=("qtexttospeech_sapi.dll")

  # The core library files are located in a bin directory rather than a Qt
  # share one:
  for libname in "${libnames_core[@]}"; do
    cp -v -p "${MINGW_BASE_DIR}/bin/${libname}.debug" .
  done

  for libname in "${libnames_plugins_audio[@]}"; do
	cp -v -p "${MINGW_BASE_DIR}/share/qt5/plugins/audio/${libname}.debug" ./audio/
  done

  for libname in "${libnames_plugins_bearer[@]}"; do
    cp -v -p "${MINGW_BASE_DIR}/share/qt5/plugins/bearer/${libname}.debug" ./bearer/
  done

  for libname in "${libnames_plugins_iconengines[@]}"; do
    cp -v -p "${MINGW_BASE_DIR}/share/qt5/plugins/iconengines/${libname}.debug" ./iconengines/
  done

  for libname in "${libnames_plugins_imageformats[@]}"; do
    cp -v -p "${MINGW_BASE_DIR}/share/qt5/plugins/imageformats/${libname}.debug" ./imageformats/
  done

  for libname in "${libnames_plugins_mediaservice[@]}"; do
    cp -v -p "${MINGW_BASE_DIR}/share/qt5/plugins/mediaservice/${libname}.debug" ./mediaservice/
  done

  for libname in "${libnames_plugins_platforms[@]}"; do
    cp -v -p "${MINGW_BASE_DIR}/share/qt5/plugins/platforms/${libname}.debug" ./platforms/
  done

  for libname in "${libnames_plugins_playlistformats[@]}"; do
    cp -v -p "${MINGW_BASE_DIR}/share/qt5/plugins/playlistformats/${libname}.debug" ./playlistformats/
  done

  for libname in "${libnames_plugins_styles[@]}"; do
    cp -v -p "${MINGW_BASE_DIR}/share/qt5/plugins/styles/${libname}.debug" ./styles/
  done

  for libname in "${libnames_plugins_texttospeech[@]}"; do
    cp -v -p "${MINGW_BASE_DIR}/share/qt5/plugins/texttospeech/${libname}.debug" ./texttospeech/
  done
else
  "${MINGW_INTERNAL_BASE_DIR}/bin/windeployqt6" --release --no-virtualkeyboard ./mudlet.exe
  ZIP_FILE_NAME="Mudlet-${MSYSTEM}"
fi


# To determine which system libraries have to be copied in it requires
# continually trying to run the executable on the target type system
# and adding in the libraries to the same directory and repeating that
# until the executable actually starts to run. Alternatively running
# ntldd ./mudlet.exe | grep "/mingw32" {for the 32 bit case, use "64" for
# the other one} inside an Mingw32 (or 64) shell as appropriate will
# produce the libraries that are likely to be needed below. Unfortunetly
# this process is a little recursive in that you may have to repeat the
# process for individual librarys. For ones used by lua modules this
# can manifest as being unable to "require" the library within lua
# and doing the above "ntldd" check revealed that, for instance,
# "luasql/sqlite3.dll" needed "libsqlite3-0.dll"!
#
echo ""
echo "Examining Mudlet application to identify other needed libraries..."
NEEDED_LIBS=$("${MINGW_INTERNAL_BASE_DIR}/bin/ntldd" --recursive ./mudlet.exe \
  | /usr/bin/grep -v "Qt5" \
  | /usr/bin/grep -i "mingw" \
  | /usr/bin/cut -d ">" -f2 \
  | /usr/bin/cut -d "(" -f1 \
  | /usr/bin/sort)
echo ""
echo "Copying these identified libraries..."
for LIB in ${NEEDED_LIBS} ; do
  cp -v -p "${LIB}" . ;
done

echo ""
echo "Copying other, known to be needed, libraries in..."
# libjasper to libwebpdemux-2 are additional image format handlers that Qt can
# use if they are present.
# libsqlite3 and libyajl are needed by lua modules (luasql-sqlite3) and at Mudlet run time.
cp -v -p -t . \
    "${MINGW_INTERNAL_BASE_DIR}/bin/libjasper.dll" \
    "${MINGW_INTERNAL_BASE_DIR}/bin/libjpeg-8.dll" \
    "${MINGW_INTERNAL_BASE_DIR}/bin/libtiff-6.dll" \
    "${MINGW_INTERNAL_BASE_DIR}/bin/libwebp-7.dll" \
    "${MINGW_INTERNAL_BASE_DIR}/bin/libwebpdemux-2.dll" \
    "${MINGW_INTERNAL_BASE_DIR}/bin/libsqlite3-0.dll" \
    "${MINGW_INTERNAL_BASE_DIR}/bin/libyajl.dll"

echo ""
echo "Copying OpenSSL libraries in..."
# The openSSL libraries has a different name depending on the bitness:
if [ "${MSYSTEM}" = "MINGW32" ]; then
    cp -v -p -t . \
        "${MINGW_INTERNAL_BASE_DIR}/bin/libcrypto-3.dll" \
        "${MINGW_INTERNAL_BASE_DIR}/bin/libssl-3.dll"

elif [ "${MSYSTEM}" = "MINGW64" ]; then
    cp -v -p -t . \
        "${MINGW_INTERNAL_BASE_DIR}/bin/libcrypto-3-x64.dll" \
        "${MINGW_INTERNAL_BASE_DIR}/bin/libssl-3-x64.dll"

fi

echo ""
echo "Copying discord-rpc library in..."
if [ "${MSYSTEM}" = "MINGW32" ]; then
    cp -v -p "${GITHUB_WORKSPACE_UNIX_PATH}/3rdparty/discord/rpc/lib/discord-rpc32.dll"  .
elif [ "${MSYSTEM}" = "MINGW64" ]; then
    cp -v -p "${GITHUB_WORKSPACE_UNIX_PATH}/3rdparty/discord/rpc/lib/discord-rpc64.dll"  .
fi
echo ""

# Lua libraries:
# If there is a demand for other rocks in the Windows installer because of
# revisions to the mappers or geyser framework or popular demand otherwise then
# the rock for those will also have to be installed and their C(.dll)/Lua (.lua)
# files included here:
echo "Copying lua C libraries in..."
cp -v -p -t . \
    "${MINGW_INTERNAL_BASE_DIR}/lib/lua/5.1/lfs.dll" \
    "${MINGW_INTERNAL_BASE_DIR}/lib/lua/5.1/lpeg.dll" \
    "${MINGW_INTERNAL_BASE_DIR}/lib/lua/5.1/lsqlite3.dll" \
    "${MINGW_INTERNAL_BASE_DIR}/lib/lua/5.1/lua-utf8.dll" \
    "${MINGW_INTERNAL_BASE_DIR}/lib/lua/5.1/rex_pcre.dll" \
    "${MINGW_INTERNAL_BASE_DIR}/lib/lua/5.1/yajl.dll"

mkdir ./luasql
cp -v -p "${MINGW_INTERNAL_BASE_DIR}/lib/lua/5.1/luasql/sqlite3.dll" ./luasql/sqlite3.dll
mkdir ./brimworks
cp -v -p "${MINGW_INTERNAL_BASE_DIR}/lib/lua/5.1/brimworks/zip.dll" ./brimworks/zip.dll
echo ""

echo "Copying Mudlet & Geyser Lua files and the Generic Mapper in..."
# Using the '/./' notation provides the point at which rsync reproduces the
# directory structure from the source into the target and avoids the need
# to change directory before and after the rsync call:

# As written it copies every file but it should be polished up to skip unneeded
# ones:
rsync -avR ${GITHUB_WORKSPACE_UNIX_PATH}/src/mudlet-lua/./* ./mudlet-lua/
echo ""

echo "Copying Lua code formatter Lua files in..."
# As written it copies every file but it should be polished up to skip unneeded
# ones:
rsync -avR ${GITHUB_WORKSPACE_UNIX_PATH}/3rdparty/lcf/./* ./lcf/
echo ""

echo "Copying Lua translation files in..."
mkdir -p ./translations/lua/translated
cp -v -p -t ./translations/lua/translated \
    ${GITHUB_WORKSPACE_UNIX_PATH}/translations/lua/translated/mudlet-lua_??_??.json
echo ""

echo "Copying Hunspell dictionaries in..."
cp -v -p -t . \
    ${GITHUB_WORKSPACE_UNIX_PATH}/src/*.aff \
    ${GITHUB_WORKSPACE_UNIX_PATH}/src/*.dic

echo ""

# For debugging purposes:
# echo "The recursive contents of the Project build sub-directory $(/usr/bin/cygpath --windows "~/src/mudlet/package"):"
# /usr/bin/ls -aRl
# echo ""

FINAL_DIR=$(/usr/bin/cygpath --windows "${PACKAGE_DIR}")
echo "${FINAL_DIR} should contain everything needed to run Mudlet!"
echo ""
echo "   ... package-mudlet-for-windows.sh shell script finished."
echo ""
echo "   You may now run the mudlet.exe file in ${FINAL_DIR} or take the file"
echo "   there: ${ZIP_FILE_NAME}.zip to somewhere else - even a different PC - unzip"
echo "   everything and run the mudlet.exe file extracted from it..."
cd ~ || exit 1

exit 0