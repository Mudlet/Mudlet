/***************************************************************************
 *   Copyright (C) 2008 by Heiko Koehn                                     *
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
#include "HostManager.h"
#include "TriggerUnit.h"
#include "TTrigger.h"
#include "TAction.h"
#include <QHeaderView>
#include <Qsci/qsciscintilla.h>
#include <Qsci/qscilexerlua.h>
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

const int dlgTriggerEditor::cmTriggerView = 1;
const int dlgTriggerEditor::cmTimerView = 2;
const int dlgTriggerEditor::cmAliasView = 3;
const int dlgTriggerEditor::cmScriptView = 4;
const int dlgTriggerEditor::cmActionView = 5;
const int dlgTriggerEditor::cmKeysView = 6;

dlgTriggerEditor::dlgTriggerEditor( Host * pH , QWidget * parent) : QMainWindow(parent), mpHost( pH )
{
    // init generated dialog
    setupUi(this);
        
    
    mIsGrabKey = false;
    QVBoxLayout * pVB1 = new QVBoxLayout(mainArea);
    
    // system message area
    mpSystemMessageArea = new dlgSystemMessageArea( mainArea );
    QSizePolicy sizePolicy6( QSizePolicy::Expanding, QSizePolicy::Fixed );
    mpSystemMessageArea->setSizePolicy( sizePolicy6 );
    pVB1->addWidget( mpSystemMessageArea );
    
    // main areas   
    
    mpTriggersMainArea = new dlgTriggersMainArea( mainArea );
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mpTriggersMainArea->setSizePolicy( sizePolicy );
    pVB1->addWidget( mpTriggersMainArea );
    mIsTriggerMainAreaEditRegex = false;
    mpTriggerMainAreaEditRegexItem = 0;
    connect(mpTriggersMainArea->lineEdit, SIGNAL(returnPressed()), this, SLOT(slot_trigger_main_area_add_regex()));
    connect(mpTriggersMainArea->listWidget_regex_list, SIGNAL(itemClicked ( QListWidgetItem *)), this, SLOT(slot_trigger_main_area_edit_regex(QListWidgetItem*)));
    
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
    
    // option areas
    
    QHBoxLayout * pHB2 = new QHBoxLayout(popupArea);    
    QSizePolicy sizePolicy2(QSizePolicy::Expanding, QSizePolicy::Maximum);
    popupArea->setMinimumSize(200,60);
    //    pHB2->setMaximumSize(170);
    pHB2->setSizeConstraint( QLayout::SetMaximumSize );
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
    
    
    
    mpSearchArea = new dlgSearchArea( popupArea );
    mpSearchArea->setSizePolicy( sizePolicy2 );
    pHB2->addWidget( mpSearchArea );
    
    
    
    // additional settings
    treeWidget->setColumnCount(1);
    treeWidget->setIsTriggerTree();
    treeWidget->setRootIsDecorated( false );
    treeWidget->setHost( mpHost );
    treeWidget->header()->hide();
    treeWidget->setPoint(this);
    
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
    
    treeWidget_actions->hide();
    treeWidget_actions->setHost( mpHost );
    treeWidget_actions->setIsActionTree();
    treeWidget_actions->setColumnCount(1);
    treeWidget_actions->header()->hide();
    treeWidget_actions->setRootIsDecorated( false );
    
    treeWidget_timers->hide();
    treeWidget_timers->setHost( mpHost );
    treeWidget_timers->setIsTimerTree();
    treeWidget_timers->setColumnCount(1);
    treeWidget_timers->header()->hide();
    treeWidget_timers->setRootIsDecorated( false );
    
    treeWidget_keys->hide();
    treeWidget_keys->setHost( mpHost );
    treeWidget_keys->setIsKeyTree();
    treeWidget_keys->setColumnCount(1);
    treeWidget_keys->header()->hide();
    treeWidget_keys->setRootIsDecorated( false );
    
    treeWidget_scripts->hide();
    treeWidget_scripts->setHost( mpHost );
    treeWidget_scripts->setIsScriptTree();
    treeWidget_scripts->setColumnCount(1);
    treeWidget_scripts->header()->hide();
    treeWidget_scripts->setRootIsDecorated( false );
    
    
    QAction * viewTriggerAction = new QAction(QIcon(":/tools-wizard.png"), tr("Triggers"), this);
    viewTriggerAction->setStatusTip(tr("Show Triggers"));
    connect(viewTriggerAction, SIGNAL(triggered()), this, SLOT(slot_show_triggers() ));
    
    QAction * viewActionAction = new QAction(QIcon(":/bookmarks.png"), tr("Actions"), this);
    viewActionAction->setStatusTip(tr("Show Actions"));
    connect(viewActionAction, SIGNAL(triggered()), this, SLOT(slot_show_actions() ));
    
    
    QAction * viewAliasAction = new QAction(QIcon(":/system-users.png"), tr("Aliases"), this);
    viewAliasAction->setStatusTip(tr("Show Aliases"));
    viewAliasAction->setEnabled( true );
    connect( viewAliasAction, SIGNAL(triggered()), this, SLOT( slot_show_aliases()));
    
    
    QAction * showTimersAction = new QAction(QIcon(":/chronometer.png"), tr("Timers"), this);
    showTimersAction->setStatusTip(tr("Show Timers"));
    connect( showTimersAction, SIGNAL(triggered()), this, SLOT( slot_show_timers()));
    
    
    QAction * viewScriptsAction = new QAction(QIcon(":/document-properties.png"), tr("Scripts"), this);
    viewScriptsAction->setEnabled( true );
    viewScriptsAction->setStatusTip(tr("Show Scripts"));
    connect( viewScriptsAction, SIGNAL(triggered()), this, SLOT( slot_show_scripts()));
    
    QAction * viewKeysAction = new QAction(QIcon(":/preferences-desktop-keyboard.png"), tr("Keys"), this);
    viewKeysAction->setStatusTip(tr("Keybindings"));
    viewKeysAction->setEnabled( true );
    connect( viewKeysAction, SIGNAL(triggered()), this, SLOT( slot_show_keys()));
    
    QAction * toggleActiveAction = new QAction(QIcon(":/document-encrypt.png"), tr("Activate"), this);
    toggleActiveAction->setShortcut(tr("Ctrl+T"));
    toggleActiveAction->setStatusTip(tr("Toggle Active or Non-Active Mode for Triggers, Scripts etc."));
    connect( toggleActiveAction, SIGNAL(triggered()), this, SLOT( slot_toggle_active()));
    
    QAction * addTriggerAction = new QAction(QIcon(":/document-new.png"), tr("Add"), this);
    addTriggerAction->setStatusTip(tr("Add new Trigger, Script, Alias or Filter"));
    connect( addTriggerAction, SIGNAL(triggered()), this, SLOT( slot_add_new()));
    
    QAction * deleteTriggerAction = new QAction(QIcon(":/edit-delete-shred.png"), tr("Delete"), this);
    deleteTriggerAction->setStatusTip(tr("Delete Trigger, Script, Alias or Filter"));
    connect( deleteTriggerAction, SIGNAL(triggered()), this, SLOT( slot_delete_item()));
        
    QAction * addFolderAction = new QAction(QIcon(":/folder-new.png"), tr("Add Group"), this);
    addFolderAction->setStatusTip(tr("Add new Group"));
    connect( addFolderAction, SIGNAL(triggered()), this, SLOT( slot_add_new_folder()));
    
    QAction * showSearchAreaAction = new QAction(QIcon(":/edit-find-user.png"), tr("Search"), this);
    showSearchAreaAction->setShortcut(tr("Ctrl+F"));
    showSearchAreaAction->setStatusTip(tr("Show Search Results List"));
    connect( showSearchAreaAction, SIGNAL(triggered()), this, SLOT( slot_show_search_area()));
    
    QAction * saveAction = new QAction(QIcon(":/document-save-as.png"), tr("Save"), this);
    saveAction->setShortcut(tr("Ctrl+S"));
    saveAction->setStatusTip(tr("Save Edited Trigger, Script, Alias etc. If information has been edited, it must be saved or the changes will be lost."));
    connect( saveAction, SIGNAL(triggered()), this, SLOT( slot_save_edit() ));
    
    QAction * importAction = new QAction(QIcon(":/application-x-cpio.png"), tr("Import"), this);
    importAction->setEnabled( true );
    connect( importAction, SIGNAL(triggered()), this, SLOT( slot_import()));
    
    QAction * exportAction = new QAction(QIcon(":/application-x-lha.png"), tr("Export"), this);
    exportAction->setEnabled( true );
    connect( exportAction, SIGNAL(triggered()), this, SLOT( slot_export()));
    
    QAction * showDebugAreaAction = new QAction(QIcon(":/tools-report-bug.png"), tr("Debug"), this);
    showDebugAreaAction->setEnabled( true );
    showDebugAreaAction->setToolTip(tr("Activates Debug Messages -> system will be *MUCH* slower"));
    connect( showDebugAreaAction, SIGNAL(triggered()), this, SLOT( slot_debug_mode() ));
    
    QAction * addTriggerMenuAction = new QAction(QIcon(":/tools-wizard.png"), tr("Triggers"), this);
    viewTriggerAction->setStatusTip(tr("Add Trigger"));
    connect(addTriggerMenuAction, SIGNAL(triggered()), this, SLOT(slot_addTrigger()));
    
    QAction * addAliasMenuAction = new QAction(QIcon(":/system-users.png"), tr("Aliases"), this);
    addAliasMenuAction->setStatusTip(tr("Add Alias"));
    addAliasMenuAction->setEnabled( true );
    connect( addAliasMenuAction, SIGNAL(triggered()), this, SLOT( slot_addAlias()));
    
    QAction * addTimersMenuAction = new QAction(QIcon(":/chronometer.png"), tr("Timers"), this);
    addTimersMenuAction->setStatusTip(tr("Add Timer"));
    addTimersMenuAction->setEnabled( true );
    connect( addTimersMenuAction, SIGNAL(triggered()), this, SLOT( slot_addTimer()));
    
    QAction * addScriptsMenuAction = new QAction(QIcon(":/document-properties.png"), tr("Scripts"), this);
    addScriptsMenuAction->setStatusTip(tr("Add Script"));
    addScriptsMenuAction->setEnabled( true );
    connect( addScriptsMenuAction, SIGNAL(triggered()), this, SLOT( slot_addScript()));
    
    QAction * addKeysMenuAction = new QAction(QIcon(":/preferences-desktop-keyboard.png"), tr("Keys"), this);
    addKeysMenuAction->setStatusTip(tr("Add Keys"));
    addKeysMenuAction->setEnabled( true );
    connect( addKeysMenuAction, SIGNAL(triggered()), this, SLOT( slot_addKey()));
    
    QMenu * addTriggerMenu = new QMenu( this );
    addTriggerMenu->addAction( addTriggerMenuAction );
    addTriggerMenu->addAction( addTimersMenuAction );
    addTriggerMenu->addAction( addScriptsMenuAction );
    addTriggerMenu->addAction( addAliasMenuAction );
    addTriggerMenu->addAction( addKeysMenuAction );
    
    addTriggerAction->setMenu( addTriggerMenu );
    
    QAction * addTriggerGroupMenuAction = new QAction(QIcon(":/tools-wizard.png"), tr("Triggers"), this);
    addTriggerGroupMenuAction->setStatusTip(tr("Add Trigger Group"));
    connect(addTriggerGroupMenuAction, SIGNAL(triggered()), this, SLOT(slot_addTriggerGroup()));
    
    QAction * addAliasGroupMenuAction = new QAction(QIcon(":/system-users.png"), tr("Aliases"), this);
    addAliasGroupMenuAction->setStatusTip(tr("Add Alias Group"));
    addAliasGroupMenuAction->setEnabled( true );
    connect( addAliasGroupMenuAction, SIGNAL(triggered()), this, SLOT( slot_addAliasGroup()));
    
    QAction * addTimersGroupMenuAction = new QAction(QIcon(":/chronometer.png"), tr("Timers"), this);
    addTimersGroupMenuAction->setStatusTip(tr("Add Timer Group"));
    addTimersGroupMenuAction->setEnabled( true );
    connect( addTimersGroupMenuAction, SIGNAL(triggered()), this, SLOT( slot_addTimerGroup()));
    
    QAction * addScriptsGroupMenuAction = new QAction(QIcon(":/document-properties.png"), tr("Scripts"), this);
    addScriptsGroupMenuAction->setStatusTip(tr("Add Script Group"));
    addScriptsGroupMenuAction->setEnabled( true );
    connect( addScriptsGroupMenuAction, SIGNAL(triggered()), this, SLOT( slot_addScriptGroup()));
    
    //QAction * addFiltersGroupMenuAction = new QAction(QIcon(":/view-filter.png"), tr("Filters"), this);
    //addFiltersGroupMenuAction->setStatusTip(tr("Add Filter Group"));
    //addFiltersGroupMenuAction->setEnabled( false );
    //connect( viewFiltersAction, SIGNAL(triggered()), this, SLOT( showFiltersView()));
    
    QMenu * addTriggerGroupMenu = new QMenu( this );
    addTriggerGroupMenu->addAction( addTriggerGroupMenuAction );
    addTriggerGroupMenu->addAction( addTimersGroupMenuAction );
    addTriggerGroupMenu->addAction( addScriptsGroupMenuAction );
    addTriggerGroupMenu->addAction( addAliasGroupMenuAction );
    //addTriggerGroupMenu->addAction( addFiltersGroupMenuAction );
    
    addFolderAction->setMenu( addTriggerGroupMenu );
    toolBar->setIconSize(QSize(32,32));
    toolBar->addAction( viewTriggerAction );
    toolBar->addAction( viewAliasAction );
    toolBar->addAction( viewScriptsAction );
    toolBar->addAction( showTimersAction );
    toolBar->addAction( viewKeysAction );
    toolBar->addAction( viewActionAction );
    toolBar->addSeparator();    
    
    toolBar->addAction( toggleActiveAction );
    toolBar->addAction( saveAction );
    toolBar->addAction( showSearchAreaAction );
    toolBar->addSeparator();
    
    toolBar->addAction( addTriggerAction );
    toolBar->addAction( addFolderAction );
    
    toolBar->addSeparator();
    toolBar->addAction( deleteTriggerAction );    
    toolBar->addAction( importAction );
    toolBar->addAction( exportAction );
    toolBar->addAction( showDebugAreaAction );
    
      
    mpLuaLexer = new QsciLexerLua;
    mpSourceEditorArea->script_scintilla->setLexer( mpLuaLexer );
    mpSourceEditorArea->script_scintilla->setAutoCompletionFillupsEnabled(true);
    
    connect( comboBox_search_triggers, SIGNAL( currentIndexChanged( const QString )), this, SLOT(slot_search_triggers( const QString ) ) );
    connect( this, SIGNAL( update() ), this, SLOT( slot_update() ) );
    connect( treeWidget, SIGNAL( currentItemChanged ( QTreeWidgetItem *, QTreeWidgetItem *) ), this, SLOT( slot_trigger_clicked( QTreeWidgetItem *, QTreeWidgetItem *) ) );
    connect( treeWidget_keys, SIGNAL( currentItemChanged ( QTreeWidgetItem *, QTreeWidgetItem *) ), this, SLOT( slot_key_clicked( QTreeWidgetItem *, QTreeWidgetItem *) ) );
    connect( treeWidget_timers, SIGNAL( currentItemChanged ( QTreeWidgetItem *, QTreeWidgetItem *) ), this, SLOT( slot_timer_clicked( QTreeWidgetItem *, QTreeWidgetItem *) ) );
    connect( treeWidget_scripts, SIGNAL( currentItemChanged ( QTreeWidgetItem *, QTreeWidgetItem *) ), this, SLOT( slot_scripts_clicked( QTreeWidgetItem *, QTreeWidgetItem *) ) );
    connect( treeWidget_alias, SIGNAL( currentItemChanged ( QTreeWidgetItem *, QTreeWidgetItem *) ), this, SLOT( slot_alias_clicked( QTreeWidgetItem *, QTreeWidgetItem *) ) );
    connect( treeWidget_actions, SIGNAL( currentItemChanged ( QTreeWidgetItem *, QTreeWidgetItem *) ), this, SLOT( slot_action_clicked( QTreeWidgetItem *, QTreeWidgetItem *) ) );
    connect( this, SIGNAL (accept()), this, SLOT (slot_connection_dlg_finnished()));
    connect( mpSearchArea->tree_widget_search_results_main_2, SIGNAL(itemClicked(QTreeWidgetItem*, int)), this, SLOT( slot_item_clicked_search_list(QTreeWidgetItem*, int)));
    connect( tree_widget_search_results_main, SIGNAL(itemClicked(QTreeWidgetItem*, int)), this, SLOT( slot_item_clicked_search_list(QTreeWidgetItem*, int)));
    connect( mpTriggersMainArea->toolButton_add, SIGNAL(pressed()), this, SLOT(slot_trigger_main_area_add_regex()));
    connect( mpTriggersMainArea->toolButton_remove, SIGNAL(pressed()), this, SLOT( slot_trigger_main_area_delete_regex()));
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
    QList<QTreeWidgetItem *> foundItemsList = treeWidget->findItems( pItem->text( 0 ), Qt::MatchContains );
    if( foundItemsList.size() == 0 )
    {
        qDebug() << "no Triggers found that match <"<<foundItemsList[0]->text(0)<<">";
        return;    
    }
    
    pItem = foundItemsList.at(0);
    
    treeWidget->show();    
    tree_widget_search_results_main->hide();
    treeWidget->scrollToItem( pItem );
    treeWidget->setCurrentItem( pItem, 0 );
}

void dlgTriggerEditor::slot_search_triggers( const QString s )             
{
    tree_widget_search_results_main->clear();
    mpSearchArea->tree_widget_search_results_main_2->clear();
    QList<QTreeWidgetItem *> foundItemsList = treeWidget->findItems( s, Qt::MatchContains | Qt::MatchRecursive );
    if( foundItemsList.size() == 0 )
    {
        qDebug()<< "no Triggers found that match <"<<s<<">";
        return;    
    }
    for( int i = 0; i < foundItemsList.size(); i++) 
    {
        qDebug()<<"found item: "<< foundItemsList.at(i)->text(0);
        tree_widget_search_results_main->insertTopLevelItem(0, foundItemsList.at(i)->clone());
        mpSearchArea->tree_widget_search_results_main_2->insertTopLevelItem(0, foundItemsList.at(i)->clone()); 
    }
    treeWidget->hide();
    tree_widget_search_results_main->show();
    mpSearchArea->show();
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
    QTreeWidgetItem * pParent = pItem->parent();
    
    if( ! pItem ) return;
    
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
}

void dlgTriggerEditor::slot_deleteAction()
{
    QTreeWidgetItem * pItem = treeWidget_actions->currentItem();    
    QTreeWidgetItem * pParent = pItem->parent();
    
    if( ! pItem ) return;
    
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
    mpHost->getActionUnit()->updateToolbar();
}

void dlgTriggerEditor::slot_deleteScript()
{
    QTreeWidgetItem * pItem = treeWidget_scripts->currentItem();    
    QTreeWidgetItem * pParent = pItem->parent();
    
    if( ! pItem ) return;
    
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
}

void dlgTriggerEditor::slot_deleteKey()
{
    QTreeWidgetItem * pItem = treeWidget_keys->currentItem();    
    QTreeWidgetItem * pParent = pItem->parent();
    
    if( ! pItem ) return;
    
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
}

void dlgTriggerEditor::slot_deleteTrigger()
{
    QTreeWidgetItem * pItem = treeWidget->currentItem();    
    QTreeWidgetItem * pParent = pItem->parent();
    
    if( ! pItem ) return;
    
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
}

void dlgTriggerEditor::slot_deleteTimer()
{
    QTreeWidgetItem * pItem = treeWidget_timers->currentItem();    
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
}


void dlgTriggerEditor::slot_trigger_toggle_active()
{
    QTreeWidgetItem * pItem = (QTreeWidgetItem *)treeWidget->currentItem();    
    if( ! pItem ) return;
    
    TTrigger * pT = mpHost->getTriggerUnit()->getTrigger(pItem->data(0, Qt::UserRole).toInt());
    if( ! pT ) return;    
        
    pT->setIsActive( ! pT->isActive() );
    
    QIcon icon;
    if( pT->isFolder() )
    {
        if( pT->isActive() )
        {
            icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-blue.png")), QIcon::Normal, QIcon::Off);    
        }
        else
        {
            icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-blue-locked.png")), QIcon::Normal, QIcon::Off);    
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

void dlgTriggerEditor::slot_timer_toggle_active()
{
    QTreeWidgetItem * pItem = (QTreeWidgetItem *)treeWidget_timers->currentItem();    
    if( ! pItem ) return;
    
    TTimer * pT = mpHost->getTimerUnit()->getTimer(pItem->data(0, Qt::UserRole).toInt());
    if( ! pT ) return;    
    
    pT->setIsActive( ! pT->isActive() );
    
    QIcon icon;
    if( pT->isFolder() )
    {
        if( pT->isActive() )
        {
            icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-green.png")), QIcon::Normal, QIcon::Off);    
        }
        else
        {
            icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-green-locked.png")), QIcon::Normal, QIcon::Off);    
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

void dlgTriggerEditor::slot_alias_toggle_active()
{
    QTreeWidgetItem * pItem = (QTreeWidgetItem *)treeWidget_alias->currentItem();    
    if( ! pItem ) return;
    QIcon icon;
    
    TAlias * pT = mpHost->getAliasUnit()->getAlias(pItem->data(0, Qt::UserRole).toInt());
    if( ! pT ) return;    
    
    pT->setIsActive( ! pT->isActive() );
    
    if( pT->isFolder() )
    {
        if( pT->isActive() )
        {
            icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-violet.png")), QIcon::Normal, QIcon::Off);    
        }
        else
        {
            icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-violet-locked.png")), QIcon::Normal, QIcon::Off);    
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

void dlgTriggerEditor::slot_script_toggle_active()
{
    QTreeWidgetItem * pItem = (QTreeWidgetItem *)treeWidget_scripts->currentItem();    
    if( ! pItem ) return;
    QIcon icon;
    
    TScript * pT = mpHost->getScriptUnit()->getScript(pItem->data(0, Qt::UserRole).toInt());
    if( ! pT ) return;    
    
    pT->setIsActive( ! pT->isActive() );
    
    if( pT->isFolder() )
    {
        if( pT->isActive() )
        {
            icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-orange.png")), QIcon::Normal, QIcon::Off);    
        }
        else
        {
            icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-orange-locked.png")), QIcon::Normal, QIcon::Off);    
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

void dlgTriggerEditor::slot_action_toggle_active()
{
    QTreeWidgetItem * pItem = (QTreeWidgetItem *)treeWidget_actions->currentItem();    
    if( ! pItem ) return;
    QIcon icon;
    
    TAction * pT = mpHost->getActionUnit()->getAction(pItem->data(0, Qt::UserRole).toInt());
    if( ! pT ) return;    
    
    pT->setIsActive( ! pT->isActive() );
    
    if( pT->isFolder() )
    {
        if( pT->isActive() )
        {
            icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-cyan.png")), QIcon::Normal, QIcon::Off);    
        }
        else
        {
            icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-cyan-locked.png")), QIcon::Normal, QIcon::Off);    
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

void dlgTriggerEditor::slot_key_toggle_active()
{
    QTreeWidgetItem * pItem = (QTreeWidgetItem *)treeWidget_keys->currentItem();    
    if( ! pItem ) return;
    
    TKey * pT = mpHost->getKeyUnit()->getKey(pItem->data(0, Qt::UserRole).toInt());
    if( ! pT ) return;    
    
    pT->setIsActive( ! pT->isActive() );
    
    QIcon icon;
    if( pT->isFolder() )
    {
        if( pT->isActive() )
        {
            icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-cyan.png")), QIcon::Normal, QIcon::Off);    
        }
        else
        {
            icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-cyan-locked.png")), QIcon::Normal, QIcon::Off);    
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


void dlgTriggerEditor::addTrigger( bool isFolder )
{
   
    QString name;
    if( isFolder ) name = "New Trigger Group";
    else name = "New Trigger";
    QStringList regexList;
    QList<int> regexPropertyList;
    QString script = "";
    QStringList nameL;
    nameL << name;
    
    QTreeWidgetItem * pParent = (QTreeWidgetItem*)treeWidget->currentItem();
    
    QTreeWidgetItem * pNewItem;
    TTrigger * pT;
    if( pParent )
    {
        int parentID = pParent->data(0, Qt::UserRole).toInt();
        TTrigger * pParentTrigger = mpHost->getTriggerUnit()->getTrigger( parentID );
        pT = new TTrigger( pParentTrigger, mpHost );
        pNewItem = new QTreeWidgetItem( pParent, nameL );
        pParent->insertChild( 0, pNewItem );
    }
    else
    {
        pT = new TTrigger( name, regexList, regexPropertyList, false, mpHost );
        pNewItem = new QTreeWidgetItem( mpTriggerBaseItem, nameL );
        treeWidget->insertTopLevelItem( 0, pNewItem );    
    }
    
    pT->setName( name );
    pT->setRegexCodeList( regexList, regexPropertyList );
    pT->setScript( script );
    pT->setIsFolder( isFolder );
    pT->setIsActive( false );
    pT->setIsMultiline( false );
    pT->registerTrigger();
    int childID = pT->getID();    
    pNewItem->setData( 0, Qt::UserRole, childID );
    QIcon icon;
    if( isFolder )
    {
        icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-red.png")), QIcon::Normal, QIcon::Off);        
    }
    else
    {
        icon.addPixmap(QPixmap(QString::fromUtf8(":/document-save-as.png")), QIcon::Normal, QIcon::Off);
    }
    pNewItem->setIcon( 0, icon );
    if( pParent ) pParent->setExpanded( true );
    mpTriggersMainArea->lineEdit_trigger_name->clear();
    mpTriggersMainArea->listWidget_regex_list->clear();
    mpSourceEditorArea->script_scintilla->clear();
    treeWidget->setCurrentItem( pNewItem );
}  



void dlgTriggerEditor::addTimer( bool isFolder )
{
    QString name;
    if( isFolder ) name = "New Timer Group";
    else name = "New Timer";
    QString command = "";
    QTime time;
    QString script = "";
    QStringList nameL;
    nameL << name;
    
    QTreeWidgetItem * pParent = (QTreeWidgetItem*)treeWidget_timers->currentItem();
    
    QTreeWidgetItem * pNewItem;
    TTimer * pT;
    if( pParent )
    {
        int parentID = pParent->data(0, Qt::UserRole).toInt();
        TTimer * pParentTimer = mpHost->getTimerUnit()->getTimer( parentID );
        pT = new TTimer( pParentTimer, mpHost );
        pNewItem = new QTreeWidgetItem( pParent, nameL );
        pParent->insertChild( 0, pNewItem );
        pParent->setExpanded( true );
    }
    else
    {
        pT = new TTimer( name, time, mpHost );
        pNewItem = new QTreeWidgetItem( mpTimerBaseItem, nameL );
        treeWidget->insertTopLevelItem( 0, pNewItem );    
    }
    
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
        icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-red.png")), QIcon::Normal, QIcon::Off);        
    }
    else
    {
        icon.addPixmap(QPixmap(QString::fromUtf8(":/document-save-as.png")), QIcon::Normal, QIcon::Off);
    }
    pNewItem->setIcon( 0, icon );
    if( pParent ) pParent->setExpanded( true );    
    //FIXME
    //mpOptionsAreaTriggers->lineEdit_trigger_name->clear();
    mpTimersMainArea->lineEdit_command->clear();
    mpSourceEditorArea->script_scintilla->clear();
    treeWidget_timers->setCurrentItem( pNewItem );
}  

void dlgTriggerEditor::addKey( bool isFolder )
{
    QString name;
    if( isFolder ) name = "New Key Group";
    else name = "New Key";
    QString script = "";
    QStringList nameL;
    nameL << name;
    
    QTreeWidgetItem * pParent = (QTreeWidgetItem*)treeWidget_keys->currentItem();
    
    QTreeWidgetItem * pNewItem;
    TKey * pT;
    if( pParent )
    {
        int parentID = pParent->data(0, Qt::UserRole).toInt();
        TKey * pParentKey = mpHost->getKeyUnit()->getKey( parentID );
        pT = new TKey( pParentKey, mpHost );
        pNewItem = new QTreeWidgetItem( pParent, nameL );
        pParent->insertChild( 0, pNewItem );
    }
    else
    {
        pT = new TKey( name, mpHost );
        pNewItem = new QTreeWidgetItem( mpKeyBaseItem, nameL );
        treeWidget_keys->insertTopLevelItem( 0, pNewItem );    
    }
    
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
        icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-red.png")), QIcon::Normal, QIcon::Off);        
    }
    else
    {
        icon.addPixmap(QPixmap(QString::fromUtf8(":/document-save-as.png")), QIcon::Normal, QIcon::Off);
    }
    pNewItem->setIcon( 0, icon );
    if( pParent ) pParent->setExpanded( true );
    mpKeysMainArea->lineEdit_command->clear();
    mpKeysMainArea->lineEdit_key->setText("no key chosen");
    mpSourceEditorArea->script_scintilla->clear();
    treeWidget_keys->setCurrentItem( pNewItem );
}  


void dlgTriggerEditor::addAlias( bool isFolder )
{
    qDebug()<<"dlgTriggerEditor::addAlias()";
    QString name;
    if( isFolder ) name = "New Alias Group";
    else name = "New Alias";
    QString regex = "";
    QString command = "";
    QString script = "";
    QStringList nameL;
    nameL << name;
    
    QTreeWidgetItem * pParent = (QTreeWidgetItem*)treeWidget_alias->currentItem();
    
    QTreeWidgetItem * pNewItem;
    TAlias * pT;
    if( pParent )
    {
        int parentID = pParent->data(0, Qt::UserRole).toInt();
        
        TAlias * pParentTrigger = mpHost->getAliasUnit()->getAlias( parentID );
        pT = new TAlias( pParentTrigger, mpHost );
        pNewItem = new QTreeWidgetItem( pParent, nameL );
        pParent->insertChild( 0, pNewItem );
    }
    else
    {
        pT = new TAlias( name, mpHost );
        pT->setRegexCode( regex );
        pNewItem = new QTreeWidgetItem( mpAliasBaseItem, nameL );
        treeWidget_alias->insertTopLevelItem( 0, pNewItem );    
    }
    
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
        icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-red.png")), QIcon::Normal, QIcon::Off);        
    }
    else
    {
        icon.addPixmap(QPixmap(QString::fromUtf8(":/document-save-as.png")), QIcon::Normal, QIcon::Off);
    }
    pNewItem->setIcon( 0, icon );
    if( pParent ) pParent->setExpanded( true );
    
    mpAliasMainArea->lineEdit_alias_name->clear();
    mpAliasMainArea->pattern_textedit->clear();
    mpAliasMainArea->pattern_textedit2->clear();
    mpSourceEditorArea->script_scintilla->clear();
    treeWidget_alias->setCurrentItem( pNewItem );
}  

void dlgTriggerEditor::addAction( bool isFolder )
{
    qDebug()<<"dlgTriggerEditor::addAction()";
    QString name;
    if( isFolder ) name = "New Action Button Group";
    else name = "New Action Button";
    QString regex = "";
    QString script = "";
    QStringList nameL;
    nameL << name;
    
    QTreeWidgetItem * pParent = (QTreeWidgetItem*)treeWidget_actions->currentItem();
    
    QTreeWidgetItem * pNewItem;
    TAction * pT;
    if( pParent )
    {
        int parentID = pParent->data(0, Qt::UserRole).toInt();
        
        TAction * pParentTrigger = mpHost->getActionUnit()->getAction( parentID );
        pT = new TAction( pParentTrigger, mpHost );
        pNewItem = new QTreeWidgetItem( pParent, nameL );
        pParent->insertChild( 0, pNewItem );
    }
    else
    {
        pT = new TAction( name, mpHost );
        pT->setRegexCode( regex );
        pNewItem = new QTreeWidgetItem( mpActionBaseItem, nameL );
        treeWidget_actions->insertTopLevelItem( 0, pNewItem );    
    }
    
    pT->setName( name );
    pT->setRegexCode( regex );
    pT->setScript( script );
    pT->setIsFolder( isFolder );
    pT->setIsActive( false );
    pT->registerAction();
    int childID = pT->getID();    
    pNewItem->setData( 0, Qt::UserRole, childID );
    QIcon icon;
    if( isFolder )
    {
        icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-red.png")), QIcon::Normal, QIcon::Off);        
    }
    else
    {
        icon.addPixmap(QPixmap(QString::fromUtf8(":/document-save-as.png")), QIcon::Normal, QIcon::Off);
    }
    pNewItem->setIcon( 0, icon );
    if( pParent ) pParent->setExpanded( true );
    mpActionsMainArea->lineEdit_action_name->clear();
    //mpActionsMainArea->pattern_textedit->clear();
    //mpActionsMainArea->pattern_textedit_2->clear();
    mpSourceEditorArea->script_scintilla->clear();
    mpHost->getActionUnit()->updateToolbar();
    treeWidget_actions->setCurrentItem( pNewItem );
}  


void dlgTriggerEditor::addScript( bool isFolder )
{
    QString name;
    if( isFolder ) name = "New Script Group";
    else name = "NewScript";
    QStringList mainFun; 
    mainFun << "----------------------------------------------------------------------------------\n"
            << "-- This is the main function of the script that gets called if an event is raised \n"
            << "-- for which this script has registered an event handler. It must have the same   \n"
            << "-- name as the script name.\n" 
            << "--\n"
            << "--      *** DON'T FORGET TO ADJUST THE FUNCTION NAME TO THE SCRIPT NAME! ***\n"
            << "--\n"
            << "-- You can define any functions you like in addition to this function, but you must \n"
            << "-- not remove it if you want your script to be called by the event system.\n"
            << "-----------------------------------------------------------------------------------\n"
            << "function "<<name<<"()\n\nend\n\n";
    QString script = mainFun.join("");
    QStringList nameL;
    nameL << name;
    
    QTreeWidgetItem * pParent = (QTreeWidgetItem*)treeWidget_scripts->currentItem();
    
    QTreeWidgetItem * pNewItem;
    TScript * pT;
    if( pParent )
    {
        int parentID = pParent->data(0, Qt::UserRole).toInt();
        TScript * pParentTrigger = mpHost->getScriptUnit()->getScript( parentID );
        pT = new TScript( pParentTrigger, mpHost );
        pNewItem = new QTreeWidgetItem( pParent, nameL );
        pParent->insertChild( 0, pNewItem );
    }
    else
    {
        pT = new TScript( name, mpHost );
        pNewItem = new QTreeWidgetItem( mpScriptsBaseItem, nameL );
        treeWidget_scripts->insertTopLevelItem( 0, pNewItem );    
    }
    
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
        icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-red.png")), QIcon::Normal, QIcon::Off);        
    }
    else
    {
        icon.addPixmap(QPixmap(QString::fromUtf8(":/document-save-as.png")), QIcon::Normal, QIcon::Off);
    }
    pNewItem->setIcon( 0, icon );
    if( pParent ) pParent->setExpanded( true );
    mpScriptsMainArea->lineEdit_scripts_name->clear();
    //FIXME mpScriptsMainArea->pattern_textedit->clear();
    mpSourceEditorArea->script_scintilla->setText( script );
    treeWidget_scripts->setCurrentItem( pNewItem );
}  

void dlgTriggerEditor::closeEvent(QCloseEvent *event)
{
    switch( mCurrentView )
    {
        case cmTriggerView:
            if (!slot_saveTriggerAfterEdit())
                event->ignore();
            break;
        
        default:
        break;
    }
}

bool dlgTriggerEditor::slot_saveTriggerAfterEdit(bool ask)
{
    bool result = true;
    QString name = mpTriggersMainArea->lineEdit_trigger_name->text();
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
        qDebug() << "itemList[i]->text()="<<itemList[i]->text();
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
        if( (fgColor == QColor(195,0,0)) && (bgColor == QColor(255,255,255)) )
        {
            regexPropertyList << REGEX_WILDCARD;
        }
        if( (fgColor == QColor(0,155,0)) && (bgColor == QColor(255,255,255)) )
        {
            regexPropertyList << REGEX_EXACT_MATCH;
        }
    }
    QString script = mpSourceEditorArea->script_scintilla->text();    
    
    QTreeWidgetItem * pItem = treeWidget->currentItem(); 
    if( pItem->parent() )
    {
        int triggerID = pItem->data( 0, Qt::UserRole ).toInt();
        TTrigger * pT2 = mpHost->getTriggerUnit()->getTrigger( triggerID );
        TTrigger * pT = new TTrigger(pT2->getParent(), mpHost);
        pT->clone(*pT2);
        if( pT )
        {
            pT->setName( name );
            pT->setRegexCodeList( regexList, regexPropertyList );
            pT->setTriggerType( mpTriggersMainArea->comboBox_regexstyle->currentIndex() );
            pT->setScript( script );
            pT->setIsMultiline( isMultiline );
            pT->setConditionLineDelta( mpTriggersMainArea->spinBox_linemargin->value() );
            QIcon icon;
            if( pT->isFolder() )
            {
                if( pT->isActive() )
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-blue.png")), QIcon::Normal, QIcon::Off);    
                }
                else
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-blue-locked.png")), QIcon::Normal, QIcon::Off);    
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

            if (!pT->isClone(*pT2))
            {
                if (ask)
                {
                    QMessageBox::StandardButton ask_result = QMessageBox::question ( this, tr("Trigger was modified"), tr("trigger \"%1\" was modified, do you want to save it?").arg(name), QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);

                    if (ask_result == QMessageBox::Cancel)
                        result = false;
                    else if (ask_result == QMessageBox::Save)
                        pT2->clone(*pT);
                } else {
                    pT2->clone(*pT);
                }
            }
            
            pItem->setIcon( 0, icon); 
            pItem->setText( 0, name );
        }
        delete pT;
    }
    mpTriggerMainAreaEditRegexItem = 0;
    return result;
}

void dlgTriggerEditor::slot_saveTimerAfterEdit()
{
    QString name = mpTimersMainArea->lineEdit_timer_name->text();
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
            //qDebug()<<"TIME="<<hours<<":"<<minutes<<"'"<<secs<<"''"<<msecs;
            QTime time(hours,minutes,secs,msecs);
            pT->setTime( time );
            pT->setCommand( command );
            pT->setScript( script );
            
            pT->setIsActive( false );
            
            QIcon icon;
            if( pT->isFolder() )
            {
                if( pT->isActive() )
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-green.png")), QIcon::Normal, QIcon::Off);    
                }
                else
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-green-locked.png")), QIcon::Normal, QIcon::Off);    
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
            pItem->setText( 0, name );
        }
    }
}

void dlgTriggerEditor::slot_saveAliasAfterEdit()
{
    
    QString name = mpAliasMainArea->lineEdit_alias_name->text();
    QString regex = mpAliasMainArea->pattern_textedit->text();
    if( (name.size() < 1) || (name=="New Alias") ) name = regex;
    QString command = mpAliasMainArea->pattern_textedit2->text();
    QString script = mpSourceEditorArea->script_scintilla->text();    
    QTreeWidgetItem * pItem = treeWidget_alias->currentItem(); 
    if( pItem )
    {
        int triggerID = pItem->data(0, Qt::UserRole).toInt();
        TAlias * pT = mpHost->getAliasUnit()->getAlias( triggerID );
        if( pT )
        {
            pT->setName( name );
            pT->setCommand( command );
            pT->setRegexCode( regex );
            pT->setScript( script );
            
            pT->setIsActive( true );//FIXME: discuss if new triggers are automatically active
            
            QIcon icon;
            if( pT->isFolder() )
            {
                if( pT->isActive() )
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-violet.png")), QIcon::Normal, QIcon::Off);    
                }
                else
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-violet-locked.png")), QIcon::Normal, QIcon::Off);    
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
            
            pItem->setIcon( 0, icon); 
            pItem->setText( 0, name );
        }
    }
}

void dlgTriggerEditor::slot_saveActionAfterEdit()
{
    
    QString name = mpActionsMainArea->lineEdit_action_name->text();
    //QString regex = mpAliasMainArea->pattern_textedit->toPlainText();
    //QString command = mpAliasMainArea->pattern_textedit_2->toPlainText();
    QString script = mpSourceEditorArea->script_scintilla->text();    
    bool isChecked = mpActionsMainArea->checkBox_pushdownbutton->isChecked();
    QTreeWidgetItem * pItem = treeWidget_actions->currentItem(); 
    if( pItem )
    {
        int triggerID = pItem->data(0, Qt::UserRole).toInt();
        TAction * pT = mpHost->getActionUnit()->getAction( triggerID );
        if( pT )
        {
            pT->setName( name );
            //pT->setCommand( command );
            //pT->setRegexCode( regex );
            pT->setScript( script );
            pT->setIsPushDownButton( isChecked );
            pT->setIsActive( true );
            
            QIcon icon;
            if( pT->isFolder() )
            {
                if( pT->isActive() )
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-cyan.png")), QIcon::Normal, QIcon::Off);    
                }
                else
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-cyan-locked.png")), QIcon::Normal, QIcon::Off);    
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
            
            pItem->setIcon( 0, icon); 
            pItem->setText( 0, name );
        }
    }
    mpHost->getActionUnit()->updateToolbar();
}


void dlgTriggerEditor::slot_saveScriptAfterEdit()
{
    QString name = mpScriptsMainArea->lineEdit_scripts_name->text();
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
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-orange.png")), QIcon::Normal, QIcon::Off);    
                }
                else
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-orange-locked.png")), QIcon::Normal, QIcon::Off);    
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
            
            pItem->setIcon( 0, icon); 
            pItem->setText( 0, name );
        }
    }
}

void dlgTriggerEditor::slot_saveKeyAfterEdit()
{
    
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
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-violet.png")), QIcon::Normal, QIcon::Off);    
                }
                else
                {
                    icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-violet-locked.png")), QIcon::Normal, QIcon::Off);    
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
            
            pItem->setIcon( 0, icon); 
            pItem->setText( 0, name );
        }
    }
}


void dlgTriggerEditor::slot_deleteProfile()
{
}

void dlgTriggerEditor::slot_trigger_clicked( QTreeWidgetItem *pItem, QTreeWidgetItem *previous)
{
    TTrigger * pT = NULL;
    if (pItem)
    {
        mpTriggersMainArea->lineEdit_trigger_name->setText(pItem->text(0));
        int ID = pItem->data(0,Qt::UserRole).toInt();
        pT = mpHost->getTriggerUnit()->getTrigger(ID);
    }

    if( pT )
    {
        QStringList patternList = pT->getRegexCodeList();
        QList<int> propertyList = pT->getRegexCodePropertyList();
        mpTriggersMainArea->listWidget_regex_list->clear();
        if( patternList.size() != propertyList.size() )
        {
            qDebug()<<"CRITICAL ERROR: dlgTriggerEditor::slot_trigger_clicked(): patternList.size() != propertyList.size()";
            return;
        }
                
        for( int i=0; i<patternList.size(); i++ )
        {
            QListWidgetItem * pItem = new QListWidgetItem( mpTriggersMainArea->listWidget_regex_list );
            switch( propertyList[i] )
            {
            case REGEX_SUBSTRING:
                pItem->setForeground(QColor(0,0,0));
                pItem->setBackground(QColor(255,255,255));
                break;
            case REGEX_PERL:
                pItem->setForeground(QColor(0,0,255));
                pItem->setBackground(QColor(255,255,255));
                break;
            case REGEX_WILDCARD:
                pItem->setForeground(QColor(195,0,0));
                pItem->setBackground(QColor(255,255,255));
                break;
            case REGEX_EXACT_MATCH:
                pItem->setForeground(QColor(0,155,0));
                pItem->setBackground(QColor(255,255,255));
                break;
            }
            pItem->setText( patternList[i] );
            mpTriggersMainArea->listWidget_regex_list->addItem( pItem );
        }

        QIcon icon;
        if( pT->isFolder() )
        {
            if( pT->isActive() )
            {
                icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-blue.png")), QIcon::Normal, QIcon::Off);    
            }
            else
            {
                icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-blue-locked.png")), QIcon::Normal, QIcon::Off);    
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
        
        
        mpTriggersMainArea->comboBox_regexstyle->setCurrentIndex( REGEX_SUBSTRING );
        mpTriggersMainArea->checkBox_multlinetrigger->setChecked( pT->isMultiline() );
        mpTriggersMainArea->spinBox_linemargin->setValue( pT->getConditionLineDelta() );
        QString script = pT->getScript();
        mpSourceEditorArea->script_scintilla->setText( script );
        mpTriggersMainArea->comboBox_regexstyle->setEnabled(true);
        mpTriggersMainArea->checkBox_multlinetrigger->setEnabled(true);
        mpSourceEditorArea->script_scintilla->setEnabled(true);
        mpTriggersMainArea->spinBox_linemargin->setEnabled(true);
        mpTriggersMainArea->listWidget_regex_list->setEnabled(true);
        mpTriggersMainArea->lineEdit_trigger_name->setEnabled(true);
        mpTriggersMainArea->lineEdit->setEnabled(true);
        mpTriggersMainArea->toolButton_add->setEnabled(true);
        mpTriggersMainArea->toolButton_remove->setEnabled(true);
    } else {
        mpSourceEditorArea->script_scintilla->clear();
        mpTriggersMainArea->spinBox_linemargin->clear();
        mpTriggersMainArea->listWidget_regex_list->clear();
        mpTriggersMainArea->checkBox_multlinetrigger->setChecked(false);
        mpTriggersMainArea->comboBox_regexstyle->setEnabled(false);
        mpTriggersMainArea->checkBox_multlinetrigger->setEnabled(false);
        mpSourceEditorArea->script_scintilla->setEnabled(false);
        mpTriggersMainArea->spinBox_linemargin->setEnabled(false);
        mpTriggersMainArea->listWidget_regex_list->setEnabled(false);
        mpTriggersMainArea->lineEdit_trigger_name->setEnabled(false);
        mpTriggersMainArea->lineEdit->setEnabled(false);
        mpTriggersMainArea->toolButton_add->setEnabled(false);
        mpTriggersMainArea->toolButton_remove->setEnabled(false);
    }
    mpTriggerMainAreaEditRegexItem = 0;
}

void dlgTriggerEditor::slot_alias_clicked( QTreeWidgetItem *pItem, QTreeWidgetItem *previous)
{
    TAlias * pT = NULL;
    if (pItem)
    {
        mpAliasMainArea->lineEdit_alias_name->setText(pItem->text(0));
        int ID = pItem->data(0,Qt::UserRole).toInt();
        pT = mpHost->getAliasUnit()->getAlias(ID);
    }

    if( pT )
    {
        QString pattern = pT->getRegexCode();
        QString command = pT->getCommand();
        QString name = pT->getName();
        mpAliasMainArea->pattern_textedit->clear();
        mpAliasMainArea->pattern_textedit2->clear();
        mpAliasMainArea->lineEdit_alias_name->clear();
        mpAliasMainArea->pattern_textedit->setText( pattern );    
        mpAliasMainArea->pattern_textedit2->setText( command );
        mpAliasMainArea->lineEdit_alias_name->setText( name );
        QString script = pT->getScript();
        mpSourceEditorArea->script_scintilla->setText( script );
        mpSourceEditorArea->script_scintilla->setEnabled(true);
        mpAliasMainArea->pattern_textedit->setEnabled(true);
        mpAliasMainArea->pattern_textedit2->setEnabled(true);
        mpAliasMainArea->lineEdit_alias_name->setEnabled(true);
        mpSourceEditorArea->script_scintilla->setEnabled(true);
    } else {
        mpSourceEditorArea->script_scintilla->clear();
        mpAliasMainArea->pattern_textedit->clear();
        mpAliasMainArea->pattern_textedit2->clear();
        mpSourceEditorArea->script_scintilla->setEnabled(false);
        mpAliasMainArea->pattern_textedit->setEnabled(false);
        mpAliasMainArea->pattern_textedit2->setEnabled(false);
        mpAliasMainArea->lineEdit_alias_name->setEnabled(false);
        mpSourceEditorArea->script_scintilla->setEnabled(false);
    }
}

void dlgTriggerEditor::slot_key_clicked( QTreeWidgetItem *pItem, QTreeWidgetItem *previous)
{
    TKey * pT = NULL;
    if (pItem)
    {
        mpKeysMainArea->lineEdit_key->setText( pItem->text(0) );
        int ID = pItem->data( 0, Qt::UserRole ).toInt();
        pT = mpHost->getKeyUnit()->getKey(ID);
    }

    if( pT )
    {
        QIcon icon;
        if( pT->isFolder() )
        {
            if( pT->isActive() )
            {
                icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-cyan.png")), QIcon::Normal, QIcon::Off);    
            }
            else
            {
                icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-cyan-locked.png")), QIcon::Normal, QIcon::Off);    
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

        QString command = pT->getCommand();
        QString name = pT->getName();
        mpKeysMainArea->lineEdit_command->clear();
        mpKeysMainArea->lineEdit_command->setText( command );
        mpKeysMainArea->lineEdit_name->setText( name );
        QString keyName = mpHost->getKeyUnit()->getKeyName( pT->getKeyCode(), pT->getKeyModifiers() );
        mpKeysMainArea->lineEdit_key->setText( keyName );
        QString script = pT->getScript();
        mpSourceEditorArea->script_scintilla->setText( script );
        mpKeysMainArea->lineEdit_key->setEnabled(true);
        mpKeysMainArea->lineEdit_name->setEnabled(true);
        mpKeysMainArea->lineEdit_command->setEnabled(true);
        mpSourceEditorArea->script_scintilla->setEnabled(true);
        mpKeysMainArea->pushButton_grabKey->setEnabled(true);
    } else {
        mpSourceEditorArea->script_scintilla->clear();
        mpKeysMainArea->lineEdit_command->clear();
        mpKeysMainArea->lineEdit_name->clear();
        mpKeysMainArea->lineEdit_key->clear();
        mpSourceEditorArea->script_scintilla->setEnabled(false);
        mpKeysMainArea->lineEdit_key->setEnabled(false);
        mpKeysMainArea->lineEdit_name->setEnabled(false);
        mpKeysMainArea->lineEdit_command->setEnabled(false);
        mpKeysMainArea->pushButton_grabKey->setEnabled(false);
    }
}

void dlgTriggerEditor::slot_action_clicked( QTreeWidgetItem *pItem, QTreeWidgetItem *previous)
{
    TAction * pT = NULL;
    if (pItem)
    {
        mpActionsMainArea->lineEdit_action_name->setText(pItem->text(0));
        int ID = pItem->data(0,Qt::UserRole).toInt();
        pT = mpHost->getActionUnit()->getAction(ID);
    }

    if( pT )
    {
        //QString pattern = pT->getRegexCode();
        //QString command = pT->getCommand();
        QString name = pT->getName();
        //mpActionsMainArea->pattern_textedit->clear();
        //mpActionsMainArea->pattern_textedit_2->clear();
        mpActionsMainArea->lineEdit_action_name->clear();
        //mpActionsMainArea->pattern_textedit->insertPlainText( pattern );    
        //mpActionsMainArea->pattern_textedit_2->insertPlainText( command );
        mpActionsMainArea->lineEdit_action_name->setText( name );
        mpActionsMainArea->checkBox_pushdownbutton->setEnabled( pT->isPushDownButton() );
        QString script = pT->getScript();
        mpSourceEditorArea->script_scintilla->setText( script );

        mpSourceEditorArea->script_scintilla->setEnabled(true);
        mpActionsMainArea->lineEdit_action_name->setEnabled(true);
        mpActionsMainArea->lineEdit_action_button_down->setEnabled(true);
        mpActionsMainArea->lineEdit_action_button_up->setEnabled(true);
        mpActionsMainArea->lineEdit_action_icon->setEnabled(true);
        mpActionsMainArea->checkBox_pushdownbutton->setEnabled(true);
        mpActionsMainArea->pushButton_chose_icon->setEnabled(true);
    } else {
        mpSourceEditorArea->script_scintilla->clear();
        mpActionsMainArea->lineEdit_action_icon->clear();
        mpActionsMainArea->checkBox_pushdownbutton->setChecked(false);
        mpSourceEditorArea->script_scintilla->setEnabled(false);
        mpActionsMainArea->lineEdit_action_name->setEnabled(false);
        mpActionsMainArea->lineEdit_action_button_down->setEnabled(false);
        mpActionsMainArea->lineEdit_action_button_up->setEnabled(false);
        mpActionsMainArea->lineEdit_action_icon->setEnabled(false);
        mpActionsMainArea->checkBox_pushdownbutton->setEnabled(false);
        mpActionsMainArea->pushButton_chose_icon->setEnabled(false);
    }
}


void dlgTriggerEditor::slot_scripts_clicked( QTreeWidgetItem *pItem, QTreeWidgetItem *previous)
{
    TScript * pT = NULL;
    if (pItem)
    {
        mpScriptsMainArea->listWidget_registered_event_handlers->clear();
        mpScriptsMainArea->lineEdit_scripts_name->setText(pItem->text(0));
        int ID = pItem->data(0,Qt::UserRole).toInt();
        pT = mpHost->getScriptUnit()->getScript(ID);
    }

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
        mpSourceEditorArea->script_scintilla->setText( script );
        mpScriptsMainArea->lineEdit_scripts_name->setText( name );

        mpSourceEditorArea->script_scintilla->setEnabled(true);
        mpScriptsMainArea->lineEdit_scripts_name->setEnabled(true);
        mpScriptsMainArea->listWidget_registered_event_handlers->setEnabled(true);
        mpScriptsMainArea->comboBox_add_system_event_handler->setEnabled(true);
        mpScriptsMainArea->lineEdit->setEnabled(true);
        mpScriptsMainArea->toolButton_add->setEnabled(true);
        mpScriptsMainArea->toolButton_remove->setEnabled(true);
    } else {
        mpSourceEditorArea->script_scintilla->clear();
        mpScriptsMainArea->listWidget_registered_event_handlers->clear();
        mpScriptsMainArea->lineEdit->clear();
        mpSourceEditorArea->script_scintilla->setEnabled(false);
        mpScriptsMainArea->lineEdit_scripts_name->setEnabled(false);
        mpScriptsMainArea->listWidget_registered_event_handlers->setEnabled(false);
        mpScriptsMainArea->comboBox_add_system_event_handler->setEnabled(false);
        mpScriptsMainArea->lineEdit->setEnabled(false);
        mpScriptsMainArea->toolButton_add->setEnabled(false);
        mpScriptsMainArea->toolButton_remove->setEnabled(false);
    }
}

void dlgTriggerEditor::slot_timer_clicked( QTreeWidgetItem *pItem, QTreeWidgetItem *previous)
{
    TTimer * pT = NULL;
    if (pItem)
    {
        mpTimersMainArea->lineEdit_timer_name->setText(pItem->text(0));
        int ID = pItem->data(0,Qt::UserRole).toInt();
        pT = mpHost->getTimerUnit()->getTimer(ID);
    }

    if( pT )
    {
        QIcon icon;
        if( pT->isFolder() )
        {
            if( pT->isActive() )
            {
                icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-green.png")), QIcon::Normal, QIcon::Off);    
            }
            else
            {
                icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-green-locked.png")), QIcon::Normal, QIcon::Off);    
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

        QString command = pT->getCommand();
        mpTimersMainArea->lineEdit_command->clear();
        mpTimersMainArea->lineEdit_command->setText( command );    
        QTime time = pT->getTime();
        int hours = time.hour();
        int minutes = time.minute();
        int secs = time.second();
        int msecs = time.msec();
        qDebug()<<"slot_timer_clicked() TIME="<<hours<<":"<<minutes<<"'"<<secs<<"''"<<msecs;
        
        QTime t2(hours,0,0,0);
        mpTimersMainArea->timeEdit_hours->setTime(t2);
        
        QTime t3(0,minutes,0,0);
        mpTimersMainArea->timeEdit_minutes->setTime(t3);
        
        QTime t4(0,0,secs);
        mpTimersMainArea->timeEdit_seconds->setTime(t4);
        
        QTime t5(0,0,0,msecs);
        mpTimersMainArea->timeEdit_msecs->setTime(t5);
        
        QString script = pT->getScript();
        mpSourceEditorArea->script_scintilla->setText( script );
        mpTimersMainArea->lineEdit_timer_name->setEnabled(true);
        mpTimersMainArea->lineEdit_command->setEnabled(true);
        mpTimersMainArea->checkBox->setEnabled(true);
        mpTimersMainArea->timeEdit_hours->setEnabled(true);
        mpTimersMainArea->timeEdit_minutes->setEnabled(true);
        mpTimersMainArea->timeEdit_seconds->setEnabled(true);
        mpTimersMainArea->timeEdit_msecs->setEnabled(true);
        mpSourceEditorArea->script_scintilla->setEnabled(true);
    } else {
        mpSourceEditorArea->script_scintilla->clear();
        mpTimersMainArea->lineEdit_command->clear();
        mpTimersMainArea->timeEdit_hours->clear();
        mpTimersMainArea->timeEdit_minutes->clear();
        mpTimersMainArea->timeEdit_seconds->clear();
        mpTimersMainArea->timeEdit_msecs->clear();
        mpTimersMainArea->checkBox->setChecked(false);
        mpTimersMainArea->lineEdit_timer_name->setEnabled(false);
        mpTimersMainArea->lineEdit_command->setEnabled(false);
        mpTimersMainArea->checkBox->setEnabled(false);
        mpTimersMainArea->timeEdit_hours->setEnabled(false);
        mpTimersMainArea->timeEdit_minutes->setEnabled(false);
        mpTimersMainArea->timeEdit_seconds->setEnabled(false);
        mpTimersMainArea->timeEdit_msecs->setEnabled(false);
        mpSourceEditorArea->script_scintilla->setEnabled(false);
    }
}


void dlgTriggerEditor::fillout_form()
{
    QStringList sL;
    sL << "Triggers";
    mpTriggerBaseItem = new QTreeWidgetItem( (QTreeWidgetItem*)0, sL );
    mpTriggerBaseItem->setBackground(0,QColor(255,254,215,255));
    QIcon mainIcon;
    mainIcon.addPixmap(QPixmap(QString::fromUtf8(":/tools-wizard.png")), QIcon::Normal, QIcon::Off);    
    mpTriggerBaseItem->setIcon( 0, mainIcon );
    treeWidget->insertTopLevelItem( 0, mpTriggerBaseItem );
    list<TTrigger *> baseNodeList = mpHost->getTriggerUnit()->getTriggerRootNodeList();
    typedef list<TTrigger *>::iterator IT;
    for(IT it=baseNodeList.begin(); it!=baseNodeList.end(); it++ )
    {
        TTrigger * pT = *it;
        QString s = pT->getName();
        QStringList sList;
        sList << s;
        QTreeWidgetItem * pItem = new QTreeWidgetItem( mpTriggerBaseItem, sList);
        pItem->setData( 0, Qt::UserRole, QVariant(pT->getID()) );
        mpTriggerBaseItem->addChild( pItem );    
        QIcon icon;
        if( pT->isFolder() )
        {
            expand_child_triggers( pT, (QTreeWidgetItem*)pItem );
            if( pT->isActive() )
            {
                icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-blue.png")), QIcon::Normal, QIcon::Off);    
            }
            else
            {
                icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-blue-locked.png")), QIcon::Normal, QIcon::Off);    
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
    mpTriggerBaseItem->setExpanded( true );
    
    QStringList sL2;
    sL2 << "Timers";
    mpTimerBaseItem = new QTreeWidgetItem( (QTreeWidgetItem*)0, sL2 );
    mpTimerBaseItem->setBackground(0,QColor(255,254,215,255));
    QIcon mainIcon2;
    mainIcon2.addPixmap(QPixmap(QString::fromUtf8(":/chronometer.png")), QIcon::Normal, QIcon::Off);    
    mpTimerBaseItem->setIcon( 0, mainIcon2 );
    treeWidget_timers->insertTopLevelItem( 0, mpTimerBaseItem );
    mpTriggerBaseItem->setExpanded( true );
    list<TTimer *> baseNodeList_timers = mpHost->getTimerUnit()->getTimerRootNodeList();
    
    for( list<TTimer *>::iterator it = baseNodeList_timers.begin(); it!=baseNodeList_timers.end(); it++ )
    {
        TTimer * pT = *it;
        QString s = pT->getName();
        //        TTimer * pTimer = *it;
        QStringList sList;
        sList << s;
        QTreeWidgetItem * pItem = new QTreeWidgetItem( mpTimerBaseItem, sList);
        pItem->setData( 0, Qt::UserRole, QVariant(pT->getID()) );
        mpTimerBaseItem->addChild( pItem );    
        QIcon icon;
        if( pT->isFolder() )
        {
            expand_child_timers( pT, (QTreeWidgetItem*)pItem );
            if( pT->isActive() )
            {
                icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-green.png")), QIcon::Normal, QIcon::Off);    
            }
            else
            {
                icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-green-locked.png")), QIcon::Normal, QIcon::Off);    
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
    mpTimerBaseItem->setExpanded( true );
    
    QStringList sL3;
    sL3 << "Scripts";
    mpScriptsBaseItem = new QTreeWidgetItem( (QTreeWidgetItem*)0, sL3 );
    mpScriptsBaseItem->setBackground(0,QColor(255,254,215,255));
    QIcon mainIcon3;
    mainIcon3.addPixmap(QPixmap(QString::fromUtf8(":/accessories-text-editor.png")), QIcon::Normal, QIcon::Off);    
    mpScriptsBaseItem->setIcon( 0, mainIcon3 );
    treeWidget_scripts->insertTopLevelItem( 0, mpScriptsBaseItem );
    mpScriptsBaseItem->setExpanded( true );
    list<TScript *> baseNodeList_scripts = mpHost->getScriptUnit()->getScriptRootNodeList();
    
    for( list<TScript *>::iterator it = baseNodeList_scripts.begin(); it!=baseNodeList_scripts.end(); it++ )
    {
        TScript * pT = *it;
        QString s = pT->getName();
        //        TTimer * pTimer = *it;
        QStringList sList;
        sList << s;
        QTreeWidgetItem * pItem = new QTreeWidgetItem( mpScriptsBaseItem, sList);
        pItem->setData( 0, Qt::UserRole, QVariant(pT->getID()) );
        mpScriptsBaseItem->addChild( pItem );    
        QIcon icon;
        if( pT->isFolder() )
        {
            expand_child_scripts( pT, (QTreeWidgetItem*)pItem );
            if( pT->isActive() )
            {
                icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-orange.png")), QIcon::Normal, QIcon::Off);    
            }
            else
            {
                icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-orange-locked.png")), QIcon::Normal, QIcon::Off);    
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
    mpScriptsBaseItem->setExpanded( true );
    
    QStringList sL4;
    sL4 << "Aliases";
    mpAliasBaseItem = new QTreeWidgetItem( (QTreeWidgetItem*)0, sL4 );
    mpAliasBaseItem->setBackground(0,QColor(255,254,215,255));
    QIcon mainIcon4;
    mainIcon4.addPixmap(QPixmap(QString::fromUtf8(":/system-users.png")), QIcon::Normal, QIcon::Off);    
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
        qDebug()<<"setting data id="<<pT->getID();
        mpAliasBaseItem->addChild( pItem );    
        QIcon icon;
        if( pT->isFolder() )
        {
            expand_child_alias( pT, (QTreeWidgetItem*)pItem );
            if( pT->isActive() )
            {
                icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-violet.png")), QIcon::Normal, QIcon::Off);    
            }
            else
            {
                icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-violet-locked.png")), QIcon::Normal, QIcon::Off);    
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
    mpAliasBaseItem->setExpanded( true );
    
    QStringList sL5;
    sL5 << "Action Buttons";
    mpActionBaseItem = new QTreeWidgetItem( (QTreeWidgetItem*)0, sL5 );
    mpActionBaseItem->setBackground(0,QColor(255,254,215,255));
    QIcon mainIcon5;
    mainIcon5.addPixmap(QPixmap(QString::fromUtf8(":/bookmarks.png")), QIcon::Normal, QIcon::Off);    
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
        if( pT->isFolder() )
        {
            expand_child_action( pT, (QTreeWidgetItem*)pItem );
            if( pT->isActive() )
            {
                icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-cyan.png")), QIcon::Normal, QIcon::Off);    
            }
            else
            {
                icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-cyan-locked.png")), QIcon::Normal, QIcon::Off);    
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
    mpActionBaseItem->setExpanded( true );
    
    QStringList sL6;
    sL5 << "Action Keys";
    mpKeyBaseItem = new QTreeWidgetItem( (QTreeWidgetItem*)0, sL6 );
    mpKeyBaseItem->setBackground(0,QColor(255,254,215,255));
    QIcon mainIcon6;
    mainIcon6.addPixmap(QPixmap(QString::fromUtf8(":/preferences-desktop-keyboard.png")), QIcon::Normal, QIcon::Off);    
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
        if( pT->isFolder() )
        {
            expand_child_key( pT, (QTreeWidgetItem*)pItem );
            if( pT->isActive() )
            {
                icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-cyan.png")), QIcon::Normal, QIcon::Off);    
            }
            else
            {
                icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-cyan-locked.png")), QIcon::Normal, QIcon::Off);    
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
        if( pT->isFolder() )
        {
            expand_child_triggers( pT, pItem );
            if( pT->isActive() )
            {
                icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-blue.png")), QIcon::Normal, QIcon::Off);    
            }
            else
            {
                icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-blue-locked.png")), QIcon::Normal, QIcon::Off);    
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
        if( pT->isFolder() )
        {
            expand_child_key( pT, pItem );
            if( pT->isActive() )
            {
                icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-blue.png")), QIcon::Normal, QIcon::Off);    
            }
            else
            {
                icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-blue-locked.png")), QIcon::Normal, QIcon::Off);    
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
        if( pT->isFolder() )
        {
            expand_child_scripts( pT, pItem );
            if( pT->isActive() )
            {
                icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-orange.png")), QIcon::Normal, QIcon::Off);    
            }
            else
            {
                icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-orange-locked.png")), QIcon::Normal, QIcon::Off);    
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
        if( pT->isFolder() )
        {
            expand_child_alias( pT, pItem );
            if( pT->isActive() )
            {
                icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-violet.png")), QIcon::Normal, QIcon::Off);    
            }
            else
            {
                icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-violet-locked.png")), QIcon::Normal, QIcon::Off);    
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
        if( pT->isFolder() )
        {
            expand_child_action( pT, pItem );
            if( pT->isActive() )
            {
                icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-cyan.png")), QIcon::Normal, QIcon::Off);    
            }
            else
            {
                icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-cyan-locked.png")), QIcon::Normal, QIcon::Off);    
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
        if( pT->isFolder() )
        {
            expand_child_timers( pT, pItem );
            if( pT->isActive() )
            {
                icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-green.png")), QIcon::Normal, QIcon::Off);    
            }
            else
            {
                icon.addPixmap(QPixmap(QString::fromUtf8(":/folder-green-locked.png")), QIcon::Normal, QIcon::Off);    
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
    }
    else 
    {
        mpSearchArea->show();
    }
}

void dlgTriggerEditor::slot_show_timers()
{
    
    mIsTriggerMainAreaEditRegex = false;
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
    
    mpTimersMainArea->show();
    mpSourceEditorArea->show();
    //mpOptionsAreaTimers->show();
    treeWidget_timers->show(); 
    slot_timer_clicked( treeWidget_timers->currentItem(), NULL);
    
}

void dlgTriggerEditor::slot_show_triggers()
{
    
    mIsTriggerMainAreaEditRegex = false;
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
    mpTriggersMainArea->show();
    mpSourceEditorArea->show();
    //mpOptionsAreaTriggers->show();
    treeWidget->show();
    slot_trigger_clicked( treeWidget->currentItem(), NULL);
    mpTriggerMainAreaEditRegexItem = 0;
}

void dlgTriggerEditor::slot_show_scripts()
{
    
    mIsTriggerMainAreaEditRegex = false;
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
    mpScriptsMainArea->show();
    treeWidget_keys->hide();
    mpSourceEditorArea->show();
    //mpOptionsAreaScripts->show();
    treeWidget_scripts->show();
    slot_scripts_clicked( treeWidget_scripts->currentItem(), NULL);
}

void dlgTriggerEditor::slot_show_keys()
{
    
    mIsTriggerMainAreaEditRegex = false;
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
    mpKeysMainArea->show();
    mpSourceEditorArea->show();
    //mpOptionsAreaScripts->show();
    treeWidget_keys->show();
    slot_key_clicked( treeWidget_keys->currentItem(), NULL);
}


void dlgTriggerEditor::slot_show_aliases()
{
    
    mIsTriggerMainAreaEditRegex = false;
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
    
    mpAliasMainArea->show();
    mpSourceEditorArea->show();
    //mpOptionsAreaAlias->show();
    treeWidget_alias->show();
    slot_alias_clicked( treeWidget_alias->currentItem(), NULL);
}

void dlgTriggerEditor::slot_show_actions()
{
    
    mIsTriggerMainAreaEditRegex = false;
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
    
    mpActionsMainArea->show();
    mpSourceEditorArea->show();
    //mpOptionsAreaActions->show();
    treeWidget_actions->show();
    mIsTriggerMainAreaEditRegex = false;
    slot_action_clicked( treeWidget_actions->currentItem(), NULL);
}

void dlgTriggerEditor::slot_save_edit()
{
    switch( mCurrentView )
    {
        case cmTriggerView:
            slot_saveTriggerAfterEdit(false);
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

void dlgTriggerEditor::slot_trigger_main_area_edit_regex(QListWidgetItem*)
{
    QListWidgetItem * pItem = mpTriggersMainArea->listWidget_regex_list->currentItem();
    if( ! pItem ) return;
    mIsTriggerMainAreaEditRegex = true;
    mpTriggerMainAreaEditRegexItem = pItem;
    QString regex = pItem->text();
    if( regex.size() < 1 )
    {
        mIsTriggerMainAreaEditRegex = false;
        return;
    }
    mpTriggersMainArea->lineEdit->setText( regex );
    QColor fgColor = pItem->foreground().color();
    QColor bgColor = pItem->background().color();
    if( (fgColor == QColor(0,0,0)) && (bgColor == QColor(255,255,255)) )
    {
        mpTriggersMainArea->comboBox_regexstyle->setCurrentIndex(REGEX_SUBSTRING);
    }
    if( (fgColor == QColor(0,0,255)) && (bgColor == QColor(255,255,255)) )
    {
        mpTriggersMainArea->comboBox_regexstyle->setCurrentIndex(REGEX_PERL);;
    }
    if( (fgColor == QColor(195,0,0)) && (bgColor == QColor(255,255,255)) )
    {
        mpTriggersMainArea->comboBox_regexstyle->setCurrentIndex(REGEX_WILDCARD);
    }
    if( (fgColor == QColor(0,155,0)) && (bgColor == QColor(255,255,255)) )
    {
        mpTriggersMainArea->comboBox_regexstyle->setCurrentIndex(REGEX_EXACT_MATCH);
    }
}
                       
void dlgTriggerEditor::slot_trigger_main_area_delete_regex()
{
    mpTriggersMainArea->listWidget_regex_list->takeItem( mpTriggersMainArea->listWidget_regex_list->currentRow() );     
}

void dlgTriggerEditor::slot_trigger_main_area_add_regex()
{
    if( mIsTriggerMainAreaEditRegex )
    {
        if( ! mpTriggerMainAreaEditRegexItem )
        {
            mIsTriggerMainAreaEditRegex = false;
            goto LAZY;
            return;
        }
        QListWidgetItem * pItem = mpTriggersMainArea->listWidget_regex_list->currentItem();
        if( ! pItem ) return;
        pItem->setText( mpTriggersMainArea->lineEdit->text() );
        int idx = mpTriggersMainArea->comboBox_regexstyle->currentIndex();
        switch( idx )
        {
            case REGEX_SUBSTRING:
                pItem->setForeground(QColor(0,0,0));
                pItem->setBackground(QColor(255,255,255));
                break;
            case REGEX_PERL:
                pItem->setForeground(QColor(0,0,255));
                pItem->setBackground(QColor(255,255,255));
                break;
            case REGEX_WILDCARD:
                pItem->setForeground(QColor(195,0,0));
                pItem->setBackground(QColor(255,255,255));
                break;
            case REGEX_EXACT_MATCH:
                pItem->setForeground(QColor(0,155,0));
                pItem->setBackground(QColor(255,255,255));
                break;
        }
        mIsTriggerMainAreaEditRegex = false;
        mpTriggerMainAreaEditRegexItem = 0;
    }
    else
    {
    LAZY:
        if( mpTriggersMainArea->lineEdit->text().size() < 1 ) return;
        QListWidgetItem * pItem = new QListWidgetItem;
        pItem->setText( mpTriggersMainArea->lineEdit->text() );
        int idx = mpTriggersMainArea->comboBox_regexstyle->currentIndex();
        switch( idx )
        {
        case REGEX_SUBSTRING:
            pItem->setForeground(QColor(0,0,0));
            pItem->setBackground(QColor(255,255,255));
            break;
        case REGEX_PERL:
            pItem->setForeground(QColor(0,0,255));
            pItem->setBackground(QColor(255,255,255));
            break;
        case REGEX_WILDCARD:
            pItem->setForeground(QColor(195,0,0));
            pItem->setBackground(QColor(255,255,255));
            break;
        case REGEX_EXACT_MATCH:
            pItem->setForeground(QColor(0,155,0));
            pItem->setBackground(QColor(255,255,255));
            break;
        }    
        mpTriggersMainArea->listWidget_regex_list->addItem( pItem );
    
    }
    mpTriggersMainArea->lineEdit->clear();
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
}


void dlgTriggerEditor::slot_export()
{
    QString fileName = QFileDialog::getExistingDirectory(this, tr("Select Profile"),
        QDir::homePath(),
        QFileDialog::ShowDirsOnly
        | QFileDialog::DontResolveSymlinks);
    if( fileName.isEmpty() ) return;
    
    mpHost->exportHost( fileName );
    return;
}

void dlgTriggerEditor::slot_import()
{
    QString fileName = QFileDialog::getExistingDirectory(this, tr("Select Profile"),
        QDir::homePath(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if( fileName.isEmpty() ) return;
    
    HostManager::self()->importHost( fileName );
    return;
}

bool dlgTriggerEditor::event( QEvent * event )
{
    if( mIsGrabKey ) 
    {
        if( event->type() == QEvent::KeyPress ) 
        {
            QKeyEvent *ke = static_cast<QKeyEvent *>( event );
            grab_key_callback( ke->key(), ke->modifiers() );
            mIsGrabKey = false;
            ke->accept();
            return true;
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



