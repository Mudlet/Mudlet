/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn                                     *
 *   KoehnHeiko@googlemail.com                                             *
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
//#include <qsciscintillabase.h>
#include "HostManager.h"
#include "TriggerUnit.h"
#include "TTrigger.h"
#include "TAction.h"
#include <QHeaderView>
//#include <Qsci/qsciscintilla.h>
//#include <Qsci/qscilexerlua.h>
#include <QtGui>
#include <QMainWindow>
#include <QListWidgetItem>
#include "dlgTriggersMainArea.h"
#include "dlgOptionsAreaTriggers.h"
#include "dlgOptionsAreaAction.h"
#include "dlgOptionsAreaTimers.h"
#include "dlgOptionsAreaScripts.h"
#include "dlgOptionsAreaAlias.h"
#include "dlgAliasMainArea.h"
#include "dlgActionMainArea.h"
#include "dlgScriptsMainArea.h"
#include "dlgSearchArea.h"
#include "dlgKeysMainArea.h"
#include "dlgOptionsAreaTimers.h"
#include "TTreeWidget.h"
#include "mudlet.h"
#include "XMLexport.h"
#include "XMLimport.h"
#include "dlgColorTrigger.h"
#include "dlgTriggerPatternEdit.h"
#include "THighlighter.h"
#include "TTextEdit.h"
using namespace std;

const int dlgTriggerEditor::cmTriggerView = 1;
const int dlgTriggerEditor::cmTimerView = 2;
const int dlgTriggerEditor::cmAliasView = 3;
const int dlgTriggerEditor::cmScriptView = 4;
const int dlgTriggerEditor::cmActionView = 5;
const int dlgTriggerEditor::cmKeysView = 6;

const QString msgInfoAddAlias = "Alias are input triggers. To make a new alias: <b>1.</b> Define an input trigger pattern with a Perl regular expression. " \
                                "<b>2.</b> Define a command to send to the MUD in clear text <b><u>instead of the alias pattern</b></u>or write a script for more complicated needs. " \
                                "<b>3.</b> <b><u>Activate</b></u> the alias. "\
                                "Check the manual for <a href='http://mudlet.sourceforge.net/mudlet_documentation.html'>more information</a>";

const QString msgInfoAddTrigger = "To add a new trigger: <b>1.</b> Define a <b><u>pattern</b></u> that you want to trigger on. <b>2.</b> select the appropriate pattern <b><u>type</b></u>."\
                                  "<b>3.</b> Define a clear text command that you want to send to the MUD if the trigger finds the pattern in the text from the MUD or write a script."\
                                  "<b>4. <u>Activate</b></u> the trigger." \
                                  "Check the manual for <a href='http://mudlet.sourceforge.net/mudlet_documentation.html'>more information</a>";

const QString msgInfoAddScript = "Check the manual for <a href='http://mudlet.sourceforge.net/mudlet_documentation.html'>more information</a>";


const QString msgInfoAddTimer = "Check the manual for <a href='http://mudlet.sourceforge.net/mudlet_documentation.html'>more information</a>";


const QString msgInfoAddButton = "To add a new button: <b>1.</b> Add a new group to define a new Button bar in case you don't have any."\
                                 "<b>2.</b> Add new buttons to a button bar." \
                                 "Check the manual for <a href='http://mudlet.sourceforge.net/mudlet_documentation.html'>more information</a>";

const QString msgInfoAddKey = "To add a new key binding <b>1.</b> add a new key <b>2.</b> click on <u><b>grab key</b></u> and then press your key combination. <b><u>NOTE:</b></u> If you want to bind a key combination you must hold down the modifier keys (e.g. control, shift etc.) down before clicking on grab key. " \
                              "<b>3.</b> Define a command that is executed when the key is hit. <b>4. <u>Activate</b></u> the new key binding." \
                              "Check the manual for <a href='http://mudlet.sourceforge.net/mudlet_documentation.html'>more information</a>";

dlgTriggerEditor::dlgTriggerEditor( Host * pH )
: mCurrentAlias( 0 )
, mCurrentTrigger( 0 )
, mCurrentTimer( 0 )
, mCurrentAction( 0 )
, mCurrentScript( 0 )
, mCurrentKey( 0 )
, mpCurrentActionItem( 0 )
, mpCurrentKeyItem( 0 )
, mpCurrentTimerItem( 0 )
, mpCurrentScriptItem( 0 )
, mpCurrentTriggerItem( 0 )
, mpCurrentAliasItem( 0 )
, mpHost( pH )
{
    // init generated dialog
    setupUi(this);
    setUnifiedTitleAndToolBarOnMac( true ); //MAC OSX: make window moveable
    setWindowTitle( mpHost->getName() );
    QIcon winIcon(":/icons/mudlet_editor.png");
    setWindowIcon( winIcon );
    QStatusBar * statusBar = new QStatusBar(this);
    statusBar->setSizeGripEnabled( true );
    setStatusBar( statusBar );
    statusBar->show();
    mIsGrabKey = false;
    QVBoxLayout * pVB1 = new QVBoxLayout(mainArea);

    // system message area
    mpSystemMessageArea = new dlgSystemMessageArea( mainArea );
    QSizePolicy sizePolicy6( QSizePolicy::Expanding, QSizePolicy::Fixed );
    mpSystemMessageArea->setSizePolicy( sizePolicy6 );
    pVB1->addWidget( mpSystemMessageArea );
    connect( mpSystemMessageArea->messageAreaCloseButton, SIGNAL(pressed()), mpSystemMessageArea, SLOT(hide()));

    // main areas

    mpTriggersMainArea = new dlgTriggersMainArea( mainArea );
    pVB1->setContentsMargins(0,0,0,0);
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mpTriggersMainArea->setSizePolicy( sizePolicy );
    pVB1->addWidget( mpTriggersMainArea );
    //mIsTriggerMainAreaEditRegex = false;
    //mpTriggerMainAreaEditRegexItem = 0;
    mpTriggersMainArea->lineEdit_soundFile->hide();
    //connect(mpTriggersMainArea->lineEdit, SIGNAL(returnPressed()), this, SLOT(slot_trigger_main_area_add_regex()));
    //connect(mpTriggersMainArea->listWidget_regex_list, SIGNAL(itemClicked ( QListWidgetItem *)), this, SLOT(slot_trigger_main_area_edit_regex(QListWidgetItem*)));
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
    //QSizePolicy sizePolicy8(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mpActionsMainArea->setSizePolicy( sizePolicy8 );
    //connect( mpActionsMainArea->pushButton_chose_icon, SIGNAL( pressed()), this, SLOT(slot_chose_action_icon()));
    //connect( mpActionsMainArea->pushButton_color, SIGNAL(pressed()), this, SLOT(slot_choseButtonColor()));
    pVB1->addWidget( mpActionsMainArea );


    mpKeysMainArea = new dlgKeysMainArea( mainArea );
    mpKeysMainArea->setSizePolicy( sizePolicy8 );
    pVB1->addWidget( mpKeysMainArea );
    connect(mpKeysMainArea->pushButton_grabKey, SIGNAL(pressed()), this, SLOT(slot_grab_key()));


    mpScriptsMainArea = new dlgScriptsMainArea( mainArea );
    QSizePolicy sizePolicy9(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mpScriptsMainArea->setSizePolicy( sizePolicy9 );
    pVB1->addWidget( mpScriptsMainArea );

    mIsScriptsMainAreaEditHandler = false;
    mpScriptsMainAreaEditHandlerItem = 0;
    connect(mpScriptsMainArea->lineEdit, SIGNAL(returnPressed()), this, SLOT(slot_script_main_area_add_handler()));
    connect(mpScriptsMainArea->listWidget_registered_event_handlers, SIGNAL(itemClicked ( QListWidgetItem *)), this, SLOT(slot_script_main_area_edit_handler(QListWidgetItem*)));

    // source editor area

    mpSourceEditorArea = new dlgSourceEditorArea( mainArea );
    QSizePolicy sizePolicy5(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mpSourceEditorArea->setSizePolicy( sizePolicy5 );
    pVB1->addWidget( mpSourceEditorArea );
    QPlainTextEdit * editp = mpSourceEditorArea->editor;
    editp->setWordWrapMode( QTextOption::NoWrap );
    //QLineEdit * mpCursorPositionIndicator = new QLineEdit;
    //(QMainWindow::statusBar())->addWidget( mpCursorPositionIndicator  );
    connect(editp,SIGNAL(cursorPositionChanged()), this, SLOT(slot_cursorPositionChanged()));
    // option areas

    QHBoxLayout * pHB2 = new QHBoxLayout(popupArea);
    QSizePolicy sizePolicy2(QSizePolicy::Expanding, QSizePolicy::Maximum);
    popupArea->setMinimumSize(200,60);
    //    pHB2->setMaximumSize(170);
    pHB2->setSizeConstraint( QLayout::SetMaximumSize );
    mpErrorConsole = new TConsole(mpHost,false, popupArea);
    mpErrorConsole->setWrapAt(100);
    mpErrorConsole->console->slot_toggleTimeStamps();
    mpErrorConsole->print("*** starting new session ***\n");
    pHB2->setContentsMargins(0,0,0,0);
    pHB2->addWidget( mpErrorConsole );
    mpErrorConsole->show();


    mpOptionsAreaTriggers = new dlgOptionsAreaTriggers( popupArea );
    mpOptionsAreaTriggers->setSizePolicy( sizePolicy2 );
    pHB2->addWidget( mpOptionsAreaTriggers );

    mpOptionsAreaAlias = new dlgOptionsAreaAlias( popupArea );
    mpOptionsAreaAlias->setSizePolicy( sizePolicy2 );
    pHB2->addWidget( mpOptionsAreaAlias );

    mpOptionsAreaActions = new dlgOptionsAreaAction( popupArea );
    mpOptionsAreaActions->setSizePolicy( sizePolicy2 );
    pHB2->addWidget( mpOptionsAreaActions );

    mpOptionsAreaScripts = new dlgOptionsAreaScripts( popupArea );
    mpOptionsAreaScripts->setSizePolicy( sizePolicy2 );
    pHB2->addWidget( mpOptionsAreaScripts );

    mpOptionsAreaTimers = new dlgOptionsAreaTimers( popupArea );
    mpOptionsAreaTimers->setSizePolicy( sizePolicy2 );
    pHB2->addWidget( mpOptionsAreaTimers );



    mpSearchArea = tree_widget_search_results_main;//new dlgSearchArea( popupArea );
    connect( messageAreaCloseButton, SIGNAL(clicked()), this, SLOT( slot_show_search_area()));
    //mpSearchArea->setSizePolicy( sizePolicy2 );
    //pHB2->addWidget( mpSearchArea );



    // additional settings
    treeWidget->setColumnCount(1);
    treeWidget->setIsTriggerTree();
    treeWidget->setRootIsDecorated( false );
    treeWidget->setHost( mpHost );
    treeWidget->header()->hide();
    connect( treeWidget, SIGNAL(itemClicked(QTreeWidgetItem *, int)), this, SLOT(slot_itemClicked(QTreeWidgetItem*,int)) );
    tree_widget_search_results_main->hide(); // hide search results

    mpOptionsAreaTriggers->hide();
    mpOptionsAreaAlias->hide();
    mpOptionsAreaScripts->hide();
    mpOptionsAreaTimers->hide();
    mpOptionsAreaActions->hide();

    treeWidget_alias->hide();
    treeWidget_alias->setHost( mpHost );
    treeWidget_alias->setIsAliasTree();
    treeWidget_alias->setColumnCount(1);
    treeWidget_alias->header()->hide();
    treeWidget_alias->setRootIsDecorated( false );
    connect( treeWidget_alias, SIGNAL(itemClicked(QTreeWidgetItem *, int)), this, SLOT(slot_itemClicked(QTreeWidgetItem*,int)) );

    treeWidget_actions->hide();
    treeWidget_actions->setHost( mpHost );
    treeWidget_actions->setIsActionTree();
    treeWidget_actions->setColumnCount(1);
    treeWidget_actions->header()->hide();
    treeWidget_actions->setRootIsDecorated( false );
    connect( treeWidget_actions, SIGNAL(itemClicked(QTreeWidgetItem *, int)), this, SLOT(slot_itemClicked(QTreeWidgetItem*,int)) );

    treeWidget_timers->hide();
    treeWidget_timers->setHost( mpHost );
    treeWidget_timers->setIsTimerTree();
    treeWidget_timers->setColumnCount(1);
    treeWidget_timers->header()->hide();
    treeWidget_timers->setRootIsDecorated( false );
    connect( treeWidget_timers, SIGNAL(itemClicked(QTreeWidgetItem *, int)), this, SLOT(slot_itemClicked(QTreeWidgetItem*,int)) );

    treeWidget_keys->hide();
    treeWidget_keys->setHost( mpHost );
    treeWidget_keys->setIsKeyTree();
    treeWidget_keys->setColumnCount(1);
    treeWidget_keys->header()->hide();
    treeWidget_keys->setRootIsDecorated( false );
    connect( treeWidget_keys, SIGNAL(itemClicked(QTreeWidgetItem *, int)), this, SLOT(slot_itemClicked(QTreeWidgetItem*,int)) );

    treeWidget_scripts->hide();
    treeWidget_scripts->setHost( mpHost );
    treeWidget_scripts->setIsScriptTree();
    treeWidget_scripts->setColumnCount(1);
    treeWidget_scripts->header()->hide();
    treeWidget_scripts->setRootIsDecorated( false );
    connect( treeWidget_scripts, SIGNAL(itemClicked(QTreeWidgetItem *, int)), this, SLOT(slot_itemClicked(QTreeWidgetItem*,int)) );

    QAction * viewTriggerAction = new QAction(QIcon(":/icons/tools-wizard.png"), tr("Triggers"), this);
    viewTriggerAction->setStatusTip(tr("Show Triggers"));
    connect(viewTriggerAction, SIGNAL(triggered()), this, SLOT(slot_show_triggers() ));

    QAction * viewActionAction = new QAction(QIcon(":/icons/bookmarks.png"), tr("Buttons"), this);
    viewActionAction->setStatusTip(tr("Show Buttons"));
    connect(viewActionAction, SIGNAL(triggered()), this, SLOT(slot_show_actions() ));


    QAction * viewAliasAction = new QAction(QIcon(":/icons/system-users.png"), tr("Aliases"), this);
    viewAliasAction->setStatusTip(tr("Show Aliases"));
    viewAliasAction->setEnabled( true );
    connect( viewAliasAction, SIGNAL(triggered()), this, SLOT( slot_show_aliases()));


    QAction * showTimersAction = new QAction(QIcon(":/icons/chronometer.png"), tr("Timers"), this);
    showTimersAction->setStatusTip(tr("Show Timers"));
    connect( showTimersAction, SIGNAL(triggered()), this, SLOT( slot_show_timers()));


    QAction * viewScriptsAction = new QAction(QIcon(":/icons/document-properties.png"), tr("Scripts"), this);
    viewScriptsAction->setEnabled( true );
    viewScriptsAction->setStatusTip(tr("Show Scripts"));
    connect( viewScriptsAction, SIGNAL(triggered()), this, SLOT( slot_show_scripts()));

    QAction * viewKeysAction = new QAction(QIcon(":/icons/preferences-desktop-keyboard.png"), tr("Keys"), this);
    viewKeysAction->setStatusTip(tr("Keybindings"));
    viewKeysAction->setEnabled( true );
    connect( viewKeysAction, SIGNAL(triggered()), this, SLOT( slot_show_keys()));

    QAction * toggleActiveAction = new QAction(QIcon(":/icons/document-encrypt.png"), tr("Activate"), this);
    toggleActiveAction->setStatusTip(tr("Toggle Active or Non-Active Mode for Triggers, Scripts etc."));
    connect( toggleActiveAction, SIGNAL(triggered()), this, SLOT( slot_toggle_active()));
    connect( treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), SLOT( slot_toggle_active()));
    connect( treeWidget_alias, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), SLOT( slot_toggle_active()));
    connect( treeWidget_timers, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), SLOT( slot_toggle_active()));
    connect( treeWidget_scripts, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), SLOT( slot_toggle_active()));
    connect( treeWidget_actions, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), SLOT( slot_toggle_active()));
    connect( treeWidget_keys, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), SLOT( slot_toggle_active()));


    QAction * addTriggerAction = new QAction(QIcon(":/icons/document-new.png"), tr("Add Item"), this);
    addTriggerAction->setStatusTip(tr("Add new Trigger, Script, Alias or Filter"));
    connect( addTriggerAction, SIGNAL(triggered()), this, SLOT( slot_add_new()));

    QAction * deleteTriggerAction = new QAction(QIcon(":/icons/edit-delete-shred.png"), tr("Delete Item"), this);
    deleteTriggerAction->setStatusTip(tr("Delete Trigger, Script, Alias or Filter"));
    connect( deleteTriggerAction, SIGNAL(triggered()), this, SLOT( slot_delete_item()));

    QAction * addFolderAction = new QAction(QIcon(":/icons/folder-new.png"), tr("Add Group"), this);
    addFolderAction->setStatusTip(tr("Add new Group"));
    connect( addFolderAction, SIGNAL(triggered()), this, SLOT( slot_add_new_folder()));

    QAction * showSearchAreaAction = new QAction(QIcon(":/icons/edit-find-user.png"), tr("Search"), this);
    //showSearchAreaAction->setShortcut(tr("Ctrl+F"));
    showSearchAreaAction->setStatusTip(tr("Show Search Results List"));
    connect( showSearchAreaAction, SIGNAL(triggered()), this, SLOT( slot_show_search_area()));

    QAction * saveAction = new QAction(QIcon(":/icons/document-save-as.png"), tr("Save Item"), this);
    //saveAction->setShortcut(tr("Ctrl+S"));
    saveAction->setStatusTip(tr("Save Edited Trigger, Script, Alias etc. If information has been edited, it must be saved or the changes will be lost."));
    connect( saveAction, SIGNAL(triggered()), this, SLOT( slot_save_edit() ));

    QAction * importAction = new QAction(QIcon(":/icons/import.png"), tr("Import"), this);
    importAction->setEnabled( true );
    connect( importAction, SIGNAL(triggered()), this, SLOT( slot_import()));

    QAction * exportAction = new QAction(QIcon(":/icons/export.png"), tr("Export"), this);
    exportAction->setEnabled( true );
    connect( exportAction, SIGNAL(triggered()), this, SLOT( slot_export()));

    QAction * saveMenu = new QAction(QIcon(":/icons/document-save-all.png"), tr("Save Profile"), this);
    saveMenu->setEnabled( true );
    connect( saveMenu, SIGNAL(triggered()), this, SLOT( slot_profileSaveAction()));

    QAction * profileSaveAction = new QAction(QIcon(":/icons/document-save-all.png"), tr("Save Profile"), this);
    profileSaveAction->setEnabled( true );
    connect( profileSaveAction, SIGNAL(triggered()), this, SLOT( slot_profileSaveAction()));

    QAction * saveProfileAsAction = new QAction(QIcon(":/icons/utilities-file-archiver.png"), tr("Save Profile As"), this);
    saveProfileAsAction->setEnabled( true );
    connect( saveProfileAsAction, SIGNAL(triggered()), this, SLOT( slot_profileSaveAsAction()));

    QAction * viewStatsAction = new QAction(QIcon(":/icons/view-statistics.png"), tr("Statistics"), this);
    viewStatsAction->setEnabled( true );
    connect( viewStatsAction, SIGNAL(triggered()), this, SLOT( slot_viewStatsAction()));

    QAction * viewErrorsAction = new QAction(QIcon(":/icons/errors.png"), tr("errors"), this);
    viewErrorsAction->setEnabled( true );
    connect( viewErrorsAction, SIGNAL(triggered()), this, SLOT( slot_viewErrorsAction()));


    //Action * saveProfileMenu = new QMenu( this );
    //saveProfileMenu->addAction( profileSaveAction );
    //saveProfileMenu->addAction( saveProfileAsAction );
    //saveMenu->setMenu( saveProfileMenu );

    /*QAction * actionProfileBackup = new QAction(QIcon(":/icons/utilities-file-archiver.png"), tr("Backup Profile"), this);
    actionProfileBackup->setStatusTip(tr("Backup Profile"));*/


    QAction * showDebugAreaAction = new QAction(QIcon(":/icons/tools-report-bug.png"), tr("Debug"), this);
    showDebugAreaAction->setEnabled( true );
    showDebugAreaAction->setToolTip(tr("Activates Debug Messages -> system will be *MUCH* slower"));
    connect( showDebugAreaAction, SIGNAL(triggered()), this, SLOT( slot_debug_mode() ));

    QAction * addTriggerMenuAction = new QAction(QIcon(":/icons/tools-wizard.png"), tr("Triggers"), this);
    viewTriggerAction->setStatusTip(tr("Add Trigger"));
    connect(addTriggerMenuAction, SIGNAL(triggered()), this, SLOT(slot_addTrigger()));

    QAction * addAliasMenuAction = new QAction(QIcon(":/icons/system-users.png"), tr("Aliases"), this);
    addAliasMenuAction->setStatusTip(tr("Add Alias"));
    addAliasMenuAction->setEnabled( true );
    connect( addAliasMenuAction, SIGNAL(triggered()), this, SLOT( slot_addAlias()));

    QAction * addTimersMenuAction = new QAction(QIcon(":/icons/chronometer.png"), tr("Timers"), this);
    addTimersMenuAction->setStatusTip(tr("Add Timer"));
    addTimersMenuAction->setEnabled( true );
    connect( addTimersMenuAction, SIGNAL(triggered()), this, SLOT( slot_addTimer()));

    QAction * addScriptsMenuAction = new QAction(QIcon(":/icons/document-properties.png"), tr("Scripts"), this);
    addScriptsMenuAction->setStatusTip(tr("Add Script"));
    addScriptsMenuAction->setEnabled( true );
    connect( addScriptsMenuAction, SIGNAL(triggered()), this, SLOT( slot_addScript()));

    QAction * addKeysMenuAction = new QAction(QIcon(":/icons/preferences-desktop-keyboard.png"), tr("Keys"), this);
    addKeysMenuAction->setStatusTip(tr("Add Keys"));
    addKeysMenuAction->setEnabled( true );
    connect( addKeysMenuAction, SIGNAL(triggered()), this, SLOT( slot_addKey()));

    QMenu * addTriggerMenu = new QMenu( this );
    addTriggerMenu->addAction( addTriggerMenuAction );
    addTriggerMenu->addAction( addTimersMenuAction );
    addTriggerMenu->addAction( addScriptsMenuAction );
    addTriggerMenu->addAction( addAliasMenuAction );
    addTriggerMenu->addAction( addKeysMenuAction );

    //addTriggerAction->setMenu( addTriggerMenu );

    QAction * addTriggerGroupMenuAction = new QAction(QIcon(":/icons/tools-wizard.png"), tr("Triggers"), this);
    addTriggerGroupMenuAction->setStatusTip(tr("Add Trigger Group"));
    connect(addTriggerGroupMenuAction, SIGNAL(triggered()), this, SLOT(slot_addTriggerGroup()));

    QAction * addAliasGroupMenuAction = new QAction(QIcon(":/icons/system-users.png"), tr("Aliases"), this);
    addAliasGroupMenuAction->setStatusTip(tr("Add Alias Group"));
    addAliasGroupMenuAction->setEnabled( true );
    connect( addAliasGroupMenuAction, SIGNAL(triggered()), this, SLOT( slot_addAliasGroup()));

    QAction * addTimersGroupMenuAction = new QAction(QIcon(":/icons/chronometer.png"), tr("Timers"), this);
    addTimersGroupMenuAction->setStatusTip(tr("Add Timer Group"));
    addTimersGroupMenuAction->setEnabled( true );
    connect( addTimersGroupMenuAction, SIGNAL(triggered()), this, SLOT( slot_addTimerGroup()));

    QAction * addScriptsGroupMenuAction = new QAction(QIcon(":/icons/document-properties.png"), tr("Scripts"), this);
    addScriptsGroupMenuAction->setStatusTip(tr("Add Script Group"));
    addScriptsGroupMenuAction->setEnabled( true );
    connect( addScriptsGroupMenuAction, SIGNAL(triggered()), this, SLOT( slot_addScriptGroup()));

    //QAction * addFiltersGroupMenuAction = new QAction(QIcon(":/icons/view-filter.png"), tr("Filters"), this);
    //addFiltersGroupMenuAction->setStatusTip(tr("Add Filter Group"));
    //addFiltersGroupMenuAction->setEnabled( false );
    //connect( viewFiltersAction, SIGNAL(triggered()), this, SLOT( showFiltersView()));

    QMenu * addTriggerGroupMenu = new QMenu( this );
    addTriggerGroupMenu->addAction( addTriggerGroupMenuAction );
    addTriggerGroupMenu->addAction( addTimersGroupMenuAction );
    addTriggerGroupMenu->addAction( addScriptsGroupMenuAction );
    addTriggerGroupMenu->addAction( addAliasGroupMenuAction );
    //addTriggerGroupMenu->addAction( addFiltersGroupMenuAction );

    //addFolderAction->setMenu( addTriggerGroupMenu );

    //toolBar->addSeparator();

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
    toolBar2->addAction( viewTriggerAction );
    toolBar2->addAction( viewAliasAction );
    toolBar2->addAction( viewScriptsAction );
    toolBar2->addAction( showTimersAction );
    toolBar2->addAction( viewKeysAction );
    toolBar2->addAction( viewActionAction );
    toolBar2->addAction( showSearchAreaAction );
    toolBar2->addAction( viewErrorsAction );
    toolBar2->addAction( viewStatsAction );
    toolBar2->addAction( showDebugAreaAction );

    toolBar2->setMovable( true );

    toolBar2->setOrientation(Qt::Vertical);

    QMainWindow::addToolBar(Qt::LeftToolBarArea, toolBar2 );
    QMainWindow::addToolBar(Qt::TopToolBarArea, toolBar );

    mpSourceEditorArea->editor->setFont( mpHost->mDisplayFont );
    /*mpSourceEditorArea->script_scintilla->setUtf8( true );
    setFont( QFont("Bitstream Vera Sans Mono", 10, QFont::Courier ) );
    mpSourceEditorArea->script_scintilla->setFont( QFont("Bitstream Vera Sans Mono", 10, QFont::Courier ) );
    mpLuaLexer = new QsciLexerLua(this);
    mpLuaLexer->setFont( QFont("Bitstream Vera Sans Mono", 10, QFont::Courier ) );
    mpSourceEditorArea->script_scintilla->setLexer( mpLuaLexer );
    mpSourceEditorArea->script_scintilla->setAutoCompletionThreshold( 3 );
    if( mpHost->mNoAntiAlias ) mpSourceEditorArea->script_scintilla->setAutoCompletionSource( QsciScintilla::AcsAll );
    mpSourceEditorArea->script_scintilla->setMarginLineNumbers( 1, true );
    mpSourceEditorArea->script_scintilla->setMarginWidth( 1, 40 );
    mpSourceEditorArea->script_scintilla->setMarginWidth( 2, 20 );
    mpSourceEditorArea->script_scintilla->setMarginLineNumbers( 2, false );*/

    connect( comboBox_search_triggers, SIGNAL( activated( const QString )), this, SLOT(slot_search_triggers( const QString ) ) );
    connect( this, SIGNAL( update() ), this, SLOT( slot_update() ) );
    connect( treeWidget, SIGNAL( itemClicked( QTreeWidgetItem *, int ) ), this, SLOT( slot_trigger_clicked( QTreeWidgetItem *, int) ) );
    connect( treeWidget_keys, SIGNAL( itemClicked( QTreeWidgetItem *, int ) ), this, SLOT( slot_key_clicked( QTreeWidgetItem *, int) ) );
    connect( treeWidget_timers, SIGNAL( itemClicked( QTreeWidgetItem *, int ) ), this, SLOT( slot_timer_clicked( QTreeWidgetItem *, int) ) );
    connect( treeWidget_scripts, SIGNAL( itemClicked( QTreeWidgetItem *, int ) ), this, SLOT( slot_scripts_clicked( QTreeWidgetItem *, int) ) );
    connect( treeWidget_alias, SIGNAL( itemClicked( QTreeWidgetItem *, int ) ), this, SLOT( slot_alias_clicked( QTreeWidgetItem *, int) ) );
    connect( treeWidget_actions, SIGNAL( itemClicked( QTreeWidgetItem *, int ) ), this, SLOT( slot_action_clicked( QTreeWidgetItem *, int) ) );
    connect( this, SIGNAL (accept()), this, SLOT (slot_connection_dlg_finnished()));
    //connect( mpSearchArea, SIGNAL(itemClicked(QTreeWidgetItem*, int)), this, SLOT( slot_item_clicked_search_list(QTreeWidgetItem*, int)));
    connect( tree_widget_search_results_main, SIGNAL(itemClicked(QTreeWidgetItem*, int)), this, SLOT( slot_item_clicked_search_list(QTreeWidgetItem*, int)));
    //connect( mpTriggersMainArea->toolButton_add, SIGNAL(pressed()), this, SLOT(slot_trigger_main_area_add_regex()));
    //connect( mpTriggersMainArea->toolButton_update, SIGNAL(pressed()), this, SLOT(slot_trigger_main_area_add_regex()));
    //connect( mpTriggersMainArea->toolButton_remove, SIGNAL(pressed()), this, SLOT( slot_trigger_main_area_delete_regex()));
    connect( mpScriptsMainArea->toolButton_add, SIGNAL(pressed()), this, SLOT(slot_script_main_area_add_handler()));
    connect( mpScriptsMainArea->toolButton_remove, SIGNAL(pressed()), this, SLOT( slot_script_main_area_delete_handler()));

    mpTriggersMainArea->hide();
    mpTimersMainArea->hide();
    mpScriptsMainArea->hide();
    mpAliasMainArea->hide();
    mpActionsMainArea->hide();
    mpKeysMainArea->hide();

    mpSourceEditorArea->hide();

    mpSystemMessageArea->hide();
    mpOptionsAreaTriggers->hide();
    mpOptionsAreaAlias->hide();
    mpOptionsAreaActions->hide();
    mpOptionsAreaScripts->hide();
    mpOptionsAreaTimers->hide();

    treeWidget->show();
    treeWidget_alias->hide();
    treeWidget_actions->hide();
    treeWidget_timers->hide();
    treeWidget_scripts->hide();
    treeWidget_keys->hide();

    popupArea->hide();
    frame_4->hide();
    mpSearchArea->hide();

    readSettings();
    setTBIconSize( 0 );
    //connect( mpTriggersMainArea->listWidget_regex_list, SIGNAL(cellChanged(int,int)), this, SLOT(slot_set_pattern_type_color(int,int)));

    tree_widget_search_results_main->setColumnCount( 4 );
    QStringList labelList;
    labelList << "Type" << "Name" << "Where" << "What";
    tree_widget_search_results_main->setHeaderLabels( labelList );
    mpScrollArea = mpTriggersMainArea->scrollArea;
    HpatternList = new QWidget;
    QVBoxLayout * lay1 = new QVBoxLayout( HpatternList );
    lay1->setContentsMargins(0,0,0,0);
    lay1->setSpacing(0);
    mpScrollArea->setWidget( HpatternList );
    for( int i=0; i<50; i++)
    {
        dlgTriggerPatternEdit * pItem = new dlgTriggerPatternEdit(HpatternList);
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
    if( frame_4->isHidden() )frame_4->show();
    else
        frame_4->hide();
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
    treeWidget->setIconSize(QSize(mudlet::self()->mTEFolderIconSize*8,mudlet::self()->mTEFolderIconSize*8));
    treeWidget_alias->setIconSize(QSize(mudlet::self()->mTEFolderIconSize*8,mudlet::self()->mTEFolderIconSize*8));
    treeWidget_timers->setIconSize(QSize(mudlet::self()->mTEFolderIconSize*8,mudlet::self()->mTEFolderIconSize*8));
    treeWidget_scripts->setIconSize(QSize(mudlet::self()->mTEFolderIconSize*8,mudlet::self()->mTEFolderIconSize*8));
    treeWidget_keys->setIconSize(QSize(mudlet::self()->mTEFolderIconSize*8,mudlet::self()->mTEFolderIconSize*8));
    treeWidget_actions->setIconSize(QSize(mudlet::self()->mTEFolderIconSize*8,mudlet::self()->mTEFolderIconSize*8));
}

void dlgTriggerEditor::slot_choseButtonColor()
{
     QColor color = QColorDialog::getColor();
     QPalette palette;
     palette.setColor( QPalette::Button, color );
     //mpActionsMainArea->pushButton_color->setPalette( palette );
}

void dlgTriggerEditor::closeEvent(QCloseEvent *event)
{
    writeSettings();
    event->accept();
}


void dlgTriggerEditor::readSettings()
{
    QSettings settings("Mudlet", "Mudlet 1.0");
    QPoint pos = settings.value("script_editor_pos", QPoint(10, 10)).toPoint();
    QSize size = settings.value("script_editor_size", QSize(600, 400)).toSize();
    resize( size );
    move( pos );
}

void dlgTriggerEditor::writeSettings()
{
    QSettings settings("Mudlet", "Mudlet 1.0");
    settings.setValue("script_editor_pos", pos());
    settings.setValue("script_editor_size", size());
}


void dlgTriggerEditor::slot_switchToExpertMonde()
{
    //    toolButton_add_trigger->show();
    //toolButton_search_area->show();
    //toolButton_add_trigger_group->show();
    //toolButton_options_area->show();
    //toolButton_toggle_active_trigger->show();
    //toolButton_delete_trigger->show();
}

void dlgTriggerEditor::slot_item_clicked_search_list(QTreeWidgetItem* pItem, int mode )
{
    qDebug()<<"dlgTriggerEditor::slot_item_clicked_search_list item="<<pItem->text(0);
    if( pItem->text(0) == QString("Trigger") )
    {
        QList<QTreeWidgetItem *> foundItemsList = treeWidget->findItems( pItem->text(1), Qt::MatchFixedString | Qt::MatchCaseSensitive | Qt::MatchRecursive, 0);

        for( int i=0; i<foundItemsList.size(); i++ )
        {
            QTreeWidgetItem * pI = foundItemsList[i];
            int idTree = pI->data(0, Qt::UserRole).toInt();
            int idSearch = pItem->data(0, Qt::UserRole).toInt();
            if( idTree == idSearch )
            {
                treeWidget->setCurrentItem( pI, 0 );
                treeWidget->scrollToItem( pI );
                slot_show_triggers();
                slot_trigger_clicked( pI, 0 );
                return;
            }
        }
    }
    if( pItem->text(0) == QString("Alias") )
    {
        QList<QTreeWidgetItem *> foundItemsList = treeWidget_alias->findItems( pItem->text(1), Qt::MatchFixedString | Qt::MatchCaseSensitive | Qt::MatchRecursive, 0);

        for( int i=0; i<foundItemsList.size(); i++ )
        {
            QTreeWidgetItem * pI = foundItemsList[i];
            int idTree = pI->data(0, Qt::UserRole).toInt();
            int idSearch = pItem->data(0, Qt::UserRole).toInt();
            if( idTree == idSearch )
            {
                treeWidget_alias->setCurrentItem( pI, 0 );
                treeWidget_alias->scrollToItem( pI );
                slot_show_aliases();
                slot_alias_clicked( pI, 0 );
                return;
            }
        }
    }
    if( pItem->text(0) == QString("Script") )
    {
        QList<QTreeWidgetItem *> foundItemsList = treeWidget_scripts->findItems( pItem->text(1), Qt::MatchFixedString | Qt::MatchCaseSensitive | Qt::MatchRecursive, 0);

        for( int i=0; i<foundItemsList.size(); i++ )
        {
            QTreeWidgetItem * pI = foundItemsList[i];
            int idTree = pI->data(0, Qt::UserRole).toInt();
            int idSearch = pItem->data(0, Qt::UserRole).toInt();
            if( idTree == idSearch )
            {
                treeWidget_scripts->setCurrentItem( pI, 0 );
                treeWidget_scripts->scrollToItem( pI );
                slot_show_scripts();
                slot_scripts_clicked( pI, 0 );
                return;
            }
        }
        return;
    }

    if( pItem->text(0) == QString("Button") )
    {
        QList<QTreeWidgetItem *> foundItemsList = treeWidget_actions->findItems( pItem->text(1), Qt::MatchFixedString | Qt::MatchCaseSensitive | Qt::MatchRecursive, 0);

        for( int i=0; i<foundItemsList.size(); i++ )
        {
            QTreeWidgetItem * pI = foundItemsList[i];
            int idTree = pI->data(0, Qt::UserRole).toInt();
            int idSearch = pItem->data(0, Qt::UserRole).toInt();
            if( idTree == idSearch )
            {
                treeWidget_actions->setCurrentItem( pI, 0 );
                treeWidget_actions->scrollToItem( pI );
                slot_show_actions();
                slot_action_clicked( pI, 0 );
                return;
            }
        }
        return;
    }

    if( pItem->text(0) == QString("Timer") )
    {
        QList<QTreeWidgetItem *> foundItemsList = treeWidget_timers->findItems( pItem->text(1), Qt::MatchFixedString | Qt::MatchCaseSensitive | Qt::MatchRecursive, 0);

        for( int i=0; i<foundItemsList.size(); i++ )
        {
            QTreeWidgetItem * pI = foundItemsList[i];
            int idTree = pI->data(0, Qt::UserRole).toInt();
            int idSearch = pItem->data(0, Qt::UserRole).toInt();
            if( idTree == idSearch )
            {
                treeWidget_timers->setCurrentItem( pI, 0 );
                treeWidget_timers->scrollToItem( pI );
                slot_show_timers();
                slot_timer_clicked( pI, 0 );
                return;
            }
        }
        return;
    }

    if( pItem->text(0) == QString("Key") )
    {
        QList<QTreeWidgetItem *> foundItemsList = treeWidget_keys->findItems( pItem->text(1), Qt::MatchFixedString | Qt::MatchCaseSensitive | Qt::MatchRecursive, 0);

        for( int i=0; i<foundItemsList.size(); i++ )
        {
            QTreeWidgetItem * pI = foundItemsList[i];
            int idTree = pI->data(0, Qt::UserRole).toInt();
            int idSearch = pItem->data(0, Qt::UserRole).toInt();
            if( idTree == idSearch )
            {
                treeWidget_keys->setCurrentItem( pI, 0 );
                treeWidget_keys->scrollToItem( pI );
                slot_show_keys();
                slot_key_clicked( pI, 0 );
                return;
            }
        }
        return;
    }
}

void dlgTriggerEditor::slot_search_triggers( const QString s )
{
    QRegExp pattern = QRegExp( s );

    //mpSourceEditorArea->highlighter->rehighlight();
    tree_widget_search_results_main->clear();
    tree_widget_search_results_main->show();

    // type   | name | line number/pattern/name what has been found
    //-------------------------------------------------------------------------
    //trigger | name | line 204: a la sd  akd
    //trigger | asdf | pattern: ^alsd
    //trigger | asdf | name     |
    if( true )
    {
        std::list<TTrigger *> nodes = mpHost->getTriggerUnit()->getTriggerRootNodeList();
        typedef list<TTrigger *>::const_iterator I;
        for( I it = nodes.begin(); it != nodes.end(); it++)
        {
            QTreeWidgetItem * pItem;
            QTreeWidgetItem * parent = 0;
            TTrigger * pChild = *it;
            QString n = pChild->getName();
            if( n.indexOf( s, 0, Qt::CaseInsensitive ) != -1 )
            {
                QStringList sl;
                sl << "Trigger" << pChild->getName() << "name";
                if( ! parent )
                {
                    parent = new QTreeWidgetItem( sl );
                    parent->setFirstColumnSpanned( false );
                    parent->setData(0, Qt::UserRole, pChild->getID() );
                    tree_widget_search_results_main->addTopLevelItem( parent );
                }
                else
                {
                    pItem = new QTreeWidgetItem( parent, sl );
                    pItem->setFirstColumnSpanned( false );
                    pItem->setData(0, Qt::UserRole, pChild->getID() );
                    parent->addChild( pItem );
                    parent->setExpanded( true );
                }
            }
            QStringList scriptList = pChild->getScript().split("\n");
            QStringList resultList = scriptList.filter( s, Qt::CaseInsensitive );
            for( int i=0; i<resultList.size(); i++ )
            {
                QStringList sl;
                sl << "Trigger" << pChild->getName() << "script" << resultList[i];
                if( ! parent )
                {
                    parent = new QTreeWidgetItem( sl );
                    parent->setFirstColumnSpanned( false );
                    parent->setData(0, Qt::UserRole, pChild->getID() );
                    tree_widget_search_results_main->addTopLevelItem( parent );
                }
                else
                {
                    pItem = new QTreeWidgetItem( parent, sl );
                    pItem->setFirstColumnSpanned( false );
                    pItem->setData(0, Qt::UserRole, pChild->getID() );
                    parent->addChild( pItem );
                    parent->setExpanded( true );
                }
            }
            QStringList regexList = pChild->getRegexCodeList();
            resultList = regexList.filter( s, Qt::CaseInsensitive );
            for( int i=0; i<resultList.size(); i++ )
            {
                QStringList sl;
                sl << "Trigger" << pChild->getName() << "pattern" << resultList[i];
                if( ! parent )
                {
                    parent = new QTreeWidgetItem( sl );
                    parent->setFirstColumnSpanned( false );
                    parent->setData(0, Qt::UserRole, pChild->getID() );
                    tree_widget_search_results_main->addTopLevelItem( parent );
                }
                else
                {
                    pItem = new QTreeWidgetItem( parent, sl );
                    pItem->setFirstColumnSpanned( false );
                    pItem->setData(0, Qt::UserRole, pChild->getID() );
                    parent->addChild( pItem );
                    parent->setExpanded( true );
                }
            }
            recursiveSearchTriggers( pChild, s );
        }
    }

    if( true )
    {
        std::list<TAlias *> nodes = mpHost->getAliasUnit()->getAliasRootNodeList();
        typedef list<TAlias *>::const_iterator I;
        for( I it = nodes.begin(); it != nodes.end(); it++)
        {
            QTreeWidgetItem * pItem;
            QTreeWidgetItem * parent = 0;
            TAlias * pChild = *it;
            QString n = pChild->getName();
            if( n.indexOf( s, 0, Qt::CaseInsensitive ) != -1 )
            {
                QStringList sl;
                sl << "Alias" << pChild->getName() << "name";
                if( ! parent )
                {
                    parent = new QTreeWidgetItem( sl );
                    parent->setFirstColumnSpanned( false );
                    parent->setData(0, Qt::UserRole, pChild->getID() );
                    tree_widget_search_results_main->addTopLevelItem( parent );
                }
                else
                {
                    pItem = new QTreeWidgetItem( parent, sl );
                    pItem->setFirstColumnSpanned( false );
                    pItem->setData(0, Qt::UserRole, pChild->getID() );
                    parent->addChild( pItem );
                    parent->setExpanded( true );
                }
            }
            QStringList scriptList = pChild->getScript().split("\n");
            QStringList resultList = scriptList.filter( s, Qt::CaseInsensitive );
            for( int i=0; i<resultList.size(); i++ )
            {
                QStringList sl;
                sl << "Alias" << pChild->getName() << "script" << resultList[i];
                if( ! parent )
                {
                    parent = new QTreeWidgetItem( sl );
                    parent->setFirstColumnSpanned( false );
                    parent->setData(0, Qt::UserRole, pChild->getID() );
                    tree_widget_search_results_main->addTopLevelItem( parent );
                }
                else
                {
                    pItem = new QTreeWidgetItem( parent, sl );
                    pItem->setFirstColumnSpanned( false );
                    pItem->setData(0, Qt::UserRole, pChild->getID() );
                    parent->addChild( pItem );
                    parent->setExpanded( true );
                }
            }

            if( pChild->getRegexCode().indexOf( s, 0, Qt::CaseInsensitive ) > -1 )
            {
                QStringList sl;
                sl << "Alias" << pChild->getName() << "pattern" << pChild->getRegexCode();
                if( ! parent )
                {
                    parent = new QTreeWidgetItem( sl );
                    parent->setFirstColumnSpanned( false );
                    parent->setData(0, Qt::UserRole, pChild->getID() );
                    tree_widget_search_results_main->addTopLevelItem( parent );
                }
                else
                {
                    pItem = new QTreeWidgetItem( parent, sl );
                    pItem->setFirstColumnSpanned( false );
                    pItem->setData(0, Qt::UserRole, pChild->getID() );
                    parent->addChild( pItem );
                    parent->setExpanded( true );
                }
            }
            recursiveSearchAlias( pChild, s );
        }
    }

    if( true )
    {
        std::list<TScript *> nodes = mpHost->getScriptUnit()->getScriptRootNodeList();
        typedef list<TScript *>::const_iterator I;
        for( I it = nodes.begin(); it != nodes.end(); it++)
        {
            QTreeWidgetItem * pItem;
            QTreeWidgetItem * parent = 0;
            TScript * pChild = *it;
            QString n = pChild->getName();
            if( n.indexOf( s, 0, Qt::CaseInsensitive ) != -1 )
            {
                QStringList sl;
                sl << "Script" << pChild->getName() << "name";
                if( ! parent )
                {
                    parent = new QTreeWidgetItem( sl );
                    parent->setFirstColumnSpanned( false );
                    parent->setData(0, Qt::UserRole, pChild->getID() );
                    tree_widget_search_results_main->addTopLevelItem( parent );
                }
                else
                {
                    pItem = new QTreeWidgetItem( parent, sl );
                    pItem->setFirstColumnSpanned( false );
                    pItem->setData(0, Qt::UserRole, pChild->getID() );
                    parent->addChild( pItem );
                    parent->setExpanded( true );
                }
            }
            QStringList scriptList = pChild->getScript().split("\n");
            QStringList resultList = scriptList.filter( s, Qt::CaseInsensitive );
            for( int i=0; i<resultList.size(); i++ )
            {
                QStringList sl;
                sl << "Script" << pChild->getName() << "script" << resultList[i];
                if( ! parent )
                {
                    parent = new QTreeWidgetItem( sl );
                    parent->setFirstColumnSpanned( false );
                    parent->setData(0, Qt::UserRole, pChild->getID() );
                    tree_widget_search_results_main->addTopLevelItem( parent );
                }
                else
                {
                    pItem = new QTreeWidgetItem( parent, sl );
                    pItem->setFirstColumnSpanned( false );
                    pItem->setData(0, Qt::UserRole, pChild->getID() );
                    parent->addChild( pItem );
                    parent->setExpanded( true );
                }
            }

            recursiveSearchScripts( pChild, s );
        }
    }

    if( true )
    {
        std::list<TAction *> nodes = mpHost->getActionUnit()->getActionRootNodeList();
        typedef list<TAction *>::const_iterator I;
        for( I it = nodes.begin(); it != nodes.end(); it++)
        {
            QTreeWidgetItem * pItem;
            QTreeWidgetItem * parent = 0;
            TAction * pChild = *it;
            QString n = pChild->getName();
            if( n.indexOf( s, 0, Qt::CaseInsensitive ) != -1 )
            {
                QStringList sl;
                sl << "Button" << pChild->getName() << "name";
                if( ! parent )
                {
                    parent = new QTreeWidgetItem( sl );
                    parent->setFirstColumnSpanned( false );
                    parent->setData(0, Qt::UserRole, pChild->getID() );
                    tree_widget_search_results_main->addTopLevelItem( parent );
                }
                else
                {
                    pItem = new QTreeWidgetItem( parent, sl );
                    pItem->setFirstColumnSpanned( false );
                    pItem->setData(0, Qt::UserRole, pChild->getID() );
                    parent->addChild( pItem );
                    parent->setExpanded( true );
                }
            }
            QStringList scriptList = pChild->getScript().split("\n");
            QStringList resultList = scriptList.filter( s, Qt::CaseInsensitive );
            for( int i=0; i<resultList.size(); i++ )
            {
                QStringList sl;
                sl << "Button" << pChild->getName() << "script" << resultList[i];
                if( ! parent )
                {
                    parent = new QTreeWidgetItem( sl );
                    parent->setFirstColumnSpanned( false );
                    parent->setData(0, Qt::UserRole, pChild->getID() );
                    tree_widget_search_results_main->addTopLevelItem( parent );
                }
                else
                {
                    pItem = new QTreeWidgetItem( parent, sl );
                    pItem->setFirstColumnSpanned( false );
                    pItem->setData(0, Qt::UserRole, pChild->getID() );
                    parent->addChild( pItem );
                    parent->setExpanded( true );
                }
            }

            recursiveSearchActions( pChild, s );
        }
    }

    if( true )
    {
        std::list<TTimer *> nodes = mpHost->getTimerUnit()->getTimerRootNodeList();
        typedef list<TTimer *>::const_iterator I;
        for( I it = nodes.begin(); it != nodes.end(); it++)
        {
            QTreeWidgetItem * pItem;
            QTreeWidgetItem * parent = 0;
            TTimer * pChild = *it;
            QString n = pChild->getName();
            if( n.indexOf( s, 0, Qt::CaseInsensitive ) != -1 )
            {
                QStringList sl;
                sl << "Timer" << pChild->getName() << "name";
                if( ! parent )
                {
                    parent = new QTreeWidgetItem( sl );
                    parent->setFirstColumnSpanned( false );
                    parent->setData(0, Qt::UserRole, pChild->getID() );
                    tree_widget_search_results_main->addTopLevelItem( parent );
                }
                else
                {
                    pItem = new QTreeWidgetItem( parent, sl );
                    pItem->setFirstColumnSpanned( false );
                    pItem->setData(0, Qt::UserRole, pChild->getID() );
                    parent->addChild( pItem );
                    parent->setExpanded( true );
                }
            }
            QStringList scriptList = pChild->getScript().split("\n");
            QStringList resultList = scriptList.filter( s, Qt::CaseInsensitive );
            for( int i=0; i<resultList.size(); i++ )
            {
                QStringList sl;
                sl << "Timer" << pChild->getName() << "script" << resultList[i];
                if( ! parent )
                {
                    parent = new QTreeWidgetItem( sl );
                    parent->setFirstColumnSpanned( false );
                    parent->setData(0, Qt::UserRole, pChild->getID() );
                    tree_widget_search_results_main->addTopLevelItem( parent );
                }
                else
                {
                    pItem = new QTreeWidgetItem( parent, sl );
                    pItem->setFirstColumnSpanned( false );
                    pItem->setData(0, Qt::UserRole, pChild->getID() );
                    parent->addChild( pItem );
                    parent->setExpanded( true );
                }
            }

            recursiveSearchTimers( pChild, s );
        }
    }

    if( true )
    {
        std::list<TKey *> nodes = mpHost->getKeyUnit()->getKeyRootNodeList();
        typedef list<TKey *>::const_iterator I;
        for( I it = nodes.begin(); it != nodes.end(); it++)
        {
            QTreeWidgetItem * pItem;
            QTreeWidgetItem * parent = 0;
            TKey * pChild = *it;
            QString n = pChild->getName();
            if( n.indexOf( s, 0, Qt::CaseInsensitive ) != -1 )
            {
                QStringList sl;
                sl << "Key" << pChild->getName() << "name";
                if( ! parent )
                {
                    parent = new QTreeWidgetItem( sl );
                    parent->setFirstColumnSpanned( false );
                    parent->setData(0, Qt::UserRole, pChild->getID() );
                    tree_widget_search_results_main->addTopLevelItem( parent );
                }
                else
                {
                    pItem = new QTreeWidgetItem( parent, sl );
                    pItem->setFirstColumnSpanned( false );
                    pItem->setData(0, Qt::UserRole, pChild->getID() );
                    parent->addChild( pItem );
                    parent->setExpanded( true );
                }
            }
            QStringList scriptList = pChild->getScript().split("\n");
            QStringList resultList = scriptList.filter( s, Qt::CaseInsensitive );
            for( int i=0; i<resultList.size(); i++ )
            {
                QStringList sl;
                sl << "Key" << pChild->getName() << "script" << resultList[i];
                if( ! parent )
                {
                    parent = new QTreeWidgetItem( sl );
                    parent->setFirstColumnSpanned( false );
                    parent->setData(0, Qt::UserRole, pChild->getID() );
                    tree_widget_search_results_main->addTopLevelItem( parent );
                }
                else
                {
                    pItem = new QTreeWidgetItem( parent, sl );
                    pItem->setFirstColumnSpanned( false );
                    pItem->setData(0, Qt::UserRole, pChild->getID() );
                    parent->addChild( pItem );
                    parent->setExpanded( true );
                }
            }

            recursiveSearchKeys( pChild, s );
        }
    }
    mpSourceEditorArea->highlighter->setSearchPattern( s );
}

void dlgTriggerEditor::recursiveSearchTriggers( TTrigger * pTriggerParent, const QString & s )
{
    list<TTrigger *> * childrenList = pTriggerParent->getChildrenList();
    typedef list<TTrigger *>::iterator I;
    for( I it=childrenList->begin(); it!=childrenList->end(); it++ )
    {
        QTreeWidgetItem * pItem;
        QTreeWidgetItem * parent = 0;
        TTrigger * pChild = *it;
        QString n = pChild->getName();
        if( n.indexOf( s, 0, Qt::CaseInsensitive ) != -1 )
        {
            QStringList sl;
            sl << "Trigger" << pChild->getName() << "name";
            if( ! parent )
            {
                parent = new QTreeWidgetItem( sl );
                parent->setFirstColumnSpanned( false );
                parent->setData(0, Qt::UserRole, pChild->getID() );
                tree_widget_search_results_main->addTopLevelItem( parent );
            }
            else
            {
                pItem = new QTreeWidgetItem( parent, sl );
                pItem->setFirstColumnSpanned( false );
                pItem->setData(0, Qt::UserRole, pChild->getID() );
                parent->addChild( pItem );
                parent->setExpanded( true );
            }
        }
        QStringList scriptList = pChild->getScript().split("\n");
        QStringList resultList = scriptList.filter( s, Qt::CaseInsensitive );
        for( int i=0; i<resultList.size(); i++ )
        {
            QStringList sl;
            sl << "Trigger" << pChild->getName() << "script" << resultList[i];
            if( ! parent )
            {
                parent = new QTreeWidgetItem( sl );
                parent->setFirstColumnSpanned( false );
                parent->setData(0, Qt::UserRole, pChild->getID() );
                tree_widget_search_results_main->addTopLevelItem( parent );
            }
            else
            {
                pItem = new QTreeWidgetItem( parent, sl );
                pItem->setFirstColumnSpanned( false );
                pItem->setData(0, Qt::UserRole, pChild->getID() );
                parent->addChild( pItem );
                parent->setExpanded( true );
            }
        }
        QStringList regexList = pChild->getRegexCodeList();
        resultList = regexList.filter( s, Qt::CaseInsensitive );
        for( int i=0; i<resultList.size(); i++ )
        {
            QStringList sl;
            sl << "Trigger" << pChild->getName() << "pattern" << resultList[i];
            if( ! parent )
            {
                parent = new QTreeWidgetItem( sl );
                parent->setFirstColumnSpanned( false );
                parent->setData(0, Qt::UserRole, pChild->getID() );
                tree_widget_search_results_main->addTopLevelItem( parent );
            }
            else
            {
                pItem = new QTreeWidgetItem( parent, sl );
                pItem->setFirstColumnSpanned( false );
                pItem->setData(0, Qt::UserRole, pChild->getID() );
                parent->addChild( pItem );
                parent->setExpanded( true );
            }
        }
        if( pChild->hasChildren() )
        {
            recursiveSearchTriggers( pChild, s );
        }
    }
}

void dlgTriggerEditor::recursiveSearchAlias( TAlias * pTriggerParent, const QString & s )
{
    list<TAlias *> * childrenList = pTriggerParent->getChildrenList();
    typedef list<TAlias *>::iterator I;
    for( I it=childrenList->begin(); it!=childrenList->end(); it++ )
    {
        QTreeWidgetItem * pItem;
        QTreeWidgetItem * parent = 0;
        TAlias * pChild = *it;
        QString n = pChild->getName();
        if( n.indexOf( s, 0, Qt::CaseInsensitive ) != -1 )
        {
            QStringList sl;
            sl << "Alias" << pChild->getName() << "name";
            if( ! parent )
            {
                parent = new QTreeWidgetItem( sl );
                parent->setFirstColumnSpanned( false );
                parent->setData(0, Qt::UserRole, pChild->getID() );
                tree_widget_search_results_main->addTopLevelItem( parent );
            }
            else
            {
                pItem = new QTreeWidgetItem( parent, sl );
                pItem->setFirstColumnSpanned( false );
                pItem->setData(0, Qt::UserRole, pChild->getID() );
                parent->addChild( pItem );
                parent->setExpanded( true );
            }
        }
        QStringList scriptList = pChild->getScript().split("\n");
        QStringList resultList = scriptList.filter( s, Qt::CaseInsensitive );
        for( int i=0; i<resultList.size(); i++ )
        {
            QStringList sl;
            sl << "Alias" << pChild->getName() << "script" << resultList[i];
            if( ! parent )
            {
                parent = new QTreeWidgetItem( sl );
                parent->setFirstColumnSpanned( false );
                parent->setData(0, Qt::UserRole, pChild->getID() );
                tree_widget_search_results_main->addTopLevelItem( parent );
            }
            else
            {
                pItem = new QTreeWidgetItem( parent, sl );
                pItem->setFirstColumnSpanned( false );
                pItem->setData(0, Qt::UserRole, pChild->getID() );
                parent->addChild( pItem );
                parent->setExpanded( true );
            }
        }


        if( pChild->getRegexCode().indexOf( s, 0, Qt::CaseInsensitive ) > -1 )
        {
            QStringList sl;
            sl << "Alias" << pChild->getName() << "pattern" << pChild->getRegexCode();
            if( ! parent )
            {
                parent = new QTreeWidgetItem( sl );
                parent->setFirstColumnSpanned( false );
                parent->setData(0, Qt::UserRole, pChild->getID() );
                tree_widget_search_results_main->addTopLevelItem( parent );
            }
            else
            {
                pItem = new QTreeWidgetItem( parent, sl );
                pItem->setFirstColumnSpanned( false );
                pItem->setData(0, Qt::UserRole, pChild->getID() );
                parent->addChild( pItem );
                parent->setExpanded( true );
            }
        }
        if( pChild->hasChildren() )
        {
            recursiveSearchAlias( pChild, s );
        }
    }
}

void dlgTriggerEditor::recursiveSearchScripts( TScript * pTriggerParent, const QString & s )
{
    list<TScript *> * childrenList = pTriggerParent->getChildrenList();
    typedef list<TScript *>::iterator I;
    for( I it=childrenList->begin(); it!=childrenList->end(); it++ )
    {
        QTreeWidgetItem * pItem;
        QTreeWidgetItem * parent = 0;
        TScript * pChild = *it;
        QString n = pChild->getName();
        if( n.indexOf( s, 0, Qt::CaseInsensitive ) != -1 )
        {
            QStringList sl;
            sl << "Script" << pChild->getName() << "name";
            if( ! parent )
            {
                parent = new QTreeWidgetItem( sl );
                parent->setFirstColumnSpanned( false );
                parent->setData(0, Qt::UserRole, pChild->getID() );
                tree_widget_search_results_main->addTopLevelItem( parent );
            }
            else
            {
                pItem = new QTreeWidgetItem( parent, sl );
                pItem->setFirstColumnSpanned( false );
                pItem->setData(0, Qt::UserRole, pChild->getID() );
                parent->addChild( pItem );
                parent->setExpanded( true );
            }
        }
        QStringList scriptList = pChild->getScript().split("\n");
        QStringList resultList = scriptList.filter( s, Qt::CaseInsensitive );
        for( int i=0; i<resultList.size(); i++ )
        {
            QStringList sl;
            sl << "Script" << pChild->getName() << "script" << resultList[i];
            if( ! parent )
            {
                parent = new QTreeWidgetItem( sl );
                parent->setFirstColumnSpanned( false );
                parent->setData(0, Qt::UserRole, pChild->getID() );
                tree_widget_search_results_main->addTopLevelItem( parent );
            }
            else
            {
                pItem = new QTreeWidgetItem( parent, sl );
                pItem->setFirstColumnSpanned( false );
                pItem->setData(0, Qt::UserRole, pChild->getID() );
                parent->addChild( pItem );
                parent->setExpanded( true );
            }
        }

        if( pChild->hasChildren() )
        {
            recursiveSearchScripts( pChild, s );
        }
    }
}

void dlgTriggerEditor::recursiveSearchActions( TAction * pTriggerParent, const QString & s )
{
    list<TAction *> * childrenList = pTriggerParent->getChildrenList();
    typedef list<TAction *>::iterator I;
    for( I it=childrenList->begin(); it!=childrenList->end(); it++ )
    {
        QTreeWidgetItem * pItem;
        QTreeWidgetItem * parent = 0;
        TAction * pChild = *it;
        QString n = pChild->getName();
        if( n.indexOf( s, 0, Qt::CaseInsensitive ) != -1 )
        {
            QStringList sl;
            sl << "Button" << pChild->getName() << "name";
            if( ! parent )
            {
                parent = new QTreeWidgetItem( sl );
                parent->setFirstColumnSpanned( false );
                parent->setData(0, Qt::UserRole, pChild->getID() );
                tree_widget_search_results_main->addTopLevelItem( parent );
            }
            else
            {
                pItem = new QTreeWidgetItem( parent, sl );
                pItem->setFirstColumnSpanned( false );
                pItem->setData(0, Qt::UserRole, pChild->getID() );
                parent->addChild( pItem );
                parent->setExpanded( true );
            }
        }
        QStringList scriptList = pChild->getScript().split("\n");
        QStringList resultList = scriptList.filter( s, Qt::CaseInsensitive );
        for( int i=0; i<resultList.size(); i++ )
        {
            QStringList sl;
            sl << "Button" << pChild->getName() << "script" << resultList[i];
            if( ! parent )
            {
                parent = new QTreeWidgetItem( sl );
                parent->setFirstColumnSpanned( false );
                parent->setData(0, Qt::UserRole, pChild->getID() );
                tree_widget_search_results_main->addTopLevelItem( parent );
            }
            else
            {
                pItem = new QTreeWidgetItem( parent, sl );
                pItem->setFirstColumnSpanned( false );
                pItem->setData(0, Qt::UserRole, pChild->getID() );
                parent->addChild( pItem );
                parent->setExpanded( true );
            }
        }

        if( pChild->hasChildren() )
        {
            recursiveSearchActions( pChild, s );
        }
    }
}

void dlgTriggerEditor::recursiveSearchTimers( TTimer * pTriggerParent, const QString & s )
{
    list<TTimer *> * childrenList = pTriggerParent->getChildrenList();
    typedef list<TTimer *>::iterator I;
    for( I it=childrenList->begin(); it!=childrenList->end(); it++ )
    {
        QTreeWidgetItem * pItem;
        QTreeWidgetItem * parent = 0;
        TTimer * pChild = *it;
        QString n = pChild->getName();
        if( n.indexOf( s, 0, Qt::CaseInsensitive ) != -1 )
        {
            QStringList sl;
            sl << "Timer" << pChild->getName() << "name";
            if( ! parent )
            {
                parent = new QTreeWidgetItem( sl );
                parent->setFirstColumnSpanned( false );
                parent->setData(0, Qt::UserRole, pChild->getID() );
                tree_widget_search_results_main->addTopLevelItem( parent );
            }
            else
            {
                pItem = new QTreeWidgetItem( parent, sl );
                pItem->setFirstColumnSpanned( false );
                pItem->setData(0, Qt::UserRole, pChild->getID() );
                parent->addChild( pItem );
                parent->setExpanded( true );
            }
        }
        QStringList scriptList = pChild->getScript().split("\n");
        QStringList resultList = scriptList.filter( s, Qt::CaseInsensitive );
        for( int i=0; i<resultList.size(); i++ )
        {
            QStringList sl;
            sl << "Timer" << pChild->getName() << "script" << resultList[i];
            if( ! parent )
            {
                parent = new QTreeWidgetItem( sl );
                parent->setFirstColumnSpanned( false );
                parent->setData(0, Qt::UserRole, pChild->getID() );
                tree_widget_search_results_main->addTopLevelItem( parent );
            }
            else
            {
                pItem = new QTreeWidgetItem( parent, sl );
                pItem->setFirstColumnSpanned( false );
                pItem->setData(0, Qt::UserRole, pChild->getID() );
                parent->addChild( pItem );
                parent->setExpanded( true );
            }
        }

        if( pChild->hasChildren() )
        {
            recursiveSearchTimers( pChild, s );
        }
    }
}

void dlgTriggerEditor::recursiveSearchKeys( TKey * pTriggerParent, const QString & s )
{
    list<TKey *> * childrenList = pTriggerParent->getChildrenList();
    typedef list<TKey *>::iterator I;
    for( I it=childrenList->begin(); it!=childrenList->end(); it++ )
    {
        QTreeWidgetItem * pItem;
        QTreeWidgetItem * parent = 0;
        TKey * pChild = *it;
        QString n = pChild->getName();
        if( n.indexOf( s, 0, Qt::CaseInsensitive ) != -1 )
        {
            QStringList sl;
            sl << "Key" << pChild->getName() << "name";
            if( ! parent )
            {
                parent = new QTreeWidgetItem( sl );
                parent->setFirstColumnSpanned( false );
                parent->setData(0, Qt::UserRole, pChild->getID() );
                tree_widget_search_results_main->addTopLevelItem( parent );
            }
            else
            {
                pItem = new QTreeWidgetItem( parent, sl );
                pItem->setFirstColumnSpanned( false );
                pItem->setData(0, Qt::UserRole, pChild->getID() );
                parent->addChild( pItem );
                parent->setExpanded( true );
            }
        }
        QStringList scriptList = pChild->getScript().split("\n");
        QStringList resultList = scriptList.filter( s, Qt::CaseInsensitive );
        for( int i=0; i<resultList.size(); i++ )
        {
            QStringList sl;
            sl << "Key" << pChild->getName() << "script" << resultList[i];
            if( ! parent )
            {
                parent = new QTreeWidgetItem( sl );
                parent->setFirstColumnSpanned( false );
                parent->setData(0, Qt::UserRole, pChild->getID() );
                tree_widget_search_results_main->addTopLevelItem( parent );
            }
            else
            {
                pItem = new QTreeWidgetItem( parent, sl );
                pItem->setFirstColumnSpanned( false );
                pItem->setData(0, Qt::UserRole, pChild->getID() );
                parent->addChild( pItem );
                parent->setExpanded( true );
            }
        }

        if( pChild->hasChildren() )
        {
            recursiveSearchKeys( pChild, s );
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
    QTreeWidgetItem * pItem = treeWidget_alias->currentItem();
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
    mCurrentAlias = 0;
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
    mCurrentAction = 0;
    mpHost->getActionUnit()->updateToolbar();
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
    mCurrentScript = 0;
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
    mCurrentKey = 0;
}

void dlgTriggerEditor::slot_deleteTrigger()
{
    QTreeWidgetItem * pItem = treeWidget->currentItem();
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
    mCurrentTrigger = 0;
}

void dlgTriggerEditor::slot_deleteTimer()
{
    QTreeWidgetItem * pItem = treeWidget_timers->currentItem();
    if( ! pItem ) return;
    QTreeWidgetItem * pParent = pItem->parent();

    if( ! pItem ) return;

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
    mCurrentTimer = 0;
}


void dlgTriggerEditor::slot_trigger_toggle_active()
{
    QTreeWidgetItem * pItem = (QTreeWidgetItem *)treeWidget->currentItem();
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
                icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/filter.png")), QIcon::Normal, QIcon::Off);
            }
            else
            {
                icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/filter-grey.png")), QIcon::Normal, QIcon::Off);
            }
        }
        else
        {
            if( pT->ancestorsActive() )
            {
                icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/filter-locked.png")), QIcon::Normal, QIcon::Off);
            }
            else
            {
                icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/filter-grey-locked.png")), QIcon::Normal, QIcon::Off);
            }
        }
    }
    else if( pT->isFolder() )
    {
        if( pT->isActive() )
        {
            if( pT->ancestorsActive() )
                icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-blue.png")), QIcon::Normal, QIcon::Off);
            else
                icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
        }
        else
        {
            if( pT->ancestorsActive() )
                icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-blue-locked.png")), QIcon::Normal, QIcon::Off);
            else
                icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
        }
    }
    else
    {
        if( pT->isActive() )
        {
            if( pT->ancestorsActive() )
                icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
            else
                icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
        }
        else
        {
            if( pT->ancestorsActive() )
                icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
            else
                icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
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
        iconError.addPixmap(QPixmap(QString::fromUtf8(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
        pItem->setIcon( 0, iconError );
    }
    showInfo( QString( "Trying to %2 Trigger %1 %3." )
              .arg(pT->getName())
              .arg( pT->shouldBeActive() ? "activated" : "deactivated" )
              .arg( pT->state() ? "succeeded" : QString("failed reason:") + pT->getError() ) );
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
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/filter.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/filter-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
                else
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/filter-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/filter-grey-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            else if( pT->isFolder() )
            {
                if( pT->isActive() )
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-blue.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
                else
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-blue-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            else
            {
                if( pT->isActive() )
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                    }

                }
                else
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox.png"),0,Qt::MonoOnly), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox-grey.png"),0,Qt::MonoOnly), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            //pItem->setDisabled(!pT->ancestorsActive());
            pItem->setIcon(0, icon);
        }
        else
        {
            QIcon iconError;
            iconError.addPixmap(QPixmap(QString::fromUtf8(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            pItem->setIcon( 0, iconError );
            showError( pT->getError() );
        }
    }

}


void dlgTriggerEditor::slot_timer_toggle_active()
{
    QTreeWidgetItem * pItem = (QTreeWidgetItem *)treeWidget_timers->currentItem();
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
            icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-green.png")), QIcon::Normal, QIcon::Off);
        }
        else
        {
            icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-green-locked.png")), QIcon::Normal, QIcon::Off);
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
                icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/offsettimer-on.png")), QIcon::Normal, QIcon::Off);
            }
            else
            {
                pT->disableTimer( pT->getID() );
                icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/offsettimer-off.png")), QIcon::Normal, QIcon::Off);
            }
        }
        else
        {
            if( pT->shouldBeActive() )
            {
                pT->enableTimer( pT->getID() );
                icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
            }
            else
            {
                pT->disableTimer( pT->getID() );
                icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
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
        iconError.addPixmap(QPixmap(QString::fromUtf8(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
        pItem->setIcon( 0, iconError );
    }


    showInfo( QString( "Trying to %2 Timer %1 %3." )
              .arg(pT->getName())
              .arg( pT->shouldBeActive() ? "activated" : "deactivated" )
              .arg( pT->state() ? "succeeded" : QString("failed reason:") + pT->getError() ) );
}

void dlgTriggerEditor::slot_alias_toggle_active()
{
    QTreeWidgetItem * pItem = (QTreeWidgetItem *)treeWidget_alias->currentItem();
    if( ! pItem ) return;
    QIcon icon;

    TAlias * pT = mpHost->getAliasUnit()->getAlias(pItem->data(0, Qt::UserRole).toInt());
    if( ! pT ) return;
    qDebug()<<"aliasToggleActive()";
    pT->setIsActive( ! pT->shouldBeActive() );

    if( pT->isFolder() )
    {
        if( pT->isActive() )
        {
            icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-violet.png")), QIcon::Normal, QIcon::Off);
        }
        else
        {
            icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-violet-locked.png")), QIcon::Normal, QIcon::Off);
        }
    }
    else
    {
        if( pT->isActive() )
        {
            icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
        }
        else
        {
            icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
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
        iconError.addPixmap(QPixmap(QString::fromUtf8(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
        pItem->setIcon( 0, iconError );
    }
    showInfo( QString( "Trying to %2 Alias %1 %3." )
              .arg(pT->getName())
              .arg( pT->shouldBeActive() ? "activate" : "deactivate" )
              .arg( pT->state() ? "succeeded" : QString("failed reason:") + pT->getError() ) );

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
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-violet.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
                else
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-violet-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            else
            {
                if( pT->isActive() )
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                    }

                }
                else
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox.png"),0,Qt::MonoOnly), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox-grey.png"),0,Qt::MonoOnly), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            pItem->setIcon(0, icon);
        }
        else
        {
            QIcon iconError;
            iconError.addPixmap(QPixmap(QString::fromUtf8(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            pItem->setIcon( 0, iconError );
            showError( pT->getError() );
        }
    }

}



void dlgTriggerEditor::slot_script_toggle_active()
{
    QTreeWidgetItem * pItem = (QTreeWidgetItem *)treeWidget_scripts->currentItem();
    if( ! pItem ) return;
    QIcon icon;

    TScript * pT = mpHost->getScriptUnit()->getScript(pItem->data(0, Qt::UserRole).toInt());
    if( ! pT ) return;

    pT->setIsActive( ! pT->shouldBeActive() );

    if( pT->isFolder() )
    {
        if( pT->isActive() )
        {
            icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-orange.png")), QIcon::Normal, QIcon::Off);
        }
        else
        {
            icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-orange-locked.png")), QIcon::Normal, QIcon::Off);
        }
    }
    else
    {
        if( pT->isActive() )
        {
            icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
        }
        else
        {
            icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
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
        iconError.addPixmap(QPixmap(QString::fromUtf8(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
        pItem->setIcon( 0, iconError );
    }
    showInfo( QString( "Trying to %2 Script %1 %3." )
              .arg(pT->getName())
              .arg( pT->shouldBeActive() ? "activated" : "deactivated" )
              .arg( pT->state() ? "succeeded" : QString("failed reason:") + pT->getError() ) );
}

void dlgTriggerEditor::slot_action_toggle_active()
{
    QTreeWidgetItem * pItem = (QTreeWidgetItem *)treeWidget_actions->currentItem();
    if( ! pItem ) return;
    QIcon icon;

    TAction * pT = mpHost->getActionUnit()->getAction(pItem->data(0, Qt::UserRole).toInt());
    if( ! pT ) return;

    pT->setIsActive( ! pT->shouldBeActive() );

    if( pT->isFolder() )
    {
        if( pT->isActive() )
        {
            icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-cyan.png")), QIcon::Normal, QIcon::Off);
        }
        else
        {
            icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-cyan-locked.png")), QIcon::Normal, QIcon::Off);
        }
    }
    else
    {
        if( pT->isActive() )
        {
            icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
        }
        else
        {
            icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
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
        iconError.addPixmap(QPixmap(QString::fromUtf8(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
        pItem->setIcon( 0, iconError );
    }
    showInfo( QString( "Trying to %2 Action %1 %3." )
              .arg(pT->getName())
              .arg( pT->shouldBeActive() ? "activated" : "deactivated" )
              .arg( pT->state() ? "succeeded" : QString("failed reason:") + pT->getError() ) );
    mpHost->getActionUnit()->updateToolbar();
}

void dlgTriggerEditor::slot_key_toggle_active()
{
    QTreeWidgetItem * pItem = (QTreeWidgetItem *)treeWidget_keys->currentItem();
    if( ! pItem ) return;
    QIcon icon;

    TKey * pT = mpHost->getKeyUnit()->getKey(pItem->data(0, Qt::UserRole).toInt());
    if( ! pT ) return;

    pT->setIsActive( ! pT->shouldBeActive() );

    if( pT->isFolder() )
    {
        if( pT->isActive() )
        {
            icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-cyan.png")), QIcon::Normal, QIcon::Off);
        }
        else
        {
            icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-cyan-locked.png")), QIcon::Normal, QIcon::Off);
        }
    }
    else
    {
        if( pT->isActive() )
        {
            icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
        }
        else
        {
            icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
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
        iconError.addPixmap(QPixmap(QString::fromUtf8(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
        pItem->setIcon( 0, iconError );
    }
    showInfo( QString( "Trying to %2 Key %1 %3." )
              .arg(pT->getName())
              .arg( pT->shouldBeActive() ? "activated" : "deactivated" )
              .arg( pT->state() ? "succeeded" : QString("failed reason:") + pT->getError() ) );
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
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-cyan.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
                else
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-cyan-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            else
            {
                if( pT->isActive() )
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                    }

                }
                else
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox.png"),0,Qt::MonoOnly), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox-grey.png"),0,Qt::MonoOnly), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            pItem->setIcon(0, icon);
        }
        else
        {
            QIcon iconError;
            iconError.addPixmap(QPixmap(QString::fromUtf8(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
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

    QTreeWidgetItem * pParent = (QTreeWidgetItem*)treeWidget->currentItem();
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
        treeWidget->insertTopLevelItem( 0, pNewItem );
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
        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-red.png")), QIcon::Normal, QIcon::Off);
    }
    else
    {
        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/document-save-as.png")), QIcon::Normal, QIcon::Off);
    }
    pNewItem->setIcon( 0, icon );
    if( pParent ) pParent->setExpanded( true );
    mpTriggersMainArea->lineEdit_trigger_name->clear();
    mpTriggersMainArea->perlSlashGOption->setChecked( false );
    mpSourceEditorArea->editor->clear();
    mpTriggersMainArea->trigger_command->clear();
    mpTriggersMainArea->filterTrigger->setChecked( false );
    mpTriggersMainArea->spinBox_stayOpen->setValue( 0 );
    mpTriggersMainArea->spinBox_linemargin->setValue( 0 );
    mpTriggersMainArea->checkBox_multlinetrigger->setChecked( false );

    QPalette pal = palette();
    QColor color =  pal.color( QPalette::Button );
    QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");

    mpTriggersMainArea->pushButtonFgColor->setChecked( false );
    mpTriggersMainArea->pushButtonBgColor->setChecked( false );
    mpTriggersMainArea->colorizerTrigger->setChecked( false );

    treeWidget->setCurrentItem( pNewItem );
    mCurrentTrigger = pNewItem;
    showInfo( msgInfoAddTrigger );
    slot_trigger_clicked( treeWidget->currentItem(), 0 );
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

    QTreeWidgetItem * pParent = (QTreeWidgetItem*)treeWidget_timers->currentItem();
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
        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-red.png")), QIcon::Normal, QIcon::Off);
    }
    else
    {
        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/document-save-as.png")), QIcon::Normal, QIcon::Off);
    }
    pNewItem->setIcon( 0, icon );
    if( pParent ) pParent->setExpanded( true );
    //FIXME
    //mpOptionsAreaTriggers->lineEdit_trigger_name->clear();
    mpTimersMainArea->lineEdit_command->clear();
    mpSourceEditorArea->editor->clear();
    treeWidget_timers->setCurrentItem( pNewItem );
    mCurrentTimer = pNewItem;
    showInfo( msgInfoAddTimer );
    slot_timer_clicked( treeWidget_timers->currentItem(), 0 );
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

    QTreeWidgetItem * pParent = (QTreeWidgetItem*)treeWidget_keys->currentItem();
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
        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-red.png")), QIcon::Normal, QIcon::Off);
    }
    else
    {
        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/document-save-as.png")), QIcon::Normal, QIcon::Off);
    }
    pNewItem->setIcon( 0, icon );
    if( pParent ) pParent->setExpanded( true );
    mpKeysMainArea->lineEdit_command->clear();
    mpKeysMainArea->lineEdit_key->setText("no key chosen");
    mpSourceEditorArea->editor->clear();
    treeWidget_keys->setCurrentItem( pNewItem );
    mCurrentKey = pNewItem;
    showInfo( msgInfoAddKey );
    slot_key_clicked( treeWidget_keys->currentItem(), 0 );
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

    QTreeWidgetItem * pParent = (QTreeWidgetItem*)treeWidget_alias->currentItem();
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
        pT->setRegexCode( regex );
        pNewItem = new QTreeWidgetItem( mpAliasBaseItem, nameL );
        treeWidget_alias->insertTopLevelItem( 0, pNewItem );
    }

    if( ! pT ) return;

    pT->setName( name );
    pT->setCommand( command );
    pT->setRegexCode( regex );
    pT->setScript( script );
    pT->setIsFolder( isFolder );
    pT->setIsActive( false );
    pT->registerAlias();
    int childID = pT->getID();
    pNewItem->setData( 0, Qt::UserRole, childID );
    QIcon icon;
    if( isFolder )
    {
        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-red.png")), QIcon::Normal, QIcon::Off);
    }
    else
    {
        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/document-save-as.png")), QIcon::Normal, QIcon::Off);
    }
    pNewItem->setIcon( 0, icon );
    if( pParent ) pParent->setExpanded( true );

    mpAliasMainArea->lineEdit_alias_name->clear();
    mpAliasMainArea->pattern_textedit->clear();
    mpAliasMainArea->substitution->clear();
    mpSourceEditorArea->editor->clear();

    mpAliasMainArea->lineEdit_alias_name->setText( name );

    treeWidget_alias->setCurrentItem( pNewItem );
    mCurrentAlias = pNewItem;
    showInfo(msgInfoAddAlias);
    slot_alias_clicked( treeWidget_alias->currentItem(), 0 );
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

    QTreeWidgetItem * pParent = (QTreeWidgetItem*)treeWidget_actions->currentItem();
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
        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-red.png")), QIcon::Normal, QIcon::Off);
    }
    else
    {
        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/document-save-as.png")), QIcon::Normal, QIcon::Off);
    }
    pNewItem->setIcon( 0, icon );
    if( pParent ) pParent->setExpanded( true );
//    mpActionsMainArea->lineEdit_action_button_down->clear();
//    mpActionsMainArea->lineEdit_action_button_up->clear();
    mpActionsMainArea->lineEdit_action_icon->clear();
    mpActionsMainArea->checkBox_pushdownbutton->setChecked(false);
    mpSourceEditorArea->editor->clear();

    mpHost->getActionUnit()->updateToolbar();

    treeWidget_actions->setCurrentItem( pNewItem );
    mpCurrentActionItem = pNewItem;
    mCurrentAction = pNewItem;
    showInfo( msgInfoAddButton );
    slot_action_clicked( treeWidget_actions->currentItem(), 0 );
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

    QTreeWidgetItem * pParent = (QTreeWidgetItem*)treeWidget_scripts->currentItem();
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
        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-red.png")), QIcon::Normal, QIcon::Off);
    }
    else
    {
        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/document-save-as.png")), QIcon::Normal, QIcon::Off);
    }
    pNewItem->setIcon( 0, icon );
    if( pParent ) pParent->setExpanded( true );
    mpScriptsMainArea->lineEdit_scripts_name->clear();
    //FIXME mpScriptsMainArea->pattern_textedit->clear();
    mpSourceEditorArea->editor->setPlainText( script );
    mCurrentScript = pNewItem;
    treeWidget_scripts->setCurrentItem( pNewItem );
    slot_scripts_clicked( treeWidget_scripts->currentItem(), 0 );
}



void dlgTriggerEditor::slot_saveTriggerAfterEdit()
{
    return saveTrigger();
    /*
    QString name = mpTriggersMainArea->lineEdit_trigger_name->text();
    QString command = mpTriggersMainArea->trigger_command->text();
    mpTriggersMainArea->lineEdit->clear();
    bool isMultiline = mpTriggersMainArea->checkBox_multlinetrigger->isChecked();
    QList<QListWidgetItem*> itemList;
    for( int i=0; i<mpTriggersMainArea->listWidget_regex_list->count(); i++ )
    {
        QListWidgetItem * pItem = mpTriggersMainArea->listWidget_regex_list->item(i);
        itemList << pItem;
    }
    QStringList regexList;
    QList<int> regexPropertyList;
    for( int i=0; i<itemList.size(); i++ )
    {
        if( itemList[i]->text().size() < 1 ) continue;
        regexList << itemList[i]->text();
        QColor fgColor = itemList[i]->foreground().color();
        QColor bgColor = itemList[i]->background().color();
        if( (fgColor == QColor(0,0,0)) && (bgColor == QColor(255,255,255)) )
        {
            regexPropertyList << REGEX_SUBSTRING;
        }
        if( (fgColor == QColor(0,0,255)) && (bgColor == QColor(255,255,255)) )
        {
            regexPropertyList << REGEX_PERL;
        }
        if( (fgColor == QColor(195,0,0)) && (bgColor == QColor(55,55,55)) )
        {
            regexPropertyList << REGEX_BEGIN_OF_LINE_SUBSTRING;
        }
        if( (fgColor == QColor(0,155,0)) && (bgColor == QColor(255,255,255)) )
        {
            regexPropertyList << REGEX_EXACT_MATCH;
        }
        if( (fgColor == QColor(155,155,0)) && (bgColor == QColor(0,0,0)) )
        {
            regexPropertyList << REGEX_LUA_CODE;
        }
    }
    QString script = mpSourceEditorArea->script_scintilla->text();

    QTreeWidgetItem * pItem = treeWidget->currentItem();
    if( pItem )
    {
        int triggerID = pItem->data( 0, Qt::UserRole ).toInt();
        TTrigger * pT = mpHost->getTriggerUnit()->getTrigger( triggerID );
        if( pT )
        {
            pT->setName( name );
            pT->setCommand( command );
            pT->setRegexCodeList( regexList, regexPropertyList );
            pT->setTriggerType( mpTriggersMainArea->comboBox_regexstyle->currentIndex() );
            pT->setScript( script );
            pT->setIsMultiline( isMultiline );
            pT->mPerlSlashGOption = mpTriggersMainArea->perlSlashGOption->isChecked();
            pT->mFilterTrigger = mpTriggersMainArea->filterTrigger->isChecked();
            pT->setConditionLineDelta( mpTriggersMainArea->spinBox_linemargin->value() );
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
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/filter.png")), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/filter-locked.png")), QIcon::Normal, QIcon::Off);
                }
            }
            else if( pT->isFolder() )
            {
                if( pT->isActive() )
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-blue.png")), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-blue-locked.png")), QIcon::Normal, QIcon::Off);
                }
            }
            else
            {
                if( pT->isActive() )
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                }
            }
            if( pT->state() )
            {
                pItem->setIcon( 0, icon);
                pItem->setText( 0, name );
                showInfo( "Success, trigger compiled and is ready for activation. You can activate it with the padlock icon." );
            }
            else
            {
                QIcon iconError;
                iconError.addPixmap(QPixmap(QString::fromUtf8(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
                pItem->setIcon( 0, iconError );
                showError( pT->getError() );
                pItem->setText( 0, name );
            }
        }
    }
    else
    {
        showError("Error: No item selected! Which item do you want to save?");
    }
    mpTriggerMainAreaEditRegexItem = 0;*/
}

void dlgTriggerEditor::saveTrigger()
{
    QTime t;
    t.start();
    QTreeWidgetItem * pItem = mCurrentTrigger;
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
    QString script = mpSourceEditorArea->editor->toPlainText();

    if( pItem )
    {
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
                    if( pT->ancestorsActive() )
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/filter.png")), QIcon::Normal, QIcon::Off);
                    else
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/filter-grey.png")), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    if( pT->ancestorsActive() )
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/filter-locked.png")), QIcon::Normal, QIcon::Off);
                    else
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/filter-grey-locked.png")), QIcon::Normal, QIcon::Off);
                }
            }
            else if( pT->isFolder() )
            {
                if( ! pT->mPackageName.isEmpty() )
                {
                    if( pT->isActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
                else if( pT->isActive() )
                {
                    if( pT->ancestorsActive() )
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-blue.png")), QIcon::Normal, QIcon::Off);
                    else
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    if( pT->ancestorsActive() )
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-blue-locked.png")), QIcon::Normal, QIcon::Off);
                    else
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                }
            }
            else
            {
                if( pT->isActive() )
                {
                    if( pT->ancestorsActive() )
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    else
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    if( pT->ancestorsActive() )
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    else
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                }
            }
            if( pT->state() )
            {
                if( old_name == "New Trigger" || old_name == "New Trigger Group" )
                {
                    QIcon _icon;
                    if( pT->isFolder() )
                    {
                        _icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-blue.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        _icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
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
                iconError.addPixmap(QPixmap(QString::fromUtf8(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
                pItem->setIcon( 0, iconError );
                pT->setIsActive( false );

            }
        }
    }
    else
    {
        showError("Error: No item selected! Which item do you want to save?");
    }
}

void dlgTriggerEditor::slot_saveTimerAfterEdit()
{
    return saveTimer();

    /*QString name = mpTimersMainArea->lineEdit_timer_name->text();
    QString script = mpSourceEditorArea->script_scintilla->text();

    QTreeWidgetItem * pItem = treeWidget_timers->currentItem();
    if( pItem )
    {
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
           // if( pT->isOffsetTimer() )
            //{
            //    pT->setShouldBeActive( true );
           // }
           // else
            //{
             //   pT->setIsActive( true );
            //}

            QIcon icon;
            if( pT->isFolder() )
            {
                if( pT->shouldBeActive() )
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-green.png")), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-green-locked.png")), QIcon::Normal, QIcon::Off);
                }
            }
            if( pT->isOffsetTimer() )
            {
                if( pT->shouldBeActive() )
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/offsettimer-on.png")), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/offsettimer-off.png")), QIcon::Normal, QIcon::Off);
                }
            }
            else
            {
                if( pT->shouldBeActive() )
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                }
            }
            if( pT->state() )
            {
                pItem->setIcon( 0, icon);
                pItem->setText( 0, name );
                showInfo( "Success, trigger compiled and is ready for activation. You can activate it with the padlock icon." );
            }
            else
            {
                QIcon iconError;
                iconError.addPixmap(QPixmap(QString::fromUtf8(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
                pItem->setIcon( 0, iconError );
                showError( pT->getError() );
                pItem->setText( 0, name );
            }
        }
    }*/
}

void dlgTriggerEditor::saveTimer()
{
    QTreeWidgetItem * pItem = mCurrentTimer;
    if( ! pItem ) return;
    QString name = mpTimersMainArea->lineEdit_timer_name->text();
    QString script = mpSourceEditorArea->editor->toPlainText();

    if( pItem )
    {
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
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
                else if( pT->shouldBeActive() )
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-green.png")), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-green-locked.png")), QIcon::Normal, QIcon::Off);
                }
            }
            if( pT->isOffsetTimer() )
            {
                if( pT->shouldBeActive() )
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/offsettimer-on.png")), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/offsettimer-off.png")), QIcon::Normal, QIcon::Off);
                }
            }
            else
            {
                if( pT->shouldBeActive() )
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    pT->setIsActive( true );
                }
                else
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
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
                iconError.addPixmap(QPixmap(QString::fromUtf8(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
                pItem->setIcon( 0, iconError );
                pItem->setText( 0, name );

            }

        }
    }
}


void dlgTriggerEditor::slot_saveAliasAfterEdit()
{
    return saveAlias();

    /*qDebug()<<"slot_saveAliasAfterEdit()";
    bool state = true;
    QString name = mpAliasMainArea->lineEdit_alias_name->text();
    QString regex = mpAliasMainArea->pattern_textedit->text();
    if( (name.size() < 1) || (name=="New Alias") )
    {
        name = regex;
    }
    QString substitution = mpAliasMainArea->substitution->text();
    QString script = mpSourceEditorArea->script_scintilla->text();
    QTreeWidgetItem * pItem = treeWidget_alias->currentItem();
    if( pItem )
    {
        int triggerID = pItem->data(0, Qt::UserRole).toInt();
        TAlias * pT = mpHost->getAliasUnit()->getAlias( triggerID );
        if( pT )
        {
            pT->setName( name );
            pT->setCommand( substitution );
            pT->setRegexCode( regex );
            pT->setScript( script );
            pT->setIsActive( true );//FIXME: discuss if new triggers are automatically active

            QIcon icon;
            if( pT->isFolder() )
            {
                if( pT->isActive() )
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-violet.png")), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-violet-locked.png")), QIcon::Normal, QIcon::Off);
                }
            }
            else
            {
                if( pT->isActive() )
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                }
            }

            if( pT->state() )
            {
                pItem->setIcon( 0, icon);
                pItem->setText( 0, name );
                showInfo( "Success, trigger compiled and is ready for activation. You can activate it with the padlock icon." );
            }
            else
            {
                QIcon iconError;
                iconError.addPixmap(QPixmap(QString::fromUtf8(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
                pItem->setIcon( 0, iconError );
                showError( pT->getError() );
                pItem->setText( 0, name );
            }
        }
    }*/
}

void dlgTriggerEditor::saveAlias()
{
    QTreeWidgetItem * pItem = mCurrentAlias;
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
    QString script = mpSourceEditorArea->editor->toPlainText();
    if( pItem )
    {
        int triggerID = pItem->data(0, Qt::UserRole).toInt();
        TAlias * pT = mpHost->getAliasUnit()->getAlias( triggerID );
        if( pT )
        {
            QString old_name = pT->getName();
            pT->setName( name );
            pT->setCommand( substitution );
            pT->setRegexCode( regex );
            pT->setScript( script );

            QIcon icon;
            if( pT->isFolder() )
            {
                if( ! pT->mPackageName.isEmpty() )
                {
                    if( pT->isActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
                else if( pT->isActive() )
                {
                    if( pT->ancestorsActive() )
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-violet.png")), QIcon::Normal, QIcon::Off);
                    else
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    if( pT->ancestorsActive() )
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-violet-locked.png")), QIcon::Normal, QIcon::Off);
                    else
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                }
            }
            else
            {
                if( pT->isActive() )
                {
                    if( pT->ancestorsActive() )
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    else
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    if( pT->ancestorsActive() )
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    else
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                }
            }

            if( pT->state() )
            {
                if( old_name == "New Alias" )//|| old_name == "New Alias Group" )
                {
                    QIcon _icon;
                    if( pT->isFolder() )
                    {
                        if( pT->ancestorsActive() )
                            _icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-violet.png")), QIcon::Normal, QIcon::Off);
                        else
                            _icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        if( pT->ancestorsActive() )
                            _icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                        _icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
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
                iconError.addPixmap(QPixmap(QString::fromUtf8(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
                pItem->setIcon( 0, iconError );
                pItem->setText( 0, name );
            }
        }
    }
}

void dlgTriggerEditor::slot_saveActionAfterEdit()
{
    return saveAction();

    /*QString name = mpActionsMainArea->lineEdit_action_name->text();
    QString cmdDown = mpActionsMainArea->lineEdit_action_button_down->text();
    QString cmdUp = mpActionsMainArea->lineEdit_action_button_up->text();
    QString icon = mpActionsMainArea->lineEdit_action_icon->text();
    QString script = mpSourceEditorArea->script_scintilla->text();
    QColor color = mpActionsMainArea->pushButton_color->palette().color(QPalette::Button);
    int sizeX = mpActionsMainArea->buttonSizeX->text().toInt();
    int sizeY = mpActionsMainArea->buttonSizeY->text().toInt();
    int posX = mpActionsMainArea->buttonPosX->text().toInt();
    int posY = mpActionsMainArea->buttonPosY->text().toInt();
    int rotation = mpActionsMainArea->buttonRotation->currentIndex();
    int columns = mpActionsMainArea->buttonColumns->text().toInt();
    bool flatButton = mpActionsMainArea->buttonFlat->isChecked();
    bool isChecked = mpActionsMainArea->checkBox_pushdownbutton->isChecked();
    int location = mpActionsMainArea->comboBox_location->currentIndex();
    int orientation = mpActionsMainArea->comboBox_orientation->currentIndex();
    bool useCustomLayout = mpActionsMainArea->useCustomLayout->isChecked();
    QTreeWidgetItem * pItem = treeWidget_actions->currentItem();
    if( pItem )
    {
        int triggerID = pItem->data(0, Qt::UserRole).toInt();
        TAction * pT = mpHost->getActionUnit()->getAction( triggerID );
        if( pT )
        {
            pT->setName( name );
            pT->setCommandButtonDown( cmdDown );
            pT->setCommandButtonUp( cmdUp );
            pT->setIcon( icon );
            pT->setScript( script );
            pT->setIsPushDownButton( isChecked );
            pT->mLocation = location;
            pT->mOrientation = orientation;
            pT->setIsActive( true );
            pT->setButtonColor( color );
            pT->setButtonRotation( rotation );
            pT->setButtonColumns( columns );
            pT->setButtonFlat( flatButton );
            pT->mUseCustomLayout = useCustomLayout;
            pT->mPosX = posX;
            pT->mPosY = posY;
            pT->mSizeX = sizeX;
            pT->mSizeY = sizeY;
            pT->css = mpActionsMainArea->css->toPlainText();
            QIcon icon;
            if( pT->isFolder() )
            {
                if( pT->isActive() )
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-cyan.png")), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-cyan-locked.png")), QIcon::Normal, QIcon::Off);
                }
            }
            else
            {
                if( pT->isActive() )
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                }
            }

            if( pT->state() )
            {
                pItem->setIcon( 0, icon);
                pItem->setText( 0, name );
                showInfo( "Success, trigger compiled and is ready for activation. You can activate it with the padlock icon." );
            }
            else
            {
                QIcon iconError;
                iconError.addPixmap(QPixmap(QString::fromUtf8(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
                pItem->setIcon( 0, iconError );
                showError( pT->getError() );
                pItem->setText( 0, name );
            }
        }
    }
    mpHost->getActionUnit()->updateToolbar();*/
}

void dlgTriggerEditor::saveAction()
{
    QTreeWidgetItem * pItem = mCurrentAction;
    if( ! pItem ) return;

    QString name = mpActionsMainArea->lineEdit_action_name->text();
    //QString cmdDown = mpActionsMainArea->lineEdit_action_button_down->text();
    //QString cmdUp = mpActionsMainArea->lineEdit_action_button_up->text();
    QString icon = mpActionsMainArea->lineEdit_action_icon->text();
    QString script = mpSourceEditorArea->editor->toPlainText();
    //QColor color = mpActionsMainArea->pushButton_color->palette().color(QPalette::Button);
//    int sizeX = mpActionsMainArea->buttonSizeX->text().toInt();
//    int sizeY = mpActionsMainArea->buttonSizeY->text().toInt();
//    int posX = mpActionsMainArea->buttonPosX->text().toInt();
//    int posY = mpActionsMainArea->buttonPosY->text().toInt();
    int rotation = mpActionsMainArea->buttonRotation->currentIndex();
    int columns = mpActionsMainArea->buttonColumns->text().toInt();
    //bool flatButton = mpActionsMainArea->buttonFlat->isChecked();
    bool isChecked = mpActionsMainArea->checkBox_pushdownbutton->isChecked();
    // bottom location is no longer supported i.e. location = 1 = 0 = location top
    int location = mpActionsMainArea->comboBox_location->currentIndex();
    if( location > 0 ) location++;

    int orientation = mpActionsMainArea->comboBox_orientation->currentIndex();
    bool useCustomLayout = false;//mpActionsMainArea->useCustomLayout->isChecked();
    if( pItem )
    {
        int triggerID = pItem->data(0, Qt::UserRole).toInt();
        TAction * pT = mpHost->getActionUnit()->getAction( triggerID );
        if( pT )
        {
            pT->setName( name );
            //pT->setCommandButtonDown( cmdDown );
            //pT->setCommandButtonUp( cmdUp );
            pT->setIcon( icon );
            pT->setScript( script );
            pT->setIsPushDownButton( isChecked );
            pT->mLocation = location;
            pT->mOrientation = orientation;
            pT->setIsActive( pT->shouldBeActive() );
            // pT->setButtonColor( color );
            pT->setButtonRotation( rotation );
            pT->setButtonColumns( columns );
      //      pT->setButtonFlat( flatButton );
            pT->mUseCustomLayout = false;//useCustomLayout;
//            pT->mPosX = posX;
//            pT->mPosY = posY;
//            pT->mSizeX = sizeX;
//            pT->mSizeY = sizeY;
            pT->css = mpActionsMainArea->css->toPlainText();
            QIcon icon;
            if( pT->isFolder() )
            {
                if( ! pT->mPackageName.isEmpty() )
                {
                    if( pT->isActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
                else if( ! pT->getParent() )
                {
                    if( pT->isActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-yellow.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-yellow-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
                else if( ! pT->getParent()->mPackageName.isEmpty() )
                {
                    if( pT->isActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-yellow.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-yellow-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
                else
                {
                    if( pT->isActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-cyan.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-cyan-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            else
            {
                if( pT->isActive() )
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
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
                iconError.addPixmap(QPixmap(QString::fromUtf8(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
                pItem->setIcon( 0, iconError );
                pItem->setText( 0, name );
            }
        }
    }
    mpHost->getActionUnit()->updateToolbar();
    mudlet::self()->processEventLoopHack();
}


void dlgTriggerEditor::slot_saveScriptAfterEdit()
{
    return saveScript();

    /*QString name = mpScriptsMainArea->lineEdit_scripts_name->text();
    QString script = mpSourceEditorArea->script_scintilla->text();

    QList<QListWidgetItem*> itemList;
    for( int i=0; i<mpScriptsMainArea->listWidget_registered_event_handlers->count(); i++ )
    {
        QListWidgetItem * pItem = mpScriptsMainArea->listWidget_registered_event_handlers->item(i);
        itemList << pItem;
    }
    QStringList handlerList;
    for( int i=0; i<itemList.size(); i++ )
    {
        if( itemList[i]->text().size() < 1 ) continue;
        handlerList << itemList[i]->text();
    }

    QTreeWidgetItem * pItem = treeWidget_scripts->currentItem();
    if( pItem )
    {
        int triggerID = pItem->data(0, Qt::UserRole).toInt();
        TScript * pT = mpHost->getScriptUnit()->getScript( triggerID );
        if( pT )
        {
            pT->setName( name );
            pT->setEventHandlerList( handlerList );
            pT->setScript( script );

            pT->setIsActive( true );//FIXME: discuss if new triggers are automatically active
            pT->compile();
            QIcon icon;
            if( pT->isFolder() )
            {
                if( pT->isActive() )
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-orange.png")), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-orange-locked.png")), QIcon::Normal, QIcon::Off);
                }
            }
            else
            {
                if( pT->isActive() )
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                }
            }

            if( pT->state() )
            {
                pItem->setIcon( 0, icon);
                pItem->setText( 0, name );
                showInfo( "Success, trigger compiled and is ready for activation. You can activate it with the padlock icon." );
            }
            else
            {
                QIcon iconError;
                iconError.addPixmap(QPixmap(QString::fromUtf8(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
                pItem->setIcon( 0, iconError );
                showError( pT->getError() );
                pItem->setText( 0, name );
            }
        }
    }*/
}

void dlgTriggerEditor::saveScript()
{
    QTreeWidgetItem * pItem = mCurrentScript;
    if( ! pItem ) return;

    QString old_name;
    QString name = mpScriptsMainArea->lineEdit_scripts_name->text();
    QString script = mpSourceEditorArea->editor->toPlainText();
    mpScriptsMainAreaEditHandlerItem = 0;
    QList<QListWidgetItem*> itemList;
    for( int i=0; i<mpScriptsMainArea->listWidget_registered_event_handlers->count(); i++ )
    {
        QListWidgetItem * pItem = mpScriptsMainArea->listWidget_registered_event_handlers->item(i);
        itemList << pItem;
    }
    QStringList handlerList;
    for( int i=0; i<itemList.size(); i++ )
    {
        if( itemList[i]->text().size() < 1 ) continue;
        handlerList << itemList[i]->text();
    }

    if( pItem )
    {
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
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
                else if( pT->isActive() )
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-orange.png")), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-orange-locked.png")), QIcon::Normal, QIcon::Off);
                }
            }
            else
            {
                if( pT->isActive() )
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                }
            }

            if( pT->state() )
            {
                if( old_name == "New Script" || old_name == "New Script Group" )
                {
                    QIcon _icon;
                    if( pT->isFolder() )
                    {
                        _icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-orange.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        _icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
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
                iconError.addPixmap(QPixmap(QString::fromUtf8(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
                pItem->setIcon( 0, iconError );
                pItem->setText( 0, name );
            }
        }
    }
}


void dlgTriggerEditor::slot_saveKeyAfterEdit()
{
    return saveKey();

    /*bool state = true;
    QString name = mpKeysMainArea->lineEdit_name->text();
    if( name.size() < 1 )
    {
        name = mpKeysMainArea->lineEdit_key->text();
    }
    QString command = mpKeysMainArea->lineEdit_command->text();
    QString script = mpSourceEditorArea->script_scintilla->text();
    QTreeWidgetItem * pItem = treeWidget_keys->currentItem();
    if( pItem )
    {
        int triggerID = pItem->data(0, Qt::UserRole).toInt();
        TKey * pT = mpHost->getKeyUnit()->getKey( triggerID );
        if( pT )
        {
            pItem->setText(0,name );
            pT->setName( name );
            pT->setCommand( command );
            pT->setScript( script );
            pT->setIsActive( true );

            QIcon icon;
            if( pT->isFolder() )
            {
                if( pT->isActive() )
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-violet.png")), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-violet-locked.png")), QIcon::Normal, QIcon::Off);
                }
            }
            else
            {
                if( pT->isActive() )
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                }
            }

            if( pT->state() )
            {
                pItem->setIcon( 0, icon);
                pItem->setText( 0, name );
                showInfo( "Success, trigger compiled and is ready for activation. You can activate it with the padlock icon." );
            }
            else
            {
                QIcon iconError;
                iconError.addPixmap(QPixmap(QString::fromUtf8(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
                pItem->setIcon( 0, iconError );
                showError( pT->getError() );
                pItem->setText( 0, name );
            }
        }
    }*/
}

void dlgTriggerEditor::saveKey()
{
    QTreeWidgetItem * pItem = mCurrentKey;
    if( ! pItem ) return;

    QString name = mpKeysMainArea->lineEdit_name->text();
    if( name.size() < 1 )
    {
        name = mpKeysMainArea->lineEdit_key->text();
    }
    QString command = mpKeysMainArea->lineEdit_command->text();
    QString script = mpSourceEditorArea->editor->toPlainText();
    if( pItem )
    {
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
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
                else if( pT->isActive() )
                {
                    if( pT->ancestorsActive() )
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-violet.png")), QIcon::Normal, QIcon::Off);
                    else
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    if( pT->ancestorsActive() )
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-violet-locked.png")), QIcon::Normal, QIcon::Off);
                    else
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                }
            }
            else
            {
                if( pT->isActive() )
                {
                    if( pT->ancestorsActive() )
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    else
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    if( pT->ancestorsActive() )
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    else
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
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
                iconError.addPixmap(QPixmap(QString::fromUtf8(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
                pItem->setIcon( 0, iconError );
                pItem->setText( 0, name );
            }
        }
    }
}


void dlgTriggerEditor::slot_deleteProfile()
{
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
        palette.setColor( QPalette::Text, QColor(0,0,0) );
        pItem->lineEdit->show();
        pItem->fgB->hide();
        pItem->bgB->hide();
        break;
    case 1:
        palette.setColor( QPalette::Text, QColor(0,0,255) );
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
    //pItem->setPalette( palette );
    pItem->lineEdit->setPalette( palette );
}

void dlgTriggerEditor::slot_trigger_clicked( QTreeWidgetItem *pItem, int column )
{
    QTime t;
    if( ! pItem ) return;

    mCurrentTrigger = pItem;
    mpTriggersMainArea->show();
    mpSourceEditorArea->show();
    mpSystemMessageArea->hide();
    mpTriggersMainArea->lineEdit_trigger_name->setText("");
    mpSourceEditorArea->editor->setPlainText( "" );
    mpTriggersMainArea->checkBox_multlinetrigger->setChecked( false );
    mpTriggersMainArea->perlSlashGOption->setChecked( false );
    mpTriggersMainArea->filterTrigger->setChecked( false );
    mpTriggersMainArea->spinBox_linemargin->setValue( 1 );
    if( (pItem == 0) || (column != 0) ) return;
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
                palette.setColor( QPalette::Text, QColor(0,0,0) );
                pBox->setCurrentIndex( 0 );
                pItem->fgB->hide();
                pItem->bgB->hide();
                pItem->lineEdit->show();
                break;
            case REGEX_PERL:
                palette.setColor( QPalette::Text, QColor(0,0,255) );
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
                QColor fgC = QColor(pT->mColorPatternList[i]->fgR,
                                    pT->mColorPatternList[i]->fgG,
                                    pT->mColorPatternList[i]->fgB);
                fgPal.setColor( QPalette::Button, fgC );
                QString fgstyleSheet = QString("QPushButton{background-color:")+fgC.name()+QString(";}");
                QColor bgC = QColor(pT->mColorPatternList[i]->bgR,
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
        mpSourceEditorArea->editor->setPlainText( script );

        /*mpTriggersMainArea->colorTrigger->setChecked( pT->mColorTrigger );
        if( pT->mColorTriggerBg )
        {
            QPalette palette;
            QColor color = pT->mColorTriggerBgColor;
            palette.setColor( QPalette::Button, color );
            QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
            mpTriggersMainArea->colorTriggerBg->setStyleSheet( styleSheet );
        }
        if( pT->mColorTriggerFg )
        {
            QPalette palette;
            QColor color = pT->mColorTriggerFgColor;
            palette.setColor( QPalette::Button, color );
            QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
            mpTriggersMainArea->colorTriggerFg->setStyleSheet( styleSheet );
        }*/

        if( ! pT->state() ) showError( pT->getError() );
    }
}

void dlgTriggerEditor::slot_alias_clicked( QTreeWidgetItem *pItem, int column )
{
    if( ! pItem ) return;
    mCurrentAlias = pItem;
    mpAliasMainArea->show();
    mpSourceEditorArea->show();
    mpSystemMessageArea->hide();
    mpAliasMainArea->lineEdit_alias_name->clear();
    mpAliasMainArea->pattern_textedit->clear();
    mpAliasMainArea->substitution->clear();
    mpSourceEditorArea->editor->setPlainText( "" );

    if( (pItem == 0) || (column != 0) )
    {
        return;
    }
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
        mpSourceEditorArea->editor->setPlainText( script );
        if( ! pT->state() ) showError( pT->getError() );
    }
}

void dlgTriggerEditor::slot_key_clicked( QTreeWidgetItem *pItem, int column )
{
    if( ! pItem ) return;
    mCurrentKey = pItem;
    mpKeysMainArea->show();
    mpSourceEditorArea->show();
    mpSystemMessageArea->hide();
    mpKeysMainArea->lineEdit_command->clear();
    mpKeysMainArea->lineEdit_key->clear();
    mpKeysMainArea->lineEdit_name->clear();
    mpSourceEditorArea->editor->setPlainText( "" );
    if( (pItem == 0) || (column != 0) )
    {
        return;
    }
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
        mpSourceEditorArea->editor->setPlainText( script );
        if( ! pT->state() ) showError( pT->getError() );
    }
}

void dlgTriggerEditor::slot_action_clicked( QTreeWidgetItem *pItem, int column )
{
    if( ! pItem ) return;
    mCurrentAction = pItem;
    mpActionsMainArea->show();
    mpSourceEditorArea->editor->show();

    mpSystemMessageArea->hide();
    //mpActionsMainArea->lineEdit_action_button_down->clear();
    mpSourceEditorArea->editor->setPlainText( "" );

    //mpActionsMainArea->lineEdit_action_button_up->clear();
    mpActionsMainArea->lineEdit_action_icon->clear();
    mpActionsMainArea->lineEdit_action_name->clear();
    mpActionsMainArea->checkBox_pushdownbutton->setChecked( false );
    mpActionsMainArea->buttonColumns->clear();
    //mpActionsMainArea->isLabel->setChecked(false);
    //mpActionsMainArea->useCustomLayout->setChecked(false);
//    mpActionsMainArea->buttonPosX->setText("0");
//    mpActionsMainArea->buttonPosY->setText("0");
//    mpActionsMainArea->buttonSizeX->setText("80");
//    mpActionsMainArea->buttonSizeY->setText("25");
    //mpActionsMainArea->buttonFlat->setChecked(false);
    //mpActionsMainArea->isLabel->setChecked(false);
    //mpActionsMainArea->useCustomLayout->setChecked(false);
    mpActionsMainArea->css->clear();
    if( (pItem == 0) || (column != 0) )
    {
        return;
    }
    mpCurrentActionItem = pItem; //remember what has been clicked to save it
    int ID = pItem->data(0,Qt::UserRole).toInt();
    TAction * pT = mpHost->getActionUnit()->getAction(ID);
    if( pT )
    {
        mpActionsMainArea->lineEdit_action_name->setText( pT->getName() );
        mpActionsMainArea->checkBox_pushdownbutton->setChecked( pT->isPushDownButton() );
        //mpActionsMainArea->lineEdit_action_button_down->setText( pT->getCommandButtonDown() );
        //mpActionsMainArea->lineEdit_action_button_up->setText( pT->getCommandButtonUp() );
        mpActionsMainArea->lineEdit_action_icon->setText( pT->getIcon() );
        mpSourceEditorArea->editor->setPlainText( pT->getScript() );
        // location = 1 = location = bottom is no longer supported
        int location = pT->mLocation;
        if( location > 0 ) location--;
        mpActionsMainArea->comboBox_location->setCurrentIndex( location );
        mpActionsMainArea->comboBox_orientation->setCurrentIndex( pT->mOrientation );
        QColor color = pT->getButtonColor();
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        //mpActionsMainArea->pushButton_color->setPalette( palette );
        mpActionsMainArea->buttonRotation->setCurrentIndex( pT->getButtonRotation() );
        mpActionsMainArea->buttonColumns->setValue( pT->getButtonColumns() );
      //  mpActionsMainArea->buttonFlat->setChecked( pT->getButtonFlat() );
//        mpActionsMainArea->buttonSizeX->setText(QString::number(pT->getSizeX()) );
//        mpActionsMainArea->buttonSizeY->setText(QString::number(pT->getSizeY()) );
        //mpActionsMainArea->isLabel->setChecked( pT->mIsLabel );
        //mpActionsMainArea->useCustomLayout->setChecked( pT->mUseCustomLayout );
//        mpActionsMainArea->buttonPosX->setText( QString::number(pT->mPosX) );
//        mpActionsMainArea->buttonPosY->setText( QString::number(pT->mPosY) );
//        mpActionsMainArea->buttonSizeX->setText( QString::number(pT->mSizeX) );
//        mpActionsMainArea->buttonSizeY->setText( QString::number(pT->mSizeY) );
        mpActionsMainArea->css->clear();
        mpActionsMainArea->css->setPlainText( pT->css );
        if( ! pT->getParent() )
        {
            mpActionsMainArea->groupBox_toolBar->show();
            mpActionsMainArea->groupBox_appearance->hide();
        }
        else
        {
            mpActionsMainArea->groupBox_toolBar->hide();
            mpActionsMainArea->groupBox_appearance->show();
        }
        if( ! pT->state() ) showError( pT->getError() );
    }
}


void dlgTriggerEditor::slot_scripts_clicked( QTreeWidgetItem *pItem, int column )
{
    if( ! pItem ) return;
    mCurrentScript = pItem;
    mpScriptsMainArea->show();
    mpSourceEditorArea->show();
    mpSystemMessageArea->hide();
    mpSourceEditorArea->editor->setPlainText( "" );
    mpScriptsMainArea->lineEdit_scripts_name->clear();
    mpScriptsMainArea->listWidget_registered_event_handlers->clear();
    if( (pItem == 0) || (column != 0) )
    {
        return;
    }
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
            QListWidgetItem * pItem = new QListWidgetItem( mpScriptsMainArea->listWidget_registered_event_handlers );
            pItem->setText( eventHandlerList[i] );
            mpScriptsMainArea->listWidget_registered_event_handlers->addItem( pItem );
        }
        mpScriptsMainArea->lineEdit_scripts_name->clear();
        QString script = pT->getScript();
        mpSourceEditorArea->editor->setPlainText( script );
        mpScriptsMainArea->lineEdit_scripts_name->setText( name );
        if( ! pT->state() ) showError( pT->getError() );
    }
}

void dlgTriggerEditor::slot_timer_clicked( QTreeWidgetItem *pItem, int column )
{
    if( ! pItem ) return;
    mCurrentTimer = pItem;
    mpTimersMainArea->show();
    mpSourceEditorArea->show();
    mpSystemMessageArea->hide();
    mpSourceEditorArea->editor->setPlainText( "" );
    mpTimersMainArea->lineEdit_command->clear();
    mpTimersMainArea->lineEdit_timer_name->clear();
    mpTimersMainArea->timeEdit_hours->clear();
    mpTimersMainArea->timeEdit_minutes->clear();
    mpTimersMainArea->timeEdit_seconds->clear();
    mpTimersMainArea->timeEdit_msecs->clear();
    if( (pItem == 0) || (column != 0) )
    {
        return;
    }
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
        mpSourceEditorArea->editor->setPlainText( script );
        if( ! pT->state() ) showError( pT->getError() );
    }
}


void dlgTriggerEditor::fillout_form()
{
    mCurrentView = 0;
    mCurrentTrigger = 0;
    mCurrentAlias = 0;
    mCurrentKey = 0;
    mCurrentAction = 0;
    mCurrentScript = 0;
    mCurrentTimer = 0;

    mNeedUpdateData = false;
    QStringList sL;
    sL << "Triggers";
    mpTriggerBaseItem = new QTreeWidgetItem( (QTreeWidgetItem*)0, sL );
    mpTriggerBaseItem->setBackground(0,QColor(255,254,215,255));
    QIcon mainIcon;
    mainIcon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tools-wizard.png")), QIcon::Normal, QIcon::Off);
    mpTriggerBaseItem->setIcon( 0, mainIcon );
    treeWidget->insertTopLevelItem( 0, mpTriggerBaseItem );
    list<TTrigger *> baseNodeList = mpHost->getTriggerUnit()->getTriggerRootNodeList();
    typedef list<TTrigger *>::iterator IT;
    for(IT it=baseNodeList.begin(); it!=baseNodeList.end(); it++ )
    {
        TTrigger * pT = *it;
        if( pT->isTempTrigger() ) continue;
        QString s = pT->getName();
        QStringList sList;
        sList << s;
        QTreeWidgetItem * pItem = new QTreeWidgetItem( mpTriggerBaseItem, sList);
        pItem->setData( 0, Qt::UserRole, QVariant(pT->getID()) );
        mpTriggerBaseItem->addChild( pItem );
        QIcon icon;
        if( pT->hasChildren() )
        {
            expand_child_triggers( pT, (QTreeWidgetItem*)pItem );
        }
        if( pT->state() )
        {
            if( pT->isFilterChain() )
            {
                if( pT->isActive() )
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/filter.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/filter-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
                else
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/filter-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/filter-grey-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            else if( pT->isFolder() )
            {
                if( ! pT->mPackageName.isEmpty() )
                {
                    if( pT->isActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
                else if( pT->isActive() )
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-blue.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
                else
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-blue-locked.png"),0,Qt::MonoOnly), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-grey-locked.png"),0,Qt::MonoOnly), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            else
            {
                if( pT->isActive() )
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
                else
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            pItem->setIcon(0, icon);
        }
        else
        {
            QIcon iconError;
            iconError.addPixmap(QPixmap(QString::fromUtf8(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            pItem->setIcon( 0, iconError );
            showError( pT->getError() );
        }
    }

    mpTriggerBaseItem->setExpanded( true );

    QStringList sL2;
    sL2 << "Timers";
    mpTimerBaseItem = new QTreeWidgetItem( (QTreeWidgetItem*)0, sL2 );
    mpTimerBaseItem->setBackground(0,QColor(255,254,215,255));
    QIcon mainIcon2;
    mainIcon2.addPixmap(QPixmap(QString::fromUtf8(":/icons/chronometer.png")), QIcon::Normal, QIcon::Off);
    mpTimerBaseItem->setIcon( 0, mainIcon2 );
    treeWidget_timers->insertTopLevelItem( 0, mpTimerBaseItem );
    mpTriggerBaseItem->setExpanded( true );
    list<TTimer *> baseNodeList_timers = mpHost->getTimerUnit()->getTimerRootNodeList();

    for( list<TTimer *>::iterator it = baseNodeList_timers.begin(); it!=baseNodeList_timers.end(); it++ )
    {
        TTimer * pT = *it;
        if( pT->isTempTimer() ) continue;
        QString s = pT->getName();
        qDebug()<<"init: name="<<pT->getName()<<" mUserActiveState="<<pT->shouldBeActive();
        QStringList sList;
        sList << s;
        QTreeWidgetItem * pItem = new QTreeWidgetItem( mpTimerBaseItem, sList);
        pItem->setData( 0, Qt::UserRole, QVariant(pT->getID()) );
        mpTimerBaseItem->addChild( pItem );
        QIcon icon;
        if( pT->hasChildren() )
        {
            expand_child_timers( pT, (QTreeWidgetItem*)pItem );
        }
        if( pT->state() )
        {
            if( pT->isFolder() )
            {
                if( ! pT->mPackageName.isEmpty() )
                {
                    if( pT->isActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
                else if( pT->shouldBeActive() )
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-green.png")), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-green-locked.png")), QIcon::Normal, QIcon::Off);
                }
            }
            else
            {
                if( pT->isOffsetTimer() )
                {
                    if( pT->shouldBeActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/offsettimer-on.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/offsettimer-off.png")), QIcon::Normal, QIcon::Off);
                    }
                }
                else
                {
                    if( pT->shouldBeActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    }
                }

            }
            pItem->setIcon(0, icon);
        }
        else
        {
            QIcon iconError;
            iconError.addPixmap(QPixmap(QString::fromUtf8(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            pItem->setIcon( 0, iconError );
            showError( pT->getError() );
        }
    }
    mpTimerBaseItem->setExpanded( true );

    QStringList sL3;
    sL3 << "Scripts";
    mpScriptsBaseItem = new QTreeWidgetItem( (QTreeWidgetItem*)0, sL3 );
    mpScriptsBaseItem->setBackground(0,QColor(255,254,215,255));
    QIcon mainIcon3;
    mainIcon3.addPixmap(QPixmap(QString::fromUtf8(":/icons/accessories-text-editor.png")), QIcon::Normal, QIcon::Off);
    mpScriptsBaseItem->setIcon( 0, mainIcon3 );
    treeWidget_scripts->insertTopLevelItem( 0, mpScriptsBaseItem );
    mpScriptsBaseItem->setExpanded( true );
    list<TScript *> baseNodeList_scripts = mpHost->getScriptUnit()->getScriptRootNodeList();

    for( list<TScript *>::iterator it = baseNodeList_scripts.begin(); it!=baseNodeList_scripts.end(); it++ )
    {
        TScript * pT = *it;
        QString s = pT->getName();

        QStringList sList;
        sList << s;
        QTreeWidgetItem * pItem = new QTreeWidgetItem( mpScriptsBaseItem, sList);
        pItem->setData( 0, Qt::UserRole, QVariant(pT->getID()) );
        mpScriptsBaseItem->addChild( pItem );
        QIcon icon;
        if( pT->hasChildren() )
        {
            expand_child_scripts( pT, (QTreeWidgetItem*)pItem );
        }
        if( pT->state() )
        {
            if( pT->isFolder() )
            {
                if( ! pT->mPackageName.isEmpty() )
                {
                    if( pT->isActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
                else if( pT->isActive() )
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-orange.png")), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-orange-locked.png")), QIcon::Normal, QIcon::Off);
                }
            }
            else
            {
                if( pT->isActive() )
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                }
            }
            pItem->setIcon(0, icon);
        }
        else
        {
            QIcon iconError;
            iconError.addPixmap(QPixmap(QString::fromUtf8(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            pItem->setIcon( 0, iconError );
            showError( pT->getError() );
        }
    }
    mpScriptsBaseItem->setExpanded( true );

    QStringList sL4;
    sL4 << "Aliases - Input Triggers";
    mpAliasBaseItem = new QTreeWidgetItem( (QTreeWidgetItem*)0, sL4 );
    mpAliasBaseItem->setBackground(0,QColor(255,254,215,255));
    QIcon mainIcon4;
    mainIcon4.addPixmap(QPixmap(QString::fromUtf8(":/icons/system-users.png")), QIcon::Normal, QIcon::Off);
    mpAliasBaseItem->setIcon( 0, mainIcon4 );
    treeWidget_alias->insertTopLevelItem( 0, mpAliasBaseItem );
    mpAliasBaseItem->setExpanded( true );
    list<TAlias *> baseNodeList_alias = mpHost->getAliasUnit()->getAliasRootNodeList();

    for( list<TAlias *>::iterator it = baseNodeList_alias.begin(); it!=baseNodeList_alias.end(); it++ )
    {
        TAlias * pT = *it;
        QString s = pT->getName();
        //        TTimer * pTimer = *it;
        QStringList sList;
        sList << s;
        QTreeWidgetItem * pItem = new QTreeWidgetItem( mpAliasBaseItem, sList);
        pItem->setData( 0, Qt::UserRole, QVariant(pT->getID()) );
        mpAliasBaseItem->addChild( pItem );
        QIcon icon;
        if( pT->hasChildren() )
        {
            expand_child_alias( pT, (QTreeWidgetItem*)pItem );
        }
        if( pT->state() )
        {
            if( pT->isFolder() )
            {
                if( ! pT->mPackageName.isEmpty() )
                {
                    if( pT->isActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
                else if( pT->isActive() )
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-violet.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
                else
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-violet-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            else
            {
                if( pT->isActive() )
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
                else
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            pItem->setIcon(0, icon);
        }
        else
        {
            QIcon iconError;
            iconError.addPixmap(QPixmap(QString::fromUtf8(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            pItem->setIcon( 0, iconError );
            showError( pT->getError() );
        }
    }
    mpAliasBaseItem->setExpanded( true );

    QStringList sL5;
    sL5 << "Buttons";
    mpActionBaseItem = new QTreeWidgetItem( (QTreeWidgetItem*)0, sL5 );
    mpActionBaseItem->setBackground(0,QColor(255,254,215,255));
    QIcon mainIcon5;
    mainIcon5.addPixmap(QPixmap(QString::fromUtf8(":/icons/bookmarks.png")), QIcon::Normal, QIcon::Off);
    mpActionBaseItem->setIcon( 0, mainIcon5 );
    treeWidget_actions->insertTopLevelItem( 0, mpActionBaseItem );
    mpActionBaseItem->setExpanded( true );
    list<TAction *> baseNodeList_action = mpHost->getActionUnit()->getActionRootNodeList();

    for( list<TAction *>::iterator it = baseNodeList_action.begin(); it!=baseNodeList_action.end(); it++ )
    {
        TAction * pT = *it;
        QString s = pT->getName();
        QStringList sList;
        sList << s;
        QTreeWidgetItem * pItem = new QTreeWidgetItem( mpActionBaseItem, sList);
        pItem->setData( 0, Qt::UserRole, QVariant(pT->getID()) );
        mpActionBaseItem->addChild( pItem );
        QIcon icon;
        if( pT->hasChildren() )
        {
            expand_child_action( pT, (QTreeWidgetItem*)pItem );
        }
        if( pT->state() )
        {
            if( pT->isFolder() )
            {
                if( ! pT->mPackageName.isEmpty() )
                {
                    if( pT->isActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
                else if( ! pT->getParent() )
                {
                    if( pT->isActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-yellow.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-yellow-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
                else if( ! pT->getParent()->mPackageName.isEmpty() )
                {
                    if( pT->isActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-yellow.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-yellow-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
                else
                {
                    if( pT->isActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-cyan.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-cyan-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            else
            {
                if( pT->isActive() )
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                }
            }
            pItem->setIcon(0, icon);
        }
        else
        {
            QIcon iconError;
            iconError.addPixmap(QPixmap(QString::fromUtf8(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            pItem->setIcon( 0, iconError );
            showError( pT->getError() );
        }
    }
    mpActionBaseItem->setExpanded( true );

    QStringList sL6;
    sL5 << "Action Key Bindings";
    mpKeyBaseItem = new QTreeWidgetItem( (QTreeWidgetItem*)0, sL6 );
    mpKeyBaseItem->setBackground(0,QColor(255,254,215,255));
    QIcon mainIcon6;
    mainIcon6.addPixmap(QPixmap(QString::fromUtf8(":/icons/preferences-desktop-keyboard.png")), QIcon::Normal, QIcon::Off);
    mpKeyBaseItem->setIcon( 0, mainIcon6 );
    treeWidget_keys->insertTopLevelItem( 0, mpKeyBaseItem );
    mpKeyBaseItem->setExpanded( true );
    list<TKey *> baseNodeList_key = mpHost->getKeyUnit()->getKeyRootNodeList();

    for( list<TKey *>::iterator it = baseNodeList_key.begin(); it!=baseNodeList_key.end(); it++ )
    {
        TKey * pT = *it;
        QString s = pT->getName();
        QStringList sList;
        sList << s;
        QTreeWidgetItem * pItem = new QTreeWidgetItem( mpKeyBaseItem, sList);
        pItem->setData( 0, Qt::UserRole, QVariant(pT->getID()) );
        mpKeyBaseItem->addChild( pItem );
        QIcon icon;
        if( pT->hasChildren() )
        {
            expand_child_key( pT, (QTreeWidgetItem*)pItem );
        }
        if( pT->state() )
        {
            if( pT->isFolder() )
            {
                if( ! pT->mPackageName.isEmpty() )
                {
                    if( pT->isActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-brown.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-brown-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
                else if( pT->isActive() )
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-cyan.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
                else
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-cyan-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            else
            {
                if( pT->isActive() )
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
                else
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            pItem->setIcon(0, icon);
        }
        else
        {
            QIcon iconError;
            iconError.addPixmap(QPixmap(QString::fromUtf8(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            pItem->setIcon( 0, iconError );
            showError( pT->getError() );
        }
    }
    mpKeyBaseItem->setExpanded( true );
}


void dlgTriggerEditor::expand_child_triggers( TTrigger * pTriggerParent, QTreeWidgetItem * pWidgetItemParent )
{
    list<TTrigger *> * childrenList = pTriggerParent->getChildrenList();
    typedef list<TTrigger *>::iterator I;
    for( I it=childrenList->begin(); it!=childrenList->end(); it++ )
    {
        TTrigger * pT = *it;
        QString s = pT->getName();
        QStringList sList;
        sList << s;
        QTreeWidgetItem * pItem = new QTreeWidgetItem( pWidgetItemParent, sList);
        pItem->setData( 0, Qt::UserRole, pT->getID() );

        pWidgetItemParent->insertChild( 0, pItem );
        QIcon icon;
        if( pT->hasChildren() )
        {
            expand_child_triggers( pT, pItem );
        }
        if( pT->state() )
        {
            if( pT->isFilterChain() )
            {
                if( pT->isActive() )
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/filter.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/filter-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
                else
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/filter-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/filter-grey-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            else if( pT->isFolder() )
            {
                if( pT->isActive() )
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-blue.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                    }
                }
                else
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-blue-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            else
            {
                if( pT->isActive() )
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                    }

                }
                else
                {
                    if( pT->ancestorsActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox.png"),0,Qt::MonoOnly), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox-grey.png"),0,Qt::MonoOnly), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            //pItem->setDisabled(!pT->ancestorsActive());
            pItem->setIcon(0, icon);
        }
        else
        {
            QIcon iconError;
            iconError.addPixmap(QPixmap(QString::fromUtf8(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            pItem->setIcon( 0, iconError );
            showError( pT->getError() );
        }
    }

}

void dlgTriggerEditor::expand_child_key( TKey * pTriggerParent, QTreeWidgetItem * pWidgetItemParent )
{
    list<TKey *> * childrenList = pTriggerParent->getChildrenList();
    typedef list<TKey *>::iterator I;
    for( I it=childrenList->begin(); it!=childrenList->end(); it++ )
    {
        TKey * pT = *it;
        QString s = pT->getName();
        QStringList sList;
        sList << s;
        QTreeWidgetItem * pItem = new QTreeWidgetItem( pWidgetItemParent, sList);
        pItem->setData( 0, Qt::UserRole, pT->getID() );

        pWidgetItemParent->insertChild( 0, pItem );
        QIcon icon;
        if( pT->hasChildren() )
        {
            expand_child_key( pT, pItem );
        }
        if( pT->state() )
        {
            if( pT->isFolder() )
            {
                if( pT->isActive() )
                {
                    if( pT->ancestorsActive() )
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-blue.png")), QIcon::Normal, QIcon::Off);
                    else
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    if( pT->ancestorsActive() )
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-blue-locked.png")), QIcon::Normal, QIcon::Off);
                    else
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                }
            }
            else
            {
                if( pT->isActive() )
                {
                    if( pT->ancestorsActive() )
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    else
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    if( pT->ancestorsActive() )
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    else
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                }
            }
            pItem->setIcon(0, icon);
        }
        else
        {
            QIcon iconError;
            iconError.addPixmap(QPixmap(QString::fromUtf8(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            pItem->setIcon( 0, iconError );
            showError( pT->getError() );
        }
    }
}


void dlgTriggerEditor::expand_child_scripts( TScript * pTriggerParent, QTreeWidgetItem * pWidgetItemParent )
{
    list<TScript *> * childrenList = pTriggerParent->getChildrenList();
    typedef list<TScript *>::iterator I;
    for( I it=childrenList->begin(); it!=childrenList->end(); it++ )
    {
        TScript * pT = *it;
        QString s = pT->getName();
        QStringList sList;
        sList << s;
        QTreeWidgetItem * pItem = new QTreeWidgetItem( pWidgetItemParent, sList);
        pItem->setData( 0, Qt::UserRole, pT->getID() );

        pWidgetItemParent->insertChild( 0, pItem );
        QIcon icon;
        if( pT->hasChildren() )
        {
            expand_child_scripts( pT, pItem );
        }
        if( pT->state() )
        {
            if( pT->isFolder() )
            {
                if( pT->isActive() )
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-orange.png")), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-orange-locked.png")), QIcon::Normal, QIcon::Off);
                }
            }
            else
            {
                if( pT->isActive() )
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                }
            }
            pItem->setIcon(0, icon);
        }
        else
        {
            QIcon iconError;
            iconError.addPixmap(QPixmap(QString::fromUtf8(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            pItem->setIcon( 0, iconError );
            showError( pT->getError() );
        }
    }
}

void dlgTriggerEditor::expand_child_alias( TAlias * pTriggerParent, QTreeWidgetItem * pWidgetItemParent )
{
    list<TAlias *> * childrenList = pTriggerParent->getChildrenList();
    typedef list<TAlias *>::iterator I;
    for( I it=childrenList->begin(); it!=childrenList->end(); it++ )
    {
        TAlias * pT = *it;
        QString s = pT->getName();
        QStringList sList;
        sList << s;
        QTreeWidgetItem * pItem = new QTreeWidgetItem( pWidgetItemParent, sList);
        pItem->setData( 0, Qt::UserRole, pT->getID() );

        pWidgetItemParent->insertChild( 0, pItem );
        QIcon icon;
        if( pT->hasChildren() )
        {
            expand_child_alias( pT, pItem );
        }
        if( pT->state() )
        {
            if( pT->isFolder() )
            {
                if( pT->isActive() )
                {
                    if( pT->ancestorsActive() )
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-violet.png")), QIcon::Normal, QIcon::Off);
                    else
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-grey.png")), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    if( pT->ancestorsActive() )
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-violet-locked.png")), QIcon::Normal, QIcon::Off);
                    else
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-grey-locked.png")), QIcon::Normal, QIcon::Off);
                }
            }
            else
            {
                if( pT->isActive() )
                {
                    if( pT->ancestorsActive() )
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    else
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked_grey.png")), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    if( pT->ancestorsActive() )
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    else
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox-grey.png")), QIcon::Normal, QIcon::Off);
                }
            }
            pItem->setIcon(0, icon);
        }
        else
        {
            QIcon iconError;
            iconError.addPixmap(QPixmap(QString::fromUtf8(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            pItem->setIcon( 0, iconError );
            showError( pT->getError() );
        }
    }
}

void dlgTriggerEditor::expand_child_action( TAction * pTriggerParent, QTreeWidgetItem * pWidgetItemParent )
{
    list<TAction *> * childrenList = pTriggerParent->getChildrenList();
    typedef list<TAction *>::iterator I;
    for( I it=childrenList->begin(); it!=childrenList->end(); it++ )
    {
        TAction * pT = *it;
        QString s = pT->getName();
        QStringList sList;
        sList << s;
        QTreeWidgetItem * pItem = new QTreeWidgetItem( pWidgetItemParent, sList);
        pItem->setData( 0, Qt::UserRole, pT->getID() );

        pWidgetItemParent->insertChild( 0, pItem );
        QIcon icon;
        if( pT->hasChildren() )
        {
            expand_child_action( pT, pItem );
        }
        if( pT->state() )
        {
            if( ! pT->getParent()->mPackageName.isEmpty() )
            {
                if( pT->isActive() )
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-yellow.png")), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-yellow-locked.png")), QIcon::Normal, QIcon::Off);
                }
            }
            else if( pT->isFolder() )
            {
                if( pT->isActive() )
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-cyan.png")), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-cyan-locked.png")), QIcon::Normal, QIcon::Off);
                }
            }
            else
            {
                if( pT->isActive() )
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                }
            }
            pItem->setIcon(0, icon);
        }
        else
        {
            QIcon iconError;
            iconError.addPixmap(QPixmap(QString::fromUtf8(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            pItem->setIcon( 0, iconError );
            showError( pT->getError() );
        }
    }
}


void dlgTriggerEditor::expand_child_timers( TTimer * pTimerParent, QTreeWidgetItem * pWidgetItemParent )
{
    list<TTimer *> * childrenList = pTimerParent->getChildrenList();
    typedef list<TTimer *>::iterator I;
    for( I it=childrenList->begin(); it!=childrenList->end(); it++ )
    {
        TTimer * pT = *it;
        QString s = pT->getName();
        QStringList sList;
        sList << s;
        QTreeWidgetItem * pItem = new QTreeWidgetItem( pWidgetItemParent, sList);
        pItem->setData( 0, Qt::UserRole, pT->getID() );

        pWidgetItemParent->insertChild( 0, pItem );
        QIcon icon;
        if( pT->hasChildren() )
        {
            expand_child_timers( pT, pItem );
        }
        if( pT->state() )
        {
            if( pT->isFolder() )
            {
                if( pT->shouldBeActive() )
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-green.png")), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/folder-green-locked.png")), QIcon::Normal, QIcon::Off);
                }
            }
            else
            {
                if( pT->isOffsetTimer() )
                {
                    if( pT->shouldBeActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/offsettimer-on.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/offsettimer-off.png")), QIcon::Normal, QIcon::Off);
                    }
                }
                else
                {
                    if( pT->shouldBeActive() )
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox_checked.png")), QIcon::Normal, QIcon::Off);
                    }
                    else
                    {
                        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/tag_checkbox.png")), QIcon::Normal, QIcon::Off);
                    }
                }
            }
            pItem->setIcon(0, icon);
        }
        else
        {
            QIcon iconError;
            iconError.addPixmap(QPixmap(QString::fromUtf8(":/icons/tools-report-bug.png")), QIcon::Normal, QIcon::Off);
            pItem->setIcon( 0, iconError );
            showError( pT->getError() );
        }
        qDebug()<<"WARNING: dlgTriggerEditor::expand_child_timers() called name:"<<pT->getName();
//        if( pT->isActive() )
//        {
//            pT->enableTimer();// pT->getName() );
//        }
//        else
//        {
//            pT->disableTimer();// pT->getName() );
//        }
    }
}



void dlgTriggerEditor::slot_connection_dlg_finnished()
{
}

void dlgTriggerEditor::slot_update()
{
    update();
}

void dlgTriggerEditor::slot_show_search_area()
{
    if( mpSearchArea->isVisible() )
    {
        mpSearchArea->hide();
        popupArea->hide();
    }
    else
    {
        mpSearchArea->show();
        popupArea->show();
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
    };

}

void dlgTriggerEditor::enterEvent( QEvent *pE )
{
    if( mNeedUpdateData )
    {
        treeWidget->clear();
        treeWidget_alias->clear();
        treeWidget_timers->clear();
        treeWidget_scripts->clear();
        treeWidget_actions->clear();
        treeWidget_keys->clear();
        fillout_form();
        mNeedUpdateData = false;
    }
    //QWidget::enterEvent( pE );
}

void dlgTriggerEditor::focusInEvent( QFocusEvent * pE )
{
    if( mNeedUpdateData )
    {
        treeWidget->clear();
        treeWidget_alias->clear();
        treeWidget_timers->clear();
        treeWidget_scripts->clear();
        treeWidget_actions->clear();
        treeWidget_keys->clear();
        fillout_form();
        mNeedUpdateData = false;
    }

    if( ! mCurrentView )
    {
        mCurrentTrigger = 0;
        mCurrentAlias = 0;
        mCurrentKey = 0;
        mCurrentAction = 0;
        mCurrentScript = 0;
        mCurrentTimer = 0;
        return;
    }

    if( mCurrentTrigger )
        mCurrentTrigger->setSelected( true );
    if( mCurrentTimer )
        mCurrentTimer->setSelected( true );
    if( mCurrentAlias )
        mCurrentAlias->setSelected( true );
    if( mCurrentScript )
        mCurrentScript->setSelected( true );
    if( mCurrentAction )
        mCurrentAction->setSelected( true );
    if( mCurrentKey )
        mCurrentKey->setSelected( true );

    //QWidget::focusInEvent( pE );
}

void dlgTriggerEditor::focusOutEvent( QFocusEvent * pE )
{
    saveOpenChanges();
}

void dlgTriggerEditor::slot_show_timers()
{

    saveOpenChanges();

    if( mNeedUpdateData )
    {
        treeWidget->clear();
        treeWidget_alias->clear();
        treeWidget_timers->clear();
        treeWidget_scripts->clear();
        treeWidget_actions->clear();
        treeWidget_keys->clear();
        fillout_form();
        mNeedUpdateData = false;
    }

    mCurrentView = cmTimerView;

    mpTriggersMainArea->hide();
    mpTimersMainArea->hide();
    mpScriptsMainArea->hide();
    mpAliasMainArea->hide();
    mpActionsMainArea->hide();
    mpKeysMainArea->hide();

    mpSystemMessageArea->hide();
    mpOptionsAreaTriggers->hide();
    mpOptionsAreaAlias->hide();
    mpOptionsAreaScripts->hide();
    mpOptionsAreaTimers->hide();
    mpOptionsAreaActions->hide();


    treeWidget->hide();
    treeWidget_alias->hide();
    treeWidget_timers->hide();
    treeWidget_scripts->hide();
    treeWidget_actions->hide();
    treeWidget_keys->hide();

    QTreeWidgetItem * pI = treeWidget_timers->topLevelItem( 0 );
    if( pI )
    {
        if( pI->childCount() > 0 )
        {
            mpTimersMainArea->show();
            mpSourceEditorArea->show();
            slot_timer_clicked( treeWidget_timers->currentItem(), 0 );
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
    saveOpenChanges();

    if( mNeedUpdateData )
    {
        treeWidget->clear();
        treeWidget_alias->clear();
        treeWidget_timers->clear();
        treeWidget_scripts->clear();
        treeWidget_actions->clear();
        treeWidget_keys->clear();
        fillout_form();
        mNeedUpdateData = false;
    }

    mCurrentView = cmTriggerView;

    mpTriggersMainArea->hide();
    mpTimersMainArea->hide();
    mpScriptsMainArea->hide();
    mpAliasMainArea->hide();
    mpActionsMainArea->hide();
    mpKeysMainArea->hide();

    mpSystemMessageArea->hide();
    mpOptionsAreaTriggers->hide();
    mpOptionsAreaAlias->hide();
    mpOptionsAreaScripts->hide();
    mpOptionsAreaTimers->hide();
    mpOptionsAreaActions->hide();

    treeWidget->hide();
    treeWidget_alias->hide();
    treeWidget_timers->hide();
    treeWidget_scripts->hide();
    treeWidget_actions->hide();
    treeWidget_keys->hide();


    QTreeWidgetItem * pI = treeWidget->topLevelItem( 0 );
    if( pI )
    {
        if( pI->childCount() > 0 )
        {
            mpTriggersMainArea->show();
            mpSourceEditorArea->show();
            slot_trigger_clicked( treeWidget->currentItem(), 0 );
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
    treeWidget->show();
}

void dlgTriggerEditor::slot_show_scripts()
{
    saveOpenChanges();

    if( mNeedUpdateData )
    {
        treeWidget->clear();
        treeWidget_alias->clear();
        treeWidget_timers->clear();
        treeWidget_scripts->clear();
        treeWidget_actions->clear();
        treeWidget_keys->clear();
        fillout_form();
        mNeedUpdateData = false;
    }

    mCurrentView = cmScriptView;

    mpTriggersMainArea->hide();
    mpTimersMainArea->hide();
    mpScriptsMainArea->hide();
    mpAliasMainArea->hide();
    mpActionsMainArea->hide();
    mpKeysMainArea->hide();

    mpSystemMessageArea->hide();
    mpOptionsAreaTriggers->hide();
    mpOptionsAreaAlias->hide();
    mpOptionsAreaScripts->hide();
    mpOptionsAreaTimers->hide();
    mpOptionsAreaActions->hide();

    treeWidget->hide();
    treeWidget_alias->hide();
    treeWidget_timers->hide();
    treeWidget_scripts->hide();
    treeWidget_actions->hide();
    mpScriptsMainArea->hide();
    treeWidget_keys->hide();


    QTreeWidgetItem * pI = treeWidget_scripts->topLevelItem( 0 );
    if( pI )
    {
        if( pI->childCount() > 0 )
        {
            mpScriptsMainArea->show();
            mpSourceEditorArea->show();
            slot_scripts_clicked( treeWidget_scripts->currentItem(), 0 );
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
    saveOpenChanges();

    if( mNeedUpdateData )
    {
        treeWidget->clear();
        treeWidget_alias->clear();
        treeWidget_timers->clear();
        treeWidget_scripts->clear();
        treeWidget_actions->clear();
        treeWidget_keys->clear();
        fillout_form();
        mNeedUpdateData = false;
    }

    mCurrentView = cmKeysView;

    mpTriggersMainArea->hide();
    mpTimersMainArea->hide();
    mpScriptsMainArea->hide();
    mpAliasMainArea->hide();
    mpActionsMainArea->hide();
    mpKeysMainArea->hide();

    mpSystemMessageArea->hide();
    mpOptionsAreaTriggers->hide();
    mpOptionsAreaAlias->hide();
    mpOptionsAreaScripts->hide();
    mpOptionsAreaTimers->hide();
    mpOptionsAreaActions->hide();

    treeWidget->hide();
    treeWidget_alias->hide();
    treeWidget_timers->hide();
    treeWidget_scripts->hide();
    treeWidget_actions->hide();
    treeWidget_keys->hide();


    QTreeWidgetItem * pI = treeWidget_keys->topLevelItem( 0 );
    if( pI )
    {
        if( pI->childCount() > 0 )
        {
            mpKeysMainArea->show();
            mpSourceEditorArea->show();
            slot_key_clicked( treeWidget_keys->currentItem(), 0 );
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


void dlgTriggerEditor::slot_show_aliases()
{
    saveOpenChanges();

    if( mNeedUpdateData )
    {
        treeWidget->clear();
        treeWidget_alias->clear();
        treeWidget_timers->clear();
        treeWidget_scripts->clear();
        treeWidget_actions->clear();
        treeWidget_keys->clear();
        fillout_form();
        mNeedUpdateData = false;
    }

    mCurrentView = cmAliasView;

    mpTriggersMainArea->hide();
    mpTimersMainArea->hide();
    mpScriptsMainArea->hide();
    mpAliasMainArea->hide();
    mpActionsMainArea->hide();
    mpKeysMainArea->hide();

    mpSourceEditorArea->hide();

    mpSystemMessageArea->hide();
    mpOptionsAreaTriggers->hide();
    mpOptionsAreaAlias->hide();
    mpOptionsAreaScripts->hide();
    mpOptionsAreaTimers->hide();
    mpOptionsAreaActions->hide();

    treeWidget->hide();
    treeWidget_alias->hide();
    treeWidget_timers->hide();
    treeWidget_scripts->hide();
    treeWidget_actions->hide();
    treeWidget_keys->hide();

    QTreeWidgetItem * pI = treeWidget_alias->topLevelItem( 0 );
    if( pI )
    {
        if( pI->childCount() > 0 )
        {
            mpAliasMainArea->show();
            mpSourceEditorArea->show();
            slot_alias_clicked( treeWidget_alias->currentItem(), 0 );
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

    treeWidget_alias->show();
}

void dlgTriggerEditor::showError( QString error )
{
    mpSystemMessageArea->notificationAreaIconLabelInformation->hide();
    mpSystemMessageArea->notificationAreaIconLabelError->show();
    mpSystemMessageArea->notificationAreaIconLabelWarning->hide();
    mpSystemMessageArea->notificationAreaMessageBox->setText( error );
    mpSystemMessageArea->show();
}

void dlgTriggerEditor::showInfo( QString error )
{
    mpSystemMessageArea->notificationAreaIconLabelError->hide();
    mpSystemMessageArea->notificationAreaIconLabelWarning->hide();
    mpSystemMessageArea->notificationAreaIconLabelInformation->show();
    mpSystemMessageArea->notificationAreaMessageBox->setText( error );
    mpSystemMessageArea->show();
}

void dlgTriggerEditor::showWarning( QString error )
{
    mpSystemMessageArea->notificationAreaIconLabelInformation->hide();
    mpSystemMessageArea->notificationAreaIconLabelError->hide();
    mpSystemMessageArea->notificationAreaIconLabelWarning->show();
    mpSystemMessageArea->notificationAreaMessageBox->setText( error );
    mpSystemMessageArea->show();
}

void dlgTriggerEditor::slot_show_actions()
{
    saveOpenChanges();

    mCurrentView = cmActionView;

    mpTriggersMainArea->hide();
    mpTimersMainArea->hide();
    mpScriptsMainArea->hide();
    mpAliasMainArea->hide();
    mpActionsMainArea->hide();
    mpKeysMainArea->hide();
    mpSourceEditorArea->hide();

    mpSystemMessageArea->hide();
    mpOptionsAreaTriggers->hide();
    mpOptionsAreaAlias->hide();
    mpOptionsAreaScripts->hide();
    mpOptionsAreaTimers->hide();
    mpOptionsAreaActions->hide();

    treeWidget->hide();
    treeWidget_alias->hide();
    treeWidget_timers->hide();
    treeWidget_scripts->hide();
    treeWidget_keys->hide();
    QTreeWidgetItem * pI = treeWidget_actions->topLevelItem( 0 );
    if( pI )
    {
        if( pI->childCount() > 0 )
        {
            mpActionsMainArea->show();
            mpSourceEditorArea->show();
            slot_action_clicked( treeWidget_actions->currentItem(), 0 );
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
            slot_saveTriggerAfterEdit();
            break;
        case cmTimerView:
            slot_saveTimerAfterEdit();
            break;
        case cmAliasView:
            slot_saveAliasAfterEdit();
            break;
        case cmScriptView:
            slot_saveScriptAfterEdit();
            break;
        case cmActionView:
            slot_saveActionAfterEdit();
            break;
        case cmKeysView:
            slot_saveKeyAfterEdit();
            break;

        default: qWarning()<<"ERROR: dlgTriggerEditor::slot_save_edit() undefined view";
    };

    mpHost->serialize();
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
    default: qDebug()<<"ERROR: dlgTriggerEditor::slot_save_edit() undefined view";
    };
}

void dlgTriggerEditor::slot_itemClicked( QTreeWidgetItem * pItem, int column )
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
        QListWidgetItem * pItem = new QListWidgetItem;
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
    QTreeWidgetItem * pItem = treeWidget->currentItem();
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
                             tr("You have to chose an item for export first. Please select a tree item and then click on export again."));
            return;
        }
    }
    else
    {
        QMessageBox::warning(this, tr("Export Package:"),
                             tr("You have to chose an item for export first. Please select a tree item and then click on export again."));
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
                             tr("You have to chose an item for export first. Please select a tree item and then click on export again."));
            return;
        }
    }
    else
    {
        QMessageBox::warning(this, tr("Export Package:"),
                             tr("You have to chose an item for export first. Please select a tree item and then click on export again."));
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
    QTreeWidgetItem * pItem = treeWidget_alias->currentItem();
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
                             tr("You have to chose an item for export first. Please select a tree item and then click on export again."));
            return;
        }
    }
    else
    {
        QMessageBox::warning(this, tr("Export Package:"),
                             tr("You have to chose an item for export first. Please select a tree item and then click on export again."));
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
                             tr("You have to chose an item for export first. Please select a tree item and then click on export again."));
            return;
        }
    }
    else
    {
        QMessageBox::warning(this, tr("Export Package:"),
                             tr("You have to chose an item for export first. Please select a tree item and then click on export again."));
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
                             tr("You have to chose an item for export first. Please select a tree item and then click on export again."));
            return;
        }
    }
    else
    {
        QMessageBox::warning(this, tr("Export Package:"),
                             tr("You have to chose an item for export first. Please select a tree item and then click on export again."));
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
                             tr("You have to chose an item for export first. Please select a tree item and then click on export again."));
            return;
        }

    }
    else
    {
        QMessageBox::warning(this, tr("Export Package:"),
                             tr("You have to chose an item for export first. Please select a tree item and then click on export again."));
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
    if(fileName.isEmpty()) return;
    fileName.append(".xml");


    QFile file(fileName);
    if( ! file.open(QFile::WriteOnly | QFile::Text) )
    {
        QMessageBox::warning(this, tr("export package:"),
                             tr("Cannot write file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
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
                             .arg(fileName)
                             .arg(file.errorString()));
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
        QString _dest = QString( "%1/%2/").arg( _home ).arg( packageName );
        QDir _tmpDir;
        _tmpDir.mkpath(_dest);
        QString _script = QString( "unzip([[%1]], [[%2]])" ).arg( fileName ).arg( _dest );
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

    treeWidget->clear();
    treeWidget_alias->clear();
    treeWidget_actions->clear();
    treeWidget_timers->clear();
    treeWidget_keys->clear();
    treeWidget_scripts->clear();

    XMLimport reader( mpHost );
    reader.importPackage( & file2, packageName );

    mpHost->setName( profileName );
    mpHost->setLogin( login );
    mpHost->setPass( pass );

    slot_profileSaveAction();

    fillout_form();

    mCurrentTrigger = 0;
    mCurrentTimer = 0;
    mCurrentAlias = 0;
    mCurrentScript = 0;
    mCurrentAction = 0;
    mCurrentKey = 0;

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
    treeWidget->clear();
    treeWidget_alias->clear();
    treeWidget_actions->clear();
    treeWidget_timers->clear();
    treeWidget_keys->clear();
    treeWidget_scripts->clear();
    fillout_form();
    mCurrentTrigger = 0;
    mCurrentTimer = 0;
    mCurrentAlias = 0;
    mCurrentScript = 0;
    mCurrentAction = 0;
    mCurrentKey = 0;
    slot_show_triggers();
}


void dlgTriggerEditor::slot_profileSaveAction()
{
    QString directory_xml = QDir::homePath()+"/.config/mudlet/profiles/"+mpHost->getName()+"/current";
    QString filename_xml = directory_xml + "/"+QDateTime::currentDateTime().toString("dd-MM-yyyy#hh-mm-ss")+".xml";
    QDir dir_xml;
    if( ! dir_xml.exists( directory_xml ) )
    {
        dir_xml.mkpath( directory_xml );
    }
    QFile file_xml( filename_xml );
    if ( file_xml.open( QIODevice::WriteOnly ) )
    {
        XMLexport writer( mpHost );
        writer.exportHost( & file_xml );
        file_xml.close();
        qDebug()<<"saving modules";
        mpHost->saveModules(1);
    }
    else
    {
        QMessageBox::critical( this, "Profile Save Failed", "Failed to save "+mpHost->getName()+" to location "+filename_xml+" because of the following error: "+file_xml.errorString() );
    }
}

void dlgTriggerEditor::slot_profileSaveAsAction()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Backup Profile"),
                                                    QDir::homePath(),
                                                    tr("trigger files (*.trigger *.xml)"));

    if(fileName.isEmpty()) return;
    fileName.append(".xml");

    QFile file(fileName);
    if( ! file.open(QFile::WriteOnly | QFile::Text) )
    {
       QMessageBox::warning(this, tr("Backup Profile:"),
                            tr("Cannot write file %1:\n%2.")
                            .arg(fileName)
                            .arg(file.errorString()));
       return;
    }
qDebug()<<"generic export";
    XMLexport writer( mpHost, true );//just export a generic package without host element
    //writer.exportHost( & file );
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
            switch( ke->key() )
            {
                case 0x01000000:
                    mIsGrabKey = false;
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

void dlgTriggerEditor::slot_colorizeTriggerSetFgColor()
{
    QTreeWidgetItem * pItem = mCurrentTrigger;
    if( ! pItem ) return;
    if( ! pItem->parent() ) return;
    int triggerID = pItem->data( 0, Qt::UserRole ).toInt();
    TTrigger * pT = mpHost->getTriggerUnit()->getTrigger( triggerID );

    QColor color = QColorDialog::getColor( mpTriggersMainArea->pushButtonFgColor->palette().color( QPalette::Button ), this );
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
    QTreeWidgetItem * pItem = mCurrentTrigger;
    if( ! pItem ) return;
    if( ! pItem->parent() ) return;
    int triggerID = pItem->data( 0, Qt::UserRole ).toInt();
    TTrigger * pT = mpHost->getTriggerUnit()->getTrigger( triggerID );

    QColor color = QColorDialog::getColor( mpTriggersMainArea->pushButtonBgColor->palette().color( QPalette::Button ), this );
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
    QString fileName = QFileDialog::getOpenFileName(this, tr("chose sound file"),
                                                    QDir::homePath(),
                                                    tr("*"));
    mpTriggersMainArea->lineEdit_soundFile->setText( fileName );
}

void dlgTriggerEditor::slot_color_trigger_fg()
{
    QTreeWidgetItem * pItem = mCurrentTrigger;
    if( ! pItem )
    {
        return;
    }
    int triggerID = pItem->data( 0, Qt::UserRole ).toInt();
    TTrigger * pT = mpHost->getTriggerUnit()->getTrigger( triggerID );
    if( ! pT ) return;

    QPushButton * pB = (QPushButton *) sender();
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

    dlgColorTrigger * pD = new dlgColorTrigger(this, pT, 0 );
    pD->setModal( true );
    pD->setWindowModality( Qt::ApplicationModal );
    pD->exec();
    QPalette palette;
    QColor color = pT->mColorTriggerFgColor;
    palette.setColor( QPalette::Button, color );
    QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");


    if( ! pB ) return;
    row = ((dlgTriggerPatternEdit*)pB->parent())->mRow;
    pI = mTriggerPatternEdit[row];
    pI->lineEdit->setText(QString("FG%1BG%2").arg(pT->mColorTriggerFgAnsi).arg(pT->mColorTriggerBgAnsi) );
    pB->setStyleSheet( styleSheet );
}

void dlgTriggerEditor::slot_color_trigger_bg()
{
    QTreeWidgetItem * pItem = mCurrentTrigger;
    if( ! pItem )
    {
        return;
    }
    int triggerID = pItem->data( 0, Qt::UserRole ).toInt();
    TTrigger * pT = mpHost->getTriggerUnit()->getTrigger( triggerID );
    if( ! pT ) return;

    QPushButton * pB = (QPushButton *) sender();
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

    dlgColorTrigger * pD = new dlgColorTrigger(this, pT, 1 );
    pD->setModal( true );
    pD->setWindowModality( Qt::ApplicationModal );
    pD->exec();
    QPalette palette;
    QColor color = pT->mColorTriggerBgColor;
    palette.setColor( QPalette::Button, color );
    QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");

    if( ! pB ) return;
    row = ((dlgTriggerPatternEdit*)pB->parent())->mRow;
    pI = mTriggerPatternEdit[row];
    if( ! pI ) return;
    pI->lineEdit->setText(QString("FG%1BG%2").arg(pT->mColorTriggerFgAnsi).arg(pT->mColorTriggerBgAnsi) );
    pB->setStyleSheet( styleSheet );

    /*QTreeWidgetItem * pItem = mCurrentTrigger;
    if( ! pItem )
    {
        return;
    }
    int triggerID = pItem->data( 0, Qt::UserRole ).toInt();
    TTrigger * pT = mpHost->getTriggerUnit()->getTrigger( triggerID );
    if( ! pT ) return;

    dlgColorTrigger * pD = new dlgColorTrigger(this, pT, 1 );
    pD->exec();
    QPalette palette;
    QColor color = pT->mColorTriggerBgColor;
    palette.setColor( QPalette::Button, color );
    QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");

    QPushButton * pB = (QPushButton *) sender();
    if( ! pB ) return;
    int row = ((dlgTriggerPatternEdit*)pB->parent())->mRow;
    dlgTriggerPatternEdit * pI = (dlgTriggerPatternEdit*)mpTriggersMainArea->listWidget_regex_list->cellWidget( row, 0 );
    if( ! pI ) return;
    pI->lineEdit->setText(QString("FG%1BG%2").arg(pT->mColorTriggerFgAnsi).arg(pT->mColorTriggerBgAnsi) );
    pB->setStyleSheet( styleSheet );*/
}

void dlgTriggerEditor::slot_cursorPositionChanged()
{
    int _line = (mpSourceEditorArea->editor->textCursor()).blockNumber();
    int _maxLines = mpSourceEditorArea->editor->blockCount();
    QString line;

    if( mCurrentView == cmScriptView )
    {
        line = QString("current line number: %1/%2").arg( _line + 1 ).arg( _maxLines );
    }
    else
    {
        line = QString("current line number: %1/%2").arg( _line + 2 ).arg( _maxLines + 1 );
    }
    QMainWindow::statusBar()->showMessage( line );
}





