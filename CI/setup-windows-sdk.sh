#!/bin/bash
###########################################################################
#   Copyright (C) 2024-2026  by John McKisson - john.mckisson@gmail.com   #
#   Copyright (C) 2023-2026  by Stephen Lyons - slysven@virginmedia.com   #
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

# Version: 2.7.0    Hold every Qt module at one version while MSYS2 is part
#                   way through publishing a new Qt release
#          2.6.0    Drop the Info-ZIP unzip pin: libarchive 3.8.9-3 opens
#                   extracted files with O_BINARY, so bsdunzip no longer
#                   mangles line endings
#          2.5.0    Pin luarocks to Info-ZIP unzip, since libarchive now
#                   hardlinks bsdunzip over "unzip" and it mangles line
#                   endings when it unpacks a rock
#          2.4.0    Add Python, needed by the fixture HTTP server the Lua
#                   tests run against
#          2.3.0    Switch from MINGW64 to CLANG64
#          2.2.0    Add CMake package for CMake-based builds
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
# * package-mudlet-for-windows.sh to assemble everything Mudlet needs to run
# * create-portable-zip-for-windows.sh to zip that up as the portable build
# * deploy-mudlet-for-windows.sh to build the installer and deploy it from a
#   github workflow

# Exit codes:
# 0 - Everything is fine. 8-)
# 1 - Failure to change to a directory
# 2 - Unsupported MSYS2 shell type
# 5 - Invalid command line argument
# 6 - One or more Luarocks could not be installed
# 7 - One of more packages failed to install


if [ "${MSYSTEM}" = "MSYS" ]; then
  echo "Please run this script from a CLANG64 type bash terminal as the MSYS one"
  echo "does not supported what is needed."
  exit 2
elif [ "${MSYSTEM}" = "CLANG64" ]; then
  echo "Building with CLANG64"
else
  echo "This script is not set up to handle systems of type ${MSYSTEM}, only"
  echo "CLANG64 is currently supported. Please rerun this in a bash terminal of"
  echo "that type."
  exit 2
fi

# We use this internally - but it is actually the same as ${MINGW_PREFIX}
# Win builds use CLANG64 now, but Mac and Linux still use this define
export MINGW_BASE_DIR=${MSYSTEM_PREFIX}

#
# FIXME: don't add duplicates but rearrange instead to put them in the "right" order:
#
export PATH="${MINGW_BASE_DIR}/usr/local/bin:${MINGW_BASE_DIR}/bin:/usr/bin:${PATH}"
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
    "${MINGW_PACKAGE_PREFIX}-qt6-base" \
    "${MINGW_PACKAGE_PREFIX}-qt6-multimedia" \
    "${MINGW_PACKAGE_PREFIX}-qt6-multimedia-wmf" \
    "${MINGW_PACKAGE_PREFIX}-qt6-multimedia-ffmpeg" \
    "${MINGW_PACKAGE_PREFIX}-qt6-svg" \
    "${MINGW_PACKAGE_PREFIX}-qt6-speech" \
    "${MINGW_PACKAGE_PREFIX}-qt6-imageformats" \
    "${MINGW_PACKAGE_PREFIX}-qt6-translations" \
    "${MINGW_PACKAGE_PREFIX}-qt6-tools" \
    "${MINGW_PACKAGE_PREFIX}-qt6-5compat" \
    "${MINGW_PACKAGE_PREFIX}-qt6-websockets" \
    "${MINGW_PACKAGE_PREFIX}-qt6-httpserver" \
    "${MINGW_PACKAGE_PREFIX}-angleproject" \
    "${MINGW_PACKAGE_PREFIX}-qtkeychain-qt6" \
    git \
    man \
    rsync \
    "${MINGW_PACKAGE_PREFIX}-ccache" \
    "${MINGW_PACKAGE_PREFIX}-toolchain" \
    "${MINGW_PACKAGE_PREFIX}-pcre2" \
    "${MINGW_PACKAGE_PREFIX}-libzip" \
    "${MINGW_PACKAGE_PREFIX}-ntldd" \
    "${MINGW_PACKAGE_PREFIX}-pugixml" \
    "${MINGW_PACKAGE_PREFIX}-lua51" \
    "${MINGW_PACKAGE_PREFIX}-lua51-lpeg" \
    "${MINGW_PACKAGE_PREFIX}-lua51-lsqlite3" \
    "${MINGW_PACKAGE_PREFIX}-hunspell" \
    "${MINGW_PACKAGE_PREFIX}-zlib" \
    "${MINGW_PACKAGE_PREFIX}-boost" \
    "${MINGW_PACKAGE_PREFIX}-yajl" \
    "${MINGW_PACKAGE_PREFIX}-oniguruma" \
    "${MINGW_PACKAGE_PREFIX}-lua-luarocks" \
    "${MINGW_PACKAGE_PREFIX}-ffmpeg" \
    "${MINGW_PACKAGE_PREFIX}-cmake" \
    "${MINGW_PACKAGE_PREFIX}-meson" \
    "${MINGW_PACKAGE_PREFIX}-ninja" \
    "${MINGW_PACKAGE_PREFIX}-assimp" \
    "${MINGW_PACKAGE_PREFIX}-curl" \
    "${MINGW_PACKAGE_PREFIX}-python" \
    "${MINGW_PACKAGE_PREFIX}-uasm" \
    "${MINGW_PACKAGE_PREFIX}-cmake" \
    "${MINGW_PACKAGE_PREFIX}-jq"; then
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

echo ""
echo "    Completed"
echo ""

if [ "${SENTRY_SEND_DEBUG}" = "1" ]; then
  echo "=== Installing Qt debug packages for Sentry ==="
  pacman_attempts=0
  while true; do
    if /usr/bin/pacman -S --needed --noconfirm \
      "${MINGW_PACKAGE_PREFIX}-qt6-base-debug" \
      "${MINGW_PACKAGE_PREFIX}-qt6-multimedia-debug" \
      "${MINGW_PACKAGE_PREFIX}-qt6-svg-debug" \
      "${MINGW_PACKAGE_PREFIX}-qt6-speech-debug" \
      "${MINGW_PACKAGE_PREFIX}-qt6-imageformats-debug" \
      "${MINGW_PACKAGE_PREFIX}-qt6-tools-debug" \
      "${MINGW_PACKAGE_PREFIX}-qt6-5compat-debug" \
      "${MINGW_PACKAGE_PREFIX}-qt6-websockets-debug" \
      "${MINGW_PACKAGE_PREFIX}-qt6-httpserver-debug"; then
        break
    fi

    pacman_attempts=$((pacman_attempts +1))

    if [ ${pacman_attempts} -lt 10 ]; then
      echo "=== Some Qt debug packages failed to install, waiting and trying again ==="
      sleep 10
    else
      echo "=== Some Qt debug packages failed to install after ${pacman_attempts} attempts, giving up ==="
      exit 7
    fi
  done
  echo ""
  echo "    Qt debug packages installed"
  echo ""
fi

# MSYS2 publishes a Qt release one module at a time, and Qt tags its private ABI
# with the exact version (QObjectPrivate(QtPrivate_6_12_0)), so a module still
# built against the previous qt6-base fails to load against the new one with
# 0xc0000139. Until the stragglers land, put every Qt module on the version
# they all have.
QT_PREFIX="${MINGW_PACKAGE_PREFIX}-qt6-"
QT_VERSIONS=$(/usr/bin/pacman -Q | awk -v p="${QT_PREFIX}" 'index($1, p) == 1 { sub(/-[0-9]+$/, "", $2); print $2 }' | sort -uV)
if [ "$(echo "${QT_VERSIONS}" | wc -l)" -gt 1 ]; then
  QT_OLDEST=$(echo "${QT_VERSIONS}" | head -n 1)
  echo "=== Qt modules are split across versions $(echo "${QT_VERSIONS}" | tr '\n' ' ')- moving them all to ${QT_OLDEST} ==="
  REPO_URL="https://repo.msys2.org/mingw/${MSYSTEM,,}"
  REPO_INDEX=$(curl -fsS --retry 5 "${REPO_URL}/") || { echo "=== Could not list ${REPO_URL} ==="; exit 7; }
  QT_DOWNGRADES=()
  while read -r pkg version; do
    [ "${version%-*}" = "${QT_OLDEST}" ] && continue
    file=$(echo "${REPO_INDEX}" | grep -oE "\"${pkg}-${QT_OLDEST}-[0-9]+-any\.pkg\.tar\.zst\"" | tr -d '"' | sort -V | tail -n 1)
    if [ -z "${file}" ]; then
      echo "=== ${pkg} ${QT_OLDEST} is no longer in ${REPO_URL} ==="
      exit 7
    fi
    QT_DOWNGRADES+=("${REPO_URL}/${file}")
  done < <(/usr/bin/pacman -Q | awk -v p="${QT_PREFIX}" 'index($1, p) == 1')
  /usr/bin/pacman -U --noconfirm "${QT_DOWNGRADES[@]}" || exit 7
  echo ""
fi

if [ "$(grep -c "/.luarocks-${MSYSTEM}" ${MINGW_BASE_DIR}/etc/luarocks/config-5.1.lua)" -eq 0 ]; then
  # The luarocks config file has not been tweaked to put the compiled rocks in
  # a location that is different for each different MSYS2 environment
  echo "  Tweaking location for constructed Luarocks so 32 and 64 bits ones do"
  echo "  not end up in the same place when --tree \"user\" is used..."
  echo ""

  cp "${MINGW_BASE_DIR}/etc/luarocks/config-5.1.lua" "${MINGW_BASE_DIR}/etc/luarocks/config-5.1.lua.orig"
  cp "${MINGW_BASE_DIR}/etc/luarocks/config-5.4.lua" "${MINGW_BASE_DIR}/etc/luarocks/config-5.4.lua.orig"
  /usr/bin/sed "s|.. \"/.luarocks\"|.. \"/.luarocks-${MSYSTEM}\"|" "${MINGW_BASE_DIR}/etc/luarocks/config-5.1.lua.orig" > "${MINGW_BASE_DIR}/etc/luarocks/config-5.1.lua"
  /usr/bin/sed "s|.. \"/.luarocks\"|.. \"/.luarocks-${MSYSTEM}\"|" "${MINGW_BASE_DIR}/etc/luarocks/config-5.4.lua.orig" > "${MINGW_BASE_DIR}/etc/luarocks/config-5.4.lua"
  echo "    Completed"
else
  echo "  Things have already been setup for Luarocks so 32 and 64 bits ones"
  echo "  do not end up in the same place"
fi
echo "For per-user Lua modules from LuaRocks:"
echo "- Use '--tree \"user\"' (literally) instead of '--local'"
echo "- Adjust LUA_PATH and LUA_CPATH to find per-user modules"
echo "- See 'luarocks path --help' for details"


ROCKCOMMAND="${MINGW_BASE_DIR}/bin/luarocks --lua-version 5.1"
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
