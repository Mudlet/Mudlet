ECHO "Bitness is set to %BUILD_BITNESS% and the base folder where we are working will be %APPVEYOR_BUILD_FOLDER%,"
ECHO "The current path is: %path%"

C:\msys64\msys2_shell.cmd -mingw%BUILD_BITNESS% -where %APPVEYOR_BUILD_FOLDER%\CI "appveyor.install.sh"
