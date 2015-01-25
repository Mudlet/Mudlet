#pragma once

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

#include "Profiles.h"

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

class Profile;
class TConsole;
class TEvent;
class TLabel;


class MainWindow : public QMainWindow, public Ui::MainWindow
{
Q_OBJECT

public:

     MainWindow();
    ~MainWindow();

    static                        MainWindow * self();
    // This method allows better debugging when mudlet::self() is called inappropriately.
    static                        void start();
    Profiles *                 getHostManager();
    void                          addSubWindow(TConsole* p);
    int                           getColumnNumber( Profile * pHost, QString & name );
    int                           getLineNumber( Profile * pHost, QString & name );
    void                          printSystemMessage( Profile * pH, const QString & s );
    void                          print( Profile *, const QString & );
    void                          addConsoleForNewHost( Profile * pH );
    void							 disableToolbarButtons();
    void							 enableToolbarButtons();
    Profile *                        getActiveHost();
    bool                          openWindow( Profile *, const QString & );
    bool                          createMiniConsole( Profile *, const QString &, int, int, int, int );
    bool                          createLabel( Profile *, const QString &, int, int, int, int, bool );
    bool                          echoWindow( Profile *, const QString &, const QString & );
    bool                          echoLink( Profile * pHost, const QString & name, const QString & text, QStringList &, QStringList &, bool customFormat=false );
    void                          insertLink( Profile *, const QString &, const QString&, QStringList &, QStringList &, bool customFormat=false );
    bool                          appendBuffer( Profile *, const QString & );
    bool                          createBuffer( Profile *, const QString & );
    bool                          showWindow( Profile *, const QString & );
    bool                          hideWindow( Profile *, const QString & );
    bool                          paste( Profile *, const QString & );
    bool                          closeWindow( Profile *, const QString & );
    bool                          resizeWindow( Profile *, const QString &, int, int );
    bool                          clearWindow( Profile *, const QString & );
    bool                          pasteWindow( Profile * pHost, const QString & name );
    bool                          setBackgroundColor( Profile *, const QString & name, int r, int g, int b, int alpha );
    bool                          setBackgroundImage( Profile *, const QString & name, QString & path );
    bool                          setTextFormat( Profile *, const QString & name, int, int, int, int, int, int, bool, bool, bool, bool );
    bool                          setLabelClickCallback( Profile *, const QString &, const QString &, const TEvent & );
    bool                          setLabelOnEnter( Profile *, const QString &, const QString &, const TEvent & );
    bool                          setLabelOnLeave( Profile *, const QString &, const QString &, const TEvent & );
    bool                          moveWindow( Profile *, const QString & name, int, int );
    void                          deleteLine( Profile *, const QString & name );
    void                          insertText( Profile *, const QString & name, const QString& );
    void                          replace( Profile *, const QString & name, const QString& );
    int                           selectString( Profile *, const QString & name, const QString& what, int );
    int                           selectSection( Profile *, const QString & name, int, int );
    void                          setBold( Profile *, const QString & name, bool );
    void                          setLink( Profile * pHost, const QString & name, const QString & linkText, QStringList & linkFunction, QStringList & );
    void                          setItalics( Profile *, const QString & name, bool );
    void                          setUnderline( Profile *, const QString & name, bool );
    void                          setStrikeOut( Profile *, const QString & name, bool );
    void                          setFgColor( Profile *, const QString & name, int, int, int );
    void                          setBgColor( Profile *, const QString & name, int, int, int );
    bool                          userWindowLineWrap( Profile * pHost, const QString & name, bool on );
    QString                       readProfileData( const QString& profile, const QString& item );
    bool                          setWindowWrap( Profile * pHost, const QString & name, int & wrap );
    bool                          setWindowWrapIndent( Profile * pHost, const QString & name, int & wrap );
    bool                          copy( Profile * pHost, const QString & name );
    bool                          moveCursorEnd( Profile *, const QString & );
    bool                          moveCursor( Profile *, const QString &, int, int );
    int                           getLastLineNumber( Profile *, const QString & );
    void                          readSettings();
    void                          writeSettings();
    void                          showUnzipProgress( const QString& txt );
    bool                          openWebPage(const QString& path);
    void                          processEventLoopHack();
    QMap<Profile *, TConsole *>     mConsoleMap;
    QMap<Profile *, QMap<QString, TConsole * > > mHostConsoleMap;
    QMap<Profile *, QMap<QString, TLabel * > > mHostLabelMap;
    QIcon *                       testicon;
    bool                          isGoingDown() { return mIsGoingDown; }
    int                           mTEFolderIconSize;
    void                          setIcoSize( int s );
    bool                          setConsoleBufferSize( Profile * pHost, const QString & name, int x1, int y1 );
    void                          showEvent( QShowEvent * event ) override;
    void                          hideEvent( QHideEvent * event ) override;
    bool                          resetFormat( Profile *, QString & name );
    bool                          moduleTableVisible();
    bool                          mWindowMinimized;
    void                          doAutoLogin( const QString & );
    void                          deselect( Profile * pHost, const QString & name );
    void                          stopSounds();
    void                          playSound( QString s );
    QString                       version;
    QPointer<Profile>                activeHost;
    QMediaPlayer *                mpMusicBox1;
    QMediaPlayer *                mpMusicBox2;
    QMediaPlayer *                mpMusicBox3;
    QMediaPlayer *                mpMusicBox4;
    QTabBar *                     tabBar;



public slots:

    void                          processEventLoopHack_timerRun();
    void                          toggleFullScreenView();
    void                          slot_multi_view();
    void                          slot_close_profile_requested(int);

protected:

    void                          closeEvent(QCloseEvent *event) override;

private slots:

    void                          slot_close_profile();
    void                          slot_tab_changed( int );

private:

    void                          goingDown() { mIsGoingDown = true; }
    QMap<QString, TConsole *>         mTabMap;
    QWidget *                     mainPane;

    QPointer<Profile>                mpDefaultHost;
    QQueue<QString>               tempLoginQueue;
    QQueue<QString>               tempPassQueue;
    QQueue<Profile *>                tempHostQueue;
    static                        QPointer<MainWindow> _self;
    QMap<QString, QDockWidget *>  dockWindowMap;
    QMap<Profile *, QToolBar *>      mUserToolbarMap;


    QMenu *                       restoreBar;
    bool                          mIsGoingDown;

    QTableWidget *                 moduleTable;
    QPushButton *                 moduleUninstallButton;
    QPushButton *                 moduleInstallButton;
    QPushButton *                 moduleHelpButton;

    Profiles                   mHostManager;
};

class TConsoleMonitor : public QObject
 {
     Q_OBJECT

public:
    TConsoleMonitor(QObject* parent) : QObject(parent) {}

 protected:
     bool eventFilter(QObject *obj, QEvent *event) override;
 };

