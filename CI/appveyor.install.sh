#!/bin/sh

echo "Running appveyor.build.sh shell script..."

if [ ${BUILD_BITNESS} != "32" -a ${BUILD_BITNESS} != "64" ] ; then
    echo "Requires environmental variable BUILD_BITNESS to exist and be set to \"32\" or \"64\" to specify bitness of target to be built."
    exit -1
fi

echo "Initial PATH is:"
echo ${PATH}
if [ ${BUILD_BITNESS} == "32" ] ; then
    echo "Fixing it for 32-bit builds:"
    export PATH=/mingw32/bin:/usr/bin:${PATH}
else
    echo "Fixing it for 64-bit builds:"
    export PATH=/mingw64/bin:/usr/bin:${PATH}
fi
echo "It is now:"
echo ${PATH}
echo " "
echo "Initial MSYSTEM is: ${MSYSTEM}"
if [ ${BUILD_BITNESS} == "32" ] ; then
    echo "Fixing it for 32-bit builds:"
    export MSYSTEM=MINGW32
else
    echo "Fixing it for 64-bit builds:"
    export MSYSTEM=MINGW64
fi
echo "It is now: ${MSYSTEM}"

# Options:
# --Sy = Sync, refresh as well as installing the specified packages
# --noconfirm = do not ask for user intervention
# --noprogressbar = do not show progress bars as they are not useful in scripts

echo " "
echo "Updating MSYS2 packages..."
if [ ${BUILD_BITNESS} == "32" ] ; then
    pacman -S --needed --noconfirm base-devel git mercurial cvs wget ruby zip p7zip python2 mingw-w64-i686-toolchain mingw-w64-i686-qt5 mingw-w64-i686-libzip mingw-w64-i686-pugixml mingw-w64-i686-lua51 mingw-w64-i686-lua51-lpeg mingw-w64-i686-lua51-lsqlite3 mingw-w64-i686-lua51-luarocks mingw-w64-i686-hunspell mingw-w64-i686-zlib mingw-w64-i686-boost
    if [ ! -f /mingw32/include/lua5.1/lua.h ] ; then
        echo "Lua system failure, failed to install needed /mingw32/include/lua5.1/lua.h file!"
        exit -2
    fi
    echo " "
    echo "Installing needed luarocks..."
    echo "  Configuration files are (system): $(luarocks config --system-config)"
    echo "  containing:"
    /bin/cat $(/mingw32/bin/luarocks config --system-config)
    echo "  and (user): $(/mingw32/bin/luarocks config --user-config)"
    echo "  containing:"
    /bin/cat $(/mingw32/bin/luarocks config --user-config)

    # For some reason we cannot write into the location for the system tree so
    # we have to use the local (user) one - remember this when we need to pull
    # the modules into the final package (we have to get them from a different
    # place):
    # Temporarily do each one individually to see which is causing problems
    echo " "
    echo "    luafilesystem"
    /mingw32/bin/luarocks --local install luafilesystem
    echo "    lua-yajl"
    /mingw32/bin/luarocks --local install lua-yajl
    echo "    luautf8"
    /mingw32/bin/luarocks --local install luautf8
    echo "    luazip"
    /mingw32/bin/luarocks --local install luazip
    echo "    lrexlib-pcre"
    /mingw32/bin/luarocks --local install lrexlib-pcre
    echo "    luasql-sqlite3"
    /mingw32/bin/luarocks --local install luasql-sqlite3

else
    pacman -S --needed --noconfirm base-devel git mercurial cvs wget ruby zip p7zip python2 mingw-w64-x86_64-toolchain mingw-w64-x86_64-qt5 mingw-w64-x86_64-libzip mingw-w64-x86_64-pugixml mingw-w64-x86_64-lua51 mingw-w64-x86_64-lua51-lpeg mingw-w64-x86_64-lua51-lsqlite3 mingw-w64-x86_64-lua51-luarocks mingw-w64-x86_64-hunspell mingw-w64-x86_64-zlib mingw-w64-x86_64-boost
    if [ ! -f /mingw64/include/lua5.1/lua.h ] ; then
        echo "Lua system failure, failed to install needed /mingw64/include/lua5.1/lua.h file!"
        exit -2
    fi
    echo " "
    echo "Installing needed luarocks..."
    echo "  Configuration files are (system): $(luarocks config --system-config)"
    echo "  containing:"
    /bin/cat $(/mingw64/bin/luarocks config --system-config)
    echo "  and (user): $(/mingw64/bin/luarocks config --user-config)"
    echo "  containing:"
    /bin/cat $(/mingw64/bin/luarocks config --user-config)

    # For some reason we cannot write into the location for the system tree so
    # we have to use the local (user) one - remember this when we need to pull
    # the modules into the final package (we have to get them from a different
    # place):
    # Temporarily do each one individually to see which is causing problems
    echo " "
    echo "    luafilesystem"
    /mingw64/bin/luarocks --local install luafilesystem
    echo "    lua-yajl"
    /mingw64/bin/luarocks --local install lua-yajl
    echo "    luautf8"
    /mingw64/bin/luarocks --local install luautf8
    echo "    luazip"
    /mingw64/bin/luarocks --local install luazip
    echo "    lrexlib-pcre"
    /mingw64/bin/luarocks --local install lrexlib-pcre
    echo "    luasql-sqlite3"
    /mingw64/bin/luarocks --local install luasql-sqlite3

fi

echo "    ... all done"
