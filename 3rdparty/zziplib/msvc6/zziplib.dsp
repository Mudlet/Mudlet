# Microsoft Developer Studio Project File - Name="zziplib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=zziplib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "zziplib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "zziplib.mak" CFG="zziplib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "zziplib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "zziplib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "zziplib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release/zziplib"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I ".." /I "d:\include" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x41d /d "NDEBUG"
# ADD RSC /l 0x41d /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "zziplib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug/zziplib"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I ".." /I "d:\include" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x41d /d "_DEBUG"
# ADD RSC /l 0x41d /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "zziplib - Win32 Release"
# Name "zziplib - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "c;cpp"
# Begin Source File

SOURCE="..\zzip\dir.c"
# End Source File
# Begin Source File

SOURCE="..\zzip\err.c"
# End Source File
# Begin Source File

SOURCE="..\zzip\file.c"
# End Source File
# Begin Source File

SOURCE="..\zzip\info.c"
# End Source File
# Begin Source File

SOURCE="..\zzip\plugin.c"
# End Source File
# Begin Source File

SOURCE="..\zzip\stat.c"
# End Source File
# Begin Source File

SOURCE="..\zzip\write.c"
# End Source File
# Begin Source File

SOURCE="..\zzip\zip.c"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h"
# Begin Source File

SOURCE="..\zzip\__dirent.h"
# End Source File
# Begin Source File

SOURCE="..\zzip\__mmap.h"
# End Source File
# Begin Source File

SOURCE="..\zzip\_config.h"
# End Source File
# Begin Source File

SOURCE="..\zzip\_msvc.h"
# End Source File
# Begin Source File

SOURCE="..\zzip\conf.h"
# End Source File
# Begin Source File

SOURCE="..\zzip\file.h"
# End Source File
# Begin Source File

SOURCE=..\zzip\format.h
# End Source File
# Begin Source File

SOURCE=..\zzip\lib.h
# End Source File
# Begin Source File

SOURCE="..\zzip\plugin.h"
# End Source File
# Begin Source File

SOURCE="..\zzip\stdint.h"
# End Source File
# Begin Source File

SOURCE=..\zzip\zzip.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\docs\README.MSVC6
# End Source File
# End Target
# End Project
