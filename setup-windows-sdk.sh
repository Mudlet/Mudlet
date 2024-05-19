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
#          1.5.0    No change
#          1.4.0    No change
#          1.3.0    Don't explicitly install the no longer supported QT 5
#                   Gamepad stuff (since PR #6787 was merged into
#                   development branch) - it may still be installed as part
#                   of a Qt5 installation but we don't use it any more.
#          1.2.0    Tweak luarocks --tree better and report on failure to
#                   complete
#          1.1.0    Updated to not do things that have already been done
#                   and to offer a choice between a base or a full install
#          1.0.0    Original version

# Script to run once in a directory such as ~/src in a MSYS2 shell to
# install as much as possible to be able to develop 64/32 Bit Windows
# version of Mudlet

# To be used prior to building Mudlet, after that run:
# * build-mudlet-for-window.sh to compile the currently checked out code
#   (defaults to the 'development' branch
# * package-mudlet-for-windows.sh to put everything together in an archive that
#   can be unzipped and run on a different PC.

# Exit codes (some shared with other two scripts):
# 0 - Everything is fine. 8-)
# 1 - Failure to change to a directory
# 2 - Unsupported MSYS2/MINGGW shell type
# 5 - Invalid configuration (32bit Qt6 build requested, or desired installion not clear
# 6 - One or more Luarocks could not be installed

if [ -z "${LEVEL}" ]; then
  # variable not set so assume that this is being called from setup-windows-sdk.sh
  # and do so now:
  if [ -n "${APPVEYOR}" ]; then
    # We are running on AppVeyor so we want a "basic" install unless BUILD_CONFIG is set to "debug"
    LEVEL="base"
  elif [ "$1" = "base" ] || [ "$1" = "full" ]; then
    LEVEL=$1
  elif [ $# -lt 1 ]; then
    echo "Usage: $0 base|full"
    echo ""
    echo "base = install sufficient packages to build Mudlet"
    echo "full = install additional packages to also work on code and develop it"
    echo ""
    exit 5
  else
    echo "Please provide either base or full as a command line option"
    exit 5
  fi
else
  if [ "${LEVEL}" = "base" ] || [ "${LEVEL}" = "full" ]; then
    LEVEL=$1
  else
    echo "Please set the LEVEL variable to either \"base\" or \"full\" as follows:"
    echo "base = install sufficient packages to build Mudlet"
    echo "full = install additional packages to also work on code and develop it"
    echo "or unset it and provide either of those values on the command line"
    echo "to ${0}."
    echo ""
    exit 5
  fi
fi

if [ -z "${QT_MAJOR_VERSION}" ]; then
  # Assume previously used Qt5 unless told otherwise
  export "QT_MAJOR_VERSION=5"
  echo "Assuming a build with Qt 5.x in absence of a QT_MAJOR_VERSION environmental variable."
fi 

if [ -z "${BUILDCOMPONENT}" ]; then
  # Variable not set so do so now
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
  # Variable not set so do so now
  # It might be nice to use the installation of MINGW32 and MINGW64 files that
  # Appveyor provides but it is not obvious that the MSYS2 pacman knows about
  # those files - so just run with the latter's environment.
#  if [ -n "${APPVEYOR}" ]; then
#    # We are running on AppVeyor so we want to use its preconfiged Mingw-w64
#    # installation:
#    if [ "${BUILD_BITNESS}" = "32" ]; then
#      export MINGW_BASE_DIR="C:\mingw-w64\i686-8.1.0-posix-dwarf-rt_v6-rev0"
#    else
#      export MINGW_BASE_DIR="C:\mingw-w64\x86_64-8.1.0-posix-seh-rt_v6-rev0"
#    fi
#  else
#    # We use this internally - but it is actually the same as ${MINGW_PREFIX}
    export MINGW_BASE_DIR="C:\msys64\mingw${BUILD_BITNESS}"
#  fi
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

if [ -z "${BUILD_DIR}" ]; then
  # Variable not set so do so now
  if [ -z "${APPVEYOR_BUILD_FOLDER}" ]; then
    # The above will be defined for AppVeyor CI builds so this is not one of
    # those, this directory will not actually exist should the code not be
    # checked out yet:
    export BUILD_DIR="${HOME}/src/mudlet/build-${MSYSTEM}-qt${QT_MAJOR_VERSION}"
  else
    # On CI builds we can use a plain build folder under the main /c/projects/mudlet
    # directory where the code is automagically placed for us:  
    export BUILD_DIR="${APPVEYOR_BUILD_FOLDER}/build"
  fi
fi

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
echo "BUILD_DIR: ${BUILD_DIR}"
echo "BUILDCOMPONENT: ${BUILDCOMPONENT}"
echo "BUILD_CONFIG: ${BUILD_CONFIG}"
echo "LEVEL: ${LEVEL}"
echo "LUA_PATH is: ${LUA_PATH}"
echo "LUA_CPATH is: ${LUA_CPATH}"
echo "MINGW_BASE_DIR: ${MINGW_BASE_DIR}"
echo "MINGW_INTERNAL_BASE_DIR: ${MINGW_INTERNAL_BASE_DIR}"
echo "MINGW_PREFIX: ${MINGW_PREFIX}"
echo "MSYSTEM: ${MSYSTEM}"
echo "PATH: ${PATH}"
echo "QT_MAJOR_VERSION: ${QT_MAJOR_VERSION}"
echo "HOME directory, (Windows form): $(cygpath -w "${HOME}")"
echo "HOME directory, (POSIX form): $(cygpath -u "${HOME}")"
echo ""

# Thigs every type of build will need:
# MINGW32 has lost the mingw-w64-i686-qt5 group package so we'll have to get
# the individual ones ourselves instead of just putting:
# "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}":
PACKAGES=( "git" \
  "mingw-w64-${BUILDCOMPONENT}-boost" \
  "mingw-w64-${BUILDCOMPONENT}-ccache" \
  "mingw-w64-${BUILDCOMPONENT}-hunspell" \
  "mingw-w64-${BUILDCOMPONENT}-libzip" \
  "mingw-w64-${BUILDCOMPONENT}-lua-luarocks" \
  "mingw-w64-${BUILDCOMPONENT}-lua51" \
  "mingw-w64-${BUILDCOMPONENT}-lua51-lpeg" \
  "mingw-w64-${BUILDCOMPONENT}-lua51-lsqlite3" \
  "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-base" \
  "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-imageformats" \
  "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-multimedia" \
  "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-speech" \
  "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-svg" \
  "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-tools" \
  "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-translations" \
  "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-virtualkeyboard" \
  "mingw-w64-${BUILDCOMPONENT}-ntldd" \
  "mingw-w64-${BUILDCOMPONENT}-pcre" \
  "mingw-w64-${BUILDCOMPONENT}-pugixml" \
  "mingw-w64-${BUILDCOMPONENT}-toolchain" \
  "mingw-w64-${BUILDCOMPONENT}-yajl" \
  "mingw-w64-${BUILDCOMPONENT}-zlib" \
  "rsync" )

# Other packages that we do not need/have available:
#     "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-3d"
#     "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-activeqt"
#     "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-charts"
#     "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-connectivity"
#     "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-datavis3d"
#     "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-declarative"
#     "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-graphicaleffects"
#     "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-location"
#     "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-lottie"
#     "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-networkauth"
#     "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-purchasing"
#     "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-quick3d"
#     "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-quickcontrols"
#     "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-quickcontrols2"
#     "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-quicktimeline"
#     "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-remoteobjects"
#     "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-script"
#     "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-scxml"
#     "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-sensors"
#     "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-serialbus"
#     "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-serialport"
#     "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-webchannel"
#     "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-webglplugin"
#     "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-websockets"
#     "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-webview"
#     "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-xmlpatterns"



if [ "${QT_MAJOR_VERSION}" = "6" ]; then
  # Add things every Qt6 build will need - just the compatibility module:
  PACKAGES+=( "mingw-w64-${BUILDCOMPONENT}-qt6-5compat" )
else
  # Add things every Qt5 build will need - which are not in Qt6:
  PACKAGES+=( "mingw-w64-${BUILDCOMPONENT}-qt5-gamepad" \
    "mingw-w64-${BUILDCOMPONENT}-qt5-winextras" )
fi

# Add things we want for 64 bit builds - just the Qt Keychain module - we have
# to build it ourselves for the 32 Bit case:
if [ "${BUILD_BITNESS}" = "64" ]; then
  PACKAGES+=( "mingw-w64-${BUILDCOMPONENT}-qtkeychain-qt${QT_MAJOR_VERSION}" )
  # We will also need to tell our project build systems not to build our
  # bundled Qt Keychain module
fi

# Add things that end-user hackers/developers will want:
if [ "${LEVEL}" = "full" ]; then
  PACKAGES+=( "man" \
    "mingw-w64-${BUILDCOMPONENT}-clang" \
    "mingw-w64-${BUILDCOMPONENT}-cmake" \
    "mingw-w64-${BUILDCOMPONENT}-ninja" )
  if [ "${BUILD_BITNESS}" = "64" ]; then
    # Additionally we can add Qt Creator and some help documentation but
    # only for the 64 bit case:
    PACKAGES+=( "mingw-w64-${BUILDCOMPONENT}-qt-creator" \
      "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-doc" \
      "mingw-w64-${BUILDCOMPONENT}-qt-creator-docs" )
  fi
fi

# Add things needed for "debug" type builds - which the "full" type of install implies:
if ([ "${LEVEL}" = "full" ] || [ "${BUILD_CONFIG}" = "debug" ]) && [ "${BUILD_BITNESS}" = "64" ]; then
  # Unfortunately these libraries are no longer available for the 32-Bit case
  # unless https://github.com/msys2/MINGW-packages/issues/20902 gets done    
  PACKAGES+=( "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-base-debug" \
    "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-imageformats-debug" \
    "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-multimedia-debug" \
    "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-speech-debug" \
    "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-svg-debug" \
    "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-tools-debug" \
    "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-virtualkeyboard-debug" )
  if [ "${QT_MAJOR_VERSION}" = "5" ]; then
    # winextras is not present in Qt6
    PACKAGES+=( "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-winextras-debug" )
  else
    # We need an extra library for the Qt5 compatibility module in Qt6:
    PACKAGES+=( "mingw-w64-${BUILDCOMPONENT}-qt${QT_MAJOR_VERSION}-5compat-debug" )
  fi
fi

if [ -n "${APPVEYOR}" ]; then
  # Our CI builds may need wget to upload the archive file to Mudlet's website
  PACKAGES+=( "wget" )
fi

# Options to consider:
# --Sy = Sync, refresh as well as installing the specified packages
# --noconfirm = do not ask for user intervention
# --noprogressbar = do not show progress bars as they are not useful in scripts
echo "  Updating and installing ${MSYSTEM} packages..."
echo ""
echo "    This could take a long time if it is needed to fetch everything, so feel free"
echo "    to go and have a cup of tea (other beverages are available) in the meantime...!"
echo ""
/usr/bin/pacman -S --needed --noconfirm "${PACKAGES[@]}"

echo ""
echo "    Completed"
echo ""

if [ -z "${APPVEYOR}" ] && [ "${MSYSTEM}" = "MINGW32" ] && [ "${LEVEL}" = "full" ]; then
  # Qt creator is only available to be run in a MINGW64 environment but it
  # can be used to run the MINGW32 components - which are needed to do that
  echo "    You WILL need to also have run the full setup mode for the MINGW64 case"
  echo "    in order to have the Qt Creator IDE to develop MINGW32 builds!"
  echo ""
fi

if [ $(grep -c "/.luarocks-${MSYSTEM}" "${MINGW_INTERNAL_BASE_DIR}/etc/luarocks/config-5.1.lua") -eq 0 ]; then
  # The luarocks config file has not been tweaked to put the compiled rocks in
  # a location that is different for each different MSYS2 environment
  echo "  Tweaking location for constructed Luarocks so 32 and 64 bits ones do"
  echo "  not end up in the same place when --tree \"user\" is used..."
  echo ""
  
  cp "${MINGW_INTERNAL_BASE_DIR}/etc/luarocks/config-5.1.lua" "${MINGW_INTERNAL_BASE_DIR}/etc/luarocks/config-5.1.lua.orig"
  cp "${MINGW_INTERNAL_BASE_DIR}/etc/luarocks/config-5.4.lua" "${MINGW_INTERNAL_BASE_DIR}/etc/luarocks/config-5.4.lua.orig"
  /usr/bin/sed "s|.. \"/.luarocks\"|.. \"/.luarocks-${MSYSTEM}\"|" "${MINGW_INTERNAL_BASE_DIR}/etc/luarocks/config-5.1.lua.orig" > "${MINGW_INTERNAL_BASE_DIR}/etc/luarocks/config-5.1.lua"
  /usr/bin/sed "s|.. \"/.luarocks\"|.. \"/.luarocks-${MSYSTEM}\"|" "${MINGW_INTERNAL_BASE_DIR}/etc/luarocks/config-5.4.lua.orig" > "${MINGW_INTERNAL_BASE_DIR}/etc/luarocks/config-5.4.lua"
  echo "    Completed" 
else
  echo "  Things have already been setup for Luarocks so 32 and 64 bits ones"
  echo "  do not end up in the same place"
fi
echo ""
echo "  When using lua modules from luarocks and you wish to use a per-user one"
echo "rather than sharing them with other users on this PC it is recommended that"
echo "you avoid using the --local option to luarocks but instead use"
echo "--tree \"user\" {the word user, NOT your username). This is so that if you"
echo "are building things for other PC systems and are working with more than"
echo "one 'bitness' or are investigating the different Mingw-w64 environments"
echo "the rocks compiled for each variant do not end up in the same set of"
echo "directories - which will break things!"
echo ""
echo "You will probably have to tweak the LUA_PATH and LUA_CPATH environmental"
echo "variables before running Mudlet so that it can find the per-user modules"
echo "You will likely want to review the output from 'luarocks path --help'"
echo "for more information (remembering to include the '--lua-version 5.1'"
echo "command line argument to get the right details when using it!)"
echo ""

# Need to overcome a problem with luarock 3.9.0 which uses Windows CMD MKDIR
# but which cannot make any missing intermediate directories if the
# luafilesystem module for Lua 5.4 is not present, see:
# https://github.com/msys2/MINGW-packages/pull/12002
if [ $(luarocks --lua-version 5.4 list | grep -c "luafilesystem") -eq 0 ]; then
  # Need to install the 5.4 luafilesystem rock
  echo "  Improving the luarocks operation by installing the 5.4 luafilesystem rock."
  NEEDED_PATH=$("${MINGW_INTERNAL_BASE_DIR}/bin/luarocks" --lua-version 5.4 install luafilesystem 2>&1 | grep "failed making directory" | cut -c 32-)
  until [ -z "${NEEDED_PATH}" ]; do
    echo "    Inserting a needed directory: ${NEEDED_PATH} ..."
    mkdir -p "${NEEDED_PATH}"
    NEEDED_PATH=$("${MINGW_INTERNAL_BASE_DIR}/bin/luarocks" --lua-version 5.4 install luafilesystem 2>&1 | grep "failed making directory" | cut -c 32-)
    echo ""
  done
  echo "  Completed"
  echo ""
fi

# Doing the above fixes things for 5.1 luarocks subsequently (as luarocks
# itself runs in a Lua 5.4 environment) - otherwise one has to do the same thing
# for EVERY luarock!

# Save the wanted Luarocks 5.1 command so it can be used repeatedly next
# this uses the "system" (shared between all users) tree, using the --local
# option does NOT work, but the alternative --tree "user" (the literal string
# user NOT the user's name) does for "per user" luarocks as the previous use
# of sed above has "fixed" things:
ROCKCOMMAND="${MINGW_INTERNAL_BASE_DIR}/bin/luarocks --lua-version 5.1"
echo ""
echo "  Checking, and installing if needed, the luarocks used by Mudlet..."
echo ""
WANTED_ROCKS=("luafilesystem" "lua-yajl" "luautf8" "lua-zip" "lrexlib-pcre" "luasql-sqlite3")

SUCCESS="true"
for ROCK in "${WANTED_ROCKS[@]}"; do
  if [ $(luarocks --lua-version 5.1 list | grep -c "${ROCK}") -eq 0 ]; then
    # This rock is not present
    echo "    ${ROCK}..."
    echo ""
    ${ROCKCOMMAND} install "${ROCK}"
    if [ $(luarocks --lua-version 5.1 list | grep -c "${ROCK}") -eq 0 ]; then
      echo "    ${ROCK} didn't get installed - try rerunning this script..."
      SUCCESS="false"
    else
      echo "    ${ROCK} now installed"
    fi
  else
    # We have it already
    echo "    ${ROCK} is already present"
  fi
  echo ""
done
echo ""
if [ "${SUCCESS}" = "true" ]; then
  echo "  ... luarocks installation completed."
  echo ""
else
  echo "  ... Failed, one or more rocks are not installed, try again!"
  echo ""
  exit 6
fi

if [ -z "${APPVEYOR}" ]; then
  # This is NOT an AppVeyor CI/CB so we may need to clone the Mudlet git
  # repository ourselves - AppVeyor does this automagically for the
  # branch/commit needed to be built - but otherwise we need TWO levels up
  # one for the release or debug "build" sub-directory and one for the
  # "mudlet" one above that to check the source repository out to: 
  GRAND_PARENT_OF_BUILD_DIR="$(echo "${BUILD_DIR}" | sed -e "s|/[^/]*$||" | sed -e "s|/[^/]*$||")"
  echo ""
  echo "  Making a ${GRAND_PARENT_OF_BUILD_DIR} directory (if it does not exist)"
  echo "  - so there is a place to put a local Mudlet git repository for"
  echo "  you to build and work on the project..."
  echo ""
  mkdir -p "${GRAND_PARENT_OF_BUILD_DIR}"
  cd "${GRAND_PARENT_OF_BUILD_DIR}" || exit 1
  if [ ! -d "./mudlet" ]; then
    echo "    Cloning the Mudlet project's source code..."
    echo ""
    # We could do a "shallow" clone for the "base" case:
    git clone https://github.com/Mudlet/Mudlet.git mudlet
  else
    echo "    There is already a ${GRAND_PARENT_OF_BUILD_DIR}/mudlet"
    echo "    directory so it seems that there is no need to clone the Mudlet"
    echo "    project's source code..."
  fi
  cd ~ || exit 1
fi
echo ""
echo "... setup-windows-sdk.sh shell script finished."
echo ""

exit 0
