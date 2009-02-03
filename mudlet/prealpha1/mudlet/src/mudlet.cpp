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

mudlet::mudlet() : Ui::MainWindow()
{
    setupUi(this);
    mudlet::debugMode = false;
    
    //mdiArea->tileSubWindows();
    mdiArea->setViewMode( QMdiArea::TabbedView );
   
    mpDebugArea = new QMainWindow(0);
    HostManager::self()->addHost("default_host", "", "","" );  
    mpDefaultHost = HostManager::self()->getHost(QString("default_host"));
    mpDebugConsole = new TConsole( mpDefaultHost, true );
    QSizePolicy sizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding);
    mpDebugConsole->setSizePolicy( sizePolicy );
    mpDebugArea->setCentralWidget( mpDebugConsole );
    mpDebugArea->resize(800,600);
    mpDebugArea->hide();
    QFont font("Monospace", 10, QFont::Courier);
    mdiArea->show();//NOTE: this is important for Apple OSX otherwise the console isnt displayed
    
    toolBar->addAction( actionConnect );
    toolBar->addAction( actionTriggers );
    toolBar->addAction( actionTimers );
    toolBar->addAction( actionAliases );
    toolBar->addAction( actionScripts );
    toolBar->addAction( actionButtons );
    toolBar->addAction( actionKeys );
    toolBar->addAction( actionHelp );
    toolBar->addAction( actionNotes );
    toolBar->addAction( actionOptions );
    toolBar->addAction( actionAbout );
    
    addToolBarBreak();
    
    connect(actionConnect, SIGNAL(triggered()), this, SLOT(connectToServer()));
    connect(actionHelp, SIGNAL(triggered()), this, SLOT(show_help_dialog()));
    connect(actionTriggers, SIGNAL(triggered()), this, SLOT(show_trigger_dialog()));
    connect(actionTimers, SIGNAL(triggered()), this, SLOT(show_timer_dialog()));
    connect(actionAliases, SIGNAL(triggered()), this, SLOT(show_alias_dialog()));
    connect(actionScripts, SIGNAL(triggered()), this, SLOT(show_script_dialog()));
    connect(actionKeys, SIGNAL(triggered()),this,SLOT(show_key_dialog()));
    connect(actionButtons, SIGNAL(triggered()), this, SLOT(show_action_dialog()));
    connect(actionOptions, SIGNAL(triggered()), this, SLOT(show_options_dialog()));
    connect(actionAbout, SIGNAL(triggered()), this, SLOT(slot_show_about_dialog()));
    
    readSettings();
    
    QTimer * timerAutologin = new QTimer( this );
    timerAutologin->setSingleShot( true );
    connect(timerAutologin, SIGNAL(timeout()), this, SLOT(startAutoLogin()));
    timerAutologin->start( 1000 );
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
    
    QToolBar * pToolbar = new QToolBar( this );
    addToolBar( pToolbar );
    pH->getActionUnit()->constructToolbar( this, pToolbar );
    
    pToolbar->show();
    
    connect(pToolbar,SIGNAL(actionTriggered( QAction * ) ), this, SLOT(slot_userToolBar_triggered(QAction*)));    
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

void mudlet::closeEvent(QCloseEvent *event)
{
    mpDebugConsole->close();
    
    foreach( TConsole * pC, mConsoleMap )
    {
        pC->mpHost->mpEditorDialog->setAttribute( Qt::WA_DeleteOnClose );
        pC->mpHost->mpEditorDialog->close();    
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
            TDebug()<<"----> Host:"<<pH->getName()<<" URL:"<<pH->getUrl()<<"Login:"<<pH->getLogin();
            addConsoleForNewHost( pH );
            pH->connectToServer();
        }
        else
        {
            // remove Hosts that don't have autologin defined from HostPool
            TDebug() << "----> [ EXPIRED ] " << profile.toLatin1().data() << " Host ist no longer an autoloader. Due to user decision.";
            HostManager::self()->deleteHost( profile );    
        }
    }
    if( hostList.size() < 1 )
        TDebug() << "----> [ OK ] nothing to be done (no autologin profiles defined)";
    else
        TDebug() << "----> [ OK ] autologin finished";
    TDebug()<<"[ AUTOLOGIN END ] currently loaded hosts after removal of non-autoloaders:"<<HostManager::self()->getHostList();
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

mudlet::~mudlet()
{
}



