COPY $Env:MINGW_BASE_DIR\lib\libyajl.dll .
COPY C:\src\lua-5.1.5\lua-5.1.5\src\lua51.dll .
COPY C:\src\openssl-1.0.2l\libeay32.dll .
COPY C:\src\openssl-1.0.2l\ssleay32.dll .
COPY $Env:MINGW_BASE_DIR\bin\libzip-5.dll .
COPY $Env:MINGW_BASE_DIR\bin\libhunspell-1.4-0.dll .
COPY $Env:MINGW_BASE_DIR\bin\libpcre-1.dll .
COPY $Env:MINGW_BASE_DIR\bin\libsqlite3-0.dll .
COPY $Env:MINGW_BASE_DIR\bin\zlib1.dll .
XCOPY /S /I /Q /Y ..\mudlet-lua mudlet-lua
COPY ..\*.dic .
COPY C:\src\luazip\luazip-master\zip.dll .
XCOPY /S /I /Q /Y $Env:MINGW_BASE_DIR\lib\lua\5.1 .
XCOPY /S /I /Q /Y ..\..\3rdparty\lcf lcf
