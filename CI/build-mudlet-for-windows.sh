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
# ${GITHUB_WORKSPACE} in a MINGW64 shell, further work is needed for this file
# to be useable by end-users/developers.

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
elif [ "${MSYSTEM}" != "MINGW64" ]; then
  echo "This script is not set up to handle systems of type ${MSYSTEM}, only"
  echo "MINGW64 is currently supported. Please rerun this in a bash terminal of"
  echo "that type."
  exit 2
fi

# In some cases we might want to change this to "debug" but that is not
# currently handled:
if [ -z "${BUILD_CONFIG}" ]; then
  BUILD_CONFIG="release"
fi

# Work out what we are doing - and export this eventually so that later scripts
# don't have to repeat the process:
BUILD_ACTION="Unknown"

# We'll need this later on so grab it now - lifted from travis.set-build-info.sh:
VERSION=$(perl -lne 'print $1 if /^VERSION = (.+)/' < "${GITHUB_WORKSPACE}/src/mudlet.pro" | tr '[:upper:]' '[:lower:]')
export VERSION

MAKE_PORTABLE='false'
export MAKE_PORTABLE
# Check that we are running on Mudlet's own GH infrastructure:
if [[ "${GITHUB_REPO_NAME}" == "Mudlet/Mudlet" ]]; then
  # Check if GITHUB_REPO_TAG is "false"
  if [[ "${GITHUB_REPO_TAG}" == "false" ]]; then
    # This is NOT a tagged build - so is NOT a "Release" build!

    # Check if this is a scheduled build
    if [[ "${GITHUB_SCHEDULED_BUILD}" == "true" ]]; then
      # See whether we need to actually do this type of build:

      # Get the commit date of the last commit
      COMMIT_DATE=$(git show -s --format="%cs")
      # Get yesterday's date in the same format
      YESTERDAY_DATE=$(date --date="yesterday" +%f)
      if [[ "${COMMIT_DATE}" < "${YESTERDAY_DATE}" ]]; then
        echo "=== No new commits, aborting public test build generation ==="
        echo "ABORT_WORKFLOW='true'" >> ${GITHUB_ENV}
        # This only terminates this script (sucessfully), the above will be used
        # to stop the remaining steps in the workflow from happening:
        exit 0
      fi

      BUILD_COMMIT=$(git rev-parse --short HEAD)
      # Get current date in YYYY-MM-DD format
      CURRENT_DATE=$(date +%F)
      MUDLET_VERSION_BUILD="-ptb-${CURRENT_DATE}"
      BUILD_ACTION="PublicTest"
      MAKE_PORTABLE='true'

    else
      # Either a PR build or a testing (PR merged into development branch) build
      if [[ -n "${GITHUB_PULL_REQUEST_NUMBER}" ]]; then
        # Use the specific commit SHA from the pull request head, since GitHub Actions merges the PR
        BUILD_COMMIT=$(git rev-parse --short "${GITHUB_PULL_REQUEST_HEAD_SHA}")
        MUDLET_VERSION_BUILD="-pr${GITHUB_PULL_REQUEST_NUMBER}"
        BUILD_ACTION="PullRequest"
        # TODO: remove before merging the PR that adds this
        MAKE_PORTABLE='true'
      else
        BUILD_COMMIT=$(git rev-parse --short HEAD)
        BUILD_ACTION="Testing"
        MUDLET_VERSION_BUILD="-testing"
      fi
    fi
  else
    # Could be a release build - although this will not happen without manual
    # intervention by Vadi to adjust the QMake/CMake project meta-build files
    # in a way that is known to him!

    BUILD_ACTION="Release"
    MAKE_PORTABLE='true'
  fi

else
  # Not running on Mudlet's own GH infrastructure so just build archive file(s)
  BUILD_ACTION="Archive"
  BUILD_COMMIT=$(git rev-parse --short --short HEAD)
  # Unlike the Mudlet GHA builds this will merely inherit the MUDLET_VERSION_BUILD
  # variable from the environment - which will normally have the Git SHA1
  # appended by the QMake/CMake project meta-build files
fi

# Convert to lowercase, not all systems deal with uppercase ASCII characters
export MUDLET_VERSION_BUILD="$(echo "${MUDLET_VERSION_BUILD}" | tr '[:upper:]' '[:lower:]')"
export BUILD_COMMIT="$(echo "${BUILD_COMMIT}" | tr '[:upper:]' '[:lower:]')"

# Extend the path to include directories we need at the front:
PATH="${MINGW_PREFIX}/usr/local/bin:${MINGW_PREFIX}/bin:/usr/bin:${PATH}"
export PATH
export CCACHE_DIR="$(cygpath -au "${RUNNER_WORKSPACE}")/ccache"

echo "MSYSTEM is: ${MSYSTEM}"
echo "CCACHE_DIR is: ${CCACHE_DIR}"
echo "PATH is now: ${PATH}"
echo ""

# This is specific to GH Actions based builds - will need reworking for end-user
# developer usage:
cd "${GITHUB_WORKSPACE}" || exit 1
# Technically we don't need the $MSYSTEM here but it will aid porting for use
# by end-users/developers on their own systems:
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

if [ "${BUILD_ACTION}" == "PublicTest" ] || [ "${BUILD_ACTION}" == "Release" ]; then
  # Tagged build, this is a release or a PTB build, include the updater
  export WITH_UPDATER="YES"
else
  # The updater is not helpful in these other environments (PullRequest or
  # Testing or Archive builds)
  export WITH_UPDATER="NO"
fi

# Since we have this already installed as a package there is no need to build
# it from a submodule:
export WITH_OWN_QTKEYCHAIN=NO

echo "Running qmake to make MAKEFILE ..."
echo ""

# Since we now only support Qt6 we only have to use qmake6 here:
qmake6 ../src/mudlet.pro -spec win32-g++ "CONFIG-=qml_debug" "CONFIG-=qtquickcompiler"

echo " ... qmake done."
echo ""

export WITH_CCACHE="YES"

if [ "${WITH_CCACHE}" = "YES" ]; then
  if [ "${BUILD_CONFIG}" == "release" ]; then
    echo "  Tweaking Makefile.Release to use ccache..."
    sed -i "s/CC            = gcc/CC            = ccache gcc/" ./Makefile.Release
    sed -i "s/CXX           = g++/CXX           = ccache g++/" ./Makefile.Release
    echo ""
  elif [ "${BUILD_CONFIG}" == "debug" ]; then
    echo "  Tweaking Makefile.Debug to use ccache..."
    sed -i "s/CC            = gcc/CC            = ccache gcc/" ./Makefile.Debug
    sed -i "s/CXX           = g++/CXX           = ccache g++/" ./Makefile.Debug
    echo ""
  fi
fi

echo "Running make to build project ..."
echo ""

# Despite the mingw32 prefix mingw32-make.exe IS the make we want for 64-Bit builds.
if [ -n "${NUMBER_OF_PROCESSORS}" ] && [ "${NUMBER_OF_PROCESSORS}" -gt 1 ]; then
  mingw32-make -j "${NUMBER_OF_PROCESSORS}"
else
  mingw32-make
fi

echo " ... make finished"
echo ""

cd ~ || exit 1

# Append these variables to the GITHUB_ENV to make them available in
# subsequent steps
{
  echo "BUILD_ACTION=$BUILD_ACTION"
  echo "BUILD_COMMIT=$BUILD_COMMIT"
  echo "BUILD_CONFIG=$BUILD_CONFIG"
  echo "MUDLET_VERSION_BUILD=$MUDLET_VERSION_BUILD"
  echo "VERSION=$VERSION"
  echo "MAKE_PORTABLE=$MAKE_PORTABLE"
} >> "${GITHUB_ENV}"

exit 0
