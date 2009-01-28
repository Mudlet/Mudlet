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
    createActions();
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
    //connect(toolButton_filters, SIGNAL(pressed()), this, SLOT(show_trigger_dialog()));
    connect(actionHelp, SIGNAL(triggered()), this, SLOT(show_help_dialog()));
    //connect(toolButton_options, SIGNAL(pressed()), this, SLOT(show_options_dialog()));
    //connect(toolButton_about, SIGNAL(pressed()), this, SLOT(show_about_dialog()));
    
    
    connect(actionTriggers, SIGNAL(triggered()), this, SLOT(show_trigger_dialog()));
    connect(actionTimers, SIGNAL(triggered()), this, SLOT(show_timer_dialog()));
    connect(actionAliases, SIGNAL(triggered()), this, SLOT(show_alias_dialog()));
    connect(actionScripts, SIGNAL(triggered()), this, SLOT(show_script_dialog()));
    connect(actionKeys, SIGNAL(triggered()),this,SLOT(show_key_dialog()));
    connect(actionButtons, SIGNAL(triggered()), this, SLOT(show_action_dialog()));
    connect(actionOptions, SIGNAL(triggered()), this, SLOT(show_options_dialog()));
    connect(actionAbout, SIGNAL(triggered()), this, SLOT(slot_show_about_dialog()));
  
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


void mudlet::createActions()
{
      //newAct = new QAction(QIcon(":/filenew.xpm"), tr("connect"), this);
      //newAct->setShortcut(tr("Ctrl+N"));
      //newAct->setStatusTip(tr("connect to server"));
      
       
/*  
      openAct = new QAction(QIcon(":/fileopen.xpm"), tr("&Open..."), this);
      openAct->setShortcut(tr("Ctrl+O"));
      openAct->setStatusTip(tr("Open an existing file"));
      connect(openAct, SIGNAL(triggered()), this, SLOT(open()));

      saveAct = new QAction(QIcon(":/filesave.xpm"), tr("&Save"), this);
      saveAct->setShortcut(tr("Ctrl+S"));
      saveAct->setStatusTip(tr("Save the document to disk"));
      connect(saveAct, SIGNAL(triggered()), this, SLOT(save()));

      saveAsAct = new QAction(tr("Save &As..."), this);
      saveAsAct->setStatusTip(tr("Save the document under a new name"));
      connect(saveAsAct, SIGNAL(triggered()), this, SLOT(saveAs()));

      exitAct = new QAction(tr("E&xit"), this);
      exitAct->setShortcut(tr("Ctrl+Q"));
      exitAct->setStatusTip(tr("Exit the application"));
      connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));

      cutAct = new QAction(QIcon(":/editcut.xpm"), tr("Cu&t"), this);
      cutAct->setShortcut(tr("Ctrl+X"));
      cutAct->setStatusTip(tr("Cut the current selection's contents to the "
                              "clipboard"));
      connect(cutAct, SIGNAL(triggered()), textEdit, SLOT(cut()));

      copyAct = new QAction(QIcon(":/editcopy.xpm"), tr("&Copy"), this);
      copyAct->setShortcut(tr("Ctrl+C"));
      copyAct->setStatusTip(tr("Copy the current selection's contents to the "
                              "clipboard"));
      connect(copyAct, SIGNAL(triggered()), textEdit, SLOT(copy()));

      pasteAct = new QAction(QIcon(":/editpaste.xpm"), tr("&Paste"), this);
      pasteAct->setShortcut(tr("Ctrl+V"));
      pasteAct->setStatusTip(tr("Paste the clipboard's contents into the current "
                              "selection"));
      connect(pasteAct, SIGNAL(triggered()), textEdit, SLOT(paste()));

      aboutAct = new QAction(tr("&About"), this);
      aboutAct->setStatusTip(tr("Show the application's About box"));
      connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));

      aboutQtAct = new QAction(tr("About &Qt"), this);
      aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
      connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

      cutAct->setEnabled(false);
      copyAct->setEnabled(false);
      connect(textEdit, SIGNAL(copyAvailable(bool)),
            cutAct, SLOT(setEnabled(bool)));
      connect(textEdit, SIGNAL(copyAvailable(bool)),
            copyAct, SLOT(setEnabled(bool)));*/
}

void mudlet::createMenus()
{
      //fileMenu = menuBar()->addMenu(tr("connect"));
      //fileMenu->addAction(newAct);
      /*
      fileMenu->addAction(openAct);
      fileMenu->addAction(saveAct);
      fileMenu->addAction(saveAsAct);
      fileMenu->addSeparator();
      fileMenu->addAction(exitAct);

      editMenu = menuBar()->addMenu(tr("&Edit"));
      editMenu->addAction(cutAct);
      editMenu->addAction(copyAct);
      editMenu->addAction(pasteAct);

      menuBar()->addSeparator();

      helpMenu = menuBar()->addMenu(tr("&Help"));
      helpMenu->addAction(aboutAct);
      helpMenu->addAction(aboutQtAct);*/
}

void mudlet::createToolBars()
{
  /*    fileToolBar = addToolBar(tr("File"));
      fileToolBar->addAction(newAct);
      fileToolBar->addAction(openAct);
      fileToolBar->addAction(saveAct);

      editToolBar = addToolBar(tr("Edit"));
      editToolBar->addAction(cutAct);
      editToolBar->addAction(copyAct);
      editToolBar->addAction(pasteAct);*/
}

void mudlet::createStatusBar()
{
      //statusBar()->showMessage(tr("Ready"));
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
    connect (pDlg, SIGNAL (signal_establish_connection( QString )), this, SLOT (slot_connection_dlg_finnished(QString)));
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


void mudlet::slot_send_login()
{
    qDebug()<<"send login";
    Host * pHost = tempHostQueue.dequeue();
    QString login = pHost->getLogin();
    pHost->sendRaw( login );
    qDebug()<<"end send login";
}

void mudlet::slot_send_pass()
{
    qDebug()<<"send pass";
    Host * pHost = tempHostQueue.dequeue();
    QString pass = pHost->getPass();
    pHost->sendRaw( pass );
    qDebug()<<"pass ende";
}

void mudlet::startAutoLogin( Host * pHost )
{
}

void mudlet::slot_connection_dlg_finnished( QString profile )
{
    qDebug()<<"ready"      ;
    Host * pHost = HostManager::self()->getHost( profile );
    if( ! pHost ) 
        return;
    addConsoleForNewHost( pHost );
    
    pHost->connectToServer();     
}

mudlet::~mudlet()
{

}



