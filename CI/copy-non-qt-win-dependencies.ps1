if (Test-Path Env:APPVEYOR) {
  $Script:DllLocation = "$Env:MINGW_BASE_DIR\bin"
} else {
  $Script:DllLocation = "$Env:VCPKG_ROOT\installed\x64-mingw-dynamic\bin"
}

# Temporary debug
# Get-Childitem -Path $Env:VCPKG_ROOT -Recurse

COPY $Script:DllLocation\libyajl.dll .

# Find alternatives
# COPY $Script:DllLocation\libcrypto-1_1.dll .
# COPY $Script:DllLocation\libssl-1_1.dll .

# Theseare always installed via functions.ps1, so a static location
COPY $Env:MINGW_BASE_DIR\bin\libzip.dll .
COPY $Env:MINGW_BASE_DIR\bin\zlib1.dll .

if (Test-Path Env:APPVEYOR) { COPY $Script:DllLocation\lua51.dll . } Else { COPY $Script:DllLocation\liblua.dll . }
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
COPY ..\*.dic .
COPY ..\*.aff .
XCOPY /S /I /Q /Y $Env:MINGW_BASE_DIR\lib\lua\5.1 .

if (Test-Path Env:APPVEYOR) {
  COPY ..\..\3rdparty\discord\rpc\lib\discord-rpc32.dll discord-rpc32.dll
} Else {
  COPY $Env:GITHUB_WORKSPACE\3rdparty\discord\rpc\lib\discord-rpc32.dll discord-rpc32.dll
}
