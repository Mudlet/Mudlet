/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2017 by Stephen Lyons - slysven@virginmedia.com    *
 *   Copyright (C) 2016 by Owen Davison - odavison@cs.dal.ca               *
 *   Copyright (C) 2016 by Ian Adkins - ieadkins@gmail.com                 *
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
#include "HostManager.h"
#include "LuaInterface.h"
#include "TAction.h"
#include "TConsole.h"
#include "THighlighter.h"
#include "TTextEdit.h"
#include "TTreeWidget.h"
#include "TTrigger.h"
#include "TriggerUnit.h"
#include "VarUnit.h"
#include "XMLexport.h"
#include "XMLimport.h"
#include "dlgActionMainArea.h"
#include "dlgAliasMainArea.h"
#include "dlgColorTrigger.h"
#include "dlgKeysMainArea.h"
#include "dlgScriptsMainArea.h"
#include "dlgTriggerPatternEdit.h"
#include "dlgTriggersMainArea.h"
#include "mudlet.h"

#include "pre_guard.h"
#include <QColorDialog>
#include <QFileDialog>
#include <QHeaderView>
#include <QListWidget>
#include <QMainWindow>
#include <QMessageBox>
#include <QSettings>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextOption>
#include <QToolBar>
#include "post_guard.h"


using namespace std;

const int dlgTriggerEditor::cmTriggerView = 1;
const int dlgTriggerEditor::cmTimerView = 2;
const int dlgTriggerEditor::cmAliasView = 3;
const int dlgTriggerEditor::cmScriptView = 4;
const int dlgTriggerEditor::cmActionView = 5;
const int dlgTriggerEditor::cmKeysView = 6;
const int dlgTriggerEditor::cmVarsView = 7;

const QString msgInfoAddAlias = "Alias are input triggers. To make a new alias: <b>1.</b> Define an input trigger pattern with a Perl regular expression. " \
                                "<b>2.</b> Define a command to send to the MUD in clear text <b><u>instead of the alias pattern</u></b> or write a script for more complicated needs. " \
                                "<b>3. <u>Activate</u></b> the alias. "\
                                "Check the manual for <a href='http://mudlet.sourceforge.net/mudlet_documentation.html'>more information</a>.";

const QString msgInfoAddTrigger = "To add a new trigger: <b>1.</b> Define a <b><u>pattern</u></b> that you want to trigger on. <b>2.</b> select the appropriate pattern <b><u>type</u></b>."\
                                  "<b>3.</b> Define a clear text command that you want to send to the MUD if the trigger finds the pattern in the text from the MUD or write a script."\
                                  "<b>4. <u>Activate</u></b> the trigger." \
                                  "Check the manual for <a href='http://mudlet.sourceforge.net/mudlet_documentation.html'>more information</a>.";

const QString msgInfoAddScript = "Check the manual for <a href='http://mudlet.sourceforge.net/mudlet_documentation.html'>more information</a>.";


const QString msgInfoAddTimer = "Check the manual for <a href='http://mudlet.sourceforge.net/mudlet_documentation.html'>more information</a>.";


const QString msgInfoAddButton = "To add a new button: <b>1.</b> Add a new group to define a new Button bar in case you don't have any."\
                                 "<b>2.</b> Add new buttons to a button bar." \
                                 "Check the manual for <a href='http://mudlet.sourceforge.net/mudlet_documentation.html'>more information</a>.";

const QString msgInfoAddKey = "To add a new key binding <b>1.</b> add a new key <b>2.</b> click on <u><b>grab key</b></u> and then press your key combination. <b><u>NOTE:</u></b> If you want to bind a key combination you must hold down the modifier keys (e.g. control, shift etc.) down before clicking on grab key. " \
                              "<b>3.</b> Define a command that is executed when the key is hit. <b>4. <u>Activate</u></b> the new key binding." \
                              "Check the manual for <a href='http://mudlet.sourceforge.net/mudlet_documentation.html'>more information</a>.";

const QString msgInfoAddVar = "Add a new variable (can be a string, integer, boolean -- delete a variable to set it to nil).";

dlgTriggerEditor::dlgTriggerEditor( Host * pH )
: mpCurrentActionItem( 0 )
, mpCurrentKeyItem( 0 )
, mpCurrentTimerItem( 0 )
, mpCurrentScriptItem( 0 )
, mpCurrentTriggerItem( 0 )
, mpCurrentAliasItem( 0 )
, mpCurrentVarItem( 0 )
, mpHost( pH )
, mpSourceEditor( 0 )
, mpSourceEditorDocument( 0 )
{
    // init generated dialog
    setupUi(this);
    setUnifiedTitleAndToolBarOnMac( true ); //MAC OSX: make window moveable
    setWindowTitle( mpHost->getName() );
    setWindowIcon( QIcon( QStringLiteral( ":/icons/mudlet_editor.png" ) ) );
    auto statusBar = new QStatusBar(this);
    statusBar->setSizeGripEnabled( true );
    setStatusBar( statusBar );
    statusBar->show();
    mIsGrabKey = false;
    auto pVB1 = new QVBoxLayout(mainArea);

    // system message area
    mpSystemMessageArea = new dlgSystemMessageArea( mainArea );
    mpSystemMessageArea->setObjectName("mpSystemMessageArea");
    QSizePolicy sizePolicy6( QSizePolicy::Expanding, QSizePolicy::Fixed );
    mpSystemMessageArea->setSizePolicy( sizePolicy6 );
    pVB1->addWidget( mpSystemMessageArea );
    connect( mpSystemMessageArea->messageAreaCloseButton, SIGNAL(clicked()), mpSystemMessageArea, SLOT(hide()));

    // main areas

    mpTriggersMainArea = new dlgTriggersMainArea( mainArea );
    pVB1->setContentsMargins(0,0,0,0);
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mpTriggersMainArea->setSizePolicy( sizePolicy );
    pVB1->addWidget( mpTriggersMainArea );
    mpTriggersMainArea->lineEdit_soundFile->hide();
    connect(mpTriggersMainArea->pushButtonFgColor, SIGNAL(clicked()), this, SLOT(slot_colorizeTriggerSetFgColor()));
    connect(mpTriggersMainArea->pushButtonBgColor, SIGNAL(clicked()), this, SLOT(slot_colorizeTriggerSetBgColor()));
    connect(mpTriggersMainArea->pushButtonSound, SIGNAL(clicked()), this, SLOT(slot_soundTrigger()));

    mpTimersMainArea = new dlgTimersMainArea( mainArea );
    QSizePolicy sizePolicy7(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mpTimersMainArea->setSizePolicy( sizePolicy7 );
    pVB1->addWidget( mpTimersMainArea );

    mpAliasMainArea = new dlgAliasMainArea( mainArea );
    QSizePolicy sizePolicy8(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mpAliasMainArea->setSizePolicy( sizePolicy8 );
    pVB1->addWidget( mpAliasMainArea );

    mpActionsMainArea = new dlgActionMainArea( mainArea );
    mpActionsMainArea->setSizePolicy( sizePolicy8 );
    connect(mpActionsMainArea->checkBox_pushdownbutton, SIGNAL(stateChanged(const int)), this, SLOT(slot_toggle_isPushDownButton(const int)));
    pVB1->addWidget( mpActionsMainArea );

    mpKeysMainArea = new dlgKeysMainArea( mainArea );
    mpKeysMainArea->setSizePolicy( sizePolicy8 );
    pVB1->addWidget( mpKeysMainArea );
    connect(mpKeysMainArea->pushButton_grabKey, SIGNAL(pressed()), this, SLOT(slot_grab_key()));

    mpVarsMainArea = new dlgVarsMainArea( mainArea );
    mpVarsMainArea->setSizePolicy( sizePolicy8 );
    pVB1->addWidget( mpVarsMainArea );

    mpScriptsMainArea = new dlgScriptsMainArea( mainArea );
    QSizePolicy sizePolicy9(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mpScriptsMainArea->setSizePolicy( sizePolicy9 );
    pVB1->addWidget( mpScriptsMainArea );

    mIsScriptsMainAreaEditHandler = false;
    mpScriptsMainAreaEditHandlerItem = 0;
    connect(mpScriptsMainArea->lineEdit, SIGNAL(returnPressed()), this, SLOT(slot_script_main_area_add_handler()));
    connect(mpScriptsMainArea->listWidget_registered_event_handlers, SIGNAL(itemClicked ( QListWidgetItem * )), this, SLOT(slot_script_main_area_edit_handler(QListWidgetItem*)));

    // source editor area

    mpSourceEditorArea = new dlgSourceEditorArea( mainArea );
    QSizePolicy sizePolicy5(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mpSourceEditorArea->setSizePolicy( sizePolicy5 );
    pVB1->addWidget( mpSourceEditorArea );
    mpSourceEditor = mpSourceEditorArea->editor;
    mpSourceEditor->setWordWrapMode( QTextOption::NoWrap );
    mpSourceEditor->setPlaceholderText( tr( "Enter your Lua code (or value for variable) here..." ) );
    mpSourceEditorDocument = mpSourceEditor->document();
    QTextOption _options = mpSourceEditorDocument->defaultTextOption();
    QTextOption::Flags _flags = _options.flags() & ~( QTextOption::ShowTabsAndSpaces | QTextOption::ShowLineAndParagraphSeparators );
    _flags |= mudlet::self()->mEditorTextOptions & ( QTextOption::ShowTabsAndSpaces | QTextOption::ShowLineAndParagraphSeparators );
    _options.setFlags( _flags );
    mpSourceEditorDocument->setDefaultTextOption( _options );

    connect( mpSourceEditor, SIGNAL( cursorPositionChanged() ), this, SLOT( slot_cursorPositionChanged() ) );
    connect( mudlet::self(), SIGNAL( signal_editorTextOptionsChanged( QTextOption::Flags ) ), this,  SLOT( slot_changeEditorTextOptions( QTextOption::Flags ) ) );

    // option areas

    auto pHB2 = new QHBoxLayout(popupArea);
    QSizePolicy sizePolicy2(QSizePolicy::Expanding, QSizePolicy::Maximum);
    popupArea->setMinimumSize(200,60);
    pHB2->setSizeConstraint( QLayout::SetMaximumSize );
    mpErrorConsole = new TConsole(mpHost,false, popupArea);
    mpErrorConsole->setWrapAt(100);
    mpErrorConsole->console->slot_toggleTimeStamps();
    mpErrorConsole->print("*** starting new session ***\n");
    pHB2->setContentsMargins(0,0,0,0);
    pHB2->addWidget( mpErrorConsole );
    mpErrorConsole->show();

    button_toggleSearchAreaResults->setStyleSheet( QStringLiteral( "QToolButton::on{border-image:url(:/icons/arrow-down_grey.png);} "
                                                                   "QToolButton{border-image:url(:/icons/arrow-right_grey.png);}"
                                                                   "QToolButton::on:hover{border-image:url(:/icons/arrow-down.png);} "
                                                                   "QToolButton:hover{border-image:url(:/icons/arrow-right.png);}" ) );
    connect( button_toggleSearchAreaResults, SIGNAL(clicked(const bool)), this, SLOT( slot_showSearchAreaResults(const bool)));

    // additional settings
    treeWidget_triggers->setColumnCount(1);
    treeWidget_triggers->setIsTriggerTree();
    treeWidget_triggers->setRootIsDecorated( false );
    treeWidget_triggers->setHost( mpHost );
    treeWidget_triggers->header()->hide();
    connect( treeWidget_triggers, SIGNAL(itemClicked(QTreeWidgetItem *, int)), this, SLOT(slot_item_selected_save(QTreeWidgetItem*)) );

    treeWidget_aliases->hide();
    treeWidget_aliases->setHost( mpHost );
    treeWidget_aliases->setIsAliasTree();
    treeWidget_aliases->setColumnCount(1);
    treeWidget_aliases->header()->hide();
    treeWidget_aliases->setRootIsDecorated( false );
    connect( treeWidget_aliases, SIGNAL(itemClicked(QTreeWidgetItem *, int)), this, SLOT(slot_item_selected_save(QTreeWidgetItem*)) );

    treeWidget_actions->hide();
    treeWidget_actions->setHost( mpHost );
    treeWidget_actions->setIsActionTree();
    treeWidget_actions->setColumnCount(1);
    treeWidget_actions->header()->hide();
    treeWidget_actions->setRootIsDecorated( false );
    connect( treeWidget_actions, SIGNAL(itemClicked(QTreeWidgetItem *, int)), this, SLOT(slot_item_selected_save(QTreeWidgetItem*)) );

    treeWidget_timers->hide();
    treeWidget_timers->setHost( mpHost );
    treeWidget_timers->setIsTimerTree();
    treeWidget_timers->setColumnCount(1);
    treeWidget_timers->header()->hide();
    treeWidget_timers->setRootIsDecorated( false );
    connect( treeWidget_timers, SIGNAL(itemClicked(QTreeWidgetItem *, int)), this, SLOT(slot_item_selected_save(QTreeWidgetItem*)) );

    treeWidget_variables->hide();
    treeWidget_variables->setHost( mpHost );
    treeWidget_variables->setIsVarTree();
    treeWidget_variables->setColumnCount(2);
    treeWidget_variables->hideColumn(1);
    treeWidget_variables->header()->hide();
    treeWidget_variables->setRootIsDecorated( false );
    connect( treeWidget_variables, SIGNAL(itemClicked(QTreeWidgetItem *, int)), this, SLOT(slot_item_selected_save(QTreeWidgetItem*)) );

    treeWidget_keys->hide();
    treeWidget_keys->setHost( mpHost );
    treeWidget_keys->setIsKeyTree();
    treeWidget_keys->setColumnCount(1);
    treeWidget_keys->header()->hide();
    treeWidget_keys->setRootIsDecorated( false );
    connect( treeWidget_keys, SIGNAL(itemClicked(QTreeWidgetItem *, int)), this, SLOT(slot_item_selected_save(QTreeWidgetItem*)) );

    treeWidget_scripts->hide();
    treeWidget_scripts->setHost( mpHost );
    treeWidget_scripts->setIsScriptTree();
    treeWidget_scripts->setColumnCount(1);
    treeWidget_scripts->header()->hide();
    treeWidget_scripts->setRootIsDecorated( false );
    connect( treeWidget_scripts, SIGNAL(itemClicked(QTreeWidgetItem *, int)), this, SLOT(slot_item_selected_save(QTreeWidgetItem*)) );

    QAction * viewTriggerAction = new QAction( QIcon( QStringLiteral( ":/icons/tools-wizard.png" ) ), tr("Triggers"), this);
    viewTriggerAction->setStatusTip(tr("Show Triggers"));
    connect(viewTriggerAction, SIGNAL(triggered()), this, SLOT(slot_show_triggers() ));

    QAction * viewActionAction = new QAction( QIcon( QStringLiteral( ":/icons/bookmarks.png" ) ), tr("Buttons"), this);
    viewActionAction->setStatusTip(tr("Show Buttons"));
    connect(viewActionAction, SIGNAL(triggered()), this, SLOT(slot_show_actions() ));


    QAction * viewAliasAction = new QAction( QIcon( QStringLiteral( ":/icons/system-users.png" ) ), tr("Aliases"), this);
    viewAliasAction->setStatusTip(tr("Show Aliases"));
    connect( viewAliasAction, SIGNAL(triggered()), this, SLOT( slot_show_aliases()));


    QAction * showTimersAction = new QAction( QIcon( QStringLiteral( ":/icons/chronometer.png" ) ), tr("Timers"), this);
    showTimersAction->setStatusTip(tr("Show Timers"));
    connect( showTimersAction, SIGNAL(triggered()), this, SLOT( slot_show_timers()));

    QAction * viewScriptsAction = new QAction( QIcon( QStringLiteral( ":/icons/document-properties.png" ) ), tr("Scripts"), this);
    viewScriptsAction->setStatusTip(tr("Show Scripts"));
    connect( viewScriptsAction, SIGNAL(triggered()), this, SLOT( slot_show_scripts()));

    QAction * viewKeysAction = new QAction( QIcon( QStringLiteral( ":/icons/preferences-desktop-keyboard.png" ) ), tr("Keys"), this);
    viewKeysAction->setStatusTip(tr("Show Keybindings"));
    connect( viewKeysAction, SIGNAL(triggered()), this, SLOT( slot_show_keys()));

    QAction * viewVarsAction = new QAction( QIcon( QStringLiteral( ":/icons/variables.png" ) ), tr("Variables"), this);
    viewVarsAction->setStatusTip(tr("Show Variables"));
    connect( viewVarsAction, SIGNAL(triggered()), this, SLOT( slot_show_vars( )));

    QAction * toggleActiveAction = new QAction( QIcon( QStringLiteral( ":/icons/document-encrypt.png" ) ), tr("Activate"), this);
    toggleActiveAction->setStatusTip(tr("Toggle Active or Non-Active Mode for Triggers, Scripts etc."));
    connect( toggleActiveAction, SIGNAL(triggered()), this, SLOT( slot_toggle_active()));
    connect( treeWidget_triggers, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), SLOT( slot_toggle_active()));
    connect( treeWidget_aliases, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), SLOT( slot_toggle_active()));
    connect( treeWidget_timers, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), SLOT( slot_toggle_active()));
    connect( treeWidget_scripts, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), SLOT( slot_toggle_active()));
    connect( treeWidget_actions, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), SLOT( slot_toggle_active()));
    connect( treeWidget_keys, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), SLOT( slot_toggle_active()));


    QAction * addTriggerAction = new QAction( QIcon( QStringLiteral( ":/icons/document-new.png" ) ), tr("Add Item"), this);
    addTriggerAction->setStatusTip(tr("Add new Trigger, Script, Alias or Filter"));
    connect( addTriggerAction, SIGNAL(triggered()), this, SLOT( slot_add_new()));

    QAction * deleteTriggerAction = new QAction( QIcon( QStringLiteral( ":/icons/edit-delete-shred.png" ) ), tr("Delete Item"), this);
    deleteTriggerAction->setStatusTip(tr("Delete Trigger, Script, Alias or Filter"));
    connect( deleteTriggerAction, SIGNAL(triggered()), this, SLOT( slot_delete_item()));

    QAction * addFolderAction = new QAction( QIcon( QStringLiteral( ":/icons/folder-new.png" ) ), tr("Add Group"), this);
    addFolderAction->setStatusTip(tr("Add new Group"));
    connect( addFolderAction, SIGNAL(triggered()), this, SLOT( slot_add_new_folder()));

    QAction * saveAction = new QAction( QIcon( QStringLiteral( ":/icons/document-save-as.png" ) ), tr("Save Item"), this);
    saveAction->setShortcut(tr("Ctrl+S"));
    saveAction->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>")
                           .arg(tr("Saves the selected trigger, script, alias or etc, so new changes take effect.\nIt will not save to disk, so changes will be lost in case of a computer/program crash (but Save Profile to the right will be secure.)")));
    saveAction->setStatusTip(tr("Saves the selected trigger, script, alias, etc, so new changes take effect - does not save to disk though..."));
    connect( saveAction, SIGNAL(triggered()), this, SLOT( slot_save_edit() ));

    QAction * importAction = new QAction( QIcon( QStringLiteral( ":/icons/import.png" ) ), tr("Import"), this);
    importAction->setEnabled( true );
    connect( importAction, SIGNAL(triggered()), this, SLOT( slot_import()));

    QAction * exportAction = new QAction( QIcon( QStringLiteral( ":/icons/export.png" ) ), tr("Export"), this);
    exportAction->setEnabled( true );
    connect( exportAction, SIGNAL(triggered()), this, SLOT( slot_export()));

    QAction * profileSaveAction = new QAction( QIcon ( QStringLiteral( ":/icons/document-save-all.png" ) ), tr("Save Profile"), this);
    profileSaveAction->setEnabled( true );
    profileSaveAction->setShortcut(tr("Ctrl+Shift+S"));
    profileSaveAction->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>")
                                  .arg(tr("Saves your entire profile (triggers, aliases, scripts, timers, buttons and keys, but not the map or script-specific settings) to your computer disk, so in case of a computer or program crash, all changes you have done will be retained.</p>"
                                          "<p>It also makes a backup of your profile, you can load an older version of it when connecting.</p>"
                                          "<p>Should there be any modules that are marked to be \"<i>synced</i>\" this will also cause them to be saved and reloaded into other profiles if they too are active.")));
    profileSaveAction->setStatusTip(tr("Saves your entire profile (triggers, aliases, etc, but not the map); also \"synchronizes\" modules that are so marked."));
    connect( profileSaveAction, SIGNAL(triggered()), this, SLOT( slot_profileSaveAction()));

    QAction * saveProfileAsAction = new QAction( QIcon( QStringLiteral( ":/icons/utilities-file-archiver.png" ) ), tr("Save Profile As"), this);
    saveProfileAsAction->setEnabled( true );
    connect( saveProfileAsAction, SIGNAL(triggered()), this, SLOT( slot_profileSaveAsAction()));

    QAction * viewStatsAction = new QAction( QIcon( QStringLiteral( ":/icons/view-statistics.png" ) ), tr("Statistics"), this);
    viewStatsAction->setStatusTip(tr("Generates a statics summary display on the main profile console."));
    connect( viewStatsAction, SIGNAL(triggered()), this, SLOT( slot_viewStatsAction()));

    QAction * viewErrorsAction = new QAction( QIcon( QStringLiteral( ":/icons/errors.png" ) ), tr("errors"), this);
    viewErrorsAction->setStatusTip(tr("Shows/Hides the errors console in the bottom right of this editor."));
    connect( viewErrorsAction, SIGNAL(triggered()), this, SLOT( slot_viewErrorsAction()));

    QAction * showDebugAreaAction = new QAction( QIcon( QStringLiteral( ":/icons/tools-report-bug.png" ) ), tr("Debug"), this);
    showDebugAreaAction->setToolTip(tr("Activates Debug Messages -> system will be <b><i>slower</i></b>."));
    showDebugAreaAction->setStatusTip(tr("Shows/Hides the separate Central Debug Console - when being displayed the system will be slower."));
    connect( showDebugAreaAction, SIGNAL(triggered()), this, SLOT( slot_debug_mode() ));

    toolBar = new QToolBar();
    toolBar->setIconSize(QSize(mudlet::self()->mMainIconSize*8,mudlet::self()->mMainIconSize*8));
    toolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    toolBar->setMovable(true);
    toolBar->addAction( toggleActiveAction );
    toolBar->addAction( saveAction );

    toolBar->addSeparator();

    toolBar->addAction( addTriggerAction );
    toolBar->addAction( addFolderAction );

    toolBar->addSeparator();
    toolBar->addAction( deleteTriggerAction );
    toolBar->addAction( importAction );
    toolBar->addAction( exportAction );
    toolBar->addAction( saveProfileAsAction );
    toolBar->addAction( profileSaveAction );



    toolBar2 = new QToolBar();
    toolBar2->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    toolBar2->setIconSize(QSize(mudlet::self()->mMainIconSize*8,mudlet::self()->mMainIconSize*8));
    connect(button_displayAllVariables, SIGNAL(toggled(bool)), this, SLOT(slot_toggleHiddenVariables(bool)));

    connect(mpVarsMainArea->hideVariable, SIGNAL(clicked(bool)), this, SLOT(slot_toggleHiddenVar( bool )));

    toolBar2->addAction( viewTriggerAction );
    toolBar2->addAction( viewAliasAction );
    toolBar2->addAction( viewScriptsAction );
    toolBar2->addAction( showTimersAction );
    toolBar2->addAction( viewKeysAction );
    toolBar2->addAction( viewVarsAction );
    toolBar2->addAction( viewActionAction );
    toolBar2->addAction( viewErrorsAction );
    toolBar2->addAction( viewStatsAction );
    toolBar2->addAction( showDebugAreaAction );

    toolBar2->setMovable( true );

    toolBar2->setOrientation(Qt::Vertical);

    QMainWindow::addToolBar(Qt::LeftToolBarArea, toolBar2 );
    QMainWindow::addToolBar(Qt::TopToolBarArea, toolBar );

    mpSourceEditor->setFont( mpHost->mDisplayFont );

    connect( comboBox_searchTerms, SIGNAL( activated( const QString )), this, SLOT(slot_search_triggers( const QString ) ) );
    connect( treeWidget_triggers, SIGNAL( itemClicked( QTreeWidgetItem *, int ) ), this, SLOT( slot_trigger_selected( QTreeWidgetItem *) ) );
    connect( treeWidget_triggers, SIGNAL( itemSelectionChanged()), this, SLOT( slot_tree_selection_changed()) );
    connect( treeWidget_keys, SIGNAL( itemClicked( QTreeWidgetItem *, int ) ), this, SLOT( slot_key_selected( QTreeWidgetItem *) ) );
    connect( treeWidget_keys, SIGNAL( itemSelectionChanged()), this, SLOT( slot_tree_selection_changed()) );
    connect( treeWidget_timers, SIGNAL( itemClicked( QTreeWidgetItem *, int ) ), this, SLOT( slot_timer_selected( QTreeWidgetItem *) ) );
    connect( treeWidget_timers, SIGNAL( itemSelectionChanged()), this, SLOT( slot_tree_selection_changed()) );
    connect( treeWidget_scripts, SIGNAL( itemClicked( QTreeWidgetItem *, int ) ), this, SLOT( slot_scripts_selected( QTreeWidgetItem *) ) );
    connect( treeWidget_scripts, SIGNAL( itemSelectionChanged()), this, SLOT( slot_tree_selection_changed()) );
    connect( treeWidget_aliases, SIGNAL( itemClicked( QTreeWidgetItem *, int ) ), this, SLOT( slot_alias_selected( QTreeWidgetItem *) ) );
    connect( treeWidget_aliases, SIGNAL( itemSelectionChanged()), this, SLOT( slot_tree_selection_changed()) );
    connect( treeWidget_actions, SIGNAL( itemClicked( QTreeWidgetItem *, int ) ), this, SLOT( slot_action_selected( QTreeWidgetItem *) ) );
    connect( treeWidget_actions, SIGNAL( itemSelectionChanged()), this, SLOT( slot_tree_selection_changed()) );
    connect( treeWidget_variables, SIGNAL( itemClicked( QTreeWidgetItem *, int ) ), this, SLOT( slot_var_selected( QTreeWidgetItem *) ) );
    connect( treeWidget_variables, SIGNAL( itemChanged(QTreeWidgetItem*,int) ), this, SLOT( slot_var_changed( QTreeWidgetItem *) ) );
    connect( treeWidget_variables, SIGNAL( itemSelectionChanged()), this, SLOT( slot_tree_selection_changed()) );
    connect( treeWidget_searchResults, SIGNAL(itemClicked(QTreeWidgetItem*, int)), this, SLOT( slot_item_selected_search_list(QTreeWidgetItem*, int)));

    // Force the size of the triangle icon button that shows/hides the search
    // results to be 3/4 of the height of the combo-box used to enter the search
    // term - this is to prevent an overlarge button on MacOS platforms where it
    // was found to be an issue!
    button_toggleSearchAreaResults->setMaximumSize( QSize( (3*comboBox_searchTerms->height())/4, (3*comboBox_searchTerms->height())/4 ) );
    button_toggleSearchAreaResults->setMinimumSize( QSize( (3*comboBox_searchTerms->height())/4, (3*comboBox_searchTerms->height())/4 ) );

    connect( mpScriptsMainArea->toolButton_add, SIGNAL(pressed()), this, SLOT(slot_script_main_area_add_handler()));
    connect( mpScriptsMainArea->toolButton_remove, SIGNAL(pressed()), this, SLOT( slot_script_main_area_delete_handler()));

    mpTriggersMainArea->hide();
    mpTimersMainArea->hide();
    mpScriptsMainArea->hide();
    mpAliasMainArea->hide();
    mpActionsMainArea->hide();
    mpKeysMainArea->hide();
    mpVarsMainArea->hide();

    mpSourceEditorArea->hide();

    mpSystemMessageArea->hide();

    treeWidget_triggers->show();
    treeWidget_aliases->hide();
    treeWidget_actions->hide();
    treeWidget_timers->hide();
    treeWidget_scripts->hide();
    treeWidget_keys->hide();
    treeWidget_variables->hide();

    popupArea->hide();
    frame_rightBottom->hide();

    readSettings();
    setTBIconSize( 0 );

    treeWidget_searchResults->setColumnCount( 4 );
    QStringList labelList;
    labelList << "Type" << "Name" << "Where" << "What";
    treeWidget_searchResults->setHeaderLabels( labelList );

    slot_showSearchAreaResults(false);

    mpScrollArea = mpTriggersMainArea->scrollArea;
    HpatternList = new QWidget;
    auto lay1 = new QVBoxLayout( HpatternList );
    lay1->setContentsMargins(0,0,0,0);
    lay1->setSpacing(0);
    mpScrollArea->setWidget( HpatternList );
    for( int i=0; i<50; i++)
    {
        auto pItem = new dlgTriggerPatternEdit(HpatternList);
        QStringList _patternList;
        _patternList << "substring"
                     << "perl regex"
                     << "begin of line substring"
                     << "exact match"
                     << "Lua function"
                     << "line spacer"
                     << "color trigger";
        QComboBox * pBox = pItem->patternType;
        pBox->addItems( _patternList );
        pBox->setItemData(0, QVariant(i) );
        connect( pBox, SIGNAL(currentIndexChanged(int)), this, SLOT(slot_set_pattern_type_color(int)));
        connect( pItem->fgB, SIGNAL(pressed()), this, SLOT(slot_color_trigger_fg()));
        connect( pItem->bgB, SIGNAL(pressed()), this, SLOT(slot_color_trigger_bg()));
        HpatternList->layout()->addWidget( pItem );
        mTriggerPatternEdit.push_back( pItem );
        pItem->mRow = i;
        pItem->fgB->hide();
        pItem->bgB->hide();
        pItem->number->setText( QString::number( i ) );
        pItem->number->show();
    }
    showHiddenVars = false;
    widget_searchTerm->updateGeometry();
}

void dlgTriggerEditor::slot_toggleHiddenVar( bool status )
{
    LuaInterface * lI = mpHost->getLuaInterface();
    VarUnit * vu = lI->getVarUnit();
    TVar * var = vu->getWVar( mpCurrentVarItem );
    if ( var )
    {
        if ( status )
            vu->addHidden( var, 1 );
        else
            vu->removeHidden( var );
    }
}

void dlgTriggerEditor::slot_toggleHiddenVariables( bool state )
{
    if( showHiddenVars != state) {
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
    if( frame_rightBottom->isHidden() ) {
        frame_rightBottom->show();
    }
    else {
        frame_rightBottom->hide();
    }
    // These will be inefffective if their container (frame_rightBottom) is not shown!
    mpErrorConsole->show();
    popupArea->show();
}


void dlgTriggerEditor::setTBIconSize( int s )
{
    if( mudlet::self()->mMainIconSize > 2 )
    {
        toolBar->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );
        toolBar2->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );
    }
    else
    {
        toolBar->setToolButtonStyle( Qt::ToolButtonIconOnly );
        toolBar2->setToolButtonStyle( Qt::ToolButtonIconOnly );
    }
    toolBar->setIconSize(QSize(mudlet::self()->mMainIconSize*8,mudlet::self()->mMainIconSize*8));
    toolBar2->setIconSize(QSize(mudlet::self()->mMainIconSize*8,mudlet::self()->mMainIconSize*8));
    treeWidget_triggers->setIconSize(QSize(mudlet::self()->mTEFolderIconSize*8,mudlet::self()->mTEFolderIconSize*8));
    treeWidget_aliases->setIconSize(QSize(mudlet::self()->mTEFolderIconSize*8,mudlet::self()->mTEFolderIconSize*8));
    treeWidget_timers->setIconSize(QSize(mudlet::self()->mTEFolderIconSize*8,mudlet::self()->mTEFolderIconSize*8));
    treeWidget_scripts->setIconSize(QSize(mudlet::self()->mTEFolderIconSize*8,mudlet::self()->mTEFolderIconSize*8));
    treeWidget_keys->setIconSize(QSize(mudlet::self()->mTEFolderIconSize*8,mudlet::self()->mTEFolderIconSize*8));
    treeWidget_actions->setIconSize(QSize(mudlet::self()->mTEFolderIconSize*8,mudlet::self()->mTEFolderIconSize*8));
    treeWidget_variables->setIconSize(QSize(mudlet::self()->mTEFolderIconSize*8,mudlet::self()->mTEFolderIconSize*8));
}

void dlgTriggerEditor::slot_choseButtonColor()
{
    auto color = QColorDialog::getColor();
    QPalette palette;
    palette.setColor(QPalette::Button, color);
}

void dlgTriggerEditor::closeEvent(QCloseEvent *event)
{
    writeSettings();
    event->accept();
}


void dlgTriggerEditor::readSettings()
{
    /*In case sensitive environments, two different config directories 
    were used: "Mudlet" for QSettings, and "mudlet" anywhere else.
    Furthermore, we skip the version from the application name to follow the convention.
    For compatibility with older settings, if no config is loaded 
    from the config directory "mudlet", application "Mudlet", we try to load from the config 
    directory "Mudlet", application "Mudlet 1.0". */
    QSettings settings_new("mudlet","Mudlet");
    QSettings settings((settings_new.contains("pos")? "mudlet":"Mudlet"),(settings_new.contains("pos")? "Mudlet":"Mudlet 1.0"));
    

    QPoint pos = settings.value("script_editor_pos", QPoint(10, 10)).toPoint();
    QSize size = settings.value("script_editor_size", QSize(600, 400)).toSize();
    resize( size );
    move( pos );
}

void dlgTriggerEditor::writeSettings()
{
    /*In case sensitive environments, two different config directories 
    were used: "Mudlet" for QSettings, and "mudlet" anywhere else. We change the QSettings directory 
    (the organization name) to "mudlet".
    Furthermore, we skip the version from the application name to follow the convention.*/
    QSettings settings("mudlet", "Mudlet");
    settings.setValue("script_editor_pos", pos());
    settings.setValue("script_editor_size", size());
}

void dlgTriggerEditor::slot_item_selected_search_list(QTreeWidgetItem* pItem, int mode )
{
    if ( !pItem )
        return;

    // For changing views from one type to another (e.g. script->triggers), we have to show
    // the new view first before changing the TreeWidgetItem. Because we save changes to
    // the current item when it is left, if we change the TreeWidgetItem and then swap
    // views the contents of the previous item will be overwritten.
    if( pItem->text(0) == QString("Trigger") )
    {
        QList<QTreeWidgetItem *> foundItemsList = treeWidget_triggers->findItems( pItem->text(1), Qt::MatchFixedString | Qt::MatchCaseSensitive | Qt::MatchRecursive, 0);

        for(auto treeWidgetItem : foundItemsList)
        {
            int idTree = treeWidgetItem->data(0, Qt::UserRole).toInt();
            int idSearch = pItem->data(0, Qt::UserRole).toInt();
            if( idTree == idSearch )
            {
                slot_show_triggers();
                slot_trigger_selected( treeWidgetItem );
                treeWidget_triggers->setCurrentItem( treeWidgetItem, 0 );
                treeWidget_triggers->scrollToItem( treeWidgetItem );
                return;
            }
        }
    }
    if( pItem->text(0) == QString("Alias") )
    {
        QList<QTreeWidgetItem *> foundItemsList = treeWidget_aliases->findItems( pItem->text(1), Qt::MatchFixedString | Qt::MatchCaseSensitive | Qt::MatchRecursive, 0);

        for(auto treeWidgetItem : foundItemsList)
        {
            int idTree = treeWidgetItem->data(0, Qt::UserRole).toInt();
            int idSearch = pItem->data(0, Qt::UserRole).toInt();
            if( idTree == idSearch )
            {
                slot_show_aliases();
                slot_alias_selected( treeWidgetItem );
                treeWidget_aliases->setCurrentItem( treeWidgetItem, 0 );
                treeWidget_aliases->scrollToItem( treeWidgetItem );
                return;
            }
        }
    }
    if( pItem->text(0) == QString("Script") )
    {
        QList<QTreeWidgetItem *> foundItemsList = treeWidget_scripts->findItems( pItem->text(1), Qt::MatchFixedString | Qt::MatchCaseSensitive | Qt::MatchRecursive, 0);

        for(auto treeWidgetItem : foundItemsList)
        {
            int idTree = treeWidgetItem->data(0, Qt::UserRole).toInt();
            int idSearch = pItem->data(0, Qt::UserRole).toInt();
            if( idTree == idSearch )
            {
                slot_show_scripts();
                slot_scripts_selected( treeWidgetItem );
                treeWidget_scripts->setCurrentItem( treeWidgetItem, 0 );
                treeWidget_scripts->scrollToItem( treeWidgetItem );
                return;
            }
        }
        return;
    }

    if( pItem->text(0) == QString("Button") )
    {
        QList<QTreeWidgetItem *> foundItemsList = treeWidget_actions->findItems( pItem->text(1), Qt::MatchFixedString | Qt::MatchCaseSensitive | Qt::MatchRecursive, 0);

        for(auto treeWidgetitem : foundItemsList)
        {
            int idTree = treeWidgetitem->data(0, Qt::UserRole).toInt();
            int idSearch = pItem->data(0, Qt::UserRole).toInt();
            if( idTree == idSearch )
            {
                slot_show_actions();
                slot_action_selected( treeWidgetitem );
                treeWidget_actions->setCurrentItem( treeWidgetitem, 0 );
                treeWidget_actions->scrollToItem( treeWidgetitem );
                return;
            }
        }
        return;
    }

    if( pItem->text(0) == QString("Timer") )
    {
        QList<QTreeWidgetItem *> foundItemsList = treeWidget_timers->findItems( pItem->text(1), Qt::MatchFixedString | Qt::MatchCaseSensitive | Qt::MatchRecursive, 0);

        for(auto treeWidgetItem : foundItemsList)
        {
            int idTree = treeWidgetItem->data(0, Qt::UserRole).toInt();
            int idSearch = pItem->data(0, Qt::UserRole).toInt();
            if( idTree == idSearch )
            {
                slot_show_timers();
                slot_timer_selected( treeWidgetItem );
                treeWidget_timers->setCurrentItem( treeWidgetItem, 0 );
                treeWidget_timers->scrollToItem( treeWidgetItem );
                return;
            }
        }
        return;
    }

    if( pItem->text(0) == QString("Key") )
    {
        QList<QTreeWidgetItem *> foundItemsList = treeWidget_keys->findItems( pItem->text(1), Qt::MatchFixedString | Qt::MatchCaseSensitive | Qt::MatchRecursive, 0);

        for(auto treeWidgetItem : foundItemsList)
        {
            int idTree = treeWidgetItem->data(0, Qt::UserRole).toInt();
            int idSearch = pItem->data(0, Qt::UserRole).toInt();
            if( idTree == idSearch )
            {
                slot_show_keys();
                slot_key_selected( treeWidgetItem );
                treeWidget_keys->setCurrentItem( treeWidgetItem, 0 );
                treeWidget_keys->scrollToItem( treeWidgetItem );
                return;
            }
        }
        return;
    }

    if( pItem->text(0) == QString("Variable") )
    {
        LuaInterface * lI = mpHost->getLuaInterface();
        VarUnit * vu = lI->getVarUnit();
        QStringList varShort = pItem->data(0, Qt::UserRole).toStringList();
        QList<QTreeWidgetItem *> list;
        recurseVariablesDown( mpVarBaseItem, list );
        QListIterator<QTreeWidgetItem *> it(list);
        while( it.hasNext() )
        {
            QTreeWidgetItem * pI = it.next();
            TVar * var = vu->getWVar( pI );
            if ( vu->shortVarName( var ) == varShort )
            {
                show_vars();
                treeWidget_variables->setCurrentItem( pI, 0 );
                treeWidget_variables->scrollToItem( pI );
                return;
            }
        }
        return;
    }
}

void dlgTriggerEditor::slot_search_triggers( const QString s )
{
    QRegExp pattern = QRegExp( s );

    treeWidget_searchResults->clear();
    slot_showSearchAreaResults(true);
    treeWidget_searchResults->setUpdatesEnabled( false );

    // type   | name | line number/pattern/name what has been found
    //-------------------------------------------------------------------------
    //trigger | name | line 204: a la sd  akd
    //trigger | asdf | pattern: ^alsd
    //trigger | asdf | name     |
    if( true )
    {
        std::list<TTrigger *> nodes = mpHost->getTriggerUnit()->getTriggerRootNodeList();
        for(auto trigger : nodes)
        {
            QTreeWidgetItem * pItem;
            QTreeWidgetItem * parent = 0;
            QString n = trigger->getName();
            if( n.indexOf( s, 0, Qt::CaseInsensitive ) != -1 )
            {
                QStringList sl;
                sl << "Trigger" << trigger->getName() << "name";
                if( ! parent )
                {
                    parent = new QTreeWidgetItem( sl );
                    parent->setFirstColumnSpanned( false );
                    parent->setData(0, Qt::UserRole, trigger->getID() );
                    treeWidget_searchResults->addTopLevelItem( parent );
                }
                else
                {
                    pItem = new QTreeWidgetItem( parent, sl );
                    pItem->setFirstColumnSpanned( false );
                    pItem->setData(0, Qt::UserRole, trigger->getID() );
                    parent->addChild( pItem );
                    parent->setExpanded( true );
                }
            }
            QStringList scriptList = trigger->getScript().split("\n");
            QStringList resultList = scriptList.filter( s, Qt::CaseInsensitive );
            for( int i=0; i<resultList.size(); i++ )
            {
                QStringList sl;
                sl << "Trigger" << trigger->getName() << "script" << resultList[i];
                if( ! parent )
                {
                    parent = new QTreeWidgetItem( sl );
                    parent->setFirstColumnSpanned( false );
                    parent->setData(0, Qt::UserRole, trigger->getID() );
                    treeWidget_searchResults->addTopLevelItem( parent );
                }
                else
                {
                    pItem = new QTreeWidgetItem( parent, sl );
                    pItem->setFirstColumnSpanned( false );
                    pItem->setData(0, Qt::UserRole, trigger->getID() );
                    parent->addChild( pItem );
                    parent->setExpanded( true );
                }
            }
            QStringList regexList = trigger->getRegexCodeList();
            resultList = regexList.filter( s, Qt::CaseInsensitive );
            for( int i=0; i<resultList.size(); i++ )
            {
                QStringList sl;
                sl << "Trigger" << trigger->getName() << "pattern" << resultList[i];
                if( ! parent )
                {
                    parent = new QTreeWidgetItem( sl );
                    parent->setFirstColumnSpanned( false );
                    parent->setData(0, Qt::UserRole, trigger->getID() );
                    treeWidget_searchResults->addTopLevelItem( parent );
                }
                else
                {
                    pItem = new QTreeWidgetItem( parent, sl );
                    pItem->setFirstColumnSpanned( false );
                    pItem->setData(0, Qt::UserRole, trigger->getID() );
                    parent->addChild( pItem );
                    parent->setExpanded( true );
                }
            }
            recursiveSearchTriggers( trigger, s );
        }
    }

    if( true )
    {
        std::list<TAlias *> nodes = mpHost->getAliasUnit()->getAliasRootNodeList();
        for(auto alias : nodes)
        {
            QTreeWidgetItem * pItem;
            QTreeWidgetItem * parent = 0;
            QString n = alias->getName();
            if( n.indexOf( s, 0, Qt::CaseInsensitive ) != -1 )
            {
                QStringList sl;
                sl << "Alias" << alias->getName() << "name";
                if( ! parent )
                {
                    parent = new QTreeWidgetItem( sl );
                    parent->setFirstColumnSpanned( false );
                    parent->setData(0, Qt::UserRole, alias->getID() );
                    treeWidget_searchResults->addTopLevelItem( parent );
                }
                else
                {
                    pItem = new QTreeWidgetItem( parent, sl );
                    pItem->setFirstColumnSpanned( false );
                    pItem->setData(0, Qt::UserRole, alias->getID() );
                    parent->addChild( pItem );
                    parent->setExpanded( true );
                }
            }
            QStringList scriptList = alias->getScript().split("\n");
            QStringList resultList = scriptList.filter( s, Qt::CaseInsensitive );
            for( int i=0; i<resultList.size(); i++ )
            {
                QStringList sl;
                sl << "Alias" << alias->getName() << "script" << resultList[i];
                if( ! parent )
                {
                    parent = new QTreeWidgetItem( sl );
                    parent->setFirstColumnSpanned( false );
                    parent->setData(0, Qt::UserRole, alias->getID() );
                    treeWidget_searchResults->addTopLevelItem( parent );
                }
                else
                {
                    pItem = new QTreeWidgetItem( parent, sl );
                    pItem->setFirstColumnSpanned( false );
                    pItem->setData(0, Qt::UserRole, alias->getID() );
                    parent->addChild( pItem );
                    parent->setExpanded( true );
                }
            }

            if( alias->getRegexCode().indexOf( s, 0, Qt::CaseInsensitive ) > -1 )
            {
                QStringList sl;
                sl << "Alias" << alias->getName() << "pattern" << alias->getRegexCode();
                if( ! parent )
                {
                    parent = new QTreeWidgetItem( sl );
                    parent->setFirstColumnSpanned( false );
                    parent->setData(0, Qt::UserRole, alias->getID() );
                    treeWidget_searchResults->addTopLevelItem( parent );
                }
                else
                {
                    pItem = new QTreeWidgetItem( parent, sl );
                    pItem->setFirstColumnSpanned( false );
                    pItem->setData(0, Qt::UserRole, alias->getID() );
                    parent->addChild( pItem );
                    parent->setExpanded( true );
                }
            }
            recursiveSearchAlias( alias, s );
        }
    }

    if( true )
    {
        std::list<TScript *> nodes = mpHost->getScriptUnit()->getScriptRootNodeList();
        for(auto script : nodes)
        {
            QTreeWidgetItem * pItem;
            QTreeWidgetItem * parent = 0;
            QString n = script->getName();
            if( n.indexOf( s, 0, Qt::CaseInsensitive ) != -1 )
            {
                QStringList sl;
                sl << "Script" << script->getName() << "name";
                if( ! parent )
                {
                    parent = new QTreeWidgetItem( sl );
                    parent->setFirstColumnSpanned( false );
                    parent->setData(0, Qt::UserRole, script->getID() );
                    treeWidget_searchResults->addTopLevelItem( parent );
                }
                else
                {
                    pItem = new QTreeWidgetItem( parent, sl );
                    pItem->setFirstColumnSpanned( false );
                    pItem->setData(0, Qt::UserRole, script->getID() );
                    parent->addChild( pItem );
                    parent->setExpanded( true );
                }
            }
            QStringList scriptList = script->getScript().split("\n");
            QStringList resultList = scriptList.filter( s, Qt::CaseInsensitive );
            for( int i=0; i<resultList.size(); i++ )
            {
                QStringList sl;
                sl << "Script" << script->getName() << "script" << resultList[i];
                if( ! parent )
                {
                    parent = new QTreeWidgetItem( sl );
                    parent->setFirstColumnSpanned( false );
                    parent->setData(0, Qt::UserRole, script->getID() );
                    treeWidget_searchResults->addTopLevelItem( parent );
                }
                else
                {
                    pItem = new QTreeWidgetItem( parent, sl );
                    pItem->setFirstColumnSpanned( false );
                    pItem->setData(0, Qt::UserRole, script->getID() );
                    parent->addChild( pItem );
                    parent->setExpanded( true );
                }
            }

            recursiveSearchScripts( script, s );
        }
    }

    if( true )
    {
        std::list<TAction *> nodes = mpHost->getActionUnit()->getActionRootNodeList();
        for(auto action : nodes)
        {
            QTreeWidgetItem * pItem;
            QTreeWidgetItem * parent = 0;
            QString n = action->getName();
            if( n.indexOf( s, 0, Qt::CaseInsensitive ) != -1 )
            {
                QStringList sl;
                sl << "Button" << action->getName() << "name";
                if( ! parent )
                {
                    parent = new QTreeWidgetItem( sl );
                    parent->setFirstColumnSpanned( false );
                    parent->setData(0, Qt::UserRole, action->getID() );
                    treeWidget_searchResults->addTopLevelItem( parent );
                }
                else
                {
                    pItem = new QTreeWidgetItem( parent, sl );
                    pItem->setFirstColumnSpanned( false );
                    pItem->setData(0, Qt::UserRole, action->getID() );
                    parent->addChild( pItem );
                    parent->setExpanded( true );
                }
            }
            QStringList scriptList = action->getScript().split("\n");
            QStringList resultList = scriptList.filter( s, Qt::CaseInsensitive );
            for( int i=0; i<resultList.size(); i++ )
            {
                QStringList sl;
                sl << "Button" << action->getName() << "script" << resultList[i];
                if( ! parent )
                {
                    parent = new QTreeWidgetItem( sl );
                    parent->setFirstColumnSpanned( false );
                    parent->setData(0, Qt::UserRole, action->getID() );
                    treeWidget_searchResults->addTopLevelItem( parent );
                }
                else
                {
                    pItem = new QTreeWidgetItem( parent, sl );
                    pItem->setFirstColumnSpanned( false );
                    pItem->setData(0, Qt::UserRole, action->getID() );
                    parent->addChild( pItem );
                    parent->setExpanded( true );
                }
            }

            recursiveSearchActions( action, s );
        }
    }

    if( true )
    {
        std::list<TTimer *> nodes = mpHost->getTimerUnit()->getTimerRootNodeList();
        for(auto timer : nodes)
        {
            QTreeWidgetItem * pItem;
            QTreeWidgetItem * parent = 0;
            QString n = timer->getName();
            if( n.indexOf( s, 0, Qt::CaseInsensitive ) != -1 )
            {
                QStringList sl;
                sl << "Timer" << timer->getName() << "name";
                if( ! parent )
                {
                    parent = new QTreeWidgetItem( sl );
                    parent->setFirstColumnSpanned( false );
                    parent->setData(0, Qt::UserRole, timer->getID() );
                    treeWidget_searchResults->addTopLevelItem( parent );
                }
                else
                {
                    pItem = new QTreeWidgetItem( parent, sl );
                    pItem->setFirstColumnSpanned( false );
                    pItem->setData(0, Qt::UserRole, timer->getID() );
                    parent->addChild( pItem );
                    parent->setExpanded( true );
                }
            }
            QStringList scriptList = timer->getScript().split("\n");
            QStringList resultList = scriptList.filter( s, Qt::CaseInsensitive );
            for( int i=0; i<resultList.size(); i++ )
            {
                QStringList sl;
                sl << "Timer" << timer->getName() << "script" << resultList[i];
                if( ! parent )
                {
                    parent = new QTreeWidgetItem( sl );
                    parent->setFirstColumnSpanned( false );
                    parent->setData(0, Qt::UserRole, timer->getID() );
                    treeWidget_searchResults->addTopLevelItem( parent );
                }
                else
                {
                    pItem = new QTreeWidgetItem( parent, sl );
                    pItem->setFirstColumnSpanned( false );
                    pItem->setData(0, Qt::UserRole, timer->getID() );
                    parent->addChild( pItem );
                    parent->setExpanded( true );
                }
            }

            recursiveSearchTimers( timer, s );
        }
    }

    if( true )
    {
        std::list<TKey *> nodes = mpHost->getKeyUnit()->getKeyRootNodeList();
        for(auto key : nodes)
        {
            QTreeWidgetItem * pItem;
            QTreeWidgetItem * parent = 0;
            QString n = key->getName();
            if( n.indexOf( s, 0, Qt::CaseInsensitive ) != -1 )
            {
                QStringList sl;
                sl << "Key" << key->getName() << "name";
                parent = new QTreeWidgetItem( sl );
                parent->setFirstColumnSpanned( false );
                parent->setData(0, Qt::UserRole, key->getID() );
                treeWidget_searchResults->addTopLevelItem( parent );
            }
            QStringList scriptList = key->getScript().split("\n");
            QStringList resultList = scriptList.filter( s, Qt::CaseInsensitive );
            for( int i=0; i<resultList.size(); i++ )
            {
                QStringList sl;
                sl << "Key" << key->getName() << "script" << resultList[i];
                if( ! parent )
                {
                    parent = new QTreeWidgetItem( sl );
                    parent->setFirstColumnSpanned( false );
                    parent->setData(0, Qt::UserRole, key->getID() );
                    treeWidget_searchResults->addTopLevelItem( parent );
                }
                else
                {
                    pItem = new QTreeWidgetItem( parent, sl );
                    pItem->setFirstColumnSpanned( false );
                    pItem->setData(0, Qt::UserRole, key->getID() );
                    parent->addChild( pItem );
                    parent->setExpanded( true );
                }
            }

            recursiveSearchKeys( key, s );
        }
    }

    if( true )
    {
        if ( mCurrentView != cmVarsView )
            repopulateVars();
        LuaInterface * lI = mpHost->getLuaInterface();
        VarUnit * vu = lI->getVarUnit();
        TVar * base = vu->getBase();
        QListIterator<TVar *> it(base->getChildren(false));
        while( it.hasNext() )
        {
            TVar * var = it.next();
            if ( ! showHiddenVars && vu->isHidden( var ) )
                continue;
            //recurse down this variable
            QList< TVar * > list;
            recurseVariablesDown( var, list, 0 );
            QListIterator<TVar *> it2(list);
            while( it2.hasNext() )
            {
                TVar * var2 = it2.next();
                if ( ! showHiddenVars && vu->isHidden(var2) )
                    continue;
                QTreeWidgetItem * pItem2;
                if ( ! var2->getName().isEmpty() && ( var2->getName().indexOf( s, 0, Qt::CaseInsensitive ) != -1 ) )
                {
                    QStringList sl;
                    sl << "Variable" << var2->getName() << "name";
                    pItem2 = new QTreeWidgetItem( sl );
                    pItem2->setFirstColumnSpanned( false );
                    pItem2->setData( 0, Qt::UserRole, vu->shortVarName(var2) );
                    treeWidget_searchResults->addTopLevelItem( pItem2 );
                }
                if ( ! var2->getValue().isEmpty() && ( var2->getValue().indexOf( s, 0, Qt::CaseInsensitive ) != -1 ) )
                {
                    QStringList sl;
                    sl << "Variable" << var2->getName() << "value";
                    pItem2 = new QTreeWidgetItem( sl );
                    pItem2->setFirstColumnSpanned( false );
                    pItem2->setData( 0, Qt::UserRole, vu->shortVarName(var2) );
                    treeWidget_searchResults->addTopLevelItem( pItem2 );
                }
            }
        }
    }
    mpSourceEditorArea->highlighter->setSearchPattern( s );
    treeWidget_searchResults->setUpdatesEnabled( true );
}

void dlgTriggerEditor::recursiveSearchTriggers( TTrigger * pTriggerParent, const QString & s )
{
    list<TTrigger *> * childrenList = pTriggerParent->getChildrenList();
    for(auto trigger : *childrenList)
    {
        QTreeWidgetItem * pItem;
        QTreeWidgetItem * parent = 0;
        QString n = trigger->getName();
        if( n.indexOf( s, 0, Qt::CaseInsensitive ) != -1 )
        {
            QStringList sl;
            sl << "Trigger" << trigger->getName() << "name";
            parent = new QTreeWidgetItem( sl );
            parent->setFirstColumnSpanned( false );
            parent->setData(0, Qt::UserRole, trigger->getID() );
            treeWidget_searchResults->addTopLevelItem( parent );
        }
        QStringList scriptList = trigger->getScript().split("\n");
        QStringList resultList = scriptList.filter( s, Qt::CaseInsensitive );
        for( int i=0; i<resultList.size(); i++ )
        {
            QStringList sl;
            sl << "Trigger" << trigger->getName() << "script" << resultList[i];
            if( ! parent )
            {
                parent = new QTreeWidgetItem( sl );
                parent->setFirstColumnSpanned( false );
                parent->setData(0, Qt::UserRole, trigger->getID() );
                treeWidget_searchResults->addTopLevelItem( parent );
            }
            else
            {
                pItem = new QTreeWidgetItem( parent, sl );
                pItem->setFirstColumnSpanned( false );
                pItem->setData(0, Qt::UserRole, trigger->getID() );
                parent->addChild( pItem );
                parent->setExpanded( true );
            }
        }
        QStringList regexList = trigger->getRegexCodeList();
        resultList = regexList.filter( s, Qt::CaseInsensitive );
        for( int i=0; i<resultList.size(); i++ )
        {
            QStringList sl;
            sl << "Trigger" << trigger->getName() << "pattern" << resultList[i];
            if( ! parent )
            {
                parent = new QTreeWidgetItem( sl );
                parent->setFirstColumnSpanned( false );
                parent->setData(0, Qt::UserRole, trigger->getID() );
                treeWidget_searchResults->addTopLevelItem( parent );
            }
            else
            {
                pItem = new QTreeWidgetItem( parent, sl );
                pItem->setFirstColumnSpanned( false );
                pItem->setData(0, Qt::UserRole, trigger->getID() );
                parent->addChild( pItem );
                parent->setExpanded( true );
            }
        }
        if( trigger->hasChildren() )
        {
            recursiveSearchTriggers( trigger, s );
        }
    }
}

void dlgTriggerEditor::recursiveSearchAlias( TAlias * pTriggerParent, const QString & s )
{
    list<TAlias *> * childrenList = pTriggerParent->getChildrenList();
    for(auto alias : *childrenList)
    {
        QTreeWidgetItem * pItem;
        QTreeWidgetItem * parent = 0;
        QString n = alias->getName();
        if( n.indexOf( s, 0, Qt::CaseInsensitive ) != -1 )
        {
            QStringList sl;
            sl << "Alias" << alias->getName() << "name";
            parent = new QTreeWidgetItem( sl );
            parent->setFirstColumnSpanned( false );
            parent->setData(0, Qt::UserRole, alias->getID() );
            treeWidget_searchResults->addTopLevelItem( parent );
        }
        QStringList scriptList = alias->getScript().split("\n");
        QStringList resultList = scriptList.filter( s, Qt::CaseInsensitive );
        for( int i=0; i<resultList.size(); i++ )
        {
            QStringList sl;
            sl << "Alias" << alias->getName() << "script" << resultList[i];
            if( ! parent )
            {
                parent = new QTreeWidgetItem( sl );
                parent->setFirstColumnSpanned( false );
                parent->setData(0, Qt::UserRole, alias->getID() );
                treeWidget_searchResults->addTopLevelItem( parent );
            }
            else
            {
                pItem = new QTreeWidgetItem( parent, sl );
                pItem->setFirstColumnSpanned( false );
                pItem->setData(0, Qt::UserRole, alias->getID() );
                parent->addChild( pItem );
                parent->setExpanded( true );
            }
        }


        if( alias->getRegexCode().indexOf( s, 0, Qt::CaseInsensitive ) > -1 )
        {
            QStringList sl;
            sl << "Alias" << alias->getName() << "pattern" << alias->getRegexCode();
            if( ! parent )
            {
                parent = new QTreeWidgetItem( sl );
                parent->setFirstColumnSpanned( false );
                parent->setData(0, Qt::UserRole, alias->getID() );
                treeWidget_searchResults->addTopLevelItem( parent );
            }
            else
            {
                pItem = new QTreeWidgetItem( parent, sl );
                pItem->setFirstColumnSpanned( false );
                pItem->setData(0, Qt::UserRole, alias->getID() );
                parent->addChild( pItem );
                parent->setExpanded( true );
            }
        }
        if( alias->hasChildren() )
        {
            recursiveSearchAlias( alias, s );
        }
    }
}

void dlgTriggerEditor::recursiveSearchScripts( TScript * pTriggerParent, const QString & s )
{
    list<TScript *> * childrenList = pTriggerParent->getChildrenList();
    for(auto script : *childrenList)
    {
        QTreeWidgetItem * pItem;
        QTreeWidgetItem * parent = 0;
        QString n = script->getName();
        if( n.indexOf( s, 0, Qt::CaseInsensitive ) != -1 )
        {
            QStringList sl;
            sl << "Script" << script->getName() << "name";
            parent = new QTreeWidgetItem( sl );
            parent->setFirstColumnSpanned( false );
            parent->setData(0, Qt::UserRole, script->getID() );
            treeWidget_searchResults->addTopLevelItem( parent );
        }
        QStringList scriptList = script->getScript().split("\n");
        QStringList resultList = scriptList.filter( s, Qt::CaseInsensitive );
        for( int i=0; i<resultList.size(); i++ )
        {
            QStringList sl;
            sl << "Script" << script->getName() << "script" << resultList[i];
            if( ! parent )
            {
                parent = new QTreeWidgetItem( sl );
                parent->setFirstColumnSpanned( false );
                parent->setData(0, Qt::UserRole, script->getID() );
                treeWidget_searchResults->addTopLevelItem( parent );
            }
            else
            {
                pItem = new QTreeWidgetItem( parent, sl );
                pItem->setFirstColumnSpanned( false );
                pItem->setData(0, Qt::UserRole, script->getID() );
                parent->addChild( pItem );
                parent->setExpanded( true );
            }
        }

        if( script->hasChildren() )
        {
            recursiveSearchScripts( script, s );
        }
    }
}

void dlgTriggerEditor::recursiveSearchActions( TAction * pTriggerParent, const QString & s )
{
    list<TAction *> * childrenList = pTriggerParent->getChildrenList();
    for(auto action : *childrenList)
    {
        QTreeWidgetItem * pItem;
        QTreeWidgetItem * parent = 0;
        QString n = action->getName();
        if( n.indexOf( s, 0, Qt::CaseInsensitive ) != -1 )
        {
            QStringList sl;
            sl << "Button" << action->getName() << "name";
            parent = new QTreeWidgetItem( sl );
            parent->setFirstColumnSpanned( false );
            parent->setData(0, Qt::UserRole, action->getID() );
            treeWidget_searchResults->addTopLevelItem( parent );
        }
        QStringList scriptList = action->getScript().split("\n");
        QStringList resultList = scriptList.filter( s, Qt::CaseInsensitive );
        for( int i=0; i<resultList.size(); i++ )
        {
            QStringList sl;
            sl << "Button" << action->getName() << "script" << resultList[i];
            if( ! parent )
            {
                parent = new QTreeWidgetItem( sl );
                parent->setFirstColumnSpanned( false );
                parent->setData(0, Qt::UserRole, action->getID() );
                treeWidget_searchResults->addTopLevelItem( parent );
            }
            else
            {
                pItem = new QTreeWidgetItem( parent, sl );
                pItem->setFirstColumnSpanned( false );
                pItem->setData(0, Qt::UserRole, action->getID() );
                parent->addChild( pItem );
                parent->setExpanded( true );
            }
        }

        if( action->hasChildren() )
        {
            recursiveSearchActions( action, s );
        }
    }
}

void dlgTriggerEditor::recursiveSearchTimers( TTimer * pTriggerParent, const QString & s )
{
    list<TTimer *> * childrenList = pTriggerParent->getChildrenList();
    for(auto timer : *childrenList)
    {
        QTreeWidgetItem * pItem;
        QTreeWidgetItem * parent = 0;
        QString n = timer->getName();
        if( n.indexOf( s, 0, Qt::CaseInsensitive ) != -1 )
        {
            QStringList sl;
            sl << "Timer" << timer->getName() << "name";
            parent = new QTreeWidgetItem( sl );
            parent->setFirstColumnSpanned( false );
            parent->setData(0, Qt::UserRole, timer->getID() );
            treeWidget_searchResults->addTopLevelItem( parent );
        }
        QStringList scriptList = timer->getScript().split("\n");
        QStringList resultList = scriptList.filter( s, Qt::CaseInsensitive );
        for( int i=0; i<resultList.size(); i++ )
        {
            QStringList sl;
            sl << "Timer" << timer->getName() << "script" << resultList[i];
            if( ! parent )
            {
                parent = new QTreeWidgetItem( sl );
                parent->setFirstColumnSpanned( false );
                parent->setData(0, Qt::UserRole, timer->getID() );
                treeWidget_searchResults->addTopLevelItem( parent );
            }
            else
            {
                pItem = new QTreeWidgetItem( parent, sl );
                pItem->setFirstColumnSpanned( false );
                pItem->setData(0, Qt::UserRole, timer->getID() );
                parent->addChild( pItem );
                parent->setExpanded( true );
            }
        }

        if( timer->hasChildren() )
        {
            recursiveSearchTimers( timer, s );
        }
    }
}

void dlgTriggerEditor::recursiveSearchKeys( TKey * pTriggerParent, const QString & s )
{
    list<TKey *> * childrenList = pTriggerParent->getChildrenList();
    for(auto key : *childrenList)
    {
        QTreeWidgetItem * pItem;
        QTreeWidgetItem * parent = 0;
        QString n = key->getName();
        if( n.indexOf( s, 0, Qt::CaseInsensitive ) != -1 )
        {
            QStringList sl;
            sl << "Key" << key->getName() << "name";
            parent = new QTreeWidgetItem( sl );
            parent->setFirstColumnSpanned( false );
            parent->setData(0, Qt::UserRole, key->getID() );
            treeWidget_searchResults->addTopLevelItem( parent );
        }
        QStringList scriptList = key->getScript().split("\n");
        QStringList resultList = scriptList.filter( s, Qt::CaseInsensitive );
        for( int i=0; i<resultList.size(); i++ )
        {
            QStringList sl;
            sl << "Key" << key->getName() << "script" << resultList[i];
            if( ! parent )
            {
                parent = new QTreeWidgetItem( sl );
                parent->setFirstColumnSpanned( false );
                parent->setData(0, Qt::UserRole, key->getID() );
                treeWidget_searchResults->addTopLevelItem( parent );
            }
            else
            {
                pItem = new QTreeWidgetItem( parent, sl );
                pItem->setFirstColumnSpanned( false );
                pItem->setData(0, Qt::UserRole, key->getID() );
                parent->addChild( pItem );
                parent->setExpanded( true );
            }
        }

        if( key->hasChildren() )
        {
            recursiveSearchKeys( key, s );
        }
    }
}



void dlgTriggerEditor::slot_addActionGroup()
{
    addAction( true ); //add action group
}

void dlgTriggerEditor::slot_addAction()
{
    addAction(false); //add normal action
}

void dlgTriggerEditor::slot_addVar()
{
    if (mpCurrentVarItem)
        addVar(false); //add normal action
}

void dlgTriggerEditor::slot_addVarGroup()
{
    if (mpCurrentVarItem)
        addVar(true);
}


void dlgTriggerEditor::slot_addAliasGroup()
{
    addAlias( true ); //add alias group
}

void dlgTriggerEditor::slot_addAlias()
{
    addAlias(false); //add normal alias
}

void dlgTriggerEditor::slot_addScriptGroup()
{
    addScript( true ); //add alias group
}

void dlgTriggerEditor::slot_addScript()
{
    addScript(false); //add normal alias
}

void dlgTriggerEditor::slot_addKeyGroup()
{
    addKey( true ); //add alias group
}

void dlgTriggerEditor::slot_addKey()
{
    addKey(false); //add normal alias
}


void dlgTriggerEditor::slot_addTriggerGroup()
{
    addTrigger(true); //add trigger group
}

void dlgTriggerEditor::slot_addTrigger()
{
    addTrigger(false); //add normal trigger
}

void dlgTriggerEditor::slot_addTimerGroup()
{
    addTimer( true );
}

void dlgTriggerEditor::slot_addTimer()
{
    addTimer( false ); //add normal trigger
}

void dlgTriggerEditor::slot_deleteAlias()
{
    QTreeWidgetItem * pItem = treeWidget_aliases->currentItem();
    if( ! pItem ) return;
    QTreeWidgetItem * pParent = pItem->parent();
    TAlias * pT = mpHost->getAliasUnit()->getAlias(pItem->data(0, Qt::UserRole).toInt());
    if( ! pT ) return;

    if( pParent )
    {
        pParent->removeChild( pItem );
    }
    else
    {
        qDebug()<<"ERROR: dlgTriggerEditor::slot_deleteAlias() child to be deleted doesnt have a parent";
    }
    delete pT;
    mpCurrentAliasItem = 0;
}

void dlgTriggerEditor::slot_deleteAction()
{
    QTreeWidgetItem * pItem = treeWidget_actions->currentItem();
    if( ! pItem ) return;
    QTreeWidgetItem * pParent = pItem->parent();
    TAction * pT = mpHost->getActionUnit()->getAction(pItem->data(0, Qt::UserRole).toInt());
    if( ! pT ) return;

    if( pParent )
    {
        pParent->removeChild( pItem );
    }
    else
    {
        qDebug()<<"ERROR: dlgTriggerEditor::slot_deleteAction() child to be deleted doesnt have a parent";
    }
    delete pT;
    mpCurrentActionItem = 0;
    mpHost->getActionUnit()->updateToolbar();
}

void dlgTriggerEditor::slot_deleteVar()
{
    QTreeWidgetItem * pItem = treeWidget_variables->currentItem();
    if( ! pItem ) return;
    QTreeWidgetItem * pParent = pItem->parent();
    LuaInterface * lI = mpHost->getLuaInterface();
    VarUnit * vu = lI->getVarUnit();
    TVar * var = vu->getWVar( pItem );
    if ( var )
    {
        lI->deleteVar( var );
        TVar * parent = var->getParent();
        if (parent)
            parent->removeChild(var);
        vu->removeVariable(var);
        delete var;
    }
    if( pParent )
    {
        pParent->removeChild( pItem );
    }
    else
    {
        qDebug()<<"ERROR: dlgTriggerEditor::slot_deleteAction() child to be deleted doesnt have a parent";
    }
    mpCurrentVarItem = 0;
}

void dlgTriggerEditor::slot_deleteScript()
{
    QTreeWidgetItem * pItem = treeWidget_scripts->currentItem();
    if( ! pItem ) return;
    QTreeWidgetItem * pParent = pItem->parent();
    TScript * pT = mpHost->getScriptUnit()->getScript(pItem->data(0, Qt::UserRole).toInt());
    if( ! pT ) return;

    if( pParent )
    {
        pParent->removeChild( pItem );
    }
    else
    {
        qDebug()<<"ERROR: dlgTriggerEditor::slot_deleteScript() child to be deleted doesnt have a parent";
    }
    delete pT;
    mpCurrentScriptItem = 0;
}

void dlgTriggerEditor::slot_deleteKey()
{
    QTreeWidgetItem * pItem = treeWidget_keys->currentItem();
    if( ! pItem ) return;
    QTreeWidgetItem * pParent = pItem->parent();

    TKey * pT = mpHost->getKeyUnit()->getKey(pItem->data(0, Qt::UserRole).toInt());
    if( ! pT ) return;

    if( pParent )
    {
        pParent->removeChild( pItem );
    }
    else
    {
        qDebug()<<"ERROR: dlgTriggerEditor::slot_deleteScript() child to be deleted doesnt have a parent";
    }
    delete pT;
    mpCurrentKeyItem = 0;
}

void dlgTriggerEditor::slot_deleteTrigger()
{
    QTreeWidgetItem * pItem = treeWidget_triggers->currentItem();
    if( ! pItem ) return;
    QTreeWidgetItem * pParent = pItem->parent();

    TTrigger * pT = mpHost->getTriggerUnit()->getTrigger(pItem->data(0, Qt::UserRole).toInt());
    if( ! pT ) return;

    if( pParent )
    {
        pParent->removeChild( pItem );
    }
    else
    {
        qDebug()<<"ERROR: dlgTriggerEditor::slot_deleteTrigger() child to be deleted doesnt have a parent";
    }
    delete pT;
    mpCurrentTriggerItem = 0;
}

void dlgTriggerEditor::slot_deleteTimer()
{
    QTreeWidgetItem * pItem = treeWidget_timers->currentItem();
    if( ! pItem ) return;
    QTreeWidgetItem * pParent = pItem->parent();

    TTimer * pT = mpHost->getTimerUnit()->getTimer(pItem->data(0, Qt::UserRole).toInt());
    if( ! pT ) return;

    if( pParent )
    {
        pParent->removeChild( pItem );
    }
    else
    {
        qDebug()<<"ERROR: dlgTriggerEditor::slot_deleteTimer() child to be deleted doesnt have a parent";
    }
    delete pT;
    mpCurrentTimerItem = 0;
}


void dlgTriggerEditor::slot_trigger_toggle_active()
{
    QTreeWidgetItem * pItem = treeWidget_triggers->currentItem();
    if( ! pItem ) return;
    QIcon icon;

    TTrigger * pT = mpHost->getTriggerUnit()->getTrigger(pItem->data(0, Qt::UserRole).toInt());
    if( ! pT ) return;

    pT->setIsActive( ! pT->shouldBeActive() );

    if( pT->isFilterChain() )
    {
        if( pT->isActive() )
        {
            if( pT->ancestorsActive() )
            {
                icon.addPixmap( QPixmap( QStringLiteral( ":/icons/filter.png" ) ), QIcon::Normal, QIcon::Off );
            }
            else
            {
                icon.addPixmap( QPixmap( QStringLiteral( ":/icons/filter-grey.png" ) ), QIcon::Normal, QIcon::Off );
            }
        }
        else
        {
            if( pT->ancestorsActive() )
            {
                icon.addPixmap( QPixmap( QStringLiteral( ":/icons/filter-locked.png" ) ), QIcon::Normal, QIcon::Off );
            }
            else
            {
                icon.addPixmap( QPixmap( QStringLiteral( ":/icons/filter-grey-locked.png" ) ), QIcon::Normal, QIcon::Off );
            }
        }
    }
    else if( pT->isFolder() )
    {
        if( pT->isActive() )
        {
            if (pT->ancestorsActive()) {
                icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-blue.png" ) ), QIcon::Normal, QIcon::Off );
            } else {
                icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-grey.png" ) ), QIcon::Normal, QIcon::Off );
            }
        }
        else
        {
            if (pT->ancestorsActive()) {
                icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-blue-locked.png" ) ), QIcon::Normal, QIcon::Off );
            } else {
                icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-grey-locked.png" ) ), QIcon::Normal, QIcon::Off );
            }
        }
    }
    else
    {
        if( pT->isActive() )
        {
            if (pT->ancestorsActive()) {
                icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked.png" ) ), QIcon::Normal, QIcon::Off );
            } else {
                icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked_grey.png" ) ), QIcon::Normal, QIcon::Off );
            }
        }
        else
        {
            if (pT->ancestorsActive()) {
                icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox.png" ) ), QIcon::Normal, QIcon::Off );
            } else {
                icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox-grey.png" ) ), QIcon::Normal, QIcon::Off );
            }
        }
    }

    if( pT->state() )
    {
        pItem->setIcon( 0, icon);
        pItem->setText( 0, pT->getName() );
    }
    else
    {
        QIcon iconError;
        iconError.addPixmap( QPixmap( QStringLiteral( ":/icons/tools-report-bug.png" ) ), QIcon::Normal, QIcon::Off );
        pItem->setIcon( 0, iconError );
    }
    showInfo(QString("Trying to %2 trigger <em>%1</em> %3.")
                     .arg(pT->getName(), pT->shouldBeActive() ? "activate" : "deactivate", pT->state() ? "succeeded" : QString("failed; reason: ") + pT->getError()));
    if( pItem->childCount() > 0 )
    {
        children_icon_triggers( pItem );
    }
}

void dlgTriggerEditor::children_icon_triggers( QTreeWidgetItem * pWidgetItemParent )
{
    for( int i = 0; i<pWidgetItemParent->childCount(); i++ )
    {
        QTreeWidgetItem * pItem = pWidgetItemParent->child(i);
        TTrigger * pT = mpHost->getTriggerUnit()->getTrigger(pItem->data(0, Qt::UserRole).toInt());
        if( ! pT ) return;

        QIcon icon;
        if( pItem->childCount() > 0 )
        {
            children_icon_triggers( pItem );
        }
        if( pT->state() )
        {
            if( pT->isFilterChain() )
            {
                if( pT->isActive() )
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/filter.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/filter-grey.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
                else
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/filter-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/filter-grey-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
            }
            else if( pT->isFolder() )
            {
                if( pT->isActive() )
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-blue.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-grey.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
                else
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-blue-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-grey-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
            }
            else
            {
                if( pT->isActive() )
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked_grey.png" ) ), QIcon::Normal, QIcon::Off );
                    }

                }
                else
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            pItem->setIcon(0, icon);
        }
        else
        {
            QIcon iconError;
            iconError.addPixmap( QPixmap( QStringLiteral( ":/icons/tools-report-bug.png" ) ), QIcon::Normal, QIcon::Off );
            pItem->setIcon( 0, iconError );
            showError( pT->getError() );
        }
    }

}


void dlgTriggerEditor::slot_timer_toggle_active()
{
    QTreeWidgetItem * pItem = treeWidget_timers->currentItem();
    if( ! pItem ) return;
    QIcon icon;

    TTimer * pT = mpHost->getTimerUnit()->getTimer(pItem->data(0, Qt::UserRole).toInt());
    if( ! pT ) return;

    if( ! pT->isOffsetTimer() )
        pT->setIsActive( ! pT->shouldBeActive() );
    else
        pT->setShouldBeActive( ! pT->shouldBeActive() );

    if( pT->isFolder() )
    {
        // disable or enable all timers in the respective branch
        // irrespective of the user defined state.
        if( pT->shouldBeActive() )
        {
            pT->enableTimer( pT->getID() );
        }
        else
        {
            pT->disableTimer( pT->getID() );
        }

        if( pT->shouldBeActive() )
        {
            icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-green.png" ) ), QIcon::Normal, QIcon::Off );
        }
        else
        {
            icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-green-locked.png" ) ), QIcon::Normal, QIcon::Off );
        }
    }
    else
    {
        if( pT->isOffsetTimer() )
        {
            // state of offset timers is managed by the trigger engine
            if( pT->shouldBeActive() )
            {
                pT->enableTimer( pT->getID() );
                icon.addPixmap( QPixmap( QStringLiteral( ":/icons/offsettimer-on.png" ) ), QIcon::Normal, QIcon::Off );
            }
            else
            {
                pT->disableTimer( pT->getID() );
                icon.addPixmap( QPixmap( QStringLiteral( ":/icons/offsettimer-off.png" ) ), QIcon::Normal, QIcon::Off );
            }
        }
        else
        {
            if( pT->shouldBeActive() )
            {
                pT->enableTimer( pT->getID() );
                icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked.png" ) ), QIcon::Normal, QIcon::Off );
            }
            else
            {
                pT->disableTimer( pT->getID() );
                icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox.png" ) ), QIcon::Normal, QIcon::Off );
            }
        }
    }
    if( pT->state() )
    {
        pItem->setIcon( 0, icon);
        pItem->setText( 0, pT->getName() );
    }
    else
    {
        QIcon iconError;
        iconError.addPixmap( QPixmap( QStringLiteral( ":/icons/tools-report-bug.png" ) ), QIcon::Normal, QIcon::Off );
        pItem->setIcon( 0, iconError );
    }


    showInfo( QString( "Trying to %2 timer <em>%1</em> %3." )
              .arg(pT->getName(), pT->shouldBeActive() ? "activate" : "deactivate", pT->state() ? "succeeded" : QString("failed; reason: ") + pT->getError() ) );
}

void dlgTriggerEditor::slot_alias_toggle_active()
{
    QTreeWidgetItem * pItem = treeWidget_aliases->currentItem();
    if( ! pItem ) return;
    QIcon icon;

    TAlias * pT = mpHost->getAliasUnit()->getAlias(pItem->data(0, Qt::UserRole).toInt());
    if( ! pT ) return;
    pT->setIsActive( ! pT->shouldBeActive() );

    if( pT->isFolder() )
    {
        if( pT->isActive() )
        {
            icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-violet.png" ) ), QIcon::Normal, QIcon::Off );
        }
        else
        {
            icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-violet-locked.png" ) ), QIcon::Normal, QIcon::Off );
        }
    }
    else
    {
        if( pT->isActive() )
        {
            icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked.png" ) ), QIcon::Normal, QIcon::Off );
        }
        else
        {
            icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox.png" ) ), QIcon::Normal, QIcon::Off );
        }
    }

    if( pT->state() )
    {
        pItem->setIcon( 0, icon);
        pItem->setText( 0, pT->getName() );
    }
    else
    {
        QIcon iconError;
        iconError.addPixmap( QPixmap( QStringLiteral( ":/icons/tools-report-bug.png" ) ), QIcon::Normal, QIcon::Off );
        pItem->setIcon( 0, iconError );
    }
    showInfo( QString( "Trying to %2 alias <em>%1</em> %3." )
              .arg(pT->getName(), pT->shouldBeActive() ? "activate" : "deactivate", pT->state() ? "succeeded" : QString("failed; reason: ") + pT->getError() ) );

    if( pItem->childCount() > 0 )
    {
        children_icon_alias( pItem );
    }
}

void dlgTriggerEditor::children_icon_alias( QTreeWidgetItem * pWidgetItemParent )
{
    for( int i = 0; i<pWidgetItemParent->childCount(); i++ )
    {
        QTreeWidgetItem * pItem = pWidgetItemParent->child(i);
        TAlias * pT = mpHost->getAliasUnit()->getAlias(pItem->data(0, Qt::UserRole).toInt());
        if( ! pT ) return;

        QIcon icon;
        if( pItem->childCount() > 0 )
        {
            children_icon_alias( pItem );
        }
        if( pT->state() )
        {
            if( pT->isFolder() )
            {
                if( pT->isActive() )
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-violet.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-grey.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
                else
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-violet-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-grey-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
            }
            else
            {
                if( pT->isActive() )
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked_grey.png" ) ), QIcon::Normal, QIcon::Off );
                    }

                }
                else
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            pItem->setIcon(0, icon);
        }
        else
        {
            QIcon iconError;
            iconError.addPixmap( QPixmap( QStringLiteral( ":/icons/tools-report-bug.png" ) ), QIcon::Normal, QIcon::Off );
            pItem->setIcon( 0, iconError );
            showError( pT->getError() );
        }
    }

}



void dlgTriggerEditor::slot_script_toggle_active()
{
    QTreeWidgetItem * pItem = treeWidget_scripts->currentItem();
    if( ! pItem ) return;
    QIcon icon;

    TScript * pT = mpHost->getScriptUnit()->getScript(pItem->data(0, Qt::UserRole).toInt());
    if( ! pT ) return;

    pT->setIsActive( ! pT->shouldBeActive() );

    if( pT->isFolder() )
    {
        if( pT->isActive() )
        {
            icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-orange.png" ) ), QIcon::Normal, QIcon::Off );
        }
        else
        {
            icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-orange-locked.png" ) ), QIcon::Normal, QIcon::Off );
        }
    }
    else
    {
        if( pT->isActive() )
        {
            icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked.png" ) ), QIcon::Normal, QIcon::Off );
        }
        else
        {
            icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox.png" ) ), QIcon::Normal, QIcon::Off );
        }
    }

    if( pT->state() )
    {
        pItem->setIcon( 0, icon);
        pItem->setText( 0, pT->getName() );
    }
    else
    {
        QIcon iconError;
        iconError.addPixmap( QPixmap( QStringLiteral( ":/icons/tools-report-bug.png" ) ), QIcon::Normal, QIcon::Off );
        pItem->setIcon( 0, iconError );
    }
    showInfo( QString( "Trying to %2 script <em>%1</em> %3." )
              .arg(pT->getName(), pT->shouldBeActive() ? "activate" : "deactivate", pT->state() ? "succeeded" : QString("failed; reason: ") + pT->getError() ) );
}

void dlgTriggerEditor::slot_action_toggle_active()
{
    QTreeWidgetItem * pItem = treeWidget_actions->currentItem();
    if( ! pItem ) return;
    QIcon icon;

    TAction * pT = mpHost->getActionUnit()->getAction(pItem->data(0, Qt::UserRole).toInt());
    if( ! pT ) return;

    pT->setIsActive( ! pT->shouldBeActive() );

    if( pT->isFolder() )
    {
        if( ! pT->getPackageName().isEmpty() )
        {
            // Has a package name - is a module master folder
            if( pT->isActive() )
            {
                icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-brown.png" ) ), QIcon::Normal, QIcon::Off );
            }
            else
            {
                icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-brown-locked.png" ) ), QIcon::Normal, QIcon::Off );
            }
        }
        else if( ! pT->getParent()
              || ! pT->getParent()->getPackageName().isEmpty() )
        {
            // Does not have a parent or the parent has a package name - is a toolbar
            if( pT->isActive() )
            {
                icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-yellow.png" ) ), QIcon::Normal, QIcon::Off );
            }
            else
            {
                icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-yellow-locked.png" ) ), QIcon::Normal, QIcon::Off );
            }
        }
        else
        {
            // Must be a menu
            if( pT->isActive() )
            {
                icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-cyan.png" ) ), QIcon::Normal, QIcon::Off );
            }
            else
            {
                icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-cyan-locked.png" ) ), QIcon::Normal, QIcon::Off );
            }
        }
    }
    else
    {
        if( pT->isActive() )
        {
            icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked.png" ) ), QIcon::Normal, QIcon::Off );
        }
        else
        {
            icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox.png" ) ), QIcon::Normal, QIcon::Off );
        }
    }

    if( pT->state() )
    {
        if( pT->shouldBeActive() )
        {
            showInfo( tr( "Trying to activate a button/menu/toolbar or the part of a module \"%1\" that contains them <em>succeeded</em>." )
                      .arg( pT->getName() ) );
        }
        else
        {
            showInfo( tr( "Trying to deactivate a button/menu/toolbar or the part of a module \"%1\" that contains them <em>succeeded</em>." )
                      .arg( pT->getName() ) );
        }
    }
    else
    {
        pT->setIsActive( false );
        showError( tr( "Unable to activate (and automatically deactivating) a button/menu/toolbar or the part of a module \"%1\" that contains them; reason: %2." )
                   .arg( pT->getName(), pT->getError() ) );
        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tools-report-bug.png" ) ), QIcon::Normal, QIcon::Off );
    }
    pItem->setIcon( 0, icon);
    pItem->setText( 0, pT->getName() );

    mpHost->getActionUnit()->updateToolbar();
}

void dlgTriggerEditor::slot_key_toggle_active()
{
    QTreeWidgetItem * pItem = treeWidget_keys->currentItem();
    if( ! pItem ) return;
    QIcon icon;

    TKey * pT = mpHost->getKeyUnit()->getKey(pItem->data(0, Qt::UserRole).toInt());
    if( ! pT ) return;

    pT->setIsActive( ! pT->shouldBeActive() );

    if( pT->isFolder() )
    {
        if( pT->isActive() )
        {
            icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-pink.png")), QIcon::Normal, QIcon::Off);
        }
        else
        {
            icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-pink-locked.png")), QIcon::Normal, QIcon::Off);
        }
    }
    else
    {
        if( pT->isActive() )
        {
            icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked.png" ) ), QIcon::Normal, QIcon::Off );
        }
        else
        {
            icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox.png" ) ), QIcon::Normal, QIcon::Off );
        }
    }

    if( pT->state() )
    {
        pItem->setIcon( 0, icon);
        pItem->setText( 0, pT->getName() );
    }
    else
    {
        QIcon iconError;
        iconError.addPixmap( QPixmap( QStringLiteral( ":/icons/tools-report-bug.png" ) ), QIcon::Normal, QIcon::Off );
        pItem->setIcon( 0, iconError );
    }
    showInfo( QString( "Trying to %2 key <em>%1</em> %3." )
              .arg(pT->getName(), pT->shouldBeActive() ? "activate" : "deactivate", pT->state() ? "succeeded" : QString("failed; reason: ") + pT->getError() ) );
    if( pItem->childCount() > 0 )
    {
        children_icon_key( pItem );
    }
}

void dlgTriggerEditor::children_icon_key( QTreeWidgetItem * pWidgetItemParent )
{
    for( int i = 0; i<pWidgetItemParent->childCount(); i++ )
    {
        QTreeWidgetItem * pItem = pWidgetItemParent->child(i);
        TKey * pT = mpHost->getKeyUnit()->getKey(pItem->data(0, Qt::UserRole).toInt());
        if( ! pT ) return;

        QIcon icon;
        if( pItem->childCount() > 0 )
        {
            children_icon_key( pItem );
        }
        if( pT->state() )
        {
            if( pT->isFolder() )
            {
                if( pT->isActive() )
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-pink.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-grey.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
                else
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-pink-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-grey-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
            }
            else
            {
                if( pT->isActive() )
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked_grey.png" ) ), QIcon::Normal, QIcon::Off );
                    }

                }
                else
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            pItem->setIcon(0, icon);
        }
        else
        {
            QIcon iconError;
            iconError.addPixmap( QPixmap( QStringLiteral( ":/icons/tools-report-bug.png" ) ), QIcon::Normal, QIcon::Off );
            pItem->setIcon( 0, iconError );
            showError( pT->getError() );
        }
    }

}


void dlgTriggerEditor::addTrigger( bool isFolder )
{
    saveTrigger();
    QString name;
    if( isFolder ) name = "New Trigger Group";
    else name = "New Trigger";
    QStringList regexList;
    QList<int> regexPropertyList;
    QString script = "";
    QStringList nameL;
    nameL << name;

    QTreeWidgetItem * pParent = treeWidget_triggers->currentItem();
    QTreeWidgetItem * pNewItem = 0;
    TTrigger * pT = 0;

    if( pParent )
    {
        int parentID = pParent->data(0, Qt::UserRole).toInt();

        TTrigger * pParentTrigger = mpHost->getTriggerUnit()->getTrigger( parentID );
        if( pParentTrigger )
        {
            // insert new items as siblings unless the parent is a folder
            if( ! pParentTrigger->isFolder() )
            {
                // handle root items
                if( ! pParentTrigger->getParent() )
                {
                    goto ROOT_TRIGGER;
                }
                else
                {
                    // insert new item as sibling of the clicked item
                    if( pParent->parent() )
                    {
                        pT = new TTrigger( pParentTrigger->getParent(), mpHost );
                        pNewItem = new QTreeWidgetItem( pParent->parent(), nameL );
                        pParent->parent()->insertChild( 0, pNewItem );
                    }
                }
            }
            else
            {
                pT = new TTrigger( pParentTrigger, mpHost );
                pNewItem = new QTreeWidgetItem( pParent, nameL );
                pParent->insertChild( 0, pNewItem );
            }
        }
        else
            goto ROOT_TRIGGER;
    }
    else
    {
        //insert a new root item
    ROOT_TRIGGER:
        pT = new TTrigger( name, regexList, regexPropertyList, false, mpHost );
        pNewItem = new QTreeWidgetItem( mpTriggerBaseItem, nameL );
        treeWidget_triggers->insertTopLevelItem( 0, pNewItem );
    }

    if( ! pT ) return;


    pT->setName( name );
    pT->setRegexCodeList( regexList, regexPropertyList );
    pT->setScript( script );
    pT->setIsFolder( isFolder );
    pT->setIsActive( false );
    pT->setIsMultiline( false );
    pT->mStayOpen = 0;
    pT->setConditionLineDelta( 0 );
    pT->registerTrigger();
    int childID = pT->getID();
    pNewItem->setData( 0, Qt::UserRole, childID );
    QIcon icon;
    if( isFolder )
    {
        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-red.png" ) ), QIcon::Normal, QIcon::Off );
    }
    else
    {
        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/document-save-as.png" ) ), QIcon::Normal, QIcon::Off );
    }
    pNewItem->setIcon( 0, icon );
    if( pParent ) pParent->setExpanded( true );
    mpTriggersMainArea->lineEdit_trigger_name->clear();
    mpTriggersMainArea->perlSlashGOption->setChecked( false );
    mpSourceEditor->clear();
    mpTriggersMainArea->trigger_command->clear();
    mpTriggersMainArea->filterTrigger->setChecked( false );
    mpTriggersMainArea->spinBox_stayOpen->setValue( 0 );
    mpTriggersMainArea->spinBox_linemargin->setValue( 0 );
    mpTriggersMainArea->checkBox_multlinetrigger->setChecked( false );

    mpTriggersMainArea->pushButtonFgColor->setChecked( false );
    mpTriggersMainArea->pushButtonBgColor->setChecked( false );
    mpTriggersMainArea->colorizerTrigger->setChecked( false );

    mpCurrentTriggerItem = pNewItem;
    treeWidget_triggers->setCurrentItem( pNewItem );
    showInfo( msgInfoAddTrigger );
    slot_trigger_selected( treeWidget_triggers->currentItem() );
}



void dlgTriggerEditor::addTimer( bool isFolder )
{
    saveTimer();
    QString name;
    if( isFolder ) name = "New Timer Group";
    else name = "New Timer";
    QString command = "";
    QTime time;
    QString script = "";
    QStringList nameL;
    nameL << name;

    QTreeWidgetItem * pParent = treeWidget_timers->currentItem();
    QTreeWidgetItem * pNewItem = 0;
    TTimer * pT = 0;

    if( pParent )
    {
        int parentID = pParent->data(0, Qt::UserRole).toInt();

        TTimer * pParentTrigger = mpHost->getTimerUnit()->getTimer( parentID );
        if( pParentTrigger )
        {
            // insert new items as siblings unless the parent is a folder
            if( ! pParentTrigger->isFolder() )
            {
                // handle root items
                if( ! pParentTrigger->getParent() )
                {
                    goto ROOT_TIMER;
                }
                else
                {
                    // insert new item as sibling of the clicked item
                    if( pParent->parent() )
                    {
                        pT = new TTimer( pParentTrigger->getParent(), mpHost );
                        pNewItem = new QTreeWidgetItem( pParent->parent(), nameL );
                        pParent->parent()->insertChild( 0, pNewItem );
                    }
                }
            }
            else
            {
                pT = new TTimer( pParentTrigger, mpHost );
                pNewItem = new QTreeWidgetItem( pParent, nameL );
                pParent->insertChild( 0, pNewItem );
            }
        }
        else
            goto ROOT_TIMER;
    }
    else
    {
        //insert a new root item
    ROOT_TIMER:
        pT = new TTimer( name, time, mpHost );
        pNewItem = new QTreeWidgetItem( mpTimerBaseItem, nameL );
        treeWidget_timers->insertTopLevelItem( 0, pNewItem );
    }

    if( ! pT ) return;

    pT->setName( name );
    pT->setCommand( command );
    pT->setScript( script );
    pT->setIsFolder( isFolder );
    pT->setIsActive( false );
    pT->registerTimer();
    int childID = pT->getID();
    pNewItem->setData( 0, Qt::UserRole, childID );
    QIcon icon;
    if( isFolder )
    {
        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-red.png" ) ), QIcon::Normal, QIcon::Off );
    }
    else
    {
        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/document-save-as.png" ) ), QIcon::Normal, QIcon::Off );
    }
    pNewItem->setIcon( 0, icon );
    if( pParent ) pParent->setExpanded( true );
    //FIXME
    //mpOptionsAreaTriggers->lineEdit_trigger_name->clear();
    mpTimersMainArea->lineEdit_command->clear();
    mpSourceEditor->clear();
    mpCurrentTimerItem = pNewItem;
    treeWidget_timers->setCurrentItem( pNewItem );
    showInfo( msgInfoAddTimer );
    slot_timer_selected( treeWidget_timers->currentItem() );
}

void dlgTriggerEditor::addVar( bool isFolder )
{
    saveVar();
    QString name;
    mpVarsMainArea->key_type->setCurrentIndex(0);
    if (isFolder)
    {
        mpSourceEditor->setReadOnly(true);
        mpVarsMainArea->var_type->setDisabled(true);
        mpVarsMainArea->var_type->setCurrentIndex(4);
        mpVarsMainArea->lineEdit_var_name->setText("");
        mpVarsMainArea->lineEdit_var_name->setPlaceholderText("Table name...");
        mpSourceEditor->setPlainText("NewTable");
        name="";
    }
    else{
        mpSourceEditor->setReadOnly(false);
        mpVarsMainArea->lineEdit_var_name->setText("");
        mpVarsMainArea->lineEdit_var_name->setPlaceholderText("Variable name...");
        mpVarsMainArea->var_type->setDisabled(false);
        mpVarsMainArea->var_type->setCurrentIndex(0);
        name = "";
    }
    QStringList nameL;
    nameL << name;
    QTreeWidgetItem * cItem = treeWidget_variables->currentItem();
    LuaInterface * lI = mpHost->getLuaInterface();
    VarUnit * vu = lI->getVarUnit();
    TVar * cVar = vu->getWVar( cItem );
    QTreeWidgetItem * pParent;
    QTreeWidgetItem * newItem;
    if ( cVar && cVar->getValueType() == LUA_TTABLE )
        pParent = cItem;
    else
        pParent = cItem->parent();
    auto newVar = new TVar();
    if (pParent)
    {
        //we're nested under something, or going to be.  This HAS to be a table
        TVar * parent = vu->getWVar(pParent);
        if ( parent && parent->getValueType() == LUA_TTABLE )
        {
            //create it under the parent
            newItem = new QTreeWidgetItem( pParent, nameL );
            newVar->setParent( parent );
        }
        else
        {
            newItem = new QTreeWidgetItem( mpVarBaseItem, nameL );
            newVar->setParent( vu->getBase() );
        }
    }
    else
    {
        newItem = new QTreeWidgetItem( mpVarBaseItem, nameL );
        newVar->setParent( vu->getBase() );
    }
    if (isFolder)
    {
        newVar->setValue( "", LUA_TTABLE);
    }
    else
        newVar->setValueType(LUA_TNONE);
    vu->addTempVar( newItem, newVar );
    newItem->setFlags(newItem->flags() & ~(Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled ));
    if (newItem)
    {
        mpCurrentVarItem = newItem;
        treeWidget_variables->setCurrentItem( newItem );
        showInfo( msgInfoAddVar );
        slot_var_selected( treeWidget_variables->currentItem() );
    }
}

void dlgTriggerEditor::addKey( bool isFolder )
{
    saveKey();
    QString name;
    if( isFolder ) name = "New Key Group";
    else name = "New Key";
    QString script = "";
    QStringList nameL;
    nameL << name;

    QTreeWidgetItem * pParent = treeWidget_keys->currentItem();
    QTreeWidgetItem * pNewItem = 0;
    TKey * pT = 0;

    if( pParent )
    {
        int parentID = pParent->data(0, Qt::UserRole).toInt();

        TKey * pParentTrigger = mpHost->getKeyUnit()->getKey( parentID );
        if( pParentTrigger )
        {
            // insert new items as siblings unless the parent is a folder
            if( ! pParentTrigger->isFolder() )
            {
                // handle root items
                if( ! pParentTrigger->getParent() )
                {
                    goto ROOT_KEY;
                }
                else
                {
                    // insert new item as sibling of the clicked item
                    if( pParent->parent() )
                    {
                        pT = new TKey( pParentTrigger->getParent(), mpHost );
                        pNewItem = new QTreeWidgetItem( pParent->parent(), nameL );
                        pParent->parent()->insertChild( 0, pNewItem );
                    }
                }
            }
            else
            {
                pT = new TKey( pParentTrigger, mpHost );
                pNewItem = new QTreeWidgetItem( pParent, nameL );
                pParent->insertChild( 0, pNewItem );
            }
        }
        else
            goto ROOT_KEY;
    }
    else
    {
        //insert a new root item
    ROOT_KEY:
        pT = new TKey( name, mpHost );
        pNewItem = new QTreeWidgetItem( mpKeyBaseItem, nameL );
        treeWidget_keys->insertTopLevelItem( 0, pNewItem );
    }

    if( ! pT ) return;

    pT->setName( name );
    pT->setKeyCode( -1 );
    pT->setKeyModifiers( -1 );
    pT->setScript( script );
    pT->setIsFolder( isFolder );
    pT->setIsActive( false );
    pT->registerKey();
    int childID = pT->getID();
    pNewItem->setData( 0, Qt::UserRole, childID );
    QIcon icon;
    if( isFolder )
    {
        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-red.png" ) ), QIcon::Normal, QIcon::Off );
    }
    else
    {
        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/document-save-as.png" ) ), QIcon::Normal, QIcon::Off );
    }
    pNewItem->setIcon( 0, icon );
    if( pParent ) pParent->setExpanded( true );
    mpKeysMainArea->lineEdit_command->clear();
    mpKeysMainArea->lineEdit_key->setText("no key chosen");
    mpSourceEditor->clear();
    mpCurrentKeyItem = pNewItem;
    treeWidget_keys->setCurrentItem( pNewItem );
    showInfo( msgInfoAddKey );
    slot_key_selected( treeWidget_keys->currentItem() );
}


void dlgTriggerEditor::addAlias( bool isFolder )
{
    saveAlias();
    QString name;
    if( isFolder ) name = "New Alias Group";
    else name = "New Alias";
    QString regex = "";
    QString command = "";
    QString script = "";
    QStringList nameL;
    nameL << name;

    QTreeWidgetItem * pParent = treeWidget_aliases->currentItem();
    QTreeWidgetItem * pNewItem = 0;
    TAlias * pT = 0;

    if( pParent )
    {
        int parentID = pParent->data(0, Qt::UserRole).toInt();

        TAlias * pParentTrigger = mpHost->getAliasUnit()->getAlias( parentID );
        if( pParentTrigger )
        {
            // insert new items as siblings unless the parent is a folder
            if( ! pParentTrigger->isFolder() )
            {
                // handle root items
                if( ! pParentTrigger->getParent() )
                {
                    goto ROOT_ALIAS;
                }
                else
                {
                    // insert new item as sibling of the clicked item
                    if( pParent->parent() )
                    {
                        pT = new TAlias( pParentTrigger->getParent(), mpHost );
                        pNewItem = new QTreeWidgetItem( pParent->parent(), nameL );
                        pParent->parent()->insertChild( 0, pNewItem );
                    }
                }
            }
            else
            {
                pT = new TAlias( pParentTrigger, mpHost );
                pNewItem = new QTreeWidgetItem( pParent, nameL );
                pParent->insertChild( 0, pNewItem );
            }
        }
        else
            goto ROOT_ALIAS;
    }
    else
    {
        //insert a new root item
ROOT_ALIAS:
        pT = new TAlias( name, mpHost );
        pT->setRegexCode( regex ); // Empty regex will always succeed to compile
        pNewItem = new QTreeWidgetItem( mpAliasBaseItem, nameL );
        treeWidget_aliases->insertTopLevelItem( 0, pNewItem );
    }

    if( ! pT ) return;

    pT->setName( name );
    pT->setCommand( command );
    pT->setRegexCode( regex ); // Empty regex will always succeed to compile
    pT->setScript( script );
    pT->setIsFolder( isFolder );
    pT->setIsActive( false );
    pT->registerAlias();
    int childID = pT->getID();
    pNewItem->setData( 0, Qt::UserRole, childID );
    QIcon icon;
    if( isFolder )
    {
        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-red.png" ) ), QIcon::Normal, QIcon::Off );
    }
    else
    {
        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/document-save-as.png" ) ), QIcon::Normal, QIcon::Off );
    }
    pNewItem->setIcon( 0, icon );
    if( pParent ) pParent->setExpanded( true );

    mpAliasMainArea->lineEdit_alias_name->clear();
    mpAliasMainArea->pattern_textedit->clear();
    mpAliasMainArea->substitution->clear();
    mpSourceEditor->clear();

    mpAliasMainArea->lineEdit_alias_name->setText( name );

    mpCurrentAliasItem = pNewItem;
    treeWidget_aliases->setCurrentItem( pNewItem );
    showInfo(msgInfoAddAlias);
    slot_alias_selected( treeWidget_aliases->currentItem() );
}

void dlgTriggerEditor::addAction( bool isFolder )
{
    saveAction();
    QString name;
    if( isFolder ) name = "new menu";
    else name = "new button";
    QString cmdButtonUp = "";
    QString cmdButtonDown = "";
    QString script = "";
    QStringList nameL;
    nameL << name;

    QTreeWidgetItem * pParent = treeWidget_actions->currentItem();
    QTreeWidgetItem * pNewItem = 0;
    TAction * pT = 0;

    if( pParent )
    {
        int parentID = pParent->data(0, Qt::UserRole).toInt();

        TAction * pParentTrigger = mpHost->getActionUnit()->getAction( parentID );
        if( pParentTrigger )
        {
            // insert new items as siblings unless the parent is a folder
            if( ! pParentTrigger->isFolder() )
            {
                // handle root items
                if( ! pParentTrigger->getParent() )
                {
                    goto ROOT_ACTION;
                }
                else
                {
                    // insert new item as sibling of the clicked item
                    if( pParent->parent() )
                    {
                        pT = new TAction( pParentTrigger->getParent(), mpHost );
                        pNewItem = new QTreeWidgetItem( pParent->parent(), nameL );
                        pParent->parent()->insertChild( 0, pNewItem );
                    }
                }
            }
            else
            {
                pT = new TAction( pParentTrigger, mpHost );
                pNewItem = new QTreeWidgetItem( pParent, nameL );
                pParent->insertChild( 0, pNewItem );
            }
        }
        else
            goto ROOT_ACTION;
    }
    else
    {
        //insert a new root item
    ROOT_ACTION:
        name = "new toolbar";
        pT = new TAction( name, mpHost );
        pT->setCommandButtonUp( cmdButtonUp );
        QStringList nl;
        nl << name;
        pNewItem = new QTreeWidgetItem( mpActionBaseItem, nl );
        treeWidget_actions->insertTopLevelItem( 0, pNewItem );
    }

    if( ! pT ) return;


    pT->setName( name );
    pT->setCommandButtonUp( cmdButtonUp );
    pT->setCommandButtonDown( cmdButtonDown );
    pT->setIsPushDownButton( false );
    pT->mLocation = 1;
    pT->mOrientation = 1;
    pT->setScript( script );
    pT->setIsFolder( isFolder );
    pT->setIsActive( false );
    pT->registerAction();
    int childID = pT->getID();
    pNewItem->setData( 0, Qt::UserRole, childID );
    QIcon icon;
    if( isFolder )
    {
        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-red.png" ) ), QIcon::Normal, QIcon::Off );
    }
    else
    {
        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/document-save-as.png" ) ), QIcon::Normal, QIcon::Off );
    }
    pNewItem->setIcon( 0, icon );
    if( pParent ) pParent->setExpanded( true );
    mpActionsMainArea->lineEdit_action_icon->clear();
    mpActionsMainArea->checkBox_pushdownbutton->setChecked(false);
    mpSourceEditor->clear();

    mpHost->getActionUnit()->updateToolbar();
    mpCurrentActionItem = pNewItem;
    treeWidget_actions->setCurrentItem( pNewItem );
    showInfo( msgInfoAddButton );
    slot_action_selected( treeWidget_actions->currentItem() );
}


void dlgTriggerEditor::addScript( bool isFolder )
{
    saveScript();
    QString name;
    if( isFolder ) name = "New Script Group";
    else name = "NewScript";
    QStringList mainFun;
    mainFun << "-------------------------------------------------\n"
            << "--         Put your Lua functions here.        --\n"
            << "--                                             --\n"
            << "-- Note that you can also use external Scripts --\n"
            << "-------------------------------------------------\n";
    QString script = mainFun.join("");
    QStringList nameL;
    nameL << name;

    QTreeWidgetItem * pParent = treeWidget_scripts->currentItem();
    QTreeWidgetItem * pNewItem = 0;
    TScript * pT = 0;

    if( pParent )
    {
        int parentID = pParent->data(0, Qt::UserRole).toInt();

        TScript * pParentTrigger = mpHost->getScriptUnit()->getScript( parentID );
        if( pParentTrigger )
        {
            // insert new items as siblings unless the parent is a folder
            if( ! pParentTrigger->isFolder() )
            {
                // handle root items
                if( ! pParentTrigger->getParent() )
                {
                    goto ROOT_SCRIPT;
                }
                else
                {
                    // insert new item as sibling of the clicked item
                    if( pParent->parent() )
                    {
                        pT = new TScript( pParentTrigger->getParent(), mpHost );
                        pNewItem = new QTreeWidgetItem( pParent->parent(), nameL );
                        pParent->parent()->insertChild( 0, pNewItem );
                    }
                }
            }
            else
            {
                pT = new TScript( pParentTrigger, mpHost );
                pNewItem = new QTreeWidgetItem( pParent, nameL );
                pParent->insertChild( 0, pNewItem );
            }
        }
        else
            goto ROOT_SCRIPT;
    }
    else
    {
        //insert a new root item
    ROOT_SCRIPT:
        pT = new TScript( name, mpHost );
        pNewItem = new QTreeWidgetItem( mpScriptsBaseItem, nameL );
        treeWidget_scripts->insertTopLevelItem( 0, pNewItem );
    }

    if( ! pT ) return;


    pT->setName( name );
    pT->setScript( script );
    pT->setIsFolder( isFolder );
    pT->setIsActive( false );
    pT->registerScript();
    int childID = pT->getID();
    pNewItem->setData( 0, Qt::UserRole, childID );
    QIcon icon;
    if( isFolder )
    {
        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-red.png" ) ), QIcon::Normal, QIcon::Off );
    }
    else
    {
        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/document-save-as.png" ) ), QIcon::Normal, QIcon::Off );
    }
    pNewItem->setIcon( 0, icon );
    if( pParent ) pParent->setExpanded( true );
    mpScriptsMainArea->lineEdit_scripts_name->clear();
    //FIXME mpScriptsMainArea->pattern_textedit->clear();
    mpSourceEditor->setPlainText( script );
    mpCurrentScriptItem = pNewItem;
    treeWidget_scripts->setCurrentItem( pNewItem );
    slot_scripts_selected( treeWidget_scripts->currentItem() );
}

void dlgTriggerEditor::saveTrigger()
{
    QTime t;
    t.start();
    QTreeWidgetItem * pItem = mpCurrentTriggerItem;
    if( ! pItem ) return;
    if( ! pItem->parent() ) return;

    QString name = mpTriggersMainArea->lineEdit_trigger_name->text();
    QString command = mpTriggersMainArea->trigger_command->text();
    bool isMultiline = mpTriggersMainArea->checkBox_multlinetrigger->isChecked();
    QStringList regexList;
    QList<int> regexPropertyList;
    for( int i=0; i<50; i++ )
    {
        QString pattern = mTriggerPatternEdit[i]->lineEdit->text();
        if( pattern.size() < 1 ) continue;
        regexList << pattern;
        int _type = mTriggerPatternEdit[i]->patternType->currentIndex();
        if( _type == 0 ) regexPropertyList << REGEX_SUBSTRING;
        else if( _type == 1 ) regexPropertyList << REGEX_PERL;
        else if( _type == 2 ) regexPropertyList << REGEX_BEGIN_OF_LINE_SUBSTRING;
        else if( _type == 3 ) regexPropertyList << REGEX_EXACT_MATCH;
        else if( _type == 4 ) regexPropertyList << REGEX_LUA_CODE;
        else if( _type == 5 ) regexPropertyList << REGEX_LINE_SPACER;
        else if( _type == 6 ) regexPropertyList << REGEX_COLOR_PATTERN;
    }
    QString script = mpSourceEditor->toPlainText();


        int triggerID = pItem->data( 0, Qt::UserRole ).toInt();
        TTrigger * pT = mpHost->getTriggerUnit()->getTrigger( triggerID );
        if( pT )
        {
            QString old_name = pT->getName();
            pT->setName( name );
            pT->setCommand( command );
            pT->setRegexCodeList( regexList, regexPropertyList );

            pT->setScript( script );
            pT->setIsMultiline( isMultiline );
            pT->mPerlSlashGOption = mpTriggersMainArea->perlSlashGOption->isChecked();
            pT->mFilterTrigger = mpTriggersMainArea->filterTrigger->isChecked();
            pT->setConditionLineDelta( mpTriggersMainArea->spinBox_linemargin->value() );
            pT->mStayOpen = mpTriggersMainArea->spinBox_stayOpen->value();
            pT->mSoundTrigger = mpTriggersMainArea->soundTrigger->isChecked();
            pT->setSound( mpTriggersMainArea->lineEdit_soundFile->text() );

            QPalette FgColorPalette;
            QPalette BgColorPalette;
            FgColorPalette = mpTriggersMainArea->pushButtonFgColor->palette();
            BgColorPalette = mpTriggersMainArea->pushButtonBgColor->palette();
            QColor fgColor = FgColorPalette.color( QPalette::Button );
            QColor bgColor = BgColorPalette.color( QPalette::Button );
            pT->setFgColor( fgColor );
            pT->setBgColor( bgColor );
            pT->setIsColorizerTrigger( mpTriggersMainArea->colorizerTrigger->isChecked() );
            QIcon icon;
            if( pT->isFilterChain() )
            {
                if( pT->isActive() )
                {
                    if (pT->ancestorsActive()) {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/filter.png" ) ), QIcon::Normal, QIcon::Off );
                    } else {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/filter-grey.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
                else
                {
                    if (pT->ancestorsActive()) {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/filter-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    } else {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/filter-grey-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
            }
            else if( pT->isFolder() )
            {
                if( ! pT->mPackageName.isEmpty() )
                {
                    if( pT->isActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-brown.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-brown-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
                else if( pT->isActive() )
                {
                    if (pT->ancestorsActive()) {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-blue.png" ) ), QIcon::Normal, QIcon::Off );
                    } else {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-grey.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
                else
                {
                    if (pT->ancestorsActive()) {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-blue-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    } else {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-grey-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
            }
            else
            {
                if( pT->isActive() )
                {
                    if (pT->ancestorsActive()) {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked.png" ) ), QIcon::Normal, QIcon::Off );
                    } else {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked_grey.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
                else
                {
                    if (pT->ancestorsActive()) {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox.png" ) ), QIcon::Normal, QIcon::Off );
                    } else {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox-grey.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
            }
            if( pT->state() )
            {
                if( old_name == "New Trigger" || old_name == "New Trigger Group" )
                {
                    QIcon _icon;
                    if( pT->isFolder() )
                    {
                        _icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-blue.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        _icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    pItem->setIcon( 0, _icon );
                    pItem->setText( 0, name );
                    pT->setIsActive( true );
                }
                else
                {
                    pItem->setIcon( 0, icon);
                    pItem->setText( 0, name );
                }
            }
            else
            {
                QIcon iconError;
                pItem->setText( 0, name );
                iconError.addPixmap( QPixmap( QStringLiteral( ":/icons/tools-report-bug.png" ) ), QIcon::Normal, QIcon::Off );
                pItem->setIcon( 0, iconError );
                pT->setIsActive( false );
                showError( pT->getError() );
            }
        }
}

void dlgTriggerEditor::saveTimer()
{
    QTreeWidgetItem * pItem = mpCurrentTimerItem;
    if( ! pItem ) return;
    QString name = mpTimersMainArea->lineEdit_timer_name->text();
    QString script = mpSourceEditor->toPlainText();


        int timerID = pItem->data(0, Qt::UserRole).toInt();
        TTimer * pT = mpHost->getTimerUnit()->getTimer( timerID );
        if( pT )
        {
            pT->setName( name );
            QString command = mpTimersMainArea->lineEdit_command->text();
            int hours = mpTimersMainArea->timeEdit_hours->time().hour();
            int minutes = mpTimersMainArea->timeEdit_minutes->time().minute();
            int secs = mpTimersMainArea->timeEdit_seconds->time().second();
            int msecs = mpTimersMainArea->timeEdit_msecs->time().msec();
            QTime time(hours,minutes,secs,msecs);
            pT->setTime( time );
            pT->setCommand( command );
            pT->setName( name );
            pT->setScript( script );

            QIcon icon;
            if( pT->isFolder() )
            {
                if( ! pT->mPackageName.isEmpty() )
                {
                    if( pT->isActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-brown.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-brown-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
                else
                {
                    if (pT->shouldBeActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-green.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-green-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            if( pT->isOffsetTimer() )
            {
                if( pT->shouldBeActive() )
                {
                    icon.addPixmap( QPixmap( QStringLiteral( ":/icons/offsettimer-on.png" ) ), QIcon::Normal, QIcon::Off );
                }
                else
                {
                    icon.addPixmap( QPixmap( QStringLiteral( ":/icons/offsettimer-off.png" ) ), QIcon::Normal, QIcon::Off );
                }
            }
            else
            {
                if( pT->shouldBeActive() )
                {
                    icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked.png" ) ), QIcon::Normal, QIcon::Off );
                    pT->setIsActive( true );
                }
                else
                {
                    icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox.png" ) ), QIcon::Normal, QIcon::Off );
                }
            }

            if( pT->state() )
            {
                pItem->setIcon( 0, icon);
                pItem->setText( 0, name );
            }
            else
            {
                QIcon iconError;
                iconError.addPixmap( QPixmap( QStringLiteral( ":/icons/tools-report-bug.png" ) ), QIcon::Normal, QIcon::Off );
                pItem->setIcon( 0, iconError );
                pItem->setText( 0, name );

            }

        }
}

void dlgTriggerEditor::saveAlias()
{
    QTreeWidgetItem * pItem = mpCurrentAliasItem;
    if( ! pItem )
    {
        return;
    }

    QString name = mpAliasMainArea->lineEdit_alias_name->text();
    QString regex = mpAliasMainArea->pattern_textedit->text();
    if( (name.size() < 1) || (name=="New Alias") )
    {
        name = regex;
    }
    QString substitution = mpAliasMainArea->substitution->text();
    //check if sub will trigger regex, ignore if there's nothing in regex - could be an alias group
    QRegExp rx(regex);
    if ( !regex.isEmpty() && rx.indexIn(substitution) != -1 )
    {
        //we have a loop
        QIcon iconError;
        iconError.addPixmap( QPixmap( QStringLiteral( ":/icons/tools-report-bug.png" ) ), QIcon::Normal, QIcon::Off );
        pItem->setIcon( 0, iconError );
        pItem->setText( 0, name );
        showError(QString( "Alias <em>%1</em> has an infinite loop - substitution matches its own pattern. Please fix it - this alias isn't good as it'll call itself forever." )
                  .arg(name));
        return;
    }
    QString script = mpSourceEditor->toPlainText();


        int triggerID = pItem->data(0, Qt::UserRole).toInt();
        TAlias * pT = mpHost->getAliasUnit()->getAlias( triggerID );
        if( pT )
        {
            QString old_name = pT->getName();
            pT->setName( name );
            pT->setCommand( substitution );
            pT->setRegexCode( regex ); // This could generate an error state if regex does not compile
            pT->setScript( script );

            QIcon icon;
            if( pT->isFolder() )
            {
                if( ! pT->mPackageName.isEmpty() )
                {
                    if( pT->isActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-brown.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-brown-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
                else if( pT->isActive() )
                {
                    if( pT->ancestorsActive() )
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-violet.png" ) ), QIcon::Normal, QIcon::Off );
                    else
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-grey.png" ) ), QIcon::Normal, QIcon::Off );
                }
                else
                {
                    if( pT->ancestorsActive() )
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-violet-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    else
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-grey-locked.png" ) ), QIcon::Normal, QIcon::Off );
                }
            }
            else
            {
                if( pT->isActive() )
                {
                    if( pT->ancestorsActive() )
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked.png" ) ), QIcon::Normal, QIcon::Off );
                    else
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked_grey.png" ) ), QIcon::Normal, QIcon::Off );
                }
                else
                {
                    if( pT->ancestorsActive() )
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox.png" ) ), QIcon::Normal, QIcon::Off );
                    else
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox-grey.png" ) ), QIcon::Normal, QIcon::Off );
                }
            }

            if( pT->state() )
            {
                if( old_name == "New Alias" )
                {
                    QIcon _icon;
                    if( pT->isFolder() )
                    {
                        if (pT->ancestorsActive()) {
                            _icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-violet.png" ) ), QIcon::Normal, QIcon::Off );
                        } else {
                            _icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-grey.png" ) ), QIcon::Normal, QIcon::Off );
                        }
                    }
                    else
                    {
                        if (pT->ancestorsActive()) {
                            _icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked.png" ) ), QIcon::Normal, QIcon::Off );
                        } else {
                            _icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked_grey.png" ) ), QIcon::Normal, QIcon::Off );
                        }
                    }
                    pItem->setIcon( 0, _icon );
                    pItem->setText( 0, name );
                    pT->setIsActive( true );
                }
                else
                {
                    pItem->setIcon( 0, icon);
                    pItem->setText( 0, name );
                }
            }
            else
            {
                QIcon iconError;
                iconError.addPixmap( QPixmap( QStringLiteral( ":/icons/tools-report-bug.png" ) ), QIcon::Normal, QIcon::Off );
                pItem->setIcon( 0, iconError );
                pItem->setText( 0, name );
                showError( pT->getError() );
            }
        }
}

void dlgTriggerEditor::saveAction()
{
    QTreeWidgetItem * pItem = mpCurrentActionItem;
    if( ! pItem ) return;

    QString name = mpActionsMainArea->lineEdit_action_name->text();
    QString icon = mpActionsMainArea->lineEdit_action_icon->text();
    QString commandDown = mpActionsMainArea->lineEdit_command_down->text();
    QString commandUp = mpActionsMainArea->lineEdit_command_up->text();
    QString script = mpSourceEditor->toPlainText();
    // currentIndex() can return -1 if no setting was previously made - need to fixup:
    int rotation = qMax( 0, mpActionsMainArea->buttonRotation->currentIndex() );
    int columns = mpActionsMainArea->buttonColumns->text().toInt();
    bool isChecked = mpActionsMainArea->checkBox_pushdownbutton->isChecked();
    // bottom location is no longer supported i.e. location = 1 = 0 = location top
    // currentIndex() can return -1 if no setting was previously made - need to fixup:
    int location = qMax( 0, mpActionsMainArea->comboBox_location->currentIndex() );
    if( location > 0 ) location++;

    // currentIndex() can return -1 if no setting was previously made - need to fixup:
    int orientation = qMax( 0, mpActionsMainArea->comboBox_orientation->currentIndex() );

    // This is an unnecessary level of indentation but has been retained to
    // reduce the noise in a git commit/diff caused by the removal of a
    // redundent "if( pITem )" - can be removed next time the file is modified
        int triggerID = pItem->data(0, Qt::UserRole).toInt();
        TAction * pT = mpHost->getActionUnit()->getAction( triggerID );
        if( pT )
        {
            // Do not change anything for a module master folder - it won't "take"
            if( pT->mPackageName.isEmpty() )
            {
                pT->setName( name );
                pT->setIcon( icon );
                pT->setScript( script );
                pT->setCommandButtonDown( commandDown );
                pT->setCommandButtonUp( commandUp );
                pT->setIsPushDownButton( isChecked );
                pT->mLocation = location;
                pT->mOrientation = orientation;
                pT->setIsActive( pT->shouldBeActive() );
                pT->setButtonRotation( rotation );
                pT->setButtonColumns( columns );
                pT->mUseCustomLayout = false;
                pT->css = mpActionsMainArea->css->toPlainText();
            }

            QIcon icon;
            if( pT->isFolder() )
            {
                if( ! pT->mPackageName.isEmpty() )
                {
                    // Has a package name so is a module master folder
                    if( pT->isActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-brown.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-brown-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
                else if( ! pT->getParent()
                      || ! pT->getParent()->mPackageName.isEmpty() )
                {
                    // No parent or it has a parent with a package name so is a toolbar
                    if( pT->isActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-yellow.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-yellow-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
                else
                {
                    // Else must be a menu
                    if( pT->isActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-cyan.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-cyan-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
            }
            else
            {
                // Is a button
                if( pT->isActive() )
                {
                    icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked.png" ) ), QIcon::Normal, QIcon::Off );
                }
                else
                {
                    icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox.png" ) ), QIcon::Normal, QIcon::Off );
                }
            }

            if( pT->state() )
            {
                pItem->setIcon( 0, icon);
                pItem->setText( 0, name );

            }
            else
            {
                QIcon iconError;
                iconError.addPixmap( QPixmap( QStringLiteral( ":/icons/tools-report-bug.png" ) ), QIcon::Normal, QIcon::Off );
                pItem->setIcon( 0, iconError );
                pItem->setText( 0, name );
            }
        }

    mpHost->getActionUnit()->updateToolbar();
    mudlet::self()->processEventLoopHack();
}

void dlgTriggerEditor::saveScript()
{
    QTreeWidgetItem * pItem = mpCurrentScriptItem;
    if( ! pItem ) return;

    QString old_name;
    QString name = mpScriptsMainArea->lineEdit_scripts_name->text();
    QString script = mpSourceEditor->toPlainText();
    mpScriptsMainAreaEditHandlerItem = 0;
    QList<QListWidgetItem*> itemList;
    for( int i=0; i<mpScriptsMainArea->listWidget_registered_event_handlers->count(); i++ )
    {
        QListWidgetItem * pItem = mpScriptsMainArea->listWidget_registered_event_handlers->item(i);
        itemList << pItem;
    }
    QStringList handlerList;
    for(auto & listWidgetItem : itemList)
    {
        if( listWidgetItem->text().size() < 1 ) continue;
        handlerList << listWidgetItem->text();
    }


        int triggerID = pItem->data(0, Qt::UserRole).toInt();
        TScript * pT = mpHost->getScriptUnit()->getScript( triggerID );
        if( pT )
        {
            old_name = pT->getName();
            pT->setName( name );
            pT->setEventHandlerList( handlerList );
            pT->setScript( script );

            pT->compile();
            QIcon icon;
            if( pT->isFolder() )
            {
                if( ! pT->mPackageName.isEmpty() )
                {
                    if( pT->isActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-brown.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-brown-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
                else
                {
                    if (pT->isActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-orange.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-orange-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            else
            {
                if( pT->isActive() )
                {
                    icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked.png" ) ), QIcon::Normal, QIcon::Off );
                }
                else
                {
                    icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox.png" ) ), QIcon::Normal, QIcon::Off );
                }
            }

            if( pT->state() )
            {
                if( old_name == "New Script" || old_name == "New Script Group" )
                {
                    QIcon _icon;
                    if( pT->isFolder() )
                    {
                        _icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-orange.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        _icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    pItem->setIcon( 0, _icon );
                    pItem->setText( 0, name );
                    pT->setIsActive( true );
                }
                else
                {
                    pItem->setIcon( 0, icon);
                    pItem->setText( 0, name );
                }
            }
            else
            {
                QIcon iconError;
                iconError.addPixmap( QPixmap( QStringLiteral( ":/icons/tools-report-bug.png" ) ), QIcon::Normal, QIcon::Off );
                pItem->setIcon( 0, iconError );
                pItem->setText( 0, name );
            }
        }
}

int dlgTriggerEditor::canRecast(QTreeWidgetItem * pItem, int nameType, int valueType)
{
    //basic checks, return 1 if we can recast, 2 if no need to recast, 0 if we can't recast
    LuaInterface * lI = mpHost->getLuaInterface();
    VarUnit * vu = lI->getVarUnit();
    TVar * var = vu->getWVar( pItem );
    if (!var)
        return 2;
    int cNameType = var->getKeyType();
    int cValueType = var->getValueType();
    //most anything is ok to do.  We just want to enforce these rules:
    //you cannot change the type of a table that has children
    //rule removed to see if anything bad happens:
    //you cannot change anything to a table that isn't a table already
    if ( cValueType == LUA_TFUNCTION || cNameType == LUA_TTABLE )
        return 0;//no recasting functions or table keys
    if ( valueType == LUA_TTABLE && cValueType != LUA_TTABLE )
    {
        //trying to change a table to something else
        if ( var->getChildren(false).size() )
        {
            return 0;
        }
        //no children, we can do this without bad things happening
        return 1;
    }
    if ( valueType == LUA_TTABLE && cValueType != LUA_TTABLE )
        return 1;//non-table to table
    if ( cNameType == nameType && cValueType == valueType )
        return 2;
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

    if (!mpCurrentVarItem)
        return;
    QTreeWidgetItem * pItem = mpCurrentVarItem;
    if (!pItem->parent())
        return;
    LuaInterface * lI = mpHost->getLuaInterface();
    VarUnit * vu = lI->getVarUnit();
    TVar * var = vu->getWVar(pItem);
    bool newVar = false;
    if ( !var )
    {
        newVar = true;
        var = vu->getTVar(pItem);
    }
    if ( !var )
        return;
    QString newName = mpVarsMainArea->lineEdit_var_name->text();
    QString newValue = mpSourceEditor->toPlainText();
    if (newName == "")
    {
        slot_var_selected(pItem);
        return;
    }
    mChangingVar = true;
    int nameType = mpVarsMainArea->key_type->itemData(mpVarsMainArea->key_type->currentIndex(), Qt::UserRole).toInt();
    int valueType = mpVarsMainArea->var_type->itemData(mpVarsMainArea->var_type->currentIndex(), Qt::UserRole).toInt();
    if ( ( nameType == 3 || nameType == 4 ) && newVar )
        nameType = -1;
    //check variable recasting
    int varRecast = canRecast(pItem,nameType,valueType);
    if ( ( nameType == -1 ) || ( var && nameType != var->getKeyType() ) )
    {
        if ( QString( newName ).toInt() )
            nameType = LUA_TNUMBER;
        else
            nameType = LUA_TSTRING;
    }
    if ( ( valueType != LUA_TTABLE ) && ( valueType == -1 ) )
    {
        if ( newValue.toInt() )
            valueType = LUA_TNUMBER;
        else if ( newValue.toLower() == "true" || newValue.toLower() == "false" )
            valueType = LUA_TBOOLEAN;
        else
            valueType = LUA_TSTRING;
    }
    if (varRecast == 2)
    {
        //we sometimes get in here from new variables
        if ( newVar )
        {
            //we're making this var
            var = vu->getTVar( pItem );
            if ( ! var )
                var = new TVar();
            var->setName( newName, nameType );
            var->setValue( newValue, valueType );
            lI->createVar( var );
            vu->addVariable(var);
            vu->addTreeItem( pItem, var );
            vu->removeTempVar( pItem );
            pItem->setText( 0, newName );
            mpCurrentVarItem = 0;
        }
        else if ( var )
        {
            if ( newName == var->getName() && ( var->getValueType() == LUA_TTABLE && newValue == var->getValue() ) )
            {
                //no change made
            }
            else
            {
                //we're trying to rename it/recast it
                int change = 0;
                if ( newName != var->getName() || nameType != var->getKeyType() )
                {
                    //lets make sure the nametype works
                    if ( var->getKeyType() == LUA_TNUMBER && newName.toInt() )
                        nameType = LUA_TNUMBER;
                    else
                        nameType = LUA_TSTRING;
                    change = change|0x1;
                }
                var->setNewName( newName, nameType );
                if ( var->getValueType() != LUA_TTABLE && ( newValue != var->getValue() || valueType != var->getValueType() ) )
                {
                    //lets check again
                    if ( var->getValueType() == LUA_TTABLE )
                    {
                        //HEIKO: obvious logic error used to be valueType == LUA_TABLE
                        valueType = LUA_TTABLE;
                    }
                    else if ( valueType == LUA_TNUMBER && newValue.toInt() )
                        valueType = LUA_TNUMBER;
                    else if ( valueType == LUA_TBOOLEAN && ( newValue.toLower() == "true" || newValue.toLower() == "false" ) )
                        valueType = LUA_TBOOLEAN;
                    else
                        valueType = LUA_TSTRING;//nope, you don't agree, you lose your value
                    var->setValue( newValue, valueType );
                    change = change|0x2;
                }
                if ( change )
                {
                    if ( change&0x1 || newVar )
                        lI->renameVar( var );
                    if ( ( var->getValueType() != LUA_TTABLE && change&0x2 ) || newVar )
                        lI->setValue( var );
                    pItem->setText( 0, newName );
                    mpCurrentVarItem = 0;
                }
                else
                    var->clearNewName();
            }
        }
    }
    else if (varRecast == 1)
    {//recast it
        TVar * var = vu->getWVar(pItem);
        if ( newVar )
        {
            //we're making this var
            var = vu->getTVar( pItem );
            var->setName( newName, nameType );
            var->setValue( newValue, valueType );
            lI->createVar( var );
            vu->addVariable(var);
            vu->addTreeItem( pItem, var );
            pItem->setText( 0, newName );
            mpCurrentVarItem = 0;
        }
        else if ( var )
        {
            //we're trying to rename it/recast it
            int change = 0;
            if ( newName != var->getName() || nameType != var->getKeyType() )
            {
                //lets make sure the nametype works
                if ( nameType == LUA_TSTRING )
                {
                    //do nothing, we can always make key to string
                }
                else if ( var->getKeyType() == LUA_TNUMBER && newName.toInt() )
                    nameType = LUA_TNUMBER;
                else
                    nameType = LUA_TSTRING;
                var->setNewName( newName, nameType );
                change = change|0x1;
            }
            if ( newValue != var->getValue() || valueType != var->getValueType() )
            {
                //lets check again
                if ( valueType == LUA_TTABLE )
                    newValue = "{}";
                else if ( valueType == LUA_TNUMBER && newValue.toInt() )
                    valueType = LUA_TNUMBER;
                else if ( valueType == LUA_TBOOLEAN && ( newValue.toLower() == "true" || newValue.toLower() == "false" ) )
                    valueType = LUA_TBOOLEAN;
                else
                    valueType = LUA_TSTRING;//nope, you don't agree, you lose your value
                var->setValue( newValue, valueType );
                change = change|0x2;
            }
            if ( change )
            {
                if ( change&0x1 || newVar )
                    lI->renameVar( var );
                if ( change&0x2 || newVar )
                    lI->setValue( var );
                pItem->setText( 0, newName );
                mpCurrentVarItem = 0;
            }
        }
    }
    //redo this here in case we changed type
    pItem->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemIsDropEnabled|Qt::ItemIsDragEnabled|Qt::ItemIsTristate|Qt::ItemIsUserCheckable);
    pItem->setToolTip(0, "Checked variables will be saved and loaded with your profile.");
    if ( ! vu->shouldSave( var ) )
    {
        pItem->setFlags(pItem->flags() & ~(Qt::ItemIsDropEnabled|Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable));
        pItem->setForeground(0, QBrush(QColor("grey")));
        pItem->setToolTip(0, "");
        pItem->setCheckState(0, Qt::Unchecked);
    }
    else if ( vu->isSaved( var ) )
    {
        pItem->setCheckState(0, Qt::Checked);
    }
    pItem->setData( 0, Qt::UserRole, var->getValueType() );
    QIcon icon;
    switch (var->getValueType())
    {
        case 5:
            icon.addPixmap( QPixmap( QStringLiteral( ":/icons/table.png" ) ), QIcon::Normal, QIcon::Off );
            break;
        case 6:
            icon.addPixmap( QPixmap( QStringLiteral( ":/icons/function.png" ) ), QIcon::Normal, QIcon::Off );
            break;
        default:
            icon.addPixmap( QPixmap( QStringLiteral( ":/icons/variable.png" ) ), QIcon::Normal, QIcon::Off );
            break;
    }
    pItem->setIcon( 0, icon );
    mChangingVar = false;
    slot_var_selected(pItem);
}

void dlgTriggerEditor::saveKey()
{
    QTreeWidgetItem * pItem = mpCurrentKeyItem;
    if( ! pItem ) return;

    QString name = mpKeysMainArea->lineEdit_name->text();
    if( name.size() < 1 )
    {
        name = mpKeysMainArea->lineEdit_key->text();
    }
    QString command = mpKeysMainArea->lineEdit_command->text();
    QString script = mpSourceEditor->toPlainText();


        int triggerID = pItem->data(0, Qt::UserRole).toInt();
        TKey * pT = mpHost->getKeyUnit()->getKey( triggerID );
        if( pT )
        {
            pItem->setText(0,name );
            pT->setName( name );
            pT->setCommand( command );
            pT->setScript( script );

            QIcon icon;
            if( pT->isFolder() )
            {
                if( ! pT->mPackageName.isEmpty() )
                {
                    if( pT->isActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-brown.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-brown-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
                else if( pT->isActive() )
                {
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-pink.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-grey.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
                else
                {
                    if (pT->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-pink-locked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-grey-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
            }
            else
            {
                if( pT->isActive() )
                {
                    if (pT->ancestorsActive()) {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked.png" ) ), QIcon::Normal, QIcon::Off );
                    } else {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked_grey.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
                else
                {
                    if (pT->ancestorsActive()) {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox.png" ) ), QIcon::Normal, QIcon::Off );
                    } else {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox-grey.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
            }

            if( pT->state() )
            {
                pItem->setIcon( 0, icon);
                pItem->setText( 0, name );

            }
            else
            {
                QIcon iconError;
                iconError.addPixmap( QPixmap( QStringLiteral( ":/icons/tools-report-bug.png" ) ), QIcon::Normal, QIcon::Off );
                pItem->setIcon( 0, iconError );
                pItem->setText( 0, name );
            }
        }
}

void dlgTriggerEditor::slot_set_pattern_type_color( int type )
{
    QComboBox * pBox = (QComboBox *) sender();
    if( ! pBox ) return;
    int row = pBox->itemData( 0 ).toInt();
    if( row < 0 || row >= 50 ) return;
    dlgTriggerPatternEdit * pItem = mTriggerPatternEdit[row];
    QPalette palette;
    switch( type )
    {
    case 0:
        palette.setColor( QPalette::Text, QColor(Qt::black) );
        pItem->lineEdit->show();
        pItem->fgB->hide();
        pItem->bgB->hide();
        break;
    case 1:
        palette.setColor( QPalette::Text, QColor(Qt::blue) );
        pItem->lineEdit->show();
        pItem->fgB->hide();
        pItem->bgB->hide();
        break;
    case 2:
        palette.setColor( QPalette::Text, QColor(195,0,0) );
        pItem->lineEdit->show();
        pItem->fgB->hide();
        pItem->bgB->hide();
        break;
    case 3:
        palette.setColor( QPalette::Text, QColor(0,195,0) );
        pItem->lineEdit->show();
        pItem->fgB->hide();
        pItem->bgB->hide();
        break;
    case 4:
        palette.setColor( QPalette::Text, QColor(0,155,155) );
        pItem->lineEdit->show();
        pItem->fgB->hide();
        pItem->bgB->hide();
        break;
    case 5:
        palette.setColor( QPalette::Text, QColor(137,0,205) );
        pItem->lineEdit->show();
        pItem->fgB->hide();
        pItem->bgB->hide();
        break;
    case 6:
        palette.setColor( QPalette::Text, QColor(100,100,100) );
        pItem->lineEdit->hide();
        pItem->fgB->show();
        pItem->bgB->show();
        break;
    }
    pItem->lineEdit->setPalette( palette );
}

void dlgTriggerEditor::slot_trigger_selected(QTreeWidgetItem *pItem)
{
    if( ! pItem ) return;

    // save the current trigger before switching to the new one
    if ( pItem != mpCurrentTriggerItem )
        saveTrigger();

    mpCurrentTriggerItem = pItem;
    mpTriggersMainArea->show();
    mpSourceEditorArea->show();
    mpSystemMessageArea->hide();
    mpTriggersMainArea->lineEdit_trigger_name->setText("");
    mpSourceEditor->setPlainText( "" );
    mpTriggersMainArea->checkBox_multlinetrigger->setChecked( false );
    mpTriggersMainArea->perlSlashGOption->setChecked( false );
    mpTriggersMainArea->filterTrigger->setChecked( false );
    mpTriggersMainArea->spinBox_linemargin->setValue( 1 );

    int ID = pItem->data( 0, Qt::UserRole ).toInt();
    TTrigger * pT = mpHost->getTriggerUnit()->getTrigger( ID );
    if( pT )
    {
        QStringList patternList = pT->getRegexCodeList();
        QList<int> propertyList = pT->getRegexCodePropertyList();

        if( patternList.size() != propertyList.size() ) return;

        for( int i=0; i<patternList.size(); i++ )
        {
            if( i >= 50 ) break; //pattern liste ist momentan auf 50 begrenzt
            if( i >= pT->mColorPatternList.size() )
            {
                break;
            }
            dlgTriggerPatternEdit * pItem = mTriggerPatternEdit[i];
            QComboBox * pBox = pItem->patternType;
            QPalette palette;
            switch( propertyList[i] )
            {
            case REGEX_SUBSTRING:
                palette.setColor( QPalette::Text, QColor(Qt::black) );
                pBox->setCurrentIndex( 0 );
                pItem->fgB->hide();
                pItem->bgB->hide();
                pItem->lineEdit->show();
                break;
            case REGEX_PERL:
                palette.setColor( QPalette::Text, QColor(Qt::blue) );
                pBox->setCurrentIndex( 1 );
                pItem->fgB->hide();
                pItem->bgB->hide();
                pItem->lineEdit->show();
                break;
            case REGEX_BEGIN_OF_LINE_SUBSTRING:
                palette.setColor( QPalette::Text, QColor(195,0,0) );
                pBox->setCurrentIndex( 2 );
                pItem->fgB->hide();
                pItem->bgB->hide();
                pItem->lineEdit->show();
                break;
            case REGEX_EXACT_MATCH:
                palette.setColor( QPalette::Text, QColor(0,195,0) );
                pBox->setCurrentIndex( 3 );
                pItem->fgB->hide();
                pItem->bgB->hide();
                pItem->lineEdit->show();
                break;
            case REGEX_LUA_CODE:
                palette.setColor( QPalette::Text, QColor(0,155,155) );
                pBox->setCurrentIndex( 4 );
                pItem->fgB->hide();
                pItem->bgB->hide();
                pItem->lineEdit->show();
                break;
            case REGEX_LINE_SPACER:
                palette.setColor( QPalette::Text, QColor(137,0,205) );
                pBox->setCurrentIndex( 5 );
                pItem->fgB->hide();
                pItem->bgB->hide();
                pItem->lineEdit->show();
                break;
            case REGEX_COLOR_PATTERN:
                palette.setColor( QPalette::Text, QColor(100,100,100) );
                pBox->setCurrentIndex( 6 );
                pItem->fgB->show();
                pItem->bgB->show();
                pItem->lineEdit->hide();
                QPalette fgPal;
                QPalette bgPal;
                if( ! pT->mColorPatternList[i] ) break;
                auto fgC = QColor(pT->mColorPatternList[i]->fgR,
                                    pT->mColorPatternList[i]->fgG,
                                    pT->mColorPatternList[i]->fgB);
                fgPal.setColor( QPalette::Button, fgC );
                QString fgstyleSheet = QString("QPushButton{background-color:")+fgC.name()+QString(";}");
                auto bgC = QColor(pT->mColorPatternList[i]->bgR,
                                    pT->mColorPatternList[i]->bgG,
                                    pT->mColorPatternList[i]->bgB);
                bgPal.setColor( QPalette::Button, bgC );
                QString bgstyleSheet = QString("QPushButton{background-color:")+bgC.name()+QString(";}");
                pItem->fgB->setStyleSheet( fgstyleSheet );
                pItem->bgB->setStyleSheet( bgstyleSheet );
                break;
            }

            pItem->lineEdit->setPalette( palette );
            pItem->lineEdit->setText( patternList[i] );
        }
        for( int i=patternList.size(); i<50; i++)
        {
            mTriggerPatternEdit[i]->lineEdit->clear();
            if( mTriggerPatternEdit[i]->lineEdit->isHidden() )
            {
                mTriggerPatternEdit[i]->lineEdit->show();
            }
            mTriggerPatternEdit[i]->fgB->hide();
            mTriggerPatternEdit[i]->bgB->hide();
            mTriggerPatternEdit[i]->patternType->setCurrentIndex(0);
        }
        QString command = pT->getCommand();
        mpTriggersMainArea->lineEdit_trigger_name->setText(pItem->text(0));
        mpTriggersMainArea->trigger_command->setText( command );
        mpTriggersMainArea->checkBox_multlinetrigger->setChecked( pT->isMultiline() );
        mpTriggersMainArea->perlSlashGOption->setChecked( pT->mPerlSlashGOption );
        mpTriggersMainArea->filterTrigger->setChecked( pT->mFilterTrigger );
        mpTriggersMainArea->spinBox_linemargin->setValue( pT->getConditionLineDelta() );
        mpTriggersMainArea->spinBox_stayOpen->setValue( pT->mStayOpen );
        mpTriggersMainArea->soundTrigger->setChecked( pT->mSoundTrigger );
        mpTriggersMainArea->lineEdit_soundFile->setText( pT->mSoundFile );
        QPalette pal = palette();
        QColor color =  pal.color( QPalette::Button );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");

        QColor fgColor = pT->getFgColor();
        QColor bgColor = pT->getBgColor();
        QPalette FgColorPalette;
        QPalette BgColorPalette;
        FgColorPalette.setColor( QPalette::Button, fgColor );
        BgColorPalette.setColor( QPalette::Button, bgColor );
        QString FgColorStyleSheet = QString("QPushButton{background-color:")+fgColor.name()+QString(";}");
        QString BgColorStyleSheet = QString("QPushButton{background-color:")+bgColor.name()+QString(";}");
        mpTriggersMainArea->pushButtonFgColor->setStyleSheet( FgColorStyleSheet );
        mpTriggersMainArea->pushButtonBgColor->setStyleSheet( BgColorStyleSheet );
        mpTriggersMainArea->pushButtonFgColor->setPalette( FgColorPalette );
        mpTriggersMainArea->pushButtonBgColor->setPalette( BgColorPalette );
        mpTriggersMainArea->colorizerTrigger->setChecked( pT->isColorizerTrigger() );
        QString script = pT->getScript();
        mpSourceEditor->setPlainText( script );

        if( ! pT->state() ) showError( pT->getError() );
    }
}

void dlgTriggerEditor::slot_alias_selected(QTreeWidgetItem *pItem)
{
    if( ! pItem ) return;

    // save the current alias before switching to the new one
    if ( pItem != mpCurrentAliasItem )
        saveAlias();

    mpCurrentAliasItem = pItem;
    mpAliasMainArea->show();
    mpSourceEditorArea->show();
    mpSystemMessageArea->hide();
    mpAliasMainArea->lineEdit_alias_name->clear();
    mpAliasMainArea->pattern_textedit->clear();
    mpAliasMainArea->substitution->clear();
    mpSourceEditor->setPlainText( "" );

    mpAliasMainArea->lineEdit_alias_name->setText(pItem->text(0));
    int ID = pItem->data(0,Qt::UserRole).toInt();
    TAlias * pT = mpHost->getAliasUnit()->getAlias(ID);
    if( pT )
    {
        QString pattern = pT->getRegexCode();
        QString command = pT->getCommand();
        QString name = pT->getName();

        mpAliasMainArea->pattern_textedit->clear();
        mpAliasMainArea->substitution->clear();
        mpAliasMainArea->lineEdit_alias_name->clear();

        mpAliasMainArea->pattern_textedit->setText( pattern );
        mpAliasMainArea->substitution->setText( command );
        mpAliasMainArea->lineEdit_alias_name->setText( name );

        QString script = pT->getScript();
        mpSourceEditor->setPlainText( script );
        if( ! pT->state() ) showError( pT->getError() );
    }
}

void dlgTriggerEditor::slot_key_selected(QTreeWidgetItem *pItem)
{
    if( ! pItem ) return;

    // save the current key before switching to the new one
    if ( pItem != mpCurrentKeyItem )
        saveKey();

    mpCurrentKeyItem = pItem;
    mpKeysMainArea->show();
    mpSourceEditorArea->show();
    mpSystemMessageArea->hide();
    mpKeysMainArea->lineEdit_command->clear();
    mpKeysMainArea->lineEdit_key->clear();
    mpKeysMainArea->lineEdit_name->clear();
    mpSourceEditor->setPlainText( "" );

    mpKeysMainArea->lineEdit_key->setText( pItem->text(0) );
    int ID = pItem->data( 0, Qt::UserRole ).toInt();
    TKey * pT = mpHost->getKeyUnit()->getKey(ID);
    if( pT )
    {
        QString command = pT->getCommand();
        QString name = pT->getName();
        mpKeysMainArea->lineEdit_command->clear();
        mpKeysMainArea->lineEdit_command->setText( command );
        mpKeysMainArea->lineEdit_name->setText( name );
        QString keyName = mpHost->getKeyUnit()->getKeyName( pT->getKeyCode(), pT->getKeyModifiers() );
        mpKeysMainArea->lineEdit_key->setText( keyName );
        QString script = pT->getScript();
        mpSourceEditor->setPlainText( script );
        if( ! pT->state() ) showError( pT->getError() );
    }
}

// This should not modify the contents of what pItem points at:
void dlgTriggerEditor::recurseVariablesUp( QTreeWidgetItem * const pItem, QList< QTreeWidgetItem * > & list)
{
    QTreeWidgetItem * pParent = pItem->parent();
    if ( pParent && pParent != mpVarBaseItem )
    {
        list.append( pParent );
        recurseVariablesUp( pParent, list );
    }
}

// This should not modify the contents of what pItem points at:
void dlgTriggerEditor::recurseVariablesDown( QTreeWidgetItem * const pItem, QList< QTreeWidgetItem * > & list)
{
    list.append( pItem );
    for(int i=0;i<pItem->childCount();++i)
        recurseVariablesDown( pItem->child(i), list );
}

void dlgTriggerEditor::recurseVariablesDown( TVar *var, QList< TVar * > & list, bool isSorted)
{
    list.append( var );
    QListIterator<TVar *> it(var->getChildren(isSorted));
    while (it.hasNext()) {
        recurseVariablesDown( it.next(), list, isSorted );
    }
}

void dlgTriggerEditor::slot_var_changed(QTreeWidgetItem *pItem){
    // This handles a small case where the radio buttom is clicked while the item is currently selected
    // which causes the variable to not save. In places where we populate the TreeWidgetItem, we have
    // to guard it with mChangingVar or else this will be called with every change such as the variable
    // name, etc.
    if( !pItem || mChangingVar )
        return;
    int column = 0;
    int state = pItem->checkState( column );
    LuaInterface * lI = mpHost->getLuaInterface();
    VarUnit * vu = lI->getVarUnit();
    TVar * var = vu->getWVar(pItem);
    if( !var )
        return;
    if ( state == Qt::Checked || state == Qt::PartiallyChecked )
    {
        if ( vu->isSaved( var ) )
            return;
        vu->addSavedVar( var );
        QList< QTreeWidgetItem * > list;
        recurseVariablesUp( pItem, list );
        for(auto & treeWidgetItem : list)
        {
            TVar * v = vu->getWVar( treeWidgetItem );
            if ( v && ( treeWidgetItem->checkState( column ) == Qt::Checked ||
                        treeWidgetItem->checkState( column ) == Qt::PartiallyChecked ) )
                vu->addSavedVar( v );
        }
        list.clear();
        recurseVariablesDown( pItem, list );
        for(auto & treeWidgetItem : list)
        {
            TVar * v = vu->getWVar( treeWidgetItem );
            if ( v && ( treeWidgetItem->checkState( column ) == Qt::Checked ||
                        treeWidgetItem->checkState( column ) == Qt::PartiallyChecked ) )
                vu->addSavedVar( v );
        }
    }
    else{
        // we're not checked, dont save us
        if( ! vu->isSaved( var ) )
            return;
        vu->removeSavedVar(var);
        QList< QTreeWidgetItem * > list;
        recurseVariablesUp( pItem, list );
        for(auto & treeWidgetItem : list)
        {
            TVar * v = vu->getWVar( treeWidgetItem );
            if ( v && ( treeWidgetItem->checkState( column ) == Qt::Checked ||
                        treeWidgetItem->checkState( column ) == Qt::PartiallyChecked ) )
                vu->removeSavedVar( v );
        }
        list.clear();
        recurseVariablesDown( pItem, list );
        for(auto & treeWidgetItem : list)
        {
            TVar * v = vu->getWVar( treeWidgetItem );
            if ( v && ( treeWidgetItem->checkState( column ) == Qt::Checked ||
                        treeWidgetItem->checkState( column ) == Qt::PartiallyChecked ) )
                vu->removeSavedVar( v );
        }
    }
}

void dlgTriggerEditor::slot_var_selected(QTreeWidgetItem *pItem)
{
    if( ! pItem ) return;

    // save the current variable before switching to the new one
    if ( pItem != mpCurrentVarItem )
        saveVar();

    mChangingVar = true;
    int column = treeWidget_variables->currentColumn();
    int state = pItem->checkState( column );
    if ( state == Qt::Checked || state == Qt::PartiallyChecked )
    {
        LuaInterface * lI = mpHost->getLuaInterface();
        VarUnit * vu = lI->getVarUnit();
        TVar * var = vu->getWVar(pItem); // This does NOT modify pItem or what it points at
        if ( var )
            vu->addSavedVar( var );
        QList< QTreeWidgetItem * > list;
        recurseVariablesUp( pItem, list ); // This does NOT modify pItem or what it points at
        for(auto & treeWidgetItem : list)
        {
            TVar * v = vu->getWVar( treeWidgetItem );
            if ( v && ( treeWidgetItem->checkState( column ) == Qt::Checked ||
                        treeWidgetItem->checkState( column ) == Qt::PartiallyChecked ) )
                vu->addSavedVar( v );
        }
        list.clear();
        recurseVariablesDown( pItem, list ); // This does NOT modify pItem or what it points at
        for(auto & treeWidgetItem : list)
        {
            TVar * v = vu->getWVar( treeWidgetItem );
            if ( v && ( treeWidgetItem->checkState( column ) == Qt::Checked ||
                        treeWidgetItem->checkState( column ) == Qt::PartiallyChecked ) )
                vu->addSavedVar( v );
        }
    }
    else if ( state == Qt::Unchecked )
    {
        LuaInterface * lI = mpHost->getLuaInterface();
        VarUnit * vu = lI->getVarUnit();
        TVar * var = vu->getWVar(pItem); // This does NOT modify pItem or what it points at
        if ( var )
            vu->removeSavedVar( var );
        QList< QTreeWidgetItem * > list;
        recurseVariablesUp( pItem, list ); // This does NOT modify pItem or what it points at
        for(auto & treeWidgetItem : list)
        {
            TVar * v = vu->getWVar( treeWidgetItem );
            if ( v && ( treeWidgetItem->checkState( column ) == Qt::Unchecked ) )
            {
                vu->removeSavedVar( v );
            }
        }
        list.clear();
        recurseVariablesDown( pItem, list ); // This does NOT modify pItem or what it points at
        for(auto & treeWidgetItem : list)
        {
            TVar * v = vu->getWVar( treeWidgetItem );
            if ( v && ( treeWidgetItem->checkState( column ) == Qt::Unchecked ) )
            {
                vu->removeSavedVar( v );
            }
        }
    }
    mpVarsMainArea->show();

    mpCurrentVarItem = pItem; //remember what has been clicked to save it
    // There was repeated test for pItem being null here but we have NOT altered
    // it since the start of the function where it was already tested for not
    // being zero so we don't need to retest it! - Slysven
    if( column )
    {
        mChangingVar = false;
        return;
    }
    LuaInterface * lI = mpHost->getLuaInterface();
    VarUnit * vu = lI->getVarUnit();
    TVar * var = vu->getWVar(pItem);
    if (!var)
    {
        mpVarsMainArea->hideVariable->setChecked( false );
        mpVarsMainArea->lineEdit_var_name->setText("");
        mpSourceEditor->setPlainText("");
        //check for temp item
        var = vu->getTVar( pItem );
        if ( var && var->getValueType() == LUA_TTABLE )
        {
            mpVarsMainArea->var_type->setDisabled(true);
            mpVarsMainArea->var_type->setCurrentIndex(4);
        }
        else
        {
            mpVarsMainArea->var_type->setDisabled(false);
            mpVarsMainArea->var_type->setCurrentIndex(0);
        }
        mpVarsMainArea->key_type->setCurrentIndex(0);
        mChangingVar = false;
        return;
    }
    int varType = var->getValueType();
    int keyType = var->getKeyType();
    QIcon icon;
    mpVarsMainArea->key_type->setEnabled(true);
    mpSourceEditor->setReadOnly(false);
    mpVarsMainArea->var_type->setEnabled(true);
    if (keyType == 4)
        mpVarsMainArea->key_type->setCurrentIndex(1);
    else if (keyType == 3)
        mpVarsMainArea->key_type->setCurrentIndex(2);
    else if (keyType == 5)
    {
        mpVarsMainArea->key_type->setCurrentIndex(3);
        mpVarsMainArea->key_type->setDisabled(true);
    }
    else if ( keyType == 6 )
    {
        mpVarsMainArea->key_type->setCurrentIndex(4);
        mpVarsMainArea->key_type->setDisabled(true);
    }
    if (varType == LUA_TTABLE || varType == LUA_TFUNCTION)
    {
        mpSourceEditor->setReadOnly(true);
        if ( varType == LUA_TTABLE )
        {
            if ( pItem->childCount() )
                mpVarsMainArea->var_type->setDisabled(true);
            mpVarsMainArea->var_type->setCurrentIndex(4);
            icon.addPixmap( QPixmap( QStringLiteral( ":/icons/table.png" ) ), QIcon::Normal, QIcon::Off );
        }
        else
        {
            mpVarsMainArea->var_type->setCurrentIndex(5);
            mpVarsMainArea->var_type->setDisabled(true);
            icon.addPixmap( QPixmap( QStringLiteral( ":/icons/function.png" ) ), QIcon::Normal, QIcon::Off );
        }
    }
    else
    {
        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/variable.png" ) ), QIcon::Normal, QIcon::Off );
        if ( varType == LUA_TSTRING )
            mpVarsMainArea->var_type->setCurrentIndex(1);
        else if ( varType == LUA_TNUMBER )
            mpVarsMainArea->var_type->setCurrentIndex(2);
        else if ( varType == LUA_TBOOLEAN )
            mpVarsMainArea->var_type->setCurrentIndex(3);
    }
    mpVarsMainArea->hideVariable->setChecked( vu->isHidden( var ) );
    mpVarsMainArea->lineEdit_var_name->setText(var->getName());
    mpSourceEditor->setPlainText(lI->getValue( var ));
    pItem->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemIsDropEnabled|Qt::ItemIsDragEnabled|Qt::ItemIsTristate|Qt::ItemIsUserCheckable);
    pItem->setToolTip(0, "Checked variables will be saved and loaded with your profile.");
    pItem->setCheckState(0, Qt::Unchecked);
    if ( ! vu->shouldSave( var ) )
    {
        pItem->setFlags(pItem->flags() & ~(Qt::ItemIsDropEnabled|Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable));
        pItem->setForeground(0, QBrush(QColor("grey")));
        pItem->setToolTip(0, "");
    }
    else if ( vu->isSaved( var ) )
        pItem->setCheckState(0, Qt::Checked);
    pItem->setData( 0, Qt::UserRole, var->getValueType() );
    pItem->setIcon( 0, icon );
    mChangingVar = false;
}

void dlgTriggerEditor::slot_action_selected(QTreeWidgetItem *pItem)
{
    if( ! pItem ) return;

    // save the current action before switching to the new one
    if ( pItem != mpCurrentActionItem )
        saveAction();

    mpActionsMainArea->show();
    mpSourceEditor->show();

    mpSystemMessageArea->hide();
    mpSourceEditor->clear();

    mpActionsMainArea->lineEdit_action_icon->clear();
    mpActionsMainArea->lineEdit_action_name->clear();
    mpActionsMainArea->checkBox_pushdownbutton->setChecked( false );
    mpActionsMainArea->lineEdit_command_down->clear();
    mpActionsMainArea->lineEdit_command_up->clear();
    mpActionsMainArea->buttonColumns->clear();
    mpActionsMainArea->css->clear();
    mpActionsMainArea->comboBox_location->setCurrentIndex( 0 );
    mpActionsMainArea->comboBox_orientation->setCurrentIndex( 0 );
    mpActionsMainArea->buttonRotation->setCurrentIndex( 0 );
    mpActionsMainArea->buttonColumns->setValue( 1 );

    mpCurrentActionItem = pItem; //remember what has been clicked to save it
    // ID will be 0 for the root of the treewidget and it is not appropriate
    // to show any right hand side details - pT will also be Q_NULLPTR!
    int ID = pItem->data(0,Qt::UserRole).toInt();
    TAction * pT = mpHost->getActionUnit()->getAction(ID);
    if( pT )
    {
        mpActionsMainArea->lineEdit_action_name->setText( pT->getName() );
        mpActionsMainArea->checkBox_pushdownbutton->setChecked( pT->isPushDownButton() );
        mpActionsMainArea->label_command_up->hide();
        mpActionsMainArea->label_command_down->hide();
        mpActionsMainArea->lineEdit_command_up->hide();
        mpActionsMainArea->lineEdit_command_down->hide();
        mpActionsMainArea->label_command_down->setText( tr( "Command:" ) );
        mpActionsMainArea->lineEdit_action_icon->setText( pT->getIcon() );
        mpActionsMainArea->lineEdit_command_down->setText( pT->getCommandButtonDown() );
        mpActionsMainArea->lineEdit_command_up->setText( pT->getCommandButtonUp() );
        mpSourceEditor->setPlainText( pT->getScript() );
        // location = 1 = location = bottom is no longer supported
        int location = pT->mLocation;
        if( location > 0 ) location--;
        mpActionsMainArea->comboBox_location->setCurrentIndex( location );
        mpActionsMainArea->comboBox_orientation->setCurrentIndex( pT->mOrientation );
        mpActionsMainArea->buttonRotation->setCurrentIndex( pT->getButtonRotation() );
        mpActionsMainArea->buttonColumns->setValue( pT->getButtonColumns() );
        mpActionsMainArea->css->setPlainText( pT->css );
        if( pT->isFolder() )
        {
            if( ! pT->getPackageName().isEmpty() )
            {
                // We have a non-empty package name (Tree<T>::mModuleName
                // is NEVER used but Tree<T>::mPackageName is for both!)
                // THUS: We are a module master folder

                mpActionsMainArea->groupBox_toolBar->hide();
                mpActionsMainArea->groupBox_appearance->hide();
                mpActionsMainArea->widget_2->hide();
                mpSourceEditorArea->hide();
            }
            else if( ! pT->getParent()
                  || ( pT->getParent() && ! pT->getParent()->getPackageName().isEmpty() ) )
                // We are a top-level folder with no parent
                // OR: We have a parent and that IS a module master folder
                // THUS: We are a toolbar
            {

                mpActionsMainArea->groupBox_toolBar->show();
                mpActionsMainArea->groupBox_appearance->hide();
                mpActionsMainArea->widget_2->show();
                mpSourceEditorArea->show();
            }
            else {
                // We must be a MENU

                mpActionsMainArea->groupBox_appearance->setTitle( tr( "Menu properties" ) );
                mpActionsMainArea->groupBox_toolBar->hide();
                mpActionsMainArea->checkBox_pushdownbutton->hide();
                mpActionsMainArea->groupBox_appearance->show();
                mpActionsMainArea->widget_2->show();
                mpSourceEditorArea->show();
            }
        }
        else {
            // We are a BUTTON

            mpActionsMainArea->groupBox_appearance->setTitle( tr( "Button properties" ) );
            mpActionsMainArea->groupBox_toolBar->hide();
            mpActionsMainArea->groupBox_appearance->show();
            mpActionsMainArea->label_command_down->show();
            mpActionsMainArea->lineEdit_command_down->show();
            mpActionsMainArea->checkBox_pushdownbutton->show();
            mpSourceEditorArea->show();
            if( pT->isPushDownButton() )
            {
                mpActionsMainArea->label_command_down->setText( tr( "Command (down);" ) );
                mpActionsMainArea->lineEdit_command_up->show();
                mpActionsMainArea->label_command_up->show();
            }

            mpActionsMainArea->widget_2->show();
        }

        if( ! pT->state() ) showError( pT->getError() );
    }
    else
    {
        // On root of treewidget_actions: - show help message instead
        mpActionsMainArea->hide();
        mpSourceEditor->hide();
        showInfo( msgInfoAddButton );
    }
}

void dlgTriggerEditor::slot_tree_selection_changed()
{
    TTreeWidget * sender = qobject_cast<TTreeWidget *>(QObject::sender());
    if(sender){
        QList<QTreeWidgetItem *> items = sender->selectedItems();
        if(items.length()){
            QTreeWidgetItem * item = items.first();
            if(sender == treeWidget_scripts)
                slot_scripts_selected(item);
            else if(sender == treeWidget_keys)
                slot_key_selected(item);
            else if(sender == treeWidget_timers)
                slot_timer_selected(item);
            else if(sender == treeWidget_aliases)
                slot_alias_selected(item);
            else if(sender == treeWidget_actions)
                slot_action_selected(item);
            else if(sender == treeWidget_variables)
                slot_var_selected(item);
            else if(sender == treeWidget_triggers)
                slot_trigger_selected(item);
        }
    }
}


void dlgTriggerEditor::slot_scripts_selected(QTreeWidgetItem *pItem)
{
    if( ! pItem ) return;

    // save the current script before switching to the new one
    if ( pItem != mpCurrentScriptItem )
        saveScript();

    mpCurrentScriptItem = pItem;
    mpScriptsMainArea->show();
    mpSourceEditorArea->show();
    mpSystemMessageArea->hide();
    mpSourceEditor->setPlainText( "" );
    mpScriptsMainArea->lineEdit_scripts_name->clear();
    mpScriptsMainArea->listWidget_registered_event_handlers->clear();

    mpScriptsMainArea->listWidget_registered_event_handlers->clear();
    mpScriptsMainArea->lineEdit_scripts_name->setText(pItem->text(0));
    int ID = pItem->data(0,Qt::UserRole).toInt();
    TScript * pT = mpHost->getScriptUnit()->getScript(ID);
    if( pT )
    {
        QString name = pT->getName();
        QStringList eventHandlerList = pT->getEventHandlerList();
        for( int i=0; i<eventHandlerList.size(); i++ )
        {
            auto pItem = new QListWidgetItem( mpScriptsMainArea->listWidget_registered_event_handlers );
            pItem->setText( eventHandlerList[i] );
            mpScriptsMainArea->listWidget_registered_event_handlers->addItem( pItem );
        }
        mpScriptsMainArea->lineEdit_scripts_name->clear();
        QString script = pT->getScript();
        mpSourceEditor->setPlainText( script );
        mpScriptsMainArea->lineEdit_scripts_name->setText( name );
        if( ! pT->state() ) showError( pT->getError() );
    }
}

void dlgTriggerEditor::slot_timer_selected(QTreeWidgetItem *pItem)
{
    if( ! pItem ) return;

    // save the current timer before switching to the new one
    if ( pItem != mpCurrentTimerItem )
        saveTimer();

    mpCurrentTimerItem = pItem;
    mpTimersMainArea->show();
    mpSourceEditorArea->show();
    mpSystemMessageArea->hide();
    mpSourceEditor->setPlainText( "" );
    mpTimersMainArea->lineEdit_command->clear();
    mpTimersMainArea->lineEdit_timer_name->clear();
    mpTimersMainArea->timeEdit_hours->clear();
    mpTimersMainArea->timeEdit_minutes->clear();
    mpTimersMainArea->timeEdit_seconds->clear();
    mpTimersMainArea->timeEdit_msecs->clear();

    mpTimersMainArea->lineEdit_timer_name->setText(pItem->text(0));
    int ID = pItem->data( 0, Qt::UserRole ).toInt();
    TTimer * pT = mpHost->getTimerUnit()->getTimer( ID );
    if( pT )
    {
        if( pT->getParent() )
            qDebug()<<"[STATUS]: timer ID="<<pT->getID()<<" name="<<pT->getName()<<" mActive = "<<pT->isActive()<<" mUserActiveState="<<pT->shouldBeActive()<<" parent="<<pT->getParent()->getName();
        else
            qDebug()<<"[STATUS]: timer ID="<<pT->getID()<<" name="<<pT->getName()<<"> mActive = "<<pT->isActive()<<" mUserActiveState="<<pT->shouldBeActive()<<" parent=0";
        QString command = pT->getCommand();
        QString name = pT->getName();
        mpTimersMainArea->lineEdit_command->setText( command );
        mpTimersMainArea->lineEdit_timer_name->setText( name );
        QTime time = pT->getTime();
        int hours = time.hour();
        int minutes = time.minute();
        int secs = time.second();
        int msecs = time.msec();

        QTime t2(hours,0,0,0);
        mpTimersMainArea->timeEdit_hours->setTime(t2);

        QTime t3(0,minutes,0,0);
        mpTimersMainArea->timeEdit_minutes->setTime(t3);

        QTime t4(0,0,secs);
        mpTimersMainArea->timeEdit_seconds->setTime(t4);

        QTime t5(0,0,0,msecs);
        mpTimersMainArea->timeEdit_msecs->setTime(t5);

        QString script = pT->getScript();
        mpSourceEditor->setPlainText( script );
        if( ! pT->state() ) showError( pT->getError() );
    }
}


void dlgTriggerEditor::fillout_form()
{
    mCurrentView = 0;
    mpCurrentTriggerItem = 0;
    mpCurrentAliasItem = 0;
    mpCurrentKeyItem = 0;
    mpCurrentActionItem = 0;
    mpCurrentScriptItem = 0;
    mpCurrentTimerItem = 0;
    mpCurrentVarItem = 0;

    mNeedUpdateData = false;
    QStringList sL;
    sL << "Triggers";
    mpTriggerBaseItem = new QTreeWidgetItem( static_cast<QTreeWidgetItem *>(0), sL );
    mpTriggerBaseItem->setBackground(0,QColor(255,254,215,255));
    QIcon mainIcon;
    mainIcon.addPixmap( QPixmap( QStringLiteral( ":/icons/tools-wizard.png" ) ), QIcon::Normal, QIcon::Off );
    mpTriggerBaseItem->setIcon( 0, mainIcon );
    treeWidget_triggers->insertTopLevelItem( 0, mpTriggerBaseItem );
    list<TTrigger *> baseNodeList = mpHost->getTriggerUnit()->getTriggerRootNodeList();
    for(auto trigger : baseNodeList)
    {
        if( trigger->isTempTrigger() ) continue;
        QString s = trigger->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem( mpTriggerBaseItem, sList);
        pItem->setData( 0, Qt::UserRole, QVariant(trigger->getID()) );
        mpTriggerBaseItem->addChild( pItem );
        QIcon icon;
        if( trigger->hasChildren() )
        {
            expand_child_triggers( trigger, pItem );
        }
        if( trigger->state() )
        {
            if( trigger->isFilterChain() )
            {
                if( trigger->isActive() )
                {
                    if( trigger->ancestorsActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/filter.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/filter-grey.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
                else
                {
                    if( trigger->ancestorsActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/filter-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/filter-grey-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
            }
            else if( trigger->isFolder() )
            {
                if( ! trigger->mPackageName.isEmpty() )
                {
                    if( trigger->isActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-brown.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-brown-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
                else if( trigger->isActive() )
                {
                    if( trigger->ancestorsActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-blue.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-grey.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
                else
                {
                    if( trigger->ancestorsActive() )
                    {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-blue-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            else
            {
                if( trigger->isActive() )
                {
                    if( trigger->ancestorsActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked_grey.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
                else
                {
                    if( trigger->ancestorsActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox-grey.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
            }
            pItem->setIcon(0, icon);
        }
        else
        {
            QIcon iconError;
            iconError.addPixmap( QPixmap( QStringLiteral( ":/icons/tools-report-bug.png" ) ), QIcon::Normal, QIcon::Off );
            pItem->setIcon( 0, iconError );
            showError( trigger->getError() );
        }
    }

    mpTriggerBaseItem->setExpanded( true );

    QStringList sL2;
    sL2 << "Timers";
    mpTimerBaseItem = new QTreeWidgetItem( static_cast<QTreeWidgetItem *>(0), sL2 );
    mpTimerBaseItem->setBackground(0,QColor(255,254,215,255));
    QIcon mainIcon2;
    mainIcon2.addPixmap( QPixmap( QStringLiteral( ":/icons/chronometer.png" ) ), QIcon::Normal, QIcon::Off );
    mpTimerBaseItem->setIcon( 0, mainIcon2 );
    treeWidget_timers->insertTopLevelItem( 0, mpTimerBaseItem );
    mpTriggerBaseItem->setExpanded( true );
    list<TTimer *> baseNodeList_timers = mpHost->getTimerUnit()->getTimerRootNodeList();
    for(auto timer : baseNodeList_timers)
    {
        if( timer->isTempTimer() ) continue;
        QString s = timer->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem( mpTimerBaseItem, sList);
        pItem->setData( 0, Qt::UserRole, QVariant(timer->getID()) );
        mpTimerBaseItem->addChild( pItem );
        QIcon icon;
        if( timer->hasChildren() )
        {
            expand_child_timers( timer, pItem );
        }
        if( timer->state() )
        {
            if( timer->isFolder() )
            {
                if( ! timer->mPackageName.isEmpty() )
                {
                    if( timer->isActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-brown.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-brown-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
                else
                {
                    if (timer->shouldBeActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-green.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-green-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            else
            {
                if( timer->isOffsetTimer() )
                {
                    if( timer->shouldBeActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/offsettimer-on.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/offsettimer-off.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
                else
                {
                    if( timer->shouldBeActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }

            }
            pItem->setIcon(0, icon);
        }
        else
        {
            QIcon iconError;
            iconError.addPixmap( QPixmap( QStringLiteral( ":/icons/tools-report-bug.png" ) ), QIcon::Normal, QIcon::Off );
            pItem->setIcon( 0, iconError );
            showError( timer->getError() );
        }
    }
    mpTimerBaseItem->setExpanded( true );

    QStringList sL3;
    sL3 << "Scripts";
    mpScriptsBaseItem = new QTreeWidgetItem( static_cast<QTreeWidgetItem *>(0), sL3 );
    mpScriptsBaseItem->setBackground(0,QColor(255,254,215,255));
    QIcon mainIcon3;
    mainIcon3.addPixmap( QPixmap( QStringLiteral( ":/icons/accessories-text-editor.png" ) ), QIcon::Normal, QIcon::Off );
    mpScriptsBaseItem->setIcon( 0, mainIcon3 );
    treeWidget_scripts->insertTopLevelItem( 0, mpScriptsBaseItem );
    mpScriptsBaseItem->setExpanded( true );
    list<TScript *> baseNodeList_scripts = mpHost->getScriptUnit()->getScriptRootNodeList();
    for(auto script : baseNodeList_scripts)
    {
        QString s = script->getName();

        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem( mpScriptsBaseItem, sList);
        pItem->setData( 0, Qt::UserRole, QVariant(script->getID()) );
        mpScriptsBaseItem->addChild( pItem );
        QIcon icon;
        if( script->hasChildren() )
        {
            expand_child_scripts( script, pItem );
        }
        if( script->state() )
        {
            if( script->isFolder() )
            {
                if( ! script->mPackageName.isEmpty() )
                {
                    if( script->isActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-brown.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-brown-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
                else
                {
                    if (script->isActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-orange.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-orange-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            else
            {
                if( script->isActive() )
                {
                    icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked.png" ) ), QIcon::Normal, QIcon::Off );
                }
                else
                {
                    icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox.png" ) ), QIcon::Normal, QIcon::Off );
                }
            }
            pItem->setIcon(0, icon);
        }
        else
        {
            QIcon iconError;
            iconError.addPixmap( QPixmap( QStringLiteral( ":/icons/tools-report-bug.png" ) ), QIcon::Normal, QIcon::Off );
            pItem->setIcon( 0, iconError );
            showError( script->getError() );
        }
    }
    mpScriptsBaseItem->setExpanded( true );

    QStringList sL4;
    sL4 << "Aliases - Input Triggers";
    mpAliasBaseItem = new QTreeWidgetItem( static_cast<QTreeWidgetItem *>(0), sL4 );
    mpAliasBaseItem->setBackground(0,QColor(255,254,215,255));
    QIcon mainIcon4;
    mainIcon4.addPixmap( QPixmap( QStringLiteral( ":/icons/system-users.png" ) ), QIcon::Normal, QIcon::Off );
    mpAliasBaseItem->setIcon( 0, mainIcon4 );
    treeWidget_aliases->insertTopLevelItem( 0, mpAliasBaseItem );
    mpAliasBaseItem->setExpanded( true );
    list<TAlias *> baseNodeList_alias = mpHost->getAliasUnit()->getAliasRootNodeList();
    for(auto alias : baseNodeList_alias)
    {
        QString s = alias->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem( mpAliasBaseItem, sList);
        pItem->setData( 0, Qt::UserRole, QVariant(alias->getID()) );
        mpAliasBaseItem->addChild( pItem );
        QIcon icon;
        if( alias->hasChildren() )
        {
            expand_child_alias( alias, pItem );
        }
        if( alias->state() )
        {
            if( alias->isFolder() )
            {
                if( ! alias->mPackageName.isEmpty() )
                {
                    if( alias->isActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-brown.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-brown-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
                else if( alias->isActive() )
                {
                    if( alias->ancestorsActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-violet.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-grey.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
                else
                {
                    if( alias->ancestorsActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-violet-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-grey-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
            }
            else
            {
                if( alias->isActive() )
                {
                    if( alias->ancestorsActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked_grey.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
                else
                {
                    if( alias->ancestorsActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox-grey.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
            }
            pItem->setIcon(0, icon);
        }
        else
        {
            QIcon iconError;
            iconError.addPixmap( QPixmap( QStringLiteral( ":/icons/tools-report-bug.png" ) ), QIcon::Normal, QIcon::Off );
            pItem->setIcon( 0, iconError );
            showError( alias->getError() );
        }
    }
    mpAliasBaseItem->setExpanded( true );

    QStringList sL5;
    sL5 << "Buttons";
    mpActionBaseItem = new QTreeWidgetItem( static_cast<QTreeWidgetItem *>(0), sL5 );
    mpActionBaseItem->setBackground(0,QColor(255,254,215,255));
    QIcon mainIcon5;
    mainIcon5.addPixmap( QPixmap( QStringLiteral( ":/icons/bookmarks.png" ) ), QIcon::Normal, QIcon::Off );
    mpActionBaseItem->setIcon( 0, mainIcon5 );
    treeWidget_actions->insertTopLevelItem( 0, mpActionBaseItem );
    mpActionBaseItem->setExpanded( true );
    list<TAction *> baseNodeList_action = mpHost->getActionUnit()->getActionRootNodeList();
    for(auto action : baseNodeList_action)
    {
        QString s = action->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem( mpActionBaseItem, sList);
        pItem->setData( 0, Qt::UserRole, QVariant(action->getID()) );
        mpActionBaseItem->addChild( pItem );
        QIcon icon;
        if( action->hasChildren() )
        {
            expand_child_action( action, pItem );
        }
        if( action->state() )
        {
            if( action->isFolder() )
            {
                if( ! action->mPackageName.isEmpty() )
                {
                    if( action->isActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-brown.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-brown-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
                else if( ! action->getParent()
                      || ! action->getParent()->mPackageName.isEmpty() )
                {
                    if( action->isActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-yellow.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-yellow-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
                else
                {
                    if( action->isActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-cyan.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-cyan-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
            }
            else
            {
                if( action->isActive() )
                {
                    icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked.png" ) ), QIcon::Normal, QIcon::Off );
                }
                else
                {
                    icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox.png" ) ), QIcon::Normal, QIcon::Off );
                }
            }
            pItem->setIcon(0, icon);
        }
        else
        {
            QIcon iconError;
            iconError.addPixmap( QPixmap( QStringLiteral( ":/icons/tools-report-bug.png" ) ), QIcon::Normal, QIcon::Off );
            pItem->setIcon( 0, iconError );
            showError( action->getError() );
        }
    }
    mpActionBaseItem->setExpanded( true );

    QStringList sL6;
    sL6 << "Action Key Bindings";
    mpKeyBaseItem = new QTreeWidgetItem( static_cast<QTreeWidgetItem *>(0), sL6 );
    mpKeyBaseItem->setBackground(0,QColor(255,254,215,255));
    QIcon mainIcon6;
    mainIcon6.addPixmap( QPixmap( QStringLiteral( ":/icons/preferences-desktop-keyboard.png" ) ), QIcon::Normal, QIcon::Off );
    mpKeyBaseItem->setIcon( 0, mainIcon6 );
    treeWidget_keys->insertTopLevelItem( 0, mpKeyBaseItem );
    mpKeyBaseItem->setExpanded( true );
    list<TKey *> baseNodeList_key = mpHost->getKeyUnit()->getKeyRootNodeList();
    for(auto key : baseNodeList_key)
    {
        QString s = key->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem( mpKeyBaseItem, sList);
        pItem->setData( 0, Qt::UserRole, QVariant(key->getID()) );
        mpKeyBaseItem->addChild( pItem );
        QIcon icon;
        if( key->hasChildren() )
        {
            expand_child_key( key, pItem );
        }
        if( key->state() )
        {
            if( key->isFolder() )
            {
                if( ! key->mPackageName.isEmpty() )
                {
                    if( key->isActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-brown.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-brown-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
                else if( key->isActive() )
                {
                    if( key->ancestorsActive() )
                    {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-pink.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-grey.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
                else
                {
                    if( key->ancestorsActive() )
                    {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-pink-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-grey-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
            }
            else
            {
                if( key->isActive() )
                {
                    if( key->ancestorsActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked_grey.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
                else
                {
                    if( key->ancestorsActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox-grey.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
            }
            pItem->setIcon(0, icon);
        }
        else
        {
            QIcon iconError;
            iconError.addPixmap( QPixmap( QStringLiteral( ":/icons/tools-report-bug.png" ) ), QIcon::Normal, QIcon::Off );
            pItem->setIcon( 0, iconError );
            showError( key->getError() );
        }
    }
    mpKeyBaseItem->setExpanded( true );
}

void dlgTriggerEditor::repopulateVars()
{
    treeWidget_variables->setUpdatesEnabled( false );
    QStringList sL7;
    sL7 << "Variables";
    mpVarBaseItem = new QTreeWidgetItem( sL7 );
    mpVarBaseItem->setTextAlignment( 0, Qt::AlignLeft|Qt::AlignVCenter );
    mpVarBaseItem->setBackground(0,QColor(255,254,215,255));
    QIcon mainIcon5;
    mainIcon5.addPixmap( QPixmap( QStringLiteral( ":/icons/variables.png" ) ), QIcon::Normal, QIcon::Off );
    mpVarBaseItem->setIcon( 0, mainIcon5 );
    treeWidget_variables->clear();
    mpCurrentVarItem = 0;
    treeWidget_variables->insertTopLevelItem( 0, mpVarBaseItem );
    mpVarBaseItem->setExpanded( true );
    LuaInterface * lI = mpHost->getLuaInterface();
    lI->getVars( false );
    VarUnit * vu = lI->getVarUnit();
    vu->buildVarTree( mpVarBaseItem, vu->getBase(), showHiddenVars );
    mpVarBaseItem->setExpanded( true );
    treeWidget_variables->setUpdatesEnabled( true );
}

void dlgTriggerEditor::expand_child_triggers( TTrigger * pTriggerParent, QTreeWidgetItem * pWidgetItemParent )
{
    list<TTrigger *> * childrenList = pTriggerParent->getChildrenList();
    for(auto trigger : *childrenList)
    {
        QString s = trigger->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem( pWidgetItemParent, sList);
        pItem->setData( 0, Qt::UserRole, trigger->getID() );

        pWidgetItemParent->insertChild( 0, pItem );
        QIcon icon;
        if( trigger->hasChildren() )
        {
            expand_child_triggers( trigger, pItem );
        }
        if( trigger->state() )
        {
            if( trigger->isFilterChain() )
            {
                if( trigger->isActive() )
                {
                    if( trigger->ancestorsActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/filter.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/filter-grey.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
                else
                {
                    if( trigger->ancestorsActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/filter-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/filter-grey-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
            }
            else if( trigger->isFolder() )
            {
                if( trigger->isActive() )
                {
                    if( trigger->ancestorsActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-blue.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-grey.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
                else
                {
                    if( trigger->ancestorsActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-blue-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-grey-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
            }
            else
            {
                if( trigger->isActive() )
                {
                    if( trigger->ancestorsActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked_grey.png" ) ), QIcon::Normal, QIcon::Off );
                    }

                }
                else
                {
                    if( trigger->ancestorsActive() )
                    {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            //pItem->setDisabled(!trigger->ancestorsActive());
            pItem->setIcon(0, icon);
        }
        else
        {
            QIcon iconError;
            iconError.addPixmap( QPixmap( QStringLiteral( ":/icons/tools-report-bug.png" ) ), QIcon::Normal, QIcon::Off );
            pItem->setIcon( 0, iconError );
            showError( trigger->getError() );
        }
    }

}

void dlgTriggerEditor::expand_child_key( TKey * pTriggerParent, QTreeWidgetItem * pWidgetItemParent )
{
    list<TKey *> * childrenList = pTriggerParent->getChildrenList();
    for(auto key : *childrenList)
    {
        QString s = key->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem( pWidgetItemParent, sList);
        pItem->setData( 0, Qt::UserRole, key->getID() );

        pWidgetItemParent->insertChild( 0, pItem );
        QIcon icon;
        if( key->hasChildren() )
        {
            expand_child_key( key, pItem );
        }
        if( key->state() )
        {
            if( key->isFolder() )
            {
                if( key->isActive() )
                {
                    if (key->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-pink.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-grey.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
                else
                {
                    if (key->ancestorsActive()) {
                        icon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-pink-locked.png")), QIcon::Normal, QIcon::Off);
                    } else {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-grey-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
            }
            else
            {
                if( key->isActive() )
                {
                    if (key->ancestorsActive()) {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked.png" ) ), QIcon::Normal, QIcon::Off );
                    } else {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked_grey.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
                else
                {
                    if (key->ancestorsActive()) {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox.png" ) ), QIcon::Normal, QIcon::Off );
                    } else {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox-grey.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
            }
            pItem->setIcon(0, icon);
        }
        else
        {
            QIcon iconError;
            iconError.addPixmap( QPixmap( QStringLiteral( ":/icons/tools-report-bug.png" ) ), QIcon::Normal, QIcon::Off );
            pItem->setIcon( 0, iconError );
            showError( key->getError() );
        }
    }
}


void dlgTriggerEditor::expand_child_scripts( TScript * pTriggerParent, QTreeWidgetItem * pWidgetItemParent )
{
    list<TScript *> * childrenList = pTriggerParent->getChildrenList();
    for(auto script : *childrenList)
    {
        QString s = script->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem( pWidgetItemParent, sList);
        pItem->setData( 0, Qt::UserRole, script->getID() );

        pWidgetItemParent->insertChild( 0, pItem );
        QIcon icon;
        if( script->hasChildren() )
        {
            expand_child_scripts( script, pItem );
        }
        if( script->state() )
        {
            if( script->isFolder() )
            {
                if( script->isActive() )
                {
                    icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-orange.png" ) ), QIcon::Normal, QIcon::Off );
                }
                else
                {
                    icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-orange-locked.png" ) ), QIcon::Normal, QIcon::Off );
                }
            }
            else
            {
                if( script->isActive() )
                {
                    icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked.png" ) ), QIcon::Normal, QIcon::Off );
                }
                else
                {
                    icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox.png" ) ), QIcon::Normal, QIcon::Off );
                }
            }
            pItem->setIcon(0, icon);
        }
        else
        {
            QIcon iconError;
            iconError.addPixmap( QPixmap( QStringLiteral( ":/icons/tools-report-bug.png" ) ), QIcon::Normal, QIcon::Off );
            pItem->setIcon( 0, iconError );
            showError( script->getError() );
        }
    }
}

void dlgTriggerEditor::expand_child_alias( TAlias * pTriggerParent, QTreeWidgetItem * pWidgetItemParent )
{
    list<TAlias *> * childrenList = pTriggerParent->getChildrenList();
    for(auto alias : *childrenList)
    {
        QString s = alias->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem( pWidgetItemParent, sList);
        pItem->setData( 0, Qt::UserRole, alias->getID() );

        pWidgetItemParent->insertChild( 0, pItem );
        QIcon icon;
        if( alias->hasChildren() )
        {
            expand_child_alias( alias, pItem );
        }
        if( alias->state() )
        {
            if( alias->isFolder() )
            {
                if( alias->isActive() )
                {
                    if (alias->ancestorsActive()) {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-violet.png" ) ), QIcon::Normal, QIcon::Off );
                    } else {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-grey.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
                else
                {
                    if (alias->ancestorsActive()) {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-violet-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    } else {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-grey-locked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
            }
            else
            {
                if( alias->isActive() )
                {
                    if (alias->ancestorsActive()) {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked.png" ) ), QIcon::Normal, QIcon::Off );
                    } else {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked_grey.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
                else
                {
                    if (alias->ancestorsActive()) {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox.png" ) ), QIcon::Normal, QIcon::Off );
                    } else {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox-grey.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
            }
            pItem->setIcon(0, icon);
        }
        else
        {
            QIcon iconError;
            iconError.addPixmap( QPixmap( QStringLiteral( ":/icons/tools-report-bug.png" ) ), QIcon::Normal, QIcon::Off );
            pItem->setIcon( 0, iconError );
            showError( alias->getError() );
        }
    }
}

void dlgTriggerEditor::expand_child_action( TAction * pTriggerParent, QTreeWidgetItem * pWidgetItemParent )
{
    list<TAction *> * childrenList = pTriggerParent->getChildrenList();
    for(auto action : *childrenList)
    {
        QString s = action->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem( pWidgetItemParent, sList);
        pItem->setData( 0, Qt::UserRole, action->getID() );

        pWidgetItemParent->insertChild( 0, pItem );
        QIcon icon;
        if( action->hasChildren() )
        {
            expand_child_action( action, pItem );
        }
        if( action->state() )
        {
            if( ! action->getParent()->mPackageName.isEmpty() )
            {
                // Must have a parent (or would not be IN this method) and the
                // parent has a package name - this is a toolbar
                if( action->isActive() )
                {
                    icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-yellow.png" ) ), QIcon::Normal, QIcon::Off );
                }
                else
                {
                    icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-yellow-locked.png" ) ), QIcon::Normal, QIcon::Off );
                }
            }
            else if( action->isFolder() )
            {
                // Is a folder and is not a toolbar - this is a menu
                if( action->isActive() )
                {
                    icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-cyan.png" ) ), QIcon::Normal, QIcon::Off );
                }
                else
                {
                    icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-cyan-locked.png" ) ), QIcon::Normal, QIcon::Off );
                }
            }
            else
            {
                // Is a button
                if( action->isActive() )
                {
                    icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked.png" ) ), QIcon::Normal, QIcon::Off );
                }
                else
                {
                    icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox.png" ) ), QIcon::Normal, QIcon::Off );
                }
            }
            pItem->setIcon(0, icon);
        }
        else
        {
            QIcon iconError;
            iconError.addPixmap( QPixmap( QStringLiteral( ":/icons/tools-report-bug.png" ) ), QIcon::Normal, QIcon::Off );
            pItem->setIcon( 0, iconError );
            showError( action->getError() );
        }
    }
}


void dlgTriggerEditor::expand_child_timers( TTimer * pTimerParent, QTreeWidgetItem * pWidgetItemParent )
{
    list<TTimer *> * childrenList = pTimerParent->getChildrenList();
    for(auto timer : *childrenList)
    {
        QString s = timer->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem( pWidgetItemParent, sList);
        pItem->setData( 0, Qt::UserRole, timer->getID() );

        pWidgetItemParent->insertChild( 0, pItem );
        QIcon icon;
        if( timer->hasChildren() )
        {
            expand_child_timers( timer, pItem );
        }
        if( timer->state() )
        {
            if( timer->isFolder() )
            {
                if( timer->shouldBeActive() )
                {
                    icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-green.png" ) ), QIcon::Normal, QIcon::Off );
                }
                else
                {
                    icon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-green-locked.png" ) ), QIcon::Normal, QIcon::Off );
                }
            }
            else
            {
                if( timer->isOffsetTimer() )
                {
                    if( timer->shouldBeActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/offsettimer-on.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/offsettimer-off.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
                else
                {
                    if( timer->shouldBeActive() )
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox_checked.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                    else
                    {
                        icon.addPixmap( QPixmap( QStringLiteral( ":/icons/tag_checkbox.png" ) ), QIcon::Normal, QIcon::Off );
                    }
                }
            }
            pItem->setIcon(0, icon);
        }
        else
        {
            QIcon iconError;
            iconError.addPixmap( QPixmap( QStringLiteral( ":/icons/tools-report-bug.png" ) ), QIcon::Normal, QIcon::Off );
            pItem->setIcon( 0, iconError );
            showError( timer->getError() );
        }
    }
}

void dlgTriggerEditor::slot_showSearchAreaResults(const bool isChecked)
{
    if( isChecked )
    {
        if( ! button_toggleSearchAreaResults->isChecked() )
        {
            // If this slot is called "manually" the checked state of the button
            // may not match the setting, so make it do so, note that the
            // setChecked(bool) method does NOT invoke the clicked(bool) signal
            // that is connected to here in the constructor, but it does the
            // toggled(bool) one, which is why we use the former...
            button_toggleSearchAreaResults->setChecked( true );
        }
        treeWidget_searchResults->show();
    }
    else {
        if( button_toggleSearchAreaResults->isChecked() )
        {
            button_toggleSearchAreaResults->setChecked( false );
        }
        treeWidget_searchResults->hide();
    }
}

void dlgTriggerEditor::saveOpenChanges()
{
    if( !mCurrentView )
    {
        return;
    }

    switch( mCurrentView )
    {
    case cmTriggerView:
        saveTrigger();
        break;
    case cmTimerView:
        saveTimer();
        break;
    case cmAliasView:
        saveAlias();
        break;
    case cmScriptView:
        saveScript();
        break;
    case cmActionView:
        saveAction();
        break;
    case cmKeysView:
        saveKey();
        break;
    case cmVarsView:
        saveVar();
        break;
    };

}

void dlgTriggerEditor::enterEvent( QEvent *pE )
{
    if( mNeedUpdateData )
    {
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

void dlgTriggerEditor::focusInEvent( QFocusEvent * pE )
{
    if( mNeedUpdateData )
    {
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

    if( ! mCurrentView )
    {
        mpCurrentTriggerItem = 0;
        mpCurrentAliasItem = 0;
        mpCurrentKeyItem = 0;
        mpCurrentActionItem = 0;
        mpCurrentScriptItem = 0;
        mpCurrentTimerItem = 0;
        return;
    }

    if( mpCurrentTriggerItem )
        mpCurrentTriggerItem->setSelected( true );
    if( mpCurrentTimerItem )
        mpCurrentTimerItem->setSelected( true );
    if( mpCurrentAliasItem )
        mpCurrentAliasItem->setSelected( true );
    if( mpCurrentScriptItem )
        mpCurrentScriptItem->setSelected( true );
    if( mpCurrentActionItem )
        mpCurrentActionItem->setSelected( true );
    if( mpCurrentKeyItem )
        mpCurrentKeyItem->setSelected( true );
}

void dlgTriggerEditor::focusOutEvent( QFocusEvent * pE )
{
    saveOpenChanges();
}

void dlgTriggerEditor::changeView( int view )
{
    saveOpenChanges();

    if( mNeedUpdateData )
    {
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

    mpSourceEditor->setReadOnly(false);
    if (mCurrentView != view)
        mpSourceEditor->clear();
    mCurrentView = view;

    mpTriggersMainArea->hide();
    mpTimersMainArea->hide();
    mpScriptsMainArea->hide();
    mpAliasMainArea->hide();
    mpActionsMainArea->hide();
    mpKeysMainArea->hide();
    mpVarsMainArea->hide();
    button_displayAllVariables->hide();

    mpSystemMessageArea->hide();
    treeWidget_triggers->hide();
    treeWidget_aliases->hide();
    treeWidget_timers->hide();
    treeWidget_scripts->hide();
    treeWidget_actions->hide();
    treeWidget_keys->hide();
    treeWidget_variables->hide();
}

void dlgTriggerEditor::slot_show_timers()
{
    changeView( cmTimerView );
    QTreeWidgetItem * pI = treeWidget_timers->topLevelItem( 0 );
    if( pI )
    {
        if( pI->childCount() > 0 )
        {
            mpTimersMainArea->show();
            mpSourceEditorArea->show();
            slot_timer_selected( treeWidget_timers->currentItem() );
        }
        else
        {
            mpTimersMainArea->hide();
            mpSystemMessageArea->show();
            mpSourceEditorArea->hide();
            mpSystemMessageArea->notificationAreaIconLabelInformation->show();
            mpSystemMessageArea->notificationAreaIconLabelError->hide();
            mpSystemMessageArea->notificationAreaIconLabelWarning->hide();
            QString msg = "To add a timer click on the 'Add' icon above.";
            mpSystemMessageArea->notificationAreaMessageBox->setText( msg );
        }
    }
    treeWidget_timers->show();
}

void dlgTriggerEditor::slot_show_triggers()
{
    changeView( cmTriggerView );
    QTreeWidgetItem * pI = treeWidget_triggers->topLevelItem( 0 );
    if( pI )
    {
        if( pI->childCount() > 0 )
        {
            mpTriggersMainArea->show();
            mpSourceEditorArea->show();
            slot_trigger_selected( treeWidget_triggers->currentItem() );
        }
        else
        {
            mpTriggersMainArea->hide();
            mpSourceEditorArea->hide();
            mpSystemMessageArea->show();
            mpSystemMessageArea->notificationAreaIconLabelInformation->show();
            mpSystemMessageArea->notificationAreaIconLabelError->hide();
            mpSystemMessageArea->notificationAreaIconLabelWarning->hide();
            QString msg = "To add a trigger click on the 'Add' icon above.";
            mpSystemMessageArea->notificationAreaMessageBox->setText( msg );
        }
    }
    treeWidget_triggers->show();
}

void dlgTriggerEditor::slot_show_scripts()
{
    changeView( cmScriptView );
    QTreeWidgetItem * pI = treeWidget_scripts->topLevelItem( 0 );
    if( pI )
    {
        if( pI->childCount() > 0 )
        {
            mpScriptsMainArea->show();
            mpSourceEditorArea->show();
            slot_scripts_selected( treeWidget_scripts->currentItem() );
        }
        else
        {
            mpScriptsMainArea->hide();
            mpSourceEditorArea->hide();
            mpSystemMessageArea->show();
            mpSystemMessageArea->notificationAreaIconLabelInformation->show();
            mpSystemMessageArea->notificationAreaIconLabelError->hide();
            mpSystemMessageArea->notificationAreaIconLabelWarning->hide();
            QString msg = "To make a new general purpose script click on the 'Add' icon above.";
            mpSystemMessageArea->notificationAreaMessageBox->setText( msg );
        }
    }
    treeWidget_scripts->show();
}

void dlgTriggerEditor::slot_show_keys()
{
    changeView( cmKeysView );
    QTreeWidgetItem * pI = treeWidget_keys->topLevelItem( 0 );
    if( pI )
    {
        if( pI->childCount() > 0 )
        {
            mpKeysMainArea->show();
            mpSourceEditorArea->show();
            slot_key_selected( treeWidget_keys->currentItem() );
        }
        else
        {
            mpKeysMainArea->hide();
            mpSourceEditorArea->hide();
            mpSystemMessageArea->show();
            mpSystemMessageArea->notificationAreaIconLabelInformation->show();
            mpSystemMessageArea->notificationAreaIconLabelError->hide();
            mpSystemMessageArea->notificationAreaIconLabelWarning->hide();
            QString msg = "To make a new key binding click on the 'Add' icon above.";
            mpSystemMessageArea->notificationAreaMessageBox->setText( msg );
        }
    }

    treeWidget_keys->show();
}

void dlgTriggerEditor::slot_show_vars( )
{
    changeView( cmVarsView );
    repopulateVars();
    mpCurrentVarItem = 0;
    mpSourceEditorArea->show();
    button_displayAllVariables->show();
    button_displayAllVariables->setChecked( showHiddenVars );
    QTreeWidgetItem * pI = treeWidget_variables->topLevelItem( 0 );
    if( pI )
    {
        if( pI->childCount() > 0 )
        {
            mpVarsMainArea->show();
            slot_var_selected( treeWidget_variables->currentItem() );
        }
        else
        {
            mpVarsMainArea->hide();
            mpSystemMessageArea->show();
            mpSystemMessageArea->notificationAreaIconLabelInformation->show();
            mpSystemMessageArea->notificationAreaIconLabelError->hide();
            mpSystemMessageArea->notificationAreaIconLabelWarning->hide();
            QString msg = "To make a new variable click on the 'Add Item' icon above.\nTo add a table click 'Add Group'.";
            mpSystemMessageArea->notificationAreaMessageBox->setText( msg );
        }
    }
    treeWidget_variables->show();
}

void dlgTriggerEditor::show_vars( )
{
    //no repopulation of variables
    changeView( cmVarsView );
    mpCurrentVarItem = 0;
    mpSourceEditorArea->show();
    button_displayAllVariables->show();
    button_displayAllVariables->setChecked( showHiddenVars );
    QTreeWidgetItem * pI = treeWidget_variables->topLevelItem( 0 );
    if( pI )
    {
        if( pI->childCount() > 0 )
        {
            mpVarsMainArea->show();
            slot_var_selected( treeWidget_variables->currentItem() );
        }
        else
        {
            mpVarsMainArea->hide();
            mpSystemMessageArea->show();
            mpSystemMessageArea->notificationAreaIconLabelInformation->show();
            mpSystemMessageArea->notificationAreaIconLabelError->hide();
            mpSystemMessageArea->notificationAreaIconLabelWarning->hide();
            QString msg = "To make a new variable click on the 'Add Item' icon above.\nTo add a table click 'Add Group'.";
            mpSystemMessageArea->notificationAreaMessageBox->setText( msg );
        }
    }
    treeWidget_variables->show();
}


void dlgTriggerEditor::slot_show_aliases()
{
    changeView( cmAliasView );
    QTreeWidgetItem * pI = treeWidget_aliases->topLevelItem( 0 );
    if( pI )
    {
        if( pI->childCount() > 0 )
        {
            mpAliasMainArea->show();
            mpSourceEditorArea->show();
            slot_alias_selected( treeWidget_aliases->currentItem() );
        }
        else
        {
            mpAliasMainArea->hide();
            mpSourceEditorArea->hide();
            mpSystemMessageArea->show();
            mpSystemMessageArea->notificationAreaIconLabelInformation->show();
            mpSystemMessageArea->notificationAreaIconLabelError->hide();
            mpSystemMessageArea->notificationAreaIconLabelWarning->hide();
            QString msg = "To make a new alias click on the 'Add' icon above.";
            mpSystemMessageArea->notificationAreaMessageBox->setText( msg );
        }
    }

    treeWidget_aliases->show();
}

void dlgTriggerEditor::showError(const QString& error )
{
    mpSystemMessageArea->notificationAreaIconLabelInformation->hide();
    mpSystemMessageArea->notificationAreaIconLabelError->show();
    mpSystemMessageArea->notificationAreaIconLabelWarning->hide();
    mpSystemMessageArea->notificationAreaMessageBox->setText( error );
    mpSystemMessageArea->show();
}

void dlgTriggerEditor::showInfo(const QString& error )
{
    mpSystemMessageArea->notificationAreaIconLabelError->hide();
    mpSystemMessageArea->notificationAreaIconLabelWarning->hide();
    mpSystemMessageArea->notificationAreaIconLabelInformation->show();
    mpSystemMessageArea->notificationAreaMessageBox->setText( error );
    mpSystemMessageArea->show();
}

void dlgTriggerEditor::showWarning(const QString& error )
{
    mpSystemMessageArea->notificationAreaIconLabelInformation->hide();
    mpSystemMessageArea->notificationAreaIconLabelError->hide();
    mpSystemMessageArea->notificationAreaIconLabelWarning->show();
    mpSystemMessageArea->notificationAreaMessageBox->setText( error );
    mpSystemMessageArea->show();
}

void dlgTriggerEditor::slot_show_actions()
{
    changeView( cmActionView );
    QTreeWidgetItem * pI = treeWidget_actions->topLevelItem( 0 );
    if( pI )
    {
        if( pI->childCount() > 0 )
        {
            mpActionsMainArea->show();
            mpSourceEditorArea->show();
            slot_action_selected( treeWidget_actions->currentItem() );
        }
        else
        {
            mpActionsMainArea->hide();
            mpSystemMessageArea->show();
            mpSystemMessageArea->notificationAreaIconLabelInformation->show();
            mpSystemMessageArea->notificationAreaIconLabelError->hide();
            mpSystemMessageArea->notificationAreaIconLabelWarning->hide();
            QString msg = "To make a new button, you have to add a button bar first and then add buttons to the group. Click on the 'Add Group' icon above to make a button bar.";
            mpSystemMessageArea->notificationAreaMessageBox->setText( msg );
        }
    }

    treeWidget_actions->show();

}

void dlgTriggerEditor::slot_save_edit()
{
    switch( mCurrentView )
    {
        case cmTriggerView:
            saveTrigger();
            break;
        case cmTimerView:
            saveTimer();
            break;
        case cmAliasView:
            saveAlias();
            break;
        case cmScriptView:
            saveScript();
            break;
        case cmActionView:
            saveAction();
            break;
        case cmKeysView:
            saveKey();
            break;
        case cmVarsView:
            saveVar();
            break;
        default: qWarning()<<"ERROR: dlgTriggerEditor::slot_save_edit() undefined view";
    };

// There was a mpHost->serialize() call here, but that code was
// "short-circuited" and returned without doing anything;
}

void dlgTriggerEditor::slot_add_new()
{
    switch( mCurrentView )
    {
    case cmTriggerView:
        slot_addTrigger();
        break;
    case cmTimerView:
        slot_addTimer();
        break;
    case cmAliasView:
        slot_addAlias();
        break;
    case cmScriptView:
        slot_addScript();
        break;
    case cmActionView:
        slot_addAction();
        break;
    case cmKeysView:
        slot_addKey();
        break;
    case cmVarsView:
        slot_addVar();
        break;
    default: qDebug()<<"ERROR: dlgTriggerEditor::slot_save_edit() undefined view";
    };
}

void dlgTriggerEditor::slot_add_new_folder()
{
    switch( mCurrentView )
    {
    case cmTriggerView:
        slot_addTriggerGroup();
        break;
    case cmTimerView:
        slot_addTimerGroup();
        break;
    case cmAliasView:
        slot_addAliasGroup();
        break;
    case cmScriptView:
        slot_addScriptGroup();
        break;
    case cmActionView:
        slot_addActionGroup();
        break;
    case cmKeysView:
        slot_addKeyGroup();
        break;
    case cmVarsView:
        slot_addVarGroup();
        break;
    default: qDebug()<<"ERROR: dlgTriggerEditor::slot_save_edit() undefined view";
    };
}

void dlgTriggerEditor::slot_toggle_active()
{
    switch( mCurrentView )
    {
    case cmTriggerView:
        slot_trigger_toggle_active();
        break;
    case cmTimerView:
        slot_timer_toggle_active();
        break;
    case cmAliasView:
        slot_alias_toggle_active();
        break;
    case cmScriptView:
        slot_script_toggle_active();
        break;
    case cmActionView:
        slot_action_toggle_active();
        break;
    case cmKeysView:
        slot_key_toggle_active();
        break;

    default: qDebug()<<"ERROR: dlgTriggerEditor::slot_save_edit() undefined view";
    };
}

void dlgTriggerEditor::slot_delete_item()
{
    switch( mCurrentView )
    {
    case cmTriggerView:
        slot_deleteTrigger();
        break;
    case cmTimerView:
        slot_deleteTimer();
        break;
    case cmAliasView:
        slot_deleteAlias();
        break;
    case cmScriptView:
        slot_deleteScript();
        break;
    case cmActionView:
        slot_deleteAction();
        break;
    case cmKeysView:
        slot_deleteKey();
        break;
    case cmVarsView:
        slot_deleteVar();
        break;
    default: qDebug()<<"ERROR: dlgTriggerEditor::slot_save_edit() undefined view";
    };
}

void dlgTriggerEditor::slot_item_selected_save( QTreeWidgetItem * pItem )
{
    if( ! pItem ) return;

    switch( mCurrentView )
    {
    case cmTriggerView:
        saveTrigger();
        break;
    case cmTimerView:
        saveTimer();
        break;
    case cmAliasView:
        saveAlias();
        break;
    case cmScriptView:
        saveScript();
        break;
    case cmActionView:
        saveAction();
        break;
    case cmKeysView:
        saveKey();
        break;
    case cmVarsView:
        saveVar();
        break;
    };

}

void dlgTriggerEditor::slot_script_main_area_edit_handler(QListWidgetItem*)
{
    QListWidgetItem * pItem = mpScriptsMainArea->listWidget_registered_event_handlers->currentItem();
    if( ! pItem ) return;
    mIsScriptsMainAreaEditHandler = true;
    mpScriptsMainAreaEditHandlerItem = pItem;
    QString regex = pItem->text();
    if( regex.size() < 1 )
    {
        mIsScriptsMainAreaEditHandler = false;
        return;
    }
    mpScriptsMainArea->lineEdit->setText( regex );
}

void dlgTriggerEditor::slot_script_main_area_delete_handler()
{
    mpScriptsMainArea->listWidget_registered_event_handlers->takeItem( mpScriptsMainArea->listWidget_registered_event_handlers->currentRow() );
}

void dlgTriggerEditor::slot_script_main_area_add_handler()
{
    if( mIsScriptsMainAreaEditHandler )
    {
        if( ! mpScriptsMainAreaEditHandlerItem )
        {
            mIsScriptsMainAreaEditHandler = false;
            goto LAZY;
            return;
        }
        QListWidgetItem * pItem = mpScriptsMainArea->listWidget_registered_event_handlers->currentItem();
        if( ! pItem ) return;
        pItem->setText( mpScriptsMainArea->lineEdit->text() );
        mIsScriptsMainAreaEditHandler = false;
        mpScriptsMainAreaEditHandlerItem = 0;
    }
    else
    {
    LAZY:
        auto pItem = new QListWidgetItem;
        pItem->setText( mpScriptsMainArea->lineEdit->text() );
        mpScriptsMainArea->listWidget_registered_event_handlers->addItem( pItem );

    }
    mpScriptsMainArea->lineEdit->clear();
}

void dlgTriggerEditor::slot_debug_mode()
{
    mudlet::mpDebugArea->setVisible(!mudlet::debugMode);
    mudlet::debugMode = !mudlet::debugMode;
    mudlet::mpDebugArea->setWindowTitle("Central Debug Console");
}

void dlgTriggerEditor::exportTrigger( QFile & file )
{
    QString name;
    TTrigger * pT = 0;
    QTreeWidgetItem * pItem = treeWidget_triggers->currentItem();
    if( pItem )
    {
        int triggerID = pItem->data( 0, Qt::UserRole ).toInt();
        pT = mpHost->getTriggerUnit()->getTrigger( triggerID );
        if( pT )
        {
            name = pT->getName();
        }
        else
        {
            QMessageBox::warning(this, tr("Export Package:"),
                             tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
            return;
        }
    }
    else
    {
        QMessageBox::warning(this, tr("Export Package:"),
                             tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
        return;
    }
    XMLexport writer( pT );
    if( writer.exportTrigger( & file ) )
    {
        statusBar()->showMessage(tr("Package ")+name+tr(" saved"), 2000);
    }

}

void dlgTriggerEditor::exportTimer( QFile & file )
{
    QString name;
    TTimer * pT = 0;
    QTreeWidgetItem * pItem = treeWidget_timers->currentItem();
    if( pItem )
    {
        int triggerID = pItem->data( 0, Qt::UserRole ).toInt();
        pT = mpHost->getTimerUnit()->getTimer( triggerID );
        if( pT )
        {
            name = pT->getName();
        }
        else
        {
            QMessageBox::warning(this, tr("Export Package:"),
                             tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
            return;
        }
    }
    else
    {
        QMessageBox::warning(this, tr("Export Package:"),
                             tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
        return;
    }
    XMLexport writer( pT );
    if( writer.exportTimer( & file ) )
    {
        statusBar()->showMessage(tr("Package")+name+tr(" saved"), 2000);
    }

}

void dlgTriggerEditor::exportAlias( QFile & file )
{
    QString name;
    TAlias * pT = 0;
    QTreeWidgetItem * pItem = treeWidget_aliases->currentItem();
    if( pItem )
    {
        int triggerID = pItem->data( 0, Qt::UserRole ).toInt();
        pT = mpHost->getAliasUnit()->getAlias( triggerID );
        if( pT )
        {
            name = pT->getName();
        }
        else
        {
            QMessageBox::warning(this, tr("Export Package:"),
                             tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
            return;
        }
    }
    else
    {
        QMessageBox::warning(this, tr("Export Package:"),
                             tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
        return;
    }
    XMLexport writer( pT );
    if( writer.exportAlias( & file ) )
    {
        statusBar()->showMessage(tr("Package ")+name+tr(" saved"), 2000);
    }

}

void dlgTriggerEditor::exportAction( QFile & file )
{
    QString name;
    TAction * pT = 0;
    QTreeWidgetItem * pItem = treeWidget_actions->currentItem();
    if( pItem )
    {
        int triggerID = pItem->data( 0, Qt::UserRole ).toInt();
        pT = mpHost->getActionUnit()->getAction( triggerID );
        if( pT )
        {
            name = pT->getName();
        }
        else
        {
            QMessageBox::warning(this, tr("Export Package:"),
                             tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
            return;
        }
    }
    else
    {
        QMessageBox::warning(this, tr("Export Package:"),
                             tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
        return;
    }
    XMLexport writer( pT );
    if( writer.exportAction( & file ) )
    {
        statusBar()->showMessage(tr("Package ")+name+tr(" saved"), 2000);
    }

}

void dlgTriggerEditor::exportScript( QFile & file )
{
    QString name;
    TScript * pT = 0;
    QTreeWidgetItem * pItem = treeWidget_scripts->currentItem();
    if( pItem )
    {
        int triggerID = pItem->data( 0, Qt::UserRole ).toInt();
        pT = mpHost->getScriptUnit()->getScript( triggerID );
        if( pT )
        {
            name = pT->getName();
        }
        else
        {
            QMessageBox::warning(this, tr("Export Package:"),
                             tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
            return;
        }
    }
    else
    {
        QMessageBox::warning(this, tr("Export Package:"),
                             tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
        return;
    }
    XMLexport writer( pT );
    if( writer.exportScript( & file ) )
    {
        statusBar()->showMessage(tr("Package ")+name+tr(" saved"), 2000);
    }

}

void dlgTriggerEditor::exportKey( QFile & file )
{
    QString name;
    TKey * pT = 0;
    QTreeWidgetItem * pItem = treeWidget_keys->currentItem();
    if( pItem )
    {
        int triggerID = pItem->data( 0, Qt::UserRole ).toInt();
        pT = mpHost->getKeyUnit()->getKey( triggerID );
        if( pT )
        {
            name = pT->getName();
        }
        else
        {
             QMessageBox::warning(this, tr("Export Package:"),
                             tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
            return;
        }

    }
    else
    {
        QMessageBox::warning(this, tr("Export Package:"),
                             tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
        return;
    }
    XMLexport writer( pT );
    if( writer.exportKey( & file ) )
    {
        statusBar()->showMessage(tr("Package ")+name+tr(" saved"), 2000);
    }

}

void dlgTriggerEditor::slot_export()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Triggers"),
                                                    QDir::currentPath(),
                                                    tr("Mudlet packages (*.xml)"));
    if( fileName.isEmpty() )
    {
        return;
    }

    // Must be case insensitive to work on MacOS platforms, possibly a cause of
    // https://bugs.launchpad.net/mudlet/+bug/1417234
    if( ! fileName.endsWith( QStringLiteral( ".xml" ), Qt::CaseInsensitive ) )
    {
        fileName.append( QStringLiteral( ".xml" ) );
    }


    QFile file(fileName);
    if( ! file.open(QFile::WriteOnly | QFile::Text) )
    {
        QMessageBox::warning(this, tr("export package:"),
                             tr("Cannot write file %1:\n%2.")
                             .arg(fileName, file.errorString()));
        return;
    }

    switch( mCurrentView )
    {
    case cmTriggerView:
        exportTrigger( file );
        break;
    case cmTimerView:
        exportTimer( file );
        break;
    case cmAliasView:
        exportAlias( file );
        break;
    case cmScriptView:
        exportScript( file );
        break;
    case cmActionView:
        exportAction( file );
        break;
    case cmKeysView:
        exportKey( file );
        break;
    };
}

void dlgTriggerEditor::slot_import()
{
    if( mCurrentView )
    {
        switch( mCurrentView )
        {
            case cmTriggerView:
                saveTrigger();
                break;
            case cmTimerView:
                saveTimer();
                break;
            case cmAliasView:
                saveAlias();
                break;
            case cmScriptView:
                saveScript();
                break;
            case cmActionView:
                saveAction();
                break;
            case cmKeysView:
                saveKey();
                break;
        };
    }

    QString fileName = QFileDialog::getOpenFileName(this, tr("Import Mudlet Package"),
                                                    QDir::currentPath());
    if( fileName.isEmpty() ) return;

    QFile file(fileName);
    if( ! file.open(QFile::ReadOnly | QFile::Text) )
    {
        QMessageBox::warning(this, tr("Import Mudlet Package:"),
                             tr("Cannot read file %1:\n%2.")
                             .arg(fileName, file.errorString()));
        return;
    }

    QString packageName = fileName.section("/",-1 );
    packageName.replace( ".zip" , "" );
    packageName.replace( "trigger", "" );
    packageName.replace( "xml", "" );
    packageName.replace( ".mpackage" , "" );
    packageName.replace( '/' , "" );
    packageName.replace( '\\' , "" );
    packageName.replace( '.' , "" );

    if( mpHost->mInstalledPackages.contains( packageName ) )
    {
        QMessageBox::information(this, tr("Import Mudlet Package:"), tr("Package %1 is already installed.").arg(packageName));
        return;
    }
    QFile file2;
    if( fileName.endsWith(".zip") || fileName.endsWith(".mpackage") )
    {
        QString _home = QDir::homePath();
        _home.append( "/.config/mudlet/profiles/" );
        _home.append( mpHost->getName() );
        QString _dest = QString( "%1/%2/").arg(_home, packageName);
        QDir _tmpDir;
        _tmpDir.mkpath(_dest);
        QString _script = QString( "unzip([[%1]], [[%2]])" ).arg(fileName, _dest);
        mpHost->mLuaInterpreter.compileAndExecuteScript( _script );

        // requirements for zip packages:
        // - packages must be compressed in zip format
        // - file extension should be .mpackage (though .zip is accepted)
        // - there can only be a single xml file per package
        // - the xml file must be located in the root directory of the zip package. example: myPack.zip contains: the folder images and the file myPack.xml

        QDir _dir( _dest );
        QStringList _filterList;
        _filterList << "*.xml" << "*.trigger";
        QFileInfoList entries = _dir.entryInfoList( _filterList, QDir::Files );
        if( entries.size() > 0 )
        {
            file2.setFileName( entries[0].absoluteFilePath() );
        }
    }
    else
    {
        file2.setFileName( fileName );
    }
    file2.open(QFile::ReadOnly | QFile::Text);

    mpHost->mInstalledPackages.append( packageName );
    QString profileName = mpHost->getName();
    QString login = mpHost->getLogin();
    QString pass = mpHost->getPass();

    treeWidget_triggers->clear();
    treeWidget_aliases->clear();
    treeWidget_actions->clear();
    treeWidget_timers->clear();
    treeWidget_keys->clear();
    treeWidget_scripts->clear();

    XMLimport reader( mpHost );
    reader.importPackage( & file2, packageName ); // TODO: Missing false return value handler

    mpHost->setName( profileName );
    mpHost->setLogin( login );
    mpHost->setPass( pass );

    slot_profileSaveAction();

    fillout_form();

    mpCurrentTriggerItem = 0;
    mpCurrentTimerItem = 0;
    mpCurrentAliasItem = 0;
    mpCurrentScriptItem = 0;
    mpCurrentActionItem = 0;
    mpCurrentKeyItem = 0;

    slot_show_triggers();
}

void dlgTriggerEditor::doCleanReset()
{
    if( mCurrentView )
    {
        switch( mCurrentView )
        {
            case cmTriggerView:
                saveTrigger();
                break;
            case cmTimerView:
                saveTimer();
                break;
            case cmAliasView:
                saveAlias();
                break;
            case cmScriptView:
                saveScript();
                break;
            case cmActionView:
                saveAction();
                break;
            case cmKeysView:
                saveKey();
                break;
        };
    }
    treeWidget_triggers->clear();
    treeWidget_aliases->clear();
    treeWidget_actions->clear();
    treeWidget_timers->clear();
    treeWidget_keys->clear();
    treeWidget_scripts->clear();
    fillout_form();
    mpCurrentTriggerItem = 0;
    mpCurrentTimerItem = 0;
    mpCurrentAliasItem = 0;
    mpCurrentScriptItem = 0;
    mpCurrentActionItem = 0;
    mpCurrentKeyItem = 0;
    slot_show_triggers();
}


void dlgTriggerEditor::slot_profileSaveAction()
{
    std::tuple<bool, QString, QString> result = mpHost->saveProfile(nullptr, true);

    if (std::get<0>(result) == false) {
        QMessageBox::critical(this, tr("Couldn't save profile"), tr("Sorry, couldn't save your profile - got the following error: %1").arg(std::get<2>(result)));
    }
}

void dlgTriggerEditor::slot_profileSaveAsAction()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Backup Profile"),
                                                    QDir::homePath(),
                                                    tr("trigger files (*.trigger *.xml)"));

    if( fileName.isEmpty() )
    {
        return;
    }
    // Must be case insensitive to work on MacOS platforms, possibly a cause of
    // https://bugs.launchpad.net/mudlet/+bug/1417234
    if(  ! fileName.endsWith( QStringLiteral( ".xml" ), Qt::CaseInsensitive )
      && ! fileName.endsWith( QStringLiteral( ".trigger" ), Qt::CaseInsensitive ) )
    {
        fileName.append( QStringLiteral( ".xml" ) );
    }

    QFile file(fileName);
    if( ! file.open(QFile::WriteOnly | QFile::Text) )
    {
       QMessageBox::warning(this, tr("Backup Profile:"),
                            tr("Cannot write file %1:\n%2.")
                            .arg(fileName, file.errorString()));
       return;
    }
    XMLexport writer( mpHost ); //just export a generic package without host element
    writer.exportGenericPackage( & file );
    file.close();
}

bool dlgTriggerEditor::event( QEvent * event )
{
    if( mIsGrabKey )
    {
        if( event->type() == QEvent::KeyPress )
        {
            QKeyEvent * ke = static_cast<QKeyEvent *>( event );
            QList<QAction *> actionList = toolBar->actions();
            switch( ke->key() )
            {
                case 0x01000000:
                    mIsGrabKey = false;
                    for(auto & action : actionList) {
                        if ( action->text() == "Save Item" ) {
                            action->setShortcut(tr("Ctrl+S"));
                        } else if ( action->text() == "Save Profile" ) {
                            action->setShortcut(tr("Ctrl+Shift+S"));
                        }
                    }
                    ke->accept();
                    return true;
                case 0x01000020:
                case 0x01000021:
                case 0x01000022:
                case 0x01000023:
                case 0x01001103:
                    break;
                default:
                    grab_key_callback( ke->key(), ke->modifiers() );
                    mIsGrabKey = false;
                    for(auto & action : actionList) {
                        if ( action->text() == "Save Item" ) {
                            action->setShortcut(tr("Ctrl+S"));
                        } else if ( action->text() == "Save Profile" ) {
                            action->setShortcut(tr("Ctrl+Shift+S"));
                        }
                    }
                    ke->accept();
                    return true;
            }
        }
    }
    return QMainWindow::event( event );
}


void dlgTriggerEditor::slot_grab_key()
{
    mIsGrabKey = true;
    QList<QAction *> actionList = toolBar->actions();
    for(auto & action : actionList) {
        if ( action->text() == "Save Item" ) {
            action->setShortcut(tr(""));
        } else if ( action->text() == "Save Profile" ) {
            action->setShortcut(tr(""));
        }
    }
}

void dlgTriggerEditor::grab_key_callback( int key, int modifier )
{
    KeyUnit * pKeyUnit = mpHost->getKeyUnit();
    if( ! pKeyUnit ) return;
    QString keyName = pKeyUnit->getKeyName( key, modifier );
    QString name = keyName;
    mpKeysMainArea->lineEdit_key->setText( name );
    QTreeWidgetItem * pItem = treeWidget_keys->currentItem();
    if( pItem )
    {
        int triggerID = pItem->data(0, Qt::UserRole).toInt();
        TKey * pT = mpHost->getKeyUnit()->getKey( triggerID );
        if( pT )
        {
            pT->setKeyCode( key );
            pT->setKeyModifiers( modifier );
        }
    }
}

void dlgTriggerEditor::slot_chose_action_icon()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Seclect Icon"),
        QDir::homePath(),
        tr("Images (*.png *.xpm *.jpg)"));
    mpActionsMainArea->lineEdit_action_icon->setText( fileName );
}

void dlgTriggerEditor::slot_toggle_isPushDownButton( const int state )
{
    if( state == Qt::Checked )
    {
        mpActionsMainArea->lineEdit_command_up->show();
        mpActionsMainArea->label_command_up->show();
        mpActionsMainArea->label_command_down->setText( tr( "Command (down):" ) );
    }
    else
    {
        mpActionsMainArea->lineEdit_command_up->hide();
        mpActionsMainArea->label_command_up->hide();
        mpActionsMainArea->label_command_down->setText( tr( "Command:" ) );
    }
}

void dlgTriggerEditor::slot_colorizeTriggerSetFgColor()
{
    QTreeWidgetItem * pItem = mpCurrentTriggerItem;
    if( ! pItem ) return;
    if( ! pItem->parent() ) return;

    auto color = QColorDialog::getColor( mpTriggersMainArea->pushButtonFgColor->palette().color( QPalette::Button ), this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color: ")+color.name()+QString(";}");
        mpTriggersMainArea->pushButtonFgColor->setStyleSheet( styleSheet );
        mpTriggersMainArea->pushButtonFgColor->setPalette( palette );
    }
}

void dlgTriggerEditor::slot_colorizeTriggerSetBgColor()
{
    QTreeWidgetItem * pItem = mpCurrentTriggerItem;
    if( ! pItem ) return;
    if( ! pItem->parent() ) return;

    auto color = QColorDialog::getColor( mpTriggersMainArea->pushButtonBgColor->palette().color( QPalette::Button ), this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        mpTriggersMainArea->pushButtonBgColor->setStyleSheet( styleSheet );
        mpTriggersMainArea->pushButtonBgColor->setPalette( palette );
    }
}

void dlgTriggerEditor::slot_soundTrigger()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("choose sound file"),
                                                    QDir::homePath(),
                                                    tr("*"));
    mpTriggersMainArea->lineEdit_soundFile->setText( fileName );
}

void dlgTriggerEditor::slot_color_trigger_fg()
{
    QTreeWidgetItem * pItem = mpCurrentTriggerItem;
    if( ! pItem )
    {
        return;
    }
    int triggerID = pItem->data( 0, Qt::UserRole ).toInt();
    TTrigger * pT = mpHost->getTriggerUnit()->getTrigger( triggerID );
    if( ! pT ) return;

    QPushButton * pB = (QPushButton *) sender();
    if( ! pB ) return;
    int row = ((dlgTriggerPatternEdit*)pB->parent())->mRow;
    dlgTriggerPatternEdit * pI = mTriggerPatternEdit[row];
    if( ! pI ) return;

    QString pattern = pI->lineEdit->text();
    QRegExp regex = QRegExp("FG(\\d+)BG(\\d+)");
    int _pos = regex.indexIn( pattern );
    int ansiFg, ansiBg;
    if( _pos == -1 )
    {
        //setup default colors
        ansiFg = 0;
        ansiBg = 0;
    }
    else
    {
        // use user defined colors
        ansiFg = regex.cap(1).toInt();
        ansiBg = regex.cap(2).toInt();
    }
    pT->mColorTriggerFgAnsi = ansiFg;
    pT->mColorTriggerBgAnsi = ansiBg;

    auto pD = new dlgColorTrigger(this, pT, 0 );
    pD->setModal( true );
    pD->setWindowModality( Qt::ApplicationModal );
    pD->exec();
    QPalette palette;
    QColor color = pT->mColorTriggerFgColor;
    palette.setColor( QPalette::Button, color );
    QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");


    row = ((dlgTriggerPatternEdit*)pB->parent())->mRow;
    pI = mTriggerPatternEdit[row];
    pI->lineEdit->setText(QString("FG%1BG%2").arg(QString::number(pT->mColorTriggerFgAnsi), QString::number(pT->mColorTriggerBgAnsi)) );
    pB->setStyleSheet( styleSheet );
}

void dlgTriggerEditor::slot_color_trigger_bg()
{
    QTreeWidgetItem * pItem = mpCurrentTriggerItem;
    if( ! pItem )
    {
        return;
    }
    int triggerID = pItem->data( 0, Qt::UserRole ).toInt();
    TTrigger * pT = mpHost->getTriggerUnit()->getTrigger( triggerID );
    if( ! pT ) return;

    QPushButton * pB = (QPushButton *) sender();
    if( ! pB ) return;
    int row = ((dlgTriggerPatternEdit*)pB->parent())->mRow;
    dlgTriggerPatternEdit * pI = mTriggerPatternEdit[row];
    if( ! pI ) return;

    QString pattern = pI->lineEdit->text();
    QRegExp regex = QRegExp("FG(\\d+)BG(\\d+)");
    int _pos = regex.indexIn( pattern );
    int ansiFg, ansiBg;
    if( _pos == -1 )
    {
        //setup default colors
        ansiFg = 0;
        ansiBg = 0;
    }
    else
    {
        // use user defined colors
        ansiFg = regex.cap(1).toInt();
        ansiBg = regex.cap(2).toInt();
    }

    pT->mColorTriggerFgAnsi = ansiFg;
    pT->mColorTriggerBgAnsi = ansiBg;

    auto pD = new dlgColorTrigger(this, pT, 1 );
    pD->setModal( true );
    pD->setWindowModality( Qt::ApplicationModal );
    pD->exec();
    QPalette palette;
    QColor color = pT->mColorTriggerBgColor;
    palette.setColor( QPalette::Button, color );
    QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");

    row = ((dlgTriggerPatternEdit*)pB->parent())->mRow;
    pI = mTriggerPatternEdit[row];
    if( ! pI ) return;
    pI->lineEdit->setText(QString("FG%1BG%2").arg(QString::number(pT->mColorTriggerFgAnsi), QString::number(pT->mColorTriggerBgAnsi)) );
    pB->setStyleSheet( styleSheet );
}

void dlgTriggerEditor::slot_cursorPositionChanged()
{
    // We only have one block so block count is the same as line count...!
    int _line = mpSourceEditor->textCursor().blockNumber();
    int _maxLines = mpSourceEditor->blockCount();
    int _character = mpSourceEditor->textCursor().position();
    int _characterInLine = mpSourceEditor->textCursor().positionInBlock();
    int _charactersInLine = mpSourceEditor->textCursor().block().length();
    int _maxCharacter = mpSourceEditorDocument->characterCount();

    QString line;
    if( mpSourceEditorDocument->isEmpty() ) {
        line = tr( "Current line: <none>, character in line: <none>, overall: <none>" );
    }
    else {
        line = tr( "Current line: %1/%2, character in line: %3/%4, overall: %5/%6" )
                   .arg( _line + 1 )
                   .arg( _maxLines )
                   .arg( _characterInLine )
                   .arg( _charactersInLine - 1 )
                   .arg( _character )
                   .arg( _maxCharacter - 1 );
    }
    QMainWindow::statusBar()->showMessage( line );
}

void dlgTriggerEditor::slot_changeEditorTextOptions( QTextOption::Flags state )
{

    QTextOption _options = mpSourceEditorDocument->defaultTextOption();
    QTextOption::Flags _flags = _options.flags() & ~( QTextOption::ShowTabsAndSpaces | QTextOption::ShowLineAndParagraphSeparators );
    _flags |= state & ( QTextOption::ShowTabsAndSpaces | QTextOption::ShowLineAndParagraphSeparators );
    _options.setFlags( _flags );
    mpSourceEditorDocument->setDefaultTextOption( _options );
}
