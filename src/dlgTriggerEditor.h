#ifndef MUDLET_DLGTRIGGEREDITOR_H
#define MUDLET_DLGTRIGGEREDITOR_H

/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017-2020 by Ian Adkins - ieadkins@gmail.com            *
 *   Copyright (C) 2015-2018, 2020 by Stephen Lyons                        *
 *                                               - slysven@virginmedia.com *
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


#include "pre_guard.h"
#include "ui_trigger_editor.h"
#include <QPointer>
#include "post_guard.h"

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

#include "pre_guard.h"
#include <QDialog>
#include <QFlag>
#include <QListWidgetItem>
#include <QScrollArea>
#include <QTreeWidget>
#include "post_guard.h"

// Edbee editor includes
#include "edbee/edbee.h"
#include "edbee/models/changes/mergablechangegroup.h"
#include "edbee/models/chardocument/chartextdocument.h"
#include "edbee/models/textdocument.h"
#include "edbee/models/texteditorconfig.h"
#include "edbee/models/textgrammar.h"
#include "edbee/models/textundostack.h"
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
class TAction;
class TKey;
class TConsole;
class dlgVarsMainArea;


class dlgTriggerEditor : public QMainWindow, private Ui::trigger_editor
{
    Q_OBJECT

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
           IndexRole = Qt::UserRole + 6
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
    // This needs to be public so that the options can be used from the Host class:
    enum SearchOption {
        // Unset:
        SearchOptionNone = 0x0,
        SearchOptionCaseSensitive = 0x1,
        SearchOptionIncludeVariables = 0x2 /*,
        SearchOptionRegExp = 0x4,
        SearchOptionWholeWord = 0x8 */
    };

    Q_DISABLE_COPY(dlgTriggerEditor)
    dlgTriggerEditor(Host*);

    Q_DECLARE_FLAGS(SearchOptions,SearchOption)

    enum class EditorViewType {
        cmUnknownView = 0,
        cmTriggerView = 0x01,
        cmTimerView = 0x02,
        cmAliasView = 0x03,
        cmScriptView = 0x04,
        cmActionView = 0x05,
        cmKeysView = 0x06,
        cmVarsView = 0x07
    };

    void closeEvent(QCloseEvent* event) override;
    void focusInEvent(QFocusEvent*) override;
    void focusOutEvent(QFocusEvent*) override;
    void enterEvent(QEvent* pE) override;
    bool eventFilter(QObject*, QEvent* event) override;
    bool event(QEvent* event) override;
    void resizeEvent(QResizeEvent *event) override;
    void fillout_form();
    void showError(const QString&);
    void showWarning(const QString&);
    void showInfo(const QString&);
    void children_icon_triggers(QTreeWidgetItem* pWidgetItemParent);
    void children_icon_alias(QTreeWidgetItem* pWidgetItemParent);
    void children_icon_key(QTreeWidgetItem* pWidgetItemParent);
    void doCleanReset();
    void writeScript(int id);
    void addVar(bool);
    int canRecast(QTreeWidgetItem*, int newNameType, int newValueType);
    void saveVar();
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
    void delete_action();
    void delete_alias();
    void delete_key();
    void delete_script();
    void delete_timer();
    void delete_trigger();
    void delete_variable();
    void setSearchOptions(const SearchOptions);

public slots:
    void slot_toggleHiddenVariables(bool);
    void slot_toggleHiddenVar(bool);
    void slot_var_selected(QTreeWidgetItem*);
    void slot_var_changed(QTreeWidgetItem*);
    void slot_show_vars();
    void slot_viewErrorsAction();
    void slot_setupPatternControls(const int);
    void slot_soundTrigger();
    void slot_colorizeTriggerSetBgColor();
    void slot_colorizeTriggerSetFgColor();
    void slot_item_selected_save(QTreeWidgetItem* pItem);
    void slot_export();
    void slot_import();
    void slot_viewStatsAction();
    void slot_debug_mode();
    void slot_next_section();
    void slot_previous_section();
    void slot_show_current();
    void slot_show_timers();
    void slot_show_triggers();
    void slot_show_scripts();
    void slot_show_aliases();
    void slot_show_actions();
    void slot_show_keys();
    void slot_activateMainWindow();
    void slot_tree_selection_changed();
    void slot_trigger_selected(QTreeWidgetItem* pItem);
    void slot_timer_selected(QTreeWidgetItem* pItem);
    void slot_scripts_selected(QTreeWidgetItem* pItem);
    void slot_alias_selected(QTreeWidgetItem* pItem);
    void slot_action_selected(QTreeWidgetItem* pItem);
    void slot_key_selected(QTreeWidgetItem* pItem);
    void slot_add_new();
    void slot_add_new_folder();
    void slot_toggle_active();
    void slot_searchMudletItems(const QString&); // Was slot_search_triggers(...)
    void slot_item_selected_search_list(QTreeWidgetItem*);
    void slot_delete_item();
    void slot_open_source_find();
    void slot_close_source_find();
    void slot_move_source_find();
    void slot_source_find_previous();
    void slot_source_find_next();
    void slot_source_find_text_changed();
    void slot_save_edit();
    void slot_copy_xml();
    void slot_paste_xml();
    void slot_chose_action_icon();
    void slot_showSearchAreaResults(bool);
    void slot_showAllTriggerControls(const bool);
    void slot_rightSplitterMoved(const int pos, const int handle);
    void slot_script_main_area_delete_handler();
    void slot_script_main_area_add_handler();
    void slot_script_main_area_edit_handler(QListWidgetItem*);
    void slot_key_grab();
    void slot_profileSaveAction();
    void slot_profileSaveAsAction();
    void slot_setToolBarIconSize(int);
    void slot_setTreeWidgetIconSize(int);
    void slot_color_trigger_fg();
    void slot_color_trigger_bg();
    void slot_updateStatusBar(const QString& statusText); // For the source code editor
    void slot_profileSaveStarted();
    void slot_profileSaveFinished();

private slots:
    void slot_changeEditorTextOptions(QTextOption::Flags);
    void slot_toggle_isPushDownButton(int);
    void slot_toggleSearchCaseSensitivity(bool);
    void slot_toggleSearchIncludeVariables(bool);
    void slot_toggleGroupBoxColorizeTrigger(const bool);
    void slot_clearSearchResults();
    void slot_clearSoundFile();
    void slot_editorContextMenu();

public:
    TConsole* mpErrorConsole;
    bool mNeedUpdateData;

private:
    void populateTriggers();
    void populateTimers();
    void populateScripts();
    void populateAliases();
    void populateActions();
    void populateKeys();
    void saveOpenChanges();
    void saveTrigger();
    void saveAlias();
    void saveTimer();
    void saveKey();
    void saveScript();
    void saveAction();
    void readSettings();
    void writeSettings();
    void addScript(bool);
    void addAlias(bool);
    void addTimer(bool);
    void addTrigger(bool);
    void addAction(bool);
    void addKey(bool);
    void timerEvent(QTimerEvent *event) override;

    void selectTriggerByID(int id);
    void selectTimerByID(int id);
    void selectAliasByID(int id);
    void selectScriptByID(int id);
    void selectActionByID(int id);
    void selectKeyByID(int id);

    void expand_child_triggers(TTrigger* pTriggerParent, QTreeWidgetItem* pItem);
    void expand_child_timers(TTimer* pTimerParent, QTreeWidgetItem* pWidgetItemParent);
    void expand_child_scripts(TScript* pTriggerParent, QTreeWidgetItem* pWidgetItemParent);
    void expand_child_alias(TAlias*, QTreeWidgetItem*);
    void expand_child_action(TAction*, QTreeWidgetItem*);
    void expand_child_key(TKey* pTriggerParent, QTreeWidgetItem* pWidgetItemParent);

    void exportTrigger(const QString &fileName);
    void exportTimer(const QString &fileName);
    void exportAlias(const QString &fileName);
    void exportAction(const QString &fileName);
    void exportScript(const QString &fileName);
    void exportKey(const QString &fileName);

    void exportTriggerToClipboard();
    void exportTimerToClipboard();
    void exportAliasToClipboard();
    void exportActionToClipboard();
    void exportScriptToClipboard();
    void exportKeyToClipboard();

    void clearDocument(edbee::TextEditorWidget* ew, const QString& initialText = QLatin1Literal(""));

    void setAllSearchData(QTreeWidgetItem* pItem, const EditorViewType& type, const QString& name, const int& id, const SearchDataResultType& what, const int& pos = 0, const int& instance = 0, const int& subInstance = 0) {
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

    void setAllSearchData(QTreeWidgetItem* pItem, const QString& name, const QStringList& id, const SearchDataResultType& what, const int& pos = 0, const int& subInstance = 0) {
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

    void searchTriggers(const QString& s);
    void searchAliases(const QString& s);
    void searchScripts(const QString& s);
    void searchActions(const QString& s);
    void searchTimers(const QString& s);
    void searchKeys(const QString& s);
    void searchVariables(const QString& s);
    void recursiveSearchTriggers(TTrigger*, const QString&);
    void recursiveSearchAlias(TAlias*, const QString& s);
    void recursiveSearchScripts(TScript*, const QString& s);
    void recursiveSearchActions(TAction*, const QString& s);
    void recursiveSearchTimers(TTimer*, const QString& s);
    void recursiveSearchKeys(TKey*, const QString& s);
    void recursiveSearchVariables(TVar*, QList<TVar*>&, bool);

    void createSearchOptionIcon();
    void clearEditorNotification() const;
    void runScheduledCleanReset();
    void autoSave();
    void setupPatternControls(const int type, dlgTriggerPatternEdit* pItem);
    void key_grab_callback(int key, int modifier);


    QToolBar* toolBar;
    QToolBar* toolBar2;
    bool showHiddenVars;

    QTreeWidgetItem* mpAliasBaseItem;
    QTreeWidgetItem* mpTriggerBaseItem;
    QTreeWidgetItem* mpScriptsBaseItem;
    QTreeWidgetItem* mpTimerBaseItem;
    QTreeWidgetItem* mpActionBaseItem;
    QTreeWidgetItem* mpKeyBaseItem;
    QTreeWidgetItem* mpVarBaseItem;

    QTreeWidgetItem* mpCurrentActionItem;
    QTreeWidgetItem* mpCurrentKeyItem;
    QTreeWidgetItem* mpCurrentTimerItem;
    QTreeWidgetItem* mpCurrentScriptItem;
    QTreeWidgetItem* mpCurrentTriggerItem;
    QTreeWidgetItem* mpCurrentAliasItem;
    QTreeWidgetItem* mpCurrentVarItem;

    EditorViewType mCurrentView;

    QScrollArea* mpScrollArea;
    QWidget* HpatternList;
    // this widget holds the errors, trigger patterns, and all other widgets that aren't edbee
    // in it, as a workaround for an extra splitter getting created by Qt below the error msg otherwise
    QWidget *mpNonCodeWidgets;
    dlgTriggersMainArea* mpTriggersMainArea;
    dlgTimersMainArea* mpTimersMainArea;
    dlgSystemMessageArea* mpSystemMessageArea;
    dlgSourceEditorArea* mpSourceEditorArea;
    dlgSourceEditorFindArea* mpSourceEditorFindArea;
    dlgAliasMainArea* mpAliasMainArea;
    dlgActionMainArea* mpActionsMainArea;
    dlgScriptsMainArea* mpScriptsMainArea;
    dlgKeysMainArea* mpKeysMainArea;
    bool mIsScriptsMainAreaEditHandler;
    QListWidgetItem* mpScriptsMainAreaEditHandlerItem;
    bool mIsGrabKey;
    QPointer<Host> mpHost;
    QList<dlgTriggerPatternEdit*> mTriggerPatternEdit;
    dlgVarsMainArea* mpVarsMainArea;
    bool mChangingVar;

    QTextDocument *             mpSourceEditorDocument;
    edbee::TextEditorWidget *   mpSourceEditorEdbee;
    edbee::TextDocument *       mpSourceEditorEdbeeDocument;
    edbee::TextSearcher *       mpSourceEditorSearcher;

    QRegularExpression* simplifyEdbeeStatusBarRegex;

    SearchOptions mSearchOptions;

    // This has a menu which the following QActions are inserted into:
    QAction* mpAction_searchOptions;
    QIcon mIcon_searchOptions;

    QAction* mpAction_searchCaseSensitive;
    QAction* mpAction_searchIncludeVariables;
    // TODO: Add other searchOptions
    // QAction* mpAction_searchWholeWords;
    // QAction* mpAction_searchRegExp;

    QAction* mProfileSaveAction;
    QAction* mProfileSaveAsAction;

    // tracks the duration of the "Save Profile As" action so
    // autosave doesn't kick in
    bool mSavingAs;

    // keeps track of the dialog reset being queued
    bool mCleanResetQueued;

    // profile autosave interval in minutes
    int mAutosaveInterval;

    // tracks location of the splitter in the trigger editor for each tab
    QByteArray mTriggerEditorSplitterState;
    QByteArray mAliasEditorSplitterState;
    QByteArray mScriptEditorSplitterState;
    QByteArray mActionEditorSplitterState;
    QByteArray mKeyEditorSplitterState;
    QByteArray mTimerEditorSplitterState;
    QByteArray mVarEditorSplitterState;

    // approximate max duration "Copy as image" can take in seconds
    int mCopyAsImageMax;

    QString msgInfoAddAlias;
    QString msgInfoAddTrigger;
    QString msgInfoAddScript;
    QString msgInfoAddTimer;
    QString msgInfoAddButton;
    QString msgInfoAddVar;
    QString msgInfoAddKey;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(dlgTriggerEditor::SearchOptions)

#endif // MUDLET_DLGTRIGGEREDITOR_H
