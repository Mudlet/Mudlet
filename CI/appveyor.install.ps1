# Exit the script whenever an error in a cmdlet occurs
$global:ErrorActionPreference = "Stop"

# activate higher TLS version. Seems PS only uses 1.0 by default
# credit: https://stackoverflow.com/questions/41618766/powershell-invoke-webrequest-fails-with-ssl-tls-secure-channel/48030563#48030563
[Net.ServicePointManager]::SecurityProtocol = [System.Security.Authentication.SslProtocols] "tls, tls11, tls12"

# Some global variables / settings
$workingBaseDir = "C:\src\"
$logFile = "$workingBaseDir\verbose_output.log"
$ciScriptDir = (Get-Item -Path ".\" -Verbose).FullName

if (-not $(Test-Path "$workingBaseDir")) {
    New-Item "$workingBaseDir" -ItemType "directory"
}

$64Bit = (Get-WmiObject Win32_OperatingSystem).OSArchitecture -eq "64-bit"
if($64Bit){
  $CMakePath = "C:\Program Files (x86)\CMake\bin"
} else {
  $CMakePath = "C:\Program Files\CMake\bin"
}

. .\appveyor.set-environment.ps1
SetQtBaseDir "$logFile"

$Env:PATH = "$CMakePath;C:\MinGW\bin;C:\MinGW\msys\1.0\bin;C:\Program Files\7-Zip;$Env:PATH"

# Helper functions
# see http://patrick.lioi.net/2011/08/18/powershell-and-calling-external-executables/
function script:exec {
    [CmdletBinding()]

  param(
    [Parameter(Position=0,Mandatory=1)][string]$cmd,
    [Parameter(Position=1,Mandatory=0)][string[]]$parameter = @(),
    [Parameter(Position=2,Mandatory=0)][string]$errorMessage = ("Error executing command: {0}" -f $cmd)
  )
  # ignore standard error for external programs
  $global:ErrorActionPreference = "Continue"
  $outLog = "$workingBaseDir\stdout.log"
  $errLog = "$workingBaseDir\stderr.log"
  if($parameter.Length -eq 0){
    $exitCode = (Start-Process -FilePath $cmd -Wait -PassThru -RedirectStandardOutput "$outLog" -RedirectStandardError "$errLog" -NoNewWindow).ExitCode
  } else {
    $exitCode = (Start-Process -FilePath $cmd -ArgumentList $parameter -Wait -PassThru -RedirectStandardOutput "$outLog" -RedirectStandardError "$errLog" -NoNewWindow).ExitCode
  }
  Get-Content $outLog, $errLog | Out-File $logFile -Append
  if ($exitCode -ne 0)
  {
    throw $errorMessage
  }
  # restore exit behavior
  $global:ErrorActionPreference = "Stop"
}

# Checks, whether sh.exe is found in the PATH. If so, these parts get filtered out and the remaining PATH gets returned.
function filterPathForSh {
    $noShPath = ($Env:PATH.Split(';') | Where-Object { -NOT (Test-Path (Join-Path $_ "sh.exe") -PathType Leaf) }) -join ';'
    return $noShPath
}

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

function DownloadFile([string] $url, [string] $outputFile, [bool] $bigDownload = $false) {
  $stepText = "Downloading"
  if ($bigDownload)
  {
    $stepText = "$stepText, this is a huge download and may take a while"
  }
  Step $stepText
  (New-Object System.Net.WebClient).DownloadFile($url, "$workingBaseDir\$outputFile") >> "$logFile" 2>&1
}

function ExtractTar([string] $tarFile, [string] $outputPath) {
  Step "Extracting source distribution"
  $file = Get-ChildItem $tarFile
  exec "7z" @("x", "$($file.FullName)", "-y")
  exec "7z" @("-o$outputPath", "x", "$($file.Directory)\$($file.BaseName)", "-y")
}

function ExtractZip([string] $zipFile, [string] $outputPath) {
  Step "Extracting source distribution"
  exec "7z" @("-o$outputPath", "x", "$zipFile", "-y")
}

function RunAutoReconfig(){
  Step "Running autoreconf"
  exec "bash" @("-c", "`"autoreconf -i`"")
}

function RunConfigure([string] $configureArguments = "--prefix=$Env:MINGW_BASE_DIR_BASH") {
  Step "Running configure"
  exec "bash" @("-c", "`"./configure $configureArguments MAKE=mingw32-make`"")
}

function RunMake([string] $makefile = "Makefile"){
  For ($retries=1; $retries -le 3; $retries++){
    Step "Running make"
    try{
      exec "mingw32-make" @("-f", "$makefile", "-j", $(Get-WmiObject win32_processor | Select -ExpandProperty "NumberOfLogicalProcessors"))
      break
    }Catch{
      Write-Output "Attempt $retries failed." | Tee-Object -File "$logFile" -Append
      if ($retries -lt 3) {
        Write-Output "Retrying..." | Tee-Object -File "$logFile" -Append
      }else{
        throw $_.Exception
      }
    }
  }
}

function RunMakeInstall([string] $makefile = "Makefile"){
  Step "Running make install"
  exec "mingw32-make" @("install", "-f", "$makefile")
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
function InstallSevenZ() {
  if($64Bit){
    $downloadUrl = "https://www.7-zip.org/a/7z1805-x64.exe"
  } else {
    $downloadUrl = "https://www.7-zip.org/a/7z1805.exe"
  }
  DownloadFile "$downloadUrl" "7z-installer.exe"
  Step "installing 7z"
  exec ".\7z-installer.exe" ("/S", "/D=`"C:\Program Files\7-Zip`"")
}

function InstallCmake() {
  DownloadFile "https://cmake.org/files/v3.9/cmake-3.9.6-win32-x86.msi" "cmake-installer.msi"
  Step "installing cmake"
  exec "msiexec.exe" @("/q", "/li", "$workingBaseDir\cmake-installer.log", "/i", "cmake-installer.msi")
  if(Test-Path -Path "$workingBaseDir\cmake-installer.log" -PathType Leaf){
    Get-Content "$workingBaseDir\cmake-installer.log" | Out-File $logFile -Append
  }
}

function InstallMingwGet() {
  DownloadFile "https://downloads.sourceforge.net/project/mingw/Installer/mingw-get/mingw-get-0.6.2-beta-20131004-1/mingw-get-0.6.2-mingw32-beta-20131004-1-bin.zip?r=https%3A%2F%2Fsourceforge.net%2Fprojects%2Fmingw%2Ffiles%2FInstaller%2Fmingw-get%2Fmingw-get-0.6.2-beta-20131004-1%2Fmingw-get-0.6.2-mingw32-beta-20131004-1-bin.zip%2Fdownload%3Fuse_mirror%3Dautoselect&ts=1524054843&use_mirror=autoselect" "mingw-get.zip"
  if (!(Test-Path -Path "C:\MinGW" -PathType Container)) {
    Step "Creating MinGW path"
    New-Item -Path "C:\MinGW" -ItemType "directory" >> "$logFile" 2>&1
  }
  ExtractZip "mingw-get.zip" "C:\MinGW"
}

function InstallMsys() {
  Step "Updating mingw-get info"
  exec "mingw-get" @("update")
  Step "Installing mingw32-autotools"
  exec "mingw-get" @("install", "mingw32-autotools")
}

function InstallBoost() {
  DownloadFile "https://sourceforge.net/projects/boost/files/boost/1.67.0/boost_1_67_0.tar.gz/download" "boost.tar.gz" $true
  if (!(Test-Path -Path "C:\Libraries\" -PathType Container)) {
    Step "Creating Boost path"
    New-Item -Path "C:\Libraries\" -ItemType "directory" >> "$logFile" 2>&1
  }
  ExtractTar "boost.tar.gz" "."
  Step "Copying folder"
  Move-Item "boost_1_67_0" "C:\Libraries\" >> "$logFile" 2>&1
}

function InstallQt() {
  DownloadFile "http://download.qt.io/official_releases/online_installers/qt-unified-windows-x86-online.exe" "qt-installer.exe"
  Step "Installing"
  exec ".\qt-installer.exe" @("--script=`"$(split-path -parent $script:MyInvocation.MyCommand.Path)\qt-silent-install.qs`"")
}

function InstallPython() {
  DownloadFile "https://www.python.org/ftp/python/2.7.14/python-2.7.14.msi" "python-installer.msi" $true
  exec "msiexec.exe" @("/q", "/li", "$workingBaseDir\python-installer.log", "/i", "python-installer.msi")
  if(Test-Path -Path "$workingBaseDir\python-installer.log" -PathType Leaf){
    Get-Content "$workingBaseDir\python-installer.log" | Out-File $logFile -Append
  }
}

function InstallOpenssl() {
  DownloadFile "http://wiki.overbyte.eu/arch/openssl-1.0.2o-win32.zip" "openssl-1.0.2o-i386-win32.zip"
  ExtractZip "openssl-1.0.2o-i386-win32.zip" "openssl-1.0.2o"
  Step "installing"
  exec "XCOPY" @("/S", "/I", "/Q", "openssl-1.0.2o", "$Env:MINGW_BASE_DIR\bin")
}

function InstallHunspell() {
  DownloadFile "https://github.com/hunspell/hunspell/archive/v1.6.2.tar.gz" "hunspell-1.6.2.tar.gz"
  ExtractTar "hunspell-1.6.2.tar.gz" "hunspell-1.6.2"
  Set-Location "hunspell-1.6.2\hunspell-1.6.2"
  Step "Changing src\tools\Makefile.am"
  (Get-Content src\tools\Makefile.am -Raw) -replace 'hzip ', '' | Out-File -encoding ASCII src\tools\Makefile.am >> "$logFile" 2>&1
  RunAutoReconfig
  RunConfigure
  RunMake
  RunMakeInstall
}

function InstallYajl() {
  $Env:Path = $NoShPath
  DownloadFile "https://github.com/lloyd/yajl/tarball/2.1.0" "yajl-2.1.0.tar.gz"
  ExtractTar "yajl-2.1.0.tar.gz" "yajl-2.1.0"
  Set-Location "yajl-2.1.0\lloyd-yajl-66cb08c"
  Step "changing CMakeLists.txt"
  (Get-Content CMakeLists.txt -Raw) -replace '\/W4' -replace '(?<=SET\(linkFlags)[^\)]+' -replace '\/wd4996 \/wd4255 \/wd4130 \/wd4100 \/wd4711' -replace '(?<=SET\(CMAKE_C_FLAGS_DEBUG .)\/D \DEBUG \/Od \/Z7', '-g' -replace '(?<=SET\(CMAKE_C_FLAGS_RELEASE .)\/D NDEBUG \/O2', '-O2' | Out-File -encoding ASCII CMakeLists.txt >> "$logFile" 2>&1
  if (!(Test-Path -Path "build" -PathType Container)) {
    Step "Creating yajl build path"
    New-Item build -ItemType Directory >> "$logFile" 2>&1
  }
  Set-Location build
  Step "running cmake"
  exec "cmake" @("-G", "`"MinGW Makefiles`"", "..")
  RunMake
  Step "installing"
  Copy-Item "yajl-2.1.0\lib\*" "$Env:MINGW_BASE_DIR\bin"
  exec "XCOPY" @("/S", "/I", "/Q", "yajl-2.1.0\include", "$Env:MINGW_BASE_DIR\include")
  $Env:Path = $ShPath
}

function InstallLua() {
  DownloadFile "http://www.lua.org/ftp/lua-5.1.5.tar.gz" "lua-5.1.5.tar.gz"
  ExtractTar "lua-5.1.5.tar.gz" "lua-5.1.5"
  DownloadFile "https://github.com/Tieske/luawinmake/archive/master.zip" "luawinmake.zip"
  ExtractZip "luawinmake.zip" "luawinmake"
  Step "copying luawinmake files"
  exec "XCOPY" @("/Y", "/S", "/I", "/Q", "$workingBaseDir\luawinmake\luawinmake-master\etc", "$workingBaseDir\lua-5.1.5\lua-5.1.5\etc")
  Set-Location lua-5.1.5\lua-5.1.5
  Step "compiling lua"
  exec "etc\winmake"
  Step "installing lua"
  exec "etc\winmake" @("install", "$Env:MINGW_BASE_DIR")
}

function InstallPcre() {
  DownloadFile "https://ftp.pcre.org/pub/pcre/pcre-8.42.tar.gz" "pcre-8.42.tar.gz"
  ExtractTar "pcre-8.42.tar.gz" "pcre-8.42"
  Set-Location pcre-8.42\pcre-8.42
  RunConfigure "--enable-utf --enable-unicode-properties --enable-pcre16 --prefix=$Env:MINGW_BASE_DIR_BASH"
  RunMake
  RunMakeInstall
}

function InstallSqlite() {
  DownloadFile "https://www.sqlite.org/2018/sqlite-autoconf-3230100.tar.gz" "sqlite-autoconf-3230100.tar.gz"
  ExtractTar "sqlite-autoconf-3230100.tar.gz" "sqlite"
  Set-Location sqlite\sqlite-autoconf-3230100
  Step "building sqlite"
  exec "gcc" @("-c", "sqlite3.c", "-O2", "-DSQLITE_ENABLE_FTS4", "-DSQLITE_ENABLE_RTREE")
  exec "ar" @("rcs", "libsqlite3.a", "sqlite3.o")
  Step "installing sqlite"
  Copy-Item "libsqlite3.a" "$Env:MINGW_BASE_DIR\lib"
  Copy-Item "sqlite3.h" "$Env:MINGW_BASE_DIR\include"
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
  Copy-Item "zlib1.dll" "$Env:MINGW_BASE_DIR\bin"
  Copy-Item "libz.dll.a" "$Env:MINGW_BASE_DIR\lib"
}

function InstallLibzip() {
  $Env:Path = $NoShPath
  DownloadFile "https://libzip.org/download/libzip-1.5.1.tar.gz" "libzip-1.5.1.tar.gz"
  ExtractTar "libzip-1.5.1.tar.gz" "libzip"
  Set-Location libzip\libzip-1.5.1
  if (!(Test-Path -Path "build" -PathType Container)) {
    Step "Creating libzip build path"
    New-Item build -ItemType Directory >> "$logFile" 2>&1
  }
  Set-Location build
  Step "running cmake"
  exec "cmake" @("-G", "`"MinGW Makefiles`"", "-DCMAKE_INSTALL_PREFIX=`"$Env:MINGW_BASE_DIR`"", "-DENABLE_OPENSSL=OFF", "..")
  RunMake
  RunMakeInstall
  $Env:Path = $ShPath
}

function InstallZziplib() {
  DownloadFile "https://github.com/keneanung/zziplib/archive/FixZzipStrndup.tar.gz" "zziplib-FixZzipStrndup.tar.gz"
  ExtractTar "zziplib-FixZzipStrndup.tar.gz" "zziplib"
  Set-Location zziplib\zziplib-FixZzipStrndup

  Step "changing configure script"
  (Get-Content configure -Raw) -replace 'uname -msr', 'uname -ms' | Out-File -encoding ASCII configure >> "$logFile" 2>&1
  RunConfigure "--disable-mmap --prefix=$Env:MINGW_BASE_DIR_BASH"
  RunMake
  RunMakeInstall
  Set-Location "$workingBaseDir"
}

function InstallLuarocks() {
  DownloadFile "http://luarocks.github.io/luarocks/releases/luarocks-2.4.4-win32.zip" "luarocks-2.4.4-win32.zip"
  ExtractZip "luarocks-2.4.4-win32.zip" "luarocks"
  Set-Location luarocks\luarocks-2.4.4-win32
  Step "installing luarocks"
  exec ".\install.bat" @("/P", "C:\LuaRocks", "/MW", "/Q")
  Set-Location \LuaRocks\lua\luarocks
  Step "changing luarocks config"
  (Get-Content cfg.lua) -replace 'mingw32-gcc', 'gcc' | Out-File -encoding ASCII cfg.lua >> "$logFile" 2>&1
}

function InstallPugixml() {
  $Env:Path = $NoShPath
  DownloadFile "http://github.com/zeux/pugixml/releases/download/v1.9/pugixml-1.9.zip" "pugixml-1.9.zip"
  ExtractZip "pugixml-1.9.zip" "pugixml"
  Set-Location pugixml\pugixml-1.9
  if (!(Test-Path -Path "build" -PathType Container)) {
    Step "Creating pugixml build path"
    New-Item build -ItemType Directory >> "$logFile" 2>&1
  }
  Set-Location build
  Step "running cmake"
  exec "cmake" @("-G", "`"MinGW Makefiles`"", "-DCMAKE_INSTALL_PREFIX=`"$Env:MINGW_BASE_DIR`"", "..")
  RunMake
  RunMakeInstall
  $Env:Path = $ShPath
}

function InstallLuaModules(){
  StartPart "Installing lua modules"
  Set-Location \LuaRocks
  Step "installing lfs"
  exec ".\luarocks" @("install", "LuaFileSystem")
  Step "installing luasql.sqlite3"
  exec ".\luarocks" @("install", "LuaSQL-SQLite3", "SQLITE_INCDIR=`"$Env:MINGW_BASE_DIR\include`"", "SQLITE_LIBDIR=`"$Env:MINGW_BASE_DIR\lib`"")
  Step "installing rex.pcre"
  exec ".\luarocks" @("install", "lrexlib-pcre", "PCRE_LIBDIR=`"$Env:MINGW_BASE_DIR\lib`"", "PCRE_INCDIR=`"$Env:MINGW_BASE_DIR\include`"")
  Step "installing lua-utf8"
  exec ".\luarocks" @("install", "luautf8")
  Step "installing lua-yajl"
  $Env:LIBRARY_PATH = "$Env:LIBRARY_PATH;$Env:MINGW_BASE_DIR/bin"
  exec ".\luarocks" @("install", "lua-yajl", "YAJL_LIBDIR=`"$Env:MINGW_BASE_DIR\bin`"", "YAJL_INCDIR=`"$Env:MINGW_BASE_DIR\include`"")

  Step "installing luazip"
  Set-Location "$workingBaseDir"
  DownloadFile "https://github.com/rjpcomputing/luazip/archive/master.zip" "luazip.zip"
  ExtractZip "luazip.zip" "luazip"
  Set-Location luazip\luazip-master
  Step "installing luazip"
  exec "gcc" @("-O2", "-c", "-o", "src/luazip.o", "-I`"$Env:MINGW_BASE_DIR/include`"", "src/luazip.c")
  exec "gcc" @("-shared", "-o", "zip.dll", "src/luazip.o", "-L`"$Env:MINGW_BASE_DIR/lib`"", "-lzzip", "-lz", "`"$Env:MINGW_BASE_DIR/bin/lua51.dll`"", "-lm")
  Copy-Item "zip.dll" "$Env:MINGW_BASE_DIR\lib\lua\5.1"
  FinishPart "Installing lua modules"
}

# install dependencies

CheckAndInstall "7z" "C:\Program Files\7-Zip\7z.exe" { InstallSevenZ }
CheckAndInstall "cmake" "$CMakePath\cmake.exe" { InstallCmake }
CheckAndInstall "mingw-get" "C:\MinGW\bin\mingw-get.exe" { InstallMingwGet }
CheckAndInstall "MSYS and autotools" "C:\MinGW\bin\autoconf" { InstallMsys }
CheckAndInstall "Boost" "C:\Libraries\boost_1_67_0\bootstrap.bat" { InstallBoost }
CheckAndInstall "Qt" "$Env:QT_BASE_DIR\bin\qmake.exe" { InstallQt }
CheckAndInstall "Python" "C:\Python27\python.exe" { InstallPython }

# Adapt the PATH variable again as we may have installed some dependencies just now and can determine their location.
SetMingwBaseDir "$logFile"
$ShPath = "$Env:MINGW_BASE_DIR\bin;C:\Python27;$Env:PATH"
$Env:PATH = $ShPath
# Filter PATH because cmake complains if an sh.exe being in the PATH and it's instructed to create MinGW makefiles.
# But on the other hand we keep sh.exe in the PATH to easily run "configure" scripts. So we create 2 variables and assign PATH accordingly.
$NoShPath = filterPathForSh

CheckAndInstall "openssl" "$Env:MINGW_BASE_DIR\bin\ssleay32.dll" { InstallOpenssl }
CheckAndInstall "hunspell" "$Env:MINGW_BASE_DIR\bin\libhunspell-1.6-0.dll" { InstallHunspell }
CheckAndInstall "yajl" "$Env:MINGW_BASE_DIR\bin\libyajl.dll" { InstallYajl }
CheckAndInstall "lua" "$Env:MINGW_BASE_DIR\bin\lua51.dll" { InstallLua }
CheckAndInstall "pcre" "$Env:MINGW_BASE_DIR\bin\libpcre-1.dll" { InstallPcre }
CheckAndInstall "sqlite" "$Env:MINGW_BASE_DIR\lib\libsqlite3.a" { InstallSqlite }
CheckAndInstall "zlib" "$Env:MINGW_BASE_DIR\bin\zlib1.dll" { InstallZlib }
CheckAndInstall "libzip" "$Env:MINGW_BASE_DIR\include\zipconf.h" { InstallLibzip }
CheckAndInstall "zziplib" "$Env:MINGW_BASE_DIR\lib\libzzip.la" { InstallZziplib }
CheckAndInstall "luarocks" "C:\LuaRocks\luarocks.bat" { InstallLuarocks }
CheckAndInstall "pugixml" "$Env:MINGW_BASE_DIR\lib\libpugixml.a" { InstallPugixml }
InstallLuaModules
