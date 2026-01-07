# Header Include Optimization Review - Second Pass

This document tracks the review of header files for include optimization opportunities.
Goal: Reduce build times by minimizing unnecessary includes and using forward declarations.

## Optimization Strategies
1. **Move heavy includes from headers to .cpp files** - Only include what's needed for declarations
2. **Use forward declarations** - Declare classes instead of including their headers when only pointers/references are used
3. **Remove duplicate includes** - Some headers include the same file multiple times
4. **Remove unused includes** - Headers that aren't actually needed

## Priority Headers (High Impact)
These headers are included by many files, so optimizing them has cascading benefits.

| File | Status | Notes |
|------|--------|-------|
| Host.h | ✅ Done | 72 files - Many by-value members require full includes, limited optimization possible |
| TLuaInterpreter.h | ✅ Done | Removed TTextCodec.h, unused TTrigger forward decl |
| TMap.h | ✅ Done | Removed QJsonArray, QJsonDocument, QPixmap, QSizeF (moved to .cpp) |
| mudlet.h | ✅ Done | Removed edbee includes, TMediaData.h, TDetachedWindow.h (forward declared) |
| TConsole.h | ✅ Done | Removed QFile, QLabel, QHBoxLayout, QSplitter, QVideoWidget (forward declared) |
| TTextEdit.h | ⏳ Pending | |
| dlgTriggerEditor.h | ⏳ Pending | |
| TRoom.h | ⏳ Pending | |
| TArea.h | ⏳ Pending | |
| TRoomDB.h | ⏳ Pending | |
| TBuffer.h | ⏳ Pending | |
| TAlias.h | ⏳ Pending | |
| TTrigger.h | ⏳ Pending | |
| TEncodingTable.h | ⏳ Pending | |
| VarUnit.h | ⏳ Pending | |

## MXP Tag Handlers

| File | Status | Notes |
|------|--------|-------|
| TMxpBRTagHandler.h | ⏳ Pending | |
| TMxpColorTagHandler.h | ⏳ Pending | |
| TMxpCustomElementTagHandler.h | ⏳ Pending | |
| TMxpElementDefinitionHandler.h | ⏳ Pending | |
| TMxpFontTagHandler.h | ⏳ Pending | |
| TMxpLinkTagHandler.h | ⏳ Pending | |
| TMxpVersionTagHandler.h | ⏳ Pending | |

## Dialog Headers

| File | Status | Notes |
|------|--------|-------|
| dlgAboutDialog.h | ⏳ Pending | |
| dlgActionMainArea.h | ⏳ Pending | |
| dlgAliasMainArea.h | ⏳ Pending | |
| dlgColorTrigger.h | ⏳ Pending | |
| dlgComposer.h | ⏳ Pending | |
| dlgConnectionProfiles.h | ⏳ Pending | |
| dlgIRC.h | ⏳ Pending | |
| dlgKeysMainArea.h | ⏳ Pending | |
| dlgMapLabel.h | ⏳ Pending | |
| dlgMapper.h | ⏳ Pending | |
| dlgModuleManager.h | ⏳ Pending | |
| dlgNotepad.h | ⏳ Pending | |
| dlgPackageExporter.h | ⏳ Pending | |
| dlgPackageManager.h | ⏳ Pending | |
| dlgProfilePreferences.h | ⏳ Pending | |
| dlgRoomExits.h | ⏳ Pending | |
| dlgRoomProperties.h | ⏳ Pending | |
| dlgScriptsMainArea.h | ⏳ Pending | |
| dlgSourceEditorArea.h | ⏳ Pending | |
| dlgSourceEditorFindArea.h | ⏳ Pending | |
| dlgSystemMessageArea.h | ⏳ Pending | |
| dlgTimersMainArea.h | ⏳ Pending | |
| dlgTriggerPatternEdit.h | ⏳ Pending | |
| dlgTriggersMainArea.h | ⏳ Pending | |
| dlgVarsMainArea.h | ⏳ Pending | |

## T-Class Headers

| File | Status | Notes |
|------|--------|-------|
| TAstar.h | ⏳ Pending | |
| TAction.h | ⏳ Pending | |
| TAlias.h | ⏳ Pending | |
| TArea.h | ⏳ Pending | |
| TBuffer.h | ⏳ Pending | |
| TCommandLine.h | ⏳ Pending | |
| TConsole.h | ⏳ Pending | |
| TDebug.h | ⏳ Pending | |
| TDockWidget.h | ⏳ Pending | |
| TEntityHandler.h | ⏳ Pending | |
| TEntityResolver.h | ⏳ Pending | |
| TEvent.h | ⏳ Pending | |
| TFlipButton.h | ⏳ Pending | |
| TForkedProcess.h | ⏳ Pending | |
| TGameDetails.h | ⏳ Pending | |
| TKey.h | ⏳ Pending | |
| TLabel.h | ⏳ Pending | |
| TLuaInterpreter.h | ⏳ Pending | |
| TMainConsole.h | ⏳ Pending | |
| TMap.h | ⏳ Pending | |
| TMapLabel.h | ⏳ Pending | |
| TMedia.h | ⏳ Pending | |
| TMediaData.h | ⏳ Pending | |
| TMxpClient.h | ⏳ Pending | |
| TRoom.h | ⏳ Pending | |
| TRoomDB.h | ⏳ Pending | |
| TScript.h | ⏳ Pending | |
| TSplitter.h | ⏳ Pending | |
| TTabBar.h | ⏳ Pending | |
| TTextCodec.h | ⏳ Pending | |
| TTimer.h | ⏳ Pending | |
| TToolBar.h | ⏳ Pending | |
| TTrigger.h | ⏳ Pending | |
| TVar.h | ⏳ Pending | |

## Other Headers

| File | Status | Notes |
|------|--------|-------|
| ActionUnit.h | ⏳ Pending | |
| AliasUnit.h | ⏳ Pending | |
| ctelnet.h | ⏳ Pending | |
| Host.h | ⏳ Pending | |
| HostManager.h | ⏳ Pending | |
| KeyUnit.h | ⏳ Pending | |
| mudlet.h | ⏳ Pending | |
| ScriptUnit.h | ⏳ Pending | |
| TimerUnit.h | ⏳ Pending | |
| TriggerUnit.h | ⏳ Pending | |
| VarUnit.h | ⏳ Pending | |
