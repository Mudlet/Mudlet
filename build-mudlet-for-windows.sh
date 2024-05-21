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

# Version: 1.6.0    Changed to allow scripts to be used for Mudlet's own
#                   CI/CB process on AppVeyor.
#          1.5.0    Change BUILD_TYPE to BUILD_CONFIG to avoid clash with
#                   CI/CB system using same variable
#          1.4.0    Rewrite Makefile to use ccache.exe if available
#          1.3.0    No changes
#          1.2.0    No changes
#          1.1.0    No changes
#          1.0.0    Original version

# Script to use each time to build the Mudlet code currently checked out in
# ${BUILD_DIR} in a MINGW32 or MINGW64 shell

# To be used AFTER setup-windows-sdk.sh has been run; once this has completed
# successfully use:
# * package-mudlet-for-windows.sh to put everything together in an archive that
#   can be unzipped andrun on a different PC.

# Exit codes (some shared with other two scripts):
# 0 - Everything is fine. 8-)
# 1 - Failure to change to a directory
# 2 - Unsupported MSYS2/MINGGW shell type (only "MINGW32" or "MINGW64" currently supported)
# 3 - Unsupported build type (BUILD_CONFIG set, but not to "debug" or "release")
# 5 - Invalid configuration (32bit Qt6 build requested, or what is desired to be installed doesn't make sense)
# 8 - A PTB build was requested but the most recent commit is more than a day ago

# No check for ${LEVEL} in this script - not needed, I hope

if [ -z "${QT_MAJOR_VERSION}" ]; then
  # Assume previously used Qt5 unless told otherwise
  export QT_MAJOR_VERSION="5"
  if [ -z "${APPVEYOR}" ]; then
    # Don't bother reporting this for CI builds as the previous script will do so
    echo "Assuming a build with Qt 5.x in absence of a QT_MAJOR_VERSION environmental variable."
  fi
fi

if [ -z "${BUILD_CONFIG}" ]; then
  # If this is present and set to "debug" then we'll do a debug type build but
  # otherwise we'll keep it as "release" - the build and package scripts will
  # also need to be told if we want a debug build
  export BUILD_CONFIG="release"
elif [ "${BUILD_CONFIG}" != "release" ] && [ "${BUILD_CONFIG}" != "debug" ]; then
  echo "Please set the environmental variable BUILD_CONFIG to one of \"Release\" or"
  echo "\"Debug\" to specify which type of build you wish this to be."
  exit 3
fi

if [ -z "${BUILDCOMPONENT}" ]; then
  if [ "${MSYSTEM}" = "MINGW64" ]; then
    # We are running in a 64-Bit terminal so assume that that is what the user
    # to build:
    export BUILD_BITNESS="64"
    export BUILDCOMPONENT="x86_64"
  elif [ "${MSYSTEM}" = "MINGW32" ]; then
    # We are running in a 32-Bit terminal so assume that that is what the user
    # to build (only possible to do a "base" build - using mingw32-qmake
    # directly as there is not a 32-Bit Qt Creator nowadays):
    export BUILD_BITNESS="32"
    export BUILDCOMPONENT="i686"
  elif [ "${MSYSTEM}" = "MSYS" ]; then
    echo "Please run this script from an MINGW32 or MINGW64 type bash terminal appropriate"
    echo "to the bitness you want to work on. You may do this once for each of them should"
    echo "you wish to do both."
    exit 2
  elif [ -z "${MSYSTEM}" ]; then
    echo "The environmental variable MSYSTEM is not set to anything so something is amiss"
    echo "Please rerun this in a Mingw-w64 MINGW32 or MINGW64 bash terminal."
    exit 2
  else
    echo "This script is not set up to handle systems of type ${MSYSTEM}, only MINGW32 or"
    echo "MINGW64 are currently supported. Please rerun this in a bash terminal of one"
    echo "of those two types."
    exit 2
  fi
fi

if [ "${QT_MAJOR_VERSION}" = "6" ] && [ "${BUILD_BITNESS}" = "32" ]; then
    echo ""
    echo "Sorry, but it is not possible to perform 32-bit (i686) builds with Qt 6, build aborted!"
    exit 5
fi

if [ -z "${MINGW_INTERNAL_BASE_DIR}" ]; then
  # Variable not set so do so now - see setup-windows-sdk.sh why we are not
  # using the MINGW32/MINGW64 files that Appveyor might provide.
  export MINGW_BASE_DIR="C:\msys64\mingw${BUILD_BITNESS}"
  # Provide an equivalent POSIX format path for internal usage:
  export MINGW_INTERNAL_BASE_DIR="$(cygpath -u "${MINGW_BASE_DIR}")"
fi

# Adjust paths so directories we want are prepended if not present:
# From https://stackoverflow.com/a/48185201/4805858:
case :$PATH: in
  *:/usr/bin:*)
    ;; # do nothing, it's there
  *)
    export PATH="/usr/bin:${PATH}"
    echo "Prepending /usr/bin to PATH"
    ;;
esac
case :$PATH: in
  *:${MINGW_INTERNAL_BASE_DIR}/bin:*)
    ;; # do nothing, it's there
  *)
    export PATH="${MINGW_INTERNAL_BASE_DIR}/bin:${PATH}"
    echo "Prepending ${MINGW_INTERNAL_BASE_DIR}/bin to PATH"
    ;;
esac
case :$PATH: in
  *:${MINGW_INTERNAL_BASE_DIR}/usr/local/bin:*)
    ;; # do nothing, it's there
  *)
    export PATH="${MINGW_INTERNAL_BASE_DIR}/usr/local/bin:${PATH}"
    echo "Prepending ${MINGW_INTERNAL_BASE_DIR}/usr/local/bin to PATH"
    ;;
esac

if [ -z "${LUA_PATH}" ] || [ -z "${LUA_PATH}" ]; then
  # One or both are not set, but we can do this if there is a luarocks available
  if luarocks --lua-version 5.1 2> /dev/null | grep -q "lua5\.1\.exe.*\(ok\)" ; then
    # We do have a 5.1 luarocks available, so proceed
    # If one is planning to use qtcreator these will probably be wanted in a
    # shell startup script so as to prepare it to use Lua 5.1 when running
    # qmake (needed to process translation files to get the translations
    # statistics):
    # We need cygpath to output "mixed" mode paths (uses "C:"s but with '/' as separator)
    if [ -z "${LUA_PATH}" ]; then
      export LUA_PATH=$(luarocks --lua-version 5.1 path --lr-path | sed -e 's|\\|/|g')
      echo "Setting LUA_PATH to: ${LUA_PATH}"
    fi
    if [ -z "${LUA_CPATH}" ]; then
      export LUA_CPATH=$(luarocks --lua-version 5.1 path --lr-cpath | sed -e 's|\\|/|g')
      echo "Setting LUA_CPATH to: ${LUA_CPATH}"
    fi
  fi
fi

if [ -z "${BUILD_DIR}" ]; then
  # Variable not set so do so now
  if [ -z "${APPVEYOR_BUILD_FOLDER}" ]; then
    # The above will be defined for AppVeyor CI builds so this is not one of
    # those, and we need to allow for the end user to have multiple
    # builds in different directories or (for 64bit builds) to use either Qt 5 or 6:
    export BUILD_DIR="${HOME}/src/mudlet/build-${MSYSTEM}-qt${QT_MAJOR_VERSION}"
  else
    # On CI builds we can use a plain build folder under the main /c/projects/mudlet
    # directory where the code is automagically placed for us:  
    export BUILD_DIR="${APPVEYOR_BUILD_FOLDER}/build"
  fi
  # Make and then go there before we start the build:
  mkdir -p "${BUILD_DIR}"
  cd "${BUILD_DIR}" || exit 1
fi

# In practice this is where the Mudlet source code git repository is placed:
export PARENT_OF_BUILD_DIR="$(echo "${BUILD_DIR}" | sed -e "s|/[^/]*$||")"

# Identify what we are going to do:
if [ -n "${APPVEYOR}" ]; then
  # This is an Appveyor CI build
  if [ "${APPVEYOR_REPO_NAME}" = "Mudlet/Mudlet" ]; then
    # This is being run on Mudlet's own repo
    if [ -n "${APPVEYOR_REPO_TAG_NAME}" ]; then
      # It is a build triggered by a tagged commit - so it is likely to be a
      # proper RELEASE build.
      export TASK="RELEASE"
      # This will only persist into the QMake/CMake makefile generation process
      # if the project files for them has been edited to allow this through as
      # an empty string:
      export MUDLET_VERSION_BUILD=""
    elif [ "${APPVEYOR_SCHEDULED_BUILD}" = "True" ]; then
      # It is a scheduled build so it is a Public Test Build
      export TASK="PTB"
      COMMIT_EPOCH=$(date -u +%s -d "$(git show -s --format="%cs")")
      NOW_EPOCH=$(date -u +%s)
      SECONDS_DIFF=$(( NOW_EPOCH - COMMIT_EPOCH ))
      DAYS_DIIF=$(( SECONDS_DIFF / 86400 ))
      if [ "${DAYS_DIIF}" != "0" ]; then
        echo "Last commit was: ${SECONDS_DIFF} seconds, i.e. at least ${DAYS_DIIF} day(s) ago - Public Test build aborted!"
        exit 8
      fi
      export BUILD_COMMIT=$(git rev-parse --short HEAD | sed 's/.*/\L&/g')
      export MUDLET_VERSION_BUILD="-ptb-$(date -u -Idate)"
    elif [ -n "${APPVEYOR_PULL_REQUEST_NUMBER}" ]; then
      # It is a PR buiild
      export TASK="PR"
      # AppVeyor builds of PRs merge the PR head onto the current development
      # branch creating a new commit - as such we need to refer to the commit
      # Git SHA1 supplied to us rather than trying to back track to the
      # ancestor in the "working" tree (though it does mean the code state is
      # not accurately described as it only reports the PR's head without
      # reference to the state of the development at the time of the build:
      # MUDLET_VERSION_BUILD might be an empty string before this line or it
      # could be a hyphen prefixed string to identify a 3rd party build
      export BUILD_COMMIT=$(git rev-parse --short "${APPVEYOR_PULL_REQUEST_HEAD_COMMIT}"| sed 's/.*/\L&/g')
      export MUDLET_VERSION_BUILD=$(echo "${MUDLET_VERSION_BUILD}-testing-PR${APPVEYOR_PULL_REQUEST_NUMBER}" | sed 's/.*/\L&/g')
    else
      # It is some other testing build which needs an archive to be made with a
      # specific name
      export TASK="TESTING"
      export BUILD_COMMIT=$(git rev-parse --short HEAD | sed 's/.*/\L&/g')
      export MUDLET_VERSION_BUILD=$(echo "${MUDLET_VERSION_BUILD}-testing" | sed 's/.*/\L&/g')
    fi
  else
    # Not Mudlet's repository so just produce a zip file
    export TASK="ZIP"
    export BUILD_COMMIT=$(git rev-parse --short HEAD | sed 's/.*/\L&/g')
    # MUDLET_VERSION_BUILD could be an empty string but it is intended for
    # third party packagers to tag customised versions of Mudlet:
    if [ -n "${MUDLET_VERSION_BUILD}" ]; then
      export MUDLET_VERSION_BUILD=$(echo "${MUDLET_VERSION_BUILD}" | sed 's/.*/\L&/g')
    fi
  fi
else
  # Not an appveyor CI build so just produce an archive
  export TASK="ZIP"
  export BUILD_COMMIT=$(git rev-parse --short HEAD | sed 's/.*/\L&/g')
  # MUDLET_VERSION_BUILD could be an empty string but it is intended for
  # third party packagers to tag customised versions of Mudlet:
  if [ -n "${MUDLET_VERSION_BUILD}" ]; then
    export MUDLET_VERSION_BUILD=$(echo "${MUDLET_VERSION_BUILD}" | sed 's/.*/\L&/g')
  fi
fi

# Extract version information from qmake project file
# sed is used to remove the spaces either side of the `=` in the one line in
# the file that will match:
VERSION=$(grep "^VERSION = " "${PARENT_OF_BUILD_DIR}/src/mudlet.pro" | sed -e 's/VERSION = //g')
export VERSION

### End of common(-ish) configuration for all three script files.

# Now display all the variables for checking in build logs:
echo ""
echo "Current variables:"
echo "APPVEYOR: ${APPVEYOR}"
if [ -n "${APPVEYOR}" ]; then
  echo "  APPVEYOR_BUILD_FOLDER: ${APPVEYOR_BUILD_FOLDER}"
  echo "  APPVEYOR_REPO_NAME: ${APPVEYOR_REPO_NAME}"
  echo "  APPVEYOR_REPO_TAG: ${APPVEYOR_REPO_TAG}"
  echo "  APPVEYOR_PULL_REQUEST_NUMBER: ${APPVEYOR_PULL_REQUEST_NUMBER}"
  echo "  APPVEYOR_REPO_COMMIT: ${APPVEYOR_REPO_COMMIT}"
  echo "  APPVEYOR_PULL_REQUEST_HEAD_COMMIT: ${APPVEYOR_PULL_REQUEST_HEAD_COMMIT}"
  echo "  APPVEYOR_PROJECT_SLUG: ${APPVEYOR_PROJECT_SLUG}"
fi
echo "BUILD_BITNESS: ${BUILD_BITNESS}"
echo "BUILD_COMMIT: ${BUILD_COMMIT}"
echo "BUILD_CONFIG: ${BUILD_CONFIG}"
echo "BUILD_DIR: ${BUILD_DIR}"
echo "BUILDCOMPONENT: ${BUILDCOMPONENT}"
echo "LUA_PATH is: ${LUA_PATH}"
echo "LUA_CPATH is: ${LUA_CPATH}"
echo "MINGW_BASE_DIR: ${MINGW_BASE_DIR}"
echo "MINGW_INTERNAL_BASE_DIR: ${MINGW_INTERNAL_BASE_DIR}"
echo "MINGW_PREFIX: ${MINGW_PREFIX}"
echo "MSYSTEM: ${MSYSTEM}"
echo "MUDLET_VERSION_BUILD: ${MUDLET_VERSION_BUILD}"
echo "PACKAGE_DIR: ${PACKAGE_DIR}"
echo "PARENT_OF_BUILD_DIR: ${PARENT_OF_BUILD_DIR}"
echo "PATH: ${PATH}"
echo "QT_MAJOR_VERSION: ${QT_MAJOR_VERSION}"
echo "VERSION: ${VERSION}"
echo "TASK: ${TASK}"
echo "HOME directory, (Windows form): $(cygpath -w "${HOME}")"
echo "HOME directory, (POSIX form): $(cygpath -u "${HOME}")"
echo ""

if [ "${BUILD_BITNESS}" = "64" ]; then
    # Tell our project build systems not to build our bundled Qt Keychain module
    export WITH_OWN_QTKEYCHAIN="NO"
else
    # We have to build the library ourselves for the 32-bit case as MSYS2+Mingw-w64
    # does not ship #
    export WITH_OWN_QTKEYCHAIN="YES"
fi

#### Qt Creator note ####
# It doesn't seem possible to configure ccache to work like this in a direct
# call of qmake - though it does inside the Qt Creator qmake additional
# arguments field:
echo "Checking for ccache..."
echo ""
if [ -f "${MINGW_INTERNAL_BASE_DIR}/bin/ccache" ]; then
  echo "  ... ccache has been found and it will try to be used for this build by"
  echo "    adjusting the Makefile."
  export WITH_CCACHE="YES"
else
  echo "  ... ccache NOT found."
  export WITH_CCACHE="NO"
fi
echo ""


#### Qt Creator note ####
# FIXME:
# The updater is not helpful in this (build it yourself) environment
export WITH_UPDATER="NO"


echo "WITH_XXX variables currently defined:"
set | grep "^WITH_"
echo ""
echo "Running qmake to make MAKEFILE ..."
echo ""

# We do not use QtQuick so there is no need for those features:
if [ "${QT_MAJOR_VERSION}" = "6" ]; then
  if [ "${BUILD_CONFIG}" = "debug" ]; then
    qmake6 ../src/mudlet.pro -spec win32-g++ "CONFIG-=qml_debug" "CONFIG-=qtquickcompiler" "CONFIG+=debug" "CONFIG+=separate_debug_info"
  else
    qmake6 ../src/mudlet.pro -spec win32-g++ "CONFIG-=qml_debug" "CONFIG-=qtquickcompiler"
  fi
else
  if [ "${BUILD_CONFIG}" = "debug" ]; then
    qmake ../src/mudlet.pro -spec win32-g++ "CONFIG-=qml_debug" "CONFIG-=qtquickcompiler" "CONFIG+=debug" "CONFIG+=separate_debug_info"
  else
    qmake ../src/mudlet.pro -spec win32-g++ "CONFIG-=qml_debug" "CONFIG-=qtquickcompiler"
  fi
fi

echo " ... qmake done."
echo ""

if [ "${WITH_CCACHE}" = "YES" ]; then
  if [ "${BUILD_CONFIG}" = "debug" ]; then
    echo "  Tweaking Makefile.Debug to use ccache..."
    sed -i "s/CC            = gcc/CC            = ccache gcc/" ./Makefile.Debug
    sed -i "s/CXX           = g++/CXX           = ccache g++/" ./Makefile.Debug
  else
    echo "  Tweaking Makefile.Release to use ccache..."
    sed -i "s/CC            = gcc/CC            = ccache gcc/" ./Makefile.Release
    sed -i "s/CXX           = g++/CXX           = ccache g++/" ./Makefile.Release
  fi
  echo ""
fi

echo "Running make to build project ..."
echo ""

# Despite the mingw32 prefix mingw32-make.exe IS the make we want.
# Use -k to "keep going" to build as much as possible this time around:
if [ -n "${NUMBER_OF_PROCESSORS}" ] && [ "${NUMBER_OF_PROCESSORS}" -gt 1 ]; then
  mingw32-make -j "${NUMBER_OF_PROCESSORS}" -k
else
  mingw32-make -k
fi

echo "   ... make finished"
echo ""

cd ~ || exit 1
echo ""
echo "... build-mudlet-for-windows shell script finished."

