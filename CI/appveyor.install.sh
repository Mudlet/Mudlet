#!/bin/sh

if [[ ${BUILD_BITNESS} != "32" -a ${BUILD_BITNESS} != "64" ]]; then
    echo "Requires environmental variable BUILD_BITNESS to exist and be set to \"32\" or \"64\" to specify bitness of target to be built."
    exit -1
fi

echo "Predefined PATH is:"
echo ${PATH}

# Put stuff here to clean up path

# Options:
# --Syu = Sync, refresh and do a sysupgrade as well as installing the specified packages
# --noconfirm = do not ask for user intervention
# --noprogressbar = do not show progress bars as they are not useful in scripts

if [[ ${BUILD_BITNESS} == "32"]]; then
    pacman -Syuu --noconfirm --noprogressbar base-devel git mercurial cvs wget ruby zip p7zip python2 mingw-w64-i686-toolchain mingw-w64-i686-qt5 mingw-w64-i686-libzip mingw-w64-i686-pugixml mingw-w64-i686-lua51 mingw-w64-i686-lua51-lpeg mingw-w64-i686-lua51-lsqlite3 mingw-w64-i686-lua51-luarocks mingw-w64-i686-hunspell mingw-w64-i686-zlib mingw-w64-i686-boost
else
    pacman -Syuu --noconfirm --noprogressbar base-devel git mercurial cvs wget ruby zip p7zip python2 mingw-w64-x86_64-toolchain mingw-w64-x86_64-qt5 mingw-w64-x86_64-libzip mingw-w64-x86_64-pugixml mingw-w64-x86_64-lua51 mingw-w64-x86_64-lua51-lpeg mingw-x86_64-i686-lua51-lsqlite3 mingw-x86_64-i686-lua51-luarocks mingw-w64-x86_64-hunspell mingw-w64-x86_64-zlib mingw-x86_64-i686-boost
fi

luarocks install luafilesystem lua-yajl luautf8 luazip lrexlib-pcre luasql-sqlite3
