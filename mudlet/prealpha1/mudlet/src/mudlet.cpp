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

QPlainTextEdit *  mudlet::mpDebugConsole = 0;
QMdiSubWindow * mudlet::mpDebugArea = 0;
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
    /*    
    QStringList hostList = HostManager::self()->getHostList();
    for( int i=0; i<hostList.size(); i++ )
    {
        Host * pH = HostManager::self()->getHost(hostList[i]);
        TConsole * pConsole = new TConsole( pH, this );
        pH->mpConsole = pConsole;
        mConsoleMap[pH] = pConsole;
        mdiArea->addSubWindow(pConsole);
    } */
    
    //mdiArea->tileSubWindows();
    mdiArea->setViewMode( QMdiArea::TabbedView );
    mpDebugConsole = new QPlainTextEdit( this );
    mpDebugConsole->setEnabled(true);
    mpDebugConsole->setReadOnly(true);
    mpDebugConsole->setWindowTitle( "Debug Messages" );
    mpDebugArea = mdiArea->addSubWindow( mpDebugConsole );
    mpDebugArea->hide();
    QFont font("Courier New", 10, QFont::Courier);
    mpDebugArea->setFont( font );
    //mdiArea->setTabShape(QTabWidget::Triangular);
    //mdiArea->tileSubWindows();
    mdiArea->show();//NOTE: this is important for Apple OSX otherwise the console isnt displayed
    //    createActions();
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
    
    QTimer * timerAutologin = new QTimer( this );
    timerAutologin->setSingleShot( true );
    connect(timerAutologin, SIGNAL(timeout()), this, SLOT(startAutoLogin()));
    timerAutologin->start( 1000 );
}

void mudlet::addConsoleForNewHost( Host * pH )
{
    if( mConsoleMap.contains( pH ) ) return;
    TConsole * pConsole = new TConsole( pH, this );
    pH->mpConsole = pConsole;
    
    pConsole->setWindowTitle( pH->getName() );
    mConsoleMap[pH] = pConsole;

    mdiArea->addSubWindow( pConsole );
    
    pConsole->show();
    mdiArea->show();
    
    dlgTriggerEditor * pEditor = new dlgTriggerEditor( pH, this );
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
    //QMap<QString, QDockWidget *>  dockWindowMap;
    //QMap<QString, TConsole *>     dockWindowConsoleMap;    
    if( ! dockWindowMap.contains( name ) )
    {
        QDockWidget * pD = new QDockWidget;
        pD->setFeatures( QDockWidget::AllDockWidgetFeatures );
        pD->setWindowTitle( name );
        dockWindowMap[name] = pD;
        TConsole * pC = new TConsole( pHost, this );
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
    //mdiArea->tileSubWindows();
    
    /*    toolButton_connect->setEnabled( true );
    toolButton_filters->setEnabled( false );
    toolButton_options->setEnabled( false );
    toolButton_triggers->setEnabled( true );
    toolButton_timers->setEnabled( true );
    toolButton_aliases->setEnabled( true );
    toolButton_scripts->setEnabled( true );
    toolButton_action_buttons->setEnabled( true );*/
}

void mudlet::closeEvent(QCloseEvent *event)
{
    
    /*  if (maybeSave()) {
            writeSettings();
            event->accept();
      } else {
            event->ignore();
      }*/
}

void mudlet::readSettings()
{
 //     QSettings settings("Trolltech", "Application Example");
 //     QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
 //     QSize size = settings.value("size", QSize(400, 400)).toSize();
//      resize(size);
  //    move(pos);
}

void mudlet::writeSettings()
{
 //     QSettings settings("Trolltech", "Application Example");
//      settings.setValue("pos", pos());
 //     settings.setValue("size", size());
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

void mudlet::printOnDisplay( Host * pH, QString s )
{
    mConsoleMap[pH]->printOnDisplay(s);
}

void mudlet::printMessageOnDisplay( Host * pH, QString s )
{
    mConsoleMap[pH]->printMessageOnDisplay(s);
}

///////////////////////////////////////////////////////////////////////////////
// these two callbacks are called from cTelnet::handleConnectedToServer()
void mudlet::slot_send_login()
{
    Host * pHost = tempHostQueue.dequeue();
    QString login = pHost->getLogin();
    pHost->sendRaw( login );
}

void mudlet::slot_send_pass()
{
    Host * pHost = tempHostQueue.dequeue();
    QString pass = pHost->getPass();
    pHost->sendRaw( pass );
}
//////////////////////////////////////////////////////////////////////////////


// this slot is called via a timer in the constructor of mudlet::mudlet()
void mudlet::startAutoLogin()
{
    cout << "[ AUTOLOGIN ] checking if there are any autologin profiles" << endl;
    QList<QString> hostList = HostManager::self()->getHostList();
    for( int i=0; i<hostList.size(); i++ )
    {
        Host * pH = HostManager::self()->getHost( hostList[i] );
        if( (pH->getLogin().size()>0) && (pH->getPass().size()>0) )
        {
            qDebug()<<"----> Host:"<<pH->getName()<<" URL:"<<pH->getUrl()<<"Login:"<<pH->getLogin();
            addConsoleForNewHost( pH );
            pH->connectToServer();
        }
    }
    if( hostList.size() < 1 )
        cout << "----> [ OK ] nothing to be done (no autologin profiles defined)"<<endl;
    else
        cout << "----> [ OK ] autologin finished" << endl;
}

void mudlet::slot_connection_dlg_finnished( QString profile, int historyVersion )
{
    Host * pHost = HostManager::self()->getHost( profile );
    if( ! pHost ) 
        return;
    addConsoleForNewHost( pHost );
    pHost->connectToServer();     
}

mudlet::~mudlet()
{
}



