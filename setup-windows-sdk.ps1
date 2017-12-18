$sourceDir = $pwd.Path
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Unrestricted
If (-NOT ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator"))
{
    $arguments = ("-ExecutionPolicy", "Bypass", "cd $sourceDir\CI ; & '.\appveyor.install.ps1'")
    Start-Process powershell -Verb runAs -ArgumentList $arguments -Wait
}
else {
    CI\appveyor.install.ps1
}

cd "$sourceDir"

. CI\appveyor.set-environment.ps1
SetQtBaseDir "C:\src\verbose_output.log"
SetMingwBaseDir "C:\src\verbose_output.log"

Write-Output "Updating git submodules"
git submodule update --init --recursive

cd "$sourceDir\src"

Write-Output "Running qmake"
$Env:PATH="C:\Program Files (x86)\CMake\bin;C:\Program Files\7-Zip;$Env:QT_BASE_DIR\bin;$Env:MINGW_BASE_DIR\bin;" + (($Env:PATH.Split(';') | Where-Object { $_ -ne 'C:\Program Files\Git\usr\bin' }) -join ';')
qmake CONFIG+=debug mudlet.pro
if("$LastExitCode" -ne "0"){
  exit 1
}

Write-Output "Running make"
mingw32-make -j $(Get-WmiObject win32_processor | Select -ExpandProperty "NumberOfLogicalProcessors")
if("$LastExitCode" -ne "0"){
  exit 1
}

cd "$sourceDir\src\debug"

windeployqt.exe mudlet.exe
. "$sourceDir\CI\copy-non-qt-win-dependencies.ps1"

Start-Sleep 10
Start-Process mudlet.exe

cd $sourceDir