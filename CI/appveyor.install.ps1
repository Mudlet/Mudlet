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

# Switch to another Qt version w/o waiting for an AppVeyor update. Make sure update QT_BASE_DIR in .appveyor.yml
# iex ((new-object net.webclient).DownloadString('https://raw.githubusercontent.com/appveyor/build-images/master/scripts/Windows/install_qt_module.ps1'))
# Install-QtComponent -Version '5.14.1' -Name 'win32_mingw73' -ExcludeDocs -ExcludeExamples
# ConfigureQtVersion 'C:\Qt' '5.14.1'

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
CheckAndInstallZziplib
CheckAndInstallLuarocks
CheckAndInstallPugixml
InstallLuaModules
