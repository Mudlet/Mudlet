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

# Version: 2.2.0    Add CMake package for CMake-based builds
#          2.1.0    Remove MINGW32 since upstream no longer supports it
#          2.0.0    Rework to build on an MSYS2 MINGW64 Github workflow
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
# install as much as possible to be able to develop 64 Bit Windows
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

# We use this internally - but it is actually the same as ${MINGW_PREFIX}
export MINGW_BASE_DIR=${MSYSTEM_PREFIX}
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

echo "=== Installing needed packages ==="
pacman_attempts=0
while true; do
  if /usr/bin/pacman -Su --needed --noconfirm \
    "mingw-w64-${BUILDCOMPONENT}-qt6-base" \
    "mingw-w64-${BUILDCOMPONENT}-qt6-multimedia" \
    "mingw-w64-${BUILDCOMPONENT}-qt6-multimedia-wmf" \
    "mingw-w64-${BUILDCOMPONENT}-qt6-svg" \
    "mingw-w64-${BUILDCOMPONENT}-qt6-speech" \
    "mingw-w64-${BUILDCOMPONENT}-qt6-imageformats" \
    "mingw-w64-${BUILDCOMPONENT}-qt6-translations" \
    "mingw-w64-${BUILDCOMPONENT}-qt6-tools" \
    "mingw-w64-${BUILDCOMPONENT}-qt6-5compat" \
    "mingw-w64-${BUILDCOMPONENT}-angleproject" \
    "mingw-w64-${BUILDCOMPONENT}-qtkeychain-qt6" \
    git \
    man \
    rsync \
    "mingw-w64-${BUILDCOMPONENT}-ccache" \
    "mingw-w64-${BUILDCOMPONENT}-toolchain" \
    "mingw-w64-${BUILDCOMPONENT}-pcre2" \
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
    "mingw-w64-${BUILDCOMPONENT}-cmake" \
    "mingw-w64-${BUILDCOMPONENT}-meson" \
    "mingw-w64-${BUILDCOMPONENT}-ninja" \
    "mingw-w64-${BUILDCOMPONENT}-assimp" \
    "mingw-w64-${BUILDCOMPONENT}-jq"; then
      break
  fi

  pacman_attempts=$((pacman_attempts +1))

  if [ ${pacman_attempts} -lt 10 ]; then
    echo "=== Some packages failed to install, waiting and trying again ==="
    sleep 10
  else
    echo "=== Some packages failed to install after ${pacman_attempts} attempts, giving up ==="
    exit 7
  fi
done

echo "Removing harfbuzz installed by qt"
pacman -Rdd --noconfirm mingw-w64-${BUILDCOMPONENT}-harfbuzz

echo "Building harfbuzz without graphite2"
git clone https://github.com/harfbuzz/harfbuzz.git
cd harfbuzz || exit 1
meson setup build --prefix=/mingw${BUILD_BITNESS} --buildtype=release -Dgraphite=disabled -Dtests=disabled
meson compile -C build
meson install -C build

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
echo "For per-user Lua modules from LuaRocks:"
echo "- Use '--tree \"user\"' (literally) instead of '--local'"
echo "- Adjust LUA_PATH and LUA_CPATH to find per-user modules"
echo "- See 'luarocks path --help' for details"


ROCKCOMMAND="${MINGW_INTERNAL_BASE_DIR}/bin/luarocks --lua-version 5.1"
echo ""
echo "  Checking, and installing if needed, the luarocks used by Mudlet..."
echo ""
WANTED_ROCKS=("luafilesystem" "lua-yajl" "luautf8" "lua-zip" "lrexlib-pcre2" "luasql-sqlite3" "argparse" "lunajson" "busted")

success="true"
for ROCK in "${WANTED_ROCKS[@]}"; do
  if [ "$(luarocks --lua-version 5.1 list | grep -c "${ROCK}")" -eq 0 ]; then
    # This rock is not present
    echo "    ${ROCK}..."
    echo ""
    # We need to force a particular version for the moment as 2.7.0-1 doesn't
    # seem to work on Windows:
    if [ "${ROCK}" = "luasql-sqlite3" ];  then
      ${ROCKCOMMAND} install "${ROCK}" "2.6.1"
    else
      ${ROCKCOMMAND} install "${ROCK}"
    fi
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
MSYS_ROOT=$(cygpath -aw /)
echo "MINGW_BASE_DIR=${MSYS_ROOT}$(echo "${MSYSTEM_PREFIX}" | sed 's/\//\\/g')"
echo "LUA_PATH=$(luarocks --lua-version 5.1 path --lr-path)"
echo "LUA_CPATH=$(luarocks --lua-version 5.1 path --lr-cpath)"

exit 0
