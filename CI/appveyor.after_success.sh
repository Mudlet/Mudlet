#!/bin/sh

echo "Running appveyor.after_success.sh shell script..."

if [ ${APPVEYOR_REPO_NAME} != "Mudlet/Mudlet" ] ; then
    # Only run this code on the main Mudlet Github repository - do nothing otherwise:
    echo "This does not appear to be running on the main Mudlet repository, packaging is not appropriate....!"
    echo " "
    exit 0
fi

if [ ${BUILD_BITNESS} != "32" ] && [ ${BUILD_BITNESS} != "64" ] ; then
    echo "Requires environmental variable BUILD_BITNESS to exist and be set to \"32\" or \"64\" to specify bitness of target to be built."
    exit 1
fi

# Commented out things only needed for failure post-mortems:
# echo "Initial MSYSTEM is: ${MSYSTEM}"
# echo "Initial PATH is:"
# echo ${PATH}
echo " "
echo "Fixing things for ${BUILD_BITNESS}-bit builds:"
export MSYSTEM=MINGW${BUILD_BITNESS}
export MINGW_BASE_DIR=C:/msys64/mingw${BUILD_BITNESS}
export MINGW_INTERNAL_BASE_DIR=/mingw${BUILD_BITNESS}
export PATH=${MINGW_INTERNAL_BASE_DIR}/bin:/usr/bin:${PATH}
# echo " "
# echo "PATH is now:"
# echo ${PATH}
# echo " "
# echo "MSYSTEM is now: ${MSYSTEM}"

echo "Moving to packaging directory: $(/usr/bin/cygpath --windows ${APPVEYOR_BUILD_FOLDER}/package)"
cd $(/usr/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER}/package)
echo "  it contains:"
ls -l
echo " "

echo "Running windeployqt..."
${MINGW_INTERNAL_BASE_DIR}/bin/windeployqt --release mudlet.exe
echo " "

echo "Copying system libraries in..."
if [ ${BUILD_BITNESS} = "32" ] ; then
    cp -v -p ${MINGW_INTERNAL_BASE_DIR}/bin/libgcc_s_dw2-1.dll .
else
    cp -v -p ${MINGW_INTERNAL_BASE_DIR}/bin/libgcc_s_seh-1.dll .
fi

cp -v -p -t . \
    ${MINGW_INTERNAL_BASE_DIR}/bin/libhunspell-1.7-0.dll \
    ${MINGW_INTERNAL_BASE_DIR}/bin/libpcre-1.dll \
    ${MINGW_INTERNAL_BASE_DIR}/bin/libwinpthread-1.dll \
    ${MINGW_INTERNAL_BASE_DIR}/bin/libyajl.dll \
    ${MINGW_INTERNAL_BASE_DIR}/bin/libzip.dll \
    ${MINGW_INTERNAL_BASE_DIR}/bin/zlib1.dll
echo " "

echo "Copying discord-rpc library in..."
if [ ${BUILD_BITNESS} = "32" ] ; then
    cp -v -p $(cygpath --unix ${APPVEYOR_BUILD_FOLDER}/3rdparty/discord/rpc/lib/discord-rpc32.dll)  .
else
    cp -v -p $(cygpath --unix ${APPVEYOR_BUILD_FOLDER}/3rdparty/discord/rpc/lib/discord-rpc64.dll)  .
fi
echo " "

# Lua libraries:
echo "Copying lua C libraries in..."
cp -v -p -t . \
    ${MINGW_INTERNAL_BASE_DIR}/bin/lua51.dll \
    ${MINGW_INTERNAL_BASE_DIR}/lib/lua/5.1/lfs.dll \
    ${MINGW_INTERNAL_BASE_DIR}/lib/lua/5.1/lpeg.dll \
    ${MINGW_INTERNAL_BASE_DIR}/lib/lua/5.1/lsqlite3.dll \
    ${MINGW_INTERNAL_BASE_DIR}/lib/lua/5.1/lua-utf8.dll \
    ${MINGW_INTERNAL_BASE_DIR}/lib/lua/5.1/rex_pcre.dll \
    ${MINGW_INTERNAL_BASE_DIR}/lib/lua/5.1/yajl.dll \
    ${MINGW_INTERNAL_BASE_DIR}/lib/lua/5.1/zip.dll

mkdir ./luasql
cp -v -p ${MINGW_INTERNAL_BASE_DIR}/lib/lua/5.1/luasql/sqlite3.dll ./luasql/sqlite3.dll
echo " "

echo "Copying Mudlet & Geyser Lua files and the Generic Mapper in..."
# Using the '/./' notation provides the point at which rsync reproduces the
# directory structure from the source into the target and avoids the need
# to change directory before and after the rsync call:

# As written it copies every file but it should be polished up to skip unneeded
# ones:
rsync -avR $(/usr/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER})/./src/mudlet-lua/* $(/usr/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER}/package/mudlet-lua/)
echo " "

echo "Copying Lua code formatter Lua files in..."
# As written it copies every file but it should be polished up to skip unneeded
# ones:
rsync -avR $(/usr/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER})/3rdparty/./lcf/* $(/usr/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER}/package/lcf/)
echo " "

echo "Copying Hunspell dictionaries in..."
cp -v -p -t . \
  $(cygpath --unix ${APPVEYOR_BUILD_FOLDER}/src/de_AT_frami.aff) \
  $(cygpath --unix ${APPVEYOR_BUILD_FOLDER}/src/de_AT_frami.dic) \
  $(cygpath --unix ${APPVEYOR_BUILD_FOLDER}/src/de_CH_frami.aff) \
  $(cygpath --unix ${APPVEYOR_BUILD_FOLDER}/src/de_CH_frami.dic) \
  $(cygpath --unix ${APPVEYOR_BUILD_FOLDER}/src/de_DE_frami.aff) \
  $(cygpath --unix ${APPVEYOR_BUILD_FOLDER}/src/de_DE_frami.dic) \
  $(cygpath --unix ${APPVEYOR_BUILD_FOLDER}/src/el_GR.aff) \
  $(cygpath --unix ${APPVEYOR_BUILD_FOLDER}/src/el_GR.dic) \
  $(cygpath --unix ${APPVEYOR_BUILD_FOLDER}/src/en_GB.aff) \
  $(cygpath --unix ${APPVEYOR_BUILD_FOLDER}/src/en_GB.dic) \
  $(cygpath --unix ${APPVEYOR_BUILD_FOLDER}/src/en_US.aff) \
  $(cygpath --unix ${APPVEYOR_BUILD_FOLDER}/src/en_US.dic) \
  $(cygpath --unix ${APPVEYOR_BUILD_FOLDER}/src/es_ES.aff) \
  $(cygpath --unix ${APPVEYOR_BUILD_FOLDER}/src/es_ES.dic) \
  $(cygpath --unix ${APPVEYOR_BUILD_FOLDER}/src/fr.aff) \
  $(cygpath --unix ${APPVEYOR_BUILD_FOLDER}/src/fr.dic) \
  $(cygpath --unix ${APPVEYOR_BUILD_FOLDER}/src/it_IT.aff) \
  $(cygpath --unix ${APPVEYOR_BUILD_FOLDER}/src/it_IT.dic) \
  $(cygpath --unix ${APPVEYOR_BUILD_FOLDER}/src/nl_NL.aff) \
  $(cygpath --unix ${APPVEYOR_BUILD_FOLDER}/src/nl_NL.dic) \
  $(cygpath --unix ${APPVEYOR_BUILD_FOLDER}/src/pl_PL.aff) \
  $(cygpath --unix ${APPVEYOR_BUILD_FOLDER}/src/pl_PL.dic) \
  $(cygpath --unix ${APPVEYOR_BUILD_FOLDER}/src/pt_BR.aff) \
  $(cygpath --unix ${APPVEYOR_BUILD_FOLDER}/src/pt_BR.dic) \
  $(cygpath --unix ${APPVEYOR_BUILD_FOLDER}/src/pt_PT.aff) \
  $(cygpath --unix ${APPVEYOR_BUILD_FOLDER}/src/pt_PT.dic) \
  $(cygpath --unix ${APPVEYOR_BUILD_FOLDER}/src/ru_RU.aff) \
  $(cygpath --unix ${APPVEYOR_BUILD_FOLDER}/src/ru_RU.dic)

echo " "

echo "Compressing all files into an archive file for distribution..."
/usr/bin/zip -rv9 mudlet *
echo " "

echo "The recursive contents of the Project build sub-directory: $(/usr/bin/cygpath --windows ${APPVEYOR_BUILD_FOLDER}/build/package):"
/usr/bin/ls -al $(/usr/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER}/build/package)
echo " "

echo "   ... appveyor.after_success.sh shell script finished!"
echo " "
