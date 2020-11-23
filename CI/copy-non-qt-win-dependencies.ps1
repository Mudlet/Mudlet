if (!(Test-Path Env:VCPKG_ROOT)) {
  $Script:DllLocation = "$Env:MINGW_BASE_DIR\bin"
} else {
  $Script:DllLocation = "$Env:VCPKG_ROOT\installed\x64-mingw-dynamic\bin"
}

$Script:SourceFolder = If (Test-Path Env:APPVEYOR) { $Env:APPVEYOR_BUILD_FOLDER } Else { "$Env:GITHUB_WORKSPACE" };

COPY $Env:MINGW_BASE_DIR\bin\libyajl.dll .

# Find alternatives
# COPY $Script:DllLocation\libcrypto-1_1.dll .
# COPY $Script:DllLocation\libssl-1_1.dll .

# These are always installed via functions.ps1, so a static location
COPY $Env:MINGW_BASE_DIR\bin\libzip.dll .
COPY $Env:MINGW_BASE_DIR\bin\zlib1.dll .
COPY $Env:MINGW_BASE_DIR\bin\lua51.dll .

# vcpkg seems to produce liblibhunspell.dll, https://github.com/microsoft/vcpkg/issues/14606
if (Test-Path Env:APPVEYOR) { COPY $Script:DllLocation\libhunspell-1.6-0.dll . } Else { COPY $Script:DllLocation\liblibhunspell.dll . }
if (Test-Path Env:APPVEYOR) { COPY $Script:DllLocation\libpcre-1.dll . } Else { COPY $Script:DllLocation\libpcre.dll . }

if (Test-Path Env:APPVEYOR) {
  XCOPY /S /I /Q /Y ..\mudlet-lua mudlet-lua
  XCOPY /S /I /Q /Y ..\..\translations\lua translations\lua
  XCOPY /S /I /Q /Y ..\..\3rdparty\lcf lcf
} Else {
  XCOPY /S /I /Q /Y $Env:GITHUB_WORKSPACE\src\mudlet-lua mudlet-lua
  XCOPY /S /I /Q /Y $Env:GITHUB_WORKSPACE\translations\lua translations\lua
  XCOPY /S /I /Q /Y $Env:GITHUB_WORKSPACE\3rdparty\lcf lcf
}

if (Test-Path Env:APPVEYOR) {
  COPY ..\*.dic .
  COPY ..\*.aff .
} else {
  COPY $Env:GITHUB_WORKSPACE\src\*.dic .
  COPY $Env:GITHUB_WORKSPACE\src\*.aff .
}

# copy in Luarocks-related DLL's
echo "debug 1"
Get-ChildItem -Recurse $Env:MINGW_BASE_DIR\lib\lua\5.1
echo "debug 2"
XCOPY /S /I /Q /Y $Env:MINGW_BASE_DIR\lib\lua\5.1 .

if (Test-Path Env:APPVEYOR) {
  COPY ..\..\3rdparty\discord\rpc\lib\discord-rpc32.dll discord-rpc32.dll
} Else {
  COPY $Env:GITHUB_WORKSPACE\3rdparty\discord\rpc\lib\discord-rpc64.dll discord-rpc64.dll
}

# cmake-specific DLLs?
if (Test-Path Env:GITHUB_WORKSPACE) {
  COPY $Env:BUILD_FOLDER\3rdparty\edbee-lib\edbee-lib\qslog\bin\libQsLog.dll .
  COPY $Env:BUILD_FOLDER\3rdparty\qtkeychain\libqt5keychain.dll .
  COPY $Env:QT_BASE_DIR\bin\libgcc_s_seh-1.dll .
  COPY $Env:QT_BASE_DIR\bin\libstdc++-6.dll .
  COPY $Env:QT_BASE_DIR\bin\libwinpthread-1.dll .
  COPY $Script:DllLocation\libpugixml.dll
}
