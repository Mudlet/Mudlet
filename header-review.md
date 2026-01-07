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
| mudlet.h | ✅ Partially Done | Moved system memory headers to cpp (65 files) |
| TConsole.h | ✅ Done | Removed redundant TTextCodec.h (via TBuffer.h) |
| TTextEdit.h | ✅ Done | Removed TBuffer.h, use forward decl |
| dlgTriggerEditor.h | ⚠️ Complex | Too many dependencies to remove from Host.h |
| TRoom.h | ✅ Done | QApplication → QCoreApplication, moved TMap.h to cpp |
| TArea.h | ✅ Reviewed | Needs TMap.h for friend decls, TMapLabel.h for member |
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
| dlgAboutDialog.h | ✅ Reviewed | Already minimal |
| dlgActionMainArea.h | ✅ Reviewed | Already minimal |
| dlgAliasMainArea.h | ✅ Done | Removed duplicate TrailingWhitespaceMarker.h |
| dlgColorTrigger.h | ✅ Reviewed | Already minimal |
| dlgComposer.h | ✅ Reviewed | Already minimal |
| dlgConnectionProfiles.h | ✅ Done | Moved pugixml.hpp to cpp, added forward decl |
| dlgIRC.h | ✅ Done | Moved IrcTextFormat, IrcUser to cpp |
| dlgKeysMainArea.h | ✅ Reviewed | Already minimal |
| dlgMapLabel.h | ✅ Done | Remove Qt dialog includes, use forward decls |
| dlgMapper.h | ✅ Done | Removed unused QDir, QMainWindow |
| dlgModuleManager.h | ✅ Done | Removed Host.h, QCloseEvent, use forward decls |
| dlgNotepad.h | ✅ Done | Removed unused QCheckBox, QSettings |
| dlgPackageExporter.h | ✅ Done | Remove Host.h, QCloseEvent, QGroupBox, use forward decls |
| dlgPackageManager.h | ✅ Done | Removed redundant Host.h, added QNetworkReply forward decl |
| dlgProfilePreferences.h | ⚠️ Complex | Too many implicit dependencies via mudlet.h |
| dlgRoomExits.h | ✅ Done | Remove QCheckBox, use forward decl |
| dlgRoomProperties.h | ✅ Done | Removed Host.h, use forward decl |
| dlgScriptsMainArea.h | ✅ Reviewed | Already minimal |
| dlgSourceEditorArea.h | ✅ Done | Move utils.h to cpp |
| dlgSourceEditorFindArea.h | ✅ Done | Remove QTextEdit, QKeyEvent, add forward decl |
| dlgSystemMessageArea.h | ✅ Reviewed | Already minimal |
| dlgTimersMainArea.h | ✅ Reviewed | Already minimal |
| dlgTriggerEditor.h | ⏳ Pending | |
| dlgTriggerPatternEdit.h | ✅ Reviewed | Uses QPalette for members, already optimized |
| dlgTriggersMainArea.h | ✅ Reviewed | Already minimal |
| dlgVarsMainArea.h | ✅ Reviewed | Already minimal |

## T-Class Headers

| File | Status | Notes |
|------|--------|-------|
| TAstar.h | ✅ Reviewed | Needs TRoom.h for template code accessing members |
| TAction.h | ✅ Reviewed | Already minimal, uses forward decls |
| TAlias.h | ✅ Reviewed | Already minimal, uses forward decl for Host |
| TArea.h | ✅ Reviewed | Needs TMap.h for friend decls, TMapLabel.h for member |
| TBuffer.h | ✅ Done | Removed unused TMxpMudlet.h, TMxpProcessor.h |
| TCommandLine.h | ✅ Done | TConsole.h → forward decl + utils.h |
| TConsole.h | ✅ Done | Removed redundant TTextCodec.h (via TBuffer.h) |
| TDebug.h | ✅ Reviewed | Already minimal, utils.h for qsl(), forward decl Host |
| TDockWidget.h | ✅ Done | Removed mudlet.h, Host.h, TConsole.h, QtEvents → forward decls |
| TEntityHandler.h | ✅ Reviewed | Already minimal, needs TEntityResolver.h for ref member |
| TEntityResolver.h | ✅ Reviewed | Already minimal |
| TEvent.h | ✅ Reviewed | Already minimal, needs QDebug for debug operator |
| TFlipButton.h | ✅ Reviewed | Already minimal, uses forward decls |
| TForkedProcess.h | ✅ Done | Removed TLuaInterpreter.h, use forward decls |
| TGameDetails.h | ✅ Reviewed | Needs utils.h for qsl macro |
| TKey.h | ✅ Done | Removed unused lua.h |
| TLabel.h | ✅ Done | Removed unused TEvent.h |
| TLuaInterpreter.h | ✅ Done (PR) | |
| TMainConsole.h | ⚠️ Complex | TScrollBox.h removal causes cascading issues |
| TMap.h | ✅ Reviewed | Heavy header, uses forward decls, needs TAstar.h |
| TMapLabel.h | ✅ Reviewed | Already minimal |
| TMedia.h | ⚠️ Complex | Host.h removal causes circular dependency issues |
| TMediaData.h | ✅ Reviewed | Already minimal |
| TMxpClient.h | ✅ Done | Removed unused utils.h |
| TRoom.h | ✅ Reviewed | Already minimal, uses forward decls |
| TRoomDB.h | ✅ Reviewed | Already minimal, uses forward decls |
| TScript.h | ✅ Reviewed | Already minimal, forward decls for Host, TEvent |
| TSplitter.h | ✅ Reviewed | Already minimal |
| TTabBar.h | ✅ Done | Removed QDrag, QMimeData, QApplication, utils.h → cpp |
| TTextCodec.h | ✅ Reviewed | Already minimal |
| TTimer.h | ✅ Reviewed | Already minimal, forward decls for Host, QTimer |
| TToolBar.h | ✅ Reviewed | Already minimal, uses forward decls |
| TTrigger.h | ✅ Reviewed | Already minimal, forward decls for Host, TLuaInterpreter, TMatchState |
| TVar.h | ✅ Reviewed | Already minimal, needs lua.h for LUA_T* constants |

## Other Headers

| File | Status | Notes |
|------|--------|-------|
| ActionUnit.h | ✅ Reviewed | Already optimized, needs utils.h for TreeItemInsertMode |
| AliasUnit.h | ✅ Reviewed | Already optimized, needs utils.h for TreeItemInsertMode |
| ctelnet.h | ✅ Reviewed | Complex networking, uses forward decls, needs system headers |
| Host.h | ✅ Partially Done | See Priority Headers section for details |
| HostManager.h | ✅ Done | Host.h → forward decl |
| KeyUnit.h | ✅ Reviewed | Already minimal, utils.h for TreeItemInsertMode |
| mudlet.h | ✅ Partially Done | Moved system memory headers to cpp |
| ScriptUnit.h | ✅ Reviewed | Already minimal, uses forward decls |
| TimerUnit.h | ✅ Reviewed | Already minimal, forward decls for Host, TTimer, QTimer |
| TriggerUnit.h | ✅ Reviewed | Already optimized, needs utils.h for TreeItemInsertMode |
| VarUnit.h | ✅ Reviewed | Already minimal, forward decls for TVar, QTreeWidgetItem |

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

