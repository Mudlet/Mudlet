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
echo "Copying critical runtime dependencies for Mudlet core functionality..."
# These are essential DLLs that Mudlet requires to start up
CRITICAL_RUNTIME_DEPS=(
    "lua51.dll"
    "libgcc_s_seh-1.dll"
    "libpcre-1.dll"
    "libpugixml.dll"
    "libstdc++-6.dll"
    "libwinpthread-1.dll"
    "libzip.dll"
    "libzstd.dll"
    "libbz2-1.dll"
    "liblzma-5.dll"
    "libhunspell-1.7-0.dll"
    "libboost_filesystem-mt.dll"
    "libboost_system-mt.dll"
    "zlib1.dll"
    "libicuuc77.dll"
    "libicuin77.dll"
    "libicudt77.dll"
    "libdouble-conversion.dll"
    "libpcre2-16-0.dll"
    "libpcre2-8-0.dll"
    "libfreetype-6.dll"
    "libglib-2.0-0.dll"
    "libgraphite2.dll"
    "libharfbuzz-0.dll"
    "libintl-8.dll"
    "libiconv-2.dll"
    "libpng16-16.dll"
    "libbrotlidec.dll"
    "libbrotlicommon.dll"
    "libmd4c.dll"
    "libsndfile-1.dll"
    "libFLAC-12.dll"
    "libmad-0.dll"
    "libportaudio-2.dll"
    "libmpg123-0.dll"
    "libvorbisfile-3.dll"
    "libopencore-amrnb-0.dll"
    "libopencore-amrwb-0.dll"
    "libvorbisenc-2.dll"
    "libtheoradec-2.dll"
    "libtheoraenc-2.dll"
)

for CRITICAL_DLL in "${CRITICAL_RUNTIME_DEPS[@]}"; do
    if [ -f "${MINGW_INTERNAL_BASE_DIR}/bin/${CRITICAL_DLL}" ]; then
        if [ ! -f "./${CRITICAL_DLL}" ]; then
            echo "  Copying critical runtime dependency: ${CRITICAL_DLL}"
            cp -v -p "${MINGW_INTERNAL_BASE_DIR}/bin/${CRITICAL_DLL}" .
        else
            echo "  Already present: ${CRITICAL_DLL}"
        fi
    else
        echo "  WARNING: Critical dependency ${CRITICAL_DLL} not found in ${MINGW_INTERNAL_BASE_DIR}/bin/"
        # Try to find with wildcards for version-specific libraries
        BASE_NAME="${CRITICAL_DLL%%-*}"
        BASE_NAME="${BASE_NAME%%.*}"
        FOUND_DLL=$(find "${MINGW_INTERNAL_BASE_DIR}/bin" -name "${BASE_NAME}*.dll" | head -1)
        if [ -n "$FOUND_DLL" ]; then
            echo "    Found alternative: $(basename "${FOUND_DLL}")"
            cp -v -p "${FOUND_DLL}" .
        else
            echo "    ERROR: No alternative found for ${CRITICAL_DLL} - Mudlet may fail to start!"
            
            # Special handling for libmd4c - try additional locations
            if [[ "$CRITICAL_DLL" == "libmd4c.dll" ]]; then
                echo "    Searching for libmd4c in additional locations..."
                for SEARCH_DIR in \
                    "${MINGW_INTERNAL_BASE_DIR}/lib" \
                    "${MINGW_INTERNAL_BASE_DIR}" \
                    "/usr/lib" \
                    "/usr/local/lib"; do
                    
                    if [ -d "$SEARCH_DIR" ]; then
                        FOUND_MD4C=$(find "$SEARCH_DIR" -name "*md4c*.dll" 2>/dev/null | head -1)
                        if [ -n "$FOUND_MD4C" ]; then
                            echo "    Found libmd4c variant: $FOUND_MD4C"
                            cp -v -p "$FOUND_MD4C" "./libmd4c.dll"
                            break
                        fi
                    fi
                done
            fi
        fi
    fi
done

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

# Copy specific FFmpeg DLLs that we know work with Qt6 multimedia
FFmpeg_DLLS=(
    "avcodec-61.dll"
    "avformat-61.dll" 
    "avutil-59.dll"
    "swresample-5.dll"
    "swscale-8.dll"
    "avfilter-10.dll"
    "postproc-58.dll"
    "avdevice-61.dll"
)

echo "Copying core FFmpeg libraries..."
for DLL in "${FFmpeg_DLLS[@]}"; do
    if [ -f "${MINGW_INTERNAL_BASE_DIR}/bin/${DLL}" ]; then
        echo "  Copying: ${DLL}"
        cp -v -p "${MINGW_INTERNAL_BASE_DIR}/bin/${DLL}" .
    else
        echo "  WARNING: ${DLL} not found in ${MINGW_INTERNAL_BASE_DIR}/bin/"
        # Try to find with wildcard
        FOUND_DLL=$(find "${MINGW_INTERNAL_BASE_DIR}/bin" -name "${DLL%%-*}-*.dll" | head -1)
        if [ -n "$FOUND_DLL" ]; then
            echo "    Found alternative: $(basename "${FOUND_DLL}")"
            cp -v -p "${FOUND_DLL}" .
        fi
    fi
done

# Copy all multimedia codec libraries that FFmpeg might need
CODEC_DLLS=(
    "libass-9.dll"
    "libaom.dll"
    "libdav1d-7.dll"
    "libgsm.dll"
    "liblc3-1.dll"
    "libmp3lame-0.dll"
    "libogg-0.dll"
    "libopenal-1.dll"
    "libopencore-amrnb-0.dll"
    "libopencore-amrwb-0.dll"
    "libopenjp2-7.dll"
    "libopus-0.dll"
    "librav1e.dll"
    "libspeex-1.dll"
    "libtheoradec-2.dll"
    "libtheoraenc-2.dll"
    "libvorbis-0.dll"
    "libvorbisenc-2.dll"
    "libvpx-1.dll"
    "libwebp-7.dll"
    "libwebpdemux-2.dll"
    "libwebpmux-3.dll"
    "libx264-164.dll"
    "libx265-215.dll"
    "xvidcore.dll"
    "libFLAC-12.dll"
    "libsndfile-1.dll"
    "libwavpack-1.dll"
    "libmodplug-1.dll"
    "libmpg123-0.dll"
    "libvorbisfile-3.dll"
    "libshout-3.dll"
    "libsamplerate-0.dll"
)

echo "Copying codec libraries for FFmpeg..."
for DLL in "${CODEC_DLLS[@]}"; do
    if [ -f "${MINGW_INTERNAL_BASE_DIR}/bin/${DLL}" ]; then
        echo "  Copying codec: ${DLL}"
        cp -v -p "${MINGW_INTERNAL_BASE_DIR}/bin/${DLL}" .
    else
        # Try to find with version wildcards for codec libraries
        BASE_NAME="${DLL%%-*}"
        FOUND_DLL=$(find "${MINGW_INTERNAL_BASE_DIR}/bin" -name "${BASE_NAME}-*.dll" -o -name "${BASE_NAME}.dll" | head -1)
        if [ -n "$FOUND_DLL" ]; then
            echo "  Copying codec variant: $(basename "${FOUND_DLL}")"
            cp -v -p "${FOUND_DLL}" .
        fi
    fi
done

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
    echo "Checking dependencies of ffmpegmediaplugin.dll with ntldd:"
    
    # Use ntldd to find all missing dependencies
    echo "=== Full dependency analysis ==="
    "${MINGW_INTERNAL_BASE_DIR}/bin/ntldd" "plugins/multimedia/ffmpegmediaplugin.dll" 2>/dev/null || echo "ntldd failed"
    
    echo ""
    echo "=== Missing dependencies ==="
    MISSING_DEPS=$("${MINGW_INTERNAL_BASE_DIR}/bin/ntldd" "plugins/multimedia/ffmpegmediaplugin.dll" 2>/dev/null \
      | grep "not found" \
      | cut -d "=" -f1 \
      | tr -d ' ')
    
    if [ -n "$MISSING_DEPS" ]; then
        echo "Found missing dependencies:"
        for DEP in $MISSING_DEPS; do
            echo "  MISSING: $DEP"
            # Try to find it in MINGW
            if [ -f "${MINGW_INTERNAL_BASE_DIR}/bin/${DEP}" ]; then
                echo "    Found in MINGW - copying..."
                cp -v -p "${MINGW_INTERNAL_BASE_DIR}/bin/${DEP}" .
            else
                echo "    Not found in MINGW bin directory"
            fi
        done
    else
        echo "No missing dependencies detected by ntldd"
    fi
    
    echo ""
    echo "=== Dependencies from MINGW that should be included ==="
    MINGW_DEPS=$("${MINGW_INTERNAL_BASE_DIR}/bin/ntldd" "plugins/multimedia/ffmpegmediaplugin.dll" 2>/dev/null \
      | grep "${MINGW_INTERNAL_BASE_DIR}" \
      | cut -d ">" -f2 \
      | cut -d "(" -f1 \
      | tr -d ' ')
    
    echo "FFmpeg plugin dependencies found in MINGW:"
    for DEP in $MINGW_DEPS; do
        DEP_NAME=$(basename "${DEP}")
        echo "  Checking: ${DEP_NAME}"
        if [ -f "${DEP}" ] && [ ! -f "./${DEP_NAME}" ]; then
            echo "    Copying missing FFmpeg plugin dependency: ${DEP_NAME}"
            cp -v -p "${DEP}" .
        elif [ -f "./${DEP_NAME}" ]; then
            echo "    Already present: ${DEP_NAME}"
        else
            echo "    WARNING: Dependency file not found: ${DEP}"
        fi
    done
    
    echo ""
    echo "=== Comprehensive missing dependency scan ==="
    # Scan all DLLs in the package for missing dependencies
    echo "Scanning ALL DLLs for missing dependencies..."
    ALL_MISSING=$("${MINGW_INTERNAL_BASE_DIR}/bin/ntldd" --recursive ./*/*.dll ./*.dll 2>/dev/null \
      | grep "not found" \
      | cut -d "=" -f1 \
      | tr -d ' ' \
      | sort -u)
    
    if [ -n "$ALL_MISSING" ]; then
        echo "Additional missing dependencies found:"
        for DEP in $ALL_MISSING; do
            # Skip Windows system DLLs that we can't/shouldn't bundle
            case "$DEP" in
                *ext-ms-*|*api-ms-*|*kernel32.dll|*user32.dll|*shell32.dll|*advapi32.dll|*ole32.dll|*oleaut32.dll|*winmm.dll|*wsock32.dll|*ws2_32.dll|*gdi32.dll|*comdlg32.dll|*winspool.drv|*msvcrt.dll|*mpr.dll|*version.dll|*setupapi.dll|*imagehlp.dll|*psapi.dll|*userenv.dll|*netapi32.dll|*HvsiFileTrust.dll|*PdmUtilities.dll)
                    echo "  SKIP (system): $DEP"
                    ;;
                *)
                    echo "  MISSING: $DEP"
                    if [ -f "${MINGW_INTERNAL_BASE_DIR}/bin/${DEP}" ]; then
                        echo "    Copying from MINGW: ${DEP}"
                        cp -v -p "${MINGW_INTERNAL_BASE_DIR}/bin/${DEP}" .
                    fi
                    ;;
            esac
        done
    else
        echo "No additional missing dependencies found"
    fi
    
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
set QT_LOGGING_RULES=qt.multimedia*=true;qt.core.plugin*=true;qt.core.library*=true;*.ffmpeg*=true;qt.multimedia.ffmpeg*=true
set QT_PLUGIN_PATH=%~dp0plugins
set QT_MEDIA_BACKEND=ffmpeg
echo.
echo Qt Plugin Path: %QT_PLUGIN_PATH%
echo Media Backend: %QT_MEDIA_BACKEND%
echo.
echo Checking for critical runtime dependencies...
echo === Core Runtime Libraries ===
if exist "lua51.dll" (
    echo [OK] lua51.dll found
) else (
    echo [ERROR] lua51.dll missing! - Mudlet will not start
)
if exist "libgcc_s_seh-1.dll" (
    echo [OK] libgcc_s_seh-1.dll found
) else (
    echo [ERROR] libgcc_s_seh-1.dll missing! - Mudlet will not start
)
if exist "libpcre-1.dll" (
    echo [OK] libpcre-1.dll found
) else (
    echo [ERROR] libpcre-1.dll missing! - Mudlet will not start
)
if exist "libpugixml.dll" (
    echo [OK] libpugixml.dll found
) else (
    echo [ERROR] libpugixml.dll missing! - Mudlet will not start
)
if exist "libstdc++-6.dll" (
    echo [OK] libstdc++-6.dll found
) else (
    echo [ERROR] libstdc++-6.dll missing! - Mudlet will not start
)
if exist "zlib1.dll" (
    echo [OK] zlib1.dll found
) else (
    echo [ERROR] zlib1.dll missing! - Mudlet will not start
)
if exist "libicuuc77.dll" (
    echo [OK] libicuuc77.dll found
) else (
    echo [ERROR] libicuuc77.dll missing! - Mudlet will not start
)
if exist "libdouble-conversion.dll" (
    echo [OK] libdouble-conversion.dll found
) else (
    echo [ERROR] libdouble-conversion.dll missing! - Mudlet will not start
)
if exist "libmd4c.dll" (
    echo [OK] libmd4c.dll found
) else (
    echo [ERROR] libmd4c.dll missing! - Mudlet will not start
)
echo.
echo === Additional Qt Dependencies ===
if exist "libicuin77.dll" (
    echo [OK] libicuin77.dll found
) else (
    echo [ERROR] libicuin77.dll missing! - May affect Qt functionality
)
if exist "libicudt77.dll" (
    echo [OK] libicudt77.dll found
) else (
    echo [ERROR] libicudt77.dll missing! - May affect Qt functionality
)
if exist "libfreetype-6.dll" (
    echo [OK] libfreetype-6.dll found
) else (
    echo [ERROR] libfreetype-6.dll missing! - May affect text rendering
)
echo.
echo === Multimedia Libraries ===
if exist "plugins\multimedia\ffmpegmediaplugin.dll" (
    echo [OK] ffmpegmediaplugin.dll found
) else (
    echo [ERROR] ffmpegmediaplugin.dll missing!
)
if exist "avcodec-61.dll" (
    echo [OK] avcodec-61.dll found
) else (
    echo [ERROR] avcodec-61.dll missing!
)
if exist "libogg-0.dll" (
    echo [OK] libogg-0.dll found
) else (
    echo [ERROR] libogg-0.dll missing!
)
if exist "libvorbis-0.dll" (
    echo [OK] libvorbis-0.dll found  
) else (
    echo [ERROR] libvorbis-0.dll missing!
)
if exist "libopus-0.dll" (
    echo [OK] libopus-0.dll found
) else (
    echo [ERROR] libopus-0.dll missing!
)
if exist "libsndfile-1.dll" (
    echo [OK] libsndfile-1.dll found
) else (
    echo [ERROR] libsndfile-1.dll missing!
)
if exist "libFLAC-12.dll" (
    echo [OK] libFLAC-12.dll found
) else (
    echo [ERROR] libFLAC-12.dll missing!
)
echo.
echo === Audio System Check ===
if exist "libopenal-1.dll" (
    echo [OK] libopenal-1.dll found
) else (
    echo [ERROR] libopenal-1.dll missing - May affect audio output!
)
echo.
echo Starting Mudlet with multimedia debug output...
echo Watch for "Cannot load library" or "procedure could not be found" errors
echo.
echo === Advanced Audio Troubleshooting ===
echo If you hear no sound, try these steps:
echo 1. Check Windows Volume Mixer - ensure Mudlet is not muted
echo 2. Verify your default audio device in Windows Sound settings
echo 3. Try running: mudlet.exe --multimedia-backend ffmpeg
echo 4. Check Windows Event Viewer for detailed error messages
echo 5. Ensure Windows Audio service is running
echo 6. Try alternative backends: mudlet.exe --multimedia-backend windows
echo 7. Test with: mudlet.exe --debug-audio --debug-plugins
echo.
echo === FFmpeg Backend Specific Checks ===
echo Verifying FFmpeg backend is properly configured...
if exist "plugins\multimedia\ffmpegmediaplugin.dll" (
    if exist "avcodec-61.dll" (
        if exist "avformat-61.dll" (
            if exist "avutil-59.dll" (
                if exist "avdevice-61.dll" (
                    if exist "swresample-5.dll" (
                        echo [OK] FFmpeg backend should be functional
                    ) else (
                        echo [ERROR] Missing swresample-5.dll - FFmpeg audio resampling will fail
                    )
                ) else (
                    echo [ERROR] Missing avdevice-61.dll - FFmpeg device access will fail
                )
            ) else (
                echo [ERROR] Missing avutil-59.dll - FFmpeg backend will fail
            )
        ) else (
            echo [ERROR] Missing avformat-61.dll - FFmpeg backend will fail
        )
    ) else (
        echo [ERROR] Missing avcodec-61.dll - FFmpeg backend will fail
    )
) else (
    echo [ERROR] Missing ffmpegmediaplugin.dll - FFmpeg backend unavailable
)
echo.
echo === OGG/Opus Specific Checks ===
if exist "libogg-0.dll" (
    if exist "libvorbis-0.dll" (
        if exist "libopus-0.dll" (
            echo [OK] OGG/Opus support libraries present
        ) else (
            echo [ERROR] Missing libopus-0.dll - Opus audio will not work
        )
    ) else (
        echo [ERROR] Missing libvorbis-0.dll - OGG Vorbis will not work
    )
) else (
    echo [ERROR] Missing libogg-0.dll - OGG container support missing
)
echo.
echo === Qt Audio Backend Debug Info ===
echo Displaying Qt audio backend information on startup...
echo Look for lines containing:
echo   - "Available audio backends"
echo   - "Default audio backend"
echo   - "FFmpeg" initialization messages
echo   - Plugin loading success/failure messages
echo.
echo === Environment Variables Summary ===
echo Current multimedia environment:
echo   QT_MEDIA_BACKEND=%QT_MEDIA_BACKEND%
echo   QT_MEDIA_PLUGINS=%QT_MEDIA_PLUGINS%
echo   QT_DEBUG_PLUGINS=%QT_DEBUG_PLUGINS%
echo   QT_PLUGIN_PATH=%QT_PLUGIN_PATH%
echo   QT_LOGGING_RULES=%QT_LOGGING_RULES%
echo.
echo Verifying FFmpeg backend is properly configured...
if exist "plugins\multimedia\ffmpegmediaplugin.dll" (
    if exist "avcodec-61.dll" (
        if exist "avformat-61.dll" (
            if exist "avutil-59.dll" (
                echo [OK] FFmpeg backend should be functional
            ) else (
                echo [ERROR] Missing avutil-59.dll - FFmpeg backend will fail
            )
        ) else (
            echo [ERROR] Missing avformat-61.dll - FFmpeg backend will fail
        )
    ) else (
        echo [ERROR] Missing avcodec-61.dll - FFmpeg backend will fail
    )
) else (
    echo [ERROR] Missing ffmpegmediaplugin.dll - FFmpeg backend unavailable
)
echo.
echo === OGG/Opus Specific Checks ===
if exist "libogg-0.dll" (
    if exist "libvorbis-0.dll" (
        if exist "libopus-0.dll" (
            echo [OK] OGG/Opus support libraries present
        ) else (
            echo [ERROR] Missing libopus-0.dll - Opus audio will not work
        )
    ) else (
        echo [ERROR] Missing libvorbis-0.dll - OGG Vorbis will not work
    )
) else (
    echo [ERROR] Missing libogg-0.dll - OGG container support missing
)
echo.
mudlet.exe
echo.
echo Mudlet has exited. Press any key to close this window.
pause
EOF

cat > ffmpeg-test.bat << 'EOF'
@echo off
echo FFmpeg Plugin Dependency Test
echo =============================
echo.
echo Testing ffmpegmediaplugin.dll dependencies...
echo.

REM Check if dependency tools are available (would need to be bundled or downloaded)
if exist "ntldd.exe" (
    echo Running dependency analysis...
    ntldd.exe plugins\multimedia\ffmpegmediaplugin.dll
) else (
    echo ntldd.exe not available - manual dependency check:
    echo.
    echo Required FFmpeg libraries:
    if exist "avcodec-61.dll" (echo [OK] avcodec-61.dll) else (echo [MISSING] avcodec-61.dll)
    if exist "avformat-61.dll" (echo [OK] avformat-61.dll) else (echo [MISSING] avformat-61.dll)
    if exist "avutil-59.dll" (echo [OK] avutil-59.dll) else (echo [MISSING] avutil-59.dll)
    if exist "swresample-5.dll" (echo [OK] swresample-5.dll) else (echo [MISSING] swresample-5.dll)
    if exist "swscale-8.dll" (echo [OK] swscale-8.dll) else (echo [MISSING] swscale-8.dll)
    echo.
    echo Required codec libraries:
    if exist "libogg-0.dll" (echo [OK] libogg-0.dll) else (echo [MISSING] libogg-0.dll)
    if exist "libvorbis-0.dll" (echo [OK] libvorbis-0.dll) else (echo [MISSING] libvorbis-0.dll)
    if exist "libopus-0.dll" (echo [OK] libopus-0.dll) else (echo [MISSING] libopus-0.dll)
    if exist "libmp3lame-0.dll" (echo [OK] libmp3lame-0.dll) else (echo [MISSING] libmp3lame-0.dll)
    if exist "libvorbisfile-3.dll" (echo [OK] libvorbisfile-3.dll) else (echo [MISSING] libvorbisfile-3.dll)
    if exist "libmpg123-0.dll" (echo [OK] libmpg123-0.dll) else (echo [MISSING] libmpg123-0.dll)
    echo.
    echo Audio system libraries:
    if exist "libsndfile-1.dll" (echo [OK] libsndfile-1.dll) else (echo [MISSING] libsndfile-1.dll)
    if exist "libFLAC-12.dll" (echo [OK] libFLAC-12.dll) else (echo [MISSING] libFLAC-12.dll)
    if exist "libopenal-1.dll" (echo [OK] libopenal-1.dll) else (echo [MISSING] libopenal-1.dll)
    if exist "libportaudio-2.dll" (echo [OK] libportaudio-2.dll) else (echo [MISSING] libportaudio-2.dll)
)
echo.
echo === Attempting FFmpeg backend test ===
echo Setting FFmpeg as default backend...
set QT_MEDIA_BACKEND=ffmpeg
set QT_MEDIA_PLUGINS=1
set QT_DEBUG_PLUGINS=1
set QT_LOGGING_RULES=qt.multimedia*=true
echo.
echo Testing with environment:
echo QT_MEDIA_BACKEND=%QT_MEDIA_BACKEND%
echo QT_MEDIA_PLUGINS=%QT_MEDIA_PLUGINS%
echo QT_DEBUG_PLUGINS=%QT_DEBUG_PLUGINS%
echo.
echo Starting Mudlet with FFmpeg backend for testing...
echo Look for "FFmpeg" mentions in the console output.
echo Also look for "loaded" or "failed to load" messages about multimedia plugins.
echo.
mudlet.exe
echo.
echo Test complete. Press any key to continue.
pause
EOF

echo "Created mudlet-debug.bat and ffmpeg-test.bat for comprehensive diagnostics"

cat > audio-test.bat << 'EOF'
@echo off
echo Audio System Diagnostics
echo ========================
echo.
echo Checking audio-related DLLs...
echo.
echo === Core Audio Libraries ===
if exist "libopenal-1.dll" (echo [OK] libopenal-1.dll) else (echo [MISSING] libopenal-1.dll)
if exist "libsndfile-1.dll" (echo [OK] libsndfile-1.dll) else (echo [MISSING] libsndfile-1.dll)
if exist "libFLAC-12.dll" (echo [OK] libFLAC-12.dll) else (echo [MISSING] libFLAC-12.dll)
if exist "libportaudio-2.dll" (echo [OK] libportaudio-2.dll) else (echo [MISSING] libportaudio-2.dll)
echo.
echo === Multimedia Plugins ===
if exist "plugins\multimedia\ffmpegmediaplugin.dll" (echo [OK] FFmpeg plugin) else (echo [MISSING] FFmpeg plugin)
if exist "plugins\multimedia\dsengine.dll" (echo [OK] DirectShow plugin) else (echo [MISSING] DirectShow plugin) 
if exist "plugins\multimedia\windowsmediaplugin.dll" (echo [OK] Windows Media plugin) else (echo [MISSING] Windows Media plugin)
echo.
echo === Windows Audio Services ===
echo Checking Windows Audio service status...
sc query AudioSrv | findstr "STATE"
echo.
echo === Audio Device Information ===
echo Current audio devices:
wmic sounddev get name,status
echo.
echo Testing basic audio functionality...
echo This will attempt to play a system beep - listen for sound:
echo.
rundll32 user32.dll,MessageBeep
echo.
echo Did you hear a beep? If not, check:
echo 1. Volume levels in Windows
echo 2. Default audio device settings
echo 3. Audio drivers
echo 4. Hardware connections
echo.
pause
EOF

echo "Created audio-test.bat for comprehensive audio diagnostics"

cat > ogg-opus-test.bat << 'EOF'
@echo off
echo OGG/Opus Audio Support Test
echo ===========================
echo.
echo This script specifically tests OGG and Opus audio format support
echo which is required for this Mudlet pull request.
echo.
echo === Checking OGG/Opus Dependencies ===
echo Core container support:
if exist "libogg-0.dll" (
    echo [OK] libogg-0.dll - OGG container support
) else (
    echo [CRITICAL] libogg-0.dll MISSING - OGG files cannot be read!
)

echo.
echo Codec support:
if exist "libvorbis-0.dll" (
    echo [OK] libvorbis-0.dll - OGG Vorbis codec
) else (
    echo [ERROR] libvorbis-0.dll MISSING - OGG Vorbis files will not play!
)

if exist "libopus-0.dll" (
    echo [OK] libopus-0.dll - Opus codec
) else (
    echo [ERROR] libopus-0.dll MISSING - Opus files will not play!
)

if exist "libvorbisfile-3.dll" (
    echo [OK] libvorbisfile-3.dll - Advanced OGG Vorbis support
) else (
    echo [WARNING] libvorbisfile-3.dll missing - May affect OGG playback quality
)

echo.
echo === FFmpeg Backend Check ===
if exist "plugins\multimedia\ffmpegmediaplugin.dll" (
    echo [OK] FFmpeg plugin available
    
    if exist "avcodec-61.dll" (
        if exist "avformat-61.dll" (
            if exist "avutil-59.dll" (
                echo [OK] FFmpeg core libraries present
            ) else (
                echo [CRITICAL] avutil-59.dll missing - FFmpeg will not work!
            )
        ) else (
            echo [CRITICAL] avformat-61.dll missing - FFmpeg will not work!
        )
    ) else (
        echo [CRITICAL] avcodec-61.dll missing - FFmpeg will not work!
    )
) else (
    echo [CRITICAL] FFmpeg plugin missing - OGG/Opus WILL NOT WORK!
    echo.
    echo The Windows Media backend does not support OGG or Opus formats.
    echo FFmpeg backend is REQUIRED for this functionality.
)

echo.
echo === Testing OGG/Opus with FFmpeg Backend ===
echo Setting optimal environment for OGG/Opus support...
set QT_MEDIA_BACKEND=ffmpeg
set QT_MEDIA_PLUGINS=1
set QT_DEBUG_PLUGINS=1
set QT_LOGGING_RULES=qt.multimedia*=true;*.ffmpeg*=true
set QT_PLUGIN_PATH=%~dp0plugins

echo Environment configured:
echo   QT_MEDIA_BACKEND=%QT_MEDIA_BACKEND%
echo   QT_PLUGIN_PATH=%QT_PLUGIN_PATH%
echo.
echo Starting Mudlet for OGG/Opus testing...
echo.
echo IMPORTANT: When Mudlet starts, try to play an OGG or Opus file.
echo Watch the console output for:
echo   - "FFmpeg" initialization messages
echo   - Codec loading messages
echo   - Any "format not supported" errors
echo.
echo If you see errors about OGG/Opus support, this indicates
echo the FFmpeg backend is not properly configured.
echo.
mudlet.exe
echo.
echo === Post-Test Analysis ===
echo Did OGG/Opus files play correctly? [Y/N]
set /p AUDIO_WORKED="Enter Y if audio played, N if not: "
if /i "%AUDIO_WORKED%"=="Y" (
    echo SUCCESS: OGG/Opus support is working!
) else (
    echo FAILURE: OGG/Opus support is not working.
    echo.
    echo Troubleshooting steps:
    echo 1. Ensure all DLLs above show [OK]
    echo 2. Check that ffmpegmediaplugin.dll exists
    echo 3. Verify FFmpeg core libraries are present
    echo 4. Check Windows Event Viewer for detailed errors
    echo 5. Try running mudlet-debug.bat for more diagnostics
)
echo.
pause
EOF

echo "Created ogg-opus-test.bat for OGG/Opus specific testing"

cat > qt-backend-test.bat << 'EOF'
@echo off
echo Qt Multimedia Backend Diagnostic Tool
echo =====================================
echo.
echo This tool will help diagnose why FFmpeg backend may not be loading.
echo.
echo === Step 1: Verify All Required FFmpeg Libraries ===
echo Checking critical FFmpeg libraries:
set FFMPEG_OK=true

if exist "avcodec-61.dll" (
    echo [OK] avcodec-61.dll
) else (
    echo [CRITICAL] avcodec-61.dll MISSING
    set FFMPEG_OK=false
)

if exist "avformat-61.dll" (
    echo [OK] avformat-61.dll  
) else (
    echo [CRITICAL] avformat-61.dll MISSING
    set FFMPEG_OK=false
)

if exist "avutil-59.dll" (
    echo [OK] avutil-59.dll
) else (
    echo [CRITICAL] avutil-59.dll MISSING
    set FFMPEG_OK=false
)

if exist "avdevice-61.dll" (
    echo [OK] avdevice-61.dll
) else (
    echo [CRITICAL] avdevice-61.dll MISSING  
    set FFMPEG_OK=false
)

if exist "swresample-5.dll" (
    echo [OK] swresample-5.dll
) else (
    echo [CRITICAL] swresample-5.dll MISSING
    set FFMPEG_OK=false
)

if exist "swscale-8.dll" (
    echo [OK] swscale-8.dll
) else (
    echo [WARNING] swscale-8.dll missing - may affect video scaling
)

echo.
echo === Step 2: Verify FFmpeg Plugin Exists ===
if exist "plugins\multimedia\ffmpegmediaplugin.dll" (
    echo [OK] ffmpegmediaplugin.dll found
) else (
    echo [CRITICAL] ffmpegmediaplugin.dll MISSING - FFmpeg backend impossible!
    set FFMPEG_OK=false
)

echo.
echo === Step 3: Check Plugin Dependencies ===
echo Verifying that ffmpegmediaplugin.dll can load its dependencies...
if exist "plugins\multimedia\ffmpegmediaplugin.dll" (
    echo Testing ffmpegmediaplugin.dll dependencies...
    REM We'll use a simple method to test if the DLL can be loaded
    echo This may show error dialogs if dependencies are missing - that's expected for diagnosis.
    echo.
    echo Attempting to test-load ffmpegmediaplugin.dll...
    REM rundll32 would try to load the DLL and show errors if dependencies missing
    rundll32.exe "plugins\multimedia\ffmpegmediaplugin.dll",DllCanUnloadNow 2>nul
    if %ERRORLEVEL% EQU 0 (
        echo [OK] Plugin DLL can be loaded successfully
    ) else (
        echo [ERROR] Plugin DLL failed to load - likely missing dependencies
        echo This usually means FFmpeg core libraries are missing or wrong version
    )
) else (
    echo Cannot test plugin - ffmpegmediaplugin.dll not found
)

echo.
echo === Step 4: Environment Configuration Test ===
echo Testing different backend configurations...
echo.
echo --- Test 1: Default Backend ---
echo Starting Mudlet with default multimedia backend...
echo Look for plugin loading messages in the console output.
echo.
set QT_DEBUG_PLUGINS=1
set QT_LOGGING_RULES=qt.multimedia*=true;qt.core.plugin*=true
mudlet.exe --help >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [OK] Mudlet executable responds to --help
) else (
    echo [ERROR] Mudlet executable may be corrupted or missing dependencies
)

echo.
echo --- Test 2: Force FFmpeg Backend ---
echo Forcing FFmpeg backend specifically...
set QT_MEDIA_BACKEND=ffmpeg
set QT_MEDIA_PLUGINS=1  
set QT_DEBUG_PLUGINS=1
set QT_LOGGING_RULES=qt.multimedia*=true;*.ffmpeg*=true;qt.core.plugin*=true

echo Environment:
echo   QT_MEDIA_BACKEND=%QT_MEDIA_BACKEND%
echo   QT_MEDIA_PLUGINS=%QT_MEDIA_PLUGINS%
echo   QT_DEBUG_PLUGINS=%QT_DEBUG_PLUGINS%
echo.
echo When Mudlet starts, watch for these messages:
echo   - "QFactoryLoader::QFactoryLoader() checking directory path"
echo   - "loaded library" or "Cannot load library"  
echo   - "FFmpeg" initialization messages
echo   - Backend selection messages
echo.
echo Press Enter to start Mudlet with FFmpeg backend forced...
pause >nul
mudlet.exe
echo.

echo === Step 5: Analysis and Recommendations ===
if "%FFMPEG_OK%"=="false" (
    echo.
    echo DIAGNOSIS: FFmpeg backend cannot load due to missing core libraries.
    echo.
    echo REQUIRED ACTIONS:
    echo 1. Ensure ALL of these FFmpeg DLLs are in the main directory:
    echo    - avcodec-61.dll
    echo    - avformat-61.dll  
    echo    - avutil-59.dll
    echo    - avdevice-61.dll
    echo    - swresample-5.dll
    echo.
    echo 2. Verify ffmpegmediaplugin.dll is in plugins\multimedia\
    echo.
    echo 3. These files must be from compatible FFmpeg build (usually same version^)
    echo.
    echo 4. Run this test again after adding missing libraries
) else (
    echo.
    echo DIAGNOSIS: All required FFmpeg libraries appear to be present.
    echo.
    echo If FFmpeg backend still not working:
    echo 1. Check Windows Event Viewer for detailed DLL loading errors
    echo 2. Verify FFmpeg libraries are from compatible build
    echo 3. Try setting QT_MEDIA_BACKEND=ffmpeg in system environment  
    echo 4. Check if antivirus is blocking DLL loading
    echo 5. Ensure Windows Audio service is running
    echo.
    echo Alternative backends to try:
    echo - Windows Media: QT_MEDIA_BACKEND=windows ^(no OGG/Opus support^)
    echo - DirectShow: Available through dsengine.dll plugin
)

echo.
echo === Interactive Backend Test ===
echo.
set /p TEST_AGAIN="Do you want to test a specific backend? [F=FFmpeg, W=Windows, N=No]: "
if /i "%TEST_AGAIN%"=="F" (
    echo Testing FFmpeg backend...
    set QT_MEDIA_BACKEND=ffmpeg
    mudlet.exe
) else if /i "%TEST_AGAIN%"=="W" (
    echo Testing Windows Media backend...
    set QT_MEDIA_BACKEND=windows  
    mudlet.exe
) else (
    echo Test complete.
)

echo.
pause
EOF

echo "Created qt-backend-test.bat for comprehensive FFmpeg backend diagnostics"

cat > qt-backend-test.bat << 'EOF'
@echo off
echo Qt Multimedia Backend Diagnostic
echo =================================
echo.
echo This script tests Qt multimedia backend configuration specifically
echo for identifying issues with audio playback in Mudlet.
echo.
echo === Qt Multimedia Environment Test ===
echo Testing different backend configurations...
echo.

echo [TEST 1] Testing with FFmpeg backend (required for OGG/Opus)
set QT_MEDIA_BACKEND=ffmpeg
set QT_MEDIA_PLUGINS=1
set QT_DEBUG_PLUGINS=1
set QT_LOGGING_RULES=qt.multimedia*=true;*.ffmpeg*=true;qt.core.plugin*=true
set QT_PLUGIN_PATH=%~dp0plugins

echo Environment:
echo   Backend: %QT_MEDIA_BACKEND%
echo   Plugin path: %QT_PLUGIN_PATH%
echo   Debug enabled: %QT_DEBUG_PLUGINS%
echo.
echo Starting Mudlet with FFmpeg backend...
echo WATCH FOR: FFmpeg plugin loading messages in console output
echo If you see "Cannot load library" errors for FFmpeg, that's the problem.
echo.
start /wait mudlet.exe
echo.
echo Did FFmpeg backend work? [Y/N]
set /p FFMPEG_WORKED="Enter Y if you saw FFmpeg loading messages, N if not: "

echo.
echo [TEST 2] Testing with Windows Media backend (fallback)
set QT_MEDIA_BACKEND=windows
echo.
echo Environment:
echo   Backend: %QT_MEDIA_BACKEND%
echo.
echo Starting Mudlet with Windows Media backend...
echo NOTE: This backend cannot play OGG/Opus files, but should work for other audio.
echo.
start /wait mudlet.exe
echo.
echo Did Windows Media backend work for non-OGG audio? [Y/N]
set /p WINDOWS_WORKED="Enter Y if other audio worked, N if not: "

echo.
echo === DIAGNOSTIC RESULTS ===
if /i "%FFMPEG_WORKED%"=="Y" (
    echo SUCCESS: FFmpeg backend is working - OGG/Opus should be supported
) else (
    echo PROBLEM: FFmpeg backend is not working
    echo.
    echo Common causes:
    echo 1. Missing FFmpeg DLLs (avcodec-61.dll, avformat-61.dll, avutil-59.dll)
    echo 2. Missing ffmpegmediaplugin.dll in plugins/multimedia/
    echo 3. Incompatible FFmpeg library versions
    echo 4. Missing codec libraries (libogg-0.dll, libvorbis-0.dll, libopus-0.dll)
)

echo.
if /i "%WINDOWS_WORKED%"=="Y" (
    echo INFO: Windows Media backend works - basic audio system is functional
) else (
    echo PROBLEM: Windows Media backend also failed - system audio issue
    echo.
    echo This suggests a deeper audio system problem:
    echo 1. Check Windows audio drivers
    echo 2. Verify default audio device is working
    echo 3. Check Windows Audio service is running
    echo 4. Test audio with other applications
)

echo.
echo === NEXT STEPS ===
if /i "%FFMPEG_WORKED%"=="N" (
    echo To fix FFmpeg backend:
    echo 1. Run mudlet-debug.bat to check for missing DLLs
    echo 2. Verify all FFmpeg and codec DLLs are present
    echo 3. Check Windows Event Viewer for detailed error messages
    echo 4. Try re-running the packaging script to ensure all dependencies
)

echo.
pause
EOF

echo "Created qt-backend-test.bat for Qt multimedia backend diagnostics"

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
ls -la ./*ffmpeg* ./*avcodec* ./*avformat* ./*avutil* ./*swresample* ./*swscale* ./*avfilter* ./*postproc* 2>/dev/null || echo "No FFmpeg libraries found"

echo ""
echo "Codec libraries present:"
ls -la ./*opus* ./*ogg* ./*vorbis* ./*mp3lame* ./*aom* ./*dav1d* 2>/dev/null || echo "No codec libraries found"

echo ""
echo "Critical runtime dependencies check:"
CRITICAL_RUNTIME_CHECK=("lua51.dll" "libgcc_s_seh-1.dll" "libpcre-1.dll" "libpugixml.dll" "libstdc++-6.dll" "zlib1.dll" "libicuuc77.dll" "libdouble-conversion.dll")
ALL_CRITICAL_PRESENT=true
for CRITICAL in "${CRITICAL_RUNTIME_CHECK[@]}"; do
    if [ -f "$CRITICAL" ]; then
        echo "  ✓ $CRITICAL found"
    else
        echo "  ✗ $CRITICAL missing (CRITICAL - Mudlet will not start!)"
        ALL_CRITICAL_PRESENT=false
    fi
done

if [ "$ALL_CRITICAL_PRESENT" = true ]; then
    echo "  ✓ All critical runtime dependencies present - Mudlet should start"
else
    echo "  ✗ Missing critical dependencies - Mudlet will fail to start!"
fi

echo ""
echo "Critical multimedia files check:"
if [ -f "plugins/multimedia/ffmpegmediaplugin.dll" ]; then
    echo "  ✓ ffmpegmediaplugin.dll found - OGG/Opus support should work"
    
    # Final dependency check
    echo ""
    echo "=== FINAL DEPENDENCY CHECK ==="
    echo "Running ntldd on ffmpegmediaplugin.dll to verify all dependencies are satisfied:"
    if command -v "${MINGW_INTERNAL_BASE_DIR}/bin/ntldd" >/dev/null 2>&1; then
        FINAL_MISSING=$("${MINGW_INTERNAL_BASE_DIR}/bin/ntldd" "plugins/multimedia/ffmpegmediaplugin.dll" 2>/dev/null | grep "not found" | wc -l)
        if [ "$FINAL_MISSING" -eq 0 ]; then
            echo "  ✓ All dependencies satisfied for ffmpegmediaplugin.dll"
        else
            echo "  ✗ $FINAL_MISSING dependencies still missing:"
            "${MINGW_INTERNAL_BASE_DIR}/bin/ntldd" "plugins/multimedia/ffmpegmediaplugin.dll" 2>/dev/null | grep "not found"
        fi
    else
        echo "  ? ntldd not available for final verification"
    fi
else
    echo "  ✗ ffmpegmediaplugin.dll MISSING - OGG/Opus support will NOT work"
fi

echo ""
echo "Running final dependency check on mudlet.exe:"
if command -v "${MINGW_INTERNAL_BASE_DIR}/bin/ntldd" >/dev/null 2>&1; then
    MUDLET_MISSING=$("${MINGW_INTERNAL_BASE_DIR}/bin/ntldd" "mudlet.exe" 2>/dev/null | grep "not found" | wc -l)
    if [ "$MUDLET_MISSING" -eq 0 ]; then
        echo "  ✓ All dependencies satisfied for mudlet.exe"
    else
        echo "  ✗ $MUDLET_MISSING dependencies still missing for mudlet.exe:"
        "${MINGW_INTERNAL_BASE_DIR}/bin/ntldd" "mudlet.exe" 2>/dev/null | grep "not found"
    fi
else
    echo "  ? ntldd not available for final verification"
fi

# Check for specific version-numbered libraries
if [ -f "avcodec-61.dll" ] && [ -f "avformat-61.dll" ] && [ -f "avutil-59.dll" ] && [ -f "avdevice-61.dll" ] && [ -f "swresample-5.dll" ]; then
    echo "  ✓ Core FFmpeg libraries found (matching Qt6 requirements)"
else
    echo "  ✗ Some core FFmpeg libraries missing or wrong version"
    echo "    Expected: avcodec-61.dll, avformat-61.dll, avutil-59.dll, avdevice-61.dll, swresample-5.dll"
    echo "    Found:"
    ls -la ./avcodec-*.dll ./avformat-*.dll ./avutil-*.dll ./avdevice-*.dll ./swresample-*.dll 2>/dev/null || echo "    None found"
fi

# Check for critical codec libraries
CRITICAL_CODECS=("libogg-0.dll" "libvorbis-0.dll" "libopus-0.dll")
for CODEC in "${CRITICAL_CODECS[@]}"; do
    if [ -f "$CODEC" ]; then
        echo "  ✓ $CODEC found"
    else
        echo "  ✗ $CODEC missing (needed for OGG/Opus support)"
    fi
done

echo ""
echo "Package verification complete. Troubleshooting guides:"
echo ""
echo "If Mudlet fails to start with missing DLL errors:"
echo "1. Run mudlet-debug.bat to check critical runtime dependencies"
echo "2. Verify these critical DLLs are present:"
echo "   - lua51.dll, libgcc_s_seh-1.dll, libpcre-1.dll, libpugixml.dll"
echo "   - zlib1.dll, libicuuc77.dll, libdouble-conversion.dll"
echo "   - libstdc++-6.dll, libfreetype-6.dll"
echo "3. Check Windows Event Viewer for detailed DLL loading errors"
echo ""
echo "For OGG/Opus audio troubleshooting:"
echo "1. Run qt-backend-test.bat to diagnose FFmpeg backend loading issues"  
echo "2. Run mudlet-debug.bat to see plugin loading details"  
echo "3. Check that ffmpegmediaplugin.dll is in plugins/multimedia/"
echo "4. Verify all FFmpeg libraries are in the main directory"
echo "5. Set QT_MEDIA_BACKEND=ffmpeg if needed"
echo "6. Run ffmpeg-test.bat for standalone dependency testing"
echo "7. Run ogg-opus-test.bat for OGG/Opus specific diagnostics"
echo "8. Run audio-test.bat for comprehensive audio system diagnostics"
echo "8. Run qt-backend-test.bat for Qt multimedia backend testing"
echo ""
echo "CRITICAL for OGG/Opus support:"
echo "- FFmpeg backend MUST be working (Windows Media backend cannot play OGG/Opus)"
echo "- Required: libogg-0.dll, libvorbis-0.dll, libopus-0.dll"
echo "- Required: avcodec-61.dll, avformat-61.dll, avutil-59.dll"
echo "- Required: ffmpegmediaplugin.dll in plugins/multimedia/"
echo "- Required: libvorbisenc-2.dll, libtheoradec-2.dll for full OGG support"

FINAL_DIR=$(/usr/bin/cygpath --windows "${PACKAGE_DIR}")
echo ""
echo "${FINAL_DIR} should contain everything needed to run Mudlet!"
echo ""
echo "   ... package-mudlet-for-windows.sh shell script finished."
echo ""
cd ~ || exit 1

exit 0
