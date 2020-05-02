COPY $Env:MINGW_BASE_DIR\bin\libyajl.dll .
COPY $Env:MINGW_BASE_DIR\bin\lua51.dll .
COPY $Env:MINGW_BASE_DIR\bin\libcrypto-1_1.dll .
COPY $Env:MINGW_BASE_DIR\bin\libssl-1_1.dll .
COPY $Env:MINGW_BASE_DIR\bin\libzip.dll .
COPY $Env:MINGW_BASE_DIR\bin\libhunspell-1.6-0.dll .
COPY $Env:MINGW_BASE_DIR\bin\libpcre-1.dll .
COPY $Env:MINGW_BASE_DIR\bin\zlib1.dll .
XCOPY /S /I /Q /Y ..\mudlet-lua mudlet-lua
XCOPY /S /I /Q /Y ..\..\translations\lua translations\lua
COPY ..\*.dic .
COPY ..\*.aff .
XCOPY /S /I /Q /Y $Env:MINGW_BASE_DIR\lib\lua\5.1 .
XCOPY /S /I /Q /Y ..\..\3rdparty\lcf lcf
COPY ..\..\3rdparty\discord\rpc\lib\discord-rpc32.dll discord-rpc32.dll
