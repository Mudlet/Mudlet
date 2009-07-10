/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn                                     *
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

//#undef QT_NO_DEBUG_OUTPUT

#ifndef MUDLET_H
#define MUDLET_H

#include <QMainWindow>
#include <QCloseEvent>
#include "ui_console.h"
#include "TConsole.h"
#include "mudlet.h"
#include "ctelnet.h"
#include "HostManager.h"
#include <map>
#include <QMap>
#include "Host.h"
#include <QMdiArea>
#include "TConsole.h"

class QAction;
class QMenu;
class QTextEdit;
class EAction;
class TConsole;
class TLabel;

class mudlet : public QMainWindow
{
Q_OBJECT

public:
        
                                 mudlet();
                                ~mudlet();
   static                        mudlet * self();
   void                          addSubWindow(TConsole* p); 
   void                          printSystemMessage( Host * pH, QString & s ); 
   void                          print( Host *, QString & );
   void                          addConsoleForNewHost( Host * pH );
   void							 disableToolbarButtons();
   void							 enableToolbarButtons();
   Host *                        getActiveHost();
   void                          registerTimer( TTimer *, QTimer * ); 
   void                          unregisterTimer( TTimer*, QTimer * ); 
   bool                          openWindow( Host *, QString & );
   bool                          createMiniConsole( Host *, QString &, int, int, int, int );
   bool                          createLabel( Host *, QString &, int, int, int, int, bool );
   bool                          echoWindow( Host *, QString &, QString & );
   bool                          appendBuffer( Host *, QString & );
   bool                          createBuffer( Host *, QString & );
   bool                          showWindow( Host *, QString & );
   bool                          hideWindow( Host *, QString & );
   bool                          closeWindow( Host *, QString & );
   bool                          resizeWindow( Host *, QString &, int, int );
   bool                          clearWindow( Host *, QString & );
   bool                          pasteWindow( Host * pHost, QString & name );
   bool                          setBackgroundColor( QString & name, int r, int g, int b, int alpha );
   bool                          setBackgroundImage( QString & name, QString & path );
   bool                          setTextFormat( QString & name, int, int, int, int, int, int, bool, bool, bool );
   bool                          setLabelClickCallback( Host *, QString &, QString &, TEvent * );
   bool                          moveWindow( QString & name, int, int );
   void                          deleteLine( QString & name );
   void                          insertText( QString & name, QString );
   void                          replace( QString & name, QString );
   int                           selectString( QString & name, QString what, int );
   int                           selectSection( QString & name, int, int );
   void                          setBold( QString & name, bool );
   void                          setItalics( QString & name, bool );
   void                          setUnderline( QString & name, bool );
   void                          setFgColor( QString & name, int, int, int );
   void                          setBgColor( QString & name, int, int, int );
   bool                          userWindowLineWrap( Host * pHost, QString & name, bool on );
   QString                       readProfileData( QString profile, QString item ); 
   bool                          setWindowWrap( Host * pHost, QString & name, int & wrap );
   bool                          setWindowWrapIndent( Host * pHost, QString & name, int & wrap );
   void                          bindMenu( QMenu *, EAction * ); 
   bool                          moveCursorEnd( QString & );
   bool                          moveCursor( QString &, int, int );
   int                           getLastLineNumber( QString & );
   void                          readSettings();
   void                          writeSettings();
   static TConsole *             mpDebugConsole; 
   static QMainWindow *          mpDebugArea; 
   static bool                   debugMode; 
   QMap<Host *, TConsole *>      mConsoleMap; 
   QMap<Host *, TLabel *>        mHostLabelMap;
   QIcon *                       testicon; 
   bool                          mShowMenuBar;
   bool                          isGoingDown() { return mIsGoingDown; }
   int                           mMainIconSize;
   int                           mTEFolderIconSize;
   void                          setIcoSize( int s );
   void                          replayStart();
   void                          replayOver();

   QTime                         mReplayTime;
   int                           mReplaySpeed;

public slots:      

   void                          slot_replayTimeChanged();
   void                          slot_replaySpeedUp();
   void                          slot_replaySpeedDown();
   void                          toggleFullScreenView(); 
   void                          slot_userToolBar_orientation_changed(Qt::Orientation); 
   void                          slot_show_about_dialog();
   void                          slot_multi_view();
   void                          slot_stopAllTriggers();
   void                          slot_userToolBar_triggered(QAction*);   
   void                          slot_userToolBar_hovered( QAction* pA );
   void                          slot_connection_dlg_finnished( QString profile, int historyVersion );
   void                          slot_timer_fires();   
   void                          slot_send_login();
   void                          slot_send_pass();
   void                          slot_replay();
   void                          slot_notes();
   void                          slot_reconnect();
   void                          startAutoLogin();
    
protected:
    
   void                          closeEvent(QCloseEvent *event);

private slots:
   
   void                          slot_close_profile();
   void                          slot_tab_changed( int );
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

   void                          goingDown() { mIsGoingDown = true; }
   QMap<QString, TConsole *>         mTabMap;
   QTabBar *                     mpTabBar;
   QWidget *                     mainPane;

   Host *                        mpDefaultHost; 
   QQueue<QString>               tempLoginQueue;
   QQueue<QString>               tempPassQueue;
   QQueue<Host *>                tempHostQueue;
   static                        mudlet * _self;
   QMap<QString, QDockWidget *>  dockWindowMap;
   QMap<QString, TConsole *>     dockWindowConsoleMap;
   QMap<QString, TLabel *>       mLabelMap;
   QMap<Host *, QToolBar *>      mUserToolbarMap; 
   QMap<QTimer *, TTimer *>      mTimerMap;
   QToolBar *                    mpMainToolBar;
   QMenu *                       restoreBar;
   bool                          mIsGoingDown;
   Host *                        mpCurrentActiveHost;
   QAction *                     actionReplaySpeedDown;
   QAction *                     actionReplaySpeedUp;

};

#endif

