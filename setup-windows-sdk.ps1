$sourceDir = $pwd.Path
If (-NOT ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator"))
{
    $arguments = "& 'CI\appveyor.install.ps1'"
    Start-Process powershell -Verb runAs -ArgumentList $arguments
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
qmake CONFIG+=debug LIBPATH+=$Env:MINGW_BASE_DIR\bin INCLUDEPATH+=C:\Libraries\boost_1_63_0 INCLUDEPATH+=$Env:MINGW_BASE_DIR\include INCLUDEPATH+=$Env:MINGW_BASE_DIR\lib\include mudlet.pro
if("$LastExitCode" -ne "0"){
  exit 1
}

mingw32-make -j $(Get-WmiObject win32_processor | Select -ExpandProperty "NumberOfLogicalProcessors")
if("$LastExitCode" -ne "0"){
  exit 1
}

cd "$sourceDir\src\debug"

.\mudlet.exe