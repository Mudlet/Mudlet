$sourceDir = $pwd.Path
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Unrestricted
If (-NOT ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator"))
{
    $arguments = ("-ExecutionPolicy", "Bypass", "cd `"$sourceDir\CI`" ; & '.\appveyor.install.ps1'")
    Start-Process powershell -Verb runAs -ArgumentList $arguments -Wait
}
else {
    cd CI
    .\appveyor.install.ps1
}

cd "$sourceDir"

. CI\appveyor.set-environment.ps1
SetQtBaseDir "C:\src\verbose_output.log"
SetMingwBaseDir "C:\src\verbose_output.log"
SetLuarocksPath "C:\src\verbose_output.log"

if(-NOT (Test-Path "$sourceDir\build")){
    New-Item -ItemType Directory "$sourceDir\build"
}

cd "$sourceDir\build"

Write-Output "Running qmake"
$Env:PATH="C:\Program Files (x86)\CMake\bin;C:\Program Files\7-Zip;$Env:QT_BASE_DIR\bin;$Env:MINGW_BASE_DIR\bin;" + (($Env:PATH.Split(';') | Where-Object { $_ -ne 'C:\Program Files\Git\usr\bin' }) -join ';')
qmake CONFIG+=debug ../src/mudlet.pro
if("$LastExitCode" -ne "0"){
  exit 1
}

Write-Output "Running make"
mingw32-make -j $(Get-WmiObject win32_processor | Select -ExpandProperty "NumberOfLogicalProcessors")
if("$LastExitCode" -ne "0"){
  exit 1
}

cd "$sourceDir\build"
Start-Process -wait debug\mudlet.exe

cd $sourceDir