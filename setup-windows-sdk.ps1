$sourceDir = $pwd.Path
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Unrestricted
If (-NOT ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator"))
{
    $arguments = ("-ExecutionPolicy", "Bypass", "-NoExit", "cd $sourceDir\CI ; & '.\appveyor.install.ps1'")
    Start-Process powershell -Verb runAs -ArgumentList $arguments -Wait
}
else {
    CI\appveyor.install.ps1
}

cd "$sourceDir"

. CI\appveyor.set-environment.ps1
SetQtBaseDir "C:\src\verbose_output.log"
SetMingwBaseDir "C:\src\verbose_output.log"

git submodule update --init --recursive

cd "$sourceDir\src"

$Env:PATH="C:\Program Files (x86)\CMake\bin;C:\Program Files\7-Zip;$Env:QT_BASE_DIR\bin;$Env:MINGW_BASE_DIR\bin;" + (($Env:PATH.Split(';') | Where-Object { $_ -ne 'C:\Program Files\Git\usr\bin' }) -join ';')
qmake CONFIG+=debug LIBPATH+=$Env:MINGW_BASE_DIR\bin INCLUDEPATH+=C:\Libraries\boost_1_60_0 INCLUDEPATH+=$Env:MINGW_BASE_DIR\include INCLUDEPATH+=$Env:MINGW_BASE_DIR\lib\include mudlet.pro
if("$LastExitCode" -ne "0"){
  exit 1
}

mingw32-make -j $(Get-WmiObject win32_processor | Select -ExpandProperty "NumberOfLogicalProcessors")
if("$LastExitCode" -ne "0"){
  exit 1
}

cd "$sourceDir\src\debug"

windeployqt.exe mudlet.exe
COPY $Env:MINGW_BASE_DIR\lib\libyajl.dll .
COPY C:\src\lua-5.1.5\lua-5.1.5\src\lua51.dll .
COPY C:\src\openssl-1.0.2l\libeay32.dll .
COPY C:\src\openssl-1.0.2l\ssleay32.dll .
COPY $Env:MINGW_BASE_DIR\bin\libzip-5.dll .
COPY $Env:MINGW_BASE_DIR\bin\libhunspell-1.4-0.dll .
COPY $Env:MINGW_BASE_DIR\bin\libpcre-1.dll .
COPY $Env:MINGW_BASE_DIR\bin\libsqlite3-0.dll .
COPY $Env:MINGW_BASE_DIR\bin\zlib1.dll .
XCOPY /S /I /Q ..\mudlet-lua mudlet-lua
COPY ..\*.dic .
COPY C:\src\luazip\luazip-master\zip.dll .
XCOPY /S /I /Q $Env:MINGW_BASE_DIR\lib\lua\5.1 .

Start-Process mudlet.exe

cd $sourceDir