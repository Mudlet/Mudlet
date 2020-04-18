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

function SetQtBaseDir([string] $logFile) {
  if(!(Test-Path Env:QT_BASE_DIR)){
    try
    {
      $Env:QT_BASE_DIR = Get-Command "qmake.exe" -ErrorAction Stop | Select-Object -ExpandProperty definition | Split-Path -Parent | Split-Path -Parent
    }
    catch
    {
      $Env:QT_BASE_DIR = "C:\Qt\5.13.2\mingw73_32"
    }
  }
  Write-Output "Using $Env:QT_BASE_DIR as QT base directory." | Tee-Object -File "$logFile" -Append
}

function SetMingwBaseDir([string] $logFile) {
  if(!(Test-Path Env:MINGW_BASE_DIR)){
    $tmp = $Env:QT_BASE_DIR.Split("\\")
    $tmp[-2] = "Tools"
    $tmp[-1] = $tmp[-1] -replace "_32", "*"
    $tmp = $tmp -join "\" | Resolve-Path
    if($tmp -is [array]){
      $tmp = $tmp[-1]
    }
    $Env:MINGW_BASE_DIR = $tmp
  }
  Write-Output "Using $Env:MINGW_BASE_DIR as MinGW base directory." | Tee-Object -File "$logFile" -Append

  if(!(Test-Path Env:MINGW_BASE_DIR_BASH)){
    $Env:MINGW_BASE_DIR_BASH = $Env:MINGW_BASE_DIR -replace "\\", "/" -replace "C:", "/c"
  }
}

function SetLuarocksPath([string] $logFile) {
  $Env:LUA_CPATH = "$Env:MINGW_BASE_DIR\lib\lua\5.1\?.dll;$Env:LUA_CPATH"
  Write-Output "Using $Env:LUA_CPATH as LuaRocks path." | Tee-Object -File "$logFile" -Append
}

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
    $downloadUrl = "https://www.7-zip.org/a/7z1900-x64.exe"
  } else {
    $downloadUrl = "https://www.7-zip.org/a/7z1900.exe"
  }
  DownloadFile "$downloadUrl" "7z-installer.exe"
  Step "installing 7z"
  exec ".\7z-installer.exe" ("/S", "/D=`"C:\Program Files\7-Zip`"")
}

function InstallCmake() {
  DownloadFile "https://github.com/Kitware/CMake/releases/download/v3.14.4/cmake-3.14.4-win32-x86.msi" "cmake-installer.msi"
  Step "installing cmake"
  exec "msiexec.exe" @("/q", "/li", "$workingBaseDir\cmake-installer.log", "/i", "cmake-installer.msi")
  if(Test-Path -Path "$workingBaseDir\cmake-installer.log" -PathType Leaf){
    Get-Content "$workingBaseDir\cmake-installer.log" | Out-File $logFile -Append
  }
}

function InstallMingwGet() {
  DownloadFile "https://osdn.net/frs/redir.php?m=rwthaachen&f=mingw%2F68260%2Fmingw-get-0.6.3-mingw32-pre-20170905-1-bin.zip" "mingw-get.zip"
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
  DownloadFile "https://sourceforge.net/projects/boost/files/boost/1.71.0.beta1/boost_1_71_0_b1.tar.gz/download" "boost.tar.gz" $true
  if (!(Test-Path -Path "C:\Libraries\" -PathType Container)) {
    Step "Creating Boost path"
    New-Item -Path "C:\Libraries\" -ItemType "directory" >> "$logFile" 2>&1
  }
  ExtractTar "boost.tar.gz" "."
  Step "Copying folder"
  Move-Item "boost_1_71_0" "C:\Libraries\" >> "$logFile" 2>&1
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
  DownloadFile "http://wiki.overbyte.eu/arch/openssl-1.1.1d-win32.zip" "openssl-win32.zip"
  ExtractZip "openssl-win32.zip" "openssl"
  Step "installing"
  exec "XCOPY" @("/S", "/I", "/Q", "openssl", "$Env:MINGW_BASE_DIR\bin")
}

function InstallHunspell() {
  DownloadFile "https://github.com/hunspell/hunspell/archive/v1.6.2.tar.gz" "hunspell.tar.gz"
  ExtractTar "hunspell.tar.gz" "hunspell"
  Set-Location "hunspell\hunspell-1.6.2"
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
  DownloadFile "https://ftp.pcre.org/pub/pcre/pcre-8.43.zip" "pcre.zip"
  ExtractZip "pcre.zip" "pcre"
  Set-Location pcre\pcre-8.43
  RunConfigure "--enable-utf --enable-unicode-properties --enable-pcre16 --prefix=$Env:MINGW_BASE_DIR_BASH"
  RunMake
  RunMakeInstall
}

function InstallSqlite() {
  DownloadFile "https://sqlite.org/2019/sqlite-autoconf-3280000.tar.gz" "sqlite.tar.gz"
  ExtractTar "sqlite.tar.gz" "sqlite"
  Set-Location sqlite\sqlite-autoconf-3280000
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
  DownloadFile "https://libzip.org/download/libzip-1.5.2.tar.gz" "libzip.tar.gz"
  ExtractTar "libzip.tar.gz" "libzip"
  Set-Location libzip\libzip-1.5.2
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

# Shouldn't be needed now:
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
  DownloadFile "http://luarocks.github.io/luarocks/releases/luarocks-3.1.2-win32.zip" "luarocks.zip"
  ExtractZip "luarocks.zip" "luarocks"
  Set-Location luarocks\luarocks-3.1.2-win32
  Step "installing luarocks"
  exec ".\install.bat" @("/P", "C:\LuaRocks", "/MW", "/Q")
  Set-Location \LuaRocks\lua\luarocks\core
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

function InstallLfs() {
  Set-Location \LuaRocks
  exec ".\luarocks" @("--tree=`"$Env:MINGW_BASE_DIR`"", "install", "LuaFileSystem")
}

function InstallLuasql() {
  Set-Location \LuaRocks
  exec ".\luarocks" @("--tree=`"$Env:MINGW_BASE_DIR`"", "install", "LuaSQL-SQLite3", "SQLITE_INCDIR=`"$Env:MINGW_BASE_DIR\include`"", "SQLITE_LIBDIR=`"$Env:MINGW_BASE_DIR\lib`"")
}

function InstallRexPcre() {
  Set-Location \LuaRocks
  exec ".\luarocks" @("--tree=`"$Env:MINGW_BASE_DIR`"", "install", "lrexlib-pcre", "PCRE_LIBDIR=`"$Env:MINGW_BASE_DIR\lib`"", "PCRE_INCDIR=`"$Env:MINGW_BASE_DIR\include`"")
}

function InstallLuaUtf8() {
  Set-Location \LuaRocks
  exec ".\luarocks" @("--tree=`"$Env:MINGW_BASE_DIR`"", "install", "luautf8")
}

function InstallLuaLunajson() {
  Set-Location \LuaRocks
  exec ".\luarocks" @("--tree=`"$Env:MINGW_BASE_DIR`"", "install", "lunajson")
}

function InstallLuaArgparse() {
  Set-Location \LuaRocks
  exec ".\luarocks" @("--tree=`"$Env:MINGW_BASE_DIR`"", "install", "argparse")
}

function InstallLuaYajl() {
  Set-Location \LuaRocks
  $Env:LIBRARY_PATH = "$Env:LIBRARY_PATH;$Env:MINGW_BASE_DIR/bin"
  exec ".\luarocks" @("--tree=`"$Env:MINGW_BASE_DIR`"", "install", "lua-yajl", "YAJL_LIBDIR=`"$Env:MINGW_BASE_DIR\bin`"", "YAJL_INCDIR=`"$Env:MINGW_BASE_DIR\include`"")
}

function InstallLuaZip () {
  Set-Location "$workingBaseDir"
  DownloadFile "https://github.com/rjpcomputing/luazip/archive/master.zip" "luazip.zip"
  # The above redirects to:
  # "https://codeload.github.com/mpeterv/luazip/zip/master.zip"
  # To avoid a dependency on zziplib we should switch to:
  # "https://codeload.github.com/brimworks/lua-zip/zip/v0.2.0"
  # TODO: it is not clear whether any extra tweaking, besides removing "-lzzip"
  # is needed for the above alternative:
  ExtractZip "luazip.zip" "luazip"
  Set-Location luazip\luazip-master
  Step "installing luazip"
  exec "gcc" @("-O2", "-c", "-o", "src/luazip.o", "-I`"$Env:MINGW_BASE_DIR/include`"", "src/luazip.c")
  exec "gcc" @("-shared", "-o", "zip.dll", "src/luazip.o", "-L`"$Env:MINGW_BASE_DIR/lib`"", "-lzzip", "-lz", "`"$Env:MINGW_BASE_DIR/bin/lua51.dll`"", "-lm")
  Copy-Item "zip.dll" "$Env:MINGW_BASE_DIR\lib\lua\5.1"
}

function InstallLuaModules(){
  CheckAndInstall "lfs" "$Env:MINGW_BASE_DIR\\lib\lua\5.1\lfs.dll" { InstallLfs }
  CheckAndInstall "luasql.sqlite3" "$Env:MINGW_BASE_DIR\\lib\lua\5.1\luasql\sqlite3.dll" { InstallLuasql }
  CheckAndInstall "rex.pcre" "$Env:MINGW_BASE_DIR\\lib\lua\5.1\rex_pcre.dll" { InstallRexPcre }
  CheckAndInstall "lua-utf8" "$Env:MINGW_BASE_DIR\\lib\lua\5.1\lua-utf8.dll" { InstallLuaUtf8 }
  CheckAndInstall "lua-yajl" "$Env:MINGW_BASE_DIR\\lib\lua\5.1\yajl.dll" { InstallLuaYajl }
  CheckAndInstall "luazip" "$Env:MINGW_BASE_DIR\\lib\lua\5.1\zip.dll" { InstallLuaZip }
  CheckAndInstall "argparse" "$Env:MINGW_BASE_DIR\\lib\lua\5.1\argparse" { InstallLuaArgparse }
  CheckAndInstall "lunajson" "$Env:MINGW_BASE_DIR\\lib\luarocks\rocks-5.1\lunajson" { InstallLuaLunajson }
}

function CheckAndInstall7z(){
    CheckAndInstall "7z" "C:\Program Files\7-Zip\7z.exe" { InstallSevenZ }
}

function CheckAndInstallCmake(){
    CheckAndInstall "cmake" "$CMakePath\cmake.exe" { InstallCmake }
}

function CheckAndInstallMingwGet(){
    CheckAndInstall "mingw-get" "C:\MinGW\bin\mingw-get.exe" { InstallMingwGet }
}

function CheckAndInstallMsys(){
    CheckAndInstall "MSYS and autotools" "C:\MinGW\bin\autoconf" { InstallMsys }
}

function CheckAndInstallBoost(){
    CheckAndInstall "Boost" "C:\Libraries\boost_1_71_0\bootstrap.bat" { InstallBoost }
}

function CheckAndInstallQt(){
    CheckAndInstall "Qt" "$Env:QT_BASE_DIR\bin\qmake.exe" { InstallQt }
}

function CheckAndInstallPython(){
    CheckAndInstall "Python" "C:\Python27\python.exe" { InstallPython }
}

function CheckAndInstallOpenSSL(){
    CheckAndInstall "openssl" "$Env:MINGW_BASE_DIR\bin\libssl-1_1.dll" { InstallOpenssl }
}

function CheckAndInstallHunspell(){
    CheckAndInstall "hunspell" "$Env:MINGW_BASE_DIR\bin\libhunspell-1.6-0.dll" { InstallHunspell }
}

function CheckAndInstallYajl(){
    CheckAndInstall "yajl" "$Env:MINGW_BASE_DIR\bin\libyajl.dll" { InstallYajl }
}

function CheckAndInstallLua(){
    CheckAndInstall "lua" "$Env:MINGW_BASE_DIR\bin\lua51.dll" { InstallLua }
}

function CheckAndInstallPcre(){
    CheckAndInstall "pcre" "$Env:MINGW_BASE_DIR\bin\libpcre-1.dll" { InstallPcre }
}

function CheckAndInstallSqlite(){
    CheckAndInstall "sqlite" "$Env:MINGW_BASE_DIR\lib\libsqlite3.a" { InstallSqlite }
}

function CheckAndInstallZlib(){
    CheckAndInstall "zlib" "$Env:MINGW_BASE_DIR\bin\zlib1.dll" { InstallZlib }
}

function CheckAndInstallLibzip(){
    CheckAndInstall "libzip" "$Env:MINGW_BASE_DIR\include\zipconf.h" { InstallLibzip }
}

function CheckAndInstallZziplib(){
    CheckAndInstall "zziplib" "$Env:MINGW_BASE_DIR\lib\libzzip.la" { InstallZziplib }
}

function CheckAndInstallLuarocks(){
    CheckAndInstall "luarocks" "C:\LuaRocks\luarocks.bat" { InstallLuarocks }
}

function CheckAndInstallPugixml(){
    CheckAndInstall "pugixml" "$Env:MINGW_BASE_DIR\lib\libpugixml.a" { InstallPugixml }
}
