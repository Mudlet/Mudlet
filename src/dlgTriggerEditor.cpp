/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2020 by Stephen Lyons - slysven@virginmedia.com    *
 *   Copyright (C) 2016 by Owen Davison - odavison@cs.dal.ca               *
 *   Copyright (C) 2016-2020 by Ian Adkins - ieadkins@gmail.com            *
 *   Copyright (C) 2017 by Tom Scheper - scheper@gmail.com                 *
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


#include "dlgTriggerEditor.h"

#include "Host.h"
#include "LuaInterface.h"
#include "TConsole.h"
#include "TEasyButtonBar.h"
#include "TTextEdit.h"
#include "TToolBar.h"
#include "VarUnit.h"
#include "XMLimport.h"
#include "dlgActionMainArea.h"
#include "dlgAliasMainArea.h"
#include "dlgColorTrigger.h"
#include "dlgKeysMainArea.h"
#include "dlgScriptsMainArea.h"
#include "dlgTriggerPatternEdit.h"
#include "mudlet.h"

#include "pre_guard.h"
#include <QColorDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QToolBar>
#include <QScrollBar>
#include "post_guard.h"

using namespace std::chrono_literals;

// Used as a QObject::property so that we can keep track of the color for the
// trigger colorizer buttons loaded from a trigger even if the user disables
// and then reenables the colorizer function (and we "grey out" the color whilst
// it is disabled):
static const char* cButtonBaseColor = "baseColor";

dlgTriggerEditor::dlgTriggerEditor(Host* pH)
: mpAliasBaseItem(nullptr)
, mpTriggerBaseItem(nullptr)
, mpScriptsBaseItem(nullptr)
, mpTimerBaseItem(nullptr)
, mpActionBaseItem(nullptr)
, mpKeyBaseItem(nullptr)
, mpVarBaseItem(nullptr)
, mpCurrentActionItem(nullptr)
, mpCurrentKeyItem(nullptr)
, mpCurrentTimerItem(nullptr)
, mpCurrentScriptItem(nullptr)
, mpCurrentTriggerItem(nullptr)
, mpCurrentAliasItem(nullptr)
, mpCurrentVarItem(nullptr)
, mpHost(pH)
, mpSourceEditorDocument(nullptr)
, mpSourceEditorEdbee(nullptr)
, mpSourceEditorEdbeeDocument(nullptr)
, mSearchOptions(pH->mSearchOptions)
, mpAction_searchOptions(nullptr)
, mIcon_searchOptions(QIcon())
, mpAction_searchCaseSensitive(nullptr)
, mpAction_searchIncludeVariables(nullptr)
// TODO: Implement other searchOptions:
//, mpAction_searchWholeWords(nullptr)
//, mpAction_searchRegExp(nullptr)
, mCleanResetQueued(false)
, mSavingAs(false)
, mAutosaveInterval{}
, mTriggerEditorSplitterState{}
, mAliasEditorSplitterState{}
, mScriptEditorSplitterState{}
, mActionEditorSplitterState{}
, mKeyEditorSplitterState{}
, mTimerEditorSplitterState{}
, mVarEditorSplitterState{}
{
    // init generated dialog
    setupUi(this);

    msgInfoAddAlias = tr("<p>Alias react on user input. To add a new alias:"
                         "<ol><li>Click on the 'Add Item' icon above.</li>"
                         "<li>Define an input <strong>pattern</strong> either literally or with a Perl regular expression.</li>"
                         "<li>Define a 'substitution' <strong>command</strong> to send to the game in clear text <strong>instead of the alias pattern</strong>, or write a script for more complicated needs.</li>"
                         "<li><strong>Activate</strong> the alias.</li></ol></p>"
                         "<p><strong>Note:</strong> Aliases can also be defined from the command line in the main profile window like this:</p>"
                         "<p><code>lua permAlias(&quot;my greets&quot;, &quot;&quot;, &quot;^hi$&quot;, [[send (&quot;say Greetings, traveller!&quot;) echo (&quot;We said hi!&quot;)]])</code></p>"
                         "<p>You can now greet by typing 'hi'</p>"
                         "<p>Check the manual for <a href='http://wiki.mudlet.org/w/Manual:Contents'>more information</a>.</p>");

    msgInfoAddTrigger = tr("<p>Triggers react on game output. To add a new trigger:"
                           "<ol><li>Click on the 'Add Item' icon above.</li>"
                           "<li>Define a <strong>pattern</strong> that you want to trigger on.</li>"
                           "<li>Select the appropriate pattern <strong>type</strong>.</li>"
                           "<li>Define a clear text <strong>command</strong> that you want to send to the game if the trigger finds the pattern in the text from the game, or write a script for more complicated needs..</li>"
                           "<li><strong>Activate</strong> the trigger.</li></ol></p>"
                           "<p><strong>Note:</strong> Triggers can also be defined from the command line in the main profile window like this:</p>"
                           "<p><code>lua permSubstringTrigger(&quot;My drink trigger&quot;, &quot;&quot;, &quot;You are thirsty.&quot;, function() send(&quot;drink water&quot;) end)</code></p>"
                           "<p>This will keep you refreshed.</p>"
                           "<p>Check the manual for <a href='http://wiki.mudlet.org/w/Manual:Contents'>more information</a>.</p>");

    msgInfoAddScript = tr("<p>Scripts organize code and can react to events. To add a new script:"
                          "<ol><li>Click on the 'Add Item' icon above.</li>"
                          "<li>Enter a script in the box below. You can for example define <strong>functions</strong> to be called by other triggers, aliases, etc.</li>"
                          "<li>If you write lua <strong>commands</strong> without defining a function, they will be run on Mudlet startup and each time you open the script for editing.</li>"
                          "<li>If needed, you can register a list of <strong>events</strong> with the + and - symbols. If one of these events take place, the function with the same name as the script item itself will be called.</li>"
                          "<li><strong>Activate</strong> the script.</li></ol></p>"
                          "<p><strong>Note:</strong> Scripts are run automatically when viewed, even if they are deactivated.</p>"
                          "<p><strong>Note:</strong> Events can also be added to a script from the command line in the main profile window like this:</p>"
                          "<p><code>lua registerAnonymousEventHandler(&quot;nameOfTheMudletEvent&quot;, &quot;nameOfYourFunctionToBeCalled&quot;)</code></p>"
                          "<p>Check the manual for <a href='http://wiki.mudlet.org/w/Manual:Contents'>more information</a>.</p>");

    msgInfoAddTimer = tr("<p>Timers react after a timespan once or regularly. To add a new timer:"
                         "<ol><li>Click on the 'Add Item' icon above.</li>"
                         "<li>Define the <strong>timespan</strong> after which the timer should react in a this format: hours : minutes : seconds.</li>"
                         "<li>Define a clear text <strong>command</strong> that you want to send to the game when the time has passed, or write a script for more complicated needs.</li>"
                         "<li><strong>Activate</strong> the timer.</li></ol></p>"
                         "<p><strong>Note:</strong> If you want the trigger to react only once and not regularly, use the Lua tempTimer() function instead.</p>"
                         "<p><strong>Note:</strong> Timers can also be defined from the command line in the main profile window like this:</p>"
                         "<p><code>lua tempTimer(3, function() echo(&quot;hello!\n&quot;) end)</code></p>"
                         "<p>This will greet you exactly 3 seconds after it was made.</p>"
                         "<p>Check the manual for <a href='http://wiki.mudlet.org/w/Manual:Contents'>more information</a>.</p>");

    msgInfoAddButton = tr("<p>Buttons react on mouse clicks. To add a new button:"
                          "<ol><li>Add a new group to define a new <strong>button bar</strong> in case you don't have any.</li>"
                          "<li>Add new groups as <strong>menus</strong> to a button bar or sub-menus to menus.<li>"
                          "<li>Add new items as <strong>buttons</strong> to a button bar or menu or sub-menu.</li>"
                          "<li>Define a clear text <strong>command</strong> that you want to send to the game if the button is pressed, or write a script for more complicated needs.</li>"
                          "<li><strong>Activate</strong> the toolbar, menu or button. </li></ol>"
                          "<p><strong>Note:</strong> Deactivated items will be hidden and if they are toolbars or menus then all the items they contain will be also be hidden.</p>"
                          "<p><strong>Note:</strong> If a button is made a <strong>click-down</strong> button then you may also define a clear text command that you want to send to the game when the button is pressed a second time to uncheck it or to write a script to run when it happens - within such a script the Lua 'getButtonState()' function reports whether the button is up or down.</p>"
                          "<p>Check the manual for <a href='http://wiki.mudlet.org/w/Manual:Contents'>more information</a>.</p>");

    msgInfoAddKey = tr("<p>Keys react on keyboard presses. To add a new key binding:"
                       "<ol><li>Click on the 'Add Item' icon above.</li>"
                       "<li>Click on <strong>'grab key'</strong> and then press your key combination, e.g. including modifier keys like Control, Shift, etc.</li>"
                       "<li>Define a clear text <strong>command</strong> that you want to send to the game if the button is pressed, or write a script for more complicated needs.</li>"
                       "<li><strong>Activate</strong> the new key binding.</li></ol></p>"
                       "<p><strong>Note:</strong> Keys can also be defined from the command line in the main profile window like this:</p>"
                       "<p><code>lua permKey(&quot;my jump key&quot;, &quot;&quot;, mudlet.key.F8, [[send(&quot;jump&quot;]]) end)</code></p>"
                       "<p>Pressing F8 will make you jump.</p>"
                       "<p>Check the manual for <a href='http://wiki.mudlet.org/w/Manual:Contents'>more information</a>.</p>");

    msgInfoAddVar = tr("<p>Variables store information. To make a new variable:"
                       "<ol><li>Click on the 'Add Item' icon above. To add a table instead click 'Add Group'.</li>"
                       "<li>Select type of variable value (can be a string, integer, boolean)</li>"
                       "<li>Enter the value you want to store in this variable.</li>"
                       "<li>If you want to keep the variable in your next Mudlet sessions, check the checkbox in the list of variables to the left.</li>"
                       "<li>To remove a variable manually, set it to 'nil' or click on the 'Delete' icon above.</li></ol></p>"
                       "<p><strong>Note:</strong> Variables created here won't be saved when Mudlet shuts down unless you check their checkbox in the list of variables to the left. You could also create scripts with the variables instead.</p>"
                       "<p><strong>Note:</strong> Variables and tables can also be defined from the command line in the main profile window like this:</p>"
                       "<p><code>lua foo = &quot;bar&quot;</code></p>"
                       "<p>This will create a string called 'foo' with 'bar' as its value.</p>"
                       "<p>Check the manual for <a href='http://wiki.mudlet.org/w/Manual:Contents'>more information</a>.</p>");

    setUnifiedTitleAndToolBarOnMac(true); //MAC OSX: make window moveable
    const QString hostName{mpHost->getName()};
    setWindowTitle(hostName);
    setWindowIcon(QIcon(QStringLiteral(":/icons/mudlet_editor.png")));
    auto statusBar = new QStatusBar(this);
    statusBar->setSizeGripEnabled(true);
    setStatusBar(statusBar);
    statusBar->show();

    mpNonCodeWidgets = new QWidget(this);
    auto *layoutColumn = new QVBoxLayout(mpNonCodeWidgets);
    splitter_right->addWidget(mpNonCodeWidgets);

    // system message area
    mpSystemMessageArea = new dlgSystemMessageArea(this);
    mpSystemMessageArea->setObjectName(QStringLiteral("mpSystemMessageArea"));
    mpSystemMessageArea->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
    // set the stretch factor of the message area to 0 and everything else to 1,
    // so our errors box doesn't stretch to produce a grey area
    layoutColumn->addWidget(mpSystemMessageArea, 0);
    connect(mpSystemMessageArea->messageAreaCloseButton, &QAbstractButton::clicked, mpSystemMessageArea, &QWidget::hide);

    // main areas
    mpTriggersMainArea = new dlgTriggersMainArea(this);
    layoutColumn->addWidget(mpTriggersMainArea, 1);
    connect(mpTriggersMainArea->pushButtonFgColor, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_colorizeTriggerSetFgColor);
    connect(mpTriggersMainArea->pushButtonBgColor, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_colorizeTriggerSetBgColor);
    connect(mpTriggersMainArea->pushButtonSound, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_soundTrigger);
    connect(mpTriggersMainArea->groupBox_triggerColorizer, &QGroupBox::clicked, this, &dlgTriggerEditor::slot_toggleGroupBoxColorizeTrigger);
    connect(mpTriggersMainArea->toolButton_clearSoundFile, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_clearSoundFile);

    mpTimersMainArea = new dlgTimersMainArea(this);
    layoutColumn->addWidget(mpTimersMainArea, 1);

    mpAliasMainArea = new dlgAliasMainArea(this);
    layoutColumn->addWidget(mpAliasMainArea, 1);

    mpActionsMainArea = new dlgActionMainArea(this);
    layoutColumn->addWidget(mpActionsMainArea, 1);
    connect(mpActionsMainArea->checkBox_action_button_isPushDown, &QCheckBox::stateChanged, this, &dlgTriggerEditor::slot_toggle_isPushDownButton);

    mpKeysMainArea = new dlgKeysMainArea(this);
    layoutColumn->addWidget(mpKeysMainArea, 1);
    connect(mpKeysMainArea->pushButton_key_grabKey, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_key_grab);

    mpVarsMainArea = new dlgVarsMainArea(this);
    layoutColumn->addWidget(mpVarsMainArea, 1);

    mpScriptsMainArea = new dlgScriptsMainArea(this);
    layoutColumn->addWidget(mpScriptsMainArea, 1);

    mIsScriptsMainAreaEditHandler = false;
    mpScriptsMainAreaEditHandlerItem = nullptr;
    connect(mpScriptsMainArea->lineEdit_script_event_handler_entry, &QLineEdit::returnPressed, this, &dlgTriggerEditor::slot_script_main_area_add_handler);
    connect(mpScriptsMainArea->listWidget_script_registered_event_handlers, &QListWidget::itemClicked, this, &dlgTriggerEditor::slot_script_main_area_edit_handler);

    // source editor area
    mpSourceEditorArea = new dlgSourceEditorArea(this);
    splitter_right->addWidget(mpSourceEditorArea);

    // And the new edbee widget
    mpSourceEditorEdbee = mpSourceEditorArea->edbeeEditorWidget;
    mpSourceEditorEdbee->setAutoScrollMargin(20);
    mpSourceEditorEdbeeDocument = mpSourceEditorEdbee->textDocument();

    // Update the status bar on changes
    connect(mpSourceEditorEdbee->controller(), &edbee::TextEditorController::updateStatusTextSignal, this, &dlgTriggerEditor::slot_updateStatusBar);
    simplifyEdbeeStatusBarRegex = new QRegularExpression(R"(^(?:\[\*\] )?(.+?) \|)");
    mpSourceEditorEdbee->controller()->setAutoScrollToCaret(edbee::TextEditorController::AutoScrollWhenFocus);

    // Update the editor preferences
    connect(mudlet::self(), &mudlet::signal_editorTextOptionsChanged, this, &dlgTriggerEditor::slot_changeEditorTextOptions);
    mpSourceEditorEdbeeDocument->setText(tr("-- Enter your lua code here\n"));

    mudlet::loadEdbeeTheme(mpHost->mEditorTheme, mpHost->mEditorThemeFile);

    // edbee editor find area
    mpSourceEditorFindArea = new dlgSourceEditorFindArea(mpSourceEditorEdbee);
    mpSourceEditorEdbee->horizontalScrollBar()->installEventFilter(mpSourceEditorFindArea);
    mpSourceEditorEdbee->verticalScrollBar()->installEventFilter(mpSourceEditorFindArea);
    mpSourceEditorFindArea->hide();

    connect(mpSourceEditorFindArea->lineEdit_findText, &QLineEdit::textChanged, this, &dlgTriggerEditor::slot_source_find_text_changed);
    connect(mpSourceEditorFindArea, &dlgSourceEditorFindArea::signal_sourceEditorMovementNecessary, this, &dlgTriggerEditor::slot_move_source_find);
    connect(mpSourceEditorFindArea->pushButton_findPrevious, &QPushButton::clicked, this, &dlgTriggerEditor::slot_source_find_previous);
    connect(mpSourceEditorFindArea->pushButton_findNext, &QPushButton::clicked, this, &dlgTriggerEditor::slot_source_find_next);
    connect(mpSourceEditorFindArea, &dlgSourceEditorFindArea::signal_sourceEditorFindPrevious, this, &dlgTriggerEditor::slot_source_find_previous);
    connect(mpSourceEditorFindArea, &dlgSourceEditorFindArea::signal_sourceEditorFindNext, this, &dlgTriggerEditor::slot_source_find_next);
    connect(mpSourceEditorFindArea->pushButton_close, &QPushButton::clicked, this, &dlgTriggerEditor::slot_close_source_find);

    auto openSourceFindAction = new QAction(this);
    openSourceFindAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    openSourceFindAction->setShortcut(QKeySequence(QKeySequence::Find));
    mpSourceEditorArea->addAction(openSourceFindAction);
    connect(openSourceFindAction, &QAction::triggered, this, &dlgTriggerEditor::slot_open_source_find);

    QAction* closeSourceFindAction = new QAction(this);
    closeSourceFindAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    closeSourceFindAction->setShortcut(QKeySequence(QKeySequence::Cancel));
    mpSourceEditorArea->addAction(closeSourceFindAction);
    connect(closeSourceFindAction, &QAction::triggered, this, &dlgTriggerEditor::slot_close_source_find);

    QAction* sourceFindNextAction = new QAction(this);
    sourceFindNextAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    sourceFindNextAction->setShortcut(QKeySequence(QKeySequence::FindNext));
    mpSourceEditorArea->addAction(sourceFindNextAction);
    connect(sourceFindNextAction, &QAction::triggered, this, &dlgTriggerEditor::slot_source_find_next);

    QAction* sourceFindPreviousAction = new QAction(this);
    sourceFindPreviousAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    sourceFindPreviousAction->setShortcut(QKeySequence(QKeySequence::FindPrevious));
    mpSourceEditorArea->addAction(sourceFindPreviousAction);
    connect(sourceFindPreviousAction, &QAction::triggered, this, &dlgTriggerEditor::slot_source_find_previous);

    auto* provider = new edbee::StringTextAutoCompleteProvider();
    //QScopedPointer<edbee::StringTextAutoCompleteProvider> provider(new edbee::StringTextAutoCompleteProvider);

    // Add lua functions and reserved lua terms to an AutoComplete provider
    for (QString key : mudlet::mLuaFunctionNames.keys()) {
        provider->add(key, 3, mudlet::mLuaFunctionNames.value(key).toString());
    }

    provider->add(QStringLiteral("and"), 14);
    provider->add(QStringLiteral("break"), 14);
    provider->add(QStringLiteral("else"), 14);
    provider->add(QStringLiteral("elseif"), 14);
    provider->add(QStringLiteral("end"), 14);
    provider->add(QStringLiteral("false"), 14);
    provider->add(QStringLiteral("for"), 14);
    provider->add(QStringLiteral("function"), 14);
    provider->add(QStringLiteral("goto"), 14);
    provider->add(QStringLiteral("local"), 14);
    provider->add(QStringLiteral("nil"), 14);
    provider->add(QStringLiteral("not"), 14);
    provider->add(QStringLiteral("repeat"), 14);
    provider->add(QStringLiteral("return"), 14);
    provider->add(QStringLiteral("then"), 14);
    provider->add(QStringLiteral("true"), 14);
    provider->add(QStringLiteral("until"), 14);
    provider->add(QStringLiteral("while"), 14);

    // Set the newly filled provider to be used by our Edbee instance
    edbee::Edbee::instance()->autoCompleteProviderList()->setParentProvider(provider);

    mpSourceEditorEdbee->textEditorComponent()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(mpSourceEditorEdbee->textEditorComponent(), &QWidget::customContextMenuRequested, this, &dlgTriggerEditor::slot_editorContextMenu);

    // option areas
    mpErrorConsole = new TConsole(mpHost, TConsole::ErrorConsole, this);
    mpErrorConsole->setWrapAt(100);
    mpErrorConsole->mUpperPane->slot_toggleTimeStamps(true);
    mpErrorConsole->mLowerPane->slot_toggleTimeStamps(true);
    mpErrorConsole->print(tr("*** starting new session ***\n"));
    mpErrorConsole->setMinimumHeight(100);
    mpErrorConsole->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
    splitter_right->addWidget(mpErrorConsole);

    splitter_right->setStretchFactor(0, 1); // mpNonCodeWidgets
    splitter_right->setCollapsible(0, false);
    splitter_right->setStretchFactor(1, 1); // mpSourceEditorArea
    splitter_right->setCollapsible(1, false);
    splitter_right->setStretchFactor(2, 1); // mpErrorConsole
    splitter_right->setCollapsible(2, false);

    mpErrorConsole->hide();

    button_toggleSearchAreaResults->setStyleSheet(QStringLiteral("QToolButton::on {border-image: url(:/icons/arrow-down_grey.png);} "
                                                                 "QToolButton {border-image: url(:/icons/arrow-right_grey.png);} "
                                                                 "QToolButton::on:hover {border-image: url(:/icons/arrow-down.png);} "
                                                                 "QToolButton:hover {border-image: url(:/icons/arrow-right.png);}"));
    connect(button_toggleSearchAreaResults, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_showSearchAreaResults);

    connect(mpTriggersMainArea->toolButton_toggleExtraControls, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_showAllTriggerControls);
    slot_showAllTriggerControls(true);

    connect(splitter_right, &QSplitter::splitterMoved, this, &dlgTriggerEditor::slot_rightSplitterMoved);
    // additional settings
    treeWidget_triggers->setColumnCount(1);
    treeWidget_triggers->setIsTriggerTree();
    treeWidget_triggers->setRootIsDecorated(false);
    treeWidget_triggers->setHost(mpHost);
    treeWidget_triggers->header()->hide();
    treeWidget_triggers->setContextMenuPolicy(Qt::ActionsContextMenu);
    connect(treeWidget_triggers, &QTreeWidget::itemClicked, this, &dlgTriggerEditor::slot_item_selected_save);

    treeWidget_aliases->hide();
    treeWidget_aliases->setHost(mpHost);
    treeWidget_aliases->setIsAliasTree();
    treeWidget_aliases->setColumnCount(1);
    treeWidget_aliases->header()->hide();
    treeWidget_aliases->setRootIsDecorated(false);
    treeWidget_aliases->setContextMenuPolicy(Qt::ActionsContextMenu);
    connect(treeWidget_aliases, &QTreeWidget::itemClicked, this, &dlgTriggerEditor::slot_item_selected_save);

    treeWidget_actions->hide();
    treeWidget_actions->setHost(mpHost);
    treeWidget_actions->setIsActionTree();
    treeWidget_actions->setColumnCount(1);
    treeWidget_actions->header()->hide();
    treeWidget_actions->setRootIsDecorated(false);
    treeWidget_actions->setContextMenuPolicy(Qt::ActionsContextMenu);
    connect(treeWidget_actions, &QTreeWidget::itemClicked, this, &dlgTriggerEditor::slot_item_selected_save);

    treeWidget_timers->hide();
    treeWidget_timers->setHost(mpHost);
    treeWidget_timers->setIsTimerTree();
    treeWidget_timers->setColumnCount(1);
    treeWidget_timers->header()->hide();
    treeWidget_timers->setRootIsDecorated(false);
    treeWidget_timers->setContextMenuPolicy(Qt::ActionsContextMenu);
    connect(treeWidget_timers, &QTreeWidget::itemClicked, this, &dlgTriggerEditor::slot_item_selected_save);

    treeWidget_variables->hide();
    treeWidget_variables->setHost(mpHost);
    treeWidget_variables->setIsVarTree();
    treeWidget_variables->setColumnCount(2);
    treeWidget_variables->hideColumn(1);
    treeWidget_variables->header()->hide();
    treeWidget_variables->setRootIsDecorated(false);
    treeWidget_variables->setContextMenuPolicy(Qt::ActionsContextMenu);
    connect(treeWidget_variables, &QTreeWidget::itemClicked, this, &dlgTriggerEditor::slot_item_selected_save);

    treeWidget_keys->hide();
    treeWidget_keys->setHost(mpHost);
    treeWidget_keys->setIsKeyTree();
    treeWidget_keys->setColumnCount(1);
    treeWidget_keys->header()->hide();
    treeWidget_keys->setRootIsDecorated(false);
    treeWidget_keys->setContextMenuPolicy(Qt::ActionsContextMenu);
    connect(treeWidget_keys, &QTreeWidget::itemClicked, this, &dlgTriggerEditor::slot_item_selected_save);

    treeWidget_scripts->hide();
    treeWidget_scripts->setHost(mpHost);
    treeWidget_scripts->setIsScriptTree();
    treeWidget_scripts->setColumnCount(1);
    treeWidget_scripts->header()->hide();
    treeWidget_scripts->setRootIsDecorated(false);
    treeWidget_scripts->setContextMenuPolicy(Qt::ActionsContextMenu);
    connect(treeWidget_scripts, &QTreeWidget::itemClicked, this, &dlgTriggerEditor::slot_item_selected_save);

    QAction* viewTriggerAction = new QAction(QIcon(QStringLiteral(":/icons/tools-wizard.png")), tr("Triggers"), this);
    viewTriggerAction->setStatusTip(tr("Show Triggers"));
    connect(viewTriggerAction, &QAction::triggered, this, &dlgTriggerEditor::slot_show_triggers);

    QAction* viewActionAction = new QAction(QIcon(QStringLiteral(":/icons/bookmarks.png")), tr("Buttons"), this);
    viewActionAction->setStatusTip(tr("Show Buttons"));
    connect(viewActionAction, &QAction::triggered, this, &dlgTriggerEditor::slot_show_actions);


    QAction* viewAliasAction = new QAction(QIcon(QStringLiteral(":/icons/system-users.png")), tr("Aliases"), this);
    viewAliasAction->setStatusTip(tr("Show Aliases"));
    connect(viewAliasAction, &QAction::triggered, this, &dlgTriggerEditor::slot_show_aliases);


    QAction* showTimersAction = new QAction(QIcon(QStringLiteral(":/icons/chronometer.png")), tr("Timers"), this);
    showTimersAction->setStatusTip(tr("Show Timers"));
    connect(showTimersAction, &QAction::triggered, this, &dlgTriggerEditor::slot_show_timers);

    QAction* viewScriptsAction = new QAction(QIcon(QStringLiteral(":/icons/document-properties.png")), tr("Scripts"), this);
    viewScriptsAction->setStatusTip(tr("Show Scripts"));
    connect(viewScriptsAction, &QAction::triggered, this, &dlgTriggerEditor::slot_show_scripts);

    QAction* viewKeysAction = new QAction(QIcon(QStringLiteral(":/icons/preferences-desktop-keyboard.png")), tr("Keys"), this);
    viewKeysAction->setStatusTip(tr("Show Keybindings"));
    connect(viewKeysAction, &QAction::triggered, this, &dlgTriggerEditor::slot_show_keys);

    QAction* viewVarsAction = new QAction(QIcon(QStringLiteral(":/icons/variables.png")), tr("Variables"), this);
    viewVarsAction->setStatusTip(tr("Show Variables"));
    connect(viewVarsAction, &QAction::triggered, this, &dlgTriggerEditor::slot_show_vars);

    QAction* toggleActiveAction = new QAction(QIcon(QStringLiteral(":/icons/document-encrypt.png")), tr("Activate"), this);
    toggleActiveAction->setStatusTip(tr("Toggle Active or Non-Active Mode for Triggers, Scripts etc."));
    connect(toggleActiveAction, &QAction::triggered, this, &dlgTriggerEditor::slot_toggle_active);
    connect(treeWidget_triggers, &QTreeWidget::itemDoubleClicked, this, &dlgTriggerEditor::slot_toggle_active);
    connect(treeWidget_aliases, &QTreeWidget::itemDoubleClicked, this, &dlgTriggerEditor::slot_toggle_active);
    connect(treeWidget_timers, &QTreeWidget::itemDoubleClicked, this, &dlgTriggerEditor::slot_toggle_active);
    connect(treeWidget_scripts, &QTreeWidget::itemDoubleClicked, this, &dlgTriggerEditor::slot_toggle_active);
    connect(treeWidget_actions, &QTreeWidget::itemDoubleClicked, this, &dlgTriggerEditor::slot_toggle_active);
    connect(treeWidget_keys, &QTreeWidget::itemDoubleClicked, this, &dlgTriggerEditor::slot_toggle_active);


    QAction* addTriggerAction = new QAction(QIcon(QStringLiteral(":/icons/document-new.png")), tr("Add Item"), this);
    addTriggerAction->setStatusTip(tr("Add new Trigger, Script, Alias or Filter"));
    connect(addTriggerAction, &QAction::triggered, this, &dlgTriggerEditor::slot_add_new);

    QAction* deleteTriggerAction = new QAction(QIcon::fromTheme(QStringLiteral(":/icons/edit-delete"), QIcon(QStringLiteral(":/icons/edit-delete.png"))), tr("Delete Item"), this);
    deleteTriggerAction->setStatusTip(tr("Delete Trigger, Script, Alias or Filter"));
    deleteTriggerAction->setToolTip(QStringLiteral("<html><head/><body><p>%1 (%2)</p></body></html>").arg(tr("Delete Item"), QKeySequence(QKeySequence::Delete).toString()));
    deleteTriggerAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    deleteTriggerAction->setShortcut(QKeySequence(QKeySequence::Delete));
    frame_left->addAction(deleteTriggerAction);
    connect(deleteTriggerAction, &QAction::triggered, this, &dlgTriggerEditor::slot_delete_item);

    QAction* addFolderAction = new QAction(QIcon(QStringLiteral(":/icons/folder-new.png")), tr("Add Group"), this);
    addFolderAction->setStatusTip(tr("Add new Group"));
    connect(addFolderAction, &QAction::triggered, this, &dlgTriggerEditor::slot_add_new_folder);

    QAction* saveAction = new QAction(QIcon(QStringLiteral(":/icons/document-save-as.png")), tr("Save Item"), this);
    saveAction->setShortcut(tr("Ctrl+S"));
    saveAction->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>")
                                   .arg(tr("Saves the selected item. (Ctrl+S)</p>Saving causes any changes to the item to take effect.\nIt will not save to disk, "
                                           "so changes will be lost in case of a computer/program crash (but Save Profile to the right will be secure.)")));
    saveAction->setStatusTip(tr("Saves the selected trigger, script, alias, etc, causing new changes to take effect - does not save to disk though..."));
    connect(saveAction, &QAction::triggered, this, &dlgTriggerEditor::slot_save_edit);

    QAction* copyAction = new QAction(tr("Copy"), this);
    copyAction->setShortcut(QKeySequence(QKeySequence::Copy));
    // only take effect if the treeview is selected, otherwise it hijacks the shortcut from edbee
    copyAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    copyAction->setToolTip(tr("Copy the trigger/script/alias/etc"));
    copyAction->setStatusTip(tr("Copy the trigger/script/alias/etc"));
    treeWidget_triggers->addAction(copyAction);
    treeWidget_aliases->addAction(copyAction);
    treeWidget_timers->addAction(copyAction);
    treeWidget_scripts->addAction(copyAction);
    treeWidget_actions->addAction(copyAction);
    treeWidget_keys->addAction(copyAction);
    connect(copyAction, &QAction::triggered, this, &dlgTriggerEditor::slot_copy_xml);

    QAction* pasteAction = new QAction(tr("Paste"), this);
    pasteAction->setShortcut(QKeySequence(QKeySequence::Paste));
    // only take effect if the treeview is selected, otherwise it hijacks the shortcut from edbee
    pasteAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    pasteAction->setToolTip(tr("Paste triggers/scripts/aliases/etc from the clipboard"));
    pasteAction->setStatusTip(tr("Paste triggers/scripts/aliases/etc from the clipboard"));
    treeWidget_triggers->addAction(pasteAction);
    treeWidget_aliases->addAction(pasteAction);
    treeWidget_timers->addAction(pasteAction);
    treeWidget_scripts->addAction(pasteAction);
    treeWidget_actions->addAction(pasteAction);
    treeWidget_keys->addAction(pasteAction);
    connect(pasteAction, &QAction::triggered, this, &dlgTriggerEditor::slot_paste_xml);

    if (!qApp->testAttribute(Qt::AA_DontShowIconsInMenus)) {
        copyAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy"), QIcon(QStringLiteral(":/icons/edit-copy.png"))));
        pasteAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-paste"), QIcon(QStringLiteral(":/icons/edit-paste.png"))));
    }

    QAction* importAction = new QAction(QIcon(QStringLiteral(":/icons/import.png")), tr("Import"), this);
    importAction->setEnabled(true);
    connect(importAction, &QAction::triggered, this, &dlgTriggerEditor::slot_import);

    QAction* exportAction = new QAction(QIcon(QStringLiteral(":/icons/export.png")), tr("Export"), this);
    exportAction->setEnabled(true);
    connect(exportAction, &QAction::triggered, this, &dlgTriggerEditor::slot_export);

    mProfileSaveAction = new QAction(QIcon(QStringLiteral(":/icons/document-save-all.png")), tr("Save Profile"), this);
    mProfileSaveAction->setEnabled(true);
    mProfileSaveAction->setShortcut(tr("Ctrl+Shift+S"));
    mProfileSaveAction->setToolTip(
            QStringLiteral("<html><head/><body><p>%1</p></body></html>")
                    .arg(tr(R"(Saves your profile. (Ctrl+Shift+S)<p>Saves your entire profile (triggers, aliases, scripts, timers, buttons and keys, but not the map or script-specific settings) to your computer disk, so in case of a computer or program crash, all changes you have done will be retained.</p><p>It also makes a backup of your profile, you can load an older version of it when connecting.</p><p>Should there be any modules that are marked to be "<i>synced</i>" this will also cause them to be saved and reloaded into other profiles if they too are active.)")));
    mProfileSaveAction->setStatusTip(
            tr(R"(Saves your entire profile (triggers, aliases, scripts, timers, buttons and keys, but not the map or script-specific settings); also "synchronizes" modules that are so marked.)"));
    connect(mProfileSaveAction, &QAction::triggered, this, &dlgTriggerEditor::slot_profileSaveAction);

    mProfileSaveAsAction = new QAction(QIcon(QStringLiteral(":/icons/utilities-file-archiver.png")), tr("Save Profile As"), this);
    mProfileSaveAsAction->setEnabled(true);
    connect(mProfileSaveAsAction, &QAction::triggered, this, &dlgTriggerEditor::slot_profileSaveAsAction);

    QAction* viewStatsAction = new QAction(QIcon(QStringLiteral(":/icons/view-statistics.png")), tr("Statistics"), this);
    viewStatsAction->setStatusTip(tr("Generates a statistics summary display on the main profile console."));
    connect(viewStatsAction, &QAction::triggered, this, &dlgTriggerEditor::slot_viewStatsAction);

    QAction* viewErrorsAction = new QAction(QIcon(QStringLiteral(":/icons/errors.png")), tr("errors"), this);
    viewErrorsAction->setStatusTip(tr("Shows/Hides the errors console in the bottom right of this editor."));
    connect(viewErrorsAction, &QAction::triggered, this, &dlgTriggerEditor::slot_viewErrorsAction);

    QAction* showDebugAreaAction = new QAction(QIcon(QStringLiteral(":/icons/tools-report-bug.png")), tr("Debug"), this);
    showDebugAreaAction->setToolTip(tr("Activates Debug Messages -> system will be <b><i>slower</i></b>."));
    showDebugAreaAction->setStatusTip(tr("Shows/Hides the separate Central Debug Console - when being displayed the system will be slower."));
    connect(showDebugAreaAction, &QAction::triggered, this, &dlgTriggerEditor::slot_debug_mode);

    auto *nextSectionShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Tab), this);
    QObject::connect(nextSectionShortcut, &QShortcut::activated, this, &dlgTriggerEditor::slot_next_section);

    QShortcut *previousSectionShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Tab), this);
    QObject::connect(previousSectionShortcut, &QShortcut::activated, this, &dlgTriggerEditor::slot_previous_section);

    QShortcut *activateMainWindowAction = new QShortcut(QKeySequence((Qt::ALT | Qt::Key_E)), this);
    QObject::connect(activateMainWindowAction, &QShortcut::activated, this, &dlgTriggerEditor::slot_activateMainWindow);

    toolBar = new QToolBar();
    toolBar2 = new QToolBar();

    connect(mudlet::self(), &mudlet::signal_setToolBarIconSize, this, &dlgTriggerEditor::slot_setToolBarIconSize);
    connect(mudlet::self(), &mudlet::signal_setTreeIconSize, this, &dlgTriggerEditor::slot_setTreeWidgetIconSize);
    slot_setToolBarIconSize(mudlet::self()->mToolbarIconSize);
    slot_setTreeWidgetIconSize(mudlet::self()->mEditorTreeWidgetIconSize);

    toolBar->setMovable(true);
    toolBar->addAction(toggleActiveAction);
    toolBar->addAction(saveAction);
    toolBar->setWindowTitle(tr("Editor Toolbar - %1 - Actions",
                               // Intentional comment to separate arguments
                               "This is the toolbar that is initally placed at the top of the editor.")
                            .arg(hostName));

    toolBar->addSeparator();

    toolBar->addAction(addTriggerAction);
    toolBar->addAction(addFolderAction);

    toolBar->addSeparator();
    toolBar->addAction(deleteTriggerAction);
    toolBar->addAction(importAction);
    toolBar->addAction(exportAction);
    toolBar->addAction(mProfileSaveAsAction);
    toolBar->addAction(mProfileSaveAction);

    connect(checkBox_displayAllVariables, &QAbstractButton::toggled, this, &dlgTriggerEditor::slot_toggleHiddenVariables);

    connect(mpVarsMainArea->checkBox_variable_hidden, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_toggleHiddenVar);

    toolBar2->addAction(viewTriggerAction);
    toolBar2->addAction(viewAliasAction);
    toolBar2->addAction(viewScriptsAction);
    toolBar2->addAction(showTimersAction);
    toolBar2->addAction(viewKeysAction);
    toolBar2->addAction(viewVarsAction);
    toolBar2->addAction(viewActionAction);

    toolBar2->addSeparator();

    toolBar2->addAction(viewErrorsAction);
    toolBar2->addAction(viewStatsAction);
    toolBar2->addAction(showDebugAreaAction);

    toolBar2->setMovable(true);
    toolBar2->setWindowTitle(tr("Editor Toolbar - %1 - Items",
                                // Intentional comment to separate arguments
                                "This is the toolbar that is initally placed at the left side of the editor.")
                             .arg(hostName));
    toolBar2->setOrientation(Qt::Vertical);

    QMainWindow::addToolBar(Qt::LeftToolBarArea, toolBar2);
    QMainWindow::addToolBar(Qt::TopToolBarArea, toolBar);

    auto config = mpSourceEditorEdbee->config();
    config->beginChanges();
    config->setThemeName(mpHost->mEditorTheme);
    config->setFont(mpHost->getDisplayFont());
    config->setShowWhitespaceMode((mudlet::self()->mEditorTextOptions & QTextOption::ShowTabsAndSpaces)
                                  ? edbee::TextEditorConfig::ShowWhitespaces
                                  : edbee::TextEditorConfig::HideWhitespaces);
    config->setUseLineSeparator(mudlet::self()->mEditorTextOptions & QTextOption::ShowLineAndParagraphSeparators);
    config->setAutocompleteAutoShow(mpHost->mEditorAutoComplete);
    config->endChanges();

    connect(comboBox_searchTerms, qOverload<const QString&>(&QComboBox::activated), this, &dlgTriggerEditor::slot_searchMudletItems);
    connect(treeWidget_triggers, &QTreeWidget::itemClicked, this, &dlgTriggerEditor::slot_trigger_selected);
    connect(treeWidget_triggers, &QTreeWidget::itemSelectionChanged, this, &dlgTriggerEditor::slot_tree_selection_changed);
    connect(treeWidget_keys, &QTreeWidget::itemClicked, this, &dlgTriggerEditor::slot_key_selected);
    connect(treeWidget_keys, &QTreeWidget::itemSelectionChanged, this, &dlgTriggerEditor::slot_tree_selection_changed);
    connect(treeWidget_timers, &QTreeWidget::itemClicked, this, &dlgTriggerEditor::slot_timer_selected);
    connect(treeWidget_timers, &QTreeWidget::itemSelectionChanged, this, &dlgTriggerEditor::slot_tree_selection_changed);
    connect(treeWidget_scripts, &QTreeWidget::itemClicked, this, &dlgTriggerEditor::slot_scripts_selected);
    connect(treeWidget_scripts, &QTreeWidget::itemSelectionChanged, this, &dlgTriggerEditor::slot_tree_selection_changed);
    connect(treeWidget_aliases, &QTreeWidget::itemClicked, this, &dlgTriggerEditor::slot_alias_selected);
    connect(treeWidget_aliases, &QTreeWidget::itemSelectionChanged, this, &dlgTriggerEditor::slot_tree_selection_changed);
    connect(treeWidget_actions, &QTreeWidget::itemClicked, this, &dlgTriggerEditor::slot_action_selected);
    connect(treeWidget_actions, &QTreeWidget::itemSelectionChanged, this, &dlgTriggerEditor::slot_tree_selection_changed);
    connect(treeWidget_variables, &QTreeWidget::itemClicked, this, &dlgTriggerEditor::slot_var_selected);
    connect(treeWidget_variables, &QTreeWidget::itemChanged, this, &dlgTriggerEditor::slot_var_changed);
    connect(treeWidget_variables, &QTreeWidget::itemSelectionChanged, this, &dlgTriggerEditor::slot_tree_selection_changed);
    connect(treeWidget_searchResults, &QTreeWidget::itemClicked, this, &dlgTriggerEditor::slot_item_selected_search_list);

    // Force the size of the triangle icon button that shows/hides the search
    // results to be 3/4 of the height of the combo-box used to enter the search
    // term - this is to prevent an overlarge button on MacOS platforms where it
    // was found to be an issue!
    button_toggleSearchAreaResults->setMaximumSize(QSize((3 * comboBox_searchTerms->height()) / 4, (3 * comboBox_searchTerms->height()) / 4));
    button_toggleSearchAreaResults->setMinimumSize(QSize((3 * comboBox_searchTerms->height()) / 4, (3 * comboBox_searchTerms->height()) / 4));

    comboBox_searchTerms->lineEdit()->setClearButtonEnabled(true);
    auto pLineEdit_searchTerm = comboBox_searchTerms->lineEdit();

    // QLineEdit does not provide a signal to hook on for the clear action
    // see https://bugreports.qt.io/browse/QTBUG-36257 for problem
    // credit to Albert for the workaround
    for (auto child : pLineEdit_searchTerm->children()) {
        auto *pAction_clear(qobject_cast<QAction *>(child));

        // The name was found by inspection - but as it is a QT internal it
        // might change in the future:
        if (pAction_clear && pAction_clear->objectName() == QLatin1String("_q_qlineeditclearaction")) {
            connect(pAction_clear, &QAction::triggered,
                    this, &dlgTriggerEditor::slot_clearSearchResults,
                    Qt::QueuedConnection);
            break;
        }
    }

    mpAction_searchOptions = new QAction(tr("Search Options"), this);
    mpAction_searchOptions->setObjectName(QStringLiteral("mpAction_searchOptions"));

    QMenu* pMenu_searchOptions = new QMenu(tr("Search Options"), this);
    pMenu_searchOptions->setObjectName(QStringLiteral("pMenu_searchOptions"));
    pMenu_searchOptions->setToolTipsVisible(true);

    mpAction_searchCaseSensitive = new QAction(tr("Case sensitive"), this);
    mpAction_searchCaseSensitive->setObjectName(QStringLiteral("mpAction_searchCaseSensitive"));
    mpAction_searchCaseSensitive->setToolTip(QStringLiteral("<p>%1</p>")
        .arg(tr("Match case precisely")));
    mpAction_searchCaseSensitive->setCheckable(true);
    pMenu_searchOptions->insertAction(nullptr, mpAction_searchCaseSensitive);

    mpAction_searchIncludeVariables = new QAction(tr("Include variables"), this);
    mpAction_searchIncludeVariables->setObjectName(QStringLiteral("mpAction_searchIncludeVariables"));
    mpAction_searchIncludeVariables->setToolTip(QStringLiteral("<p>%1</p>")
        .arg(tr("Search variables (slower)")));
    mpAction_searchIncludeVariables->setCheckable(true);
    pMenu_searchOptions->insertAction(nullptr, mpAction_searchIncludeVariables);

    // This will set the icon and the Search Options menu items - and needs to
    // be done BEFORE the menu items are connect()ed:
    setSearchOptions(mSearchOptions);

    connect(mpAction_searchCaseSensitive, &QAction::triggered, this, &dlgTriggerEditor::slot_toggleSearchCaseSensitivity);
    connect(mpAction_searchIncludeVariables, &QAction::triggered, this, &dlgTriggerEditor::slot_toggleSearchIncludeVariables);


    mpAction_searchOptions->setMenu(pMenu_searchOptions);

    pLineEdit_searchTerm->addAction(mpAction_searchOptions, QLineEdit::LeadingPosition);

    connect(mpScriptsMainArea->toolButton_script_add_event_handler, &QAbstractButton::pressed, this, &dlgTriggerEditor::slot_script_main_area_add_handler);
    connect(mpScriptsMainArea->toolButton_script_remove_event_handler, &QAbstractButton::pressed, this, &dlgTriggerEditor::slot_script_main_area_delete_handler);

    mpTriggersMainArea->hide();
    mpTimersMainArea->hide();
    mpScriptsMainArea->hide();
    mpAliasMainArea->hide();
    mpActionsMainArea->hide();
    mpKeysMainArea->hide();
    mpVarsMainArea->hide();

    mpSourceEditorArea->hide();

    clearEditorNotification();

    treeWidget_triggers->show();
    treeWidget_aliases->hide();
    treeWidget_actions->hide();
    treeWidget_timers->hide();
    treeWidget_scripts->hide();
    treeWidget_keys->hide();
    treeWidget_variables->hide();

    readSettings();

    treeWidget_searchResults->setColumnCount(4);
    QStringList labelList;
    labelList << tr("Type", "Heading for the first column of the search results")
              << tr("Name", "Heading for the second column of the search results")
              << tr("Where", "Heading for the third column of the search results")
              << tr("What", "Heading for the fourth column of the search results");
    treeWidget_searchResults->setHeaderLabels(labelList);

    slot_showSearchAreaResults(false);

    mpScrollArea = mpTriggersMainArea->scrollArea;
    HpatternList = new QWidget;
    auto lay1 = new QVBoxLayout(HpatternList);
    lay1->setContentsMargins(0, 0, 0, 0);
    lay1->setSpacing(0);
    mpScrollArea->setWidget(HpatternList);

    QPixmap pixMap_subString(256, 256);
    pixMap_subString.fill(Qt::black);
    QIcon icon_subString(pixMap_subString);

    QPixmap pixMap_perl_regex(256, 256);
    pixMap_perl_regex.fill(Qt::blue);
    QIcon icon_perl_regex(pixMap_perl_regex);

    QPixmap pixMap_begin_of_line_substring(256, 256);
    pixMap_begin_of_line_substring.fill(Qt::red);
    QIcon icon_begin_of_line_substring(pixMap_begin_of_line_substring);

    QPixmap pixMap_exact_match(256, 256);
    pixMap_exact_match.fill(Qt::green);
    QIcon icon_exact_match(pixMap_exact_match);

    QPixmap pixMap_lua_function(256, 256);
    pixMap_lua_function.fill(Qt::cyan);
    QIcon icon_lua_function(pixMap_lua_function);

    QPixmap pixMap_line_spacer(256, 256);
    pixMap_line_spacer.fill(Qt::magenta);
    QIcon icon_line_spacer(pixMap_line_spacer);

    QPixmap pixMap_color_trigger(256, 256);
    pixMap_color_trigger.fill(Qt::lightGray);
    QIcon icon_color_trigger(pixMap_color_trigger);

    QPixmap pixMap_prompt(256, 256);
    pixMap_prompt.fill(Qt::yellow);
    QIcon icon_prompt(pixMap_prompt);

    QStringList patternList;
    patternList << tr("substring")
                << tr("perl regex")
                << tr("start of line")
                << tr("exact match")
                << tr("lua function")
                << tr("line spacer")
                << tr("color trigger")
                << tr("prompt");

    for (int i = 0; i < 50; i++) {
        auto pItem = new dlgTriggerPatternEdit(HpatternList);
        QComboBox* pBox = pItem->comboBox_patternType;
        pBox->addItems(patternList);
        pBox->setItemData(0, QVariant(i));
        pBox->setItemIcon(0, icon_subString);
        pBox->setItemIcon(1, icon_perl_regex);
        pBox->setItemIcon(2, icon_begin_of_line_substring);
        pBox->setItemIcon(3, icon_exact_match);
        pBox->setItemIcon(4, icon_lua_function);
        pBox->setItemIcon(5, icon_line_spacer);
        pBox->setItemIcon(6, icon_color_trigger);
        pBox->setItemIcon(7, icon_prompt);
        connect(pBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &dlgTriggerEditor::slot_setupPatternControls);
        connect(pItem->pushButton_fgColor, &QAbstractButton::pressed, this, &dlgTriggerEditor::slot_color_trigger_fg);
        connect(pItem->pushButton_bgColor, &QAbstractButton::pressed, this, &dlgTriggerEditor::slot_color_trigger_bg);
        HpatternList->layout()->addWidget(pItem);
        mTriggerPatternEdit.push_back(pItem);
        pItem->mRow = i;
        pItem->pushButton_fgColor->hide();
        pItem->pushButton_bgColor->hide();
        pItem->pushButton_prompt->hide();
        pItem->label_patternNumber->setText(QString::number(i+1));
        pItem->label_patternNumber->show();
    }
    // force the minimum size of the scroll area for the trigger items to be one
    // and a half trigger item widgets:
    int triggerWidgetItemMinHeight = qRound(mTriggerPatternEdit.at(0)->minimumSizeHint().height() * 1.5);
    mpScrollArea->setMinimumHeight(triggerWidgetItemMinHeight);

    showHiddenVars = false;
    widget_searchTerm->updateGeometry();

    if (mAutosaveInterval > 0) {
        startTimer(mAutosaveInterval * 1min);
    }
}

void dlgTriggerEditor::slot_toggleHiddenVar(bool status)
{
    LuaInterface* lI = mpHost->getLuaInterface();
    VarUnit* vu = lI->getVarUnit();
    TVar* var = vu->getWVar(mpCurrentVarItem);
    if (var) {
        if (status) {
            vu->addHidden(var, 1);
        } else {
            vu->removeHidden(var);
        }
    }
}

void dlgTriggerEditor::slot_toggleHiddenVariables(bool state)
{
    if (showHiddenVars != state) {
        showHiddenVars = state;
        repopulateVars();
    }
}

void dlgTriggerEditor::slot_viewStatsAction()
{
    mpHost->mpConsole->showStatistics();
    mudlet::self()->raise();
    mudlet::self()->activateWindow();
    mudlet::self()->raise();
}

void dlgTriggerEditor::slot_viewErrorsAction()
{
    mpErrorConsole->setVisible(!mpErrorConsole->isVisible());
}


void dlgTriggerEditor::slot_setToolBarIconSize(const int s)
{
    if (s <= 0) {
        return;
    }

    if (s > 2) {
        toolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        toolBar2->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    } else {
        toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
        toolBar2->setToolButtonStyle(Qt::ToolButtonIconOnly);
    }

    QSize newSize(s * 8, s * 8);
    toolBar->setIconSize(newSize);
    toolBar2->setIconSize(newSize);
}

void dlgTriggerEditor::slot_setTreeWidgetIconSize(const int s)
{
    if (s <= 0) {
        return;
    }

    QSize newSize(s * 8, s * 8);
    treeWidget_triggers->setIconSize(newSize);
    treeWidget_aliases->setIconSize(newSize);
    treeWidget_timers->setIconSize(newSize);
    treeWidget_scripts->setIconSize(newSize);
    treeWidget_keys->setIconSize(newSize);
    treeWidget_actions->setIconSize(newSize);
    treeWidget_variables->setIconSize(newSize);
}

void dlgTriggerEditor::closeEvent(QCloseEvent* event)
{
    writeSettings();
    event->accept();
}

void dlgTriggerEditor::readSettings()
{
    /* In case sensitive environments, two different config directories
       were used: "Mudlet" for QSettings, and "mudlet" anywhere else.
       Furthermore, we skip the version from the application name to follow the convention.
       For compatibility with older settings, if no config is loaded
       from the config directory "mudlet", application "Mudlet", we try to load from the config
       directory "Mudlet", application "Mudlet 1.0". */
    QSettings settings_new("mudlet", "Mudlet");
    QSettings settings((settings_new.contains("pos") ? "mudlet" : "Mudlet"), (settings_new.contains("pos") ? "Mudlet" : "Mudlet 1.0"));

    QPoint pos = settings.value("script_editor_pos", QPoint(10, 10)).toPoint();
    QSize size = settings.value("script_editor_size", QSize(600, 400)).toSize();
    resize(size);
    move(pos);

    mAutosaveInterval = settings.value("autosaveIntervalMinutes", 2).toInt();

    mTriggerEditorSplitterState = settings.value("mTriggerEditorSplitterState", QByteArray()).toByteArray();
    mAliasEditorSplitterState = settings.value("mAliasEditorSplitterState", QByteArray()).toByteArray();
    mScriptEditorSplitterState = settings.value("mScriptEditorSplitterState", QByteArray()).toByteArray();
    mActionEditorSplitterState = settings.value("mActionEditorSplitterState", QByteArray()).toByteArray();
    mKeyEditorSplitterState = settings.value("mKeyEditorSplitterState", QByteArray()).toByteArray();
    mTimerEditorSplitterState = settings.value("mTimerEditorSplitterState", QByteArray()).toByteArray();
    mVarEditorSplitterState = settings.value("mVarEditorSplitterState", QByteArray()).toByteArray();
}

void dlgTriggerEditor::writeSettings()
{
    /* In case sensitive environments, two different config directories
       were used: "Mudlet" for QSettings, and "mudlet" anywhere else. We change the QSettings directory
       (the organization name) to "mudlet".
       Furthermore, we skip the version from the application name to follow the convention.*/
    QSettings settings("mudlet", "Mudlet");
    settings.setValue("script_editor_pos", pos());
    settings.setValue("script_editor_size", size());
    settings.setValue("autosaveIntervalMinutes", mAutosaveInterval);

    settings.setValue("mTriggerEditorSplitterState", mTriggerEditorSplitterState);
    settings.setValue("mAliasEditorSplitterState", mAliasEditorSplitterState);
    settings.setValue("mScriptEditorSplitterState", mScriptEditorSplitterState);
    settings.setValue("mActionEditorSplitterState", mActionEditorSplitterState);
    settings.setValue("mKeyEditorSplitterState", mKeyEditorSplitterState);
    settings.setValue("mTimerEditorSplitterState", mTimerEditorSplitterState);
    settings.setValue("mVarEditorSplitterState", mVarEditorSplitterState);
}

void dlgTriggerEditor::slot_item_selected_search_list(QTreeWidgetItem* pItem)
{
    if (!pItem) {
        return;
    }

    // For changing views from one type to another (e.g. script->triggers), we have to show
    // the new view first before changing the TreeWidgetItem. Because we save changes to
    // the current item when it is left, if we change the TreeWidgetItem and then swap
    // views the contents of the previous item will be overwritten.
    QList<QTreeWidgetItem*> foundItemsList;
    switch (static_cast<EditorViewType>(pItem->data(0, ItemRole).toInt())) {
    case EditorViewType::cmTriggerView: { // DONE
        // These searches are to be case sensitive and recursive and find an
        // exact match - we are trying to find the "Name" of the item and then,
        // in case of duplicates we do a match on exact ID number
        foundItemsList = treeWidget_triggers->findItems(pItem->data(0, NameRole).toString(), Qt::MatchCaseSensitive | Qt::MatchFixedString| Qt::MatchRecursive, 0);

        // This was inside the loop but it is a constant value for the duration
        // of this method!
        int idSearch = pItem->data(0, IdRole).toInt();

        for (auto treeWidgetItem : qAsConst(foundItemsList)) {

            if (treeWidgetItem->data(0, IdRole).toInt() == idSearch) {
                slot_show_triggers();
                slot_trigger_selected(treeWidgetItem);
                treeWidget_triggers->setCurrentItem(treeWidgetItem, 0);
                treeWidget_triggers->scrollToItem(treeWidgetItem);

                // highlight all instances of the item that we're searching for.
                // edbee already remembers this from a setSearchTerm() call elsewhere
                auto controller = mpSourceEditorEdbee->controller();
                auto searcher = controller->textSearcher();
                searcher->markAll(controller->borderedTextRanges());
                controller->update();

                switch (pItem->data(0, TypeRole).toInt()) {
                case SearchResultIsScript:
                    mpSourceEditorEdbee->setFocus();
                    controller->moveCaretTo(pItem->data(0, PatternOrLineRole).toInt(), pItem->data(0, PositionRole).toInt(), false);
                    break;
                case SearchResultIsName:
                    mpTriggersMainArea->lineEdit_trigger_name->setFocus(Qt::OtherFocusReason);
                    mpTriggersMainArea->lineEdit_trigger_name->setCursorPosition(pItem->data(0, PositionRole).toInt());
                    break;
                case SearchResultIsPattern: {
                    dlgTriggerPatternEdit * pTriggerPattern = mTriggerPatternEdit.at(pItem->data(0, PatternOrLineRole).toInt());
                    mpScrollArea->ensureWidgetVisible(pTriggerPattern);
                    if (pTriggerPattern->lineEdit_pattern->isVisible()) {
                        // If is a colour trigger the lineEdit_pattern is not shown
                        pTriggerPattern->lineEdit_pattern->setFocus();
                        pTriggerPattern->lineEdit_pattern->setCursorPosition(pItem->data(0, PositionRole).toInt());
                    }
                    break;
                }
                case SearchResultIsCommand:
                    mpTriggersMainArea->lineEdit_trigger_command->setFocus(Qt::OtherFocusReason);
                    mpTriggersMainArea->lineEdit_trigger_command->setCursorPosition(pItem->data(0, PositionRole).toInt());
                    break;
                default:
                    qDebug() << "dlgTriggerEditor::slot_item_selected_list(...) Called for a TRIGGER type item but handler for element of type:"
                             << treeWidgetItem->data(0, TypeRole).toInt() << "not yet done/applicable...!";
                }
                return;
            }
        }
        break;
    }

    case EditorViewType::cmAliasView: {
        foundItemsList = treeWidget_aliases->findItems(pItem->data(0, NameRole).toString(), Qt::MatchCaseSensitive | Qt::MatchFixedString| Qt::MatchRecursive, 0);

        int idSearch = pItem->data(0, IdRole).toInt();

        for (auto treeWidgetItem : qAsConst(foundItemsList)) {

            if (treeWidgetItem->data(0, IdRole).toInt() == idSearch) {
                slot_show_aliases();
                slot_alias_selected(treeWidgetItem);
                treeWidget_aliases->setCurrentItem(treeWidgetItem, 0);
                treeWidget_aliases->scrollToItem(treeWidgetItem);

                // highlight all instances of the item that we're searching for.
                // edbee already remembers this from a setSearchTerm() call elsewhere
                auto controller = mpSourceEditorEdbee->controller();
                auto searcher = controller->textSearcher();
                searcher->markAll(controller->borderedTextRanges());
                controller->update();

                switch (pItem->data(0, TypeRole).toInt()) {
                case SearchResultIsScript:
                    mpSourceEditorEdbee->setFocus();
                    controller->moveCaretTo(pItem->data(0, PatternOrLineRole).toInt(), pItem->data(0, PositionRole).toInt(), false);
                    controller->setAutoScrollToCaret(edbee::TextEditorController::AutoScrollWhenFocus);
                    break;
                case SearchResultIsName:
                    mpAliasMainArea->lineEdit_alias_name->setFocus(Qt::OtherFocusReason);
                    mpAliasMainArea->lineEdit_alias_name->setCursorPosition(pItem->data(0, PositionRole).toInt());
                    break;
                case SearchResultIsPattern:
                    mpAliasMainArea->lineEdit_alias_pattern->setFocus(Qt::OtherFocusReason);
                    mpAliasMainArea->lineEdit_alias_pattern->setCursorPosition(pItem->data(0, PositionRole).toInt());
                    break;
                case SearchResultIsCommand:
                    mpAliasMainArea->lineEdit_alias_command->setFocus(Qt::OtherFocusReason);
                    mpAliasMainArea->lineEdit_alias_command->setCursorPosition(pItem->data(0, PositionRole).toInt());
                    break;
                default:
                    qDebug() << "dlgTriggerEditor::slot_item_selected_list(...) Called for a ALIAS type item but handler for element of type:"
                             << treeWidgetItem->data(0, TypeRole).toInt() << "not yet done/applicable...!";
                }
                return;
            }
        }
        break;
    }

    case EditorViewType::cmScriptView: {
        foundItemsList = treeWidget_scripts->findItems(pItem->data(0, NameRole).toString(), Qt::MatchCaseSensitive | Qt::MatchFixedString| Qt::MatchRecursive, 0);

        int idSearch = pItem->data(0, IdRole).toInt();

        for (auto treeWidgetItem : qAsConst(foundItemsList)) {

            if (treeWidgetItem->data(0, IdRole).toInt() == idSearch) {
                slot_show_scripts();
                slot_scripts_selected(treeWidgetItem);
                treeWidget_scripts->setCurrentItem(treeWidgetItem, 0);
                treeWidget_scripts->scrollToItem(treeWidgetItem);

                // highlight all instances of the item that we're searching for.
                // edbee already remembers this from a setSearchTerm() call elsewhere
                auto controller = mpSourceEditorEdbee->controller();
                auto searcher = controller->textSearcher();
                searcher->markAll(controller->borderedTextRanges());
                controller->update();

                switch (pItem->data(0, TypeRole).toInt()) {
                case SearchResultIsScript:
                    mpSourceEditorEdbee->setFocus();
                    controller->moveCaretTo(pItem->data(0, PatternOrLineRole).toInt(), pItem->data(0, PositionRole).toInt(), false);
                    break;
                case SearchResultIsName:
                    mpScriptsMainArea->lineEdit_script_name->setFocus(Qt::OtherFocusReason);
                    mpScriptsMainArea->lineEdit_script_name->setCursorPosition(pItem->data(0, PositionRole).toInt());
                    break;
                case SearchResultIsEventHandler:
                    mpScriptsMainArea->listWidget_script_registered_event_handlers->setCurrentRow(pItem->data(0, PatternOrLineRole).toInt(), QItemSelectionModel::Clear);
                    mpScriptsMainArea->listWidget_script_registered_event_handlers->scrollTo(mpScriptsMainArea->listWidget_script_registered_event_handlers->currentIndex());
                    // Taken from slot_script_main_area_edit_handler():
                    // Note the handler item being edited:
                    mpScriptsMainAreaEditHandlerItem = mpScriptsMainArea->listWidget_script_registered_event_handlers->currentItem();
                    // Copy the event name to the entry widget:
                    mpScriptsMainArea->lineEdit_script_event_handler_entry->setText(mpScriptsMainAreaEditHandlerItem->text());
                    // Activate editing flag:
                    mIsScriptsMainAreaEditHandler = true;
                    break;
                default:
                    qDebug() << "dlgTriggerEditor::slot_item_selected_list(...) Called for a SCRIPT type item but handler for element of type:"
                             << treeWidgetItem->data(0, TypeRole).toInt() << "not yet done/applicable...!";
                }

                return;
            }
        }
        break;
    }

    case EditorViewType::cmActionView: {
        foundItemsList = treeWidget_actions->findItems(pItem->data(0, NameRole).toString(), Qt::MatchCaseSensitive | Qt::MatchFixedString| Qt::MatchRecursive, 0);

        int idSearch = pItem->data(0, IdRole).toInt();

        for (auto treeWidgetitem : qAsConst(foundItemsList)) {

            if (treeWidgetitem->data(0, IdRole).toInt() == idSearch) {
                slot_show_actions();
                slot_action_selected(treeWidgetitem);
                treeWidget_actions->setCurrentItem(treeWidgetitem, 0);
                treeWidget_actions->scrollToItem(treeWidgetitem);

                // highlight all instances of the item that we're searching for.
                // edbee already remembers this from a setSearchTerm() call elsewhere
                auto controller = mpSourceEditorEdbee->controller();
                auto searcher = controller->textSearcher();
                searcher->markAll(controller->borderedTextRanges());
                controller->update();

                switch (pItem->data(0, TypeRole).toInt()) {
                case SearchResultIsScript:
                    mpSourceEditorEdbee->setFocus();
                    controller->moveCaretTo(pItem->data(0, PatternOrLineRole).toInt(), pItem->data(0, PositionRole).toInt(), false);
                    break;
                case SearchResultIsName:
                    mpActionsMainArea->lineEdit_action_name->setFocus(Qt::OtherFocusReason);
                    mpActionsMainArea->lineEdit_action_name->setCursorPosition(pItem->data(0, PositionRole).toInt());
                    break;
                case SearchResultIsCommand:
                    mpActionsMainArea->lineEdit_action_button_command_down ->setFocus(Qt::OtherFocusReason);
                    mpActionsMainArea->lineEdit_action_button_command_down->setCursorPosition(pItem->data(0, PositionRole).toInt());
                    break;
                case SearchResultIsExtraCommand:
                    if (mpActionsMainArea->checkBox_action_button_isPushDown->isChecked()) {
                        mpActionsMainArea->lineEdit_action_button_command_up->setFocus(Qt::OtherFocusReason);
                        mpActionsMainArea->lineEdit_action_button_command_up->setCursorPosition(pItem->data(0, PositionRole).toInt());
                    }
                    break;
                case SearchResultsIsCss: {
                    mpActionsMainArea->plainTextEdit_action_css->setFocus(Qt::OtherFocusReason);
                    QTextCursor cssCursor(mpActionsMainArea->plainTextEdit_action_css->textCursor());
                    cssCursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
                    if (pItem->data(0, PatternOrLineRole).toInt()) {
                        // Are we not on the first line - so move down that many lines?
                        cssCursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, pItem->data(0, PatternOrLineRole).toInt());
                    }
                    if (pItem->data(0, PositionRole).toInt()) {
                        // Are we not on the first character - if so move right that many QChars...
                        cssCursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, pItem->data(0, PositionRole).toInt());
                    }
                    mpActionsMainArea->plainTextEdit_action_css->setTextCursor(cssCursor);
                } // End case SearchResultsIsCss
                    break;
                default:
                    qDebug() << "dlgTriggerEditor::slot_item_selected_list(...) Called for a BUTTON type item but handler for element of type:"
                             << treeWidgetitem->data(0, TypeRole).toInt() << "not yet done/applicable...!";
                } // End or switch()
                return;
            } // End of if()
        } // End of for()
        break;
    } // End of case EditorViewType::cmActionView

    case EditorViewType::cmTimerView: {
        foundItemsList = treeWidget_timers->findItems(pItem->data(0, NameRole).toString(), Qt::MatchCaseSensitive | Qt::MatchFixedString| Qt::MatchRecursive, 0);

        int idSearch = pItem->data(0, IdRole).toInt();

        for (auto treeWidgetItem : qAsConst(foundItemsList)) {

            if (treeWidgetItem->data(0, IdRole).toInt() == idSearch) {
                slot_show_timers();
                slot_timer_selected(treeWidgetItem);
                treeWidget_timers->setCurrentItem(treeWidgetItem, 0);
                treeWidget_timers->scrollToItem(treeWidgetItem);

                // highlight all instances of the item that we're searching for.
                // edbee already remembers this from a setSearchTerm() call elsewhere
                auto controller = mpSourceEditorEdbee->controller();
                auto searcher = controller->textSearcher();
                searcher->markAll(controller->borderedTextRanges());
                controller->update();

                switch (pItem->data(0, TypeRole).toInt()) {
                case SearchResultIsScript:
                    mpSourceEditorEdbee->setFocus();
                    controller->moveCaretTo(pItem->data(0, PatternOrLineRole).toInt(), pItem->data(0, PositionRole).toInt(), false);
                    break;
                case SearchResultIsName:
                    mpTimersMainArea->lineEdit_timer_name->setFocus(Qt::OtherFocusReason);
                    mpTimersMainArea->lineEdit_timer_name->setCursorPosition(pItem->data(0, PositionRole).toInt());
                    break;
                case SearchResultIsCommand:
                    mpTimersMainArea->lineEdit_timer_command->setFocus(Qt::OtherFocusReason);
                    mpTimersMainArea->lineEdit_timer_command->setCursorPosition(pItem->data(0, PositionRole).toInt());
                    break;
                default:
                    qDebug() << "dlgTriggerEditor::slot_item_selected_list(...) Called for a TIMER type item but handler for element of type:"
                             << treeWidgetItem->data(0, TypeRole).toInt() << "not yet done/applicable...!";
                } // End of switch()
                return;
            } // End of if()
        } // End of for()
        break;
    } // End of case EditorViewType::cmTimerView

    case EditorViewType::cmKeysView: {
        foundItemsList = treeWidget_keys->findItems(pItem->data(0, NameRole).toString(), Qt::MatchCaseSensitive | Qt::MatchFixedString| Qt::MatchRecursive, 0);

        for (auto treeWidgetItem : qAsConst(foundItemsList)) {
            int idTree = treeWidgetItem->data(0, IdRole).toInt();
            int idSearch = pItem->data(0, IdRole).toInt();
            if (idTree == idSearch) {
                slot_show_keys();
                slot_key_selected(treeWidgetItem);
                treeWidget_keys->setCurrentItem(treeWidgetItem, 0);
                treeWidget_keys->scrollToItem(treeWidgetItem);

                // highlight all instances of the item that we're searching for.
                // edbee already remembers this from a setSearchTerm() call elsewhere
                auto controller = mpSourceEditorEdbee->controller();
                auto searcher = controller->textSearcher();
                searcher->markAll(controller->borderedTextRanges());
                controller->update();

                switch (pItem->data(0, TypeRole).toInt()) {
                case SearchResultIsScript:
                    mpSourceEditorEdbee->setFocus();
                    controller->moveCaretTo(pItem->data(0, PatternOrLineRole).toInt(), pItem->data(0, PositionRole).toInt(), false);
                    break;
                case SearchResultIsName:
                    mpTriggersMainArea->lineEdit_trigger_name->setFocus(Qt::OtherFocusReason);
                    mpTriggersMainArea->lineEdit_trigger_name->setCursorPosition(pItem->data(0, PositionRole).toInt());
                    break;
                case SearchResultIsPattern: {
                    dlgTriggerPatternEdit * pTriggerPattern = mTriggerPatternEdit.at(pItem->data(0, PatternOrLineRole).toInt());
                    mpScrollArea->ensureWidgetVisible(pTriggerPattern);
                    if (pTriggerPattern->lineEdit_pattern->isVisible()) {
                        // If is a colour trigger the lineEdit_pattern is not shown
                        pTriggerPattern->lineEdit_pattern->setFocus();
                        pTriggerPattern->lineEdit_pattern->setCursorPosition(pItem->data(0, PositionRole).toInt());
                    }
                    break;
                }
                case SearchResultIsCommand:
                    mpTriggersMainArea->lineEdit_trigger_command->setFocus(Qt::OtherFocusReason);
                    mpTriggersMainArea->lineEdit_trigger_command->setCursorPosition(pItem->data(0, PositionRole).toInt());
                    break;
                default:
                    qDebug() << "dlgTriggerEditor::slot_item_selected_list(...) Called for a KEY type item but handler for element of type:"
                             << treeWidgetItem->data(0, TypeRole).toInt() << "not yet done/applicable...!";
                } // End of switch()
                return;
            } // End of if
        } // End of for
        break;
    } // End of case EditorViewType::cmKeysView

    case EditorViewType::cmVarsView: {
        LuaInterface* lI = mpHost->getLuaInterface();
        VarUnit* vu = lI->getVarUnit();
        QStringList varShort = pItem->data(0, IdRole).toStringList();
        QList<QTreeWidgetItem*> list;
        recurseVariablesDown(mpVarBaseItem, list);
        QListIterator<QTreeWidgetItem*> it(list);
        while (it.hasNext()) {
            QTreeWidgetItem* treeWidgetItem = it.next();
            TVar* var = vu->getWVar(treeWidgetItem);
            if (vu->shortVarName(var) == varShort) {
                show_vars();
                treeWidget_variables->setCurrentItem(treeWidgetItem, 0);
                treeWidget_variables->scrollToItem(treeWidgetItem);

                // highlight all instances of the item that we're searching for.
                // edbee already remembers this from a setSearchTerm() call elsewhere
                auto controller = mpSourceEditorEdbee->controller();
                auto searcher = controller->textSearcher();
                searcher->markAll(controller->borderedTextRanges());
                controller->update();

                switch (pItem->data(0, TypeRole).toInt()) {
                case SearchResultIsName:
                    mpVarsMainArea->lineEdit_var_name->setFocus(Qt::OtherFocusReason);
                    mpVarsMainArea->lineEdit_var_name->setCursorPosition(pItem->data(0, PositionRole).toInt());
                    break;
                case SearchResultIsValue:
                    mpSourceEditorEdbee->setFocus();
                    controller->moveCaretTo(pItem->data(0, PatternOrLineRole).toInt(), pItem->data(0, PositionRole).toInt(), false);
                    break;
                default:
                    qDebug() << "dlgTriggerEditor::slot_item_selected_list(...) Called for a VAR type item but handler for element of type:"
                             << treeWidgetItem->data(0, TypeRole).toInt() << "not yet done/applicable...!";
                }
                return;
            }
        }
    }  // End of case static_cast<int>(EditorViewType::cmVarsView)
        break;
    default:
        ; // No-op
    } // End of switch()
}

void dlgTriggerEditor::slot_searchMudletItems(const QString& s)
{
    if (s.isEmpty()) {
        return;
    }

    treeWidget_searchResults->clear();
    slot_showSearchAreaResults(true);
    treeWidget_searchResults->setUpdatesEnabled(false);

    searchTriggers(s);
    searchAliases(s);
    searchScripts(s);
    searchActions(s);
    searchTimers(s);
    searchKeys(s);

    if (mSearchOptions & SearchOptionIncludeVariables) {
        searchVariables(s);
    }

    // TODO: Edbee search term highlighter

    // As it is, findNext() and selectNext() are exactly the same. You could
    // do a selectAll(), but that would create a cursor for each found instance,
    // and would likely do things the user wasn't expecting.

    // Although there are some findHighlight code entries in libedbee, the
    // functionality isn't implemented.

    mpSourceEditorEdbee->controller()->textSearcher()->setSearchTerm(s);
    mpSourceEditorEdbee->controller()->textSearcher()->setCaseSensitive(mSearchOptions & SearchOptionCaseSensitive);

    treeWidget_searchResults->setUpdatesEnabled(true);

    // Need to highlight the contents if something is already showing in the editor:
    mpSourceEditorEdbee->controller()->update();
}

void dlgTriggerEditor::searchVariables(const QString& s)
{
    if (mCurrentView != EditorViewType::cmVarsView) {
        // repopulateVars can take some time should there be a large number
        // of variables or big tables... 8-(
        repopulateVars();
    }

    LuaInterface* lI = mpHost->getLuaInterface();
    VarUnit* vu = lI->getVarUnit();
    TVar* base = vu->getBase();
    QListIterator<TVar*> itBaseVarChildren(base->getChildren(false));
    while (itBaseVarChildren.hasNext()) {
        TVar* var = itBaseVarChildren.next();
        // We do not search for hidden variables - probably because we would
        // have to unhide all of them to show the hidden ones found by
        // searching
        if (!showHiddenVars && vu->isHidden(var)) {
            continue;
        }

        //recurse down this variable
        QList<TVar*> list;
        recursiveSearchVariables(var, list, false);
        QListIterator<TVar*> itVarDecendent(list);
        while (itVarDecendent.hasNext()) {
            TVar* varDecendent = itVarDecendent.next();
            if (!showHiddenVars && vu->isHidden(varDecendent)) {
                continue;
            }

            QTreeWidgetItem* pItem;
            QTreeWidgetItem* parent = nullptr;
            QString name = varDecendent->getName();
            QString value = varDecendent->getValue();
            QStringList idStringList = vu->shortVarName(varDecendent);
            QString idString;
            // Take the first element - to comply with lua requirement it
            // must begin with not a digit and not contain any spaces so is
            // a string - and it is used "unquoted" as is to be the base
            // of a lua table
            if (idStringList.size() > 1) {
                QStringList midStrings = idStringList;
                idString = midStrings.takeFirst();
                QStringListIterator itSubString(midStrings);
                while (itSubString.hasNext()) {
                    QString intermediate = itSubString.next();
                    bool isOk = false;
                    int numberValue = intermediate.toInt(&isOk);
                    if (isOk && QString::number(numberValue) == intermediate) {
                        // This seems to be an integer
                        idString.append(QStringLiteral("[%1]").arg(intermediate));
                    } else {
                        idString.append(QStringLiteral("[\"%1\"]").arg(intermediate));
                    }
                }
            } else if (!idStringList.empty()) {
                idString = idStringList.at(0);
            }

            int startPos = 0;
            if ((startPos = name.indexOf(s, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
                QStringList sl;
                sl << tr("Variable") << idString << tr("Name");
                parent = new QTreeWidgetItem(sl);
                // We do not (yet) worry about multiple search results in the "name"
                setAllSearchData(parent, name, vu->shortVarName(varDecendent), SearchResultIsName, startPos);
                treeWidget_searchResults->addTopLevelItem(parent);
            }

            // The additional first test is needed to exclude the case when
            // the search term matches on the word "function" which will
            // appear in EVERY "value" for a lua function in the variable
            // tree widget...
            if (value != QLatin1String("function") && (startPos = value.indexOf(s, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
                QStringList sl;
                if (!parent) {
                    sl << tr("Variable") << idString << tr("Value") << value;
                    parent = new QTreeWidgetItem(sl);
                    // We do not (yet) worry about multiple search results in the "value"
                    setAllSearchData(parent, name, vu->shortVarName(varDecendent), SearchResultIsValue, startPos);
                    treeWidget_searchResults->addTopLevelItem(parent);
                } else {
                    sl << QString() << QString() << tr("Value") << value;
                    pItem = new QTreeWidgetItem(sl);
                    // We do not (yet) worry about multiple search results in the "value"
                    setAllSearchData(pItem, name, vu->shortVarName(varDecendent), SearchResultIsValue, startPos);
                    parent->addChild(pItem);
                    parent->setExpanded(true);
                }
            }
        }
    }
}

void dlgTriggerEditor::searchKeys(const QString& s)
{
    std::list<TKey*> nodes = mpHost->getKeyUnit()->getKeyRootNodeList();
    for (auto key : nodes) {
        QTreeWidgetItem* pItem;
        QTreeWidgetItem* parent = nullptr;
        QString name = key->getName();
        int startPos = 0;

        if ((startPos = name.indexOf(s, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
            QStringList sl;
            sl << tr("Key") << name << tr("Name");
            // This part can never have a parent as it is the first part of this item
            parent = new QTreeWidgetItem(sl);
            setAllSearchData(parent, EditorViewType::cmKeysView, name, key->getID(), SearchResultIsName, startPos);
            treeWidget_searchResults->addTopLevelItem(parent);
        }

        // The simple "command"
        // TODO: (A) Revise to count multiple instances of search string within command?
        if ((startPos = key->getCommand().indexOf(s, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
            QStringList sl;
            if (!parent) {
                sl << tr("Key") << name << tr("Command");
                parent = new QTreeWidgetItem(sl);
                setAllSearchData(parent, EditorViewType::cmKeysView, name, key->getID(), SearchResultIsCommand, startPos);
                treeWidget_searchResults->addTopLevelItem(parent);
            } else {
                sl << QString() << QString() << tr("Command");
                pItem = new QTreeWidgetItem(parent, sl);
                setAllSearchData(pItem, EditorViewType::cmKeysView, name, key->getID(), SearchResultIsCommand, startPos);
                parent->addChild(pItem);
                parent->setExpanded(true);
            }
        }

        // Script content
        QStringList textList = key->getScript().split("\n");
        int total = textList.count();
        for (int index = 0; index < total; ++index) {
            // CHECK: This may NOT be an optimisation...!
            if (textList.at(index).isEmpty() || !textList.at(index).contains(s, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) {
                // Short-cuts that mean we do not have to examine the line in more detail
                continue;
            }

            int instance = 0;
            startPos = 0;
            while ((startPos = textList.at(index).indexOf(s, startPos, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
                QString whatText(textList.at(index));
                whatText.replace(QString(QChar::Tabulation), QString(QChar::Space).repeated(2));
                QStringList sl;
                if (!parent) {
                    sl << tr("Key") << name << tr("Lua code (%1:%2)").arg(index + 1).arg(startPos + 1) << whatText;
                    parent = new QTreeWidgetItem(sl);
                    setAllSearchData(parent, EditorViewType::cmKeysView, name, key->getID(), SearchResultIsScript, startPos, index, instance++);
                    treeWidget_searchResults->addTopLevelItem(parent);
                } else {
                    sl << QString() << QString() << tr("Lua code (%1:%2)").arg(index + 1).arg(startPos + 1) << whatText;
                    pItem = new QTreeWidgetItem(parent, sl);
                    setAllSearchData(pItem, EditorViewType::cmKeysView, name, key->getID(), SearchResultIsScript, startPos, index, instance++);
                    parent->addChild(pItem);
                    parent->setExpanded(true);
                }
                ++startPos;
            }
        }

        recursiveSearchKeys(key, s);
    }
}

void dlgTriggerEditor::searchTimers(const QString& s)
{
    std::list<TTimer*> nodes = mpHost->getTimerUnit()->getTimerRootNodeList();
    for (auto timer : nodes) {
        QTreeWidgetItem* pItem;
        QTreeWidgetItem* parent = nullptr;
        QString name = timer->getName();
        int startPos = 0;

        if ((startPos = name.indexOf(s, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
            QStringList sl;
            sl << tr("Timer") << name << tr("Name");
            // This part can never have a parent as it is the first part of this item
            parent = new QTreeWidgetItem(sl);
            setAllSearchData(parent, EditorViewType::cmTimerView, name, timer->getID(), SearchResultIsName, startPos);
            treeWidget_searchResults->addTopLevelItem(parent);
        }

        // The simple "command"
        // TODO: (A) Revise to count multiple instances of search string within command?
        if (timer->getCommand().contains(s, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) {
            QStringList sl;
            if (!parent) {
                sl << tr("Timer") << name << tr("Command");
                parent = new QTreeWidgetItem(sl);
                setAllSearchData(parent, EditorViewType::cmTimerView, name, timer->getID(), SearchResultIsCommand, startPos);
                treeWidget_searchResults->addTopLevelItem(parent);
            } else {
                sl << QString() << QString() << tr("Command");
                pItem = new QTreeWidgetItem(parent, sl);
                setAllSearchData(pItem, EditorViewType::cmTimerView, name, timer->getID(), SearchResultIsCommand, startPos);
                parent->addChild(pItem);
                parent->setExpanded(true);
            }
        }

        // Script content
        QStringList textList = timer->getScript().split("\n");
        int total = textList.count();
        for (int index = 0; index < total; ++index) {
            // CHECK: This may NOT be an optimisation...!
            if (textList.at(index).isEmpty() || !textList.at(index).contains(s, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) {
                // Short-cuts that mean we do not have to examine the line in more detail
                continue;
            }

            int instance = 0;
            startPos = 0;
            while ((startPos = textList.at(index).indexOf(s, startPos, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
                QString whatText(textList.at(index));
                whatText.replace(QString(QChar::Tabulation), QString(QChar::Space).repeated(2));
                QStringList sl;
                if (!parent) {
                    sl << tr("Timer") << name << tr("Lua code (%1:%2)").arg(index + 1).arg(startPos + 1) << whatText;
                    parent = new QTreeWidgetItem(sl);
                    setAllSearchData(parent, EditorViewType::cmTimerView, name, timer->getID(), SearchResultIsScript, startPos, index, instance++);
                    treeWidget_searchResults->addTopLevelItem(parent);
                } else {
                    sl << QString() << QString() << tr("Lua code (%1:%2)").arg(index + 1).arg(startPos + 1) << whatText;
                    pItem = new QTreeWidgetItem(parent, sl);
                    setAllSearchData(pItem, EditorViewType::cmTimerView, name, timer->getID(), SearchResultIsScript, startPos, index, instance++);
                    parent->addChild(pItem);
                    parent->setExpanded(true);
                }
                ++startPos;
            }
        }

        recursiveSearchTimers(timer, s);
    }
}

void dlgTriggerEditor::searchActions(const QString& s)
{
    std::list<TAction*> nodes = mpHost->getActionUnit()->getActionRootNodeList();
    for (auto action : nodes) {
        QTreeWidgetItem* pItem;
        QTreeWidgetItem* parent = nullptr;
        QString name = action->getName();
        int startPos = 0;

        if ((startPos = name.indexOf(s, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
            QStringList sl;
            sl << tr("Button") << name << tr("Name");
            // This part can never have a parent as it is the first part of this item
            parent = new QTreeWidgetItem(sl);
            setAllSearchData(parent, EditorViewType::cmActionView, name, action->getID(), SearchResultIsName, startPos);
            treeWidget_searchResults->addTopLevelItem(parent);
        }

        // The simple (down) "command"
        // TODO: (A) Revise to count multiple instances of search string within command?
        if ((startPos = action->getCommandButtonDown().indexOf(s, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
            QStringList sl;
            if (!parent) {
                sl << tr("Button") << name << (action->isPushDownButton() ? tr("Command {Down}") : tr("Command"));
                parent = new QTreeWidgetItem(sl);
                setAllSearchData(parent, EditorViewType::cmActionView, name, action->getID(), SearchResultIsCommand, startPos);
                treeWidget_searchResults->addTopLevelItem(parent);
            } else {
                sl << QString() << QString() << (action->isPushDownButton() ? tr("Command {Down}") : tr("Command"));
                pItem = new QTreeWidgetItem(parent, sl);
                setAllSearchData(pItem, EditorViewType::cmActionView, name, action->getID(), SearchResultIsCommand, startPos);
                parent->addChild(pItem);
                parent->setExpanded(true);
            }
        }

        if (action->isPushDownButton()) {
            // We should only search this field if it IS a push-down button
            // as we can not show it if it is not...!
            if ((startPos = action->getCommandButtonUp().indexOf(s, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
                QStringList sl;
                if (!parent) {
                    sl << tr("Button") << name << tr("Command {Up}");
                    parent = new QTreeWidgetItem(sl);
                    setAllSearchData(parent, EditorViewType::cmActionView, name, action->getID(), SearchResultIsExtraCommand, startPos);
                    treeWidget_searchResults->addTopLevelItem(parent);
                } else {
                    sl << QString() << QString() << tr("Command {Up}");
                    pItem = new QTreeWidgetItem(parent, sl);
                    setAllSearchData(pItem, EditorViewType::cmActionView, name, action->getID(), SearchResultIsExtraCommand, startPos);
                    parent->addChild(pItem);
                    parent->setExpanded(true);
                }
            }
        }

        // Css / StyleSheet
        QStringList textList = action->css.split("\n");
        int total = textList.count();
        for (int index = 0; index < total; ++index) {
            // CHECK: This may NOT be an optimisation...!
            if (textList.at(index).isEmpty() || !textList.at(index).contains(s, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) {
                // Short-cuts that mean we do not have to examine the line in more detail
                continue;
            }

            int instance = 0;
            startPos = 0;
            while ((startPos = textList.at(index).indexOf(s, startPos, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
                QStringList sl;
                if (!parent) {
                    sl << tr("Action") << name << tr("Stylesheet {L: %1 C: %2}").arg(index + 1).arg(startPos + 1) << textList.at(index);
                    parent = new QTreeWidgetItem(sl);
                    setAllSearchData(parent, EditorViewType::cmActionView, name, action->getID(), SearchResultsIsCss, startPos, index, instance++);
                    treeWidget_searchResults->addTopLevelItem(parent);
                } else {
                    sl << QString() << QString() << tr("Stylesheet {L: %1 C: %2}").arg(index + 1).arg(startPos + 1) << textList.at(index);
                    pItem = new QTreeWidgetItem(parent, sl);
                    setAllSearchData(pItem, EditorViewType::cmActionView, name, action->getID(), SearchResultsIsCss, startPos, index, instance++);
                    parent->addChild(pItem);
                    parent->setExpanded(true);
                }
                ++startPos;
            }
        }

        // Script content - now put last
        textList = action->getScript().split("\n");
        total = textList.count();
        for (int index = 0; index < total; ++index) {
            // CHECK: This may NOT be an optimisation...!
            if (textList.at(index).isEmpty() || !textList.at(index).contains(s, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) {
                // Short-cuts that mean we do not have to examine the line in more detail
                continue;
            }

            int instance = 0;
            startPos = 0;
            while ((startPos = textList.at(index).indexOf(s, startPos, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
                QString whatText(textList.at(index));
                whatText.replace(QString(QChar::Tabulation), QString(QChar::Space).repeated(2));
                QStringList sl;
                if (!parent) {
                    sl << tr("Button") << name << tr("Lua code (%1:%2)").arg(index + 1).arg(startPos + 1) << whatText;
                    parent = new QTreeWidgetItem(sl);
                    setAllSearchData(parent, EditorViewType::cmActionView, name, action->getID(), SearchResultIsScript, startPos, index, instance++);
                    treeWidget_searchResults->addTopLevelItem(parent);
                } else {
                    sl << QString() << QString() << tr("Lua code (%1:%2)").arg(index + 1).arg(startPos + 1) << whatText;
                    pItem = new QTreeWidgetItem(parent, sl);
                    setAllSearchData(pItem, EditorViewType::cmActionView, name, action->getID(), SearchResultIsScript, startPos, index, instance++);
                    parent->addChild(pItem);
                    parent->setExpanded(true);
                }
                ++startPos;
            }
        }

        recursiveSearchActions(action, s);
    }
}

void dlgTriggerEditor::searchScripts(const QString& s)
{
    std::list<TScript*> nodes = mpHost->getScriptUnit()->getScriptRootNodeList();
    for (auto script : nodes) {
        QTreeWidgetItem* pItem;
        QTreeWidgetItem* parent = nullptr;
        QString name = script->getName();
        int startPos = 0;

        if ((startPos = name.indexOf(s, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
            QStringList sl;
            sl << tr("Script") << name << tr("Name");
            // This part can never have a parent as it is the first part of this item
            parent = new QTreeWidgetItem(sl);
            setAllSearchData(parent, EditorViewType::cmScriptView, name, script->getID(), SearchResultIsName, startPos);
            treeWidget_searchResults->addTopLevelItem(parent);
        }

        // New: Also search event handlers
        QStringList textList = script->getEventHandlerList();
        int total = textList.count();
        for (int index = 0; index < total; ++index) {
            // CHECK: This may NOT be an optimisation...!
            if (textList.at(index).isEmpty() || !textList.at(index).contains(s, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) {
                // Short-cuts that mean we do not have to examine the line in more detail
                continue;
            }

            int instance = 0;
            startPos = 0;
            while ((startPos = textList.at(index).indexOf(s, startPos, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
                QStringList sl;
                if (!parent) {
                    sl << tr("Script") << name << tr("Event Handler") << textList.at(index);
                    parent = new QTreeWidgetItem(sl);
                    setAllSearchData(parent, EditorViewType::cmScriptView, name, script->getID(), SearchResultIsEventHandler, startPos, index, instance++);
                    treeWidget_searchResults->addTopLevelItem(parent);
                } else {
                    sl << QString() << QString() << tr("Event Handler").arg(index + 1) << textList.at(index);
                    pItem = new QTreeWidgetItem(parent, sl);
                    setAllSearchData(pItem, EditorViewType::cmScriptView, name, script->getID(), SearchResultIsEventHandler, startPos, index, instance++);
                    parent->addChild(pItem);
                    parent->setExpanded(true);
                }
                ++startPos;
            }
        }

        // Script content
        textList = script->getScript().split("\n");
        total = textList.count();
        for (int index = 0; index < total; ++index) {
            // CHECK: This may NOT be an optimisation...!
            if (textList.at(index).isEmpty() || !textList.at(index).contains(s, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) {
                // Short-cuts that mean we do not have to examine the line in more detail
                continue;
            }

            int instance = 0;
            int startPos = 0;
            while ((startPos = textList.at(index).indexOf(s, startPos, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
                QString whatText(textList.at(index));
                whatText.replace(QString(QChar::Tabulation), QString(QChar::Space).repeated(2));
                QStringList sl;
                if (!parent) {
                    sl << tr("Script") << name << tr("Lua code (%1:%2)").arg(index + 1).arg(startPos + 1) << whatText;
                    parent = new QTreeWidgetItem(sl);
                    setAllSearchData(parent, EditorViewType::cmScriptView, name, script->getID(), SearchResultIsScript, startPos, index, instance++);
                    treeWidget_searchResults->addTopLevelItem(parent);
                } else {
                    sl << QString() << QString() << tr("Lua code (%1:%2)").arg(index + 1).arg(startPos + 1) << whatText;
                    pItem = new QTreeWidgetItem(parent, sl);
                    setAllSearchData(pItem, EditorViewType::cmScriptView, name, script->getID(), SearchResultIsScript, startPos, index, instance++);
                    parent->addChild(pItem);
                    parent->setExpanded(true);
                }
                ++startPos;
            }
        }

        recursiveSearchScripts(script, s);
    }
}

void dlgTriggerEditor::searchAliases(const QString& s)
{
    std::list<TAlias*> nodes = mpHost->getAliasUnit()->getAliasRootNodeList();
    for (auto alias : nodes) {
        QTreeWidgetItem* pItem;
        QTreeWidgetItem* parent = nullptr;
        QString name = alias->getName();
        int startPos = 0;

        if ((startPos = name.indexOf(s, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
            QStringList sl;
            sl << tr("Alias") << name << tr("Name");
            parent = new QTreeWidgetItem(sl);
            setAllSearchData(parent, EditorViewType::cmAliasView, name, alias->getID(), SearchResultIsName, startPos);
            treeWidget_searchResults->addTopLevelItem(parent);
        }

        // The simple "command"
        if ((startPos = alias->getCommand().indexOf(s, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
            QStringList sl;
            if (!parent) {
                sl << tr("Alias") << name << tr("Command");
                parent = new QTreeWidgetItem(sl);
                setAllSearchData(parent, EditorViewType::cmAliasView, name, alias->getID(), SearchResultIsCommand, startPos);
                treeWidget_searchResults->addTopLevelItem(parent);
            } else {
                sl << QString() << QString() << tr("Command");
                pItem = new QTreeWidgetItem(parent, sl);
                setAllSearchData(pItem, EditorViewType::cmAliasView, name, alias->getID(), SearchResultIsCommand, startPos);
                parent->addChild(pItem);
                parent->setExpanded(true);
            }
        }

        // There is only ONE entry for "Patterns" for Aliases
        if ((startPos = alias->getRegexCode().indexOf(s, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
            QStringList sl;
            if (!parent) {
                sl << tr("Alias") << name << tr("Pattern") << alias->getRegexCode();
                parent = new QTreeWidgetItem(sl);
                setAllSearchData(parent, EditorViewType::cmAliasView, name, alias->getID(), SearchResultIsPattern, startPos);
                treeWidget_searchResults->addTopLevelItem(parent);
            } else {
                sl << QString() << QString() << tr("Pattern") << alias->getRegexCode();
                pItem = new QTreeWidgetItem(parent, sl);
                setAllSearchData(pItem, EditorViewType::cmAliasView, name, alias->getID(), SearchResultIsPattern, startPos);
                parent->addChild(pItem);
                parent->setExpanded(true);
            }
        }

        // Script content - now put last
        QStringList textList = alias->getScript().split("\n");
        int total = textList.count();
        for (int index = 0; index < total; ++index) {
            // CHECK: This may NOT be an optimisation...!
            if (textList.at(index).isEmpty() || !textList.at(index).contains(s, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) {
                // Short-cuts that mean we do not have to examine the line in more detail
                continue;
            }

            int instance = 0;
            startPos = 0;
            while ((startPos = textList.at(index).indexOf(s, startPos, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
                QString whatText(textList.at(index));
                whatText.replace(QString(QChar::Tabulation), QString(QChar::Space).repeated(2));
                QStringList sl;
                if (!parent) {
                    sl << tr("Alias") << name << tr("Lua code (%1:%2)").arg(index + 1).arg(startPos + 1) << whatText;
                    parent = new QTreeWidgetItem(sl);
                    setAllSearchData(parent, EditorViewType::cmAliasView, name, alias->getID(), SearchResultIsScript, startPos, index, instance++);
                    treeWidget_searchResults->addTopLevelItem(parent);
                } else {
                    sl << QString() << QString() << tr("Lua code (%1:%2)").arg(index + 1).arg(startPos + 1) << whatText;
                    pItem = new QTreeWidgetItem(parent, sl);
                    setAllSearchData(pItem, EditorViewType::cmAliasView, name, alias->getID(), SearchResultIsScript, startPos, index, instance++);
                    parent->addChild(pItem);
                    parent->setExpanded(true);
                }
                ++startPos;
            }
        }

        recursiveSearchAlias(alias, s);
    }
}

void dlgTriggerEditor::searchTriggers(const QString& s)
{
    std::list<TTrigger*> nodes = mpHost->getTriggerUnit()->getTriggerRootNodeList();
    for (auto trigger : nodes) {
        QTreeWidgetItem* pItem;
        QTreeWidgetItem* parent = nullptr;
        QString name = trigger->getName();
        int startPos = 0;

        if ((startPos = name.indexOf(s, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
            QStringList sl;
            sl << tr("Trigger") << name << tr("Name");
            // This part can never have a parent as it is the first part of this item
            parent = new QTreeWidgetItem(sl);
            setAllSearchData(parent, EditorViewType::cmTriggerView, name, trigger->getID(), SearchResultIsName, startPos);
            treeWidget_searchResults->addTopLevelItem(parent);
        }

        // The simple "command"
        // TODO: (A) Revise to count multiple instances of search string within command?
        if ((startPos = trigger->getCommand().indexOf(s, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
            QStringList sl;
            if (!parent) {
                sl << tr("Trigger") << name << tr("Command");
                parent = new QTreeWidgetItem(sl);
                setAllSearchData(parent, EditorViewType::cmTriggerView, name, trigger->getID(), SearchResultIsCommand, startPos);
                treeWidget_searchResults->addTopLevelItem(parent);
            } else {
                sl << QString() << QString() << tr("Command");
                pItem = new QTreeWidgetItem(parent, sl);
                setAllSearchData(pItem, EditorViewType::cmTriggerView, name, trigger->getID(), SearchResultIsCommand, startPos);
                parent->addChild(pItem);
                parent->setExpanded(true);
            }
        }

        // Trigger patterns
        QStringList textList = trigger->getRegexCodeList();
        int total = textList.count();
        for (int index = 0; index < total; ++index) {
            // CHECK: This may NOT be an optimisation...!
            if (textList.at(index).isEmpty() || !textList.at(index).contains(s, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) {
                // Short-cuts that mean we do not have to examine this line in more detail
                continue;
            }

            int instance = 0;
            startPos = 0;
            while ((startPos = textList.at(index).indexOf(s, startPos, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
                QStringList sl;
                if (!parent) {
                    sl << tr("Trigger") << name << tr("Pattern {%1}").arg(index + 1) << textList.at(index);
                    parent = new QTreeWidgetItem(sl);
                    setAllSearchData(parent, EditorViewType::cmTriggerView, name, trigger->getID(), SearchResultIsPattern, startPos, index, instance++);
                    treeWidget_searchResults->addTopLevelItem(parent);
                } else {
                    sl << QString() << QString() << tr("Pattern {%1}").arg(index + 1) << textList.at(index);
                    pItem = new QTreeWidgetItem(parent, sl);
                    setAllSearchData(pItem, EditorViewType::cmTriggerView, name, trigger->getID(), SearchResultIsPattern, startPos, index, instance++);
                    parent->addChild(pItem);
                    parent->setExpanded(true);
                }
                ++startPos;
            }
        }

        // Script content - now put last
        textList = trigger->getScript().split("\n");
        total = textList.count();
        for (int index = 0; index < total; ++index) {
            // CHECK: This may NOT be an optimisation...!
            if (textList.at(index).isEmpty() || !textList.at(index).contains(s, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) {
                // Short-cuts that mean we do not have to examine the line in more detail
                continue;
            }

            int instance = 0;
            startPos = 0;
            while ((startPos = textList.at(index).indexOf(s, startPos, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
                QStringList sl;
                QString whatText(textList.at(index));
                whatText.replace(QString(QChar::Tabulation), QString(QChar::Space).repeated(2));
                if (!parent) {
                    sl << tr("Trigger") << name << tr("Lua code (%1:%2)").arg(index + 1).arg(startPos + 1) << whatText;
                    parent = new QTreeWidgetItem(sl);
                    setAllSearchData(parent, EditorViewType::cmTriggerView, name, trigger->getID(), SearchResultIsScript, startPos, index, instance++);
                    treeWidget_searchResults->addTopLevelItem(parent);
                } else {
                    sl << QString() << QString() << tr("Lua code (%1:%2)").arg(index + 1).arg(startPos + 1) << whatText;
                    pItem = new QTreeWidgetItem(parent, sl);
                    setAllSearchData(pItem, EditorViewType::cmTriggerView, name, trigger->getID(), SearchResultIsScript, startPos, index, instance++);
                    parent->addChild(pItem);
                    parent->setExpanded(true);
                }
                ++startPos;
            }
        }

        recursiveSearchTriggers(trigger, s);
    }
}

void dlgTriggerEditor::recursiveSearchTriggers(TTrigger* pTriggerParent, const QString& s)
{
    std::list<TTrigger*>* childrenList = pTriggerParent->getChildrenList();
    for (auto trigger : *childrenList) {
        QTreeWidgetItem* pItem;
        QTreeWidgetItem* parent = nullptr;
        QString name = trigger->getName();
        int startPos = 0;

        if ((startPos = name.indexOf(s, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
            QStringList sl;
            sl << tr("Trigger") << name << tr("Name");
            // This part can never have a parent as it is the first part of this item
            parent = new QTreeWidgetItem(sl);
            setAllSearchData(parent, EditorViewType::cmTriggerView, name, trigger->getID(), SearchResultIsName, startPos);
            treeWidget_searchResults->addTopLevelItem(parent);
        }

        // The simple "command"
        // TODO: (A) Revise to count multiple instances of search string within command?
        if ((startPos = trigger->getCommand().indexOf(s, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
            QStringList sl;
            if (!parent) {
                sl << tr("Trigger") << name << tr("Command");
                parent = new QTreeWidgetItem(sl);
                setAllSearchData(parent, EditorViewType::cmTriggerView, name, trigger->getID(), SearchResultIsCommand, startPos);
                treeWidget_searchResults->addTopLevelItem(parent);
            } else {
                sl << QString() << QString() << tr("Command");
                pItem = new QTreeWidgetItem(parent, sl);
                setAllSearchData(pItem, EditorViewType::cmTriggerView, name, trigger->getID(), SearchResultIsCommand, startPos);
                parent->addChild(pItem);
                parent->setExpanded(true);
            }
        }

        // Trigger patterns
        QStringList textList = trigger->getRegexCodeList();
        int total = textList.count();
        for (int index = 0; index < total; ++index) {
            if (textList.at(index).isEmpty() || !textList.at(index).contains(s, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) {
                // Short-cuts that mean we do not have to examine the line in more detail
                continue;
            }

            int instance = 0;
            startPos = 0;
            while ((startPos = textList.at(index).indexOf(s, startPos, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
                QStringList sl;
                if (!parent) {
                    sl << tr("Trigger") << name << tr("Pattern {%1}").arg(index + 1) << textList.at(index);
                    parent = new QTreeWidgetItem(sl);
                    setAllSearchData(parent, EditorViewType::cmTriggerView, name, trigger->getID(), SearchResultIsPattern, startPos, index, instance++);
                    treeWidget_searchResults->addTopLevelItem(parent);
                } else {
                    sl << QString() << QString() << tr("Pattern {%1}").arg(index + 1) << textList.at(index);
                    pItem = new QTreeWidgetItem(parent, sl);
                    setAllSearchData(pItem, EditorViewType::cmTriggerView, name, trigger->getID(), SearchResultIsPattern, startPos, index, instance++);
                    parent->addChild(pItem);
                    parent->setExpanded(true);
                }
                ++startPos;
            }
        }

        // Script content - now put last
        textList = trigger->getScript().split("\n");
        total = textList.count();
        for (int index = 0; index < total; ++index) {
            if (textList.at(index).isEmpty() || !textList.at(index).contains(s, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) {
                // Short-cuts that mean we do not have to examine the line in more detail
                continue;
            }

            int instance = 0;
            startPos = 0;
            while ((startPos = textList.at(index).indexOf(s, startPos, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
                // We need to replace tabs in the script with two spaces
                // otherwise the displayed text A) does not match the main
                // editor settings and B). often gets shifted out of view by
                // any leading tabs which are quite common in Lua formatting...!
                QString whatText(textList.at(index));
                whatText.replace(QString(QChar::SpecialCharacter::Tabulation), QString(QChar::Space).repeated(2));
                QStringList sl;
                if (!parent) {
                    sl << tr("Trigger") << name << tr("Lua code (%1:%2)").arg(index+1).arg(startPos+1) << whatText;
                    parent = new QTreeWidgetItem(sl);
                    setAllSearchData(parent, EditorViewType::cmTriggerView, name, trigger->getID(), SearchResultIsScript, startPos, index, instance++);
                    treeWidget_searchResults->addTopLevelItem(parent);
                } else {
                    sl << QString() << QString() << tr("Lua code (%1:%2)").arg(index+1).arg(startPos+1) << whatText;
                    pItem = new QTreeWidgetItem(parent, sl);
                    setAllSearchData(pItem, EditorViewType::cmTriggerView, name, trigger->getID(), SearchResultIsScript, startPos, index, instance++);
                    parent->addChild(pItem);
                    parent->setExpanded(true);
                }
                ++startPos;
            }
        }

        if (trigger->hasChildren()) {
            recursiveSearchTriggers(trigger, s);
        }
    }
}

void dlgTriggerEditor::recursiveSearchAlias(TAlias* pTriggerParent, const QString& s)
{
    std::list<TAlias*>* childrenList = pTriggerParent->getChildrenList();
    for (auto alias : *childrenList) {
        QTreeWidgetItem* pItem;
        QTreeWidgetItem* parent = nullptr;
        QString name = alias->getName();
        int startPos = 0;

        if ((startPos = name.indexOf(s, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
            QStringList sl;
            sl << tr("Alias") << name << tr("Name");
            parent = new QTreeWidgetItem(sl);
            setAllSearchData(parent, EditorViewType::cmAliasView, name, alias->getID(), SearchResultIsName, startPos);
            treeWidget_searchResults->addTopLevelItem(parent);
        }

        // The simple "command"
        if ((startPos = alias->getCommand().indexOf(s, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
            QStringList sl;
            if (!parent) {
                sl << tr("Alias") << name << tr("Command");
                parent = new QTreeWidgetItem(sl);
                setAllSearchData(parent, EditorViewType::cmAliasView, name, alias->getID(), SearchResultIsCommand, startPos);
                treeWidget_searchResults->addTopLevelItem(parent);
            } else {
                sl << QString() << QString() << tr("Command");
                pItem = new QTreeWidgetItem(parent, sl);
                setAllSearchData(pItem, EditorViewType::cmAliasView, name, alias->getID(), SearchResultIsCommand, startPos);
                parent->addChild(pItem);
                parent->setExpanded(true);
            }
        }

        // There is only ONE entry for "Patterns" for Aliases
        if ((startPos = alias->getRegexCode().indexOf(s, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
            QStringList sl;
            if (!parent) {
                sl << tr("Alias") << name << tr("Pattern") << alias->getRegexCode();
                parent = new QTreeWidgetItem(sl);
                setAllSearchData(parent, EditorViewType::cmAliasView, name, alias->getID(), SearchResultIsPattern, startPos);
                treeWidget_searchResults->addTopLevelItem(parent);
            } else {
                sl << QString() << QString() << tr("Pattern") << alias->getRegexCode();
                pItem = new QTreeWidgetItem(parent, sl);
                setAllSearchData(pItem, EditorViewType::cmAliasView, name, alias->getID(), SearchResultIsPattern, startPos);
                parent->addChild(pItem);
                parent->setExpanded(true);
            }
        }

        // Script content - now put last
        QStringList textList = alias->getScript().split("\n");
        int total = textList.count();
        for (int index = 0; index < total; ++index) {
            // CHECK: This may NOT be an optimisation...!
            if (textList.at(index).isEmpty() || !textList.at(index).contains(s, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) {
                // Short-cuts that mean we do not have to examine the line in more detail
                continue;
            }

            int instance = 0;
            startPos = 0;
            while ((startPos = textList.at(index).indexOf(s, startPos, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
                QString whatText(textList.at(index));
                whatText.replace(QString(QChar::SpecialCharacter::Tabulation), QString(QChar::Space).repeated(2));
                QStringList sl;
                if (!parent) {
                    sl << tr("Alias") << name << tr("Lua code (%1:%2)").arg(index + 1).arg(startPos + 1) << whatText;
                    parent = new QTreeWidgetItem(sl);
                    setAllSearchData(parent, EditorViewType::cmAliasView, name, alias->getID(), SearchResultIsScript, startPos, index, instance++);
                    treeWidget_searchResults->addTopLevelItem(parent);
                } else {
                    sl << QString() << QString() << tr("Lua code (%1:%2)").arg(index + 1).arg(startPos + 1) << whatText;
                    pItem = new QTreeWidgetItem(parent, sl);
                    setAllSearchData(pItem, EditorViewType::cmAliasView, name, alias->getID(), SearchResultIsScript, startPos, index, instance++);
                    parent->addChild(pItem);
                    parent->setExpanded(true);
                }
                ++startPos;
            }
        }

        if (alias->hasChildren()) {
            recursiveSearchAlias(alias, s);
        }
    }
}

void dlgTriggerEditor::recursiveSearchScripts(TScript* pTriggerParent, const QString& s)
{
    std::list<TScript*>* childrenList = pTriggerParent->getChildrenList();
    for (auto script : *childrenList) {
        QTreeWidgetItem* pItem;
        QTreeWidgetItem* parent = nullptr;
        QString name = script->getName();
        int startPos = 0;

        if ((startPos = name.indexOf(s, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
            QStringList sl;
            sl << tr("Script") << name << tr("Name");
            // This part can never have a parent as it is the first part of this item
            parent = new QTreeWidgetItem(sl);
            setAllSearchData(parent, EditorViewType::cmScriptView, name, script->getID(), SearchResultIsName, startPos);
            treeWidget_searchResults->addTopLevelItem(parent);
        }

        // New: Also search event handlers
        QStringList textList = script->getEventHandlerList();
        int total = textList.count();
        for (int index = 0; index < total; ++index) {
            // CHECK: This may NOT be an optimisation...!
            if (textList.at(index).isEmpty() || !textList.at(index).contains(s, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) {
                // Short-cuts that mean we do not have to examine the line in more detail
                continue;
            }

            int instance = 0;
            startPos = 0;
            while ((startPos = textList.at(index).indexOf(s, startPos, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
                QStringList sl;
                if (!parent) {
                    sl << tr("Script") << name << tr("Event Handler") << textList.at(index);
                    parent = new QTreeWidgetItem(sl);
                    setAllSearchData(parent, EditorViewType::cmScriptView, name, script->getID(), SearchResultIsEventHandler, startPos, index, instance++);
                    treeWidget_searchResults->addTopLevelItem(parent);
                } else {
                    sl << QString() << QString() << tr("Event Handler").arg(index + 1) << textList.at(index);
                    pItem = new QTreeWidgetItem(parent, sl);
                    setAllSearchData(pItem, EditorViewType::cmScriptView, name, script->getID(), SearchResultIsEventHandler, startPos, index, instance++);
                    parent->addChild(pItem);
                    parent->setExpanded(true);
                }
                ++startPos;
            }
        }

        // Script content
        textList = script->getScript().split("\n");
        total = textList.count();
        for (int index = 0; index < total; ++index) {
            // CHECK: This may NOT be an optimisation...!
            if (textList.at(index).isEmpty() || !textList.at(index).contains(s, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) {
                // Short-cuts that mean we do not have to examine the line in more detail
                continue;
            }

            int instance = 0;
            startPos = 0;
            while ((startPos = textList.at(index).indexOf(s, startPos, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
                QString whatText(textList.at(index));
                whatText.replace(QString(QChar::SpecialCharacter::Tabulation), QString(QChar::Space).repeated(2));
                QStringList sl;
                if (!parent) {
                    sl << tr("Script") << name << tr("Lua code (%1:%2)").arg(index + 1).arg(startPos + 1) << whatText;
                    parent = new QTreeWidgetItem(sl);
                    setAllSearchData(parent, EditorViewType::cmScriptView, name, script->getID(), SearchResultIsScript, startPos, index, instance++);
                    treeWidget_searchResults->addTopLevelItem(parent);
                } else {
                    sl << QString() << QString() << tr("Lua code (%1:%2)").arg(index + 1).arg(startPos + 1) << whatText;
                    pItem = new QTreeWidgetItem(parent, sl);
                    setAllSearchData(pItem, EditorViewType::cmScriptView, name, script->getID(), SearchResultIsScript, startPos, index, instance++);
                    parent->addChild(pItem);
                    parent->setExpanded(true);
                }
                ++startPos;
            }
        }

        if (script->hasChildren()) {
            recursiveSearchScripts(script, s);
        }
    }
}

void dlgTriggerEditor::recursiveSearchActions(TAction* pTriggerParent, const QString& s)
{
    std::list<TAction*>* childrenList = pTriggerParent->getChildrenList();
    for (auto action : *childrenList) {
        QTreeWidgetItem* pItem;
        QTreeWidgetItem* parent = nullptr;
        QString name = action->getName();
        int startPos = 0;

        if ((startPos = name.indexOf(s, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
            QStringList sl;
            sl << tr("Button") << name << tr("Name");
            // This part can never have a parent as it is the first part of this item
            parent = new QTreeWidgetItem(sl);
            setAllSearchData(parent, EditorViewType::cmActionView, name, action->getID(), SearchResultIsName, startPos);
            treeWidget_searchResults->addTopLevelItem(parent);
        }

        // The simple (down) "command"
        // TODO: (A) Revise to count multiple instances of search string within command?
        if ((startPos = action->getCommandButtonDown().indexOf(s, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
            QStringList sl;
            if (!parent) {
                sl << tr("Button") << name << (action->isPushDownButton() ? tr("Command {Down}") : tr("Command"));
                parent = new QTreeWidgetItem(sl);
                setAllSearchData(parent, EditorViewType::cmActionView, name, action->getID(), SearchResultIsCommand, startPos);
                treeWidget_searchResults->addTopLevelItem(parent);
            } else {
                sl << QString() << QString() << (action->isPushDownButton() ? tr("Command {Down}") : tr("Command"));
                pItem = new QTreeWidgetItem(parent, sl);
                setAllSearchData(pItem, EditorViewType::cmActionView, name, action->getID(), SearchResultIsCommand, startPos);
                parent->addChild(pItem);
                parent->setExpanded(true);
            }
        }

        if (action->isPushDownButton()) {
            // We should only search this field if it IS a push-down button
            // as we can not show it if it is not...!
            if ((startPos = action->getCommandButtonUp().indexOf(s, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
                QStringList sl;
                if (!parent) {
                    sl << tr("Button") << name << tr("Command {Up}");
                    parent = new QTreeWidgetItem(sl);
                    setAllSearchData(parent, EditorViewType::cmActionView, name, action->getID(), SearchResultIsExtraCommand, startPos);
                    treeWidget_searchResults->addTopLevelItem(parent);
                } else {
                    sl << QString() << QString() << tr("Command {Up}");
                    pItem = new QTreeWidgetItem(parent, sl);
                    setAllSearchData(pItem, EditorViewType::cmActionView, name, action->getID(), SearchResultIsExtraCommand, startPos);
                    parent->addChild(pItem);
                    parent->setExpanded(true);
                }
            }
        }

        // Css / StyleSheet
        QStringList textList = action->css.split("\n");
        int total = textList.count();
        for (int index = 0; index < total; ++index) {
            // CHECK: This may NOT be an optimisation...!
            if (textList.at(index).isEmpty() || !textList.at(index).contains(s, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) {
                // Short-cuts that mean we do not have to examine the line in more detail
                continue;
            }

            int instance = 0;
            startPos = 0;
            while ((startPos = textList.at(index).indexOf(s, startPos, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
                QStringList sl;
                if (!parent) {
                    sl << tr("Action") << name << tr("Stylesheet {L: %1 C: %2}").arg(index + 1).arg(startPos + 1) << textList.at(index);
                    parent = new QTreeWidgetItem(sl);
                    setAllSearchData(parent, EditorViewType::cmActionView, name, action->getID(), SearchResultsIsCss, startPos, index, instance++);
                    treeWidget_searchResults->addTopLevelItem(parent);
                } else {
                    sl << QString() << QString() << tr("Stylesheet {L: %1 C: %2}").arg(index + 1).arg(startPos + 1) << textList.at(index);
                    pItem = new QTreeWidgetItem(parent, sl);
                    setAllSearchData(pItem, EditorViewType::cmActionView, name, action->getID(), SearchResultsIsCss, startPos, index, instance++);
                    parent->addChild(pItem);
                    parent->setExpanded(true);
                }
                ++startPos;
            }
        }

        // Script content - now put last
        textList = action->getScript().split("\n");
        total = textList.count();
        for (int index = 0; index < total; ++index) {
            // CHECK: This may NOT be an optimisation...!
            if (textList.at(index).isEmpty() || !textList.at(index).contains(s, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) {
                // Short-cuts that mean we do not have to examine the line in more detail
                continue;
            }

            int instance = 0;
            startPos = 0;
            while ((startPos = textList.at(index).indexOf(s, startPos, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
                QString whatText(textList.at(index));
                whatText.replace(QString(QChar::SpecialCharacter::Tabulation), QString(QChar::Space).repeated(2));
                QStringList sl;
                if (!parent) {
                    sl << tr("Button") << name << tr("Lua code (%1:%2)").arg(index + 1).arg(startPos + 1) << whatText;
                    parent = new QTreeWidgetItem(sl);
                    setAllSearchData(parent, EditorViewType::cmActionView, name, action->getID(), SearchResultIsScript, startPos, index, instance++);
                    treeWidget_searchResults->addTopLevelItem(parent);
                } else {
                    sl << QString() << QString() << tr("Lua code (%1:%2)").arg(index + 1).arg(startPos + 1) << whatText;
                    pItem = new QTreeWidgetItem(parent, sl);
                    setAllSearchData(pItem, EditorViewType::cmActionView, name, action->getID(), SearchResultIsScript, startPos, index, instance++);
                    parent->addChild(pItem);
                    parent->setExpanded(true);
                }
                ++startPos;
            }
        }

        if (action->hasChildren()) {
            recursiveSearchActions(action, s);
        }
    }
}

void dlgTriggerEditor::recursiveSearchTimers(TTimer* pTriggerParent, const QString& s)
{
    std::list<TTimer*>* childrenList = pTriggerParent->getChildrenList();
    for (auto timer : *childrenList) {
        QTreeWidgetItem* pItem;
        QTreeWidgetItem* parent = nullptr;
        QString name = timer->getName();
        int startPos = 0;

        if ((startPos = name.indexOf(s, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
            QStringList sl;
            sl << tr("Timer") << name << tr("Name");
            // This part can never have a parent as it is the first part of this item
            parent = new QTreeWidgetItem(sl);
            setAllSearchData(parent, EditorViewType::cmTimerView, name, timer->getID(), SearchResultIsName, startPos);
            treeWidget_searchResults->addTopLevelItem(parent);
        }

        // The simple "command"
        // TODO: (A) Revise to count multiple instances of search string within command?
        if ((startPos = timer->getCommand().indexOf(s, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
            QStringList sl;
            if (!parent) {
                sl << tr("Timer") << name << tr("Command");
                parent = new QTreeWidgetItem(sl);
                setAllSearchData(parent, EditorViewType::cmTimerView, name, timer->getID(), SearchResultIsCommand, startPos);
                treeWidget_searchResults->addTopLevelItem(parent);
            } else {
                sl << QString() << QString() << tr("Command");
                pItem = new QTreeWidgetItem(parent, sl);
                setAllSearchData(pItem, EditorViewType::cmTimerView, name, timer->getID(), SearchResultIsCommand, startPos);
                parent->addChild(pItem);
                parent->setExpanded(true);
            }
        }

        // Script content
        QStringList textList = timer->getScript().split("\n");
        int total = textList.count();
        for (int index = 0; index < total; ++index) {
            // CHECK: This may NOT be an optimisation...!
            if (textList.at(index).isEmpty() || !textList.at(index).contains(s, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) {
                // Short-cuts that mean we do not have to examine the line in more detail
                continue;
            }

            int instance = 0;
            startPos = 0;
            while ((startPos = textList.at(index).indexOf(s, startPos, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
                QString whatText(textList.at(index));
                whatText.replace(QString(QChar::SpecialCharacter::Tabulation), QString(QChar::Space).repeated(2));
                QStringList sl;
                if (!parent) {
                    sl << tr("Timer") << name << tr("Lua code (%1:%2)").arg(index + 1).arg(startPos + 1) << whatText;
                    parent = new QTreeWidgetItem(sl);
                    setAllSearchData(parent, EditorViewType::cmTimerView, name, timer->getID(), SearchResultIsScript, startPos, index, instance++);
                    treeWidget_searchResults->addTopLevelItem(parent);
                } else {
                    sl << QString() << QString() << tr("Lua code (%1:%2)").arg(index + 1).arg(startPos + 1) << whatText;
                    pItem = new QTreeWidgetItem(parent, sl);
                    setAllSearchData(pItem, EditorViewType::cmTimerView, name, timer->getID(), SearchResultIsScript, startPos, index, instance++);
                    parent->addChild(pItem);
                    parent->setExpanded(true);
                }
                ++startPos;
            }
        }

        if (timer->hasChildren()) {
            recursiveSearchTimers(timer, s);
        }
    }
}

void dlgTriggerEditor::recursiveSearchKeys(TKey* pTriggerParent, const QString& s)
{
    std::list<TKey*>* childrenList = pTriggerParent->getChildrenList();
    for (auto key : *childrenList) {
        QTreeWidgetItem* pItem;
        QTreeWidgetItem* parent = nullptr;
        QString name = key->getName();
        int startPos = 0;

        if ((startPos = name.indexOf(s, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
            QStringList sl;
            sl << tr("Key") << name << tr("Name");
            // This part can never have a parent as it is the first part of this item
            parent = new QTreeWidgetItem(sl);
            setAllSearchData(parent, EditorViewType::cmKeysView, name, key->getID(), SearchResultIsName, startPos);
            treeWidget_searchResults->addTopLevelItem(parent);
        }

        // The simple "command"
        // TODO: (A) Revise to count multiple instances of search string within command?
        if ((startPos = key->getCommand().indexOf(s, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
            QStringList sl;
            if (!parent) {
                sl << tr("Key") << name << tr("Command");
                parent = new QTreeWidgetItem(sl);
                setAllSearchData(parent, EditorViewType::cmKeysView, name, key->getID(), SearchResultIsCommand, startPos);
                treeWidget_searchResults->addTopLevelItem(parent);
            } else {
                sl << QString() << QString() << tr("Command");
                pItem = new QTreeWidgetItem(parent, sl);
                setAllSearchData(pItem, EditorViewType::cmKeysView, name, key->getID(), SearchResultIsCommand, startPos);
                parent->addChild(pItem);
                parent->setExpanded(true);
            }
        }

        // Script content
        QStringList textList = key->getScript().split("\n");
        int total = textList.count();
        for (int index = 0; index < total; ++index) {
            // CHECK: This may NOT be an optimisation...!
            if (textList.at(index).isEmpty() || !textList.at(index).contains(s, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) {
                // Short-cuts that mean we do not have to examine the line in more detail
                continue;
            }

            int instance = 0;
            startPos = 0;
            while ((startPos = textList.at(index).indexOf(s, startPos, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
                QString whatText(textList.at(index));
                whatText.replace(QString(QChar::SpecialCharacter::Tabulation), QString(QChar::Space).repeated(2));
                QStringList sl;
                if (!parent) {
                    sl << tr("Key") << name << tr("Lua code (%1:%2)").arg(index + 1).arg(startPos + 1) << whatText;
                    parent = new QTreeWidgetItem(sl);
                    setAllSearchData(parent, EditorViewType::cmKeysView, name, key->getID(), SearchResultIsScript, startPos, index, instance++);
                    treeWidget_searchResults->addTopLevelItem(parent);
                } else {
                    sl << QString() << QString() << tr("Lua code (%1:%2)").arg(index + 1).arg(startPos + 1) << whatText;
                    pItem = new QTreeWidgetItem(parent, sl);
                    setAllSearchData(pItem, EditorViewType::cmKeysView, name, key->getID(), SearchResultIsScript, startPos, index, instance++);
                    parent->addChild(pItem);
                    parent->setExpanded(true);
                }
                ++startPos;
            }
        }

        if (key->hasChildren()) {
            recursiveSearchKeys(key, s);
        }
    }
}

void dlgTriggerEditor::delete_alias()
{
    QTreeWidgetItem* pItem = treeWidget_aliases->currentItem();
    if (!pItem) {
        return;
    }
    QTreeWidgetItem* pParent = pItem->parent();
    TAlias* pT = mpHost->getAliasUnit()->getAlias(pItem->data(0, Qt::UserRole).toInt());
    if (!pT) {
        return;
    }

    if (pParent) {
        pParent->removeChild(pItem);
    } else {
        qDebug() << "ERROR: dlgTriggerEditor::delete_alias() child to be deleted does not have a parent";
    }
    delete pT;
    mpCurrentAliasItem = nullptr;
}

void dlgTriggerEditor::delete_action()
{
    QTreeWidgetItem* pItem = treeWidget_actions->currentItem();
    if (!pItem) {
        return;
    }
    QTreeWidgetItem* pParent = pItem->parent();
    TAction* pT = mpHost->getActionUnit()->getAction(pItem->data(0, Qt::UserRole).toInt());
    if (!pT) {
        return;
    }

    // if active, deactivate.
    if (pT->isActive()) {
        pT->deactivate();
    }

    // set this and the parent TActions as changed so the toolbar is updated.
    pT->setDataChanged();

    if (pParent) {
        pParent->removeChild(pItem);
    } else {
        qDebug() << "ERROR: dlgTriggerEditor::delete_action() child to be deleted does not have a parent";
    }
    delete pT;
    mpCurrentActionItem = nullptr;
    mpHost->getActionUnit()->updateToolbar();
}

void dlgTriggerEditor::delete_variable()
{
    QTreeWidgetItem* pItem = treeWidget_variables->currentItem();
    if (!pItem) {
        return;
    }
    QTreeWidgetItem* pParent = pItem->parent();
    LuaInterface* lI = mpHost->getLuaInterface();
    VarUnit* vu = lI->getVarUnit();
    TVar* var = vu->getWVar(pItem);
    if (var) {
        lI->deleteVar(var);
        TVar* parent = var->getParent();
        if (parent) {
            parent->removeChild(var);
        }
        vu->removeVariable(var);
        delete var;
    }
    if (pParent) {
        pParent->removeChild(pItem);
    } else {
        qDebug() << "ERROR: dlgTriggerEditor::delete_action() child to be deleted does not have a parent";
    }
    mpCurrentVarItem = nullptr;
}

void dlgTriggerEditor::delete_script()
{
    QTreeWidgetItem* pItem = treeWidget_scripts->currentItem();
    if (!pItem) {
        return;
    }
    QTreeWidgetItem* pParent = pItem->parent();
    TScript* pT = mpHost->getScriptUnit()->getScript(pItem->data(0, Qt::UserRole).toInt());
    if (!pT) {
        return;
    }

    if (pParent) {
        pParent->removeChild(pItem);
    } else {
        qDebug() << "ERROR: dlgTriggerEditor::delete_script() child to be deleted does not have a parent";
    }
    delete pT;
    mpCurrentScriptItem = nullptr;
}

void dlgTriggerEditor::delete_key()
{
    QTreeWidgetItem* pItem = treeWidget_keys->currentItem();
    if (!pItem) {
        return;
    }
    QTreeWidgetItem* pParent = pItem->parent();

    TKey* pT = mpHost->getKeyUnit()->getKey(pItem->data(0, Qt::UserRole).toInt());
    if (!pT) {
        return;
    }

    if (pParent) {
        pParent->removeChild(pItem);
    } else {
        qDebug() << "ERROR: dlgTriggerEditor::delete_key() child to be deleted does not have a parent";
    }
    delete pT;
    mpCurrentKeyItem = nullptr;
}

void dlgTriggerEditor::delete_trigger()
{
    QTreeWidgetItem* pItem = treeWidget_triggers->currentItem();
    if (!pItem) {
        return;
    }
    QTreeWidgetItem* pParent = pItem->parent();

    TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(pItem->data(0, Qt::UserRole).toInt());
    if (!pT) {
        return;
    }

    if (pParent) {
        pParent->removeChild(pItem);
    } else {
        qDebug() << "ERROR: dlgTriggerEditor::delete_trigger() child to be deleted does not have a parent";
    }
    delete pT;
    mpCurrentTriggerItem = nullptr;
}

void dlgTriggerEditor::delete_timer()
{
    QTreeWidgetItem* pItem = treeWidget_timers->currentItem();
    if (!pItem) {
        return;
    }
    QTreeWidgetItem* pParent = pItem->parent();

    TTimer* pT = mpHost->getTimerUnit()->getTimer(pItem->data(0, Qt::UserRole).toInt());
    if (!pT) {
        return;
    }

    if (pParent) {
        pParent->removeChild(pItem);
    } else {
        qDebug() << "ERROR: dlgTriggerEditor::delete_timer() child to be deleted does not have a parent";
    }
    delete pT;
    mpCurrentTimerItem = nullptr;
}


void dlgTriggerEditor::activeToggle_trigger()
{
    QTreeWidgetItem* pItem = treeWidget_triggers->currentItem();
    if (!pItem) {
        return;
    }
    QIcon icon;

    TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(pItem->data(0, Qt::UserRole).toInt());
    if (!pT) {
        return;
    }

    pT->setIsActive(!pT->shouldBeActive());

    if (pT->isFilterChain()) {
        if (pT->isActive()) {
            if (pT->ancestorsActive()) {
                icon.addPixmap(QPixmap(QStringLiteral(":/icons/filter.png")), QIcon::Normal, QIcon::Off);
            } else {
                icon.addPixmap(QPixmap(QStringLiteral(":/icons/filter-grey.png")), QIcon::Normal, QIcon::Off);
            }
        } else {
            if (pT->ancestorsActive()) {
                icon.addPixmap(QPixmap(QStringLiteral(":/icons/filter-locked.png")), QIcon::Normal, QIcon::Off);
            } else {
                icon.addPixmap(QPixmap(QStringLiteral(":/icons/filter-grey-locked.png")), QIcon::Normal, QIcon::Off);
            }
        }
    } else if (pT->isFolder()) {
        if (pT->isActive()) {
            if (pT->ancestorsActive()) {
                icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-blue.png")), QIcon::Normal, QIcon::Off);
            } else {
                icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
            }
        } else {
            if (pT->ancestorsActive()) {
                icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-blue-locked.png")), QIcon::Normal, QIcon::Off);
            } else {
                icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
            }
        }
    } else {
        if (pT->isActive()) {
            if (pT->ancestorsActive()) {
                icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
            } else {
                icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
            }
        } else {
            if (pT->ancestorsActive()) {
                icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
            } else {
                icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
            }
        }
    }

    if (pT->state()) {
        if (pT->shouldBeActive()) {
            showInfo(tr(R"(Trying to activate a trigger group, filter or trigger or the part of a module "<tt>%1</tt>" that contains them <em>succeeded</em>.)").arg(pT->getName()));
        } else {
            showInfo(tr(R"(Trying to deactivate a trigger group, filter or trigger or the part of a module "<tt>%1</tt>" that contains them <em>succeeded</em>.)").arg(pT->getName()));
        }
    } else {
        pT->setIsActive(false);
        showError(tr(R"(<b>Unable to activate a filter or trigger or the part of a module "<tt>%1</tt>" that contains them; reason: %2.</b></p>
                     <p><i>You will need to reactivate this after the problem has been corrected.</i></p>)").arg(pT->getName(), pT->getError()));
        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
    }
    pItem->setIcon(0, icon);
    pItem->setText(0, pT->getName());

    if (pItem->childCount() > 0) {
        children_icon_triggers(pItem);
    }
}

void dlgTriggerEditor::children_icon_triggers(QTreeWidgetItem* pWidgetItemParent)
{
    for (int i = 0; i < pWidgetItemParent->childCount(); i++) {
        QTreeWidgetItem* pItem = pWidgetItemParent->child(i);
        TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(pItem->data(0, Qt::UserRole).toInt());
        if (!pT) {
            return;
        }

        QIcon icon;
        if (pItem->childCount() > 0) {
            children_icon_triggers(pItem);
        }
        if (pT->state()) {
            if (pT->isFilterChain()) {
                if (pT->isActive()) {
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/filter.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/filter-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                } else {
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/filter-locked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/filter-grey-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            } else if (pT->isFolder()) {
                if (pT->isActive()) {
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-blue.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                } else {
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-blue-locked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            } else {
                if (pT->isActive()) {
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                    }

                } else {
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            pItem->setIcon(0, icon);
        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(QStringLiteral(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            pItem->setIcon(0, iconError);
            showError(pT->getError());
        }
    }
}


void dlgTriggerEditor::activeToggle_timer()
{
    QTreeWidgetItem* pItem = treeWidget_timers->currentItem();
    if (!pItem) {
        return;
    }
    QIcon icon;

    TTimer* pT = mpHost->getTimerUnit()->getTimer(pItem->data(0, Qt::UserRole).toInt());
    if (!pT) {
        return;
    }

    if (!pT->isOffsetTimer()) {
        pT->setIsActive(!pT->shouldBeActive());
    } else {
        pT->setShouldBeActive(!pT->shouldBeActive());
    }

    if (pT->isFolder()) {
        // disable or enable all timers in the respective branch
        // irrespective of the user defined state.
        if (pT->shouldBeActive()) {
            pT->enableTimer(pT->getID());
        } else {
            pT->disableTimer(pT->getID());
        }

        if (pT->shouldBeActive()) {
            icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-green.png")), QIcon::Normal, QIcon::Off);
        } else {
            icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-green-locked.png")), QIcon::Normal, QIcon::Off);
        }
    } else {
        if (pT->isOffsetTimer()) {
            // state of offset timers is managed by the trigger engine
            if (pT->shouldBeActive()) {
                pT->enableTimer(pT->getID());
                icon.addPixmap(QPixmap(QStringLiteral(":/icons/offsettimer-on.png")), QIcon::Normal, QIcon::Off);
            } else {
                pT->disableTimer(pT->getID());
                icon.addPixmap(QPixmap(QStringLiteral(":/icons/offsettimer-off.png")), QIcon::Normal, QIcon::Off);
            }
        } else {
            if (pT->shouldBeActive()) {
                pT->enableTimer(pT->getID());
                icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
            } else {
                pT->disableTimer(pT->getID());
                icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
            }
        }
    }

    if (pT->state()) {
        if (pT->shouldBeActive()) {
            showInfo(tr(R"(Trying to activate a timer group, offset timer, timer or the part of a module "<tt>%1</tt>" that contains them <em>succeeded</em>.)").arg(pT->getName()));
        } else {
            showInfo(tr(R"(Trying to deactivate a timer group, offset timer, timer or the part of a module "<tt>%1</tt>" that contains them <em>succeeded</em>.)").arg(pT->getName()));
        }
    } else {
        pT->setIsActive(false);
        showError(tr(R"(<p><b>Unable to activate an offset timer or timer or the part of a module "<tt>%1</tt>" that contains them; reason: %2.</b></p>
                     <p><i>You will need to reactivate this after the problem has been corrected.</i></p>)").arg(pT->getName(), pT->getError()));
        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
    }
    pItem->setIcon(0, icon);
    pItem->setText(0, pT->getName());
}

void dlgTriggerEditor::activeToggle_alias()
{
    QTreeWidgetItem* pItem = treeWidget_aliases->currentItem();
    if (!pItem) {
        return;
    }
    QIcon icon;

    TAlias* pT = mpHost->getAliasUnit()->getAlias(pItem->data(0, Qt::UserRole).toInt());
    if (!pT) {
        return;
    }
    pT->setIsActive(!pT->shouldBeActive());

    if (pT->isFolder()) {
        if (pT->isActive()) {
            icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-violet.png")), QIcon::Normal, QIcon::Off);
        } else {
            icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-violet-locked.png")), QIcon::Normal, QIcon::Off);
        }
    } else {
        if (pT->isActive()) {
            icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
        } else {
            icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
        }
    }

    if (pT->state()) {
        if (pT->shouldBeActive()) {
            showInfo(tr(R"(Trying to activate an alias group, alias or the part of a module "<tt>%1</tt>" that contains them <em>succeeded</em>.)").arg(pT->getName()));
        } else {
            showInfo(tr(R"(Trying to deactivate an alias group, alias or the part of a module "<tt>%1</tt>" that contains them <em>succeeded</em>.)").arg(pT->getName()));
        }
    } else {
        pT->setIsActive(false);
        showError(tr(R"(<p><b>Unable to activate an alias or the part of a module "<tt>%1</tt>" that contains them; reason: %2.</b></p>
                     <p><i>You will need to reactivate this after the problem has been corrected.</i></p>)").arg(pT->getName(), pT->getError()));
        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
    }
    pItem->setIcon(0, icon);
    pItem->setText(0, pT->getName());

    if (pItem->childCount() > 0) {
        children_icon_alias(pItem);
    }
}

void dlgTriggerEditor::children_icon_alias(QTreeWidgetItem* pWidgetItemParent)
{
    for (int i = 0; i < pWidgetItemParent->childCount(); i++) {
        QTreeWidgetItem* pItem = pWidgetItemParent->child(i);
        TAlias* pT = mpHost->getAliasUnit()->getAlias(pItem->data(0, Qt::UserRole).toInt());
        if (!pT) {
            return;
        }

        QIcon icon;
        if (pItem->childCount() > 0) {
            children_icon_alias(pItem);
        }
        if (pT->state()) {
            if (pT->isFolder()) {
                if (pT->isActive()) {
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-violet.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                } else {
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-violet-locked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            } else {
                if (pT->isActive()) {
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                    }

                } else {
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            pItem->setIcon(0, icon);
        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(QStringLiteral(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            pItem->setIcon(0, iconError);
            showError(pT->getError());
        }
    }
}


void dlgTriggerEditor::activeToggle_script()
{
    QTreeWidgetItem* pItem = treeWidget_scripts->currentItem();
    if (!pItem) {
        return;
    }
    QIcon icon;

    TScript* pT = mpHost->getScriptUnit()->getScript(pItem->data(0, Qt::UserRole).toInt());
    if (!pT) {
        return;
    }

    pT->setIsActive(!pT->shouldBeActive());

    if (pT->isFolder()) {
        if (pT->isActive()) {
            icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-orange.png")), QIcon::Normal, QIcon::Off);
        } else {
            icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-orange-locked.png")), QIcon::Normal, QIcon::Off);
        }
    } else {
        if (pT->isActive()) {
            icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
        } else {
            icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
        }
    }

    if (pT->state()) {
        if (pT->shouldBeActive()) {
            showInfo(tr(R"(Trying to activate a script group, script or the part of a module "<tt>%1</tt>" that contains them <em>succeeded</em>.)").arg(pT->getName()));
        } else {
            showInfo(tr(R"(Trying to deactivate a script group, script or the part of a module "<tt>%1</tt>" that contains them <em>succeeded</em>.)").arg(pT->getName()));
        }
    } else {
        pT->setIsActive(false);
        showError(tr(R"(<p><b>Unable to activate a script group or script or the part of a module "<tt>%1</tt>" that contains them; reason: %2.</b></p>
                     <p><i>You will need to reactivate this after the problem has been corrected.</i></p>)").arg(pT->getName(), pT->getError()));
        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
    }
    pItem->setIcon(0, icon);
    pItem->setText(0, pT->getName());
}

void dlgTriggerEditor::activeToggle_action()
{
    QTreeWidgetItem* pItem = treeWidget_actions->currentItem();
    if (!pItem) {
        return;
    }
    QIcon icon;

    TAction* pT = mpHost->getActionUnit()->getAction(pItem->data(0, Qt::UserRole).toInt());
    if (!pT) {
        return;
    }

    pT->setIsActive(!pT->shouldBeActive());
    pT->setDataChanged();

    if (pT->mpToolBar) {
        if (!pT->isActive()) {
            pT->mpToolBar->hide();
        } else {
            pT->mpToolBar->show();
        }
    }

    if (pT->isFolder()) {
        if (!pT->getPackageName().isEmpty()) {
            // Has a package name - is a module master folder
            if (pT->isActive()) {
                icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
            } else {
                icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
            }
        } else if (!pT->getParent() || !pT->getParent()->getPackageName().isEmpty()) {
            // Does not have a parent or the parent has a package name - is a toolbar
            if (pT->isActive()) {
                icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-yellow.png")), QIcon::Normal, QIcon::Off);
            } else {
                icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-yellow-locked.png")), QIcon::Normal, QIcon::Off);
            }
        } else {
            // Must be a menu
            if (pT->isActive()) {
                icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-cyan.png")), QIcon::Normal, QIcon::Off);
            } else {
                icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-cyan-locked.png")), QIcon::Normal, QIcon::Off);
            }
        }
    } else {
        if (pT->isActive()) {
            icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
        } else {
            icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
        }
    }

    if (pT->state()) {
        if (pT->shouldBeActive()) {
            showInfo(tr(R"(Trying to activate a button/menu/toolbar or the part of a module "<tt>%1</tt>" that contains them <em>succeeded</em>.)").arg(pT->getName()));
        } else {
            showInfo(tr(R"(Trying to deactivate a button/menu/toolbar or the part of a module "<tt>%1</tt>" that contains them <em>succeeded</em>.)").arg(pT->getName()));
        }
    } else {
        pT->setIsActive(false);
        showError(tr(R"(<p><b>Unable to activate a button/menu/toolbar or the part of a module "<tt>%1</tt>" that contains them; reason: %2.</b></p>
                     <p><i>You will need to reactivate this after the problem has been corrected.</i></p>)").arg(pT->getName(), pT->getError()));
        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
    }
    pItem->setIcon(0, icon);
    pItem->setText(0, pT->getName());

    mpHost->getActionUnit()->updateToolbar();
}

void dlgTriggerEditor::activeToggle_key()
{
    QTreeWidgetItem* pItem = treeWidget_keys->currentItem();
    if (!pItem) {
        return;
    }
    QIcon icon;

    TKey* pT = mpHost->getKeyUnit()->getKey(pItem->data(0, Qt::UserRole).toInt());
    if (!pT) {
        return;
    }

    pT->setIsActive(!pT->shouldBeActive());

    if (pT->isFolder()) {
        if (pT->isActive()) {
            icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-pink.png")), QIcon::Normal, QIcon::Off);
        } else {
            icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-pink-locked.png")), QIcon::Normal, QIcon::Off);
        }
    } else {
        if (pT->isActive()) {
            icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
        } else {
            icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
        }
    }

    if (pT->state()) {
        pItem->setIcon(0, icon);
        pItem->setText(0, pT->getName());
    } else {
        QIcon iconError;
        iconError.addPixmap(QPixmap(QStringLiteral(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
        pItem->setIcon(0, iconError);
    }
    showInfo(
            QString("Trying to %2 key <em>%1</em> %3.").arg(pT->getName(), pT->shouldBeActive() ? "activate" : "deactivate", pT->state() ? "succeeded" : QString("failed; reason: ") + pT->getError()));
    if (pItem->childCount() > 0) {
        children_icon_key(pItem);
    }
}

void dlgTriggerEditor::children_icon_key(QTreeWidgetItem* pWidgetItemParent)
{
    for (int i = 0; i < pWidgetItemParent->childCount(); i++) {
        QTreeWidgetItem* pItem = pWidgetItemParent->child(i);
        TKey* pT = mpHost->getKeyUnit()->getKey(pItem->data(0, Qt::UserRole).toInt());
        if (!pT) {
            return;
        }

        QIcon icon;
        if (pItem->childCount() > 0) {
            children_icon_key(pItem);
        }
        if (pT->state()) {
            if (pT->isFolder()) {
                if (pT->isActive()) {
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-pink.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                } else {
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-pink-locked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            } else {
                if (pT->isActive()) {
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                    }

                } else {
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            pItem->setIcon(0, icon);
        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(QStringLiteral(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            pItem->setIcon(0, iconError);
            showError(pT->getError());
        }
    }
}


void dlgTriggerEditor::addTrigger(bool isFolder)
{
    saveTrigger();
    QString name;
    if (isFolder) {
        name = tr("New trigger group");
    } else {
        name = tr("New trigger");
    }
    QStringList regexList;
    QList<int> regexPropertyList;
    QString script = "";
    QStringList nameL;
    nameL << name;

    QTreeWidgetItem* pParent = treeWidget_triggers->currentItem();
    QTreeWidgetItem* pNewItem = nullptr;
    TTrigger* pT = nullptr;

    if (pParent) {
        int parentID = pParent->data(0, Qt::UserRole).toInt();

        TTrigger* pParentTrigger = mpHost->getTriggerUnit()->getTrigger(parentID);
        if (pParentTrigger) {
            // insert new items as siblings unless the parent is a folder
            if (!pParentTrigger->isFolder()) {
                // handle root items
                if (!pParentTrigger->getParent()) {
                    goto ROOT_TRIGGER;
                } else {
                    // insert new item as sibling of the clicked item
                    if (pParent->parent()) {
                        pT = new TTrigger(pParentTrigger->getParent(), mpHost);
                        pNewItem = new QTreeWidgetItem(pParent->parent(), nameL);
                        pParent->parent()->insertChild(0, pNewItem);
                    }
                }
            } else {
                pT = new TTrigger(pParentTrigger, mpHost);
                pNewItem = new QTreeWidgetItem(pParent, nameL);
                pParent->insertChild(0, pNewItem);
            }
        } else {
            goto ROOT_TRIGGER;
        }
    } else {
    //insert a new root item
    ROOT_TRIGGER:
        pT = new TTrigger(name, regexList, regexPropertyList, false, mpHost);
        pNewItem = new QTreeWidgetItem(mpTriggerBaseItem, nameL);
        treeWidget_triggers->insertTopLevelItem(0, pNewItem);
    }

    if (!pT) {
        return;
    }


    pT->setName(name);
    pT->setRegexCodeList(regexList, regexPropertyList);
    pT->setScript(script);
    pT->setIsFolder(isFolder);
    pT->setIsActive(false);
    pT->setIsMultiline(false);
    pT->mStayOpen = 0;
    pT->setConditionLineDelta(0);
    pT->registerTrigger();
    int childID = pT->getID();
    pNewItem->setData(0, Qt::UserRole, childID);
    QIcon icon;
    if (isFolder) {
        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-red.png")), QIcon::Normal, QIcon::Off);
    } else {
        icon.addPixmap(QPixmap(QStringLiteral(":/icons/document-save-as.png")), QIcon::Normal, QIcon::Off);
    }
    pNewItem->setIcon(0, icon);
    if (pParent) {
        pParent->setExpanded(true);
    }
    mpTriggersMainArea->lineEdit_trigger_name->clear();
    mpTriggersMainArea->groupBox_perlSlashGOption->setChecked(false);

    clearDocument(mpSourceEditorEdbee); // New Trigger

    mpTriggersMainArea->lineEdit_trigger_command->clear();
    mpTriggersMainArea->groupBox_filterTrigger->setChecked(false);
    mpTriggersMainArea->spinBox_stayOpen->setValue(0);
    mpTriggersMainArea->spinBox_lineMargin->setValue(0);
    mpTriggersMainArea->groupBox_multiLineTrigger->setChecked(false);

    mpTriggersMainArea->pushButtonFgColor->setChecked(false);
    mpTriggersMainArea->pushButtonBgColor->setChecked(false);
    mpTriggersMainArea->groupBox_triggerColorizer->setChecked(false);

    mpCurrentTriggerItem = pNewItem;
    treeWidget_triggers->setCurrentItem(pNewItem);
    showInfo(msgInfoAddTrigger);
    slot_trigger_selected(treeWidget_triggers->currentItem());
}


void dlgTriggerEditor::addTimer(bool isFolder)
{
    saveTimer();
    QString name;
    if (isFolder) {
        name = tr("New timer group");
    } else {
        name = tr("New timer");
    }
    QString command = "";
    QTime time;
    QString script = "";
    QStringList nameL;
    nameL << name;

    QTreeWidgetItem* pParent = treeWidget_timers->currentItem();
    QTreeWidgetItem* pNewItem = nullptr;
    TTimer* pT = nullptr;

    if (pParent) {
        int parentID = pParent->data(0, Qt::UserRole).toInt();

        TTimer* pParentTrigger = mpHost->getTimerUnit()->getTimer(parentID);
        if (pParentTrigger) {
            // insert new items as siblings unless the parent is a folder
            if (!pParentTrigger->isFolder()) {
                // handle root items
                if (!pParentTrigger->getParent()) {
                    goto ROOT_TIMER;
                } else {
                    // insert new item as sibling of the clicked item
                    if (pParent->parent()) {
                        pT = new TTimer(pParentTrigger->getParent(), mpHost);
                        pNewItem = new QTreeWidgetItem(pParent->parent(), nameL);
                        pParent->parent()->insertChild(0, pNewItem);
                    }
                }
            } else {
                pT = new TTimer(pParentTrigger, mpHost);
                pNewItem = new QTreeWidgetItem(pParent, nameL);
                pParent->insertChild(0, pNewItem);
            }
        } else {
            goto ROOT_TIMER;
        }
    } else {
    //insert a new root item
    ROOT_TIMER:
        pT = new TTimer(name, time, mpHost);
        pNewItem = new QTreeWidgetItem(mpTimerBaseItem, nameL);
        treeWidget_timers->insertTopLevelItem(0, pNewItem);
    }

    if (!pT) {
        return;
    }

    pT->setName(name);
    pT->setCommand(command);
    pT->setScript(script);
    pT->setIsFolder(isFolder);
    pT->setIsActive(false);
    mpHost->getTimerUnit()->registerTimer(pT);
    int childID = pT->getID();
    pNewItem->setData(0, Qt::UserRole, childID);
    QIcon icon;
    if (isFolder) {
        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-red.png")), QIcon::Normal, QIcon::Off);
    } else {
        icon.addPixmap(QPixmap(QStringLiteral(":/icons/document-save-as.png")), QIcon::Normal, QIcon::Off);
    }
    pNewItem->setIcon(0, icon);
    if (pParent) {
        pParent->setExpanded(true);
    }
    //FIXME
    //mpOptionsAreaTriggers->lineEdit_trigger_name->clear();
    mpTimersMainArea->lineEdit_timer_command->clear();
    clearDocument(mpSourceEditorEdbee); // New Timer
    mpCurrentTimerItem = pNewItem;
    treeWidget_timers->setCurrentItem(pNewItem);
    showInfo(msgInfoAddTimer);
    slot_timer_selected(treeWidget_timers->currentItem());
}

void dlgTriggerEditor::addVar(bool isFolder)
{
    saveVar();
    mpVarsMainArea->comboBox_variable_key_type->setCurrentIndex(0);
    if (isFolder) {
        // Edbee doesn't have a readonly option, so I'm using setEnabled
        mpSourceEditorEdbee->setEnabled(false);
        mpVarsMainArea->comboBox_variable_value_type->setDisabled(true);
        mpVarsMainArea->comboBox_variable_value_type->setCurrentIndex(4);
        mpVarsMainArea->lineEdit_var_name->setText(QString());
        mpVarsMainArea->lineEdit_var_name->setPlaceholderText(tr("Table name..."));

        clearDocument(mpSourceEditorEdbee, QLatin1Literal("NewTable"));
    } else {
        // Edbee doesn't have a readonly option, so I'm using setEnabled
        mpSourceEditorEdbee->setEnabled(true);
        mpVarsMainArea->lineEdit_var_name->setText(QString());
        mpVarsMainArea->lineEdit_var_name->setPlaceholderText(tr("Variable name..."));
        mpVarsMainArea->comboBox_variable_value_type->setDisabled(false);
        mpVarsMainArea->comboBox_variable_value_type->setCurrentIndex(0);
    }

    QStringList nameL;
    nameL << QString();
    QTreeWidgetItem* cItem = treeWidget_variables->currentItem();
    LuaInterface* lI = mpHost->getLuaInterface();
    VarUnit* vu = lI->getVarUnit();
    TVar* cVar = vu->getWVar(cItem);
    QTreeWidgetItem* pParent;
    QTreeWidgetItem* pNewItem;
    if (cVar && cVar->getValueType() == LUA_TTABLE) {
        pParent = cItem;
    } else {
        pParent = cItem->parent();
    }

    auto newVar = new TVar();
    if (pParent) {
        //we're nested under something, or going to be.  This HAS to be a table
        TVar* parent = vu->getWVar(pParent);
        if (parent && parent->getValueType() == LUA_TTABLE) {
            //create it under the parent
            pNewItem = new QTreeWidgetItem(pParent, nameL);
            newVar->setParent(parent);
        } else {
            pNewItem = new QTreeWidgetItem(mpVarBaseItem, nameL);
            newVar->setParent(vu->getBase());
        }
    } else {
        pNewItem = new QTreeWidgetItem(mpVarBaseItem, nameL);
        newVar->setParent(vu->getBase());
    }

    if (isFolder) {
        newVar->setValue(QString(), LUA_TTABLE);
    } else {
        newVar->setValueType(LUA_TNONE);
    }
    vu->addTempVar(pNewItem, newVar);
    pNewItem->setFlags(pNewItem->flags() & ~(Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled));
// The following test is pointless - we will already have seg. faulted if pNewItem is a nullptr...!
//    if (pNewItem) {
        mpCurrentVarItem = pNewItem;
        treeWidget_variables->setCurrentItem(pNewItem);
        showInfo(msgInfoAddVar);
        slot_var_selected(treeWidget_variables->currentItem());
//    }
}

void dlgTriggerEditor::addKey(bool isFolder)
{
    saveKey();
    QString name;
    if (isFolder) {
        name = tr("New key group");
    } else {
        name = tr("New key");
    }
    QString script = "";
    QStringList nameL;
    nameL << name;

    QTreeWidgetItem* pParent = treeWidget_keys->currentItem();
    QTreeWidgetItem* pNewItem = nullptr;
    TKey* pT = nullptr;

    if (pParent) {
        int parentID = pParent->data(0, Qt::UserRole).toInt();

        TKey* pParentTrigger = mpHost->getKeyUnit()->getKey(parentID);
        if (pParentTrigger) {
            // insert new items as siblings unless the parent is a folder
            if (!pParentTrigger->isFolder()) {
                // handle root items
                if (!pParentTrigger->getParent()) {
                    goto ROOT_KEY;
                } else {
                    // insert new item as sibling of the clicked item
                    if (pParent->parent()) {
                        pT = new TKey(pParentTrigger->getParent(), mpHost);
                        pNewItem = new QTreeWidgetItem(pParent->parent(), nameL);
                        pParent->parent()->insertChild(0, pNewItem);
                    }
                }
            } else {
                pT = new TKey(pParentTrigger, mpHost);
                pNewItem = new QTreeWidgetItem(pParent, nameL);
                pParent->insertChild(0, pNewItem);
            }
        } else {
            goto ROOT_KEY;
        }
    } else {
    //insert a new root item
    ROOT_KEY:
        pT = new TKey(name, mpHost);
        pNewItem = new QTreeWidgetItem(mpKeyBaseItem, nameL);
        treeWidget_keys->insertTopLevelItem(0, pNewItem);
    }

    if (!pT) {
        return;
    }

    pT->setName(name);
    pT->setKeyCode(-1);
    pT->setKeyModifiers(-1);
    pT->setScript(script);
    pT->setIsFolder(isFolder);
    pT->setIsActive(false);
    pT->registerKey();
    int childID = pT->getID();
    pNewItem->setData(0, Qt::UserRole, childID);
    QIcon icon;
    if (isFolder) {
        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-red.png")), QIcon::Normal, QIcon::Off);
    } else {
        icon.addPixmap(QPixmap(QStringLiteral(":/icons/document-save-as.png")), QIcon::Normal, QIcon::Off);
    }
    pNewItem->setIcon(0, icon);
    if (pParent) {
        pParent->setExpanded(true);
    }
    mpKeysMainArea->lineEdit_key_command->clear();
    mpKeysMainArea->lineEdit_key_binding->setText("no key chosen");
    clearDocument(mpSourceEditorEdbee); // New Key
    mpCurrentKeyItem = pNewItem;
    treeWidget_keys->setCurrentItem(pNewItem);
    showInfo(msgInfoAddKey);
    slot_key_selected(treeWidget_keys->currentItem());
}


void dlgTriggerEditor::addAlias(bool isFolder)
{
    saveAlias();
    QString name;
    if (isFolder) {
        name = tr("New alias group");
    } else {
        name = tr("New alias");
    }
    QString regex = "";
    QString command = "";
    QString script = "";
    QStringList nameL;
    nameL << name;

    QTreeWidgetItem* pParent = treeWidget_aliases->currentItem();
    QTreeWidgetItem* pNewItem = nullptr;
    TAlias* pT = nullptr;

    if (pParent) {
        int parentID = pParent->data(0, Qt::UserRole).toInt();

        TAlias* pParentTrigger = mpHost->getAliasUnit()->getAlias(parentID);
        if (pParentTrigger) {
            // insert new items as siblings unless the parent is a folder
            if (!pParentTrigger->isFolder()) {
                // handle root items
                if (!pParentTrigger->getParent()) {
                    goto ROOT_ALIAS;
                } else {
                    // insert new item as sibling of the clicked item
                    if (pParent->parent()) {
                        pT = new TAlias(pParentTrigger->getParent(), mpHost);
                        pNewItem = new QTreeWidgetItem(pParent->parent(), nameL);
                        pParent->parent()->insertChild(0, pNewItem);
                    }
                }
            } else {
                pT = new TAlias(pParentTrigger, mpHost);
                pNewItem = new QTreeWidgetItem(pParent, nameL);
                pParent->insertChild(0, pNewItem);
            }
        } else {
            goto ROOT_ALIAS;
        }
    } else {
    //insert a new root item
    ROOT_ALIAS:
        pT = new TAlias(name, mpHost);
        pT->setRegexCode(regex); // Empty regex will always succeed to compile
        pNewItem = new QTreeWidgetItem(mpAliasBaseItem, nameL);
        treeWidget_aliases->insertTopLevelItem(0, pNewItem);
    }

    if (!pT) {
        return;
    }

    pT->setName(name);
    pT->setCommand(command);
    pT->setRegexCode(regex); // Empty regex will always succeed to compile
    pT->setScript(script);
    pT->setIsFolder(isFolder);
    pT->setIsActive(false);
    pT->registerAlias();
    int childID = pT->getID();
    pNewItem->setData(0, Qt::UserRole, childID);
    QIcon icon;
    if (isFolder) {
        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-red.png")), QIcon::Normal, QIcon::Off);
    } else {
        icon.addPixmap(QPixmap(QStringLiteral(":/icons/document-save-as.png")), QIcon::Normal, QIcon::Off);
    }
    pNewItem->setIcon(0, icon);
    if (pParent) {
        pParent->setExpanded(true);
    }

    mpAliasMainArea->lineEdit_alias_name->clear();
    mpAliasMainArea->lineEdit_alias_pattern->clear();
    mpAliasMainArea->lineEdit_alias_command->clear();
    clearDocument(mpSourceEditorEdbee); // New Alias

    mpAliasMainArea->lineEdit_alias_name->setText(name);

    mpCurrentAliasItem = pNewItem;
    treeWidget_aliases->setCurrentItem(pNewItem);
    showInfo(msgInfoAddAlias);
    slot_alias_selected(treeWidget_aliases->currentItem());
}

void dlgTriggerEditor::addAction(bool isFolder)
{
    saveAction();
    QString name;
    if (isFolder) {
        name = tr("New menu");
    } else {
        name = tr("New button");
    }
    QString cmdButtonUp = "";
    QString cmdButtonDown = "";
    QString script = "";
    QStringList nameL;
    nameL << name;

    QTreeWidgetItem* pParent = treeWidget_actions->currentItem();
    QTreeWidgetItem* pNewItem = nullptr;
    QPointer<TAction> pT = nullptr;

    if (pParent) {
        int parentID = pParent->data(0, Qt::UserRole).toInt();

        TAction* pParentAction = mpHost->getActionUnit()->getAction(parentID);
        if (pParentAction) {
            // insert new items as siblings unless the parent is a folder
            if (!pParentAction->isFolder()) {
                // handle root items
                if (!pParentAction->getParent()) {
                    goto ROOT_ACTION;
                } else {
                    // insert new item as sibling of the clicked item
                    if (pParent->parent()) {
                        pT = new TAction(pParentAction->getParent(), mpHost);
                        pNewItem = new QTreeWidgetItem(pParent->parent(), nameL);
                        pParent->parent()->insertChild(0, pNewItem);
                    }
                }
            } else {
                pT = new TAction(pParentAction, mpHost);
                pNewItem = new QTreeWidgetItem(pParent, nameL);
                pParent->insertChild(0, pNewItem);
            }
        } else {
            goto ROOT_ACTION;
        }
    } else {
    //insert a new root item
    ROOT_ACTION:
        name = tr("New toolbar");
        pT = new TAction(name, mpHost);
        pT->setCommandButtonUp(cmdButtonUp);
        QStringList nl;
        nl << name;
        pNewItem = new QTreeWidgetItem(mpActionBaseItem, nl);
        treeWidget_actions->insertTopLevelItem(0, pNewItem);
    }

    if (!pT) {
        return;
    }


    pT->setName(name);
    pT->setCommandButtonUp(cmdButtonUp);
    pT->setCommandButtonDown(cmdButtonDown);
    pT->setIsPushDownButton(false);
    pT->mLocation = 1;
    pT->mOrientation = 1;
    pT->setScript(script);
    pT->setIsFolder(isFolder);
    pT->setIsActive(false);
    pT->registerAction();
    int childID = pT->getID();
    pNewItem->setData(0, Qt::UserRole, childID);
    QIcon icon;
    if (isFolder) {
        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-red.png")), QIcon::Normal, QIcon::Off);
    } else {
        icon.addPixmap(QPixmap(QStringLiteral(":/icons/document-save-as.png")), QIcon::Normal, QIcon::Off);
    }
    pNewItem->setIcon(0, icon);
    if (pParent) {
        pParent->setExpanded(true);
    }
    mpActionsMainArea->lineEdit_action_icon->clear();
    mpActionsMainArea->checkBox_action_button_isPushDown->setChecked(false);
    clearDocument(mpSourceEditorEdbee); // New Action


    // This prevents reloading a Floating toolbar when an empty action is added.
    // After the action is saved it may trigger the rebuild.
    pT->setDataSaved();

    mpHost->getActionUnit()->updateToolbar();
    mpCurrentActionItem = pNewItem;
    treeWidget_actions->setCurrentItem(pNewItem);
    showInfo(msgInfoAddButton);
    slot_action_selected(treeWidget_actions->currentItem());
}


void dlgTriggerEditor::addScript(bool isFolder)
{
    saveScript();
    QString name;
    if (isFolder) {
        name = tr("New script group");
    } else {
        name = tr("New script");
    }
    QStringList mainFun;
    mainFun << "-------------------------------------------------\n"
            << "--         Put your Lua functions here.        --\n"
            << "--                                             --\n"
            << "-- Note that you can also use external scripts --\n"
            << "-------------------------------------------------\n";
    QString script = mainFun.join("");
    QStringList nameL;
    nameL << name;

    QTreeWidgetItem* pParent = treeWidget_scripts->currentItem();
    QTreeWidgetItem* pNewItem = nullptr;
    TScript* pT = nullptr;

    if (pParent) {
        int parentID = pParent->data(0, Qt::UserRole).toInt();

        TScript* pParentTrigger = mpHost->getScriptUnit()->getScript(parentID);
        if (pParentTrigger) {
            // insert new items as siblings unless the parent is a folder
            if (!pParentTrigger->isFolder()) {
                // handle root items
                if (!pParentTrigger->getParent()) {
                    goto ROOT_SCRIPT;
                } else {
                    // insert new item as sibling of the clicked item
                    if (pParent->parent()) {
                        pT = new TScript(pParentTrigger->getParent(), mpHost);
                        pNewItem = new QTreeWidgetItem(pParent->parent(), nameL);
                        pParent->parent()->insertChild(0, pNewItem);
                    }
                }
            } else {
                pT = new TScript(pParentTrigger, mpHost);
                pNewItem = new QTreeWidgetItem(pParent, nameL);
                pParent->insertChild(0, pNewItem);
            }
        } else {
            goto ROOT_SCRIPT;
        }
    } else {
    //insert a new root item
    ROOT_SCRIPT:
        pT = new TScript(name, mpHost);
        pNewItem = new QTreeWidgetItem(mpScriptsBaseItem, nameL);
        treeWidget_scripts->insertTopLevelItem(0, pNewItem);
    }

    if (!pT) {
        return;
    }


    pT->setName(name);
    pT->setScript(script);
    pT->setIsFolder(isFolder);
    pT->setIsActive(false);
    pT->registerScript();
    int childID = pT->getID();
    pNewItem->setData(0, Qt::UserRole, childID);
    QIcon icon;
    if (isFolder) {
        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-red.png")), QIcon::Normal, QIcon::Off);
    } else {
        icon.addPixmap(QPixmap(QStringLiteral(":/icons/document-save-as.png")), QIcon::Normal, QIcon::Off);
    }
    pNewItem->setIcon(0, icon);
    if (pParent) {
        pParent->setExpanded(true);
    }
    mpScriptsMainArea->lineEdit_script_name->clear();
    //FIXME mpScriptsMainArea->pattern_textedit->clear();

    clearDocument(mpSourceEditorEdbee, script);
    mpCurrentScriptItem = pNewItem;
    treeWidget_scripts->setCurrentItem(pNewItem);
    slot_scripts_selected(treeWidget_scripts->currentItem());
}

void dlgTriggerEditor::selectTriggerByID(int id)
{
    slot_show_triggers();
    QTreeWidgetItemIterator it(treeWidget_triggers);
    while (*it) {
        if ((*it)->data(0, Qt::UserRole).toInt() == id) {
            slot_trigger_selected((*it));
            treeWidget_triggers->setCurrentItem((*it), 0);
            treeWidget_triggers->scrollToItem((*it));
            mpCurrentTriggerItem = (*it);
            return;
        }
        ++it;
    }
}

void dlgTriggerEditor::selectTimerByID(int id)
{
    slot_show_timers();
    QTreeWidgetItemIterator it(treeWidget_timers);
    while (*it) {
        if ((*it)->data(0, Qt::UserRole).toInt() == id) {
            slot_timer_selected((*it));
            treeWidget_timers->setCurrentItem((*it), 0);
            treeWidget_timers->scrollToItem((*it));
            mpCurrentTimerItem = (*it);
            return;
        }
        ++it;
    }
}

void dlgTriggerEditor::selectAliasByID(int id)
{
    slot_show_aliases();
    QTreeWidgetItemIterator it(treeWidget_aliases);
    while (*it) {
        if ((*it)->data(0, Qt::UserRole).toInt() == id) {
            slot_alias_selected((*it));
            treeWidget_aliases->setCurrentItem((*it), 0);
            treeWidget_aliases->scrollToItem((*it));
            mpCurrentAliasItem = (*it);
            return;
        }
        ++it;
    }
}

void dlgTriggerEditor::selectScriptByID(int id)
{
    slot_show_scripts();
    QTreeWidgetItemIterator it(treeWidget_scripts);
    while (*it) {
        if ((*it)->data(0, Qt::UserRole).toInt() == id) {
            slot_scripts_selected((*it));
            treeWidget_scripts->setCurrentItem((*it), 0);
            treeWidget_scripts->scrollToItem((*it));
            mpCurrentScriptItem = (*it);
            return;
        }
        ++it;
    }
}

void dlgTriggerEditor::selectActionByID(int id)
{
    slot_show_actions();
    QTreeWidgetItemIterator it(treeWidget_actions);
    while (*it) {
        if ((*it)->data(0, Qt::UserRole).toInt() == id) {
            slot_action_selected((*it));
            treeWidget_actions->setCurrentItem((*it), 0);
            treeWidget_actions->scrollToItem((*it));
            mpCurrentActionItem = (*it);
            return;
        }
        ++it;
    }
}

void dlgTriggerEditor::selectKeyByID(int id)
{
    slot_show_keys();
    QTreeWidgetItemIterator it(treeWidget_keys);
    while (*it) {
        if ((*it)->data(0, Qt::UserRole).toInt() == id) {
            slot_key_selected((*it));
            treeWidget_keys->setCurrentItem((*it), 0);
            treeWidget_keys->scrollToItem((*it));
            mpCurrentKeyItem = (*it);
            return;
        }
        ++it;
    }
}

void dlgTriggerEditor::saveTrigger()
{
    QTreeWidgetItem* pItem = mpCurrentTriggerItem;
    if (!pItem) {
        return;
    }
    if (!pItem->parent()) {
        return;
    }

    mpTriggersMainArea->trimName();
    QString name = mpTriggersMainArea->lineEdit_trigger_name->text();
    QString command = mpTriggersMainArea->lineEdit_trigger_command->text();
    bool isMultiline = mpTriggersMainArea->groupBox_multiLineTrigger->isChecked();
    QStringList regexList;
    QList<int> regexPropertyList;
    for (int i = 0; i < 50; i++) {
        QString pattern = mTriggerPatternEdit.at(i)->lineEdit_pattern->text();
        int patternType = mTriggerPatternEdit.at(i)->comboBox_patternType->currentIndex();
        if (pattern.isEmpty() && patternType != REGEX_PROMPT) {
            continue;
        }
        regexList << pattern;

        switch (mTriggerPatternEdit.at(i)->comboBox_patternType->currentIndex()) {
        case 0:
            regexPropertyList << REGEX_SUBSTRING;
            break;
        case 1:
            regexPropertyList << REGEX_PERL;
            break;
        case 2:
            regexPropertyList << REGEX_BEGIN_OF_LINE_SUBSTRING;
            break;
        case 3:
            regexPropertyList << REGEX_EXACT_MATCH;
            break;
        case 4:
            regexPropertyList << REGEX_LUA_CODE;
            break;
        case 5:
            regexPropertyList << REGEX_LINE_SPACER;
            break;
        case 6:
            regexPropertyList << REGEX_COLOR_PATTERN;
            break;
        case 7:
            regexPropertyList << REGEX_PROMPT;
            break;
        }
    }

    QString script = mpSourceEditorEdbeeDocument->text();

    int triggerID = pItem->data(0, Qt::UserRole).toInt();
    TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(triggerID);
    if (pT) {
        QString old_name = pT->getName();
        pT->setName(name);
        pT->setCommand(command);
        pT->setRegexCodeList(regexList, regexPropertyList);

        pT->setScript(script);
        pT->setIsMultiline(isMultiline);
        pT->mPerlSlashGOption = mpTriggersMainArea->groupBox_perlSlashGOption->isChecked();
        pT->mFilterTrigger = mpTriggersMainArea->groupBox_filterTrigger->isChecked();
        pT->setConditionLineDelta(mpTriggersMainArea->spinBox_lineMargin->value());
        pT->mStayOpen = mpTriggersMainArea->spinBox_stayOpen->value();
        pT->mSoundTrigger = mpTriggersMainArea->groupBox_soundTrigger->isChecked();
        pT->setSound(mpTriggersMainArea->lineEdit_soundFile->text());

        QColor fgColor;
        if (!mpTriggersMainArea->pushButtonFgColor->property(cButtonBaseColor).toString().isEmpty()) {
            fgColor = QColor(mpTriggersMainArea->pushButtonFgColor->property(cButtonBaseColor).toString());
        }
        pT->setColorizerFgColor(fgColor);
        QColor bgColor;
        if (!mpTriggersMainArea->pushButtonBgColor->property(cButtonBaseColor).toString().isEmpty()) {
            bgColor = QColor(mpTriggersMainArea->pushButtonBgColor->property(cButtonBaseColor).toString());
        }
        pT->setColorizerBgColor(bgColor);
        pT->setIsColorizerTrigger(mpTriggersMainArea->groupBox_triggerColorizer->isChecked());
        QIcon icon;
        if (pT->isFilterChain()) {
            if (pT->isActive()) {
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/filter.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/filter-grey.png")), QIcon::Normal, QIcon::Off);
                }
            } else {
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/filter-locked.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/filter-grey-locked.png")), QIcon::Normal, QIcon::Off);
                }
            }
        } else if (pT->isFolder()) {
            if (!pT->mPackageName.isEmpty()) {
                if (pT->isActive()) {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
                }
            } else if (pT->isActive()) {
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-blue.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                }
            } else {
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-blue-locked.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                }
            }
        } else {
            if (pT->isActive()) {
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                }
            } else {
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                }
            }
        }
        if (pT->state()) {
            clearEditorNotification();

            if (old_name == tr("New trigger") || old_name == tr("New trigger group")) {
                QIcon _icon;
                if (pT->isFolder()) {
                    _icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-blue.png")), QIcon::Normal, QIcon::Off);
                } else {
                    _icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                }
                pItem->setIcon(0, _icon);
                pItem->setText(0, name);
                pT->setIsActive(true);
            } else {
                pItem->setIcon(0, icon);
                pItem->setText(0, name);
            }
        } else {
            QIcon iconError;
            pItem->setText(0, name);
            iconError.addPixmap(QPixmap(QStringLiteral(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            pItem->setIcon(0, iconError);
            pT->setIsActive(false);
            showError(pT->getError());
        }
    }
}

void dlgTriggerEditor::saveTimer()
{
    QTreeWidgetItem* pItem = mpCurrentTimerItem;
    if (!pItem) {
        return;
    }

    mpTimersMainArea->trimName();
    QString name = mpTimersMainArea->lineEdit_timer_name->text();
    QString script = mpSourceEditorEdbeeDocument->text();


    int timerID = pItem->data(0, Qt::UserRole).toInt();
    TTimer* pT = mpHost->getTimerUnit()->getTimer(timerID);
    if (pT) {
        pT->setName(name);
        QString command = mpTimersMainArea->lineEdit_timer_command->text();
        int hours = mpTimersMainArea->timeEdit_timer_hours->time().hour();
        int minutes = mpTimersMainArea->timeEdit_timer_minutes->time().minute();
        int secs = mpTimersMainArea->timeEdit_timer_seconds->time().second();
        int msecs = mpTimersMainArea->timeEdit_timer_msecs->time().msec();
        QTime time(hours, minutes, secs, msecs);
        pT->setTime(time);
        pT->setCommand(command);
        pT->setName(name);
        pT->setScript(script);

        QIcon icon;
        if (pT->isFolder()) {
            if (!pT->mPackageName.isEmpty()) {
                if (pT->isActive()) {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
                }
            } else {
                if (pT->shouldBeActive()) {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-green.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-green-locked.png")), QIcon::Normal, QIcon::Off);
                }
            }
        }
        if (pT->isOffsetTimer()) {
            if (pT->shouldBeActive()) {
                icon.addPixmap(QPixmap(QStringLiteral(":/icons/offsettimer-on.png")), QIcon::Normal, QIcon::Off);
            } else {
                icon.addPixmap(QPixmap(QStringLiteral(":/icons/offsettimer-off.png")), QIcon::Normal, QIcon::Off);
            }
        } else {
            if (pT->shouldBeActive()) {
                icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                pT->setIsActive(true);
            } else {
                icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
            }
        }

        if (pT->state()) {
            clearEditorNotification();

            pItem->setIcon(0, icon);
            pItem->setText(0, name);
        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(QStringLiteral(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            pItem->setIcon(0, iconError);
            pItem->setText(0, name);
            showError(pT->getError());
        }
    }
}

void dlgTriggerEditor::saveAlias()
{
    QTreeWidgetItem* pItem = mpCurrentAliasItem;
    if (!pItem) {
        return;
    }

    mpAliasMainArea->trimName();
    QString name = mpAliasMainArea->lineEdit_alias_name->text();
    QString regex = mpAliasMainArea->lineEdit_alias_pattern->text();
    if (!regex.isEmpty() && ((name.isEmpty()) || (name == tr("New alias")))) {
        name = regex;
    }
    QString substitution = mpAliasMainArea->lineEdit_alias_command->text();
    //check if sub will trigger regex, ignore if there's nothing in regex - could be an alias group
    QRegularExpression rx(regex);
    QRegularExpressionMatch match = rx.match(substitution);

    if (!regex.isEmpty() && match.capturedStart() != -1) {
        //we have a loop
        QIcon iconError;
        iconError.addPixmap(QPixmap(QStringLiteral(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
        pItem->setIcon(0, iconError);
        pItem->setText(0, name);
        showError(tr("Alias <em>%1</em> has an infinite loop - substitution matches its own pattern. Please fix it - this alias isn't good as it'll call itself forever.").arg(name));
        return;
    }

    QString script = mpSourceEditorEdbeeDocument->text();


    int triggerID = pItem->data(0, Qt::UserRole).toInt();
    TAlias* pT = mpHost->getAliasUnit()->getAlias(triggerID);
    if (pT) {
        QString old_name = pT->getName();
        pT->setName(name);
        pT->setCommand(substitution);
        pT->setRegexCode(regex); // This could generate an error state if regex does not compile
        pT->setScript(script);

        QIcon icon;
        if (pT->isFolder()) {
            if (!pT->mPackageName.isEmpty()) {
                if (pT->isActive()) {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
                }
            } else if (pT->isActive()) {
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-violet.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                }
            } else {
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-violet-locked.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                }
            }
        } else {
            if (pT->isActive()) {
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                }
            } else {
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                }
            }
        }

        if (pT->state()) {
            clearEditorNotification();

            if (old_name == tr("New alias")) {
                QIcon _icon;
                if (pT->isFolder()) {
                    if (pT->ancestorsActive()) {
                        _icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-violet.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        _icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                } else {
                    if (pT->ancestorsActive()) {
                        _icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        _icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
                pItem->setIcon(0, _icon);
                pItem->setText(0, name);
                pT->setIsActive(true);
            } else {
                pItem->setIcon(0, icon);
                pItem->setText(0, name);
            }
        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(QStringLiteral(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            pItem->setIcon(0, iconError);
            pItem->setText(0, name);
            showError(pT->getError());
        }
    }
}

void dlgTriggerEditor::saveAction()
{
    QTreeWidgetItem* pItem = mpCurrentActionItem;
    if (!pItem) {
        return;
    }

    QString name = mpActionsMainArea->lineEdit_action_name->text();
    QString icon = mpActionsMainArea->lineEdit_action_icon->text();
    QString commandDown = mpActionsMainArea->lineEdit_action_button_command_down->text();
    QString commandUp = mpActionsMainArea->lineEdit_action_button_command_up->text();
    QString script = mpSourceEditorEdbeeDocument->text();
    // currentIndex() can return -1 if no setting was previously made - need to fixup:
    int rotation = qMax(0, mpActionsMainArea->comboBox_action_button_rotation->currentIndex());
    int columns = mpActionsMainArea->spinBox_action_bar_columns->text().toInt();
    bool isChecked = mpActionsMainArea->checkBox_action_button_isPushDown->isChecked();
    // bottom location is no longer supported i.e. location = 1 = 0 = location top
    // currentIndex() can return -1 if no setting was previously made - need to fixup:
    int location = qMax(0, mpActionsMainArea->comboBox_action_bar_location->currentIndex());
    if (location > 0) {
        location++;
    }

    // currentIndex() can return -1 if no setting was previously made - need to fixup:
    int orientation = qMax(0, mpActionsMainArea->comboBox_action_bar_orientation->currentIndex());

    // This is an unnecessary level of indentation but has been retained to
    // reduce the noise in a git commit/diff caused by the removal of a
    // redundant "if( pITem )" - can be removed next time the file is modified
    int actionID = pItem->data(0, Qt::UserRole).toInt();
    TAction* pA = mpHost->getActionUnit()->getAction(actionID);
    if (pA) {
        // Check if data has been changed before it gets updated.
        bool actionDataChanged = false;
        if (pA->mLocation != location || pA->mOrientation != orientation || pA->css != mpActionsMainArea->plainTextEdit_action_css->toPlainText()) {
            actionDataChanged = true;
        }

        // Do not change anything for a module master folder - it won't "take"
        if (pA->mPackageName.isEmpty()) {
            pA->setName(name);
            pA->setIcon(icon);
            pA->setScript(script);
            pA->setCommandButtonDown(commandDown);
            pA->setCommandButtonUp(commandUp);
            pA->setIsPushDownButton(isChecked);
            pA->mLocation = location;
            pA->mOrientation = orientation;
            pA->setIsActive(pA->shouldBeActive());
            pA->setButtonRotation(rotation);
            pA->setButtonColumns(columns);
            pA->mUseCustomLayout = false;
            pA->css = mpActionsMainArea->plainTextEdit_action_css->toPlainText();
        }

        QIcon icon;
        if (pA->isFolder()) {
            if (!pA->mPackageName.isEmpty()) {
                // Has a package name so is a module master folder
                if (pA->isActive()) {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
                }
            } else if (!pA->getParent() || !pA->getParent()->mPackageName.isEmpty()) {
                // No parent or it has a parent with a package name so is a toolbar
                if (pA->isActive()) {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-yellow.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-yellow-locked.png")), QIcon::Normal, QIcon::Off);
                }
            } else {
                // Else must be a menu
                if (pA->isActive()) {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-cyan.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-cyan-locked.png")), QIcon::Normal, QIcon::Off);
                }
            }
        } else {
            // Is a button
            if (pA->isActive()) {
                icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
            } else {
                icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
            }
        }

        if (pA->state()) {
            clearEditorNotification();

            pItem->setIcon(0, icon);
            pItem->setText(0, name);
        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(QStringLiteral(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            pItem->setIcon(0, iconError);
            pItem->setText(0, name);
            showError(pA->getError());
        }

        // If not active, don't bother raising the TToolBar for this save.
        if (!pA->shouldBeActive()) {
            pA->setDataSaved();
        }

        if (actionDataChanged) {
            pA->setDataChanged();
        }

        // if the action has a TToolBar instance with a script error, hide that toolbar.
        if (pA->mpToolBar && !pA->state()) {
            pA->mpToolBar->hide();
        }

        // if the action location is changed, make sure the old toolbar instance is hidden.
        if (pA->mLocation == 4 && pA->mpEasyButtonBar) {
            pA->mpEasyButtonBar->hide();
        }
        if (pA->mLocation != 4 && pA->mpToolBar) {
            pA->mpToolBar->hide();
        }
    }

    mpHost->getActionUnit()->updateToolbar();
    mudlet::self()->processEventLoopHack();
}

void dlgTriggerEditor::writeScript(int id)
{
    QTreeWidgetItem* pItem = mpCurrentScriptItem;
    if (!pItem) {
        return;
    }
    if (mCurrentView == EditorViewType::cmUnknownView || mCurrentView != EditorViewType::cmScriptView) {
        return;
    }
    int scriptID = pItem->data(0, Qt::UserRole).toInt();
    if (scriptID != id) {
        return;
    }

    TScript* pT = mpHost->getScriptUnit()->getScript(scriptID);
    if (!pT) {
        return;
    }

    QString scriptCode = pT->getScript();
    mpSourceEditorEdbeeDocument->setText(scriptCode);
}

void dlgTriggerEditor::saveScript()
{
    QTreeWidgetItem* pItem = mpCurrentScriptItem;
    if (!pItem) {
        return;
    }

    QString old_name;

    mpScriptsMainArea->trimName();
    QString name = mpScriptsMainArea->lineEdit_script_name->text();
    QString script = mpSourceEditorEdbeeDocument->text();
    mpScriptsMainAreaEditHandlerItem = nullptr;
    QList<QListWidgetItem*> itemList;
    for (int i = 0; i < mpScriptsMainArea->listWidget_script_registered_event_handlers->count(); i++) {
        QListWidgetItem* pItem = mpScriptsMainArea->listWidget_script_registered_event_handlers->item(i);
        itemList << pItem;
    }
    QStringList handlerList;
    for (auto& listWidgetItem : itemList) {
        if (listWidgetItem->text().size() < 1) {
            continue;
        }
        handlerList << listWidgetItem->text();
    }


    int scriptID = pItem->data(0, Qt::UserRole).toInt();
    TScript* pT = mpHost->getScriptUnit()->getScript(scriptID);
    if (!pT) {
        return;
    }

    old_name = pT->getName();
    pT->setName(name);
    pT->setEventHandlerList(handlerList);
    pT->setScript(script);

    pT->compile();
    mpHost->getTriggerUnit()->doCleanup();
    QIcon icon;
    if (pT->isFolder()) {
        if (!pT->mPackageName.isEmpty()) {
            if (pT->isActive()) {
                icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
            } else {
                icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
            }
        } else {
            if (pT->isActive()) {
                icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-orange.png")), QIcon::Normal, QIcon::Off);
            } else {
                icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-orange-locked.png")), QIcon::Normal, QIcon::Off);
            }
        }
    } else {
        if (pT->isActive()) {
            icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
        } else {
            icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
        }
    }

    if (pT->state()) {
        clearEditorNotification();

        if (old_name == tr("New script") || old_name == tr("New script group")) {
            QIcon _icon;
            if (pT->isFolder()) {
                _icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-orange.png")), QIcon::Normal, QIcon::Off);
            } else {
                _icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
            }
            pItem->setIcon(0, _icon);
            pItem->setText(0, name);
            pT->setIsActive(true);
        } else {
            pItem->setIcon(0, icon);
            pItem->setText(0, name);
        }

    } else {
        QIcon iconError;
        iconError.addPixmap(QPixmap(QStringLiteral(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
        pItem->setIcon(0, iconError);
        pItem->setText(0, name);
        showError(pT->getError());
    }
}

void dlgTriggerEditor::clearEditorNotification() const
{
    mpSystemMessageArea->hide();
}

int dlgTriggerEditor::canRecast(QTreeWidgetItem* pItem, int newNameType, int newValueType)
{
    //basic checks, return 1 if we can recast, 2 if no need to recast, 0 if we can't recast
    LuaInterface* lI = mpHost->getLuaInterface();
    VarUnit* vu = lI->getVarUnit();
    TVar* var = vu->getWVar(pItem);
    if (!var) {
        return 2;
    }
    int currentNameType = var->getKeyType();
    int currentValueType = var->getValueType();
    //most anything is ok to do.  We just want to enforce these rules:
    //you cannot change the type of a table that has children
    //rule removed to see if anything bad happens:
    //you cannot change anything to a table that isn't a table already
    if (currentValueType == LUA_TFUNCTION || currentNameType == LUA_TTABLE) {
        return 0; //no recasting functions or table keys
    }
    if (newValueType == LUA_TTABLE && currentValueType != LUA_TTABLE) {
        //trying to change a table to something else
        if (!var->getChildren(false).empty()) {
            return 0;
        }
        //no children, we can do this without bad things happening
        return 1;
    }
    if (newValueType == LUA_TTABLE && currentValueType != LUA_TTABLE) {
        return 1; //non-table to table
    }
    if (currentNameType == newNameType && currentValueType == newValueType) {
        return 2;
    }
    return 1;
}

void dlgTriggerEditor::saveVar()
{
    // We can enter this function if:
    // we click on a variable without having one selected ( no parent )
    // we click on a variable from another variable
    // we click on a variable from having the top-most element selected ( parent but parent is not a variable/table )
    // we click on a variable from the same variable (such as a double click)
    // we add a new variable
    // we switch away from a variable (so we are saving the old variable)

    if (!mpCurrentVarItem) {
        return;
    }
    QTreeWidgetItem* pItem = mpCurrentVarItem;
    if (!pItem->parent()) {
        return;
    }
    auto* luaInterface = mpHost->getLuaInterface();
    auto* varUnit = luaInterface->getVarUnit();
    TVar* variable = varUnit->getWVar(pItem);
    bool newVar = false;
    if (!variable) {
        newVar = true;
        variable = varUnit->getTVar(pItem);
    }
    if (!variable) {
        return;
    }
    QString newName = mpVarsMainArea->lineEdit_var_name->text();
    QString newValue = mpSourceEditorEdbeeDocument->text();
    if (newName.isEmpty()) {
        slot_var_selected(pItem);
        return;
    }
    mChangingVar = true;
    int uiNameType = mpVarsMainArea->comboBox_variable_key_type->itemData(mpVarsMainArea->comboBox_variable_key_type->currentIndex(), Qt::UserRole).toInt();
    int uiValueType = mpVarsMainArea->comboBox_variable_value_type->itemData(mpVarsMainArea->comboBox_variable_value_type->currentIndex(), Qt::UserRole).toInt();
    if ((uiNameType == LUA_TNUMBER || uiNameType == LUA_TSTRING) && newVar) {
        uiNameType = LUA_TNONE;
    }
    //check variable recasting
    int varRecast = canRecast(pItem, uiNameType, uiValueType);
    if ((uiNameType == -1) || (variable && uiNameType != variable->getKeyType())) {
        if (QString(newName).toInt()) {
            uiNameType = LUA_TNUMBER;
        } else {
            uiNameType = LUA_TSTRING;
        }
    }
    if ((uiValueType != LUA_TTABLE) && (uiValueType == -1)) {
        if (newValue.toInt()) {
            uiValueType = LUA_TNUMBER;
        } else if (newValue.toLower() == "true" || newValue.toLower() == "false") {
            uiValueType = LUA_TBOOLEAN;
        } else {
            uiValueType = LUA_TSTRING;
        }
    }
    if (varRecast == 2) {
        //we sometimes get in here from new variables
        if (newVar) {
            //we're making this var
            variable = varUnit->getTVar(pItem);
            if (!variable) {
                variable = new TVar();
            }
            variable->setName(newName, uiNameType);
            variable->setValue(newValue, uiValueType);
            luaInterface->createVar(variable);
            varUnit->addVariable(variable);
            varUnit->addTreeItem(pItem, variable);
            varUnit->removeTempVar(pItem);
            varUnit->getBase()->addChild(variable);
            pItem->setText(0, newName);
            mpCurrentVarItem = nullptr;
        } else if (variable) {
            if (newName == variable->getName() && (variable->getValueType() == LUA_TTABLE && newValue == variable->getValue())) {
                //no change made
            } else {
                //we're trying to rename it/recast it
                int change = 0;
                if (newName != variable->getName() || uiNameType != variable->getKeyType()) {
                    //lets make sure the nametype works
                    if (variable->getKeyType() == LUA_TNUMBER && newName.toInt()) {
                        uiNameType = LUA_TNUMBER;
                    } else {
                        uiNameType = LUA_TSTRING;
                    }
                    change = change | 0x1;
                }
                variable->setNewName(newName, uiNameType);
                if (variable->getValueType() != LUA_TTABLE && (newValue != variable->getValue() || uiValueType != variable->getValueType())) {
                    //lets check again
                    if (variable->getValueType() == LUA_TTABLE) {
                        //HEIKO: obvious logic error used to be valueType == LUA_TABLE
                        uiValueType = LUA_TTABLE;
                    } else if (uiValueType == LUA_TNUMBER && newValue.toInt()) {
                        uiValueType = LUA_TNUMBER;
                    } else if (uiValueType == LUA_TBOOLEAN && (newValue.toLower() == "true" || newValue.toLower() == "false")) {
                        uiValueType = LUA_TBOOLEAN;
                    } else {
                        uiValueType = LUA_TSTRING; //nope, you don't agree, you lose your value
                    }
                    variable->setValue(newValue, uiValueType);
                    change = change | 0x2;
                }
                if (change) {
                    if (change & 0x1 || newVar) {
                        luaInterface->renameVar(variable);
                    }
                    if ((variable->getValueType() != LUA_TTABLE && change & 0x2) || newVar) {
                        luaInterface->setValue(variable);
                    }
                    pItem->setText(0, newName);
                    mpCurrentVarItem = nullptr;
                } else {
                    variable->clearNewName();
                }
            }
        }
    } else if (varRecast == 1) { //recast it
        TVar* var = varUnit->getWVar(pItem);
        if (newVar) {
            //we're making this var
            var = varUnit->getTVar(pItem);
            var->setName(newName, uiNameType);
            var->setValue(newValue, uiValueType);
            luaInterface->createVar(var);
            varUnit->addVariable(var);
            varUnit->addTreeItem(pItem, var);
            pItem->setText(0, newName);
            mpCurrentVarItem = nullptr;
        } else if (var) {
            //we're trying to rename it/recast it
            int change = 0;
            if (newName != var->getName() || uiNameType != var->getKeyType()) {
                //lets make sure the nametype works
                if (uiNameType == LUA_TSTRING) {
                    //do nothing, we can always make key to string
                } else if (var->getKeyType() == LUA_TNUMBER && newName.toInt()) {
                    uiNameType = LUA_TNUMBER;
                } else {
                    uiNameType = LUA_TSTRING;
                }
                var->setNewName(newName, uiNameType);
                change = change | 0x1;
            }
            if (newValue != var->getValue() || uiValueType != var->getValueType()) {
                //lets check again
                if (uiValueType == LUA_TTABLE) {
                    newValue = "{}";
                } else if (uiValueType == LUA_TNUMBER && newValue.toInt()) {
                    uiValueType = LUA_TNUMBER;
                } else if (uiValueType == LUA_TBOOLEAN && (newValue.toLower() == QLatin1String("true") || newValue.toLower() == QLatin1String("false"))) {
                    uiValueType = LUA_TBOOLEAN;
                } else {
                    uiValueType = LUA_TSTRING; //nope, you don't agree, you lose your value
                }
                var->setValue(newValue, uiValueType);
                change = change | 0x2;
            }
            if (change) {
                if (change & 0x1 || newVar) {
                    luaInterface->renameVar(var);
                }
                if (change & 0x2 || newVar) {
                    luaInterface->setValue(var);
                }
                pItem->setText(0, newName);
                mpCurrentVarItem = nullptr;
            }
        }
    }
    //redo this here in case we changed type
    pItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsTristate | Qt::ItemIsUserCheckable);
    pItem->setToolTip(0, tr("Checked variables will be saved and loaded with your profile."));
    if (!varUnit->shouldSave(variable)) {
        pItem->setFlags(pItem->flags() & ~(Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsUserCheckable));
        pItem->setForeground(0, QBrush(QColor("grey")));
        pItem->setToolTip(0, "");
        pItem->setCheckState(0, Qt::Unchecked);
    } else if (varUnit->isSaved(variable)) {
        pItem->setCheckState(0, Qt::Checked);
    }
    pItem->setData(0, Qt::UserRole, variable->getValueType());
    QIcon icon;
    switch (variable->getValueType()) {
    case 5:
        icon.addPixmap(QPixmap(QStringLiteral(":/icons/table.png")), QIcon::Normal, QIcon::Off);
        break;
    case 6:
        icon.addPixmap(QPixmap(QStringLiteral(":/icons/function.png")), QIcon::Normal, QIcon::Off);
        break;
    default:
        icon.addPixmap(QPixmap(QStringLiteral(":/icons/variable.png")), QIcon::Normal, QIcon::Off);
        break;
    }
    pItem->setIcon(0, icon);
    mChangingVar = false;
    slot_var_selected(pItem);
}

void dlgTriggerEditor::saveKey()
{
    QTreeWidgetItem* pItem = mpCurrentKeyItem;
    if (!pItem) {
        return;
    }

    QString name = mpKeysMainArea->lineEdit_key_name->text();
    if (name.isEmpty()) {
        name = mpKeysMainArea->lineEdit_key_binding->text();
    }
    QString command = mpKeysMainArea->lineEdit_key_command->text();
    QString script = mpSourceEditorEdbeeDocument->text();


    int triggerID = pItem->data(0, Qt::UserRole).toInt();
    TKey* pT = mpHost->getKeyUnit()->getKey(triggerID);
    if (pT) {
        pItem->setText(0, name);
        pT->setName(name);
        pT->setCommand(command);
        pT->setScript(script);

        QIcon icon;
        if (pT->isFolder()) {
            if (!pT->mPackageName.isEmpty()) {
                if (pT->isActive()) {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
                }
            } else if (pT->isActive()) {
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-pink.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                }
            } else {
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-pink-locked.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                }
            }
        } else {
            if (pT->isActive()) {
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                }
            } else {
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                }
            }
        }

        if (pT->state()) {
            clearEditorNotification();
            pItem->setIcon(0, icon);
            pItem->setText(0, name);
        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(QStringLiteral(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            pItem->setIcon(0, iconError);
            pItem->setText(0, name);
            showError(pT->getError());
        }
    }
}

void dlgTriggerEditor::setupPatternControls(const int type, dlgTriggerPatternEdit* pItem)
{
    switch (type) {
    case REGEX_SUBSTRING:
        pItem->lineEdit_pattern->show();
        pItem->pushButton_fgColor->hide();
        pItem->pushButton_bgColor->hide();
        pItem->pushButton_prompt->hide();
        break;
    case REGEX_PERL:
        pItem->lineEdit_pattern->show();
        pItem->pushButton_fgColor->hide();
        pItem->pushButton_bgColor->hide();
        pItem->pushButton_prompt->hide();
        break;
    case REGEX_BEGIN_OF_LINE_SUBSTRING:
        pItem->lineEdit_pattern->show();
        pItem->pushButton_fgColor->hide();
        pItem->pushButton_bgColor->hide();
        pItem->pushButton_prompt->hide();
        break;
    case REGEX_EXACT_MATCH:
        pItem->lineEdit_pattern->show();
        pItem->pushButton_fgColor->hide();
        pItem->pushButton_bgColor->hide();
        pItem->pushButton_prompt->hide();
        break;
    case REGEX_LUA_CODE:
        pItem->lineEdit_pattern->show();
        pItem->pushButton_fgColor->hide();
        pItem->pushButton_bgColor->hide();
        pItem->pushButton_prompt->hide();
        break;
    case REGEX_LINE_SPACER:
        pItem->lineEdit_pattern->show();
        pItem->pushButton_fgColor->hide();
        pItem->pushButton_bgColor->hide();
        pItem->pushButton_prompt->hide();
        break;
    case REGEX_COLOR_PATTERN:
        // CHECKME: Do we need to regenerate (hidden patter text) and button texts/colors?
        pItem->lineEdit_pattern->hide();
        pItem->pushButton_fgColor->show();
        pItem->pushButton_bgColor->show();
        pItem->pushButton_prompt->hide();
        break;
    case REGEX_PROMPT:
        pItem->lineEdit_pattern->hide();
        pItem->pushButton_fgColor->hide();
        pItem->pushButton_bgColor->hide();
        if (mpHost->mTelnet.mGA_Driver) {
            pItem->pushButton_prompt->setText(tr("match on the prompt line"));
            pItem->pushButton_prompt->setToolTip(QString());
        } else {
            pItem->pushButton_prompt->setText(tr("match on the prompt line (disabled)"));
            pItem->pushButton_prompt->setToolTip(tr("A Go-Ahead (GA) signal from the game is required to make this feature work"));
        }
        pItem->pushButton_prompt->show();
        break;
    }
}

// This can get called after the lineEdit contents has changed and it is now a
// color pattern - ought to update coloration if it has been edited by hand
// but need to source the colors
void dlgTriggerEditor::slot_setupPatternControls(int type)
{
    QComboBox* pBox = qobject_cast<QComboBox*>(sender());
    if (!pBox) {
        return;
    }

    int row = pBox->itemData(0).toInt();
    if (row < 0 || row >= 50) {
        return;
    }

    // This is the collection of widgets that make up one of the 50 patterns
    // in the dlgTriggerMainArea:
    dlgTriggerPatternEdit* pPatternItem = mTriggerPatternEdit[row];
    setupPatternControls(type, pPatternItem);
    if (type == REGEX_COLOR_PATTERN) {
        if (pPatternItem->lineEdit_pattern->text().isEmpty()) {
            // This COLOR trigger is a new one in that there is NO text
            // So set it to the default (ignore both) - which will generate an
            // error if saved without setting a color for at least one element:

            pPatternItem->lineEdit_pattern->setText(TTrigger::createColorPatternText(TTrigger::scmIgnored, TTrigger::scmIgnored));
        }

        // Only process the text if it looks like it should:
        if ((pPatternItem->lineEdit_pattern->text().startsWith(QLatin1String("ANSI_COLORS_F{"))
              && pPatternItem->lineEdit_pattern->text().contains(QLatin1String("}_B{"))
              && pPatternItem->lineEdit_pattern->text().endsWith(QLatin1String("}")))) {

            // It looks as though there IS a valid color pattern string in the
            // lineEdit, so, in case it has been edited by hand, regenerate the
            // colors that are used:
            int textAnsiFg = TTrigger::scmIgnored;
            int textAnsiBg = TTrigger::scmIgnored;
            TTrigger::decodeColorPatternText(pPatternItem->lineEdit_pattern->text(), textAnsiFg, textAnsiBg);

            if (textAnsiFg == TTrigger::scmIgnored) {
                pPatternItem->pushButton_fgColor->setStyleSheet(QString());
                pPatternItem->pushButton_fgColor->setText(tr("Foreground color ignored",
                                                             "Color trigger ignored foreground color button, ensure all three instances have the same text"));
            } else if (textAnsiFg == TTrigger::scmDefault) {
                pPatternItem->pushButton_fgColor->setStyleSheet(QString());
                pPatternItem->pushButton_fgColor->setText(tr("Default foreground color",
                                                             "Color trigger default foreground color button, ensure all three instances have the same text"));
            } else {
                pPatternItem->pushButton_fgColor->setStyleSheet(generateButtonStyleSheet(mpHost->getAnsiColor(textAnsiFg, false)));
                pPatternItem->pushButton_fgColor->setText(tr("Foreground color [ANSI %1]",
                                                             "Color trigger ANSI foreground color button, ensure all three instances have the same text")
                                                          .arg(QString::number(textAnsiFg)));
            }

            if (textAnsiBg == TTrigger::scmIgnored) {
                pPatternItem->pushButton_bgColor->setStyleSheet(QString());
                pPatternItem->pushButton_bgColor->setText(tr("Background color ignored",
                                                             "Color trigger ignored background color button, ensure all three instances have the same text"));
            } else if (textAnsiBg == TTrigger::scmDefault) {
                pPatternItem->pushButton_bgColor->setStyleSheet(QString());
                pPatternItem->pushButton_bgColor->setText(tr("Default background color",
                                                             "Color trigger default background color button, ensure all three instances have the same text"));
            } else {
                pPatternItem->pushButton_bgColor->setStyleSheet(generateButtonStyleSheet(mpHost->getAnsiColor(textAnsiBg, true)));
                pPatternItem->pushButton_bgColor->setText(tr("Background color [ANSI %1]",
                                                             "Color trigger ANSI background color button, ensure all three instances have the same text")
                                                          .arg(QString::number(textAnsiBg)));
            }

        } /*else {
            qDebug() << "dlgTriggerEditor::slot_setupPatternControls(...) ERROR: Pattern listed as item:"
                     << row + 1
                     << "is supposed to be a color pattern trigger but the stored text that contains the color codes:"
                     << pPatternItem->lineEdit_pattern->text()
                     << "does not fit the pattern!";
        }*/

    } else {
        // Is NOT a REGEX_COLOR_PATTERN - if the text corresponds to the color
        // pattern text equivalent to ignore both fore and back ground then
        // clear the text - otherwise leave as is:
        if (pPatternItem->lineEdit_pattern->text().compare(QLatin1String("ANSI_COLORS_F{IGNORE}_B{IGNORE}")) == 0) {
            pPatternItem->lineEdit_pattern->clear();
        }
    }
}

void dlgTriggerEditor::slot_trigger_selected(QTreeWidgetItem* pItem)
{
    if (!pItem) {
        return;
    }

    // save the current trigger before switching to the new one
    if (pItem != mpCurrentTriggerItem) {
        saveTrigger();
    }

    mpCurrentTriggerItem = pItem;
    mpTriggersMainArea->show();
    mpSourceEditorArea->show();
    clearEditorNotification();
    mpTriggersMainArea->lineEdit_trigger_name->setText("");
    clearDocument(mpSourceEditorEdbee); // Trigger Select
    mpTriggersMainArea->groupBox_multiLineTrigger->setChecked(false);
    mpTriggersMainArea->groupBox_perlSlashGOption->setChecked(false);
    mpTriggersMainArea->groupBox_filterTrigger->setChecked(false);
    mpTriggersMainArea->groupBox_triggerColorizer->setChecked(false);
    mpTriggersMainArea->pushButtonFgColor->setStyleSheet(QString());
    mpTriggersMainArea->pushButtonFgColor->setProperty(cButtonBaseColor, QVariant());
    mpTriggersMainArea->pushButtonBgColor->setStyleSheet(QString());
    mpTriggersMainArea->pushButtonBgColor->setProperty(cButtonBaseColor, QVariant());
    mpTriggersMainArea->spinBox_lineMargin->setValue(1);

    int ID = pItem->data(0, Qt::UserRole).toInt();
    TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(ID);
    if (pT) {
        QStringList patternList = pT->getRegexCodeList();
        QList<int> propertyList = pT->getRegexCodePropertyList();

        if (patternList.size() != propertyList.size()) {
            return;
        }

        for (int i = 0; i < patternList.size(); i++) {
            if (i >= 50) {
                break; //pattern liste ist momentan auf 50 begrenzt
            }
            if (i >= pT->mColorPatternList.size()) {
                break;
            }
            // Use operator[] so we have write access to the array/list member:
            dlgTriggerPatternEdit* pPatternItem = mTriggerPatternEdit[i];
            int pType = propertyList.at(i);
            if (!pType) {
                // If the control is for the default (0) case nudge the setting
                // up and down so that it copies the coloure icon for the
                // subString type across into the QLineEdit:
                pPatternItem->comboBox_patternType->setCurrentIndex(1);
                setupPatternControls(1, pPatternItem);
            }
            pPatternItem->comboBox_patternType->setCurrentIndex(pType);
            setupPatternControls(pType, pPatternItem);
            if (pType == REGEX_PROMPT) {
                pPatternItem->lineEdit_pattern->clear();

            } else if (pType == REGEX_COLOR_PATTERN) {
                pPatternItem->lineEdit_pattern->setText(patternList.at(i));
                if (pT->mColorPatternList.at(i)) {
                    if (pT->mColorPatternList.at(i)->ansiFg == TTrigger::scmIgnored) {
                        pPatternItem->pushButton_fgColor->setStyleSheet(QString());
                        pPatternItem->pushButton_fgColor->setText(tr("Foreground color ignored",
                                                                     "Color trigger ignored foreground color button, ensure all three instances have the same text"));
                    } else if (pT->mColorPatternList.at(i)->ansiFg == TTrigger::scmDefault) {
                        pPatternItem->pushButton_fgColor->setStyleSheet(QString());
                        pPatternItem->pushButton_fgColor->setText(tr("Default foreground color",
                                                                     "Color trigger default foreground color button, ensure all three instances have the same text"));
                    } else {
                        pPatternItem->pushButton_fgColor->setStyleSheet(generateButtonStyleSheet(pT->mColorPatternList.at(i)->mFgColor));
                        pPatternItem->pushButton_fgColor->setText(tr("Foreground color [ANSI %1]",
                                                                     "Color trigger ANSI foreground color button, ensure all three instances have the same text")
                                                                     .arg(QString::number(pT->mColorPatternList.at(i)->ansiFg)));
                    }

                    if (pT->mColorPatternList.at(i)->ansiBg == TTrigger::scmIgnored) {
                        pPatternItem->pushButton_bgColor->setStyleSheet(QString());
                        pPatternItem->pushButton_bgColor->setText(tr("Background color ignored",
                                                                     "Color trigger ignored background color button, ensure all three instances have the same text"));
                    } else if (pT->mColorPatternList.at(i)->ansiBg == TTrigger::scmDefault) {
                        pPatternItem->pushButton_bgColor->setStyleSheet(QString());
                        pPatternItem->pushButton_bgColor->setText(tr("Default background color",
                                                                     "Color trigger default background color button, ensure all three instances have the same text"));
                    } else {
                        pPatternItem->pushButton_bgColor->setStyleSheet(generateButtonStyleSheet(pT->mColorPatternList.at(i)->mBgColor));
                        pPatternItem->pushButton_bgColor->setText(tr("Background color [ANSI %1]",
                                                                     "Color trigger ANSI background color button, ensure all three instances have the same text")
                                                                  .arg(QString::number(pT->mColorPatternList.at(i)->ansiBg)));
                    }
                } else {
                    qWarning() << "dlgTriggerEditor::slot_trigger_selected(...) ERROR: TTrigger instance has an mColorPattern of size:"
                               << pT->mColorPatternList.size()
                               << "but array element:"
                               << i
                               << "is a nullptr";
                    pPatternItem->pushButton_fgColor->setStyleSheet(QString());
                    pPatternItem->pushButton_fgColor->setText(tr("fault"));
                    pPatternItem->pushButton_bgColor->setStyleSheet(QString());
                    pPatternItem->pushButton_fgColor->setText(tr("fault"));
                }

            } else {
                pPatternItem->lineEdit_pattern->setText(patternList.at(i));
            }
        }

        // reset the rest of the patterns that don't have any data
        for (int i = patternList.size(); i < 50; i++) {
            mTriggerPatternEdit[i]->lineEdit_pattern->clear();
            if (mTriggerPatternEdit[i]->lineEdit_pattern->isHidden()) {
                mTriggerPatternEdit[i]->lineEdit_pattern->show();
            }
            mTriggerPatternEdit[i]->pushButton_fgColor->hide();
            mTriggerPatternEdit[i]->pushButton_bgColor->hide();
            mTriggerPatternEdit[i]->pushButton_prompt->hide();
            // Nudge the type up and down so that the appropriate (coloured) icon is copied across to the QLineEdit:
            mTriggerPatternEdit[i]->comboBox_patternType->setCurrentIndex(1);
            mTriggerPatternEdit[i]->comboBox_patternType->setCurrentIndex(0);
        }
        // Scroll to the last used pattern:
        mpScrollArea->ensureWidgetVisible(mTriggerPatternEdit.at(qBound(0, patternList.size(), 49)));
        QString command = pT->getCommand();
        mpTriggersMainArea->lineEdit_trigger_name->setText(pItem->text(0));
        mpTriggersMainArea->lineEdit_trigger_command->setText(command);
        mpTriggersMainArea->groupBox_multiLineTrigger->setChecked(pT->isMultiline());
        mpTriggersMainArea->groupBox_perlSlashGOption->setChecked(pT->mPerlSlashGOption);
        mpTriggersMainArea->groupBox_filterTrigger->setChecked(pT->mFilterTrigger);
        mpTriggersMainArea->spinBox_lineMargin->setValue(pT->getConditionLineDelta());
        mpTriggersMainArea->spinBox_stayOpen->setValue(pT->mStayOpen);
        mpTriggersMainArea->groupBox_soundTrigger->setChecked(pT->mSoundTrigger);
        if (!pT->mSoundFile.isEmpty()) {
            mpTriggersMainArea->lineEdit_soundFile->setToolTip(pT->mSoundFile);
        }
        mpTriggersMainArea->lineEdit_soundFile->setText(pT->mSoundFile);
        mpTriggersMainArea->lineEdit_soundFile->setCursorPosition(mpTriggersMainArea->lineEdit_soundFile->text().length());
        mpTriggersMainArea->toolButton_clearSoundFile->setEnabled(!mpTriggersMainArea->lineEdit_soundFile->text().isEmpty());
        mpTriggersMainArea->groupBox_triggerColorizer->setChecked(pT->isColorizerTrigger());
        mpTriggersMainArea->pushButtonFgColor->setStyleSheet(generateButtonStyleSheet(pT->getFgColor(), pT->isColorizerTrigger()));
        mpTriggersMainArea->pushButtonFgColor->setProperty(cButtonBaseColor, pT->getFgColor().name());
        mpTriggersMainArea->pushButtonBgColor->setStyleSheet(generateButtonStyleSheet(pT->getBgColor(), pT->isColorizerTrigger()));
        mpTriggersMainArea->pushButtonBgColor->setProperty(cButtonBaseColor, pT->getBgColor().name());

        clearDocument(mpSourceEditorEdbee, pT->getScript());

        if (!pT->state()) {
            showError(pT->getError());
        }

    } else {
        // No details to show - as will be the case if the top item (ID = 0) is
        // selected - so show the help message:
        mpTriggersMainArea->hide();
        mpSourceEditorArea->hide();
        showInfo(msgInfoAddTrigger);
    }
}

void dlgTriggerEditor::slot_alias_selected(QTreeWidgetItem* pItem)
{
    if (!pItem) {
        // No details to show - so show the help message:
        mpAliasMainArea->hide();
        mpSourceEditorArea->hide();
        showInfo(msgInfoAddAlias);
        return;
    }

    // save the current alias before switching to the new one
    if (pItem != mpCurrentAliasItem) {
        saveAlias();
    }

    mpCurrentAliasItem = pItem;
    mpAliasMainArea->show();
    mpSourceEditorArea->show();
    clearEditorNotification();
    mpAliasMainArea->lineEdit_alias_name->clear();
    mpAliasMainArea->lineEdit_alias_pattern->clear();
    mpAliasMainArea->lineEdit_alias_command->clear();
    clearDocument(mpSourceEditorEdbee); // Alias Select

    mpAliasMainArea->lineEdit_alias_name->setText(pItem->text(0));
    int ID = pItem->data(0, Qt::UserRole).toInt();
    TAlias* pT = mpHost->getAliasUnit()->getAlias(ID);
    if (pT) {
        QString pattern = pT->getRegexCode();
        QString command = pT->getCommand();
        QString name = pT->getName();

        mpAliasMainArea->lineEdit_alias_pattern->setText(pattern);
        mpAliasMainArea->lineEdit_alias_command->setText(command);
        mpAliasMainArea->lineEdit_alias_name->setText(name);

        clearDocument(mpSourceEditorEdbee, pT->getScript());

        if (!pT->state()) {
            showError(pT->getError());
        }

    } else {
        // No details to show - as will be the case if the top item (ID = 0) is
        // selected - so show the help message:
        mpAliasMainArea->hide();
        mpSourceEditorArea->hide();
        showInfo(msgInfoAddAlias);
    }
}

void dlgTriggerEditor::slot_key_selected(QTreeWidgetItem* pItem)
{
    if (!pItem) {
        // No details to show - so show the help message:
        mpKeysMainArea->hide();
        mpSourceEditorArea->hide();
        showInfo(msgInfoAddKey);
        return;
    }

    // save the current key before switching to the new one
    if (pItem != mpCurrentKeyItem) {
        saveKey();
    }

    mpCurrentKeyItem = pItem;
    mpKeysMainArea->show();
    mpSourceEditorArea->show();
    clearEditorNotification();
    mpKeysMainArea->lineEdit_key_command->clear();
    mpKeysMainArea->lineEdit_key_binding->clear();
    mpKeysMainArea->lineEdit_key_name->clear();
    clearDocument(mpSourceEditorEdbee); // Key Select

    mpKeysMainArea->lineEdit_key_binding->setText(pItem->text(0));
    int ID = pItem->data(0, Qt::UserRole).toInt();
    TKey* pT = mpHost->getKeyUnit()->getKey(ID);
    if (pT) {
        QString command = pT->getCommand();
        QString name = pT->getName();
        mpKeysMainArea->lineEdit_key_command->clear();
        mpKeysMainArea->lineEdit_key_command->setText(command);
        mpKeysMainArea->lineEdit_key_name->setText(name);
        QString keyName = mpHost->getKeyUnit()->getKeyName(pT->getKeyCode(), pT->getKeyModifiers());
        mpKeysMainArea->lineEdit_key_binding->setText(keyName);

        clearDocument(mpSourceEditorEdbee, pT->getScript());

        if (!pT->state()) {
            showError(pT->getError());
        }
    } else {
        // No details to show - as will be the case if the top item (ID = 0) is
        // selected - so show the help message:
        mpKeysMainArea->hide();
        mpSourceEditorArea->hide();
        showInfo(msgInfoAddKey);
    }
}

// This should not modify the contents of what pItem points at:
void dlgTriggerEditor::recurseVariablesUp(QTreeWidgetItem* const pItem, QList<QTreeWidgetItem*>& list)
{
    QTreeWidgetItem* pParent = pItem->parent();
    if (pParent && pParent != mpVarBaseItem) {
        list.append(pParent);
        recurseVariablesUp(pParent, list);
    }
}

// This should not modify the contents of what pItem points at:
void dlgTriggerEditor::recurseVariablesDown(QTreeWidgetItem* const pItem, QList<QTreeWidgetItem*>& list)
{
    list.append(pItem);
    for (int i = 0; i < pItem->childCount(); ++i) {
        recurseVariablesDown(pItem->child(i), list);
    }
}

// This WAS called recurseVariablesDown(TVar*, QList<TVar*>&, bool) but it is
// used for searching like the other resursiveSearchXxxxx(...) are
void dlgTriggerEditor::recursiveSearchVariables(TVar* var, QList<TVar*>& list, bool isSorted)
{
    list.append(var);
    QListIterator<TVar*> it(var->getChildren(isSorted));
    while (it.hasNext()) {
        recursiveSearchVariables(it.next(), list, isSorted);
    }
}

void dlgTriggerEditor::slot_var_changed(QTreeWidgetItem* pItem)
{
    // This handles a small case where the radio buttom is clicked while the item is currently selected
    // which causes the variable to not save. In places where we populate the TreeWidgetItem, we have
    // to guard it with mChangingVar or else this will be called with every change such as the variable
    // name, etc.
    if (!pItem || mChangingVar) {
        return;
    }
    int column = 0;
    int state = pItem->checkState(column);
    LuaInterface* lI = mpHost->getLuaInterface();
    VarUnit* vu = lI->getVarUnit();
    TVar* var = vu->getWVar(pItem);
    if (!var) {
        return;
    }
    if (state == Qt::Checked || state == Qt::PartiallyChecked) {
        if (vu->isSaved(var)) {
            return;
        }
        vu->addSavedVar(var);
        QList<QTreeWidgetItem*> list;
        recurseVariablesUp(pItem, list);
        for (auto& treeWidgetItem : list) {
            TVar* v = vu->getWVar(treeWidgetItem);
            if (v && (treeWidgetItem->checkState(column) == Qt::Checked || treeWidgetItem->checkState(column) == Qt::PartiallyChecked)) {
                vu->addSavedVar(v);
            }
        }
        list.clear();
        recurseVariablesDown(pItem, list);
        for (auto& treeWidgetItem : list) {
            TVar* v = vu->getWVar(treeWidgetItem);
            if (v && (treeWidgetItem->checkState(column) == Qt::Checked || treeWidgetItem->checkState(column) == Qt::PartiallyChecked)) {
                vu->addSavedVar(v);
            }
        }
    } else {
        // we're not checked, dont save us
        if (!vu->isSaved(var)) {
            return;
        }
        vu->removeSavedVar(var);
        QList<QTreeWidgetItem*> list;
        recurseVariablesUp(pItem, list);
        for (auto& treeWidgetItem : list) {
            TVar* v = vu->getWVar(treeWidgetItem);
            if (v && (treeWidgetItem->checkState(column) == Qt::Checked || treeWidgetItem->checkState(column) == Qt::PartiallyChecked)) {
                vu->removeSavedVar(v);
            }
        }
        list.clear();
        recurseVariablesDown(pItem, list);
        for (auto& treeWidgetItem : list) {
            TVar* v = vu->getWVar(treeWidgetItem);
            if (v && (treeWidgetItem->checkState(column) == Qt::Checked || treeWidgetItem->checkState(column) == Qt::PartiallyChecked)) {
                vu->removeSavedVar(v);
            }
        }
    }
}

void dlgTriggerEditor::slot_var_selected(QTreeWidgetItem* pItem)
{
    if (!pItem ||treeWidget_variables->indexOfTopLevelItem(pItem) == 0) {
        // Null item or it is for the first row of the tree
        mpVarsMainArea->hide();
        mpSourceEditorArea->hide();
        showInfo(msgInfoAddVar);
        return;
    }

    clearEditorNotification();

    // save the current variable before switching to the new one
    if (pItem != mpCurrentVarItem) {
        saveVar();
    }

    mChangingVar = true;
    int column = treeWidget_variables->currentColumn();
    int state = pItem->checkState(column);
    LuaInterface* lI = mpHost->getLuaInterface();
    VarUnit* vu = lI->getVarUnit();
    TVar* var = vu->getWVar(pItem); // This does NOT modify pItem or what it points at
    QList<QTreeWidgetItem*> list;
    if (state == Qt::Checked || state == Qt::PartiallyChecked) {
        if (var) {
            vu->addSavedVar(var);
        }
        recurseVariablesUp(pItem, list); // This does NOT modify pItem or what it points at
        for (auto& treeWidgetItem : list) {
            TVar* v = vu->getWVar(treeWidgetItem);
            if (v && (treeWidgetItem->checkState(column) == Qt::Checked || treeWidgetItem->checkState(column) == Qt::PartiallyChecked)) {
                vu->addSavedVar(v);
            }
        }
        list.clear();
        recurseVariablesDown(pItem, list); // This does NOT modify pItem or what it points at
        for (auto& treeWidgetItem : list) {
            TVar* v = vu->getWVar(treeWidgetItem);
            if (v && (treeWidgetItem->checkState(column) == Qt::Checked || treeWidgetItem->checkState(column) == Qt::PartiallyChecked)) {
                vu->addSavedVar(v);
            }
        }
    } else {
        if (var) {
            vu->removeSavedVar(var);
        }
        recurseVariablesUp(pItem, list); // This does NOT modify pItem or what it points at
        for (auto& treeWidgetItem : list) {
            TVar* v = vu->getWVar(treeWidgetItem);
            if (v && (treeWidgetItem->checkState(column) == Qt::Unchecked)) {
                vu->removeSavedVar(v);
            }
        }
        list.clear();
        recurseVariablesDown(pItem, list); // This does NOT modify pItem or what it points at
        for (auto& treeWidgetItem : list) {
            TVar* v = vu->getWVar(treeWidgetItem);
            if (v && (treeWidgetItem->checkState(column) == Qt::Unchecked)) {
                vu->removeSavedVar(v);
            }
        }
    }
    mpVarsMainArea->show();
    mpSourceEditorArea->show();

    mpCurrentVarItem = pItem; //remember what has been clicked to save it
    // There was repeated test for pItem being null here but we have NOT altered
    // it since the start of the function where it was already tested for not
    // being zero so we don't need to retest it! - Slysven
    if (column) {
        mChangingVar = false;
        return;
    }

    if (!var) {
        mpVarsMainArea->checkBox_variable_hidden->setChecked(false);
        mpVarsMainArea->lineEdit_var_name->clear();
        clearDocument(mpSourceEditorEdbee); // Var Select
        //check for temp item
        var = vu->getTVar(pItem);
        if (var && var->getValueType() == LUA_TTABLE) {
            mpVarsMainArea->comboBox_variable_value_type->setDisabled(true);
            // index 4 = "table"
            mpVarsMainArea->comboBox_variable_value_type->setCurrentIndex(4);
        } else {
            mpVarsMainArea->comboBox_variable_value_type->setDisabled(false);
            // index 0 = "Auto-type"
            mpVarsMainArea->comboBox_variable_value_type->setCurrentIndex(0);
        }
        mpVarsMainArea->comboBox_variable_key_type->setCurrentIndex(0);
        mChangingVar = false;
        return;
    }

    int varType = var->getValueType();
    int keyType = var->getKeyType();
    QIcon icon;

    switch (keyType) {
//    case LUA_TNONE: // -1
//    case LUA_TNIL: // 0
//    case LUA_TBOOLEAN: // 1
//    case LUA_TLIGHTUSERDATA: // 2
    case LUA_TNUMBER: // 3
        // index 2 = "index (integer number)"
        mpVarsMainArea->comboBox_variable_key_type->setCurrentIndex(2);
        mpVarsMainArea->comboBox_variable_key_type->setEnabled(true);
        break;
    case LUA_TSTRING: // 4
        // index 1 = "key (string)"
        mpVarsMainArea->comboBox_variable_key_type->setCurrentIndex(1);
        mpVarsMainArea->comboBox_variable_key_type->setEnabled(true);
        break;
    case LUA_TTABLE: // 5
        // index 3 = "table (use \"Add Group\" to create"
        mpVarsMainArea->comboBox_variable_key_type->setCurrentIndex(3);
        mpVarsMainArea->comboBox_variable_key_type->setEnabled(false);
        break;
    case LUA_TFUNCTION: // 6
        // index 4 = "function (cannot create from GUI)"
        mpVarsMainArea->comboBox_variable_key_type->setCurrentIndex(4);
        mpVarsMainArea->comboBox_variable_key_type->setEnabled(false);
        break;
//    case LUA_TUSERDATA: // 7
//    case LUA_TTHREAD: // 8
    }

    switch (varType) {
    case LUA_TNONE:
        [[fallthrough]];
    case LUA_TNIL:
        mpSourceEditorArea->hide();
        break;
    case LUA_TBOOLEAN:
        mpSourceEditorArea->show();
        mpSourceEditorEdbee->setEnabled(true);
        icon.addPixmap(QPixmap(QStringLiteral(":/icons/variable.png")), QIcon::Normal, QIcon::Off);
        // index 3 = "boolean"
        mpVarsMainArea->comboBox_variable_value_type->setCurrentIndex(3);
        mpVarsMainArea->comboBox_variable_value_type->setEnabled(true);
        break;
    case LUA_TNUMBER:
        mpSourceEditorArea->show();
        mpSourceEditorEdbee->setEnabled(true);
        icon.addPixmap(QPixmap(QStringLiteral(":/icons/variable.png")), QIcon::Normal, QIcon::Off);
        // index 2 = "number"
        mpVarsMainArea->comboBox_variable_value_type->setCurrentIndex(2);
        mpVarsMainArea->comboBox_variable_value_type->setEnabled(true);
        break;
    case LUA_TSTRING:
        mpSourceEditorArea->show();
        mpSourceEditorEdbee->setEnabled(true);
        icon.addPixmap(QPixmap(QStringLiteral(":/icons/variable.png")), QIcon::Normal, QIcon::Off);
        // index 1 = "string"
        mpVarsMainArea->comboBox_variable_value_type->setCurrentIndex(1);
        mpVarsMainArea->comboBox_variable_value_type->setEnabled(true);
        break;
    case LUA_TTABLE:
        mpSourceEditorArea->hide();
        mpSourceEditorEdbee->setEnabled(false);
        // Only allow the type to be changed away from a table if it is empty:
        mpVarsMainArea->comboBox_variable_value_type->setEnabled(!(pItem->childCount() > 0));
        // index 4 = "table"
        mpVarsMainArea->comboBox_variable_value_type->setCurrentIndex(4);
        icon.addPixmap(QPixmap(QStringLiteral(":/icons/table.png")), QIcon::Normal, QIcon::Off);
        break;
    case LUA_TFUNCTION:
        mpSourceEditorArea->hide();
        mpSourceEditorEdbee->setEnabled(false);
        mpVarsMainArea->comboBox_variable_value_type->setCurrentIndex(5);
        mpVarsMainArea->comboBox_variable_value_type->setEnabled(false);
        icon.addPixmap(QPixmap(QStringLiteral(":/icons/function.png")), QIcon::Normal, QIcon::Off);
        break;
    case LUA_TLIGHTUSERDATA:
        [[fallthrough]];
    case LUA_TUSERDATA:
        [[fallthrough]];
    case LUA_TTHREAD:
        ; // No-op
    }

    mpVarsMainArea->checkBox_variable_hidden->setChecked(vu->isHidden(var));
    mpVarsMainArea->lineEdit_var_name->setText(var->getName());
    clearDocument(mpSourceEditorEdbee, lI->getValue(var));
    pItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsTristate | Qt::ItemIsUserCheckable);
    pItem->setToolTip(0, "Checked variables will be saved and loaded with your profile.");
    pItem->setCheckState(0, Qt::Unchecked);
    if (!vu->shouldSave(var)) {
        pItem->setFlags(pItem->flags() & ~(Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsUserCheckable));
        pItem->setForeground(0, QBrush(QColor("grey")));
        pItem->setToolTip(0, "");
    } else if (vu->isSaved(var)) {
        pItem->setCheckState(0, Qt::Checked);
    }
    pItem->setData(0, Qt::UserRole, var->getValueType());
    pItem->setIcon(0, icon);
    mChangingVar = false;
}

void dlgTriggerEditor::slot_action_selected(QTreeWidgetItem* pItem)
{
    if (!pItem) {
        // No details to show - so show the help message:
        mpActionsMainArea->hide();
        mpSourceEditorArea->hide();
        showInfo(msgInfoAddButton);
        return;
    }

    // save the current action before switching to the new one
    if (pItem != mpCurrentActionItem) {
        saveAction();
    }

    mpActionsMainArea->show();
    mpSourceEditorArea->show();

    clearEditorNotification();
    clearDocument(mpSourceEditorEdbee); // Action Select

    mpActionsMainArea->lineEdit_action_icon->clear();
    mpActionsMainArea->lineEdit_action_name->clear();
    mpActionsMainArea->checkBox_action_button_isPushDown->setChecked(false);
    mpActionsMainArea->lineEdit_action_button_command_down->clear();
    mpActionsMainArea->lineEdit_action_button_command_up->clear();
    mpActionsMainArea->spinBox_action_bar_columns->clear();
    mpActionsMainArea->plainTextEdit_action_css->clear();
    mpActionsMainArea->comboBox_action_bar_location->setCurrentIndex(0);
    mpActionsMainArea->comboBox_action_bar_orientation->setCurrentIndex(0);
    mpActionsMainArea->comboBox_action_button_rotation->setCurrentIndex(0);
    mpActionsMainArea->spinBox_action_bar_columns->setValue(1);

    mpCurrentActionItem = pItem; //remember what has been clicked to save it
    // ID will be 0 for the root of the treewidget and it is not appropriate
    // to show any right hand side details - pT will also be Q_NULLPTR!
    int ID = pItem->data(0, Qt::UserRole).toInt();
    TAction* pT = mpHost->getActionUnit()->getAction(ID);
    if (pT) {
        mpActionsMainArea->lineEdit_action_name->setText(pT->getName());
        mpActionsMainArea->checkBox_action_button_isPushDown->setChecked(pT->isPushDownButton());
        mpActionsMainArea->label_action_button_command_up->hide();
        mpActionsMainArea->label_action_button_command_down->hide();
        mpActionsMainArea->lineEdit_action_button_command_up->hide();
        mpActionsMainArea->lineEdit_action_button_command_down->hide();
        mpActionsMainArea->label_action_button_command_down->setText(tr("Command:"));
        mpActionsMainArea->lineEdit_action_icon->setText(pT->getIcon());
        mpActionsMainArea->lineEdit_action_button_command_down->setText(pT->getCommandButtonDown());
        mpActionsMainArea->lineEdit_action_button_command_up->setText(pT->getCommandButtonUp());

        clearDocument(mpSourceEditorEdbee, pT->getScript());

        // location = 1 = location = bottom is no longer supported
        int location = pT->mLocation;
        if (location > 0) {
            location--;
        }
        mpActionsMainArea->comboBox_action_bar_location->setCurrentIndex(location);
        mpActionsMainArea->comboBox_action_bar_orientation->setCurrentIndex(pT->mOrientation);
        mpActionsMainArea->comboBox_action_button_rotation->setCurrentIndex(pT->getButtonRotation());
        mpActionsMainArea->spinBox_action_bar_columns->setValue(pT->getButtonColumns());
        mpActionsMainArea->plainTextEdit_action_css->setPlainText(pT->css);
        if (pT->isFolder()) {
            if (!pT->getPackageName().isEmpty()) {
                // We have a non-empty package name (Tree<T>::mModuleName
                // is NEVER used but Tree<T>::mPackageName is for both!)
                // THUS: We are a module master folder

                mpActionsMainArea->groupBox_action_bar->hide();
                mpActionsMainArea->groupBox_action_button_appearance->hide();
                mpActionsMainArea->widget_top->hide();
                mpSourceEditorArea->hide();
            } else if (!pT->getParent() || (pT->getParent() && !pT->getParent()->getPackageName().isEmpty())) {
                // We are a top-level folder with no parent
                // OR: We have a parent and that IS a module master folder
                // THUS: We are a toolbar

                mpActionsMainArea->groupBox_action_bar->show();
                mpActionsMainArea->groupBox_action_button_appearance->hide();
                mpActionsMainArea->widget_top->show();
                mpSourceEditorArea->show();
            } else {
                // We must be a MENU

                mpActionsMainArea->groupBox_action_button_appearance->setTitle(tr("Menu properties"));
                mpActionsMainArea->groupBox_action_bar->hide();
                mpActionsMainArea->checkBox_action_button_isPushDown->hide();
                mpActionsMainArea->groupBox_action_button_appearance->show();
                mpActionsMainArea->widget_top->show();
                mpSourceEditorArea->show();
            }
        } else {
            // We are a BUTTON

            mpActionsMainArea->groupBox_action_button_appearance->setTitle(tr("Button properties"));
            mpActionsMainArea->groupBox_action_bar->hide();
            mpActionsMainArea->groupBox_action_button_appearance->show();
            mpActionsMainArea->label_action_button_command_down->show();
            mpActionsMainArea->lineEdit_action_button_command_down->show();
            mpActionsMainArea->checkBox_action_button_isPushDown->show();
            mpSourceEditorArea->show();
            if (pT->isPushDownButton()) {
                mpActionsMainArea->label_action_button_command_down->setText(tr("Command (down);"));
                mpActionsMainArea->lineEdit_action_button_command_up->show();
                mpActionsMainArea->label_action_button_command_up->show();
            }

            mpActionsMainArea->widget_top->show();
        }

        if (!pT->state()) {
            showError(pT->getError());
        }
    } else {
        // On root of treewidget_actions: - show help message instead
        mpActionsMainArea->hide();
        mpSourceEditorArea->hide();
        showInfo(msgInfoAddButton);
    }
}

void dlgTriggerEditor::slot_tree_selection_changed()
{
    auto * sender = qobject_cast<TTreeWidget*>(QObject::sender());
    if (sender) {
        QList<QTreeWidgetItem*> items = sender->selectedItems();
        if (items.length()) {
            QTreeWidgetItem* item = items.first();
            if (sender == treeWidget_scripts) {
                slot_scripts_selected(item);
            } else if (sender == treeWidget_keys) {
                slot_key_selected(item);
            } else if (sender == treeWidget_timers) {
                slot_timer_selected(item);
            } else if (sender == treeWidget_aliases) {
                slot_alias_selected(item);
            } else if (sender == treeWidget_actions) {
                slot_action_selected(item);
            } else if (sender == treeWidget_variables) {
                slot_var_selected(item);
            } else if (sender == treeWidget_triggers) {
                slot_trigger_selected(item);
            }
        }
    }
}


void dlgTriggerEditor::slot_scripts_selected(QTreeWidgetItem* pItem)
{
    if (!pItem) {
        // No details to show - so show the help message:
        mpScriptsMainArea->hide();
        mpSourceEditorArea->hide();
        showInfo(msgInfoAddScript);
        return;
    }

    // save the current script before switching to the new one
    if (pItem != mpCurrentScriptItem) {
        saveScript();
    }

    mpCurrentScriptItem = pItem;
    mpScriptsMainArea->show();
    mpSourceEditorArea->show();
    clearEditorNotification();
    clearDocument(mpSourceEditorEdbee); // Script Select
    mpScriptsMainArea->lineEdit_script_name->clear();
    mpScriptsMainArea->listWidget_script_registered_event_handlers->clear();
    mpScriptsMainArea->lineEdit_script_name->setText(pItem->text(0));
    int ID = pItem->data(0, Qt::UserRole).toInt();
    TScript* pT = mpHost->getScriptUnit()->getScript(ID);
    if (pT) {
        QString name = pT->getName();
        QStringList eventHandlerList = pT->getEventHandlerList();
        for (int i = 0; i < eventHandlerList.size(); i++) {
            auto pItem = new QListWidgetItem(mpScriptsMainArea->listWidget_script_registered_event_handlers);
            pItem->setText(eventHandlerList[i]);
            mpScriptsMainArea->listWidget_script_registered_event_handlers->addItem(pItem);
        }
        mpScriptsMainArea->lineEdit_script_name->clear();
        QString script = pT->getScript();
        clearDocument(mpSourceEditorEdbee, script);

        mpScriptsMainArea->lineEdit_script_name->setText(name);
        if (!pT->state()) {
            showError(pT->getError());
        }

    } else {
        // No details to show - as will be the case if the top item (ID = 0) is
        // selected - so show the help message:
        mpScriptsMainArea->hide();
        mpSourceEditorArea->hide();
        showInfo(msgInfoAddScript);
    }
}

void dlgTriggerEditor::slot_timer_selected(QTreeWidgetItem* pItem)
{
    if (!pItem) {
        // No details to show - so show the help message:
        mpTimersMainArea->hide();
        mpSourceEditorArea->hide();
        showInfo(msgInfoAddTimer);
        return;
    }

    // save the current timer before switching to the new one
    if (pItem != mpCurrentTimerItem) {
        saveTimer();
    }

    mpCurrentTimerItem = pItem;
    mpTimersMainArea->show();
    mpSourceEditorArea->show();
    clearEditorNotification();
    clearDocument(mpSourceEditorEdbee); // Timer Select

    mpTimersMainArea->lineEdit_timer_command->clear();
    mpTimersMainArea->timeEdit_timer_hours->setTime(QTime(0, 0, 0, 0));
    mpTimersMainArea->timeEdit_timer_minutes->setTime(QTime(0, 0, 0, 0));
    mpTimersMainArea->timeEdit_timer_seconds->setTime(QTime(0, 0, 0, 0));
    mpTimersMainArea->timeEdit_timer_msecs->setTime(QTime(0, 0, 0, 0));
    mpTimersMainArea->lineEdit_timer_name->setText(pItem->text(0));

    int ID = pItem->data(0, Qt::UserRole).toInt();
    TTimer* pT = mpHost->getTimerUnit()->getTimer(ID);
    if (pT) {
        if (pT->getParent()) {
            qDebug() << "[STATUS]: timer ID=" << pT->getID() << " name=" << pT->getName() << " mActive = " << pT->isActive() << " mUserActiveState=" << pT->shouldBeActive()
                     << " parent=" << pT->getParent()->getName();
        } else {
            qDebug() << "[STATUS]: timer ID=" << pT->getID() << " name=" << pT->getName() << "> mActive = " << pT->isActive() << " mUserActiveState=" << pT->shouldBeActive() << " parent=0";
        }
        QString command = pT->getCommand();
        QString name = pT->getName();
        mpTimersMainArea->lineEdit_timer_command->setText(command);
        mpTimersMainArea->lineEdit_timer_name->setText(name);
        QTime time = pT->getTime();
        mpTimersMainArea->timeEdit_timer_hours->setTime(QTime(time.hour(), 0, 0, 0));
        mpTimersMainArea->timeEdit_timer_minutes->setTime(QTime(0, time.minute(), 0, 0));
        mpTimersMainArea->timeEdit_timer_seconds->setTime(QTime(0, 0, time.second(), 0));
        mpTimersMainArea->timeEdit_timer_msecs->setTime(QTime(0, 0, 0, time.msec()));

        clearDocument(mpSourceEditorEdbee, pT->getScript());

        if (!pT->state()) {
            showError(pT->getError());
        }
    } else {
        // No details to show - as will be the case if the top item (ID = 0) is
        // selected - so show the help message:
        mpTimersMainArea->hide();
        mpSourceEditorArea->hide();
        showInfo(msgInfoAddTimer);
    }
}

void dlgTriggerEditor::fillout_form()
{
    mCurrentView = EditorViewType::cmUnknownView;
    mpCurrentTriggerItem = nullptr;
    mpCurrentAliasItem = nullptr;
    mpCurrentKeyItem = nullptr;
    mpCurrentActionItem = nullptr;
    mpCurrentScriptItem = nullptr;
    mpCurrentTimerItem = nullptr;
    mpCurrentVarItem = nullptr;

    mNeedUpdateData = false;
    mpTriggerBaseItem = new QTreeWidgetItem(static_cast<QTreeWidgetItem*>(nullptr), QStringList(tr("Triggers")));
    mpTriggerBaseItem->setIcon(0, QPixmap(QStringLiteral(":/icons/tools-wizard.png")));
    treeWidget_triggers->insertTopLevelItem(0, mpTriggerBaseItem);
    populateTriggers();
    mpTriggerBaseItem->setExpanded(true);

    mpTimerBaseItem = new QTreeWidgetItem(static_cast<QTreeWidgetItem*>(nullptr), QStringList(tr("Timers")));
    mpTimerBaseItem->setIcon(0, QPixmap(QStringLiteral(":/icons/chronometer.png")));
    treeWidget_timers->insertTopLevelItem(0, mpTimerBaseItem);
    populateTimers();
    mpTimerBaseItem->setExpanded(true);

    mpScriptsBaseItem = new QTreeWidgetItem(static_cast<QTreeWidgetItem*>(nullptr), QStringList(tr("Scripts")));
    mpScriptsBaseItem->setIcon(0, QPixmap(QStringLiteral(":/icons/accessories-text-editor.png")));
    treeWidget_scripts->insertTopLevelItem(0, mpScriptsBaseItem);
    populateScripts();
    mpScriptsBaseItem->setExpanded(true);

    mpAliasBaseItem = new QTreeWidgetItem(static_cast<QTreeWidgetItem*>(nullptr), QStringList(tr("Aliases - Input Triggers")));
    mpAliasBaseItem->setIcon(0, QPixmap(QStringLiteral(":/icons/system-users.png")));
    treeWidget_aliases->insertTopLevelItem(0, mpAliasBaseItem);
    populateAliases();
    mpAliasBaseItem->setExpanded(true);

    mpActionBaseItem = new QTreeWidgetItem(static_cast<QTreeWidgetItem*>(nullptr), QStringList(tr("Buttons")));
    mpActionBaseItem->setIcon(0, QPixmap(QStringLiteral(":/icons/bookmarks.png")));
    treeWidget_actions->insertTopLevelItem(0, mpActionBaseItem);
    populateActions();
    mpActionBaseItem->setExpanded(true);

    mpKeyBaseItem = new QTreeWidgetItem(static_cast<QTreeWidgetItem*>(nullptr), QStringList(tr("Key Bindings")));
    mpKeyBaseItem->setIcon(0, QPixmap(QStringLiteral(":/icons/preferences-desktop-keyboard.png")));
    treeWidget_keys->insertTopLevelItem(0, mpKeyBaseItem);
    populateKeys();
    mpKeyBaseItem->setExpanded(true);
}

void dlgTriggerEditor::populateKeys()
{
    std::list<TKey*> baseNodeList_key = mpHost->getKeyUnit()->getKeyRootNodeList();
    for (auto key : baseNodeList_key) {
        if (key->isTemporary()) {
            continue;
        }

        QString s = key->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem(mpKeyBaseItem, sList);
        pItem->setData(0, Qt::UserRole, QVariant(key->getID()));
        mpKeyBaseItem->addChild(pItem);
        QIcon icon;
        if (key->hasChildren()) {
            expand_child_key(key, pItem);
        }
        if (key->state()) {
            clearEditorNotification();

            if (key->isFolder()) {
                if (!key->mPackageName.isEmpty()) {
                    if (key->isActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                } else if (key->isActive()) {
                    if (key->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-pink.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                } else {
                    if (key->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-pink-locked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            } else {
                if (key->isActive()) {
                    if (key->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                    }
                } else {
                    if (key->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            pItem->setIcon(0, icon);
        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(QStringLiteral(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            pItem->setIcon(0, iconError);
            showError(key->getError());
        }
    }
}
void dlgTriggerEditor::populateActions()
{
    std::list<TAction*> baseNodeList_action = mpHost->getActionUnit()->getActionRootNodeList();
    for (auto action : baseNodeList_action) {
        if (action->isTemporary()) {
            continue;
        }

        QString s = action->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem(mpActionBaseItem, sList);
        pItem->setData(0, Qt::UserRole, QVariant(action->getID()));
        mpActionBaseItem->addChild(pItem);
        QIcon icon;
        if (action->hasChildren()) {
            expand_child_action(action, pItem);
        }
        if (action->state()) {
            clearEditorNotification();

            if (action->isFolder()) {
                if (!action->mPackageName.isEmpty()) {
                    if (action->isActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                } else if (!action->getParent() || !action->getParent()->mPackageName.isEmpty()) {
                    if (action->isActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-yellow.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-yellow-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                } else {
                    if (action->isActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-cyan.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-cyan-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            } else {
                if (action->isActive()) {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                }
            }
            pItem->setIcon(0, icon);
        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(QStringLiteral(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            pItem->setIcon(0, iconError);
            showError(action->getError());
        }
    }
}
void dlgTriggerEditor::populateAliases()
{
    std::list<TAlias*> baseNodeList_alias = mpHost->getAliasUnit()->getAliasRootNodeList();
    for (auto alias : baseNodeList_alias) {
        if (alias->isTemporary()) {
            continue;
        }

        QString s = alias->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem(mpAliasBaseItem, sList);
        pItem->setData(0, Qt::UserRole, QVariant(alias->getID()));
        mpAliasBaseItem->addChild(pItem);
        QIcon icon;
        if (alias->hasChildren()) {
            expand_child_alias(alias, pItem);
        }
        if (alias->state()) {
            clearEditorNotification();

            if (alias->isFolder()) {
                if (!alias->mPackageName.isEmpty()) {
                    if (alias->isActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                } else if (alias->isActive()) {
                    if (alias->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-violet.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                } else {
                    if (alias->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-violet-locked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            } else {
                if (alias->isActive()) {
                    if (alias->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                    }
                } else {
                    if (alias->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            pItem->setIcon(0, icon);
        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(QStringLiteral(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            pItem->setIcon(0, iconError);
            showError(alias->getError());
        }
    }
}
void dlgTriggerEditor::populateScripts()
{
    std::list<TScript*> baseNodeList_scripts = mpHost->getScriptUnit()->getScriptRootNodeList();
    for (auto script : baseNodeList_scripts) {
        QString s = script->getName();

        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem(mpScriptsBaseItem, sList);
        pItem->setData(0, Qt::UserRole, QVariant(script->getID()));
        mpScriptsBaseItem->addChild(pItem);
        QIcon icon;
        if (script->hasChildren()) {
            expand_child_scripts(script, pItem);
        }
        if (script->state()) {
            clearEditorNotification();

            if (script->isFolder()) {
                if (!script->mPackageName.isEmpty()) {
                    if (script->isActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                } else {
                    if (script->isActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-orange.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-orange-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            } else {
                if (script->isActive()) {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                }
            }
            pItem->setIcon(0, icon);
        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(QStringLiteral(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            pItem->setIcon(0, iconError);
            showError(script->getError());
        }
    }
}
void dlgTriggerEditor::populateTimers()
{
    std::list<TTimer *> baseNodeList_timers = mpHost->getTimerUnit()->getTimerRootNodeList();
    for (auto timer : baseNodeList_timers) {
        if (timer->isTemporary()) {
            continue;
        }
        QString s = timer->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem(mpTimerBaseItem, sList);
        pItem->setData(0, Qt::UserRole, QVariant(timer->getID()));
        mpTimerBaseItem->addChild(pItem);
        QIcon icon;
        if (timer->hasChildren()) {
            expand_child_timers(timer, pItem);
        }
        if (timer->state()) {
            clearEditorNotification();

            if (timer->isFolder()) {
                if (!timer->mPackageName.isEmpty()) {
                    if (timer->isActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                } else {
                    if (timer->shouldBeActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-green.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-green-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            } else {
                if (timer->isOffsetTimer()) {
                    if (timer->shouldBeActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/offsettimer-on.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/offsettimer-off.png")), QIcon::Normal, QIcon::Off);
                    }
                } else {
                    if (timer->shouldBeActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            pItem->setIcon(0, icon);
        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(QStringLiteral(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            pItem->setIcon(0, iconError);
            showError(timer->getError());
        }
    }
}
void dlgTriggerEditor::populateTriggers()
{
    std::list<TTrigger *> baseNodeList = mpHost->getTriggerUnit()->getTriggerRootNodeList();
    for (auto trigger : baseNodeList) {
        if (trigger->isTemporary()) {
            continue;
        }
        QString s = trigger->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem(mpTriggerBaseItem, sList);
        pItem->setData(0, Qt::UserRole, QVariant(trigger->getID()));
        mpTriggerBaseItem->addChild(pItem);
        QIcon icon;
        if (trigger->hasChildren()) {
            expand_child_triggers(trigger, pItem);
        }
        if (trigger->state()) {
            clearEditorNotification();

            if (trigger->isFilterChain()) {
                if (trigger->isActive()) {
                    if (trigger->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/filter.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/filter-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                } else {
                    if (trigger->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/filter-locked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/filter-grey-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            } else if (trigger->isFolder()) {
                if (!trigger->mPackageName.isEmpty()) {
                    if (trigger->isActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                } else if (trigger->isActive()) {
                    if (trigger->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-blue.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                } else {
                    if (trigger->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-blue-locked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            } else {
                if (trigger->isActive()) {
                    if (trigger->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                    }
                } else {
                    if (trigger->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            pItem->setIcon(0, icon);
        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(QStringLiteral(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            pItem->setIcon(0, iconError);
            showError(trigger->getError());
        }
    }
}

void dlgTriggerEditor::repopulateVars()
{
    treeWidget_variables->setUpdatesEnabled(false);
    mpVarBaseItem = new QTreeWidgetItem(QStringList(tr("Variables")));
    mpVarBaseItem->setTextAlignment(0, Qt::AlignLeft | Qt::AlignVCenter);
    mpVarBaseItem->setIcon(0, QPixmap(QStringLiteral(":/icons/variables.png")));
    treeWidget_variables->clear();
    mpCurrentVarItem = nullptr;
    treeWidget_variables->insertTopLevelItem(0, mpVarBaseItem);
    mpVarBaseItem->setExpanded(true);
    LuaInterface* lI = mpHost->getLuaInterface();
    lI->getVars(false);
    VarUnit* vu = lI->getVarUnit();
    vu->buildVarTree(mpVarBaseItem, vu->getBase(), showHiddenVars);
    mpVarBaseItem->setExpanded(true);
    treeWidget_variables->setUpdatesEnabled(true);
}

void dlgTriggerEditor::expand_child_triggers(TTrigger* pTriggerParent, QTreeWidgetItem* pWidgetItemParent)
{
    std::list<TTrigger*>* childrenList = pTriggerParent->getChildrenList();
    for (auto trigger : *childrenList) {
        QString s = trigger->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem(pWidgetItemParent, sList);
        pItem->setData(0, Qt::UserRole, trigger->getID());

        pWidgetItemParent->insertChild(0, pItem);
        QIcon icon;
        if (trigger->hasChildren()) {
            expand_child_triggers(trigger, pItem);
        }
        if (trigger->state()) {
            clearEditorNotification();

            if (trigger->isFilterChain()) {
                if (trigger->isActive()) {
                    if (trigger->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/filter.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/filter-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                } else {
                    if (trigger->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/filter-locked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/filter-grey-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            } else if (trigger->isFolder()) {
                if (trigger->isActive()) {
                    if (trigger->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-blue.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                } else {
                    if (trigger->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-blue-locked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            } else {
                if (trigger->isActive()) {
                    if (trigger->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                    }

                } else {
                    if (trigger->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            //pItem->setDisabled(!trigger->ancestorsActive());
            pItem->setIcon(0, icon);
        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(QStringLiteral(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            pItem->setIcon(0, iconError);
            showError(trigger->getError());
        }
    }
}

void dlgTriggerEditor::expand_child_key(TKey* pTriggerParent, QTreeWidgetItem* pWidgetItemParent)
{
    std::list<TKey*>* childrenList = pTriggerParent->getChildrenList();
    for (auto key : *childrenList) {
        QString s = key->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem(pWidgetItemParent, sList);
        pItem->setData(0, Qt::UserRole, key->getID());

        pWidgetItemParent->insertChild(0, pItem);
        QIcon icon;
        if (key->hasChildren()) {
            expand_child_key(key, pItem);
        }
        if (key->state()) {
            clearEditorNotification();

            if (key->isFolder()) {
                if (key->isActive()) {
                    if (key->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-pink.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                } else {
                    if (key->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-pink-locked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            } else {
                if (key->isActive()) {
                    if (key->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                    }
                } else {
                    if (key->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            pItem->setIcon(0, icon);
        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(QStringLiteral(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            pItem->setIcon(0, iconError);
            showError(key->getError());
        }
    }
}


void dlgTriggerEditor::expand_child_scripts(TScript* pTriggerParent, QTreeWidgetItem* pWidgetItemParent)
{
    std::list<TScript*>* childrenList = pTriggerParent->getChildrenList();
    for (auto script : *childrenList) {
        QString s = script->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem(pWidgetItemParent, sList);
        pItem->setData(0, Qt::UserRole, script->getID());

        pWidgetItemParent->insertChild(0, pItem);
        QIcon icon;
        if (script->hasChildren()) {
            expand_child_scripts(script, pItem);
        }
        if (script->state()) {
            clearEditorNotification();

            if (script->isFolder()) {
                if (script->isActive()) {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-orange.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-orange-locked.png")), QIcon::Normal, QIcon::Off);
                }
            } else {
                if (script->isActive()) {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                }
            }
            pItem->setIcon(0, icon);
        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(QStringLiteral(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            pItem->setIcon(0, iconError);
            showError(script->getError());
        }
    }
}

void dlgTriggerEditor::expand_child_alias(TAlias* pTriggerParent, QTreeWidgetItem* pWidgetItemParent)
{
    std::list<TAlias*>* childrenList = pTriggerParent->getChildrenList();
    for (auto alias : *childrenList) {
        QString s = alias->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem(pWidgetItemParent, sList);
        pItem->setData(0, Qt::UserRole, alias->getID());

        pWidgetItemParent->insertChild(0, pItem);
        QIcon icon;
        if (alias->hasChildren()) {
            expand_child_alias(alias, pItem);
        }
        if (alias->state()) {
            clearEditorNotification();

            if (alias->isFolder()) {
                if (alias->isActive()) {
                    if (alias->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-violet.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                } else {
                    if (alias->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-violet-locked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            } else {
                if (alias->isActive()) {
                    if (alias->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                    }
                } else {
                    if (alias->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            pItem->setIcon(0, icon);
        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(QStringLiteral(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            pItem->setIcon(0, iconError);
            showError(alias->getError());
        }
    }
}

void dlgTriggerEditor::expand_child_action(TAction* pTriggerParent, QTreeWidgetItem* pWidgetItemParent)
{
    std::list<TAction*>* childrenList = pTriggerParent->getChildrenList();
    for (auto action : *childrenList) {
        QString s = action->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem(pWidgetItemParent, sList);
        pItem->setData(0, Qt::UserRole, action->getID());

        pWidgetItemParent->insertChild(0, pItem);
        QIcon icon;
        if (action->hasChildren()) {
            expand_child_action(action, pItem);
        }
        if (action->state()) {
            clearEditorNotification();

            if (!action->getParent()->mPackageName.isEmpty()) {
                // Must have a parent (or would not be IN this method) and the
                // parent has a package name - this is a toolbar
                if (action->isActive()) {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-yellow.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-yellow-locked.png")), QIcon::Normal, QIcon::Off);
                }
            } else if (action->isFolder()) {
                // Is a folder and is not a toolbar - this is a menu
                if (action->isActive()) {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-cyan.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-cyan-locked.png")), QIcon::Normal, QIcon::Off);
                }
            } else {
                // Is a button
                if (action->isActive()) {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                }
            }
            pItem->setIcon(0, icon);
        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(QStringLiteral(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            pItem->setIcon(0, iconError);
            showError(action->getError());
        }
    }
}


void dlgTriggerEditor::expand_child_timers(TTimer* pTimerParent, QTreeWidgetItem* pWidgetItemParent)
{
    std::list<TTimer*>* childrenList = pTimerParent->getChildrenList();
    for (auto timer : *childrenList) {
        QString s = timer->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem(pWidgetItemParent, sList);
        pItem->setData(0, Qt::UserRole, timer->getID());

        pWidgetItemParent->insertChild(0, pItem);
        QIcon icon;
        if (timer->hasChildren()) {
            expand_child_timers(timer, pItem);
        }
        if (timer->state()) {
            clearEditorNotification();

            if (timer->isFolder()) {
                if (timer->shouldBeActive()) {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-green.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-green-locked.png")), QIcon::Normal, QIcon::Off);
                }
            } else {
                if (timer->isOffsetTimer()) {
                    if (timer->shouldBeActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/offsettimer-on.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/offsettimer-off.png")), QIcon::Normal, QIcon::Off);
                    }
                } else {
                    if (timer->shouldBeActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            pItem->setIcon(0, icon);
        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(QStringLiteral(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            pItem->setIcon(0, iconError);
            showError(timer->getError());
        }
    }
}

void dlgTriggerEditor::slot_showSearchAreaResults(const bool isChecked)
{
    if (isChecked) {
        if (!button_toggleSearchAreaResults->isChecked()) {
            // If this slot is called "manually" the checked state of the button
            // may not match the setting, so make it do so, note that the
            // setChecked(bool) method does NOT invoke the clicked(bool) signal
            // that is connected to here in the constructor, but it does the
            // toggled(bool) one, which is why we use the former...
            button_toggleSearchAreaResults->setChecked(true);
        }
        treeWidget_searchResults->show();
    } else {
        if (button_toggleSearchAreaResults->isChecked()) {
            button_toggleSearchAreaResults->setChecked(false);
        }
        treeWidget_searchResults->hide();
    }
}

void dlgTriggerEditor::saveOpenChanges()
{
    if (mCurrentView == EditorViewType::cmUnknownView) {
        return;
    }

    switch (mCurrentView) {
    case EditorViewType::cmTriggerView:
        saveTrigger();
        break;
    case EditorViewType::cmTimerView:
        saveTimer();
        break;
    case EditorViewType::cmAliasView:
        saveAlias();
        break;
    case EditorViewType::cmScriptView:
        saveScript();
        break;
    case EditorViewType::cmActionView:
        saveAction();
        break;
    case EditorViewType::cmKeysView:
        saveKey();
        break;
    case EditorViewType::cmVarsView:
        saveVar();
        break;
    }
}

void dlgTriggerEditor::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event);

    if (isActiveWindow()) {
        autoSave();
    }
}

void dlgTriggerEditor::autoSave()
{
    mpHost->saveProfile(QString(), QStringLiteral("autosave"));
}

void dlgTriggerEditor::enterEvent(QEvent* pE)
{
    if (mNeedUpdateData) {
        saveOpenChanges();
        treeWidget_triggers->clear();
        treeWidget_aliases->clear();
        treeWidget_timers->clear();
        treeWidget_scripts->clear();
        treeWidget_actions->clear();
        treeWidget_keys->clear();
        treeWidget_variables->clear();
        fillout_form();
        mNeedUpdateData = false;
    }
}

void dlgTriggerEditor::focusInEvent(QFocusEvent* pE)
{
    if (mNeedUpdateData) {
        saveOpenChanges();
        treeWidget_triggers->clear();
        treeWidget_aliases->clear();
        treeWidget_timers->clear();
        treeWidget_scripts->clear();
        treeWidget_actions->clear();
        treeWidget_keys->clear();
        treeWidget_variables->clear();
        fillout_form();
        mNeedUpdateData = false;
    }

    if (mCurrentView == EditorViewType::cmUnknownView) {
        mpCurrentTriggerItem = nullptr;
        mpCurrentAliasItem = nullptr;
        mpCurrentKeyItem = nullptr;
        mpCurrentActionItem = nullptr;
        mpCurrentScriptItem = nullptr;
        mpCurrentTimerItem = nullptr;
        return;
    }

    if (mpCurrentTriggerItem) {
        mpCurrentTriggerItem->setSelected(true);
    }
    if (mpCurrentTimerItem) {
        mpCurrentTimerItem->setSelected(true);
    }
    if (mpCurrentAliasItem) {
        mpCurrentAliasItem->setSelected(true);
    }
    if (mpCurrentScriptItem) {
        mpCurrentScriptItem->setSelected(true);
    }
    if (mpCurrentActionItem) {
        mpCurrentActionItem->setSelected(true);
    }
    if (mpCurrentKeyItem) {
        mpCurrentKeyItem->setSelected(true);
    }
}

void dlgTriggerEditor::focusOutEvent(QFocusEvent* pE)
{
    Q_UNUSED(pE);

    saveOpenChanges();
}

void dlgTriggerEditor::changeView(EditorViewType view)
{
    saveOpenChanges();

    if (mNeedUpdateData) {
        treeWidget_triggers->clear();
        treeWidget_aliases->clear();
        treeWidget_timers->clear();
        treeWidget_scripts->clear();
        treeWidget_actions->clear();
        treeWidget_keys->clear();
        treeWidget_variables->clear();
        fillout_form();
        mNeedUpdateData = false;
    }

    // Edbee doesn't have a readonly option, so I'm using setEnabled
     mpSourceEditorEdbee->setEnabled(true);

    if (mCurrentView != view) {
        clearDocument(mpSourceEditorEdbee); // Change View
    }
    mCurrentView = view;

    mpActionsMainArea->setVisible(view == EditorViewType::cmActionView);
    treeWidget_actions->setVisible(view == EditorViewType::cmActionView);

    mpAliasMainArea->setVisible(view == EditorViewType::cmAliasView);
    treeWidget_aliases->setVisible(view == EditorViewType::cmAliasView);

    mpKeysMainArea->setVisible(view == EditorViewType::cmKeysView);
    treeWidget_keys->setVisible(view == EditorViewType::cmKeysView);

    mpScriptsMainArea->setVisible(view == EditorViewType::cmScriptView);
    treeWidget_scripts->setVisible(view == EditorViewType::cmScriptView);

    mpTimersMainArea->setVisible(view == EditorViewType::cmTimerView);
    treeWidget_timers->setVisible(view == EditorViewType::cmTimerView);

    mpTriggersMainArea->setVisible(view == EditorViewType::cmTriggerView);
    treeWidget_triggers->setVisible(view == EditorViewType::cmTriggerView);

    mpVarsMainArea->setVisible(view == EditorViewType::cmVarsView);
    treeWidget_variables->setVisible(view == EditorViewType::cmVarsView);
    checkBox_displayAllVariables->setVisible(view == EditorViewType::cmVarsView);
}

void dlgTriggerEditor::slot_show_timers()
{
    changeView(EditorViewType::cmTimerView);
    QTreeWidgetItem* pI = treeWidget_timers->topLevelItem(0);
    if (!pI || pI == treeWidget_timers->currentItem() || !pI->childCount()) {
        // There is no root item, we are on the root item or there are no other
        // items - so show the help message:
        mpTimersMainArea->hide();
        mpSourceEditorArea->hide();
        showInfo(msgInfoAddTimer);
    } else {
        mpTimersMainArea->show();
        mpSourceEditorArea->show();
        slot_timer_selected(treeWidget_timers->currentItem());
    }
    if (!mTimerEditorSplitterState.isEmpty()) {
        splitter_right->restoreState(mTimerEditorSplitterState);
    } else {
        const QList<int> sizes = {30, 900, 30};
        splitter_right->setSizes(sizes);
        mTimerEditorSplitterState = splitter_right->saveState();
    }
}

void dlgTriggerEditor::slot_show_current()
{
    if (mCurrentView != EditorViewType::cmUnknownView) {
        return;
    }

    changeView(EditorViewType::cmTriggerView);
    QTreeWidgetItem* pI = treeWidget_triggers->topLevelItem(0);
    if (!pI || pI == treeWidget_triggers->currentItem() || !pI->childCount()) {
        // There is no root item, we are on the root item or there are no other
        // items - so show the help message:
        mpTriggersMainArea->hide();
        mpSourceEditorArea->hide();
        showInfo(msgInfoAddTrigger);
    } else {
        mpTriggersMainArea->show();
        mpSourceEditorArea->show();
        slot_trigger_selected(treeWidget_triggers->currentItem());
    }
}

void dlgTriggerEditor::slot_show_triggers()
{
    changeView(EditorViewType::cmTriggerView);
    QTreeWidgetItem* pI = treeWidget_triggers->topLevelItem(0);
    if (!pI || pI == treeWidget_triggers->currentItem() || !pI->childCount()) {
        // There is no root item, we are on the root item or there are no other
        // items - so show the help message:
        mpTriggersMainArea->hide();
        mpSourceEditorArea->hide();
        showInfo(msgInfoAddTrigger);
    } else {
        mpTriggersMainArea->show();
        mpSourceEditorArea->show();
        slot_trigger_selected(treeWidget_triggers->currentItem());
    }
    if (!mTriggerEditorSplitterState.isEmpty()) {
        splitter_right->restoreState(mTriggerEditorSplitterState);
    } else {
        const QList<int> sizes = {30, 900, 30};
        splitter_right->setSizes(sizes);
        mTriggerEditorSplitterState = splitter_right->saveState();
    }
}

void dlgTriggerEditor::slot_show_scripts()
{
    changeView(EditorViewType::cmScriptView);
    QTreeWidgetItem* pI = treeWidget_scripts->topLevelItem(0);
    if (!pI || pI == treeWidget_scripts->currentItem() || !pI->childCount()) {
        // There is no root item, we are on the root item or there are no other
        // items - so show the help message:
        mpScriptsMainArea->hide();
        mpSourceEditorArea->hide();
        showInfo(msgInfoAddScript);
    } else {
        mpScriptsMainArea->show();
        mpSourceEditorArea->show();
        slot_scripts_selected(treeWidget_scripts->currentItem());
    }
    if (!mScriptEditorSplitterState.isEmpty()) {
        splitter_right->restoreState(mScriptEditorSplitterState);
    } else {
        const QList<int> sizes = {30, 900, 30};
        splitter_right->setSizes(sizes);
        mScriptEditorSplitterState = splitter_right->saveState();
    }
}

void dlgTriggerEditor::slot_show_keys()
{
    changeView(EditorViewType::cmKeysView);
    QTreeWidgetItem* pI = treeWidget_keys->topLevelItem(0);
    if (!pI || pI == treeWidget_keys->currentItem() || !pI->childCount()) {
        // There is no root item, we are on the root item or there are no other
        // items - so show the help message:
        mpKeysMainArea->hide();
        mpSourceEditorArea->hide();
        showInfo(msgInfoAddKey);
    } else {
        mpKeysMainArea->show();
        mpSourceEditorArea->show();
        slot_key_selected(treeWidget_keys->currentItem());
    }
    if (!mKeyEditorSplitterState.isEmpty()) {
        splitter_right->restoreState(mKeyEditorSplitterState);
    } else {
        const QList<int> sizes = {30, 900, 30};
        splitter_right->setSizes(sizes);
        mKeyEditorSplitterState = splitter_right->saveState();
    }
}

void dlgTriggerEditor::slot_show_vars()
{
    changeView(EditorViewType::cmVarsView);
    repopulateVars();
    mpCurrentVarItem = nullptr;
    checkBox_displayAllVariables->show();
    checkBox_displayAllVariables->setChecked(showHiddenVars);
    QTreeWidgetItem* pI = treeWidget_variables->topLevelItem(0);
    if (!pI || pI == treeWidget_variables->currentItem() || !pI->childCount()) {
        // There is no root item, we are on the root item or there are no other
        // items - so show the help message:
        mpVarsMainArea->hide();
        mpSourceEditorArea->hide();
        showInfo(msgInfoAddVar);
    } else {
        mpVarsMainArea->show();
        mpSourceEditorArea->show();
        slot_var_selected(treeWidget_variables->currentItem());
    }
    if (!mVarEditorSplitterState.isEmpty()) {
        splitter_right->restoreState(mVarEditorSplitterState);
    } else {
        const QList<int> sizes = {30, 900, 30};
        splitter_right->setSizes(sizes);
        mVarEditorSplitterState = splitter_right->saveState();
    }
}

void dlgTriggerEditor::show_vars()
{
    //no repopulation of variables
    changeView(EditorViewType::cmVarsView);
    mpCurrentVarItem = nullptr;
    mpSourceEditorArea->show();
    checkBox_displayAllVariables->show();
    checkBox_displayAllVariables->setChecked(showHiddenVars);
    QTreeWidgetItem* pI = treeWidget_variables->topLevelItem(0);
    if (pI) {
        if (pI->childCount() > 0) {
            mpVarsMainArea->show();
            slot_var_selected(treeWidget_variables->currentItem());
        } else {
            mpVarsMainArea->hide();
            showInfo(msgInfoAddVar);
        }
    }
    treeWidget_variables->show();
}


void dlgTriggerEditor::slot_show_aliases()
{
    changeView(EditorViewType::cmAliasView);
    QTreeWidgetItem* pI = treeWidget_aliases->topLevelItem(0);
    if (!pI || pI == treeWidget_aliases->currentItem() || !pI->childCount()) {
        // There is no root item, we are on the root item or there are no other
        // items - so show the help message:
        mpAliasMainArea->hide();
        mpSourceEditorArea->hide();
        showInfo(msgInfoAddAlias);
    } else {
        mpAliasMainArea->show();
        mpSourceEditorArea->show();
        slot_alias_selected(treeWidget_aliases->currentItem());
    }
    if (!mAliasEditorSplitterState.isEmpty()) {
        splitter_right->restoreState(mAliasEditorSplitterState);
    } else {
        const QList<int> sizes = {30, 900, 30};
        splitter_right->setSizes(sizes);
        mAliasEditorSplitterState = splitter_right->saveState();
    }
}

void dlgTriggerEditor::showError(const QString& error)
{
    mpSystemMessageArea->notificationAreaIconLabelInformation->hide();
    mpSystemMessageArea->notificationAreaIconLabelError->show();
    mpSystemMessageArea->notificationAreaIconLabelWarning->hide();
    mpSystemMessageArea->notificationAreaMessageBox->setText(error);
    mpSystemMessageArea->show();
}

void dlgTriggerEditor::showInfo(const QString& error)
{
    mpSystemMessageArea->notificationAreaIconLabelError->hide();
    mpSystemMessageArea->notificationAreaIconLabelWarning->hide();
    mpSystemMessageArea->notificationAreaIconLabelInformation->show();
    mpSystemMessageArea->notificationAreaMessageBox->setText(error);
    mpSystemMessageArea->show();
}

void dlgTriggerEditor::showWarning(const QString& error)
{
    mpSystemMessageArea->notificationAreaIconLabelInformation->hide();
    mpSystemMessageArea->notificationAreaIconLabelError->hide();
    mpSystemMessageArea->notificationAreaIconLabelWarning->show();
    mpSystemMessageArea->notificationAreaMessageBox->setText(error);
    mpSystemMessageArea->show();
}

void dlgTriggerEditor::slot_show_actions()
{
    changeView(EditorViewType::cmActionView);
    QTreeWidgetItem* pI = treeWidget_actions->topLevelItem(0);
    if (!pI || pI == treeWidget_actions->currentItem() || !pI->childCount()) {
        // There is no root item, we are on the root item or there are no other
        // items - so show the help message:
        mpActionsMainArea->hide();
        mpSourceEditorArea->hide();
        showInfo(msgInfoAddButton);
    } else {
        mpActionsMainArea->show();
        mpSourceEditorArea->show();
        slot_action_selected(treeWidget_actions->currentItem());
    }
    if (!mActionEditorSplitterState.isEmpty()) {
        splitter_right->restoreState(mActionEditorSplitterState);
    } else {
        const QList<int> sizes = {30, 900, 30};
        splitter_right->setSizes(sizes);
        mActionEditorSplitterState = splitter_right->saveState();
    }
}

void dlgTriggerEditor::slot_save_edit()
{
    switch (mCurrentView) {
    case EditorViewType::cmTriggerView:
        saveTrigger();
        break;
    case EditorViewType::cmTimerView:
        saveTimer();
        break;
    case EditorViewType::cmAliasView:
        saveAlias();
        break;
    case EditorViewType::cmScriptView:
        saveScript();
        break;
    case EditorViewType::cmActionView:
        saveAction();
        break;
    case EditorViewType::cmKeysView:
        saveKey();
        break;
    case EditorViewType::cmVarsView:
        saveVar();
        break;
    default:
        qWarning() << "ERROR: dlgTriggerEditor::slot_save_edit() undefined view";
    }

    // There was a mpHost->serialize() call here, but that code was
    // "short-circuited" and returned without doing anything;
}

void dlgTriggerEditor::slot_add_new()
{
    switch (mCurrentView) {
    case EditorViewType::cmTriggerView:
        addTrigger(false); //add normal trigger
        break;
    case EditorViewType::cmTimerView:
        addTimer(false); //add normal trigger
        break;
    case EditorViewType::cmAliasView:
        addAlias(false); //add normal alias
        break;
    case EditorViewType::cmScriptView:
        addScript(false); //add normal alias
        break;
    case EditorViewType::cmActionView:
        addAction(false); //add normal action
        break;
    case EditorViewType::cmKeysView:
        addKey(false); //add normal alias
        break;
    case EditorViewType::cmVarsView:
        if (mpCurrentVarItem) {
            addVar(false); //add normal action
        }
        break;
    default:
        qDebug() << "ERROR: dlgTriggerEditor::slot_save_edit() undefined view";
    }
}

void dlgTriggerEditor::slot_add_new_folder()
{
    switch (mCurrentView) {
    case EditorViewType::cmTriggerView:
        addTrigger(true); //add trigger group
        break;
    case EditorViewType::cmTimerView:
        addTimer(true);
        break;
    case EditorViewType::cmAliasView:
        addAlias(true); //add alias group
        break;
    case EditorViewType::cmScriptView:
        addScript(true); //add alias group
        break;
    case EditorViewType::cmActionView:
        addAction(true); //add action group
        break;
    case EditorViewType::cmKeysView:
        addKey(true); //add alias group
        break;
    case EditorViewType::cmVarsView:
        if (mpCurrentVarItem) {
            addVar(true);
        }
        break;
    default:
        qDebug() << "ERROR: dlgTriggerEditor::slot_save_edit() undefined view";
    }
}

void dlgTriggerEditor::slot_toggle_active()
{
    switch (mCurrentView) {
    case EditorViewType::cmTriggerView:
        activeToggle_trigger();
        break;
    case EditorViewType::cmTimerView:
        activeToggle_timer();
        break;
    case EditorViewType::cmAliasView:
        activeToggle_alias();
        break;
    case EditorViewType::cmScriptView:
        activeToggle_script();
        break;
    case EditorViewType::cmActionView:
        activeToggle_action();
        break;
    case EditorViewType::cmKeysView:
        activeToggle_key();
        break;

    default:
        qDebug() << "ERROR: dlgTriggerEditor::slot_save_edit() undefined view";
    }
}

void dlgTriggerEditor::slot_move_source_find()
{
    int x = mpSourceEditorEdbee->width() - mpSourceEditorFindArea->width();
    int y = mpSourceEditorEdbee->height() - mpSourceEditorFindArea->height();
    if (mpSourceEditorEdbee->verticalScrollBar()->isVisible()) {
        x = x - mpSourceEditorEdbee->verticalScrollBar()->width();
    }
    if (mpSourceEditorEdbee->horizontalScrollBar()->isVisible()) {
        y = y - mpSourceEditorEdbee->horizontalScrollBar()->height();
    }
    mpSourceEditorFindArea->move(x, y);
    mpSourceEditorFindArea->update();
}

void dlgTriggerEditor::slot_open_source_find()
{
    slot_move_source_find();
    mpSourceEditorFindArea->show();
    mpSourceEditorFindArea->lineEdit_findText->setFocus();
    mpSourceEditorFindArea->lineEdit_findText->selectAll();
}

void dlgTriggerEditor::slot_close_source_find()
{
    auto controller = mpSourceEditorEdbee->controller();
    controller->borderedTextRanges()->clear();
    controller->textSelection()->range(0).clearSelection();
    controller->update();
    mpSourceEditorFindArea->hide();
    mpSourceEditorEdbee->setFocus();
}

void dlgTriggerEditor::slot_source_find_previous()
{
    auto controller = mpSourceEditorEdbee->controller();
    auto searcher = controller->textSearcher();
    searcher->setSearchTerm(mpSourceEditorFindArea->lineEdit_findText->text());
    searcher->setCaseSensitive(false);
    searcher->findPrev(mpSourceEditorEdbee);
    controller->scrollCaretVisible();
    controller->update();
    slot_move_source_find();
}

void dlgTriggerEditor::slot_source_find_next()
{
    auto controller = mpSourceEditorEdbee->controller();
    auto searcher = controller->textSearcher();
    searcher->setSearchTerm(mpSourceEditorFindArea->lineEdit_findText->text());
    searcher->setCaseSensitive(false);
    searcher->findNext(mpSourceEditorEdbee);
    controller->scrollCaretVisible();
    controller->update();
    slot_move_source_find();
}

void dlgTriggerEditor::slot_source_find_text_changed()
{
    auto controller = mpSourceEditorEdbee->controller();
    auto searcher = controller->textSearcher();
    controller->borderedTextRanges()->clear();
    controller->textSelection()->range(0).clearSelection();
    searcher->setSearchTerm(mpSourceEditorFindArea->lineEdit_findText->text());
    searcher->markAll(controller->borderedTextRanges());
    controller->update();
}

void dlgTriggerEditor::slot_delete_item()
{
    switch (mCurrentView) {
    case EditorViewType::cmTriggerView:
        delete_trigger();
        break;
    case EditorViewType::cmTimerView:
        delete_timer();
        break;
    case EditorViewType::cmAliasView:
        delete_alias();
        break;
    case EditorViewType::cmScriptView:
        delete_script();
        break;
    case EditorViewType::cmActionView:
        delete_action();
        break;
    case EditorViewType::cmKeysView:
        delete_key();
        break;
    case EditorViewType::cmVarsView:
        delete_variable();
        break;
    default:
        qDebug() << "ERROR: dlgTriggerEditor::slot_save_edit() undefined view";
    }
}

void dlgTriggerEditor::slot_item_selected_save(QTreeWidgetItem* pItem)
{
    if (!pItem) {
        return;
    }

    switch (mCurrentView) {
    case EditorViewType::cmTriggerView:
        saveTrigger();
        break;
    case EditorViewType::cmTimerView:
        saveTimer();
        break;
    case EditorViewType::cmAliasView:
        saveAlias();
        break;
    case EditorViewType::cmScriptView:
        saveScript();
        break;
    case EditorViewType::cmActionView:
        saveAction();
        break;
    case EditorViewType::cmKeysView:
        saveKey();
        break;
    case EditorViewType::cmVarsView:
        saveVar();
        break;
    }
}

// Should the functionality change in this method be sure to review the code
// for "case SearchResultIsEventHandler" for "Scripts" in:
// slot_item_selected_search_list(...)
void dlgTriggerEditor::slot_script_main_area_edit_handler(QListWidgetItem*)
{
    QListWidgetItem* pItem = mpScriptsMainArea->listWidget_script_registered_event_handlers->currentItem();
    if (!pItem) {
        return;
    }
    mIsScriptsMainAreaEditHandler = true;
    mpScriptsMainAreaEditHandlerItem = pItem;
    QString regex = pItem->text();
    if (regex.size() < 1) {
        mIsScriptsMainAreaEditHandler = false;
        return;
    }
    mpScriptsMainArea->lineEdit_script_event_handler_entry->setText(regex);
}

void dlgTriggerEditor::slot_script_main_area_delete_handler()
{
    mpScriptsMainArea->listWidget_script_registered_event_handlers->takeItem(mpScriptsMainArea->listWidget_script_registered_event_handlers->currentRow());
}

void dlgTriggerEditor::slot_script_main_area_add_handler()
{
    auto addEventHandler = [&] () {
        auto pItem = new QListWidgetItem;
        pItem->setText(mpScriptsMainArea->lineEdit_script_event_handler_entry->text());
        mpScriptsMainArea->listWidget_script_registered_event_handlers->addItem(pItem);
    };

    mpScriptsMainArea->trimEventHandlerName();
    if (mIsScriptsMainAreaEditHandler) {
        if (!mpScriptsMainAreaEditHandlerItem) {
            mIsScriptsMainAreaEditHandler = false;
            addEventHandler();
        } else {
            QListWidgetItem* pItem = mpScriptsMainArea->listWidget_script_registered_event_handlers->currentItem();
            if (!pItem) {
                addEventHandler();
            }
        }
        mIsScriptsMainAreaEditHandler = false;
        mpScriptsMainAreaEditHandlerItem = nullptr;
    } else {
        addEventHandler();
    }
    mpScriptsMainArea->lineEdit_script_event_handler_entry->clear();
}

void dlgTriggerEditor::slot_debug_mode()
{
    mudlet::self()->attachDebugArea(mpHost->getName());

    mudlet::mpDebugArea->setVisible(!mudlet::debugMode);
    mudlet::debugMode = !mudlet::debugMode;
    mudlet::mpDebugArea->setWindowTitle("Central Debug Console");
}

void dlgTriggerEditor::slot_next_section()
{
    switch (mCurrentView) {
    case EditorViewType::cmTriggerView:
        if (QStringLiteral("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            treeWidget_triggers->setFocus();
            return;
        } else if (treeWidget_triggers->hasFocus()) {
            mpTriggersMainArea->lineEdit_trigger_name->setFocus();
            return;
        } else if (mpTriggersMainArea->hasFocus()) {
            mTriggerPatternEdit[0]->lineEdit_pattern->setFocus();
            return;
        } else {
            for (auto child : mpTriggersMainArea->scrollArea->findChildren<QWidget*>()) {
                if (child->hasFocus()) {
                    mpSourceEditorEdbee->setFocus();
                    return;
                }
            }
            for (auto child : mpTriggersMainArea->findChildren<QWidget*>()) {
                if (child->hasFocus()) {
                    mTriggerPatternEdit[0]->lineEdit_pattern->setFocus();
                    return;
                }
            }
        }
        break;
    case EditorViewType::cmTimerView:
        if (QStringLiteral("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            treeWidget_timers->setFocus();
            return;
        } else if (treeWidget_timers->hasFocus()) {
            mpTimersMainArea->lineEdit_timer_name->setFocus();
            return;
        } else {
            for (auto child : mpTimersMainArea->findChildren<QWidget*>()) {
                if (child->hasFocus()) {
                    mpSourceEditorEdbee->setFocus();
                    return;
                }
            }
        }
        break;
    case EditorViewType::cmAliasView:
        if (QString("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            treeWidget_aliases->setFocus();
            return;
        } else if (treeWidget_aliases->hasFocus()) {
            mpAliasMainArea->lineEdit_alias_name->setFocus();
            return;
        } else {
            for (auto child : mpAliasMainArea->findChildren<QWidget*>()) {
                if (child->hasFocus()) {
                    mpSourceEditorEdbee->setFocus();
                    return;
                }
            }
        }
        break;
    case EditorViewType::cmScriptView:
        if (QStringLiteral("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            treeWidget_scripts->setFocus();
            return;
        } else if (treeWidget_scripts->hasFocus()) {
            mpScriptsMainArea->lineEdit_script_name->setFocus();
            return;
        } else {
            for (auto child : mpScriptsMainArea->findChildren<QWidget*>()) {
                if (child->hasFocus()) {
                    mpSourceEditorEdbee->setFocus();
                    return;
                }
            }
        }
        break;
    case EditorViewType::cmActionView:
        if (QStringLiteral("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            treeWidget_actions->setFocus();
            return;
        } else if (treeWidget_actions->hasFocus()) {
            mpActionsMainArea->lineEdit_action_name->setFocus();
            return;
        } else {
            for (auto child : mpActionsMainArea->findChildren<QWidget*>()) {
                if (child->hasFocus()) {
                    mpSourceEditorEdbee->setFocus();
                    return;
                }
            }
        }
        break;
    case EditorViewType::cmKeysView:
        if (QStringLiteral("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            treeWidget_keys->setFocus();
            return;
        } else if (treeWidget_keys->hasFocus()) {
            mpKeysMainArea->lineEdit_key_name->setFocus();
            return;
        } else {
            for (auto child : mpKeysMainArea->findChildren<QWidget*>()) {
                if (child->hasFocus()) {
                    mpSourceEditorEdbee->setFocus();
                    return;
                }
            }
        }
        break;
    case EditorViewType::cmVarsView:
        if (QStringLiteral("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            treeWidget_variables->setFocus();
            return;
        } else if (treeWidget_variables->hasFocus()) {
            mpVarsMainArea->lineEdit_var_name->setFocus();
            return;
        } else {
            for (auto child : mpVarsMainArea->findChildren<QWidget*>()) {
                if (child->hasFocus()) {
                    mpSourceEditorEdbee->setFocus();
                    return;
                }
            }
        }
        break;
    case EditorViewType::cmUnknownView:
        return;
    }
}

void dlgTriggerEditor::slot_previous_section()
{
    switch (mCurrentView) {
    case EditorViewType::cmTriggerView:
        if (QString("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            mTriggerPatternEdit[0]->lineEdit_pattern->setFocus();
            return;
        } else if (treeWidget_triggers->hasFocus()) {
            mpSourceEditorEdbee->setFocus();
            return;
        } else {
            for (auto child : mpTriggersMainArea->scrollArea->findChildren<QWidget *>()) {
                if (child->hasFocus()){
                    mpTriggersMainArea->lineEdit_trigger_name->setFocus();
                    return;
                }
            }
            for (auto child : mpTriggersMainArea->findChildren<QWidget *>()) {
                if (child->hasFocus()){
                    treeWidget_triggers->setFocus();
                    return;
                }
            }
        }
        break;
    case EditorViewType::cmTimerView:
        if (QString("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            mpTimersMainArea->lineEdit_timer_name->setFocus();
            return;
        } else if (treeWidget_timers->hasFocus()) {
            mpSourceEditorEdbee->setFocus();
            return;
        } else {
            for (auto child : mpTimersMainArea->findChildren<QWidget *>()) {
                if (child->hasFocus()){
                    treeWidget_timers->setFocus();
                    return;
                }
            }
        }
        break;
    case EditorViewType::cmAliasView:
        if (QString("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            mpAliasMainArea->lineEdit_alias_name->setFocus();
            return;
        } else if (treeWidget_aliases->hasFocus()) {
            mpSourceEditorEdbee->setFocus();
            return;
        } else {
            for (auto child : mpAliasMainArea->findChildren<QWidget *>()) {
                if (child->hasFocus()){
                    treeWidget_aliases->setFocus();
                    return;
                }
            }
        }
        break;
    case EditorViewType::cmScriptView:
        if (QString("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            mpScriptsMainArea->lineEdit_script_name->setFocus();
            return;
        } else if (treeWidget_scripts->hasFocus()) {
            mpSourceEditorEdbee->setFocus();
            return;
        } else {
            for (auto child : mpScriptsMainArea->findChildren<QWidget *>()) {
                if (child->hasFocus()){
                    treeWidget_scripts->setFocus();
                    return;
                }
            }
        }
        break;
    case EditorViewType::cmActionView:
        if (QString("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            mpActionsMainArea->lineEdit_action_name->setFocus();
            return;
        } else if (treeWidget_actions->hasFocus()) {
            mpSourceEditorEdbee->setFocus();
            return;
        } else {
            for (auto child : mpActionsMainArea->findChildren<QWidget *>()) {
                if (child->hasFocus()){
                    treeWidget_actions->setFocus();
                    return;
                }
            }
        }
        break;
    case EditorViewType::cmKeysView:
        if (QString("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            mpKeysMainArea->lineEdit_key_name->setFocus();
            return;
        } else if (treeWidget_keys->hasFocus()) {
            mpSourceEditorEdbee->setFocus();
            return;
        } else {
            for (auto child : mpKeysMainArea->findChildren<QWidget *>()) {
                if (child->hasFocus()){
                    treeWidget_keys->setFocus();
                    return;
                }
            }
        }
        break;
    case EditorViewType::cmVarsView:
        if (QString("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            mpVarsMainArea->lineEdit_var_name->setFocus();
            return;
        } else if (treeWidget_variables->hasFocus()) {
            mpSourceEditorEdbee->setFocus();
            return;
        } else {
            for (auto child : mpVarsMainArea->findChildren<QWidget *>()) {
                if (child->hasFocus()){
                    treeWidget_variables->setFocus();
                    return;
                }
            }
        }
        break;
    case EditorViewType::cmUnknownView:
        return;
    }
}

void dlgTriggerEditor::slot_activateMainWindow()
{
    mudlet::self()->activateWindow();
    mpHost->mpConsole->setFocus();
}

void dlgTriggerEditor::exportTrigger(const QString& fileName)
{
    QString name;
    TTrigger* pT = nullptr;
    QTreeWidgetItem* pItem = treeWidget_triggers->currentItem();
    if (pItem) {
        int triggerID = pItem->data(0, Qt::UserRole).toInt();
        pT = mpHost->getTriggerUnit()->getTrigger(triggerID);
        if (pT) {
            name = pT->getName();
        } else {
            QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
            return;
        }
    } else {
        QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
        return;
    }
    XMLexport writer(pT);
    if (writer.exportTrigger(fileName)) {
        statusBar()->showMessage(tr("Package %1 saved").arg(name), 2000);
    }
}

void dlgTriggerEditor::exportTimer(const QString& fileName)
{
    QString name;
    TTimer* pT = nullptr;
    QTreeWidgetItem* pItem = treeWidget_timers->currentItem();
    if (pItem) {
        int triggerID = pItem->data(0, Qt::UserRole).toInt();
        pT = mpHost->getTimerUnit()->getTimer(triggerID);
        if (pT) {
            name = pT->getName();
        } else {
            QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
            return;
        }
    } else {
        QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
        return;
    }
    XMLexport writer(pT);
    if (writer.exportTimer(fileName)) {
        statusBar()->showMessage(tr("Package %1 saved").arg(name), 2000);
    }
}

void dlgTriggerEditor::exportAlias(const QString& fileName)
{
    QString name;
    TAlias* pT = nullptr;
    QTreeWidgetItem* pItem = treeWidget_aliases->currentItem();
    if (pItem) {
        int triggerID = pItem->data(0, Qt::UserRole).toInt();
        pT = mpHost->getAliasUnit()->getAlias(triggerID);
        if (pT) {
            name = pT->getName();
        } else {
            QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
            return;
        }
    } else {
        QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
        return;
    }
    XMLexport writer(pT);
    if (writer.exportAlias(fileName)) {
        statusBar()->showMessage(tr("Package %1 saved").arg(name), 2000);
    }
}

void dlgTriggerEditor::exportAction(const QString& fileName)
{
    QString name;
    TAction* pT = nullptr;
    QTreeWidgetItem* pItem = treeWidget_actions->currentItem();
    if (pItem) {
        int triggerID = pItem->data(0, Qt::UserRole).toInt();
        pT = mpHost->getActionUnit()->getAction(triggerID);
        if (pT) {
            name = pT->getName();
        } else {
            QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
            return;
        }
    } else {
        QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
        return;
    }
    XMLexport writer(pT);
    if (writer.exportAction(fileName)) {
        statusBar()->showMessage(tr("Package %1 saved").arg(name), 2000);
    }
}

void dlgTriggerEditor::exportScript(const QString& fileName)
{
    QString name;
    TScript* pT = nullptr;
    QTreeWidgetItem* pItem = treeWidget_scripts->currentItem();
    if (pItem) {
        int triggerID = pItem->data(0, Qt::UserRole).toInt();
        pT = mpHost->getScriptUnit()->getScript(triggerID);
        if (pT) {
            name = pT->getName();
        } else {
            QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
            return;
        }
    } else {
        QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
        return;
    }
    XMLexport writer(pT);
    if (writer.exportScript(fileName)) {
        statusBar()->showMessage(tr("Package %1 saved").arg(name), 2000);
    }
}

void dlgTriggerEditor::exportKey(const QString& fileName)
{
    QString name;
    TKey* pT = nullptr;
    QTreeWidgetItem* pItem = treeWidget_keys->currentItem();
    if (pItem) {
        int triggerID = pItem->data(0, Qt::UserRole).toInt();
        pT = mpHost->getKeyUnit()->getKey(triggerID);
        if (pT) {
            name = pT->getName();
        } else {
            QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
            return;
        }

    } else {
        QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
        return;
    }
    XMLexport writer(pT);
    if (writer.exportKey(fileName)) {
        statusBar()->showMessage(tr("Package %1 saved").arg(name), 2000);
    }
}

void dlgTriggerEditor::exportTriggerToClipboard()
{
    QString name;
    TTrigger* pT = nullptr;
    QTreeWidgetItem* pItem = treeWidget_triggers->currentItem();
    if (pItem) {
        int triggerID = pItem->data(0, Qt::UserRole).toInt();
        pT = mpHost->getTriggerUnit()->getTrigger(triggerID);
        if (pT) {
            name = pT->getName();
        } else {
            QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
            return;
        }
    } else {
        QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
        return;
    }
    XMLexport writer(pT);
    writer.exportToClipboard(pT);
    statusBar()->showMessage(tr("Copied %1 to clipboard").arg(name), 2000);
}

void dlgTriggerEditor::exportTimerToClipboard()
{
    QString name;
    TTimer* pT = nullptr;
    QTreeWidgetItem* pItem = treeWidget_timers->currentItem();
    if (pItem) {
        int triggerID = pItem->data(0, Qt::UserRole).toInt();
        pT = mpHost->getTimerUnit()->getTimer(triggerID);
        if (pT) {
            name = pT->getName();
        } else {
            QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
            return;
        }
    } else {
        QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
        return;
    }
    XMLexport writer(pT);
    writer.exportToClipboard(pT);
    statusBar()->showMessage(tr("Copied %1 to clipboard").arg(name), 2000);
}

void dlgTriggerEditor::exportAliasToClipboard()
{
    QString name;
    TAlias* pT = nullptr;
    QTreeWidgetItem* pItem = treeWidget_aliases->currentItem();
    if (pItem) {
        int triggerID = pItem->data(0, Qt::UserRole).toInt();
        pT = mpHost->getAliasUnit()->getAlias(triggerID);
        if (pT) {
            name = pT->getName();
        } else {
            QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
            return;
        }
    } else {
        QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
        return;
    }
    XMLexport writer(pT);
    writer.exportToClipboard(pT);
    statusBar()->showMessage(tr("Copied %1 to clipboard").arg(name), 2000);
}

void dlgTriggerEditor::exportActionToClipboard()
{
    QString name;
    TAction* pT = nullptr;
    QTreeWidgetItem* pItem = treeWidget_actions->currentItem();
    if (pItem) {
        int triggerID = pItem->data(0, Qt::UserRole).toInt();
        pT = mpHost->getActionUnit()->getAction(triggerID);
        if (pT) {
            name = pT->getName();
        } else {
            QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
            return;
        }
    } else {
        QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
        return;
    }
    XMLexport writer(pT);
    writer.exportToClipboard(pT);
    statusBar()->showMessage(tr("Copied %1 to clipboard").arg(name), 2000);
}

void dlgTriggerEditor::exportScriptToClipboard()
{
    QString name;
    TScript* pT = nullptr;
    QTreeWidgetItem* pItem = treeWidget_scripts->currentItem();
    if (pItem) {
        int triggerID = pItem->data(0, Qt::UserRole).toInt();
        pT = mpHost->getScriptUnit()->getScript(triggerID);
        if (pT) {
            name = pT->getName();
        } else {
            QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
            return;
        }
    } else {
        QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
        return;
    }
    XMLexport writer(pT);
    writer.exportToClipboard(pT);
    statusBar()->showMessage(tr("Copied %1 to clipboard").arg(name), 2000);
}

void dlgTriggerEditor::exportKeyToClipboard()
{
    QString name;
    TKey* pT = nullptr;
    QTreeWidgetItem* pItem = treeWidget_keys->currentItem();
    if (pItem) {
        int triggerID = pItem->data(0, Qt::UserRole).toInt();
        pT = mpHost->getKeyUnit()->getKey(triggerID);
        if (pT) {
            name = pT->getName();
        } else {
            QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
            return;
        }

    } else {
        QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
        return;
    }
    XMLexport writer(pT);
    writer.exportToClipboard(pT);
    statusBar()->showMessage(tr("Copied %1 to clipboard").arg(name), 2000);
}


void dlgTriggerEditor::slot_export()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Triggers"), QDir::currentPath(), tr("Mudlet packages (*.xml)"));
    if (fileName.isEmpty()) {
        return;
    }

    // Must be case insensitive to work on MacOS platforms, possibly a cause of
    // https://bugs.launchpad.net/mudlet/+bug/1417234
    if (!fileName.endsWith(QStringLiteral(".xml"), Qt::CaseInsensitive)) {
        fileName.append(QStringLiteral(".xml"));
    }


    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("export package:"), tr("Cannot write file %1:\n%2.").arg(fileName, file.errorString()));
        return;
    }
    // Should close the file that we have confirmed can be opened:
    file.close();

    switch (mCurrentView) {
    case EditorViewType::cmTriggerView:
        exportTrigger(fileName);
        break;
    case EditorViewType::cmTimerView:
        exportTimer(fileName);
        break;
    case EditorViewType::cmAliasView:
        exportAlias(fileName);
        break;
    case EditorViewType::cmScriptView:
        exportScript(fileName);
        break;
    case EditorViewType::cmActionView:
        exportAction(fileName);
        break;
    case EditorViewType::cmKeysView:
        exportKey(fileName);
        break;
    }
}

void dlgTriggerEditor::slot_copy_xml()
{
    switch (mCurrentView) {
    case EditorViewType::cmTriggerView:
        exportTriggerToClipboard();
        break;
    case EditorViewType::cmTimerView:
        exportTimerToClipboard();
        break;
    case EditorViewType::cmAliasView:
        exportAliasToClipboard();
        break;
    case EditorViewType::cmScriptView:
        exportScriptToClipboard();
        break;
    case EditorViewType::cmActionView:
        exportActionToClipboard();
        break;
    case EditorViewType::cmKeysView:
        exportKeyToClipboard();
        break;
    }
}

void dlgTriggerEditor::slot_paste_xml()
{
    XMLimport reader(mpHost);
    EditorViewType importedItemType = EditorViewType::cmUnknownView;
    int importedItemID = 0;

    switch (mCurrentView) {
    case EditorViewType::cmTriggerView:
        saveTrigger();
        break;
    case EditorViewType::cmTimerView:
        saveTimer();
        break;
    case EditorViewType::cmAliasView:
        saveAlias();
        break;
    case EditorViewType::cmScriptView:
        saveScript();
        break;
    case EditorViewType::cmActionView:
        saveAction();
        break;
    case EditorViewType::cmKeysView:
        saveKey();
        break;
    }

    std::tie(importedItemType, importedItemID) = reader.importFromClipboard();

    // don't reset the view if what we pasted wasn't a Mudlet editor item
    if (importedItemType == EditorViewType::cmUnknownView && importedItemID == 0) {
        return;
    }

    mCurrentView = static_cast<EditorViewType>(importedItemType);
    // importing drops the item at the bottom of the list - move it to be a sibling
    // of the currently selected item instead
    switch (mCurrentView) {
    case EditorViewType::cmTriggerView: {
        // in case this is a nested item, grab the parent data for the move function
        // as well. In case it's a root item, this doesn't seem to matter
        auto parent = treeWidget_triggers->currentIndex().parent();
        auto parentRow = parent.row();
        auto parentId = parent.data(Qt::UserRole).toInt();

        int siblingRow = treeWidget_triggers->currentIndex().row() + 1;
        mpHost->getTriggerUnit()->reParentTrigger(importedItemID, 0, parentId, parentRow, siblingRow);
        break;
    }
    case EditorViewType::cmTimerView: {
        auto parent = treeWidget_timers->currentIndex().parent();
        auto parentRow = parent.row();
        auto parentId = parent.data(Qt::UserRole).toInt();

        int siblingRow = treeWidget_timers->currentIndex().row() + 1;
        mpHost->getTimerUnit()->reParentTimer(importedItemID, 0, parentId, parentRow, siblingRow);
        break;
    }
    case EditorViewType::cmAliasView: {
        auto parent = treeWidget_aliases->currentIndex().parent();
        auto parentRow = parent.row();
        auto parentId = parent.data(Qt::UserRole).toInt();

        int siblingRow = treeWidget_aliases->currentIndex().row() + 1;
        mpHost->getAliasUnit()->reParentAlias(importedItemID, 0, parentId, parentRow, siblingRow);
        break;
    }
    case EditorViewType::cmScriptView: {
        auto parent = treeWidget_scripts->currentIndex().parent();
        auto parentRow = parent.row();
        auto parentId = parent.data(Qt::UserRole).toInt();

        int siblingRow = treeWidget_scripts->currentIndex().row() + 1;
        mpHost->getScriptUnit()->reParentScript(importedItemID, 0, parentId, parentRow, siblingRow);
        break;
    }
    case EditorViewType::cmActionView: {
        auto parent = treeWidget_actions->currentIndex().parent();
        auto parentRow = parent.row();
        auto parentId = parent.data(Qt::UserRole).toInt();

        int siblingRow = treeWidget_actions->currentIndex().row() + 1;
        mpHost->getActionUnit()->reParentAction(importedItemID, 0, parentId, parentRow, siblingRow);
        break;
    }
    case EditorViewType::cmKeysView: {
        auto parent = treeWidget_keys->currentIndex().parent();
        auto parentRow = parent.row();
        auto parentId = parent.data(Qt::UserRole).toInt();

        int siblingRow = treeWidget_keys->currentIndex().row() + 1;
        mpHost->getKeyUnit()->reParentKey(importedItemID, 0, parentId, parentRow, siblingRow);
        break;
    }
    }

    // flag for re-rendering so the new item shows up in the right spot
    mNeedUpdateData = true;

    switch (importedItemType) {
    case EditorViewType::cmTriggerView: {
        // the view becomes collapsed as a result of the clear & redo and then
        // animates back into the unfolding, which doesn't look nice - so turn
        // off animation temporarily
        auto animated = treeWidget_triggers->isAnimated();
        treeWidget_triggers->setAnimated(false);
        selectTriggerByID(importedItemID);
        treeWidget_triggers->setAnimated(animated);

        // set the focus because hiding checkBox_displayAllVariables in changeView
        // changes the focus to the search box for some reason. This thus breaks
        // successive pastes because you'll now be pasting into the search box
        treeWidget_triggers->setFocus();
        break;
    }
    case EditorViewType::cmTimerView: {
        auto animated = treeWidget_timers->isAnimated();
        treeWidget_timers->setAnimated(false);
        selectTimerByID(importedItemID);
        treeWidget_timers->setAnimated(animated);
        treeWidget_timers->setFocus();
        break;
    }
    case EditorViewType::cmAliasView: {
        auto animated = treeWidget_aliases->isAnimated();
        treeWidget_aliases->setAnimated(false);
        selectAliasByID(importedItemID);
        treeWidget_aliases->setAnimated(animated);
        treeWidget_aliases->setFocus();
        break;
    }
    case EditorViewType::cmScriptView: {
        auto animated = treeWidget_scripts->isAnimated();
        treeWidget_scripts->setAnimated(false);
        selectScriptByID(importedItemID);
        treeWidget_scripts->setAnimated(animated);
        treeWidget_scripts->setFocus();
        break;
    }
    case EditorViewType::cmActionView: {
        auto animated = treeWidget_actions->isAnimated();
        treeWidget_actions->setAnimated(false);
        selectActionByID(importedItemID);
        treeWidget_actions->setAnimated(animated);
        treeWidget_actions->setFocus();
        break;
    }
    case EditorViewType::cmKeysView: {
        auto animated = treeWidget_keys->isAnimated();
        treeWidget_keys->setAnimated(false);
        selectKeyByID(importedItemID);
        treeWidget_keys->setAnimated(animated);
        treeWidget_keys->setFocus();
        break;
    }
    }
}

// CHECKME: This seems to largely duplicate the actions of Host::installPackage(...)
// Do we really need two different sets of code to import packages?
void dlgTriggerEditor::slot_import()
{
    switch (mCurrentView) {
    case EditorViewType::cmTriggerView:
        saveTrigger();
        break;
    case EditorViewType::cmTimerView:
        saveTimer();
        break;
    case EditorViewType::cmAliasView:
        saveAlias();
        break;
    case EditorViewType::cmScriptView:
        saveScript();
        break;
    case EditorViewType::cmActionView:
        saveAction();
        break;
    case EditorViewType::cmKeysView:
        saveKey();
        break;
    }

    QString fileName = QFileDialog::getOpenFileName(this, tr("Import Mudlet Package"), QDir::currentPath());
    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Import Mudlet Package:"), tr("Cannot read file %1:\n%2.").arg(fileName, file.errorString()));
        return;
    }

    QString packageName = fileName.section(QChar('/'), -1);
    packageName.remove(QStringLiteral(".zip"), Qt::CaseInsensitive);
    packageName.remove(QStringLiteral(".trigger"), Qt::CaseInsensitive);
    packageName.remove(QStringLiteral(".xml"), Qt::CaseInsensitive);
    packageName.remove(QStringLiteral(".mpackage"), Qt::CaseInsensitive);
    packageName.remove(QChar('/'));
    packageName.remove(QChar('\\'));
    packageName.remove(QChar('.'));

    if (mpHost->mInstalledPackages.contains(packageName)) {
        QMessageBox::information(this, tr("Import Mudlet Package:"), tr("Package %1 is already installed.").arg(packageName));
        file.close();
        return;
    }

    QFile file2;
    if (fileName.endsWith(QStringLiteral(".zip"), Qt::CaseInsensitive) || fileName.endsWith(QStringLiteral(".mpackage"), Qt::CaseInsensitive)) {
        QString _dest = mudlet::getMudletPath(mudlet::profilePackagePath, mpHost->getName(), packageName);
        QDir _tmpDir;
        _tmpDir.mkpath(_dest);
        QString _script = QStringLiteral("unzip([[%1]], [[%2]])").arg(fileName, _dest);
        mpHost->mLuaInterpreter.compileAndExecuteScript(_script);

        // requirements for zip packages:
        // - packages must be compressed in zip format
        // - file extension should be .mpackage (though .zip is accepted)
        // - there can only be a single xml file per package
        // - the xml file must be located in the root directory of the zip package. example: myPack.zip contains: the folder images and the file myPack.xml

        QDir _dir(_dest);
        QStringList _filterList;
        _filterList << "*.xml"
                    << "*.trigger";
        QFileInfoList entries = _dir.entryInfoList(_filterList, QDir::Files);
        if (!entries.empty()) {
            file2.setFileName(entries[0].absoluteFilePath());
        }
    } else {
        file2.setFileName(fileName);
    }
    file2.open(QFile::ReadOnly | QFile::Text);

    mpHost->mInstalledPackages.append(packageName);
    QString profileName = mpHost->getName();
    QString login = mpHost->getLogin();
    QString pass = mpHost->getPass();

    treeWidget_triggers->clear();
    treeWidget_aliases->clear();
    treeWidget_actions->clear();
    treeWidget_timers->clear();
    treeWidget_keys->clear();
    treeWidget_scripts->clear();

    XMLimport reader(mpHost);
    reader.importPackage(&file2, packageName); // TODO: Missing false return value handler

    mpHost->setName(profileName);
    mpHost->setLogin(login);
    mpHost->setPass(pass);

    slot_profileSaveAction();

    fillout_form();

    mpCurrentTriggerItem = nullptr;
    mpCurrentTimerItem = nullptr;
    mpCurrentAliasItem = nullptr;
    mpCurrentScriptItem = nullptr;
    mpCurrentActionItem = nullptr;
    mpCurrentKeyItem = nullptr;

    slot_show_triggers();
}

void dlgTriggerEditor::doCleanReset()
{
    if (mCleanResetQueued) {
        return;
    }

    mCleanResetQueued = true;

    QTimer::singleShot(0, this, [=]() {
        mCleanResetQueued = false;

        runScheduledCleanReset();
    });
}
void dlgTriggerEditor::runScheduledCleanReset()
{
    switch (mCurrentView) {
    case EditorViewType::cmTriggerView:
        saveTrigger();
        break;
    case EditorViewType::cmTimerView:
        saveTimer();
        break;
    case EditorViewType::cmAliasView:
        saveAlias();
        break;
    case EditorViewType::cmScriptView:
        saveScript();
        break;
    case EditorViewType::cmActionView:
        saveAction();
        break;
    case EditorViewType::cmKeysView:
        saveKey();
        break;
    }

    treeWidget_triggers->clear();
    treeWidget_aliases->clear();
    treeWidget_actions->clear();
    treeWidget_timers->clear();
    treeWidget_keys->clear();
    treeWidget_scripts->clear();
    fillout_form();
    mpCurrentTriggerItem = nullptr;
    mpCurrentTimerItem = nullptr;
    mpCurrentAliasItem = nullptr;
    mpCurrentScriptItem = nullptr;
    mpCurrentActionItem = nullptr;
    mpCurrentKeyItem = nullptr;
    slot_show_triggers();
}

void dlgTriggerEditor::slot_profileSaveAction()
{
    std::tuple<bool, QString, QString> result = mpHost->saveProfile(nullptr, nullptr, true);

    if (!std::get<0>(result)) {
        QMessageBox::critical(this, tr("Couldn't save profile"), tr("Sorry, couldn't save your profile - got the following error: %1").arg(std::get<2>(result)));
    }
}

void dlgTriggerEditor::slot_profileSaveAsAction()
{
    mSavingAs = true;
    QString fileName = QFileDialog::getSaveFileName(this, tr("Backup Profile"), QDir::homePath(), tr("trigger files (*.trigger *.xml)"));

    if (fileName.isEmpty()) {
        return;
    }
    // Must be case insensitive to work on MacOS platforms, possibly a cause of
    // https://bugs.launchpad.net/mudlet/+bug/1417234
    if (!fileName.endsWith(QStringLiteral(".xml"), Qt::CaseInsensitive) && !fileName.endsWith(QStringLiteral(".trigger"), Qt::CaseInsensitive)) {
        fileName.append(QStringLiteral(".xml"));
    }

    mpHost->saveProfileAs(fileName);
    mSavingAs = false;
}

bool dlgTriggerEditor::eventFilter(QObject*, QEvent* event)
{
    if (event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        switch (keyEvent->key())
        {
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_Left:
        case Qt::Key_Right:
        case Qt::Key_Escape: // This one is needed to allow it to be used to CANCEL the key grab
            this->event(event);
            return true;
        default:
            return false;
        }
    }
    return false;
}

bool dlgTriggerEditor::event(QEvent* event)
{
    if (mIsGrabKey) {
        if (event->type() == QEvent::KeyPress) {
            auto * ke = static_cast<QKeyEvent*>(event);
            QList<QAction*> actionList = toolBar->actions();
            switch (ke->key()) {
            case 0x01000000:
                mIsGrabKey = false;
                for (auto& action : actionList) {
                    if (action->text() == "Save Item") {
                        action->setShortcut(tr("Ctrl+S"));
                    } else if (action->text() == "Save Profile") {
                        action->setShortcut(tr("Ctrl+Shift+S"));
                    }
                }
                QCoreApplication::instance()->removeEventFilter(this);
                ke->accept();
                return true;
            case 0x01000020:
            case 0x01000021:
            case 0x01000022:
            case 0x01000023:
            case 0x01001103:
                break;
            default:
                key_grab_callback(ke->key(), ke->modifiers());
                mIsGrabKey = false;
                for (auto& action : actionList) {
                    if (action->text() == "Save Item") {
                        action->setShortcut(tr("Ctrl+S"));
                    } else if (action->text() == "Save Profile") {
                        action->setShortcut(tr("Ctrl+Shift+S"));
                    }
                }
                QCoreApplication::instance()->removeEventFilter(this);
                ke->accept();
                return true;
            }
        }
    }
    return QMainWindow::event(event);
}

void dlgTriggerEditor::resizeEvent(QResizeEvent* event)
{
    if (mpSourceEditorArea->isVisible()) {
        slot_move_source_find();
    }
}

void dlgTriggerEditor::slot_key_grab()
{
    mIsGrabKey = true;
    QList<QAction*> actionList = toolBar->actions();
    for (auto& action : actionList) {
        if (action->text() == "Save Item") {
            action->setShortcut(tr(""));
        } else if (action->text() == "Save Profile") {
            action->setShortcut(tr(""));
        }
    }
    QCoreApplication::instance()->installEventFilter(this);
}

void dlgTriggerEditor::key_grab_callback(int key, int modifier)
{
    KeyUnit* pKeyUnit = mpHost->getKeyUnit();
    if (!pKeyUnit) {
        return;
    }
    QString keyName = pKeyUnit->getKeyName(key, modifier);
    QString name = keyName;
    mpKeysMainArea->lineEdit_key_binding->setText(name);
    QTreeWidgetItem* pItem = treeWidget_keys->currentItem();
    if (pItem) {
        int triggerID = pItem->data(0, Qt::UserRole).toInt();
        TKey* pT = mpHost->getKeyUnit()->getKey(triggerID);
        if (pT) {
            pT->setKeyCode(key);
            pT->setKeyModifiers(modifier);
        }
    }
}

void dlgTriggerEditor::slot_chose_action_icon()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Seclect Icon"), QDir::homePath(), tr("Images (*.png *.xpm *.jpg)"));
    mpActionsMainArea->lineEdit_action_icon->setText(fileName);
}

void dlgTriggerEditor::slot_toggle_isPushDownButton(const int state)
{
    if (state == Qt::Checked) {
        mpActionsMainArea->lineEdit_action_button_command_up->show();
        mpActionsMainArea->label_action_button_command_up->show();
        mpActionsMainArea->label_action_button_command_down->setText(tr("Command (down):"));
    } else {
        mpActionsMainArea->lineEdit_action_button_command_up->hide();
        mpActionsMainArea->label_action_button_command_up->hide();
        mpActionsMainArea->label_action_button_command_down->setText(tr("Command:"));
    }
}

// Set the foreground color that will be applied to text that matches the trigger pattern(s)
void dlgTriggerEditor::slot_colorizeTriggerSetFgColor()
{
    QTreeWidgetItem* pItem = mpCurrentTriggerItem;
    if (!pItem) {
        return;
    }
    if (!pItem->parent()) {
        return;
    }

    auto color = QColorDialog::getColor(QColor(mpTriggersMainArea->pushButtonFgColor->property(cButtonBaseColor).toString()),
                                        this,
                                        tr("Select foreground color to apply to matches"));
    if (color.isValid()) {
        mpTriggersMainArea->pushButtonFgColor->setStyleSheet(generateButtonStyleSheet(color));
        mpTriggersMainArea->pushButtonFgColor->setProperty(cButtonBaseColor, color.name());
    }
}

// Set the background color that will be applied to text that matches the trigger pattern(s)
void dlgTriggerEditor::slot_colorizeTriggerSetBgColor()
{
    QTreeWidgetItem* pItem = mpCurrentTriggerItem;
    if (!pItem) {
        return;
    }
    if (!pItem->parent()) {
        return;
    }

    auto color = QColorDialog::getColor(QColor(mpTriggersMainArea->pushButtonBgColor->property(cButtonBaseColor).toString()),
                                        this,
                                        tr("Select background color to apply to matches"));
    if (color.isValid()) {
        mpTriggersMainArea->pushButtonBgColor->setStyleSheet(generateButtonStyleSheet(color));
        mpTriggersMainArea->pushButtonBgColor->setProperty(cButtonBaseColor, color.name());
    }
}

void dlgTriggerEditor::slot_soundTrigger()
{
    // Use the existing path/filename if it is not empty, otherwise start in
    // profile home directory:
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Choose sound file"),
                                                    mpTriggersMainArea->lineEdit_soundFile->text().isEmpty()
                                                    ? mudlet::getMudletPath(mudlet::profileHomePath, mpHost->getName())
                                                    : mpTriggersMainArea->lineEdit_soundFile->text(),
                                                    tr("Audio files(*.aac *.mp3 *.mp4a *.oga *.ogg *.pcm *.wav *.wma);;"
                                                       "Advanced Audio Coding-stream(*.aac);;"
                                                       "MPEG-2 Audio Layer 3(*.mp3);;"
                                                       "MPEG-4 Audio(*.mp4a);;"
                                                       "Ogg Vorbis(*.oga *.ogg);;"
                                                       "PCM Audio(*.pcm);;"
                                                       "Wave(*.wav);;"
                                                       "Windows Media Audio(*.wma);;"
                                                       "All files(*.*)",
                                                       // Intentional comment
                                                       "This the list of file extensions that are considered for sounds from triggers, the terms inside of the '('...')' and the \";;\" are used programmatically and should not be changed."));
    if (!fileName.isEmpty()) {
        // This will only be executed if the user did not press cancel
        mpTriggersMainArea->lineEdit_soundFile->setToolTip(fileName);
        mpTriggersMainArea->lineEdit_soundFile->setText(fileName);
        mpTriggersMainArea->lineEdit_soundFile->setCursorPosition(mpTriggersMainArea->lineEdit_soundFile->text().length());
        mpTriggersMainArea->toolButton_clearSoundFile->setEnabled(!mpTriggersMainArea->lineEdit_soundFile->text().isEmpty());
    }
}

// Get the color from the user to use as that to look for as the foreground in
// a color trigger:
void dlgTriggerEditor::slot_color_trigger_fg()
{
    QTreeWidgetItem* pItem = mpCurrentTriggerItem;
    if (!pItem) {
        return;
    }
    int triggerID = pItem->data(0, Qt::UserRole).toInt();
    TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(triggerID);
    if (!pT) {
        return;
    }

    auto * pB = qobject_cast<QPushButton*>(sender());
    if (!pB) {
        return;
    }

    dlgTriggerPatternEdit* pPatternItem = qobject_cast<dlgTriggerPatternEdit*>(pB->parent());
    if (!pPatternItem) {
        return;
    }

    // This method parses the pattern text and extracts the ansi color values
    // from it - including the special values of DEFAULT (-2) and IGNORE (-1)
    // and assigns the values to the other arguments:
    TTrigger::decodeColorPatternText(pPatternItem->lineEdit_pattern->text(), pT->mColorTriggerFgAnsi, pT->mColorTriggerBgAnsi);

    // The following method wants to know BOTH existing fore and backgrounds
    // it will select the appropriate as a result of the third argument and it
    // uses both to determine whether the result to return is valid considering
    // the other, non used (background in this method) part:
    auto pD = new dlgColorTrigger(this, pT, false, tr("Select foreground trigger color for item %1").arg(QString::number(pPatternItem->mRow+1)));
    pD->setModal(true);
    // This sounds a bit iffy - prevent access to other application windows
    // whilst we get a colour setting:
    pD->setWindowModality(Qt::ApplicationModal);
    pD->exec();

    QColor color = pT->mColorTriggerFgColor;
    // The above will be an invalid colour if the colour has been reset/ignored
    // The dialogue should have changed pT->mColorTriggerFgAnsi
    QString styleSheet;
    if (color.isValid()) {
        styleSheet = generateButtonStyleSheet(color);
    }
    pB->setStyleSheet(styleSheet);

    pPatternItem->lineEdit_pattern->setText(TTrigger::createColorPatternText(pT->mColorTriggerFgAnsi, pT->mColorTriggerBgAnsi));

    if (pT->mColorTriggerFgAnsi == TTrigger::scmIgnored) {
        pB->setText(tr("Foreground color ignored",
                       "Color trigger ignored foreground color button, ensure all three instances have the same text"));
    } else if (pT->mColorTriggerFgAnsi == TTrigger::scmDefault) {
        pB->setText(tr("Default foreground color",
                       "Color trigger default foreground color button, ensure all three instances have the same text"));
    } else {
        pB->setText(tr("Foreground color [ANSI %1]",
                       "Color trigger ANSI foreground color button, ensure all three instances have the same text")
                    .arg(QString::number(pT->mColorTriggerFgAnsi)));
    }
}

// Get the color from the user to use as that to look for as the background in
// a color trigger:
void dlgTriggerEditor::slot_color_trigger_bg()
{
    QTreeWidgetItem* pItem = mpCurrentTriggerItem;
    if (!pItem) {
        return;
    }
    int triggerID = pItem->data(0, Qt::UserRole).toInt();
    TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(triggerID);
    if (!pT) {
        return;
    }

    auto * pB = qobject_cast<QPushButton*>(sender());
    if (!pB) {
        return;
    }

    dlgTriggerPatternEdit* pPatternItem = qobject_cast<dlgTriggerPatternEdit*>(pB->parent());
    if (!pPatternItem) {
        return;
    }

    // This method parses the pattern text and extracts the ansi color values
    // from it - including the special values of DEFAULT (-2) and IGNORE (-1)
    // and assigns the values to the other arguments:
    TTrigger::decodeColorPatternText(pPatternItem->lineEdit_pattern->text(), pT->mColorTriggerFgAnsi, pT->mColorTriggerBgAnsi);

    // The following method wants to know BOTH existing fore and backgrounds
    // it will select the appropriate as a result of the third argument and it
    // uses both to determine whether the result to return is valid considering
    // the other, non used (background in this method) part:
    auto pD = new dlgColorTrigger(this, pT, true, tr("Select background trigger color for item %1").arg(QString::number(pPatternItem->mRow+1)));
    pD->setModal(true);
    // This sounds a bit iffy - prevent access to other application windows
    // whilst we get a colour setting:
    pD->setWindowModality(Qt::ApplicationModal);
    pD->exec();

    QColor color = pT->mColorTriggerBgColor;
    // The above will be an invalid colour if the colour has been reset/ignored
    QString styleSheet;
    if (color.isValid()) {
        styleSheet = generateButtonStyleSheet(color);
    }
    pB->setStyleSheet(styleSheet);

    pPatternItem->lineEdit_pattern->setText(TTrigger::createColorPatternText(pT->mColorTriggerFgAnsi, pT->mColorTriggerBgAnsi));

    if (pT->mColorTriggerBgAnsi == TTrigger::scmIgnored) {
        pB->setText(tr("Background color ignored",
                       "Color trigger ignored background color button, ensure all three instances have the same text"));
    } else if (pT->mColorTriggerBgAnsi == TTrigger::scmDefault) {
        pB->setText(tr("Default background color",
                       "Color trigger default background color button, ensure all three instances have the same text"));
    } else {
        pB->setText(tr("Background color [ANSI %1]",
                       "Color trigger ANSI background color button, ensure all three instances have the same text")
                    .arg(QString::number(pT->mColorTriggerBgAnsi)));
    }
}

void dlgTriggerEditor::slot_updateStatusBar(const QString& statusText)
{
    // edbee adds the scope and last command which is rather technical debugging information,
    // so strip it away by removing the first pipe and everything after it
    QRegularExpressionMatch match = simplifyEdbeeStatusBarRegex->match(statusText, 0, QRegularExpression::PartialPreferFirstMatch);
    QString stripped;
    if (match.hasPartialMatch() || match.hasMatch()) {
        stripped = match.captured(1);
    } else {
        stripped = statusText;
    }

    QMainWindow::statusBar()->showMessage(stripped);
}

void dlgTriggerEditor::slot_profileSaveStarted()
{
    mProfileSaveAction->setDisabled(true);
    mProfileSaveAsAction->setDisabled(true);
    mProfileSaveAction->setText(tr("Saving…"));
}

void dlgTriggerEditor::slot_profileSaveFinished()
{
    mProfileSaveAction->setEnabled(true);
    mProfileSaveAsAction->setEnabled(true);
    mProfileSaveAction->setText(tr("Save Profile"));
}

void dlgTriggerEditor::slot_changeEditorTextOptions(QTextOption::Flags state)
{
    edbee::TextEditorConfig* config = mpSourceEditorEdbee->config();

    config->beginChanges();
    config->setShowWhitespaceMode((state & QTextOption::ShowTabsAndSpaces)
                                  ? edbee::TextEditorConfig::ShowWhitespaces
                                  : edbee::TextEditorConfig::HideWhitespaces);
    config->setUseLineSeparator(state & QTextOption::ShowLineAndParagraphSeparators);
    config->endChanges();
}

//
// clearDocument( edbee::TextEditorWidget* ew)
//
// A temporary measure for dealing with the undo spanning over multiple documents bug,
// in place until we create a proper multi-document solution. This gets called whenever
// the editor needs to be "cleared", usually when a different alias/trigger/etc is
// made or selected.

void dlgTriggerEditor::clearDocument(edbee::TextEditorWidget* ew, const QString& initialText)
{
    mpSourceEditorFindArea->hide();
    mpSourceEditorEdbeeDocument = new edbee::CharTextDocument();
    // Buck.lua is a fake filename for edbee to figure out its lexer type with. Referencing the
    // lexer directly by name previously gave problems.
    mpSourceEditorEdbeeDocument->setLanguageGrammar(edbee::Edbee::instance()->grammarManager()->detectGrammarWithFilename(QLatin1Literal("Buck.lua")));
    ew->controller()->giveTextDocument(mpSourceEditorEdbeeDocument);

    auto config = mpSourceEditorEdbee->config();
    config->beginChanges();
    config->setThemeName(mpHost->mEditorTheme);
    config->setFont(mpHost->getDisplayFont());
    config->setShowWhitespaceMode((mudlet::self()->mEditorTextOptions & QTextOption::ShowTabsAndSpaces)
                                  ? edbee::TextEditorConfig::ShowWhitespaces
                                  : edbee::TextEditorConfig::HideWhitespaces);
    config->setUseLineSeparator(mudlet::self()->mEditorTextOptions & QTextOption::ShowLineAndParagraphSeparators);
    config->setSmartTab(true);
    config->setUseTabChar(false); // when you press Enter for a newline, pad with spaces and not tabs
    config->setCaretBlinkRate(200);
    config->setIndentSize(2);
    config->setCaretWidth(1);
    config->setAutocompleteAutoShow(mpHost->mEditorAutoComplete);
    config->endChanges();

    // If undo is not disabled when setting the initial text, the
    // setting of the text will be undoable.
    mpSourceEditorEdbeeDocument->setUndoCollectionEnabled(false);
    mpSourceEditorEdbeeDocument->setText(initialText);
    mpSourceEditorEdbeeDocument->setUndoCollectionEnabled(true);
}

// We do NOT want to change every profile's editor theme when the setting is
// changed in the settings dialog so this has been moved out of a lambda wired
// up as a slot to respond to a
// mudlet::signal_editorThemeChanged(const QString& theme) signal
void dlgTriggerEditor::setThemeAndOtherSettings(const QString& theme)
{
        auto localConfig = mpSourceEditorEdbee->config();
        localConfig->beginChanges();
        localConfig->setThemeName(theme);
        localConfig->setFont(mpHost->getDisplayFont());
        localConfig->setShowWhitespaceMode((mudlet::self()->mEditorTextOptions & QTextOption::ShowTabsAndSpaces)
                                           ? edbee::TextEditorConfig::ShowWhitespaces
                                           : edbee::TextEditorConfig::HideWhitespaces);
        localConfig->setUseLineSeparator(mudlet::self()->mEditorTextOptions & QTextOption::ShowLineAndParagraphSeparators);
        localConfig->setAutocompleteAutoShow(mpHost->mEditorAutoComplete);
        localConfig->endChanges();
}

void dlgTriggerEditor::createSearchOptionIcon()
{
    // When we add new search options we must create icons for each combination
    // beforehand - which is simpler than having to do code to combine the
    // QPixMaps...
    QIcon newIcon;
    switch (mSearchOptions) {
    // Each combination must be handled here
    case SearchOptionCaseSensitive|SearchOptionIncludeVariables:
        newIcon.addPixmap(QPixmap(":/icons/searchOptions-caseSensitive+withVariables.png"));
        break;

    case SearchOptionIncludeVariables:
        newIcon.addPixmap(QPixmap(":/icons/searchOptions-withVariables.png"));
        break;

    case SearchOptionCaseSensitive:
        newIcon.addPixmap(QPixmap(":/icons/searchOptions-caseSensitive.png"));
        break;

    case SearchOptionNone:
        // Use the grey icon as that is appropriate for the "No options set" case
        newIcon.addPixmap(QPixmap(":/icons/searchOptions-none.png"));
        break;

    default:
        // Don't grey out this one - is a diagnositic for an uncoded combination
        newIcon.addPixmap(QPixmap(":/icons/searchOptions-unspecified.png"));
    }

    // Store the current setting icon - may need to copy it into the grandparent QComboBox items
    mIcon_searchOptions = newIcon;
    // Applied it to the QLineEdit for display purposes
    mpAction_searchOptions->setIcon(newIcon);
}

void dlgTriggerEditor::slot_toggleSearchCaseSensitivity(const bool state)
{
    if ((mSearchOptions & SearchOptionCaseSensitive) != state) {
        mSearchOptions = (mSearchOptions & ~(SearchOptionCaseSensitive)) | (state ? SearchOptionCaseSensitive : SearchOptionNone);
        createSearchOptionIcon();
        mpHost->mSearchOptions = mSearchOptions;
    }
}

void dlgTriggerEditor::slot_toggleSearchIncludeVariables(const bool state)
{
    if ((mSearchOptions & SearchOptionIncludeVariables) != state) {
        mSearchOptions = (mSearchOptions & ~(SearchOptionIncludeVariables)) | (state ? SearchOptionIncludeVariables : SearchOptionNone);
        createSearchOptionIcon();
        mpHost->mSearchOptions = mSearchOptions;
    }
}

void dlgTriggerEditor::slot_clearSearchResults()
{
    // Want the clearing of the search results to show:
    treeWidget_searchResults->clear();
    treeWidget_searchResults->update();

    // unhighlight all instances of the item that we've searched for.
    // edbee already remembers this from a setSearchTerm() call elsewhere
    auto controller = mpSourceEditorEdbee->controller();
    auto textRanges = controller->borderedTextRanges();
    textRanges->clear();
    controller->update();
}

// shows a custom right-click menu for the editor, including the indent action
void dlgTriggerEditor::slot_editorContextMenu()
{
    edbee::TextEditorWidget* editor = mpSourceEditorEdbee;
    if (!editor) {
        return;
    }

    edbee::TextEditorController* controller = mpSourceEditorEdbee->controller();

    auto menu = new QMenu();
    auto formatAction = new QAction(tr("Format All"), menu);
    // appropriate shortcuts are automatically supplied by edbee here
    if (qApp->testAttribute(Qt::AA_DontShowIconsInMenus)) {
        menu->addAction(controller->createAction("cut", tr("Cut"), QIcon(), menu));
        menu->addAction(controller->createAction("copy", tr("Copy"), QIcon(), menu));
        menu->addAction(controller->createAction("paste", tr("Paste"), QIcon(), menu));
        menu->addSeparator();
        menu->addAction(controller->createAction("sel_all", tr("Select All"), QIcon(), menu));
    } else {
        menu->addAction(controller->createAction("cut", tr("Cut"), QIcon::fromTheme(QStringLiteral("edit-cut"), QIcon(QStringLiteral(":/icons/edit-cut.png"))), menu));
        menu->addAction(controller->createAction("copy", tr("Copy"), QIcon::fromTheme(QStringLiteral("edit-copy"), QIcon(QStringLiteral(":/icons/edit-copy.png"))), menu));
        menu->addAction(controller->createAction("paste", tr("Paste"), QIcon::fromTheme(QStringLiteral("edit-paste"), QIcon(QStringLiteral(":/icons/edit-paste.png"))), menu));
        menu->addSeparator();
        menu->addAction(controller->createAction("sel_all", tr("Select All"), QIcon::fromTheme(QStringLiteral("edit-select-all"), QIcon(QStringLiteral(":/icons/edit-select-all.png"))), menu));
        formatAction->setIcon(QIcon::fromTheme(QStringLiteral("run-build-clean"), QIcon::fromTheme(QStringLiteral("run-build-clean"))));
    }

    connect(formatAction, &QAction::triggered, this, [=]() {
        auto formattedText = mpHost->mLuaInterpreter.formatLuaCode(mpSourceEditorEdbeeDocument->text());
        // workaround for crash if undo is used, see https://github.com/edbee/edbee-lib/issues/66
        controller->beginUndoGroup();
        mpSourceEditorEdbeeDocument->setText(formattedText);
        // don't coalesce the format text action - not that it matters for us since we we only change
        // the text once during the undo group
        controller->endUndoGroup(edbee::CoalesceId::CoalesceId_None, false);
    });

    menu->addAction(formatAction);
    menu->exec(QCursor::pos());

    delete menu;
}

QString dlgTriggerEditor::generateButtonStyleSheet(const QColor& color, const bool isEnabled)
{
    if (color.isValid()) {
        if (isEnabled) {
            return QStringLiteral("QPushButton {color: %1; background-color: %2; }")
                    .arg(color.lightness() > 127 ? QLatin1String("black") : QLatin1String("white"),
                         color.name());
        }

        QColor disabledColor = QColor::fromHsl(color.hslHue(), color.hslSaturation()/4, color.lightness());
        return QStringLiteral("QPushButton {color: %1; background-color: %2; }")
                .arg(QLatin1String("darkGray"), disabledColor.name());
    }
    return QString();
}

// Retrive the background-color or color setting from the previous method, the
// colors used can theoretically be:
// * any strings of those from http://www.w3.org/TR/SVG/types.html#ColorKeywords
// * #RGB (each of R, G, and B is a single hex digit) 3 Digits
// * #RRGGBB 6 Digits
// * #AARRGGBB (Since 5.2) 8 Digits
// * #RRRGGGBBB 9 Digits
// * #RRRRGGGGBBBB 12 Digits
// * "transparent"
QColor dlgTriggerEditor::parseButtonStyleSheetColors(const QString& styleSheetText, const bool isToGetForeground)
{
    if (styleSheetText.isEmpty()) {
        return QColor();
    }

    QRegularExpression hexColorRegex;
    QRegularExpression namedColorRegex;
    if (isToGetForeground) {
        hexColorRegex.setPattern(QLatin1String("(?:[{ ])color:\\s*(?:#)([[:xdigit:]]{3,12})\\s*;")); // Capture group 1 is a foreground color made of hex digits
        QRegularExpressionMatch match = hexColorRegex.match(styleSheetText);
        if (match.hasMatch()) {
            switch (match.capturedLength(1)) {
            case 3: // RGB
                [[fallthrough]];
            case 6: // RRGGBB
                [[fallthrough]];
            case 9: // RRRGGGBBB
                [[fallthrough]];
            case 12: // RRRRGGGGBBBB
                return QColor(match.captured(1).prepend(QLatin1Char('#')));

            default:
            // case 8: // AARRGGBB - Invalid here
                qDebug().noquote().nospace() << "dlgTriggerEditor::parseButtonStyleSheetColors(\"" << styleSheetText << "\", " << isToGetForeground << ") ERROR - Invalid hex string as foreground color!";
                return QColor();
            }
        } else {
            namedColorRegex.setPattern(QLatin1String("(?:[{ ])color:\\s*(\\w{3,})\\s*;")); // Capture group 1 is a word for a foreground color
            match = namedColorRegex.match(styleSheetText);
            if (match.hasMatch()) {
                if (QColor::isValidColor(match.captured(1))) {
                    return QColor(match.captured(1));
                } else {
                    qDebug().noquote().nospace() << "dlgTriggerEditor::parseButtonStyleSheetColors(\"" << styleSheetText << "\", " << isToGetForeground << ") ERROR - Invaid string \"" <<  match.captured(1) << "\" found as name of foreground color!";
                    return QColor();
                }
            } else {
                qDebug().noquote().nospace() << "dlgTriggerEditor::parseButtonStyleSheetColors(\"" << styleSheetText << "\", " << isToGetForeground << ") ERROR - No string as name of foreground color found!";
                return QColor();
            }
        }
    } else {
        hexColorRegex.setPattern(QLatin1String("(?:[{ ])background-color:\\s*(?:#)([[:xdigit:]]{3,12})\\s*;")); // Capture group 1 is a background color made of hex digits
        QRegularExpressionMatch match = hexColorRegex.match(styleSheetText);
        if (match.hasMatch()) {
            switch (match.capturedLength(1)) {
            case 3: // RGB
                [[fallthrough]];
            case 6: // RRGGBB
                [[fallthrough]];
            case 9: // RRRGGGBBB
                [[fallthrough]];
            case 12: // RRRRGGGGBBBB
                return QColor(match.captured(1).prepend(QLatin1Char('#')));

            default:
            // case 8: // AARRGGBB - Invalid here
                qDebug().noquote().nospace() << "dlgTriggerEditor::parseButtonStyleSheetColors(\"" << styleSheetText << "\", " << isToGetForeground << ") ERROR - Invalid hex string as background color!";
                return QColor();
            }
        } else {
            namedColorRegex.setPattern(QLatin1String("(?:[{ ])background-color:\\s*(\\w{3,})\\s*;")); // Capture group 1 is a word for a background color
            match = namedColorRegex.match(styleSheetText);
            if (match.hasMatch()) {
                if (QColor::isValidColor(match.captured(1))) {
                    return QColor(match.captured(1));
                } else {
                    qDebug().noquote().nospace() << "dlgTriggerEditor::parseButtonStyleSheetColors(\"" << styleSheetText << "\", " << isToGetForeground << ") ERROR - Invaid string \"" <<  match.captured(1) << "\" found as name of background color!";
                    return QColor();
                }
            } else {
                qDebug().noquote().nospace() << "dlgTriggerEditor::parseButtonStyleSheetColors(\"" << styleSheetText << "\", " << isToGetForeground << ") ERROR - No string as name of background color found!";
                return QColor();
            }
        }
    }
}

void dlgTriggerEditor::slot_toggleGroupBoxColorizeTrigger(const bool state)
{
    if (mpTriggersMainArea->groupBox_triggerColorizer->isChecked() != state) {
        mpTriggersMainArea->groupBox_triggerColorizer->setChecked(state);
    }

    if (state) {
        // Enabled so make buttons have full colour:
        mpTriggersMainArea->pushButtonFgColor->setStyleSheet(generateButtonStyleSheet(mpTriggersMainArea->pushButtonFgColor->property(cButtonBaseColor).toString(), true));
        mpTriggersMainArea->pushButtonBgColor->setStyleSheet(generateButtonStyleSheet(mpTriggersMainArea->pushButtonBgColor->property(cButtonBaseColor).toString(), true));
    } else {
        // Disabled so make buttons greyed out a bit:
        mpTriggersMainArea->pushButtonFgColor->setStyleSheet(generateButtonStyleSheet(mpTriggersMainArea->pushButtonFgColor->property(cButtonBaseColor).toString(), false));
        mpTriggersMainArea->pushButtonBgColor->setStyleSheet(generateButtonStyleSheet(mpTriggersMainArea->pushButtonBgColor->property(cButtonBaseColor).toString(), false));
    }
}

void dlgTriggerEditor::slot_clearSoundFile()
{
    mpTriggersMainArea->lineEdit_soundFile->clear();
    mpTriggersMainArea->toolButton_clearSoundFile->setEnabled(false);
    mpTriggersMainArea->lineEdit_soundFile->setToolTip(tr("<p>Sound file to play when the trigger fires.</p>"));
}

void dlgTriggerEditor::slot_showAllTriggerControls(const bool isShown)
{
    if (mpTriggersMainArea->toolButton_toggleExtraControls->isChecked() != isShown) {
        mpTriggersMainArea->toolButton_toggleExtraControls->setChecked(isShown);
    }

    if (mpTriggersMainArea->widget_bottom->isVisible() != isShown) {
        mpTriggersMainArea->widget_bottom->setVisible(isShown);
    }

    if (mpTriggersMainArea->widget_right->isVisible() != isShown) {
        mpTriggersMainArea->widget_right->setVisible(isShown);
    }
}

void dlgTriggerEditor::slot_rightSplitterMoved(const int, const int)
{
    /*
     * With all widgets shown:              With some hidden:
     *  +--------------------------------+   +--------------------------------+
     *  | name / control toggle /command |   | name / control toggle /command |
     *--+----------------------+---------+ --+----------------------+---------+
     *  |+--------------------+|         |   |+------------------------------+|
     *w_||                    ||         |   ||                              ||
     *il||    scroll area     || widget  |   ||         scroll area          ||
     *de||                    || _right  |   ||                              ||
     *gf|+--------------------+|         |   |+------------------------------+|
     *et|+--------------------+|         | --+--------------------------------+
     *t ||   widget_bottom    ||         |
     *=>|+--------------------+|         |
     *--+----------------------+---------+
     */
    const int hysteresis = 10;
    static int bottomWidgetHeight = 0;
    if (mpTriggersMainArea->isVisible()) {
        mTriggerEditorSplitterState = splitter_right->saveState();
        // The triggersMainArea is visible
        if (mpTriggersMainArea->toolButton_toggleExtraControls->isChecked()) {
            // The extra controls are visible in the triggersMainArea
            bottomWidgetHeight = mpTriggersMainArea->widget_bottom->height();
            if ((mpTriggersMainArea->widget_left->height()) <= (mpTriggersMainArea->widget_right->minimumSizeHint().height() + hysteresis)
                || mpTriggersMainArea->widget_verticalSpacer_right->height() == 0) {
                // And it is not tall enough to show the right hand side - so
                // hide them:
                slot_showAllTriggerControls(false);
            }

        } else {
            // And the extra controls are NOT visible
            if ((mpTriggersMainArea->widget_left->height() - bottomWidgetHeight) > mpTriggersMainArea->widget_right->minimumSizeHint().height() - hysteresis) {
                // And it is tall enough to show the right hand side completely
                // so show them:
                slot_showAllTriggerControls(true);
            }
        }
    } else if (mpActionsMainArea->isVisible()) {
        mActionEditorSplitterState = splitter_right->saveState();
    } else if (mpAliasMainArea->isVisible()) {
        mAliasEditorSplitterState = splitter_right->saveState();
    } else if (mpKeysMainArea->isVisible()) {
        mKeyEditorSplitterState = splitter_right->saveState();
    } else if (mpScriptsMainArea->isVisible()) {
        mScriptEditorSplitterState = splitter_right->saveState();
    } else if (mpTimersMainArea->isVisible()) {
        mTimerEditorSplitterState = splitter_right->saveState();
    } else if (mpVarsMainArea->isVisible()) {
        mVarEditorSplitterState = splitter_right->saveState();
    }
    if (mpSourceEditorFindArea->isVisible()) {
        slot_move_source_find();
    }
}

// Only for other classes to set the options - as they will not be carried from
// here to the parent Host instance, whereas the slots that change the
// individual options DO also notify that Host instance about the changes they
// make:
void dlgTriggerEditor::setSearchOptions(const SearchOptions optionsState)
{
    mSearchOptions = optionsState;
    mpAction_searchCaseSensitive->setChecked(optionsState & SearchOptionCaseSensitive);
    mpAction_searchIncludeVariables->setChecked(optionsState & SearchOptionIncludeVariables);
    createSearchOptionIcon();
}
