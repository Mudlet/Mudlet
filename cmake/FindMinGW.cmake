FIND_FILE(CMAKE_MINGW_DLL mingwm10.dll PATHS
  "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MinGW;InstallLocation]/bin"
  c:/MinGW/bin /MinGW/bin)

MARK_AS_ADVANCED(CMAKE_MINGW_DLL)

