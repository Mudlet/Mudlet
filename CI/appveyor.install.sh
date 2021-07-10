#!/bin/sh

if [ -z "${BUILD_PROCESS}" ]; then
    echo "BUILD_PROCESS not set - it should be \"Original\" or \"Replacement\" and needs to be the second one to run this script!"
    exit 1
fi

if [ ! "${BUILD_PROCESS}" = "Replacement" ]; then
    # Silently skip this script for the Original PowerShell based (32-Bit only) build system
    exit 0
fi

echo "Running appveyor.install.sh shell script..."

# Source/setup some variables (including PATH):
. "/c/projects/mudlet/CI/appveyor.set-build-info.sh"

# The above will define ABORT_PT_BUILDS to be "true" if this is deduced to be
# a "public_test" BUILD_TYPE - AND there has not been any change in the
# development branch in the last 24 hours - so we should abort this build
# as soon as possible.
if [ "${ABORT_PT_BUILDS}" = "true" ]; then
    appveyor AddMessage "INFORMATION: No change in development code in last day, scheduled public test build halted." -Category Information
    # Forcible terminate build (successfully) - but will still carry out
    # on_success and on_finish steps in yaml file:
    appveyor exit
    exit 0
fi

if [ "${BUILD_BITNESS}" != "32" ] && [ "${BUILD_BITNESS}" != "64" ]; then
    echo "Requires environmental variable BUILD_BITNESS to exist and be set to \"32\" or \"64\" to specify bitness of target to be built."
    appveyor AddMessage "ERROR: Environmental variable BUILD_BITNESS does not exist or not set to \"32\" or \"64\"! Build aborted."
    exit 1
fi

appveyor AddMessage "This is a \"${BUILD_TYPE}\" ${BUILD_BITNESS} bit Mudlet ${VERSION}${MUDLET_VERSION_BUILD} build ..." -Category Information

# Options:
# --Sy = Sync, refresh as well as installing the specified packages
# --noconfirm = do not ask for user intervention
# --noprogressbar = do not show progress bars as they are not useful in scripts

echo "Updating MSYS2 packages..."

# Uncomment this to use user luarocks
# ROCKOPTARGS=--local
if [ "${BUILD_BITNESS}" = "32" ]; then
    BUILDCOMPONENT="i686"
else
    BUILDCOMPONENT="x86_64"
fi

ROCKCOMMAND="${MINGW_INTERNAL_BASE_DIR}/bin/luarocks"

/usr/bin/pacman -S --needed --noconfirm \
    base-devel \
    coreutils \
    msys2-runtime \
    git \
    mercurial \
    cvs \
    wget \
    ruby \
    zip \
    p7zip \
    python \
    rsync \
    openssh \
    mingw-w64-${BUILDCOMPONENT}-toolchain \
    mingw-w64-${BUILDCOMPONENT}-ccache \
    mingw-w64-${BUILDCOMPONENT}-qt5 \
    mingw-w64-${BUILDCOMPONENT}-libzip \
    mingw-w64-${BUILDCOMPONENT}-ntldd \
    mingw-w64-${BUILDCOMPONENT}-pugixml \
    mingw-w64-${BUILDCOMPONENT}-lua51 \
    mingw-w64-${BUILDCOMPONENT}-lua51-lpeg \
    mingw-w64-${BUILDCOMPONENT}-lua51-lsqlite3 \
    mingw-w64-${BUILDCOMPONENT}-hunspell \
    mingw-w64-${BUILDCOMPONENT}-zlib \
    mingw-w64-${BUILDCOMPONENT}-boost \
    mingw-w64-${BUILDCOMPONENT}-yajl \
    mingw-w64-${BUILDCOMPONENT}-SDL2

echo "Working around MSYS2/Mingw-w64 issue 9037 with Luarocks..."
# We now need to work around https://github.com/msys2/MINGW-packages/issues/9037
# by downloading the old version of the Luarocks package and manually installing
# it:
/usr/bin/wget https://repo.msys2.org/mingw/mingw64/mingw-w64-${BUILDCOMPONENT}-lua51-luarocks-2.4.4-2-any.pkg.tar.zst
/usr/bin/pacman -U mingw-w64-${BUILDCOMPONENT}-lua51-luarocks-2.4.4-2-any.pkg.tar.zst

# This was to fix https://github.com/msys2/MINGW-packages/issues/5928 but that
# has now been done by https://github.com/msys2/MINGW-packages/pull/6580:
#if [ ${BUILD_BITNESS} = "32" ]; then
#    # The site_config.lua file for the MINGW32 case has so many wrong values
#    # it prevents luarocks from working - however it can be repaired by some
#    # editing:
#    cp /mingw32/share/lua/5.1/luarocks/site_config.lua /mingw32/share/lua/5.1/luarocks/site_config.lua.orig
#    /usr/bin/sed "s|/mingw32|c:/msys64/mingw32|g" /mingw32/share/lua/5.1/luarocks/site_config.lua.orig \
#      | /usr/bin/sed "s|/lib/luarocks/rocks|/lib/luarocks/rocks-5.1|" > /mingw32/share/lua/5.1/luarocks/site_config.lua
#
#    # Also need to change one thing in the config-5.1.lua file:
#    cp /mingw32/etc/luarocks/config-5.1.lua /mingw32/etc/luarocks/config-5.1.lua.orig
#    /usr/bin/sed "s|/mingw32|c:/msys64/mingw32|g" /mingw32/etc/luarocks/config-5.1.lua.orig > /mingw32/etc/luarocks/config-5.1.lua
#fi

echo ""
echo "    .... MSYS2 Package installation completed."
echo ""

echo " Lua configuration files are: (system): $("${ROCKCOMMAND}" config --system-config)"
echo "                            and (user): $("${ROCKCOMMAND}" config --user-config)"
echo ""
echo "   The system one contains:"
/usr/bin/cat "$("${ROCKCOMMAND}" config --system-config)"

echo ""
echo "  Installing needed luarocks..."
echo ""
echo "    luafilesystem"
"${ROCKCOMMAND}" ${ROCKOPTARGS} install luafilesystem
echo ""
echo "    lua-yajl"
"${ROCKCOMMAND}" ${ROCKOPTARGS} install lua-yajl
echo ""
echo "    luautf8"
"${ROCKCOMMAND}" ${ROCKOPTARGS} install luautf8
echo ""
echo "    lua-zip"
"${ROCKCOMMAND}" ${ROCKOPTARGS} install lua-zip
echo ""
echo "    lrexlib-pcre"
"${ROCKCOMMAND}" ${ROCKOPTARGS} install lrexlib-pcre
echo ""
echo "    luasql-sqlite3"
"${ROCKCOMMAND}" ${ROCKOPTARGS} install luasql-sqlite3
echo ""
if [ "${BUILD_TYPE}" = "public_test" ]; then
    # These two are needed for preperation of the PTB changelog
    echo "    lunajson"
    "${ROCKCOMMAND}" ${ROCKOPTARGS} install lunajson
    echo ""
    echo "    argparse"
    "${ROCKCOMMAND}" ${ROCKOPTARGS} install argparse
    echo ""
fi
echo "    ... luarocks installation done"
echo ""
echo "  ... appveyor.install.sh shell script finished!"
echo ""
