/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2024 by Stephen Lyons - slysven@virginmedia.com    *
 *   Copyright (C) 2016 by Owen Davison - odavison@cs.dal.ca               *
 *   Copyright (C) 2016-2020 by Ian Adkins - ieadkins@gmail.com            *
 *   Copyright (C) 2017 by Tom Scheper - scheper@gmail.com                 *
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


#include "dlgTriggerEditor.h"

#include "Host.h"
#include "LuaInterface.h"
#include "TConsole.h"
#include "TDebug.h"
#include "TEasyButtonBar.h"
#include "TTextEdit.h"
#include "TToolBar.h"
#include "TrailingWhitespaceMarker.h"
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
#include <QToolBar>
#include <QColorDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QScrollBar>
#include <QShortcut>

#include "post_guard.h"

using namespace std::chrono_literals;

// Used as a QObject::property so that we can keep track of the color for the
// trigger colorizer buttons loaded from a trigger even if the user disables
// and then reenables the colorizer function (and we "grey out" the color while
// it is disabled):
static const char* cButtonBaseColor = "baseColor";

dlgTriggerEditor::dlgTriggerEditor(Host* pH)
: mpHost(pH)
, mSearchOptions(pH->mSearchOptions)
{
    // init generated dialog
    setupUi(this);
    undoStack = new QUndoStack(this);
    createUndoView();

    msgInfoAddAlias = tr(
            "<p>Alias react on user input. To add a new alias:"
            "<ol><li>Click on the 'Add Item' icon above.</li>"
            "<li>Define an input <strong>pattern</strong> either literally or with a Perl regular expression.</li>"
            "<li>Define a 'substitution' <strong>command</strong> to send to the game in clear text <strong>instead of the alias pattern</strong>, or write a script for more complicated needs.</li>"
            "<li><strong>Activate</strong> the alias.</li></ol></p>"
            "<p>That's it! If you'd like to be able to create aliases from the input line, there are a <a href='https://forums.mudlet.org/viewtopic.php?f=6&t=22609'>couple</a> of <a "
            "href='https://forums.mudlet.org/viewtopic.php?f=6&t=16462'>packages</a> that can help you."
            "<p>Check the manual for <a href='http://wiki.mudlet.org/w/Manual:Introduction#Aliases'>more information</a>.</p>");

    msgInfoAddTrigger = tr("<p>Triggers react on game output. To add a new trigger:"
                           "<ol><li>Click on the 'Add Item' icon above.</li>"
                           "<li>Define a <strong>pattern</strong> that you want to trigger on.</li>"
                           "<li>Select the appropriate pattern <strong>type</strong>.</li>"
                           "<li>Define a clear text <strong>command</strong> that you want to send to the game if the trigger finds the pattern in the text from the game, or write a script for more "
                           "complicated needs..</li>"
                           "<li><strong>Activate</strong> the trigger.</li></ol></p>"
                           "<p>That's it! If you'd like to be able to create triggers from the input line, there are a <a href='https://forums.mudlet.org/viewtopic.php?f=6&t=22609'>couple</a> of <a "
                           "href='https://forums.mudlet.org/viewtopic.php?f=6&t=16462'>packages</a> that can help you."
                           "<p>Check the manual for <a href='http://wiki.mudlet.org/w/Manual:Introduction#Triggers'>more information</a>.</p>");

    msgInfoAddScript = tr("<p>Scripts organize code and can react to events. To add a new script:"
                          "<ol><li>Click on the 'Add Item' icon above.</li>"
                          "<li>Enter a script in the box below. You can for example define <strong>functions</strong> to be called by other triggers, aliases, etc.</li>"
                          "<li>If you write lua <strong>commands</strong> without defining a function, they will be run on Mudlet startup and each time you open the script for editing.</li>"
                          "<li>If needed, you can register a list of <strong>events</strong> with the + and - symbols. If one of these events take place, the function with the same name as the "
                          "script item itself will be called.</li>"
                          "<li><strong>Activate</strong> the script.</li></ol></p>"
                          "<p><strong>Note:</strong> Scripts are run automatically when viewed, even if they are deactivated.</p>"
                          "<p><strong>Note:</strong> Events can also be added to a script from the command line in the main profile window like this:</p>"
                          "<p><code>lua registerAnonymousEventHandler(&quot;nameOfTheMudletEvent&quot;, &quot;nameOfYourFunctionToBeCalled&quot;)</code></p>"
                          "<p>Check the manual for <a href='http://wiki.mudlet.org/w/Manual:Introduction#Scripts'>more information</a>.</p>");

    msgInfoAddTimer = tr("<p>Timers react after a timespan once or regularly. To add a new timer:"
                         "<ol><li>Click on the 'Add Item' icon above.</li>"
                         "<li>Define the <strong>timespan</strong> after which the timer should react in a this format: hours : minutes : seconds.</li>"
                         "<li>Define a clear text <strong>command</strong> that you want to send to the game when the time has passed, or write a script for more complicated needs.</li>"
                         "<li><strong>Activate</strong> the timer.</li></ol></p>"
                         "<p><strong>Note:</strong> If you want the trigger to react only once and not regularly, use the Lua tempTimer() function instead.</p>"
                         "<p><strong>Note:</strong> Timers can also be defined from the command line in the main profile window like this:</p>"
                         "<p><code>lua tempTimer(3, function() echo(&quot;hello!\n&quot;) end)</code></p>"
                         "<p>This will greet you exactly 3 seconds after it was made.</p>"
                         "<p>Check the manual for <a href='http://wiki.mudlet.org/w/Manual:Introduction#Timers'>more information</a>.</p>");

    msgInfoAddButton = tr(
            "<p>Buttons react on mouse clicks. To add a new button:"
            "<ol><li>Add a new group to define a new <strong>button bar</strong> in case you don't have any.</li>"
            "<li>Add new groups as <strong>menus</strong> to a button bar or sub-menus to menus.<li>"
            "<li>Add new items as <strong>buttons</strong> to a button bar or menu or sub-menu.</li>"
            "<li>Define a clear text <strong>command</strong> that you want to send to the game if the button is pressed, or write a script for more complicated needs.</li>"
            "<li><strong>Activate</strong> the toolbar, menu or button. </li></ol>"
            "<p><strong>Note:</strong> Deactivated items will be hidden and if they are toolbars or menus then all the items they contain will be also be hidden.</p>"
            "<p><strong>Note:</strong> If a button is made a <strong>click-down</strong> button then you may also define a clear text command that you want to send to the game when the button is "
            "pressed a second time to uncheck it or to write a script to run when it happens - within such a script the Lua 'getButtonState()' function reports whether the button is up or down.</p>"
            "<p>Check the manual for <a href='http://wiki.mudlet.org/w/Manual:Introduction#Buttons'>more information</a>.</p>");

    msgInfoAddKey = tr("<p>Keys react on keyboard presses. To add a new key binding:"
                       "<ol><li>Click on the 'Add Item' icon above.</li>"
                       "<li>Click on <strong>'grab key'</strong> and then press your key combination, e.g. including modifier keys like Control, Shift, etc.</li>"
                       "<li>Define a clear text <strong>command</strong> that you want to send to the game if the button is pressed, or write a script for more complicated needs.</li>"
                       "<li><strong>Activate</strong> the new key binding.</li></ol></p>"
                       "<p><strong>Note:</strong> Keys can also be defined from the command line in the main profile window like this:</p>"
                       "<p><code>lua permKey(&quot;my jump key&quot;, &quot;&quot;, mudlet.key.F8, [[send(&quot;jump&quot;]]) end)</code></p>"
                       "<p>Pressing F8 will make you jump.</p>"
                       "<p>Check the manual for <a href='http://wiki.mudlet.org/w/Manual:Introduction#Keybindings'>more information</a>.</p>");

    msgInfoAddVar = tr("<p>Variables store information. To make a new variable:"
                       "<ol><li>Click on the 'Add Item' icon above. To add a table instead click 'Add Group'.</li>"
                       "<li>Select type of variable value (can be a string, integer, boolean)</li>"
                       "<li>Enter the value you want to store in this variable.</li>"
                       "<li>If you want to keep the variable in your next Mudlet sessions, check the checkbox in the list of variables to the left.</li>"
                       "<li>To remove a variable manually, set it to 'nil' or click on the 'Delete' icon above.</li></ol></p>"
                       "<p><strong>Note:</strong> Variables created here won't be saved when Mudlet shuts down unless you check their checkbox in the list of variables to the left. You could also "
                       "create scripts with the variables instead.</p>"
                       "<p><strong>Note:</strong> Variables and tables can also be defined from the command line in the main profile window like this:</p>"
                       "<p><code>lua foo = &quot;bar&quot;</code></p>"
                       "<p>This will create a string called 'foo' with 'bar' as its value.</p>"
                       "<p>Check the manual for <a href='http://wiki.mudlet.org/w/Manual:Introduction#Variables'>more information</a>.</p>");

    // Descriptions for screen readers, clarify to translators that the context of "activated" is current status and not confirmation of toggle.
    //: Item is currently on, short enough to be spoken
    descActive = tr("activated");
    //: Item is currently off, short enough to be spoken
    descInactive = tr("deactivated");
    //: Folder is currently turned on
    descActiveFolder = tr("activated folder");
    //: Folder is currently turned off
    descInactiveFolder = tr("deactivated folder");
    //: Item is currently inactive because of errors, short enough to be spoken
    descError = tr("deactivated due to error");
    //: Item is currently turned on individually, but is member of an inactive group
    descInactiveParent = tr("%1 in a deactivated group");
    //: A trigger that unlocks other triggers is currently turned on, short enough to be spoken
    descActiveFilterChain = tr("activated filter chain");
    //: A trigger that unlocks other triggers is currently turned off, short enough to be spoken
    descInactiveFilterChain = tr("deactivated filter chain");
    //: A timer that starts after another timer is currently turned on
    descActiveOffsetTimer = tr("activated offset timer");
    //: A timer that starts after another timer is currently turned off
    descInactiveOffsetTimer = tr("deactivated offset timer");
    descNewFolder = tr("new folder");
    descNewItem = tr("new item");

    setUnifiedTitleAndToolBarOnMac(true); //MAC OSX: make window moveable
    const QString hostName{mpHost->getName()};
    setWindowTitle(tr("%1 - Editor").arg(hostName));
    setWindowIcon(QIcon(qsl(":/icons/mudlet_editor.png")));
    auto statusBar = new QStatusBar(this);
    statusBar->setSizeGripEnabled(true);
    setStatusBar(statusBar);
    statusBar->show();

    mpNonCodeWidgets = new QWidget(this);
    auto* layoutColumn = new QVBoxLayout(mpNonCodeWidgets);
    splitter_right->addWidget(mpNonCodeWidgets);

    // system message area
    mpSystemMessageArea = new dlgSystemMessageArea(this);
    mpSystemMessageArea->setObjectName(qsl("mpSystemMessageArea"));
    mpSystemMessageArea->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
    // set the stretch factor of the message area to 0 and everything else to 1,
    // so our errors box doesn't stretch to produce a grey area
    layoutColumn->addWidget(mpSystemMessageArea, 0);
    connect(mpSystemMessageArea->messageAreaCloseButton, &QAbstractButton::clicked, this, &dlgTriggerEditor::hideSystemMessageArea);

    // main areas
    mpTriggersMainArea = new dlgTriggersMainArea(this);
    layoutColumn->addWidget(mpTriggersMainArea, 1);
    connect(mpTriggersMainArea->pushButtonFgColor, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_colorizeTriggerSetFgColor);
    connect(mpTriggersMainArea->pushButtonBgColor, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_colorizeTriggerSetBgColor);
    connect(mpTriggersMainArea->pushButtonSound, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_soundTrigger);
    connect(mpTriggersMainArea->groupBox_triggerColorizer, &QGroupBox::clicked, this, &dlgTriggerEditor::slot_toggleGroupBoxColorizeTrigger);
    connect(mpTriggersMainArea->toolButton_clearSoundFile, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_clearSoundFile);
    connect(mpTriggersMainArea->lineEdit_trigger_name, &QLineEdit::editingFinished, this, &dlgTriggerEditor::slot_lineEditTriggerNameTextEdited);
    connect(mpTriggersMainArea->lineEdit_trigger_command, &QLineEdit::editingFinished, this, &dlgTriggerEditor::slot_lineEditTriggerCommandTextEdited);
    connect(mpTriggersMainArea->spinBox_stayOpen, &QSpinBox::valueChanged, this, &dlgTriggerEditor::slot_triggerFireLengthEdited);
    connect(mpTriggersMainArea->groupBox_soundTrigger, &QGroupBox::toggled, this, &dlgTriggerEditor::slot_triggerPlaySoundEdited);
    connect(mpTriggersMainArea->lineEdit_soundFile, &QLineEdit::textChanged, this, &dlgTriggerEditor::slot_triggerPlaySoundFileEdited);
    connect(mpTriggersMainArea->groupBox_triggerColorizer, &QGroupBox::toggled, this, &dlgTriggerEditor::slot_triggerColorizerEdited);
    connect(mpTriggersMainArea->groupBox_perlSlashGOption, &QGroupBox::toggled, this, &dlgTriggerEditor::slot_triggerPerlSlashGOptionEdited);
    connect(mpTriggersMainArea->groupBox_filterTrigger, &QGroupBox::toggled, this, &dlgTriggerEditor::slot_triggerGroupFilterEdited);
    connect(mpTriggersMainArea->groupBox_multiLineTrigger, &QGroupBox::toggled, this, &dlgTriggerEditor::slot_triggerMultiLineEdited);
    connect(mpTriggersMainArea->spinBox_lineMargin, &QSpinBox::valueChanged, this, &dlgTriggerEditor::slot_triggerLineMarginEdited);

    mpTimersMainArea = new dlgTimersMainArea(this);
    layoutColumn->addWidget(mpTimersMainArea, 1);

    mpAliasMainArea = new dlgAliasMainArea(this);
    layoutColumn->addWidget(mpAliasMainArea, 1);

    mpActionsMainArea = new dlgActionMainArea(this);
    layoutColumn->addWidget(mpActionsMainArea, 1);
    connect(mpActionsMainArea->checkBox_action_button_isPushDown, &QCheckBox::stateChanged, this, &dlgTriggerEditor::slot_toggleIsPushDownButton);

    mpKeysMainArea = new dlgKeysMainArea(this);
    layoutColumn->addWidget(mpKeysMainArea, 1);
    connect(mpKeysMainArea->pushButton_key_grabKey, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_keyGrab);

    mpVarsMainArea = new dlgVarsMainArea(this);
    layoutColumn->addWidget(mpVarsMainArea, 1);

    mpScriptsMainArea = new dlgScriptsMainArea(this);
    layoutColumn->addWidget(mpScriptsMainArea, 1);

    connect(mpScriptsMainArea->lineEdit_script_event_handler_entry, &QLineEdit::returnPressed, this, &dlgTriggerEditor::slot_scriptMainAreaAddHandler);
    connect(mpScriptsMainArea->listWidget_script_registered_event_handlers, &QListWidget::itemClicked, this, &dlgTriggerEditor::slot_scriptMainAreaEditHandler);

    // source editor area
    mpSourceEditorArea = new dlgSourceEditorArea(this);
    splitter_right->addWidget(mpSourceEditorArea);

    // And the edbee widget
    mpSourceEditorEdbee = mpSourceEditorArea->edbeeEditorWidget;
    mpSourceEditorEdbee->setAutoScrollMargin(20);
    mpSourceEditorEdbee->setPlaceholderText(tr("-- add your Lua code here"));
    mpSourceEditorEdbeeDocument = mpSourceEditorEdbee->textDocument();

    // Update the status bar on changes
    connect(mpSourceEditorEdbee->controller(), &edbee::TextEditorController::updateStatusTextSignal, this, &dlgTriggerEditor::slot_updateStatusBar);
    mpSourceEditorEdbee->controller()->setAutoScrollToCaret(edbee::TextEditorController::AutoScrollWhenFocus);

    // Update the editor preferences
    connect(mudlet::self(), &mudlet::signal_editorTextOptionsChanged, this, &dlgTriggerEditor::slot_changeEditorTextOptions);
    mpSourceEditorEdbeeDocument->setText(qsl("%1\n").arg(tr("-- Enter your lua code here")));

    mudlet::loadEdbeeTheme(mpHost->mEditorTheme, mpHost->mEditorThemeFile);

    // edbee editor find area
    mpSourceEditorFindArea = new dlgSourceEditorFindArea(mpSourceEditorEdbee);
    mpSourceEditorEdbee->horizontalScrollBar()->installEventFilter(mpSourceEditorFindArea);
    mpSourceEditorEdbee->verticalScrollBar()->installEventFilter(mpSourceEditorFindArea);
    mpSourceEditorFindArea->hide();

    connect(mpSourceEditorFindArea->lineEdit_findText, &QLineEdit::textChanged, this, &dlgTriggerEditor::slot_sourceFindTextChanges);
    connect(mpSourceEditorFindArea, &dlgSourceEditorFindArea::signal_sourceEditorMovementNecessary, this, &dlgTriggerEditor::slot_sourceFindMove);
    connect(mpSourceEditorFindArea->pushButton_findPrevious, &QPushButton::clicked, this, &dlgTriggerEditor::slot_sourceFindPrevious);
    connect(mpSourceEditorFindArea->pushButton_findNext, &QPushButton::clicked, this, &dlgTriggerEditor::slot_sourceFindNext);
    connect(mpSourceEditorFindArea->pushButton_replace, &QPushButton::clicked, this, &dlgTriggerEditor::slot_sourceReplace);
    connect(mpSourceEditorFindArea, &dlgSourceEditorFindArea::signal_sourceEditorFindPrevious, this, &dlgTriggerEditor::slot_sourceFindPrevious);
    connect(mpSourceEditorFindArea, &dlgSourceEditorFindArea::signal_sourceEditorFindNext, this, &dlgTriggerEditor::slot_sourceFindNext);
    connect(mpSourceEditorFindArea, &dlgSourceEditorFindArea::signal_sourceEditorReplace, this, &dlgTriggerEditor::slot_sourceReplace);
    connect(mpSourceEditorFindArea->pushButton_close, &QPushButton::clicked, this, &dlgTriggerEditor::slot_closeSourceFind);

    auto openSourceFindAction = new QAction(this);
    openSourceFindAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    openSourceFindAction->setShortcut(QKeySequence(QKeySequence::Find));
    mpSourceEditorArea->addAction(openSourceFindAction);
    connect(openSourceFindAction, &QAction::triggered, this, &dlgTriggerEditor::slot_openSourceFind);

    QAction* closeSourceFindAction = new QAction(this);
    closeSourceFindAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    closeSourceFindAction->setShortcut(QKeySequence(QKeySequence::Cancel));
    mpSourceEditorArea->addAction(closeSourceFindAction);
    connect(closeSourceFindAction, &QAction::triggered, this, &dlgTriggerEditor::slot_closeSourceFind);

    QAction* sourceFindNextAction = new QAction(this);
    sourceFindNextAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    sourceFindNextAction->setShortcut(QKeySequence(QKeySequence::FindNext));
    mpSourceEditorArea->addAction(sourceFindNextAction);
    connect(sourceFindNextAction, &QAction::triggered, this, &dlgTriggerEditor::slot_sourceFindNext);

    QAction* sourceFindPreviousAction = new QAction(this);
    sourceFindPreviousAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    sourceFindPreviousAction->setShortcut(QKeySequence(QKeySequence::FindPrevious));
    mpSourceEditorArea->addAction(sourceFindPreviousAction);
    connect(sourceFindPreviousAction, &QAction::triggered, this, &dlgTriggerEditor::slot_sourceFindPrevious);

    auto* provider = new edbee::StringTextAutoCompleteProvider();
    //QScopedPointer<edbee::StringTextAutoCompleteProvider> provider(new edbee::StringTextAutoCompleteProvider);

    // Add lua functions and reserved lua terms to an AutoComplete provider
    for (const QString& key : mudlet::smLuaFunctionNames.keys()) {
        provider->add(key, 3, mudlet::smLuaFunctionNames.value(key).toString());
    }

    provider->add(qsl("and"), 14);
    provider->add(qsl("break"), 14);
    provider->add(qsl("else"), 14);
    provider->add(qsl("elseif"), 14);
    provider->add(qsl("end"), 14);
    provider->add(qsl("false"), 14);
    provider->add(qsl("for"), 14);
    provider->add(qsl("function"), 14);
    provider->add(qsl("goto"), 14);
    provider->add(qsl("local"), 14);
    provider->add(qsl("nil"), 14);
    provider->add(qsl("not"), 14);
    provider->add(qsl("repeat"), 14);
    provider->add(qsl("return"), 14);
    provider->add(qsl("then"), 14);
    provider->add(qsl("true"), 14);
    provider->add(qsl("until"), 14);
    provider->add(qsl("while"), 14);

    // Set the newly filled provider to be used by our Edbee instance
    edbee::Edbee::instance()->autoCompleteProviderList()->setParentProvider(provider);

    mpSourceEditorEdbee->textEditorComponent()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(mpSourceEditorEdbee->textEditorComponent(), &QWidget::customContextMenuRequested, this, &dlgTriggerEditor::slot_editorContextMenu);

    // option areas
    mpErrorConsole = new TConsole(mpHost, qsl("errors_%1").arg(hostName), TConsole::ErrorConsole, this);
    mpErrorConsole->setWrapAt(100);
    mpErrorConsole->mUpperPane->slot_toggleTimeStamps(true);
    mpErrorConsole->mLowerPane->slot_toggleTimeStamps(true);
    mpErrorConsole->print(qsl("%1\n").arg(tr("*** starting new session ***")));
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

    button_toggleSearchAreaResults->setStyleSheet(qsl("QToolButton::on {border-image: url(:/icons/arrow-down_grey.png);} "
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
    connect(treeWidget_triggers, &QTreeWidget::itemClicked, this, &dlgTriggerEditor::slot_saveSelectedItem);


    treeWidget_aliases->hide();
    treeWidget_aliases->setHost(mpHost);
    treeWidget_aliases->setIsAliasTree();
    treeWidget_aliases->setColumnCount(1);
    treeWidget_aliases->header()->hide();
    treeWidget_aliases->setRootIsDecorated(false);
    treeWidget_aliases->setContextMenuPolicy(Qt::ActionsContextMenu);
    connect(treeWidget_aliases, &QTreeWidget::itemClicked, this, &dlgTriggerEditor::slot_saveSelectedItem);

    treeWidget_actions->hide();
    treeWidget_actions->setHost(mpHost);
    treeWidget_actions->setIsActionTree();
    treeWidget_actions->setColumnCount(1);
    treeWidget_actions->header()->hide();
    treeWidget_actions->setRootIsDecorated(false);
    treeWidget_actions->setContextMenuPolicy(Qt::ActionsContextMenu);
    connect(treeWidget_actions, &QTreeWidget::itemClicked, this, &dlgTriggerEditor::slot_saveSelectedItem);

    treeWidget_timers->hide();
    treeWidget_timers->setHost(mpHost);
    treeWidget_timers->setIsTimerTree();
    treeWidget_timers->setColumnCount(1);
    treeWidget_timers->header()->hide();
    treeWidget_timers->setRootIsDecorated(false);
    treeWidget_timers->setContextMenuPolicy(Qt::ActionsContextMenu);
    connect(treeWidget_timers, &QTreeWidget::itemClicked, this, &dlgTriggerEditor::slot_saveSelectedItem);

    treeWidget_variables->hide();
    treeWidget_variables->setHost(mpHost);
    treeWidget_variables->setIsVarTree();
    treeWidget_variables->setColumnCount(2);
    treeWidget_variables->hideColumn(1);
    treeWidget_variables->header()->hide();
    treeWidget_variables->setRootIsDecorated(false);
    treeWidget_variables->setContextMenuPolicy(Qt::ActionsContextMenu);
    connect(treeWidget_variables, &QTreeWidget::itemClicked, this, &dlgTriggerEditor::slot_saveSelectedItem);

    treeWidget_keys->hide();
    treeWidget_keys->setHost(mpHost);
    treeWidget_keys->setIsKeyTree();
    treeWidget_keys->setColumnCount(1);
    treeWidget_keys->header()->hide();
    treeWidget_keys->setRootIsDecorated(false);
    treeWidget_keys->setContextMenuPolicy(Qt::ActionsContextMenu);
    connect(treeWidget_keys, &QTreeWidget::itemClicked, this, &dlgTriggerEditor::slot_saveSelectedItem);

    treeWidget_scripts->hide();
    treeWidget_scripts->setHost(mpHost);
    treeWidget_scripts->setIsScriptTree();
    treeWidget_scripts->setColumnCount(1);
    treeWidget_scripts->header()->hide();
    treeWidget_scripts->setRootIsDecorated(false);
    treeWidget_scripts->setContextMenuPolicy(Qt::ActionsContextMenu);
    connect(treeWidget_scripts, &QTreeWidget::itemClicked, this, &dlgTriggerEditor::slot_saveSelectedItem);

    QAction* viewTriggerAction = new QAction(QIcon(qsl(":/icons/tools-wizard.png")), tr("Triggers"), this);
    viewTriggerAction->setStatusTip(tr("Show Triggers"));
    viewTriggerAction->setToolTip(qsl("%1 (%2)").arg(tr("Show Triggers"), tr("Ctrl+1")));
    connect(viewTriggerAction, &QAction::triggered, this, &dlgTriggerEditor::slot_showTriggers);

    QAction* viewAliasAction = new QAction(QIcon(qsl(":/icons/system-users.png")), tr("Aliases"), this);
    viewAliasAction->setStatusTip(tr("Show Aliases"));
    viewAliasAction->setToolTip(qsl("%1 (%2)").arg(tr("Show Aliases"), tr("Ctrl+2")));
    connect(viewAliasAction, &QAction::triggered, this, &dlgTriggerEditor::slot_showAliases);

    QAction* viewScriptsAction = new QAction(QIcon(qsl(":/icons/document-properties.png")), tr("Scripts"), this);
    viewScriptsAction->setStatusTip(tr("Show Scripts"));
    viewScriptsAction->setToolTip(qsl("%1 (%2)").arg(tr("Show Scripts"), tr("Ctrl+3")));
    connect(viewScriptsAction, &QAction::triggered, this, &dlgTriggerEditor::slot_showScripts);

    QAction* showTimersAction = new QAction(QIcon(qsl(":/icons/chronometer.png")), tr("Timers"), this);
    showTimersAction->setStatusTip(tr("Show Timers"));
    showTimersAction->setToolTip(qsl("%1 (%2)").arg(tr("Show Timers"), tr("Ctrl+4")));
    connect(showTimersAction, &QAction::triggered, this, &dlgTriggerEditor::slot_showTimers);

    QAction* viewKeysAction = new QAction(QIcon(qsl(":/icons/preferences-desktop-keyboard.png")), tr("Keys"), this);
    viewKeysAction->setStatusTip(tr("Show Keybindings"));
    viewKeysAction->setToolTip(qsl("%1 (%2)").arg(tr("Show Keybindings"), tr("Ctrl+5")));
    connect(viewKeysAction, &QAction::triggered, this, &dlgTriggerEditor::slot_showKeys);

    QAction* viewVarsAction = new QAction(QIcon(qsl(":/icons/variables.png")), tr("Variables"), this);
    viewVarsAction->setStatusTip(tr("Show Variables"));
    viewVarsAction->setToolTip(qsl("%1 (%2)").arg(tr("Show Variables"), tr("Ctrl+6")));
    connect(viewVarsAction, &QAction::triggered, this, &dlgTriggerEditor::slot_showVariables);

    QAction* viewActionAction = new QAction(QIcon(qsl(":/icons/bookmarks.png")), tr("Buttons"), this);
    viewActionAction->setStatusTip(tr("Show Buttons"));
    viewActionAction->setToolTip(qsl("%1 (%2)").arg(tr("Show Buttons"), tr("Ctrl+7")));
    connect(viewActionAction, &QAction::triggered, this, &dlgTriggerEditor::slot_showActions);


    QAction* viewErrorsAction = new QAction(QIcon(qsl(":/icons/errors.png")), tr("Errors"), this);
    viewErrorsAction->setStatusTip(tr("Show/Hide the errors console in the bottom right of this editor."));
    viewErrorsAction->setToolTip(qsl("%1 (%2)").arg(tr("Show/Hide errors console"), tr("Ctrl+8")));
    connect(viewErrorsAction, &QAction::triggered, this, &dlgTriggerEditor::slot_viewErrorsAction);

    QAction* viewStatsAction = new QAction(QIcon(qsl(":/icons/view-statistics.png")), tr("Statistics"), this);
    viewStatsAction->setStatusTip(tr("Generate a statistics summary display on the main profile console."));
    viewStatsAction->setToolTip(qsl("%1 (%2)").arg(tr("Generate statistics"), tr("Ctrl+9")));
    connect(viewStatsAction, &QAction::triggered, this, &dlgTriggerEditor::slot_viewStatsAction);

    QAction* showDebugAreaAction = new QAction(QIcon(qsl(":/icons/tools-report-bug.png")), tr("Debug"), this);
    showDebugAreaAction->setStatusTip(tr("Show/Hide the separate Central Debug Console - when being displayed the system will be slower."));
    showDebugAreaAction->setToolTip(utils::richText(tr("Show/Hide Debug Console (Ctrl+0) -> system will be <b><i>slower</i></b>.")));
    connect(showDebugAreaAction, &QAction::triggered, this, &dlgTriggerEditor::slot_toggleCentralDebugConsole);


    QAction* toggleActiveAction = new QAction(QIcon(qsl(":/icons/document-encrypt.png")), tr("Activate"), this);
    toggleActiveAction->setStatusTip(tr("Toggle Active or Non-Active Mode for Triggers, Scripts etc."));
    connect(toggleActiveAction, &QAction::triggered, this, &dlgTriggerEditor::slot_toggleItemOrGroupActiveFlag);
    connect(treeWidget_triggers, &QTreeWidget::itemActivated, this, &dlgTriggerEditor::slot_toggleItemOrGroupActiveFlag);
    // connect(treeWidget_triggers, &TTreeWidget::itemDropEvent, this, &dlgTriggerEditor::slot_treeWidget_triggers_drop);
    // connect(treeWidget_triggers, &QTreeWidget::currentItemChanged, this, &dlgTriggerEditor::slot_treeWidget_triggers_drop);
    connect(treeWidget_aliases, &QTreeWidget::itemActivated, this, &dlgTriggerEditor::slot_toggleItemOrGroupActiveFlag);
    connect(treeWidget_timers, &QTreeWidget::itemActivated, this, &dlgTriggerEditor::slot_toggleItemOrGroupActiveFlag);
    connect(treeWidget_scripts, &QTreeWidget::itemActivated, this, &dlgTriggerEditor::slot_toggleItemOrGroupActiveFlag);
    connect(treeWidget_actions, &QTreeWidget::itemActivated, this, &dlgTriggerEditor::slot_toggleItemOrGroupActiveFlag);
    connect(treeWidget_keys, &QTreeWidget::itemActivated, this, &dlgTriggerEditor::slot_toggleItemOrGroupActiveFlag);


    mAddItem = new QAction(QIcon(qsl(":/icons/document-new.png")), QString(), this);
    mAddItem->setToolTip(qsl("<p>%1 (%2)</p>").arg(tr("Add Item"), QKeySequence(QKeySequence::New).toString()));
    mAddItem->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    mAddItem->setShortcut(QKeySequence(QKeySequence::New));
    frame_left->addAction(mAddItem);
    connect(mAddItem, &QAction::triggered, this, &dlgTriggerEditor::slot_addNewItem);

    mDeleteItem = new QAction(QIcon::fromTheme(qsl(":/icons/edit-delete"), QIcon(qsl(":/icons/edit-delete.png"))), QString(), this);
    mDeleteItem->setToolTip(qsl("<p>%1 (%2)</p>").arg(tr("Delete Item"), QKeySequence(QKeySequence::Delete).toString()));
    mDeleteItem->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    mDeleteItem->setShortcut(QKeySequence(QKeySequence::Delete));
    frame_left->addAction(mDeleteItem);
    connect(mDeleteItem, &QAction::triggered, this, &dlgTriggerEditor::slot_deleteItemOrGroup);

    mAddGroup = new QAction(QIcon(qsl(":/icons/folder-new.png")), QString(), this);
    mAddGroup->setToolTip(tr("Add Group (Control+Shift+N)"));
    mAddGroup->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    mAddGroup->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N));
    frame_left->addAction(mAddGroup);
    connect(mAddGroup, &QAction::triggered, this, &dlgTriggerEditor::slot_addNewGroup);

    // 'Save Item' does not see to be translated as it is only ever used programmatically and not visible to the player
    // PLACEMARKER 1/3 save button texts need to be kept in sync
    mSaveItem = new QAction(QIcon(qsl(":/icons/document-save-as.png")), qsl("Save Item"), this);
    mSaveItem->setToolTip(tr("<p>Saves the selected item. (Ctrl+S)</p>"
                             "<p>Saving causes any changes to the item to take effect. It will not save to disk, "
                             "so changes will be lost in case of a computer/program crash (but Save Profile to the right will be secure.)</p>"));
    connect(mSaveItem, &QAction::triggered, this, &dlgTriggerEditor::slot_saveEdits);

    undoAction = undoStack->createUndoAction(this, tr("&Undo"));
    undoAction->setIcon(QIcon(":/icons/undo.png"));

    redoAction = undoStack->createRedoAction(this, tr("&Redo"));
    redoAction->setIcon(QIcon(":/icons/redo.png"));

    QAction* copyAction = new QAction(tr("Copy"), this);
    copyAction->setShortcut(QKeySequence(QKeySequence::Copy));
    // only take effect if the treeview is selected, otherwise it hijacks the shortcut from edbee
    copyAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    copyAction->setToolTip(utils::richText(tr("Copy the trigger/script/alias/etc")));
    copyAction->setStatusTip(tr("Copy the trigger/script/alias/etc"));
    treeWidget_triggers->addAction(copyAction);
    treeWidget_aliases->addAction(copyAction);
    treeWidget_timers->addAction(copyAction);
    treeWidget_scripts->addAction(copyAction);
    treeWidget_actions->addAction(copyAction);
    treeWidget_keys->addAction(copyAction);
    connect(copyAction, &QAction::triggered, this, &dlgTriggerEditor::slot_copyXml);

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
    connect(pasteAction, &QAction::triggered, this, &dlgTriggerEditor::slot_pasteXml);

    if (!qApp->testAttribute(Qt::AA_DontShowIconsInMenus)) {
        copyAction->setIcon(QIcon::fromTheme(qsl("edit-copy"), QIcon(qsl(":/icons/edit-copy.png"))));
        pasteAction->setIcon(QIcon::fromTheme(qsl("edit-paste"), QIcon(qsl(":/icons/edit-paste.png"))));
    }

    QAction* importAction = new QAction(QIcon(qsl(":/icons/import.png")), tr("Import"), this);
    importAction->setEnabled(true);
    connect(importAction, &QAction::triggered, this, &dlgTriggerEditor::slot_import);

    mpExportAction = new QAction(QIcon(qsl(":/icons/export.png")), tr("Export"), this);
    mpExportAction->setEnabled(true);
    connect(mpExportAction, &QAction::triggered, this, &dlgTriggerEditor::slot_export);

    mProfileSaveAction = new QAction(QIcon(qsl(":/icons/document-save-all.png")), tr("Save Profile"), this);
    mProfileSaveAction->setToolTip(tr("<p>Saves your profile. (Ctrl+Shift+S)</p>"
                                      "<p>Saves your entire profile (triggers, aliases, scripts, timers, buttons and "
                                      "keys, but not the map or script-specific settings) to your computer disk, so "
                                      "in case of a computer or program crash, all changes you have done will be "
                                      "retained.</p>"
                                      "<p>It also makes a backup of your profile, you can load an older version of it "
                                      "when connecting.</p>"
                                      "<p>Should there be any modules that are marked to be \"<i>synced</i>\" this will "
                                      "also cause them to be saved and reloaded into other profiles if they too are "
                                      "active.</p>"));
    mProfileSaveAction->setStatusTip(
            tr(R"(Saves your entire profile (triggers, aliases, scripts, timers, buttons and keys, but not the map or script-specific settings); also "synchronizes" modules that are so marked.)"));

    mProfileSaveAsAction = new QAction(QIcon(qsl(":/icons/utilities-file-archiver.png")), tr("Save Profile As"), this);

    if (mpHost->mLoadedOk) {
        connect(mProfileSaveAction, &QAction::triggered, this, &dlgTriggerEditor::slot_profileSaveAction);
        connect(mProfileSaveAsAction, &QAction::triggered, this, &dlgTriggerEditor::slot_profileSaveAsAction);
    } else {
        mProfileSaveAction->setDisabled(true);
        mProfileSaveAsAction->setDisabled(true);
        auto disabledSaving = tr("Something went wrong loading your Mudlet profile and it could not be loaded. "
                                 "Try loading an older version in 'Connect - Options - Profile history'");
        mProfileSaveAction->setToolTip(disabledSaving);
        mProfileSaveAsAction->setToolTip(disabledSaving);
    }

    auto* nextSectionShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Tab), this);
    QObject::connect(nextSectionShortcut, &QShortcut::activated, this, &dlgTriggerEditor::slot_nextSection);

    QShortcut* previousSectionShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Tab), this);
    connect(previousSectionShortcut, &QShortcut::activated, this, &dlgTriggerEditor::slot_previousSection);

    QShortcut* activateMainWindowAction = new QShortcut(QKeySequence((Qt::ALT | Qt::Key_E)), this);
    connect(activateMainWindowAction, &QShortcut::activated, this, &dlgTriggerEditor::slot_activateMainWindow);

    toolBar = new QToolBar();
    toolBar2 = new QToolBar();

    connect(mudlet::self(), &mudlet::signal_setToolBarIconSize, this, &dlgTriggerEditor::slot_setToolBarIconSize);
    connect(mudlet::self(), &mudlet::signal_setTreeIconSize, this, &dlgTriggerEditor::slot_setTreeWidgetIconSize);
    slot_setToolBarIconSize(mudlet::self()->mToolbarIconSize);
    slot_setTreeWidgetIconSize(mudlet::self()->mEditorTreeWidgetIconSize);

    toolBar->setMovable(true);
    toolBar->addAction(toggleActiveAction);
    toolBar->addAction(mSaveItem);
    //: This is the toolbar that is initially placed at the top of the editor.
    toolBar->setWindowTitle(tr("Editor Toolbar - %1 - Actions").arg(hostName));

    toolBar->addSeparator();

    toolBar->addAction(mAddItem);
    toolBar->addAction(mAddGroup);

    toolBar->addSeparator();
    toolBar->addAction(mDeleteItem);
    toolBar->addAction(importAction);
    toolBar->addAction(mpExportAction);
    toolBar->addAction(mProfileSaveAsAction);
    toolBar->addAction(mProfileSaveAction);
    toolBar->addAction(undoAction);
    toolBar->addAction(redoAction);

    connect(checkBox_displayAllVariables, &QAbstractButton::toggled, this, &dlgTriggerEditor::slot_toggleHiddenVariables);

    connect(mpVarsMainArea->checkBox_variable_hidden, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_hideVariable);

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
    //: This is the toolbar that is initially placed at the left side of the editor.
    toolBar2->setWindowTitle(tr("Editor Toolbar - %1 - Items").arg(hostName));
    toolBar2->setOrientation(Qt::Vertical);

    // Inserting them in this order also causes the first one (the top toolbar)
    // to be listed first in the QMainWindows's default context menu:
    QMainWindow::addToolBar(Qt::TopToolBarArea, toolBar);
    QMainWindow::addToolBar(Qt::LeftToolBarArea, toolBar2);

    // (Top) "Actions" toolbar:
    //: This will restore that toolbar in the editor window, after a user has hidden it or moved it to another docking location or floated it elsewhere.
    mpAction_restoreEditorActionsToolbar = new QAction(tr("Restore Actions toolbar"), this);
    // (Left) "Items" toolbar:
    //: This will restore that toolbar in the editor window, after a user has hidden it or moved it to another docking location or floated it elsewhere.
    mpAction_restoreEditorItemsToolbar = new QAction(tr("Restore Items toolbar"), this);

    connect(mpAction_restoreEditorActionsToolbar, &QAction::triggered, this, &dlgTriggerEditor::slot_restoreEditorActionsToolbar);
    connect(mpAction_restoreEditorItemsToolbar, &QAction::triggered, this, &dlgTriggerEditor::slot_restoreEditorItemsToolbar);
    connect(toolBar, &QToolBar::visibilityChanged, this, &dlgTriggerEditor::slot_visibilityChangedEditorActionsToolbar);
    connect(toolBar2, &QToolBar::visibilityChanged, this, &dlgTriggerEditor::slot_visibilityChangedEditorItemsToolbar);
    connect(toolBar, &QToolBar::topLevelChanged, this, &dlgTriggerEditor::slot_floatingChangedEditorActionsToolbar);
    connect(toolBar2, &QToolBar::topLevelChanged, this, &dlgTriggerEditor::slot_floatingChangedEditorItemsToolbar);

    treeWidget_triggers->addAction(mpAction_restoreEditorActionsToolbar);
    treeWidget_aliases->addAction(mpAction_restoreEditorActionsToolbar);
    treeWidget_timers->addAction(mpAction_restoreEditorActionsToolbar);
    treeWidget_scripts->addAction(mpAction_restoreEditorActionsToolbar);
    treeWidget_actions->addAction(mpAction_restoreEditorActionsToolbar);
    treeWidget_keys->addAction(mpAction_restoreEditorActionsToolbar);

    treeWidget_triggers->addAction(mpAction_restoreEditorItemsToolbar);
    treeWidget_aliases->addAction(mpAction_restoreEditorItemsToolbar);
    treeWidget_timers->addAction(mpAction_restoreEditorItemsToolbar);
    treeWidget_scripts->addAction(mpAction_restoreEditorItemsToolbar);
    treeWidget_actions->addAction(mpAction_restoreEditorItemsToolbar);
    treeWidget_keys->addAction(mpAction_restoreEditorItemsToolbar);

    // These only have to be shown should the associated toolbar get hidden
    // and by default the starting state for those is a visible one so these
    // need to be hidden at the start:
    mpAction_restoreEditorActionsToolbar->setVisible(false);
    mpAction_restoreEditorItemsToolbar->setVisible(false);
    setShortcuts();

    auto config = mpSourceEditorEdbee->config();
    config->beginChanges();
    config->setThemeName(mpHost->mEditorTheme);
    config->setFont(mpHost->getDisplayFont());
    config->setShowWhitespaceMode((mudlet::self()->mEditorTextOptions & QTextOption::ShowTabsAndSpaces) ? edbee::TextEditorConfig::ShowWhitespaces : edbee::TextEditorConfig::HideWhitespaces);
    config->setUseLineSeparator(mudlet::self()->mEditorTextOptions & QTextOption::ShowLineAndParagraphSeparators);
    config->setAutocompleteAutoShow(mpHost->mEditorAutoComplete);
    config->setRenderBidiContolCharacters(mpHost->getEditorShowBidi());
    config->setAutocompleteMinimalCharacters(3);
    config->endChanges();

    connect(comboBox_searchTerms, qOverload<int>(&QComboBox::activated), this, &dlgTriggerEditor::slot_searchMudletItems);
    connect(treeWidget_triggers, &QTreeWidget::itemClicked, this, &dlgTriggerEditor::slot_triggerSelected);
    connect(treeWidget_triggers, &QTreeWidget::itemSelectionChanged, this, &dlgTriggerEditor::slot_treeSelectionChanged);
    connect(treeWidget_keys, &QTreeWidget::itemClicked, this, &dlgTriggerEditor::slot_keySelected);
    connect(treeWidget_keys, &QTreeWidget::itemSelectionChanged, this, &dlgTriggerEditor::slot_treeSelectionChanged);
    connect(treeWidget_timers, &QTreeWidget::itemClicked, this, &dlgTriggerEditor::slot_timerSelected);
    connect(treeWidget_timers, &QTreeWidget::itemSelectionChanged, this, &dlgTriggerEditor::slot_treeSelectionChanged);
    connect(treeWidget_scripts, &QTreeWidget::itemClicked, this, &dlgTriggerEditor::slot_scriptsSelected);
    connect(treeWidget_scripts, &QTreeWidget::itemSelectionChanged, this, &dlgTriggerEditor::slot_treeSelectionChanged);
    connect(treeWidget_aliases, &QTreeWidget::itemClicked, this, &dlgTriggerEditor::slot_aliasSelected);
    connect(treeWidget_aliases, &QTreeWidget::itemSelectionChanged, this, &dlgTriggerEditor::slot_treeSelectionChanged);
    connect(treeWidget_actions, &QTreeWidget::itemClicked, this, &dlgTriggerEditor::slot_actionSelected);
    connect(treeWidget_actions, &QTreeWidget::itemSelectionChanged, this, &dlgTriggerEditor::slot_treeSelectionChanged);
    connect(treeWidget_variables, &QTreeWidget::itemClicked, this, &dlgTriggerEditor::slot_variableSelected);
    connect(treeWidget_variables, &QTreeWidget::itemChanged, this, &dlgTriggerEditor::slot_variableChanged);
    connect(treeWidget_variables, &QTreeWidget::itemSelectionChanged, this, &dlgTriggerEditor::slot_treeSelectionChanged);
    connect(treeWidget_searchResults, &QTreeWidget::itemClicked, this, &dlgTriggerEditor::slot_itemSelectedInSearchResults);

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
        auto* pAction_clear(qobject_cast<QAction*>(child));

        // The name was found by inspection - but as it is a QT internal it
        // might change in the future:
        if (pAction_clear && pAction_clear->objectName() == QLatin1String("_q_qlineeditclearaction")) {
            connect(pAction_clear, &QAction::triggered, this, &dlgTriggerEditor::slot_clearSearchResults, Qt::QueuedConnection);
            break;
        }
    }

    mpAction_searchOptions = new QAction(tr("Search Options"), this);
    mpAction_searchOptions->setObjectName(qsl("mpAction_searchOptions"));

    QMenu* pMenu_searchOptions = new QMenu(tr("Search Options"), this);
    pMenu_searchOptions->setObjectName(qsl("pMenu_searchOptions"));
    pMenu_searchOptions->setToolTipsVisible(true);

    mpAction_searchCaseSensitive = new QAction(tr("Case sensitive"), this);
    mpAction_searchCaseSensitive->setObjectName(qsl("mpAction_searchCaseSensitive"));
    mpAction_searchCaseSensitive->setToolTip(utils::richText(tr("Match case precisely")));
    mpAction_searchCaseSensitive->setCheckable(true);
    pMenu_searchOptions->insertAction(nullptr, mpAction_searchCaseSensitive);

    mpAction_searchIncludeVariables = new QAction(tr("Include variables"), this);
    mpAction_searchIncludeVariables->setObjectName(qsl("mpAction_searchIncludeVariables"));
    mpAction_searchIncludeVariables->setToolTip(utils::richText(tr("Search variables (slower)")));
    mpAction_searchIncludeVariables->setCheckable(true);
    pMenu_searchOptions->insertAction(nullptr, mpAction_searchIncludeVariables);

    // This will set the icon and the Search Options menu items - and needs to
    // be done BEFORE the menu items are connect()ed:
    setSearchOptions(mSearchOptions);

    connect(mpAction_searchCaseSensitive, &QAction::triggered, this, &dlgTriggerEditor::slot_toggleSearchCaseSensitivity);
    connect(mpAction_searchIncludeVariables, &QAction::triggered, this, &dlgTriggerEditor::slot_toggleSearchIncludeVariables);


    mpAction_searchOptions->setMenu(pMenu_searchOptions);

    pLineEdit_searchTerm->addAction(mpAction_searchOptions, QLineEdit::LeadingPosition);

    connect(mpScriptsMainArea->toolButton_script_add_event_handler, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_scriptMainAreaAddHandler);
    connect(mpScriptsMainArea->toolButton_script_remove_event_handler, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_scriptMainAreaDeleteHandler);

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
    //: Heading for the first column of the search results
    labelList << tr("Type")
              //: Heading for the second column of the search results
              << tr("Name")
              //: Heading for the third column of the search results
              << tr("Where")
              //: Heading for the fourth column of the search results
              << tr("What");
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
    const QIcon icon_subString(pixMap_subString);

    QPixmap pixMap_perl_regex(256, 256);
    pixMap_perl_regex.fill(Qt::blue);
    const QIcon icon_perl_regex(pixMap_perl_regex);

    QPixmap pixMap_begin_of_line_substring(256, 256);
    pixMap_begin_of_line_substring.fill(Qt::red);
    const QIcon icon_begin_of_line_substring(pixMap_begin_of_line_substring);

    QPixmap pixMap_exact_match(256, 256);
    pixMap_exact_match.fill(Qt::green);
    const QIcon icon_exact_match(pixMap_exact_match);

    QPixmap pixMap_lua_function(256, 256);
    pixMap_lua_function.fill(Qt::cyan);
    const QIcon icon_lua_function(pixMap_lua_function);

    QPixmap pixMap_line_spacer(256, 256);
    pixMap_line_spacer.fill(Qt::magenta);
    const QIcon icon_line_spacer(pixMap_line_spacer);

    QPixmap pixMap_color_trigger(256, 256);
    pixMap_color_trigger.fill(Qt::lightGray);
    const QIcon icon_color_trigger(pixMap_color_trigger);

    QPixmap pixMap_prompt(256, 256);
    pixMap_prompt.fill(Qt::yellow);
    const QIcon icon_prompt(pixMap_prompt);

    QStringList patternList;
    patternList << tr("substring") << tr("perl regex") << tr("start of line") << tr("exact match") << tr("lua function") << tr("line spacer") << tr("color trigger") << tr("prompt");

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
        mpPrevTriggerPatternItemEdit.insert(i, pBox->currentIndex());
        mpPrevTriggerPatternEdit.insert(i, pItem->lineEdit_pattern->text());
        mPrevLineSpacer.insert(i, pItem->spinBox_lineSpacer->value());
        connect(pBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &dlgTriggerEditor::slot_setupPatternControls);
        connect(pItem->pushButton_fgColor, &QAbstractButton::clicked, this, [=]()->void{dlgTriggerEditor::slot_colorTriggerFg(i);});
        connect(pItem->pushButton_bgColor, &QAbstractButton::clicked, this, [=]()->void{dlgTriggerEditor::slot_colorTriggerBg(i);});
        connect(pItem->lineEdit_pattern, &QLineEdit::textChanged, this, &dlgTriggerEditor::slot_changedPattern);
        connect(pBox, &QComboBox::activated, this, &dlgTriggerEditor::slot_triggerLinePatternItemEdited);
        connect(pItem->lineEdit_pattern, &QLineEdit::editingFinished, this, [=]()->void{dlgTriggerEditor::slot_triggerLinePatternEdited(i);});
        connect(pItem->spinBox_lineSpacer, &QSpinBox::valueChanged, this, [=]()->void{dlgTriggerEditor::slot_triggerLineSpacerEdited(i);});
        HpatternList->layout()->addWidget(pItem);
        mTriggerPatternEdit.push_back(pItem);
        pItem->mRow = i;
        pItem->pushButton_fgColor->hide();
        pItem->pushButton_bgColor->hide();
        pItem->pushButton_prompt->hide();
        pItem->spinBox_lineSpacer->hide();
        pItem->label_patternNumber->setText(QString::number(i + 1));
        pItem->label_patternNumber->show();


        // Populate default of false
        lineEditShouldMarkSpaces[pItem->lineEdit_pattern] = false;

        if (i == 0) {
            pItem->lineEdit_pattern->setPlaceholderText(tr("Text to find (trigger pattern)"));
        }
        if(i > 0)
        {
            pItem->lineEdit_pattern->setEnabled(false);
        }
    }
    // force the minimum size of the scroll area for the trigger items to be one
    // and a half trigger item widgets:
    const int triggerWidgetItemMinHeight = qRound(mTriggerPatternEdit.at(0)->minimumSizeHint().height() * 1.5);
    mpScrollArea->setMinimumHeight(triggerWidgetItemMinHeight);

    widget_searchTerm->updateGeometry();

    showIDLabels(mpHost->showIdsInEditor());
    if (mAutosaveInterval > 0) {
        startTimer(mAutosaveInterval * 1min);
    }
}

void dlgTriggerEditor::slot_hideVariable(bool status)
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

    const QSize newSize(s * 8, s * 8);
    toolBar->setIconSize(newSize);
    toolBar2->setIconSize(newSize);
}

void dlgTriggerEditor::slot_setTreeWidgetIconSize(const int s)
{
    if (s <= 0) {
        return;
    }

    const QSize newSize(s * 8, s * 8);
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
    QSettings& settings = *mudlet::getQSettings();

    const QPoint pos = settings.value("script_editor_pos", QPoint(10, 10)).toPoint();
    const QSize size = settings.value("script_editor_size", QSize(600, 400)).toSize();
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

void dlgTriggerEditor::createUndoView()
{
    QDockWidget* undoDockWidget = new QDockWidget;
    undoDockWidget->setWindowTitle(tr("Command List"));
    undoDockWidget->setWidget(new QUndoView(undoStack));
    treeWidget_triggers->mpUndoStack = undoStack;
    treeWidget_aliases->mpUndoStack = undoStack;
    treeWidget_timers->mpUndoStack = undoStack;
    treeWidget_scripts->mpUndoStack = undoStack;
    treeWidget_keys->mpUndoStack = undoStack;
    treeWidget_actions->mpUndoStack = undoStack;
    treeWidget_variables->mpUndoStack = undoStack;
    addDockWidget(Qt::RightDockWidgetArea, undoDockWidget);
}

void dlgTriggerEditor::writeSettings()
{
    QSettings& settings = *mudlet::getQSettings();
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

void dlgTriggerEditor::slot_itemSelectedInSearchResults(QTreeWidgetItem* pItem)
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
        foundItemsList = treeWidget_triggers->findItems(pItem->data(0, NameRole).toString(), Qt::MatchCaseSensitive | Qt::MatchFixedString | Qt::MatchRecursive, 0);

        // This was inside the loop but it is a constant value for the duration
        // of this method!
        const int idSearch = pItem->data(0, IdRole).toInt();

        for (auto treeWidgetItem : std::as_const(foundItemsList)) {
            if (treeWidgetItem->data(0, IdRole).toInt() == idSearch) {
                slot_showTriggers();
                slot_triggerSelected(treeWidgetItem);
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
                    dlgTriggerPatternEdit* pTriggerPattern = mTriggerPatternEdit.at(pItem->data(0, PatternOrLineRole).toInt());
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
                    qDebug() << "dlgTriggerEditor::slot_item_selected_list(...) Called for a TRIGGER type item but handler for element of type:" << treeWidgetItem->data(0, TypeRole).toInt()
                             << "not yet done/applicable...!";
                }
                return;
            }
        }
        break;
    }

    case EditorViewType::cmAliasView: {
        foundItemsList = treeWidget_aliases->findItems(pItem->data(0, NameRole).toString(), Qt::MatchCaseSensitive | Qt::MatchFixedString | Qt::MatchRecursive, 0);

        const int idSearch = pItem->data(0, IdRole).toInt();

        for (auto treeWidgetItem : std::as_const(foundItemsList)) {
            if (treeWidgetItem->data(0, IdRole).toInt() == idSearch) {
                slot_showAliases();
                slot_aliasSelected(treeWidgetItem);
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
                    qDebug() << "dlgTriggerEditor::slot_item_selected_list(...) Called for a ALIAS type item but handler for element of type:" << treeWidgetItem->data(0, TypeRole).toInt()
                             << "not yet done/applicable...!";
                }
                return;
            }
        }
        break;
    }

    case EditorViewType::cmScriptView: {
        foundItemsList = treeWidget_scripts->findItems(pItem->data(0, NameRole).toString(), Qt::MatchCaseSensitive | Qt::MatchFixedString | Qt::MatchRecursive, 0);

        const int idSearch = pItem->data(0, IdRole).toInt();

        for (auto treeWidgetItem : std::as_const(foundItemsList)) {
            if (treeWidgetItem->data(0, IdRole).toInt() == idSearch) {
                slot_showScripts();
                slot_scriptsSelected(treeWidgetItem);
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
                    // Taken from slot_scriptMainAreaEditHandler():
                    // Note the handler item being edited:
                    mpScriptsMainAreaEditHandlerItem = mpScriptsMainArea->listWidget_script_registered_event_handlers->currentItem();
                    // Copy the event name to the entry widget:
                    mpScriptsMainArea->lineEdit_script_event_handler_entry->setText(mpScriptsMainAreaEditHandlerItem->text());
                    // Activate editing flag:
                    mIsScriptsMainAreaEditHandler = true;
                    break;
                default:
                    qDebug() << "dlgTriggerEditor::slot_item_selected_list(...) Called for a SCRIPT type item but handler for element of type:" << treeWidgetItem->data(0, TypeRole).toInt()
                             << "not yet done/applicable...!";
                }

                return;
            }
        }
        break;
    }

    case EditorViewType::cmActionView: {
        foundItemsList = treeWidget_actions->findItems(pItem->data(0, NameRole).toString(), Qt::MatchCaseSensitive | Qt::MatchFixedString | Qt::MatchRecursive, 0);

        const int idSearch = pItem->data(0, IdRole).toInt();

        for (auto treeWidgetitem : std::as_const(foundItemsList)) {
            if (treeWidgetitem->data(0, IdRole).toInt() == idSearch) {
                slot_showActions();
                slot_actionSelected(treeWidgetitem);
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
                    mpActionsMainArea->lineEdit_action_button_command_down->setFocus(Qt::OtherFocusReason);
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
                    qDebug() << "dlgTriggerEditor::slot_item_selected_list(...) Called for a BUTTON type item but handler for element of type:" << treeWidgetitem->data(0, TypeRole).toInt()
                             << "not yet done/applicable...!";
                } // End or switch()
                return;
            } // End of if()
        } // End of for()
        break;
    } // End of case EditorViewType::cmActionView

    case EditorViewType::cmTimerView: {
        foundItemsList = treeWidget_timers->findItems(pItem->data(0, NameRole).toString(), Qt::MatchCaseSensitive | Qt::MatchFixedString | Qt::MatchRecursive, 0);

        const int idSearch = pItem->data(0, IdRole).toInt();

        for (auto treeWidgetItem : std::as_const(foundItemsList)) {
            if (treeWidgetItem->data(0, IdRole).toInt() == idSearch) {
                slot_showTimers();
                slot_timerSelected(treeWidgetItem);
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
                    qDebug() << "dlgTriggerEditor::slot_item_selected_list(...) Called for a TIMER type item but handler for element of type:" << treeWidgetItem->data(0, TypeRole).toInt()
                             << "not yet done/applicable...!";
                } // End of switch()
                return;
            } // End of if()
        } // End of for()
        break;
    } // End of case EditorViewType::cmTimerView

    case EditorViewType::cmKeysView: {
        foundItemsList = treeWidget_keys->findItems(pItem->data(0, NameRole).toString(), Qt::MatchCaseSensitive | Qt::MatchFixedString | Qt::MatchRecursive, 0);

        for (auto treeWidgetItem : std::as_const(foundItemsList)) {
            const int idTree = treeWidgetItem->data(0, IdRole).toInt();
            const int idSearch = pItem->data(0, IdRole).toInt();
            if (idTree == idSearch) {
                slot_showKeys();
                slot_keySelected(treeWidgetItem);
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
                    dlgTriggerPatternEdit* pTriggerPattern = mTriggerPatternEdit.at(pItem->data(0, PatternOrLineRole).toInt());
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
                    qDebug() << "dlgTriggerEditor::slot_item_selected_list(...) Called for a KEY type item but handler for element of type:" << treeWidgetItem->data(0, TypeRole).toInt()
                             << "not yet done/applicable...!";
                } // End of switch()
                return;
            } // End of if
        } // End of for
        break;
    } // End of case EditorViewType::cmKeysView

    case EditorViewType::cmVarsView: {
        LuaInterface* lI = mpHost->getLuaInterface();
        VarUnit* vu = lI->getVarUnit();
        const QStringList varShort = pItem->data(0, IdRole).toStringList();
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
                    qDebug() << "dlgTriggerEditor::slot_item_selected_list(...) Called for a VAR type item but handler for element of type:" << treeWidgetItem->data(0, TypeRole).toInt()
                             << "not yet done/applicable...!";
                }
                return;
            }
        }
    } // End of case static_cast<int>(EditorViewType::cmVarsView)
    break;
    default:; // No-op
    } // End of switch()
}

void dlgTriggerEditor::slot_searchMudletItems(const int index)
{
    if (index < 0) {
        return;
    }
    const QString s{comboBox_searchTerms->itemText(index)};
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

void dlgTriggerEditor::searchVariables(const QString& text)
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
            const QString name = varDecendent->getName();
            const QString value = varDecendent->getValue();
            const QStringList idStringList = vu->shortVarName(varDecendent);
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
                    const QString intermediate = itSubString.next();
                    bool isOk = false;
                    const int numberValue = intermediate.toInt(&isOk);
                    if (isOk && QString::number(numberValue) == intermediate) {
                        // This seems to be an integer
                        idString.append(qsl("[%1]").arg(intermediate));
                    } else {
                        idString.append(qsl("[\"%1\"]").arg(intermediate));
                    }
                }
            } else if (!idStringList.empty()) {
                idString = idStringList.at(0);
            }

            int startPos = 0;
            if ((startPos = name.indexOf(text, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
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
            if (value != QLatin1String("function") && (startPos = value.indexOf(text, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
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

void dlgTriggerEditor::searchKeys(const QString& text)
{
    std::list<TKey*> const nodes = mpHost->getKeyUnit()->getKeyRootNodeList();
    for (auto key : nodes) {
        QTreeWidgetItem* pItem;
        QTreeWidgetItem* parent = nullptr;
        const QString name = key->getName();
        int startPos = 0;

        if ((startPos = name.indexOf(text, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
            QStringList sl;
            sl << tr("Key") << name << tr("Name");
            // This part can never have a parent as it is the first part of this item
            parent = new QTreeWidgetItem(sl);
            setAllSearchData(parent, EditorViewType::cmKeysView, name, key->getID(), SearchResultIsName, startPos);
            treeWidget_searchResults->addTopLevelItem(parent);
        }

        // The simple "command"
        // TODO: (A) Revise to count multiple instances of search string within command?
        if ((startPos = key->getCommand().indexOf(text, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
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
        const QStringList textList = key->getScript().split("\n");
        const int total = textList.count();
        for (int index = 0; index < total; ++index) {
            // CHECK: This may NOT be an optimisation...!
            if (textList.at(index).isEmpty() || !textList.at(index).contains(text, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) {
                // Short-cuts that mean we do not have to examine the line in more detail
                continue;
            }

            int instance = 0;
            startPos = 0;
            while ((startPos = textList.at(index).indexOf(text, startPos, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
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

        recursiveSearchKeys(key, text);
    }
}

void dlgTriggerEditor::searchTimers(const QString& text)
{
    std::list<TTimer*> const nodes = mpHost->getTimerUnit()->getTimerRootNodeList();
    for (auto timer : nodes) {
        QTreeWidgetItem* pItem;
        QTreeWidgetItem* parent = nullptr;
        const QString name = timer->getName();
        int startPos = 0;

        if ((startPos = name.indexOf(text, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
            QStringList sl;
            sl << tr("Timer") << name << tr("Name");
            // This part can never have a parent as it is the first part of this item
            parent = new QTreeWidgetItem(sl);
            setAllSearchData(parent, EditorViewType::cmTimerView, name, timer->getID(), SearchResultIsName, startPos);
            treeWidget_searchResults->addTopLevelItem(parent);
        }

        // The simple "command"
        // TODO: (A) Revise to count multiple instances of search string within command?
        if (timer->getCommand().contains(text, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) {
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
        const QStringList textList = timer->getScript().split("\n");
        const int total = textList.count();
        for (int index = 0; index < total; ++index) {
            // CHECK: This may NOT be an optimisation...!
            if (textList.at(index).isEmpty() || !textList.at(index).contains(text, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) {
                // Short-cuts that mean we do not have to examine the line in more detail
                continue;
            }

            int instance = 0;
            startPos = 0;
            while ((startPos = textList.at(index).indexOf(text, startPos, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
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

        recursiveSearchTimers(timer, text);
    }
}

void dlgTriggerEditor::searchActions(const QString& text)
{
    std::list<TAction*> const nodes = mpHost->getActionUnit()->getActionRootNodeList();
    for (auto action : nodes) {
        QTreeWidgetItem* pItem;
        QTreeWidgetItem* parent = nullptr;
        const QString name = action->getName();
        int startPos = 0;

        if ((startPos = name.indexOf(text, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
            QStringList sl;
            sl << tr("Button") << name << tr("Name");
            // This part can never have a parent as it is the first part of this item
            parent = new QTreeWidgetItem(sl);
            setAllSearchData(parent, EditorViewType::cmActionView, name, action->getID(), SearchResultIsName, startPos);
            treeWidget_searchResults->addTopLevelItem(parent);
        }

        // The simple (down) "command"
        // TODO: (A) Revise to count multiple instances of search string within command?
        if ((startPos = action->getCommandButtonDown().indexOf(text, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
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
            if ((startPos = action->getCommandButtonUp().indexOf(text, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
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
            if (textList.at(index).isEmpty() || !textList.at(index).contains(text, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) {
                // Short-cuts that mean we do not have to examine the line in more detail
                continue;
            }

            int instance = 0;
            startPos = 0;
            while ((startPos = textList.at(index).indexOf(text, startPos, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
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
            if (textList.at(index).isEmpty() || !textList.at(index).contains(text, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) {
                // Short-cuts that mean we do not have to examine the line in more detail
                continue;
            }

            int instance = 0;
            startPos = 0;
            while ((startPos = textList.at(index).indexOf(text, startPos, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
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

        recursiveSearchActions(action, text);
    }
}

void dlgTriggerEditor::searchScripts(const QString& text)
{
    std::list<TScript*> const nodes = mpHost->getScriptUnit()->getScriptRootNodeList();
    for (auto script : nodes) {
        QTreeWidgetItem* pItem;
        QTreeWidgetItem* parent = nullptr;
        const QString name = script->getName();
        int startPos = 0;

        if ((startPos = name.indexOf(text, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
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
            if (textList.at(index).isEmpty() || !textList.at(index).contains(text, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) {
                // Short-cuts that mean we do not have to examine the line in more detail
                continue;
            }

            int instance = 0;
            startPos = 0;
            while ((startPos = textList.at(index).indexOf(text, startPos, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
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
            if (textList.at(index).isEmpty() || !textList.at(index).contains(text, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) {
                // Short-cuts that mean we do not have to examine the line in more detail
                continue;
            }

            int instance = 0;
            int startPos = 0;
            while ((startPos = textList.at(index).indexOf(text, startPos, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
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

        recursiveSearchScripts(script, text);
    }
}

void dlgTriggerEditor::searchAliases(const QString& text)
{
    std::list<TAlias*> const nodes = mpHost->getAliasUnit()->getAliasRootNodeList();
    for (auto alias : nodes) {
        QTreeWidgetItem* pItem;
        QTreeWidgetItem* parent = nullptr;
        const QString name = alias->getName();
        int startPos = 0;

        if ((startPos = name.indexOf(text, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
            QStringList sl;
            sl << tr("Alias") << name << tr("Name");
            parent = new QTreeWidgetItem(sl);
            setAllSearchData(parent, EditorViewType::cmAliasView, name, alias->getID(), SearchResultIsName, startPos);
            treeWidget_searchResults->addTopLevelItem(parent);
        }

        // The simple "command"
        if ((startPos = alias->getCommand().indexOf(text, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
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
        if ((startPos = alias->getRegexCode().indexOf(text, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
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
        const QStringList textList = alias->getScript().split("\n");
        const int total = textList.count();
        for (int index = 0; index < total; ++index) {
            // CHECK: This may NOT be an optimisation...!
            if (textList.at(index).isEmpty() || !textList.at(index).contains(text, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) {
                // Short-cuts that mean we do not have to examine the line in more detail
                continue;
            }

            int instance = 0;
            startPos = 0;
            while ((startPos = textList.at(index).indexOf(text, startPos, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
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

        recursiveSearchAlias(alias, text);
    }
}

void dlgTriggerEditor::searchTriggers(const QString& text)
{
    std::list<TTrigger*> const nodes = mpHost->getTriggerUnit()->getTriggerRootNodeList();
    for (auto trigger : nodes) {
        QTreeWidgetItem* pItem;
        QTreeWidgetItem* parent = nullptr;
        const QString name = trigger->getName();
        int startPos = 0;

        if ((startPos = name.indexOf(text, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
            QStringList sl;
            sl << tr("Trigger") << name << tr("Name");
            // This part can never have a parent as it is the first part of this item
            parent = new QTreeWidgetItem(sl);
            setAllSearchData(parent, EditorViewType::cmTriggerView, name, trigger->getID(), SearchResultIsName, startPos);
            treeWidget_searchResults->addTopLevelItem(parent);
        }

        // The simple "command"
        // TODO: (A) Revise to count multiple instances of search string within command?
        if ((startPos = trigger->getCommand().indexOf(text, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
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
        QStringList textList = trigger->getPatternsList();
        int total = textList.count();
        for (int index = 0; index < total; ++index) {
            // CHECK: This may NOT be an optimisation...!
            if (textList.at(index).isEmpty() || !textList.at(index).contains(text, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) {
                // Short-cuts that mean we do not have to examine this line in more detail
                continue;
            }

            int instance = 0;
            startPos = 0;
            while ((startPos = textList.at(index).indexOf(text, startPos, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
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
            if (textList.at(index).isEmpty() || !textList.at(index).contains(text, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) {
                // Short-cuts that mean we do not have to examine the line in more detail
                continue;
            }

            int instance = 0;
            startPos = 0;
            while ((startPos = textList.at(index).indexOf(text, startPos, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
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

        recursiveSearchTriggers(trigger, text);
    }
}

void dlgTriggerEditor::recursiveSearchTriggers(TTrigger* pTriggerParent, const QString& text)
{
    std::list<TTrigger*>* childrenList = pTriggerParent->getChildrenList();
    for (auto trigger : *childrenList) {
        QTreeWidgetItem* pItem;
        QTreeWidgetItem* parent = nullptr;
        const QString name = trigger->getName();
        int startPos = 0;

        if ((startPos = name.indexOf(text, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
            QStringList sl;
            sl << tr("Trigger") << name << tr("Name");
            // This part can never have a parent as it is the first part of this item
            parent = new QTreeWidgetItem(sl);
            setAllSearchData(parent, EditorViewType::cmTriggerView, name, trigger->getID(), SearchResultIsName, startPos);
            treeWidget_searchResults->addTopLevelItem(parent);
        }

        // The simple "command"
        // TODO: (A) Revise to count multiple instances of search string within command?
        if ((startPos = trigger->getCommand().indexOf(text, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
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
        QStringList textList = trigger->getPatternsList();
        int total = textList.count();
        for (int index = 0; index < total; ++index) {
            if (textList.at(index).isEmpty() || !textList.at(index).contains(text, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) {
                // Short-cuts that mean we do not have to examine the line in more detail
                continue;
            }

            int instance = 0;
            startPos = 0;
            while ((startPos = textList.at(index).indexOf(text, startPos, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
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
            if (textList.at(index).isEmpty() || !textList.at(index).contains(text, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) {
                // Short-cuts that mean we do not have to examine the line in more detail
                continue;
            }

            int instance = 0;
            startPos = 0;
            while ((startPos = textList.at(index).indexOf(text, startPos, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
                // We need to replace tabs in the script with two spaces
                // otherwise the displayed text A) does not match the main
                // editor settings and B). often gets shifted out of view by
                // any leading tabs which are quite common in Lua formatting...!
                QString whatText(textList.at(index));
                whatText.replace(QString(QChar::SpecialCharacter::Tabulation), QString(QChar::Space).repeated(2));
                QStringList sl;
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

        if (trigger->hasChildren()) {
            recursiveSearchTriggers(trigger, text);
        }
    }
}

void dlgTriggerEditor::recursiveSearchAlias(TAlias* pTriggerParent, const QString& text)
{
    std::list<TAlias*>* childrenList = pTriggerParent->getChildrenList();
    for (auto alias : *childrenList) {
        QTreeWidgetItem* pItem;
        QTreeWidgetItem* parent = nullptr;
        const QString name = alias->getName();
        int startPos = 0;

        if ((startPos = name.indexOf(text, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
            QStringList sl;
            sl << tr("Alias") << name << tr("Name");
            parent = new QTreeWidgetItem(sl);
            setAllSearchData(parent, EditorViewType::cmAliasView, name, alias->getID(), SearchResultIsName, startPos);
            treeWidget_searchResults->addTopLevelItem(parent);
        }

        // The simple "command"
        if ((startPos = alias->getCommand().indexOf(text, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
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
        if ((startPos = alias->getRegexCode().indexOf(text, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
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
        const QStringList textList = alias->getScript().split("\n");
        const int total = textList.count();
        for (int index = 0; index < total; ++index) {
            // CHECK: This may NOT be an optimisation...!
            if (textList.at(index).isEmpty() || !textList.at(index).contains(text, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) {
                // Short-cuts that mean we do not have to examine the line in more detail
                continue;
            }

            int instance = 0;
            startPos = 0;
            while ((startPos = textList.at(index).indexOf(text, startPos, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
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
            recursiveSearchAlias(alias, text);
        }
    }
}

void dlgTriggerEditor::recursiveSearchScripts(TScript* pTriggerParent, const QString& text)
{
    std::list<TScript*>* childrenList = pTriggerParent->getChildrenList();
    for (auto script : *childrenList) {
        QTreeWidgetItem* pItem;
        QTreeWidgetItem* parent = nullptr;
        const QString name = script->getName();
        int startPos = 0;

        if ((startPos = name.indexOf(text, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
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
            if (textList.at(index).isEmpty() || !textList.at(index).contains(text, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) {
                // Short-cuts that mean we do not have to examine the line in more detail
                continue;
            }

            int instance = 0;
            startPos = 0;
            while ((startPos = textList.at(index).indexOf(text, startPos, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
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
            if (textList.at(index).isEmpty() || !textList.at(index).contains(text, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) {
                // Short-cuts that mean we do not have to examine the line in more detail
                continue;
            }

            int instance = 0;
            startPos = 0;
            while ((startPos = textList.at(index).indexOf(text, startPos, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
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
            recursiveSearchScripts(script, text);
        }
    }
}

void dlgTriggerEditor::recursiveSearchActions(TAction* pTriggerParent, const QString& text)
{
    std::list<TAction*>* childrenList = pTriggerParent->getChildrenList();
    for (auto action : *childrenList) {
        QTreeWidgetItem* pItem;
        QTreeWidgetItem* parent = nullptr;
        const QString name = action->getName();
        int startPos = 0;

        if ((startPos = name.indexOf(text, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
            QStringList sl;
            sl << tr("Button") << name << tr("Name");
            // This part can never have a parent as it is the first part of this item
            parent = new QTreeWidgetItem(sl);
            setAllSearchData(parent, EditorViewType::cmActionView, name, action->getID(), SearchResultIsName, startPos);
            treeWidget_searchResults->addTopLevelItem(parent);
        }

        // The simple (down) "command"
        // TODO: (A) Revise to count multiple instances of search string within command?
        if ((startPos = action->getCommandButtonDown().indexOf(text, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
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
            if ((startPos = action->getCommandButtonUp().indexOf(text, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
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
            if (textList.at(index).isEmpty() || !textList.at(index).contains(text, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) {
                // Short-cuts that mean we do not have to examine the line in more detail
                continue;
            }

            int instance = 0;
            startPos = 0;
            while ((startPos = textList.at(index).indexOf(text, startPos, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
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
            if (textList.at(index).isEmpty() || !textList.at(index).contains(text, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) {
                // Short-cuts that mean we do not have to examine the line in more detail
                continue;
            }

            int instance = 0;
            startPos = 0;
            while ((startPos = textList.at(index).indexOf(text, startPos, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
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
            recursiveSearchActions(action, text);
        }
    }
}

void dlgTriggerEditor::recursiveSearchTimers(TTimer* pTriggerParent, const QString& text)
{
    std::list<TTimer*>* childrenList = pTriggerParent->getChildrenList();
    for (auto timer : *childrenList) {
        QTreeWidgetItem* pItem;
        QTreeWidgetItem* parent = nullptr;
        const QString name = timer->getName();
        int startPos = 0;

        if ((startPos = name.indexOf(text, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
            QStringList sl;
            sl << tr("Timer") << name << tr("Name");
            // This part can never have a parent as it is the first part of this item
            parent = new QTreeWidgetItem(sl);
            setAllSearchData(parent, EditorViewType::cmTimerView, name, timer->getID(), SearchResultIsName, startPos);
            treeWidget_searchResults->addTopLevelItem(parent);
        }

        // The simple "command"
        // TODO: (A) Revise to count multiple instances of search string within command?
        if ((startPos = timer->getCommand().indexOf(text, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
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
        const QStringList textList = timer->getScript().split("\n");
        const int total = textList.count();
        for (int index = 0; index < total; ++index) {
            // CHECK: This may NOT be an optimisation...!
            if (textList.at(index).isEmpty() || !textList.at(index).contains(text, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) {
                // Short-cuts that mean we do not have to examine the line in more detail
                continue;
            }

            int instance = 0;
            startPos = 0;
            while ((startPos = textList.at(index).indexOf(text, startPos, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
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
            recursiveSearchTimers(timer, text);
        }
    }
}

void dlgTriggerEditor::recursiveSearchKeys(TKey* pTriggerParent, const QString& text)
{
    std::list<TKey*>* childrenList = pTriggerParent->getChildrenList();
    for (auto key : *childrenList) {
        QTreeWidgetItem* pItem;
        QTreeWidgetItem* parent = nullptr;
        const QString name = key->getName();
        int startPos = 0;

        if ((startPos = name.indexOf(text, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
            QStringList sl;
            sl << tr("Key") << name << tr("Name");
            // This part can never have a parent as it is the first part of this item
            parent = new QTreeWidgetItem(sl);
            setAllSearchData(parent, EditorViewType::cmKeysView, name, key->getID(), SearchResultIsName, startPos);
            treeWidget_searchResults->addTopLevelItem(parent);
        }

        // The simple "command"
        // TODO: (A) Revise to count multiple instances of search string within command?
        if ((startPos = key->getCommand().indexOf(text, 0, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
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
        const QStringList textList = key->getScript().split("\n");
        const int total = textList.count();
        for (int index = 0; index < total; ++index) {
            // CHECK: This may NOT be an optimisation...!
            if (textList.at(index).isEmpty() || !textList.at(index).contains(text, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) {
                // Short-cuts that mean we do not have to examine the line in more detail
                continue;
            }

            int instance = 0;
            startPos = 0;
            while ((startPos = textList.at(index).indexOf(text, startPos, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) != -1) {
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
            recursiveSearchKeys(key, text);
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
    clearAliasForm();
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
    clearActionForm();
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
        const int cindex = pParent->indexOfChild(pItem);
        if (cindex >= 0) {
            pParent->removeChild(pItem);
        }
    } else {
        qDebug() << "ERROR: dlgTriggerEditor::delete_action() child to be deleted does not have a parent";
    }

    mpCurrentVarItem = nullptr;
    clearVarForm();
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
    clearScriptForm();
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
    clearKeyForm();
}

void dlgTriggerEditor::delete_trigger()
{
    QTreeWidgetItem* pItem = treeWidget_triggers->currentItem();
    if (!pItem) {
        return;
    }
    QTreeWidgetItem* pParent = pItem->parent();
    int ID = pItem->data(0, Qt::UserRole).toInt();
    TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(ID);
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
    clearTriggerForm();
}

void dlgTriggerEditor::deleteTriggerCommand()
{
    QTreeWidgetItem* pItem = treeWidget_triggers->currentItem();
    QTreeWidgetItem* parent = nullptr;
    TriggerUnit* triggerUnit = mpHost->getTriggerUnit();
    dlgTriggerEditor* editor = this;

    DeleteTriggerCommand* command = new DeleteTriggerCommand(pItem, triggerUnit, treeWidget_triggers);
    command->mpEditor = editor;
    command->mpHost = mpHost;
    undoStack->push(command);
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
    clearTimerForm();
}


void dlgTriggerEditor::activeToggle_trigger()
{
    QTreeWidgetItem* pItem = treeWidget_triggers->currentItem();
    if (!pItem) {
        return;
    }
    QIcon icon;
    QString itemDescription;

    TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(pItem->data(0, Qt::UserRole).toInt());
    if (!pT) {
        return;
    }

    pT->setIsActive(!pT->shouldBeActive());

    if (pT->isFilterChain()) {
        if (pT->isActive()) {
            itemDescription = descActiveFilterChain;
            if (pT->ancestorsActive()) {
                icon.addPixmap(QPixmap(qsl(":/icons/filter.png")), QIcon::Normal, QIcon::Off);
            } else {
                icon.addPixmap(QPixmap(qsl(":/icons/filter-grey.png")), QIcon::Normal, QIcon::Off);
                itemDescription = descInactiveParent.arg(itemDescription);
            }
        } else {
            itemDescription = descInactiveFilterChain;
            if (pT->ancestorsActive()) {
                icon.addPixmap(QPixmap(qsl(":/icons/filter-locked.png")), QIcon::Normal, QIcon::Off);
            } else {
                icon.addPixmap(QPixmap(qsl(":/icons/filter-grey-locked.png")), QIcon::Normal, QIcon::Off);
            }
        }
    } else if (pT->isFolder()) {
        if (pT->isActive()) {
            itemDescription = descActiveFolder;
            if (pT->ancestorsActive()) {
                icon.addPixmap(QPixmap(qsl(":/icons/folder-blue.png")), QIcon::Normal, QIcon::Off);
            } else {
                icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                itemDescription = descInactiveParent.arg(itemDescription);
            }
        } else {
            itemDescription = descInactiveFolder;
            if (pT->ancestorsActive()) {
                icon.addPixmap(QPixmap(qsl(":/icons/folder-blue-locked.png")), QIcon::Normal, QIcon::Off);
            } else {
                icon.addPixmap(QPixmap(qsl(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
            }
        }
    } else {
        if (pT->isActive()) {
            itemDescription = descActive;
            if (pT->ancestorsActive()) {
                icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
            } else {
                icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                itemDescription = descInactiveParent.arg(itemDescription);
            }
        } else {
            itemDescription = descInactive;
            if (pT->ancestorsActive()) {
                icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
            } else {
                icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
            }
        }
    }

    if (pT->state()) {
        if (pT->shouldBeActive()) {
            showInfo(tr(R"(Trying to activate a trigger group, filter or trigger or the part of a module "<tt>%1</tt>" that contains them <em>succeeded</em>.)").arg(pT->getName().toHtmlEscaped()));
        } else {
            showInfo(tr(R"(Trying to deactivate a trigger group, filter or trigger or the part of a module "<tt>%1</tt>" that contains them <em>succeeded</em>.)").arg(pT->getName().toHtmlEscaped()));
        }
    } else {
        pT->setIsActive(false);
        showError(tr(R"(<b>Unable to activate a filter or trigger or the part of a module "<tt>%1</tt>" that contains them; reason: %2.</b></p>
                     <p><i>You will need to reactivate this after the problem has been corrected.</i></p>)")
                          .arg(pT->getName().toHtmlEscaped(), pT->getError()));
        icon.addPixmap(QPixmap(qsl(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
        itemDescription = descError;
    }
    pItem->setIcon(0, icon);
    pItem->setText(0, pT->getName());
    pItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);

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
        QString itemDescription;
        if (pItem->childCount() > 0) {
            children_icon_triggers(pItem);
        }
        if (pT->state()) {
            if (pT->isFilterChain()) {
                if (pT->isActive()) {
                    itemDescription = descActiveFilterChain;
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/filter.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/filter-grey.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descInactiveFilterChain;
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/filter-locked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/filter-grey-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            } else if (pT->isFolder()) {
                if (pT->isActive()) {
                    itemDescription = descActiveFolder;
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-blue.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descInactiveFolder;
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-blue-locked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            } else {
                if (pT->isActive()) {
                    itemDescription = descActive;
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }

                } else {
                    itemDescription = descInactive;
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            pItem->setIcon(0, icon);
        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(qsl(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            itemDescription = descError;
            pItem->setIcon(0, iconError);
            showError(pT->getError());
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);
    }
}


void dlgTriggerEditor::activeToggle_timer()
{
    QTreeWidgetItem* pItem = treeWidget_timers->currentItem();
    if (!pItem) {
        return;
    }
    QIcon icon;
    QString itemDescription;

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
            itemDescription = descActiveFolder;
            if (pT->ancestorsActive()) {
                if (!pT->mPackageName.isEmpty()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-green.png")), QIcon::Normal, QIcon::Off);
                }
            } else {
                icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                itemDescription = descInactiveParent.arg(itemDescription);
            }
        } else {
            itemDescription = descInactiveFolder;
            if (pT->ancestorsActive()) {
                if (!pT->mPackageName.isEmpty()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-green-locked.png")), QIcon::Normal, QIcon::Off);
                }
            } else {
                icon.addPixmap(QPixmap(qsl(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
            }
        }
    } else {
        if (pT->isOffsetTimer()) {
            // state of offset timers is managed by the trigger engine
            if (pT->shouldBeActive()) {
                pT->enableTimer(pT->getID());
                itemDescription = descActiveOffsetTimer;
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/offsettimer-on.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/offsettimer-on-grey.png")), QIcon::Normal, QIcon::Off);
                    itemDescription = descInactiveParent.arg(itemDescription);
                }
            } else {
                pT->disableTimer(pT->getID());
                itemDescription = descInactiveOffsetTimer;
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/offsettimer-off.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/offsettimer-off-grey.png")), QIcon::Normal, QIcon::Off);
                }
            }
        } else {
            if (pT->shouldBeActive()) {
                pT->enableTimer(pT->getID());
                itemDescription = descActive;
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                    itemDescription = descInactiveParent.arg(itemDescription);
                }
            } else {
                pT->disableTimer(pT->getID());
                icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                itemDescription = descInactive;
            }
        }
    }

    if (pT->state()) {
        if (pT->shouldBeActive()) {
            showInfo(tr(R"(Trying to activate a timer group, offset timer, timer or the part of a module "<tt>%1</tt>" that contains them <em>succeeded</em>.)").arg(pT->getName().toHtmlEscaped()));
        } else {
            showInfo(tr(R"(Trying to deactivate a timer group, offset timer, timer or the part of a module "<tt>%1</tt>" that contains them <em>succeeded</em>.)").arg(pT->getName().toHtmlEscaped()));
        }
    } else {
        pT->setIsActive(false);
        showError(tr(R"(<p><b>Unable to activate an offset timer or timer or the part of a module "<tt>%1</tt>" that contains them; reason: %2.</b></p>
                     <p><i>You will need to reactivate this after the problem has been corrected.</i></p>)")
                          .arg(pT->getName().toHtmlEscaped(), pT->getError()));
        icon.addPixmap(QPixmap(qsl(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
        itemDescription = descError;
    }
    pItem->setIcon(0, icon);
    pItem->setText(0, pT->getName());
    pItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);

    if (pItem->childCount() > 0) {
        children_icon_timer(pItem);
    }
}

void dlgTriggerEditor::children_icon_timer(QTreeWidgetItem* pWidgetItemParent)
{
    for (int i = 0; i < pWidgetItemParent->childCount(); i++) {
        QTreeWidgetItem* pItem = pWidgetItemParent->child(i);
        TTimer* pT = mpHost->getTimerUnit()->getTimer(pItem->data(0, Qt::UserRole).toInt());
        if (!pT) {
            return;
        }

        QIcon icon;
        QString itemDescription;
        const bool itemActive = (pT->isActive() || pT->shouldBeActive());

        if (pItem->childCount() > 0) {
            children_icon_timer(pItem);
        }
        if (pT->state()) {
            if (pT->isFolder()) {
                itemDescription = (itemActive ? descActiveFolder : descInactiveFolder);
                if (itemActive) {
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-green.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-green-locked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            } else {
                if (pT->isOffsetTimer()) {
                    if (pT->shouldBeActive()) {
                        itemDescription = descActiveOffsetTimer;
                        if (pT->ancestorsActive()) {
                            icon.addPixmap(QPixmap(qsl(":/icons/offsettimer-on.png")), QIcon::Normal, QIcon::Off);
                        } else {
                            icon.addPixmap(QPixmap(qsl(":/icons/offsettimer-on-grey.png")), QIcon::Normal, QIcon::Off);
                            itemDescription = descInactiveParent.arg(itemDescription);
                        }
                    } else {
                        itemDescription = descInactiveOffsetTimer;
                        if (pT->ancestorsActive()) {
                            icon.addPixmap(QPixmap(qsl(":/icons/offsettimer-off.png")), QIcon::Normal, QIcon::Off);
                        } else {
                            icon.addPixmap(QPixmap(qsl(":/icons/offsettimer-off-grey.png")), QIcon::Normal, QIcon::Off);
                            itemDescription = descInactiveParent.arg(itemDescription);
                        }
                    }
                } else {
                    if (itemActive) {
                        itemDescription = descActive;
                        if (pT->ancestorsActive()) {
                            icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                        } else {
                            icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                            itemDescription = descInactiveParent.arg(itemDescription);
                        }
                    } else {
                        itemDescription = descInactive;
                        if (pT->ancestorsActive()) {
                            icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                        } else {
                            icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                        }
                    }
                }
            }
            pItem->setIcon(0, icon);
        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(qsl(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            itemDescription = descError;
            pItem->setIcon(0, iconError);
            showError(pT->getError());
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);
    }
}

void dlgTriggerEditor::activeToggle_alias()
{
    QTreeWidgetItem* pItem = treeWidget_aliases->currentItem();
    if (!pItem) {
        return;
    }
    QIcon icon;
    QString itemDescription;

    TAlias* pT = mpHost->getAliasUnit()->getAlias(pItem->data(0, Qt::UserRole).toInt());
    if (!pT) {
        return;
    }
    pT->setIsActive(!pT->shouldBeActive());

    if (pT->isFolder()) {
        if (pT->isActive()) {
            icon.addPixmap(QPixmap(qsl(":/icons/folder-violet.png")), QIcon::Normal, QIcon::Off);
            itemDescription = descActiveFolder;
        } else {
            icon.addPixmap(QPixmap(qsl(":/icons/folder-violet-locked.png")), QIcon::Normal, QIcon::Off);
            itemDescription = descInactiveFolder;
        }
    } else {
        if (pT->isActive()) {
            itemDescription = descActive;
            if (pT->ancestorsActive()) {
                icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
            } else {
                icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                itemDescription = descInactiveParent.arg(itemDescription);
            }
        } else {
            icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
            itemDescription = descInactive;
        }
    }

    if (pT->state()) {
        if (pT->shouldBeActive()) {
            showInfo(tr(R"(Trying to activate an alias group, alias or the part of a module "<tt>%1</tt>" that contains them <em>succeeded</em>.)").arg(pT->getName().toHtmlEscaped()));
        } else {
            showInfo(tr(R"(Trying to deactivate an alias group, alias or the part of a module "<tt>%1</tt>" that contains them <em>succeeded</em>.)").arg(pT->getName().toHtmlEscaped()));
        }
    } else {
        pT->setIsActive(false);
        showError(tr(R"(<p><b>Unable to activate an alias or the part of a module "<tt>%1</tt>" that contains them; reason: %2.</b></p>
                     <p><i>You will need to reactivate this after the problem has been corrected.</i></p>)")
                          .arg(pT->getName().toHtmlEscaped(), pT->getError()));
        icon.addPixmap(QPixmap(qsl(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
        itemDescription = descError;
    }
    pItem->setIcon(0, icon);
    pItem->setText(0, pT->getName());
    pItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);

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
        QString itemDescription;
        if (pItem->childCount() > 0) {
            children_icon_alias(pItem);
        }
        if (pT->state()) {
            if (pT->isFolder()) {
                if (pT->isActive()) {
                    itemDescription = descActiveFolder;
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-violet.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descInactiveFolder;
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-violet-locked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            } else {
                if (pT->isActive()) {
                    itemDescription = descActive;
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }

                } else {
                    itemDescription = descInactive;
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            pItem->setIcon(0, icon);
        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(qsl(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            itemDescription = descError;
            pItem->setIcon(0, iconError);
            showError(pT->getError());
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);
    }
}


void dlgTriggerEditor::activeToggle_script()
{
    QTreeWidgetItem* pItem = treeWidget_scripts->currentItem();
    if (!pItem) {
        return;
    }
    QIcon icon;
    QString itemDescription;

    TScript* pT = mpHost->getScriptUnit()->getScript(pItem->data(0, Qt::UserRole).toInt());
    if (!pT) {
        return;
    }

    pT->setIsActive(!pT->shouldBeActive());

    if (pT->isFolder()) {
        if (pT->isActive()) {
            icon.addPixmap(QPixmap(qsl(":/icons/folder-orange.png")), QIcon::Normal, QIcon::Off);
            itemDescription = descActiveFolder;
        } else {
            icon.addPixmap(QPixmap(qsl(":/icons/folder-orange-locked.png")), QIcon::Normal, QIcon::Off);
            itemDescription = descInactiveFolder;
        }
    } else {
        if (pT->isActive()) {
            itemDescription = descActive;
            if (pT->ancestorsActive()) {
                icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
            } else {
                icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                itemDescription = descInactiveParent.arg(itemDescription);
            }
        } else {
            icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
            itemDescription = descInactive;
        }
    }

    if (pT->state()) {
        if (pT->shouldBeActive()) {
            showInfo(tr(R"(Trying to activate a script group, script or the part of a module "<tt>%1</tt>" that contains them <em>succeeded</em>.)").arg(pT->getName().toHtmlEscaped()));
        } else {
            showInfo(tr(R"(Trying to deactivate a script group, script or the part of a module "<tt>%1</tt>" that contains them <em>succeeded</em>.)").arg(pT->getName().toHtmlEscaped()));
        }
    } else {
        pT->setIsActive(false);
        showError(tr(R"(<p><b>Unable to activate a script group or script or the part of a module "<tt>%1</tt>" that contains them; reason: %2.</b></p>
                     <p><i>You will need to reactivate this after the problem has been corrected.</i></p>)")
                          .arg(pT->getName().toHtmlEscaped(), pT->getError()));
        icon.addPixmap(QPixmap(qsl(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
        itemDescription = descError;
    }
    pItem->setIcon(0, icon);
    pItem->setText(0, pT->getName());
    pItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);
    if (pItem->childCount() > 0) {
        children_icon_script(pItem);
    }
}

void dlgTriggerEditor::children_icon_script(QTreeWidgetItem* pWidgetItemParent)
{
    for (int i = 0; i < pWidgetItemParent->childCount(); i++) {
        QTreeWidgetItem* pItem = pWidgetItemParent->child(i);
        TScript* pT = mpHost->getScriptUnit()->getScript(pItem->data(0, Qt::UserRole).toInt());
        if (!pT) {
            return;
        }

        QIcon icon;
        QString itemDescription;
        if (pItem->childCount() > 0) {
            children_icon_script(pItem);
        }
        if (pT->state()) {
            if (pT->isFolder()) {
                if (pT->isActive()) {
                    itemDescription = descActiveFolder;
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-orange.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descInactiveFolder;
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-orange-locked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            } else {
                if (pT->isActive()) {
                    itemDescription = descActive;
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descInactive;
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            pItem->setIcon(0, icon);
        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(qsl(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            itemDescription = descError;
            pItem->setIcon(0, iconError);
            showError(pT->getError());
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);
    }
}


void dlgTriggerEditor::activeToggle_action()
{
    QTreeWidgetItem* pItem = treeWidget_actions->currentItem();
    if (!pItem) {
        return;
    }
    QIcon icon;
    QString itemDescription;

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

    const bool itemActive = pT->isActive();
    if (pT->isFolder()) {
        itemDescription = (itemActive ? descActiveFolder : descInactiveFolder);
        if (!pT->ancestorsActive()) {
            // It is okay to test for being inactiveed by an ancestor before testing whether
            // the item is a package/module as those are not expected to have any parents to
            // be inactive.
            if (itemActive) {
                icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                itemDescription = descInactiveParent.arg(itemDescription);
            } else {
                icon.addPixmap(QPixmap(qsl(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
            }
        } else if (!pT->mPackageName.isEmpty()) {
            // Has a package name - is a module or package master folder
            if (itemActive) {
                icon.addPixmap(QPixmap(qsl(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
            } else {
                icon.addPixmap(QPixmap(qsl(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
            }
        } else if (!pT->getParent() || !pT->getParent()->mPackageName.isEmpty()) {
            // Does not have a parent or the parent has a package name - is a toolbar
            if (itemActive) {
                icon.addPixmap(QPixmap(qsl(":/icons/folder-yellow.png")), QIcon::Normal, QIcon::Off);
            } else {
                icon.addPixmap(QPixmap(qsl(":/icons/folder-yellow-locked.png")), QIcon::Normal, QIcon::Off);
            }
        } else {
            // Must be a menu
            if (itemActive) {
                icon.addPixmap(QPixmap(qsl(":/icons/folder-cyan.png")), QIcon::Normal, QIcon::Off);
            } else {
                icon.addPixmap(QPixmap(qsl(":/icons/folder-cyan-locked.png")), QIcon::Normal, QIcon::Off);
            }
        }
    } else {
        if (itemActive) {
            itemDescription = descActive;
            if (pT->ancestorsActive()) {
                icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
            } else {
                icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                itemDescription = descInactiveParent.arg(itemDescription);
            }
        } else {
            icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
            itemDescription = descInactive;
        }
    }

    if (pT->state()) {
        if (pT->shouldBeActive()) {
            showInfo(tr(R"(Trying to activate a button/menu/toolbar or the part of a module "<tt>%1</tt>" that contains them <em>succeeded</em>.)").arg(pT->getName().toHtmlEscaped()));
        } else {
            showInfo(tr(R"(Trying to deactivate a button/menu/toolbar or the part of a module "<tt>%1</tt>" that contains them <em>succeeded</em>.)").arg(pT->getName().toHtmlEscaped()));
        }
    } else {
        pT->setIsActive(false);
        showError(tr(R"(<p><b>Unable to activate a button/menu/toolbar or the part of a module "<tt>%1</tt>" that contains them; reason: %2.</b></p>
                     <p><i>You will need to reactivate this after the problem has been corrected.</i></p>)")
                          .arg(pT->getName().toHtmlEscaped(), pT->getError()));
        icon.addPixmap(QPixmap(qsl(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
        itemDescription = descError;
    }
    pItem->setIcon(0, icon);
    pItem->setText(0, pT->getName());
    pItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);

    mpHost->getActionUnit()->updateToolbar();
    if (pItem->childCount() > 0) {
        children_icon_action(pItem);
    }
}

void dlgTriggerEditor::children_icon_action(QTreeWidgetItem* pWidgetItemParent)
{
    for (int i = 0; i < pWidgetItemParent->childCount(); i++) {
        QTreeWidgetItem* pItem = pWidgetItemParent->child(i);
        TAction* pT = mpHost->getActionUnit()->getAction(pItem->data(0, Qt::UserRole).toInt());
        if (!pT) {
            return;
        }

        QIcon icon;
        QString itemDescription;
        const bool itemActive = pT->isActive();
        if (pItem->childCount() > 0) {
            children_icon_action(pItem);
        }
        if (pT->state()) {
            if (pT->isFolder()) {
                itemDescription = (itemActive ? descActiveFolder : descInactiveFolder);
                if (!pT->mPackageName.isEmpty()) {
                    // Has a package name - is a module or package master
                    // folder
                    if (pT->isActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                } else if (!pT->ancestorsActive()) {
                    if (pT->isActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descInactiveParent.arg(itemDescription);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                } else if (!pT->getParent() || !pT->getParent()->mPackageName.isEmpty()) {
                    // Does not have a parent or the parent has a package name
                    // so the parent is a module or package master folder - so
                    // this is a toolbar:
                    if (pT->isActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-yellow.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-yellow-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                } else {
                    // Must be a menu
                    if (pT->isActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-cyan.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-cyan-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            } else {
                if (pT->isActive()) {
                    itemDescription = descActive;
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }

                } else {
                    itemDescription = descInactive;
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            pItem->setIcon(0, icon);
        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(qsl(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            itemDescription = descError;
            pItem->setIcon(0, iconError);
            showError(pT->getError());
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);
    }
}


void dlgTriggerEditor::activeToggle_key()
{
    QTreeWidgetItem* pItem = treeWidget_keys->currentItem();
    if (!pItem) {
        return;
    }
    QIcon icon;
    QString itemDescription;

    TKey* pT = mpHost->getKeyUnit()->getKey(pItem->data(0, Qt::UserRole).toInt());
    if (!pT) {
        return;
    }

    pT->setIsActive(!pT->shouldBeActive());

    if (pT->isFolder()) {
        if (pT->isActive()) {
            itemDescription = descActiveFolder;
            if (pT->ancestorsActive()) {
                icon.addPixmap(QPixmap(qsl(":/icons/folder-pink.png")), QIcon::Normal, QIcon::Off);
            } else {
                icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                itemDescription = descInactiveParent.arg(itemDescription);
            }
        } else {
            itemDescription = descInactiveFolder;
            if (pT->ancestorsActive()) {
                icon.addPixmap(QPixmap(qsl(":/icons/folder-pink-locked.png")), QIcon::Normal, QIcon::Off);
            } else {
                icon.addPixmap(QPixmap(qsl(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
            }
        }
    } else {
        if (pT->isActive()) {
            itemDescription = descActive;
            if (pT->ancestorsActive()) {
                icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
            } else {
                icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                itemDescription = descInactiveParent.arg(itemDescription);
            }
        } else {
            itemDescription = descInactive;
            if (pT->ancestorsActive()) {
                icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
            } else {
                icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
            }
        }
    }

    if (pT->state()) {
        pItem->setIcon(0, icon);
        pItem->setText(0, pT->getName());
    } else {
        QIcon iconError;
        iconError.addPixmap(QPixmap(qsl(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
        itemDescription = descError;
        pItem->setIcon(0, iconError);
    }
    pItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);
    showInfo(QString("Trying to %2 key <em>%1</em> %3.")
                     .arg(pT->getName().toHtmlEscaped(), pT->shouldBeActive() ? "activate" : "deactivate", pT->state() ? "succeeded" : QString("failed; reason: %1").arg(pT->getError())));
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
        QString itemDescription;
        if (pItem->childCount() > 0) {
            children_icon_key(pItem);
        }
        if (pT->state()) {
            if (pT->isFolder()) {
                if (pT->isActive()) {
                    itemDescription = descActiveFolder;
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-pink.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descInactiveFolder;
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-pink-locked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            } else {
                if (pT->isActive()) {
                    itemDescription = descActive;
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }

                } else {
                    itemDescription = descInactive;
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            pItem->setIcon(0, icon);
        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(qsl(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            itemDescription = descError;
            pItem->setIcon(0, iconError);
            showError(pT->getError());
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);
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
    const QStringList patterns;
    QList<int> const patternKinds;
    const QString script = "";
    QStringList nameL;
    nameL << name;

    QTreeWidgetItem* pParent = treeWidget_triggers->currentItem();
    QTreeWidgetItem* pNewItem = nullptr;
    TTrigger* pT = nullptr;

    if (pParent) {
        const int parentID = pParent->data(0, Qt::UserRole).toInt();

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
        pT = new TTrigger(name, patterns, patternKinds, false, mpHost);
        pNewItem = new QTreeWidgetItem(mpTriggerBaseItem, nameL);
        treeWidget_triggers->insertTopLevelItem(0, pNewItem);
    }

    if (!pT) {
        return;
    }


    pT->setName(name);
    pT->setRegexCodeList(patterns, patternKinds);
    pT->setScript(script);
    pT->setIsFolder(isFolder);
    pT->setIsActive(false);
    pT->setIsMultiline(false);
    pT->mStayOpen = 0;
    pT->setConditionLineDelta(0);
    pT->registerTrigger();
    const int childID = pT->getID();
    pNewItem->setData(0, Qt::UserRole, childID);
    QIcon icon;
    QString itemDescription;
    if (isFolder) {
        itemDescription = descNewFolder;
        icon.addPixmap(QPixmap(qsl(":/icons/folder-red.png")), QIcon::Normal, QIcon::Off);
    } else {
        itemDescription = descNewItem;
        icon.addPixmap(QPixmap(qsl(":/icons/document-save-as.png")), QIcon::Normal, QIcon::Off);
    }
    pNewItem->setIcon(0, icon);
    pNewItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);
    if (pParent) {
        pParent->setExpanded(true);
    }
    mpTriggersMainArea->lineEdit_trigger_name->clear();
    mpTriggersMainArea->label_idNumber->clear();
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
    slot_triggerSelected(treeWidget_triggers->currentItem());
}

void dlgTriggerEditor::addTriggerCommand(bool isFolder)
{
    QTreeWidgetItem* pItem = nullptr;
    QTreeWidgetItem* parent = nullptr;
    TriggerUnit* triggerUnit = mpHost->getTriggerUnit();
    dlgTriggerEditor* editor = this;
    AddTriggerCommand* command = new AddTriggerCommand(pItem, triggerUnit, treeWidget_triggers, isFolder);
    command->mpEditor = editor;
    undoStack->push(command);
}

void dlgTriggerEditor::addAliasCommand(bool isFolder)
{
    QTreeWidgetItem* pItem = nullptr;
    QTreeWidgetItem* parent = nullptr;
    AliasUnit* aliasUnit = mpHost->getAliasUnit();
    dlgTriggerEditor* editor = this;
    AddAliasCommand* command = new AddAliasCommand(pItem, aliasUnit, treeWidget_aliases, isFolder);
    command->mpEditor = editor;
    undoStack->push(command);
}

void dlgTriggerEditor::deleteAliasCommand()
{
    QTreeWidgetItem* pItem = treeWidget_aliases->currentItem();
    QTreeWidgetItem* parent = nullptr;
    AliasUnit* aliasUnit = mpHost->getAliasUnit();
    dlgTriggerEditor* editor = this;
    DeleteAliasCommand* command = new DeleteAliasCommand(pItem, aliasUnit, treeWidget_aliases);
    command->mpEditor = editor;
    command->mpHost = mpHost;
    undoStack->push(command);
}

void dlgTriggerEditor::deleteScriptCommand()
{
    QTreeWidgetItem* pItem = treeWidget_scripts->currentItem();
    QTreeWidgetItem* parent = nullptr;
    ScriptUnit* scriptUnit = mpHost->getScriptUnit();
    dlgTriggerEditor* editor = this;
    DeleteScriptCommand* command = new DeleteScriptCommand(pItem, scriptUnit, treeWidget_scripts);
    command->mpEditor = editor;
    command->mpHost = mpHost;
    undoStack->push(command);
}

void dlgTriggerEditor::deleteKeyCommand()
{
    QTreeWidgetItem* pItem = treeWidget_keys->currentItem();
    QTreeWidgetItem* parent = nullptr;
    KeyUnit* keyUnit = mpHost->getKeyUnit();
    dlgTriggerEditor* editor = this;
    DeleteKeyCommand* command = new DeleteKeyCommand(pItem, keyUnit, treeWidget_keys);
    command->mpEditor = editor;
    command->mpHost = mpHost;
    undoStack->push(command);
}

void dlgTriggerEditor::addTimerCommand(bool isFolder)
{
    QTreeWidgetItem* pItem = nullptr;
    QTreeWidgetItem* parent = nullptr;
    TimerUnit* timerUnit = mpHost->getTimerUnit();
    dlgTriggerEditor* editor = this;
    AddTimerCommand* command = new AddTimerCommand(pItem, timerUnit, treeWidget_timers, isFolder);
    command->mpEditor = editor;
    undoStack->push(command);
}

void dlgTriggerEditor::addScriptCommand(bool isFolder)
{
    QTreeWidgetItem* pItem = nullptr;
    QTreeWidgetItem* parent = nullptr;
    ScriptUnit* scriptUnit = mpHost->getScriptUnit();
    dlgTriggerEditor* editor = this;
    AddScriptCommand* command = new AddScriptCommand(pItem, scriptUnit, treeWidget_scripts, isFolder);
    command->mpEditor = editor;
    undoStack->push(command);
}

void dlgTriggerEditor::addKeyCommand(bool isFolder)
{
    QTreeWidgetItem* pItem = nullptr;
    QTreeWidgetItem* parent = nullptr;
    KeyUnit* keyUnit = mpHost->getKeyUnit();
    dlgTriggerEditor* editor = this;
    AddKeyCommand* command = new AddKeyCommand(pItem, keyUnit, treeWidget_keys, isFolder);
    command->mpEditor = editor;
    undoStack->push(command);
}

void dlgTriggerEditor::addActionCommand(bool isFolder)
{
    QTreeWidgetItem* pItem = nullptr;
    QTreeWidgetItem* parent = nullptr;
    ActionUnit* actionUnit = mpHost->getActionUnit();
    dlgTriggerEditor* editor = this;
    AddActionCommand* command = new AddActionCommand(pItem, actionUnit, treeWidget_actions, isFolder);
    command->mpEditor = editor;
    undoStack->push(command);
}

void dlgTriggerEditor::addVarCommand(bool isFolder)
{
    QTreeWidgetItem* pItem = nullptr;
    QTreeWidgetItem* parent = nullptr;
    LuaInterface* lI = mpHost->getLuaInterface();
    VarUnit* varUnit = lI->getVarUnit();
    dlgTriggerEditor* editor = this;
    AddVarCommand* command = new AddVarCommand(pItem, varUnit, treeWidget_variables, isFolder);
    command->mpEditor = editor;
    undoStack->push(command);
}

void dlgTriggerEditor::deleteActionCommand()
{
    QTreeWidgetItem* pItem = treeWidget_actions->currentItem();
    QTreeWidgetItem* parent = nullptr;
    ActionUnit* actionUnit = mpHost->getActionUnit();
    dlgTriggerEditor* editor = this;
    DeleteActionCommand* command = new DeleteActionCommand(pItem, actionUnit, treeWidget_actions);
    command->mpEditor = editor;
    command->mpHost = mpHost;
    undoStack->push(command);
}

void dlgTriggerEditor::deleteVarCommand()
{
    QTreeWidgetItem* pItem = treeWidget_variables->currentItem();
    QTreeWidgetItem* parent = nullptr;
    LuaInterface* lI = mpHost->getLuaInterface();
    VarUnit* varUnit = lI->getVarUnit();
    dlgTriggerEditor* editor = this;
    DeleteVarCommand* command = new DeleteVarCommand(pItem, varUnit, treeWidget_variables);
    command->mpEditor = editor;
    command->mpHost = mpHost;
    undoStack->push(command);
}

void dlgTriggerEditor::deleteTimerCommand()
{
    QTreeWidgetItem* pItem = treeWidget_timers->currentItem();
    QTreeWidgetItem* parent = nullptr;
    TimerUnit* timerUnit = mpHost->getTimerUnit();
    dlgTriggerEditor* editor = this;
    DeleteTimerCommand* command = new DeleteTimerCommand(pItem, timerUnit, treeWidget_timers);
    command->mpEditor = editor;
    command->mpHost = mpHost;
    undoStack->push(command);
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
    const QString command = "";
    const QTime time;
    const QString script = "";
    QStringList nameL;
    nameL << name;

    QTreeWidgetItem* pParent = treeWidget_timers->currentItem();
    QTreeWidgetItem* pNewItem = nullptr;
    TTimer* pT = nullptr;

    if (pParent) {
        const int parentID = pParent->data(0, Qt::UserRole).toInt();

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
    const int childID = pT->getID();
    pNewItem->setData(0, Qt::UserRole, childID);
    QIcon icon;

    QString itemDescription;
    if (isFolder) {
        itemDescription = descNewFolder;
        icon.addPixmap(QPixmap(qsl(":/icons/folder-red.png")), QIcon::Normal, QIcon::Off);
    } else {
        itemDescription = descNewItem;
        icon.addPixmap(QPixmap(qsl(":/icons/document-save-as.png")), QIcon::Normal, QIcon::Off);
    }
    pNewItem->setIcon(0, icon);
    pNewItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);
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
    slot_timerSelected(treeWidget_timers->currentItem());
}

void dlgTriggerEditor::addVar(bool isFolder)
{
    saveVar();
    mpVarsMainArea->comboBox_variable_key_type->setCurrentIndex(0);
    if (isFolder) {
        // in lieu of readonly
        mpSourceEditorEdbee->setEnabled(false);
        mpVarsMainArea->comboBox_variable_value_type->setDisabled(true);
        mpVarsMainArea->comboBox_variable_value_type->setCurrentIndex(4);
        mpVarsMainArea->lineEdit_var_name->setText(QString());
        mpVarsMainArea->lineEdit_var_name->setPlaceholderText(tr("Table name..."));

        clearDocument(mpSourceEditorEdbee, QLatin1String("NewTable"));
    } else {
        // in lieu of readonly
        mpSourceEditorEdbee->setEnabled(true);
        mpVarsMainArea->lineEdit_var_name->setText(QString());
        mpVarsMainArea->lineEdit_var_name->setPlaceholderText(tr("Variable name..."));
        mpVarsMainArea->comboBox_variable_value_type->setDisabled(false);
        mpVarsMainArea->comboBox_variable_value_type->setCurrentIndex(0);
    }

    LuaInterface* lI = mpHost->getLuaInterface();
    VarUnit* vu = lI->getVarUnit();

    QStringList nameL;
    nameL << QString(isFolder ? tr("New table name") : tr("New variable name"));

    QTreeWidgetItem* pParent = nullptr;
    QTreeWidgetItem* pNewItem;
    QTreeWidgetItem* cItem = treeWidget_variables->currentItem();
    if (cItem) {
        TVar* cVar = vu->getWVar(cItem);
        if (cVar && cVar->getValueType() == LUA_TTABLE) {
            pParent = cItem;
        } else {
            pParent = cItem->parent();
        }
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

    mpCurrentVarItem = pNewItem;
    treeWidget_variables->setCurrentItem(pNewItem);
    showInfo(msgInfoAddVar);
    slot_variableSelected(treeWidget_variables->currentItem());
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
    const QString script = "";
    QStringList nameL;
    nameL << name;

    QTreeWidgetItem* pParent = treeWidget_keys->currentItem();
    QTreeWidgetItem* pNewItem = nullptr;
    TKey* pT = nullptr;

    if (pParent) {
        const int parentID = pParent->data(0, Qt::UserRole).toInt();

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
    pT->setKeyCode(Qt::Key_unknown);
    pT->setKeyModifiers(Qt::NoModifier);
    pT->setScript(script);
    pT->setIsFolder(isFolder);
    pT->setIsActive(false);
    pT->registerKey();
    const int childID = pT->getID();
    pNewItem->setData(0, Qt::UserRole, childID);
    QIcon icon;
    QString itemDescription;
    if (isFolder) {
        itemDescription = descNewFolder;
        icon.addPixmap(QPixmap(qsl(":/icons/folder-red.png")), QIcon::Normal, QIcon::Off);
    } else {
        itemDescription = descNewItem;
        icon.addPixmap(QPixmap(qsl(":/icons/document-save-as.png")), QIcon::Normal, QIcon::Off);
    }
    pNewItem->setIcon(0, icon);
    pNewItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);
    if (pParent) {
        pParent->setExpanded(true);
    }
    mpKeysMainArea->lineEdit_key_command->clear();
    mpKeysMainArea->lineEdit_key_binding->setText("no key chosen");
    clearDocument(mpSourceEditorEdbee); // New Key
    mpCurrentKeyItem = pNewItem;
    treeWidget_keys->setCurrentItem(pNewItem);
    showInfo(msgInfoAddKey);
    slot_keySelected(treeWidget_keys->currentItem());
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
    const QString regex = "";
    const QString command = "";
    const QString script = "";
    QStringList nameL;
    nameL << name;

    QTreeWidgetItem* pParent = treeWidget_aliases->currentItem();
    QTreeWidgetItem* pNewItem = nullptr;
    TAlias* pT = nullptr;

    if (pParent) {
        const int parentID = pParent->data(0, Qt::UserRole).toInt();

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
    const int childID = pT->getID();
    pNewItem->setData(0, Qt::UserRole, childID);
    QIcon icon;
    QString itemDescription;
    if (isFolder) {
        itemDescription = descNewFolder;
        icon.addPixmap(QPixmap(qsl(":/icons/folder-red.png")), QIcon::Normal, QIcon::Off);
    } else {
        itemDescription = descNewItem;
        icon.addPixmap(QPixmap(qsl(":/icons/document-save-as.png")), QIcon::Normal, QIcon::Off);
    }
    pNewItem->setIcon(0, icon);
    pNewItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);
    if (pParent) {
        pParent->setExpanded(true);
    }

    mpAliasMainArea->lineEdit_alias_name->clear();
    mpAliasMainArea->label_idNumber->clear();
    mpAliasMainArea->lineEdit_alias_pattern->clear();
    mpAliasMainArea->lineEdit_alias_command->clear();
    clearDocument(mpSourceEditorEdbee); // New Alias

    mpAliasMainArea->lineEdit_alias_name->setText(name);
    mpAliasMainArea->label_idNumber->setText(QString::number(childID));

    mpCurrentAliasItem = pNewItem;
    treeWidget_aliases->setCurrentItem(pNewItem);
    showInfo(msgInfoAddAlias);
    slot_aliasSelected(treeWidget_aliases->currentItem());
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
    const QString cmdButtonUp = "";
    const QString cmdButtonDown = "";
    const QString script = "";
    QStringList nameL;
    nameL << name;

    QTreeWidgetItem* pParent = treeWidget_actions->currentItem();
    QTreeWidgetItem* pNewItem = nullptr;
    QPointer<TAction> pT = nullptr;

    if (pParent) {
        const int parentID = pParent->data(0, Qt::UserRole).toInt();

        TAction* pParentAction = mpHost->getActionUnit()->getAction(parentID);
        if (pParentAction) {
            // insert new items as siblings unless the parent is a folder
            if (pParentAction->isFolder()) {
                pT = new TAction(pParentAction, mpHost);
                pNewItem = new QTreeWidgetItem(pParent, nameL);
                pParent->insertChild(0, pNewItem);
            } else if (pParentAction->getParent() && pParent->parent()) {
                pT = new TAction(pParentAction->getParent(), mpHost);
                pNewItem = new QTreeWidgetItem(pParent->parent(), nameL);
                pParent->parent()->insertChild(0, pNewItem);
            }
        }
    }
    // Otherwise: insert a new root item
    if (!pT) {
        name = tr("New toolbar");
        pT = new TAction(name, mpHost);
        pT->setCommandButtonUp(cmdButtonUp);
        QStringList nl;
        nl << name;
        pNewItem = new QTreeWidgetItem(mpActionBaseItem, nl);
        treeWidget_actions->insertTopLevelItem(0, pNewItem);
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
    const int childID = pT->getID();
    pNewItem->setData(0, Qt::UserRole, childID);
    QIcon icon;
    QString itemDescription;
    if (isFolder) {
        itemDescription = descNewFolder;
        icon.addPixmap(QPixmap(qsl(":/icons/folder-red.png")), QIcon::Normal, QIcon::Off);
    } else {
        itemDescription = descNewItem;
        icon.addPixmap(QPixmap(qsl(":/icons/document-save-as.png")), QIcon::Normal, QIcon::Off);
    }
    pNewItem->setIcon(0, icon);
    pNewItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);
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
    slot_actionSelected(treeWidget_actions->currentItem());
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
    const QString script;
    QStringList nameL;
    nameL << name;

    QTreeWidgetItem* pParent = treeWidget_scripts->currentItem();
    QTreeWidgetItem* pNewItem = nullptr;
    TScript* pT = nullptr;

    if (pParent) {
        const int parentID = pParent->data(0, Qt::UserRole).toInt();

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
    const int childID = pT->getID();
    pNewItem->setData(0, Qt::UserRole, childID);
    QIcon icon;
    QString itemDescription;
    if (isFolder) {
        itemDescription = descNewFolder;
        icon.addPixmap(QPixmap(qsl(":/icons/folder-red.png")), QIcon::Normal, QIcon::Off);
    } else {
        itemDescription = descNewItem;
        icon.addPixmap(QPixmap(qsl(":/icons/document-save-as.png")), QIcon::Normal, QIcon::Off);
    }
    pNewItem->setIcon(0, icon);
    pNewItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);
    if (pParent) {
        pParent->setExpanded(true);
    }
    mpScriptsMainArea->lineEdit_script_name->clear();
    mpScriptsMainArea->label_idNumber->clear();

    clearDocument(mpSourceEditorEdbee, script);
    mpCurrentScriptItem = pNewItem;
    treeWidget_scripts->setCurrentItem(pNewItem);
    slot_scriptsSelected(treeWidget_scripts->currentItem());
}

void dlgTriggerEditor::selectTriggerByID(int id)
{
    slot_showTriggers();
    QTreeWidgetItemIterator it(treeWidget_triggers);
    while (*it) {
        if ((*it)->data(0, Qt::UserRole).toInt() == id) {
            slot_triggerSelected((*it));
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
    slot_showTimers();
    QTreeWidgetItemIterator it(treeWidget_timers);
    while (*it) {
        if ((*it)->data(0, Qt::UserRole).toInt() == id) {
            slot_timerSelected((*it));
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
    slot_showAliases();
    QTreeWidgetItemIterator it(treeWidget_aliases);
    while (*it) {
        if ((*it)->data(0, Qt::UserRole).toInt() == id) {
            slot_aliasSelected((*it));
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
    slot_showScripts();
    QTreeWidgetItemIterator it(treeWidget_scripts);
    while (*it) {
        if ((*it)->data(0, Qt::UserRole).toInt() == id) {
            slot_scriptsSelected((*it));
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
    slot_showActions();
    QTreeWidgetItemIterator it(treeWidget_actions);
    while (*it) {
        if ((*it)->data(0, Qt::UserRole).toInt() == id) {
            slot_actionSelected((*it));
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
    slot_showKeys();
    QTreeWidgetItemIterator it(treeWidget_keys);
    while (*it) {
        if ((*it)->data(0, Qt::UserRole).toInt() == id) {
            slot_keySelected((*it));
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
    const QString name = mpTriggersMainArea->lineEdit_trigger_name->text();
    mPrevTriggerName = mpTriggersMainArea->lineEdit_trigger_name->text();
    const QString command = mpTriggersMainArea->lineEdit_trigger_command->text();
    mPrevTriggerCommand = mpTriggersMainArea->lineEdit_trigger_command->text();
    const bool isMultiline = mpTriggersMainArea->groupBox_multiLineTrigger->isChecked();
    mPrevMultiLineTrigger = mpTriggersMainArea->groupBox_multiLineTrigger->isChecked();
    QStringList patterns;
    QList<int> patternKinds;
    for (int i = 0; i < 50; i++) {
        mpPrevTriggerPatternItemEdit[i] = mTriggerPatternEdit[i]->comboBox_patternType->currentIndex();
        mpPrevTriggerPatternEdit[i] = mTriggerPatternEdit[i]->lineEdit_pattern->text();
        mPrevLineSpacer[i] = mTriggerPatternEdit[i]->spinBox_lineSpacer->value();
        QString pattern = mTriggerPatternEdit.at(i)->lineEdit_pattern->text();

        // Spaces in the pattern may be marked with middle dots, convert them back
        unmarkQString(&pattern);

        const int patternType = mTriggerPatternEdit.at(i)->comboBox_patternType->currentIndex();
        if (pattern.isEmpty() && patternType != REGEX_PROMPT && patternType != REGEX_LINE_SPACER) {
            continue;
        }

        switch (patternType) {
        case 0:
            patternKinds << REGEX_SUBSTRING;
            break;
        case 1:
            patternKinds << REGEX_PERL;
            break;
        case 2:
            patternKinds << REGEX_BEGIN_OF_LINE_SUBSTRING;
            break;
        case 3:
            patternKinds << REGEX_EXACT_MATCH;
            break;
        case 4:
            patternKinds << REGEX_LUA_CODE;
            break;
        case 5:
            patternKinds << REGEX_LINE_SPACER;
            pattern = mTriggerPatternEdit.at(i)->spinBox_lineSpacer->text();
            break;
        case 6:
            patternKinds << REGEX_COLOR_PATTERN;
            break;
        case 7:
            patternKinds << REGEX_PROMPT;
            break;
        }
        patterns << pattern;
    }

    const QString script = mpSourceEditorEdbeeDocument->text();

    const int triggerID = pItem->data(0, Qt::UserRole).toInt();
    TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(triggerID);
    if (pT) {
        const QString old_name = pT->getName();
        pT->setName(name);
        pT->setCommand(command);
        pT->setRegexCodeList(patterns, patternKinds);

        pT->setScript(script);
        pT->setIsMultiline(isMultiline);
        pT->mPerlSlashGOption = mpTriggersMainArea->groupBox_perlSlashGOption->isChecked();
        mPrevPerlSlashGOption = mpTriggersMainArea->groupBox_perlSlashGOption->isChecked();
        pT->mFilterTrigger = mpTriggersMainArea->groupBox_filterTrigger->isChecked();
        mPrevFilterTrigger = mpTriggersMainArea->groupBox_filterTrigger->isChecked();
        pT->setConditionLineDelta(mpTriggersMainArea->spinBox_lineMargin->value());
        mPrevLineMargin = mpTriggersMainArea->spinBox_lineMargin->value();
        pT->mStayOpen = mpTriggersMainArea->spinBox_stayOpen->value();
        mPrevFireLength = mpTriggersMainArea->spinBox_stayOpen->value();
        pT->mSoundTrigger = mpTriggersMainArea->groupBox_soundTrigger->isChecked();
        mPrevGroupBox_soundTrigger = mpTriggersMainArea->groupBox_soundTrigger->isChecked();
        pT->setSound(mpTriggersMainArea->lineEdit_soundFile->text());
        mPrevLineEdit_soundFile = mpTriggersMainArea->lineEdit_soundFile->text();

        QColor fgColor(QColorConstants::Transparent);
        QColor bgColor(QColorConstants::Transparent);
        if (!mpTriggersMainArea->pushButtonFgColor->property(cButtonBaseColor).toString().isEmpty()) {
            fgColor = QColor(mpTriggersMainArea->pushButtonFgColor->property(cButtonBaseColor).toString());
            mPrevfgColor = fgColor;
        }
        pT->setColorizerFgColor(fgColor);
        if (!mpTriggersMainArea->pushButtonBgColor->property(cButtonBaseColor).toString().isEmpty()) {
            bgColor = QColor(mpTriggersMainArea->pushButtonBgColor->property(cButtonBaseColor).toString());
            mPrevbgColor = bgColor;
        }
        pT->setColorizerBgColor(bgColor);
        pT->setIsColorizerTrigger(mpTriggersMainArea->groupBox_triggerColorizer->isChecked());
        mPrevBox_triggerColorizer = mpTriggersMainArea->groupBox_triggerColorizer->isChecked();
        QIcon icon;
        QString itemDescription;
        if (pT->isFilterChain()) {
            if (pT->isActive()) {
                itemDescription = descActiveFilterChain;
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/filter.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/filter-grey.png")), QIcon::Normal, QIcon::Off);
                    itemDescription = descInactiveParent.arg(itemDescription);
                }
            } else {
                itemDescription = descInactiveFilterChain;
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/filter-locked.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/filter-grey-locked.png")), QIcon::Normal, QIcon::Off);
                }
            }
        } else if (pT->isFolder()) {
            if (!pT->mPackageName.isEmpty()) {
                if (pT->isActive()) {
                    itemDescription = descActiveFolder;
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
                    itemDescription = descInactiveFolder;
                }
            } else if (pT->isActive()) {
                itemDescription = descActiveFolder;
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-blue.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                    itemDescription = descInactiveParent.arg(itemDescription);
                }
            } else {
                itemDescription = descInactiveFolder;
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-blue-locked.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                }
            }
        } else {
            if (pT->isActive()) {
                itemDescription = descActive;
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                    itemDescription = descInactiveParent.arg(itemDescription);
                }
            } else {
                itemDescription = descInactive;
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                }
            }
        }
        if (pT->state()) {
            clearEditorNotification();

            if (old_name == tr("New trigger") || old_name == tr("New trigger group")) {
                if (pT->isFolder()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-blue.png")), QIcon::Normal, QIcon::Off);
                    itemDescription = descActiveFolder;
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    itemDescription = descActive;
                }
                pItem->setIcon(0, icon);
                pItem->setText(0, name);
                pT->setIsActive(true);
            } else {
                pItem->setIcon(0, icon);
                pItem->setText(0, name);
            }
        } else {
            QIcon iconError;
            pItem->setText(0, name);
            iconError.addPixmap(QPixmap(qsl(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            itemDescription = descError;
            pItem->setIcon(0, iconError);
            pT->setIsActive(false);
            showError(pT->getError());
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);
    }
}

void dlgTriggerEditor::saveTimer()
{
    QTreeWidgetItem* pItem = mpCurrentTimerItem;
    if (!pItem) {
        return;
    }

    mpTimersMainArea->trimName();
    const QString name = mpTimersMainArea->lineEdit_timer_name->text();
    const QString script = mpSourceEditorEdbeeDocument->text();


    const int timerID = pItem->data(0, Qt::UserRole).toInt();
    TTimer* pT = mpHost->getTimerUnit()->getTimer(timerID);
    if (pT) {
        pT->setName(name);
        const QString command = mpTimersMainArea->lineEdit_timer_command->text();
        const int hours = mpTimersMainArea->timeEdit_timer_hours->time().hour();
        const int minutes = mpTimersMainArea->timeEdit_timer_minutes->time().minute();
        const int secs = mpTimersMainArea->timeEdit_timer_seconds->time().second();
        const int msecs = mpTimersMainArea->timeEdit_timer_msecs->time().msec();
        const QTime time(hours, minutes, secs, msecs);
        pT->setTime(time);
        pT->setCommand(command);
        pT->setName(name);
        pT->setScript(script);

        QIcon icon;
        QString itemDescription;
        if (pT->isFolder()) {
            if (!pT->mPackageName.isEmpty()) {
                if (pT->isActive()) {
                    itemDescription = descActiveFolder;
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
                    itemDescription = descInactiveFolder;
                }
            } else {
                if (pT->shouldBeActive()) {
                    itemDescription = descActiveFolder;
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-green.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descInactiveFolder;
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-green-locked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
        } else if (pT->isOffsetTimer()) {
            if (pT->shouldBeActive()) {
                itemDescription = descActiveOffsetTimer;
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/offsettimer-on.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/offsettimer-on-grey.png")), QIcon::Normal, QIcon::Off);
                    itemDescription = descInactiveParent.arg(itemDescription);
                }
            } else {
                itemDescription = descInactiveOffsetTimer;
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/offsettimer-off.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/offsettimer-off-grey.png")), QIcon::Normal, QIcon::Off);
                }
            }
        } else {
            if (pT->shouldBeActive()) {
                itemDescription = descActive;
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                    itemDescription = descInactiveParent.arg(itemDescription);
                }
                pT->setIsActive(true);
            } else {
                itemDescription = descInactive;
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                }
            }
        }

        if (pT->state()) {
            clearEditorNotification();

            // don't activate new timers by default - might be annoying
            pItem->setIcon(0, icon);
            pItem->setText(0, name);

        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(qsl(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            itemDescription = descError;
            pItem->setIcon(0, iconError);
            pItem->setText(0, name);
            showError(pT->getError());
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);
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
    unmarkQString(&regex);


    if (!regex.isEmpty() && ((name.isEmpty()) || (name == tr("New alias")))) {
        name = regex;
    }
    const QString substitution = mpAliasMainArea->lineEdit_alias_command->text();
    //check if sub will trigger regex, ignore if there's nothing in regex - could be an alias group
    const QRegularExpression rx(regex);
    const QRegularExpressionMatch match = rx.match(substitution);

    QString itemDescription;
    if (!regex.isEmpty() && match.capturedStart() != -1) {
        //we have a loop
        QIcon iconError;
        iconError.addPixmap(QPixmap(qsl(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
        itemDescription = descError;
        pItem->setIcon(0, iconError);
        pItem->setText(0, name);
        showError(tr("Alias <em>%1</em> has an infinite loop - substitution matches its own pattern. Please fix it - this alias isn't good as it'll call itself forever.").arg(name.toHtmlEscaped()));
        return;
    }

    const QString script = mpSourceEditorEdbeeDocument->text();


    const int triggerID = pItem->data(0, Qt::UserRole).toInt();
    TAlias* pT = mpHost->getAliasUnit()->getAlias(triggerID);
    if (pT) {
        const QString old_name = pT->getName();
        pT->setName(name);
        pT->setCommand(substitution);
        pT->setRegexCode(regex); // This could generate an error state if regex does not compile
        pT->setScript(script);

        QIcon icon;
        QString itemDescription;
        if (pT->isFolder()) {
            if (!pT->mPackageName.isEmpty()) {
                if (pT->isActive()) {
                    itemDescription = descActiveFolder;
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
                    itemDescription = descInactiveFolder;
                }
            } else if (pT->isActive()) {
                itemDescription = descActiveFolder;
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-violet.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                    itemDescription = descInactiveParent.arg(itemDescription);
                }
            } else {
                itemDescription = descInactiveFolder;
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-violet-locked.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                }
            }
        } else {
            if (pT->isActive()) {
                itemDescription = descActive;
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                    itemDescription = descInactiveParent.arg(itemDescription);
                }
            } else {
                itemDescription = descInactive;
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                }
            }
        }

        if (pT->state()) {
            clearEditorNotification();

            if (old_name == tr("New alias")) {
                if (pT->isFolder()) {
                    itemDescription = descActiveFolder;
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-violet.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descActive;
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                }
                pItem->setIcon(0, icon);
                pItem->setText(0, name);
                pT->setIsActive(true);
            } else {
                pItem->setIcon(0, icon);
                pItem->setText(0, name);
            }
        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(qsl(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            itemDescription = descError;
            pItem->setIcon(0, iconError);
            pItem->setText(0, name);
            showError(pT->getError());
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);
    }
}

void dlgTriggerEditor::saveAction()
{
    QTreeWidgetItem* pItem = mpCurrentActionItem;
    if (!pItem) {
        return;
    }

    mpActionsMainArea->trimName();
    const QString name = mpActionsMainArea->lineEdit_action_name->text();
    const QString icon = mpActionsMainArea->lineEdit_action_icon->text();
    const QString commandDown = mpActionsMainArea->lineEdit_action_button_command_down->text();
    const QString commandUp = mpActionsMainArea->lineEdit_action_button_command_up->text();
    const QString script = mpSourceEditorEdbeeDocument->text();
    // currentIndex() can return -1 if no setting was previously made - need to fixup:
    const int rotation = qMax(0, mpActionsMainArea->comboBox_action_button_rotation->currentIndex());
    const int columns = mpActionsMainArea->spinBox_action_bar_columns->text().toInt();
    const bool isChecked = mpActionsMainArea->checkBox_action_button_isPushDown->isChecked();
    // bottom location is no longer supported i.e. location = 1 = 0 = location top
    // currentIndex() can return -1 if no setting was previously made - need to fixup:
    int location = qMax(0, mpActionsMainArea->comboBox_action_bar_location->currentIndex());
    if (location > 0) {
        location++;
    }

    // currentIndex() can return -1 if no setting was previously made - need to fixup:
    const int orientation = qMax(0, mpActionsMainArea->comboBox_action_bar_orientation->currentIndex());

    // This is an unnecessary level of indentation but has been retained to
    // reduce the noise in a git commit/diff caused by the removal of a
    // redundant "if( pITem )" - can be removed next time the file is modified
    const int actionID = pItem->data(0, Qt::UserRole).toInt();
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
        QString itemDescription;
        const bool itemActive = pA->isActive();
        if (pA->isFolder()) {
            itemDescription = (itemActive ? descActiveFolder : descInactiveFolder);
            if (!pA->mPackageName.isEmpty()) {
                // Has a package name so is a module master folder
                if (itemActive) {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
                }
            } else if (!pA->getParent() || !pA->getParent()->mPackageName.isEmpty()) {
                // No parent or it has a parent with a package name so is a toolbar
                if (itemActive) {
                    if (pA->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-yellow.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-yellow-locked.png")), QIcon::Normal, QIcon::Off);
                }
            } else {
                // Else must be a menu
                if (itemActive) {
                    if (pA->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-cyan.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-cyan-locked.png")), QIcon::Normal, QIcon::Off);
                }
            }
        } else {
            // Is a button
            if (itemActive) {
                itemDescription = descActive;
                if (pA->ancestorsActive()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                    itemDescription = descInactiveParent.arg(itemDescription);
                }
            } else {
                icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                itemDescription = descInactive;
            }
        }

        if (pA->state()) {
            clearEditorNotification();

            pItem->setIcon(0, icon);
            pItem->setText(0, name);
        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(qsl(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            itemDescription = descError;
            pItem->setIcon(0, iconError);
            pItem->setText(0, name);
            showError(pA->getError());
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);

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
    const int scriptID = pItem->data(0, Qt::UserRole).toInt();
    if (scriptID != id) {
        return;
    }

    TScript* pT = mpHost->getScriptUnit()->getScript(scriptID);
    if (!pT) {
        return;
    }

    const QString scriptCode = pT->getScript();
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
    const QString name = mpScriptsMainArea->lineEdit_script_name->text();
    const QString script = mpSourceEditorEdbeeDocument->text();
    mpScriptsMainAreaEditHandlerItem = nullptr;
    QList<QListWidgetItem*> itemList;
    for (int i = 0; i < mpScriptsMainArea->listWidget_script_registered_event_handlers->count(); i++) {
        QListWidgetItem* pItem = mpScriptsMainArea->listWidget_script_registered_event_handlers->item(i);
        itemList << pItem;
    }
    QStringList handlerList;
    for (auto& listWidgetItem : itemList) {
        if (listWidgetItem->text().isEmpty()) {
            continue;
        }
        handlerList << listWidgetItem->text();
    }


    const int scriptID = pItem->data(0, Qt::UserRole).toInt();
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
    QString itemDescription;
    const bool itemActive = pT->isActive();
    if (pT->isFolder()) {
        itemDescription = (itemActive ? descActiveFolder : descInactiveFolder);
        if (!pT->mPackageName.isEmpty()) {
            if (itemActive) {
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                    itemDescription = descInactiveParent.arg(itemDescription);
                }
            } else {
                icon.addPixmap(QPixmap(qsl(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
            }
        } else {
            if (itemActive) {
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-orange.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                    itemDescription = descInactiveParent.arg(itemDescription);
                }
            } else {
                icon.addPixmap(QPixmap(qsl(":/icons/folder-orange-locked.png")), QIcon::Normal, QIcon::Off);
            }
        }
    } else {
        if (itemActive) {
            itemDescription = descActive;
            if (pT->ancestorsActive()) {
                icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
            } else {
                icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                itemDescription = descInactiveParent.arg(itemDescription);
            }
        } else {
            icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
            itemDescription = descInactive;
        }
    }

    if (pT->state()) {
        if (auto error = pT->getLoadingError(); error) {
            showWarning(tr("While loading the profile, this script had an error that has since been fixed, "
                           "possibly by another script. The error was:%2%3")
                                .arg(qsl("<br>"), error.value()));
        } else {
            clearEditorNotification();
        }

        if (old_name == tr("New script") || old_name == tr("New script group")) {
            if (pT->isFolder()) {
                itemDescription = descActiveFolder;
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-orange.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                    itemDescription = descInactiveParent.arg(itemDescription);
                }
            } else {
                itemDescription = descActive;
                icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                    itemDescription = descInactiveParent.arg(itemDescription);
                }
            }
            pItem->setIcon(0, icon);
            pItem->setText(0, name);
            pT->setIsActive(true);
        } else {
            pItem->setIcon(0, icon);
            pItem->setText(0, name);
        }

    } else {
        QIcon iconError;
        iconError.addPixmap(QPixmap(qsl(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
        itemDescription = descError;
        pItem->setIcon(0, iconError);
        pItem->setText(0, name);
        showError(pT->getError());
    }
    pItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);
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
    const int currentNameType = var->getKeyType();
    const int currentValueType = var->getValueType();
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
    const QString newName = mpVarsMainArea->lineEdit_var_name->text();
    QString newValue = mpSourceEditorEdbeeDocument->text();
    if (newName.isEmpty()) {
        slot_variableSelected(pItem);
        return;
    }
    mChangingVar = true;
    int uiNameType = mpVarsMainArea->comboBox_variable_key_type->itemData(mpVarsMainArea->comboBox_variable_key_type->currentIndex(), Qt::UserRole).toInt();
    int uiValueType = mpVarsMainArea->comboBox_variable_value_type->itemData(mpVarsMainArea->comboBox_variable_value_type->currentIndex(), Qt::UserRole).toInt();
    if ((uiNameType == LUA_TNUMBER || uiNameType == LUA_TSTRING) && newVar) {
        uiNameType = LUA_TNONE;
    }
    //check variable recasting
    const int varRecast = canRecast(pItem, uiNameType, uiValueType);
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
                    //let's make sure the nametype works
                    if (variable->getKeyType() == LUA_TNUMBER && newName.toInt()) {
                        uiNameType = LUA_TNUMBER;
                    } else {
                        uiNameType = LUA_TSTRING;
                    }
                    change = change | 0x1;
                }
                variable->setNewName(newName, uiNameType);
                if (variable->getValueType() != LUA_TTABLE && (newValue != variable->getValue() || uiValueType != variable->getValueType())) {
                    //let's check again
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
                //let's make sure the nametype works
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
                //let's check again
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
    pItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsAutoTristate | Qt::ItemIsUserCheckable);
    pItem->setToolTip(0, utils::richText(tr("Checked variables will be saved and loaded with your profile.")));
    if (!varUnit->shouldSave(variable)) {
        pItem->setFlags(pItem->flags() & ~(Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsUserCheckable));
        pItem->setForeground(0, QBrush(QColor("grey")));
        pItem->setToolTip(0, QString());
        pItem->setCheckState(0, Qt::Unchecked);
    } else if (varUnit->isSaved(variable)) {
        pItem->setCheckState(0, Qt::Checked);
    }
    pItem->setData(0, Qt::UserRole, QVariant(variable->getId()));
    QIcon icon;
    switch (variable->getValueType()) {
    case 5:
        icon.addPixmap(QPixmap(qsl(":/icons/table.png")), QIcon::Normal, QIcon::Off);
        break;
    case 6:
        icon.addPixmap(QPixmap(qsl(":/icons/function.png")), QIcon::Normal, QIcon::Off);
        break;
    default:
        icon.addPixmap(QPixmap(qsl(":/icons/variable.png")), QIcon::Normal, QIcon::Off);
        break;
    }
    pItem->setIcon(0, icon);

    mChangingVar = false;
    slot_variableSelected(pItem);
}

void dlgTriggerEditor::saveKey()
{
    QTreeWidgetItem* pItem = mpCurrentKeyItem;
    if (!pItem) {
        return;
    }

    mpKeysMainArea->trimName();
    QString name = mpKeysMainArea->lineEdit_key_name->text();
    if (name.isEmpty() || name == tr("New key")) {
        name = mpKeysMainArea->lineEdit_key_binding->text();
    }
    const QString command = mpKeysMainArea->lineEdit_key_command->text();
    const QString script = mpSourceEditorEdbeeDocument->text();


    const int triggerID = pItem->data(0, Qt::UserRole).toInt();
    TKey* pT = mpHost->getKeyUnit()->getKey(triggerID);
    if (pT) {
        const QString old_name = pT->getName();
        pItem->setText(0, name);
        pT->setName(name);
        pT->setCommand(command);
        pT->setScript(script);

        QIcon icon;
        QString itemDescription;
        const bool itemActive = pT->isActive();
        if (pT->isFolder()) {
            itemDescription = (itemActive ? descActiveFolder : descInactiveFolder);
            if (!pT->mPackageName.isEmpty()) {
                if (itemActive) {
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
                }
            } else if (itemActive) {
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-pink.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                    itemDescription = descInactiveParent.arg(itemDescription);
                }
            } else {
                itemDescription = descInactiveFolder;
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-pink-locked.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                }
            }
        } else {
            if (itemActive) {
                itemDescription = descActive;
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                    itemDescription = descInactiveParent.arg(itemDescription);
                }
            } else {
                itemDescription = descInactive;
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                }
            }
        }

        if (pT->state()) {
            clearEditorNotification();
            if (old_name == tr("New key")) {
                if (pT->isFolder()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-pink.png")), QIcon::Normal, QIcon::Off);
                    itemDescription = descActiveFolder;
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    itemDescription = descActive;
                }
                pItem->setIcon(0, icon);
                pItem->setText(0, name);
                pT->setIsActive(true);
            } else {
                pItem->setIcon(0, icon);
                pItem->setText(0, name);
            }
        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(qsl(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            itemDescription = descError;
            pItem->setIcon(0, iconError);
            pItem->setText(0, name);
            showError(pT->getError());
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);
    }
}


void dlgTriggerEditor::setupPatternControls(const int type, dlgTriggerPatternEdit* pItem)
{
    // Display middle dots for potentially unwanted spaces in perl regex
    if (type == REGEX_PERL) {
        markQLineEdit(pItem->lineEdit_pattern);
        lineEditShouldMarkSpaces[pItem->lineEdit_pattern] = true;
    } else {
        unmarkQLineEdit(pItem->lineEdit_pattern);
        lineEditShouldMarkSpaces[pItem->lineEdit_pattern] = false;
    }

    switch (type) {
    case REGEX_SUBSTRING:
    case REGEX_PERL:
    case REGEX_BEGIN_OF_LINE_SUBSTRING:
    case REGEX_EXACT_MATCH:
    case REGEX_LUA_CODE:
        pItem->lineEdit_pattern->show();
        pItem->pushButton_fgColor->hide();
        pItem->pushButton_bgColor->hide();
        pItem->pushButton_prompt->hide();
        pItem->spinBox_lineSpacer->hide();
        break;
    case REGEX_LINE_SPACER:
        pItem->lineEdit_pattern->hide();
        pItem->pushButton_fgColor->hide();
        pItem->pushButton_bgColor->hide();
        pItem->pushButton_prompt->hide();
        pItem->spinBox_lineSpacer->show();
        break;
    case REGEX_COLOR_PATTERN:
        // CHECKME: Do we need to regenerate (hidden patter text) and button texts/colors?
        pItem->lineEdit_pattern->hide();
        pItem->pushButton_fgColor->show();
        pItem->pushButton_bgColor->show();
        pItem->pushButton_prompt->hide();
        pItem->spinBox_lineSpacer->hide();
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
            pItem->pushButton_prompt->setToolTip(utils::richText(tr("A Go-Ahead (GA) signal from the game is required to make this feature work")));
        }
        pItem->pushButton_prompt->show();
        pItem->spinBox_lineSpacer->hide();
        break;
    }
}

void dlgTriggerEditor::slot_changedPattern()
{
    QLineEdit* lineEdit = qobject_cast<QLineEdit*>(sender());
    if (lineEditShouldMarkSpaces[lineEdit]) {
        markQLineEdit(lineEdit);
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

    const int row = pBox->itemData(0).toInt();
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
        if ((pPatternItem->lineEdit_pattern->text().startsWith(QLatin1String("ANSI_COLORS_F{")) && pPatternItem->lineEdit_pattern->text().contains(QLatin1String("}_B{"))
             && pPatternItem->lineEdit_pattern->text().endsWith(QLatin1String("}")))) {
            // It looks as though there IS a valid color pattern string in the
            // lineEdit, so, in case it has been edited by hand, regenerate the
            // colors that are used:
            int textAnsiFg = TTrigger::scmIgnored;
            int textAnsiBg = TTrigger::scmIgnored;
            TTrigger::decodeColorPatternText(pPatternItem->lineEdit_pattern->text(), textAnsiFg, textAnsiBg);

            if (textAnsiFg == TTrigger::scmIgnored) {
                pPatternItem->pushButton_fgColor->setStyleSheet(QString());
                //: Color trigger ignored foreground color button, ensure all three instances have the same text
                pPatternItem->pushButton_fgColor->setText(tr("Foreground color ignored"));
            } else if (textAnsiFg == TTrigger::scmDefault) {
                pPatternItem->pushButton_fgColor->setStyleSheet(QString());
                //: Color trigger default foreground color button, ensure all three instances have the same text
                pPatternItem->pushButton_fgColor->setText(tr("Default foreground color"));
            } else {
                pPatternItem->pushButton_fgColor->setStyleSheet(generateButtonStyleSheet(mpHost->getAnsiColor(textAnsiFg, false)));
                //: Color trigger ANSI foreground color button, ensure all three instances have the same text
                pPatternItem->pushButton_fgColor->setText(tr("Foreground color [ANSI %1]").arg(QString::number(textAnsiFg)));
            }

            if (textAnsiBg == TTrigger::scmIgnored) {
                pPatternItem->pushButton_bgColor->setStyleSheet(QString());
                //: Color trigger ignored background color button, ensure all three instances have the same text
                pPatternItem->pushButton_bgColor->setText(tr("Background color ignored"));
            } else if (textAnsiBg == TTrigger::scmDefault) {
                pPatternItem->pushButton_bgColor->setStyleSheet(QString());
                //: Color trigger default background color button, ensure all three instances have the same text
                pPatternItem->pushButton_bgColor->setText(tr("Default background color"));
            } else {
                pPatternItem->pushButton_bgColor->setStyleSheet(generateButtonStyleSheet(mpHost->getAnsiColor(textAnsiBg, true)));
                //: Color trigger ANSI background color button, ensure all three instances have the same text
                pPatternItem->pushButton_bgColor->setText(tr("Background color [ANSI %1]").arg(QString::number(textAnsiBg)));
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

void dlgTriggerEditor::slot_triggerLinePatternItemEdited(int i)
{
    QComboBox* pBox = qobject_cast<QComboBox*>(sender());
    if (!pBox) {
        return;
    }
    const int row = pBox->itemData(0).toInt();
    if (row < 0 || row >= 50) {
        return;
    }
    dlgTriggerPatternEdit* pPatternItem = mTriggerPatternEdit[row];
    if (i == mpPrevTriggerPatternItemEdit[row]) {
        return;
    }
    TriggerLineEditPatternItemEditedCommand* command = new TriggerLineEditPatternItemEditedCommand(mpTriggersMainArea);
    command->mpEditor = this;
    command->mpTreeWidget_triggers = treeWidget_triggers;
    command->mpItem = treeWidget_triggers->currentItem();
    command->mRow = row;
    command->mpTriggerUnit = mpHost->getTriggerUnit();
    command->mPrevTriggerPatternEdit = mpPrevTriggerPatternItemEdit[row];
    command->mTriggerPatternEdit = pPatternItem->comboBox_patternType->currentIndex();
    command->pBox = pBox;
    undoStack->push(command);
    saveTrigger();
}

void dlgTriggerEditor::slot_triggerLinePatternEdited(int i)
{
    QLineEdit* lineEdit = qobject_cast<QLineEdit*>(sender());
    if(lineEdit->text() == mpPrevTriggerPatternEdit[i])
    {
        return;
    }
    dlgTriggerPatternEdit* pTriggerPattern = mTriggerPatternEdit.at(i);

    TriggerLineEditPatternEditedCommand* command = new TriggerLineEditPatternEditedCommand(mpTriggersMainArea);
    command->mpEditor = this;
    command->mpTriggerPattern = pTriggerPattern;
    command->mpTreeWidget_triggers = treeWidget_triggers;
    command->mpItem = treeWidget_triggers->currentItem();
    command->mPrevLineEdit_trigger_pattern = mpPrevTriggerPatternEdit[i];
    command->mLineEdit_trigger_pattern = lineEdit->text();
    command->mRow = i;
    command->mpTriggerPatternEdit = mTriggerPatternEdit;
    undoStack->push(command);
    saveTrigger();
}

void dlgTriggerEditor::slot_triggerSelected(QTreeWidgetItem* pItem)
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
    mpTriggersMainArea->lineEdit_trigger_name->clear();
    mpTriggersMainArea->label_idNumber->clear();
    clearDocument(mpSourceEditorEdbee); // Trigger Select
    mpTriggersMainArea->groupBox_multiLineTrigger->blockSignals(true);
    mpTriggersMainArea->groupBox_multiLineTrigger->setChecked(false);
    mpTriggersMainArea->groupBox_multiLineTrigger->blockSignals(false);
    mpTriggersMainArea->groupBox_perlSlashGOption->blockSignals(true);
    mpTriggersMainArea->groupBox_perlSlashGOption->setChecked(false);
    mpTriggersMainArea->groupBox_perlSlashGOption->blockSignals(false);
    mpTriggersMainArea->groupBox_filterTrigger->blockSignals(true);
    mpTriggersMainArea->groupBox_filterTrigger->setChecked(false);
    mpTriggersMainArea->groupBox_filterTrigger->blockSignals(false);
    mpTriggersMainArea->groupBox_triggerColorizer->blockSignals(true);
    mpTriggersMainArea->groupBox_triggerColorizer->setChecked(false);
    mpTriggersMainArea->groupBox_triggerColorizer->blockSignals(false);
    mpTriggersMainArea->pushButtonFgColor->setStyleSheet(QString());
    mpTriggersMainArea->pushButtonFgColor->setProperty(cButtonBaseColor, QVariant());
    mpTriggersMainArea->pushButtonBgColor->setStyleSheet(QString());
    mpTriggersMainArea->pushButtonBgColor->setProperty(cButtonBaseColor, QVariant());
    mpTriggersMainArea->spinBox_lineMargin->blockSignals(true);
    mpTriggersMainArea->spinBox_lineMargin->setValue(1);
    mpTriggersMainArea->spinBox_lineMargin->blockSignals(false);
    const int ID = pItem->data(0, Qt::UserRole).toInt();
    TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(ID);
    if (pT) {
        const QStringList patternList = pT->getPatternsList();
        QList<int> const propertyList = pT->getRegexCodePropertyList();

        if (patternList.size() != propertyList.size()) {
            return;
        }

        for (int i = 0; i < patternList.size(); i++) {
            if (i >= 50) {
                break; // pattern list is limited to 50 at the moment
            }
            if (i >= pT->mColorPatternList.size()) {
                break;
            }
            // Use operator[] so we have write access to the array/list member:
            dlgTriggerPatternEdit* pPatternItem = mTriggerPatternEdit[i];
            const int pType = propertyList.at(i);
            if (!pType) {
                // If the control is for the default (0) case nudge the setting
                // up and down so that it copies the colour icon for the
                // subString type across into the QLineEdit:
                pPatternItem->comboBox_patternType->blockSignals(true);
                pPatternItem->comboBox_patternType->setCurrentIndex(1);
                pPatternItem->comboBox_patternType->blockSignals(false);
                setupPatternControls(1, pPatternItem);
            }
            pPatternItem->comboBox_patternType->blockSignals(true);
            pPatternItem->comboBox_patternType->setCurrentIndex(pType);
            pPatternItem->comboBox_patternType->blockSignals(false);
            mpPrevTriggerPatternItemEdit[i] = mTriggerPatternEdit[i]->comboBox_patternType->currentIndex();
            mpPrevTriggerPatternEdit[i] = mTriggerPatternEdit[i]->lineEdit_pattern->text();
            mPrevLineSpacer[i] = mTriggerPatternEdit[i]->spinBox_lineSpacer->value();
            setupPatternControls(pType, pPatternItem);
            if (pType == REGEX_PROMPT) {
                pPatternItem->lineEdit_pattern->clear();

            } else if (pType == REGEX_COLOR_PATTERN) {
                pPatternItem->lineEdit_pattern->setText(patternList.at(i));
                if (pT->mColorPatternList.at(i)) {
                    if (pT->mColorPatternList.at(i)->ansiFg == TTrigger::scmIgnored) {
                        pPatternItem->pushButton_fgColor->setStyleSheet(QString());
                        //: Color trigger ignored foreground color button, ensure all three instances have the same text
                        pPatternItem->pushButton_fgColor->setText(tr("Foreground color ignored"));
                    } else if (pT->mColorPatternList.at(i)->ansiFg == TTrigger::scmDefault) {
                        pPatternItem->pushButton_fgColor->setStyleSheet(QString());
                        //: Color trigger default foreground color button, ensure all three instances have the same text
                        pPatternItem->pushButton_fgColor->setText(tr("Default foreground color"));
                    } else {
                        pPatternItem->pushButton_fgColor->setStyleSheet(generateButtonStyleSheet(pT->mColorPatternList.at(i)->mFgColor));
                        //: Color trigger ANSI foreground color button, ensure all three instances have the same text
                        pPatternItem->pushButton_fgColor->setText(tr("Foreground color [ANSI %1]").arg(QString::number(pT->mColorPatternList.at(i)->ansiFg)));
                    }

                    if (pT->mColorPatternList.at(i)->ansiBg == TTrigger::scmIgnored) {
                        pPatternItem->pushButton_bgColor->setStyleSheet(QString());
                        //: Color trigger ignored background color button, ensure all three instances have the same text
                        pPatternItem->pushButton_bgColor->setText(tr("Background color ignored"));
                    } else if (pT->mColorPatternList.at(i)->ansiBg == TTrigger::scmDefault) {
                        pPatternItem->pushButton_bgColor->setStyleSheet(QString());
                        //: Color trigger default background color button, ensure all three instances have the same text
                        pPatternItem->pushButton_bgColor->setText(tr("Default background color"));
                    } else {
                        pPatternItem->pushButton_bgColor->setStyleSheet(generateButtonStyleSheet(pT->mColorPatternList.at(i)->mBgColor));
                        //: Color trigger ANSI background color button, ensure all three instances have the same text
                        pPatternItem->pushButton_bgColor->setText(tr("Background color [ANSI %1]").arg(QString::number(pT->mColorPatternList.at(i)->ansiBg)));
                    }
                } else {
                    qWarning() << "dlgTriggerEditor::slot_triggerSelected(...) ERROR: TTrigger instance has an mColorPattern of size:" << pT->mColorPatternList.size() << "but array element:" << i
                               << "is a nullptr";
                    pPatternItem->pushButton_fgColor->setStyleSheet(QString());
                    pPatternItem->pushButton_fgColor->setText(tr("fault"));
                    pPatternItem->pushButton_bgColor->setStyleSheet(QString());
                    pPatternItem->pushButton_fgColor->setText(tr("fault"));
                }
            } else if (pType == REGEX_LINE_SPACER) {
                pPatternItem->spinBox_lineSpacer->blockSignals(true);
                pPatternItem->spinBox_lineSpacer->setValue(patternList.at(i).toInt());
                pPatternItem->spinBox_lineSpacer->blockSignals(false);
            } else {
                // Keep track of lineEdits that should have trailing spaces marked
                if (pType == REGEX_PERL) {
                    lineEditShouldMarkSpaces[pPatternItem->lineEdit_pattern] = true;
                }
                pPatternItem->lineEdit_pattern->setText(patternList.at(i));
            }
        }

        // reset the rest of the patterns that don't have any data
        for (int i = 0; i < 50; i++) {
            if(i == patternList.size())
            {
                mTriggerPatternEdit[i]->lineEdit_pattern->setEnabled(true);
                continue;
            }

            if(mTriggerPatternEdit[i]->lineEdit_pattern->text().isEmpty() && mTriggerPatternEdit[i]->lineEdit_pattern->isEnabled())
            {
                mTriggerPatternEdit[i]->lineEdit_pattern->setEnabled(false);
            }
            if(!mTriggerPatternEdit[i]->lineEdit_pattern->text().isEmpty() && !mTriggerPatternEdit[i]->lineEdit_pattern->isEnabled())
            {
                mTriggerPatternEdit[i]->lineEdit_pattern->setEnabled(true);
            }
        }

        for (int i = patternList.size(); i < 50; i++) {
            mTriggerPatternEdit[i]->lineEdit_pattern->clear();
            if (mTriggerPatternEdit[i]->lineEdit_pattern->isHidden()) {
                mTriggerPatternEdit[i]->lineEdit_pattern->show();
            }
            mTriggerPatternEdit[i]->pushButton_fgColor->hide();
            mTriggerPatternEdit[i]->pushButton_bgColor->hide();
            mTriggerPatternEdit[i]->pushButton_prompt->hide();
            mTriggerPatternEdit[i]->spinBox_lineSpacer->hide();
            // Nudge the type up and down so that the appropriate (coloured) icon is copied across to the QLineEdit:
            mTriggerPatternEdit[i]->comboBox_patternType->blockSignals(true);
            mTriggerPatternEdit[i]->comboBox_patternType->setCurrentIndex(1);
            mTriggerPatternEdit[i]->comboBox_patternType->setCurrentIndex(0);
            mTriggerPatternEdit[i]->comboBox_patternType->blockSignals(false);
        }
        // Scroll to the last used pattern:
        mpScrollArea->ensureWidgetVisible(mTriggerPatternEdit.at(qBound(0, patternList.size(), 49)));
        const QString command = pT->getCommand();
        mpTriggersMainArea->lineEdit_trigger_name->setText(pItem->text(0));
        mpTriggersMainArea->label_idNumber->setText(QString::number(ID));
        mpTriggersMainArea->lineEdit_trigger_command->blockSignals(true);
        mpTriggersMainArea->lineEdit_trigger_command->setText(command);
        mpTriggersMainArea->lineEdit_trigger_command->blockSignals(false);
        mpTriggersMainArea->groupBox_multiLineTrigger->blockSignals(true);
        mpTriggersMainArea->groupBox_multiLineTrigger->setChecked(pT->isMultiline());
        mpTriggersMainArea->groupBox_multiLineTrigger->blockSignals(false);
        mpTriggersMainArea->groupBox_perlSlashGOption->blockSignals(true);
        mpTriggersMainArea->groupBox_perlSlashGOption->setChecked(pT->mPerlSlashGOption);
        mpTriggersMainArea->groupBox_perlSlashGOption->blockSignals(false);
        mpTriggersMainArea->groupBox_filterTrigger->blockSignals(true);
        mpTriggersMainArea->groupBox_filterTrigger->setChecked(pT->mFilterTrigger);
        mpTriggersMainArea->groupBox_filterTrigger->blockSignals(false);
        mpTriggersMainArea->spinBox_lineMargin->blockSignals(true);
        mpTriggersMainArea->spinBox_lineMargin->setValue(pT->getConditionLineDelta());
        mpTriggersMainArea->spinBox_lineMargin->blockSignals(false);
        mpTriggersMainArea->spinBox_stayOpen->blockSignals(true);
        mpTriggersMainArea->spinBox_stayOpen->setValue(pT->mStayOpen);
        mpTriggersMainArea->spinBox_stayOpen->blockSignals(false);
        mpTriggersMainArea->groupBox_soundTrigger->blockSignals(true);
        mpTriggersMainArea->groupBox_soundTrigger->setChecked(pT->mSoundTrigger);
        mpTriggersMainArea->groupBox_soundTrigger->blockSignals(false);
        if (!pT->mSoundFile.isEmpty()) {
            mpTriggersMainArea->lineEdit_soundFile->setToolTip(pT->mSoundFile);
        }
        mpTriggersMainArea->lineEdit_soundFile->blockSignals(true);
        mpTriggersMainArea->lineEdit_soundFile->setText(pT->mSoundFile);
        mpTriggersMainArea->lineEdit_soundFile->blockSignals(false);
        mpTriggersMainArea->lineEdit_soundFile->setCursorPosition(mpTriggersMainArea->lineEdit_soundFile->text().length());
        mpTriggersMainArea->toolButton_clearSoundFile->setEnabled(!mpTriggersMainArea->lineEdit_soundFile->text().isEmpty());
        mpTriggersMainArea->groupBox_triggerColorizer->blockSignals(true);
        mpTriggersMainArea->groupBox_triggerColorizer->setChecked(pT->isColorizerTrigger());
        mpTriggersMainArea->groupBox_triggerColorizer->blockSignals(false);

        const QColor fgColor(pT->getFgColor());
        const QColor bgColor(pT->getBgColor());
        const bool transparentFg = fgColor == QColorConstants::Transparent;
        const bool transparentBg = bgColor == QColorConstants::Transparent;
        mpTriggersMainArea->pushButtonFgColor->blockSignals(true);
        mpTriggersMainArea->pushButtonFgColor->setStyleSheet(generateButtonStyleSheet(fgColor, pT->isColorizerTrigger()));
        mpTriggersMainArea->pushButtonFgColor->setProperty(cButtonBaseColor, transparentFg ? qsl("transparent") : fgColor.name());
        //: Keep the existing colour on matches to highlight. Use shortest word possible so it fits on the button
        mpTriggersMainArea->pushButtonFgColor->setText(transparentFg ? tr("keep") : QString());
        mpTriggersMainArea->pushButtonFgColor->blockSignals(false);
        mpTriggersMainArea->pushButtonBgColor->blockSignals(true);
        mpTriggersMainArea->pushButtonBgColor->setStyleSheet(generateButtonStyleSheet(pT->getBgColor(), pT->isColorizerTrigger()));
        mpTriggersMainArea->pushButtonBgColor->setProperty(cButtonBaseColor, transparentBg ? qsl("transparent") : bgColor.name());
        //: Keep the existing colour on matches to highlight. Use shortest word possible so it fits on the button
        mpTriggersMainArea->pushButtonBgColor->setText(transparentBg ? tr("keep") : QString());
        mpTriggersMainArea->pushButtonBgColor->blockSignals(false);

        clearDocument(mpSourceEditorEdbee, pT->getScript());

        if (!pT->state()) {
            showError(pT->getError());
        }

    } else {
        // No details to show - as will be the case if the top item (ID = 0) is
        // selected - so show the help message:
        clearTriggerForm();
    }
}

void dlgTriggerEditor::slot_aliasSelected(QTreeWidgetItem* pItem)
{
    if (!pItem) {
        // No details to show - so show the help message:
        clearAliasForm();
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
    mpAliasMainArea->label_idNumber->clear();
    mpAliasMainArea->lineEdit_alias_pattern->clear();
    mpAliasMainArea->lineEdit_alias_command->clear();
    clearDocument(mpSourceEditorEdbee); // Alias Select

    // mpAliasMainArea->lineEdit_alias_name->setText(pItem->text(0));
    const int ID = pItem->data(0, Qt::UserRole).toInt();
    TAlias* pT = mpHost->getAliasUnit()->getAlias(ID);
    if (pT) {
        const QString pattern = pT->getRegexCode();
        const QString command = pT->getCommand();
        const QString name = pT->getName();

        mpAliasMainArea->lineEdit_alias_pattern->setText(pattern);
        mpAliasMainArea->lineEdit_alias_command->setText(command);
        mpAliasMainArea->lineEdit_alias_name->setText(name);
        mpAliasMainArea->label_idNumber->setText(QString::number(ID));

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

void dlgTriggerEditor::slot_keySelected(QTreeWidgetItem* pItem)
{
    if (!pItem) {
        // No details to show - so show the help message:
        clearKeyForm();
        return;
    }

    // save the current key before switching to the new one
    if (pItem != mpCurrentKeyItem) {
        saveKey();
    }
    qDebug() << "itemid " << pItem->data(0, Qt::UserRole).toInt() << " text: " << pItem->text(0);
    mpCurrentKeyItem = pItem;
    mpKeysMainArea->show();
    mpSourceEditorArea->show();
    clearEditorNotification();
    mpKeysMainArea->lineEdit_key_command->clear();
    mpKeysMainArea->lineEdit_key_binding->clear();
    mpKeysMainArea->lineEdit_key_name->clear();
    mpKeysMainArea->label_idNumber->clear();
    clearDocument(mpSourceEditorEdbee); // Key Select

    mpKeysMainArea->lineEdit_key_binding->setText(pItem->text(0));
    const int ID = pItem->data(0, Qt::UserRole).toInt();
    TKey* pT = mpHost->getKeyUnit()->getKey(ID);
    if (pT) {
        const QString command = pT->getCommand();
        const QString name = pT->getName();
        mpKeysMainArea->lineEdit_key_command->setText(command);
        mpKeysMainArea->lineEdit_key_name->setText(name);
        mpKeysMainArea->label_idNumber->setText(QString::number(ID));
        const QString keyName = mpHost->getKeyUnit()->getKeyName(pT->getKeyCode(), pT->getKeyModifiers());
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

void dlgTriggerEditor::slot_variableChanged(QTreeWidgetItem* pItem)
{
    // This handles a small case where the radio button is clicked while the item is currently selected
    // which causes the variable to not save. In places where we populate the TreeWidgetItem, we have
    // to guard it with mChangingVar or else this will be called with every change such as the variable
    // name, etc.
    if (!pItem || mChangingVar) {
        return;
    }
    const int column = 0;
    const int state = pItem->checkState(column);
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

void dlgTriggerEditor::slot_variableSelected(QTreeWidgetItem* pItem)
{
    if (!pItem || treeWidget_variables->indexOfTopLevelItem(pItem) == 0) {
        // Null item or it is for the first row of the tree
        clearVarForm();
        return;
    }

    clearEditorNotification();

    // save the current variable before switching to the new one
    if (pItem != mpCurrentVarItem) {
        saveVar();
    }

    mChangingVar = true;
    const int column = treeWidget_variables->currentColumn();
    const int state = pItem->checkState(column);
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

    const int varType = var->getValueType();
    const int keyType = var->getKeyType();
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
        icon.addPixmap(QPixmap(qsl(":/icons/variable.png")), QIcon::Normal, QIcon::Off);
        // index 3 = "boolean"
        mpVarsMainArea->comboBox_variable_value_type->setCurrentIndex(3);
        mpVarsMainArea->comboBox_variable_value_type->setEnabled(true);
        break;
    case LUA_TNUMBER:
        mpSourceEditorArea->show();
        mpSourceEditorEdbee->setEnabled(true);
        icon.addPixmap(QPixmap(qsl(":/icons/variable.png")), QIcon::Normal, QIcon::Off);
        // index 2 = "number"
        mpVarsMainArea->comboBox_variable_value_type->setCurrentIndex(2);
        mpVarsMainArea->comboBox_variable_value_type->setEnabled(true);
        break;
    case LUA_TSTRING:
        mpSourceEditorArea->show();
        mpSourceEditorEdbee->setEnabled(true);
        icon.addPixmap(QPixmap(qsl(":/icons/variable.png")), QIcon::Normal, QIcon::Off);
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
        icon.addPixmap(QPixmap(qsl(":/icons/table.png")), QIcon::Normal, QIcon::Off);
        break;
    case LUA_TFUNCTION:
        mpSourceEditorArea->hide();
        mpSourceEditorEdbee->setEnabled(false);
        mpVarsMainArea->comboBox_variable_value_type->setCurrentIndex(5);
        mpVarsMainArea->comboBox_variable_value_type->setEnabled(false);
        icon.addPixmap(QPixmap(qsl(":/icons/function.png")), QIcon::Normal, QIcon::Off);
        break;
    case LUA_TLIGHTUSERDATA:
        [[fallthrough]];
    case LUA_TUSERDATA:
        [[fallthrough]];
    case LUA_TTHREAD:; // No-op
    }

    mpVarsMainArea->checkBox_variable_hidden->setChecked(vu->isHidden(var));
    mpVarsMainArea->lineEdit_var_name->setText(var->getName());
    clearDocument(mpSourceEditorEdbee, lI->getValue(var));
    pItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsAutoTristate | Qt::ItemIsUserCheckable);
    pItem->setToolTip(0, utils::richText(tr("Checked variables will be saved and loaded with your profile.")));
    pItem->setCheckState(0, Qt::Unchecked);
    if (!vu->shouldSave(var)) {
        pItem->setFlags(pItem->flags() & ~(Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsUserCheckable));
        pItem->setForeground(0, QBrush(QColor("grey")));
        pItem->setToolTip(0, "");
    } else if (vu->isSaved(var)) {
        pItem->setCheckState(0, Qt::Checked);
    }
    pItem->setData(0, Qt::UserRole, var->getId());
    pItem->setIcon(0, icon);
    mChangingVar = false;
}

void dlgTriggerEditor::slot_actionSelected(QTreeWidgetItem* pItem)
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
    mpActionsMainArea->label_idNumber->clear();
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
    // to show any right hand side details - pT will also be nullptr!
    const int ID = pItem->data(0, Qt::UserRole).toInt();
    TAction* pT = mpHost->getActionUnit()->getAction(ID);
    if (pT) {
        mpActionsMainArea->lineEdit_action_name->setText(pT->getName());
        mpActionsMainArea->label_idNumber->setText(QString::number(ID));
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
            if (!pT->mPackageName.isEmpty()) {
                // We have a non-empty package name (Tree<T>::mModuleName
                // is NEVER used but Tree<T>::mPackageName is for both!)
                // THUS: We are a module master folder

                mpActionsMainArea->groupBox_action_bar->hide();
                mpActionsMainArea->groupBox_action_button_appearance->hide();
                mpActionsMainArea->widget_top->hide();
                mpSourceEditorArea->hide();
            } else if (!pT->getParent() || (pT->getParent() && !pT->getParent()->mPackageName.isEmpty())) {
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

void dlgTriggerEditor::slot_treeSelectionChanged()
{
    auto* sender = qobject_cast<TTreeWidget*>(QObject::sender());
    if (sender) {
        QList<QTreeWidgetItem*> items = sender->selectedItems();
        if (!items.empty()) {
            QTreeWidgetItem* item = items.first();
            if (sender == treeWidget_scripts) {
                slot_scriptsSelected(item);
            } else if (sender == treeWidget_keys) {
                slot_keySelected(item);
            } else if (sender == treeWidget_timers) {
                slot_timerSelected(item);
            } else if (sender == treeWidget_aliases) {
                slot_aliasSelected(item);
            } else if (sender == treeWidget_actions) {
                slot_actionSelected(item);
            } else if (sender == treeWidget_variables) {
                slot_variableSelected(item);
            } else if (sender == treeWidget_triggers) {
                slot_triggerSelected(item);
            }
        }
    }
}


void dlgTriggerEditor::slot_scriptsSelected(QTreeWidgetItem* pItem)
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
    mpScriptsMainArea->label_idNumber->clear();
    mpScriptsMainArea->listWidget_script_registered_event_handlers->clear();
    // mpScriptsMainArea->lineEdit_script_name->setText(pItem->text(0));
    const int ID = pItem->data(0, Qt::UserRole).toInt();
    TScript* pT = mpHost->getScriptUnit()->getScript(ID);
    if (pT) {
        const QString name = pT->getName();
        QStringList eventHandlerList = pT->getEventHandlerList();
        for (int i = 0; i < eventHandlerList.size(); i++) {
            auto pItem = new QListWidgetItem(mpScriptsMainArea->listWidget_script_registered_event_handlers);
            pItem->setText(eventHandlerList[i]);
            mpScriptsMainArea->listWidget_script_registered_event_handlers->addItem(pItem);
        }
        const QString script = pT->getScript();
        clearDocument(mpSourceEditorEdbee, script);

        mpScriptsMainArea->lineEdit_script_name->setText(name);
        mpScriptsMainArea->label_idNumber->setText(QString::number(ID));
        if (auto error = pT->getLoadingError(); error) {
            showWarning(tr("While loading the profile, this script had an error that has since been fixed, "
                           "possibly by another script. The error was:%2%3")
                                .arg(qsl("<br>"), error.value()));
        } else if (!pT->state()) {
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

void dlgTriggerEditor::slot_timerSelected(QTreeWidgetItem* pItem)
{
    if (!pItem) {
        // No details to show - so show the help message:
        clearTimerForm();
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
    mpTimersMainArea->label_idNumber->clear();
    // mpTimersMainArea->lineEdit_timer_name->setText(pItem->text(0));

    const int ID = pItem->data(0, Qt::UserRole).toInt();
    TTimer* pT = mpHost->getTimerUnit()->getTimer(ID);
    if (pT) {
        const QString command = pT->getCommand();
        const QString name = pT->getName();
        mpTimersMainArea->lineEdit_timer_command->setText(command);
        mpTimersMainArea->lineEdit_timer_name->setText(name);
        mpTimersMainArea->label_idNumber->setText(QString::number(ID));
        const QTime time = pT->getTime();
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
    mpTriggerBaseItem->setIcon(0, QPixmap(qsl(":/icons/tools-wizard.png")));
    treeWidget_triggers->insertTopLevelItem(0, mpTriggerBaseItem);
    populateTriggers();
    mpTriggerBaseItem->setExpanded(true);

    mpTimerBaseItem = new QTreeWidgetItem(static_cast<QTreeWidgetItem*>(nullptr), QStringList(tr("Timers")));
    mpTimerBaseItem->setIcon(0, QPixmap(qsl(":/icons/chronometer.png")));
    treeWidget_timers->insertTopLevelItem(0, mpTimerBaseItem);
    populateTimers();
    mpTimerBaseItem->setExpanded(true);

    mpScriptsBaseItem = new QTreeWidgetItem(static_cast<QTreeWidgetItem*>(nullptr), QStringList(tr("Scripts")));
    mpScriptsBaseItem->setIcon(0, QPixmap(qsl(":/icons/accessories-text-editor.png")));
    treeWidget_scripts->insertTopLevelItem(0, mpScriptsBaseItem);
    populateScripts();
    mpScriptsBaseItem->setExpanded(true);

    mpAliasBaseItem = new QTreeWidgetItem(static_cast<QTreeWidgetItem*>(nullptr), QStringList(tr("Aliases - Input Triggers")));
    mpAliasBaseItem->setIcon(0, QPixmap(qsl(":/icons/system-users.png")));
    treeWidget_aliases->insertTopLevelItem(0, mpAliasBaseItem);
    populateAliases();
    mpAliasBaseItem->setExpanded(true);

    mpActionBaseItem = new QTreeWidgetItem(static_cast<QTreeWidgetItem*>(nullptr), QStringList(tr("Buttons")));
    mpActionBaseItem->setIcon(0, QPixmap(qsl(":/icons/bookmarks.png")));
    treeWidget_actions->insertTopLevelItem(0, mpActionBaseItem);
    populateActions();
    mpActionBaseItem->setExpanded(true);

    mpKeyBaseItem = new QTreeWidgetItem(static_cast<QTreeWidgetItem*>(nullptr), QStringList(tr("Key Bindings")));
    mpKeyBaseItem->setIcon(0, QPixmap(qsl(":/icons/preferences-desktop-keyboard.png")));
    treeWidget_keys->insertTopLevelItem(0, mpKeyBaseItem);
    populateKeys();
    mpKeyBaseItem->setExpanded(true);
}

void dlgTriggerEditor::populateKeys()
{
    std::list<TKey*> const baseNodeList_key = mpHost->getKeyUnit()->getKeyRootNodeList();
    for (auto key : baseNodeList_key) {
        if (key->isTemporary()) {
            continue;
        }

        const QString s = key->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem(mpKeyBaseItem, sList);
        pItem->setData(0, Qt::UserRole, QVariant(key->getID()));
        mpKeyBaseItem->addChild(pItem);
        QIcon icon;
        QString itemDescription;
        const bool itemActive = key->isActive();
        if (key->hasChildren()) {
            expand_child_key(key, pItem);
        }
        if (key->state()) {
            clearEditorNotification();

            if (key->isFolder()) {
                itemDescription = (itemActive ? descActiveFolder : descInactiveFolder);
                if (!key->mPackageName.isEmpty()) {
                    if (key->isActive()) {
                        if (key->ancestorsActive()) {
                            icon.addPixmap(QPixmap(qsl(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
                        } else {
                            icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                            itemDescription = descInactiveParent.arg(itemDescription);
                        }
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                } else if (key->isActive()) {
                    if (key->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-pink.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    if (key->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-pink-locked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            } else {
                if (key->isActive()) {
                    itemDescription = descActive;
                    if (key->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descInactive;
                    if (key->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            pItem->setIcon(0, icon);
        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(qsl(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            itemDescription = descError;
            pItem->setIcon(0, iconError);
            showError(key->getError());
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);
    }
}
void dlgTriggerEditor::populateActions()
{
    std::list<TAction*> const baseNodeList_action = mpHost->getActionUnit()->getActionRootNodeList();
    for (auto action : baseNodeList_action) {
        if (action->isTemporary()) {
            continue;
        }

        const QString s = action->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem(mpActionBaseItem, sList);
        pItem->setData(0, Qt::UserRole, QVariant(action->getID()));
        mpActionBaseItem->addChild(pItem);
        QIcon icon;
        QString itemDescription;
        if (action->hasChildren()) {
            expand_child_action(action, pItem);
        }
        if (action->state()) {
            clearEditorNotification();
            const bool itemActive = action->isActive();
            if (action->isFolder()) {
                itemDescription = (itemActive ? descActiveFolder : descInactiveFolder);
                if (!action->mPackageName.isEmpty()) {
                    if (itemActive) {
                        if (action->ancestorsActive()) {
                            icon.addPixmap(QPixmap(qsl(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
                        } else {
                            icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                            itemDescription = descInactiveParent.arg(itemDescription);
                        }
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                } else if (!action->getParent() || !action->getParent()->mPackageName.isEmpty()) {
                    if (itemActive) {
                        if (action->ancestorsActive()) {
                            icon.addPixmap(QPixmap(qsl(":/icons/folder-yellow.png")), QIcon::Normal, QIcon::Off);
                        } else {
                            icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                            itemDescription = descInactiveParent.arg(itemDescription);
                        }
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-yellow-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                } else {
                    if (itemActive) {
                        if (action->ancestorsActive()) {
                            icon.addPixmap(QPixmap(qsl(":/icons/folder-cyan.png")), QIcon::Normal, QIcon::Off);
                        } else {
                            icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                            itemDescription = descInactiveParent.arg(itemDescription);
                        }
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-cyan-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            } else {
                if (itemActive) {
                    itemDescription = descActive;
                    if (action->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    itemDescription = descInactive;
                }
            }
            pItem->setIcon(0, icon);
        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(qsl(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            itemDescription = descError;
            pItem->setIcon(0, iconError);
            showError(action->getError());
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);
    }
}
void dlgTriggerEditor::populateAliases()
{
    std::list<TAlias*> const baseNodeList_alias = mpHost->getAliasUnit()->getAliasRootNodeList();
    for (auto alias : baseNodeList_alias) {
        if (alias->isTemporary()) {
            continue;
        }

        const QString s = alias->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem(mpAliasBaseItem, sList);
        pItem->setData(0, Qt::UserRole, QVariant(alias->getID()));
        mpAliasBaseItem->addChild(pItem);
        QIcon icon;
        QString itemDescription;
        const bool itemActive = alias->isActive();
        if (alias->hasChildren()) {
            expand_child_alias(alias, pItem);
        }
        if (alias->state()) {
            clearEditorNotification();

            if (alias->isFolder()) {
                itemDescription = (itemActive ? descActiveFolder : descInactiveFolder);
                if (!alias->mPackageName.isEmpty()) {
                    if (itemActive) {
                        if (alias->ancestorsActive()) {
                            icon.addPixmap(QPixmap(qsl(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
                        } else {
                            icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                            itemDescription = descInactiveParent.arg(itemDescription);
                        }
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                } else if (itemActive) {
                    if (alias->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-violet.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    if (alias->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-violet-locked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            } else {
                if (alias->isActive()) {
                    itemDescription = descActive;
                    if (alias->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descInactive;
                    if (alias->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            pItem->setIcon(0, icon);
        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(qsl(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            itemDescription = descError;
            pItem->setIcon(0, iconError);
            showError(alias->getError());
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);
    }
}
void dlgTriggerEditor::populateScripts()
{
    std::list<TScript*> const baseNodeList_scripts = mpHost->getScriptUnit()->getScriptRootNodeList();
    for (auto script : baseNodeList_scripts) {
        const QString s = script->getName();

        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem(mpScriptsBaseItem, sList);
        pItem->setData(0, Qt::UserRole, QVariant(script->getID()));
        mpScriptsBaseItem->addChild(pItem);
        QIcon icon;
        QString itemDescription;
        const bool itemActive = script->isActive();
        if (script->hasChildren()) {
            expand_child_scripts(script, pItem);
        }
        if (script->state()) {
            clearEditorNotification();

            if (script->isFolder()) {
                itemDescription = (itemActive ? descActiveFolder : descInactiveFolder);
                if (!script->mPackageName.isEmpty()) {
                    if (itemActive) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                } else {
                    if (itemActive) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-orange.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-orange-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            } else {
                if (script->isActive()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    itemDescription = descActive;
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    itemDescription = descInactive;
                }
            }
            pItem->setIcon(0, icon);
        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(qsl(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            itemDescription = descError;
            pItem->setIcon(0, iconError);
            showError(script->getError());
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);
    }
}
void dlgTriggerEditor::populateTimers()
{
    std::list<TTimer*> const baseNodeList_timers = mpHost->getTimerUnit()->getTimerRootNodeList();
    for (auto timer : baseNodeList_timers) {
        if (timer->isTemporary()) {
            continue;
        }
        const QString s = timer->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem(mpTimerBaseItem, sList);
        pItem->setData(0, Qt::UserRole, QVariant(timer->getID()));
        mpTimerBaseItem->addChild(pItem);
        QIcon icon;
        QString itemDescription;
        const bool itemActive = timer->isActive();
        if (timer->hasChildren()) {
            expand_child_timers(timer, pItem);
        }
        if (timer->state()) {
            clearEditorNotification();

            if (timer->isFolder()) {
                itemDescription = (itemActive ? descActiveFolder : descInactiveFolder);
                if (!timer->mPackageName.isEmpty()) {
                    if (itemActive) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                } else {
                    if (timer->shouldBeActive()) {
                        if (timer->ancestorsActive()) {
                            icon.addPixmap(QPixmap(qsl(":/icons/folder-green.png")), QIcon::Normal, QIcon::Off);
                        } else {
                            icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                            itemDescription = descInactiveParent.arg(itemDescription);
                        }
                    } else {
                        if (timer->ancestorsActive()) {
                            icon.addPixmap(QPixmap(qsl(":/icons/folder-green-locked.png")), QIcon::Normal, QIcon::Off);
                        } else {
                            icon.addPixmap(QPixmap(qsl(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                        }
                    }
                }
            } else {
                if (timer->isOffsetTimer()) {
                    if (timer->shouldBeActive()) {
                        itemDescription = descActiveOffsetTimer;
                        if (timer->ancestorsActive()) {
                            icon.addPixmap(QPixmap(qsl(":/icons/offsettimer-on.png")), QIcon::Normal, QIcon::Off);
                        } else {
                            icon.addPixmap(QPixmap(qsl(":/icons/offsettimer-on-grey.png")), QIcon::Normal, QIcon::Off);
                            itemDescription = descInactiveParent.arg(itemDescription);
                        }
                    } else {
                        itemDescription = descInactiveOffsetTimer;
                        if (timer->ancestorsActive()) {
                            icon.addPixmap(QPixmap(qsl(":/icons/offsettimer-off.png")), QIcon::Normal, QIcon::Off);
                        } else {
                            icon.addPixmap(QPixmap(qsl(":/icons/offsettimer-off-grey.png")), QIcon::Normal, QIcon::Off);
                        }
                    }
                } else {
                    if (timer->shouldBeActive()) {
                        itemDescription = descActive;
                        if (timer->ancestorsActive()) {
                            icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                        } else {
                            icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                            itemDescription = descInactiveParent.arg(itemDescription);
                        }
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descInactive;
                    }
                }
            }
            pItem->setIcon(0, icon);
        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(qsl(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            itemDescription = descError;
            pItem->setIcon(0, iconError);
            showError(timer->getError());
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);
    }
}
void dlgTriggerEditor::populateTriggers()
{
    std::list<TTrigger*> const baseNodeList = mpHost->getTriggerUnit()->getTriggerRootNodeList();
    for (auto trigger : baseNodeList) {
        if (trigger->isTemporary()) {
            continue;
        }
        const QString s = trigger->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem(mpTriggerBaseItem, sList);
        pItem->setData(0, Qt::UserRole, QVariant(trigger->getID()));
        mpTriggerBaseItem->addChild(pItem);
        QIcon icon;
        QString itemDescription;
        const bool itemActive = trigger->isActive();
        if (trigger->hasChildren()) {
            expand_child_triggers(trigger, pItem);
        }
        if (trigger->state()) {
            clearEditorNotification();

            if (trigger->isFilterChain()) {
                if (itemActive) {
                    itemDescription = descActiveFilterChain;
                    if (trigger->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/filter.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/filter-grey.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descInactiveFilterChain;
                    if (trigger->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/filter-locked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/filter-grey-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            } else if (trigger->isFolder()) {
                itemDescription = (itemActive ? descActiveFolder : descInactiveFolder);
                if (!trigger->mPackageName.isEmpty()) {
                    if (itemActive) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                } else if (itemActive) {
                    if (trigger->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-blue.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    if (trigger->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-blue-locked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            } else {
                if (itemActive) {
                    itemDescription = descActive;
                    if (trigger->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descInactive;
                    if (trigger->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            pItem->setIcon(0, icon);
        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(qsl(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            itemDescription = descError;
            pItem->setIcon(0, iconError);
            showError(trigger->getError());
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);
    }
}

void dlgTriggerEditor::repopulateVars()
{
    treeWidget_variables->setUpdatesEnabled(false);
    mpVarBaseItem = new QTreeWidgetItem(QStringList(tr("Variables")));
    mpVarBaseItem->setTextAlignment(0, Qt::AlignLeft | Qt::AlignVCenter);
    mpVarBaseItem->setIcon(0, QPixmap(qsl(":/icons/variables.png")));
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
        const QString s = trigger->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem(pWidgetItemParent, sList);
        pItem->setData(0, Qt::UserRole, trigger->getID());

        pWidgetItemParent->insertChild(0, pItem);
        QIcon icon;
        QString itemDescription;
        if (trigger->hasChildren()) {
            expand_child_triggers(trigger, pItem);
        }
        if (trigger->state()) {
            clearEditorNotification();

            if (trigger->isFilterChain()) {
                if (trigger->isActive()) {
                    itemDescription = descActiveFilterChain;
                    if (trigger->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/filter.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/filter-grey.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descInactiveFilterChain;
                    if (trigger->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/filter-locked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/filter-grey-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            } else if (trigger->isFolder()) {
                if (trigger->isActive()) {
                    itemDescription = descActiveFolder;
                    if (trigger->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-blue.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descInactiveFolder;
                    if (trigger->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-blue-locked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            } else {
                if (trigger->isActive()) {
                    itemDescription = descActive;
                    if (trigger->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descInactive;
                    if (trigger->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            //pItem->setDisabled(!trigger->ancestorsActive());
            pItem->setIcon(0, icon);
        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(qsl(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            itemDescription = descError;
            pItem->setIcon(0, iconError);
            showError(trigger->getError());
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);
    }
}

void dlgTriggerEditor::expand_child_key(TKey* pTriggerParent, QTreeWidgetItem* pWidgetItemParent)
{
    std::list<TKey*>* childrenList = pTriggerParent->getChildrenList();
    for (auto key : *childrenList) {
        const QString s = key->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem(pWidgetItemParent, sList);
        pItem->setData(0, Qt::UserRole, key->getID());

        pWidgetItemParent->insertChild(0, pItem);
        QIcon icon;
        QString itemDescription;
        if (key->hasChildren()) {
            expand_child_key(key, pItem);
        }
        if (key->state()) {
            clearEditorNotification();

            if (key->isFolder()) {
                if (key->isActive()) {
                    itemDescription = descActiveFolder;
                    if (key->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-pink.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descInactiveFolder;
                    if (key->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-pink-locked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            } else {
                if (key->isActive()) {
                    itemDescription = descActive;
                    if (key->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descInactive;
                    if (key->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            pItem->setIcon(0, icon);
        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(qsl(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            itemDescription = descError;
            pItem->setIcon(0, iconError);
            showError(key->getError());
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);
    }
}


void dlgTriggerEditor::expand_child_scripts(TScript* pTriggerParent, QTreeWidgetItem* pWidgetItemParent)
{
    std::list<TScript*>* childrenList = pTriggerParent->getChildrenList();
    for (auto script : *childrenList) {
        const QString s = script->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem(pWidgetItemParent, sList);
        pItem->setData(0, Qt::UserRole, script->getID());

        pWidgetItemParent->insertChild(0, pItem);
        QIcon icon;
        QString itemDescription;
        if (script->hasChildren()) {
            expand_child_scripts(script, pItem);
        }
        if (script->state()) {
            clearEditorNotification();

            if (script->isFolder()) {
                if (script->isActive()) {
                    itemDescription = descActiveFolder;
                    if (script->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-orange.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-orange-locked.png")), QIcon::Normal, QIcon::Off);
                    itemDescription = descInactiveFolder;
                }
            } else {
                if (script->isActive()) {
                    itemDescription = descActive;
                    if (script->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    itemDescription = descInactive;
                }
            }
            pItem->setIcon(0, icon);
        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(qsl(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            itemDescription = descError;
            pItem->setIcon(0, iconError);
            showError(script->getError());
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);
    }
}

void dlgTriggerEditor::expand_child_alias(TAlias* pTriggerParent, QTreeWidgetItem* pWidgetItemParent)
{
    std::list<TAlias*>* childrenList = pTriggerParent->getChildrenList();
    for (auto alias : *childrenList) {
        const QString s = alias->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem(pWidgetItemParent, sList);
        pItem->setData(0, Qt::UserRole, alias->getID());

        pWidgetItemParent->insertChild(0, pItem);
        QIcon icon;
        QString itemDescription;
        if (alias->hasChildren()) {
            expand_child_alias(alias, pItem);
        }
        if (alias->state()) {
            clearEditorNotification();

            if (alias->isFolder()) {
                if (alias->isActive()) {
                    itemDescription = descActiveFolder;
                    if (alias->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-violet.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descInactiveFolder;
                    if (alias->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-violet-locked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            } else {
                if (alias->isActive()) {
                    itemDescription = descActive;
                    if (alias->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descInactive;
                    if (alias->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            pItem->setIcon(0, icon);
        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(qsl(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            itemDescription = descError;
            pItem->setIcon(0, iconError);
            showError(alias->getError());
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);
    }
}

void dlgTriggerEditor::expand_child_action(TAction* pTriggerParent, QTreeWidgetItem* pWidgetItemParent)
{
    std::list<TAction*>* childrenList = pTriggerParent->getChildrenList();
    for (auto action : *childrenList) {
        const QString s = action->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem(pWidgetItemParent, sList);
        pItem->setData(0, Qt::UserRole, action->getID());

        pWidgetItemParent->insertChild(0, pItem);
        QIcon icon;
        QString itemDescription;
        if (action->hasChildren()) {
            expand_child_action(action, pItem);
        }
        if (action->state()) {
            clearEditorNotification();

            if (!action->getParent()->mPackageName.isEmpty()) {
                // Must have a parent (or would not be IN this method) and the
                // parent has a package name - this is a toolbar
                if (action->isActive()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-yellow.png")), QIcon::Normal, QIcon::Off);
                    itemDescription = descActiveFolder;
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-yellow-locked.png")), QIcon::Normal, QIcon::Off);
                    itemDescription = descInactiveFolder;
                }
            } else if (action->isFolder()) {
                // Is a folder and is not a toolbar - this is a menu
                if (action->isActive()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-cyan.png")), QIcon::Normal, QIcon::Off);
                    itemDescription = descActiveFolder;
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-cyan-locked.png")), QIcon::Normal, QIcon::Off);
                    itemDescription = descInactiveFolder;
                }
            } else {
                // Is a button
                if (action->isActive()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    itemDescription = descActive;
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    itemDescription = descInactive;
                }
            }
            pItem->setIcon(0, icon);
        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(qsl(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            itemDescription = descError;
            pItem->setIcon(0, iconError);
            showError(action->getError());
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);
    }
}

void dlgTriggerEditor::expand_child_timers(TTimer* pTimerParent, QTreeWidgetItem* pWidgetItemParent)
{
    std::list<TTimer*>* childrenList = pTimerParent->getChildrenList();
    for (auto timer : *childrenList) {
        const QString s = timer->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem(pWidgetItemParent, sList);
        pItem->setData(0, Qt::UserRole, timer->getID());

        pWidgetItemParent->insertChild(0, pItem);
        QIcon icon;
        QString itemDescription;
        if (timer->hasChildren()) {
            expand_child_timers(timer, pItem);
        }
        if (timer->state()) {
            clearEditorNotification();

            if (timer->isFolder()) {
                if (timer->shouldBeActive()) {
                    if (timer->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-green.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descActiveFolder;
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descInactiveFolder;
                    if (timer->ancestorsActive()) {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-green-locked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            } else {
                if (timer->isOffsetTimer()) {
                    if (timer->shouldBeActive()) {
                        itemDescription = descActiveOffsetTimer;
                        if (timer->ancestorsActive()) {
                            icon.addPixmap(QPixmap(qsl(":/icons/offsettimer-on.png")), QIcon::Normal, QIcon::Off);
                        } else {
                            icon.addPixmap(QPixmap(qsl(":/icons/offsettimer-on-grey.png")), QIcon::Normal, QIcon::Off);
                            itemDescription = descInactiveParent.arg(itemDescription);
                        }
                    } else {
                        itemDescription = descInactiveOffsetTimer;
                        if (timer->ancestorsActive()) {
                            icon.addPixmap(QPixmap(qsl(":/icons/offsettimer-off.png")), QIcon::Normal, QIcon::Off);
                        } else {
                            icon.addPixmap(QPixmap(qsl(":/icons/offsettimer-off-grey.png")), QIcon::Normal, QIcon::Off);
                        }
                    }
                } else {
                    if (timer->shouldBeActive()) {
                        itemDescription = descActive;
                        if (timer->ancestorsActive()) {
                            icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                        } else {
                            icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                            itemDescription = descInactiveParent.arg(itemDescription);
                        }
                    } else {
                        itemDescription = descInactive;
                        if (timer->ancestorsActive()) {
                            icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                        } else {
                            icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                        }
                    }
                }
            }
            pItem->setIcon(0, icon);
        } else {
            QIcon iconError;
            iconError.addPixmap(QPixmap(qsl(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            itemDescription = descError;
            pItem->setIcon(0, iconError);
            showError(timer->getError());
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);
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
    case EditorViewType::cmUnknownView:
        return; // Silently ignore this case
    }
}

void dlgTriggerEditor::timerEvent(QTimerEvent* event)
{
    Q_UNUSED(event);

    if (isActiveWindow()) {
        autoSave();
    }
}

void dlgTriggerEditor::autoSave()
{
    mpHost->saveProfile(QString(), qsl("autosave"));
}

void dlgTriggerEditor::enterEvent(TEnterEvent* event)
{
    Q_UNUSED(event)
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
    Q_UNUSED(pE)
    qDebug() << "focusInEvent fired!!";
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

    // in lieu of readonly
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

    mpExportAction->setEnabled(view != EditorViewType::cmVarsView);

    // texts are duplicated here so that translators can work with the full string
    switch (mCurrentView) {
    case EditorViewType::cmTriggerView:
        // PLACEMARKER 2/3 save button texts need to be kept in sync
        mAddItem->setText(tr("Add Trigger"));
        mAddItem->setStatusTip(tr("Add new trigger"));
        mAddGroup->setText(tr("Add Trigger Group"));
        mAddGroup->setStatusTip(tr("Add new group of triggers"));
        mDeleteItem->setText(tr("Delete Trigger"));
        mDeleteItem->setStatusTip(tr("Delete the selected trigger"));
        mSaveItem->setText(tr("Save Trigger"));
        mSaveItem->setStatusTip(tr("Saves the selected trigger, causing new changes to take effect - does not save to disk though..."));
        break;
    case EditorViewType::cmTimerView:
        mAddItem->setText(tr("Add Timer"));
        mAddItem->setStatusTip(tr("Add new timer"));
        mAddGroup->setText(tr("Add Timer Group"));
        mAddGroup->setStatusTip(tr("Add new group of timers"));
        mDeleteItem->setText(tr("Delete Timer"));
        mDeleteItem->setStatusTip(tr("Delete the selected timer"));
        mSaveItem->setText(tr("Save Timer"));
        mSaveItem->setStatusTip(tr("Saves the selected timer, causing new changes to take effect - does not save to disk though..."));
        break;
    case EditorViewType::cmAliasView:
        mAddItem->setText(tr("Add Alias"));
        mAddItem->setStatusTip(tr("Add new alias"));
        mAddGroup->setText(tr("Add Alias Group"));
        mAddGroup->setStatusTip(tr("Add new group of aliases"));
        mDeleteItem->setText(tr("Delete Alias"));
        mDeleteItem->setStatusTip(tr("Delete the selected alias"));
        mSaveItem->setText(tr("Save Alias"));
        mSaveItem->setStatusTip(tr("Saves the selected alias, causing new changes to take effect - does not save to disk though..."));
        break;
    case EditorViewType::cmScriptView:
        mAddItem->setText(tr("Add Script"));
        mAddItem->setStatusTip(tr("Add new script"));
        mAddGroup->setText(tr("Add Script Group"));
        mAddGroup->setStatusTip(tr("Add new group of scripts"));
        mDeleteItem->setText(tr("Delete Script"));
        mDeleteItem->setStatusTip(tr("Delete the selected script"));
        mSaveItem->setText(tr("Save Script"));
        mSaveItem->setStatusTip(tr("Saves the selected script, causing new changes to take effect - does not save to disk though..."));
        break;
    case EditorViewType::cmActionView:
        mAddItem->setText(tr("Add Button"));
        mAddItem->setStatusTip(tr("Add new button"));
        mAddGroup->setText(tr("Add Button Group"));
        mAddGroup->setStatusTip(tr("Add new group of buttons"));
        mDeleteItem->setText(tr("Delete Button"));
        mDeleteItem->setStatusTip(tr("Delete the selected button"));
        mSaveItem->setText(tr("Save Button"));
        mSaveItem->setStatusTip(tr("Saves the selected button, causing new changes to take effect - does not save to disk though..."));
        break;
    case EditorViewType::cmKeysView:
        mAddItem->setText(tr("Add Key"));
        mAddItem->setStatusTip(tr("Add new key"));
        mAddGroup->setText(tr("Add Key Group"));
        mAddGroup->setStatusTip(tr("Add new group of keys"));
        mDeleteItem->setText(tr("Delete Key"));
        mDeleteItem->setStatusTip(tr("Delete the selected key"));
        mSaveItem->setText(tr("Save Key"));
        mSaveItem->setStatusTip(tr("Saves the selected key, causing new changes to take effect - does not save to disk though..."));
        break;
    case EditorViewType::cmVarsView:
        mAddItem->setText(tr("Add Variable"));
        mAddItem->setStatusTip(tr("Add new variable"));
        mAddGroup->setText(tr("Add Lua table"));
        mAddGroup->setStatusTip(tr("Add new Lua table"));
        mDeleteItem->setText(tr("Delete Variable"));
        mDeleteItem->setStatusTip(tr("Delete the selected variable"));
        mSaveItem->setText(tr("Save Variable"));
        mSaveItem->setStatusTip(tr("Saves the selected variable, causing new changes to take effect - does not save to disk though..."));
        break;
    default:
        qDebug() << "ERROR: dlgTriggerEditor::changeView() undefined view";
    }
}

void dlgTriggerEditor::slot_showTimers()
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
        slot_timerSelected(treeWidget_timers->currentItem());
    }
    if (!mTimerEditorSplitterState.isEmpty()) {
        splitter_right->restoreState(mTimerEditorSplitterState);
    } else {
        const QList<int> sizes = {30, 900, 30};
        splitter_right->setSizes(sizes);
        mTimerEditorSplitterState = splitter_right->saveState();
    }
    treeWidget_timers->setFocus();
}

void dlgTriggerEditor::showCurrentTriggerItem()
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
        slot_triggerSelected(treeWidget_triggers->currentItem());
    }
}

void dlgTriggerEditor::slot_showTriggers()
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
        slot_triggerSelected(treeWidget_triggers->currentItem());
    }
    if (!mTriggerEditorSplitterState.isEmpty()) {
        splitter_right->restoreState(mTriggerEditorSplitterState);
    } else {
        const QList<int> sizes = {30, 900, 30};
        splitter_right->setSizes(sizes);
        mTriggerEditorSplitterState = splitter_right->saveState();
    }
    treeWidget_triggers->setFocus();
}

void dlgTriggerEditor::slot_showScripts()
{
    changeView(EditorViewType::cmScriptView);
    QTreeWidgetItem* pI = treeWidget_scripts->topLevelItem(0);
    if (!pI || pI == treeWidget_scripts->currentItem() || !pI->childCount()) {
        // There is no root item, we are on the root item or there are no other
        // items - so show the help message:
        clearScriptForm();
    } else {
        mpScriptsMainArea->show();
        mpSourceEditorArea->show();
        slot_scriptsSelected(treeWidget_scripts->currentItem());
    }
    if (!mScriptEditorSplitterState.isEmpty()) {
        splitter_right->restoreState(mScriptEditorSplitterState);
    } else {
        const QList<int> sizes = {30, 900, 30};
        splitter_right->setSizes(sizes);
        mScriptEditorSplitterState = splitter_right->saveState();
    }
    treeWidget_scripts->setFocus();
}

void dlgTriggerEditor::slot_showKeys()
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
        slot_keySelected(treeWidget_keys->currentItem());
    }
    if (!mKeyEditorSplitterState.isEmpty()) {
        splitter_right->restoreState(mKeyEditorSplitterState);
    } else {
        const QList<int> sizes = {30, 900, 30};
        splitter_right->setSizes(sizes);
        mKeyEditorSplitterState = splitter_right->saveState();
    }
    treeWidget_keys->setFocus();
}

void dlgTriggerEditor::slot_showVariables()
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
        slot_variableSelected(treeWidget_variables->currentItem());
    }
    if (!mVarEditorSplitterState.isEmpty()) {
        splitter_right->restoreState(mVarEditorSplitterState);
    } else {
        const QList<int> sizes = {30, 900, 30};
        splitter_right->setSizes(sizes);
        mVarEditorSplitterState = splitter_right->saveState();
    }
    treeWidget_variables->setFocus();
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
            slot_variableSelected(treeWidget_variables->currentItem());
        } else {
            mpVarsMainArea->hide();
            showInfo(msgInfoAddVar);
        }
    }
    treeWidget_variables->show();
}


void dlgTriggerEditor::slot_showAliases()
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
        slot_aliasSelected(treeWidget_aliases->currentItem());
    }
    if (!mAliasEditorSplitterState.isEmpty()) {
        splitter_right->restoreState(mAliasEditorSplitterState);
    } else {
        const QList<int> sizes = {30, 900, 30};
        splitter_right->setSizes(sizes);
        mAliasEditorSplitterState = splitter_right->saveState();
    }
    treeWidget_aliases->setFocus();
}

void dlgTriggerEditor::showError(const QString& error)
{
    mpSystemMessageArea->notificationAreaIconLabelInformation->hide();
    mpSystemMessageArea->notificationAreaIconLabelError->show();
    mpSystemMessageArea->notificationAreaIconLabelWarning->hide();
    mpSystemMessageArea->notificationAreaMessageBox->setText(error);
    mpSystemMessageArea->show();
    if (!mpHost->mIsProfileLoadingSequence) {
        mudlet::self()->announce(error);
    }
}

void dlgTriggerEditor::showInfo(const QString& error)
{
    mpSystemMessageArea->notificationAreaIconLabelError->hide();
    mpSystemMessageArea->notificationAreaIconLabelWarning->hide();
    mpSystemMessageArea->notificationAreaIconLabelInformation->show();
    mpSystemMessageArea->notificationAreaMessageBox->setText(error);
    mpSystemMessageArea->show();
    if (!mpHost->mIsProfileLoadingSequence) {
        mudlet::self()->announce(error);
    }
}

void dlgTriggerEditor::showWarning(const QString& error)
{
    mpSystemMessageArea->notificationAreaIconLabelInformation->hide();
    mpSystemMessageArea->notificationAreaIconLabelError->hide();
    mpSystemMessageArea->notificationAreaIconLabelWarning->show();
    mpSystemMessageArea->notificationAreaMessageBox->setText(error);
    mpSystemMessageArea->show();
    if (!mpHost->mIsProfileLoadingSequence) {
        mudlet::self()->announce(error);
    }
}

void dlgTriggerEditor::slot_showActions()
{
    changeView(EditorViewType::cmActionView);
    QTreeWidgetItem* pI = treeWidget_actions->topLevelItem(0);
    if (!pI || pI == treeWidget_actions->currentItem() || !pI->childCount()) {
        // There is no root item, we are on the root item or there are no other
        // items - so show the help message:
        clearActionForm();
    } else {
        mpActionsMainArea->show();
        mpSourceEditorArea->show();
        slot_actionSelected(treeWidget_actions->currentItem());
    }
    if (!mActionEditorSplitterState.isEmpty()) {
        splitter_right->restoreState(mActionEditorSplitterState);
    } else {
        const QList<int> sizes = {30, 900, 30};
        splitter_right->setSizes(sizes);
        mActionEditorSplitterState = splitter_right->saveState();
    }
    treeWidget_actions->setFocus();
}

void dlgTriggerEditor::slot_saveEdits()
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
        qWarning() << "ERROR: dlgTriggerEditor::slot_saveEdits() undefined view, not sure what to save";
    }

    // There was a mpHost->serialize() call here, but that code was
    // "short-circuited" and returned without doing anything;
}

void dlgTriggerEditor::slot_addNewItem()
{
    switch (mCurrentView) {
    case EditorViewType::cmTriggerView:
        // addTrigger(false); //add normal trigger
        addTriggerCommand(false);
        mpTriggersMainArea->lineEdit_trigger_name->setFocus();
        mpTriggersMainArea->lineEdit_trigger_name->selectAll();
        break;
    case EditorViewType::cmTimerView:
        // addTimer(false); //add normal timer
        addTimerCommand(false);
        mpTimersMainArea->lineEdit_timer_name->setFocus();
        mpTimersMainArea->lineEdit_timer_name->selectAll();
        break;
    case EditorViewType::cmAliasView:
        // addAlias(false); //add normal alias
        addAliasCommand(false);
        mpAliasMainArea->lineEdit_alias_name->setFocus();
        mpAliasMainArea->lineEdit_alias_name->selectAll();
        break;
    case EditorViewType::cmScriptView:
        // addScript(false); //add normal script
        addScriptCommand(false);
        mpScriptsMainArea->lineEdit_script_name->setFocus();
        mpScriptsMainArea->lineEdit_script_name->selectAll();
        break;
    case EditorViewType::cmActionView:
        // addAction(false); //add normal action
        addActionCommand(false);
        mpActionsMainArea->lineEdit_action_name->setFocus();
        mpActionsMainArea->lineEdit_action_name->selectAll();
        break;
    case EditorViewType::cmKeysView:
        // addKey(false); //add normal key
        addKeyCommand(false);
        mpKeysMainArea->lineEdit_key_name->setFocus();
        mpKeysMainArea->lineEdit_key_name->selectAll();
        break;
    case EditorViewType::cmVarsView:
        // addVar(false); //add variable
        addVarCommand(false);
        mpVarsMainArea->lineEdit_var_name->setFocus();
        // variables start without a default name
        break;
    default:
        qDebug() << "ERROR: dlgTriggerEditor::slot_saveEdits() undefined view";
    }
}

void dlgTriggerEditor::slot_addNewGroup()
{
    switch (mCurrentView) {
    case EditorViewType::cmTriggerView:
        // addTrigger(true); //add trigger group
        addTriggerCommand(true);
        mpTriggersMainArea->lineEdit_trigger_name->setFocus();
        mpTriggersMainArea->lineEdit_trigger_name->selectAll();
        break;
    case EditorViewType::cmTimerView:
        // addTimer(true); //add timer group
        addTimerCommand(true);
        mpTimersMainArea->lineEdit_timer_name->setFocus();
        mpTimersMainArea->lineEdit_timer_name->selectAll();
        break;
    case EditorViewType::cmAliasView:
        // addAlias(true); //add alias group
        addAliasCommand(true);
        mpAliasMainArea->lineEdit_alias_name->setFocus();
        mpAliasMainArea->lineEdit_alias_name->selectAll();
        break;
    case EditorViewType::cmScriptView:
        // addScript(true); //add script group
        addScriptCommand(true);
        mpScriptsMainArea->lineEdit_script_name->setFocus();
        mpScriptsMainArea->lineEdit_script_name->selectAll();
        break;
    case EditorViewType::cmActionView:
        // addAction(true); //add action group
        addActionCommand(true);
        mpActionsMainArea->lineEdit_action_name->setFocus();
        mpActionsMainArea->lineEdit_action_name->selectAll();
        break;
    case EditorViewType::cmKeysView:
        // addKey(true); //add keys group
        addKeyCommand(true);
        mpKeysMainArea->lineEdit_key_name->setFocus();
        mpKeysMainArea->lineEdit_key_name->selectAll();
        break;
    case EditorViewType::cmVarsView:
        // addVar(true); // add lua table
        addVarCommand(true);
        mpVarsMainArea->lineEdit_var_name->setFocus();
        // variables start without a default name
        break;
    default:
        qDebug() << "ERROR: dlgTriggerEditor::slot_saveEdits() undefined view";
    }
}

void dlgTriggerEditor::slot_toggleItemOrGroupActiveFlag()
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
        qDebug() << "ERROR: dlgTriggerEditor::slot_saveEdits() undefined view";
    }
}

void dlgTriggerEditor::slot_sourceFindMove()
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

void dlgTriggerEditor::slot_openSourceFind()
{
    slot_sourceFindMove();
    mpSourceEditorFindArea->show();
    mpSourceEditorFindArea->lineEdit_findText->setFocus();
    mpSourceEditorFindArea->lineEdit_findText->selectAll();
}

void dlgTriggerEditor::slot_closeSourceFind()
{
    auto controller = mpSourceEditorEdbee->controller();
    controller->borderedTextRanges()->clear();
    controller->textSelection()->range(0).clearSelection();
    controller->update();
    mpSourceEditorFindArea->hide();
    mpSourceEditorEdbee->setFocus();
}

void dlgTriggerEditor::slot_sourceReplace()
{
    auto controller = mpSourceEditorEdbee->controller();
    auto replaceText = mpSourceEditorFindArea->lineEdit_replaceText->text();
    for (int i = 0; i < controller->textSelection()->rangeCount(); i++) {
        auto& range = controller->textSelection()->range(i);
        if (mpSourceEditorEdbee->textDocument()->text().mid(range.anchor(), range.length()) == replaceText) {
            slot_sourceFindNext();
            continue;
        }
        if (!range.hasSelection()) {
            slot_sourceFindPrevious();
            continue;
        }
        mpSourceEditorEdbee->textDocument()->replace(range.anchor(), range.length(), replaceText);
        range.setLength(mpSourceEditorFindArea->lineEdit_replaceText->text().length());
    }
}

void dlgTriggerEditor::slot_sourceFindPrevious()
{
    auto controller = mpSourceEditorEdbee->controller();
    auto searcher = controller->textSearcher();
    searcher->setSearchTerm(mpSourceEditorFindArea->lineEdit_findText->text());
    searcher->setCaseSensitive(false);
    searcher->findPrev(mpSourceEditorEdbee);
    controller->scrollCaretVisible();
    controller->update();
    slot_sourceFindMove();
}

void dlgTriggerEditor::slot_sourceFindNext()
{
    auto controller = mpSourceEditorEdbee->controller();
    auto searcher = controller->textSearcher();
    searcher->setSearchTerm(mpSourceEditorFindArea->lineEdit_findText->text());
    searcher->setCaseSensitive(false);
    searcher->findNext(mpSourceEditorEdbee);
    controller->scrollCaretVisible();
    controller->update();
    slot_sourceFindMove();
}

void dlgTriggerEditor::slot_sourceFindTextChanges()
{
    auto text = mpSourceEditorFindArea->lineEdit_findText->text();
    if (text.length() <= 2) {
        return;
    }

    auto controller = mpSourceEditorEdbee->controller();
    auto searcher = controller->textSearcher();
    controller->borderedTextRanges()->clear();
    controller->textSelection()->range(0).clearSelection();
    searcher->setSearchTerm(text);
    searcher->markAll(controller->borderedTextRanges());
    controller->update();
}

void dlgTriggerEditor::slot_deleteItemOrGroup()
{
    switch (mCurrentView) {
    case EditorViewType::cmTriggerView:
        // delete_trigger();
        deleteTriggerCommand();
        break;
    case EditorViewType::cmTimerView:
        // delete_timer();
        deleteTimerCommand();
        break;
    case EditorViewType::cmAliasView:
        // delete_alias();
        deleteAliasCommand();
        break;
    case EditorViewType::cmScriptView:
        // delete_script();
        deleteScriptCommand();
        break;
    case EditorViewType::cmActionView:
        // delete_action();
        deleteActionCommand();
        break;
    case EditorViewType::cmKeysView:
        // delete_key();
        deleteKeyCommand();
        break;
    case EditorViewType::cmVarsView:
        // delete_variable();
        deleteVarCommand();
        break;
    default:
        qDebug() << "ERROR: dlgTriggerEditor::slot_saveEdits() undefined view";
    }
}

void dlgTriggerEditor::slot_saveSelectedItem(QTreeWidgetItem* pItem)
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
    case EditorViewType::cmUnknownView:
        qWarning().nospace().noquote() << "dlgTriggerEditor::slot_saveSelectedItem() WARNING - switch(EditorViewType) not expected to be called for \"EditorViewType::cmUnknownView!\"";
    }
}

// Should the functionality change in this method be sure to review the code
// for "case SearchResultIsEventHandler" for "Scripts" in:
// slot_itemSelectedInSearchResults(...)
void dlgTriggerEditor::slot_scriptMainAreaEditHandler(QListWidgetItem*)
{
    QListWidgetItem* pItem = mpScriptsMainArea->listWidget_script_registered_event_handlers->currentItem();
    if (!pItem) {
        return;
    }
    mIsScriptsMainAreaEditHandler = true;
    mpScriptsMainAreaEditHandlerItem = pItem;
    const QString regex = pItem->text();
    if (regex.isEmpty()) {
        mIsScriptsMainAreaEditHandler = false;
        return;
    }
    mpScriptsMainArea->lineEdit_script_event_handler_entry->setText(regex);
}

void dlgTriggerEditor::slot_scriptMainAreaDeleteHandler()
{
    mpScriptsMainArea->listWidget_script_registered_event_handlers->takeItem(mpScriptsMainArea->listWidget_script_registered_event_handlers->currentRow());
}

void dlgTriggerEditor::slot_scriptMainAreaAddHandler()
{
    auto addEventHandler = [&]() {
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

void dlgTriggerEditor::slot_toggleCentralDebugConsole()
{
    mudlet::self()->attachDebugArea(mpHost->getName());

    mudlet::smpDebugArea->setVisible(!mudlet::smDebugMode);
    mudlet::smDebugMode = !mudlet::smDebugMode;
    mudlet::smpDebugArea->setWindowTitle(tr("Central Debug Console"));
    if (mudlet::smDebugMode) {
        // If this is the first time the window is shown we want any previously
        // enqueued messages to be painted onto the central debug console:
        TDebug::flushMessageQueue();
    }
    mudlet::self()->refreshTabBar();
}

void dlgTriggerEditor::slot_nextSection()
{
    switch (mCurrentView) {
    case EditorViewType::cmTriggerView:
        if (qsl("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            treeWidget_triggers->setFocus();
            return;
        }
        if (treeWidget_triggers->hasFocus()) {
            mpTriggersMainArea->lineEdit_trigger_name->setFocus();
            return;
        }
        if (mpTriggersMainArea->hasFocus()) {
            mTriggerPatternEdit[0]->lineEdit_pattern->setFocus();
            return;
        }
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
        break;
    case EditorViewType::cmTimerView:
        if (qsl("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            treeWidget_timers->setFocus();
            return;
        }
        if (treeWidget_timers->hasFocus()) {
            mpTimersMainArea->lineEdit_timer_name->setFocus();
            return;
        }
        for (auto child : mpTimersMainArea->findChildren<QWidget*>()) {
            if (child->hasFocus()) {
                mpSourceEditorEdbee->setFocus();
                return;
            }
        }
        break;
    case EditorViewType::cmAliasView:
        if (QString("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            treeWidget_aliases->setFocus();
            return;
        }
        if (treeWidget_aliases->hasFocus()) {
            mpAliasMainArea->lineEdit_alias_name->setFocus();
            return;
        }
        for (auto child : mpAliasMainArea->findChildren<QWidget*>()) {
            if (child->hasFocus()) {
                mpSourceEditorEdbee->setFocus();
                return;
            }
        }
        break;
    case EditorViewType::cmScriptView:
        if (qsl("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            treeWidget_scripts->setFocus();
            return;
        }
        if (treeWidget_scripts->hasFocus()) {
            mpScriptsMainArea->lineEdit_script_name->setFocus();
            return;
        }
        for (auto child : mpScriptsMainArea->findChildren<QWidget*>()) {
            if (child->hasFocus()) {
                mpSourceEditorEdbee->setFocus();
                return;
            }
        }
        break;
    case EditorViewType::cmActionView:
        if (qsl("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            treeWidget_actions->setFocus();
            return;
        }
        if (treeWidget_actions->hasFocus()) {
            mpActionsMainArea->lineEdit_action_name->setFocus();
            return;
        }
        for (auto child : mpActionsMainArea->findChildren<QWidget*>()) {
            if (child->hasFocus()) {
                mpSourceEditorEdbee->setFocus();
                return;
            }
        }
        break;
    case EditorViewType::cmKeysView:
        if (qsl("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            treeWidget_keys->setFocus();
            return;
        }
        if (treeWidget_keys->hasFocus()) {
            mpKeysMainArea->lineEdit_key_name->setFocus();
            return;
        }
        for (auto child : mpKeysMainArea->findChildren<QWidget*>()) {
            if (child->hasFocus()) {
                mpSourceEditorEdbee->setFocus();
                return;
            }
        }
        break;
    case EditorViewType::cmVarsView:
        if (qsl("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            treeWidget_variables->setFocus();
            return;
        }
        if (treeWidget_variables->hasFocus()) {
            mpVarsMainArea->lineEdit_var_name->setFocus();
            return;
        }
        for (auto child : mpVarsMainArea->findChildren<QWidget*>()) {
            if (child->hasFocus()) {
                mpSourceEditorEdbee->setFocus();
                return;
            }
        }
        break;
    case EditorViewType::cmUnknownView:
        return;
    }
}

void dlgTriggerEditor::slot_previousSection()
{
    switch (mCurrentView) {
    case EditorViewType::cmTriggerView:
        if (QString("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            mTriggerPatternEdit[0]->lineEdit_pattern->setFocus();
            return;
        }
        if (treeWidget_triggers->hasFocus()) {
            mpSourceEditorEdbee->setFocus();
            return;
        }
        for (auto child : mpTriggersMainArea->scrollArea->findChildren<QWidget*>()) {
            if (child->hasFocus()) {
                mpTriggersMainArea->lineEdit_trigger_name->setFocus();
                return;
            }
        }
        for (auto child : mpTriggersMainArea->findChildren<QWidget*>()) {
            if (child->hasFocus()) {
                treeWidget_triggers->setFocus();
                return;
            }
        }
        break;
    case EditorViewType::cmTimerView:
        if (QString("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            mpTimersMainArea->lineEdit_timer_name->setFocus();
            return;
        }
        if (treeWidget_timers->hasFocus()) {
            mpSourceEditorEdbee->setFocus();
            return;
        }
        for (auto child : mpTimersMainArea->findChildren<QWidget*>()) {
            if (child->hasFocus()) {
                treeWidget_timers->setFocus();
                return;
            }
        }
        break;
    case EditorViewType::cmAliasView:
        if (QString("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            mpAliasMainArea->lineEdit_alias_name->setFocus();
            return;
        }
        if (treeWidget_aliases->hasFocus()) {
            mpSourceEditorEdbee->setFocus();
            return;
        }
        for (auto child : mpAliasMainArea->findChildren<QWidget*>()) {
            if (child->hasFocus()) {
                treeWidget_aliases->setFocus();
                return;
            }
        }
        break;
    case EditorViewType::cmScriptView:
        if (QString("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            mpScriptsMainArea->lineEdit_script_name->setFocus();
            return;
        }
        if (treeWidget_scripts->hasFocus()) {
            mpSourceEditorEdbee->setFocus();
            return;
        }
        for (auto child : mpScriptsMainArea->findChildren<QWidget*>()) {
            if (child->hasFocus()) {
                treeWidget_scripts->setFocus();
                return;
            }
        }
        break;
    case EditorViewType::cmActionView:
        if (QString("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            mpActionsMainArea->lineEdit_action_name->setFocus();
            return;
        }
        if (treeWidget_actions->hasFocus()) {
            mpSourceEditorEdbee->setFocus();
            return;
        }
        for (auto child : mpActionsMainArea->findChildren<QWidget*>()) {
            if (child->hasFocus()) {
                treeWidget_actions->setFocus();
                return;
            }
        }
        break;
    case EditorViewType::cmKeysView:
        if (QString("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            mpKeysMainArea->lineEdit_key_name->setFocus();
            return;
        }
        if (treeWidget_keys->hasFocus()) {
            mpSourceEditorEdbee->setFocus();
            return;
        }
        for (auto child : mpKeysMainArea->findChildren<QWidget*>()) {
            if (child->hasFocus()) {
                treeWidget_keys->setFocus();
                return;
            }
        }
        break;
    case EditorViewType::cmVarsView:
        if (QString("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            mpVarsMainArea->lineEdit_var_name->setFocus();
            return;
        }
        if (treeWidget_variables->hasFocus()) {
            mpSourceEditorEdbee->setFocus();
            return;
        }
        for (auto child : mpVarsMainArea->findChildren<QWidget*>()) {
            if (child->hasFocus()) {
                treeWidget_variables->setFocus();
                return;
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
        const int triggerID = pItem->data(0, Qt::UserRole).toInt();
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
        statusBar()->showMessage(tr("Package %1 saved").arg(name.toHtmlEscaped()), 2000);
    }
}

void dlgTriggerEditor::exportTimer(const QString& fileName)
{
    QString name;
    TTimer* pT = nullptr;
    QTreeWidgetItem* pItem = treeWidget_timers->currentItem();
    if (pItem) {
        const int triggerID = pItem->data(0, Qt::UserRole).toInt();
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
        statusBar()->showMessage(tr("Package %1 saved").arg(name.toHtmlEscaped()), 2000);
    }
}

void dlgTriggerEditor::exportAlias(const QString& fileName)
{
    QString name;
    TAlias* pT = nullptr;
    QTreeWidgetItem* pItem = treeWidget_aliases->currentItem();
    if (pItem) {
        const int triggerID = pItem->data(0, Qt::UserRole).toInt();
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
        statusBar()->showMessage(tr("Package %1 saved").arg(name.toHtmlEscaped()), 2000);
    }
}

void dlgTriggerEditor::exportAction(const QString& fileName)
{
    QString name;
    TAction* pT = nullptr;
    QTreeWidgetItem* pItem = treeWidget_actions->currentItem();
    if (pItem) {
        const int triggerID = pItem->data(0, Qt::UserRole).toInt();
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
        statusBar()->showMessage(tr("Package %1 saved").arg(name.toHtmlEscaped()), 2000);
    }
}

void dlgTriggerEditor::exportScript(const QString& fileName)
{
    QString name;
    TScript* pT = nullptr;
    QTreeWidgetItem* pItem = treeWidget_scripts->currentItem();
    if (pItem) {
        const int triggerID = pItem->data(0, Qt::UserRole).toInt();
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
        statusBar()->showMessage(tr("Package %1 saved").arg(name.toHtmlEscaped()), 2000);
    }
}

void dlgTriggerEditor::exportKey(const QString& fileName)
{
    QString name;
    TKey* pT = nullptr;
    QTreeWidgetItem* pItem = treeWidget_keys->currentItem();
    if (pItem) {
        const int triggerID = pItem->data(0, Qt::UserRole).toInt();
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
        statusBar()->showMessage(tr("Package %1 saved").arg(name.toHtmlEscaped()), 2000);
    }
}

void dlgTriggerEditor::exportTriggerToClipboard()
{
    QString name;
    TTrigger* pT = nullptr;
    QTreeWidgetItem* pItem = treeWidget_triggers->currentItem();
    if (pItem) {
        const int triggerID = pItem->data(0, Qt::UserRole).toInt();
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
    statusBar()->showMessage(tr("Copied %1 to clipboard").arg(name.toHtmlEscaped()), 2000);
}

void dlgTriggerEditor::exportTimerToClipboard()
{
    QString name;
    TTimer* pT = nullptr;
    QTreeWidgetItem* pItem = treeWidget_timers->currentItem();
    if (pItem) {
        const int triggerID = pItem->data(0, Qt::UserRole).toInt();
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
    statusBar()->showMessage(tr("Copied %1 to clipboard").arg(name.toHtmlEscaped()), 2000);
}

void dlgTriggerEditor::exportAliasToClipboard()
{
    QString name;
    TAlias* pT = nullptr;
    QTreeWidgetItem* pItem = treeWidget_aliases->currentItem();
    if (pItem) {
        const int triggerID = pItem->data(0, Qt::UserRole).toInt();
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
    statusBar()->showMessage(tr("Copied %1 to clipboard").arg(name.toHtmlEscaped()), 2000);
}

void dlgTriggerEditor::exportActionToClipboard()
{
    QString name;
    TAction* pT = nullptr;
    QTreeWidgetItem* pItem = treeWidget_actions->currentItem();
    if (pItem) {
        const int triggerID = pItem->data(0, Qt::UserRole).toInt();
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
    statusBar()->showMessage(tr("Copied %1 to clipboard").arg(name.toHtmlEscaped()), 2000);
}

void dlgTriggerEditor::exportScriptToClipboard()
{
    QString name;
    TScript* pT = nullptr;
    QTreeWidgetItem* pItem = treeWidget_scripts->currentItem();
    if (pItem) {
        const int triggerID = pItem->data(0, Qt::UserRole).toInt();
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
    statusBar()->showMessage(tr("Copied %1 to clipboard").arg(name.toHtmlEscaped()), 2000);
}

void dlgTriggerEditor::exportKeyToClipboard()
{
    QString name;
    TKey* pT = nullptr;
    QTreeWidgetItem* pItem = treeWidget_keys->currentItem();
    if (pItem) {
        const int triggerID = pItem->data(0, Qt::UserRole).toInt();
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
    statusBar()->showMessage(tr("Copied %1 to clipboard").arg(name.toHtmlEscaped()), 2000);
}


void dlgTriggerEditor::slot_export()
{
    if (mCurrentView == EditorViewType::cmUnknownView || mCurrentView == EditorViewType::cmVarsView) {
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Item"), QDir::currentPath(), tr("Mudlet packages (*.xml)"));
    if (fileName.isEmpty()) {
        return;
    }

    // Must be case insensitive to work on MacOS platforms, possibly a cause of
    // https://bugs.launchpad.net/mudlet/+bug/1417234
    if (!fileName.endsWith(qsl(".xml"), Qt::CaseInsensitive)) {
        fileName.append(qsl(".xml"));
    }


    QFile checkWriteability(fileName);
    if (!checkWriteability.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("export package:"), tr("Cannot write file %1:\n%2.").arg(fileName.toHtmlEscaped(), checkWriteability.errorString()));
        return;
    }
    // Should close the checkWriteability that we have confirmed can be opened:
    checkWriteability.close();

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
    case EditorViewType::cmVarsView:
        [[fallthrough]];
    case EditorViewType::cmUnknownView:
        // These two have already been handled so this place in the code should
        // indeed be:
        Q_UNREACHABLE();
    }
}

void dlgTriggerEditor::slot_copyXml()
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
    case EditorViewType::cmVarsView:
        qWarning().nospace().noquote() << "dlgTriggerEditor::slot_copyXml() WARNING - switch(EditorViewType) not expected to be called for \"EditorViewType::cmVarsView!\"";
        break;
    case EditorViewType::cmUnknownView:
        qWarning().nospace().noquote() << "dlgTriggerEditor::slot_copyXml() WARNING - switch(EditorViewType) not expected to be called for \"EditorViewType::cmUnknownView!\"";
        break;
    }
}

// FIXME: The switch cases in here need to handle EditorViewType::cmVarsView but how is not clear
void dlgTriggerEditor::slot_pasteXml()
{
    XMLimport reader(mpHost);

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
        qWarning().nospace().noquote() << "dlgTriggerEditor::slot_pasteXml() WARNING - switch(EditorViewType) number 1 not expected to be called for \"EditorViewType::cmVarsView!\"";
        break;
    case EditorViewType::cmUnknownView:
        qWarning().nospace().noquote() << "dlgTriggerEditor::slot_pasteXml() WARNING - switch(EditorViewType) number 1 not expected to be called for \"EditorViewType::cmUnknownView!\"";
        break;
    }

    auto [importedItemType, importedItemID] = reader.importFromClipboard();

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

        const int siblingRow = treeWidget_triggers->currentIndex().row() + 1;
        mpHost->getTriggerUnit()->reParentTrigger(importedItemID, 0, parentId, parentRow, siblingRow);
        break;
    }
    case EditorViewType::cmTimerView: {
        auto parent = treeWidget_timers->currentIndex().parent();
        auto parentRow = parent.row();
        auto parentId = parent.data(Qt::UserRole).toInt();

        const int siblingRow = treeWidget_timers->currentIndex().row() + 1;
        mpHost->getTimerUnit()->reParentTimer(importedItemID, 0, parentId, parentRow, siblingRow);
        break;
    }
    case EditorViewType::cmAliasView: {
        auto parent = treeWidget_aliases->currentIndex().parent();
        auto parentRow = parent.row();
        auto parentId = parent.data(Qt::UserRole).toInt();

        const int siblingRow = treeWidget_aliases->currentIndex().row() + 1;
        mpHost->getAliasUnit()->reParentAlias(importedItemID, 0, parentId, parentRow, siblingRow);
        break;
    }
    case EditorViewType::cmScriptView: {
        auto parent = treeWidget_scripts->currentIndex().parent();
        auto parentRow = parent.row();
        auto parentId = parent.data(Qt::UserRole).toInt();

        const int siblingRow = treeWidget_scripts->currentIndex().row() + 1;
        mpHost->getScriptUnit()->reParentScript(importedItemID, 0, parentId, parentRow, siblingRow);
        break;
    }
    case EditorViewType::cmActionView: {
        auto parent = treeWidget_actions->currentIndex().parent();
        auto parentRow = parent.row();
        auto parentId = parent.data(Qt::UserRole).toInt();

        const int siblingRow = treeWidget_actions->currentIndex().row() + 1;
        mpHost->getActionUnit()->reParentAction(importedItemID, 0, parentId, parentRow, siblingRow);
        break;
    }
    case EditorViewType::cmKeysView: {
        auto parent = treeWidget_keys->currentIndex().parent();
        auto parentRow = parent.row();
        auto parentId = parent.data(Qt::UserRole).toInt();

        const int siblingRow = treeWidget_keys->currentIndex().row() + 1;
        mpHost->getKeyUnit()->reParentKey(importedItemID, 0, parentId, parentRow, siblingRow);
        break;
    }
    case EditorViewType::cmVarsView:
        qWarning().nospace().noquote() << "dlgTriggerEditor::slot_pasteXml() WARNING - switch(EditorViewType) number 2 not expected to be called for \"EditorViewType::cmVarsView!\"";
        break;
    case EditorViewType::cmUnknownView:
        qWarning().nospace().noquote() << "dlgTriggerEditor::slot_pasteXml() WARNING - switch(EditorViewType) number 2 not expected to be called for \"EditorViewType::cmUnknownView!\"";
        break;
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
    case EditorViewType::cmVarsView:
        qWarning().nospace().noquote() << "dlgTriggerEditor::slot_pasteXml() WARNING - switch(EditorViewType) number 3 not expected to be called for \"EditorViewType::cmVarsView!\"";
        break;
    case EditorViewType::cmUnknownView:
        qWarning().nospace().noquote() << "dlgTriggerEditor::slot_pasteXml() WARNING - switch(EditorViewType) number 3 not expected to be called for \"EditorViewType::cmUnknownView!\"";
        break;
    }
}

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
    case EditorViewType::cmVarsView:
        qWarning().nospace().noquote() << "dlgTriggerEditor::slot_import() WARNING - switch(EditorViewType) not expected to be called for \"EditorViewType::cmVarsView!\"";
        break;
    case EditorViewType::cmUnknownView:
        qWarning().nospace().noquote() << "dlgTriggerEditor::slot_import() WARNING - switch(EditorViewType) not expected to be called for \"EditorViewType::cmUnknownView!\"";
    }

    const QString fileName = QFileDialog::getOpenFileName(this, tr("Import Mudlet Package"), QDir::currentPath());
    if (fileName.isEmpty()) {
        return;
    }

    mpHost->installPackage(fileName, 0);

    treeWidget_triggers->clear();
    treeWidget_aliases->clear();
    treeWidget_actions->clear();
    treeWidget_timers->clear();
    treeWidget_keys->clear();
    treeWidget_scripts->clear();

    slot_profileSaveAction();

    fillout_form();

    mpCurrentTriggerItem = nullptr;
    mpCurrentTimerItem = nullptr;
    mpCurrentAliasItem = nullptr;
    mpCurrentScriptItem = nullptr;
    mpCurrentActionItem = nullptr;
    mpCurrentKeyItem = nullptr;

    slot_showTriggers();
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
    case EditorViewType::cmVarsView:
        // FIXME: The switch in here need to handle (or at least treat correctly) the
        // EditorViewType:cmVarsView case but how is not clear:
        qWarning().nospace().noquote() << "dlgTriggerEditor::runScheduledCleanReset() WARNING - switch(EditorViewType) not expected to be called for \"EditorViewType::cmVarsView!\"";
        break;
    case EditorViewType::cmUnknownView:
        qWarning().nospace().noquote() << "dlgTriggerEditor::runScheduledCleanReset() WARNING - switch(EditorViewType) not expected to be called for \"EditorViewType::cmUnknownView!\"";
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
    slot_showTriggers();
}

void dlgTriggerEditor::slot_profileSaveAction()
{
    auto [ok, filename, error] = mpHost->saveProfile(nullptr, nullptr, true);

    if (!ok) {
        QMessageBox::critical(this, tr("Couldn't save profile"), tr("Sorry, couldn't save your profile - got the following error: %1").arg(error));
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
    if (!fileName.endsWith(qsl(".xml"), Qt::CaseInsensitive) && !fileName.endsWith(qsl(".trigger"), Qt::CaseInsensitive)) {
        fileName.append(qsl(".xml"));
    }

    mpHost->saveProfileAs(fileName);
    mSavingAs = false;
}

bool dlgTriggerEditor::eventFilter(QObject*, QEvent* event)
{
    if (event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        switch (keyEvent->key()) {
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
            auto* ke = static_cast<QKeyEvent*>(event);
            switch (ke->key()) {
            case Qt::Key_Escape:
                mIsGrabKey = false;
                setShortcuts();
                QCoreApplication::instance()->removeEventFilter(this);
                ke->accept();
                return true;

            case Qt::Key_Shift:
                [[fallthrough]];
            case Qt::Key_Control:
                [[fallthrough]];
            case Qt::Key_Meta:
                [[fallthrough]];
            case Qt::Key_Alt:
                [[fallthrough]];
            case Qt::Key_AltGr:
                break;

            default:
                keyGrabCallback(static_cast<Qt::Key>(ke->key()), static_cast<Qt::KeyboardModifiers>(ke->modifiers()));
                mIsGrabKey = false;
                setShortcuts();
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
    Q_UNUSED(event)
    if (mpSourceEditorArea->isVisible()) {
        slot_sourceFindMove();
    }
}

void dlgTriggerEditor::slot_keyGrab()
{
    mIsGrabKey = true;
    setShortcuts(false);
    QCoreApplication::instance()->installEventFilter(this);
}

// Activate shortcuts for editor menu items like Ctrl+S for "Save Item" etc.
// Deactivate instead with optional "false" - to allow these for keybindings
void dlgTriggerEditor::setShortcuts(const bool active)
{
    setShortcuts(toolBar->actions(), active);
    setShortcuts(toolBar2->actions(), active);
}

void dlgTriggerEditor::setShortcuts(QList<QAction*> actionList, const bool active)
{
    QString buttonLabel;
    for (auto& action : actionList) {
        if (!active) {
            action->setShortcut(QString());
            continue;
        }
        buttonLabel = action->text();
        if (auto it = mButtonShortcuts.find(buttonLabel); it != mButtonShortcuts.end()) {
            action->setShortcut(it->second);
        }
    }
}

void dlgTriggerEditor::keyGrabCallback(const Qt::Key key, const Qt::KeyboardModifiers modifier)
{
    KeyUnit* pKeyUnit = mpHost->getKeyUnit();
    if (!pKeyUnit) {
        return;
    }
    const QString keyName = pKeyUnit->getKeyName(key, modifier);
    const QString name = keyName;
    mpKeysMainArea->lineEdit_key_binding->setText(name);
    QTreeWidgetItem* pItem = treeWidget_keys->currentItem();
    if (pItem) {
        const int triggerID = pItem->data(0, Qt::UserRole).toInt();
        TKey* pT = mpHost->getKeyUnit()->getKey(triggerID);
        if (pT) {
            pT->setKeyCode(key);
            pT->setKeyModifiers(modifier);
        }
    }
}

void dlgTriggerEditor::slot_toggleIsPushDownButton(const int state)
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

    auto color = QColorDialog::getColor(QColor(mpTriggersMainArea->pushButtonFgColor->property(cButtonBaseColor).toString()), this, tr("Select foreground color to apply to matches"));
    color = color.isValid() ? color : QColorConstants::Transparent;
    const bool keepColor = color == QColorConstants::Transparent;
    mpTriggersMainArea->pushButtonFgColor->setStyleSheet(generateButtonStyleSheet(color));
    TriggerColorizerFgColorEditedCommand* command = new TriggerColorizerFgColorEditedCommand(mpTriggersMainArea);
    command->mpEditor = this;
    command->mpTreeWidget_triggers = treeWidget_triggers;
    command->mpItem = treeWidget_triggers->currentItem();
    command->mPrevfgColor = QColor(mpTriggersMainArea->pushButtonFgColor->property(cButtonBaseColor).toString()).name();
    command->mFgColor = color.name();
    undoStack->push(command);
    //: Keep the existing colour on matches to highlight. Use shortest word possible so it fits on the button
    mpTriggersMainArea->pushButtonFgColor->setText(keepColor ? tr("keep") : QString());
    mpTriggersMainArea->pushButtonFgColor->setProperty(cButtonBaseColor, keepColor ? qsl("transparent") : color.name());
    saveTrigger();
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

    auto color = QColorDialog::getColor(QColor(mpTriggersMainArea->pushButtonBgColor->property(cButtonBaseColor).toString()), this, tr("Select background color to apply to matches"));
    color = color.isValid() ? color : QColorConstants::Transparent;
    const bool keepColor = color == QColorConstants::Transparent;
    mpTriggersMainArea->pushButtonBgColor->setStyleSheet(generateButtonStyleSheet(color));
    TriggerColorizerBgColorEditedCommand* command = new TriggerColorizerBgColorEditedCommand(mpTriggersMainArea);
    command->mpEditor = this;
    command->mpTreeWidget_triggers = treeWidget_triggers;
    command->mpItem = treeWidget_triggers->currentItem();
    command->mPrevbgColor = QColor(mpTriggersMainArea->pushButtonBgColor->property(cButtonBaseColor).toString()).name();
    command->mBgColor = color.name();
    undoStack->push(command);
    //: Keep the existing colour on matches to highlight. Use shortest word possible so it fits on the button
    mpTriggersMainArea->pushButtonBgColor->setText(keepColor ? tr("keep") : QString());
    mpTriggersMainArea->pushButtonBgColor->setProperty(cButtonBaseColor, keepColor ? qsl("transparent") : color.name());
    saveTrigger();
}

void dlgTriggerEditor::slot_soundTrigger()
{
    // Use the existing path/filename if it is not empty, otherwise start in
    // profile home directory:
    const QString fileName = QFileDialog::getOpenFileName(
            this,
            tr("Choose sound file"),
            mpTriggersMainArea->lineEdit_soundFile->text().isEmpty() ? mudlet::getMudletPath(mudlet::profileHomePath, mpHost->getName()) : mpTriggersMainArea->lineEdit_soundFile->text(),
            //: This the list of file extensions that are considered for sounds from triggers, the terms inside of the '('...')' and the ";;" are used programmatically and should not be changed.
            tr("Audio files(*.aac *.mp3 *.mp4a *.oga *.ogg *.pcm *.wav *.wma);;"
               "Advanced Audio Coding-stream(*.aac);;"
               "MPEG-2 Audio Layer 3(*.mp3);;"
               "MPEG-4 Audio(*.mp4a);;"
               "Ogg Vorbis(*.oga *.ogg);;"
               "PCM Audio(*.pcm);;"
               "Wave(*.wav);;"
               "Windows Media Audio(*.wma);;"
               "All files(*.*)"));
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
void dlgTriggerEditor::slot_colorTriggerFg(int i)
{
    QTreeWidgetItem* pItem = mpCurrentTriggerItem;
    if (!pItem) {
        return;
    }
    const int triggerID = pItem->data(0, Qt::UserRole).toInt();
    TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(triggerID);
    if (!pT) {
        return;
    }

    auto* pB = qobject_cast<QPushButton*>(sender());
    if (!pB) {
        return;
    }

    dlgTriggerPatternEdit* pPatternItem = qobject_cast<dlgTriggerPatternEdit*>(pB->parent());
    if (!pPatternItem) {
        return;
    }

    if (pT->mColorTriggerFgColor.isValid()) {
        mPrevColorTriggerFgColor = pT->mColorTriggerFgColor;
    } else {
        mPrevColorTriggerFgColor = QColor();
    }
    // This method parses the pattern text and extracts the ansi color values
    // from it - including the special values of DEFAULT (-2) and IGNORE (-1)
    // and assigns the values to the other arguments:
    TTrigger::decodeColorPatternText(pPatternItem->lineEdit_pattern->text(), pT->mColorTriggerFgAnsi, pT->mColorTriggerBgAnsi);

    // The following method wants to know BOTH existing fore and backgrounds
    // it will select the appropriate as a result of the third argument and it
    // uses both to determine whether the result to return is valid considering
    // the other, non used (background in this method) part:
    auto pD = new dlgColorTrigger(this, pT, false, tr("Select foreground trigger color for item %1").arg(QString::number(pPatternItem->mRow + 1)));
    pD->setModal(true);
    // This sounds a bit iffy - prevent access to other application windows
    // while we get a colour setting:
    pD->setWindowModality(Qt::ApplicationModal);
    pD->exec();

    const QColor color = pT->mColorTriggerFgColor;
    // The above will be an invalid colour if the colour has been reset/ignored
    // The dialogue should have changed pT->mColorTriggerFgAnsi
    QString styleSheet;
    if (color.isValid()) {
        styleSheet = generateButtonStyleSheet(color);
    }
    pB->setStyleSheet(styleSheet);

    pPatternItem->lineEdit_pattern->setText(TTrigger::createColorPatternText(pT->mColorTriggerFgAnsi, pT->mColorTriggerBgAnsi));

    if (pT->mColorTriggerFgAnsi == TTrigger::scmIgnored) {
        //: Color trigger ignored foreground color button, ensure all three instances have the same text
        pB->setText(tr("Foreground color ignored"));
    } else if (pT->mColorTriggerFgAnsi == TTrigger::scmDefault) {
        //: Color trigger default foreground color button, ensure all three instances have the same text
        pB->setText(tr("Default foreground color"));
    } else {
        //: Color trigger ANSI foreground color button, ensure all three instances have the same text
        pB->setText(tr("Foreground color [ANSI %1]").arg(QString::number(pT->mColorTriggerFgAnsi)));
    }
    if(pT->mColorTriggerFgAnsi == TTrigger::scmIgnored)
    {
        return;
    }

    TriggerColorFGEditedCommand* command = new TriggerColorFGEditedCommand(mpTriggersMainArea);
    command->mpEditor = this;
    command->mpItem = pItem;
    command->mpTreeWidget_triggers = treeWidget_triggers;
    command->mpTriggerUnit = mpHost->getTriggerUnit();
    command->mpPushButton = pB;
    command->mpTriggerPatternEdit = mTriggerPatternEdit;
    command->mRow = i;
    command->mpPatternItem = pPatternItem;
    command->mPrevColorTriggerFgColor = mPrevColorTriggerFgColor;
    command->mColorTriggerFgColor = color;
    undoStack->push(command);
    saveTrigger();
}

// Get the color from the user to use as that to look for as the background in
// a color trigger:
void dlgTriggerEditor::slot_colorTriggerBg(int i)
{
    QTreeWidgetItem* pItem = mpCurrentTriggerItem;
    if (!pItem) {
        return;
    }
    const int triggerID = pItem->data(0, Qt::UserRole).toInt();
    TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(triggerID);
    if (!pT) {
        return;
    }

    auto* pB = qobject_cast<QPushButton*>(sender());
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
    auto pD = new dlgColorTrigger(this, pT, true, tr("Select background trigger color for item %1").arg(QString::number(pPatternItem->mRow + 1)));
    pD->setModal(true);
    // This sounds a bit iffy - prevent access to other application windows
    // while we get a colour setting:
    pD->setWindowModality(Qt::ApplicationModal);
    pD->exec();

    const QColor color = pT->mColorTriggerBgColor;
    // The above will be an invalid colour if the colour has been reset/ignored
    QString styleSheet;
    if (color.isValid()) {
        styleSheet = generateButtonStyleSheet(color);
    }
    pB->setStyleSheet(styleSheet);

    pPatternItem->lineEdit_pattern->setText(TTrigger::createColorPatternText(pT->mColorTriggerFgAnsi, pT->mColorTriggerBgAnsi));

    if (pT->mColorTriggerBgAnsi == TTrigger::scmIgnored) {
        //: Color trigger ignored background color button, ensure all three instances have the same text
        pB->setText(tr("Background color ignored"));
    } else if (pT->mColorTriggerBgAnsi == TTrigger::scmDefault) {
        //: Color trigger default background color button, ensure all three instances have the same text
        pB->setText(tr("Default background color"));
    } else {
        //: Color trigger ANSI background color button, ensure all three instances have the same text
        pB->setText(tr("Background color [ANSI %1]").arg(QString::number(pT->mColorTriggerBgAnsi)));
    }
    if (pT->mColorTriggerBgAnsi == TTrigger::scmIgnored) {
        return;
    }
    if(i >= pT->mColorPatternList.size())
    {
        mPrevColorTriggerBgColor = QColor();
    }
    else
    {
        mPrevColorTriggerBgColor = pT->mColorPatternList.at(i)->mBgColor;
    }
    TriggerColorBGEditedCommand* command = new TriggerColorBGEditedCommand(mpTriggersMainArea);
    command->mpEditor = this;
    command->mpItem = pItem;
    command->mpTreeWidget_triggers = treeWidget_triggers;
    command->mpTriggerUnit = mpHost->getTriggerUnit();
    command->mpPushButton = pB;
    command->mpPatternItem = pPatternItem;
    command->mPrevColorTriggerBgColor = mPrevColorTriggerBgColor;
    command->mColorTriggerBgColor = color;
    undoStack->push(command);
    saveTrigger();
}

void dlgTriggerEditor::slot_updateStatusBar(const QString& statusText)
{
    // edbee adds the scope and last command which is rather technical debugging information,
    // so strip it away by removing the first pipe and everything after it
    const QRegularExpressionMatch match = csmSimplifyStatusBarRegex.match(statusText, 0, QRegularExpression::PartialPreferFirstMatch);
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
    config->setShowWhitespaceMode((state & QTextOption::ShowTabsAndSpaces) ? edbee::TextEditorConfig::ShowWhitespaces : edbee::TextEditorConfig::HideWhitespaces);
    config->setUseLineSeparator(state & QTextOption::ShowLineAndParagraphSeparators);
    config->endChanges();
}

// clearDocument( edbee::TextEditorWidget* pEditorWidget)
//
// A temporary measure for dealing with the undo spanning over multiple documents bug,
// in place until we create a proper multi-document solution. This gets called whenever
// the editor needs to be "cleared", usually when a different alias/trigger/etc is
// made or selected.
void dlgTriggerEditor::clearDocument(edbee::TextEditorWidget* pEditorWidget, const QString& initialText)
{
    mpSourceEditorFindArea->hide();
    mpSourceEditorEdbeeDocument = new edbee::CharTextDocument();
    // Buck.lua is a fake filename for edbee to figure out its lexer type with. Referencing the
    // lexer directly by name previously gave problems.
    mpSourceEditorEdbeeDocument->setLanguageGrammar(edbee::Edbee::instance()->grammarManager()->detectGrammarWithFilename(QLatin1String("Buck.lua")));
    pEditorWidget->controller()->giveTextDocument(mpSourceEditorEdbeeDocument);

    auto config = mpSourceEditorEdbee->config();
    config->beginChanges();
    config->setThemeName(mpHost->mEditorTheme);
    config->setFont(mpHost->getDisplayFont());
    config->setShowWhitespaceMode((mudlet::self()->mEditorTextOptions & QTextOption::ShowTabsAndSpaces) ? edbee::TextEditorConfig::ShowWhitespaces : edbee::TextEditorConfig::HideWhitespaces);
    config->setUseLineSeparator(mudlet::self()->mEditorTextOptions & QTextOption::ShowLineAndParagraphSeparators);
    config->setSmartTab(true);
    config->setUseTabChar(false); // when you press Enter for a newline, pad with spaces and not tabs
    config->setCaretBlinkRate(200);
    config->setIndentSize(2);
    config->setCaretWidth(1);
    config->setAutocompleteAutoShow(mpHost->mEditorAutoComplete);
    config->setRenderBidiContolCharacters(mpHost->getEditorShowBidi());
    config->setAutocompleteMinimalCharacters(3);
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
    localConfig->setShowWhitespaceMode((mudlet::self()->mEditorTextOptions & QTextOption::ShowTabsAndSpaces) ? edbee::TextEditorConfig::ShowWhitespaces : edbee::TextEditorConfig::HideWhitespaces);
    localConfig->setUseLineSeparator(mudlet::self()->mEditorTextOptions & QTextOption::ShowLineAndParagraphSeparators);
    localConfig->setAutocompleteAutoShow(mpHost->mEditorAutoComplete);
    localConfig->setRenderBidiContolCharacters(mpHost->getEditorShowBidi());
    localConfig->setAutocompleteMinimalCharacters(3);
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
    case SearchOptionCaseSensitive | SearchOptionIncludeVariables:
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
        menu->addAction(controller->createAction("cut", tr("Cut"), QIcon::fromTheme(qsl("edit-cut"), QIcon(qsl(":/icons/edit-cut.png"))), menu));
        menu->addAction(controller->createAction("copy", tr("Copy"), QIcon::fromTheme(qsl("edit-copy"), QIcon(qsl(":/icons/edit-copy.png"))), menu));
        menu->addAction(controller->createAction("paste", tr("Paste"), QIcon::fromTheme(qsl("edit-paste"), QIcon(qsl(":/icons/edit-paste.png"))), menu));
        menu->addSeparator();
        menu->addAction(controller->createAction("sel_all", tr("Select All"), QIcon::fromTheme(qsl("edit-select-all"), QIcon(qsl(":/icons/edit-select-all.png"))), menu));
        formatAction->setIcon(QIcon::fromTheme(qsl("run-build-clean"), QIcon::fromTheme(qsl("run-build-clean"))));
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
    if (color != QColorConstants::Transparent && color.isValid()) {
        if (isEnabled) {
            return mudlet::self()->mTEXT_ON_BG_STYLESHEET.arg(color.lightness() > 127 ? QLatin1String("black") : QLatin1String("white"), color.name());
        }

        const QColor disabledColor = QColor::fromHsl(color.hslHue(), color.hslSaturation() / 4, color.lightness());
        return mudlet::self()->mTEXT_ON_BG_STYLESHEET.arg(QLatin1String("darkGray"), disabledColor.name());
    } else {
        return QString();
    }
}

// Retrieve the background-color or color setting from the previous method, the
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
                qDebug().noquote().nospace() << "dlgTriggerEditor::parseButtonStyleSheetColors(\"" << styleSheetText << "\", " << isToGetForeground
                                             << ") ERROR - Invalid hex string as foreground color!";
                return QColor();
            }
        } else {
            namedColorRegex.setPattern(QLatin1String("(?:[{ ])color:\\s*(\\w{3,})\\s*;")); // Capture group 1 is a word for a foreground color
            match = namedColorRegex.match(styleSheetText);
            if (match.hasMatch()) {
#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
                if (QColor::isValidColor(match.captured(1))) {
#else
                if (QColor::isValidColorName(match.captured(1))) {
#endif
                    return QColor(match.captured(1));
                } else {
                    qDebug().noquote().nospace() << "dlgTriggerEditor::parseButtonStyleSheetColors(\"" << styleSheetText << "\", " << isToGetForeground << ") ERROR - Invalid string \""
                                                 << match.captured(1) << "\" found as name of foreground color!";
                    return QColor();
                }
            } else {
                qDebug().noquote().nospace() << "dlgTriggerEditor::parseButtonStyleSheetColors(\"" << styleSheetText << "\", " << isToGetForeground
                                             << ") ERROR - No string as name of foreground color found!";
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
                qDebug().noquote().nospace() << "dlgTriggerEditor::parseButtonStyleSheetColors(\"" << styleSheetText << "\", " << isToGetForeground
                                             << ") ERROR - Invalid hex string as background color!";
                return QColor();
            }
        } else {
            namedColorRegex.setPattern(QLatin1String("(?:[{ ])background-color:\\s*(\\w{3,})\\s*;")); // Capture group 1 is a word for a background color
            match = namedColorRegex.match(styleSheetText);
            if (match.hasMatch()) {
#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
                if (QColor::isValidColor(match.captured(1))) {
#else
                if (QColor::isValidColorName(match.captured(1))) {
#endif
                    return QColor(match.captured(1));
                } else {
                    qDebug().noquote().nospace() << "dlgTriggerEditor::parseButtonStyleSheetColors(\"" << styleSheetText << "\", " << isToGetForeground << ") ERROR - Invalid string \""
                                                 << match.captured(1) << "\" found as name of background color!";
                    return QColor();
                }
            } else {
                qDebug().noquote().nospace() << "dlgTriggerEditor::parseButtonStyleSheetColors(\"" << styleSheetText << "\", " << isToGetForeground
                                             << ") ERROR - No string as name of background color found!";
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
        const QString fgColor = mpTriggersMainArea->pushButtonFgColor->property(cButtonBaseColor).toString();
        const QString bgColor = mpTriggersMainArea->pushButtonBgColor->property(cButtonBaseColor).toString();
        mpTriggersMainArea->pushButtonFgColor->setStyleSheet(generateButtonStyleSheet(fgColor, true));
        mpTriggersMainArea->pushButtonBgColor->setStyleSheet(generateButtonStyleSheet(bgColor, true));
        //: Keep the existing colour on matches to highlight. Use shortest word possible so it fits on the button
        mpTriggersMainArea->pushButtonFgColor->setText(fgColor == QLatin1String("transparent") ? tr("keep") : QString());
        //: Keep the existing colour on matches to highlight. Use shortest word possible so it fits on the button
        mpTriggersMainArea->pushButtonBgColor->setText(bgColor == QLatin1String("transparent") ? tr("keep") : QString());
    } else {
        // Disabled so make buttons greyed out a bit:
        mpTriggersMainArea->pushButtonFgColor->setStyleSheet(generateButtonStyleSheet(mpTriggersMainArea->pushButtonFgColor->property(cButtonBaseColor).toString(), false));
        mpTriggersMainArea->pushButtonBgColor->setStyleSheet(generateButtonStyleSheet(mpTriggersMainArea->pushButtonBgColor->property(cButtonBaseColor).toString(), false));
        mpTriggersMainArea->pushButtonFgColor->setText(QString());
        mpTriggersMainArea->pushButtonBgColor->setText(QString());
    }
}

void dlgTriggerEditor::slot_clearSoundFile()
{
    mpTriggersMainArea->lineEdit_soundFile->clear();
    mpTriggersMainArea->toolButton_clearSoundFile->setEnabled(false);
    mpTriggersMainArea->lineEdit_soundFile->setToolTip(utils::richText(tr("Sound file to play when the trigger fires.")));
}

void dlgTriggerEditor::slot_lineEditTriggerNameTextEdited()
{
    TriggerNameTextEditedCommand* command = new TriggerNameTextEditedCommand(mpTriggersMainArea);
    command->mpEditor = this;
    command->mpTreeWidget_triggers = treeWidget_triggers;
    command->mpItem = treeWidget_triggers->currentItem();
    command->mPrevLineEdit_trigger_name = mPrevTriggerName;
    command->mLineEdit_trigger_name = mpTriggersMainArea->lineEdit_trigger_name->text();
    undoStack->push(command);
    saveTrigger();
}

void dlgTriggerEditor::slot_lineEditTriggerCommandTextEdited()
{
    TriggerCommandTextEditedCommand* command = new TriggerCommandTextEditedCommand(mpTriggersMainArea);
    command->mpEditor = this;
    command->mpTreeWidget_triggers = treeWidget_triggers;
    command->mpItem = treeWidget_triggers->currentItem();
    command->mPrevLineEdit_trigger_command = mPrevTriggerCommand;
    command->mLineEdit_trigger_command = mpTriggersMainArea->lineEdit_trigger_command->text();
    undoStack->push(command);
    saveTrigger();
}

void dlgTriggerEditor::slot_triggerFireLengthEdited(int i)
{
    TriggerFireLengthEditedCommand* command = new TriggerFireLengthEditedCommand(mpTriggersMainArea);
    command->mpEditor = this;
    command->mpTreeWidget_triggers = treeWidget_triggers;
    command->mpItem = treeWidget_triggers->currentItem();
    command->mPrevFireLength = mPrevFireLength;
    command->mFireLength = mpTriggersMainArea->spinBox_stayOpen->value();
    undoStack->push(command);
    saveTrigger();
}

void dlgTriggerEditor::slot_triggerPlaySoundEdited(bool on)
{
    TriggerPlaySoundEditedCommand* command = new TriggerPlaySoundEditedCommand(mpTriggersMainArea);
    command->mpEditor = this;
    command->mpTreeWidget_triggers = treeWidget_triggers;
    command->mpItem = treeWidget_triggers->currentItem();
    command->mPrevGroupBox_soundTrigger = mPrevGroupBox_soundTrigger;
    command->mGroupBox_soundTrigger = mpTriggersMainArea->groupBox_soundTrigger->isChecked();
    undoStack->push(command);
    saveTrigger();
}

void dlgTriggerEditor::slot_triggerPlaySoundFileEdited(const QString &text)
{
    TriggerPlaySoundFileEditedCommand* command = new TriggerPlaySoundFileEditedCommand(mpTriggersMainArea);
    command->mpEditor = this;
    command->mpTreeWidget_triggers = treeWidget_triggers;
    command->mpItem = treeWidget_triggers->currentItem();
    command->mPrevLineEdit_soundFile = mPrevLineEdit_soundFile;
    command->mLineEdit_soundFile = mpTriggersMainArea->lineEdit_soundFile->text();
    undoStack->push(command);
    saveTrigger();
}

void dlgTriggerEditor::slot_triggerColorizerEdited(bool on)
{
    TriggerColorizerEditedCommand* command = new TriggerColorizerEditedCommand(mpTriggersMainArea);
    command->mpEditor = this;
    command->mpTreeWidget_triggers = treeWidget_triggers;
    command->mpItem = treeWidget_triggers->currentItem();
    command->mPrevBox_triggerColorizer = mPrevBox_triggerColorizer;
    command->mBox_triggerColorizer = mpTriggersMainArea->groupBox_triggerColorizer->isChecked();
    undoStack->push(command);
    saveTrigger();
}

void dlgTriggerEditor::slot_triggerPerlSlashGOptionEdited(bool on)
{
    TriggerPerlSlashGOptionEditedCommand* command = new TriggerPerlSlashGOptionEditedCommand(mpTriggersMainArea);
    command->mpEditor = this;
    command->mpTreeWidget_triggers = treeWidget_triggers;
    command->mpItem = treeWidget_triggers->currentItem();
    command->mPrevPerlSlashGOption = mPrevPerlSlashGOption;
    command->mPerlSlashGOption = mpTriggersMainArea->groupBox_perlSlashGOption->isChecked();
    undoStack->push(command);
    saveTrigger();
}

void dlgTriggerEditor::slot_triggerGroupFilterEdited(bool on)
{
    TriggerGroupFilterEditedCommand* command = new TriggerGroupFilterEditedCommand(mpTriggersMainArea);
    command->mpEditor = this;
    command->mpTreeWidget_triggers = treeWidget_triggers;
    command->mpItem = treeWidget_triggers->currentItem();
    command->mPrevFilterTrigger = mPrevFilterTrigger;
    command->mFilterTrigger = mpTriggersMainArea->groupBox_filterTrigger->isChecked();
    undoStack->push(command);
    saveTrigger();
}

void dlgTriggerEditor::slot_triggerMultiLineEdited(bool on)
{
    TriggerMultiLineEditedCommand* command = new TriggerMultiLineEditedCommand(mpTriggersMainArea);
    command->mpEditor = this;
    command->mpTreeWidget_triggers = treeWidget_triggers;
    command->mpItem = treeWidget_triggers->currentItem();
    command->mPrevMultiLineTrigger = mPrevMultiLineTrigger;
    command->mMultiLineTrigger = mpTriggersMainArea->groupBox_multiLineTrigger->isChecked();
    undoStack->push(command);
    saveTrigger();
}

void dlgTriggerEditor::slot_triggerLineMarginEdited(int i)
{
    TriggerLineMarginEditedCommand* command = new TriggerLineMarginEditedCommand(mpTriggersMainArea);
    command->mpEditor = this;
    command->mpTreeWidget_triggers = treeWidget_triggers;
    command->mpItem = treeWidget_triggers->currentItem();
    command->mPrevLineMargin = mPrevLineMargin;
    command->mLineMargin = mpTriggersMainArea->spinBox_lineMargin->value();
    undoStack->push(command);
    saveTrigger();
}

void dlgTriggerEditor::slot_triggerLineSpacerEdited(int i)
{
    TriggerLineSpacerEditedCommand* command = new TriggerLineSpacerEditedCommand(mpTriggersMainArea);
    command->mpEditor = this;
    command->mpPatternItem = mTriggerPatternEdit[i];
    command->mpTreeWidget_triggers = treeWidget_triggers;
    command->mpItem = treeWidget_triggers->currentItem();
    command->mPrevLineSpacer = mPrevLineSpacer[i];
    command->mLineSpacer = mTriggerPatternEdit[i]->spinBox_lineSpacer->value();
    undoStack->push(command);
    saveTrigger();
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
        slot_sourceFindMove();
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

void dlgTriggerEditor::showOrHideRestoreEditorActionsToolbarAction()
{
    if ((!toolBar->isVisible()) || toolBar->isFloating()
        || (QMainWindow::toolBarArea(toolBar) & (Qt::ToolBarArea::LeftToolBarArea | Qt::ToolBarArea::RightToolBarArea | Qt::ToolBarArea::BottomToolBarArea))) {
        // If it is NOT visible
        // OR If the toolbar is floating
        // OR it is docked in an area other than the top one
        // then show the restore action
        mpAction_restoreEditorActionsToolbar->setVisible(true);
    } else {
        // Otherwise - i.e. it is visible AND docked AND docked to the original
        // area:
        mpAction_restoreEditorActionsToolbar->setVisible(false);
    }
}

void dlgTriggerEditor::showOrHideRestoreEditorItemsToolbarAction()
{
    if ((!toolBar2->isVisible()) || toolBar2->isFloating()
        || (QMainWindow::toolBarArea(toolBar2) & (Qt::ToolBarArea::TopToolBarArea | Qt::ToolBarArea::RightToolBarArea | Qt::ToolBarArea::BottomToolBarArea))) {
        mpAction_restoreEditorItemsToolbar->setVisible(true);
    } else {
        mpAction_restoreEditorItemsToolbar->setVisible(false);
    }
}

// These two slots show/hide the restore option for the relevant toolbar
// as the toolbar itself is hidden/shown:
void dlgTriggerEditor::slot_visibilityChangedEditorActionsToolbar()
{
    showOrHideRestoreEditorActionsToolbarAction();
}

void dlgTriggerEditor::slot_visibilityChangedEditorItemsToolbar()
{
    showOrHideRestoreEditorItemsToolbarAction();
}

// These two get triggered twice during the dragging of a toolbar from one
// docking area to another - as it briefly floats during the drag:
void dlgTriggerEditor::slot_floatingChangedEditorActionsToolbar()
{
    showOrHideRestoreEditorActionsToolbarAction();
}

void dlgTriggerEditor::slot_floatingChangedEditorItemsToolbar()
{
    showOrHideRestoreEditorItemsToolbarAction();
}

// These two also triggers the corresponding signal that is connected to:
// the showOrHideRestoreEditorXxxxxToolbarAction() SLOT:
void dlgTriggerEditor::slot_restoreEditorActionsToolbar()
{
    if (!toolBar->isVisible()) {
        // Reshow it
        toolBar->show();
    }
    // Forces it to redock in the starting area:
    QMainWindow::addToolBar(Qt::TopToolBarArea, toolBar);
}

void dlgTriggerEditor::slot_restoreEditorItemsToolbar()
{
    if (!toolBar2->isVisible()) {
        toolBar2->show();
    }
    QMainWindow::addToolBar(Qt::LeftToolBarArea, toolBar2);
}

void dlgTriggerEditor::clearTriggerForm()
{
    mpTriggersMainArea->hide();
    mpSourceEditorArea->hide();
    showInfo(msgInfoAddTrigger);
}

void dlgTriggerEditor::clearTimerForm()
{
    mpTimersMainArea->hide();
    mpTimersMainArea->hide();
    showInfo(msgInfoAddTimer);
}

void dlgTriggerEditor::clearAliasForm()
{
    mpAliasMainArea->hide();
    mpSourceEditorArea->hide();
    showInfo(msgInfoAddAlias);
}

void dlgTriggerEditor::clearScriptForm()
{
    mpScriptsMainArea->hide();
    mpSourceEditorArea->hide();
    showInfo(msgInfoAddScript);
}

void dlgTriggerEditor::clearActionForm()
{
    mpActionsMainArea->hide();
    mpSourceEditorArea->hide();
    showInfo(msgInfoAddButton);
}

void dlgTriggerEditor::clearKeyForm()
{
    mpKeysMainArea->hide();
    mpSourceEditorArea->hide();
    showInfo(msgInfoAddKey);
}

void dlgTriggerEditor::clearVarForm()
{
    mpVarsMainArea->hide();
    mpSourceEditorArea->hide();
    showInfo(msgInfoAddVar);
}

void dlgTriggerEditor::setEditorShowBidi(const bool state)
{
    auto config = mpSourceEditorEdbee->config();
    config->beginChanges();
    config->setRenderBidiContolCharacters(state);
    config->endChanges();
    mpSourceEditorEdbee->controller()->update();
}

void dlgTriggerEditor::hideSystemMessageArea()
{
    mpSystemMessageArea->hide();

    if (mCurrentView != EditorViewType::cmScriptView) {
        return;
    }

    QTreeWidgetItem* pItem = treeWidget_scripts->currentItem();
    if (pItem) {
        TScript* pT = mpHost->getScriptUnit()->getScript(pItem->data(0, Qt::UserRole).toInt());
        if (pT && pT->getLoadingError()) {
            pT->clearLoadingError();
        }
    }
}

// In case the profile was reset while the editor was out of focus, checks for any script loading errors and displays them
void dlgTriggerEditor::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::ActivationChange && this->isActiveWindow()) {
        if (mCurrentView == EditorViewType::cmScriptView) {
            auto scriptTreeWidgetItem = treeWidget_scripts->currentItem();
            if (!scriptTreeWidgetItem) {
                return;
            }

            TScript* script = mpHost->getScriptUnit()->getScript(scriptTreeWidgetItem->data(0, Qt::UserRole).toInt());
            if (!script) {
                return;
            }
            if (auto error = script->getLoadingError(); error) {
                showWarning(tr("While loading the profile, this script had an error that has since been fixed, "
                               "possibly by another script. The error was:%2%3")
                                    .arg(qsl("<br>"), error.value()));
            }
        }
    }
}

void dlgTriggerEditor::showIDLabels(const bool visible)
{
    mpAliasMainArea->frameId->setVisible(visible);
    mpActionsMainArea->frameId->setVisible(visible);
    mpKeysMainArea->frameId->setVisible(visible);
    mpScriptsMainArea->frameId->setVisible(visible);
    mpTimersMainArea->frameId->setVisible(visible);
    mpTriggersMainArea->frameId->setVisible(visible);
}

QUndoStack* dlgTriggerEditor::getQUndoStack()
{
    return undoStack;
}
