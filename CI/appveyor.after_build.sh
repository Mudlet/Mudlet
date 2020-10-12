#!/bin/sh

# This assembles all the files we need into a package sub-directory

echo "Running appveyor.after_build.sh shell script..."
echo ""

if [ "${APPVEYOR_REPO_NAME}" != "Mudlet/Mudlet" ]; then
    # Only run this code on the main Mudlet Github repository - do nothing otherwise:
    echo "This does not appear to be running on the main Mudlet repository, packaging is not appropriate....!"
    echo ""
    exit 0
fi

# Source/setup some variables (including PATH):
. $(/usr/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER}/CI/appveyor.set-build-info.sh)

echo "Moving to packaging directory: $(/usr/bin/cygpath --windows ${APPVEYOR_BUILD_FOLDER}/package)"
cd $(/usr/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER}/package)
echo "  it contains:"
ls -l
echo ""

# Since Qt 5.14 using the --release switch is broken (it now seems to be
# assumed), --debug still seems to work.
# https://bugreports.qt.io/browse/QTBUG-80806 seems relevant.
echo "Running windeployqt..."
${MINGW_INTERNAL_BASE_DIR}/bin/windeployqt --no-virtualkeyboard mudlet.exe
echo ""

# To determine which system libraries have to be copied in it requires
# continually trying to run the executable on the target type system
# and adding in the libraries to the same directory and repeating that
# until the executable actually starts to run. Alternatively running
# ldd ./mudlet.exe | grep "/mingw32" {for the 32 bit case, use "64" for
# the other one} inside an Mingw32 (or 64) shell as appropriate will
# produce the libraries that are likely to be needed below. Unfortunetly
# this process is a little recursive in that you may have to repeat the
# process for individual librarys. For ones used by lua modules this
# can manifest as being unable to "require" the library within lua
# and doing the above "ldd" check revealed that "zip.dll" needed
# "libzzip-0-13.dll" and "luasql/sqlite3.dll" needed "libsqlite3-0.dll"!
#
echo "Examining Mudlet application to identify other needed libraries..."
NEEDED_LIBS=$(${MINGW_INTERNAL_BASE_DIR}/bin/ntldd --recursive ./mudlet.exe \
  | /usr/bin/grep -v "Qt5" \
  | /usr/bin/grep -i "mingw" \
  | /usr/bin/cut -d ">" -f2 \
  | /usr/bin/cut -d "(" -f1 \
  | /usr/bin/sort)
echo ""
echo "Copying these libraries..."
for LIB in ${NEEDED_LIBS} ; do
  cp -v ${LIB} . ;
done

echo ""
echo "Copying other known to be needed libraries in..."
# libjasper-4 to libwebpdemux-2 are additional image format handlers that Qt can
# use if they are present.
# libsqlite3 and libyajl are needed by lua modules at Mudlet run time.
# SDL2 helps with Gamepad support that QtGamepad can use if it is present.
cp -v -p -t . \
    ${MINGW_INTERNAL_BASE_DIR}/bin/libjasper-4.dll \
    ${MINGW_INTERNAL_BASE_DIR}/bin/libjpeg-8.dll \
    ${MINGW_INTERNAL_BASE_DIR}/bin/libtiff-5.dll \
    ${MINGW_INTERNAL_BASE_DIR}/bin/libwebp-7.dll \
    ${MINGW_INTERNAL_BASE_DIR}/bin/libwebpdemux-2.dll \
    ${MINGW_INTERNAL_BASE_DIR}/bin/libsqlite3-0.dll \
    ${MINGW_INTERNAL_BASE_DIR}/bin/libyajl.dll \
    ${MINGW_INTERNAL_BASE_DIR}/bin/SDL2.dll

echo ""
echo "Copying OpenSSL libraries in..."
# The openSSL libraries has a different name depending on the bitness:
if [ "${BUILD_BITNESS}" = "32" ]; then
    cp -v -p -t . \
        ${MINGW_INTERNAL_BASE_DIR}/bin/libcrypto-1_1.dll \
        ${MINGW_INTERNAL_BASE_DIR}/bin/libssl-1_1.dll

else
    cp -v -p -t . \
        ${MINGW_INTERNAL_BASE_DIR}/bin/libcrypto-1_1-x64.dll \
        ${MINGW_INTERNAL_BASE_DIR}/bin/libssl-1_1-x64.dll

fi

echo ""
echo "Copying discord-rpc library in..."
if [ "${BUILD_BITNESS}" = "32" ]; then
    cp -v -p $(/usr/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER}/3rdparty/discord/rpc/lib/discord-rpc32.dll)  .
else
    cp -v -p $(/usr/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER}/3rdparty/discord/rpc/lib/discord-rpc64.dll)  .
fi
echo ""

# Lua libraries:
# If there is a demand for other rocks in the Windows installer because of
# revisions to the mappers or geyser framework or popular demand otherwise then
# the rock for those will also have to be installed and their C(.dll)/Lua (.lua)
# files included here:
echo "Copying lua C libraries in..."
cp -v -p -t . \
    ${MINGW_INTERNAL_BASE_DIR}/lib/lua/5.1/lfs.dll \
    ${MINGW_INTERNAL_BASE_DIR}/lib/lua/5.1/lpeg.dll \
    ${MINGW_INTERNAL_BASE_DIR}/lib/lua/5.1/lsqlite3.dll \
    ${MINGW_INTERNAL_BASE_DIR}/lib/lua/5.1/lua-utf8.dll \
    ${MINGW_INTERNAL_BASE_DIR}/lib/lua/5.1/rex_pcre.dll \
    ${MINGW_INTERNAL_BASE_DIR}/lib/lua/5.1/yajl.dll

mkdir ./luasql
cp -v -p ${MINGW_INTERNAL_BASE_DIR}/lib/lua/5.1/luasql/sqlite3.dll ./luasql/sqlite3.dll
mkdir ./brimworks
cp -v -p ${MINGW_INTERNAL_BASE_DIR}/lib/lua/5.1/brimworks/zip.dll ./brimworks/zip.dll
echo ""

echo "Copying Mudlet & Geyser Lua files and the Generic Mapper in..."
# Using the '/./' notation provides the point at which rsync reproduces the
# directory structure from the source into the target and avoids the need
# to change directory before and after the rsync call:

# As written it copies every file but it should be polished up to skip unneeded
# ones:
rsync -avR $(/usr/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER})/src/mudlet-lua/./* $(/usr/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER}/package/mudlet-lua/)
echo ""

echo "Copying Lua code formatter Lua files in..."
# As written it copies every file but it should be polished up to skip unneeded
# ones:
rsync -avR $(/usr/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER})/3rdparty/lcf/./* $(/usr/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER}/package/lcf/)
echo ""

echo "Copying Lua translation files in..."
mkdir -p $(/usr/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER}/package/translations/lua/translated/)
cp -v -p -t $(/usr/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER}/package/translations/lua/translated/) \
    $(/usr/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER})/translations/lua/translated/mudlet-lua_??_??.json
echo ""

echo "Copying Hunspell dictionaries in..."
cp -v -p -t . \
  $(/usr/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER}/src/*.aff) \
  $(/usr/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER}/src/*.dic)

echo ""

# For debugging purposes:
# echo "The recursive contents of the Project build sub-directory $(/usr/bin/cygpath --windows ${APPVEYOR_BUILD_FOLDER}/build/package):"
# /usr/bin/ls -aRl
# echo ""

echo "   ... appveyor.after_build.sh shell script finished."
echo "${APPVEYOR_BUILD_FOLDER}\package should contain everything needed to run Mudlet!"
echo ""
