ECHO "Bitness is set to %BUILD_BITNESS% and the base folder where we are working will be %APPVEYOR_BUILD_FOLDER% ."
CD %APPVEYOR_BUILD_FOLDER%/CI
IF %BUILD_BITNESS%==32 (C:\msys64\msys2_shell.cmd -mingw32 "appveyor.install.sh") ELSE IF %BUILD_BITNESS%==64 (C:\msys64\msys2_shell.cmd -mingw64 "appveyor.install.sh") ELSE EXIT /B -1
