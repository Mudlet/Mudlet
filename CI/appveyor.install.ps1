git submodule update --init --recursive
Get-ChildItem Env:
if (-not $(Test-Path "C:\src")) {
    New-Item "C:\src" -ItemType "directory"
}
Set-Location "C:\src"
$Env:PATH="$Env:MINGW_BASE_DIR\bin;C:\MinGW\msys\1.0\bin;C:\Program Files (x86)\CMake\bin;C:\Program Files\7-Zip;$Env:PATH"

Write-Output "==== downloading dependencies ===="  | Tee-Object -File "verbose_output.log" -Append
Invoke-WebRequest https://github.com/hunspell/hunspell/archive/v1.4.1.tar.gz -OutFile hunspell-1.4.1.tar.gz >> verbose_output.log 2>&1
Invoke-WebRequest http://www.lua.org/ftp/lua-5.1.5.tar.gz -OutFile lua-5.1.5.tar.gz >> verbose_output.log 2>&1
Invoke-WebRequest https://sourceforge.net/projects/pcre/files/pcre/8.38/pcre-8.38.tar.gz/download -OutFile pcre-8.38.tar.gz -UserAgent [Microsoft.PowerShell.Commands.PSUserAgent]::Chrome >> verbose_output.log 2>&1
Invoke-WebRequest http://zlib.net/zlib-1.2.11.tar.gz -OutFile zlib-1.2.11.tar.gz >> verbose_output.log 2>&1
Invoke-WebRequest http://www.sqlite.org/2013/sqlite-autoconf-3071700.tar.gz -OutFile sqlite-autoconf-3071700.tar.gz >> verbose_output.log 2>&1
Invoke-WebRequest https://launchpad.net/ubuntu/+archive/primary/+files/libzip_0.11.2.orig.tar.gz -OutFile libzip_0.11.2.orig.tar.gz >> verbose_output.log 2>&1
Invoke-WebRequest https://github.com/lloyd/yajl/tarball/2.0.1 -OutFile yajl-2.0.1.tar.gz >> verbose_output.log 2>&1
Invoke-WebRequest https://sourceforge.net/projects/zziplib/files/zziplib13/0.13.62/zziplib-0.13.62.tar.bz2/download -OutFile zziplib-0.13.62.tar.bz2 -UserAgent [Microsoft.PowerShell.Commands.PSUserAgent]::Chrome >> verbose_output.log 2>&1
Invoke-WebRequest https://indy.fulgan.com/SSL/openssl-1.0.2l-i386-win32.zip -OutFile openssl-1.0.2l-i386-win32.zip >> verbose_output.log 2>&1
Invoke-WebRequest http://keplerproject.github.io/luarocks/releases/luarocks-2.4.0-win32.zip -OutFile luarocks-2.4.0-win32.zip >> verbose_output.log 2>&1
Invoke-WebRequest https://github.com/rjpcomputing/luazip/archive/master.zip -OutFile luazip.zip >> verbose_output.log 2>&1
Invoke-WebRequest https://github.com/Tieske/luawinmake/archive/master.zip -OutFile luawinmake.zip >> verbose_output.log 2>&1
Invoke-WebRequest https://installbuilder.bitrock.com/installbuilder-qt-enterprise-17.3.0-windows-installer.exe -OutFile installbuilder-qt-installer.exe >> verbose_output.log 2>&1
Write-Output "==== finished downloading dependencies ====" | Tee-Object -File "verbose_output.log" -Append

Write-Output "==== extracting archives ====" | Tee-Object -File "verbose_output.log" -Append
Get-ChildItem "C:\src" -Filter *.tar.gz | 
Foreach-Object {
  7z x "$($_.FullName)" -y >> verbose_output.log 2>&1
  7z x "$($_.Directory)\$($_.BaseName)" -y >> verbose_output.log 2>&1
}
Get-ChildItem "C:\src" -Filter *.tar.bz2 | 
Foreach-Object {
  7z x "$($_.FullName)" -y >> verbose_output.log 2>&1
  7z x "$($_.Directory)\$($_.BaseName)" -y >> verbose_output.log 2>&1
}
7z -o"openssl-1.0.2l" e "openssl-1.0.2l-i386-win32.zip" -y >> verbose_output.log 2>&1
7z x "luarocks-2.4.0-win32.zip" -y >> verbose_output.log 2>&1
7z x "luazip.zip" -y >> verbose_output.log 2>&1
7z x "luawinmake.zip" -y >> verbose_output.log 2>&1
Write-Output "==== finished extracting archives ====" | Tee-Object -File "verbose_output.log" -Append

cd hunspell-1.4.1
Write-Output "==== compiling and installing hunspell ====" | Tee-Object -File "..\verbose_output.log" -Append
Write-Output "---- running configure ----" | Tee-Object -File "..\verbose_output.log" -Append
bash -c "./configure --prefix=$Env:MINGW_BASE_DIR_BASH" >> ..\verbose_output.log 2>&1
Write-Output "---- running make ----" | Tee-Object -File "..\verbose_output.log" -Append
mingw32-make -j 2 >> ..\verbose_output.log 2>&1
Write-Output "---- running make install ----" | Tee-Object -File "..\verbose_output.log" -Append
mingw32-make install >> ..\verbose_output.log 2>&1
Write-Output "==== finished handling hunspell ====" | Tee-Object -File "..\verbose_output.log" -Append
cd ..

cd lloyd-yajl-f4b2b1a
Write-Output "==== compiling and installing yajl ====" | Tee-Object -File "..\verbose_output.log" -Append
Write-Output "---- changing CMakeLists.txt ----" | Tee-Object -File "..\verbose_output.log" -Append
(Get-Content CMakeLists.txt -Raw) -replace '\/W4' -replace '(?<=SET\(linkFlags)[^\)]+' -replace '\/wd4996 \/wd4255 \/wd4130 \/wd4100 \/wd4711' -replace '(?<=SET\(CMAKE_C_FLAGS_DEBUG .)\/D \DEBUG \/Od \/Z7', '-g' -replace '(?<=SET\(CMAKE_C_FLAGS_RELEASE .)\/D NDEBUG \/O2', '-O2' | Out-File -encoding ASCII CMakeLists.txt >> ..\verbose_output.log 2>&1
mkdir build
cd build
Write-Output "---- running cmake ----" | Tee-Object -File "..\..\verbose_output.log" -Append
cmake -G "MSYS Makefiles" ..  >> ..\..\verbose_output.log 2>&1
Write-Output "---- running make ----" | Tee-Object -File "..\..\verbose_output.log" -Append
mingw32-make -j 2  >> ..\..\verbose_output.log 2>&1
Write-Output "---- installing yajl ----" | Tee-Object -File "..\..\verbose_output.log" -Append
COPY yajl-2.0.1\lib\* $Env:MINGW_BASE_DIR\lib >> ..\..\verbose_output.log 2>&1
XCOPY /S /I /Q yajl-2.0.1\include $Env:MINGW_BASE_DIR\include >> ..\..\verbose_output.log 2>&1
Write-Output "==== finished handling yajl ====" | Tee-Object -File "..\..\verbose_output.log" -Append
cd ..\..

Write-Output "==== compiling and installing lua ====" | Tee-Object -File "verbose_output.log" -Append
Write-Output "---- copying luawinmake files ----" | Tee-Object -File "verbose_output.log" -Append
XCOPY /S /I /Q C:\src\luawinmake-master\etc C:\src\lua-5.1.5\etc >> verbose_output.log 2>&1
cd lua-5.1.5
Write-Output "---- compiling lua ----" | Tee-Object -File "..\verbose_output.log" -Append
etc\winmake >> ..\verbose_output.log 2>&1
Write-Output "---- installing lua ----" | Tee-Object -File "..\verbose_output.log" -Append
etc\winmake install $Env:MINGW_BASE_DIR | Tee-Object -File "..\verbose_output.log" -Append
Write-Output "==== finished handling lua ====" | Tee-Object -File "..\verbose_output.log" -Append
cd ..

cd pcre-8.38
Write-Output "==== compiling and installing pcre ====" | Tee-Object -File "..\verbose_output.log" -Append
Write-Output "---- running configure ----" | Tee-Object -File "..\verbose_output.log" -Append
bash -c "./configure --prefix=$Env:MINGW_BASE_DIR_BASH" >> ..\verbose_output.log 2>&1
Write-Output "---- running make ----" | Tee-Object -File "..\verbose_output.log" -Append
mingw32-make -j 2 >> ..\verbose_output.log 2>&1
Write-Output "---- running make install ----" | Tee-Object -File "..\verbose_output.log" -Append
mingw32-make install >> ..\verbose_output.log 2>&1
Write-Output "==== finished handling pcre ====" | Tee-Object -File "..\verbose_output.log" -Append
cd ..

cd sqlite-autoconf-3071700
Write-Output "==== compiling and installing sqlite ====" | Tee-Object -File "..\verbose_output.log" -Append
Write-Output "---- running configure ----" | Tee-Object -File "..\verbose_output.log" -Append
bash -c "./configure --prefix=$Env:MINGW_BASE_DIR_BASH" >> ..\verbose_output.log 2>&1
Write-Output "---- running make ----" | Tee-Object -File "..\verbose_output.log" -Append
mingw32-make -j 2 >> ..\verbose_output.log 2>&1
Write-Output "---- running make install ----" | Tee-Object -File "..\verbose_output.log" -Append
mingw32-make install >> ..\verbose_output.log 2>&1
Write-Output "==== finished handling sqlite ====" | Tee-Object -File "..\verbose_output.log" -Append
cd ..

cd zlib-1.2.11
Write-Output "==== compiling and installing zlib ====" | Tee-Object -File "..\verbose_output.log" -Append
Write-Output "---- running make ----" | Tee-Object -File "..\verbose_output.log" -Append
mingw32-make -f win32/Makefile.gcc -j 2 >> ..\verbose_output.log 2>&1
Write-Output "---- running make install ----" | Tee-Object -File "..\verbose_output.log" -Append
SET INCLUDE_PATH=$Env:MINGW_BASE_DIR\include
SET LIBRARY_PATH=$Env:MINGW_BASE_DIR\lib
SET BINARY_PATH=$Env:MINGW_BASE_DIR\bin
mingw32-make -f win32/Makefile.gcc install >> ..\verbose_output.log 2>&1
COPY zlib1.dll $Env:MINGW_BASE_DIR\bin >> ..\verbose_output.log 2>&1
COPY libz.dll.a $Env:MINGW_BASE_DIR\lib >> ..\verbose_output.log 2>&1
Write-Output "==== finished handling zlib ====" | Tee-Object -File "..\verbose_output.log" -Append
cd ..

cd libzip-0.11.2
Write-Output "==== compiling and installing libzip ====" | Tee-Object -File "..\verbose_output.log" -Append
Write-Output "---- running configure ----" | Tee-Object -File "..\verbose_output.log" -Append
bash -c "./configure --prefix=$Env:MINGW_BASE_DIR_BASH" >> ..\verbose_output.log 2>&1
Write-Output "---- running make ----" | Tee-Object -File "..\verbose_output.log" -Append
mingw32-make -j 2 >> ..\verbose_output.log 2>&1
Write-Output "---- running make install ----" | Tee-Object -File "..\verbose_output.log" -Append
mingw32-make install >> ..\verbose_output.log 2>&1
COPY lib\zipconf.h $Env:MINGW_BASE_DIR\include >> ..\verbose_output.log 2>&1
Write-Output "==== finished handling libzip ====" | Tee-Object -File "..\verbose_output.log" -Append
cd ..

cd zziplib-0.13.62
Write-Output "==== compiling and installing zziplib ====" | Tee-Object -File "..\verbose_output.log" -Append
Write-Output "---- changing configure script ----" | Tee-Object -File "..\verbose_output.log" -Append
(Get-Content configure -Raw) -replace 'uname -msr', 'uname -ms' | Out-File -encoding ASCII configure >> ..\verbose_output.log 2>&1
Write-Output "---- running configure ----" | Tee-Object -File "..\verbose_output.log" -Append
bash -c "./configure --disable-mmap --prefix=$Env:MINGW_BASE_DIR_BASH" >> ..\verbose_output.log 2>&1
Write-Output "---- running make ----" | Tee-Object -File "..\verbose_output.log" -Append
mingw32-make -j 2 >> ..\verbose_output.log 2>&1
Write-Output "---- running make install ----" | Tee-Object -File "..\verbose_output.log" -Append
mingw32-make install >> ..\verbose_output.log 2>&1
Write-Output "==== finished handling zziplib ====" | Tee-Object -File "..\verbose_output.log" -Append
cd ..

cd luarocks-2.4.0-win32
Write-Output "==== installing luarocks and lua libraries ====" | Tee-Object -File "..\verbose_output.log" -Append
Write-Output "---- installing luarocks ----" | Tee-Object -File "..\verbose_output.log" -Append
install.bat /P C:\LuaRocks /MW /Q >> ..\verbose_output.log 2>&1
cd \LuaRocks\lua\luarocks
Write-Output "---- changing luarocks config ----" | Tee-Object -File "..\verbose_output.log" -Append
(gc cfg.lua) -replace 'mingw32-gcc', 'gcc' | Out-File -encoding ASCII cfg.lua >> C:\src\verbose_output.log 2>&1
cd \LuaRocks
Write-Output "---- installing lfs ----" | Tee-Object -File "C:\src\verbose_output.log" -Append
luarocks install LuaFileSystem >> C:\src\verbose_output.log 2>&1
Write-Output "---- installing luasql.sqlite3 ----" | Tee-Object -File "C:\src\verbose_output.log" -Append
luarocks install LuaSQL-SQLite3 SQLITE_INCDIR="$Env:MINGW_BASE_DIR\include" SQLITE_LIBDIR="$Env:MINGW_BASE_DIR\lib" >> C:\src\verbose_output.log 2>&1
Write-Output "---- installing rex.pcre ----" | Tee-Object -File "C:\src\verbose_output.log" -Append
luarocks install lrexlib-pcre PCRE_LIBDIR="$Env:MINGW_BASE_DIR\lib" PCRE_INCDIR="$Env:MINGW_BASE_DIR\include" >> C:\src\verbose_output.log 2>&1
Write-Output "---- installing lua-utf8 ----" | Tee-Object -File "C:\src\verbose_output.log" -Append
luarocks install luautf8 >> C:\src\verbose_output.log 2>&1
cd C:\src\luazip-master
Write-Output "---- installing luazip ----" | Tee-Object -File "C:\src\verbose_output.log" -Append
gcc -O2 -c -o src/luazip.o -I$Env:MINGW_BASE_DIR\include src/luazip.c >> ..\verbose_output.log 2>&1
gcc -shared -o zip.dll src/luazip.o -L$Env:MINGW_BASE_DIR\lib -lzzip -lz $Env:MINGW_BASE_DIR\bin\lua51.dll -lm >> ..\verbose_output.log 2>&1
Write-Output "==== finished installing luarocks and lua libraries ====" | Tee-Object -File "C:\src\verbose_output.log" -Append

if ($Env:APPVEYOR_REPO_TAG -eq "false") {
  $Env:MUDLET_VERSION_BUILD = "-testing"
  if (Test-Path Env:APPVEYOR_PULL_REQUEST_NUMBER) {
      $Script:Commit = git rev-parse --short $Env:APPVEYOR_REPO_COMMIT
      $Env:MUDLET_VERSION_BUILD = "$Env:MUDLET_VERSION_BUILD-$Env:APPVEYOR_PULL_REQUEST_NUMBER$Commit"
  } else {
    $Script:Commit = git rev-parse --short HEAD
    $Env:MUDLET_VERSION_BUILD = "$Env:MUDLET_VERSION_BUILD-$Commit"
  }
}

$VersionLine = Select-String -Pattern "Version =" $Env:APPVEYOR_BUILD_FOLDER/src/mudlet.pro
$VersionRegex = [regex]'= {1}(.+)$'
$Env:VERSION = $VersionRegex.Match($VersionLine).Groups[1].Value

Write-Output "BUILDING MUDLET $Env:VERSION$Env:MUDLET_VERSION_BUILD"
