#!/bin/sh

echo "Running appveyor.build.sh shell script..."

if [ ${BUILD_BITNESS} != "32" -a ${BUILD_BITNESS} != "64" ] ; then
    echo "Requires environmental variable BUILD_BITNESS to exist and be set to \"32\" or \"64\" to specify bitness of target to be built."
    exit -1
fi

echo "Initial MSYSTEM is: ${MSYSTEM}"
echo "Initial PATH is:"
echo ${PATH}
echo "Fixing things for ${BUILD_BITNESS}-bit builds:"
export MSYSTEM=MINGW${BUILD_BITNESS}
# MINGW_BASE_DIR Previously used in external (power shell) scripts
export MINGW_BASE_DIR=C:/msys64/mingw${BUILD_BITNESS}
# For use within sh scripts as it does not include a DRIVE element
export MINGW_INTERNAL_BASE_DIR=/mingw${BUILD_BITNESS}
export PATH=${MINGW_INTERNAL_BASE_DIR}/bin:/usr/bin:${PATH}
echo "It is now:"
echo ${PATH}
echo " "
echo "MSYSTEM is now: ${MSYSTEM}"

# Options:
# --Sy = Sync, refresh as well as installing the specified packages
# --noconfirm = do not ask for user intervention
# --noprogressbar = do not show progress bars as they are not useful in scripts

echo " "
echo "Updating MSYS2 packages..."

# Clear this to use system luarocks
ROCKOPTARGS=--local
if [ ${BUILD_BITNESS} == "32" ] ; then
    BUILDCOMPONENT="i686"
else
    BUILDCOMPONENT="x86_64"
fi

ROCKCOMMAND=${MINGW_INTERNAL_BASE_DIR}/bin/luarocks

/usr/bin/pacman -S --needed --noconfirm base-devel coreutils msys2-runtime git mercurial cvs wget ruby zip p7zip python2 mingw-w64-${BUILDCOMPONENT}-toolchain mingw-w64-i686-qt5 mingw-w64-${BUILDCOMPONENT}-libzip mingw-w64-${BUILDCOMPONENT}-pugixml mingw-w64-${BUILDCOMPONENT}-lua51 mingw-w64-${BUILDCOMPONENT}-lua51-lpeg mingw-w64-${BUILDCOMPONENT}-lua51-lsqlite3 mingw-w64-${BUILDCOMPONENT}-lua51-luarocks mingw-w64-${BUILDCOMPONENT}-hunspell mingw-w64-${BUILDCOMPONENT}-zlib mingw-w64-${BUILDCOMPONENT}-boost

# FIX THINGS HERE: This test does seem to pass but the luarocks build/installs do not seem to see the header file?
if [ ! -f ${MINGW_INTERNAL_BASE_DIR}/include/lua5.1/lua.h ] ; then
    echo "Lua system failure, failed to install needed ${MINGW_INTERNAL_BASE_DIR}/include/lua5.1/lua.h file!"
    exit -2
fi

echo " "
echo "Installing needed luarocks..."

echo "  Configuration files are (system): $(${ROCKCOMMAND} config --system-config) echo and (user): $(${ROCKCOMMAND} config --user-config)"
echo "  The system one contains:"
/usr/bin/cat $(${ROCKCOMMAND} config --system-config)

# For some reason we cannot write into the location for the system tree so
# we have to use the local (user) one - remember this when we need to pull
# the modules into the final package (we have to get them from a different
# place):
# Temporarily do each one individually to see which is causing problems
echo " "
echo "    luafilesystem"
${ROCKCOMMAND} ${ROCKOPTARGS} install luafilesystem
echo " "
echo "    lua-yajl"
${ROCKCOMMAND} ${ROCKOPTARGS} install lua-yajl
echo " "
echo "    luautf8"
${ROCKCOMMAND} ${ROCKOPTARGS} install luautf8
echo " "
echo "    luazip"
${ROCKCOMMAND} ${ROCKOPTARGS} install luazip
echo " "
echo "    lrexlib-pcre"
${ROCKCOMMAND} ${ROCKOPTARGS} install lrexlib-pcre
echo " "
echo "    luasql-sqlite3"
${ROCKCOMMAND} ${ROCKOPTARGS} install luasql-sqlite3
echo " "
echo "    ... all done"
