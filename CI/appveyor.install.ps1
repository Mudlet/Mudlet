# Exit the script whenever an error in a cmdlet occurs
$global:ErrorActionPreference = "Stop"

# activate higher TLS version. Seems PS only uses 1.0 by default
# credit: https://stackoverflow.com/questions/41618766/powershell-invoke-webrequest-fails-with-ssl-tls-secure-channel/48030563#48030563
[Net.ServicePointManager]::SecurityProtocol = [System.Security.Authentication.SslProtocols] "tls, tls11, tls12"

. .\appveyor.functions.ps1

SetQtBaseDir "$logFile"

$Env:PATH = "$CMakePath;C:\MinGW\bin;C:\MinGW\msys\1.0\bin;C:\Program Files\7-Zip;$Env:PATH"

# install dependencies

CheckAndInstall7z
CheckAndInstallCmake
CheckAndInstallMingwGet
CheckAndInstallMsys
CheckAndInstallBoost
CheckAndInstallQt
CheckAndInstallPython

# Adapt the PATH variable again as we may have installed some dependencies just now and can determine their location.
SetMingwBaseDir "$logFile"
$ShPath = "$Env:MINGW_BASE_DIR\bin;C:\Python27;$Env:PATH"
$Env:PATH = $ShPath
# Filter PATH because cmake complains if an sh.exe being in the PATH and it's instructed to create MinGW makefiles.
# But on the other hand we keep sh.exe in the PATH to easily run "configure" scripts. So we create 2 variables and assign PATH accordingly.
$NoShPath = filterPathForSh

CheckAndInstallOpenSSL
CheckAndInstallHunspell
CheckAndInstallYajl
CheckAndInstallLua
CheckAndInstallPcre
CheckAndInstallSqlite
CheckAndInstallZlib
CheckAndInstallLibzip
# Shouldn't be needed as it is only used for the luazip module from:
# https://github.com/mpeterv/luazip and we can use lua-zip from:
# https://github.com/brimworks/lua-zip :
CheckAndInstallZziplib
CheckAndInstallLuarocks
CheckAndInstallPugixml
InstallLuaModules
