/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2013-2014 by Stephen Lyons - slysven@virginmedia.com    *
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


#include <QTimer>
#include <QToolButton>
#include <QSettings>
#include "mudlet.h"


#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "Host.h"
#include "HostManager.h"
#include "TCommandLine.h"
#include "TConsole.h"
#include "TDebug.h"
#include "TEvent.h"
#include "TLabel.h"
#include "TTextEdit.h"
#include "XMLimport.h"

#include "pre_guard.h"
#include <QtEvents>
#include <QApplication>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QDockWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QScrollBar>
#include <QTextCharFormat>
#include <QToolBar>
#include <QtUiTools/quiloader.h>
#include "post_guard.h"


using namespace std;

bool TConsoleMonitor::eventFilter(QObject *obj, QEvent *event)
{
    if( event->type() == QEvent::Close )
    {
        return QObject::eventFilter(obj,event);
    }
    else
    {
        return QObject::eventFilter(obj, event);
    }
}

static const QString timeFormat = "hh:mm:ss";

QPointer<mudlet> mudlet::_self;

void mudlet::start()
{
    _self = new mudlet;
}

mudlet * mudlet::self()
{
    return _self;
}


mudlet::mudlet()
: QMainWindow()
, mWindowMinimized( false )
, version( QString("Mudlet ") + QString(APP_VERSION) + QString(APP_BUILD) )
, mpCurrentActiveHost( 0 )
, mIsGoingDown( false )
{
    setupUi(this);
    setUnifiedTitleAndToolBarOnMac( true );
    setContentsMargins(0,0,0,0);
    setAttribute( Qt::WA_DeleteOnClose );
    QSizePolicy sizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding);
    setWindowTitle(version);
    setWindowIcon( QIcon( QStringLiteral( ":/icons/mudlet_main_16px.png" ) ) );
    QWidget * frame = new QWidget( this );
    frame->setFocusPolicy( Qt::NoFocus );
    setCentralWidget( frame );
    mpTabBar = new QTabBar( frame );
    mpTabBar->setMaximumHeight(30);
    mpTabBar->setFocusPolicy( Qt::NoFocus );
#if QT_VERSION >= 0x040500
    mpTabBar->setTabsClosable ( true );
    connect( mpTabBar, SIGNAL(tabCloseRequested(int)), this, SLOT(slot_close_profile_requested(int)));
    mpTabBar->setMovable(true);
#endif
    connect( mpTabBar, SIGNAL(currentChanged(int)), this, SLOT(slot_tab_changed(int)));
    QVBoxLayout * layoutTopLevel = new QVBoxLayout(frame);
    layoutTopLevel->setContentsMargins(0,0,0,0);
    layoutTopLevel->setContentsMargins(0,0,0,0);
    layoutTopLevel->addWidget( mpTabBar );
    mainPane = new QWidget( frame );
    QPalette mainPalette;
    mainPane->setPalette( mainPalette );
    mainPane->setAutoFillBackground(true);
    mainPane->setFocusPolicy( Qt::NoFocus );
    layoutTopLevel->addWidget( mainPane );
    QHBoxLayout * layout = new QHBoxLayout( mainPane );
    layout->setContentsMargins(0,0,0,0);
    layout->setContentsMargins(0,0,0,0);

    mainPane->setContentsMargins(0,0,0,0);
    mainPane->setSizePolicy( sizePolicy );
    mainPane->setFocusPolicy( Qt::NoFocus );

    QFile file_autolog( QDir::homePath()+"/.config/mudlet/autolog" );
    if( file_autolog.exists() )
    {
        mAutolog = true;
    }
    else
    {
        mAutolog = false;
    }


    mHostManager.addHost("default_host", "", "","" );
    mpDefaultHost = mHostManager.getHost(QString("default_host"));

    QSize generalRule( qApp->desktop()->size() );
    generalRule -= QSize( 30, 30 );
    QFont mainFont;
    mainFont = QFont("Bitstream Vera Sans Mono", 8, QFont::Courier);
    QFont mdiFont = QFont("Bitstream Vera Sans Mono", 6, QFont::Courier);
    setFont( mainFont );
    mainPane->setFont( mainFont );
    mpTabBar->setFont( mdiFont );

    mainPane->show();

    QAction * mactionConnect = new QAction(tr("Connect"), this);
    QAction * mactionTriggers = new QAction(tr("Triggers"), this);
    QAction * mactionAlias = new QAction(tr("Aliases"), this);
    QAction * mactionTimers = new QAction(tr("Timers"), this);
    QAction * mactionButtons = new QAction(tr("Actions"), this);
    QAction * mactionScripts = new QAction(tr("Scripts"), this);
    QAction * mactionKeys = new QAction(tr("Keys"), this);
    QAction * mactionMapper = new QAction(tr("Map"), this);
    QAction * mactionHelp = new QAction(tr("Help"), this);
    QAction * mactionOptions = new QAction(tr("Preferences"), this);
    QAction * mactionMultiView = new QAction(tr("MultiView"), this);
    QAction * mactionAbout = new QAction(tr("About"), this);
    QAction * mactionCloseProfile = new QAction(tr("Close"), this);

    connect(mactionConnect, SIGNAL(triggered()), this, SLOT(connectToServer()));
    connect(dactionConnect, SIGNAL(triggered()), this, SLOT(connectToServer()));
    connect(dactionReconnect, SIGNAL(triggered()), this, SLOT(slot_reconnect()));
    connect(dactionDisconnect, SIGNAL(triggered()), this, SLOT(slot_disconnect()));
    connect(dactionNotepad, SIGNAL(triggered()), this, SLOT(slot_notes()));
    connect(dactionReplay, SIGNAL(triggered()), this, SLOT(slot_replay()));


    connect(mactionHelp, SIGNAL(triggered()), this, SLOT(show_help_dialog()));
    connect(dactionHelp, SIGNAL(triggered()), this, SLOT(show_help_dialog()));
    connect(dactionVideo, SIGNAL(triggered()), this, SLOT(slot_show_help_dialog_video()));
    connect(dactionForum, SIGNAL(triggered()), this, SLOT(slot_show_help_dialog_forum()));
    connect(actionShow_Map, SIGNAL(triggered()), this, SLOT(slot_mapper()));
    connect(dactionDownload, SIGNAL(triggered()), this, SLOT(slot_show_help_dialog_download()));
    connect(actionPackage_manager, SIGNAL(triggered()), this, SLOT(slot_package_manager()));
    connect(actionPackage_Exporter, SIGNAL(triggered()), this, SLOT(slot_package_exporter()));
    connect(actionModule_manager, SIGNAL(triggered()), this, SLOT(slot_module_manager()));
    connect(dactionMultiView, SIGNAL(triggered()), this, SLOT(slot_multi_view()));

    connect(mactionTriggers, SIGNAL(triggered()), this, SLOT(show_trigger_dialog()));
    connect(dactionScriptEditor, SIGNAL(triggered()), this, SLOT(show_trigger_dialog()));
    connect(mactionMapper, SIGNAL(triggered()), this, SLOT(slot_mapper()));
    connect(mactionTimers, SIGNAL(triggered()), this, SLOT(show_timer_dialog()));
    connect(mactionAlias, SIGNAL(triggered()), this, SLOT(show_alias_dialog()));
    connect(mactionScripts, SIGNAL(triggered()), this, SLOT(show_script_dialog()));
    connect(mactionKeys, SIGNAL(triggered()),this,SLOT(show_key_dialog()));
    connect(mactionButtons, SIGNAL(triggered()), this, SLOT(show_action_dialog()));

    connect(mactionOptions, SIGNAL(triggered()), this, SLOT(show_options_dialog()));
    connect(dactionOptions, SIGNAL(triggered()), this, SLOT(show_options_dialog()));

    connect(mactionAbout, SIGNAL(triggered()), this, SLOT(slot_show_about_dialog()));
    connect(dactionAbout, SIGNAL(triggered()), this, SLOT(slot_show_about_dialog()));

    connect(mactionMultiView, SIGNAL(triggered()), this, SLOT(slot_multi_view()));
    connect(mactionCloseProfile, SIGNAL(triggered()), this, SLOT(slot_close_profile()));

    readSettings();

    QTimer * timerAutologin = new QTimer( this );
    timerAutologin->setSingleShot( true );
    connect(timerAutologin, SIGNAL(timeout()), this, SLOT(startAutoLogin()));
    timerAutologin->start( 1000 );

    mpMusicBox1 = new QMediaPlayer(this);
    mpMusicBox2 = new QMediaPlayer(this);
    mpMusicBox3 = new QMediaPlayer(this);
    mpMusicBox4 = new QMediaPlayer(this);

}

HostManager * mudlet::getHostManager()
{
    return &mHostManager;
}

bool mudlet::moduleTableVisible()
{
    if (moduleTable)
        return moduleTable->isVisible();
    return false;
}

bool mudlet::openWebPage(const QString& path){
    if (path.isEmpty() || path.isNull())
        return false;
    QUrl url(path,QUrl::TolerantMode);
    if (!url.isValid())
        return false;
    return QDesktopServices::openUrl(url);
}


void mudlet::slot_close_profile_requested( int tab )
{
    QString name = mpTabBar->tabText( tab );
    Host * pH = getHostManager()->getHost( name );
    if( ! pH ) return;

    if( ! pH->mpConsole->close() )
        return;
    else
        pH->mpConsole->mUserAgreedToCloseConsole = true;
    mConsoleMap[pH]->close();
    if( mTabMap.contains( pH->getName() ) )
    {
        mpTabBar->removeTab( tab );
        mConsoleMap.remove( pH );
        mTabMap.remove( pH->getName() );
        getHostManager()->deleteHost( pH->getName() );
    }

    // hide the tab bar if we only have 1 or no tabs available. saves screen space.
    if( mConsoleMap.size() > 1 )
    {
        mpTabBar->show();
    }
    else
        mpTabBar->hide();

}

void mudlet::slot_close_profile()
{
    if( mpCurrentActiveHost )
    {
        if( mConsoleMap.contains( mpCurrentActiveHost ) )
        {
            QString name = mpCurrentActiveHost->getName();
            Host * pH = mpCurrentActiveHost;
            mConsoleMap[mpCurrentActiveHost]->close();
            if( mTabMap.contains( pH->getName() ) )
            {
                mpTabBar->removeTab( mpTabBar->currentIndex() );
                mConsoleMap.remove( pH );
                getHostManager()->deleteHost( pH->getName() );
                mTabMap.remove( pH->getName() );
            }

        }
    }
}

void mudlet::slot_tab_changed( int tabID )
{
    if( ( ! mTabMap.contains( mpTabBar->tabText( tabID ) ) ) && ( tabID != -1 ) )
    {
        mpCurrentActiveHost = 0;
        return;
    }

    if( mConsoleMap.contains( mpCurrentActiveHost ) )
    {
        mpCurrentActiveHost->mpConsole->hide();
        QString host = mpTabBar->tabText( tabID );
        if( mTabMap.contains( host ) )
        {
            mpCurrentActiveHost = mTabMap[host]->mpHost;
        }
        else
        {
            mpCurrentActiveHost = 0;
            return;
        }
    }
    else
    {
        if( mTabMap.size() > 0 )
        {
            mpCurrentActiveHost = mTabMap.begin().value()->mpHost;
        }
        else
        {
            mpCurrentActiveHost = 0;
            return;
        }
    }

    if( ! mpCurrentActiveHost || mConsoleMap.contains( mpCurrentActiveHost ) )
    {
        if( ! mpCurrentActiveHost ) return;
        mpCurrentActiveHost->mpConsole->show();
        mpCurrentActiveHost->mpConsole->repaint();
        mpCurrentActiveHost->mpConsole->refresh();
        mpCurrentActiveHost->mpConsole->mpCommandLine->repaint();
        mpCurrentActiveHost->mpConsole->mpCommandLine->setFocus();
        mpCurrentActiveHost->mpConsole->show();

        int x = mpCurrentActiveHost->mpConsole->width();
        int y = mpCurrentActiveHost->mpConsole->height();
        QSize s = QSize(x,y);
        QResizeEvent event(s, s);
        QApplication::sendEvent( mpCurrentActiveHost->mpConsole, &event);
    }
    else
    {
        mpCurrentActiveHost = 0;
        return;
    }

    // update the window title for the currently selected profile
    setWindowTitle(mpCurrentActiveHost->getName() + " - " + version);
}

void mudlet::addConsoleForNewHost( Host * pH )
{
    if( mConsoleMap.contains( pH ) ) return;
    pH->mLogStatus = mAutolog;
    TConsole * pConsole = new TConsole( pH, false );
    if( ! pConsole ) return;
    pH->mpConsole = pConsole;
    pConsole->setWindowTitle( pH->getName() );
    pConsole->setObjectName( pH->getName() );
    mConsoleMap[pH] = pConsole;
    int newTabID = mpTabBar->addTab( pH->getName() );
    mTabMap[pH->getName()] = pConsole;
    if( mConsoleMap.size() > 1 )
    {
        mpTabBar->show();
    }
    else
        mpTabBar->hide();
    //update the main window title when we spawn a new tab
    setWindowTitle(pH->getName() + " - " + version);

    mainPane->layout()->addWidget( pConsole );
    if( mpCurrentActiveHost )
        mpCurrentActiveHost->mpConsole->hide();
    mpCurrentActiveHost = pH;
    if( pH->mLogStatus ) pConsole->logButton->click();

    pConsole->show();
    connect( pConsole->emergencyStop, SIGNAL(pressed()), this , SLOT(slot_stopAllTriggers()));

    QMap<QString, TConsole *> miniConsoleMap;
    mHostConsoleMap[mpCurrentActiveHost] = miniConsoleMap;
    QMap<QString, TLabel *> labelMap;
    mHostLabelMap[mpCurrentActiveHost] = labelMap;
    mpCurrentActiveHost->mpConsole->show();
    mpCurrentActiveHost->mpConsole->repaint();
    mpCurrentActiveHost->mpConsole->refresh();
    mpCurrentActiveHost->mpConsole->mpCommandLine->repaint();
    mpCurrentActiveHost->mpConsole->mpCommandLine->setFocus();
    mpCurrentActiveHost->mpConsole->show();
    mpTabBar->setCurrentIndex( newTabID );
    int x = mpCurrentActiveHost->mpConsole->width();
    int y = mpCurrentActiveHost->mpConsole->height();
    QSize s = QSize(x,y);
    QResizeEvent event(s, s);
    QApplication::sendEvent( mpCurrentActiveHost->mpConsole, &event);
}

bool mudlet::openWindow( Host * pHost, const QString & name )
{
    if( ! dockWindowMap.contains( name ) )
    {
        QDockWidget * pD = new QDockWidget;
        pD->setContentsMargins(0,0,0,0);
        pD->setFeatures( QDockWidget::AllDockWidgetFeatures );
        pD->setWindowTitle( name );
        dockWindowMap[name] = pD;
        TConsole * pC = new TConsole( pHost, false );
        pC->setContentsMargins(0,0,0,0);
        pD->setWidget( pC );
        pC->show();
        pC->layerCommandLine->hide();
        pC->mpScrollBar->hide();
        pC->setUserWindow();
        QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
        dockWindowConsoleMap[name] = pC;
        addDockWidget(Qt::RightDockWidgetArea, pD);
        return true;
    }
    else return false;
}

bool mudlet::createMiniConsole( Host * pHost, const QString & name, int x, int y, int width, int height )
{
    if( ! pHost ) return false;
    if( ! pHost->mpConsole ) return false;

    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( ! dockWindowConsoleMap.contains( name ) )
    {
        TConsole * pC = pHost->mpConsole->createMiniConsole( name, x, y, width, height );
        pC->mConsoleName = name;
        if( pC )
        {
            dockWindowConsoleMap[name] = pC;
            std::string _n = name.toStdString();
            pC->setMiniConsoleFontSize( _n, 12 );
            return true;
        }
    }
    else
    {
        TConsole * pC = dockWindowConsoleMap[name];
        pC->resize( width, height );
        pC->move( x, y );
    }
    return false;
}

bool mudlet::createLabel( Host * pHost, const QString & name, int x, int y, int width, int height, bool fillBg )
{
    if( ! pHost ) return false;
    if( ! pHost->mpConsole ) return false;
    QMap<QString, TLabel *> & labelMap = mHostLabelMap[pHost];
    if( ! labelMap.contains( name ) )
    {
        TLabel * pL = pHost->mpConsole->createLabel( name, x, y, width, height, fillBg );
        if( pL )
        {
            labelMap[name] = pL;
            return true;
        }
    }
    return false;
}

bool mudlet::createBuffer( Host * pHost, const QString & name )
{
    if( ! pHost ) return false;
    if( ! pHost->mpConsole ) return false;

    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( ! dockWindowConsoleMap.contains( name ) )
    {
        TConsole * pC = pHost->mpConsole->createBuffer( name );
        pC->mConsoleName = name;
        if( pC )
        {
            dockWindowConsoleMap[name] = pC;
            return true;
        }
    }
    return false;
}

bool mudlet::setBackgroundColor( Host * pHost, const QString & name, int r, int g, int b, int alpha )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->setConsoleBgColor(r,g,b);
        return true;
    }
    else
    {
        QMap<QString, TLabel *> & labelMap = mHostLabelMap[pHost];
        if( labelMap.contains( name ) )
        {
            QPalette mainPalette;
            mainPalette.setColor( QPalette::Window, QColor(r, g, b, alpha) );
            labelMap[name]->setPalette( mainPalette );
            return true;
        }
    }
    return false;
}

bool mudlet::setBackgroundImage( Host * pHost, const QString & name, QString & path )
{
    QMap<QString, TLabel *> & labelMap = mHostLabelMap[pHost];
    if( labelMap.contains( name ) )
    {
        if( QDir::homePath().contains('\\') )
        {
            path.replace('/', "\\");
        }
        else
            path.replace('\\', "/");
        QPixmap bgPixmap( path );
        labelMap[name]->setPixmap( bgPixmap );
        return true;
    }
    else
        return false;
}

bool mudlet::setTextFormat( Host * pHost, const QString & name, int r1, int g1, int b1, int r2, int g2, int b2, bool bold, bool underline, bool italics, bool strikeout )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        TConsole * pC = dockWindowConsoleMap[name];
        pC->mFormatCurrent.bgR = r1;
        pC->mFormatCurrent.bgG = g1;
        pC->mFormatCurrent.bgB = b1;
        pC->mFormatCurrent.fgR = r2;
        pC->mFormatCurrent.fgG = g2;
        pC->mFormatCurrent.fgB = b2;
        if( bold )
            pC->mFormatCurrent.flags |= TCHAR_BOLD;
        else
            pC->mFormatCurrent.flags &= ~(TCHAR_BOLD);
        if( underline )
            pC->mFormatCurrent.flags |= TCHAR_UNDERLINE;
        else
            pC->mFormatCurrent.flags &= ~(TCHAR_UNDERLINE);
        if( italics )
            pC->mFormatCurrent.flags |= TCHAR_ITALICS;
        else
            pC->mFormatCurrent.flags &= ~(TCHAR_ITALICS);
        if( strikeout )
            pC->mFormatCurrent.flags |= TCHAR_STRIKEOUT;
        else
            pC->mFormatCurrent.flags &= ~(TCHAR_STRIKEOUT);
        return true;
    }
    else
        return false;
}

void mudlet::showEvent( QShowEvent * event )
{
    mWindowMinimized = false;
    QMainWindow::showEvent( event );
}

void mudlet::hideEvent( QHideEvent * event )
{
    mWindowMinimized = true;
    QMainWindow::hideEvent( event );
}

bool mudlet::clearWindow( Host * pHost, const QString & name )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->buffer.clear();
        dockWindowConsoleMap[name]->console->update();
        return true;
    }
    else
        return false;
}

bool mudlet::showWindow( Host * pHost, const QString & name )
{
    // check labels first as they are shown/hidden more often
    QMap<QString, TLabel *> & labelMap = mHostLabelMap[pHost];
    if( labelMap.contains( name ) )
    {
        labelMap[name]->show();
        return true;
    }

    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->console->show();
        dockWindowConsoleMap[name]->console->forceUpdate();
        return true;
    }

    return false;
}

bool mudlet::paste( Host * pHost, const QString & name )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->paste();
        return true;
    }
    else
        return false;
}

bool mudlet::hideWindow( Host * pHost, const QString & name )
{
    // check labels first as they are shown/hidden more often
    QMap<QString, TLabel *> & labelMap = mHostLabelMap[pHost];
    if( labelMap.contains( name ) )
    {
        labelMap[name]->hide();
        return true;
    }

    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->console->hide();
        dockWindowConsoleMap[name]->console->forceUpdate();
        return true;
    }

    return false;
}

bool mudlet::resizeWindow( Host * pHost, const QString & name, int x1, int y1 )
{
    QMap<QString, TLabel *> & labelMap = mHostLabelMap[pHost];
    if( labelMap.contains( name ) )
    {
        labelMap[name]->resize( x1, y1 );
        return true;
    }

    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->resize( x1, y1 );
        return true;
    }

    return false;
}

bool mudlet::setConsoleBufferSize( Host * pHost, const QString & name, int x1, int y1 )
{
    if( name == "main" )
    {
        pHost->mpConsole->buffer.setBufferSize( x1, y1 );
        return true;
    }

    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];

    if( dockWindowConsoleMap.contains( name ) )
    {
        (dockWindowConsoleMap[name]->buffer).setBufferSize( x1, y1 );
        return true;
    }
    else
        return false;
}



bool mudlet::resetFormat( Host * pHost, QString & name )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->reset();
        return true;
    }
    else
        return false;
}

bool mudlet::moveWindow( Host * pHost, const QString & name, int x1, int y1 )
{
    QMap<QString, TLabel *> & labelMap = mHostLabelMap[pHost];
    if( labelMap.contains( name ) )
    {
        labelMap[name]->move( x1, y1 );
        return true;
    }

    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->move( x1, y1 );
        dockWindowConsoleMap[name]->mOldX = x1;
        dockWindowConsoleMap[name]->mOldY = y1;
        return true;
    }

    return false;
}

bool mudlet::closeWindow( Host * pHost, const QString & name )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->console->close();
        return true;
    }
    else
        return false;
}

bool mudlet::setLabelClickCallback( Host * pHost, const QString & name, const QString & func, const TEvent & pA )
{
    QMap<QString, TLabel *> & labelMap = mHostLabelMap[pHost];
    if( labelMap.contains( name ) )
    {
        labelMap[name]->setScript( pHost, func, pA );
        return true;
    }
    else
        return false;
}

bool mudlet::setLabelOnEnter( Host * pHost, const QString & name, const QString & func, const TEvent & pA )
{
    QMap<QString, TLabel *> & labelMap = mHostLabelMap[pHost];
    if( labelMap.contains( name ) )
    {
        labelMap[name]->setEnter( pHost, func, pA );
        return true;
    }
    else
        return false;
}

bool mudlet::setLabelOnLeave( Host * pHost, const QString & name, const QString & func, const TEvent & pA )
{
    QMap<QString, TLabel *> & labelMap = mHostLabelMap[pHost];
    if( labelMap.contains( name ) )
    {
        labelMap[name]->setLeave( pHost, func, pA );
        return true;
    }
    else
        return false;
}

int mudlet::getLineNumber( Host * pHost, QString & name )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        return dockWindowConsoleMap[name]->getLineNumber();
    }
    return -1;
}

int mudlet::getColumnNumber( Host * pHost, QString & name )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        return dockWindowConsoleMap[name]->getColumnNumber();
    }
    return -1;
}


int mudlet::getLastLineNumber( Host * pHost, const QString & name )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        return dockWindowConsoleMap[name]->getLastLineNumber();
    }
    return -1;
}

bool mudlet::moveCursorEnd( Host * pHost, const QString & name )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->moveCursorEnd();
        return true;
    }
    else
        return false;
}

bool mudlet::moveCursor( Host * pHost, const QString & name, int x, int y )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        return dockWindowConsoleMap[name]->moveCursor( x, y );
    }
    else
        return false;
}

void mudlet::deleteLine( Host * pHost, const QString & name )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->skipLine();
    }
}

void mudlet::insertText( Host * pHost, const QString & name, const QString& text )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->insertText( text );
    }
}

void mudlet::insertLink( Host * pHost, const QString & name, const QString& text, QStringList & func, QStringList & hint, bool customFormat )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->insertLink( text, func, hint, customFormat );
    }
}

void mudlet::replace( Host * pHost, const QString & name, const QString& text )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->replace( text );
    }
}

void mudlet::setLink( Host * pHost, const QString & name, const QString & linkText, QStringList & linkFunction, QStringList & linkHint )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        TConsole * pC = dockWindowConsoleMap[name];
        pC->setLink( linkText, linkFunction, linkHint );
    }
}

void mudlet::setBold( Host * pHost, const QString & name, bool b )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        TConsole * pC = dockWindowConsoleMap[name];
        pC->setBold( b );
    }
}

void mudlet::setItalics( Host * pHost, const QString & name, bool b )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        TConsole * pC = dockWindowConsoleMap[name];
        pC->setItalics( b );
    }
}

void mudlet::setUnderline( Host * pHost, const QString & name, bool b )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        TConsole * pC = dockWindowConsoleMap[name];
        pC->setUnderline( b );
    }
}

void mudlet::setStrikeOut( Host * pHost, const QString & name, bool b )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        TConsole * pC = dockWindowConsoleMap[name];
        pC->setStrikeOut( b );
    }
}

void mudlet::setFgColor( Host * pHost, const QString & name, int r, int g, int b )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        TConsole * pC = dockWindowConsoleMap[name];
        pC->setFgColor( r, g, b );
    }
}

void mudlet::setBgColor( Host * pHost, const QString & name, int r, int g, int b )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        TConsole * pC = dockWindowConsoleMap[name];
        pC->setBgColor( r, g, b );
    }
}

int mudlet::selectString( Host * pHost, const QString & name, const QString& text, int num )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        TConsole * pC = dockWindowConsoleMap[name];
        return pC->select( text, num );
    }
    else
        return -1;
}

int mudlet::selectSection( Host * pHost, const QString & name, int f, int t )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        TConsole * pC = dockWindowConsoleMap[name];
        return pC->selectSection( f, t );
    }
    else
        return -1;
}

void mudlet::deselect( Host * pHost, const QString & name )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        TConsole * pC = dockWindowConsoleMap[name];
        pC->deselect();
    }
}

bool mudlet::setWindowWrap( Host * pHost, const QString & name, int & wrap )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    QString wn = name;
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[wn]->setWrapAt( wrap );
        return true;
    }
    else
        return false;
}

bool mudlet::setWindowWrapIndent( Host * pHost, const QString & name, int & wrap )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    QString wn = name;
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[wn]->setIndentCount( wrap );
        return true;
    }
    else
        return false;
}

bool mudlet::echoWindow( Host * pHost, const QString & name, const QString & text )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    QString t = text;
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->echoUserWindow( t );
        return true;
    }
    QMap<QString, TLabel *> & labelMap = mHostLabelMap[pHost];
    if( labelMap.contains( name ) )
    {
        labelMap[name]->setText( t );
        return true;
    }
    else
        return false;
}

bool mudlet::echoLink( Host * pHost, const QString & name, const QString & text, QStringList & func, QStringList & hint, bool customFormat )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    QString t = text;
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->echoLink( text, func, hint, customFormat );
        return true;
    }
    else
        return false;
}

bool mudlet::copy( Host * pHost, const QString & name )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->copy();
        return true;
    }
    else return false;
}

bool mudlet::pasteWindow( Host * pHost, const QString & name )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->pasteWindow( mConsoleMap[pHost]->mClipboard );
        return true;
    }
    else return false;
}

bool mudlet::appendBuffer( Host * pHost, const QString & name )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->appendBuffer( mConsoleMap[pHost]->mClipboard );
        return true;
    }
    else return false;
}

void mudlet::slot_userToolBar_hovered( QAction* pA )
{
    QStringList sL;
    if( pA->menu() )
    {
        pA->menu()->exec();
    }


}



void mudlet::slot_userToolBar_orientation_changed( Qt::Orientation dir )
{
}

Host * mudlet::getActiveHost()
{
    if( mConsoleMap.contains( mpCurrentActiveHost ) )
    {
        return mpCurrentActiveHost;
    }
    else
        return 0;
}



void mudlet::addSubWindow( TConsole* pConsole )
{
    mainPane->layout()->addWidget( pConsole );
    pConsole->show();//NOTE: this is important for Apple OSX otherwise the console isnt displayed
}

void mudlet::closeEvent(QCloseEvent *event)
{
    foreach( TConsole * pC, mConsoleMap )
    {
        if( ! pC->close() )
        {
            event->ignore();
            return;
        }
        else
            pC->mUserAgreedToCloseConsole = true;
    }

    writeSettings();

    goingDown();
    foreach( TConsole * pC, mConsoleMap )
    {
        if( pC->mpHost->getName() != "default_host" )
        {
            pC->close();
        }
    }

    event->accept();
    qApp->quit();
}


void mudlet::readSettings()
{
    QSettings settings("Mudlet", "Mudlet 1.0");
    QPoint pos = settings.value("pos", QPoint(0, 0)).toPoint();
    QSize size = settings.value("size", QSize(750, 550)).toSize();
    mTEFolderIconSize = settings.value("tefoldericonsize", QVariant(3)).toInt();
    resize( size );
    move( pos );

    if( settings.value("maximized", false).toBool() )
    {
        showMaximized();
    }
}


void mudlet::writeSettings()
{
    QSettings settings("Mudlet", "Mudlet 1.0");
    settings.setValue("pos", pos());
    settings.setValue("size", size());
    settings.setValue("tefoldericonsize",mTEFolderIconSize);
    settings.setValue("maximized", isMaximized());
}

void mudlet::connectToServer()
{
    dlgConnectionProfiles * pDlg = new dlgConnectionProfiles(this);
    connect (pDlg, SIGNAL (signal_establish_connection( QString, int )), this, SLOT (slot_connection_dlg_finnished(QString, int)));
    pDlg->fillout_form();
    pDlg->exec();
}

void mudlet::slot_reconnect()
{
    Host * pHost = getActiveHost();
    if( ! pHost ) return;
    pHost->mTelnet.connectIt( pHost->getUrl(), pHost->getPort() );
}

void mudlet::slot_disconnect()
{
    Host * pHost = getActiveHost();
    if( ! pHost ) return;
    pHost->mTelnet.disconnect();
}

void mudlet::printSystemMessage( Host * pH, const QString & s )
{
    mConsoleMap[pH]->printSystemMessage( s );
}

void mudlet::print( Host * pH, const QString & s )
{
    mConsoleMap[pH]->print( s );
}


QString mudlet::readProfileData( const QString& profile, const QString& item )
{
    QFile file( QDir::homePath()+"/.config/mudlet/profiles/"+profile+"/"+item );
    file.open( QIODevice::ReadOnly );
    if( ! file.exists() ) return "";

    QDataStream ifs( & file );
    QString ret;

    ifs >> ret;
    file.close();
    return ret;
}

// this slot is called via a timer in the constructor of mudlet::mudlet()
void mudlet::startAutoLogin()
{
    QStringList hostList = QDir(QDir::homePath()+"/.config/mudlet/profiles").entryList(QDir::Dirs, QDir::Name);
    hostList.removeAll(".");
    hostList.removeAll("..");
    for( int i = 0; i< hostList.size(); i++ )
    {
        QString item = "autologin";
        QString val = readProfileData( hostList[i], item );
        if( val.toInt() == Qt::Checked )
        {
            doAutoLogin( hostList[i] );
        }
    }

}

void mudlet::doAutoLogin( const QString & profile_name )
{
    if( profile_name.size() < 1 )
        return;

    Host * pOH = getHostManager()->getHost( profile_name );
    if( pOH )
    {
        pOH->mTelnet.connectIt( pOH->getUrl(), pOH->getPort() );
        return;
    }
    // load an old profile if there is any
    getHostManager()->addHost( profile_name, "", "", "" );
    Host * pHost = getHostManager()->getHost( profile_name );

    if( ! pHost ) return;

    QString folder = QDir::homePath()+"/.config/mudlet/profiles/"+profile_name+"/current/";
    QDir dir( folder );
    dir.setSorting(QDir::Time);
    QStringList entries = dir.entryList( QDir::Files, QDir::Time );
    if( entries.size() > 0 )
    {
        QFile file(folder+"/"+entries[0]);
        file.open(QFile::ReadOnly | QFile::Text);

    }

    QString login = "login";
    QString val1 = readProfileData( profile_name, login );
    pHost->setLogin( val1 );
    QString pass = "password";
    QString val2 = readProfileData( profile_name, pass );
    pHost->setPass( val2 );
    slot_connection_dlg_finnished( profile_name, 0 );
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



void mudlet::processEventLoopHack()
{
    QTimer::singleShot(1, this, SLOT(processEventLoopHack_timerRun()));
}

void mudlet::processEventLoopHack_timerRun()
{
    Host * pH = getActiveHost();
    if( !pH ) return;
    pH->mpConsole->refresh();
}

void mudlet::slot_connection_dlg_finnished( const QString& profile, int historyVersion )
{
    Host * pHost = getHostManager()->getHost( profile );
    if( ! pHost ) return;
    pHost->mIsProfileLoadingSequence = true;
    addConsoleForNewHost( pHost );
    pHost->mBlockScriptCompile = false;
    pHost->mIsProfileLoadingSequence = false;

    //do modules here
    QMapIterator<QString, int> it (pHost->mModulePriorities);
    QMap<int, QStringList> moduleOrder;
    while( it.hasNext() ){
        it.next();
        QStringList moduleEntry = moduleOrder[it.value()];
        moduleEntry << it.key();
        moduleOrder[it.value()] = moduleEntry;
    }
    QMapIterator<int, QStringList> it2 (moduleOrder);
    while (it2.hasNext()){
        it2.next();
    }


    TEvent event;
    event.mArgumentList.append( "sysLoadEvent" );
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);

    // TODO Add Event

    //NOTE: this is a potential problem if users connect by hand quickly
    //      and one host has a slower response time as the other one, but
    //      the worst that can happen is that they have to login manually.

    tempHostQueue.enqueue( pHost );
    tempHostQueue.enqueue( pHost );
    pHost->connectToServer();
}

void mudlet::slot_multi_view()
{
     QMapIterator<Host *, TConsole *> it( mConsoleMap );
     while( it.hasNext() )
     {
         it.next();
         it.value()->show();
     }
}

mudlet::~mudlet()
{
    mudlet::_self = 0;
}

void mudlet::toggleFullScreenView()
{
    if( isFullScreen() )
        showNormal();
    else
        showFullScreen();
}

void mudlet::stopSounds()
{
    mpMusicBox1->stop();
    mpMusicBox2->stop();
    mpMusicBox3->stop();
    mpMusicBox4->stop();
}

void mudlet::playSound( QString s )
{
    if( mpMusicBox1->state() != QMediaPlayer::PlayingState )
    {
        mpMusicBox1->setMedia( QUrl::fromLocalFile( s ) );
        mpMusicBox1->play();
    }
    else if( mpMusicBox2->state() != QMediaPlayer::PlayingState )
    {
        mpMusicBox2->setMedia( QUrl::fromLocalFile( s ) );
        mpMusicBox2->play();
    }
    else if( mpMusicBox3->state() != QMediaPlayer::PlayingState )
    {
        mpMusicBox3->setMedia( QUrl::fromLocalFile( s ) );
        mpMusicBox3->play();
    }
    else
    {
        mpMusicBox4->stop();
        mpMusicBox4->setMedia( QUrl::fromLocalFile( s ) );
        mpMusicBox4->play();
    }
}
