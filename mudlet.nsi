; bigtest.nsi
;
; This script attempts to test most of the functionality of the NSIS exehead.

;--------------------------------

!ifdef HAVE_UPX
!packhdr tmp.dat "upx\upx -9 tmp.dat"
!endif

!ifdef NOCOMPRESS
SetCompress off
!endif

;--------------------------------

Name "Mudlet Installer"
Caption "Installer for Mudlet mud client"
Icon "src\win32-installer\mudlet_main_512x512.ico"
OutFile "mudlet-installer-3.0.exe"
BrandingText "Mudlet Installer"

SetDateSave on
SetDatablockOptimize on
CRCCheck on
SilentInstall normal
;BGGradient 000000 800000 FFFFFF
;InstallColors FF8080 000030
XPStyle on

InstallDir "$PROGRAMFILES\Mudlet"
InstallDirRegKey HKLM "Software\Mudlet\Mudletinst" "Install_Dir"

CheckBitmap "${NSISDIR}\Contrib\Graphics\Checks\simple-round.bmp"

LicenseText "GNU GENERAL PUBLIC LICENSE"
LicenseData "src\win32-installer\gpl.txt"

RequestExecutionLevel admin

VIAddVersionKey "ProductName" "Mudlet"
VIAddVersionKey "FileVersion" "3.0.0.0"
VIProductVersion "3.0.0.0"
	

;--------------------------------

Page license
Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

Section "" ; empty string makes it hidden, so would starting with -

  ; write uninstall strings
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Mudlet" "DisplayName" "Mudlet (remove only)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Mudlet" "UninstallString" '"$INSTDIR\uninst.exe"'

  SetOutPath $INSTDIR
  WriteUninstaller "uninst.exe"
  
  Nop ; for fun

SectionEnd

Function .onInit
 
  ReadRegStr $R0 HKLM \
  "Software\Microsoft\Windows\CurrentVersion\Uninstall\Mudlet" \
  "UninstallString"
  StrCmp $R0 "" done
 
  MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
  "Mudlet is already installed. $\n$\nClick `OK` to remove the \
  previous version or `Cancel` to cancel this upgrade." \
  IDOK uninst
  Abort
 
;Run the uninstaller
uninst:
  ClearErrors
  Exec $INSTDIR\uninst.exe
 
done:
 
FunctionEnd

Section "Install Mudlet"

  ;TODO: Make these read from environment first
  !define QTDIR "C:\Qt\5.3\mingw482_32\"
  !define MINGBIN "C:\mingw32\bin\"
  !define LUAROCKS "C:\Users\Chris\AppData\Roaming\LuaRocks\"
  !define MUDLETDIR "build-src-Desktop_Qt_5_3_MinGW_32bit-Release\release\"

  SetOutPath $INSTDIR
  File "src\LuaGlobal.lua"
  File "${QTDIR}bin\Qt5Core.dll"
  File "${QTDIR}bin\Qt5Gui.dll"
  File "${QTDIR}bin\Qt5Network.dll"
  File "${QTDIR}bin\Qt5Multimedia.dll"
  File "${QTDIR}bin\Qt5OpenGL.dll"
  File "${QTDIR}bin\Qt5Widgets.dll"
  File "${QTDIR}bin\libgcc_s_dw2-1.dll"
  File "${QTDIR}bin\libstdc++-6.dll"
  File "${QTDIR}bin\icuin52.dll"
  File "${QTDIR}bin\icuuc52.dll"
  File "${QTDIR}bin\icudt52.dll"
  File /oname=lua51.dll "${MINGBIN}lua51.dll"
  File /oname=lua5.1.dll "${MINGBIN}lua51.dll"
  File "${MINGBIN}zlib1.dll"
  File "${MINGBIN}libyajl.dll"
  File "${MINGBIN}libhunspell-1.3-0.dll"
  File "${MINGBIN}libpcre-1.dll"
  File "${MINGBIN}libzip-2.dll"
  File "${MINGBIN}libwinpthread-1.dll"
  File "${MINGBIN}libsqlite3-0.dll"
  File "${MUDLETDIR}mudlet.exe"
  File /r "${LUAROCKS}lib\lua\5.1\*"
  File "src\win32-installer\mudlet_main_512x512.ico"
  SetOutPath "$INSTDIR\mudlet-lua\lua"
  File /r /x .gitignore "src\mudlet-lua\lua\*"
  SetOutPath "$INSTDIR\platforms"
  File "${QTDIR}plugins\platforms\qwindows.dll"

SectionEnd

Section "Create Start Menu Shortcuts"

  Call CSCTest

SectionEnd

Section "Create Desktop Shortcut"

  Call CSCTest2

SectionEnd

;--------------------------------

Function "CSCTest"
  
  CreateDirectory "$SMPROGRAMS\Mudlet"
  SetOutPath $INSTDIR ; for working directory
  CreateShortCut "$SMPROGRAMS\Mudlet\Uninstall Mudlet.lnk" "$INSTDIR\uninst.exe" ; use defaults for parameters, icon, etc.
  ; this one will use notepad's icon, start it minimized, and give it a hotkey (of Ctrl+Shift+Q)
  CreateShortCut "$SMPROGRAMS\Mudlet\Mudlet.lnk" "$INSTDIR\mudlet.exe" "" "$INSTDIR\mudlet_main_512x512.ico" 0 SW_SHOWMAXIMIZED CONTROL|SHIFT|Z

FunctionEnd

Function "CSCTest2"
  
  SetOutPath $INSTDIR ; for working directory
  CreateShortCut "$DESKTOP\Mudlet.lnk" "$INSTDIR\mudlet.exe" "" "$INSTDIR\mudlet_main_512x512.ico" 0 SW_SHOWMAXIMIZED CONTROL|SHIFT|Z

FunctionEnd

;--------------------------------

; Uninstaller

UninstallText "This will uninstall Mudlet. Hit next to continue."
UninstallIcon "${NSISDIR}\Contrib\Graphics\Icons\nsis1-uninstall.ico"

Section "Uninstall"

  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Mudlet"
  DeleteRegKey HKLM "SOFTWARE\Mudlet\Mudlet"
  Delete /REBOOTOK "$INSTDIR\uninst.exe"
  Delete "$INSTDIR\test.ini"
  Delete "$SMPROGRAMS\Mudlet\*.*"
  Delete "$DESKTOP\Mudlet.lnk"
  RMDir "$SMPROGRAMS\Mudlet"
  RMDir /r "$INSTDIR"
  
  IfFileExists "$INSTDIR" 0 NoErrorMsg
    MessageBox MB_OK "Note: $INSTDIR could not be removed!" IDOK 0 ; skipped if file doesn't exist
  NoErrorMsg:

SectionEnd
