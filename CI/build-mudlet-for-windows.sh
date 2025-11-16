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

# Version: 3.0.0    Switch from qmake to CMake with Release builds
#          2.1.0    Remove MINGW32 since upstream no longer supports it
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
GITHUB_WORKSPACE_UNIX_PATH=$(echo "${GITHUB_WORKSPACE}" | sed 's|\\|/|g' | sed 's|D:|/d|g' | sed 's|C:|/c|g')


echo "MSYSTEM is: ${MSYSTEM}"
echo "CCACHE_DIR is: ${CCACHE_DIR}"
echo "PATH is now:"
echo "${PATH}"
echo ""

cd "${GITHUB_WORKSPACE}" || exit 1
mkdir -p "build-${MSYSTEM}"

cd "${GITHUB_WORKSPACE}"/build-"${MSYSTEM}" || exit 1

#### Lua environment setup ####
# Set up Lua 5.1 paths for translation processing and runtime
LUA_PATH=$(cygpath -u "$(luarocks --lua-version 5.1 path --lr-path)" )
export LUA_PATH
LUA_CPATH=$(cygpath -u "$(luarocks --lua-version 5.1 path --lr-cpath)" )
export LUA_CPATH

echo ""
echo "Adjusting LUA paths for Lua 5.1:"
echo "LUA_PATH is: ${LUA_PATH}"
echo "LUA_CPATH is: ${LUA_CPATH}"
echo ""

echo "Running CMake to configure project ..."
echo ""

# Since we have this already installed as a package there is no need to build
# it from a submodule - set as environment variable for CMake
export WITH_OWN_QTKEYCHAIN=NO

QMAKE_EXTRA_ARGS=""
if [ "${WITH_SENTRY}" = "yes" ]; then
  echo "  Building with Sentry support enabled"

  # Debug current environment
  echo "  Debug: Current directory: $(pwd)"
  echo "  Debug: GITHUB_WORKSPACE: ${GITHUB_WORKSPACE}"
  echo "  Debug: GITHUB_WORKSPACE_UNIX_PATH: ${GITHUB_WORKSPACE_UNIX_PATH}"
  echo "  Debug: MSYSTEM: ${MSYSTEM}"
  echo "  Debug: BUILD_CONFIG: ${BUILD_CONFIG}"
  echo "  Debug: Contents of GITHUB_WORKSPACE:"
  ls -la "${GITHUB_WORKSPACE_UNIX_PATH}/" || echo "  Failed to list GITHUB_WORKSPACE"
  echo "  Debug: Looking for 3rdparty directory:"
  ls -la "${GITHUB_WORKSPACE_UNIX_PATH}/3rdparty/" || echo "  3rdparty directory not found"

  # Build Sentry Native SDK first
  echo "  Building Sentry Native SDK..."
  cd "${GITHUB_WORKSPACE_UNIX_PATH}" || exit 1

  # Create build directory for Sentry
  mkdir -p build-sentry
  cd build-sentry || exit 1
  echo "  Debug: Created and entered build-sentry directory: $(pwd)"

  # Create a minimal CMakeLists.txt to build Sentry
  cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.10)
project(SentryBuild)
# Set Sentry version
set(SENTRY_VERSION "0.10.0")
# Build configuration
set(CMAKE_BUILD_TYPE Release)
set(SENTRY_BACKEND "crashpad" CACHE STRING "Use crashpad backend" FORCE)
set(SENTRY_INTEGRATION_QT "ON" CACHE STRING "Enable Qt integration" FORCE)
# Include the BuildSentry.cmake from the parent project
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/sentry ${CMAKE_MODULE_PATH})
include(BuildSentry)
EOF

  echo "  Debug: Created CMakeLists.txt for Sentry build"
  echo "  Debug: Contents of CMakeLists.txt:"
  cat CMakeLists.txt

  # Build Sentry using CMake with Crashpad backend
  echo "  Debug: Running cmake configure..."
  if ! cmake . -DCMAKE_INSTALL_PREFIX="$(pwd)/install"; then
    echo "  Error: CMake configure failed"
    exit 1
  fi
  echo "  Debug: Running build with cmake..."
  if ! cmake --build . --parallel "${NUMBER_OF_PROCESSORS:-1}"; then
    echo "  Error: Sentry build failed"
    exit 1
  fi
  echo "  Debug: Running install..."
  if ! cmake --install .; then
    echo "  Error: Sentry install failed"
    exit 1
  fi

  # Set environment variables for qmake to find Sentry
  SENTRY_INSTALL_DIR="$(pwd)/install"
  export SENTRY_INSTALL_DIR

  echo "  Debug: Sentry install directory contents:"
  find "${SENTRY_INSTALL_DIR}" -type f | head -20

  QMAKE_EXTRA_ARGS="INCLUDEPATH+=${SENTRY_INSTALL_DIR}/include LIBS+=-L${SENTRY_INSTALL_DIR}/lib LIBS+=-lsentry DEFINES+=INCLUDE_SENTRY"
  echo "  Sentry Native SDK built and installed to: ${SENTRY_INSTALL_DIR}"
  echo "  Debug: QMAKE_EXTRA_ARGS: ${QMAKE_EXTRA_ARGS}"

  # Return to the build directory
  BUILD_DIR="${GITHUB_WORKSPACE_UNIX_PATH}/build-${MSYSTEM}/${BUILD_CONFIG}"
  echo "  Debug: Changing to build directory: ${BUILD_DIR}"
  cd "${BUILD_DIR}" || exit 1
  echo "  Debug: Successfully changed to: $(pwd)"
else
  echo "  Building without Sentry support"
fi

echo "Running qmake to make MAKEFILE ..."
echo ""

if [ "${MSYSTEM}" = "MINGW64" ]; then
    qmake6 ../src/mudlet.pro -spec win32-g++ "CONFIG-=qml_debug" "CONFIG-=qtquickcompiler" ${QMAKE_EXTRA_ARGS}
else
    qmake ../src/mudlet.pro -spec win32-g++ "CONFIG-=qml_debug" "CONFIG-=qtquickcompiler" ${QMAKE_EXTRA_ARGS}
# Set updater flag based on build type
if [[ "${MUDLET_VERSION_BUILD,,}" == *"-testing"* ]]; then
  # The updater is not helpful in this environment (PR testing build)
  export WITH_UPDATER=NO
else
  # Tagged build, this is a release or a PTB build, include the updater
  export WITH_UPDATER=YES
fi

# Configure CMake with ccache and Release build type
CMAKE_ARGS=(
  -G Ninja
  -DCMAKE_BUILD_TYPE=Release
  -DCMAKE_PREFIX_PATH="${MINGW_INTERNAL_BASE_DIR}"
  -DCMAKE_RUNTIME_OUTPUT_DIRECTORY="${GITHUB_WORKSPACE}/build-${MSYSTEM}/release"
)

# Enable ccache for CMake
export WITH_CCACHE="YES"
if [ "${WITH_CCACHE}" = "YES" ]; then
  echo "  Configuring CMake to use ccache..."
  CMAKE_ARGS+=(
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
    -DCMAKE_C_COMPILER_LAUNCHER=ccache
  )
fi

cmake .. "${CMAKE_ARGS[@]}"

echo " ... CMake configuration done."
echo ""

echo "Running CMake build ..."
echo ""

# Build using CMake with parallel jobs
# Note: --config is only relevant for multi-config generators (e.g., Visual Studio)
# but is harmless for single-config generators like Unix Makefiles
if [ -n "${NUMBER_OF_PROCESSORS}" ] && [ "${NUMBER_OF_PROCESSORS}" -gt 1 ]; then
  cmake --build . --parallel "${NUMBER_OF_PROCESSORS}"
else
  cmake --build .
fi

echo " ... CMake build finished"
echo ""

# Copy crashpad_handler if Sentry is enabled
if [ "${WITH_SENTRY}" = "yes" ]; then
  echo "Copying crashpad_handler.exe for Windows..."
  echo "  Debug: Looking for crashpad_handler.exe in Sentry install directory"
  find "${SENTRY_INSTALL_DIR}" -name "crashpad_handler*" -type f || echo "  No crashpad_handler files found"

  # Try different possible locations for crashpad_handler
  CRASHPAD_LOCATIONS=(
    "${SENTRY_INSTALL_DIR}/bin/crashpad_handler.exe"
    "${SENTRY_INSTALL_DIR}/lib/crashpad_handler.exe"
    "${SENTRY_INSTALL_DIR}/crashpad_handler.exe"
    "${GITHUB_WORKSPACE_UNIX_PATH}/build-sentry/_deps/sentry-build/crashpad_build/handler/crashpad_handler.exe"
  )

  CRASHPAD_FOUND=false
  for CRASHPAD_PATH in "${CRASHPAD_LOCATIONS[@]}"; do
    if [ -f "$CRASHPAD_PATH" ]; then
      cp "$CRASHPAD_PATH" .
      echo "  crashpad_handler.exe copied from: $CRASHPAD_PATH"
      CRASHPAD_FOUND=true
      break
    fi
  done

  if [ "$CRASHPAD_FOUND" = false ]; then
    echo "  Warning: crashpad_handler.exe not found in any expected locations"
    echo "  Debug: Searched locations:"
    for CRASHPAD_PATH in "${CRASHPAD_LOCATIONS[@]}"; do
      echo "    - $CRASHPAD_PATH"
    done
  fi
  echo ""
fi
cd ~ || exit 1
exit 0
