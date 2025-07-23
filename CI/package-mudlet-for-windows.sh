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
# 2 - Unsupported MSYS2/MINGGW shell type
# 3 - Unsupported build type
# 4 - Directory to be used to assemble the package is NOT empty
# 6 - No Mudlet.exe file found to work with

if [ "${MSYSTEM}" = "MSYS" ]; then
  echo "Please run this script from a MINGW64, CLANG64 or UCRT64 type bash terminal as the MSYS one"
  echo "does not support what is needed."
  exit 2
elif [ "${MSYSTEM}" = "MINGW64" ]; then
  export BUILD_BITNESS="64"
  export BUILDCOMPONENT="x86_64"
elif [ "${MSYSTEM}" = "CLANG64" ]; then
  export BUILD_BITNESS="64"
  export BUILDCOMPONENT="clang-x86_64"
elif [ "${MSYSTEM}" = "UCRT64" ]; then
  export BUILD_BITNESS="64"
  export BUILDCOMPONENT="ucrt-x86_64"
else
  echo "This script is not set up to handle systems of type ${MSYSTEM}, only"
  echo "MINGW64, CLANG64 or UCRT64 are currently supported. Please rerun this in a bash terminal of"
  echo "one of those types."
  exit 2
fi

BUILD_CONFIG="release"
if [ -z "${MINGW_INTERNAL_BASE_DIR}" ]; then
  MINGW_BASE_DIR="$(cygpath -m "${MSYSTEM_PREFIX}")"
  export MINGW_BASE_DIR
  MINGW_INTERNAL_BASE_DIR="$(cygpath -u "${MINGW_BASE_DIR}")"
  export MINGW_INTERNAL_BASE_DIR
fi
GITHUB_WORKSPACE_UNIX_PATH=$(echo "${GITHUB_WORKSPACE}" | sed 's|\\|/|g' | sed 's|D:|/d|g' | sed 's|C:|/c|g')
PACKAGE_DIR="${GITHUB_WORKSPACE_UNIX_PATH}/package-${MSYSTEM}-${BUILD_CONFIG}"

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

# The location that windeployqt6 puts the Qt translation files by default is "./translations"
# unfortunately this is not what
# "QLibraryInfo::path(QLibraryInfo::TranslationsPath)" in the calls to
# "QString mudlet::getMudletPath(const enums::mudletPathType, const QString&, const QString&)"
# with "enums::qtTranslationsPath" as the first argument returns:
# "./share/Qt6/translations" - which means the Qt translations were not getting
# loaded for our Windows builds:
"${MINGW_INTERNAL_BASE_DIR}/bin/windeployqt6" "--translationdir" "./share/qt6/translations" "./mudlet.exe"

# To determine which system libraries have to be copied in it requires
# continually trying to run the executable on the target type system
# and adding in the libraries to the same directory and repeating that
# until the executable actually starts to run. Alternatively running
# ntldd ./mudlet.exe | grep "/mingw64" inside an Mingw63 shell as appropriate 
# will produce the libraries that are likely to be needed below. Unfortunately
# this process is a little recursive in that you may have to repeat the
# process for individual librarys. For ones used by lua modules this
# can manifest as being unable to "require" the library within lua
# and doing the above "ntldd" check revealed that, for instance,
# "luasql/sqlite3.dll" needed "libsqlite3-0.dll"!
#
echo ""
echo "Examining Mudlet application to identify other needed libraries..."
NEEDED_LIBS=$("${MINGW_INTERNAL_BASE_DIR}/bin/ntldd" --recursive ./mudlet.exe \
  | /usr/bin/grep -v "Qt6" \
  | /usr/bin/grep -i "mingw\|clang\|ucrt" \
  | /usr/bin/cut -d ">" -f2 \
  | /usr/bin/cut -d "(" -f1 \
  | /usr/bin/sort \
  | /usr/bin/uniq)

# As well as the executable we also need to scan the plugins as they are not
# checked in the above use of ntldd as they get loaded dynamically on demand:
echo "Examining all the plugin sub-directories:"
PLUGIN_DIRS=( "generic" "iconengines" "imageformats" "multimedia" "networkinformation" "platforms" "styles" "texttospeech" "tls" )
for PLUGIN_DIR in "${PLUGIN_DIRS[@]}" ; do
  if [ -d "${PLUGIN_DIR}" ]; then
    echo "  Checking ${PLUGIN_DIR} directory..."
    PLUGIN_LIBS=$("${MINGW_INTERNAL_BASE_DIR}/bin/ntldd" --recursive ./${PLUGIN_DIR}/*.dll 2>/dev/null \
      | /usr/bin/grep -v "Qt6" \
      | /usr/bin/grep -i "mingw\|clang\|ucrt" \
      | /usr/bin/cut -d ">" -f2 \
      | /usr/bin/cut -d "(" -f1 \
      | /usr/bin/sort \
      | /usr/bin/uniq)
    NEEDED_LIBS=( "${NEEDED_LIBS[@]}" ${PLUGIN_LIBS} )
  fi
done

# Remove duplicates from the combined array
NEEDED_LIBS=( $(printf '%s\n' "${NEEDED_LIBS[@]}" | sort -u) )

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
echo "Copying additional system dependencies identified from ntldd analysis..."
# Copy all libraries that ntldd shows might be missing or loaded from system
ADDITIONAL_DEPS=(
    "libdatrie-1.dll"
    "libexpat-1.dll" 
    "libfribidi-0.dll"
    "libgmodule-2.0-0.dll"
    "libgmp-10.dll"
    "libgomp-1.dll"
    "libhogweed-6.dll"
    "libidn2-0.dll"
    "libnettle-8.dll"
    "libp11-kit-0.dll"
    "libpangoft2-1.0-0.dll"
    "libpangowin32-1.0-0.dll"
    "libshaderc_shared.dll"
    "libsharpyuv-0.dll"
    "libspirv-cross-c-shared.dll"
    "libtasn1-6.dll"
    "libthai-0.dll"
    "libunibreak-6.dll"
    "libunistring-5.dll"
    "vulkan-1.dll"
)

for DEP in "${ADDITIONAL_DEPS[@]}"; do
    if [ -f "${MINGW_INTERNAL_BASE_DIR}/bin/${DEP}" ]; then
        if [ ! -f "./${DEP}" ]; then
            echo "  Copying additional dependency: ${DEP}"
            cp -v -p "${MINGW_INTERNAL_BASE_DIR}/bin/${DEP}" .
        fi
    else
        echo "  WARNING: ${DEP} not found in ${MINGW_INTERNAL_BASE_DIR}/bin/"
    fi
done

echo ""
echo "Aggressively copying FFmpeg and Qt multimedia plugin DLLs for maximum compatibility..."
# Copy all FFmpeg DLLs from MSYS2/mingw64, vcpkg, and Qt bin directories
find "${MINGW_INTERNAL_BASE_DIR}/bin" -iname "avcodec-*.dll" -exec cp -v -p {} . \;
find "${MINGW_INTERNAL_BASE_DIR}/bin" -iname "avformat-*.dll" -exec cp -v -p {} . \;
find "${MINGW_INTERNAL_BASE_DIR}/bin" -iname "avutil-*.dll" -exec cp -v -p {} . \;
find "${MINGW_INTERNAL_BASE_DIR}/bin" -iname "swresample-*.dll" -exec cp -v -p {} . \;
find "${MINGW_INTERNAL_BASE_DIR}/bin" -iname "swscale-*.dll" -exec cp -v -p {} . \;
find "${MINGW_INTERNAL_BASE_DIR}/bin" -iname "*.dll" -exec cp -v -p {} . \;

echo ""
echo "Copying comprehensive Qt plugins like SlySven's approach..."

# Define plugins to copy with their respective directories (matching SlySven's setup)
declare -A QT_PLUGINS=(
  ["generic"]="qtuiotouchplugin.dll"
  ["iconengines"]="qsvgicon.dll"
  ["imageformats"]="qgif.dll qico.dll qjpeg.dll qpdf.dll qsvg.dll qwebp.dll"
  ["multimedia"]="ffmpegmediaplugin.dll dsengine.dll windowsmediaplugin.dll"
  ["networkinformation"]="qnetworklistmanager.dll"
  ["platforms"]="qdirect2d.dll qminimal.dll qoffscreen.dll qwindows.dll"
  ["styles"]="qwindowsvistastyle.dll"
  ["texttospeech"]="qtexttospeech_sapi.dll"
  ["tls"]="qschannelbackend.dll qcertonlybackend.dll"
)

# Create plugin directories and copy files
for PLUGIN_TYPE in "${!QT_PLUGINS[@]}"; do
  if [ ! -d "plugins/${PLUGIN_TYPE}" ]; then
    echo "  Creating plugins/${PLUGIN_TYPE} directory..."
    mkdir -p "plugins/${PLUGIN_TYPE}"
  fi
  
  # Convert plugin list to array
  read -ra PLUGIN_FILES <<< "${QT_PLUGINS[$PLUGIN_TYPE]}"
  
  for PLUGIN_FILE in "${PLUGIN_FILES[@]}"; do
    # Try multiple Qt plugin locations
    FOUND=false
    for QT_PLUGIN_DIR in \
      "${QT_DIR}/plugins/${PLUGIN_TYPE}" \
      "${MINGW_INTERNAL_BASE_DIR}/lib/qt6/plugins/${PLUGIN_TYPE}" \
      "${MINGW_INTERNAL_BASE_DIR}/share/qt6/plugins/${PLUGIN_TYPE}" \
      "${MINGW_INTERNAL_BASE_DIR}/plugins/${PLUGIN_TYPE}"; do
      
      if [ -f "${QT_PLUGIN_DIR}/${PLUGIN_FILE}" ]; then
        echo "    Copying ${PLUGIN_FILE} from ${QT_PLUGIN_DIR}..."
        cp "${QT_PLUGIN_DIR}/${PLUGIN_FILE}" "plugins/${PLUGIN_TYPE}/"
        FOUND=true
        break
      fi
    done
    
    if [ "$FOUND" = false ]; then
      echo "    WARNING: ${PLUGIN_FILE} not found in any Qt plugin directory"
    fi
  done
done

# Special focus on ffmpegmediaplugin.dll - critical for OGG/Opus
if [ ! -f "plugins/multimedia/ffmpegmediaplugin.dll" ]; then
  echo "  CRITICAL: ffmpegmediaplugin.dll missing - OGG/Opus support will not work!"
fi

echo ""
echo "Verifying ffmpegmediaplugin.dll dependencies and copying missing ones..."
if [ -f "plugins/multimedia/ffmpegmediaplugin.dll" ]; then
    echo "Checking dependencies of ffmpegmediaplugin.dll:"
    "${MINGW_INTERNAL_BASE_DIR}/bin/ntldd" "plugins/multimedia/ffmpegmediaplugin.dll" | grep -E "(not found|${MINGW_INTERNAL_BASE_DIR})"
    
    # Copy any missing dependencies specifically for the FFmpeg plugin
    FFMPEG_DEPS=$("${MINGW_INTERNAL_BASE_DIR}/bin/ntldd" "plugins/multimedia/ffmpegmediaplugin.dll" 2>/dev/null \
      | grep "${MINGW_INTERNAL_BASE_DIR}" \
      | cut -d ">" -f2 \
      | cut -d "(" -f1 \
      | tr -d ' ')
    
    echo "FFmpeg plugin dependencies found:"
    for DEP in ${FFMPEG_DEPS}; do
        DEP_NAME=$(basename "${DEP}")
        echo "  Checking: ${DEP_NAME}"
        if [ -f "${DEP}" ] && [ ! -f "./${DEP_NAME}" ]; then
            echo "    Copying missing FFmpeg plugin dependency: ${DEP_NAME}"
            cp -v -p "${DEP}" .
        else
            echo "    Already present: ${DEP_NAME}"
        fi
    done
    
    echo "FFmpeg plugin verification complete."
else
    echo "ERROR: ffmpegmediaplugin.dll not found - OGG/Opus will not work!"
    echo "Available multimedia plugins:"
    ls -la plugins/multimedia/ || echo "No multimedia plugins directory found"
fi

echo ""
echo "Copying OpenSSL libraries in..."
# The openSSL libraries has a different name depending on the bitness - but we
# only do 64-bits now:
cp -v -p -t . \
    "${MINGW_INTERNAL_BASE_DIR}/bin/libcrypto-3-x64.dll" \
    "${MINGW_INTERNAL_BASE_DIR}/bin/libssl-3-x64.dll"

echo ""
echo "Copying discord-rpc library in..."
cp -v -p "${GITHUB_WORKSPACE_UNIX_PATH}/3rdparty/discord/rpc/lib/discord-rpc64.dll"  .
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
echo "Creating debug batch file for multimedia plugin diagnostics..."
cat > mudlet-debug.bat << 'EOF'
@echo off
echo Setting debug environment variables for Qt multimedia troubleshooting...
set QT_MEDIA_PLUGINS=1
set QT_DEBUG_PLUGINS=1
set QT_LOGGING_RULES=qt.multimedia*=true
echo Starting Mudlet with multimedia debug output...
mudlet.exe
pause
EOF
echo "Created mudlet-debug.bat - run this to see detailed plugin loading information"

echo ""

# For debugging purposes:
# echo "The recursive contents of the Project build sub-directory $(/usr/bin/cygpath --windows "~/src/mudlet/package"):"
# /usr/bin/ls -aRl
# echo ""

echo ""
echo "Final verification of multimedia plugin setup..."
echo "Multimedia plugins present:"
ls -la plugins/multimedia/ 2>/dev/null || echo "No multimedia plugins directory found"

echo ""
echo "FFmpeg libraries present in main directory:"
ls -la *ffmpeg* *avcodec* *avformat* *avutil* *swresample* *swscale* 2>/dev/null || echo "No FFmpeg libraries found"

echo ""
echo "Critical multimedia files check:"
if [ -f "plugins/multimedia/ffmpegmediaplugin.dll" ]; then
    echo "  ✓ ffmpegmediaplugin.dll found - OGG/Opus support should work"
else
    echo "  ✗ ffmpegmediaplugin.dll MISSING - OGG/Opus support will NOT work"
fi

if [ -f "avcodec-61.dll" ] && [ -f "avformat-61.dll" ] && [ -f "avutil-59.dll" ]; then
    echo "  ✓ Core FFmpeg libraries found"
else
    echo "  ✗ Some core FFmpeg libraries missing"
fi

echo ""
echo "Package verification complete. For OGG/Opus troubleshooting:"
echo "1. Run mudlet-debug.bat to see plugin loading details"
echo "2. Check that ffmpegmediaplugin.dll is in plugins/multimedia/"
echo "3. Verify all FFmpeg libraries are in the main directory"
echo "4. Set QT_MEDIA_BACKEND=ffmpeg if needed"

FINAL_DIR=$(/usr/bin/cygpath --windows "${PACKAGE_DIR}")
echo ""
echo "${FINAL_DIR} should contain everything needed to run Mudlet!"
echo ""
echo "   ... package-mudlet-for-windows.sh shell script finished."
echo ""
cd ~ || exit 1

exit 0
