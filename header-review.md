# Header Include Optimization Review

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
| Host.h | ✅ Done | Removed XMLexport.h, added forward decl |
| TLuaInterpreter.h | ✅ Done (PR) | Removed TMap.h include |
| TMap.h | ✅ Done | Removed redundant QApplication |
| mudlet.h | ⏳ Pending | Main app singleton, 65 files |
| TConsole.h | ⏳ Pending | Text display, 40 files |
| TTextEdit.h | ⏳ Pending | Text editing widget |
| dlgTriggerEditor.h | ⚠️ Complex | Too many dependencies to remove from Host.h |
| TRoom.h | ✅ Done | QApplication → QCoreApplication, moved TMap.h to cpp |
| TArea.h | ⏳ Pending | Area data |
| TRoomDB.h | ✅ Done | Removed redundant QApplication |
| TBuffer.h | ✅ Done | Removed unused QApplication |
| TAlias.h | ✅ Done | Removed unused QApplication |
| TTrigger.h | ✅ Done | Removed unused QApplication |
| TEncodingTable.h | ✅ Done | Removed unused QApplication |
| VarUnit.h | ✅ Done | QApplication → QCoreApplication |

## MXP Tag Handlers (Duplicate Includes Fixed in PR)

| File | Status | Notes |
|------|--------|-------|
| TMxpBRTagHandler.h | ✅ Done (PR) | Removed duplicate TMxpTagHandler.h |
| TMxpColorTagHandler.h | ✅ Done (PR) | Removed duplicate TMxpTagHandler.h |
| TMxpCustomElementTagHandler.h | ✅ Done (PR) | Removed duplicate TMxpTagHandler.h |
| TMxpElementDefinitionHandler.h | ✅ Done (PR) | Removed duplicate TMxpTagHandler.h |
| TMxpFontTagHandler.h | ✅ Done (PR) | Removed duplicate TMxpTagHandler.h |
| TMxpLinkTagHandler.h | ✅ Done (PR) | Removed duplicate TMxpTagHandler.h |
| TMxpVersionTagHandler.h | ✅ Done (PR) | Removed duplicate TMxpTagHandler.h |

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
| dlgTriggerEditor.h | ⏳ Pending | |
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
| TLuaInterpreter.h | ✅ Done (PR) | |
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
| TTextEdit.h | ⏳ Pending | |
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

---

## Detailed Review Notes

### Host.h
**Status:** ✅ Partially Done
**Impact:** 72 files include this header
**Includes to review:**

| Include | Needed? | Reason | Action |
|---------|---------|--------|--------|
| ActionUnit.h | ✅ Yes | Member variable `mActionUnit` | Keep |
| AliasUnit.h | ✅ Yes | Member variable `mAliasUnit` | Keep |
| KeyUnit.h | ✅ Yes | Member variable `mKeyUnit` | Keep |
| ScriptUnit.h | ✅ Yes | Member variable `mScriptUnit` | Keep |
| GifTracker.h | ✅ Yes | Member variable `mGifTracker` | Keep |
| TCommandLine.h | ✅ Yes | Enum in method signatures | Keep |
| TLuaInterpreter.h | ✅ Yes | Member variable `mLuaInterpreter` | Keep |
| TimerUnit.h | ✅ Yes | Member variable `mTimerUnit` | Keep |
| TMainConsole.h | ⚠️ Maybe | Only QPointer used | Check if forward decl works |
| TriggerUnit.h | ✅ Yes | Member variable `mTriggerUnit` | Keep |
| XMLexport.h | ✅ **DONE** | Only friend declaration | **Replaced with forward decl** |
| ctelnet.h | ✅ Yes | Member variable `mTelnet` | Keep |
| dlgTriggerEditor.h | ⚠️ Heavy | Nested type `SearchOptions` + inline functions | Too complex to remove |
| enums.h | ✅ Yes | Various enums | Keep |
| TMxpMudlet.h | ✅ Yes | Member variable | Keep |
| TMxpProcessor.h | ✅ Yes | Member variable | Keep |
| TMxpFrameManager.h | ✅ Yes | Member variable | Keep |

**Completed optimizations:**
- ✅ XMLexport.h removed - saves pugixml.hpp from 72 compilation units

**Attempted but reverted:**
- dlgTriggerEditor.h - Too many implicit dependencies, would require extensive refactoring

**Files that needed explicit includes after XMLexport.h removal:**
- MudletInstanceCoordinator.h - needed QMutex
- TLuaInterpreterUI.cpp - needed QClipboard
- dlgPackageExporter.cpp - needed XMLexport.h
- dlgPackageExporter.h - needed QGroupBox
- dlgPackageManager.h - needed QJsonArray
- dlgRoomProperties.h - needed QListWidget

### TMap.h
**Status:** ⏳ Pending
**Current includes:**
- TAstar.h
- utils.h
- QApplication (heavy!)
- Many Qt headers

