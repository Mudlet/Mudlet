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

#undef QT_NO_DEBUG_OUTPUT

#ifndef MUDLET_H
#define MUDLET_H

#include <QMainWindow>
#include <QCloseEvent>
#include "ui_console.h"
#include "ui_main_window.h"
#include "TConsole.h"
#include "mudlet.h"
#include "ctelnet.h"
#include "HostManager.h"
#include <map>
#include <QMap>
#include "Host.h"

class QAction;
class QMenu;
class QTextEdit;



class mudlet : public QMainWindow, private Ui::MainWindow
{
Q_OBJECT

public:
        
                                 mudlet();
                                ~mudlet();
   static                        mudlet * self();
   void                          addSubWindow(TConsole* p); 
   void                          printMessageOnDisplay( Host * pH, QString s ); 
   void                          printOnDisplay( Host * pH, QString s );
   void                          addConsoleForNewHost( Host * pH );
   Host *                        getActiveHost();
   void                          registerTimer( TTimer *, QTimer * ); 
   void                          unregisterTimer( TTimer*, QTimer * ); 
   void                          openUserWindow( Host *, QString & );
   void                          echoUserWindow( Host *, QString &, QString & ); 
   void                          clearUserWindow( Host *, QString & ); 
   void                          pasteWindow( Host * pHost, QString name ); 
   void                          userWindowLineWrap( Host * pHost, QString & name, bool on );
   QString                       readProfileData( QString profile, QString item ); 
    
    
    
   static QPlainTextEdit *       mpDebugConsole; 
   static QMdiSubWindow *        mpDebugArea; 
   static bool                   debugMode; 
   QMap<Host *, TConsole *>      mConsoleMap; 

public slots:      
   void                          slot_show_about_dialog();
   void                          slot_userToolBar_triggered(QAction*);   
   void                          slot_userToolBar_hovered( QAction* pA );
   void                          slot_connection_dlg_finnished( QString profile, int historyVersion );
   void                          slot_timer_fires();   
   void                          slot_send_login();
   void                          slot_send_pass();
   void                          startAutoLogin();
    
protected:
    
   void                          closeEvent(QCloseEvent *event);

private slots:
   
   void                          show_help_dialog(); 
   void                          connectToServer();
   void                          show_trigger_dialog();
   void                          show_alias_dialog();
   void                          show_script_dialog();
   void                          show_timer_dialog();
   void                          show_action_dialog();
   void                          show_key_dialog(); 
   void                          show_options_dialog();
    
    
private:
    
    
    
    /*void                          createActions();
   void                          createMenus();
   void                          createToolBars();
   void                          createStatusBar();*/
   void                          readSettings();
   void                          writeSettings();
    
    
    QQueue<QString> tempLoginQueue;
    QQueue<QString> tempPassQueue;
    QQueue<Host *>  tempHostQueue;
    
    
   static                        mudlet * _self;
  
   QMap<QString, QDockWidget *>  dockWindowMap;
   QMap<QString, TConsole *>     dockWindowConsoleMap;
   QMap<Host *, QToolBar *>      mUserToolbarMap; 
   QMap<QTimer *, TTimer *>      mTimerMap;
   QToolBar *                    mpUserToolBar;
   
    
    /*QString                       curFile;

   QMenu *                       fileMenu;
   QMenu *                       editMenu;
   QMenu *                       helpMenu;
   QToolBar *                    fileToolBar;
   
   QToolBar *                    editToolBar;
   QAction *                     newAct;
   QAction *                     openAct;
   QAction *                     saveAct;
   QAction *                     saveAsAct;
   QAction *                     exitAct;
   QAction *                     cutAct;
   QAction *                     copyAct;
   QAction *                     pasteAct;
   QAction *                     aboutAct;
   QAction *                     aboutQtAct;*/
};

#endif

