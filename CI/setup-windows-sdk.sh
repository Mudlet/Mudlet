#!/bin/bash
###########################################################################
#   Copyright (C) 2024-2024  by John McKisson - john.mckisson@gmail.com   #
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

# Version: 2.0.0    Rework to build on an MSYS2 MINGW64 Github workflow
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

# Script to run once in a ${GITHUB_WORKFLOW} directory in a MSYS2 shell to
# install as much as possible to be able to develop 64/32 Bit Windows
# version of Mudlet

# To be used prior to building Mudlet, after that run:
# * build-mudlet-for-window.sh to compile the currently checked out code
# * package-mudlet-for-windows.sh to put everything together in an archive that
#   will be deployed from a github workflow

# Exit codes:
# 0 - Everything is fine. 8-)
# 1 - Failure to change to a directory
# 2 - Unsupported MSYS2/MINGGW shell type
# 5 - Invalid command line argument
# 6 - One or more Luarocks could not be installed
# 7 - One of more packages failed to install


if [ "${MSYSTEM}" = "MINGW64" ]; then
  export BUILD_BITNESS="64"
  export BUILDCOMPONENT="x86_64"
elif [ "${MSYSTEM}" = "MINGW32" ]; then
  export BUILD_BITNESS="32"
  export BUILDCOMPONENT="i686"
elif [ "${MSYSTEM}" = "MSYS" ]; then
  echo "Please run this script from an MINGW32 or MINGW64 type bash terminal appropriate"
  echo "to the bitness you want to work on. You may do this once for each of them should"
  echo "you wish to do both."
  exit 2
else
  echo "This script is not set up to handle systems of type ${MSYSTEM}, only MINGW32 or"
  echo "MINGW64 are currently supported. Please rerun this in a bash terminal of one"
  echo "of those two types."
  exit 2
fi

# We use this internally - but it is actually the same as ${MINGW_PREFIX}
export MINGW_BASE_DIR=$MSYSTEM_PREFIX
# A more compact - but not necessarily understood by other than MSYS/MINGW
# executables - path:
export MINGW_INTERNAL_BASE_DIR="/mingw${BUILD_BITNESS}"
#
# FIXME: don't add duplicates but rearrange instead to put them in the "right" order:
#
export PATH="${MINGW_INTERNAL_BASE_DIR}/usr/local/bin:${MINGW_INTERNAL_BASE_DIR}/bin:/usr/bin:${PATH}"
echo "MSYSTEM is: ${MSYSTEM}"
echo "PATH is now: ${PATH}"
echo ""

# Options to consider:
# --Sy = Sync, refresh as well as installing the specified packages
# --noconfirm = do not ask for user intervention
# --noprogressbar = do not show progress bars as they are not useful in scripts
echo "  Updating and installing ${MSYSTEM} packages..."
echo ""
echo "    This could take a long time if it is needed to fetch everything, so feel free"
echo "    to go and have a cup of tea (other beverages are available) in the meantime...!"
echo ""

if [ "${MSYSTEM}" = "MINGW64" ]; then
  echo "=== Installing Qt6 Packages ==="
  pacman_attempts=1
  while true; do
    if /usr/bin/pacman -Su --needed --noconfirm \
      "mingw-w64-${BUILDCOMPONENT}-qt6-base" \
      "mingw-w64-${BUILDCOMPONENT}-qt6-multimedia" \
      "mingw-w64-${BUILDCOMPONENT}-qt6-svg" \
      "mingw-w64-${BUILDCOMPONENT}-qt6-speech" \
      "mingw-w64-${BUILDCOMPONENT}-qt6-imageformats" \
      "mingw-w64-${BUILDCOMPONENT}-qt6-tools" \
      "mingw-w64-${BUILDCOMPONENT}-qt6-5compat" \
      "mingw-w64-${BUILDCOMPONENT}-qtkeychain-qt6"; then
        break
    fi

    if [ $pacman_attempts -eq 10 ]; then
      exit 7
    fi
    pacman_attempts=$((pacman_attempts +1))

    echo "=== Some packages failed to install, waiting and trying again ==="
    sleep 10
  done

else

  echo "=== Installing Qt5 Packages ==="
  pacman_attempts=1
  while true; do
    if /usr/bin/pacman -Su --needed --noconfirm \
      "mingw-w64-${BUILDCOMPONENT}-qt5-base" \
      "mingw-w64-${BUILDCOMPONENT}-qt5-multimedia" \
      "mingw-w64-${BUILDCOMPONENT}-qt5-svg" \
      "mingw-w64-${BUILDCOMPONENT}-qt5-speech" \
      "mingw-w64-${BUILDCOMPONENT}-qt5-imageformats" \
      "mingw-w64-${BUILDCOMPONENT}-qt5-winextras" \
      "mingw-w64-${BUILDCOMPONENT}-qt5-tools"; then
        break
    fi

    if [ $pacman_attempts -eq 10 ]; then
      exit 7
    fi
    pacman_attempts=$((pacman_attempts +1))

    echo "=== Some packages failed to install, waiting and trying again ==="
    sleep 10
  done
fi

pacman_attempts=1
while true; do
  if /usr/bin/pacman -Su --needed --noconfirm \
    git \
    man \
    rsync \
    "mingw-w64-${BUILDCOMPONENT}-ccache" \
    "mingw-w64-${BUILDCOMPONENT}-toolchain" \
    "mingw-w64-${BUILDCOMPONENT}-pcre" \
    "mingw-w64-${BUILDCOMPONENT}-libzip" \
    "mingw-w64-${BUILDCOMPONENT}-ntldd" \
    "mingw-w64-${BUILDCOMPONENT}-pugixml" \
    "mingw-w64-${BUILDCOMPONENT}-lua51" \
    "mingw-w64-${BUILDCOMPONENT}-lua51-lpeg" \
    "mingw-w64-${BUILDCOMPONENT}-lua51-lsqlite3" \
    "mingw-w64-${BUILDCOMPONENT}-hunspell" \
    "mingw-w64-${BUILDCOMPONENT}-zlib" \
    "mingw-w64-${BUILDCOMPONENT}-boost" \
    "mingw-w64-${BUILDCOMPONENT}-yajl" \
    "mingw-w64-${BUILDCOMPONENT}-lua-luarocks" \
    "mingw-w64-${BUILDCOMPONENT}-jq"; then
      break
  fi

  if [ $pacman_attempts -eq 10 ]; then
    exit 7
  fi
  pacman_attempts=$((pacman_attempts +1))

  echo "=== Some packages failed to install, waiting and trying again ==="
  sleep 10
done

echo ""
echo "    Completed"
echo ""


if [ "$(grep -c "/.luarocks-${MSYSTEM}" ${MINGW_INTERNAL_BASE_DIR}/etc/luarocks/config-5.1.lua)" -eq 0 ]; then
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
if [ "$(luarocks --lua-version 5.4 list | grep -c "luafilesystem")" -eq 0 ]; then
  # Need to install the 5.4 luafilesystem rock
  echo "  Improving the luarocks operation by installing the 5.4 luafilesystem rock."
  neededPath=$(${MINGW_INTERNAL_BASE_DIR}/bin/luarocks --lua-version 5.4 install luafilesystem 2>&1 | grep "failed making directory" | cut -c 32-)
  until [ -z "${neededPath}" ]; do
    echo "    Inserting a needed directory: ${neededPath} ..."
    mkdir -p "${neededPath}"
    neededPath=$(${MINGW_INTERNAL_BASE_DIR}/bin/luarocks --lua-version 5.4 install luafilesystem 2>&1 | grep "failed making directory" | cut -c 32-)
    echo ""
  done
  echo "    Completed"
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
# CHECKCOMMAND=$(luarocks --lua-version 5.4 list | grep -c "luafilesystem")
# FIXME: Only install if needed - this whole script is safe to be rerun
# (idempotent) but it is an unnecessary delay to reinstall the same things over
# again:
echo ""
echo "  Checking, and installing if needed, the luarocks used by Mudlet..."
echo ""
WANTED_ROCKS=("luafilesystem" "lua-yajl" "luautf8" "lua-zip" "lrexlib-pcre" "luasql-sqlite3" "argparse" "lunajson")

success="true"
for ROCK in "${WANTED_ROCKS[@]}"; do
  if [ "$(luarocks --lua-version 5.1 list | grep -c "${ROCK}")" -eq 0 ]; then
    # This rock is not present
    echo "    ${ROCK}..."
    echo ""
    ${ROCKCOMMAND} install "${ROCK}"
	if [ "$(luarocks --lua-version 5.1 list | grep -c "${ROCK}")" -eq 0 ]; then
	    echo "    ${ROCK} didn't get installed - try rerunning this script..."
		success="false"
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
echo "    ... luarocks installation completed"
echo ""
if [ "${success}" = "true" ]; then
  echo "    ... Completed, all rocks installed."
  echo ""
else
  echo "    ... Failed, one or more rocks are not installed, try again!"
  echo ""
  exit 6
fi
cd ~ || exit 1
echo "  ... setup-windows-sdk.sh shell script finished."
echo ""

echo "Copy the following lines into the build environment for a project in Qt Creator:"
echo "See https://doc.qt.io/qtcreator/creator-how-set-project-environment.html#change-the-environment-for-a-project"
echo ""
echo "WITH_MAIN_BUILD_SYSTEM=NO"
MSYS_ROOT=$(cygpath -aw /)
echo "MINGW_BASE_DIR=${MSYS_ROOT}$(echo ${MSYSTEM_PREFIX} | sed 's/\//\\/g')"
echo "LUA_PATH=$(luarocks --lua-version 5.1 path --lr-path)"
echo "LUA_CPATH=$(luarocks --lua-version 5.1 path --lr-cpath)"

exit 0
