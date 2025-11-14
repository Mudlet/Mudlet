/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2024 by Stephen Lyons - slysven@virginmedia.com    *
 *   Copyright (C) 2016 by Owen Davison - odavison@cs.dal.ca               *
 *   Copyright (C) 2016-2020 by Ian Adkins - ieadkins@gmail.com            *
 *   Copyright (C) 2017 by Tom Scheper - scheper@gmail.com                 *
 *   Copyright (C) 2023-2025 by Lecker Kebap - Leris@mudlet.org            *
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
#include "VarUnit.h"
#include "XMLimport.h"
#include "dlgActionMainArea.h"
#include "dlgAliasMainArea.h"
#include "dlgColorTrigger.h"
#include "dlgKeysMainArea.h"
#include "dlgPackageExporter.h"
#include "dlgProfilePreferences.h"
#include "dlgScriptsMainArea.h"
#include "dlgTriggerPatternEdit.h"
#include "SingleLineTextEdit.h"
#include "TrailingWhitespaceMarker.h"
#include "mudlet.h"
#include "utils.h"
#include "edbee/models/textdocumentscopes.h"

#include <QCheckBox>
#include <QAbstractButton>
#include <QColorDialog>
#include <QFileDialog>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMargins>
#include <QMessageBox>
#include <QMetaEnum>
#include <QPalette>
#include <QPoint>
#include <QScrollBar>
#include <QShortcut>
#include <QSpinBox>
#include <QStyle>
#include <QTextCursor>
#include <QShowEvent>
#include <QToolButton>
#include <QToolBar>
#include <QVBoxLayout>


using namespace std::chrono_literals;

static const auto patternNavigationBannerKey = qsl("pattern-navigation");

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

    // clang-format off
    introAddItem.insert(EditorViewType::cmAliasView, {
        //: Headline for the Alias intro
        tr("Alias react on user input."), {
        //: Name of a selectable option for the Alias intro
        {qsl("alias1"), tr("How to add a new alias now"),
        //: Help contents of a selectable option for the Alias intro
            tr("<ol><li>Click on the 'Add Item' icon above.</li>"
               "<li>Define an input <strong>pattern</strong> either literally or with a Perl regular expression.</li>"
               "<li>Define a 'substitution' <strong>command</strong> to send to the game in clear text <strong>instead of the alias pattern</strong>, or write a script for more complicated needs.</li>"
               "<li><strong>Activate</strong> the alias.</li></ol>")},
        //: Name of a selectable option for the Alias intro
        {qsl("alias2"), tr("How to add a new alias from the input line"),
            qsl("%1%2%3%4").arg(
                //: Help contents of a selectable option for the Alias intro
                qsl("<p>%1</p>").arg(tr("There are a <a href='https://forums.mudlet.org/viewtopic.php?f=6&t=22609'>couple</a> of <a href='https://forums.mudlet.org/viewtopic.php?f=6&t=16462'>packages</a> that can help you.")),
                //: Part of the Alias intro - This introductory text will be followed by a Lua code example for a trigger.
                qsl("<p>%1</p>").arg(tr("Alias can also be defined from the input line in the main profile window like this:")),
                qsl("<p><code>%1</code></p>").arg(qsl("lua permAlias(&quot;%1&quot;, &quot;&quot;, &quot;%2&quot;, function() send(&quot;%3&quot;) echo(&quot;%4&quot;) end)").arg(
                    //: Part of the Alias intro, code example for an alias - This is the name of the alias which reacts on the player typing "hi" by saying "Greetings, traveller!" in game.
                    tr("My greetings"),
                    //: Part of the Alias intro, code example for an alias - This is the text input from the player which will be reacted on by saying "Greetings, traveller!" in game.
                    tr("hi"),
                    //: Part of the Alias intro, code example for an alias - This is the command that Mudlet will send to the game after the player typed "hi".
                    tr("say Greetings, traveller!"),
                    //: Part of the Alias intro, code example for an alias - This is the confirmation text shown to the player after they typed "hi" and we said "Greetings, traveller!" in game.
                    tr("We said hi!"))),
                //: Part of the Alias intro - This is the conclusion after the code example for an alias which reacts on the player typing "hi" by saying "Greetings, traveller!" in game.
                qsl("<p>%1</p>").arg("You can now greet by typing 'hi'"))},
        {qsl("alias3"), tr("Where to find more information"),
            qsl("<ul>%1%2%3%4</ul>").arg( // reduce clutter for translators
                qsl("<li><p>%1</p><li>").arg(tr("Watch a <a href='%1'>video demonstration</a> of the basic functionality.")
                    .arg(qsl("https://youtu.be/Uz6EDvZYNvE"))),
                qsl("<li><p>%1</p></li>").arg(tr("Read the <a href='http://wiki.mudlet.org/w/Manual:Introduction#Aliases'>Introduction to Aliases</a> for a detailed overview.")),
                qsl("<li><p>%1</p>").arg(tr("Do you maybe have any other suggestions, questions or doubts?")),
                qsl("<p>%1</p></li>").arg(tr("Join our community on <a href='https://www.mudlet.org/chat'>Discord</a> or in <a href='https://forums.mudlet.org/'>Mudlet forums</a> - See you there!")))}}});

    introAddItem.insert(EditorViewType::cmTriggerView, {
        //: Headline for the Trigger intro
        tr("Triggers react on game output."), {
        //: Name of a selectable option for the Trigger intro
        {qsl("trigger1"), tr("How to add a new trigger now"),
        //: Help contents of a selectable option for the Trigger intro
            tr("<ol><li>Click on the 'Add Item' icon above.</li>"
               "<li>Define a <strong>pattern</strong> that you want to trigger on.</li>"
               "<li>Select the appropriate pattern <strong>type</strong>.</li>"
               "<li>Define a clear text <strong>command</strong> that you want to send to the game if the trigger finds the pattern in the text from the game, or write a script for more complicated needs..</li>"
               "<li><strong>Activate</strong> the trigger.</li></ol>")},
        //: Name of a selectable option for the Trigger intro
        {qsl("trigger2"), tr("How to add a new trigger from the input line"),
            qsl("%1%2%3%4").arg(
                //: Help contents of a selectable option for the Trigger intro
                qsl("<p>%1</p>").arg(tr("There are a <a href='https://forums.mudlet.org/viewtopic.php?f=6&t=22609'>couple</a> of <a href='https://forums.mudlet.org/viewtopic.php?f=6&t=16462'>packages</a> that can help you.")),
                //: Part of the Trigger intro - This introductory text will be followed by a Lua code example for a trigger.
                qsl("<p>%1</p>").arg(tr("Triggers can also be defined from the input line in the main profile window like this:")),
                qsl("<p><code>%1</code></p>").arg(qsl("lua permSubstringTrigger(&quot;%1&quot;, &quot;&quot;, &quot;%2&quot;, function() send(&quot;%3&quot;) end)").arg(
                    //: Part of the Trigger intro, code example for a trigger - This is the name of the trigger which reacts on "You are thirsty" with "drink water".
                    tr("My drink trigger"),
                    //: Part of the Trigger intro, code example for a trigger - This is the text from game which will be triggered on, and reacted to with "drink water".
                    tr("You are thirsty."),
                    //: Part of the Trigger intro, code example for a trigger - This is the command sent to game after we triggered on text "You are thirsty." from game.
                    tr("drink water"))),
                //: Part of the Trigger intro - This is the conclusion after the code example for a trigger which reacts on "You are thirsty" with "drink water".
                qsl("<p>%1</p>").arg("This will keep you refreshed."))},
        {qsl("trigger3"), tr("Where to find more information"),
            qsl("<ul>%1%2%3%4</ul>").arg( // reduce clutter for translators
                qsl("<li><p>%1</p><li>").arg(tr("Watch a <a href='%1'>video demonstration</a> of the basic functionality.")
                    .arg(qsl("https://youtu.be/jYjop54-Y3I"))),
                qsl("<li><p>%1</p></li>").arg(tr("Read the <a href='http://wiki.mudlet.org/w/Manual:Introduction#Triggers'>Introduction to Triggers</a> for a detailed overview.")),
                qsl("<li><p>%1</p>").arg(tr("Do you maybe have any other suggestions, questions or doubts?")),
                qsl("<p>%1</p></li>").arg(tr("Join our community on <a href='https://www.mudlet.org/chat'>Discord</a> or in <a href='https://forums.mudlet.org/'>Mudlet forums</a> - See you there!")))}}});

    introAddItem.insert(EditorViewType::cmScriptView, {
        //: Headline for the Script intro
        tr("Scripts organize code and can react to events."), {
        //: Name of a selectable option for the Script intro
        {qsl("script1"), tr("How to add a new script now"),
        //: Help contents of a selectable option for the Script intro
            tr("<ol><li>Click on the 'Add Item' icon above.</li>"
               "<li>Enter a script in the box below. You can for example define <strong>functions</strong> to be called by other triggers, aliases, etc.</li>"
               "<li>If you write lua <strong>commands</strong> without defining a function, they will be run on Mudlet startup and each time you open the script for editing.</li>"
               "<li><strong>Activate</strong> the script.</li></ol>"
               "<p><strong>Note:</strong> Scripts are run automatically when viewed, even if they are deactivated.</p>")},
        //: Name of a selectable option for the Script intro
        {qsl("script2"), tr("How to have a script react to events"),
        //: Help contents of a selectable option for the Script intro
            tr("<p>You can register a list of <strong>events</strong> with the + and - symbols. If one of these events take place, the function with the same name as the script item itself will be called.</p>"
               "<p><strong>Note:</strong> Events can also be added to a script from the command line in the main profile window like this:</p>"
               "<p><code>lua registerAnonymousEventHandler(&quot;nameOfTheMudletEvent&quot;, &quot;nameOfYourFunctionToBeCalled&quot;)</code></p>")},
        {qsl("script3"), tr("Where to find more information"),
            qsl("<ul>%1%2%3%4</ul>").arg( // reduce clutter for translators
                qsl("<li><p>%1</p><li>").arg(tr("Watch a <a href='%1'>video demonstration</a> of the basic functionality.")
                    .arg(qsl("https://youtu.be/10mJUh4Hq-A"))),
                qsl("<li><p>%1</p></li>").arg(tr("Read the <a href='http://wiki.mudlet.org/w/Manual:Introduction#Scripts'>Introduction to Scripts</a> for a detailed overview.")),
                qsl("<li><p>%1</p>").arg(tr("Do you maybe have any other suggestions, questions or doubts?")),
                qsl("<p>%1</p></li>").arg(tr("Join our community on <a href='https://www.mudlet.org/chat'>Discord</a> or in <a href='https://forums.mudlet.org/'>Mudlet forums</a> - See you there!")))}}});

    introAddItem.insert(EditorViewType::cmTimerView, {
        //: Headline for the Timer intro
        tr("Timers react after a timespan once or regularly."), {
        //: Name of a selectable option for the Timer intro
        {qsl("timer1"), tr("How to add a new timer now"),
        //: Help contents of a selectable option for the Timer intro
            tr("<ol><li>Click on the 'Add Item' icon above.</li>"
               "<li>Define the <strong>timespan</strong> after which the timer should react in a this format: hours : minutes : seconds.</li>"
               "<li>Define a clear text <strong>command</strong> that you want to send to the game when the time has passed, or write a script for more complicated needs.</li>"
               "<li><strong>Activate</strong> the timer.</li></ol>"
               "<p><strong>Note:</strong> If you want the trigger to react only once and not regularly, use the Lua tempTimer() function instead.</p>")},
        //: Name of a selectable option for the Timer intro
        {qsl("timer2"), tr("How to add a new timer from the input line"),
        //: Help contents of a selectable option for the Timer intro
            tr("<p>Timers can also be defined from the input line in the main profile window like this:</p>"
               "<p><code>lua tempTimer(3, function() echo(&quot;hello!\n&quot;) end)</code></p>"
               "<p>This will greet you exactly 3 seconds after it was made.</p>")},
        {qsl("timer3"), tr("Where to find more information"),
            qsl("<ul>%1%2%3</ul>").arg( // reduce clutter for translators
                qsl("<li><p>%1</p></li>").arg(tr("Read the <a href='http://wiki.mudlet.org/w/Manual:Introduction#Timers'>Introduction to Timers</a> for a detailed overview.")),
                qsl("<li><p>%1</p>").arg(tr("Do you maybe have any other suggestions, questions or doubts?")),
                qsl("<p>%1</p></li>").arg(tr("Join our community on <a href='https://www.mudlet.org/chat'>Discord</a> or in <a href='https://forums.mudlet.org/'>Mudlet forums</a> - See you there!")))}}});

    introAddItem.insert(EditorViewType::cmActionView, {
        //: Headline for the Button intro
        tr("Buttons react on mouse clicks."), {
        //: Name of a selectable option for the Button intro
        {qsl("button1"), tr("How to add a new button now"),
        //: Help contents of a selectable option for the Button intro
            tr("<ol><li>Add a new group to define a new <strong>button bar</strong> in case you don't have any.</li>"
               "<li>Add new groups as <strong>menus</strong> to a button bar or sub-menus to menus.<li>"
               "<li>Add new items as <strong>buttons</strong> to a button bar or menu or sub-menu.</li>"
               "<li>Define a clear text <strong>command</strong> that you want to send to the game if the button is pressed, or write a script for more complicated needs.</li>"
               "<li><strong>Activate</strong> the toolbar, menu or button. </li></ol>"
               "<p><strong>Note:</strong> Deactivated items will be hidden and if they are toolbars or menus then all the items they contain will be also be hidden.</p>"
               "<p><strong>Note:</strong> If a button is made a <strong>click-down</strong> button then you may also define a clear text command that you want to send to the game when the button is pressed a second time to uncheck it or to write a script to run when it happens - within such a script the Lua 'getButtonState()' function reports whether the button is up or down.</p>")},
//        {qsl("button2"), tr("How to add a new button from the input line"),
//            tr("")},
        {qsl("button3"), tr("Where to find more information"),
            qsl("<ul>%1%2%3</ul>").arg( // reduce clutter for translators
                qsl("<li><p>%1</p></li>").arg(tr("Read the <a href='http://wiki.mudlet.org/w/Manual:Introduction#Buttons'>Introduction to Buttons</a> for a detailed overview.")),
                qsl("<li><p>%1</p>").arg(tr("Do you maybe have any other suggestions, questions or doubts?")),
                qsl("<p>%1</p></li>").arg(tr("Join our community on <a href='https://www.mudlet.org/chat'>Discord</a> or in <a href='https://forums.mudlet.org/'>Mudlet forums</a> - See you there!")))}}});

    introAddItem.insert(EditorViewType::cmKeysView, {
        //: Headline for the Keys intro
        tr("Keys react on keyboard presses."), {
        //: Name of a selectable option for the Keys intro
        {qsl("key1"), tr("How to add a new keybinding now"),
        //: Help contents of a selectable option for the Keys intro
            tr("<ol><li>Click on the 'Add Item' icon above.</li>"
               "<li>Click on <strong>'grab key'</strong> and then press your key combination, e.g. including modifier keys like Control, Shift, etc.</li>"
               "<li>Define a clear text <strong>command</strong> that you want to send to the game if the button is pressed, or write a script for more complicated needs.</li>"
               "<li><strong>Activate</strong> the new key binding.</li></ol>")},
        //: Name of a selectable option for the Keys intro
        {qsl("key2"), tr("How to add a new keybinding from the input line"),
        //: Help contents of a selectable option for the Keys intro
            tr("<p>Keys can be defined from the input line in the main profile window like this:</p>"
               "<p><code>lua permKey(&quot;my jump key&quot;, &quot;&quot;, mudlet.key.F8, [[send(&quot;jump&quot;]]) end)</code></p>"
               "<p>Pressing F8 will make you jump.</p>")},
        {qsl("key3"), tr("Where to find more information"),
            qsl("<ul>%1%2%3%4</ul>").arg( // reduce clutter for translators
                qsl("<li><p>%1</p><li>").arg(tr("Watch a <a href='%1'>video demonstration</a> of the basic functionality.")
                    .arg(qsl("https://youtu.be/ZYRPZ-8fJWA"))),
                qsl("<li><p>%1</p></li>").arg(tr("Read the <a href='http://wiki.mudlet.org/w/Manual:Introduction#Keybindings'>Introduction to Keybindings</a> for a detailed overview.")),
                qsl("<li><p>%1</p>").arg(tr("Do you maybe have any other suggestions, questions or doubts?")),
                qsl("<p>%1</p></li>").arg(tr("Join our community on <a href='https://www.mudlet.org/chat'>Discord</a> or in <a href='https://forums.mudlet.org/'>Mudlet forums</a> - See you there!")))}}});

    introAddItem.insert(EditorViewType::cmVarsView, {
        //: Headline for the Variable intro
        tr("Variables store information."), {
        //: Name of a selectable option for the Variable intro
        {qsl("variable1"), tr("How to add a new variable now"),
        //: Help contents of a selectable option for the Variable intro
            tr("<ol><li>Click on the 'Add Item' icon above. To add a table instead click 'Add Group'.</li>"
               "<li>Select type of variable value (can be a string, integer, boolean)</li>"
               "<li>Enter the value you want to store in this variable.</li>"
               "<li>If you want to keep the variable in your next Mudlet sessions, check the checkbox in the list of variables to the left.</li>"
               "<li>To remove a variable manually, set it to 'nil' or click on the 'Delete' icon above.</li></ol>"
               "<p><strong>Note:</strong> Variables created here won't be saved when Mudlet shuts down unless you check their checkbox in the list of variables to the left. You could also create scripts with the variables instead.</p>")},
        //: Name of a selectable option for the Variable intro
        {qsl("variable2"), tr("How to add a new variable from the input line"),
        //: Help contents of a selectable option for the Variable intro
            tr("<p>Variables and tables can also be defined from the input line in the main profile window like this:</p>"
               "<p><code>lua foo = &quot;bar&quot;</code></p>"
               "<p>This will create a string called 'foo' with 'bar' as its value.</p>")},
        {qsl("variable3"), tr("Where to find more information"),
            qsl("<ul>%1%2%3</ul>").arg( // reduce clutter for translators
                qsl("<li><p>%1</p></li>").arg(tr("Read the <a href='http://wiki.mudlet.org/w/Manual:Introduction#Variables'>Introduction to Variables</a> for a detailed overview.")),
                qsl("<li><p>%1</p>").arg(tr("Do you maybe have any other suggestions, questions or doubts?")),
                qsl("<p>%1</p></li>").arg(tr("Join our community on <a href='https://www.mudlet.org/chat'>Discord</a> or in <a href='https://forums.mudlet.org/'>Mudlet forums</a> - See you there!")))}}});
    // clang-format on

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
    //: Accessible description for a newly created folder, shown after the folder name
    descNewFolder = tr("new folder");
    //: Accessible description for a newly created item, shown after the item name
    descNewItem = tr("new item");
    //: Accessible description indicating an item belongs to a package, shown after the item name. Keep short, as it's appended to other descriptions like "activated, package item"
    descPackageItem = tr("package item");

    setUnifiedTitleAndToolBarOnMac(true); //MAC OSX: make window moveable
    const QString hostName{mpHost->getName()};
    setWindowTitle(tr("%1 - Editor").arg(hostName));
    setWindowIcon(QIcon(qsl(":/icons/mudlet_editor.png")));
    auto statusBar = new QStatusBar(this);
    statusBar->setSizeGripEnabled(true);
    setStatusBar(statusBar);
    statusBar->show();

    mpNonCodeWidgets = new QWidget(this);
    auto *layoutColumn = new QVBoxLayout(mpNonCodeWidgets);
    splitter_right->addWidget(mpNonCodeWidgets);

    // system message area
    mpSystemMessageArea = new dlgSystemMessageArea(this);
    mpSystemMessageArea->setObjectName(qsl("mpSystemMessageArea"));
    mpSystemMessageArea->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
    // set the stretch factor of the message area to 0 and everything else to 1,
    // so our errors box doesn't stretch to produce a grey area
    layoutColumn->addWidget(mpSystemMessageArea, 0);
    connect(mpSystemMessageArea->messageAreaCloseButton, &QAbstractButton::clicked, this, &dlgTriggerEditor::hideSystemMessageArea);
    connect(mpSystemMessageArea->notificationAreaMessageBox, &QLabel::linkActivated, this, &dlgTriggerEditor::slot_clickedMessageBox);

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
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    connect(mpActionsMainArea->checkBox_action_button_isPushDown, &QCheckBox::checkStateChanged, this, &dlgTriggerEditor::slot_toggleIsPushDownButton);
#else
    connect(mpActionsMainArea->checkBox_action_button_isPushDown, &QCheckBox::stateChanged, this, &dlgTriggerEditor::slot_toggleIsPushDownButton);
#endif

    mpKeysMainArea = new dlgKeysMainArea(this);
    layoutColumn->addWidget(mpKeysMainArea, 1);
    connect(mpKeysMainArea->pushButton_key_grabKey, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_keyGrab);

    mpVarsMainArea = new dlgVarsMainArea(this);
    layoutColumn->addWidget(mpVarsMainArea, 1);

    mpScriptsMainArea = new dlgScriptsMainArea(this);
    layoutColumn->addWidget(mpScriptsMainArea, 1);

    connect(mpScriptsMainArea->lineEdit_script_event_handler_entry, &QLineEdit::returnPressed, this, &dlgTriggerEditor::slot_scriptMainAreaAddHandler);
    connect(mpScriptsMainArea->listWidget_script_registered_event_handlers, &QListWidget::itemSelectionChanged, this, &dlgTriggerEditor::slot_scriptMainAreaEditHandler);
    connect(mpScriptsMainArea->listWidget_script_registered_event_handlers, &QListWidget::itemActivated, this, &dlgTriggerEditor::slot_scriptMainAreaClearHandlerSelection);


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

    // Lua reserved keywords (highest priority for basic syntax)
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

    // Standard Lua library functions (priority 4 - between Mudlet functions and keywords)
    // String library
    provider->add(qsl("string.byte"), 4, qsl("string.byte(s [, i [, j]])"));
    provider->add(qsl("string.char"), 4, qsl("string.char(...)"));
    provider->add(qsl("string.dump"), 4, qsl("string.dump(function)"));
    provider->add(qsl("string.find"), 4, qsl("string.find(s, pattern [, init [, plain]])"));
    provider->add(qsl("string.format"), 4, qsl("string.format(formatstring, ...)"));
    provider->add(qsl("string.gmatch"), 4, qsl("string.gmatch(s, pattern)"));
    provider->add(qsl("string.gsub"), 4, qsl("string.gsub(s, pattern, repl [, n])"));
    provider->add(qsl("string.len"), 4, qsl("string.len(s)"));
    provider->add(qsl("string.lower"), 4, qsl("string.lower(s)"));
    provider->add(qsl("string.match"), 4, qsl("string.match(s, pattern [, init])"));
    provider->add(qsl("string.rep"), 4, qsl("string.rep(s, n)"));
    provider->add(qsl("string.reverse"), 4, qsl("string.reverse(s)"));
    provider->add(qsl("string.sub"), 4, qsl("string.sub(s, i [, j])"));
    provider->add(qsl("string.upper"), 4, qsl("string.upper(s)"));

    // Table library
    provider->add(qsl("table.concat"), 4, qsl("table.concat(list [, sep [, i [, j]]])"));
    provider->add(qsl("table.insert"), 4, qsl("table.insert(list, [pos,] value)"));
    provider->add(qsl("table.pack"), 4, qsl("table.pack(...)"));
    provider->add(qsl("table.remove"), 4, qsl("table.remove(list [, pos])"));
    provider->add(qsl("table.sort"), 4, qsl("table.sort(list [, comp])"));
    provider->add(qsl("table.unpack"), 4, qsl("table.unpack(list [, i [, j]])"));

    // Math library
    provider->add(qsl("math.abs"), 4, qsl("math.abs(x)"));
    provider->add(qsl("math.acos"), 4, qsl("math.acos(x)"));
    provider->add(qsl("math.asin"), 4, qsl("math.asin(x)"));
    provider->add(qsl("math.atan"), 4, qsl("math.atan(x)"));
    provider->add(qsl("math.atan2"), 4, qsl("math.atan2(y, x)"));
    provider->add(qsl("math.ceil"), 4, qsl("math.ceil(x)"));
    provider->add(qsl("math.cos"), 4, qsl("math.cos(x)"));
    provider->add(qsl("math.cosh"), 4, qsl("math.cosh(x)"));
    provider->add(qsl("math.deg"), 4, qsl("math.deg(x)"));
    provider->add(qsl("math.exp"), 4, qsl("math.exp(x)"));
    provider->add(qsl("math.floor"), 4, qsl("math.floor(x)"));
    provider->add(qsl("math.fmod"), 4, qsl("math.fmod(x, y)"));
    provider->add(qsl("math.frexp"), 4, qsl("math.frexp(x)"));
    provider->add(qsl("math.huge"), 4, qsl("math.huge"));
    provider->add(qsl("math.ldexp"), 4, qsl("math.ldexp(m, e)"));
    provider->add(qsl("math.log"), 4, qsl("math.log(x [, base])"));
    provider->add(qsl("math.log10"), 4, qsl("math.log10(x)"));
    provider->add(qsl("math.max"), 4, qsl("math.max(x, ...)"));
    provider->add(qsl("math.min"), 4, qsl("math.min(x, ...)"));
    provider->add(qsl("math.modf"), 4, qsl("math.modf(x)"));
    provider->add(qsl("math.pi"), 4, qsl("math.pi"));
    provider->add(qsl("math.pow"), 4, qsl("math.pow(x, y)"));
    provider->add(qsl("math.rad"), 4, qsl("math.rad(x)"));
    provider->add(qsl("math.random"), 4, qsl("math.random([m [, n]])"));
    provider->add(qsl("math.randomseed"), 4, qsl("math.randomseed(x)"));
    provider->add(qsl("math.sin"), 4, qsl("math.sin(x)"));
    provider->add(qsl("math.sinh"), 4, qsl("math.sinh(x)"));
    provider->add(qsl("math.sqrt"), 4, qsl("math.sqrt(x)"));
    provider->add(qsl("math.tan"), 4, qsl("math.tan(x)"));
    provider->add(qsl("math.tanh"), 4, qsl("math.tanh(x)"));

    // IO library
    provider->add(qsl("io.close"), 4, qsl("io.close([file])"));
    provider->add(qsl("io.flush"), 4, qsl("io.flush()"));
    provider->add(qsl("io.input"), 4, qsl("io.input([file])"));
    provider->add(qsl("io.lines"), 4, qsl("io.lines([filename, ...])"));
    provider->add(qsl("io.open"), 4, qsl("io.open(filename [, mode])"));
    provider->add(qsl("io.output"), 4, qsl("io.output([file])"));
    provider->add(qsl("io.popen"), 4, qsl("io.popen(prog [, mode])"));
    provider->add(qsl("io.read"), 4, qsl("io.read(...)"));
    provider->add(qsl("io.tmpfile"), 4, qsl("io.tmpfile()"));
    provider->add(qsl("io.type"), 4, qsl("io.type(obj)"));
    provider->add(qsl("io.write"), 4, qsl("io.write(...)"));

    // OS library
    provider->add(qsl("os.clock"), 4, qsl("os.clock()"));
    provider->add(qsl("os.date"), 4, qsl("os.date([format [, time]])"));
    provider->add(qsl("os.difftime"), 4, qsl("os.difftime(t2, t1)"));
    provider->add(qsl("os.execute"), 4, qsl("os.execute([command])"));
    provider->add(qsl("os.exit"), 4, qsl("os.exit([code [, close]])"));
    provider->add(qsl("os.getenv"), 4, qsl("os.getenv(varname)"));
    provider->add(qsl("os.remove"), 4, qsl("os.remove(filename)"));
    provider->add(qsl("os.rename"), 4, qsl("os.rename(oldname, newname)"));
    provider->add(qsl("os.setlocale"), 4, qsl("os.setlocale(locale [, category])"));
    provider->add(qsl("os.time"), 4, qsl("os.time([table])"));
    provider->add(qsl("os.tmpname"), 4, qsl("os.tmpname()"));

    // Coroutine library
    provider->add(qsl("coroutine.create"), 4, qsl("coroutine.create(f)"));
    provider->add(qsl("coroutine.resume"), 4, qsl("coroutine.resume(co [, val1, ...])"));
    provider->add(qsl("coroutine.running"), 4, qsl("coroutine.running()"));
    provider->add(qsl("coroutine.status"), 4, qsl("coroutine.status(co)"));
    provider->add(qsl("coroutine.wrap"), 4, qsl("coroutine.wrap(f)"));
    provider->add(qsl("coroutine.yield"), 4, qsl("coroutine.yield(...)"));

    // Debug library
    provider->add(qsl("debug.debug"), 4, qsl("debug.debug()"));
    provider->add(qsl("debug.gethook"), 4, qsl("debug.gethook([thread])"));
    provider->add(qsl("debug.getinfo"), 4, qsl("debug.getinfo([thread,] f [, what])"));
    provider->add(qsl("debug.getlocal"), 4, qsl("debug.getlocal([thread,] f, local)"));
    provider->add(qsl("debug.getmetatable"), 4, qsl("debug.getmetatable(value)"));
    provider->add(qsl("debug.getregistry"), 4, qsl("debug.getregistry()"));
    provider->add(qsl("debug.getupvalue"), 4, qsl("debug.getupvalue(f, up)"));
    provider->add(qsl("debug.getuservalue"), 4, qsl("debug.getuservalue(u)"));
    provider->add(qsl("debug.sethook"), 4, qsl("debug.sethook([thread,] hook, mask [, count])"));
    provider->add(qsl("debug.setlocal"), 4, qsl("debug.setlocal([thread,] level, local, value)"));
    provider->add(qsl("debug.setmetatable"), 4, qsl("debug.setmetatable(value, table)"));
    provider->add(qsl("debug.setupvalue"), 4, qsl("debug.setupvalue(f, up, value)"));
    provider->add(qsl("debug.setuservalue"), 4, qsl("debug.setuservalue(udata, value)"));
    provider->add(qsl("debug.traceback"), 4, qsl("debug.traceback([thread,] [message [, level]])"));
    provider->add(qsl("debug.upvalueid"), 4, qsl("debug.upvalueid(f, n)"));
    provider->add(qsl("debug.upvaluejoin"), 4, qsl("debug.upvaluejoin(f1, n1, f2, n2)"));

    // Package library
    provider->add(qsl("package.config"), 4, qsl("package.config"));
    provider->add(qsl("package.cpath"), 4, qsl("package.cpath"));
    provider->add(qsl("package.loaded"), 4, qsl("package.loaded"));
    provider->add(qsl("package.loadlib"), 4, qsl("package.loadlib(libname, funcname)"));
    provider->add(qsl("package.path"), 4, qsl("package.path"));
    provider->add(qsl("package.preload"), 4, qsl("package.preload"));
    provider->add(qsl("package.searchers"), 4, qsl("package.searchers"));
    provider->add(qsl("package.searchpath"), 4, qsl("package.searchpath(name, path [, sep [, rep]])"));

    // Mudlet framework namespaced functions (priority 4 - same as Lua stdlib)
    // Geyser UI Framework
    provider->add(qsl("Geyser.Container:new"), 4, qsl("Geyser.Container:new(cons, container)"));
    provider->add(qsl("Geyser.Window:new"), 4, qsl("Geyser.Window:new(cons, container)"));
    provider->add(qsl("Geyser.Label:new"), 4, qsl("Geyser.Label:new(cons, container)"));
    provider->add(qsl("Geyser.MiniConsole:new"), 4, qsl("Geyser.MiniConsole:new(cons, container)"));
    provider->add(qsl("Geyser.Button:new"), 4, qsl("Geyser.Button:new(cons, container)"));
    provider->add(qsl("Geyser.Gauge:new"), 4, qsl("Geyser.Gauge:new(cons, container)"));
    provider->add(qsl("Geyser.Mapper:new"), 4, qsl("Geyser.Mapper:new(cons, container)"));
    provider->add(qsl("Geyser.UserWindow:new"), 4, qsl("Geyser.UserWindow:new(cons)"));
    provider->add(qsl("Geyser.CommandLine:new"), 4, qsl("Geyser.CommandLine:new(cons, container)"));
    provider->add(qsl("Geyser.HBox:new"), 4, qsl("Geyser.HBox:new(cons, container)"));
    provider->add(qsl("Geyser.VBox:new"), 4, qsl("Geyser.VBox:new(cons, container)"));
    provider->add(qsl("Geyser.ScrollBox:new"), 4, qsl("Geyser.ScrollBox:new(cons, container)"));
    provider->add(qsl("Geyser.ScrollBox:new2"), 4, qsl("Geyser.ScrollBox:new2()"));
    provider->add(qsl("Geyser.StyleSheet:new"), 4, qsl("Geyser.StyleSheet:new(stylesheet, parent, target)"));

    // Geyser namespace functions
    provider->add(qsl("Geyser.Color.parse"), 4, qsl("Geyser.Color.parse(color)"));
    provider->add(qsl("Geyser.Color.hex"), 4, qsl("Geyser.Color.hex(color)"));
    provider->add(qsl("Geyser.Color.hexa"), 4, qsl("Geyser.Color.hexa(color)"));
    provider->add(qsl("Geyser.Color.hhex"), 4, qsl("Geyser.Color.hhex(color)"));
    provider->add(qsl("Geyser.Color.hhexa"), 4, qsl("Geyser.Color.hhexa(color)"));
    provider->add(qsl("Geyser.Color.hdec"), 4, qsl("Geyser.Color.hdec(color)"));
    provider->add(qsl("Geyser.Color.hdeca"), 4, qsl("Geyser.Color.hdeca(color)"));

    // Adjustable Container Framework
    provider->add(qsl("Adjustable.Container:new"), 4, qsl("Adjustable.Container:new(cons, container)"));

    // Database Framework
    provider->add(qsl("db.create"), 4, qsl("db.create(db_name, schema)"));
    provider->add(qsl("db.query"), 4, qsl("db.query(db_name, query, ...)"));
    provider->add(qsl("db.insert"), 4, qsl("db.insert(db_name, sheet_name, values)"));
    provider->add(qsl("db.update"), 4, qsl("db.update(db_name, sheet_name, values, query)"));
    provider->add(qsl("db.delete"), 4, qsl("db.delete(db_name, sheet_name, query)"));
    provider->add(qsl("db.fetch"), 4, qsl("db.fetch(db_name, query, ...)"));
    provider->add(qsl("db.aggregate"), 4, qsl("db.aggregate(db_name, query, ...)"));

    // DateTime utilities
    provider->add(qsl("datetime.parse"), 4, qsl("datetime.parse(format, date_string)"));

    // Set the newly filled provider to be used by our Edbee instance
    edbee::Edbee::instance()->autoCompleteProviderList()->setParentProvider(provider);

    mpSourceEditorEdbee->textEditorComponent()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(mpSourceEditorEdbee->textEditorComponent(), &QWidget::customContextMenuRequested, this, &dlgTriggerEditor::slot_editorContextMenu);

    // option areas
    mpErrorConsole = new TConsole(mpHost, qsl("errors_%1").arg(hostName), TConsole::ErrorConsole, this);
    mpErrorConsole->setWrapAt(100);
    mpErrorConsole->slot_toggleTimeStamps(true);
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

    // Add delete action to all tree widgets for right-click context menu
    treeWidget_triggers->addAction(mDeleteItem);
    treeWidget_aliases->addAction(mDeleteItem);
    treeWidget_timers->addAction(mDeleteItem);
    treeWidget_scripts->addAction(mDeleteItem);
    treeWidget_actions->addAction(mDeleteItem);
    treeWidget_keys->addAction(mDeleteItem);
    treeWidget_variables->addAction(mDeleteItem);

    // Add separators and additional actions to context menu
    QAction* separator1 = new QAction(this);
    separator1->setSeparator(true);
    QAction* separator2 = new QAction(this);
    separator2->setSeparator(true);

    // Add context menu actions to all tree widgets
    QList<QTreeWidget*> treeWidgets = {treeWidget_triggers, treeWidget_aliases, treeWidget_timers,
                                       treeWidget_scripts, treeWidget_actions, treeWidget_keys, treeWidget_variables};

    for (QTreeWidget* widget : treeWidgets) {
        widget->addAction(mAddItem);
        widget->addAction(mAddGroup);
        widget->addAction(separator1);
        // Copy, Paste, Delete are already added above
        widget->addAction(separator2);
    }

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

    mpCreateModuleAction = new QAction(QIcon(qsl(":/icons/package-exporter.png")), tr("Create Module"), this);
    mpCreateModuleAction->setEnabled(true);
    mpCreateModuleAction->setToolTip(tr("<p>Create a module from selected items</p>"));
    connect(mpCreateModuleAction, &QAction::triggered, this, &dlgTriggerEditor::slot_createModule);

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
    mProfileSaveAction->setStatusTip(tr(R"(Saves your entire profile (triggers, aliases, scripts, timers, buttons and keys, but not the map or script-specific settings); also "synchronizes" modules that are so marked.)"));

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

    auto *nextSectionShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Tab), this);
    QObject::connect(nextSectionShortcut, &QShortcut::activated, this, &dlgTriggerEditor::slot_nextSection);

    QShortcut *previousSectionShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Tab), this);
    connect(previousSectionShortcut, &QShortcut::activated, this, &dlgTriggerEditor::slot_previousSection);

    QShortcut *activateMainWindowAction = new QShortcut(QKeySequence((Qt::ALT | Qt::Key_E)), this);
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
    toolBar->addAction(mpCreateModuleAction);
    toolBar->addAction(mProfileSaveAsAction);
    toolBar->addAction(mProfileSaveAction);

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
    config->setShowWhitespaceMode((mudlet::self()->mEditorTextOptions & QTextOption::ShowTabsAndSpaces)
                                  ? edbee::TextEditorConfig::ShowWhitespaces
                                  : edbee::TextEditorConfig::HideWhitespaces);
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

    // triggers
    connect(mpTriggersMainArea->lineEdit_trigger_name, &QLineEdit::textEdited, this, &dlgTriggerEditor::slot_itemEdited);
    connect(mpTriggersMainArea->lineEdit_trigger_command, &QLineEdit::textEdited, this, &dlgTriggerEditor::slot_itemEdited);
    connect(mpTriggersMainArea->pushButtonSound, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_itemEdited);
    connect(mpSourceEditorEdbeeDocument, &edbee::TextDocument::textChanged, this, &dlgTriggerEditor::slot_itemEdited);
    // aliases
    connect(mpAliasMainArea->lineEdit_alias_name, &QLineEdit::textEdited, this, &dlgTriggerEditor::slot_itemEdited);
    connect(mpAliasMainArea->lineEdit_alias_pattern, &QLineEdit::textEdited, this, &dlgTriggerEditor::slot_itemEdited);
    connect(mpAliasMainArea->lineEdit_alias_command, &QLineEdit::textEdited, this, &dlgTriggerEditor::slot_itemEdited);
    // scripts
    connect(mpScriptsMainArea->lineEdit_script_name, &QLineEdit::textEdited, this, &dlgTriggerEditor::slot_itemEdited);
    connect(mpScriptsMainArea->lineEdit_script_event_handler_entry, &QLineEdit::textEdited, this, &dlgTriggerEditor::slot_itemEdited);
    // timers
    connect(mpTimersMainArea->lineEdit_timer_name, &QLineEdit::textEdited, this, &dlgTriggerEditor::slot_itemEdited);
    connect(mpTimersMainArea->lineEdit_timer_command, &QLineEdit::textEdited, this, &dlgTriggerEditor::slot_itemEdited);
    // keys
    connect(mpKeysMainArea->lineEdit_key_name, &QLineEdit::textEdited, this, &dlgTriggerEditor::slot_itemEdited);
    connect(mpKeysMainArea->lineEdit_key_command, &QLineEdit::textEdited, this, &dlgTriggerEditor::slot_itemEdited);
    connect(mpKeysMainArea->pushButton_key_grabKey, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_itemEdited);
    // buttons
    connect(mpActionsMainArea->lineEdit_action_name, &QLineEdit::textEdited, this, &dlgTriggerEditor::slot_itemEdited);
    connect(mpActionsMainArea->lineEdit_action_name, &QLineEdit::textEdited, this, &dlgTriggerEditor::slot_itemEdited);

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

    comboBox_searchTerms->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QFrame *searchContainer = new QFrame();
    QVBoxLayout *searchLayout = new QVBoxLayout(searchContainer);
    searchLayout->addWidget(checkBox_displayAllVariables);
    searchLayout->addWidget(comboBox_searchTerms);
    searchLayout->addWidget(treeWidget_searchResults);

    searchSplitter = new QSplitter(Qt::Vertical);

    connect(searchSplitter, &QSplitter::splitterMoved, this, &dlgTriggerEditor::slot_searchSplitterMoved);

    QFrame *itemContainer = new QFrame();
    QVBoxLayout *itemLayout = new QVBoxLayout(itemContainer);

    itemLayout->addWidget(treeWidget_triggers);
    itemLayout->addWidget(treeWidget_aliases);
    itemLayout->addWidget(treeWidget_actions);
    itemLayout->addWidget(treeWidget_timers);
    itemLayout->addWidget(treeWidget_scripts);
    itemLayout->addWidget(treeWidget_keys);
    itemLayout->addWidget(treeWidget_variables);

    searchSplitter->addWidget(itemContainer);
    searchSplitter->setStretchFactor(0, 1);
    searchSplitter->setCollapsible(0, false);
    searchSplitter->addWidget(searchContainer);
    searchSplitter->setStretchFactor(1, 1);
    searchSplitter->setCollapsible(1, true);

    verticalLayout_frame_left->addWidget(searchSplitter);

    searchSplitter->restoreState(mSearchSplitterState);

    mpScrollArea = mpTriggersMainArea->scrollArea;
    mpWidget_triggerItems = new QWidget;
    auto lay1 = new QVBoxLayout(mpWidget_triggerItems);
    lay1->setContentsMargins(0, 0, 0, 0);
    lay1->setSpacing(0);
    mpScrollArea->setWidget(mpWidget_triggerItems);

    lay1->addStretch();

    mPatternNavigationHintBanner = new QFrame(mpWidget_triggerItems);
    mPatternNavigationHintBanner->setObjectName(qsl("patternNavigationHintBanner"));
    mPatternNavigationHintBanner->setFrameShape(QFrame::StyledPanel);
    mPatternNavigationHintBanner->setFrameShadow(QFrame::Raised);
    mPatternNavigationHintBanner->setFocusPolicy(Qt::StrongFocus);

    auto* patternNavigationHintLayout = new QHBoxLayout(mPatternNavigationHintBanner);
    patternNavigationHintLayout->setContentsMargins(12, 12, 12, 12);
    patternNavigationHintLayout->setSpacing(8);

    auto* patternNavigationHintIcon = new QLabel(mPatternNavigationHintBanner);
    patternNavigationHintIcon->setObjectName(qsl("patternNavigationHintIcon"));
    patternNavigationHintIcon->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    const int patternNavigationHintIconSize = qMax(24, style()->pixelMetric(QStyle::PM_SmallIconSize, nullptr, mPatternNavigationHintBanner));
    patternNavigationHintIcon->setPixmap(QIcon(qsl(":/icons/dialog-information.png")).pixmap(patternNavigationHintIconSize, patternNavigationHintIconSize));
    patternNavigationHintLayout->addWidget(patternNavigationHintIcon);

    mPatternNavigationHintLabel = new QLabel(mPatternNavigationHintBanner);
    mPatternNavigationHintLabel->setObjectName(qsl("patternNavigationHintLabel"));
    mPatternNavigationHintLabel->setWordWrap(true);
    mPatternNavigationHintLabel->setTextFormat(Qt::RichText);
    mPatternNavigationHintLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    mPatternNavigationHintLabel->setTextInteractionFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);
    QFont hintFont = mPatternNavigationHintLabel->font();
    hintFont.setPointSizeF(qMax(7.0, hintFont.pointSizeF() - 1.0));
    mPatternNavigationHintLabel->setFont(hintFont);
    patternNavigationHintLayout->addWidget(mPatternNavigationHintLabel, 1);

    mPatternNavigationHintCloseButton = new QToolButton(mPatternNavigationHintBanner);
    mPatternNavigationHintCloseButton->setObjectName(qsl("patternNavigationHintCloseButton"));
    mPatternNavigationHintCloseButton->setAutoRaise(true);
    mPatternNavigationHintCloseButton->setFixedSize(16, 16);
    mPatternNavigationHintCloseButton->setIcon(QIcon::fromTheme(qsl("dialog-close"), QIcon(qsl(":/icons/dialog-close.png"))));
    //: Tooltip for the button that hides the pattern navigation hint banner.
    mPatternNavigationHintCloseButton->setToolTip(tr("Hide this hint"));
    patternNavigationHintLayout->addWidget(mPatternNavigationHintCloseButton);
    patternNavigationHintLayout->setAlignment(mPatternNavigationHintCloseButton, Qt::AlignTop);

    connect(mPatternNavigationHintCloseButton, &QAbstractButton::clicked, this, &dlgTriggerEditor::handlePatternNavigationHintDismiss);

    updatePatternNavigationHint();
    lay1->insertWidget(lay1->count() - 1, mPatternNavigationHintBanner);

    if (mPatternNavigationHintHidden && mPatternNavigationHintBanner) {
        mPatternNavigationHintBanner->hide();
    }

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

    mPatternList << tr("substring")
                 << tr("perl regex")
                 << tr("start of line")
                 << tr("exact match")
                 << tr("lua function")
                 << tr("line spacer")
                 << tr("color trigger")
                 << tr("prompt");

    mPatternIcons = {icon_subString,
                     icon_perl_regex,
                     icon_begin_of_line_substring,
                     icon_exact_match,
                     icon_lua_function,
                     icon_line_spacer,
                     icon_color_trigger,
                     icon_prompt};


    showPatternItems(2);
    setupPatternNavigationShortcuts();
    updatePatternTabOrder();

    connect(mpHost, &Host::signal_editorThemeChanged, this, &dlgTriggerEditor::slot_editorThemeChanged);
    // fire this now as the theme has already been set and we need the syntax highlighter to pick it up
    mpHost->editorThemeChanged();

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

void dlgTriggerEditor::slot_searchSplitterMoved(const int pos, const int index)
{
    Q_UNUSED(pos)
    Q_UNUSED(index)
    mSearchSplitterState = searchSplitter->saveState();
}

void dlgTriggerEditor::slot_clickedMessageBox(const QString& URL)
{
    if (URL.startsWith("http")) {
        QDesktopServices::openUrl(URL);
    } else { // internal links used by expanding info text navigation
        showIntro(URL);
    }
}

void dlgTriggerEditor::slot_editorThemeChanged()
{
    for (int i = 0; i < mTriggerPatternEdit.size(); i++) {
        applyPatternWidgetStyle(mTriggerPatternEdit.at(i));
    }
}

void dlgTriggerEditor::applyPatternWidgetStyle(dlgTriggerPatternEdit* patternWidget)
{
    if (!patternWidget || !mpHost) {
        return;
    }

    QPalette referencePalette;
    QFont referenceFont;
    bool hasReference = false;

    if (mpTriggersMainArea && mpTriggersMainArea->lineEdit_trigger_name) {
        referencePalette = mpTriggersMainArea->lineEdit_trigger_name->palette();
        referenceFont = mpTriggersMainArea->lineEdit_trigger_name->font();
        hasReference = true;
    }

    patternWidget->singleLineTextEdit_pattern->setTheme(mpHost->mEditorTheme);
    if (!hasReference) {
        referencePalette = patternWidget->singleLineTextEdit_pattern->palette();
        referenceFont = mpHost->getDisplayFont();
    }
    patternWidget->applyThemePalette(referencePalette);
    patternWidget->singleLineTextEdit_pattern->setFont(referenceFont);
}

void dlgTriggerEditor::createPatternItem(int index)
{
    auto* pItem = new dlgTriggerPatternEdit(mpWidget_triggerItems);
    QComboBox* pBox = pItem->comboBox_patternType;
    pBox->addItems(mPatternList);
    pBox->setItemData(0, QVariant(index));
    for (int i = 0; i < mPatternIcons.size(); ++i) {
        pBox->setItemIcon(i, mPatternIcons.at(i));
    }
    connect(pBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &dlgTriggerEditor::slot_setupPatternControls);
    connect(pItem->pushButton_fgColor, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_colorTriggerFg);
    connect(pItem->pushButton_bgColor, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_colorTriggerBg);
    connect(pItem->singleLineTextEdit_pattern, &QPlainTextEdit::textChanged, this, &dlgTriggerEditor::slot_changedPattern);
    connect(pItem->singleLineTextEdit_pattern, &QPlainTextEdit::textChanged, this, &dlgTriggerEditor::slot_itemEdited);
    connect(pItem->spinBox_lineSpacer, qOverload<int>(&QSpinBox::valueChanged), this, &dlgTriggerEditor::slot_lineSpacerChanged);
    connect(pItem->spinBox_lineSpacer, qOverload<int>(&QSpinBox::valueChanged), this, &dlgTriggerEditor::slot_itemEdited);

    auto* pLayout = static_cast<QVBoxLayout*>(mpWidget_triggerItems->layout());
    int insertIndex = pLayout->count() - 1;
    if (mPatternNavigationHintBanner) {
        const int hintIndex = pLayout->indexOf(mPatternNavigationHintBanner);
        if (hintIndex >= 0) {
            insertIndex = hintIndex;
        }
    }
    pLayout->insertWidget(insertIndex, pItem);

    mTriggerPatternEdit.push_back(pItem);
    pItem->mRow = index;
    pItem->pushButton_fgColor->hide();
    pItem->pushButton_bgColor->hide();
    pItem->label_prompt->hide();
    pItem->spinBox_lineSpacer->hide();
    pItem->label_patternNumber->setText(QString::number(index + 1));
    pItem->label_patternNumber->show();

    lineEditShouldMarkSpaces[pItem->singleLineTextEdit_pattern] = false;

    pItem->singleLineTextEdit_pattern->installEventFilter(this);
    applyPatternWidgetStyle(pItem);
    pItem->spinBox_lineSpacer->installEventFilter(this);
}

void dlgTriggerEditor::showPatternItems(int count)
{
    count = qBound(0, count, 50);
    while (mTriggerPatternEdit.size() < count) {
        createPatternItem(mTriggerPatternEdit.size());
    }

    for (int i = 0; i < mTriggerPatternEdit.size(); ++i) {
        auto* pItem = mTriggerPatternEdit[i];
        if (!pItem) {
            continue;
        }

        if (i < count) {
            pItem->show();

            const int currentType = pItem->comboBox_patternType->currentIndex();
            if (currentType >= 0) {
                pItem->slot_triggerTypeComboBoxChanged(currentType);
            }
        } else {
            auto* edit = pItem->singleLineTextEdit_pattern;
            edit->blockSignals(true);
            edit->clear();
            edit->blockSignals(false);
            lineEditShouldMarkSpaces[edit] = false;

            auto* combo = pItem->comboBox_patternType;
            combo->blockSignals(true);
            combo->setCurrentIndex(REGEX_SUBSTRING);
            combo->blockSignals(false);

            pItem->slot_triggerTypeComboBoxChanged(REGEX_SUBSTRING);

            pItem->pushButton_fgColor->hide();
            pItem->pushButton_bgColor->hide();
            pItem->label_prompt->hide();
            pItem->spinBox_lineSpacer->hide();
            pItem->hide();
        }
    }

    mVisiblePatternCount = count;
    updatePatternPlaceholders();
    updatePatternTabOrder();
    updatePatternNavigationHint();
}

void dlgTriggerEditor::updatePatternPlaceholders()
{
    for (int i = 0; i < mVisiblePatternCount; ++i) {
        auto* patternItem = mTriggerPatternEdit.value(i, nullptr);
        if (!patternItem) {
            continue;
        }

        auto* edit = patternItem->singleLineTextEdit_pattern;
        if (!edit) {
            continue;
        }

        if (!edit->isVisible() || !edit->toPlainText().isEmpty()) {
            edit->setPlaceholderText(QString());
            continue;
        }

        const QString placeholder = patternPlaceholderText(patternItem->comboBox_patternType->currentIndex());
        edit->setPlaceholderText(placeholder);
    }
}

QString dlgTriggerEditor::patternPlaceholderText(const int patternType) const
{
    switch (patternType) {
    case REGEX_SUBSTRING:
        return tr("Text to find (anywhere in the game output)");
    case REGEX_PERL:
        return tr("Text to find (as a regular expression pattern)");
    case REGEX_BEGIN_OF_LINE_SUBSTRING:
        return tr("Text to find (from beginning of the line)");
    case REGEX_EXACT_MATCH:
        return tr("Exact line to match");
    case REGEX_LUA_CODE:
        return tr("Lua code to run (return true to match)");
    default:
        return QString();
    }
}

void dlgTriggerEditor::setupPatternNavigationShortcuts()
{
    if (mFirstPatternShortcut) {
        mFirstPatternShortcut->deleteLater();
        mFirstPatternShortcut = nullptr;
    }

    if (mLastPatternShortcut) {
        mLastPatternShortcut->deleteLater();
        mLastPatternShortcut = nullptr;
    }

    for (auto* shortcut : mPatternNavigationShortcuts) {
        if (shortcut) {
            shortcut->deleteLater();
        }
    }
    mPatternNavigationShortcuts.clear();

    if (!mpTriggersMainArea) {
        return;
    }

    mFirstPatternShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Up), mpTriggersMainArea);
    mFirstPatternShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(mFirstPatternShortcut, &QShortcut::activated, this, [this]() {
        if (mVisiblePatternCount < 1) {
            return;
        }
        focusPatternItem(0, Qt::ShortcutFocusReason);
    });


    mLastPatternShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Down), mpTriggersMainArea);
    mLastPatternShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(mLastPatternShortcut, &QShortcut::activated, this, [this]() {
        if (mVisiblePatternCount < 1) {
            return;
        }
        focusPatternItem(mVisiblePatternCount - 1, Qt::ShortcutFocusReason);
    });

    const bool enableShortcuts = mCurrentView == EditorViewType::cmTriggerView;
    if (mFirstPatternShortcut) {
        mFirstPatternShortcut->setEnabled(enableShortcuts);
    }

    if (mLastPatternShortcut) {
        mLastPatternShortcut->setEnabled(enableShortcuts);
    }
}

void dlgTriggerEditor::updatePatternNavigationHint()
{
    if (!mPatternNavigationHintBanner || !mPatternNavigationHintLabel) {
        return;
    }

    const bool permanentlyHidden = bannerPermanentlyHidden(EditorViewType::cmTriggerView, patternNavigationBannerKey, false);
    mPatternNavigationHintBanner->setVisible(!mPatternNavigationHintHidden && !permanentlyHidden);

    constexpr int baseHorizontalPadding = 12;
    constexpr int baseVerticalPadding = 12;

    int leftMargin = 0;
    if (mpWidget_triggerItems && mPatternNavigationHintBanner->isVisible()) {
        const QPoint hintPos = mPatternNavigationHintBanner->mapTo(mpWidget_triggerItems, QPoint());
        const int itemCount = qMin(mVisiblePatternCount, mTriggerPatternEdit.size());
        for (int i = 0; i < itemCount; ++i) {
            auto* patternItem = mTriggerPatternEdit.value(i, nullptr);
            if (!patternItem || !patternItem->isVisible()) {
                continue;
            }
        }
    }

    const QString settingsKey = bannerSettingsKey(EditorViewType::cmTriggerView, patternNavigationBannerKey);
    const QString baseKey = bannerSettingsKey(EditorViewType::cmTriggerView, QString());
    if (settingsKey.isEmpty()) {
        return;
    }

    const int topMargin = qMax(baseVerticalPadding, mPatternNavigationHintLabel->fontMetrics().lineSpacing());
    if (auto* bannerLayout = qobject_cast<QHBoxLayout*>(mPatternNavigationHintBanner->layout())) {
        const QMargins currentMargins = bannerLayout->contentsMargins();
        const int desiredLeft = baseHorizontalPadding + leftMargin;
        if (currentMargins.left() != desiredLeft || currentMargins.top() != topMargin
            || currentMargins.right() != baseHorizontalPadding || currentMargins.bottom() != baseVerticalPadding) {
            bannerLayout->setContentsMargins(desiredLeft, topMargin, baseHorizontalPadding, baseVerticalPadding);
        }
    }

    mPatternNavigationHintLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    //: Hint shown below trigger patterns explaining navigation shortcuts. Contains HTML markup.
    mPatternNavigationHintLabel->setText(tr(
        "<p><strong>Navigation shortcuts</strong></p>"
        "<ul>"
        "<li>Press <strong>Ctrl+Shift+Up</strong> to focus the first pattern field.</li>"
        "<li>Press <strong>Ctrl+Shift+Down</strong> to jump to the last visible pattern field.</li>"
        "<li>Press <strong>Ctrl+Up</strong> or <strong>Ctrl+Down</strong> to move between pattern fields.</li>"
        "<li>Press <strong>Ctrl+Tab</strong> to toggle the Lua code editor.</li>"
        "</ul>"
    ));
    
}


void dlgTriggerEditor::handlePatternNavigationHintDismiss()
{
    mPatternNavigationHintHidden = true;
    if (mPatternNavigationHintBanner) {
        mPatternNavigationHintBanner->hide();
    }

    if (auto* settings = mudlet::getQSettings()) {
        const QString patternHintKey = patternNavigationHintSettingsKey();
        settings->setValue(patternHintKey, true);
    }

    showPatternNavigationHintUndoToast();
}

void dlgTriggerEditor::showPatternNavigationHintUndoToast()
{
    if (!mpSystemMessageArea) {
        return;
    }

    if (mpBannerUndoTimer) {
        mpBannerUndoTimer->stop();
        mpBannerUndoTimer->deleteLater();
    }

    mCurrentBannerKey.clear();

    mpBannerUndoTimer = new QTimer(this);
    mpBannerUndoTimer->setSingleShot(true);
    mpBannerUndoTimer->setInterval(std::chrono::seconds(5));

    //: Toast notification shown when user dismisses the trigger pattern navigation hint. Allows them to undo or permanently hide the hint.
    const QString toastMessage = tr("Hint hidden. <a href='undo' style='color: inherit; text-decoration: underline;'>Undo</a> | <a href='hide-permanently' style='color: inherit; text-decoration: underline;'>Hide permanently</a>");

    mpSystemMessageArea->notificationAreaIconLabelError->hide();
    mpSystemMessageArea->notificationAreaIconLabelWarning->hide();
    mpSystemMessageArea->notificationAreaIconLabelInformation->show();
    mpSystemMessageArea->notificationAreaMessageBox->setText(toastMessage);
    mpSystemMessageArea->show();

    connect(mpBannerUndoTimer, &QTimer::timeout, this, &dlgTriggerEditor::hideSystemMessageArea);
    mpBannerUndoTimer->start();

    disconnect(mpSystemMessageArea->notificationAreaMessageBox, &QLabel::linkActivated, nullptr, nullptr);
    connect(mpSystemMessageArea->notificationAreaMessageBox, &QLabel::linkActivated, this, [this](const QString& link) {
        if (link == QLatin1String("undo")) {
            undoPatternNavigationHintDismiss();
        } else if (link == QLatin1String("hide-permanently")) {
            handlePatternNavigationHintPermanentDismiss();
        } else {
            slot_clickedMessageBox(link);
        }
    });
}

void dlgTriggerEditor::undoPatternNavigationHintDismiss()
{
    if (mpBannerUndoTimer) {
        mpBannerUndoTimer->stop();
        mpBannerUndoTimer->deleteLater();
        mpBannerUndoTimer = nullptr;
    }

    mPatternNavigationHintHidden = false;
    if (auto* settings = mudlet::getQSettings()) {
        const QString patternHintKey = patternNavigationHintSettingsKey();
        settings->setValue(patternHintKey, false);
    }

    setBannerPermanentlyHidden(EditorViewType::cmTriggerView, patternNavigationBannerKey, false);

    updatePatternNavigationHint();
    hideSystemMessageArea();
}

void dlgTriggerEditor::handlePatternNavigationHintPermanentDismiss()
{
    if (mpBannerUndoTimer) {
        mpBannerUndoTimer->stop();
        mpBannerUndoTimer->deleteLater();
        mpBannerUndoTimer = nullptr;
    }

    mPatternNavigationHintHidden = true;
    if (auto* settings = mudlet::getQSettings()) {
        const QString patternHintKey = patternNavigationHintSettingsKey();
        settings->setValue(patternHintKey, true);
    }

    setBannerPermanentlyHidden(EditorViewType::cmTriggerView, patternNavigationBannerKey, true);

    hideSystemMessageArea();
}


void dlgTriggerEditor::slot_addPattern()
{
    showPatternItems(mVisiblePatternCount + 1);
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
    emit editorClosing();
    writeSettings();
    event->accept();
}

void dlgTriggerEditor::readSettings()
{
    QSettings& settings = *mudlet::getQSettings();

    const QSize size = settings.value("script_editor_size", QSize(600, 400)).toSize();
    resize(size);

    // Use smart positioning instead of blindly restoring saved position
    // This ensures the dialog opens on the same screen as the active profile
    utils::positionDialogOnActiveProfileScreen(this, nullptr, mpHost->mpConsole);

    mAutosaveInterval = settings.value("autosaveIntervalMinutes", 2).toInt();

    mTriggerEditorSplitterState = settings.value("mTriggerEditorSplitterState", QByteArray()).toByteArray();
    mAliasEditorSplitterState = settings.value("mAliasEditorSplitterState", QByteArray()).toByteArray();
    mScriptEditorSplitterState = settings.value("mScriptEditorSplitterState", QByteArray()).toByteArray();
    mActionEditorSplitterState = settings.value("mActionEditorSplitterState", QByteArray()).toByteArray();
    mKeyEditorSplitterState = settings.value("mKeyEditorSplitterState", QByteArray()).toByteArray();
    mTimerEditorSplitterState = settings.value("mTimerEditorSplitterState", QByteArray()).toByteArray();
    mVarEditorSplitterState = settings.value("mVarEditorSplitterState", QByteArray()).toByteArray();
    mSearchSplitterState = settings.value("mSearchSplitterState", QByteArray()).toByteArray();

    const QString patternHintKey = patternNavigationHintSettingsKey();
    const QString legacyPatternHintKey = qsl("patternNavigationHintHidden");
    if (!patternHintKey.isEmpty() && patternHintKey != legacyPatternHintKey && settings.contains(legacyPatternHintKey)) {
        settings.remove(legacyPatternHintKey);
    }

    mPatternNavigationHintHidden = settings.value(patternHintKey, false).toBool();

    const bool permanentlyHidden = bannerPermanentlyHidden(EditorViewType::cmTriggerView, patternNavigationBannerKey, false);
    if (mPatternNavigationHintHidden && !permanentlyHidden) {
        mPatternNavigationHintHidden = false;
        settings.setValue(patternHintKey, false);
        updatePatternNavigationHint();
    }
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
    settings.setValue("mSearchSplitterState", mSearchSplitterState);

    const QString patternHintKey = patternNavigationHintSettingsKey();
    settings.setValue(patternHintKey, mPatternNavigationHintHidden);
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
        foundItemsList = treeWidget_triggers->findItems(pItem->data(0, NameRole).toString(), Qt::MatchCaseSensitive | Qt::MatchFixedString| Qt::MatchRecursive, 0);

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
                    controller->moveCaretTo(static_cast<size_t>(pItem->data(0, PatternOrLineRole).toInt()), static_cast<size_t>(pItem->data(0, PositionRole).toInt()), false);
                    break;
                case SearchResultIsName:
                    mpTriggersMainArea->lineEdit_trigger_name->setFocus(Qt::OtherFocusReason);
                    mpTriggersMainArea->lineEdit_trigger_name->setCursorPosition(pItem->data(0, PositionRole).toInt());
                    break;
                case SearchResultIsPattern: {
                    dlgTriggerPatternEdit * pTriggerPattern = mTriggerPatternEdit.at(pItem->data(0, PatternOrLineRole).toInt());
                    mpScrollArea->ensureWidgetVisible(pTriggerPattern);
                    if (pTriggerPattern->singleLineTextEdit_pattern->isVisible()) {
                        // If is a colour trigger the singleLineTextEdit_pattern is not shown
                        pTriggerPattern->singleLineTextEdit_pattern->setFocus();
                        pTriggerPattern->singleLineTextEdit_pattern->textCursor().setPosition(pItem->data(0, PositionRole).toInt());

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
                    controller->moveCaretTo(static_cast<size_t>(pItem->data(0, PatternOrLineRole).toInt()), static_cast<size_t>(pItem->data(0, PositionRole).toInt()), false);
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
                    controller->moveCaretTo(static_cast<size_t>(pItem->data(0, PatternOrLineRole).toInt()), static_cast<size_t>(pItem->data(0, PositionRole).toInt()), false);
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
                    controller->moveCaretTo(static_cast<size_t>(pItem->data(0, PatternOrLineRole).toInt()), static_cast<size_t>(pItem->data(0, PositionRole).toInt()), false);
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
                    controller->moveCaretTo(static_cast<size_t>(pItem->data(0, PatternOrLineRole).toInt()), static_cast<size_t>(pItem->data(0, PositionRole).toInt()), false);
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
                    controller->moveCaretTo(static_cast<size_t>(pItem->data(0, PatternOrLineRole).toInt()), static_cast<size_t>(pItem->data(0, PositionRole).toInt()), false);
                    break;
                case SearchResultIsName:
                    mpTriggersMainArea->lineEdit_trigger_name->setFocus(Qt::OtherFocusReason);
                    mpTriggersMainArea->lineEdit_trigger_name->setCursorPosition(pItem->data(0, PositionRole).toInt());
                    break;
                case SearchResultIsPattern: {
                    dlgTriggerPatternEdit * pTriggerPattern = mTriggerPatternEdit.at(pItem->data(0, PatternOrLineRole).toInt());
                    mpScrollArea->ensureWidgetVisible(pTriggerPattern);
                    if (pTriggerPattern->singleLineTextEdit_pattern->isVisible()) {
                        // If is a colour trigger the singleLineTextEdit_pattern is not shown
                        pTriggerPattern->singleLineTextEdit_pattern->setFocus();
                        pTriggerPattern->singleLineTextEdit_pattern->textCursor().setPosition(pItem->data(0, PositionRole).toInt());
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
                    controller->moveCaretTo(static_cast<size_t>(pItem->data(0, PatternOrLineRole).toInt()), static_cast<size_t>(pItem->data(0, PositionRole).toInt()), false);
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
            if (textList.at(index).isEmpty() ||
               !textList.at(index).contains(text, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive))) {
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

bool dlgTriggerEditor::showDeleteConfirmation(const QString& title, const QString& message)
{
    QSettings& settings = *mudlet::getQSettings();
    const bool dontAskAgain = settings.value("triggerEditor/dontAskDeleteConfirmation", false).toBool();

    if (dontAskAgain) {
        return true;
    }

    QMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.setIcon(QMessageBox::Question);

    QCheckBox* dontAskCheckBox = new QCheckBox(tr("Don't ask again"));
    msgBox.setCheckBox(dontAskCheckBox);

    int result = msgBox.exec();

    if (dontAskCheckBox->isChecked()) {
        settings.setValue("triggerEditor/dontAskDeleteConfirmation", true);
    }

    return result == QMessageBox::Yes;
}

void dlgTriggerEditor::delete_alias()
{
    QList<QTreeWidgetItem*> selectedItems = treeWidget_aliases->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    QStringList itemNames;
    QList<TAlias*> aliasesToDelete;

    for (QTreeWidgetItem* pItem : selectedItems) {
        TAlias* pT = mpHost->getAliasUnit()->getAlias(pItem->data(0, Qt::UserRole).toInt());
        if (pT) {
            itemNames << pT->getName();
            aliasesToDelete << pT;
        }
    }

    if (aliasesToDelete.isEmpty()) {
        return;
    }

    // Show confirmation dialog for multiple items
    QString message;
    if (aliasesToDelete.size() == 1) {
        message = tr("Do you really want to delete alias \"%1\"?").arg(itemNames.first());
    } else {
        message = tr("Do you really want to delete %1 aliases?\n\nItems to be deleted:\n%2")
                    .arg(aliasesToDelete.size())
                    .arg(itemNames.join(", "));
    }

    if (!showDeleteConfirmation(tr("Delete Alias(es)"), message)) {
        return;
    }

    // Sort items by their position in tree (top to bottom) to delete correctly
    std::sort(selectedItems.begin(), selectedItems.end(), [this](QTreeWidgetItem* a, QTreeWidgetItem* b) {
        QModelIndex indexA = treeWidget_aliases->indexFromItem(a);
        QModelIndex indexB = treeWidget_aliases->indexFromItem(b);
        return indexA.row() < indexB.row();
    });

    // Delete in reverse order to maintain valid indices
    std::reverse(selectedItems.begin(), selectedItems.end());

    QTreeWidgetItem* newSelection = nullptr;
    for (QTreeWidgetItem* pItem : selectedItems) {
        QTreeWidgetItem* pParentItem = pItem->parent();
        TAlias* pT = mpHost->getAliasUnit()->getAlias(pItem->data(0, Qt::UserRole).toInt());

        if (pT) {
            if (pParentItem && !newSelection) {
                newSelection = pParentItem;
            }
            if (pParentItem) {
                pParentItem->removeChild(pItem);
            }
            delete pT;
        }
    }

    // Set new selection
    if (newSelection) {
        mpCurrentAliasItem = newSelection;
        treeWidget_aliases->setCurrentItem(newSelection);
        slot_aliasSelected(newSelection);
    } else {
        mpCurrentAliasItem = nullptr;
        clearAliasForm();
    }
}

void dlgTriggerEditor::delete_action()
{
    QList<QTreeWidgetItem*> selectedItems = treeWidget_actions->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    QStringList itemNames;
    QList<TAction*> actionsToDelete;

    for (QTreeWidgetItem* pItem : selectedItems) {
        TAction* pT = mpHost->getActionUnit()->getAction(pItem->data(0, Qt::UserRole).toInt());
        if (pT) {
            itemNames << pT->getName();
            actionsToDelete << pT;
        }
    }

    if (actionsToDelete.isEmpty()) {
        return;
    }

    // Show confirmation dialog for multiple items
    QString message;
    if (actionsToDelete.size() == 1) {
        message = tr("Do you really want to delete button \"%1\"?").arg(itemNames.first());
    } else {
        message = tr("Do you really want to delete %1 buttons?\n\nItems to be deleted:\n%2")
                    .arg(actionsToDelete.size())
                    .arg(itemNames.join(", "));
    }

    if (!showDeleteConfirmation(tr("Delete Button(s)"), message)) {
        return;
    }

    // Sort items by their position in tree (top to bottom) to delete correctly
    std::sort(selectedItems.begin(), selectedItems.end(), [this](QTreeWidgetItem* a, QTreeWidgetItem* b) {
        QModelIndex indexA = treeWidget_actions->indexFromItem(a);
        QModelIndex indexB = treeWidget_actions->indexFromItem(b);
        return indexA.row() < indexB.row();
    });

    // Delete in reverse order to maintain valid indices
    std::reverse(selectedItems.begin(), selectedItems.end());

    QTreeWidgetItem* newSelection = nullptr;
    for (QTreeWidgetItem* pItem : selectedItems) {
        QTreeWidgetItem* pParentItem = pItem->parent();
        TAction* pT = mpHost->getActionUnit()->getAction(pItem->data(0, Qt::UserRole).toInt());

        if (pT) {
            // if active, deactivate.
            if (pT->isActive()) {
                pT->deactivate();
            }
            // set this and the parent TActions as changed so the toolbar is updated.
            pT->setDataChanged();

            if (pParentItem && !newSelection) {
                newSelection = pParentItem;
            }
            if (pParentItem) {
                pParentItem->removeChild(pItem);
            }
            delete pT;
        }
    }

    // Set new selection
    if (newSelection) {
        mpCurrentActionItem = newSelection;
        treeWidget_actions->setCurrentItem(newSelection);
        slot_actionSelected(newSelection);
    } else {
        mpCurrentActionItem = nullptr;
        clearActionForm();
    }

    mpHost->getActionUnit()->updateToolbar();
}

void dlgTriggerEditor::delete_variable()
{
    QList<QTreeWidgetItem*> selectedItems = treeWidget_variables->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    QStringList itemNames;
    QList<TVar*> varsToDelete;
    LuaInterface* lI = mpHost->getLuaInterface();
    VarUnit* vu = lI->getVarUnit();

    for (QTreeWidgetItem* pItem : selectedItems) {
        TVar* var = vu->getWVar(pItem);
        if (var) {
            itemNames << var->getName();
            varsToDelete << var;
        }
    }

    if (varsToDelete.isEmpty()) {
        return;
    }

    // Show confirmation dialog for multiple items
    QString message;
    if (varsToDelete.size() == 1) {
        message = tr("Do you really want to delete variable \"%1\"?").arg(itemNames.first());
    } else {
        message = tr("Do you really want to delete %1 variables?\n\nItems to be deleted:\n%2")
                    .arg(varsToDelete.size())
                    .arg(itemNames.join(", "));
    }

    if (!showDeleteConfirmation(tr("Delete Variable(s)"), message)) {
        return;
    }

    // Sort items by their position in tree (top to bottom) to delete correctly
    std::sort(selectedItems.begin(), selectedItems.end(), [this](QTreeWidgetItem* a, QTreeWidgetItem* b) {
        QModelIndex indexA = treeWidget_variables->indexFromItem(a);
        QModelIndex indexB = treeWidget_variables->indexFromItem(b);
        return indexA.row() < indexB.row();
    });

    // Delete in reverse order to maintain valid indices
    std::reverse(selectedItems.begin(), selectedItems.end());

    QTreeWidgetItem* newSelection = nullptr;
    for (QTreeWidgetItem* pItem : selectedItems) {
        QTreeWidgetItem* pParentItem = pItem->parent();
        TVar* var = vu->getWVar(pItem);

        if (var) {
            lI->deleteVar(var);
            TVar* parent = var->getParent();
            if (parent) {
                parent->removeChild(var);
            }
            vu->removeVariable(var);

            if (pParentItem && !newSelection) {
                newSelection = pParentItem;
            }
            if (pParentItem) {
                pParentItem->removeChild(pItem);
            }
            delete var;
        }
    }

    // Set new selection
    if (newSelection) {
        mpCurrentVarItem = newSelection;
        treeWidget_variables->setCurrentItem(newSelection);
        slot_variableSelected(newSelection);
    } else {
        mpCurrentVarItem = nullptr;
        clearVarForm();
    }
}

void dlgTriggerEditor::delete_script()
{
    QList<QTreeWidgetItem*> selectedItems = treeWidget_scripts->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    QStringList itemNames;
    QList<TScript*> scriptsToDelete;

    for (QTreeWidgetItem* pItem : selectedItems) {
        TScript* pT = mpHost->getScriptUnit()->getScript(pItem->data(0, Qt::UserRole).toInt());
        if (pT) {
            itemNames << pT->getName();
            scriptsToDelete << pT;
        }
    }

    if (scriptsToDelete.isEmpty()) {
        return;
    }

    // Show confirmation dialog for multiple items
    QString message;
    if (scriptsToDelete.size() == 1) {
        message = tr("Do you really want to delete script \"%1\"?").arg(itemNames.first());
    } else {
        message = tr("Do you really want to delete %1 scripts?\n\nItems to be deleted:\n%2")
                    .arg(scriptsToDelete.size())
                    .arg(itemNames.join(", "));
    }

    if (!showDeleteConfirmation(tr("Delete Script(s)"), message)) {
        return;
    }

    // Sort items by their position in tree (top to bottom) to delete correctly
    std::sort(selectedItems.begin(), selectedItems.end(), [this](QTreeWidgetItem* a, QTreeWidgetItem* b) {
        QModelIndex indexA = treeWidget_scripts->indexFromItem(a);
        QModelIndex indexB = treeWidget_scripts->indexFromItem(b);
        return indexA.row() < indexB.row();
    });

    // Delete in reverse order to maintain valid indices
    std::reverse(selectedItems.begin(), selectedItems.end());

    QTreeWidgetItem* newSelection = nullptr;
    for (QTreeWidgetItem* pItem : selectedItems) {
        QTreeWidgetItem* pParentItem = pItem->parent();
        TScript* pT = mpHost->getScriptUnit()->getScript(pItem->data(0, Qt::UserRole).toInt());

        if (pT) {
            if (pParentItem && !newSelection) {
                newSelection = pParentItem;
            }
            if (pParentItem) {
                pParentItem->removeChild(pItem);
            }
            delete pT;
        }
    }

    // Set new selection
    if (newSelection) {
        mpCurrentScriptItem = newSelection;
        treeWidget_scripts->setCurrentItem(newSelection);
        slot_scriptsSelected(newSelection);
    } else {
        mpCurrentScriptItem = nullptr;
        clearScriptForm();
    }
}

void dlgTriggerEditor::delete_key()
{
    QList<QTreeWidgetItem*> selectedItems = treeWidget_keys->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    QStringList itemNames;
    QList<TKey*> keysToDelete;

    for (QTreeWidgetItem* pItem : selectedItems) {
        TKey* pT = mpHost->getKeyUnit()->getKey(pItem->data(0, Qt::UserRole).toInt());
        if (pT) {
            itemNames << pT->getName();
            keysToDelete << pT;
        }
    }

    if (keysToDelete.isEmpty()) {
        return;
    }

    // Show confirmation dialog for multiple items
    QString message;
    if (keysToDelete.size() == 1) {
        message = tr("Do you really want to delete key \"%1\"?").arg(itemNames.first());
    } else {
        message = tr("Do you really want to delete %1 keys?\n\nItems to be deleted:\n%2")
                    .arg(keysToDelete.size())
                    .arg(itemNames.join(", "));
    }

    if (!showDeleteConfirmation(tr("Delete Key(s)"), message)) {
        return;
    }

    // Sort items by their position in tree (top to bottom) to delete correctly
    std::sort(selectedItems.begin(), selectedItems.end(), [this](QTreeWidgetItem* a, QTreeWidgetItem* b) {
        QModelIndex indexA = treeWidget_keys->indexFromItem(a);
        QModelIndex indexB = treeWidget_keys->indexFromItem(b);
        return indexA.row() < indexB.row();
    });

    // Delete in reverse order to maintain valid indices
    std::reverse(selectedItems.begin(), selectedItems.end());

    QTreeWidgetItem* newSelection = nullptr;
    for (QTreeWidgetItem* pItem : selectedItems) {
        QTreeWidgetItem* pParentItem = pItem->parent();
        TKey* pT = mpHost->getKeyUnit()->getKey(pItem->data(0, Qt::UserRole).toInt());

        if (pT) {
            if (pParentItem && !newSelection) {
                newSelection = pParentItem;
            }
            if (pParentItem) {
                pParentItem->removeChild(pItem);
            }
            delete pT;
        }
    }

    // Set new selection
    if (newSelection) {
        mpCurrentKeyItem = newSelection;
        treeWidget_keys->setCurrentItem(newSelection);
        slot_keySelected(newSelection);
    } else {
        mpCurrentKeyItem = nullptr;
        clearKeyForm();
    }
}

void dlgTriggerEditor::delete_trigger()
{
    QList<QTreeWidgetItem*> selectedItems = treeWidget_triggers->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    QStringList itemNames;
    QList<TTrigger*> triggersToDelete;

    for (QTreeWidgetItem* pItem : selectedItems) {
        TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(pItem->data(0, Qt::UserRole).toInt());
        if (pT) {
            itemNames << pT->getName();
            triggersToDelete << pT;
        }
    }

    if (triggersToDelete.isEmpty()) {
        return;
    }

    // Show confirmation dialog for multiple items
    QString message;
    if (triggersToDelete.size() == 1) {
        message = tr("Do you really want to delete trigger \"%1\"?").arg(itemNames.first());
    } else {
        message = tr("Do you really want to delete %1 triggers?\n\nItems to be deleted:\n%2")
                    .arg(triggersToDelete.size())
                    .arg(itemNames.join(", "));
    }

    if (!showDeleteConfirmation(tr("Delete Trigger(s)"), message)) {
        return;
    }

    // Sort items by their position in tree (top to bottom) to delete correctly
    std::sort(selectedItems.begin(), selectedItems.end(), [this](QTreeWidgetItem* a, QTreeWidgetItem* b) {
        QModelIndex indexA = treeWidget_triggers->indexFromItem(a);
        QModelIndex indexB = treeWidget_triggers->indexFromItem(b);
        return indexA.row() < indexB.row();
    });

    // Delete in reverse order to maintain valid indices
    std::reverse(selectedItems.begin(), selectedItems.end());

    QTreeWidgetItem* newSelection = nullptr;
    for (QTreeWidgetItem* pItem : selectedItems) {
        QTreeWidgetItem* pParentItem = pItem->parent();
        TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(pItem->data(0, Qt::UserRole).toInt());

        if (pT) {
            if (pParentItem && !newSelection) {
                newSelection = pParentItem;
            }
            if (pParentItem) {
                pParentItem->removeChild(pItem);
            }
            delete pT;
        }
    }

    // Set new selection
    if (newSelection) {
        mpCurrentTriggerItem = newSelection;
        treeWidget_triggers->setCurrentItem(newSelection);
        slot_triggerSelected(newSelection);
    } else {
        mpCurrentTriggerItem = nullptr;
        clearTriggerForm();
    }
}

void dlgTriggerEditor::delete_timer()
{
    QList<QTreeWidgetItem*> selectedItems = treeWidget_timers->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    QStringList itemNames;
    QList<TTimer*> timersToDelete;

    for (QTreeWidgetItem* pItem : selectedItems) {
        TTimer* pT = mpHost->getTimerUnit()->getTimer(pItem->data(0, Qt::UserRole).toInt());
        if (pT) {
            itemNames << pT->getName();
            timersToDelete << pT;
        }
    }

    if (timersToDelete.isEmpty()) {
        return;
    }

    // Show confirmation dialog for multiple items
    QString message;
    if (timersToDelete.size() == 1) {
        message = tr("Do you really want to delete timer \"%1\"?").arg(itemNames.first());
    } else {
        message = tr("Do you really want to delete %1 timers?\n\nItems to be deleted:\n%2")
                    .arg(timersToDelete.size())
                    .arg(itemNames.join(", "));
    }

    if (!showDeleteConfirmation(tr("Delete Timer(s)"), message)) {
        return;
    }

    // Sort items by their position in tree (top to bottom) to delete correctly
    std::sort(selectedItems.begin(), selectedItems.end(), [this](QTreeWidgetItem* a, QTreeWidgetItem* b) {
        QModelIndex indexA = treeWidget_timers->indexFromItem(a);
        QModelIndex indexB = treeWidget_timers->indexFromItem(b);
        return indexA.row() < indexB.row();
    });

    // Delete in reverse order to maintain valid indices
    std::reverse(selectedItems.begin(), selectedItems.end());

    QTreeWidgetItem* newSelection = nullptr;
    for (QTreeWidgetItem* pItem : selectedItems) {
        QTreeWidgetItem* pParentItem = pItem->parent();
        TTimer* pT = mpHost->getTimerUnit()->getTimer(pItem->data(0, Qt::UserRole).toInt());

        if (pT) {
            if (pParentItem && !newSelection) {
                newSelection = pParentItem;
            }
            if (pParentItem) {
                pParentItem->removeChild(pItem);
            }
            delete pT;
        }
    }

    // Set new selection
    if (newSelection) {
        mpCurrentTimerItem = newSelection;
        treeWidget_timers->setCurrentItem(newSelection);
        slot_timerSelected(newSelection);
    } else {
        mpCurrentTimerItem = nullptr;
        clearTimerForm();
    }
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

    if (!pT->state()) {
        pT->setIsActive(false);
        showError(tr(R"(<p>Unable to activate "<tt>%1</tt>": %2</p>
                     <p><i>You will need to reactivate this after the problem has been corrected.</i></p>)").arg(pT->getName().toHtmlEscaped(), pT->getError()));
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

    if (!pT->state()) {
        pT->setIsActive(false);
        showError(tr(R"(<p><b>Unable to activate "<tt>%1</tt>": %2.</b></p>
                     <p><i>You will need to reactivate this after the problem has been corrected.</i></p>)").arg(pT->getName().toHtmlEscaped(), pT->getError()));
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

    if (!pT->state()) {
        pT->setIsActive(false);
        showError(tr(R"(<p><b>Unable to activate "<tt>%1</tt>"; %2.</b></p>
                     <p><i>You will need to reactivate this after the problem has been corrected.</i></p>)").arg(pT->getName().toHtmlEscaped(), pT->getError()));
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

    if (!pT->state()) {
        pT->setIsActive(false);
        showError(tr(R"(<p><b>Unable to activate "<tt>%1</tt>"; %2.</b></p>
                     <p><i>You will need to reactivate this after the problem has been corrected.</i></p>)").arg(pT->getName().toHtmlEscaped(), pT->getError()));
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

    if (!pT->state()) {
        pT->setIsActive(false);
        showError(tr(R"(<p><b>Unable to activate "<tt>%1</tt>"; %2.</b></p>
                     <p><i>You will need to reactivate this after the problem has been corrected.</i></p>)").arg(pT->getName().toHtmlEscaped(), pT->getError()));
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

    if (!pT->state()) {
        pT->setIsActive(false);
        showError(tr(R"(<p><b>Unable to activate "<tt>%1</tt>"; %2.</b></p>
                     <p><i>You will need to reactivate this after the problem has been corrected.</i></p>)").arg(pT->getName().toHtmlEscaped(), pT->getError()));
        icon.addPixmap(QPixmap(qsl(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
        itemDescription = descError;
    }

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

    QString name = isFolder ? tr("New trigger group") : tr("New trigger");
    QStringList nameList { name };
    const QStringList patterns;
    QList<int> const patternKinds;
    const QString script = "";

    QTreeWidgetItem* pParentItem = treeWidget_triggers->currentItem();
    QTreeWidgetItem* pNewItem = nullptr;
    TTrigger* pNewTrigger = nullptr;

    if (pParentItem) {
        const int parentID = pParentItem->data(0, Qt::UserRole).toInt();
        TTrigger* pParentTrigger = mpHost->getTriggerUnit()->getTrigger(parentID);

        if (pParentTrigger) {
            // insert new items as siblings unless the parent is a folder
            if (pParentTrigger->isFolder()) {
                pNewTrigger = new TTrigger(pParentTrigger, mpHost);
                pNewItem = new QTreeWidgetItem(pParentItem, nameList);
                pParentItem->insertChild(0, pNewItem);
            } else if (pParentTrigger->getParent() && pParentItem->parent()) {
                pNewTrigger = new TTrigger(pParentTrigger->getParent(), mpHost);
                pNewItem = new QTreeWidgetItem(pParentItem->parent(), nameList);
                pParentItem->parent()->insertChild(0, pNewItem);
            }
        }
    }

    if (!pNewTrigger) {
        // Fallback to insert a new root item
        pNewTrigger = new TTrigger(name, patterns, patternKinds, false, mpHost);
        pNewItem = new QTreeWidgetItem(mpTriggerBaseItem, nameList);
        treeWidget_triggers->insertTopLevelItem(0, pNewItem);
    }


    if (!pNewTrigger) {
        return;
    }

    // Initialize logic object properties
    pNewTrigger->setName(name);
    pNewTrigger->setRegexCodeList(patterns, patternKinds, false);
    pNewTrigger->setScript(script);
    pNewTrigger->setIsFolder(isFolder);
    pNewTrigger->setIsActive(false);
    pNewTrigger->setIsMultiline(false);
    pNewTrigger->mStayOpen = 0;
    pNewTrigger->setConditionLineDelta(0);
    pNewTrigger->registerTrigger();

    // Initialize tree item properties
    pNewItem->setData(0, Qt::UserRole, pNewTrigger->getID());
    pNewItem->setIcon(0, QIcon(QPixmap(isFolder ?
        qsl(":/icons/folder-red.png") :
        qsl(":/icons/document-save-as.png"))));
    pNewItem->setData(0, Qt::AccessibleDescriptionRole, isFolder ? descNewFolder : descNewItem);

    // Expand parent if applicable
    if (pParentItem) {
        pParentItem->setExpanded(true);
    }

    // Reset UI
    mpTriggersMainArea->lineEdit_trigger_name->clear();
    mpTriggersMainArea->label_idNumber->clear();
    mpTriggersMainArea->checkBox_perlSlashGOption->setChecked(false);
    clearDocument(mpSourceEditorEdbee); // New Trigger
    mpTriggersMainArea->lineEdit_trigger_command->clear();
    mpTriggersMainArea->checkBox_filterTrigger->setChecked(false);
    mpTriggersMainArea->spinBox_stayOpen->setValue(0);
    mpTriggersMainArea->spinBox_lineMargin->setValue(-1);
    mpTriggersMainArea->pushButtonFgColor->setChecked(false);
    mpTriggersMainArea->pushButtonBgColor->setChecked(false);
    mpTriggersMainArea->groupBox_triggerColorizer->setChecked(false);

    // Finalize selection
    mpCurrentTriggerItem = pNewItem;
    treeWidget_triggers->setCurrentItem(pNewItem);
    slot_triggerSelected(treeWidget_triggers->currentItem());
}


void dlgTriggerEditor::addTimer(bool isFolder)
{
    saveTimer();

    QString name = isFolder ? tr("New timer group") : tr("New timer");
    QStringList nameList = { name };
    const QString command = "";
    const QTime time;
    const QString script = "";

    QTreeWidgetItem* pParentItem = treeWidget_timers->currentItem();
    QTreeWidgetItem* pNewItem = nullptr;
    TTimer* pNewTimer = nullptr;

    if (pParentItem) {
        const int parentID = pParentItem->data(0, Qt::UserRole).toInt();
        TTimer* pParentTrigger = mpHost->getTimerUnit()->getTimer(parentID);

        if (pParentTrigger) {
            // insert new items as siblings unless the parent is a folder
            if (pParentTrigger->isFolder()) {
                pNewTimer = new TTimer(pParentTrigger, mpHost);
                pNewItem = new QTreeWidgetItem(pParentItem, nameList);
                pParentItem->insertChild(0, pNewItem);
            } else if (pParentTrigger->getParent() && pParentItem->parent()) {
                pNewTimer = new TTimer(pParentTrigger->getParent(), mpHost);
                pNewItem = new QTreeWidgetItem(pParentItem->parent(), nameList);
                pParentItem->parent()->insertChild(0, pNewItem);
            }
        }
    }

    if (!pNewTimer) {
        // Fallback to insert a new root item
        pNewTimer = new TTimer(name, time, mpHost);
        pNewItem = new QTreeWidgetItem(mpTimerBaseItem, nameList);
        treeWidget_timers->insertTopLevelItem(0, pNewItem);
    }

    if (!pNewTimer) {
        return;
    }

    // Initialize logic object properties
    pNewTimer->setName(name);
    pNewTimer->setCommand(command);
    pNewTimer->setScript(script);
    pNewTimer->setIsFolder(isFolder);
    pNewTimer->setIsActive(false);
    mpHost->getTimerUnit()->registerTimer(pNewTimer);

    // Initialize tree item properties
    pNewItem->setData(0, Qt::UserRole, pNewTimer->getID());
    pNewItem->setIcon(0, QIcon(QPixmap(isFolder ?
        qsl(":/icons/folder-red.png") :
        qsl(":/icons/document-save-as.png"))));
    pNewItem->setData(0, Qt::AccessibleDescriptionRole, isFolder ? descNewFolder : descNewItem);

    // Expand parent if applicable
    if (pParentItem) {
        pParentItem->setExpanded(true);
    }

    // Reset UI
    //FIXME
    //mpOptionsAreaTriggers->lineEdit_trigger_name->clear();
    mpTimersMainArea->lineEdit_timer_command->clear();
    clearDocument(mpSourceEditorEdbee); // New Timer

    // Finalize selection
    mpCurrentTimerItem = pNewItem;
    treeWidget_timers->setCurrentItem(pNewItem);
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
        clearDocument(mpSourceEditorEdbee, QLatin1String("NewTable"));
    } else {
        // in lieu of readonly
        mpSourceEditorEdbee->setEnabled(true);
        mpVarsMainArea->comboBox_variable_value_type->setDisabled(false);
        mpVarsMainArea->comboBox_variable_value_type->setCurrentIndex(0);
    }

    LuaInterface* lI = mpHost->getLuaInterface();
    VarUnit* vu = lI->getVarUnit();

    QStringList nameList = { QString(isFolder ? tr("table_variable") : tr("variable_name")) };
    mpVarsMainArea->lineEdit_var_name->setText(nameList[0]);
    QTreeWidgetItem* pParentItem = nullptr;
    QTreeWidgetItem* pNewItem;
    QTreeWidgetItem* cItem = treeWidget_variables->currentItem();
    if (cItem) {
        TVar* cVar = vu->getWVar(cItem);
        if (cVar && cVar->getValueType() == LUA_TTABLE) {
            pParentItem = cItem;
        } else {
            pParentItem = cItem->parent();
        }
    }

    auto newVar = new TVar();
    if (pParentItem) {
        //we're nested under something, or going to be.  This HAS to be a table
        TVar* parent = vu->getWVar(pParentItem);
        if (parent && parent->getValueType() == LUA_TTABLE) {
            //create it under the parent
            pNewItem = new QTreeWidgetItem(pParentItem, nameList);
            newVar->setParent(parent);
        } else {
            pNewItem = new QTreeWidgetItem(mpVarBaseItem, nameList);
            newVar->setParent(vu->getBase());
        }
    } else {
        pNewItem = new QTreeWidgetItem(mpVarBaseItem, nameList);
        newVar->setParent(vu->getBase());
    }

    if (isFolder) {
        newVar->setValue(QString(), LUA_TTABLE);
    } else {
        newVar->setValueType(LUA_TNONE);
    }
    vu->addTempVar(pNewItem, newVar);
    pNewItem->setFlags(pNewItem->flags() & ~(Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled));

    // Finalize selection
    mpCurrentVarItem = pNewItem;
    treeWidget_variables->setCurrentItem(pNewItem);
    slot_variableSelected(treeWidget_variables->currentItem());
    saveVar();
}

void dlgTriggerEditor::addKey(bool isFolder)
{
    saveKey();

    QString name = isFolder? tr("New key group") : tr("New key");
    QStringList nameList = { name };
    const QString script = "";

    QTreeWidgetItem* pParentItem = treeWidget_keys->currentItem();
    QTreeWidgetItem* pNewItem = nullptr;
    TKey* pNewKey = nullptr;

    if (pParentItem) {
        const int parentID = pParentItem->data(0, Qt::UserRole).toInt();
        TKey* pParentTrigger = mpHost->getKeyUnit()->getKey(parentID);

        if (pParentTrigger) {
            // insert new items as siblings unless the parent is a folder
            if (pParentTrigger->isFolder()) {
                pNewKey = new TKey(pParentTrigger, mpHost);
                pNewItem = new QTreeWidgetItem(pParentItem, nameList);
                pParentItem->insertChild(0, pNewItem);
            } else if (pParentTrigger->getParent() && pParentItem->parent()) {
                pNewKey = new TKey(pParentTrigger->getParent(), mpHost);
                pNewItem = new QTreeWidgetItem(pParentItem->parent(), nameList);
                pParentItem->parent()->insertChild(0, pNewItem);
            }
        }
    }

    if (!pNewKey) {
        // Fallback to insert a new root item
        pNewKey = new TKey(name, mpHost);
        pNewItem = new QTreeWidgetItem(mpKeyBaseItem, nameList);
        treeWidget_keys->insertTopLevelItem(0, pNewItem);
    }

    if (!pNewKey) {
        return;
    }

    // Initialize logic object properties
    pNewKey->setName(name);
    pNewKey->setKeyCode(Qt::Key_unknown);
    pNewKey->setKeyModifiers(Qt::NoModifier);
    pNewKey->setScript(script);
    pNewKey->setIsFolder(isFolder);
    pNewKey->setIsActive(false);
    pNewKey->registerKey();

    // Initialize tree item properties
    pNewItem->setData(0, Qt::UserRole, pNewKey->getID());
    pNewItem->setIcon(0, QIcon(QPixmap(isFolder ?
        qsl(":/icons/folder-red.png") :
        qsl(":/icons/document-save-as.png"))));
    pNewItem->setData(0, Qt::AccessibleDescriptionRole, isFolder ? descNewFolder : descNewItem);

    // Expand parent if applicable
    if (pParentItem) {
        pParentItem->setExpanded(true);
    }

    // Reset UI
    mpKeysMainArea->lineEdit_key_command->clear();
    mpKeysMainArea->lineEdit_key_binding->setText("no key chosen");
    clearDocument(mpSourceEditorEdbee); // New Key

    // Finalize selection
    mpCurrentKeyItem = pNewItem;
    treeWidget_keys->setCurrentItem(pNewItem);
    slot_keySelected(treeWidget_keys->currentItem());
}


void dlgTriggerEditor::addAlias(bool isFolder)
{
    saveAlias();

    QString name = isFolder ? tr("New alias group") : tr("New alias");
    QStringList nameList = { name };
    const QString regex = "";
    const QString command = "";
    const QString script = "";

    QTreeWidgetItem* pParentItem = treeWidget_aliases->currentItem();
    QTreeWidgetItem* pNewItem = nullptr;
    TAlias* pNewAlias = nullptr;

    if (pParentItem) {
        const int parentID = pParentItem->data(0, Qt::UserRole).toInt();
        TAlias* pParentTrigger = mpHost->getAliasUnit()->getAlias(parentID);

        if (pParentTrigger) {
            // insert new items as siblings unless the parent is a folder
            if (pParentTrigger->isFolder()) {
                pNewAlias = new TAlias(pParentTrigger, mpHost);
                pNewItem = new QTreeWidgetItem(pParentItem, nameList);
                pParentItem->insertChild(0, pNewItem);
            } else if (pParentTrigger->getParent() && pParentItem->parent()) {
                pNewAlias = new TAlias(pParentTrigger->getParent(), mpHost);
                pNewItem = new QTreeWidgetItem(pParentItem->parent(), nameList);
                pParentItem->parent()->insertChild(0, pNewItem);
            }
        }
    }

    if (!pNewAlias) {
        //insert a new root item
        pNewAlias = new TAlias(name, mpHost);
        pNewAlias->setRegexCode(regex); // Empty regex will always succeed to compile
        pNewItem = new QTreeWidgetItem(mpAliasBaseItem, nameList);
        treeWidget_aliases->insertTopLevelItem(0, pNewItem);
    }

    if (!pNewAlias) {
        return;
    }

    // Initialize logic object properties
    pNewAlias->setName(name);
    pNewAlias->setCommand(command);
    pNewAlias->setRegexCode(regex); // Empty regex will always succeed to compile
    pNewAlias->setScript(script);
    pNewAlias->setIsFolder(isFolder);
    pNewAlias->setIsActive(false);
    pNewAlias->registerAlias();

    // Initialize tree item properties
    pNewItem->setData(0, Qt::UserRole, pNewAlias->getID());
    pNewItem->setIcon(0, QIcon(QPixmap(isFolder ?
        qsl(":/icons/folder-red.png") :
        qsl(":/icons/document-save-as.png"))));
    pNewItem->setData(0, Qt::AccessibleDescriptionRole, isFolder ? descNewFolder : descNewItem);

    // Expand parent if applicable
    if (pParentItem) {
        pParentItem->setExpanded(true);
    }

    // Reset UI
    mpAliasMainArea->lineEdit_alias_name->clear();
    mpAliasMainArea->label_idNumber->clear();
    mpAliasMainArea->lineEdit_alias_pattern->clear();
    mpAliasMainArea->lineEdit_alias_command->clear();
    clearDocument(mpSourceEditorEdbee); // New Alias
    mpAliasMainArea->lineEdit_alias_name->setText(name);
    mpAliasMainArea->label_idNumber->setText(QString::number(pNewAlias->getID()));

    // Finalize selection
    mpCurrentAliasItem = pNewItem;
    treeWidget_aliases->setCurrentItem(pNewItem);
    slot_aliasSelected(treeWidget_aliases->currentItem());
}

void dlgTriggerEditor::addAction(bool isFolder)
{
    saveAction();

    QString name = isFolder ? tr("New menu") : tr("New button");
    QStringList nameList = { name };
    const QString cmdButtonUp = "";
    const QString cmdButtonDown = "";
    const QString script = "";

    QTreeWidgetItem* pParentItem = treeWidget_actions->currentItem();
    QTreeWidgetItem* pNewItem = nullptr;
    QPointer<TAction> pNewAction = nullptr;

    if (pParentItem) {
        const int parentID = pParentItem->data(0, Qt::UserRole).toInt();
        TAction* pParentAction = mpHost->getActionUnit()->getAction(parentID);

        if (pParentAction) {
            // insert new items as siblings unless the parent is a folder
            if (pParentAction->isFolder()) {
                pNewAction = new TAction(pParentAction, mpHost);
                pNewItem = new QTreeWidgetItem(pParentItem, nameList);
                pParentItem->insertChild(0, pNewItem);
            } else if (pParentAction->getParent() && pParentItem->parent()) {
                pNewAction = new TAction(pParentAction->getParent(), mpHost);
                pNewItem = new QTreeWidgetItem(pParentItem->parent(), nameList);
                pParentItem->parent()->insertChild(0, pNewItem);
            }
        }
    }
    // Otherwise: insert a new root item
    if (!pNewAction) {
        name = isFolder ? tr("New toolbar") : tr("New button");
        pNewAction = new TAction(name, mpHost);
        pNewAction->setCommandButtonUp(cmdButtonUp);
        QStringList nl;
        nl << name;
        pNewItem = new QTreeWidgetItem(mpActionBaseItem, nl);
        treeWidget_actions->insertTopLevelItem(0, pNewItem);
    }

    // Initialize logic object properties
    pNewAction->setName(name);
    pNewAction->setCommandButtonUp(cmdButtonUp);
    pNewAction->setCommandButtonDown(cmdButtonDown);
    pNewAction->setIsPushDownButton(false);
    pNewAction->mLocation = 1;
    pNewAction->mOrientation = 1;
    pNewAction->setScript(script);
    pNewAction->setIsFolder(isFolder);
    pNewAction->setIsActive(false);
    pNewAction->registerAction();

    // Initialize tree item properties
    pNewItem->setData(0, Qt::UserRole, pNewAction->getID());
    pNewItem->setIcon(0, QIcon(QPixmap(isFolder ?
        qsl(":/icons/folder-red.png") :
        qsl(":/icons/document-save-as.png"))));
    pNewItem->setData(0, Qt::AccessibleDescriptionRole, isFolder ? descNewFolder : descNewItem);

    // Expand parent if applicable
    if (pParentItem) {
        pParentItem->setExpanded(true);
    }

    // Reset UI
    mpActionsMainArea->lineEdit_action_icon->clear();
    mpActionsMainArea->checkBox_action_button_isPushDown->setChecked(false);
    clearDocument(mpSourceEditorEdbee); // New Action

    // This prevents reloading a Floating toolbar when an empty action is added.
    // After the action is saved it may trigger the rebuild.
    pNewAction->setDataSaved();

    mpHost->getActionUnit()->updateToolbar();

    // Finalize selection
    mpCurrentActionItem = pNewItem;
    treeWidget_actions->setCurrentItem(pNewItem);
    slot_actionSelected(treeWidget_actions->currentItem());
}


void dlgTriggerEditor::addScript(bool isFolder)
{
    saveScript();
    QString name = isFolder ? tr("New script group") : tr("New script");
    QStringList nameList = { name };
    const QString script;

    QTreeWidgetItem* pParentItem = treeWidget_scripts->currentItem();
    QTreeWidgetItem* pNewItem = nullptr;
    TScript* pNewScript = nullptr;

    if (pParentItem) {
        const int parentID = pParentItem->data(0, Qt::UserRole).toInt();
        TScript* pParentTrigger = mpHost->getScriptUnit()->getScript(parentID);

        if (pParentTrigger) {
            // insert new items as siblings unless the parent is a folder
            if (pParentTrigger->isFolder()) {
                pNewScript = new TScript(pParentTrigger, mpHost);
                pNewItem = new QTreeWidgetItem(pParentItem, nameList);
                pParentItem->insertChild(0, pNewItem);
            } else if (pParentTrigger->getParent() && pParentItem->parent()) {
                pNewScript = new TScript(pParentTrigger->getParent(), mpHost);
                pNewItem = new QTreeWidgetItem(pParentItem->parent(), nameList);
                pParentItem->parent()->insertChild(0, pNewItem);
            }
        }
    }

    if (!pNewScript) {
        // Fallback to insert a new root item
        pNewScript = new TScript(name, mpHost);
        pNewItem = new QTreeWidgetItem(mpScriptsBaseItem, nameList);
        treeWidget_scripts->insertTopLevelItem(0, pNewItem);
    }

    if (!pNewScript) {
        return;
    }

    // Initialize logic object properties
    pNewScript->setName(name);
    pNewScript->setScript(script);
    pNewScript->setIsFolder(isFolder);
    pNewScript->setIsActive(false);
    pNewScript->registerScript();

    // Initialize tree item properties
    pNewItem->setData(0, Qt::UserRole, pNewScript->getID());
    pNewItem->setIcon(0, QIcon(QPixmap(isFolder ?
        qsl(":/icons/folder-red.png") :
        qsl(":/icons/document-save-as.png"))));
    pNewItem->setData(0, Qt::AccessibleDescriptionRole, isFolder ? descNewFolder : descNewItem);

    // Expand parent if applicable
    if (pParentItem) {
        pParentItem->setExpanded(true);
    }

    // Reset UI
    mpScriptsMainArea->lineEdit_script_name->clear();
    mpScriptsMainArea->label_idNumber->clear();
    mpScriptsMainArea->lineEdit_script_event_handler_entry->clear();
    clearDocument(mpSourceEditorEdbee, script);

    // Finalize selection
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
            treeWidget_triggers->clearSelection();
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
            treeWidget_timers->clearSelection();
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
            treeWidget_aliases->clearSelection();
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
            treeWidget_scripts->clearSelection();
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
            treeWidget_actions->clearSelection();
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
            treeWidget_keys->clearSelection();
            treeWidget_keys->setCurrentItem((*it), 0);
            treeWidget_keys->scrollToItem((*it));
            mpCurrentKeyItem = (*it);
            return;
        }
        ++it;
    }
}

TTrigger* dlgTriggerEditor::getTriggerFromTreeItem(QTreeWidgetItem* item)
{
    if (!item || !item->parent()) {
        return nullptr;
    }

    const int triggerID = item->data(0, Qt::UserRole).toInt();
    return mpHost->getTriggerUnit()->getTrigger(triggerID);
}

TAlias* dlgTriggerEditor::getAliasFromTreeItem(QTreeWidgetItem* item)
{
    if (!item || !item->parent()) {
        return nullptr;
    }

    const int aliasID = item->data(0, Qt::UserRole).toInt();
    return mpHost->getAliasUnit()->getAlias(aliasID);
}

TScript* dlgTriggerEditor::getScriptFromTreeItem(QTreeWidgetItem* item)
{
    if (!item || !item->parent()) {
        return nullptr;
    }

    const int scriptID = item->data(0, Qt::UserRole).toInt();
    return mpHost->getScriptUnit()->getScript(scriptID);
}

TTimer* dlgTriggerEditor::getTimerFromTreeItem(QTreeWidgetItem* item)
{
    if (!item || !item->parent()) {
        return nullptr;
    }

    const int timerID = item->data(0, Qt::UserRole).toInt();
    return mpHost->getTimerUnit()->getTimer(timerID);
}

TKey* dlgTriggerEditor::getKeyFromTreeItem(QTreeWidgetItem* item)
{
    if (!item || !item->parent()) {
        return nullptr;
    }

    const int keyID = item->data(0, Qt::UserRole).toInt();
    return mpHost->getKeyUnit()->getKey(keyID);
}

TAction* dlgTriggerEditor::getActionFromTreeItem(QTreeWidgetItem* item)
{
    if (!item || !item->parent()) {
        return nullptr;
    }

    const int actionID = item->data(0, Qt::UserRole).toInt();
    return mpHost->getActionUnit()->getAction(actionID);
}

void dlgTriggerEditor::slot_itemEdited()
{
    QString packageName;
    switch (mCurrentView) {
    case EditorViewType::cmTriggerView: {
        if (auto trigger = getTriggerFromTreeItem(mpCurrentTriggerItem)) {
            packageName = trigger->packageName(trigger);
        }
        break;
    }
    case EditorViewType::cmAliasView: {
        if (auto alias = getAliasFromTreeItem(mpCurrentAliasItem)) {
            packageName = alias->packageName(alias);
        }
        break;
    }
    case EditorViewType::cmTimerView: {
        if (auto timer = getTimerFromTreeItem(mpCurrentTimerItem)) {
            packageName = timer->packageName(timer);
        }
        break;
    }
    case EditorViewType::cmScriptView: {
        if (auto script = getScriptFromTreeItem(mpCurrentScriptItem)) {
            packageName = script->packageName(script);
        }
        break;
    }
    case EditorViewType::cmActionView: {
        if (auto action = getActionFromTreeItem(mpCurrentActionItem)) {
            packageName = action->packageName(action);
        }
        break;
    }
    case EditorViewType::cmKeysView: {
        if (auto key = getKeyFromTreeItem(mpCurrentKeyItem)) {
            packageName = key->packageName(key);
        }
        break;
    }
    case EditorViewType::cmUnknownView:
        [[fallthrough]];
    case EditorViewType::cmVarsView:
        break;
    }

    if (!packageName.isEmpty()) {
        //: Package item warning shown in trigger editor when editing package items. Should only be announced to screen readers once per item, not repeatedly on every edit.
        showWarning(tr("This item is part of a package. To best preserve your changes, copy this item before editing as package upgrades may overwrite modifications."), false);
    }
}

void dlgTriggerEditor::saveTrigger()
{
    QTreeWidgetItem* pItem = mpCurrentTriggerItem;
    if (!pItem) {
        return;
    }

    // Additional safety check: ensure the item's parent is still valid
    // and that the item is still part of the tree widget
    if (!pItem->parent() || pItem->treeWidget() != treeWidget_triggers) {
        return;
    }

    mpTriggersMainArea->trimName();
    const QString name = mpTriggersMainArea->lineEdit_trigger_name->text();
    const QString command = mpTriggersMainArea->lineEdit_trigger_command->text();
    QStringList patterns;
    QList<int> patternKinds;
    int validItems = 0;
    for (int i = 0; i < mTriggerPatternEdit.size(); i++) {
        QString pattern = mTriggerPatternEdit.at(i)->singleLineTextEdit_pattern->toPlainText();

        // Spaces in the pattern may be marked with middle dots, convert them back
        unmarkQString(&pattern);

        const int patternType = mTriggerPatternEdit.at(i)->comboBox_patternType->currentIndex();
        if (pattern.isEmpty() && patternType != REGEX_PROMPT && patternType != REGEX_LINE_SPACER) {
            continue;
        }

        ++validItems;
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
    const bool isMultiline = (mpTriggersMainArea->spinBox_lineMargin->value() > -1) && (validItems > 1);
    const QString script = mpSourceEditorEdbeeDocument->text();

    const int triggerID = pItem->data(0, Qt::UserRole).toInt();
    TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(triggerID);
    if (pT) {
        pT->setName(name);
        pT->setCommand(command);
        pT->setRegexCodeList(patterns, patternKinds);

        pT->setScript(script);
        pT->setIsMultiline(isMultiline);
        pT->mPerlSlashGOption = mpTriggersMainArea->checkBox_perlSlashGOption->isChecked();
        pT->mFilterTrigger = mpTriggersMainArea->checkBox_filterTrigger->isChecked();
        if (mpTriggersMainArea->spinBox_lineMargin->value() >= 0) {
            pT->setConditionLineDelta(mpTriggersMainArea->spinBox_lineMargin->value());
        }
        pT->mStayOpen = mpTriggersMainArea->spinBox_stayOpen->value();
        pT->mSoundTrigger = mpTriggersMainArea->groupBox_soundTrigger->isChecked();
        pT->setSound(mpTriggersMainArea->lineEdit_soundFile->text());

        QColor fgColor(QColorConstants::Transparent);
        QColor bgColor(QColorConstants::Transparent);
        if (!mpTriggersMainArea->pushButtonFgColor->property(cButtonBaseColor).toString().isEmpty()) {
            fgColor = QColor(mpTriggersMainArea->pushButtonFgColor->property(cButtonBaseColor).toString());
        }
        pT->setColorizerFgColor(fgColor);
        if (!mpTriggersMainArea->pushButtonBgColor->property(cButtonBaseColor).toString().isEmpty()) {
            bgColor = QColor(mpTriggersMainArea->pushButtonBgColor->property(cButtonBaseColor).toString());
        }
        pT->setColorizerBgColor(bgColor);
        pT->setIsColorizerTrigger(mpTriggersMainArea->groupBox_triggerColorizer->isChecked());
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

            if (pT->checkIfNew()) {
                if (pT->isFolder()) {
                    if (pT->shouldBeActive()) {
                        itemDescription = descActiveFolder;
                        if (pT->ancestorsActive()) {
                            icon.addPixmap(QPixmap(qsl(":/icons/folder-blue.png")), QIcon::Normal, QIcon::Off);
                        } else {
                            icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                            itemDescription = descInactiveParent.arg(itemDescription);
                        }
                    } else {
                        itemDescription = descInactiveFolder;
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-blue-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                } else {
                    // Set visual appearance based on actual active state, not "new" status
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
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    }
                }
                pItem->setIcon(0, icon);
                pItem->setText(0, name);

                // Only enable truly new triggers, not existing disabled ones being loaded
                if (pT->shouldBeActive()) {
                    pT->setIsActive(true);
                }
                pT->unmarkAsNew();
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

    // Ensure the item is still part of the tree widget
    if (pItem->treeWidget() != treeWidget_timers) {
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

    // Ensure the item is still part of the tree widget
    if (pItem->treeWidget() != treeWidget_aliases) {
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

            if (pT->checkIfNew()) {
                if (pT->isFolder()) {
                    if (pT->shouldBeActive()) {
                        itemDescription = descActiveFolder;
                        if (pT->ancestorsActive()) {
                            icon.addPixmap(QPixmap(qsl(":/icons/folder-violet.png")), QIcon::Normal, QIcon::Off);
                        } else {
                            icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                            itemDescription = descInactiveParent.arg(itemDescription);
                        }
                    } else {
                        itemDescription = descInactiveFolder;
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-violet-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                } else {
                    // Set visual appearance based on actual active state, not "new" status
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
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    }
                }
                pItem->setIcon(0, icon);
                pItem->setText(0, name);

                // Only enable truly new aliases, not existing disabled ones being loaded
                if (pT->shouldBeActive()) {
                    pT->setIsActive(true);
                }
                pT->unmarkAsNew();
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

    // Ensure the item is still part of the tree widget
    if (pItem->treeWidget() != treeWidget_actions) {
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

    disconnect(mpSourceEditorEdbeeDocument, &edbee::TextDocument::textChanged, this, &dlgTriggerEditor::slot_itemEdited);
    mpSourceEditorEdbeeDocument->setText(scriptCode);
    connect(mpSourceEditorEdbeeDocument, &edbee::TextDocument::textChanged, this, &dlgTriggerEditor::slot_itemEdited);
}

void dlgTriggerEditor::saveScript()
{
    QTreeWidgetItem* pItem = mpCurrentScriptItem;
    if (!pItem) {
        return;
    }

    // Ensure the item is still part of the tree widget
    if (pItem->treeWidget() != treeWidget_scripts) {
        return;
    }

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

    pT->setName(name);
    pT->setEventHandlerList(handlerList);
    pT->setScript(script);

    pT->compileAll();
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

        if (pT->checkIfNew()) {
            if (pT->isFolder()) {
                itemDescription = descActiveFolder;
                if (pT->ancestorsActive()) {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-orange.png")), QIcon::Normal, QIcon::Off);
                } else {
                    icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                    itemDescription = descInactiveParent.arg(itemDescription);
                }
            } else {
                // Set visual appearance based on actual active state, not "new" status
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
                    icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                }
            }
            pItem->setIcon(0, icon);
            pItem->setText(0, name);

            // Only enable truly new scripts, not existing disabled ones being loaded
            if (pT->shouldBeActive()) {
                pT->setIsActive(true);
            }
            pT->unmarkAsNew();
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

void dlgTriggerEditor::clearEditorNotification()
{
    mpSystemMessageArea->hide();
    mCurrentBannerKey.clear();
}

void dlgTriggerEditor::updatePackageItemAccessibility(QTreeWidgetItem* pItem, const QString& currentDescription)
{
    // Append package description to existing accessible description
    // Screen readers will announce: "Item Name, [current description], package item"
    QString newDescription;
    if (currentDescription.isEmpty()) {
        newDescription = descPackageItem;
    } else {
        // Combine descriptions: e.g., "activated, package item"
        newDescription = currentDescription + qsl(", ") + descPackageItem;
    }
    pItem->setData(0, Qt::AccessibleDescriptionRole, newDescription);
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
    pItem->setData(0, Qt::UserRole, variable->getValueType());
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

    // Ensure the item is still part of the tree widget
    if (pItem->treeWidget() != treeWidget_keys) {
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
                    if (pT->shouldBeActive()) {
                        itemDescription = descActiveFolder;
                        if (pT->ancestorsActive()) {
                            icon.addPixmap(QPixmap(qsl(":/icons/folder-pink.png")), QIcon::Normal, QIcon::Off);
                        } else {
                            icon.addPixmap(QPixmap(qsl(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                            itemDescription = descInactiveParent.arg(itemDescription);
                        }
                    } else {
                        itemDescription = descInactiveFolder;
                        icon.addPixmap(QPixmap(qsl(":/icons/folder-pink-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                } else {
                    // Set visual appearance based on actual active state, not "new" status
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
                        icon.addPixmap(QPixmap(qsl(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    }
                }
                pItem->setIcon(0, icon);
                pItem->setText(0, name);

                // Only enable truly new keys, not existing disabled ones being loaded
                if (pT->shouldBeActive()) {
                    pT->setIsActive(true);
                }
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
        markQTextEdit(pItem->singleLineTextEdit_pattern);
        lineEditShouldMarkSpaces[pItem->singleLineTextEdit_pattern] = true;
        pItem->singleLineTextEdit_pattern->blockSignals(true);
        pItem->singleLineTextEdit_pattern->rehighlight();
        pItem->singleLineTextEdit_pattern->blockSignals(false);
    } else {
        unmarkQTextEdit(pItem->singleLineTextEdit_pattern);
        lineEditShouldMarkSpaces[pItem->singleLineTextEdit_pattern] = false;
    }

    switch (type) {
    case REGEX_SUBSTRING:
    case REGEX_PERL:
    case REGEX_BEGIN_OF_LINE_SUBSTRING:
    case REGEX_EXACT_MATCH:
    case REGEX_LUA_CODE:
        pItem->singleLineTextEdit_pattern->setHighlightingEnabled(type == REGEX_PERL);
        pItem->singleLineTextEdit_pattern->show();
        pItem->pushButton_fgColor->hide();
        pItem->pushButton_bgColor->hide();
        pItem->label_prompt->hide();
        pItem->spinBox_lineSpacer->hide();
        break;
    case REGEX_LINE_SPACER:
        pItem->singleLineTextEdit_pattern->hide();
        pItem->pushButton_fgColor->hide();
        pItem->pushButton_bgColor->hide();
        pItem->label_prompt->hide();
        pItem->spinBox_lineSpacer->show();
        break;
    case REGEX_COLOR_PATTERN:
        // CHECKME: Do we need to regenerate (hidden patter text) and button texts/colors?
        pItem->singleLineTextEdit_pattern->hide();
        pItem->pushButton_fgColor->show();
        pItem->pushButton_bgColor->show();
        pItem->label_prompt->hide();
        pItem->spinBox_lineSpacer->hide();
        break;
    case REGEX_PROMPT:
        pItem->singleLineTextEdit_pattern->hide();
        pItem->pushButton_fgColor->hide();
        pItem->pushButton_bgColor->hide();
        if (mpHost->mTelnet.mGA_Driver) {
            pItem->label_prompt->setText(tr("match on the prompt line"));
            pItem->label_prompt->setToolTip(QString());
            pItem->label_prompt->setEnabled(true);
        } else {
            pItem->label_prompt->setText(tr("match on the prompt line (disabled)"));
            pItem->label_prompt->setToolTip(utils::richText(tr("A Go-Ahead (GA) signal from the game is required to make this feature work")));
            pItem->label_prompt->setEnabled(false);
        }
        pItem->label_prompt->show();
        pItem->spinBox_lineSpacer->hide();
        break;
    }

    checkForMoreThanOneTriggerItem();
    updatePatternTabOrder();
    updatePatternPlaceholders();
    updatePatternNavigationHint();
}

void dlgTriggerEditor::handlePatternChange(dlgTriggerPatternEdit* patternItem, bool hasContentHint)
{
    checkForMoreThanOneTriggerItem();

    bool hasContent = hasContentHint;
    bool forceLineSpacerActive = false;
    if (patternItem) {
        const int type = patternItem->comboBox_patternType->currentIndex();
        if (type == REGEX_PROMPT) {
            hasContent = true;
        } else if (type == REGEX_LINE_SPACER) {
            forceLineSpacerActive = hasContentHint;
            if (!forceLineSpacerActive) {
                hasContent = patternItem->spinBox_lineSpacer->value() > 0;
            } else {
                hasContent = true;
            }
        }

        if (patternItem->mRow == mVisiblePatternCount - 1 && hasContent && mVisiblePatternCount < 50) {
            showPatternItems(mVisiblePatternCount + 1);
        }
    }

    int lastActive = -1;
    for (int i = 0; i < mVisiblePatternCount; ++i) {
        auto* item = mTriggerPatternEdit[i];
        bool itemHasContent = !item->singleLineTextEdit_pattern->toPlainText().isEmpty();
        const int type = item->comboBox_patternType->currentIndex();
        if (type == REGEX_PROMPT) {
            itemHasContent = true;
        } else if (type == REGEX_LINE_SPACER) {
            itemHasContent = item->spinBox_lineSpacer->value() > 0
                             || item->spinBox_lineSpacer->isVisible();
            if (forceLineSpacerActive && item == patternItem) {
                itemHasContent = true;
            }
        }

        if (itemHasContent) {
            lastActive = i;
        }
    }

    const int desiredCount = qMax(lastActive + 2, 2);
    if (desiredCount != mVisiblePatternCount) {
        showPatternItems(desiredCount);
    } else {
        updatePatternPlaceholders();
    }
}

QWidget* dlgTriggerEditor::firstFocusablePatternWidget(const dlgTriggerPatternEdit* patternItem) const
{
    if (!patternItem) {
        return nullptr;
    }

    if (patternItem->singleLineTextEdit_pattern->isVisible()) {
        return patternItem->singleLineTextEdit_pattern;
    }
    if (patternItem->spinBox_lineSpacer->isVisible()) {
        return patternItem->spinBox_lineSpacer;
    }
    if (patternItem->pushButton_fgColor->isVisible()) {
        return patternItem->pushButton_fgColor;
    }
    if (patternItem->pushButton_bgColor->isVisible()) {
        return patternItem->pushButton_bgColor;
    }

    return patternItem->comboBox_patternType;
}

bool dlgTriggerEditor::focusPatternItem(const int row, const Qt::FocusReason reason)
{
    if (row < 0 || row >= mVisiblePatternCount || row >= mTriggerPatternEdit.size()) {
        return false;
    }

    auto* patternItem = mTriggerPatternEdit.value(row, nullptr);
    if (!patternItem || !patternItem->isVisible()) {
        return false;
    }

    QWidget* target = firstFocusablePatternWidget(patternItem);
    if (!target) {
        return false;
    }

    mpScrollArea->ensureWidgetVisible(patternItem);
    target->setFocus(reason);

    if (auto* edit = qobject_cast<SingleLineTextEdit*>(target)) {
        auto cursor = edit->textCursor();
        cursor.select(QTextCursor::Document);
        edit->setTextCursor(cursor);
    } else if (auto* spinBox = qobject_cast<QSpinBox*>(target)) {
        spinBox->selectAll();
    }

    return true;
}

bool dlgTriggerEditor::focusNextPatternItem(const dlgTriggerPatternEdit* currentItem)
{
    if (!currentItem) {
        return false;
    }

    int nextRow = currentItem->mRow + 1;
    while (nextRow < mVisiblePatternCount && nextRow < mTriggerPatternEdit.size()) {
        auto* nextItem = mTriggerPatternEdit.value(nextRow, nullptr);
        if (nextItem && nextItem->isVisible()) {
            return focusPatternItem(nextRow);
        }
        ++nextRow;
    }

    return false;
}


bool dlgTriggerEditor::focusPreviousPatternItem(const dlgTriggerPatternEdit* currentItem)
{
    if (!currentItem) {
        return false;
    }

    int previousRow = currentItem->mRow - 1;
    while (previousRow >= 0) {
        auto* previousItem = mTriggerPatternEdit.value(previousRow, nullptr);
        if (previousItem && previousItem->isVisible()) {
            return focusPatternItem(previousRow);
        }
        --previousRow;
    }

    return false;
}


void dlgTriggerEditor::updatePatternTabOrder()
{
    if (!mpTriggersMainArea) {
        return;
    }

    QWidget* previous = mpTriggersMainArea->lineEdit_trigger_name;
    auto addToChain = [&previous, this](QWidget* next) {
        if (!next || !previous) {
            if (next) {
                previous = next;
            }
            return;
        }

        if (!next->isVisibleTo(mpTriggersMainArea)) {
            return;
        }

        QWidget::setTabOrder(previous, next);
        previous = next;
    };

    addToChain(mpTriggersMainArea->toolButton_toggleExtraControls);
    addToChain(mpTriggersMainArea->lineEdit_trigger_command);

    for (int i = 0; i < mVisiblePatternCount && i < mTriggerPatternEdit.size(); ++i) {
        auto* item = mTriggerPatternEdit.value(i, nullptr);
        if (!item || !item->isVisible()) {
            continue;
        }

        QWidget* first = firstFocusablePatternWidget(item);
        addToChain(first);

        if (item->spinBox_lineSpacer->isVisible() && item->spinBox_lineSpacer != first) {
            addToChain(item->spinBox_lineSpacer);
        }
        if (item->pushButton_fgColor->isVisible()) {
            addToChain(item->pushButton_fgColor);
        }
        if (item->pushButton_bgColor->isVisible()) {
            addToChain(item->pushButton_bgColor);
        }
        if (item->comboBox_patternType->isVisible()) {
            addToChain(item->comboBox_patternType);
        }
    }
    addToChain(mpTriggersMainArea->spinBox_stayOpen);
    addToChain(mpTriggersMainArea->groupBox_soundTrigger);
    addToChain(mpTriggersMainArea->pushButtonSound);
    addToChain(mpTriggersMainArea->toolButton_clearSoundFile);
    addToChain(mpTriggersMainArea->spinBox_lineMargin);
    addToChain(mpTriggersMainArea->checkBox_filterTrigger);
    addToChain(mpTriggersMainArea->checkBox_perlSlashGOption);
    addToChain(mpTriggersMainArea->groupBox_triggerColorizer);
    addToChain(mpTriggersMainArea->pushButtonFgColor);
    addToChain(mpTriggersMainArea->pushButtonBgColor);
    addToChain(mPatternNavigationHintBanner);
    addToChain(mpSourceEditorEdbee);

}

void dlgTriggerEditor::slot_changedPattern()
{
    SingleLineTextEdit* textEdit = qobject_cast<SingleLineTextEdit*>(sender());

    if (textEdit && lineEditShouldMarkSpaces[textEdit]) {
        markQTextEdit(textEdit);
        textEdit->blockSignals(true);
        textEdit->rehighlight();
        textEdit->blockSignals(false);
    }

    auto* patternItem = textEdit ? qobject_cast<dlgTriggerPatternEdit*>(textEdit->parentWidget()) : nullptr;
    const bool hasText = textEdit && !textEdit->toPlainText().isEmpty();
    handlePatternChange(patternItem, hasText);
}

void dlgTriggerEditor::slot_lineSpacerChanged(int value)
{
    auto* spinBox = qobject_cast<QSpinBox*>(sender());
    if (!spinBox) {
        return;
    }

    auto* patternItem = qobject_cast<dlgTriggerPatternEdit*>(spinBox->parentWidget());
    if (!patternItem) {
        return;
    }

    handlePatternChange(patternItem, value > 0);
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
    if (row < 0 || row >= mTriggerPatternEdit.size()) {
        return;
    }

    // This is the collection of widgets that make up one of the patterns
    // in the dlgTriggerMainArea:
    dlgTriggerPatternEdit* pPatternItem = mTriggerPatternEdit[row];
    setupPatternControls(type, pPatternItem);
    if (type == REGEX_COLOR_PATTERN) {
        if (pPatternItem->singleLineTextEdit_pattern->toPlainText().isEmpty()) {
            // This COLOR trigger is a new one in that there is NO text
            // So set it to the default (ignore both) - which will generate an
            // error if saved without setting a color for at least one element:

            pPatternItem->singleLineTextEdit_pattern->setPlainText(TTrigger::createColorPatternText(TTrigger::scmIgnored, TTrigger::scmIgnored));
        }

        // Only process the text if it looks like it should:
        if ((pPatternItem->singleLineTextEdit_pattern->toPlainText().startsWith(QLatin1String("ANSI_COLORS_F{"))
              && pPatternItem->singleLineTextEdit_pattern->toPlainText().contains(QLatin1String("}_B{"))
              && pPatternItem->singleLineTextEdit_pattern->toPlainText().endsWith(QLatin1String("}")))) {

            // It looks as though there IS a valid color pattern string in the
            // lineEdit, so, in case it has been edited by hand, regenerate the
            // colors that are used:
            int textAnsiFg = TTrigger::scmIgnored;
            int textAnsiBg = TTrigger::scmIgnored;
            TTrigger::decodeColorPatternText(pPatternItem->singleLineTextEdit_pattern->toPlainText(), textAnsiFg, textAnsiBg);

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
                     << pPatternItem->singleLineTextEdit_pattern->toPlainText()
                     << "does not fit the pattern!";
        }*/

    } else {
        // Is NOT a REGEX_COLOR_PATTERN - if the text corresponds to the color
        // pattern text equivalent to ignore both fore and back ground then
        // clear the text - otherwise leave as is:
        if (pPatternItem->singleLineTextEdit_pattern->toPlainText().compare(QLatin1String("ANSI_COLORS_F{IGNORE}_B{IGNORE}")) == 0) {
            pPatternItem->singleLineTextEdit_pattern->clear();
        }
    }

    const bool hasText = !pPatternItem->singleLineTextEdit_pattern->toPlainText().isEmpty();
    const bool treatAsContent = hasText || type == REGEX_PROMPT || type == REGEX_LINE_SPACER;
    handlePatternChange(pPatternItem, treatAsContent);
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
    mpTriggersMainArea->checkBox_perlSlashGOption->setChecked(false);
    mpTriggersMainArea->checkBox_filterTrigger->setChecked(false);
    mpTriggersMainArea->groupBox_triggerColorizer->setChecked(false);
    mpTriggersMainArea->pushButtonFgColor->setStyleSheet(QString());
    mpTriggersMainArea->pushButtonFgColor->setProperty(cButtonBaseColor, QVariant());
    mpTriggersMainArea->pushButtonBgColor->setStyleSheet(QString());
    mpTriggersMainArea->pushButtonBgColor->setProperty(cButtonBaseColor, QVariant());
    mpTriggersMainArea->spinBox_lineMargin->setValue(-1);

    const int ID = pItem->data(0, Qt::UserRole).toInt();
    TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(ID);
    if (pT) {
        const QStringList patternList = pT->getPatternsList();
        QList<int> const propertyList = pT->getRegexCodePropertyList();

        if (patternList.size() != propertyList.size()) {
            return;
        }

        showPatternItems(qMax(patternList.size(), 2));
        for (int i = 0; i < patternList.size() && i < mTriggerPatternEdit.size(); i++) {
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
                pPatternItem->comboBox_patternType->setCurrentIndex(1);
                setupPatternControls(1, pPatternItem);
            }
            pPatternItem->comboBox_patternType->setCurrentIndex(pType);
            setupPatternControls(pType, pPatternItem);
            if (pType == REGEX_PROMPT) {
                pPatternItem->singleLineTextEdit_pattern->clear();

            } else if (pType == REGEX_COLOR_PATTERN) {
                pPatternItem->singleLineTextEdit_pattern->setPlainText(patternList.at(i));
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
                    qWarning() << "dlgTriggerEditor::slot_triggerSelected(...) ERROR: TTrigger instance has an mColorPattern of size:"
                               << pT->mColorPatternList.size()
                               << "but array element:"
                               << i
                               << "is a nullptr";
                    pPatternItem->pushButton_fgColor->setStyleSheet(QString());
                    pPatternItem->pushButton_fgColor->setText(tr("fault"));
                    pPatternItem->pushButton_bgColor->setStyleSheet(QString());
                    pPatternItem->pushButton_fgColor->setText(tr("fault"));
                }
            } else if (pType == REGEX_LINE_SPACER) {
                pPatternItem->spinBox_lineSpacer->setValue(patternList.at(i).toInt());
            } else {
                // Keep track of lineEdits that should have trailing spaces marked
                if (pType == REGEX_PERL) {
                    lineEditShouldMarkSpaces[pPatternItem->singleLineTextEdit_pattern] = true;
                }
                pPatternItem->singleLineTextEdit_pattern->setPlainText(patternList.at(i));
            }
        }

        // reset the rest of the patterns that don't have any data
        for (int i = patternList.size(); i < mVisiblePatternCount; i++) {
            auto* patternItem = mTriggerPatternEdit[i];
            patternItem->singleLineTextEdit_pattern->clear();
            patternItem->pushButton_fgColor->hide();
            patternItem->pushButton_bgColor->hide();
            patternItem->label_prompt->hide();
            patternItem->spinBox_lineSpacer->hide();
            patternItem->comboBox_patternType->setCurrentIndex(0);
        }
        // Scroll to the last used pattern:
        mpScrollArea->ensureWidgetVisible(mTriggerPatternEdit.at(qBound(0, patternList.size(), mVisiblePatternCount - 1)));
        const QString command = pT->getCommand();
        mpTriggersMainArea->lineEdit_trigger_name->setText(pItem->text(0));
        mpTriggersMainArea->label_idNumber->setText(QString::number(ID));
        mpTriggersMainArea->lineEdit_trigger_command->setText(command);
        mpTriggersMainArea->checkBox_perlSlashGOption->setChecked(pT->mPerlSlashGOption);
        mpTriggersMainArea->checkBox_filterTrigger->setChecked(pT->mFilterTrigger);
        if (pT->isMultiline()) {
            mpTriggersMainArea->spinBox_lineMargin->setValue(pT->getConditionLineDelta());
        } else {
            mpTriggersMainArea->spinBox_lineMargin->setValue(-1);
        }
        mpTriggersMainArea->spinBox_stayOpen->setValue(pT->mStayOpen);
        mpTriggersMainArea->groupBox_soundTrigger->setChecked(pT->mSoundTrigger);
        if (!pT->mSoundFile.isEmpty()) {
            mpTriggersMainArea->lineEdit_soundFile->setToolTip(pT->mSoundFile);
        }
        mpTriggersMainArea->lineEdit_soundFile->setText(pT->mSoundFile);
        mpTriggersMainArea->lineEdit_soundFile->setCursorPosition(mpTriggersMainArea->lineEdit_soundFile->text().length());
        mpTriggersMainArea->toolButton_clearSoundFile->setEnabled(!mpTriggersMainArea->lineEdit_soundFile->text().isEmpty());
        mpTriggersMainArea->groupBox_triggerColorizer->setChecked(pT->isColorizerTrigger());

        const QColor fgColor(pT->getFgColor());
        const QColor bgColor(pT->getBgColor());
        const bool transparentFg = fgColor == QColorConstants::Transparent;
        const bool transparentBg = bgColor == QColorConstants::Transparent;
        mpTriggersMainArea->pushButtonFgColor->setStyleSheet(generateButtonStyleSheet(fgColor, pT->isColorizerTrigger()));
        mpTriggersMainArea->pushButtonFgColor->setProperty(cButtonBaseColor, transparentFg ? qsl("transparent") : fgColor.name());
        //: Keep the existing colour on matches to highlight. Use shortest word possible so it fits on the button
        mpTriggersMainArea->pushButtonFgColor->setText(transparentFg ? tr("keep") : QString());
        mpTriggersMainArea->pushButtonBgColor->setStyleSheet(generateButtonStyleSheet(pT->getBgColor(), pT->isColorizerTrigger()));
        mpTriggersMainArea->pushButtonBgColor->setProperty(cButtonBaseColor, transparentBg ? qsl("transparent") : bgColor.name());
        //: Keep the existing colour on matches to highlight. Use shortest word possible so it fits on the button
        mpTriggersMainArea->pushButtonBgColor->setText(transparentBg ? tr("keep") : QString());

        checkForMoreThanOneTriggerItem();

        clearDocument(mpSourceEditorEdbee, pT->getScript());

        if (!pT->state()) {
            showError(pT->getError());
        } else {
            // Show package warning if this item belongs to a package
            QString packageName = pT->packageName(pT);
            if (!packageName.isEmpty()) {
                // Update accessibility description for screen readers (appears after item name)
                QString currentDesc = pItem->data(0, Qt::AccessibleDescriptionRole).toString();
                updatePackageItemAccessibility(pItem, currentDesc);

                // Show visual warning banner (without screen reader announcement to avoid spam)
                //: Package item warning banner shown in trigger editor when selecting package items
                showWarning(tr("This item is part of a package. To best preserve your changes, copy this item before editing as package upgrades may overwrite modifications."), false);

                // Announce full educational message only on first package item encountered
                static bool firstPackageAnnounced = false;
                if (!firstPackageAnnounced) {
                    //: First-time educational message for screen reader users about package items
                    mudlet::self()->announce(tr("Package item. Copy before editing to preserve changes."));
                    firstPackageAnnounced = true;
                }
            }
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
        } else {
            // Show package warning if this item belongs to a package
            QString packageName = pT->packageName(pT);
            if (!packageName.isEmpty()) {
                // Update accessibility description for screen readers (appears after item name)
                QString currentDesc = pItem->data(0, Qt::AccessibleDescriptionRole).toString();
                updatePackageItemAccessibility(pItem, currentDesc);

                // Show visual warning banner (without screen reader announcement to avoid spam)
                //: Package item warning banner shown in trigger editor when selecting package items
                showWarning(tr("This item is part of a package. To best preserve your changes, copy this item before editing as package upgrades may overwrite modifications."), false);

                // Announce full educational message only on first package item encountered
                static bool firstPackageAnnounced = false;
                if (!firstPackageAnnounced) {
                    //: First-time educational message for screen reader users about package items
                    mudlet::self()->announce(tr("Package item. Copy before editing to preserve changes."));
                    firstPackageAnnounced = true;
                }
            }
        }

    } else {
        // No details to show - as will be the case if the top item (ID = 0) is
        // selected - so show the help message:
        clearAliasForm();
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
        } else {
            // Show package warning if this item belongs to a package
            QString packageName = pT->packageName(pT);
            if (!packageName.isEmpty()) {
                // Update accessibility description for screen readers (appears after item name)
                QString currentDesc = pItem->data(0, Qt::AccessibleDescriptionRole).toString();
                updatePackageItemAccessibility(pItem, currentDesc);

                // Show visual warning banner (without screen reader announcement to avoid spam)
                //: Package item warning banner shown in trigger editor when selecting package items
                showWarning(tr("This item is part of a package. To best preserve your changes, copy this item before editing as package upgrades may overwrite modifications."), false);

                // Announce full educational message only on first package item encountered
                static bool firstPackageAnnounced = false;
                if (!firstPackageAnnounced) {
                    //: First-time educational message for screen reader users about package items
                    mudlet::self()->announce(tr("Package item. Copy before editing to preserve changes."));
                    firstPackageAnnounced = true;
                }
            }
        }
    } else {
        // No details to show - as will be the case if the top item (ID = 0) is
        // selected - so show the help message:
        clearKeyForm();
    }
}

// This should not modify the contents of what pItem points at:
void dlgTriggerEditor::recurseVariablesUp(QTreeWidgetItem* const pItem, QList<QTreeWidgetItem*>& list)
{
    QTreeWidgetItem* pParentItem = pItem->parent();
    if (pParentItem && pParentItem != mpVarBaseItem) {
        list.append(pParentItem);
        recurseVariablesUp(pParentItem, list);
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
    if (!pItem ||treeWidget_variables->indexOfTopLevelItem(pItem) == 0) {
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
    case LUA_TTHREAD:
        ; // No-op
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
    pItem->setData(0, Qt::UserRole, var->getValueType());
    pItem->setIcon(0, icon);
    mChangingVar = false;
}

void dlgTriggerEditor::slot_actionSelected(QTreeWidgetItem* pItem)
{
    if (!pItem) {
        // No details to show - so show the help message:
        clearActionForm();
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
        } else {
            // Show package warning if this item belongs to a package
            QString packageName = pT->packageName(pT);
            if (!packageName.isEmpty()) {
                // Update accessibility description for screen readers (appears after item name)
                QString currentDesc = pItem->data(0, Qt::AccessibleDescriptionRole).toString();
                updatePackageItemAccessibility(pItem, currentDesc);

                // Show visual warning banner (without screen reader announcement to avoid spam)
                //: Package item warning banner shown in trigger editor when selecting package items
                showWarning(tr("This item is part of a package. To best preserve your changes, copy this item before editing as package upgrades may overwrite modifications."), false);

                // Announce full educational message only on first package item encountered
                static bool firstPackageAnnounced = false;
                if (!firstPackageAnnounced) {
                    //: First-time educational message for screen reader users about package items
                    mudlet::self()->announce(tr("Package item. Copy before editing to preserve changes."));
                    firstPackageAnnounced = true;
                }
            }
        }
    } else {
        // On root of treewidget_actions: - show help message instead
        clearActionForm();
    }
}

void dlgTriggerEditor::slot_treeSelectionChanged()
{
    auto * sender = qobject_cast<TTreeWidget*>(QObject::sender());
    if (sender) {
        QTreeWidgetItem* item = sender->currentItem();
        if (!item) {
            QList<QTreeWidgetItem*> items = sender->selectedItems();
            if (items.empty()) {
                return;
            }
            item = items.first();
        }

        if (item) {
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
        clearScriptForm();
        return;
    }

    const int ID = pItem->data(0, Qt::UserRole).toInt();
    TScript* pT = mpHost->getScriptUnit()->getScript(ID);

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

    if (pT) {
        const QString name = pT->getName();
        QStringList eventHandlerList = pT->getEventHandlerList();
        for (const QString& handler : eventHandlerList) {
            auto pItem = new QListWidgetItem(mpScriptsMainArea->listWidget_script_registered_event_handlers);
            pItem->setText(handler);
            mpScriptsMainArea->listWidget_script_registered_event_handlers->addItem(pItem);
        }
        const QString script = pT->getScript();
        clearDocument(mpSourceEditorEdbee, script);

        mpScriptsMainArea->lineEdit_script_name->setText(name);
        mpScriptsMainArea->label_idNumber->setText(QString::number(ID));
        if (auto error = pT->getLoadingError(); error) {
            showWarning(tr("While loading the profile, this script had an error that has since been fixed, "
                           "possibly by another script. The error was:%2%3").arg(qsl("<br>"), error.value()));
        } else if (!pT->state()) {
            showError(pT->getError());
        } else {
            // Show package warning if this item belongs to a package
            QString packageName = pT->packageName(pT);
            if (!packageName.isEmpty()) {
                // Update accessibility description for screen readers (appears after item name)
                QString currentDesc = pItem->data(0, Qt::AccessibleDescriptionRole).toString();
                updatePackageItemAccessibility(pItem, currentDesc);

                // Show visual warning banner (without screen reader announcement to avoid spam)
                //: Package item warning banner shown in trigger editor when selecting package items
                showWarning(tr("This item is part of a package. To best preserve your changes, copy this item before editing as package upgrades may overwrite modifications."), false);

                // Announce full educational message only on first package item encountered
                static bool firstPackageAnnounced = false;
                if (!firstPackageAnnounced) {
                    //: First-time educational message for screen reader users about package items
                    mudlet::self()->announce(tr("Package item. Copy before editing to preserve changes."));
                    firstPackageAnnounced = true;
                }
            }
        }

    } else {
        // No details to show - as will be the case if the top item (ID = 0) is
        // selected - so show the help message:
        clearScriptForm();
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
        } else {
            // Show package warning if this item belongs to a package
            QString packageName = pT->packageName(pT);
            if (!packageName.isEmpty()) {
                // Update accessibility description for screen readers (appears after item name)
                QString currentDesc = pItem->data(0, Qt::AccessibleDescriptionRole).toString();
                updatePackageItemAccessibility(pItem, currentDesc);

                // Show visual warning banner (without screen reader announcement to avoid spam)
                //: Package item warning banner shown in trigger editor when selecting package items
                showWarning(tr("This item is part of a package. To best preserve your changes, copy this item before editing as package upgrades may overwrite modifications."), false);

                // Announce full educational message only on first package item encountered
                static bool firstPackageAnnounced = false;
                if (!firstPackageAnnounced) {
                    //: First-time educational message for screen reader users about package items
                    mudlet::self()->announce(tr("Package item. Copy before editing to preserve changes."));
                    firstPackageAnnounced = true;
                }
            }
        }
    } else {
        // No details to show - as will be the case if the top item (ID = 0) is
        // selected - so show the help message:
        clearTimerForm();
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
    treeWidget_triggers->setCurrentItem(mpTriggerBaseItem);

    mpTimerBaseItem = new QTreeWidgetItem(static_cast<QTreeWidgetItem*>(nullptr), QStringList(tr("Timers")));
    mpTimerBaseItem->setIcon(0, QPixmap(qsl(":/icons/chronometer.png")));
    treeWidget_timers->insertTopLevelItem(0, mpTimerBaseItem);
    populateTimers();
    mpTimerBaseItem->setExpanded(true);
    treeWidget_timers->setCurrentItem(mpTimerBaseItem);

    mpScriptsBaseItem = new QTreeWidgetItem(static_cast<QTreeWidgetItem*>(nullptr), QStringList(tr("Scripts")));
    mpScriptsBaseItem->setIcon(0, QPixmap(qsl(":/icons/accessories-text-editor.png")));
    treeWidget_scripts->insertTopLevelItem(0, mpScriptsBaseItem);
    populateScripts();
    mpScriptsBaseItem->setExpanded(true);
    treeWidget_scripts->setCurrentItem(mpScriptsBaseItem);

    mpAliasBaseItem = new QTreeWidgetItem(static_cast<QTreeWidgetItem*>(nullptr), QStringList(tr("Aliases - Input Triggers")));
    mpAliasBaseItem->setIcon(0, QPixmap(qsl(":/icons/system-users.png")));
    treeWidget_aliases->insertTopLevelItem(0, mpAliasBaseItem);
    populateAliases();
    mpAliasBaseItem->setExpanded(true);
    treeWidget_aliases->setCurrentItem(mpAliasBaseItem);

    mpActionBaseItem = new QTreeWidgetItem(static_cast<QTreeWidgetItem*>(nullptr), QStringList(tr("Buttons")));
    mpActionBaseItem->setIcon(0, QPixmap(qsl(":/icons/bookmarks.png")));
    treeWidget_actions->insertTopLevelItem(0, mpActionBaseItem);
    populateActions();
    mpActionBaseItem->setExpanded(true);
    treeWidget_actions->setCurrentItem(mpActionBaseItem);

    mpKeyBaseItem = new QTreeWidgetItem(static_cast<QTreeWidgetItem*>(nullptr), QStringList(tr("Key Bindings")));
    mpKeyBaseItem->setIcon(0, QPixmap(qsl(":/icons/preferences-desktop-keyboard.png")));
    treeWidget_keys->insertTopLevelItem(0, mpKeyBaseItem);
    populateKeys();
    mpKeyBaseItem->setExpanded(true);
    treeWidget_keys->setCurrentItem(mpKeyBaseItem);
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
    std::list<TTimer *> const baseNodeList_timers = mpHost->getTimerUnit()->getTimerRootNodeList();
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
    std::list<TTrigger *> const baseNodeList = mpHost->getTriggerUnit()->getTriggerRootNodeList();
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
    treeWidget_variables->setCurrentItem(mpVarBaseItem);

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

void dlgTriggerEditor::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event)

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
    Q_UNUSED(pE)

    saveOpenChanges();
}

void dlgTriggerEditor::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);

    // Always reposition the dialog to the correct screen when shown
    // This ensures it follows the active profile, especially after reattachment
    utils::positionDialogOnActiveProfileScreen(this, nullptr, mpHost->mpConsole);
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

    if (mpBannerUndoTimer && mpBannerUndoTimer->isActive()) {
        mpBannerUndoTimer->stop();
        mpBannerUndoTimer->deleteLater();
        mpBannerUndoTimer = nullptr;
    }

    if (bannerPermanentlyHidden(mCurrentView)) {
        hideSystemMessageArea();
    }

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

    const bool enablePatternShortcuts = view == EditorViewType::cmTriggerView;

    if (mFirstPatternShortcut) {
        mFirstPatternShortcut->setEnabled(enablePatternShortcuts);
    }
    for (auto* shortcut : mPatternNavigationShortcuts) {
        if (shortcut) {
            shortcut->setEnabled(enablePatternShortcuts);
        }
    }
    if (mLastPatternShortcut) {
        mLastPatternShortcut->setEnabled(enablePatternShortcuts);
    }

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
        clearTimerForm();
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
        clearTriggerForm();
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
        clearTriggerForm();
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
        clearKeyForm();
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
        clearVarForm();
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
            clearVarForm();
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
        clearAliasForm();
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

void dlgTriggerEditor::showError(const QString& text)
{
    mpSystemMessageArea->notificationAreaIconLabelInformation->hide();
    mpSystemMessageArea->notificationAreaIconLabelError->show();
    mpSystemMessageArea->notificationAreaIconLabelWarning->hide();
    mpSystemMessageArea->notificationAreaMessageBox->setText(text);
    mpSystemMessageArea->show();
    mCurrentBannerKey.clear();

    // Reconnect close button to normal hide behavior (not banner dismiss)
    disconnect(mpSystemMessageArea->messageAreaCloseButton, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_bannerDismissClicked);
    connect(mpSystemMessageArea->messageAreaCloseButton, &QAbstractButton::clicked, this, &dlgTriggerEditor::hideSystemMessageArea);

    if (!mpHost->mIsProfileLoadingSequence) {
        mudlet::self()->announce(text);
    }
}

void dlgTriggerEditor::showWarning(const QString& text, bool announce)
{
    mpSystemMessageArea->notificationAreaIconLabelInformation->hide();
    mpSystemMessageArea->notificationAreaIconLabelError->hide();
    mpSystemMessageArea->notificationAreaIconLabelWarning->show();
    mpSystemMessageArea->notificationAreaMessageBox->setText(text);
    mpSystemMessageArea->show();
    mCurrentBannerKey.clear();

    // Reconnect close button to normal hide behavior (not banner dismiss)
    disconnect(mpSystemMessageArea->messageAreaCloseButton, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_bannerDismissClicked);
    connect(mpSystemMessageArea->messageAreaCloseButton, &QAbstractButton::clicked, this, &dlgTriggerEditor::hideSystemMessageArea);

    if (!mpHost->mIsProfileLoadingSequence && announce) {
        mudlet::self()->announce(text);
    }
}

void dlgTriggerEditor::showInfo(const QString& text)
{
    mpSystemMessageArea->notificationAreaIconLabelError->hide();
    mpSystemMessageArea->notificationAreaIconLabelWarning->hide();
    mpSystemMessageArea->notificationAreaIconLabelInformation->show();
    mpSystemMessageArea->notificationAreaMessageBox->setText(text);
    mpSystemMessageArea->show();
    mCurrentBannerKey.clear();
    if (!mpHost->mIsProfileLoadingSequence) {
        mudlet::self()->announce(text);
    }
}

void dlgTriggerEditor::showIntro(const QString& desiredOption)
{
    if (!introAddItem.contains(mCurrentView)) {
        qWarning() << "ERROR: dlgTriggerEditor::showIntro() undefined view";
        return;
    }

    static const auto bannerKey = qsl("intro");
    bool includeBasePreference = true;
    if (mCurrentView == EditorViewType::cmTriggerView) {
        // The trigger intro banner predates the global suppression toggle, so keep
        // honouring only its explicit "hide permanently" preference to ensure it
        // still shows up for profiles that never opted out directly.
        includeBasePreference = false;
    }

    if (bannerPermanentlyHidden(mCurrentView, bannerKey, includeBasePreference)) {
        return;
    }

    introTextParts introAddCurrentItem = introAddItem.value(mCurrentView);
    QString introTextOptions;
    for (const auto &[name, headline, contents] : introAddCurrentItem.options) {
        introTextOptions.append(
            (name != desiredOption)
            ? qsl("<li><a href='%1' style='color: inherit; text-decoration: underline;'>%2</a></li>").arg(name, headline)
            : qsl("<li><strong>%1</strong>%2</li>").arg(headline, contents));
    }

    QString content = qsl("<p>%1</p><ul>%2</ul>")
        .arg(introAddCurrentItem.summary, introTextOptions);

    showHideableBanner(content, bannerKey);
}

void dlgTriggerEditor::showHideableBanner(const QString& content, const QString& bannerKey)
{
    if (!mpSystemMessageArea) {
        return;
    }

    const QString settingsKey = bannerSettingsKey(mCurrentView, bannerKey);
    const QString baseKey = bannerSettingsKey(mCurrentView, QString());
    if (settingsKey.isEmpty()) {
        return;
    }

    if (mTemporarilyHiddenBanners.contains(settingsKey) || (!bannerKey.isEmpty() && mTemporarilyHiddenBanners.contains(baseKey))) {
        return;
    }

    bool includeBasePreference = true;
    if (mCurrentView == EditorViewType::cmTriggerView && bannerKey == qsl("intro")) {
        // Match the behaviour in showIntro(): ignore the view-wide suppression
        // switch so the legacy trigger intro reappears unless it was hidden via
        // its own banner controls.
        includeBasePreference = false;
    }

    if (bannerPermanentlyHidden(mCurrentView, bannerKey, includeBasePreference)) {
        return;
    }

    if (mpSystemMessageArea->isVisible() && mCurrentBannerKey != bannerKey
        && !mpSystemMessageArea->notificationAreaMessageBox->text().isEmpty()) {
        return;
    }

    if (mpSystemMessageArea->isVisible() && mCurrentBannerKey == bannerKey
        && mpSystemMessageArea->notificationAreaMessageBox->text() == content) {
        return;
    }

    disconnect(mpSystemMessageArea->messageAreaCloseButton, &QAbstractButton::clicked, this, &dlgTriggerEditor::hideSystemMessageArea);
    disconnect(mpSystemMessageArea->messageAreaCloseButton, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_bannerDismissClicked);
    connect(mpSystemMessageArea->messageAreaCloseButton, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_bannerDismissClicked);

    disconnect(mpSystemMessageArea->notificationAreaMessageBox, &QLabel::linkActivated, nullptr, nullptr);
    connect(mpSystemMessageArea->notificationAreaMessageBox, &QLabel::linkActivated, this, &dlgTriggerEditor::slot_clickedMessageBox);

    showInfo(content);
    mCurrentBannerKey = bannerKey;
}

QString dlgTriggerEditor::bannerSettingsKey(EditorViewType viewType, const QString& bannerKey) const
{
    const QString legacyKey = legacyBannerSettingsKey(viewType, bannerKey);
    if (legacyKey.isEmpty()) {
        return legacyKey;
    }

    const QString prefix = profileSettingsPrefix();
    if (prefix.isEmpty()) {
        return legacyKey;
    }

    return qsl("%1/%2").arg(prefix, legacyKey);
}

QString dlgTriggerEditor::legacyBannerSettingsKey(EditorViewType viewType, const QString& bannerKey) const
{
    const QMetaEnum metaEnum = QMetaEnum::fromType<EditorViewType>();
    const char* enumName = metaEnum.valueToKey(static_cast<int>(viewType));

    if (!enumName) {
        return QString();
    }

    QString key = QString::fromLatin1(enumName).toLower();
    if (!bannerKey.isEmpty()) {
        key += qsl("/%1").arg(bannerKey);
    }

    return key;
}

QString dlgTriggerEditor::profileSettingsPrefix() const
{
    if (!mpHost) {
        return QString();
    }

    const QString profileName = mpHost->getName();
    if (profileName.isEmpty()) {
        return QString();
    }

    const QString sanitized = utils::sanitizeForPath(profileName);
    if (sanitized.isEmpty()) {
        return QString();
    }

    return qsl("profiles/%1").arg(sanitized);
}

QString dlgTriggerEditor::patternNavigationHintSettingsKey() const
{
    const QString prefix = profileSettingsPrefix();
    if (prefix.isEmpty()) {
        return qsl("patternNavigationHintHidden");
    }

    return qsl("%1/patternNavigationHintHidden").arg(prefix);
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
        addTrigger(false); //add normal trigger
        mpTriggersMainArea->lineEdit_trigger_name->setFocus();
        mpTriggersMainArea->lineEdit_trigger_name->selectAll();
        break;
    case EditorViewType::cmTimerView:
        addTimer(false); //add normal timer
        mpTimersMainArea->lineEdit_timer_name->setFocus();
        mpTimersMainArea->lineEdit_timer_name->selectAll();
        break;
    case EditorViewType::cmAliasView:
        addAlias(false); //add normal alias
        mpAliasMainArea->lineEdit_alias_name->setFocus();
        mpAliasMainArea->lineEdit_alias_name->selectAll();
        break;
    case EditorViewType::cmScriptView:
        addScript(false); //add normal script
        mpScriptsMainArea->lineEdit_script_name->setFocus();
        mpScriptsMainArea->lineEdit_script_name->selectAll();
        break;
    case EditorViewType::cmActionView:
        addAction(false); //add normal action
        mpActionsMainArea->lineEdit_action_name->setFocus();
        mpActionsMainArea->lineEdit_action_name->selectAll();
        break;
    case EditorViewType::cmKeysView:
        addKey(false); //add normal key
        mpKeysMainArea->lineEdit_key_name->setFocus();
        mpKeysMainArea->lineEdit_key_name->selectAll();
        break;
    case EditorViewType::cmVarsView:
        addVar(false); //add variable
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
        addTrigger(true); //add trigger group
        mpTriggersMainArea->lineEdit_trigger_name->setFocus();
        mpTriggersMainArea->lineEdit_trigger_name->selectAll();
        break;
    case EditorViewType::cmTimerView:
        addTimer(true); //add timer group
        mpTimersMainArea->lineEdit_timer_name->setFocus();
        mpTimersMainArea->lineEdit_timer_name->selectAll();
        break;
    case EditorViewType::cmAliasView:
        addAlias(true); //add alias group
        mpAliasMainArea->lineEdit_alias_name->setFocus();
        mpAliasMainArea->lineEdit_alias_name->selectAll();
        break;
    case EditorViewType::cmScriptView:
        addScript(true); //add script group
        mpScriptsMainArea->lineEdit_script_name->setFocus();
        mpScriptsMainArea->lineEdit_script_name->selectAll();
        break;
    case EditorViewType::cmActionView:
        addAction(true); //add action group
        mpActionsMainArea->lineEdit_action_name->setFocus();
        mpActionsMainArea->lineEdit_action_name->selectAll();
        break;
    case EditorViewType::cmKeysView:
        addKey(true); //add keys group
        mpKeysMainArea->lineEdit_key_name->setFocus();
        mpKeysMainArea->lineEdit_key_name->selectAll();
        break;
    case EditorViewType::cmVarsView:
        addVar(true); // add lua table
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
    for (size_t i = 0; i < controller->textSelection()->rangeCount(); i++) {
        auto &range = controller->textSelection()->range(i);
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
void dlgTriggerEditor::slot_scriptMainAreaEditHandler()
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

void dlgTriggerEditor::slot_scriptMainAreaClearHandlerSelection(QListWidgetItem* item)
{
    Q_UNUSED(item)
    mpScriptsMainArea->listWidget_script_registered_event_handlers->clearSelection();
    mpScriptsMainArea->lineEdit_script_event_handler_entry->clear();
    mIsScriptsMainAreaEditHandler = false;
    mpScriptsMainAreaEditHandlerItem = nullptr;
}

void dlgTriggerEditor::slot_scriptMainAreaDeleteHandler()
{
    mpScriptsMainArea->listWidget_script_registered_event_handlers->takeItem(mpScriptsMainArea->listWidget_script_registered_event_handlers->currentRow());
    slot_scriptMainAreaClearHandlerSelection(nullptr);
}

void dlgTriggerEditor::slot_scriptMainAreaAddHandler()
{
    auto addEventHandler = [&] () {
        if (mpScriptsMainArea->lineEdit_script_event_handler_entry->text().isEmpty()) {
            return;
        }

        // check for duplicate handlers
        QString newHandlerText = mpScriptsMainArea->lineEdit_script_event_handler_entry->text();
        QListWidget* list = mpScriptsMainArea->listWidget_script_registered_event_handlers;
        for (int i = 0; i < list->count(); i++) {
            if (list->item(i)->text() == newHandlerText) {
                return;
            }
        }

        auto pItem = new QListWidgetItem;
        pItem->setText(newHandlerText);
        mpScriptsMainArea->listWidget_script_registered_event_handlers->addItem(pItem);
    };

    mpScriptsMainArea->trimEventHandlerName();
    if (mIsScriptsMainAreaEditHandler) {
        if (!mpScriptsMainAreaEditHandlerItem) {
            mIsScriptsMainAreaEditHandler = false;
            addEventHandler();
        } else {
            if (mpScriptsMainAreaEditHandlerItem->text() == mpScriptsMainArea->lineEdit_script_event_handler_entry->text()
            || mpScriptsMainArea->lineEdit_script_event_handler_entry->text().isEmpty()) {
                return;
            }
            mpScriptsMainAreaEditHandlerItem->setText(mpScriptsMainArea->lineEdit_script_event_handler_entry->text());
            mpScriptsMainArea->listWidget_script_registered_event_handlers->clearSelection();
        }
    } else {
        addEventHandler();
    }

    slot_scriptMainAreaClearHandlerSelection(nullptr);
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
            mTriggerPatternEdit[0]->singleLineTextEdit_pattern->setFocus();
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
                mTriggerPatternEdit[0]->singleLineTextEdit_pattern->setFocus();
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
            mTriggerPatternEdit[0]->singleLineTextEdit_pattern->setFocus();
            return;
        }
        if (treeWidget_triggers->hasFocus()) {
            mpSourceEditorEdbee->setFocus();
            return;
        }
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
        for (auto child : mpTimersMainArea->findChildren<QWidget *>()) {
            if (child->hasFocus()){
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
        for (auto child : mpAliasMainArea->findChildren<QWidget *>()) {
            if (child->hasFocus()){
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
        for (auto child : mpScriptsMainArea->findChildren<QWidget *>()) {
            if (child->hasFocus()){
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
        for (auto child : mpActionsMainArea->findChildren<QWidget *>()) {
            if (child->hasFocus()){
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
        for (auto child : mpKeysMainArea->findChildren<QWidget *>()) {
            if (child->hasFocus()){
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
        for (auto child : mpVarsMainArea->findChildren<QWidget *>()) {
            if (child->hasFocus()){
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
    QList<QTreeWidgetItem*> selectedItems = treeWidget_triggers->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
        return;
    }

    QStringList triggerNames;
    QList<TTrigger*> triggersToExport;

    for (QTreeWidgetItem* pItem : selectedItems) {
        const int triggerID = pItem->data(0, Qt::UserRole).toInt();
        TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(triggerID);
        if (pT) {
            triggerNames << pT->getName();
            triggersToExport << pT;
        }
    }

    if (triggersToExport.isEmpty()) {
        QMessageBox::warning(this, tr("Export Package:"), tr("No valid triggers found to export."));
        return;
    }

    if (triggersToExport.size() == 1) {
        // Single item - use existing method
        XMLexport writer(triggersToExport.first());
        writer.exportToClipboard(triggersToExport.first());
        statusBar()->showMessage(tr("Copied %1 to clipboard").arg(triggerNames.first().toHtmlEscaped()), 2000);
    } else {
        // Multiple items - export them individually and let user paste multiple times
        exportMultipleTriggersToClipboard(triggersToExport);
        statusBar()->showMessage(tr("Copied %1 triggers to clipboard").arg(triggersToExport.size()), 2000);
    }
}

void dlgTriggerEditor::exportMultipleTriggersToClipboard(const QList<TTrigger*>& triggers)
{
    if (triggers.isEmpty()) {
        return;
    }

    // Store multiple XML packages separated by a special delimiter
    // This allows the paste function to split and import each item individually
    QStringList xmlPackages;

    for (TTrigger* trigger : triggers) {
        XMLexport writer(trigger);

        // Get the XML for this trigger by temporarily using the clipboard
        QString originalClipboard = QApplication::clipboard()->text();
        writer.exportToClipboard(trigger);
        QString triggerXml = QApplication::clipboard()->text();
        QApplication::clipboard()->setText(originalClipboard);

        xmlPackages << triggerXml;
    }

    // Combine all XML packages with a special separator that paste can recognize
    QString combinedXml = xmlPackages.join("\n<!--MUDLET_MULTI_ITEM_SEPARATOR-->\n");
    QApplication::clipboard()->setText(combinedXml);
}

void dlgTriggerEditor::exportTimerToClipboard()
{
    QList<QTreeWidgetItem*> selectedItems = treeWidget_timers->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
        return;
    }

    QStringList timerNames;
    QList<TTimer*> timersToExport;

    for (QTreeWidgetItem* pItem : selectedItems) {
        const int timerID = pItem->data(0, Qt::UserRole).toInt();
        TTimer* pT = mpHost->getTimerUnit()->getTimer(timerID);
        if (pT) {
            timerNames << pT->getName();
            timersToExport << pT;
        }
    }

    if (timersToExport.isEmpty()) {
        QMessageBox::warning(this, tr("Export Package:"), tr("No valid timers found to export."));
        return;
    }

    if (timersToExport.size() == 1) {
        XMLexport writer(timersToExport.first());
        writer.exportToClipboard(timersToExport.first());
        statusBar()->showMessage(tr("Copied %1 to clipboard").arg(timerNames.first().toHtmlEscaped()), 2000);
    } else {
        exportMultipleTimersToClipboard(timersToExport);
        statusBar()->showMessage(tr("Copied %1 timers to clipboard").arg(timersToExport.size()), 2000);
    }
}

void dlgTriggerEditor::exportMultipleTimersToClipboard(const QList<TTimer*>& timers)
{
    if (timers.isEmpty()) {
        return;
    }

    QStringList xmlParts;

    for (TTimer* timer : timers) {
        XMLexport writer(timer);
        QString originalClipboard = QApplication::clipboard()->text();
        writer.exportToClipboard(timer);
        QString timerXml = QApplication::clipboard()->text();
        xmlParts << timerXml;
        QApplication::clipboard()->setText(originalClipboard);
    }

    QString combinedXml = xmlParts.join("\n");
    QApplication::clipboard()->setText(combinedXml);
}

void dlgTriggerEditor::exportAliasToClipboard()
{
    QList<QTreeWidgetItem*> selectedItems = treeWidget_aliases->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
        return;
    }

    QStringList aliasNames;
    QList<TAlias*> aliasesToExport;

    for (QTreeWidgetItem* pItem : selectedItems) {
        const int aliasID = pItem->data(0, Qt::UserRole).toInt();
        TAlias* pT = mpHost->getAliasUnit()->getAlias(aliasID);
        if (pT) {
            aliasNames << pT->getName();
            aliasesToExport << pT;
        }
    }

    if (aliasesToExport.isEmpty()) {
        QMessageBox::warning(this, tr("Export Package:"), tr("No valid aliases found to export."));
        return;
    }

    if (aliasesToExport.size() == 1) {
        XMLexport writer(aliasesToExport.first());
        writer.exportToClipboard(aliasesToExport.first());
        statusBar()->showMessage(tr("Copied %1 to clipboard").arg(aliasNames.first().toHtmlEscaped()), 2000);
    } else {
        exportMultipleAliasesToClipboard(aliasesToExport);
        statusBar()->showMessage(tr("Copied %1 aliases to clipboard").arg(aliasesToExport.size()), 2000);
    }
}

void dlgTriggerEditor::exportMultipleAliasesToClipboard(const QList<TAlias*>& aliases)
{
    if (aliases.isEmpty()) {
        return;
    }

    QStringList xmlParts;

    for (TAlias* alias : aliases) {
        XMLexport writer(alias);
        QString originalClipboard = QApplication::clipboard()->text();
        writer.exportToClipboard(alias);
        QString aliasXml = QApplication::clipboard()->text();
        xmlParts << aliasXml;
        QApplication::clipboard()->setText(originalClipboard);
    }

    QString combinedXml = xmlParts.join("\n");
    QApplication::clipboard()->setText(combinedXml);
}

void dlgTriggerEditor::exportActionToClipboard()
{
    QList<QTreeWidgetItem*> selectedItems = treeWidget_actions->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
        return;
    }

    QStringList actionNames;
    QList<TAction*> actionsToExport;

    for (QTreeWidgetItem* pItem : selectedItems) {
        const int actionID = pItem->data(0, Qt::UserRole).toInt();
        TAction* pT = mpHost->getActionUnit()->getAction(actionID);
        if (pT) {
            actionNames << pT->getName();
            actionsToExport << pT;
        }
    }

    if (actionsToExport.isEmpty()) {
        QMessageBox::warning(this, tr("Export Package:"), tr("No valid actions found to export."));
        return;
    }

    if (actionsToExport.size() == 1) {
        XMLexport writer(actionsToExport.first());
        writer.exportToClipboard(actionsToExport.first());
        statusBar()->showMessage(tr("Copied %1 to clipboard").arg(actionNames.first().toHtmlEscaped()), 2000);
    } else {
        exportMultipleActionsToClipboard(actionsToExport);
        statusBar()->showMessage(tr("Copied %1 actions to clipboard").arg(actionsToExport.size()), 2000);
    }
}

void dlgTriggerEditor::exportMultipleActionsToClipboard(const QList<TAction*>& actions)
{
    if (actions.isEmpty()) {
        return;
    }

    QStringList xmlParts;

    for (TAction* action : actions) {
        XMLexport writer(action);
        QString originalClipboard = QApplication::clipboard()->text();
        writer.exportToClipboard(action);
        QString actionXml = QApplication::clipboard()->text();
        xmlParts << actionXml;
        QApplication::clipboard()->setText(originalClipboard);
    }

    QString combinedXml = xmlParts.join("\n");
    QApplication::clipboard()->setText(combinedXml);
}

void dlgTriggerEditor::exportScriptToClipboard()
{
    QList<QTreeWidgetItem*> selectedItems = treeWidget_scripts->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
        return;
    }

    QStringList scriptNames;
    QList<TScript*> scriptsToExport;

    for (QTreeWidgetItem* pItem : selectedItems) {
        const int scriptID = pItem->data(0, Qt::UserRole).toInt();
        TScript* pT = mpHost->getScriptUnit()->getScript(scriptID);
        if (pT) {
            scriptNames << pT->getName();
            scriptsToExport << pT;
        }
    }

    if (scriptsToExport.isEmpty()) {
        QMessageBox::warning(this, tr("Export Package:"), tr("No valid scripts found to export."));
        return;
    }

    if (scriptsToExport.size() == 1) {
        XMLexport writer(scriptsToExport.first());
        writer.exportToClipboard(scriptsToExport.first());
        statusBar()->showMessage(tr("Copied %1 to clipboard").arg(scriptNames.first().toHtmlEscaped()), 2000);
    } else {
        exportMultipleScriptsToClipboard(scriptsToExport);
        statusBar()->showMessage(tr("Copied %1 scripts to clipboard").arg(scriptsToExport.size()), 2000);
    }
}

void dlgTriggerEditor::exportMultipleScriptsToClipboard(const QList<TScript*>& scripts)
{
    if (scripts.isEmpty()) {
        return;
    }

    QStringList xmlParts;

    for (TScript* script : scripts) {
        XMLexport writer(script);
        QString originalClipboard = QApplication::clipboard()->text();
        writer.exportToClipboard(script);
        QString scriptXml = QApplication::clipboard()->text();
        xmlParts << scriptXml;
        QApplication::clipboard()->setText(originalClipboard);
    }

    QString combinedXml = xmlParts.join("\n");
    QApplication::clipboard()->setText(combinedXml);
}

void dlgTriggerEditor::exportKeyToClipboard()
{
    QList<QTreeWidgetItem*> selectedItems = treeWidget_keys->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
        return;
    }

    QStringList keyNames;
    QList<TKey*> keysToExport;

    for (QTreeWidgetItem* pItem : selectedItems) {
        const int keyID = pItem->data(0, Qt::UserRole).toInt();
        TKey* pT = mpHost->getKeyUnit()->getKey(keyID);
        if (pT) {
            keyNames << pT->getName();
            keysToExport << pT;
        }
    }

    if (keysToExport.isEmpty()) {
        QMessageBox::warning(this, tr("Export Package:"), tr("No valid keys found to export."));
        return;
    }

    if (keysToExport.size() == 1) {
        XMLexport writer(keysToExport.first());
        writer.exportToClipboard(keysToExport.first());
        statusBar()->showMessage(tr("Copied %1 to clipboard").arg(keyNames.first().toHtmlEscaped()), 2000);
    } else {
        exportMultipleKeysToClipboard(keysToExport);
        statusBar()->showMessage(tr("Copied %1 keys to clipboard").arg(keysToExport.size()), 2000);
    }
}

void dlgTriggerEditor::exportMultipleKeysToClipboard(const QList<TKey*>& keys)
{
    if (keys.isEmpty()) {
        return;
    }

    QStringList xmlParts;

    for (TKey* key : keys) {
        XMLexport writer(key);
        QString originalClipboard = QApplication::clipboard()->text();
        writer.exportToClipboard(key);
        QString keyXml = QApplication::clipboard()->text();
        xmlParts << keyXml;
        QApplication::clipboard()->setText(originalClipboard);
    }

    QString combinedXml = xmlParts.join("\n");
    QApplication::clipboard()->setText(combinedXml);
}

void dlgTriggerEditor::slot_export()
{
    if (mCurrentView == EditorViewType::cmUnknownView || mCurrentView == EditorViewType::cmVarsView) {
        return;
    }

    QSettings& settings = *mudlet::getQSettings();
    QString lastDir = settings.value("lastFileDialogLocation", QDir::homePath()).toString();

    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Item"), lastDir, tr("Mudlet packages (*.xml)"));
    if (fileName.isEmpty()) {
        return;
    }

    lastDir = QFileInfo(fileName).absolutePath();
    settings.setValue("lastFileDialogLocation", lastDir);

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

void dlgTriggerEditor::slot_createModule()
{
    if (mCurrentView == EditorViewType::cmUnknownView || mCurrentView == EditorViewType::cmVarsView) {
        return;
    }

    // Open the package exporter dialog with module creation mode
    auto* packageExporter = new dlgPackageExporter(this, mpHost);

    // Pre-select the current item for export
    switch (mCurrentView) {
    case EditorViewType::cmTriggerView:
        if (mpCurrentTriggerItem) {
            packageExporter->preselectTrigger(mpCurrentTriggerItem);
        }
        break;
    case EditorViewType::cmTimerView:
        if (mpCurrentTimerItem) {
            packageExporter->preselectTimer(mpCurrentTimerItem);
        }
        break;
    case EditorViewType::cmAliasView:
        if (mpCurrentAliasItem) {
            packageExporter->preselectAlias(mpCurrentAliasItem);
        }
        break;
    case EditorViewType::cmScriptView:
        if (mpCurrentScriptItem) {
            packageExporter->preselectScript(mpCurrentScriptItem);
        }
        break;
    case EditorViewType::cmActionView:
        if (mpCurrentActionItem) {
            packageExporter->preselectAction(mpCurrentActionItem);
        }
        break;
    case EditorViewType::cmKeysView:
        if (mpCurrentKeyItem) {
            packageExporter->preselectKey(mpCurrentKeyItem);
        }
        break;
    default:
        break;
    }

    // Set module creation mode
    packageExporter->setModuleCreationMode(true);
    packageExporter->show();
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

    // Check if clipboard contains multiple items (separated by our delimiter)
    QString clipboardText = QApplication::clipboard()->text();
    QStringList xmlPackages = clipboardText.split("\n<!--MUDLET_MULTI_ITEM_SEPARATOR-->\n");

    EditorViewType importedItemType;
    int importedItemID;

    if (xmlPackages.size() > 1) {
        // Multiple items detected - import each one individually
        QList<int> importedIDs;
        EditorViewType firstImportType = EditorViewType::cmUnknownView;

        QString originalClipboard = QApplication::clipboard()->text();

        for (const QString& xmlItem : xmlPackages) {
            QString xmlItemTrimmed = xmlItem.trimmed();
            if (xmlItemTrimmed.isEmpty()) {
                continue; // Skip empty items
            }

            // Temporarily set clipboard to single item
            QApplication::clipboard()->setText(xmlItemTrimmed);

            // Import this single item
            XMLimport itemReader(mpHost);
            auto [itemType, itemID] = itemReader.importFromClipboard();

            if (itemType != EditorViewType::cmUnknownView && itemID != 0) {
                importedIDs << itemID;
                if (firstImportType == EditorViewType::cmUnknownView) {
                    firstImportType = itemType;
                }
            }
        }

        // Restore original clipboard once at the end
        QApplication::clipboard()->setText(originalClipboard);

        if (!importedIDs.isEmpty()) {
            // For multiple items, we need to handle the reparenting here instead of later
            // since the later logic only handles one item at a time
            if (firstImportType == EditorViewType::cmTriggerView) {
                QModelIndex targetIndex = treeWidget_triggers->currentIndex();
                if (!targetIndex.isValid()) {
                    QList<QTreeWidgetItem*> selectedItems = treeWidget_triggers->selectedItems();
                    if (!selectedItems.isEmpty()) {
                        targetIndex = treeWidget_triggers->indexFromItem(selectedItems.first());
                    }
                }

                // Apply the same group detection logic for all imported triggers
                if (targetIndex.isValid()) {
                    QTreeWidgetItem* targetItem = treeWidget_triggers->itemFromIndex(targetIndex);
                    int targetId = targetIndex.data(Qt::UserRole).toInt();
                    TTrigger* targetTrigger = mpHost->getTriggerUnit()->getTrigger(targetId);

                    bool isGroup = (targetItem && targetItem->childCount() > 0) ||
                                  (targetTrigger && targetTrigger->isFolder());

                    for (int itemID : importedIDs) {
                        if (isGroup) {
                            mpHost->getTriggerUnit()->reParentTrigger(itemID, 0, targetId, -1, -1);
                        } else {
                            auto parent = targetIndex.parent();
                            auto parentId = parent.data(Qt::UserRole).toInt();
                            mpHost->getTriggerUnit()->reParentTrigger(itemID, 0, parentId, -1, -1);
                        }
                    }
                }
            }

            // Use the first imported item's type and ID for the rest of the function
            importedItemType = firstImportType;
            importedItemID = importedIDs.first();

            statusBar()->showMessage(tr("Pasted %1 items successfully").arg(importedIDs.size()), 3000);
        } else {
            return;
        }
    } else {
        // Single item - use original import method
        auto [itemType, itemID] = reader.importFromClipboard();
        importedItemType = itemType;
        importedItemID = itemID;

        // don't reset the view if what we pasted wasn't a Mudlet editor item
        if (importedItemType == EditorViewType::cmUnknownView && importedItemID == 0) {
            return;
        }
    }

    mCurrentView = static_cast<EditorViewType>(importedItemType);
    // importing drops the item at the bottom of the list - move it to be a sibling
    // of the currently selected item instead
    switch (mCurrentView) {
    case EditorViewType::cmTriggerView: {
        // Handle multi-selection: use the first selected item as reference
        QModelIndex targetIndex = treeWidget_triggers->currentIndex();
        if (!targetIndex.isValid()) {
            // If no current index, try to get from selected items
            QList<QTreeWidgetItem*> selectedItems = treeWidget_triggers->selectedItems();
            if (!selectedItems.isEmpty()) {
                targetIndex = treeWidget_triggers->indexFromItem(selectedItems.first());
            }
        }

        if (targetIndex.isValid()) {
            // Check if the selected item is a trigger group/folder
            QTreeWidgetItem* targetItem = treeWidget_triggers->itemFromIndex(targetIndex);
            int targetId = targetIndex.data(Qt::UserRole).toInt();
            TTrigger* targetTrigger = mpHost->getTriggerUnit()->getTrigger(targetId);

            // Check if target is a group/folder (has children OR is a group trigger)
            bool isGroup = (targetItem && targetItem->childCount() > 0) ||
                          (targetTrigger && targetTrigger->isFolder());

            if (isGroup) {
                // Paste INSIDE the selected group/folder
                mpHost->getTriggerUnit()->reParentTrigger(importedItemID, 0, targetId, -1, -1);
            } else {
                // Paste as sibling next to the selected item
                auto parent = targetIndex.parent();
                auto parentRow = parent.row();
                auto parentId = parent.data(Qt::UserRole).toInt();

                const int siblingRow = targetIndex.row() + 1;
                mpHost->getTriggerUnit()->reParentTrigger(importedItemID, 0, parentId, parentRow, siblingRow);
            }
        } else {
            // If no valid target, place at the root level
            mpHost->getTriggerUnit()->reParentTrigger(importedItemID, 0, 0, -1, -1);
        }
        break;
    }
    case EditorViewType::cmTimerView: {
        QModelIndex targetIndex = treeWidget_timers->currentIndex();
        if (!targetIndex.isValid()) {
            QList<QTreeWidgetItem*> selectedItems = treeWidget_timers->selectedItems();
            if (!selectedItems.isEmpty()) {
                targetIndex = treeWidget_timers->indexFromItem(selectedItems.first());
            }
        }

        if (targetIndex.isValid()) {
            auto parent = targetIndex.parent();
            auto parentRow = parent.row();
            auto parentId = parent.data(Qt::UserRole).toInt();

            const int siblingRow = targetIndex.row() + 1;
            mpHost->getTimerUnit()->reParentTimer(importedItemID, 0, parentId, parentRow, siblingRow);
        } else {
            mpHost->getTimerUnit()->reParentTimer(importedItemID, 0, 0, -1, -1);
        }
        break;
    }
    case EditorViewType::cmAliasView: {
        QModelIndex targetIndex = treeWidget_aliases->currentIndex();
        if (!targetIndex.isValid()) {
            QList<QTreeWidgetItem*> selectedItems = treeWidget_aliases->selectedItems();
            if (!selectedItems.isEmpty()) {
                targetIndex = treeWidget_aliases->indexFromItem(selectedItems.first());
            }
        }

        if (targetIndex.isValid()) {
            auto parent = targetIndex.parent();
            auto parentRow = parent.row();
            auto parentId = parent.data(Qt::UserRole).toInt();

            const int siblingRow = targetIndex.row() + 1;
            mpHost->getAliasUnit()->reParentAlias(importedItemID, 0, parentId, parentRow, siblingRow);
        } else {
            mpHost->getAliasUnit()->reParentAlias(importedItemID, 0, 0, -1, -1);
        }
        break;
    }
    case EditorViewType::cmScriptView: {
        QModelIndex targetIndex = treeWidget_scripts->currentIndex();
        if (!targetIndex.isValid()) {
            QList<QTreeWidgetItem*> selectedItems = treeWidget_scripts->selectedItems();
            if (!selectedItems.isEmpty()) {
                targetIndex = treeWidget_scripts->indexFromItem(selectedItems.first());
            }
        }

        if (targetIndex.isValid()) {
            auto parent = targetIndex.parent();
            auto parentRow = parent.row();
            auto parentId = parent.data(Qt::UserRole).toInt();

            const int siblingRow = targetIndex.row() + 1;
            mpHost->getScriptUnit()->reParentScript(importedItemID, 0, parentId, parentRow, siblingRow);
        } else {
            mpHost->getScriptUnit()->reParentScript(importedItemID, 0, 0, -1, -1);
        }
        break;
    }
    case EditorViewType::cmActionView: {
        QModelIndex targetIndex = treeWidget_actions->currentIndex();
        if (!targetIndex.isValid()) {
            QList<QTreeWidgetItem*> selectedItems = treeWidget_actions->selectedItems();
            if (!selectedItems.isEmpty()) {
                targetIndex = treeWidget_actions->indexFromItem(selectedItems.first());
            }
        }

        if (targetIndex.isValid()) {
            auto parent = targetIndex.parent();
            auto parentRow = parent.row();
            auto parentId = parent.data(Qt::UserRole).toInt();

            const int siblingRow = targetIndex.row() + 1;
            mpHost->getActionUnit()->reParentAction(importedItemID, 0, parentId, parentRow, siblingRow);
        } else {
            mpHost->getActionUnit()->reParentAction(importedItemID, 0, 0, -1, -1);
        }
        break;
    }
    case EditorViewType::cmKeysView: {
        QModelIndex targetIndex = treeWidget_keys->currentIndex();
        if (!targetIndex.isValid()) {
            QList<QTreeWidgetItem*> selectedItems = treeWidget_keys->selectedItems();
            if (!selectedItems.isEmpty()) {
                targetIndex = treeWidget_keys->indexFromItem(selectedItems.first());
            }
        }

        if (targetIndex.isValid()) {
            auto parent = targetIndex.parent();
            auto parentRow = parent.row();
            auto parentId = parent.data(Qt::UserRole).toInt();

            const int siblingRow = targetIndex.row() + 1;
            mpHost->getKeyUnit()->reParentKey(importedItemID, 0, parentId, parentRow, siblingRow);
        } else {
            mpHost->getKeyUnit()->reParentKey(importedItemID, 0, 0, -1, -1);
        }
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

    QSettings& settings = *mudlet::getQSettings();
    QString lastDir = settings.value("lastFileDialogLocation", QDir::homePath()).toString();
    const QString fileName = QFileDialog::getOpenFileName(this, tr("Import Mudlet Package"), lastDir);
    if (fileName.isEmpty()) {
        return;
    }
    lastDir = QFileInfo(fileName).absolutePath();
    settings.setValue("lastFileDialogLocation", lastDir);

    mpHost->installPackage(fileName, enums::PackageModuleType::Package);

    treeWidget_triggers->clear();
    treeWidget_aliases->clear();
    treeWidget_actions->clear();
    treeWidget_timers->clear();
    treeWidget_keys->clear();
    treeWidget_scripts->clear();

    // Nullify current item pointers before saving to prevent use-after-free
    mpCurrentTriggerItem = nullptr;
    mpCurrentTimerItem = nullptr;
    mpCurrentAliasItem = nullptr;
    mpCurrentScriptItem = nullptr;
    mpCurrentActionItem = nullptr;
    mpCurrentKeyItem = nullptr;

    slot_profileSaveAction();

    fillout_form();

    slot_showTriggers();
}

void dlgTriggerEditor::doCleanReset()
{
    if (mCleanResetQueued) {
        return;
    }

    mCleanResetQueued = true;

    QTimer::singleShot(0, this, [=, this]() {
        mCleanResetQueued = false;

        runScheduledCleanReset();
    });
}

void dlgTriggerEditor::runScheduledCleanReset()
{
    // Clear all current item pointers BEFORE attempting to save or clear tree widgets
    // to prevent heap-use-after-free when the tree widgets are cleared
    mpCurrentTriggerItem = nullptr;
    mpCurrentTimerItem = nullptr;
    mpCurrentAliasItem = nullptr;
    mpCurrentScriptItem = nullptr;
    mpCurrentActionItem = nullptr;
    mpCurrentKeyItem = nullptr;

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
    slot_showTriggers();
}

void dlgTriggerEditor::slot_profileSaveAction()
{
    slot_saveEdits();

    auto [ok, filename, error] = mpHost->saveProfile(nullptr, nullptr, true);

    if (!ok) {
        QMessageBox::critical(this, tr("Couldn't save profile"), tr("Sorry, couldn't save your profile - got the following error: %1").arg(error));
    }
}

void dlgTriggerEditor::slot_profileSaveAsAction()
{
    mSavingAs = true;

    QSettings& settings = *mudlet::getQSettings();
    QString lastDir = settings.value("lastFileDialogLocation", QDir::homePath()).toString();
    QString fileName = QFileDialog::getSaveFileName(this, tr("Backup Profile"), lastDir, tr("trigger files (*.trigger *.xml)"));

    if (fileName.isEmpty()) {
        return;
    }
    lastDir = QFileInfo(fileName).absolutePath();
    settings.setValue("lastFileDialogLocation", lastDir);

    // Must be case insensitive to work on MacOS platforms, possibly a cause of
    // https://bugs.launchpad.net/mudlet/+bug/1417234
    if (!fileName.endsWith(qsl(".xml"), Qt::CaseInsensitive) && !fileName.endsWith(qsl(".trigger"), Qt::CaseInsensitive)) {
        fileName.append(qsl(".xml"));
    }
    slot_saveEdits();

    mpHost->saveProfileAs(fileName);
    mSavingAs = false;
}

bool dlgTriggerEditor::eventFilter(QObject* watched, QEvent* event)
{
    if (mIsGrabKey) {
        if (event->type() == QEvent::KeyPress) {
            auto* keyEvent = static_cast<QKeyEvent*>(event);
            switch (keyEvent->key()) {
            case Qt::Key_Up:
            case Qt::Key_Down:
            case Qt::Key_Left:
            case Qt::Key_Right:
            case Qt::Key_Escape:
                this->event(event);
                return true;
            default:
                return false;
            }
        }
        return false;
    }

    if (event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        const Qt::KeyboardModifiers modifiers = keyEvent->modifiers();
        const Qt::KeyboardModifiers additionalModifiers = modifiers & (Qt::ShiftModifier | Qt::AltModifier | Qt::MetaModifier | Qt::GroupSwitchModifier | Qt::KeypadModifier);
        if (modifiers.testFlag(Qt::ControlModifier) && additionalModifiers == Qt::NoModifier) {
            if (auto* edit = qobject_cast<SingleLineTextEdit*>(watched)) {
                auto* patternItem = qobject_cast<dlgTriggerPatternEdit*>(edit->parentWidget());
                if (keyEvent->key() == Qt::Key_Down) {
                    if (focusNextPatternItem(patternItem)) {
                        return true;
                    }
                } else if (keyEvent->key() == Qt::Key_Up) {
                    if (focusPreviousPatternItem(patternItem)) {
                        return true;
                    }
                }
            }
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

bool dlgTriggerEditor::event(QEvent* event)
{
    if (mIsGrabKey) {
        if (event->type() == QEvent::KeyPress) {
            auto * ke = static_cast<QKeyEvent*>(event);
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

    auto color = QColorDialog::getColor(QColor(mpTriggersMainArea->pushButtonFgColor->property(cButtonBaseColor).toString()),
                                        this,
                                        tr("Select foreground color to apply to matches"));
    color = color.isValid() ? color : QColorConstants::Transparent;
    const bool keepColor = color == QColorConstants::Transparent;
    mpTriggersMainArea->pushButtonFgColor->setStyleSheet(generateButtonStyleSheet(color));
    //: Keep the existing colour on matches to highlight. Use shortest word possible so it fits on the button
    mpTriggersMainArea->pushButtonFgColor->setText(keepColor ? tr("keep") : QString());
    mpTriggersMainArea->pushButtonFgColor->setProperty(cButtonBaseColor, keepColor ? qsl("transparent") : color.name());
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
    color = color.isValid() ? color : QColorConstants::Transparent;
    const bool keepColor = color == QColorConstants::Transparent;
    mpTriggersMainArea->pushButtonBgColor->setStyleSheet(generateButtonStyleSheet(color));
    //: Keep the existing colour on matches to highlight. Use shortest word possible so it fits on the button
    mpTriggersMainArea->pushButtonBgColor->setText(keepColor ? tr("keep") : QString());
    mpTriggersMainArea->pushButtonBgColor->setProperty(cButtonBaseColor, keepColor ? qsl("transparent") : color.name());
}

void dlgTriggerEditor::slot_soundTrigger()
{
    // Use the existing path/filename if it is not empty, otherwise start in last global user dir
    QSettings& settings = *mudlet::getQSettings();
    QString lastDir = settings.value("lastFileDialogLocation", QDir::homePath()).toString();

    const QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Choose sound file"),
                                                    mpTriggersMainArea->lineEdit_soundFile->text().isEmpty()
                                                    ? lastDir
                                                    : mpTriggersMainArea->lineEdit_soundFile->text(),
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
        lastDir = QFileInfo(fileName).absolutePath();
        settings.setValue("lastFileDialogLocation", lastDir);
    }
}

// Get the color from the user to use as that to look for as the foreground in
// a color trigger:
void dlgTriggerEditor::slot_colorTriggerFg()
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
    TTrigger::decodeColorPatternText(pPatternItem->singleLineTextEdit_pattern->toPlainText(), pT->mColorTriggerFgAnsi, pT->mColorTriggerBgAnsi);

    // The following method wants to know BOTH existing fore and backgrounds
    // it will select the appropriate as a result of the third argument and it
    // uses both to determine whether the result to return is valid considering
    // the other, non used (background in this method) part:
    auto pD = new dlgColorTrigger(this, pT, false, tr("Select foreground trigger color for item %1").arg(QString::number(pPatternItem->mRow+1)));
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

    pPatternItem->singleLineTextEdit_pattern->setPlainText(TTrigger::createColorPatternText(pT->mColorTriggerFgAnsi, pT->mColorTriggerBgAnsi));

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
}

// Get the color from the user to use as that to look for as the background in
// a color trigger:
void dlgTriggerEditor::slot_colorTriggerBg()
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
    TTrigger::decodeColorPatternText(pPatternItem->singleLineTextEdit_pattern->toPlainText(), pT->mColorTriggerFgAnsi, pT->mColorTriggerBgAnsi);

    // The following method wants to know BOTH existing fore and backgrounds
    // it will select the appropriate as a result of the third argument and it
    // uses both to determine whether the result to return is valid considering
    // the other, non used (background in this method) part:
    auto pD = new dlgColorTrigger(this, pT, true, tr("Select background trigger color for item %1").arg(QString::number(pPatternItem->mRow+1)));
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

    pPatternItem->singleLineTextEdit_pattern->setPlainText(TTrigger::createColorPatternText(pT->mColorTriggerFgAnsi, pT->mColorTriggerBgAnsi));

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
    config->setShowWhitespaceMode((state & QTextOption::ShowTabsAndSpaces)
                                  ? edbee::TextEditorConfig::ShowWhitespaces
                                  : edbee::TextEditorConfig::HideWhitespaces);
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
    connect(mpSourceEditorEdbeeDocument, &edbee::TextDocument::textChanged, this, &dlgTriggerEditor::slot_itemEdited);
    // Buck.lua is a fake filename for edbee to figure out its lexer type with. Referencing the
    // lexer directly by name previously gave problems.
    // Don't apply Lua syntax highlighting for the Variables view since it displays plain data values, not code
    if (mCurrentView != EditorViewType::cmVarsView) {
        mpSourceEditorEdbeeDocument->setLanguageGrammar(edbee::Edbee::instance()->grammarManager()->detectGrammarWithFilename(QLatin1String("Buck.lua")));
    }
    pEditorWidget->controller()->giveTextDocument(mpSourceEditorEdbeeDocument);

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
    config->setRenderBidiContolCharacters(mpHost->getEditorShowBidi());
    config->setAutocompleteMinimalCharacters(3);
    config->endChanges();

    // If undo is not disabled when setting the initial text, the
    // setting of the text will be undoable.
    mpSourceEditorEdbeeDocument->setUndoCollectionEnabled(false);
    disconnect(mpSourceEditorEdbeeDocument, &edbee::TextDocument::textChanged, this, &dlgTriggerEditor::slot_itemEdited);
    mpSourceEditorEdbeeDocument->setText(initialText);
    connect(mpSourceEditorEdbeeDocument, &edbee::TextDocument::textChanged, this, &dlgTriggerEditor::slot_itemEdited);
    mpSourceEditorEdbeeDocument->setUndoCollectionEnabled(true);
}

void dlgTriggerEditor::setThemeAndOtherSettings(const QString& theme)
{
    auto localConfig = mpSourceEditorEdbee->config();
    localConfig->beginChanges();
    localConfig->setThemeName(theme);
    mpHost->editorThemeChanged();
    localConfig->setFont(mpHost->getDisplayFont());
    localConfig->setShowWhitespaceMode((mudlet::self()->mEditorTextOptions & QTextOption::ShowTabsAndSpaces)
                                               ? edbee::TextEditorConfig::ShowWhitespaces
                                               : edbee::TextEditorConfig::HideWhitespaces);
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
        menu->addAction(controller->createAction("cut", tr("Cut"), QIcon::fromTheme(qsl("edit-cut"), QIcon(qsl(":/icons/edit-cut.png"))), menu));
        menu->addAction(controller->createAction("copy", tr("Copy"), QIcon::fromTheme(qsl("edit-copy"), QIcon(qsl(":/icons/edit-copy.png"))), menu));
        menu->addAction(controller->createAction("paste", tr("Paste"), QIcon::fromTheme(qsl("edit-paste"), QIcon(qsl(":/icons/edit-paste.png"))), menu));
        menu->addSeparator();
        menu->addAction(controller->createAction("sel_all", tr("Select All"), QIcon::fromTheme(qsl("edit-select-all"), QIcon(qsl(":/icons/edit-select-all.png"))), menu));
        formatAction->setIcon(QIcon::fromTheme(qsl("run-build-clean"), QIcon::fromTheme(qsl("run-build-clean"))));
    }

    connect(formatAction, &QAction::triggered, this, [=, this]() {
        auto formattedText = mpHost->mLuaInterpreter.formatLuaCode(mpSourceEditorEdbeeDocument->text());
        // workaround for crash if undo is used, see https://github.com/edbee/edbee-lib/issues/66
        controller->beginUndoGroup();
        disconnect(mpSourceEditorEdbeeDocument, &edbee::TextDocument::textChanged, this, &dlgTriggerEditor::slot_itemEdited);
        mpSourceEditorEdbeeDocument->setText(formattedText);
        connect(mpSourceEditorEdbeeDocument, &edbee::TextDocument::textChanged, this, &dlgTriggerEditor::slot_itemEdited);
        // don't coalesce the format text action - not that it matters for us since we we only change
        // the text once during the undo group
        controller->endUndoGroup(edbee::CoalesceId_None, false);
    });

    menu->addAction(formatAction);
    menu->exec(QCursor::pos());

    delete menu;
}

QString dlgTriggerEditor::generateButtonStyleSheet(const QColor& color, const bool isEnabled)
{
    if (color != QColorConstants::Transparent && color.isValid()) {
        if (isEnabled) {
            return mudlet::self()->mTEXT_ON_BG_STYLESHEET
                    .arg(color.lightness() > 127 ? QLatin1String("black") : QLatin1String("white"),
                         color.name());
        }

        const QColor disabledColor = QColor::fromHsl(color.hslHue(), color.hslSaturation()/4, color.lightness());
        return mudlet::self()->mTEXT_ON_BG_STYLESHEET
                .arg(QLatin1String("darkGray"), disabledColor.name());
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
                qDebug().noquote().nospace() << "dlgTriggerEditor::parseButtonStyleSheetColors(\"" << styleSheetText << "\", " << isToGetForeground << ") ERROR - Invalid hex string as foreground color!";
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
                    qDebug().noquote().nospace() << "dlgTriggerEditor::parseButtonStyleSheetColors(\"" << styleSheetText << "\", " << isToGetForeground << ") ERROR - Invalid string \"" <<  match.captured(1) << "\" found as name of foreground color!";
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
#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
                if (QColor::isValidColor(match.captured(1))) {
#else
                if (QColor::isValidColorName(match.captured(1))) {
#endif
                    return QColor(match.captured(1));
                } else {
                    qDebug().noquote().nospace() << "dlgTriggerEditor::parseButtonStyleSheetColors(\"" << styleSheetText << "\", " << isToGetForeground << ") ERROR - Invalid string \"" <<  match.captured(1) << "\" found as name of background color!";
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

void dlgTriggerEditor::slot_showAllTriggerControls(const bool isShown)
{
    if (mpTriggersMainArea->toolButton_toggleExtraControls->isChecked() != isShown) {
        mpTriggersMainArea->toolButton_toggleExtraControls->setChecked(isShown);
    }

    if (mpTriggersMainArea->widget_right->isVisible() != isShown) {
        mpTriggersMainArea->widget_right->setVisible(isShown);
    }

    updatePatternTabOrder();
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
     *gf||                    ||         |   |+------------------------------+|
     *et||                    ||         | --+--------------------------------+
     *t ||                    ||         |
     *=>|+--------------------+|         |
     *--+----------------------+---------+
     */
    const int hysteresis = 10;
    if (mpTriggersMainArea->isVisible()) {
        mTriggerEditorSplitterState = splitter_right->saveState();
        // The triggersMainArea is visible
        if (mpTriggersMainArea->toolButton_toggleExtraControls->isChecked()) {
            // The extra controls are visible in the triggersMainArea
            if (mpTriggersMainArea->widget_verticalSpacer_right->height() <= hysteresis) {
                // And it is not tall enough to show the right hand side - so
                // hide them - we are using the spacer to detect if there is any
                // space:
                slot_showAllTriggerControls(false);
                // And the first time note down the required height:
                if (mTriggerMainAreaMinimumHeightToShowAll < 1) {
                    mTriggerMainAreaMinimumHeightToShowAll = mpTriggersMainArea->widget_left->height();
                }
            }

        } else {
            // And the extra controls are NOT visible
            if (mTriggerMainAreaMinimumHeightToShowAll > 0 && mpTriggersMainArea->widget_left->height() > mTriggerMainAreaMinimumHeightToShowAll) {
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
    if ((!toolBar->isVisible())
        || toolBar->isFloating()
        || (QMainWindow::toolBarArea(toolBar) & (Qt::ToolBarArea::LeftToolBarArea|Qt::ToolBarArea::RightToolBarArea|Qt::ToolBarArea::BottomToolBarArea))) {
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
    if ((!toolBar2->isVisible())
        || toolBar2->isFloating()
        || (QMainWindow::toolBarArea(toolBar2) & (Qt::ToolBarArea::TopToolBarArea|Qt::ToolBarArea::RightToolBarArea|Qt::ToolBarArea::BottomToolBarArea))) {

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
    if (mCurrentView != EditorViewType::cmUnknownView) {
        showIntro();
    }
}

void dlgTriggerEditor::clearTimerForm()
{
    mpTimersMainArea->hide();
    mpTimersMainArea->hide();
    if (mCurrentView != EditorViewType::cmUnknownView) {
        showIntro();
    }
}

void dlgTriggerEditor::clearAliasForm()
{
    mpAliasMainArea->hide();
    mpSourceEditorArea->hide();
    if (mCurrentView != EditorViewType::cmUnknownView) {
        showIntro();
    }
}

void dlgTriggerEditor::clearScriptForm()
{
    mpScriptsMainArea->hide();
    mpSourceEditorArea->hide();
    if (mCurrentView != EditorViewType::cmUnknownView) {
        showIntro();
    }
}

void dlgTriggerEditor::clearActionForm()
{
    mpActionsMainArea->hide();
    mpSourceEditorArea->hide();
    if (mCurrentView != EditorViewType::cmUnknownView) {
        showIntro();
    }
}

void dlgTriggerEditor::clearKeyForm()
{
    mpKeysMainArea->hide();
    mpSourceEditorArea->hide();
    if (mCurrentView != EditorViewType::cmUnknownView) {
        showIntro();
    }
}

void dlgTriggerEditor::clearVarForm()
{
    mpVarsMainArea->hide();
    mpSourceEditorArea->hide();
    if (mCurrentView != EditorViewType::cmUnknownView) {
        showIntro();
    }
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
    QMainWindow::changeEvent(e);

    if (e->type() == QEvent::LanguageChange) {
        updatePatternNavigationHint();
    }

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

void dlgTriggerEditor::checkForMoreThanOneTriggerItem()
{
    int activeItems = 0;
    if (!mpWidget_triggerItems || !mpWidget_triggerItems->layout()) {
        return;
    }
    auto pLayout = mpWidget_triggerItems->layout();
    for (qsizetype i = 0, total = pLayout->count(); i < total; ++i) {
        auto pLayoutItem = pLayout->itemAt(i)->widget();
        if (pLayoutItem) {
            auto* psingleLineTextEdit_pattern = pLayoutItem->findChild<SingleLineTextEdit*>(qsl("singleLineTextEdit_pattern"));
            auto* pComboBox_type = pLayoutItem->findChild<QComboBox*>(qsl("comboBox_patternType"));
            if (pComboBox_type && (pComboBox_type->currentIndex() == REGEX_PROMPT || pComboBox_type->currentIndex() == REGEX_LINE_SPACER)) {
                // These automatically counts as an active item - though if there
                // isn't any GA signals the first won't work...
                ++activeItems;
            } else {
                if (psingleLineTextEdit_pattern && !psingleLineTextEdit_pattern->toPlainText().isEmpty()) {
                    ++activeItems;
                }
            }
        }
    }

    mpTriggersMainArea->groupBox_multiLineTrigger->setEnabled(activeItems > 1);
}

void dlgTriggerEditor::setDisplayFont(const QFont& newFont)
{
    if (mpErrorConsole) {
        mpErrorConsole->setFont(newFont);
    }

    auto config = mpSourceEditorEdbee->config();
    config->beginChanges();
    config->setFont(newFont);
    config->endChanges();
}

void dlgTriggerEditor::slot_bannerDismissClicked()
{
    handleBannerDismiss();
}

void dlgTriggerEditor::handleBannerDismiss()
{
    mLastDismissedBannerView = mCurrentView;
    mLastDismissedBannerContent = mpSystemMessageArea->notificationAreaMessageBox->text();
    mLastDismissedBannerKey = mCurrentBannerKey;

    const QString settingsKey = bannerSettingsKey(mCurrentView, mCurrentBannerKey);
    if (!settingsKey.isEmpty()) {
        mTemporarilyHiddenBanners.insert(settingsKey);
    }

    hideSystemMessageArea();
    mCurrentBannerKey.clear();
    showBannerUndoToast();
}

void dlgTriggerEditor::showBannerUndoToast()
{
    if (mpBannerUndoTimer) {
        mpBannerUndoTimer->stop();
        mpBannerUndoTimer->deleteLater();
    }

    mCurrentBannerKey.clear();

    mpBannerUndoTimer = new QTimer(this);
    mpBannerUndoTimer->setSingleShot(true);
    mpBannerUndoTimer->setInterval(std::chrono::seconds(5));

    //: Toast notification shown when user dismisses an editor tip banner. Allows them to undo or permanently hide the tips for this editor view type.
    QString toastMessage = tr("Banner hidden. <a href='undo' style='color: inherit; text-decoration: underline;'>Undo</a> | <a href='hide-permanently' style='color: inherit; text-decoration: underline;'>Hide permanently</a>");

    mpSystemMessageArea->notificationAreaIconLabelError->hide();
    mpSystemMessageArea->notificationAreaIconLabelWarning->hide();
    mpSystemMessageArea->notificationAreaIconLabelInformation->show();
    mpSystemMessageArea->notificationAreaMessageBox->setText(toastMessage);
    mpSystemMessageArea->show();

    connect(mpBannerUndoTimer, &QTimer::timeout, this, &dlgTriggerEditor::hideSystemMessageArea);
    mpBannerUndoTimer->start();

    disconnect(mpSystemMessageArea->notificationAreaMessageBox, &QLabel::linkActivated, this, &dlgTriggerEditor::slot_clickedMessageBox);
    connect(mpSystemMessageArea->notificationAreaMessageBox, &QLabel::linkActivated, this, [this](const QString& link) {
        if (link == "undo") {
            undoBannerDismiss();
        } else if (link == "hide-permanently") {
            handlePermanentBannerDismiss();
        } else {
            slot_clickedMessageBox(link);
        }
    });
}

void dlgTriggerEditor::undoBannerDismiss()
{
    if (mpBannerUndoTimer) {
        mpBannerUndoTimer->stop();
        mpBannerUndoTimer->deleteLater();
        mpBannerUndoTimer = nullptr;
    }

    const QString settingsKey = bannerSettingsKey(mLastDismissedBannerView, mLastDismissedBannerKey);
    if (!settingsKey.isEmpty()) {
        mTemporarilyHiddenBanners.remove(settingsKey);
    }

    setBannerPermanentlyHidden(mLastDismissedBannerView, mLastDismissedBannerKey, false);

    // Remove the undo toast before restoring the banner so the new content can
    // be shown immediately without being blocked by the active notification.
    if (mpSystemMessageArea) {
        mpSystemMessageArea->hide();
    }
    mCurrentBannerKey.clear();

    if (mLastDismissedBannerView == mCurrentView && !mLastDismissedBannerContent.isEmpty()) {
        showHideableBanner(mLastDismissedBannerContent, mLastDismissedBannerKey);
    }
}


void dlgTriggerEditor::handlePermanentBannerDismiss()
{
    setBannerPermanentlyHidden(mLastDismissedBannerView, mLastDismissedBannerKey, true);
    hideSystemMessageArea();
    mCurrentBannerKey.clear();
}

bool dlgTriggerEditor::bannerPermanentlyHidden(EditorViewType viewType, const QString& bannerKey, bool includeBasePreference)
{
    const QString key = bannerSettingsKey(viewType, bannerKey);
    const QString baseKey = bannerSettingsKey(viewType, QString());
    const QString legacyKey = legacyBannerSettingsKey(viewType, bannerKey);
    const QString legacyBaseKey = legacyBannerSettingsKey(viewType, QString());
    if (key.isEmpty()) {
        return false;
    }

    QSettings* settings = mudlet::getQSettings();
    if (!settings) {
        return false;
    }

    auto migrateLegacyKey = [settings](const QString& newKey, const QString& oldKey) {
        if (newKey.isEmpty() || oldKey.isEmpty() || newKey == oldKey) {
            return;
        }

        const QString oldPath = qsl("Editor/banner_permanently_hidden/%1").arg(oldKey);
        if (!settings->contains(oldPath)) {
            return;
        }

        settings->remove(oldPath);
    };

    migrateLegacyKey(key, legacyKey);
    migrateLegacyKey(baseKey, legacyBaseKey);

    if (includeBasePreference && !bannerKey.isEmpty() && !baseKey.isEmpty()) {
        if (settings->value(qsl("Editor/banner_permanently_hidden/%1").arg(baseKey), false).toBool()) {
            return true;
        }
    }

    return settings->value(qsl("Editor/banner_permanently_hidden/%1").arg(key), false).toBool();
}

void dlgTriggerEditor::setBannerPermanentlyHidden(EditorViewType viewType, const QString& bannerKey, bool hidden)
{
    const QString key = bannerSettingsKey(viewType, bannerKey);
    const QString legacyKey = legacyBannerSettingsKey(viewType, bannerKey);
    if (key.isEmpty()) {
        return;
    }

    QSettings* settings = mudlet::getQSettings();
    settings->setValue(qsl("Editor/banner_permanently_hidden/%1").arg(key), hidden);

    if (!legacyKey.isEmpty() && legacyKey != key) {
        settings->remove(qsl("Editor/banner_permanently_hidden/%1").arg(legacyKey));
    }

    if (!hidden) {
        mTemporarilyHiddenBanners.remove(key);
    }
}
