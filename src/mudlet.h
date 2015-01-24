#ifndef MUDLET_MUDLET_H
#define MUDLET_MUDLET_H

/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
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

#include <QTableWidget>

#include "HostManager.h"

#include "pre_guard.h"
#include "ui_main_window.h"
#include <QMainWindow>
#include <QMap>
#include <QMediaPlayer>
#include <QPointer>
#include <QQueue>
#include <QTime>
#include "post_guard.h"

#include <assert.h>

class QAction;
class QCloseEvent;
class QMenu;
class QLabel;
class QListWidget;
class QPushButton;
class QTableWidget;
class QTableWidgetItem;
class QTextEdit;
class QTimer;

class Host;
class TConsole;
class TEvent;
class TLabel;
class TTimer;


class mudlet : public QMainWindow, public Ui::MainWindow
{
Q_OBJECT

public:

                                 mudlet();
                                ~mudlet();
   static                        mudlet * self();
   // This method allows better debugging when mudlet::self() is called inappropriately.
   static                        void start();
   HostManager *                 getHostManager();
   void                          addSubWindow(TConsole* p);
   int                           getColumnNumber( Host * pHost, QString & name );
   int                           getLineNumber( Host * pHost, QString & name );
   void                          printSystemMessage( Host * pH, const QString & s );
   void                          print( Host *, const QString & );
   void                          addConsoleForNewHost( Host * pH );
   void							 disableToolbarButtons();
   void							 enableToolbarButtons();
   Host *                        getActiveHost();
   void                          registerTimer( TTimer *, QTimer * );
   void                          unregisterTimer( QTimer * );
   bool                          openWindow( Host *, const QString & );
   bool                          createMiniConsole( Host *, const QString &, int, int, int, int );
   bool                          createLabel( Host *, const QString &, int, int, int, int, bool );
   bool                          echoWindow( Host *, const QString &, const QString & );
   bool                          echoLink( Host * pHost, const QString & name, const QString & text, QStringList &, QStringList &, bool customFormat=false );
   void                          insertLink( Host *, const QString &, const QString&, QStringList &, QStringList &, bool customFormat=false );
   bool                          appendBuffer( Host *, const QString & );
   bool                          createBuffer( Host *, const QString & );
   bool                          showWindow( Host *, const QString & );
   bool                          hideWindow( Host *, const QString & );
   bool                          paste( Host *, const QString & );
   bool                          closeWindow( Host *, const QString & );
   bool                          resizeWindow( Host *, const QString &, int, int );
   bool                          clearWindow( Host *, const QString & );
   bool                          pasteWindow( Host * pHost, const QString & name );
   bool                          setBackgroundColor( Host *, const QString & name, int r, int g, int b, int alpha );
   bool                          setBackgroundImage( Host *, const QString & name, QString & path );
   bool                          setTextFormat( Host *, const QString & name, int, int, int, int, int, int, bool, bool, bool, bool );
   bool                          setLabelClickCallback( Host *, const QString &, const QString &, const TEvent & );
   bool                          setLabelOnEnter( Host *, const QString &, const QString &, const TEvent & );
   bool                          setLabelOnLeave( Host *, const QString &, const QString &, const TEvent & );
   bool                          moveWindow( Host *, const QString & name, int, int );
   void                          deleteLine( Host *, const QString & name );
   void                          insertText( Host *, const QString & name, const QString& );
   void                          replace( Host *, const QString & name, const QString& );
   int                           selectString( Host *, const QString & name, const QString& what, int );
   int                           selectSection( Host *, const QString & name, int, int );
   void                          setBold( Host *, const QString & name, bool );
   void                          setLink( Host * pHost, const QString & name, const QString & linkText, QStringList & linkFunction, QStringList & );
   void                          setItalics( Host *, const QString & name, bool );
   void                          setUnderline( Host *, const QString & name, bool );
   void                          setStrikeOut( Host *, const QString & name, bool );
   void                          setFgColor( Host *, const QString & name, int, int, int );
   void                          setBgColor( Host *, const QString & name, int, int, int );
   bool                          userWindowLineWrap( Host * pHost, const QString & name, bool on );
   QString                       readProfileData( const QString& profile, const QString& item );
   bool                          setWindowWrap( Host * pHost, const QString & name, int & wrap );
   bool                          setWindowWrapIndent( Host * pHost, const QString & name, int & wrap );
   bool                          copy( Host * pHost, const QString & name );
   bool                          moveCursorEnd( Host *, const QString & );
   bool                          moveCursor( Host *, const QString &, int, int );
   int                           getLastLineNumber( Host *, const QString & );
   void                          readSettings();
   void                          writeSettings();
   void                          showUnzipProgress( const QString& txt );
   bool                          openWebPage(const QString& path);
   void                          processEventLoopHack();
   QMap<Host *, TConsole *>     mConsoleMap;
   QMap<Host *, QMap<QString, TConsole * > > mHostConsoleMap;
   QMap<Host *, QMap<QString, TLabel * > > mHostLabelMap;
   QIcon *                       testicon;
   bool                          isGoingDown() { return mIsGoingDown; }
   int                           mTEFolderIconSize;
   void                          setIcoSize( int s );
   void                          replayStart();
   bool                          setConsoleBufferSize( Host * pHost, const QString & name, int x1, int y1 );
   void                          replayOver();
   void                          showEvent( QShowEvent * event ) override;
   void                          hideEvent( QHideEvent * event ) override;
   bool                          resetFormat( Host *, QString & name );
   bool                          moduleTableVisible();
   bool                          mWindowMinimized;
   void                          doAutoLogin( const QString & );
   void                          deselect( Host * pHost, const QString & name );
   void                          stopSounds();
   void                          playSound( QString s );
   QString                       version;
   QPointer<Host>                mpCurrentActiveHost;
   bool                          mAutolog;
   QMediaPlayer *                mpMusicBox1;
   QMediaPlayer *                mpMusicBox2;
   QMediaPlayer *                mpMusicBox3;
   QMediaPlayer *                mpMusicBox4;
   QTabBar *                     mpTabBar;



public slots:

   void                          processEventLoopHack_timerRun();
   void                          slot_replayTimeChanged();
   void                          slot_replaySpeedUp();
   void                          slot_replaySpeedDown();
   void                          toggleFullScreenView();
   void                          slot_userToolBar_orientation_changed(Qt::Orientation);
   void                          slot_module_clicked(QTableWidgetItem*);
   void                          slot_module_changed(QTableWidgetItem*);
   void                          slot_multi_view();
   void                          slot_userToolBar_hovered( QAction* pA );
   void                          slot_connection_dlg_finnished( const QString& profile, int historyVersion );
   void                          slot_timer_fires();
   void                          slot_send_login();
   void                          slot_send_pass();
   void                          slot_replay();
   void                          slot_disconnect();
   void                          slot_notes();
   void                          slot_reconnect();
   void                          slot_close_profile_requested(int);
   void                          startAutoLogin();

protected:

   void                          closeEvent(QCloseEvent *event) override;

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
   void                          show_variable_dialog();
   void                          show_options_dialog();

private:

   void                          goingDown() { mIsGoingDown = true; }
   QMap<QString, TConsole *>         mTabMap;
   QWidget *                     mainPane;

   QPointer<Host>                mpDefaultHost;
   QQueue<QString>               tempLoginQueue;
   QQueue<QString>               tempPassQueue;
   QQueue<Host *>                tempHostQueue;
   static                        QPointer<mudlet> _self;
   QMap<QString, QDockWidget *>  dockWindowMap;
   QMap<Host *, QToolBar *>      mUserToolbarMap;


   QMenu *                       restoreBar;
   bool                          mIsGoingDown;

   QTableWidget *                 moduleTable;
   QPushButton *                 moduleUninstallButton;
   QPushButton *                 moduleInstallButton;
   QPushButton *                 moduleHelpButton;

   HostManager                   mHostManager;
};

class TConsoleMonitor : public QObject
 {
     Q_OBJECT

public:
    TConsoleMonitor(QObject* parent) : QObject(parent) {}

 protected:
     bool eventFilter(QObject *obj, QEvent *event) override;
 };

#endif // MUDLET_MUDLET_H
