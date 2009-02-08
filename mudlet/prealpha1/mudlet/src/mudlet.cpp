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


#include <QtGui>
#include "mudlet.h"
#include "TConsole.h"

#include <QTextEdit>
#include <QTextStream>
#include <QCloseEvent>
#include <QFileDialog>
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "dlgTriggerEditor.h"
#include "dlgAboutDialog.h"
#include "TCommandLine.h"
#include "EAction.h"
#include "dlgHelpDialog.h"
#include "dlgProfilePreferences.h"
#include "TDebug.h"
#include "XMLimport.h"
#include "EAction.h"


TConsole *  mudlet::mpDebugConsole = 0;
QMainWindow * mudlet::mpDebugArea = 0;
bool mudlet::debugMode = false;

mudlet * mudlet::_self = 0;

mudlet * mudlet::self() 
{
    if( ! _self )
    {
        _self = new mudlet;
    }
    return _self;
}

mudlet::mudlet() 
: QMainWindow() //Ui::MainWindow()
{
    //setupUi(this);
    mudlet::debugMode = false;
    QVBoxLayout * layout = new QVBoxLayout( this );
    layout->setContentsMargins(0,0,0,0);
    QSizePolicy sizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    
    mpMainToolBar = new QToolBar( this );
    mpMainToolBar->setIconSize(QSize(32,32));
    mpMainToolBar->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );
    mpUserToolBar = new QToolBar( this );
    
    addToolBar( mpMainToolBar );
    mpMainToolBar->setMovable( false );
    addToolBarBreak();
    
    addToolBar( mpUserToolBar );  
    
    mdiArea = new QMdiArea( this );
    mdiArea->setViewMode( QMdiArea::TabbedView );
    mdiArea->setSizePolicy( sizePolicy );
    layout->addWidget(mdiArea);
    setCentralWidget( mdiArea );
    
    QAction * actionConnect = new QAction(QIcon(":/icons/preferences-web-browser-cache.png"), tr("Connect"), this);
    actionConnect->setStatusTip(tr("Connect To Server"));
    mpMainToolBar->addAction( actionConnect );
    
    QAction * actionTriggers = new QAction(QIcon(":/icons/tools-wizard.png"), tr("Triggers"), this);
    actionTriggers->setStatusTip(tr("Show Triggers"));
    mpMainToolBar->addAction( actionTriggers );
    
    QAction * actionAlias = new QAction(QIcon(":/icons/system-users.png"), tr("Aliases"), this);
    actionAlias->setStatusTip(tr("Show Aliases"));
    actionAlias->setEnabled( true );
    mpMainToolBar->addAction( actionAlias );    
    
    QAction * actionTimers = new QAction(QIcon(":/icons/chronometer.png"), tr("Timers"), this);
    actionTimers->setStatusTip(tr("Show Timers"));
    mpMainToolBar->addAction( actionTimers );    
    
    QAction * actionButtons = new QAction(QIcon(":/icons/bookmarks.png"), tr("Actions"), this);
    actionButtons->setStatusTip(tr("Show Actions"));
    mpMainToolBar->addAction( actionButtons );    
    
    QAction * actionScripts = new QAction(QIcon(":/icons/document-properties.png"), tr("Scripts"), this);
    actionScripts->setEnabled( true );
    actionScripts->setStatusTip(tr("Show Scripts"));
    mpMainToolBar->addAction( actionScripts );
    
    
    
    QAction * actionKeys = new QAction(QIcon(":/icons/preferences-desktop-keyboard.png"), tr("Keys"), this);
    actionKeys->setStatusTip(tr("Options"));
    actionKeys->setEnabled( true );
    mpMainToolBar->addAction( actionKeys );
    
    QAction * actionHelp = new QAction(QIcon(":/icons/help-hint.png"), tr("Manual"), this);
    actionHelp->setStatusTip(tr("Browse Reference Material and Documentation"));
    mpMainToolBar->addAction( actionHelp );
    
    QAction * actionOptions = new QAction(QIcon(":/icons/configure.png"), tr("Settings"), this);
    actionOptions->setStatusTip(tr("Settings, Options and Preferences"));
    mpMainToolBar->addAction( actionOptions );
    
    QAction * actionNotes = new QAction(QIcon(":/icons/view-pim-notes.png"), tr("Notepad"), this);
    actionNotes->setStatusTip(tr("take notes"));
    mpMainToolBar->addAction( actionNotes );
    
        
    QAction * actionMultiView = new QAction(QIcon(":/icons/view-split-left-right.png"), tr("MultiView"), this);
    actionMultiView->setStatusTip(tr("MultiView"));
    mpMainToolBar->addAction( actionMultiView );
    
    QAction * actionStopAllTriggers = new QAction(QIcon(":/icons/edit-bomb.png"), tr("Stop All Triggers"), this);
    actionStopAllTriggers->setStatusTip(tr("stop all triggers, alias, actions, timers and scripts"));
    mpMainToolBar->addAction( actionStopAllTriggers );
    
    
    /* QAction * actionProfileBackup = new QAction(QIcon(":/icons/utilities-file-archiver.png"), tr("Backup Profile"), this);
    actionProfileBackup->setStatusTip(tr("Backup Profile"));
    mpMainToolBar->addAction( actionProfileBackup );*/
    
    
    QAction * actionAbout = new QAction(QIcon(":/icons/mudlet_main_32px.png"), tr("About"), this);
    actionAbout->setStatusTip(tr("About"));
    mpMainToolBar->addAction( actionAbout );
    
    mpDebugArea = new QMainWindow(0);
    HostManager::self()->addHost("default_host", "", "","" );  
    mpDefaultHost = HostManager::self()->getHost(QString("default_host"));
    mpDebugConsole = new TConsole( mpDefaultHost, true );
    
    mpDebugConsole->setSizePolicy( sizePolicy );
    mpDebugArea->setCentralWidget( mpDebugConsole );
    mpDebugArea->resize(750,550);
    mpDebugArea->hide();
    QFont font("Monospace", 10, QFont::Courier);
    mdiArea->show();//NOTE: this is important for Apple OSX otherwise the console isnt displayed
    
    
    
    connect(actionConnect, SIGNAL(triggered()), this, SLOT(connectToServer()));
    connect(actionHelp, SIGNAL(triggered()), this, SLOT(show_help_dialog()));
    connect(actionTriggers, SIGNAL(triggered()), this, SLOT(show_trigger_dialog()));
    connect(actionTimers, SIGNAL(triggered()), this, SLOT(show_timer_dialog()));
    connect(actionAlias, SIGNAL(triggered()), this, SLOT(show_alias_dialog()));
    connect(actionScripts, SIGNAL(triggered()), this, SLOT(show_script_dialog()));
    connect(actionKeys, SIGNAL(triggered()),this,SLOT(show_key_dialog()));
    connect(actionButtons, SIGNAL(triggered()), this, SLOT(show_action_dialog()));
    connect(actionOptions, SIGNAL(triggered()), this, SLOT(show_options_dialog()));
    connect(actionAbout, SIGNAL(triggered()), this, SLOT(slot_show_about_dialog()));
    connect(actionMultiView, SIGNAL(triggered()), this, SLOT(slot_multi_view()));
    connect(actionStopAllTriggers, SIGNAL(triggered()), this, SLOT(slot_stopAllTriggers()));
    
    readSettings();
    
    QTimer * timerAutologin = new QTimer( this );
    timerAutologin->setSingleShot( true );
    connect(timerAutologin, SIGNAL(timeout()), this, SLOT(startAutoLogin()));
    timerAutologin->start( 1000 );
    
   
    
    connect(mpUserToolBar,SIGNAL(actionTriggered( QAction * ) ), this, SLOT(slot_userToolBar_triggered(QAction*)));    
    
    
}

void mudlet::addConsoleForNewHost( Host * pH )
{
    if( mConsoleMap.contains( pH ) ) return;
    TConsole * pConsole = new TConsole( pH, false );
    pH->mpConsole = pConsole;
    
    pConsole->setWindowTitle( pH->getName() );
    mConsoleMap[pH] = pConsole;

    mdiArea->addSubWindow( pConsole );
    
    pConsole->show();
    mdiArea->show();
    
    dlgTriggerEditor * pEditor = new dlgTriggerEditor( pH );
    pH->mpEditorDialog = pEditor;
    pEditor->fillout_form();
    
    pH->getActionUnit()->constructToolbar( this, mpUserToolBar );
    
    //mdiArea->tileSubWindows();
}

/*void mudlet::connectActionMenu( QAction * pA )
{
    connect(pToolbar,SIGNAL(hovered()), this, SLOT(slot_userToolBar_hovered(QAction*)));    
} */

void mudlet::slot_timer_fires()
{
    QTimer * pQT = (QTimer*)sender();
    if( mTimerMap.contains( pQT ) )
    {
        TTimer * pTT = mTimerMap[pQT];
        pTT->execute();
    }
    else
    {
        qDebug()<<"MUDLET CRITICAL ERROR: Timer not registered!";
    }
}

void mudlet::unregisterTimer( TTimer * pTT, QTimer * pQT )
{
    if( mTimerMap.contains( pQT ) )
    {
        mTimerMap.remove( pQT );
    }
    else
    {
        qDebug()<<"MUDLET CRITICAL ERROR: trying to unregister Timer but it is not registered!";
    }    
}

void mudlet::registerTimer( TTimer * pTT, QTimer * pQT )
{
    if( ! mTimerMap.contains( pQT ) )
    {
        mTimerMap[pQT] = pTT;
    }
}

void mudlet::openUserWindow( Host * pHost, QString & name )
{
    if( ! dockWindowMap.contains( name ) )
    {
        QDockWidget * pD = new QDockWidget;
        pD->setFeatures( QDockWidget::AllDockWidgetFeatures );
        pD->setWindowTitle( name );
        dockWindowMap[name] = pD;
        TConsole * pC = new TConsole( pHost, false );
        pD->setWidget( pC );
        pC->show();
        pC->mpCommandLine->hide();
        pC->setUserWindow();
        dockWindowConsoleMap[name] = pC;
        addDockWidget(Qt::RightDockWidgetArea, pD);
    }
}

void mudlet::clearUserWindow( Host * pHost, QString & name )
{
    if( dockWindowMap.contains( name ) )
    { 
        //FIXME
        //dockWindowConsoleMap[name]->textEdit->clear();    
    }
    else qDebug()<<"ERROR: window doesnt exit";
}

void mudlet::userWindowLineWrap( Host * pHost, QString & name, bool on )
{
    //FIXME
    /* if( dockWindowMap.contains( name ) )
    { 
        if( ! on ) dockWindowConsoleMap[name]->textEdit->setLineWrapMode( QPlainTextEdit::NoWrap );    
        else dockWindowConsoleMap[name]->textEdit->setLineWrapMode( QPlainTextEdit::WidgetWidth );    
    }
    else qDebug()<<"ERROR: window doesnt exit";*/
}


void mudlet::echoUserWindow( Host * pHost, QString & name, QString & text )
{
    //FIXME
    /*
    QString wn = name;
    QString t=text;
    if( dockWindowMap.contains( name ) )
    { 
        dockWindowConsoleMap[wn]->echoUserWindow( t );    
    }
    else qDebug()<<"ERROR: window doesnt exit";*/
}

void mudlet::pasteWindow( Host * pHost, QString name )
{
    //FIXME
    /*
    if( dockWindowMap.contains( name ) )
    { 
        dockWindowConsoleMap[name]->pasteWindow( mConsoleMap[pHost]->getCurrentFragment() );    
    }
    else qDebug()<<"ERROR: window doesnt exit";*/
}

void mudlet::slot_userToolBar_hovered( QAction* pA )
{
    QStringList sL;
    if( pA->menu() )
    {
        pA->menu()->exec();
    }
    
    
}

void mudlet::slot_userToolBar_triggered( QAction* pA )
{
    if( pA->isChecked() )
    {
        ((EAction*)pA)->mpHost->getActionUnit()->getAction(((EAction*)pA)->mID )->mButtonState = 2;
    }
    else
    {
        ((EAction*)pA)->mpHost->getActionUnit()->getAction(((EAction*)pA)->mID )->mButtonState = 1;    
    }
    QStringList sL;
    ((EAction*)pA)->mpHost->getActionUnit()->getAction(((EAction*)pA)->mID )->execute(sL);
}

Host * mudlet::getActiveHost()
{
    if( mConsoleMap.size() < 1 ) return 0;
    if( mdiArea->activeSubWindow()->widget() == mpDebugConsole )
    {
        return 0;
    }
    else
    {
        TConsole * pConsole = (TConsole *)mdiArea->activeSubWindow()->widget();
        Host * pH = pConsole->getHost();
        return pH;
    }
}



void mudlet::addSubWindow( TConsole* pConsole ) 
{  
    mdiArea->addSubWindow( pConsole );
    pConsole->show();//NOTE: this is important for Apple OSX otherwise the console isnt displayed
}

EAction * mudlet::generateAction( QString name, QString icon, QToolBar * pT )
{
    /* action->setIcon( ic );
    pT->addAction( action );
    return new EAction( ic, name, this );*/
}

void mudlet::closeEvent(QCloseEvent *event)
{
    
    mpDebugConsole->close();
    
    foreach( TConsole * pC, mConsoleMap )
    {
        qDebug()<<"[SAVING] host="<< pC->mpHost->getName();
        if( pC->mpHost->getName() != "Default Host" )
        {
            if( QMessageBox::question( this, "Question", "Do you want to save the profile "+pC->mpHost->getName(), QMessageBox::Yes|QMessageBox::No ) == QMessageBox::Yes )
            {
                qDebug()<<"The user wants to save the profile.";
                pC->mpHost->mSaveProfileOnExit = true;
            }
            else 
            {
                qDebug()<<"User doesn't like to save the profile.";
                pC->mpHost->mSaveProfileOnExit = false;
            }
            pC->mpHost->mpEditorDialog->setAttribute( Qt::WA_DeleteOnClose );
            pC->mpHost->mpEditorDialog->close();    
        }
    }
    
    writeSettings();
    event->accept();
}

void mudlet::readSettings()
{
    QSettings settings("Mudlet", "Mudlet 1.0");
    QPoint pos = settings.value("pos", QPoint(0, 0)).toPoint();
    QSize size = settings.value("size", QSize(750, 550)).toSize();
    resize( size );
    move( pos );
}

void mudlet::writeSettings()
{
    QSettings settings("Mudlet", "Mudlet 1.0");
    settings.setValue("pos", pos());
    settings.setValue("size", size());
}

void mudlet::connectToServer()
{
    dlgConnectionProfiles * pDlg = new dlgConnectionProfiles(this);
    connect (pDlg, SIGNAL (signal_establish_connection( QString, int )), this, SLOT (slot_connection_dlg_finnished(QString, int)));
    pDlg->fillout_form();
    pDlg->exec();
}

void mudlet::show_trigger_dialog()
{
    Host * pHost = getActiveHost();
    if( ! pHost ) return;
    dlgTriggerEditor * pEditor = pHost->mpEditorDialog;
    if( ! pEditor ) return;
    pEditor->slot_show_triggers();
    pEditor->show();
}

void mudlet::show_alias_dialog()
{
    Host * pHost = getActiveHost();
    if( ! pHost ) return;
    dlgTriggerEditor * pEditor = pHost->mpEditorDialog;
    if( ! pEditor ) return;
    pEditor->slot_show_aliases();
    pEditor->show();
}

void mudlet::show_timer_dialog()
{
    Host * pHost = getActiveHost();
    if( ! pHost ) return;
    dlgTriggerEditor * pEditor = pHost->mpEditorDialog;
    if( ! pEditor ) return;
    pEditor->slot_show_timers();
    pEditor->show();
}

void mudlet::show_script_dialog()
{
    Host * pHost = getActiveHost();
    if( ! pHost ) return;
    dlgTriggerEditor * pEditor = pHost->mpEditorDialog;  
    if( ! pEditor ) return;
    pEditor->slot_show_scripts();
    pEditor->show();
}

void mudlet::show_key_dialog()
{
    Host * pHost = getActiveHost();
    if( ! pHost ) return;
    dlgTriggerEditor * pEditor = pHost->mpEditorDialog;
    if( ! pEditor ) return;
    pEditor->slot_show_keys();
    pEditor->show();
}

void mudlet::show_action_dialog()
{
    Host * pHost = getActiveHost();
    if( ! pHost ) return;
    dlgTriggerEditor * pEditor = pHost->mpEditorDialog;
    if( ! pEditor ) return;
    pEditor->slot_show_actions();
    pEditor->show();
}

void mudlet::show_options_dialog()
{
    Host * pHost = getActiveHost();
    if( ! pHost ) return;
    dlgProfilePreferences * pDlg = new dlgProfilePreferences( this );
    if( ! pDlg ) return;
    pDlg->show();
}


void mudlet::show_help_dialog()
{
    dlgHelpDialog * pDlg = new dlgHelpDialog(this);
    pDlg->show();
}

void mudlet::slot_show_about_dialog()
{
    dlgAboutDialog * pDlg = new dlgAboutDialog(this);
    pDlg->show();
}

void mudlet::printOnDisplay( Host * pH, QString & s )
{
    mConsoleMap[pH]->printOnDisplay(s);
}

void mudlet::printSystemMessage( Host * pH, QString & s )
{
    mConsoleMap[pH]->printSystemMessage( s );
}

void mudlet::print( Host * pH, QString & s )
{
    mConsoleMap[pH]->print( s );
}


QString mudlet::readProfileData( QString profile, QString item )
{
    QFile file( QDir::homePath()+"/.config/mudlet/profiles/"+profile+"/"+item );
    file.open( QIODevice::ReadOnly );
    if( ! file.exists() )
        return "";
    
    QDataStream ifs( & file ); 
    QString ret;
    
    ifs >> ret;
    file.close();
    return ret;
}

// this slot is called via a timer in the constructor of mudlet::mudlet()
void mudlet::startAutoLogin()
{
    TDebug() << "[ AUTOLOGIN BEGIN ] checking if there are any autologin profiles";
    QList<QString> hostList = HostManager::self()->getHostList();
    for( int i=0; i<hostList.size(); i++ )
    {
        Host * pH = HostManager::self()->getHost( hostList[i] );
        QString profile = pH->getName();
        if( profile.size() < 1 )
        {
            HostManager::self()->deleteHost( profile ); 
            continue;
        }
            
        TDebug()<<"----> verifying autoloader status: "<<profile;
        QString item = "autologin";
        QString val = readProfileData( profile, item );
        if( val.toInt() == Qt::Checked )
        {
            qDebug()<<"----> Host:"<<pH->getName()<<" URL:"<<pH->getUrl()<<"Login:"<<pH->getLogin();
            addConsoleForNewHost( pH );
            pH->connectToServer();
        }
        else
        {
            // remove Hosts that don't have autologin defined from HostPool
            qDebug() << "----> [ EXPIRED ] " << profile.toLatin1().data() << " Host ist no longer an autoloader. Due to user decision.";
            HostManager::self()->deleteHost( profile );    
        }
    }
    if( hostList.size() < 1 )
        qDebug() << "----> [ OK ] nothing to be done (no autologin profiles defined)";
    else
        qDebug() << "----> [ OK ] autologin finished";
    qDebug()<<"[ AUTOLOGIN END ] currently loaded hosts after removal of non-autoloaders:"<<HostManager::self()->getHostList();
}

///////////////////////////////////////////////////////////////////////////////
// these two callbacks are called from cTelnet::handleConnectedToServer()
void mudlet::slot_send_login()
{
    if( tempHostQueue.isEmpty() )
        return;
    Host * pHost = tempHostQueue.dequeue();
    QString login = pHost->getLogin();
    pHost->sendRaw( login );
}

void mudlet::slot_send_pass()
{
    if( tempHostQueue.isEmpty() )
        return;
    Host * pHost = tempHostQueue.dequeue();
    QString pass = pHost->getPass();
    pHost->sendRaw( pass );
}
//////////////////////////////////////////////////////////////////////////////


void mudlet::slot_connection_dlg_finnished( QString profile, int historyVersion )
{
    Host * pHost = HostManager::self()->getHost( profile );
    if( ! pHost ) 
        return;
    
    addConsoleForNewHost( pHost );
    
    //NOTE: this is a potential problem if users connect by hand quickly 
    //      and one host has a slower response time as the other one, but
    //      the worst that can happen is that they have to login manually.

    tempHostQueue.enqueue( pHost );
    tempHostQueue.enqueue( pHost );
    pHost->connectToServer();     
}

void mudlet::slot_multi_view()
{
    mdiArea->tileSubWindows();    
}

void mudlet::slot_stopAllTriggers()
{
    foreach( TConsole * pC, mConsoleMap )
    {
        pC->mpHost->stopAllTriggers();
    }
}   

mudlet::~mudlet()
{
}



