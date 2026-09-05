<!-- GENERATED FILE - do not edit by hand. -->
<!-- Regenerate: bash cmake/audit-core-widgets.sh > docs/libmudlet-widgets-report.md -->

# libmudlet: Qt Widgets dependency audit (`mudlet_core`)

Measures how many source files in the `mudlet_core` static-library target
(`src/CMakeLists.txt`) still depend on Qt Widgets. Part of the libmudlet
refactor (#8681, #9011): the goal is to drive this count to **0** so `mudlet_core`
can build with Qt Widgets absent, after which this audit becomes an enforcing CI
guard (`--enforce`).

A file is counted as depending on Qt Widgets if it either:

- directly `#include`s a QtWidgets-module header (bare `<QWidget>`,
  `<qwidget.h>`, or module-qualified `<QtWidgets/...>` / `<QtWidgets>`), or
- references a QtWidgets class symbol (e.g. `QApplication`, `QSizePolicy`) even
  when the header arrives transitively - those symbols are what break once Qt
  Widgets is removed.

The QtWidgets header and class sets are derived from the installed Qt's module
layout (headers in `QtWidgets/` that are not also in `QtGui/`/`QtCore/`), so
Qt6 relocations such as `QAction`/`QShortcut` -> QtGui are excluded
automatically. The offending-file count is stable across Qt 6.x releases.

Genuine widget classes (the `dlg*`, `T*`-widget, and `mudlet` UI files) are
included in the count; the refactor plan moves those wholesale to a future
`mudlet_app` target rather than de-widgeting them, so the count drops through a
mix of moving and refactoring.

**Regenerate** - baseline first, so the report records it. Write through a temporary
file: a redirect truncates its target before the audit runs, so a fatal error would
leave the baseline empty and every later run would abort on it.

```sh
bash cmake/audit-core-widgets.sh --count > baseline.tmp && mv baseline.tmp cmake/core-widgets-baseline.txt
bash cmake/audit-core-widgets.sh > report.tmp && mv report.tmp docs/libmudlet-widgets-report.md
```

Nothing gates on this count yet: steps 3-10 of the refactor legitimately move files
between the core and app targets, so an intermediate step can correctly raise it.
`--enforce` becomes a CI gate once the count reaches **0**. Until then this report
and the baseline are regenerated in each libmudlet PR, so drift shows up in the diff.

## Summary

| Metric | Count |
| --- | ---: |
| Source files in `mudlet_core` | 421 |
| Files depending on Qt Widgets | 149 |
| Clean files | 272 |
| Committed baseline | 149 |

## Offending files

Sorted by total references (includes + symbols), then by path. `Inc` = direct
QtWidgets header includes; `Sym` = QtWidgets class-symbol references.

| File | Inc | Sym | Widget references |
| --- | ---: | ---: | --- |
| `dlgTriggerEditor.cpp` | 16 | 607 | QApplication, QCheckBox, QAbstractButton, QColorDialog, QDialogButtonBox, QFileDialog, QFrame, QHBoxLayout, QLabel, QMessageBox, QScrollBar, QSpinBox, QStyle, QToolButton, QToolBar, QVBoxLayout, QStatusBar, QWidget, QSizePolicy, QGroupBox, QLineEdit, QListWidget, QPushButton, QMainWindow, QSplitter, QTreeWidget, QComboBox, QTimeEdit, QPlainTextEdit, QMenu, QTreeWidgetItem, QTreeWidgetItemIterator, QListWidgetItem |
| `dlgProfilePreferences.cpp` | 19 | 579 | QAbstractScrollArea, QAbstractSpinBox, QApplication, QColorDialog, QDoubleSpinBox, QFileDialog, QFontDialog, QMessageBox, QTableWidget, QToolBar, QLineEdit, QHBoxLayout, QListWidget, QScrollArea, QScrollBar, QStackedWidget, QStyle, QStyledItemDelegate, QToolButton, QWidget, QDialog, QRadioButton, QCheckBox, QAbstractButton, QComboBox, QPushButton, QLayout, QLayoutItem, QLabel, QSizePolicy, QVBoxLayout, QListWidgetItem, QFrame, QStyleOptionViewItem, QGroupBox, QGridLayout, QBoxLayout, QFontComboBox, QStyleOptionGroupBox, QSpinBox, QDateTimeEdit, QMenu, QKeySequenceEdit, QTableWidgetItem |
| `T2DMap.cpp` | 12 | 143 | QMenu, QWidget, QAbstractItemView, QDialog, QLabel, QListWidget, QVBoxLayout, QHBoxLayout, QPushButton, QInputDialog, QMessageBox, QComboBox, QSizePolicy, QAbstractScrollArea, QFrame, QHeaderView, QTreeWidget, QTreeWidgetItem, QLineEdit, QCheckBox, QAbstractButton, QGridLayout, QFileDialog, QDialogButtonBox, QListWidgetItem, QColorDialog |
| `dlgRoomExits.cpp` | 0 | 152 | QStyledItemDelegate, QWidget, QStyleOptionViewItem, QSpinBox, QGroupBox, QLineEdit, QDialog, QTreeWidgetItem, QCheckBox, QRadioButton, QAbstractButton, QTreeWidget, QButtonGroup |
| `dlgPackageExporter.cpp` | 3 | 136 | QFileDialog, QInputDialog, QMessageBox, QWidget, QDialog, QTreeWidgetItem, QTreeWidget, QDialogButtonBox, QAbstractButton, QPushButton, QLineEdit, QTextEdit, QComboBox, QTreeWidgetItemIterator, QApplication, QListView, QAbstractItemView, QTreeView |
| `mudlet.cpp` | 11 | 102 | QApplication, QFileDialog, QMessageBox, QScrollBar, QSplitter, QStyleFactory, QTableWidget, QToolBar, QToolButton, QToolTip, QStyle, QMenu, QWidget, QMainWindow, QSizePolicy, QTabBar, QVBoxLayout, QHBoxLayout, QDockWidget, QDialog, QLabel |
| `dlgConnectionProfiles.cpp` | 3 | 102 | QApplication, QColorDialog, QTabBar, QWidget, QDialog, QAbstractItemView, QAbstractButton, QDialogButtonBox, QPushButton, QTextBrowser, QLineEdit, QCheckBox, QPlainTextEdit, QListWidget, QListView, QListWidgetItem, QGroupBox, QVBoxLayout, QLabel, QMenu, QFileDialog |
| `TConsole.cpp` | 8 | 95 | QAccessibleWidget, QFrame, QHBoxLayout, QLabel, QLineEdit, QMessageBox, QScrollBar, QSplitter, QWidget, QToolButton, QSizePolicy, QVBoxLayout, QAbstractButton, QMenu, QAbstractSlider, QApplication, QStyle |
| `dlgProfilePreferences.h` | 1 | 89 | QDialog, QDoubleSpinBox, QFrame, QListWidget, QListWidgetItem, QScrollArea, QStackedWidget, QToolButton, QVBoxLayout, QWidget, QLineEdit, QPushButton, QComboBox, QGroupBox, QCheckBox, QLabel, QMenu |
| `dlgTriggerEditor.h` | 5 | 67 | QDialog, QDockWidget, QListWidgetItem, QScrollArea, QTreeWidget, QLabel, QFrame, QToolButton, QMainWindow, QTreeWidgetItem, QWidget, QToolBar, QSplitter |
| `dlgPackageExporter.h` | 2 | 59 | QDialog, QTextEdit, QGroupBox, QTreeWidget, QTreeWidgetItem, QWidget, QPushButton |
| `TDetachedWindow.cpp` | 11 | 49 | QVBoxLayout, QMenuBar, QMenu, QApplication, QToolBar, QToolButton, QLabel, QStackedWidget, QSizePolicy, QWidget, QDockWidget, QMainWindow, QTabBar |
| `dlgRoomProperties.cpp` | 2 | 51 | QColorDialog, QMenu, QWidget, QDialog, QLineEdit, QComboBox, QAbstractButton, QSpinBox, QListWidgetItem, QVBoxLayout, QSizePolicy, QListWidget, QListView, QHBoxLayout, QPushButton |
| `dlgMapper.cpp` | 10 | 41 | QFileDialog, QFrame, QLabel, QListWidget, QMenu, QMessageBox, QProgressBar, QProgressDialog, QPushButton, QVBoxLayout, QWidget, QAbstractButton, QToolButton, QComboBox, QApplication, QDialog, QSizePolicy |
| `dlgRoomExits.h` | 2 | 49 | QDialog, QStyledItemDelegate, QCheckBox, QWidget, QStyleOptionViewItem, QLineEdit, QTreeWidgetItem, QRadioButton, QSpinBox |
| `TMainConsole.cpp` | 8 | 39 | QDialog, QDockWidget, QLabel, QLineEdit, QMessageBox, QProgressDialog, QScrollBar, QSizePolicy, QWidget, QApplication |
| `dlgNotepad.cpp` | 9 | 36 | QApplication, QHBoxLayout, QInputDialog, QLabel, QLineEdit, QMenu, QPlainTextEdit, QTabBar, QToolButton, QWidget, QTabWidget, QMainWindow, QTextEdit |
| `dlgPackageManager.cpp` | 3 | 42 | QFileDialog, QMessageBox, QProgressDialog, QWidget, QDialog, QLineEdit, QListWidget, QAbstractButton, QListWidgetItem, QButtonGroup |
| `TTreeWidget.cpp` | 2 | 38 | QHeaderView, QToolTip, QWidget, QTreeWidget, QAbstractItemView, QTreeWidgetItem, QStyle |
| `updater/UpdateDialog.cpp` | 6 | 34 | QAbstractButton, QApplication, QLabel, QMessageBox, QTextBrowser, QToolButton, QWidget, QDialog, QPushButton, QCheckBox |
| `mudlet.h` | 3 | 36 | QDockWidget, QMainWindow, QSystemTrayIcon, QMenu, QLabel, QListWidget, QPushButton, QSplitter, QTableWidget, QTableWidgetItem, QTextEdit, QToolButton, QToolBar, QWidget, QHBoxLayout |
| `TMxpFrameManager.cpp` | 4 | 34 | QFrame, QMainWindow, QSizePolicy, QVBoxLayout, QTabWidget, QWidget |
| `dlgMapLabel.cpp` | 3 | 34 | QColorDialog, QFileDialog, QFontDialog, QWidget, QDialog, QComboBox, QToolButton, QCheckBox, QPlainTextEdit, QPushButton, QApplication |
| `TTextEdit.cpp` | 6 | 31 | QApplication, QScrollBar, QLabel, QLineEdit, QToolTip, QWidgetAction, QWidget, QAbstractSlider, QMenu |
| `TConsole.h` | 1 | 31 | QWidget, QHBoxLayout, QLineEdit, QScrollBar, QSplitter, QToolButton |
| `dlgColorTrigger.cpp` | 0 | 31 | QWidget, QDialog, QDialogButtonBox, QAbstractButton, QAbstractSlider, QPushButton |
| `dlgModuleManager.cpp` | 2 | 28 | QFileDialog, QMessageBox, QWidget, QDialog, QAbstractButton, QTableWidget, QHeaderView, QTableWidgetItem |
| `TDebugFilterBar.cpp` | 7 | 23 | QComboBox, QCompleter, QLabel, QLineEdit, QMenu, QStyle, QToolButton, QWidget, QToolBar |
| `TMapView.cpp` | 4 | 26 | QApplication, QHBoxLayout, QLabel, QVBoxLayout, QWidget, QSizePolicy, QToolButton, QComboBox, QAbstractButton |
| `TTabBar.cpp` | 3 | 26 | QApplication, QStyleOption, QStyleOptionTab, QWidget, QStyle, QProxyStyle, QTabBar |
| `TUiTour.cpp` | 8 | 21 | QFrame, QHBoxLayout, QLabel, QMenu, QMenuBar, QPushButton, QToolBar, QVBoxLayout, QWidget |
| `TCommandLine.cpp` | 3 | 22 | QLineEdit, QScrollBar, QToolButton, QWidget, QPlainTextEdit, QFrame, QApplication, QMenu |
| `TDetachedWindow.h` | 7 | 16 | QMainWindow, QVBoxLayout, QToolBar, QToolButton, QLabel, QStackedWidget, QDockWidget, QMenu |
| `T2DMap.h` | 2 | 19 | QTreeWidget, QWidget, QCheckBox, QComboBox, QListWidgetItem, QPushButton, QTreeWidgetItem, QMenu, QDialog |
| `dlgTriggerPatternEdit.cpp` | 8 | 11 | QAbstractButton, QAbstractItemView, QAbstractScrollArea, QAbstractSpinBox, QPlainTextEdit, QComboBox, QLineEdit, QWidget |
| `widgetutils.h` | 2 | 17 | QApplication, QWidget |
| `TEasyButtonBar.cpp` | 1 | 17 | QGridLayout, QWidget, QVBoxLayout, QSizePolicy, QAbstractButton, QPushButton |
| `TFlipButton.cpp` | 3 | 15 | QMenu, QStyleOptionButton, QStylePainter, QPushButton, QSizePolicy, QStyle |
| `TToolBar.cpp` | 0 | 18 | QWidget, QDockWidget, QGridLayout, QSizePolicy, QAbstractButton, QPushButton |
| `TTabBar.h` | 2 | 15 | QProxyStyle, QTabBar, QStyleOption, QWidget, QStyleOptionTab |
| `dlgConnectionProfiles.h` | 0 | 16 | QTabBar, QDialog, QWidget, QListWidgetItem, QListWidget, QLabel, QPushButton |
| `TFeatureCallout.cpp` | 4 | 12 | QHBoxLayout, QLabel, QPushButton, QVBoxLayout, QWidget |
| `TLabel.cpp` | 0 | 16 | QWidget, QLabel |
| `main.cpp` | 3 | 12 | QCheckBox, QMessageBox, QSplashScreen, QApplication |
| `dlgNotepad.h` | 0 | 14 | QLabel, QLineEdit, QPlainTextEdit, QToolButton, QMainWindow, QWidget |
| `dlgMapper.h` | 0 | 13 | QFrame, QLabel, QProgressBar, QPushButton, QWidget, QMenu |
| `TDebugFilterBar.h` | 1 | 12 | QToolBar, QLabel, QLineEdit, QMenu, QToolButton, QWidget |
| `TUiTour.h` | 1 | 11 | QWidget, QFrame, QLabel, QPushButton |
| `updater.cpp` | 2 | 10 | QMessageBox, QPushButton, QApplication, QAbstractButton |
| `TMainConsole.h` | 1 | 10 | QWidget, QDialog, QDockWidget, QProgressDialog |
| `updater/UpdateDialog.h` | 1 | 10 | QDialog, QAbstractButton, QLabel, QWidget |
| `VarUnit.cpp` | 1 | 10 | QTreeWidgetItem |
| `VarUnit.h` | 0 | 11 | QTreeWidgetItem |
| `TMapView.h` | 3 | 7 | QComboBox, QToolButton, QWidget |
| `TTreeWidget.h` | 1 | 9 | QTreeWidget, QWidget, QTreeWidgetItem |
| `dlgMapLabel.h` | 1 | 8 | QDialog, QColorDialog, QFontDialog, QWidget |
| `PackageItemDelegate.cpp` | 1 | 8 | QApplication, QStyledItemDelegate, QStyleOptionViewItem, QStyle |
| `AltFocusMenuBarDisable.cpp` | 0 | 8 | QProxyStyle, QStyleFactory, QStyleOption, QWidget, QStyleHintReturn, QStyle |
| `dlgPackageManager.h` | 4 | 4 | QButtonGroup, QDialog, QListWidget, QTextBrowser, QWidget, QListWidgetItem |
| `TAction.cpp` | 1 | 7 | QMenu |
| `TKeySequenceEdit.cpp` | 1 | 7 | QLineEdit, QWidget, QKeySequenceEdit |
| `TMxpFrameManager.h` | 5 | 3 | QBoxLayout, QHBoxLayout, QTabWidget, QVBoxLayout, QWidget |
| `dlgComposer.cpp` | 1 | 6 | QMenu, QAbstractButton, QWidget, QMainWindow |
| `TDockWidget.cpp` | 0 | 7 | QDockWidget, QWidget |
| `AltFocusMenuBarDisable.h` | 2 | 4 | QProxyStyle, QStyleFactory, QStyleOption, QWidget, QStyleHintReturn |
| `dlgIRC.cpp` | 1 | 5 | QScrollBar, QLineEdit, QTextBrowser, QListView |
| `dlgVarsMainArea.cpp` | 1 | 5 | QListWidgetItem, QWidget, QListWidget |
| `SingleLineTextEdit.cpp` | 0 | 6 | QWidget, QPlainTextEdit |
| `TAccessibleTextEdit.h` | 1 | 5 | QAccessibleWidget, QWidget |
| `TCommandLine.h` | 2 | 4 | QPlainTextEdit, QToolButton, QWidget, QMenu |
| `TEasyButtonBar.h` | 1 | 5 | QWidget, QGridLayout |
| `TToolBar.h` | 1 | 5 | QDockWidget, QGridLayout, QWidget |
| `ActionUnit.cpp` | 3 | 2 | QDockWidget, QLayout, QWidget |
| `dlgActionMainArea.cpp` | 0 | 5 | QWidget, QLineEdit, QSpinBox, QComboBox |
| `dlgModuleManager.h` | 1 | 4 | QDialog, QWidget, QTableWidgetItem |
| `LuaInterface.h` | 0 | 5 | QTreeWidgetItem |
| `PackageItemDelegate.h` | 2 | 3 | QStyledItemDelegate, QStyleOptionViewItem |
| `TAccessibleConsole.h` | 1 | 4 | QAccessibleWidget, QWidget |
| `TFeatureCallout.h` | 1 | 4 | QWidget |
| `TKeySequenceEdit.h` | 1 | 4 | QKeySequenceEdit, QLineEdit, QWidget |
| `TrailingWhitespaceMarker.cpp` | 1 | 4 | QLineEdit, QPlainTextEdit |
| `TrailingWhitespaceMarker.h` | 1 | 4 | QLineEdit, QPlainTextEdit |
| `TScrollBox.h` | 1 | 4 | QScrollArea, QWidget |
| `DarkTheme.h` | 2 | 2 | QProxyStyle, QStyleFactory, QStyle |
| `dlgAliasMainArea.cpp` | 0 | 4 | QWidget, QLineEdit |
| `dlgRoomProperties.h` | 1 | 3 | QListWidget, QDialog, QWidget, QListWidgetItem |
| `dlgScriptsMainArea.cpp` | 0 | 4 | QWidget, QLineEdit |
| `exitstreewidget.cpp` | 0 | 4 | QWidget, QTreeWidget, QAbstractItemView, QTreeWidgetItem |
| `LuaInterface.cpp` | 0 | 4 | QTreeWidgetItem |
| `RoomContextMenuHandler.cpp` | 1 | 3 | QMenu |
| `TAction.h` | 0 | 4 | QMenu |
| `TMapViewManager.cpp` | 0 | 4 | QDockWidget |
| `TScrollBox.cpp` | 0 | 4 | QWidget, QScrollArea |
| `TSplitter.h` | 1 | 3 | QSplitter, QWidget, QSplitterHandle |
| `TTextEdit.h` | 1 | 3 | QWidget, QScrollBar |
| `updater.h` | 0 | 4 | QAbstractButton, QPushButton |
| `WideComboBox.h` | 1 | 3 | QComboBox |
| `DarkTheme.cpp` | 0 | 3 | QStyle, QProxyStyle |
| `dlgColorTrigger.h` | 0 | 3 | QDialog, QWidget, QPushButton |
| `dlgComposer.h` | 0 | 3 | QMenu, QMainWindow |
| `dlgKeysMainArea.cpp` | 0 | 3 | QWidget, QLineEdit |
| `dlgTimersMainArea.cpp` | 0 | 3 | QWidget, QLineEdit |
| `dlgTriggersMainArea.cpp` | 0 | 3 | QWidget, QLineEdit |
| `exitstreewidget.h` | 1 | 2 | QTreeWidget, QWidget |
| `RoomContextMenuHandler.h` | 0 | 3 | QMenu |
| `SingleLineTextEdit.h` | 1 | 2 | QPlainTextEdit, QWidget |
| `TAccessibleTextEdit.cpp` | 0 | 3 | QAccessibleWidget |
| `TFlipButton.h` | 1 | 2 | QPushButton, QStyleOptionButton |
| `TLabel.h` | 1 | 2 | QLabel, QWidget |
| `TLuaInterpreterMudletObjects.cpp` | 1 | 2 | QFileDialog |
| `TSplitter.cpp` | 0 | 3 | QWidget, QSplitter, QSplitterHandle |
| `TTextBox.cpp` | 0 | 3 | QWidget, QPlainTextEdit, QFrame |
| `TTextBox.h` | 1 | 2 | QPlainTextEdit, QWidget |
| `WideComboBox.cpp` | 2 | 1 | QScrollBar, QAbstractItemView, QComboBox |
| `CustomLineDrawContextMenuHandler.cpp` | 1 | 1 | QMenu |
| `CustomLineEditContextMenuHandler.cpp` | 1 | 1 | QMenu |
| `dlgAboutDialog.cpp` | 0 | 2 | QWidget, QDialog |
| `dlgAboutDialog.h` | 0 | 2 | QDialog, QWidget |
| `dlgActionMainArea.h` | 0 | 2 | QWidget |
| `dlgAliasMainArea.h` | 0 | 2 | QWidget |
| `dlgKeysMainArea.h` | 0 | 2 | QWidget |
| `dlgScriptsMainArea.h` | 0 | 2 | QWidget |
| `dlgSourceEditorArea.cpp` | 0 | 2 | QWidget |
| `dlgSourceEditorArea.h` | 0 | 2 | QWidget |
| `dlgSourceEditorFindArea.cpp` | 0 | 2 | QWidget |
| `dlgSourceEditorFindArea.h` | 0 | 2 | QWidget |
| `dlgSystemMessageArea.cpp` | 0 | 2 | QWidget |
| `dlgSystemMessageArea.h` | 0 | 2 | QWidget |
| `dlgTimersMainArea.h` | 0 | 2 | QWidget |
| `dlgTriggerPatternEdit.h` | 0 | 2 | QWidget |
| `dlgTriggersMainArea.h` | 0 | 2 | QWidget |
| `dlgVarsMainArea.h` | 0 | 2 | QWidget |
| `glwidget_integration.h` | 0 | 2 | QWidget |
| `Host.cpp` | 1 | 1 | QApplication, QDockWidget |
| `Host.h` | 0 | 2 | QDockWidget |
| `LabelInteractionHandler.cpp` | 1 | 1 | QMenu |
| `modern_glwidget.cpp` | 0 | 2 | QWidget |
| `SelectionRectangleHandler.cpp` | 1 | 1 | QTreeWidgetItem |
| `TDockWidget.h` | 1 | 1 | QDockWidget |
| `TLuaInterpreter.cpp` | 1 | 1 | QApplication |
| `TMapViewManager.h` | 1 | 1 | QDockWidget |
| `TSplitterHandle.cpp` | 0 | 2 | QSplitterHandle, QSplitter |
| `TSplitterHandle.h` | 1 | 1 | QSplitterHandle |
| `dlgIRC.h` | 0 | 1 | QMainWindow |
| `glwidget.cpp` | 0 | 1 | QWidget |
| `glwidget.h` | 0 | 1 | QWidget |
| `glwidget_integration.cpp` | 0 | 1 | QWidget |
| `HostManager.cpp` | 0 | 1 | QApplication |
| `modern_glwidget.h` | 0 | 1 | QWidget |

## Clean files (272)

<details>
<summary>Files with no Qt Widgets dependency</summary>

- `../3rdparty/discord/rpc/include/discord_register.h`
- `../3rdparty/discord/rpc/include/discord_rpc.h`
- `../3rdparty/kdtoolbox/singleshot_connect/singleshot_connect.h`
- `ActionUnit.h`
- `AliasUnit.cpp`
- `AliasUnit.h`
- `CameraController.cpp`
- `CameraController.h`
- `CredentialManager.cpp`
- `CredentialManager.h`
- `ctelnet.cpp`
- `ctelnet.h`
- `CustomLineDrawContextMenuHandler.h`
- `CustomLineDrawHandler.cpp`
- `CustomLineDrawHandler.h`
- `CustomLineEditContextMenuHandler.h`
- `CustomLineEditHandler.cpp`
- `CustomLineEditHandler.h`
- `CustomLineSession.cpp`
- `CustomLineSession.h`
- `discord.cpp`
- `discord.h`
- `EAction.cpp`
- `EAction.h`
- `EditorAddItemCommand.cpp`
- `EditorAddItemCommand.h`
- `EditorCommand.h`
- `EditorDeleteItemCommand.cpp`
- `EditorDeleteItemCommand.h`
- `EditorItemXMLHelpers.cpp`
- `EditorItemXMLHelpers.h`
- `EditorModifyPropertyCommand.cpp`
- `EditorModifyPropertyCommand.h`
- `EditorMoveItemCommand.cpp`
- `EditorMoveItemCommand.h`
- `EditorToggleActiveCommand.cpp`
- `EditorToggleActiveCommand.h`
- `EditorUndoStack.cpp`
- `EditorUndoStack.h`
- `enums.h`
- `EventLoopPump.cpp`
- `EventLoopPump.h`
- `FileOpenHandler.cpp`
- `FileOpenHandler.h`
- `FontManager.cpp`
- `FontManager.h`
- `GeometryManager.cpp`
- `GeometryManager.h`
- `GifTracker.cpp`
- `GifTracker.h`
- `GMCPAuthenticator.cpp`
- `GMCPAuthenticator.h`
- `HostManager.h`
- `ircmessageformatter.cpp`
- `ircmessageformatter.h`
- `KeyUnit.cpp`
- `KeyUnit.h`
- `LabelInteractionHandler.h`
- `LabelTextureCache.cpp`
- `LabelTextureCache.h`
- `LuaLiteral.cpp`
- `LuaLiteral.h`
- `MacMicrophonePermission.h`
- `MacMicrophonePermission.mm`
- `mapInfoContributorManager.cpp`
- `mapInfoContributorManager.h`
- `MiddleMousePanHandler.cpp`
- `MiddleMousePanHandler.h`
- `MMCP.h`
- `MMCPClient.cpp`
- `MMCPClient.h`
- `MMCPServer.cpp`
- `MMCPServer.h`
- `MudletInstanceCoordinator.cpp`
- `MudletInstanceCoordinator.h`
- `MxpTag.cpp`
- `MxpTag.h`
- `OAuthClientFlow.cpp`
- `OAuthClientFlow.h`
- `PanInteractionHandler.cpp`
- `PanInteractionHandler.h`
- `RenderCommand.cpp`
- `RenderCommand.h`
- `RenderCommandQueue.cpp`
- `RenderCommandQueue.h`
- `ResourceManager.cpp`
- `ResourceManager.h`
- `RoomMoveActivationHandler.cpp`
- `RoomMoveActivationHandler.h`
- `RoomMoveDragHandler.cpp`
- `RoomMoveDragHandler.h`
- `ScriptUnit.cpp`
- `ScriptUnit.h`
- `SecureStringUtils.cpp`
- `SecureStringUtils.h`
- `SelectionRectangleHandler.h`
- `SentryWrapper.cpp`
- `SentryWrapper.h`
- `ShaderManager.cpp`
- `ShaderManager.h`
- `ShortcutsManager.cpp`
- `ShortcutsManager.h`
- `sparkleupdater.h`
- `sparkleupdater.mm`
- `SpeechAudioCapture.cpp`
- `SpeechAudioCapture.h`
- `SpeechRecognizer.h`
- `SpeechRecognizerFactory.cpp`
- `SpeechRecognizerFactory.h`
- `TAlias.cpp`
- `TAlias.h`
- `TArea.cpp`
- `TArea.h`
- `TAreaGridIndex.cpp`
- `TAreaGridIndex.h`
- `TAreaLodExitIndex.cpp`
- `TAreaLodExitIndex.h`
- `TAreaSpanIndex.cpp`
- `TAreaSpanIndex.h`
- `TAreaZLevelIndex.cpp`
- `TAreaZLevelIndex.h`
- `TAstar.h`
- `TBuffer.cpp`
- `TBuffer.h`
- `TConsoleModel.cpp`
- `TConsoleModel.h`
- `TDebug.cpp`
- `TDebug.h`
- `TEncodingHelper.cpp`
- `TEncodingHelper.h`
- `TEncodingTable.cpp`
- `TEncodingTable.h`
- `TEntityHandler.cpp`
- `TEntityHandler.h`
- `TEntityResolver.cpp`
- `TEntityResolver.h`
- `TEvent.h`
- `TForkedProcess.cpp`
- `TForkedProcess.h`
- `TGameDetails.h`
- `THyperlinkCompactManager.cpp`
- `THyperlinkCompactManager.h`
- `THyperlinkSelectionManager.cpp`
- `THyperlinkSelectionManager.h`
- `THyperlinkStyling.h`
- `THyperlinkVisibilityManager.cpp`
- `THyperlinkVisibilityManager.h`
- `TimerUnit.cpp`
- `TimerUnit.h`
- `TKey.cpp`
- `TKey.h`
- `TLabelModel.cpp`
- `TLabelModel.h`
- `TLinkStore.cpp`
- `TLinkStore.h`
- `TLuaInterpreter.h`
- `TLuaInterpreterDiscord.cpp`
- `TLuaInterpreterMapper.cpp`
- `TLuaInterpreterMedia.cpp`
- `TLuaInterpreterMMCP.cpp`
- `TLuaInterpreterNetworking.cpp`
- `TLuaInterpreterSpeechToText.cpp`
- `TLuaInterpreterTextToSpeech.cpp`
- `TLuaInterpreterUI.cpp`
- `TMap.cpp`
- `TMap.h`
- `TMapLabel.cpp`
- `TMapLabel.h`
- `TMatchState.h`
- `TMedia.cpp`
- `TMedia.h`
- `TMediaData.h`
- `TMediaPlaylist.cpp`
- `TMediaPlaylist.h`
- `TMxpBRTagHandler.cpp`
- `TMxpBRTagHandler.h`
- `TMxpClient.h`
- `TMxpColorTagHandler.cpp`
- `TMxpColorTagHandler.h`
- `TMxpContext.h`
- `TMxpCustomElementTagHandler.cpp`
- `TMxpCustomElementTagHandler.h`
- `TMxpDestTagHandler.cpp`
- `TMxpDestTagHandler.h`
- `TMxpElementDefinitionHandler.cpp`
- `TMxpElementDefinitionHandler.h`
- `TMxpElementRegistry.cpp`
- `TMxpElementRegistry.h`
- `TMxpEntityTagHandler.cpp`
- `TMxpEntityTagHandler.h`
- `TMxpEvent.h`
- `TMxpExpireTagHandler.cpp`
- `TMxpExpireTagHandler.h`
- `TMxpFontTagHandler.cpp`
- `TMxpFontTagHandler.h`
- `TMxpFormattingTagsHandler.cpp`
- `TMxpFormattingTagsHandler.h`
- `TMxpFrameTagHandler.cpp`
- `TMxpFrameTagHandler.h`
- `TMxpHRTagHandler.cpp`
- `TMxpHRTagHandler.h`
- `TMxpImageTagHandler.cpp`
- `TMxpImageTagHandler.h`
- `TMxpLinkTagHandler.cpp`
- `TMxpLinkTagHandler.h`
- `TMxpMudlet.cpp`
- `TMxpMudlet.h`
- `TMxpMusicTagHandler.cpp`
- `TMxpMusicTagHandler.h`
- `TMxpNodeBuilder.cpp`
- `TMxpNodeBuilder.h`
- `TMxpProcessor.cpp`
- `TMxpProcessor.h`
- `TMxpSendTagHandler.cpp`
- `TMxpSendTagHandler.h`
- `TMxpSoundTagHandler.cpp`
- `TMxpSoundTagHandler.h`
- `TMxpStatTagHandler.cpp`
- `TMxpStatTagHandler.h`
- `TMxpSupportTagHandler.cpp`
- `TMxpSupportTagHandler.h`
- `TMxpTagHandler.cpp`
- `TMxpTagHandler.h`
- `TMxpTagHandlerResult.h`
- `TMxpTagParser.cpp`
- `TMxpTagParser.h`
- `TMxpTagProcessor.cpp`
- `TMxpTagProcessor.h`
- `TMxpVarTagHandler.cpp`
- `TMxpVarTagHandler.h`
- `TMxpVersionTagHandler.cpp`
- `TMxpVersionTagHandler.h`
- `TPrintSink.h`
- `Tree.h`
- `TriggerHighlighter.cpp`
- `TriggerHighlighter.h`
- `TriggerUnit.cpp`
- `TriggerUnit.h`
- `TRoom.cpp`
- `TRoom.h`
- `TRoomDB.cpp`
- `TRoomDB.h`
- `TScript.cpp`
- `TScript.h`
- `TStringUtils.cpp`
- `TStringUtils.h`
- `TTextCodec.cpp`
- `TTextCodec.h`
- `TTextProperties.h`
- `TTimer.cpp`
- `TTimer.h`
- `TTrigger.cpp`
- `TTrigger.h`
- `TVar.cpp`
- `TVar.h`
- `TWindowRegistry.h`
- `UntrustedText.cpp`
- `UntrustedText.h`
- `updater/Feed.cpp`
- `updater/Feed.h`
- `updater/Release.cpp`
- `updater/Release.h`
- `updater/SemVer.cpp`
- `updater/SemVer.h`
- `utils.h`
- `VoskRecognizer.cpp`
- `VoskRecognizer.h`
- `widechar_width.h`
- `XMLexport.cpp`
- `XMLexport.h`
- `XMLimport.cpp`
- `XMLimport.h`

</details>
