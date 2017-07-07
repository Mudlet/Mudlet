git submodule update --init --recursive
SET
cd C:\src
SET PATH=%MINGW_BASE_DIR%\bin;C:\MinGW\msys\1.0\bin;C:\Program Files (x86)\CMake\bin;C:\Program Files\7-Zip;%PATH%

ECHO ==== downloading dependencies ==== >> verbose_output.log 2>&1
wget --no-check-certificate -nv --output-document hunspell-1.4.1.tar.gz https://github.com/hunspell/hunspell/archive/v1.4.1.tar.gz >> verbose_output.log 2>&1
wget -nv http://www.lua.org/ftp/lua-5.1.5.tar.gz >> verbose_output.log 2>&1
wget --no-check-certificate -nv https://sourceforge.net/projects/pcre/files/pcre/8.38/pcre-8.38.tar.gz/download >> verbose_output.log 2>&1
wget -nv http://zlib.net/zlib-1.2.11.tar.gz >> verbose_output.log 2>&1
wget -nv http://www.sqlite.org/2013/sqlite-autoconf-3071700.tar.gz >> verbose_output.log 2>&1
wget -nv --no-check-certificate https://launchpad.net/ubuntu/+archive/primary/+files/libzip_0.11.2.orig.tar.gz >> verbose_output.log 2>&1
wget -nv --no-check-certificate --output-document yajl-2.0.1.tar.gz https://github.com/lloyd/yajl/tarball/2.0.1 >> verbose_output.log 2>&1
wget -nv --no-check-certificate https://sourceforge.net/projects/zziplib/files/zziplib13/0.13.62/zziplib-0.13.62.tar.bz2/download >> verbose_output.log 2>&1
wget -nv --no-check-certificate https://indy.fulgan.com/SSL/openssl-1.0.2l-i386-win32.zip >> verbose_output.log 2>&1
wget -nv --no-check-certificate http://keplerproject.github.io/luarocks/releases/luarocks-2.4.0-win32.zip >> verbose_output.log 2>&1
wget -nv --no-check-certificate --output-document luazip.zip https://github.com/rjpcomputing/luazip/archive/master.zip >> verbose_output.log 2>&1
wget -nv --no-check-certificate --output-document luawinmake.zip https://github.com/Tieske/luawinmake/archive/master.zip >> verbose_output.log 2>&1
wget --no-check-certificate -nv --output-document installbuilder-qt-installer.exe https://installbuilder.bitrock.com/installbuilder-qt-enterprise-17.3.0-windows-installer.exe >> verbose_output.log 2>&1
ECHO ==== finished downloading dependencies ====  >> verbose_output.log 2>&1

ECHO ==== extracting archives ==== >> verbose_output.log 2>&1
bash -c "cd /c/src/ && (for a in `ls -1 *.tar.gz`; do if [ \"$a\" != \"boost_1_60_0.tar.gz\" ]; then  tar -zxf $a || true; fi; done) && (for a in `ls -1 *.tar.bz2`; do tar xfj $a || true; done)"  >> verbose_output.log 2>&1
7z -oopenssl-1.0.2l e openssl-1.0.2l-i386-win32.zip >> verbose_output.log 2>&1
7z x luarocks-2.4.0-win32.zip >> verbose_output.log 2>&1
7z x luazip.zip >> verbose_output.log 2>&1
7z x luawinmake.zip >> verbose_output.log 2>&1
ECHO ==== finished extracting archives ==== >> verbose_output.log 2>&1

cd hunspell-1.4.1
ECHO ==== compiling and installing hunspell ==== >> ..\verbose_output.log 2>&1
ECHO ---- running configure ---- >> ..\verbose_output.log 2>&1
bash -c "./configure --prefix=%MINGW_BASE_DIR_BASH%" >> ..\verbose_output.log 2>&1
ECHO ---- running make ----  >> ..\verbose_output.log 2>&1
make -j 2 >> ..\verbose_output.log 2>&1
ECHO ---- running make install ---- >> ..\verbose_output.log 2>&1
make install >> ..\verbose_output.log 2>&1
ECHO ==== finished handling hunspell ==== >> ..\verbose_output.log 2>&1
cd ..

cd lloyd-yajl-f4b2b1a
ECHO ==== compiling and installing yajl ==== >> ..\verbose_output.log 2>&1
ECHO ---- changing CMakeLists.txt ---- >> ..\verbose_output.log 2>&1
powershell -Command "(Get-Content CMakeLists.txt -Raw) -replace '\/W4' -replace '(?<=SET\(linkFlags)[^\)]+' -replace '\/wd4996 \/wd4255 \/wd4130 \/wd4100 \/wd4711' -replace '(?<=SET\(CMAKE_C_FLAGS_DEBUG .)\/D \DEBUG \/Od \/Z7', '-g' -replace '(?<=SET\(CMAKE_C_FLAGS_RELEASE .)\/D NDEBUG \/O2', '-O2' | Out-File -encoding ASCII CMakeLists.txt" >> ..\verbose_output.log 2>&1
mkdir build
cd build
ECHO ---- running cmake ---- >> ..\..\verbose_output.log 2>&1
cmake -G "MSYS Makefiles" ..  >> ..\..\verbose_output.log 2>&1
ECHO ---- running make ---- >> ..\..\verbose_output.log 2>&1
make -j 2  >> ..\..\verbose_output.log 2>&1
ECHO ---- installing yajl ---- >> ..\..\verbose_output.log 2>&1
COPY yajl-2.0.1\lib\* %MINGW_BASE_DIR%\lib >> ..\..\verbose_output.log 2>&1
XCOPY /S /I /Q yajl-2.0.1\include %MINGW_BASE_DIR%\include >> ..\..\verbose_output.log 2>&1
ECHO ==== finished handling yajl ==== >> ..\..\verbose_output.log 2>&1
cd ..\..

ECHO ==== compiling and installing lua ==== >> verbose_output.log 2>&1
ECHO ---- copying luawinmake files ---- >> verbose_output.log 2>&1
XCOPY /S /I /Q C:\src\luawinmake-master\etc C:\src\lua-5.1.5\etc >> ..\verbose_output.log 2>&1
cd lua-5.1.5
ECHO ---- compiling lua ---- >> ..\verbose_output.log 2>&1
etc\winmake >> ..\verbose_output.log 2>&1
ECHO ---- installing lua ---- >> ..\verbose_output.log 2>&1
etc\winmake install %MINGW_BASE_DIR% >> ..\verbose_output.log 2>&1
ECHO ==== finished handling lua ==== >> ..\verbose_output.log 2>&1
cd ..

cd pcre-8.38
ECHO ==== compiling and installing pcre ==== >> ..\verbose_output.log 2>&1
ECHO ---- running configure ---- >> ..\verbose_output.log 2>&1
bash -c "./configure --prefix=%MINGW_BASE_DIR_BASH%" >> ..\verbose_output.log 2>&1
ECHO ---- running make ----  >> ..\verbose_output.log 2>&1
make -j 2 >> ..\verbose_output.log 2>&1
ECHO ---- running make install ---- >> ..\verbose_output.log 2>&1
make install >> ..\verbose_output.log 2>&1
ECHO ==== finished handling pcre ==== >> ..\verbose_output.log 2>&1
cd ..

cd sqlite-autoconf-3071700
ECHO ==== compiling and installing sqlite ==== >> ..\verbose_output.log 2>&1
ECHO ---- running configure ---- >> ..\verbose_output.log 2>&1
bash -c "./configure --prefix=%MINGW_BASE_DIR_BASH%" >> ..\verbose_output.log 2>&1
ECHO ---- running make ----  >> ..\verbose_output.log 2>&1
make -j 2 >> ..\verbose_output.log 2>&1
ECHO ---- running make install ---- >> ..\verbose_output.log 2>&1
make install >> ..\verbose_output.log 2>&1
ECHO ==== finished handling sqlite ==== >> ..\verbose_output.log 2>&1
cd ..

cd zlib-1.2.11
ECHO ==== compiling and installing zlib ==== >> ..\verbose_output.log 2>&1
ECHO ---- running make ----  >> ..\verbose_output.log 2>&1
make -f win32/Makefile.gcc -j 2 >> ..\verbose_output.log 2>&1
ECHO ---- running make install ---- >> ..\verbose_output.log 2>&1
SET INCLUDE_PATH=%MINGW_BASE_DIR%\include
SET LIBRARY_PATH=%MINGW_BASE_DIR%\lib
SET BINARY_PATH=%MINGW_BASE_DIR%\bin
make -f win32/Makefile.gcc install >> ..\verbose_output.log 2>&1
COPY zlib1.dll %MINGW_BASE_DIR%\bin >> ..\verbose_output.log 2>&1
COPY libz.dll.a %MINGW_BASE_DIR%\lib >> ..\verbose_output.log 2>&1
ECHO ==== finished handling zlib ==== >> ..\verbose_output.log 2>&1
cd ..

cd libzip-0.11.2
ECHO ==== compiling and installing libzip ==== >> ..\verbose_output.log 2>&1
ECHO ---- running configure ---- >> ..\verbose_output.log 2>&1
bash -c "./configure --prefix=%MINGW_BASE_DIR_BASH%" >> ..\verbose_output.log 2>&1
ECHO ---- running make ----  >> ..\verbose_output.log 2>&1
make -j 2 >> ..\verbose_output.log 2>&1
ECHO ---- running make install ---- >> ..\verbose_output.log 2>&1
make install >> ..\verbose_output.log 2>&1
COPY lib\zipconf.h %MINGW_BASE_DIR%\include >> ..\verbose_output.log 2>&1
ECHO ==== finished handling libzip ==== >> ..\verbose_output.log 2>&1
cd ..

cd zziplib-0.13.62
ECHO ==== compiling and installing zziplib ==== >> ..\verbose_output.log 2>&1
ECHO ---- changing configure script ---- >> ..\verbose_output.log 2>&1
powershell -Command "(Get-Content configure -Raw) -replace 'uname -msr', 'uname -ms' | Out-File -encoding ASCII configure" >> ..\verbose_output.log 2>&1
ECHO ---- running configure ---- >> ..\verbose_output.log 2>&1
bash -c "./configure --disable-mmap --prefix=%MINGW_BASE_DIR_BASH%" >> ..\verbose_output.log 2>&1
ECHO ---- running make ----  >> ..\verbose_output.log 2>&1
make -j 2 >> ..\verbose_output.log 2>&1
ECHO ---- running make install ---- >> ..\verbose_output.log 2>&1
make install >> ..\verbose_output.log 2>&1
ECHO ==== finished handling zziplib ==== >> ..\verbose_output.log 2>&1
cd ..

cd luarocks-2.4.0-win32
ECHO ==== installing luarocks and lua libraries ==== >> ..\verbose_output.log 2>&1
ECHO ---- installing luarocks ---- >> ..\verbose_output.log 2>&1
install.bat /P C:\LuaRocks /MW /Q >> ..\verbose_output.log 2>&1
cd \LuaRocks\lua\luarocks
ECHO ---- changing luarocks config ---- >> C:\src\verbose_output.log 2>&1
powershell -Command "(gc cfg.lua) -replace 'mingw32-gcc', 'gcc' | Out-File -encoding ASCII cfg.lua" >> C:\src\verbose_output.log 2>&1
cd \LuaRocks
ECHO ---- installing lfs ---- >> C:\src\verbose_output.log 2>&1
luarocks install LuaFileSystem >> C:\src\verbose_output.log 2>&1
ECHO ---- installing luasql.sqlite3 ---- >> C:\src\verbose_output.log 2>&1
luarocks install LuaSQL-SQLite3 SQLITE_INCDIR="%MINGW_BASE_DIR%\include" SQLITE_LIBDIR="%MINGW_BASE_DIR%\lib" >> C:\src\verbose_output.log 2>&1
ECHO ---- installing rex.pcre ---- >> C:\src\verbose_output.log 2>&1
luarocks install lrexlib-pcre PCRE_LIBDIR="%MINGW_BASE_DIR%\lib" PCRE_INCDIR="%MINGW_BASE_DIR%\include" >> C:\src\verbose_output.log 2>&1
ECHO ---- installing lua-utf8 ---- >> C:\src\verbose_output.log 2>&1
luarocks install luautf8 >> C:\src\verbose_output.log 2>&1
cd C:\src\luazip-master
ECHO ---- installing luazip ---- >> ..\verbose_output.log 2>&1
gcc -O2 -c -o src/luazip.o -I%MINGW_BASE_DIR%\include src/luazip.c >> ..\verbose_output.log 2>&1
gcc -shared -o zip.dll src/luazip.o -L%MINGW_BASE_DIR%\lib -lzzip -lz %MINGW_BASE_DIR%\bin\lua51.dll -lm >> ..\verbose_output.log 2>&1
ECHO ==== finished installing luarocks and lua libraries ==== >> ..\verbose_output.log 2>&1

IF %APPVEYOR_REPO_TAG%==false (
  SET MUDLET_VERSION_BUILD=-testing
  IF DEFINED %APPVEYOR_PULL_REQUEST_NUMBER% (
    FOR /F USEBACKQ %%SHA1 IN (`git rev-parse --short %APPVEYOR_REPO_COMMIT%`) (
      SET COMMIT=%%SHA1
    )
    SET MUDLET_VERSION_BUILD=%MUDLET_VERSION_BUILD%-PR%APPVEYOR_PULL_REQUEST_NUMBER%-%COMMIT%
  ) ELSE (
    FOR /F USEBACKQ %%SHA1 IN (`git rev-parse --short HEAD`) (
      SET COMMIT=%%SHA1
    )
    SET MUDLET_VERSION_BUILD=%MUDLET_VERSION_BUILD%-%COMMIT%
  )
)

FOR /F USEBACKQ %%VER IN (`print $1 if /^VERSION = (.+)/' < %APPVEYOR_BUILD_FOLDER%/src/src.pro`) (
  SET VERSION=%%VER
)

ECHO BUILDING MUDLET %VERSION%%MUDLET_VERSION_BUILD%