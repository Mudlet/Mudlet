#ifndef MUDLET_MUDLET_H
#define MUDLET_MUDLET_H

/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2015-2016 by Stephen Lyons - slysven@virginmedia.com    *
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


#include "HostManager.h"

#include "pre_guard.h"
#include "ui_main_window.h"
#include <QFlags>
#include <QMainWindow>
#include <QMap>
#include <QMediaPlayer>
#include <QQueue>
#include <QTime>
#include <QTextOption>
#include "post_guard.h"

#include <assert.h>

class QAction;
class QCloseEvent;
class QMenu;
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
class dlgIRC;


class mudlet : public QMainWindow, public Ui::MainWindow
{
Q_OBJECT

public:

                                 mudlet();
                                ~mudlet();
   static                        mudlet * self();
   void                          addSubWindow(TConsole* p);
   int                           getColumnNumber( Host * pHost, QString & name );
   int                           getLineNumber( Host * pHost, QString & name );
   void                          printSystemMessage( Host * pH, QString & s );
   void                          print( Host *, QString & );
   void                          addConsoleForNewHost( Host * pH );
   void							 disableToolbarButtons();
   void							 enableToolbarButtons();
   Host *                        getActiveHost();
   void                          registerTimer( TTimer *, QTimer * );
   void                          unregisterTimer( QTimer * );
   bool                          openWindow( Host *, QString & );
   bool                          createMiniConsole( Host *, QString &, int, int, int, int );
   bool                          createLabel( Host *, QString &, int, int, int, int, bool );
   bool                          echoWindow( Host *, QString &, QString & );
   bool                          echoLink( Host * pHost, QString & name, QString & text, QStringList &, QStringList &, bool customFormat=false );
   void                          insertLink( Host *, QString &, QString, QStringList &, QStringList &, bool customFormat=false );
   bool                          appendBuffer( Host *, QString & );
   bool                          createBuffer( Host *, QString & );
   bool                          showWindow( Host *, QString & );
   bool                          hideWindow( Host *, QString & );
   bool                          paste( Host *, QString & );
   bool                          closeWindow( Host *, QString & );
   bool                          resizeWindow( Host *, QString &, int, int );
   bool                          clearWindow( Host *, QString & );
   bool                          pasteWindow( Host * pHost, QString & name );
   bool                          setBackgroundColor( Host *, QString & name, int r, int g, int b, int alpha );
   bool                          setBackgroundImage( Host *, QString & name, QString & path );
   bool                          setTextFormat( Host *, QString & name, int, int, int, int, int, int, bool, bool, bool, bool );
   bool                          setLabelClickCallback( Host *, QString &, QString &, TEvent * );
   bool                          setLabelOnEnter( Host *, QString &, QString &, TEvent * );
   bool                          setLabelOnLeave( Host *, QString &, QString &, TEvent * );
   bool                          moveWindow( Host *, QString & name, int, int );
   void                          deleteLine( Host *, QString & name );
   void                          insertText( Host *, QString & name, QString );
   void                          replace( Host *, QString & name, QString );
   int                           selectString( Host *, QString & name, QString what, int );
   int                           selectSection( Host *, QString & name, int, int );
   void                          setBold( Host *, QString & name, bool );
   void                          setLink( Host * pHost, QString & name, QString & linkText, QStringList & linkFunction, QStringList & );
   void                          setItalics( Host *, QString & name, bool );
   void                          setUnderline( Host *, QString & name, bool );
   void                          setStrikeOut( Host *, QString & name, bool );
   void                          setFgColor( Host *, QString & name, int, int, int );
   void                          setBgColor( Host *, QString & name, int, int, int );
   bool                          userWindowLineWrap( Host * pHost, QString & name, bool on );
   QString                       readProfileData( QString profile, QString item );
   bool                          setWindowWrap( Host * pHost, QString & name, int & wrap );
   bool                          setWindowWrapIndent( Host * pHost, QString & name, int & wrap );
   bool                          copy( Host * pHost, QString & name );
   bool                          moveCursorEnd( Host *, QString & );
   bool                          moveCursor( Host *, QString &, int, int );
   int                           getLastLineNumber( Host *, QString & );
   void                          readSettings();
   void                          writeSettings();
   void                          showUnzipProgress( QString txt );
   bool                          openWebPage(QString path);
   void                          processEventLoopHack();
   static TConsole *             mpDebugConsole;
   static QMainWindow *          mpDebugArea;
   static bool                   debugMode;
   QMap<Host *, TConsole *>     mConsoleMap;
   QMap<Host *, QMap<QString, TConsole * > > mHostConsoleMap;
   QMap<Host *, QMap<QString, TLabel * > > mHostLabelMap;
   QIcon *                       testicon;
   bool                          mShowMenuBar;
   bool                          mShowToolbar;
   bool                          isGoingDown() { return mIsGoingDown; }
   int                           mMainIconSize;
   int                           mTEFolderIconSize;
   void                          setIcoSize( int s );
   void                          replayStart();
   bool                          setConsoleBufferSize( Host * pHost, QString & name, int x1, int y1 );
   void                          replayOver();
   void                          showEvent( QShowEvent * event );
   void                          hideEvent( QHideEvent * event );
   bool                          resetFormat( Host *, QString & name );
   bool                          moduleTableVisible();
   bool                          mWindowMinimized;
   //QString                       readProfileData( QString profile, QString item );
   void                          doAutoLogin( QString & );
   void                          deselect( Host * pHost, QString & name );
   void                          stopSounds();
   void                          playSound( QString s );
   QTime                         mReplayTime;
   int                           mReplaySpeed;
   QToolBar *                    mpMainToolBar;
   QMap<QTimer *, TTimer *>      mTimerMap;
   dlgIRC *                      mpIRC;
   QString                       version;
   Host *                        mpCurrentActiveHost;
   bool                          mAutolog;
   QString                       mIrcNick;
   QMediaPlayer *                mpMusicBox1;
   QMediaPlayer *                mpMusicBox2;
   QMediaPlayer *                mpMusicBox3;
   QMediaPlayer *                mpMusicBox4;
   QTabBar *                     mpTabBar;
   QStringList                   packagesToInstallList;
   QTextOption::Flags           mEditorTextOptions; // Used for editor area, but
                                                    // only ::ShowTabsAndSpaces
                                                    // and ::ShowLineAndParagraphSeparators
                                                    // are considered/used/stored
   void                         setEditorTextoptions( const bool, const bool );

   enum StatusBarOption {
       statusBarHidden = 0x0,     // Currently not on display
       statusBarAutoShown = 0x1,  // Currently shown but to hide as soon as there is no text to display
       statusBarAlwaysShown = 0x2
   };

   Q_DECLARE_FLAGS(StatusBarOptions, StatusBarOption)
   StatusBarOptions             mStatusBarState;

   void                         requestProfilesToReloadMaps( QList<QString> );
                                // Used by a profile to tell the mudlet class
                                // to tell other profiles to reload the updated
                                // maps (via signal_profileMapReloadRequested(...))

    const bool                  getAuditErrorsToConsoleEnabled() { return mIsToDisplayMapAuditErrorsToConsole; }
    void                        setAuditErrorsToConsoleEnabled( const bool state ) { mIsToDisplayMapAuditErrorsToConsole = state; }
    void                        createMapper( bool isToLoadDefaultMapFile = true );


public slots:

   void                          processEventLoopHack_timerRun();
   void                          slot_mapper();
   void                          slot_replayTimeChanged();
   void                          slot_replaySpeedUp();
   void                          slot_replaySpeedDown();
   void                          toggleFullScreenView();
   void                          slot_userToolBar_orientation_changed(Qt::Orientation);
   void                          slot_show_about_dialog();
   void                          slot_show_help_dialog_video();
   void                          slot_show_help_dialog_forum();
   void                          slot_show_help_dialog_irc();
   void                          slot_show_help_dialog_download();
   void                          slot_open_mappingscripts_page();
   void                          slot_module_clicked(QTableWidgetItem*);
   void                          slot_module_changed(QTableWidgetItem*);
   void                          slot_multi_view();
   void                          slot_stopAllTriggers();
   void                          slot_userToolBar_hovered( QAction* pA );
   void                          slot_connection_dlg_finnished( QString profile, int historyVersion );
   void                          slot_timer_fires();
   void                          slot_send_login();
   void                          slot_send_pass();
   void                          slot_replay();
   void                          slot_disconnect();
   void                          slot_notes();
   void                          slot_reconnect();
   void                          slot_close_profile_requested(int);
   void                          startAutoLogin();
   void                          slot_irc();
   void                          slot_uninstall_package();
   void                          slot_install_package();
   void                          slot_package_manager();
   void                          slot_package_exporter();
   void                          slot_uninstall_module();
   void                          slot_install_module();
   void                          slot_module_manager();
   void                          layoutModules();
   void                          slot_help_module();

protected:

   void                          closeEvent(QCloseEvent *event);

signals:

   void                         signal_editorTextOptionsChanged( QTextOption::Flags );
   void                         signal_profileMapReloadRequested( QList<QString> );

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
   void                         slot_statusBarMessageChanged( QString );

private:

   void                          goingDown() { mIsGoingDown = true; }
   QMap<QString, TConsole *>         mTabMap;
   //QTabBar *                     mpTabBar;
   QWidget *                     mainPane;

   Host *                        mpDefaultHost;
   QQueue<QString>               tempLoginQueue;
   QQueue<QString>               tempPassQueue;
   QQueue<Host *>                tempHostQueue;
   static                        mudlet * _self;
   QMap<QString, QDockWidget *>  dockWindowMap;
   //QMap<QString, TConsole *>     dockWindowConsoleMap;
   //QMap<QString, TLabel *>>       mLabelMap;
   QMap<Host *, QToolBar *>      mUserToolbarMap;


   QMenu *                       restoreBar;
   bool                          mIsGoingDown;

   QAction *                     actionReplaySpeedDown;
   QAction *                     actionReplaySpeedUp;
   QAction *                     actionReconnect;

   void                          check_for_mappingscript();

   QListWidget *                 packageList;
   QPushButton *                 uninstallButton;
   QPushButton *                 installButton;

   QTableWidget *                 moduleTable;
   QPushButton *                 moduleUninstallButton;
   QPushButton *                 moduleInstallButton;
   QPushButton *                 moduleHelpButton;

   QStatusBar *                 mpMainStatusBar;

   bool                         mIsToDisplayMapAuditErrorsToConsole;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(mudlet::StatusBarOptions)

class TConsoleMonitor : public QObject
 {
     Q_OBJECT

 protected:
     bool eventFilter(QObject *obj, QEvent *event);
 };

#endif // MUDLET_MUDLET_H
