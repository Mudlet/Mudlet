cd "$Env:APPVEYOR_BUILD_FOLDER\src"

. ..\CI\appveyor.set-build-info.ps1

$Env:PATH="C:\Program Files (x86)\CMake\bin;C:\Program Files\7-Zip;$Env:QT_BASE_DIR\bin;$Env:MINGW_BASE_DIR\bin;C:\MinGW\msys\1.0\bin;$Env:PATH"
qmake CONFIG+=release LIBPATH+=$Env:MINGW_BASE_DIR_BASH/bin INCLUDEPATH+=/c/Libraries/boost_1_63_0 INCLUDEPATH+=$Env:MINGW_BASE_DIR_BASH/include INCLUDEPATH+=$Env:MINGW_BASE_DIR_BASH/lib/include mudlet.pro

make -j 2