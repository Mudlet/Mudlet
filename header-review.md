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
| TTextEdit.h | ✅ Done | Already optimized - uses forward declarations for Host, TBuffer, TConsole, TChar |
| dlgTriggerEditor.h | ✅ Done | Complex - has many transitive includes other files depend on, not safe to optimize |
| TRoom.h | ✅ Done | QVector3D needed for transitive includes (debug output), no optimization possible |
| TArea.h | ✅ Done | TMap.h needed for friend decls, TMapLabel.h for by-value member, no optimization possible |
| TRoomDB.h | ✅ Done | Already optimized with forward declarations |
| TBuffer.h | ✅ Done | Removed TTextCodec.h, QStringBuilder, QTime (moved to .cpp) |
| TAlias.h | ✅ Done | Already optimized - all includes needed for members |
| TTrigger.h | ✅ Done | Already optimized - all includes needed for members |
| TEncodingTable.h | ✅ Done | Already minimal |
| VarUnit.h | ✅ Done | Already minimal |

## MXP Tag Handlers

| File | Status | Notes |
|------|--------|-------|
| TMxpBRTagHandler.h | ✅ Done | Already minimal (only TMxpTagHandler.h) |
| TMxpColorTagHandler.h | ✅ Done | Already minimal (only TMxpTagHandler.h) |
| TMxpCustomElementTagHandler.h | ✅ Done | Removed TMxpElementRegistry.h (provided by TMxpContext.h) |
| TMxpElementDefinitionHandler.h | ✅ Done | Removed TMxpElementRegistry.h and utils.h (provided transitively) |
| TMxpFontTagHandler.h | ✅ Done | Already minimal (only TMxpTagHandler.h) |
| TMxpLinkTagHandler.h | ✅ Done | Already minimal (only TMxpTagHandler.h) |
| TMxpVersionTagHandler.h | ✅ Done | Already minimal (only TMxpTagHandler.h) |

## Dialog Headers

| File | Status | Notes |
|------|--------|-------|
| dlgAboutDialog.h | ✅ Done | Already minimal |
| dlgActionMainArea.h | ✅ Done | Already minimal (only UI file) |
| dlgAliasMainArea.h | ✅ Done | Already minimal (only UI file) |
| dlgColorTrigger.h | ✅ Done | Already optimized with forward declarations |
| dlgComposer.h | ✅ Done | Already optimized with forward declarations |
| dlgConnectionProfiles.h | ✅ Done | Already optimized with forward declarations |
| dlgIRC.h | ✅ Done | IRC library includes needed for types |
| dlgKeysMainArea.h | ✅ Done | Already minimal (only UI file) |
| dlgMapLabel.h | ✅ Done | Already optimized with forward declarations |
| dlgMapper.h | ✅ Done | Already optimized with forward declarations |
| dlgModuleManager.h | ✅ Done | Already optimized with forward declarations |
| dlgNotepad.h | ✅ Done | Removed QCloseEvent, QLabel, QLineEdit, QTimer (forward declared) |
| dlgPackageExporter.h | ✅ Done | Already optimized with forward declarations |
| dlgPackageManager.h | ✅ Done | QJsonArray needed for by-value member |
| dlgProfilePreferences.h | ✅ Done | Removed QtCore, 7 T* headers (forward declared) |
| dlgRoomExits.h | ✅ Done | Already optimized with forward declarations |
| dlgRoomProperties.h | ✅ Done | Already optimized with forward declarations |
| dlgScriptsMainArea.h | ✅ Done | Already minimal (only UI file) |
| dlgSourceEditorArea.h | ✅ Done | Already minimal (only UI file) |
| dlgSourceEditorFindArea.h | ✅ Done | Already minimal (only UI file) |
| dlgSystemMessageArea.h | ✅ Done | Already minimal (only UI file) |
| dlgTimersMainArea.h | ✅ Done | Already minimal (only UI file) |
| dlgTriggerPatternEdit.h | ✅ Done | Already minimal (only UI file) |
| dlgTriggersMainArea.h | ✅ Done | Already minimal (only UI file) |
| dlgVarsMainArea.h | ✅ Done | Already minimal (only UI file) |

## T-Class Headers

Note: High-impact T-Class headers were reviewed in Priority Headers section above.

| File | Status | Notes |
|------|--------|-------|
| TAstar.h | ✅ Done | Minimal - only QList needed |
| TAction.h | ✅ Done | All includes needed for members |
| TAlias.h | ✅ Done | See Priority Headers |
| TArea.h | ✅ Done | See Priority Headers |
| TBuffer.h | ✅ Done | See Priority Headers |
| TCommandLine.h | ✅ Done | All includes needed |
| TConsole.h | ✅ Done | See Priority Headers |
| TDebug.h | ✅ Done | Minimal |
| TDockWidget.h | ✅ Done | Minimal |
| TEntityHandler.h | ✅ Done | Minimal |
| TEntityResolver.h | ✅ Done | Minimal |
| TEvent.h | ✅ Done | Minimal |
| TFlipButton.h | ✅ Done | Minimal |
| TForkedProcess.h | ✅ Done | Uses forward declarations |
| TGameDetails.h | ✅ Done | Minimal |
| TKey.h | ✅ Done | All includes needed for members |
| TLabel.h | ✅ Done | Uses forward declarations |
| TLuaInterpreter.h | ✅ Done | See Priority Headers |
| TMainConsole.h | ✅ Done | All includes needed |
| TMap.h | ✅ Done | See Priority Headers |
| TMapLabel.h | ✅ Done | Minimal |
| TMedia.h | ✅ Done | All includes needed |
| TMediaData.h | ✅ Done | All includes needed |
| TMxpClient.h | ✅ Done | Uses forward declarations |
| TRoom.h | ✅ Done | See Priority Headers |
| TRoomDB.h | ✅ Done | See Priority Headers |
| TScript.h | ✅ Done | All includes needed for members |
| TSplitter.h | ✅ Done | Minimal |
| TTabBar.h | ✅ Done | Minimal |
| TTextCodec.h | ✅ Done | Minimal |
| TTimer.h | ✅ Done | All includes needed for members |
| TToolBar.h | ✅ Done | Uses forward declarations |
| TTrigger.h | ✅ Done | See Priority Headers |
| TVar.h | ✅ Done | Minimal |

## Other Headers

Note: High-impact headers were reviewed in Priority Headers section above.

| File | Status | Notes |
|------|--------|-------|
| ActionUnit.h | ✅ Done | All includes needed |
| AliasUnit.h | ✅ Done | All includes needed |
| ctelnet.h | ✅ Done | Uses forward declarations |
| Host.h | ✅ Done | See Priority Headers |
| HostManager.h | ✅ Done | Minimal |
| KeyUnit.h | ✅ Done | All includes needed |
| mudlet.h | ✅ Done | See Priority Headers |
| ScriptUnit.h | ✅ Done | All includes needed |
| TimerUnit.h | ✅ Done | All includes needed |
| TriggerUnit.h | ✅ Done | All includes needed |
| VarUnit.h | ✅ Done | See Priority Headers |
