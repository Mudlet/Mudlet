#ifndef MUDLET_DLGTRIGGEREDITOR_H
#define MUDLET_DLGTRIGGEREDITOR_H

/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017-2020 by Ian Adkins - ieadkins@gmail.com            *
 *   Copyright (C) 2015-2018, 2020, 2022-2023 by Stephen Lyons             *
 *                                               - slysven@virginmedia.com *
 *   Copyright (C) 2023 by Lecker Kebap - Leris@mudlet.org                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#include "EditorCommand.h"
#include "ui_trigger_editor.h"

#include <QPointer>
#include <QRegularExpression>
#include <memory>
#include <unordered_map>

#include "TAction.h"
#include "TAlias.h"
#include "TKey.h"
#include "TScript.h"
#include "TTimer.h"
#include "TTreeWidget.h"
#include "TTrigger.h"
#include "TVar.h"
#include "dlgSourceEditorArea.h"
#include "dlgSourceEditorFindArea.h"
#include "dlgSystemMessageArea.h"
#include "dlgTimersMainArea.h"
#include "dlgTriggersMainArea.h"
#include "dlgVarsMainArea.h"
#include "enums.h"
#include "SingleLineTextEdit.h"
#include "EditorPlaceholderButton.h"
#include "EditorUndoStack.h"

#include <QDialog>
#include <QDockWidget>
#include <QFlag>
#include <QHash>
#include <QIcon>
#include <QKeySequence>
#include <QPixmap>
#include <QListWidgetItem>
#include <QScrollArea>
#include <QTreeWidget>
#include <QDesktopServices>
#include <QSet>
#include <QStringList>
#include <QVector>

// Edbee editor includes
#include "edbee/edbee.h"
#include "edbee/models/changes/mergablechangegroup.h"
#include "edbee/models/chardocument/chartextdocument.h"
#include "edbee/models/textdocument.h"
#include "edbee/models/texteditorconfig.h"
#include "edbee/models/textgrammar.h"
#include "edbee/models/textundostack.h"
#include "edbee/models/textautocompleteprovider.h"
#include "edbee/texteditorcommand.h"
#include "edbee/texteditorcontroller.h"
#include "edbee/texteditorwidget.h"
#include "edbee/views/components/texteditorcomponent.h"
#include "edbee/views/textselection.h"

#include "edbee/models/textsearcher.h" // These three are required for search highlighting
#include "edbee/views/texttheme.h"
#include "edbee/views/textrenderer.h"

class dlgTimersMainArea;
class dlgSystemMessageArea;
class dlgSourceEditorArea;
class dlgSourceEditorFindArea;
class dlgTriggersMainArea;
class dlgActionMainArea;
class dlgSearchArea;
class dlgAliasMainArea;
class dlgScriptsMainArea;
class dlgKeysMainArea;
class dlgTriggerPatternEdit;
class QComboBox;
class QLabel;
class QFrame;
class QListWidget;
class QRadioButton;
class QSpinBox;
class QToolButton;
class TAction;
class TKey;
class TVar;
class TConsole;
class TConsoleModel;
class dlgVarsMainArea;
class QShortcut;

namespace uiDesign {
struct ActionGlyph;
class ChipRow;
class SidebarToggle;
class EditorLineMarginDelegate;
class EditorTreeDelegate;
class SearchResultDelegate;
class VariableTreeDelegate;
}


class dlgTriggerEditor : public QMainWindow, private Ui::trigger_editor
{
    Q_OBJECT

    // Allow QTest-based test classes to access private members
    friend class AddonControlsTest;
    friend class dlgTriggerEditorUndoRedoTest;
    friend class EditorActivateActionTest;
    friend class EditorAddItemTest;
    friend class EditorBannerViewSwitchTest;
    friend class EditorCaretReadingTest;
    friend class EditorChromeTest;
    friend class EditorClipboardXmlTest;
    friend class EditorCodeHeadingTest;
    friend class EditorColumnAlignmentTest;
    friend class EditorColumnFontTest;
    friend class EditorDefaultSizeTest;
    friend class EditorEmergencyStopTest;
    friend class EditorEventChipRowTest;
    friend class EditorEmptySelectionTest;
    friend class EditorErrorLineLinkTest;
    friend class EditorFolderViewTest;
    friend class EditorFontTest;
    friend class EditorFormShellTest;
    friend class EditorIconScaleTest;
    friend class EditorKeyCaptureTest;
    friend class EditorKeyGrabShortcutsTest;
    friend class EditorKeyboardMoveTest;
    friend class EditorLazyCreationTest;
    friend class EditorMinimumSizeTest;
    friend class EditorModuleFolderTest;
    friend class EditorNoticeGlyphTest;
    friend class EditorNoticeSeamTest;
    friend class EditorOptionsPanelDefaultTest;
    friend class EditorReshowTest;
    friend class EditorSearchTest;
    friend class EditorSplitterRestoreTest;
    friend class EditorStatusBarTest;
    friend class EditorTabAtStartTest;
    friend class EditorTimerIntervalTest;
    friend class EditorToolBarOverflowTest;
    friend class EditorTreeDotClickTest;
    friend class EditorTreeErrorMarkTest;
    friend class EditorTreeHeadingIconTest;
    friend class EditorTreeRowHeightTest;
    friend class EditorTreeSelectionPillTest;
    friend class EditorTriggerOptionsStripTest;
    friend class EditorVariablesFormTest;
    friend class EditorVariablesTreeTest;
    friend class HostWidgetDecouplingTest;
    friend class ReadabilityAuditTest;
    friend class ScriptEventHandlerLifetimeTest;
    friend class SidebarToggleTest;
    friend class TreeWidgetItemMoveTest;
    friend class TriggerEditorDisclosureTest;
    friend class TriggerEditorTest;
    friend class TriggerPatternListLayoutTest;
    friend class VariableEditorWriteBackTest;

    enum SearchDataRole {
        // Value is the ID of the item found MUST BE Qt::UserRole to avoid
        // having to modify existing code that puts it into the item:
        IdRole = Qt::UserRole,
        // Was the "name" field inserted into the search widget tree {as
        // pItem->text(1)} but since we now suppress that for subsequent
        // elements for the same "item" we need to carry the same data
        // internally even when we do not insert the text in the display:
        NameRole = Qt::UserRole + 1,
        // What the Item is (one of the cmXxxxxView values) so we know how to
        // interpret the search result:
        ItemRole = Qt::UserRole + 2,
        // Value of one of SearchDataResultType (below)
        TypeRole = Qt::UserRole + 3,
        // When the result is a pattern or event handler ("Script" item) type or
        // lua script this is the pattern number (0-49 for "Triggers"), (event
        // handler index for "Scripts") or script line (so we know which
        // field/line to jump to)
        PatternOrLineRole = Qt::UserRole + 4,
        // Value is the position (starting at 0, counting in QChars) of the
        // particular find used to position cursor at start of match:
        PositionRole = Qt::UserRole + 5,
        // Value is the index (starting at 0) of the particular find used to
        // disambiguate multiple finds in the same "thing" (so we know which one
        // to jump to) - may not be as much use as it seems...
        IndexRole = Qt::UserRole + 6,
        // Folders view only - the node's identity, stable across rebuilds of that tree:
        FolderKeyRole = Qt::UserRole + 7,
        // Folders view only - the "view:id" keys of the real items a node stands for;
        // more than one when same-named folders of different item types were merged:
        FolderPartsRole = Qt::UserRole + 8
    };

    // Classify the search result - so we know where to position the cursor as
    // we implement moving the focus to the origin of the result:
    enum SearchDataResultType {
        // Unset (?):
        SearchResultIsUnknown = 0x0,
        // The contents in the Edbee Editor widget:
        SearchResultIsScript = 0x1,
        // The item's "Name":
        SearchResultIsName = 0x2,
        // Only for "Triggers"/"Aliases" (and only the former has multiples):
        SearchResultIsPattern = 0x3,
        // All but "Variable" - the simple "Command":
        SearchResultIsCommand = 0x4,
        // Only Push-down "Buttons" - the additional "Up" "Command" field:
        SearchResultIsExtraCommand = 0x5,
        // Only "Buttons" - "Css" - unlikely to be useful currently but might be
        //useful in future if we really get into stylesheets:
        SearchResultsIsCss = 0x6,
        // Only "Scripts":
        SearchResultIsEventHandler = 0x7,
        // Only "Variables":
        SearchResultIsValue = 0x8
    };

public:
    Q_DISABLE_COPY(dlgTriggerEditor)
    // Without an error console model it makes a fresh one
    dlgTriggerEditor(Host*, std::unique_ptr<TConsoleModel> errorConsoleModel);
    ~dlgTriggerEditor();

    static std::unique_ptr<TConsoleModel> newErrorConsoleModel(Host*);
    static void ensureLuaAutoCompleteProvider();
    static QString takenKeyWarning(Host*, const TKey*);

    void closeEvent(QCloseEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void focusInEvent(QFocusEvent*) override;
    void focusOutEvent(QFocusEvent*) override;
    void showEvent(QShowEvent* event) override;
    void moveEvent(QMoveEvent* event) override;
    void enterEvent(TEnterEvent* event) override;
    bool eventFilter(QObject*, QEvent* event) override;
    bool event(QEvent* event) override;
    // Hide QWidget::setFont() and QWidget::setStyleSheet(): a font has to be carried past the stylesheets in the window, see StyleSheetFontLift
    void setFont(const QFont& font);
    void setStyleSheet(const QString& styleSheet);
    void resizeEvent(QResizeEvent* event) override;
    void changeEvent(QEvent* e) override;
    // On the regions, not the window: Host::setProfileStyleSheet() gives the window the profile's Lua sheet. Reads the
    // application palette: a sheet freezes its widget's palette, and the palette change is undelivered in changeEvent().
    void applyEditorShellStyle();
    // The options menu hangs off the window, out of the shell sheets' reach, and is built after the first style
    // pass, so both call this
    void styleSearchOptionsMenu();
    void restyleEditorIcons();
    void applyEditorToolbarButtonStyles();
    void fitEditorToolBarToItsLength();
    void updateEditorItemActionToolTips();
    void updateEditorActivateAction();
    void setupEditorPanel();
    void setupEditorCodeHeader();
    [[nodiscard]] QString codeHeaderTitleFor(const EditorViewType view) const;
    void updateEditorCodeHeading();
    void elideCompileNote();
    void jumpToCompileErrorLine();
    void clearCompileState();
    // Shown on the heading, never the banner: a banner repeated on each tree rebuild named whichever broken item came last
    void reportCompileError(const QString& error);
    // Bracket a save: what reportCompileError() is given is held to the end and the heading written once, so a save
    // that passed clears the note a failed one left
    void beginSaveErrorCapture();
    void endSaveErrorCapture();
    void buildTriggerOptionsStrip();
    void buildEditorFormHeadRows();
    void buildScriptEventRow();
    void buildTimerIntervalRow();
    void showTimerIntervalSentence(const bool offsetTimer);
    void refreshShownTimerIntervalSentence();
    void buildKeyBindingRow();
    void buildVariableTypeRows();
    void buildActionRows();
    void showKeyBinding();
    void showKeyBindingListening();
    // Every way out of a grab ends here: a grab takes the editor's shortcuts away and filters the application, and
    // nothing may end one without undoing both
    void endKeyGrab();
    // showKeyBinding() calls this rather than endKeyGrab(), so drawing the row ends a grab without the two calling
    // each other in a circle
    void releaseKeyGrab();
    bool handleKeyBindingFieldEvent(QEvent* pEvent);
    [[nodiscard]] QList<QLabel*> editorFormRowLabels() const;
    [[nodiscard]] QList<QLabel*> editorFormLeadLabels() const;
    void alignEditorFormLeadLabels();
    void fitEditorOptionsSpinBoxes();
    void reflectTriggerMatchMode();
    // The one way the options strip is opened or closed on purpose
    void setTriggerOptionsShown(const bool shown);
    void showTriggerSoundFile(const QString& path);
    void acceptTriggerSoundFile(const QString& fileName);
    [[nodiscard]] QString triggerSoundFilePath() const;
    bool handleSoundFileFieldEvent(QEvent* pEvent);
    void turnTriggerHighlightOn();
    void describeColourWell(QPushButton* pWell);
    int formPaneHeightForItsContents(const int paneTotal) const;
    void fitFormPaneToItsContents();
    void queueFormPaneRefit();
    void applyFormPaneSeamPolicy();
    void holdFormPaneToItsContents();
    [[nodiscard]] int formColumnHeightForItsWidth() const;
    [[nodiscard]] int noticeRoomInFormColumn() const;
    void moveSeamByNoticeChange(const int noticeChange);
    [[nodiscard]] QWidget* currentFormArea() const;
    void updateEditorItemCounts();
    void scheduleEditorItemCountUpdate();
    void buildEditorSidebar();
    QListWidgetItem* addEditorSidebarRow(QAction* pAction, const EditorViewType view, const QString& iconFile);
    void addEditorSidebarSeparator();
    void restyleEditorSidebarIcons(const QColor& normal, const QColor& selected);
    void restyleEditorTreeHeadingIcons();
    void refreshVariableRow(QTreeWidgetItem* pItem);
    void updateHiddenVariablesCount();
    void updateEditorSidebarToggle();
    void setEditorSidebarLabelsShown(const bool shown);
    void syncEditorSidebarSelection();
    struct EditorSidebarWidths
    {
        int expanded = 0;
        int collapseBelow = 0;
    };
    [[nodiscard]] EditorSidebarWidths editorSidebarWidths() const;
    // Call wherever the rows, the font or the style change: none of those reach a resize
    void invalidateEditorSidebarWidths();
    void updateEditorSidebarMode();
    void focusPanelTree(QWidget* pTreeWidget);
    void fillout_form();
    void showError(const QString&);
    [[nodiscard]] QString linkErrorLineToScript(const QString& error);
    void jumpToErrorLine(int line);
    void showActivationFailure(const QString& name, const QString& error);
    [[nodiscard]] QString spokenRunState(const QString& description) const;
    bool currentScriptChunk(int& itemId, QString& chunkName, QString& error);
    void showWarning(const QString&, bool announce = true);
    // The one place the package banner is raised from
    void showPackageWarning(const QString& packageName, QTreeWidgetItem* pItem = nullptr);
    void showInfo(const QString&);
    void children_icon_triggers(QTreeWidgetItem* pWidgetItemParent);
    void children_icon_alias(QTreeWidgetItem* pWidgetItemParent);
    void children_icon_key(QTreeWidgetItem* pWidgetItemParent);
    void children_icon_timer(QTreeWidgetItem* pWidgetItemParent);
    void children_icon_script(QTreeWidgetItem* pWidgetItemParent);
    void children_icon_action(QTreeWidgetItem* pWidgetItemParent);
    // Refreshes a single item's row from its current TX::isActive()
    // state, without a full tree rebuild - for GUI-external state changes such
    // as the enableTrigger()/disableTrigger() family of Lua functions.
    void refreshTriggerIcon(int triggerID);
    void refreshAliasIcon(int aliasID);
    void refreshScriptIcon(int scriptID);
    void refreshTimerIcon(int timerID);
    void refreshKeyIcon(int keyID);
    void doCleanReset();
    void writeScript(int id);
    void addVar(bool);
    int canRecast(QTreeWidgetItem*, int newNameType, int newValueType);
    void saveVar();
    void moveVariableRowToSortedPlace(QTreeWidgetItem*, TVar*);
    void showVariableRenameRefused(TVar*);
    void repopulateVars();
    void changeView(EditorViewType);
    void recurseVariablesUp(QTreeWidgetItem* const, QList<QTreeWidgetItem*>&);
    void recurseVariablesDown(QTreeWidgetItem* const, QList<QTreeWidgetItem*>&);
    void show_vars();
    void setThemeAndOtherSettings(const QString&);
    // Helper to ensure the foreground color for a button is always
    // readable/contrasts with the background when the latter is colored@
    static QString generateButtonStyleSheet(const QColor& color, const bool isEnabled = true);
    // Reader of the above - is a bit simple and may not work if the
    // stylesheetText has more that one item being styled with a "color" and
    // "background-color" attribute:
    static QColor parseButtonStyleSheetColors(const QString& styleSheetText, const bool isToGetForeground = false);
    void activeToggle_action();
    void activeToggle_alias();
    void activeToggle_key();
    void activeToggle_script();
    void activeToggle_timer();
    void activeToggle_trigger();
    void slot_itemMoved(int itemID, int oldParentID, int newParentID, int oldPosition, int newPosition);
    void slot_batchMoveStarted();
    void slot_batchMoveEnded();
    void delete_action();
    void delete_alias();
    void delete_key();
    void delete_script();
    void delete_timer();
    void delete_trigger();
    void delete_variable();
    void setSearchOptions(const enums::EditorSearchOptions);
    void setEditorShowBidi(const bool);
    void showCurrentTriggerItem();
    void hideSystemMessageArea();
    void showIDLabels(const bool);
    void setEditorFont(const QFont&);

signals:
    void editorClosing();

public slots:
    void slot_toggleHiddenVariables(bool);
    void slot_hideVariable(bool);
    void slot_variableSelected(QTreeWidgetItem*);
    void slot_variableChanged(QTreeWidgetItem*);
    void slot_toggleVariableKept();
    void slot_showVariables();
    void slot_viewErrorsAction();
    void slot_setupPatternControls(const int);
    void slot_soundTrigger();
    void slot_colorizeTriggerSetBgColor();
    void slot_colorizeTriggerSetFgColor();
    void slot_saveSelectedItem();
    void slot_export();
    void slot_import();
    void slot_createModule();
    void slot_viewStatsAction();
    void slot_toggleCentralDebugConsole();
    void slot_nextSection();
    void slot_previousSection();
    void slot_showTimers();
    void slot_showTriggers();
    void slot_showScripts();
    void slot_showAliases();
    void slot_showActions();
    void slot_showKeys();
    void slot_showFolders();
    void slot_activateMainWindow();
    void slot_updateActivateMainWindowShortcut();
    void slot_treeSelectionChanged();
    void slot_triggerSelected(QTreeWidgetItem* pItem);
    void slot_timerSelected(QTreeWidgetItem* pItem);
    void slot_scriptsSelected(QTreeWidgetItem* pItem);
    void slot_aliasSelected(QTreeWidgetItem* pItem);
    void slot_actionSelected(QTreeWidgetItem* pItem);
    void slot_keySelected(QTreeWidgetItem* pItem);
    void slot_addNewItem();
    void slot_addNewGroup();
    void slot_toggleItemOrGroupActiveFlag();
    void slot_searchMudletItems(const int);
    void slot_itemSelectedInSearchResults(QTreeWidgetItem*);
    void slot_deleteItemOrGroup();
    void slot_openSourceFind();
    void slot_closeSourceFind();
    void slot_sourceFindMove();
    void slot_sourceFindPrevious();
    void slot_sourceFindNext();
    void slot_sourceFindTextChanges();
    void slot_sourceReplace();
    void slot_saveEdits();
    void slot_copyXml();
    void slot_pasteXml();
    // Not used:    void slot_choseActionIcon();
    void slot_rightSplitterMoved(const int pos, const int handle);
    void slot_keyGrab();
    void slot_profileSaveAction();
    void slot_profileSaveAsAction();
    void slot_setToolBarIconSize(int);
    void slot_setTreeWidgetIconSize(int);
    void slot_colouredIconsChanged();
    void slot_colorTriggerFg();
    void slot_colorTriggerBg();
    void slot_updateCaretPosition(const QString& statusText); // For the source code editor
    void slot_codeHeadingClicked(QWidget* pPiece);
    void slot_profileSaveStarted();
    void slot_profileSaveFinished();
    void slot_editorThemeChanged();
    void slot_smartUndo();
    void slot_smartRedo();
    void slot_updateUndoRedoButtonStates();

private slots:
    void slot_showAllTriggerControls(const bool isShown);
    void slot_editorSidebarRowChanged(const int row);
    void slot_editorSidebarItemActivated(QListWidgetItem* pItem);
    void slot_changeEditorTextOptions(QTextOption::Flags);
    void slot_toggleIsPushDownButton(int);
    void slot_toggleSearchCaseSensitivity(bool);
    void slot_toggleSearchIncludeVariables(bool);
    void slot_toggleSearchWholeWord(bool);
    void slot_toggleGroupBoxColorizeTrigger(const bool);
    void slot_changedPattern();
    void slot_pastedExtraPatternLines(const QStringList& lines);
    void slot_lineSpacerChanged(int value);
    void slot_clearSearchResults();
    void slot_clearSoundFile();
    void slot_editorContextMenu();
    void slot_visibilityChangedEditorActionsToolbar();
    void slot_floatingChangedEditorActionsToolbar();
    void slot_restoreEditorActionsToolbar();
    void slot_itemEdited();
    void slot_clickedMessageBox(const QString&);
    void slot_addPattern();
    void slot_deletePatternRow();
    void slot_bannerDismissClicked();
    void slot_packageWarningDismissed();
    void slot_refreshBannerLinkColors();
    void slot_itemsChanged(EditorViewTypes::EditorViewType viewType, QList<int> affectedItemIDs);

    // Per-property immediate save slots for triggers (create individual undo entries)
    void slot_saveProperty_TriggerName();
    void slot_saveProperty_TriggerCommand();
    void slot_saveProperty_TriggerStayOpen();
    void slot_saveProperty_TriggerLineMargin();
    void slot_saveProperty_TriggerFilterTrigger();
    void slot_saveProperty_TriggerPerlSlashG();
    void slot_saveProperty_TriggerSoundEnabled();
    void slot_saveProperty_TriggerSoundFile();
    void slot_saveProperty_TriggerColorizer();
    void slot_saveProperty_TriggerPattern(int patternIndex);
    void slot_saveProperty_TriggerPatternType(int patternIndex);

    // Per-property immediate save slots for aliases
    void slot_saveProperty_AliasName();
    void slot_saveProperty_AliasPattern();
    void slot_saveProperty_AliasCommand();

    // Per-property immediate save slots for timers
    void slot_saveProperty_TimerName();
    void slot_saveProperty_TimerCommand();
    void slot_saveProperty_TimerTime();

    // Per-property immediate save slots for scripts
    void slot_saveProperty_ScriptName();
    void slot_saveProperty_ScriptEventHandlers();

    // Per-property immediate save slots for keys
    void slot_saveProperty_KeyName();
    void slot_saveProperty_KeyCommand();

    // Per-property immediate save slots for actions (buttons)
    void slot_saveProperty_ActionName();
    void slot_saveProperty_ActionCommandDown();
    void slot_saveProperty_ActionCommandUp();
    void slot_saveProperty_ActionIsPushDown();
    void slot_saveProperty_ActionBarColumns();
    void slot_saveProperty_ActionBarFillerOffset();
    void slot_saveProperty_ActionBarOrientation();
    void slot_saveProperty_ActionBarLocation();
    void slot_saveProperty_ActionButtonRotation();
    void slot_saveProperty_ActionCSS();

public:
    TConsole* mpErrorConsole = nullptr;
    bool mNeedUpdateData = false;

private:
    // One match: where it leads, as setAllSearchData() writes it, and the line shown with the query marked in it
    struct SearchResultRow
    {
        EditorViewType view = EditorViewType::cmUnknownView;
        // Already translated: "Trigger", "Alias"...
        QString typeLabel;
        // The item's name, or for a variable the Lua expression that reaches it
        QString title;
        QString name;
        int id = 0;
        QStringList variableId;
        QString where;
        // As in the item; the whitespace is taken out where the row is put together
        QString snippet;
        int matchStart = 0;
        SearchDataResultType what = SearchResultIsUnknown;
        int instance = 0;
        int subInstance = 0;
    };
    void addSearchResult(QTreeWidgetItem*& pParent, const SearchResultRow& row);
    void setSearchResultsShown(const bool shown);

    void populateTriggers();
    void populateTimers();
    void populateScripts();
    void populateAliases();
    void populateActions();
    void populateKeys();
    // False when the open item refused its edits, which are then only in the form
    bool saveOpenChanges();
    enum class KeyboardMove { Up, Down, OutOfGroup, IntoGroupAbove };
    void moveCurrentItem(const KeyboardMove move);
    QList<int> unitChildIDs(const EditorViewType view, const int parentID) const;
    void reParentInUnit(const EditorViewType view, const int itemID, const int oldParentID, const int newParentID, const int position);
    EditorViewType determineViewFromVisibleTree();
    EditorViewType resolveCurrentView();
    void clearTypeTrees();
    void rebuildStaleTrees();
    void populateFolderTree();
    void addFolderTreeNode(QTreeWidgetItem* pSource, const EditorViewType view, QTreeWidgetItem* pParent, QTreeWidgetItem* pLooseItems, QHash<QString, int>& folderCounts);
    void refreshFolderTreeRows(const EditorViewType view, const QModelIndex& topLeft, const QModelIndex& bottomRight);
    void scheduleFolderTreeRefresh();
    void showView(const EditorViewType view);
    void focusItemTree(QTreeWidget* pTypeTree);
    QTreeWidget* sectionTree(QTreeWidget* pTypeTree) const;
    void syncFolderTreeSelection(const QString& fallbackKey);
    void selectFolderTreeNode(QTreeWidgetItem* pNode);
    void slot_folderTreeItemChanged(QTreeWidgetItem* pItem);
    void openItemFromFolderTree(const EditorViewType view, const int id);
    void showFolderGroup(QTreeWidgetItem* pNode);
    void hideFolderGroup();
    void collapseHiddenSelections();
    void leaveFolderView();
    void setFolderViewAvailable(const bool);
    bool folderItem(const EditorViewType view, const int id);
    QString moduleOfMasterFolder(const EditorViewType view, const int id) const;
    void offerToUninstallModule(const QString& moduleName);
    QTreeWidget* treeForView(const EditorViewType view) const;
    void selectItemInView(const EditorViewType view, QTreeWidgetItem* pItem);
    static QTreeWidgetItem* findItemById(QTreeWidget* pTree, const int id);
    static QString folderTreeItemKey(const EditorViewType view, const int id);
    static QString folderTreeTypeName(const EditorViewType view, const bool plural);
    void saveTrigger();
    bool saveAlias();
    void computeAliasDescription(TAlias* pT, QString& itemDescription) const;
    void computeTriggerDescription(TTrigger* pT, QString& itemDescription) const;
    void computeTimerDescription(TTimer* pT, QString& itemDescription) const;
    void computeScriptDescription(TScript* pT, QString& itemDescription) const;
    void computeKeyDescription(TKey* pT, QString& itemDescription) const;
    // respectNewState: a freshly-added, not-yet-saved alias keeps its "unsaved"
    // reading rather than an active/inactive one - true for every GUI-driven path,
    // since TAlias::mIsNew only ever clears via an explicit Save. The
    // Lua-triggered refreshAliasIcon() passes false: a profile's aliases are
    // still "new" until manually saved, so respecting it there marked every
    // Lua-toggled alias unsaved instead of reporting its state.
    void setAliasNormalState(QTreeWidgetItem* pItem, TAlias* pT, bool touchNotification = true, bool respectNewState = true);
    void showAliasError(QTreeWidgetItem* pItem, const QString& name, const QString& error, bool touchNotification = true);
    void showAliasLoopWarning(QTreeWidgetItem* pItem, const QString& name);
    void applyAliasState(QTreeWidgetItem* pItem, TAlias* pT, bool touchNotification = true, bool respectNewState = true);
    bool aliasSubstitutionLoops(const QString& regex, const QString& substitution) const;
    QString aliasNameForPattern(const TAlias* pT, const QString& typedName, const QString& pattern) const;
    void saveTimer();
    void saveKey();
    void saveScript();
    void saveAction();
    void readSettings();
    void writeSettings();
    void restoreWindowGeometry();
    QSize defaultEditorSize() const;
    void repositionOnProfileScreen();
    bool onSameScreenAsProfile() const;
    void addScript(bool);
    void addAlias(bool);
    void addTimer(bool);
    void addTrigger(bool);
    void addAction(bool);
    void addKey(bool);
    void timerEvent(QTimerEvent* event) override;

    void selectTriggerByID(int id);
    void selectTimerByID(int id);
    void selectAliasByID(int id);
    void selectScriptByID(int id);
    void selectActionByID(int id);
    void selectKeyByID(int id);

    void clearTriggerForm();
    void clearTimerForm();
    void clearAliasForm();
    void clearScriptForm();
    void clearActionForm();
    void clearKeyForm();
    void clearVarForm();

    void updatePackageItemAccessibility(QTreeWidgetItem* pItem, const QString& currentDescription);
    void showKeyTakenWarning(QTreeWidgetItem* pItem, const QString& warning, bool announce);
    void rewarnAboutCurrentKey();

    void expand_child_triggers(TTrigger* pTriggerParent, QTreeWidgetItem* pItem);
    void expand_child_timers(TTimer* pTimerParent, QTreeWidgetItem* pWidgetItemParent);
    void expand_child_scripts(TScript* pTriggerParent, QTreeWidgetItem* pWidgetItemParent);
    void expand_child_alias(TAlias*, QTreeWidgetItem*);
    void expand_child_action(TAction*, QTreeWidgetItem*);
    void expand_child_key(TKey* pTriggerParent, QTreeWidgetItem* pWidgetItemParent);

    void exportTrigger(const QString& fileName);
    void exportTimer(const QString& fileName);
    void exportAlias(const QString& fileName);
    void exportAction(const QString& fileName);
    void exportScript(const QString& fileName);
    void exportKey(const QString& fileName);

    void exportTriggerToClipboard();
    void exportTimerToClipboard();
    void exportAliasToClipboard();
    void exportActionToClipboard();
    void exportScriptToClipboard();
    void exportKeyToClipboard();

    // Multi-selection export functions
    void exportMultipleTriggersToClipboard(const QList<TTrigger*>& triggers);
    void exportMultipleTimersToClipboard(const QList<TTimer*>& timers);
    void exportMultipleAliasesToClipboard(const QList<TAlias*>& aliases);
    void exportMultipleActionsToClipboard(const QList<TAction*>& actions);
    void exportMultipleScriptsToClipboard(const QList<TScript*>& scripts);
    void exportMultipleKeysToClipboard(const QList<TKey*>& keys);

    void placePastedItems(EditorViewType itemType, const QList<int>& itemIDs);

    void clearDocument(edbee::TextEditorWidget* pEditorWidget, const QString& initialText = QString());

    class SearchResultItem;
    void setAllSearchData(QTreeWidgetItem* pItem,
                          const EditorViewType& type,
                          const QString& name,
                          const int& id,
                          const SearchDataResultType& what,
                          const int& pos = 0,
                          const int& instance = 0,
                          const int& subInstance = 0)
    {
        // Which is it? A Trigger, an alias etc:
        pItem->setData(0, ItemRole, static_cast<int>(type));
        // What is its name:
        pItem->setData(0, NameRole, name);
        // What is its (Unique per Item Type) identifier - note that variables
        // use a different data type (QStringList):
        pItem->setData(0, IdRole, id);
        // What part of the "item" is it: the "name", the "command", the
        // "lua script", etc.:
        pItem->setData(0, TypeRole, what);
        // How far into the line/string is the start of the match, used to
        // position cursor there when chosen in search results
        pItem->setData(0, PositionRole, pos);
        // If it is a script: what line is it on (starting at 0 not 1), if a
        // trigger pattern: which one (0 to 49):
        pItem->setData(0, PatternOrLineRole, instance);
        // If there is more than one match within what the above specify - which
        // one is it, (not all things support/need to support multiples)
        pItem->setData(0, IndexRole, subInstance);
    }

    void setAllSearchData(QTreeWidgetItem* pItem, const QString& name, const QStringList& id, const SearchDataResultType& what, const int& pos = 0, const int& subInstance = 0)
    {
        // Which is it? A Trigger, an alias etc:
        pItem->setData(0, ItemRole, static_cast<int>(EditorViewType::cmVarsView));
        // What is its name:
        pItem->setData(0, NameRole, name);
        // What is its (Unique per item type) identifier - note that things
        // other then variables use a simple integer:
        pItem->setData(0, IdRole, id);
        // What part of the "item" is it: the "name", the "command", the
        // "lua script", etc.:
        pItem->setData(0, TypeRole, what);
        // How far into the line/string is the start of the match, used to
        // position cursor there when chosen in search results
        pItem->setData(0, PositionRole, pos);
        // Not used for variables:
        pItem->setData(0, PatternOrLineRole, 0);
        // If there is more than one match within what the above specify - which
        // one is it, (not all things support/need to support multiples)
        pItem->setData(0, IndexRole, subInstance);
    }

    void searchTriggers(const QString& text);
    void searchAliases(const QString& text);
    void searchScripts(const QString& text);
    void searchActions(const QString& text);
    void searchTimers(const QString& text);
    void searchKeys(const QString& text);
    void searchVariables(const QString& text);
    void recursiveSearchTriggers(TTrigger*, const QString&);
    void recursiveSearchAlias(TAlias*, const QString& text);
    void recursiveSearchScripts(TScript*, const QString& text);
    void recursiveSearchActions(TAction*, const QString& text);
    void recursiveSearchTimers(TTimer*, const QString& text);
    void recursiveSearchKeys(TKey*, const QString& text);
    void recursiveSearchVariables(TVar*, QList<TVar*>&, bool);
    void searchSingleTrigger(TTrigger* trigger, const QString& text);
    void searchSingleAlias(TAlias* alias, const QString& text);
    void searchSingleScript(TScript* script, const QString& text);
    void searchSingleAction(TAction* action, const QString& text);
    void searchSingleTimer(TTimer* timer, const QString& text);
    void searchSingleKey(TKey* key, const QString& text);
    void highlightSearchMatches();
    void
    emitScriptSearchMatches(const QString& scriptText, const QString& searchText, const QString& name, int objectId, const QString& parentLabel, EditorViewType viewType, QTreeWidgetItem*& parent);

    void createSearchOptionIcon();
    void updateSearchHistoryAction();
    int findSearchMatch(const QString& haystack, const QString& needle, int from = 0) const;
    bool containsSearchMatch(const QString& haystack, const QString& needle) const;
    void clearEditorNotification();
    void runScheduledCleanReset();
    void resetTrees();
    void buildDeferredTrees(const bool asAtLoad);
    void autoSave();
    void setupPatternControls(const int type, dlgTriggerPatternEdit* pItem);
    void createPatternItem(int index);
    void showPatternItems(int count);
    void updatePatternPlaceholders();
    [[nodiscard]] QString patternPlaceholderText(int patternType) const;
    void handlePatternChange();
    void compactPatternRows();
    void applyPatternWidgetStyle(dlgTriggerPatternEdit* patternWidget);

    // So a row can move or go without reordering widgets: the rows are a pool of up to fifty that never change place.
    // The colour buttons' text and fill are read off the pattern when it is loaded, not each time it is shown.
    struct PatternRowContent
    {
        int type = 0;
        QString pattern;
        int lineSpacerValue = 0;
        QString foregroundText;
        QString foregroundStyleSheet;
        QString backgroundText;
        QString backgroundStyleSheet;
        bool markSpaces = false;
    };
    [[nodiscard]] PatternRowContent patternRowContent(const dlgTriggerPatternEdit* patternItem) const;
    void setPatternRowContent(dlgTriggerPatternEdit* patternItem, const PatternRowContent& content);
    // The row at `from` ends up at `to`, and everything it passed over shifts one place back
    void movePatternRowContent(const int from, const int to);
    void deletePatternRow(const int row);
    [[nodiscard]] int lastVisiblePatternRow() const;
    // -1 when the keyboard is not in a pattern row
    [[nodiscard]] int focusedPatternRow() const;
    void movePatternRowByKeyboard(const int offset);
    void setupAddPatternButton();
    void updateAddPatternButton();
    void restyleAddPatternIcon();
    // A point below every row clamps to the last, so a drag let go past the end drops on the final row; -1 only
    // when no row is on show
    [[nodiscard]] int patternRowAt(const QPoint& itemsPos) const;
    void showPatternDropIndicator(const int targetRow);
    void endPatternRowDrag(const bool dropped);
    bool handlePatternHandleEvent(QObject* watched, QEvent* event);
    [[nodiscard]] QString patternRowStyleSheet() const;
    void restylePatternTypeIcons();
    void applyPatternTypeIcons(QComboBox* pBox) const;
    void applyPatternGripGlyph(QLabel* pHandle) const;
    [[nodiscard]] QIcon patternDeleteIcon() const;
    [[nodiscard]] QColor patternHoverTint() const;
    [[nodiscard]] int patternTypeColumnWidth(const QFont& typeFont) const;

    void keyGrabCallback(const Qt::Key, const Qt::KeyboardModifiers);
    [[nodiscard]] QString unboundKeyText() const;
    QList<QAction*> toolbarActions() const;
    void suspendShortcuts();
    void restoreShortcuts();

    void showOrHideRestoreEditorActionsToolbarAction();
    void checkForMoreThanOneTriggerItem();
    TTrigger* getTriggerFromTreeItem(QTreeWidgetItem* item);
    TAlias* getAliasFromTreeItem(QTreeWidgetItem* item);
    TScript* getScriptFromTreeItem(QTreeWidgetItem* item);
    TTimer* getTimerFromTreeItem(QTreeWidgetItem* item);
    TKey* getKeyFromTreeItem(QTreeWidgetItem* item);
    TAction* getActionFromTreeItem(QTreeWidgetItem* item);
    void updatePatternTabOrder();
    QWidget* firstFocusablePatternWidget(const dlgTriggerPatternEdit* patternItem) const;
    bool focusNextPatternItem(const dlgTriggerPatternEdit* currentItem);
    bool focusPreviousPatternItem(const dlgTriggerPatternEdit* currentItem);

    bool focusPatternItem(const int row, const Qt::FocusReason reason = Qt::TabFocusReason);
    void setupPatternNavigationShortcuts();

    std::unordered_map<SingleLineTextEdit*, bool> lineEditShouldMarkSpaces;
    // Toolbar shortcuts held aside while a key is being grabbed, so all of them come back afterwards
    QHash<QAction*, QList<QKeySequence>> mSuspendedShortcuts;

    QToolBar* toolBar = nullptr;

    // Undo and Redo are pictures from the start, so they are in no group
    struct EditorToolBarGroup
    {
        QList<QAction*> actions;
        bool labelsShown = true;
    };
    QList<EditorToolBarGroup> mEditorToolBarGroups;
    // A style change relays the bar out, which on a torn-off bar is another resize: guards the fit against re-entry
    bool mEditorToolBarFitting = false;

    // Each action's glyph resource, kept to re-tint it from the palette
    QList<uiDesign::ActionGlyph> mEditorActionGlyphs;

    // Kept to hand a theme change on to the state dots they draw
    QList<uiDesign::EditorTreeDelegate*> mEditorTreeDelegates;
    uiDesign::VariableTreeDelegate* mpVariableTreeDelegate = nullptr;
    QPointer<QLabel> mpLabel_hiddenVariablesCount;

    // QPointer: the splitter handle owns the strip and can take it down on teardown while this window is still
    // asked about the heading
    QPointer<QWidget> mpWidget_editorCodeHeader;
    QPointer<QLabel> mpLabel_editorCodeHeaderIcon;
    QPointer<QLabel> mpLabel_editorCodeHeaderTitle;
    QPointer<QLabel> mpLabel_editorCaretPosition;
    QPointer<QWidget> mpWidget_editorCompileNote;
    QPointer<QLabel> mpLabel_editorCompileMark;
    QPointer<QLabel> mpLabel_editorCompileMessage;
    // Empty means it compiled. Written only through reportCompileError(), which saving and opening the item reach,
    // and nothing about another item or an activation the engine refused does.
    QString mEditorCompileMessage;
    // Unelided, as the strip it is cut to keeps changing width
    QString mEditorCompileNoteText;
    // Counted from one as Lua does; zero when the compiler named none
    int mEditorCompileErrorLine = 0;
    // Owned by the code pane's margin
    uiDesign::EditorLineMarginDelegate* mpEditorLineMargin = nullptr;
    bool mEditorSaveErrorCaptureOpen = false;
    QString mEditorSaveErrorCaptured;

    // Below this the code pane cannot be typed into. On the class so a test can hold the floor to the same number.
    static constexpr int scmEditorSourcePaneFloor = 120;

    QLabel* mpLabel_optionsRow = nullptr;
    // As deep as a change inside the form goes: the column's measurement is invalidated from here
    QWidget* mpWidget_triggerOptionsRow = nullptr;
    // Never stored: the strip opens closed every time, so this only outlives an item or view switch
    bool mShowAllTriggerControls = false;
    QRadioButton* mpRadioButton_matchAny = nullptr;
    QRadioButton* mpRadioButton_matchAll = nullptr;
    QSpinBox* mpSpinBox_matchWithinLines = nullptr;
    QWidget* mpWidget_matchWithinRow = nullptr;
    QWidget* mpWidget_matchModeRows = nullptr;
    QLabel* mpLabel_matchModeHint = nullptr;
    uiDesign::ChipRow* mpChipRow_scriptEvents = nullptr;
    QWidget* mpWidget_timerInterval = nullptr;
    // So choosing another timer of the same kind leaves the words where they are
    bool mTimerIntervalRowBuilt = false;
    bool mTimerIntervalOffset = false;
    QLabel* mpLabel_keyHint = nullptr;
    QToolButton* mpButton_keyClear = nullptr;
    QPointer<QWidget> mpWidget_keyBindingRow;
    // Hidden for a module's master folder, which has no name to type and no ID to show
    QWidget* mpWidget_actionHeadRow = nullptr;
    QWidget* mpWidget_actionRotationRow = nullptr;
    // Holding the column changes the layout it was measured from, so the pass is barred from re-entering
    bool mHoldingFormPaneToItsContents = false;
    // What the notice took last time, so the two seams the reader places move by the difference
    int mNoticeRoomInFormColumn = 0;

    QWidget* mpWidget_editorSidebarPane = nullptr;
    QListWidget* mpListWidget_editorSidebar = nullptr;
    uiDesign::SidebarToggle* mpToggle_editorSidebar = nullptr;
    QList<QPair<QListWidgetItem*, QString>> mEditorSidebarGlyphs;
    // Also what the Ctrl+1 to Ctrl+0 shortcuts are on, which a key grab takes off
    QList<QAction*> mEditorViewActions;
    QListWidgetItem* mpSidebarItem_folders = nullptr;
    bool mEditorSidebarActionInFlight = false;
    // Measuring walks every row, and a resize asks on every frame of a drag
    mutable EditorSidebarWidths mEditorSidebarWidths;
    mutable bool mEditorSidebarWidthsKnown = false;
    // Stored under editorSidebarLabelsShown; about the names alone, as the sidebar is there either way
    bool mEditorSidebarLabelsShown = true;
    // The window's answer rather than the user's, so never stored
    bool mEditorSidebarNamesFit = true;
    // From the "Icon size toolbars" preference via slot_setToolBarIconSize(); 18px is what its default maps to
    int mEditorIconSize = 18;
    // A placement the user picked is moved only when unreachable; until there is one, the editor follows the profile window
    bool mEditorPlacementChosen = false;
    // So the editor's own move is not mistaken for one the user made
    bool mRepositioningEditorWindow = false;

    // Surfaces rather than tokens taken off them, since a card and a border move only when a page does
    struct EditorShellStyleInputs
    {
        QRgb page = 0;
        QRgb field = 0;
        QRgb text = 0;
        QRgb accent = 0;
        bool operator==(const EditorShellStyleInputs&) const = default;
    };
    EditorShellStyleInputs mEditorShellStyleInputs;
    bool mEditorShellStyleApplied = false;
    QLabel* mpLabel_statusCounts = nullptr;
    QLabel* mpLabel_statusAutosave = nullptr;
    // Counting walks the whole tree, which a fillout would otherwise do per item added
    QTimer* mpTimer_statusCounts = nullptr;
    bool showHiddenVars = false;

    QTreeWidgetItem* mpActionBaseItem = nullptr;
    QTreeWidgetItem* mpAliasBaseItem = nullptr;
    QTreeWidgetItem* mpKeyBaseItem = nullptr;
    QTreeWidgetItem* mpScriptsBaseItem = nullptr;
    QTreeWidgetItem* mpTimerBaseItem = nullptr;
    QTreeWidgetItem* mpTriggerBaseItem = nullptr;
    QTreeWidgetItem* mpVarBaseItem = nullptr;

    QTreeWidgetItem* mpCurrentActionItem = nullptr;
    QTreeWidgetItem* mpCurrentAliasItem = nullptr;
    QTreeWidgetItem* mpCurrentKeyItem = nullptr;
    QTreeWidgetItem* mpCurrentScriptItem = nullptr;
    QTreeWidgetItem* mpCurrentTimerItem = nullptr;
    QTreeWidgetItem* mpCurrentTriggerItem = nullptr;
    QTreeWidgetItem* mpCurrentVarItem = nullptr;

    EditorViewType mCurrentView = EditorViewType::cmUnknownView;

    QScrollArea* mpScrollArea = nullptr;
    QWidget* mpWidget_triggerItems = nullptr;
    // this widget holds the errors, trigger patterns, and all other widgets that aren't edbee
    // in it, as a workaround for an extra splitter getting created by Qt below the error msg otherwise
    QWidget* mpNonCodeWidgets = nullptr;
    dlgActionMainArea* mpActionsMainArea = nullptr;
    dlgAliasMainArea* mpAliasMainArea = nullptr;
    dlgKeysMainArea* mpKeysMainArea = nullptr;
    dlgScriptsMainArea* mpScriptsMainArea = nullptr;
    dlgTriggersMainArea* mpTriggersMainArea = nullptr;
    dlgTimersMainArea* mpTimersMainArea = nullptr;
    dlgVarsMainArea* mpVarsMainArea = nullptr;

    dlgSourceEditorArea* mpSourceEditorArea = nullptr;
    dlgSourceEditorFindArea* mpSourceEditorFindArea = nullptr;
    dlgSystemMessageArea* mpSystemMessageArea = nullptr;

    bool mIsGrabKey = false;
    QPointer<Host> mpHost;
    QList<dlgTriggerPatternEdit*> mTriggerPatternEdit;
    // The same on all fifty rows, so cached; thrown away when the theme or the font moves
    QIcon mPatternDeleteIcon;
    QColor mPatternHoverTint;
    mutable int mPatternTypeColumnWidth = 0;
    int mVisiblePatternCount = 0;
    uiDesign::PlaceholderButton* mpButton_addPattern = nullptr;
    QFrame* mpFrame_patternDropIndicator = nullptr;
    int mPatternDragSourceRow = -1;
    int mPatternDragTargetRow = -1;
    QPoint mPatternDragPressPos;
    bool mPatternDragActive = false;
    // Set while a move or delete shifts row contents, so chrome that reads every row waits for the end
    bool mPatternBulkEdit = false;
    QStringList mPatternList;
    QVector<QIcon> mPatternIcons;
    QPixmap mPatternGripGlyph;

    QShortcut* mpShortcutActivateMainWindow = nullptr;
    QShortcut* mFirstPatternShortcut = nullptr;
    QShortcut* mLastPatternShortcut = nullptr;
    QVector<QShortcut*> mPatternNavigationShortcuts;
    bool mChangingVar = false;

    QTextDocument* mpSourceEditorDocument = nullptr;
    edbee::TextEditorWidget* mpSourceEditorEdbee = nullptr;
    edbee::TextDocument* mpSourceEditorEdbeeDocument = nullptr;
    edbee::TextSearcher* mpSourceEditorSearcher = nullptr;

    inline static const QRegularExpression csmSimplifyCaretReportRegex{qsl(R"(^(?:\[\*\] )?(.+?) \|)")};
    inline static const QRegularExpression csmCompileErrorLineRegex{qsl(R"(^(?:Lua syntax error:\s*)?\[string ".*?"\]:(\d+):\s*(.*)$)")};

    QAction* mAddItem = nullptr;
    QAction* mDeleteItem = nullptr;
    QAction* mAddGroup = nullptr;
    QAction* mSaveItem = nullptr;

    enums::EditorSearchOptions mSearchOptions = enums::EditorSearchOptionNone;
    // Reused for every field a whole-word search looks at, as each new QRegularExpression is JIT-compiled anew
    mutable QString mWholeWordSearchNeedle;
    mutable QRegularExpression mWholeWordSearchRegex;
    // A search's top-level rows, kept out of the tree until it ends: every row added to the tree, and every role set on one there, emits a model signal
    QList<QTreeWidgetItem*> mPendingSearchResults;
    QSplitter* searchSplitter;
    QWidget* mpWidget_searchResultsPane = nullptr;
    QWidget* mpWidget_itemTreesPane = nullptr;
    uiDesign::SearchResultDelegate* mpSearchResultDelegate = nullptr;
    // Its length is how much of each snippet the marker covers
    QString mSearchTerm;

    // This has a menu which the following QActions are inserted into:
    QAction* mpAction_searchOptions = nullptr;
    QAction* mpAction_searchHistory = nullptr;
    QIcon mIcon_searchOptions;

    QAction* mpAction_searchCaseSensitive = nullptr;
    QAction* mpAction_searchIncludeVariables = nullptr;
    QAction* mpAction_searchWholeWord = nullptr;
    // TODO: Add other searchOptions
    // QAction* mpAction_searchRegExp;

    QAction* mProfileSaveAction = nullptr;
    QAction* mProfileSaveAsAction = nullptr;

    // Enables the toolbar to be unhidden if it gets hid:
    QAction* mpAction_restoreEditorActionsToolbar = nullptr;

    // We need to keep a record of these buttons as we have to disable them
    // for the "Variables" view:
    QAction* mpAction_toggleActive = nullptr;
    QAction* mpExportAction = nullptr;
    QAction* mpViewFoldersAction = nullptr;
    // Every item type at once, grouped by folder - built from the type trees, never edited directly
    TTreeWidget* mpFolderTree = nullptr;
    QTimer* mpFolderTreeRefreshTimer = nullptr;
    // FolderKeyRole of a folder node, or "view:id" of a real item -> its Folders view node
    QHash<QString, QTreeWidgetItem*> mFolderTreeNodes;
    QHash<QTreeWidget*, QSet<int>> mExpandedIdsBeforeRefill;
    // FolderKeyRole of the current Folders view node when it is not one item - a merged folder or "Not in a folder" - and no form shows
    QString mFolderGroupKey;
    bool mFolderViewActive = false;
    bool mFolderTreeStale = true;
    QAction* mpCreateModuleAction = nullptr;

    // Smart undo/redo actions (route based on focus):
    QAction* mpUndoAction = nullptr;
    QAction* mpRedoAction = nullptr;

    // Undo system for item-level operations (using Qt's QUndoStack framework):
    EditorUndoStack* mpUndoStack = nullptr;

    // Guarded pointer to text editor's undo stack (for safe signal connections):
    QPointer<edbee::TextUndoStack> mpTextUndoStack;

    // Track whether auto-complete provider has been initialized
    static bool smAutoCompleteInitialized;

    // tracks the duration of the "Save Profile As" action so
    // autosave doesn't kick in
    bool mSavingAs = false;

    // keeps track of the dialog reset being queued
    bool mCleanResetQueued = false;
    // A reset that arrived before the editor was ever shown, run on first show unless a view switch populated the trees first
    bool mCleanResetDeferred = false;
    // Trigger IDs whose tree row is stale; refreshed in one tree walk on the
    // next event-loop turn, or when the editor is next shown, rather than one
    // O(tree) lookup per Lua toggle.
    QSet<int> mPendingTriggerIconRefresh;
    bool mTriggerIconRefreshQueued = false;
    void flushPendingTriggerIconRefresh();
    void refreshTriggerIconsIn(QTreeWidgetItem* pParent, bool ancestorDirty, bool ancestorTouchNotification, int& remaining);
    void paintTriggerItem(QTreeWidgetItem* pItem, TTrigger* pT, bool touchNotification);

    // Same coalescing as mPendingTriggerIconRefresh, for the other four unit
    // types refreshXIcon() covers. The Flush/Paint counters exist only for
    // TriggerEditorTest, to prove the queue collapses N pending toggles into
    // one flush and that paintXItem() leaves a row whose reading has not
    // changed alone, rather than just the end state those produce either way.
    QSet<int> mPendingAliasIconRefresh;
    bool mAliasIconRefreshQueued = false;
    int mAliasIconFlushCount = 0;
    int mAliasIconPaintCount = 0;
    void flushPendingAliasIconRefresh();
    void refreshAliasIconsIn(QTreeWidgetItem* pParent, bool ancestorDirty, bool ancestorTouchNotification, int& remaining);
    void paintAliasItem(QTreeWidgetItem* pItem, TAlias* pT, bool touchNotification);

    QSet<int> mPendingTimerIconRefresh;
    bool mTimerIconRefreshQueued = false;
    int mTimerIconFlushCount = 0;
    int mTimerIconPaintCount = 0;
    void flushPendingTimerIconRefresh();
    void refreshTimerIconsIn(QTreeWidgetItem* pParent, bool ancestorDirty, bool ancestorTouchNotification, int& remaining);
    void paintTimerItem(QTreeWidgetItem* pItem, TTimer* pT, bool touchNotification);

    QSet<int> mPendingScriptIconRefresh;
    bool mScriptIconRefreshQueued = false;
    int mScriptIconFlushCount = 0;
    int mScriptIconPaintCount = 0;
    void flushPendingScriptIconRefresh();
    void refreshScriptIconsIn(QTreeWidgetItem* pParent, bool ancestorDirty, bool ancestorTouchNotification, int& remaining);
    void paintScriptItem(QTreeWidgetItem* pItem, TScript* pT, bool touchNotification);

    QSet<int> mPendingKeyIconRefresh;
    bool mKeyIconRefreshQueued = false;
    int mKeyIconFlushCount = 0;
    int mKeyIconPaintCount = 0;
    void flushPendingKeyIconRefresh();
    void refreshKeyIconsIn(QTreeWidgetItem* pParent, bool ancestorDirty, bool ancestorTouchNotification, int& remaining);
    void paintKeyItem(QTreeWidgetItem* pItem, TKey* pT, bool touchNotification);

    // tracks whether the initial profile load has completed (to avoid clearing undo stack on refreshes)
    bool mInitialLoadDone = false;

    // Blocks property saves during UI updates (e.g., when loading a selected item or during undo/redo)
    // to prevent recursive saves and duplicate undo entries
    bool mBlockPropertySave = false;

    // profile autosave interval in minutes
    int mAutosaveInterval = 2;

    // Every profile's editor shares one saved window position, so one built but never shown must not
    // overwrite it. Also what the first show's measurements wait for.
    bool mHasBeenShown = false;

    // Per view, the form height the user dragged the splitter to, kept as items change; a view not named snaps to
    // its item. Not stored: a restart has every view snapping again.
    QHash<EditorViewType, int> mDraggedFormPaneHeights;
    bool mFormPaneRefitQueued = false;

    struct EditorState
    {
        int caretLine = 0;
        int caretColumn = 0;
        int verticalScrollPos = 0;
        int horizontalScrollPos = 0;
    };

    QMap<EditorViewType, QMap<int, EditorState>> mEditorStates;

    void saveEditorState(EditorViewType viewType, int itemId);
    void restoreEditorState(EditorViewType viewType, int itemId);
    void clearEditorState(EditorViewType viewType, int itemId);

    // approximate max duration "Copy as image" can take in seconds
    int mCopyAsImageMax = 0;

    struct introOption
    {
        QString name;
        QString headline;
        QString contents;
    };

    struct introTextParts
    {
        QString summary;
        QVector<introOption> options;
    };

    QMap<EditorViewType, introTextParts> introAddItem;

    void showIntro(const QString& = QString());
    void showHideableBanner(const QString& content, const QString& bannerKey);
    // The one way anything reaches the message label
    void setSystemMessage(const QString& content);
    [[nodiscard]] QString systemMessage() const;
    [[nodiscard]] QString bannerSettingsKey(EditorViewType viewType, const QString& bannerKey) const;
    [[nodiscard]] QString legacyBannerSettingsKey(EditorViewType viewType, const QString& bannerKey) const;
    [[nodiscard]] QString profileSettingsPrefix() const;

    // Banner state tracking
    QTimer* mpBannerUndoTimer = nullptr;
    EditorViewType mLastDismissedBannerView = EditorViewType::cmUnknownView;
    QString mLastDismissedBannerContent;
    QString mCurrentBannerKey;

    // The item whose script the error banner's line link points into
    EditorViewType mErrorLineView = EditorViewType::cmUnknownView;
    int mErrorLineItemId = -1;
    QString mLastDismissedBannerKey;
    QSet<QString> mTemporarilyHiddenBanners;

    // Not a tip banner (no settings key, never restored), so the package warning keeps its own memory
    QSet<QString> mWarnedPackages;
    bool mPackageWarningsDismissed = false;
    bool mPackageWarningAnnounced = false;

    // Banner methods
    void handleBannerDismiss();
    void cancelBannerUndoTimer();
    void showBannerUndoToast();
    void undoBannerDismiss();
    void handlePermanentBannerDismiss();
    bool bannerPermanentlyHidden(EditorViewType viewType, const QString& bannerKey = QString(), bool includeBasePreference = true);
    void setBannerPermanentlyHidden(EditorViewType viewType, const QString& bannerKey, bool hidden);

    QString descActive;
    QString descInactive;
    QString descActiveFolder;
    QString descInactiveFolder;
    QString descError;
    QString descInactiveParent;
    QString descActiveFilterChain;
    QString descInactiveFilterChain;
    QString descActiveOffsetTimer;
    QString descHeldByStop;
    QString descInactiveOffsetTimer;
    QString descNewFolder;
    QString descNewItem;
    QString descPackageItem;
    QString descKeyTaken;
};

#endif // MUDLET_DLGTRIGGEREDITOR_H
