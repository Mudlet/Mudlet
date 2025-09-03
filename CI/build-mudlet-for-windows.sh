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
#          1.4.0    Rewrite Makefile to use ccache.exe if available
#          1.3.0    No changes
#          1.2.0    No changes
#          1.1.0    No changes
#          1.0.0    Original version

# Script to build the Mudlet code currently checked out in
# ${GITHUB_WORKSPACE} in a MINGW64 shell

# To be used AFTER setup-windows-sdk.sh has been run; once this has completed
# successfully, package-mudlet-for-windows.sh is run by the workflow

# Exit codes:
# 0 - Everything is fine. 8-)
# 1 - Failure to change to a directory
# 2 - Unsupported MSYS2/MINGGW shell type
# 3 - Unsupported build type

if [ "${MSYSTEM}" = "MSYS" ]; then
  echo "Please run this script from a MINGW64 type bash terminal as the MSYS one"
  echo "does not supported what is needed."
  exit 2
elif [ "${MSYSTEM}" = "MINGW64" ]; then
  export BUILD_BITNESS="64"
  export BUILDCOMPONENT="x86_64"
else
  echo "This script is not set up to handle systems of type ${MSYSTEM}, only"
  echo "MINGW64 is currently supported. Please rerun this in a bash terminal of"
  echo "that type."
  exit 2
fi

# Check if GITHUB_REPO_TAG is "false"
if [[ "${GITHUB_REPO_TAG}" == "false" ]]; then
  echo "=== GITHUB_REPO_TAG is FALSE ==="

  # Check if this is a scheduled build
  if [[ "${GITHUB_SCHEDULED_BUILD}" == "true" ]]; then
    echo "=== GITHUB_SCHEDULED_BUILD is TRUE, this is a PTB ==="
    MUDLET_VERSION_BUILD="-ptb"
  else
    MUDLET_VERSION_BUILD="-testing"
  fi

  # Check if this is a pull request
  if [[ -n "${GITHUB_PULL_REQUEST_NUMBER}" ]]; then
    # Use the specific commit SHA from the pull request head, since GitHub Actions merges the PR
    BUILD_COMMIT=$(git rev-parse --short "${GITHUB_PULL_REQUEST_HEAD_SHA}")
    MUDLET_VERSION_BUILD="${MUDLET_VERSION_BUILD}-PR${GITHUB_PULL_REQUEST_NUMBER}"
  else
    BUILD_COMMIT=$(git rev-parse --short HEAD)

    if [[ "${MUDLET_VERSION_BUILD}" == "-ptb" ]]; then
      # Get current date in YYYY-MM-DD format
      DATE=$(date +%F)
      MUDLET_VERSION_BUILD="${MUDLET_VERSION_BUILD}-${DATE}"
    fi
  fi
fi

# Convert to lowercase, not all systems deal with uppercase ASCII characters
export MUDLET_VERSION_BUILD="${MUDLET_VERSION_BUILD,,}"
export BUILD_COMMIT="${BUILD_COMMIT,,}"

MINGW_BASE_DIR="${GHCUP_MSYS2}\mingw32"
export MINGW_BASE_DIR
MINGW_INTERNAL_BASE_DIR="/mingw${BUILD_BITNESS}"
export MINGW_INTERNAL_BASE_DIR
PATH="${MINGW_INTERNAL_BASE_DIR}/usr/local/bin:${MINGW_INTERNAL_BASE_DIR}/bin:/usr/bin:${PATH}"
export PATH
RUNNER_WORKSPACE_UNIX_PATH=$(echo "${RUNNER_WORKSPACE}" | sed 's|\\|/|g' | sed 's|D:|/d|g')
export CCACHE_DIR=${RUNNER_WORKSPACE_UNIX_PATH}/ccache

echo "MSYSTEM is: ${MSYSTEM}"
echo "CCACHE_DIR is: ${CCACHE_DIR}"
echo "PATH is now:"
echo "${PATH}"
echo ""

cd "${GITHUB_WORKSPACE}" || exit 1
mkdir -p "build-${MSYSTEM}"

cd "${GITHUB_WORKSPACE}"/build-"${MSYSTEM}" || exit 1

#### Qt Creator note ####
# If one is planning to use qtcreator these will probably be wanted in a
# shell startup script so as to prepare it to use Lua 5.1 when running
# qmake (needed to process translation files to get the translations
# statistics):
LUA_PATH=$(cygpath -u "$(luarocks --lua-version 5.1 path --lr-path)" )
export LUA_PATH
LUA_CPATH=$(cygpath -u "$(luarocks --lua-version 5.1 path --lr-cpath)" )
export LUA_CPATH

echo ""
echo "Adjusting LUA paths for Lua 5.1:"
echo "LUA_PATH is: ${LUA_PATH}"
echo "LUA_CPATH is: ${LUA_CPATH}"
echo ""

# Configure GStreamer for Qt6 Multimedia to support OGG and other formats
export GST_PLUGIN_PATH="${MINGW_INTERNAL_BASE_DIR}/lib/gstreamer-1.0"
export GST_PLUGIN_SYSTEM_PATH="${MINGW_INTERNAL_BASE_DIR}/lib/gstreamer-1.0"
export QT_MULTIMEDIA_PREFERRED_PLUGINS="gstreamer"

echo "Setting up GStreamer for multimedia support:"
echo "GST_PLUGIN_PATH is: ${GST_PLUGIN_PATH}"
echo "GST_PLUGIN_SYSTEM_PATH is: ${GST_PLUGIN_SYSTEM_PATH}"
echo "QT_MULTIMEDIA_PREFERRED_PLUGINS is: ${QT_MULTIMEDIA_PREFERRED_PLUGINS}"
echo ""

if [[ "${MUDLET_VERSION_BUILD,,}" == *"-testing"* ]]; then
    # The updater is not helpful in this environment (PR testing build)
    export WITH_UPDATER="NO"
else
    # Tagged build, this is a release or a PTB build, include the updater
    export WITH_UPDATER="YES"
fi

echo "Running qmake to make MAKEFILE ..."
echo ""

if [ "${MSYSTEM}" = "MINGW64" ]; then
    qmake6 ../src/mudlet.pro -spec win32-g++ "CONFIG-=qml_debug" "CONFIG-=qtquickcompiler"
else
    qmake ../src/mudlet.pro -spec win32-g++ "CONFIG-=qml_debug" "CONFIG-=qtquickcompiler"
fi

echo " ... qmake done."
echo ""

export WITH_CCACHE="YES"

if [ "${WITH_CCACHE}" = "YES" ]; then
  echo "  Tweaking Makefile.Release to use ccache..."
  sed -i "s/CC            = gcc/CC            = ccache gcc/" ./Makefile.Release
  sed -i "s/CXX           = g++/CXX           = ccache g++/" ./Makefile.Release
  echo ""
fi

echo "Running make to build project ..."
echo ""

# Despite the mingw32 prefix mingw32-make.exe IS the make we want.
if [ -n "${NUMBER_OF_PROCESSORS}" ] && [ "${NUMBER_OF_PROCESSORS}" -gt 1 ]; then
  mingw32-make -j "${NUMBER_OF_PROCESSORS}"
else
  mingw32-make
fi

echo " ... make finished"
echo ""

echo "Copying GStreamer plugins to build directory for runtime availability..."
GSTREAMER_DEST_DIR="./${BUILD_CONFIG}/gstreamer-1.0"
mkdir -p "${GSTREAMER_DEST_DIR}"
if [ -d "${MINGW_INTERNAL_BASE_DIR}/lib/gstreamer-1.0" ]; then
  find "${MINGW_INTERNAL_BASE_DIR}/lib/gstreamer-1.0" -name "*.dll" -exec cp -v {} "${GSTREAMER_DEST_DIR}/" \;
  echo "GStreamer plugins copied successfully to ${GSTREAMER_DEST_DIR}"
  ls -la "${GSTREAMER_DEST_DIR}/"
else
  echo "Warning: GStreamer plugins directory not found at ${MINGW_INTERNAL_BASE_DIR}/lib/gstreamer-1.0"
  echo "Available directories:"
  for dir in "${MINGW_INTERNAL_BASE_DIR}/lib/"*gst*; do
    if [ -d "$dir" ]; then
      echo "  $(basename "$dir")"
    fi
  done
  if [ ! -d "${MINGW_INTERNAL_BASE_DIR}/lib/"*gst* ]; then
    echo "  No GStreamer directories found"
  fi
fi
echo ""

cd ~ || exit 1
exit 0
