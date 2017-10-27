# Some global variables / settings
$workingBaseDir = "C:\src\"
$logFile = "$workingBaseDir\verbose_output.log"
$ShPath = "$Env:MINGW_BASE_DIR\bin;C:\MinGW\msys\1.0\bin;C:\Program Files (x86)\CMake\bin;C:\Program Files\7-Zip;$Env:PATH"
$NoShPath = ($ShPath.Split(';') | Where-Object { $_ -ne 'C:\MinGW\msys\1.0\bin' } | Where-Object { $_ -ne 'C:\Program Files\Git\usr\bin' }) -join ';'

# Helper functions
function PartSkeleton([string] $content) {
  Write-Output "==== $content ====" | Tee-Object -File "$logFile" -Append
}

function StartPart([string] $partName) {
  PartSkeleton "compiling and installing $partName"
}

function FinishPart([string] $partName) {
  PartSkeleton "$partName compiled and installed"
}

function Step([string] $stepName) {
  Write-Output "---- $stepName ----" | Tee-Object -File "$logFile" -Append
}

function DownloadFile([string] $url, [string] $outputFile) {
  Step "Downloading"
  Invoke-WebRequest "$url" -OutFile "$outputFile" -UserAgent [Microsoft.PowerShell.Commands.PSUserAgent]::Chrome >> "$logFile" 2>&1
}

function ExtractTar([string] $tarFile, [string] $outputPath) {
  Step "Extracting source distribution"
  $file = Get-ChildItem $tarFile
  7z x "$($file.FullName)" -y >> "$logFile" 2>&1
  7z -o"$outputPath" x "$($file.Directory)\$($file.BaseName)" -y >> "$logFile" 2>&1
}

function ExtractZip([string] $zipFile, [string] $outputPath) {
  Step "Extracting source distribution"
  7z -o"$outputPath" x "$zipFile" -y >> "$logFile" 2>&1
}

function RunConfigure([string] $configureArguments = "--prefix=$Env:MINGW_BASE_DIR_BASH") {
  Step "Running configure"
  bash -c "./configure $configureArguments" >> "$logFile" 2>&1
}

function RunMake([string] $makefile = "Makefile"){
  Step "Running make"
  mingw32-make -f "$makefile" -j 2 >> "$logFile" 2>&1
}

function RunMakeInstall([string] $makefile = "Makefile"){
  Step "Running make install"
  mingw32-make install -f "$makefile" >> "$logFile" 2>&1
}

function CheckAndInstall([string] $dependencyName, [string] $signalFile, [scriptblock] $installationFunction){
  StartPart $dependencyName
  if(Test-Path "$signalFile" -PathType Leaf){
    Step "$dependencyName is already installed, skipping..."
  } else {
    Set-Location "$workingBaseDir"
    & $installationFunction
  }
  FinishPart $dependencyName
}

# installation functions
function InstallOpenssl() {
  DownloadFile "https://indy.fulgan.com/SSL/openssl-1.0.2l-i386-win32.zip" "openssl-1.0.2l-i386-win32.zip"
  ExtractZip "openssl-1.0.2l-i386-win32.zip" "openssl-1.0.2l"
}

function InstallHunspell() {
  DownloadFile "https://github.com/hunspell/hunspell/archive/v1.4.1.tar.gz" "hunspell-1.4.1.tar.gz"
  ExtractTar "hunspell-1.4.1.tar.gz" "hunspell-1.4.1"
  Set-Location "hunspell-1.4.1\hunspell-1.4.1"
  RunConfigure
  RunMake
  RunMakeInstall
}

function InstallYajl() {
  $Env:Path = $NoShPath
  DownloadFile "https://github.com/lloyd/yajl/tarball/2.0.1" "yajl-2.0.1.tar.gz"
  ExtractTar "yajl-2.0.1.tar.gz" "yajl-2.0.1"
  Set-Location "yajl-2.0.1\lloyd-yajl-f4b2b1a"
  Step "changing CMakeLists.txt"
  (Get-Content CMakeLists.txt -Raw) -replace '\/W4' -replace '(?<=SET\(linkFlags)[^\)]+' -replace '\/wd4996 \/wd4255 \/wd4130 \/wd4100 \/wd4711' -replace '(?<=SET\(CMAKE_C_FLAGS_DEBUG .)\/D \DEBUG \/Od \/Z7', '-g' -replace '(?<=SET\(CMAKE_C_FLAGS_RELEASE .)\/D NDEBUG \/O2', '-O2' | Out-File -encoding ASCII CMakeLists.txt >> "$logFile" 2>&1
  New-Item build -ItemType Directory >> "$logFile" 2>&1
  Set-Location build
  Step "running cmake"
  cmake -G "MinGW Makefiles" ..  >> "$logFile" 2>&1
  RunMake
  Step "installing"
  COPY yajl-2.0.1\lib\* $Env:MINGW_BASE_DIR\lib >> "$logFile" 2>&1
  XCOPY /S /I /Q yajl-2.0.1\include $Env:MINGW_BASE_DIR\include >> "$logFile" 2>&1
  $Env:Path = $ShPath
}

function InstallLua() {
  DownloadFile "http://www.lua.org/ftp/lua-5.1.5.tar.gz" "lua-5.1.5.tar.gz"
  ExtractTar "lua-5.1.5.tar.gz" "lua-5.1.5"
  DownloadFile "https://github.com/Tieske/luawinmake/archive/master.zip" "luawinmake.zip"
  ExtractZip "luawinmake.zip" "luawinmake"
  Step "copying luawinmake files"
  XCOPY /S /I /Q "$workingBaseDir\luawinmake\luawinmake-master\etc" "$workingBaseDir\lua-5.1.5\lua-5.1.5\etc" >> "$logFile" 2>&1
  Set-Location lua-5.1.5\lua-5.1.5
  Step "compiling lua"
  .\etc\winmake >> "$logFile" 2>&1
  Step "installing lua"
  .\etc\winmake install $Env:MINGW_BASE_DIR >> "$logFile" 2>&1
}

function InstallPcre() {
  DownloadFile "https://sourceforge.net/projects/pcre/files/pcre/8.38/pcre-8.38.tar.gz/download" "pcre-8.38.tar.gz"
  ExtractTar "pcre-8.38.tar.gz" "pcre-8.38"
  Set-Location pcre-8.38\pcre-8.38
  RunConfigure
  RunMake
  RunMakeInstall
}

function InstallSqlite() {
  DownloadFile "http://www.sqlite.org/2013/sqlite-autoconf-3071700.tar.gz" "sqlite-autoconf-3071700.tar.gz"
  ExtractTar "sqlite-autoconf-3071700.tar.gz" "sqlite"
  Set-Location sqlite\sqlite-autoconf-3071700
  RunConfigure
  RunMake
  RunMakeInstall
}

function InstallZlib() {
  DownloadFile "http://zlib.net/zlib-1.2.11.tar.gz" "zlib-1.2.11.tar.gz"
  ExtractTar "zlib-1.2.11.tar.gz" "zlib"
  Set-Location zlib\zlib-1.2.11
  RunMake "win32/Makefile.gcc"
  $Env:INCLUDE_PATH = "$Env:MINGW_BASE_DIR\include"
  $Env:LIBRARY_PATH = "$Env:MINGW_BASE_DIR\lib"
  $Env:BINARY_PATH = "$Env:MINGW_BASE_DIR\bin"
  RunMakeInstall "win32/Makefile.gcc"
  COPY zlib1.dll $Env:MINGW_BASE_DIR\bin >> "$logFile" 2>&1
  COPY libz.dll.a $Env:MINGW_BASE_DIR\lib >> "$logFile" 2>&1
}

function InstallLibzip() {
  DownloadFile "https://libzip.org/download/libzip-1.3.0.tar.gz" "libzip-1.3.0.tar.gz"
  ExtractTar "libzip-1.3.0.tar.gz" "libzip"
  Set-Location libzip\libzip-1.3.0
  RunConfigure
  RunMake
  RunMakeInstall
  COPY lib\zipconf.h $Env:MINGW_BASE_DIR\include >> "$logFile" 2>&1
}

function InstallZziplib() {
  DownloadFile "https://sourceforge.net/projects/zziplib/files/zziplib13/0.13.62/zziplib-0.13.62.tar.bz2/download" "zziplib-0.13.62.tar.bz2"
  ExtractTar "zziplib-0.13.62.tar.bz2" "zziplib"
  cd zziplib\zziplib-0.13.62
  Step "changing configure script"
  (Get-Content configure -Raw) -replace 'uname -msr', 'uname -ms' | Out-File -encoding ASCII configure >> "$logFile" 2>&1
  RunConfigure "--disable-mmap --prefix=$Env:MINGW_BASE_DIR_BASH"
  RunMake
  RunMakeInstall
}

function InstallLuarocks() {
  DownloadFile "http://keplerproject.github.io/luarocks/releases/luarocks-2.4.0-win32.zip" "luarocks-2.4.0-win32.zip"
  ExtractZip "luarocks-2.4.0-win32.zip" "luarocks"
  Set-Location luarocks\luarocks-2.4.0-win32
  Step "installing luarocks"
  .\install.bat /P C:\LuaRocks /MW /Q >> "$logFile" 2>&1
  Set-Location \LuaRocks\lua\luarocks
  Step "changing luarocks config"
  (Get-Content cfg.lua) -replace 'mingw32-gcc', 'gcc' | Out-File -encoding ASCII cfg.lua >> "$logFile" 2>&1
}

function InstallLuaModules(){
  StartPart "Installing lua modules"
  Set-Location \LuaRocks
  Step "installing lfs"
  .\luarocks install LuaFileSystem >> "$logFile" 2>&1
  Step "installing luasql.sqlite3"
  .\luarocks install LuaSQL-SQLite3 SQLITE_INCDIR="$Env:MINGW_BASE_DIR\include" SQLITE_LIBDIR="$Env:MINGW_BASE_DIR\lib" >> "$logFile" 2>&1
  Step "installing rex.pcre"
  .\luarocks install lrexlib-pcre PCRE_LIBDIR="$Env:MINGW_BASE_DIR\lib" PCRE_INCDIR="$Env:MINGW_BASE_DIR\include" >> "$logFile" 2>&1
  Step "installing lua-utf8"
  .\luarocks install luautf8 >> "$logFile" 2>&1

  Step "installing luazip"
  Set-Location "$workingBaseDir"
  DownloadFile "https://github.com/rjpcomputing/luazip/archive/master.zip" "luazip.zip"
  ExtractZip "luazip.zip" "luazip"
  Set-Location luazip\luazip-master
  Step "installing luazip"
  gcc -O2 -c -o src/luazip.o -I"$Env:MINGW_BASE_DIR/include" src/luazip.c >> "$logFile" 2>&1
  gcc -shared -o zip.dll src/luazip.o -L"$Env:MINGW_BASE_DIR/lib" -lzzip -lz "$Env:MINGW_BASE_DIR/bin/lua51.dll" -lm >> "$logFile" 2>&1
  FinishPart "Installing lua modules"
}

git submodule update --init --recursive
if (-not $(Test-Path "$workingBaseDir")) {
    New-Item "$workingBaseDir" -ItemType "directory"
}

# install dependencies

$Env:PATH=$ShPath

CheckAndInstall "openssl" "$workingBaseDir\openssl-1.0.2l\ssleay32.dll" { & InstallOpenssl }
CheckAndInstall "hunspell" "$Env:MINGW_BASE_DIR\bin\libhunspell-1.4-0.dll" { & InstallHunspell }
CheckAndInstall "yajl" "$Env:MINGW_BASE_DIR\lib\libyajl.dll" { & InstallYajl }
CheckAndInstall "lua" "$Env:MINGW_BASE_DIR\bin\lua51.dll" { & InstallLua }
CheckAndInstall "pcre" "$Env:MINGW_BASE_DIR\bin\libpcre-1.dll" { & InstallPcre }
CheckAndInstall "sqlite" "$Env:MINGW_BASE_DIR\lib\libsqlite3-0.dll" { & InstallSqlite }
CheckAndInstall "zlib" "$Env:MINGW_BASE_DIR\bin\zlib1.dll" { & InstallZlib }
CheckAndInstall "libzip" "$Env:MINGW_BASE_DIR\include\zipconf.h" { & InstallLibzip }
CheckAndInstall "zziplib" "$Env:MINGW_BASE_DIR\lib\libzzip.la" { & InstallZziplib }
CheckAndInstall "luarocks" "C:\LuaRocks\luarocks.bat" { & InstallLuarocks }
InstallLuaModules
