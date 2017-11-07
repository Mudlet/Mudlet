cd "$Env:APPVEYOR_BUILD_FOLDER"

. CI\appveyor.set-environment.ps1
SetQtBaseDir
SetMingwBaseDir

git submodule update --init --recursive
. CI\appveyor.set-build-info.ps1

cd "$Env:APPVEYOR_BUILD_FOLDER\src"

$Env:PATH="C:\Program Files (x86)\CMake\bin;C:\Program Files\7-Zip;$Env:QT_BASE_DIR\bin;$Env:MINGW_BASE_DIR\bin;" + (($Env:PATH.Split(';') | Where-Object { $_ -ne 'C:\Program Files\Git\usr\bin' }) -join ';')
qmake CONFIG+=release LIBPATH+=$Env:MINGW_BASE_DIR\bin INCLUDEPATH+=C:\Libraries\boost_1_63_0 INCLUDEPATH+=$Env:MINGW_BASE_DIR\include INCLUDEPATH+=$Env:MINGW_BASE_DIR\lib\include mudlet.pro
if("$LastExitCode" -ne "0"){
  exit 1
}

mingw32-make -j $(Get-WmiObject win32_processor | Select -ExpandProperty "NumberOfLogicalProcessors")
if("$LastExitCode" -ne "0"){
  exit 1
}

cd "$Env:APPVEYOR_BUILD_FOLDER"

.\CI\appveyor.after_success.ps1
