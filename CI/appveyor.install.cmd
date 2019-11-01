REM To be called from the .appveyor.yml interpreter so that the environmental
REM variables it sets can be used to invoke an MSYS2/MINGW32/MINGW64 shell
REM from where we can run a *nix sh script...

ECHO Bitness is set to %BUILD_BITNESS% and the base folder where we are working will be "%APPVEYOR_BUILD_FOLDER%".
CD %APPVEYOR_BUILD_FOLDER%/CI
IF %BUILD_BITNESS%==32 GOTO BUILD32
IF %BUILD_BITNESS%==64 GOTO BUILD32

ECHO BUILD_BITNESS variable not set, aborting.
GOTO END

:BUILD32
REM Try for a 32 bit build and invoke
C:\msys64\mingw32.exe "appveyor.install.sh"

GOTO END

:BUILD64
SET MSYSTEM=MINGW64 & "C:\msys64\usr\bin\bash" --login -i "appveyor.install.sh"

GOTO END



:END
