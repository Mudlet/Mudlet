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
#include <QMenu>
#include "mudlet.h"


#include "ctelnet.h"
#include "Host.h"
#include "HostManager.h"
#include "TCommandLine.h"
#include "TConsole.h"
#include "TEvent.h"
#include "TLabel.h"
#include "TTextEdit.h"

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
    qDebug() << "new mudlet";

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

    mpTabBar->setTabsClosable ( true );
    connect( mpTabBar, SIGNAL(tabCloseRequested(int)), this, SLOT(slot_close_profile_requested(int)));
    mpTabBar->setMovable(true);

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

    mHostManager.addHost("default_host", "", "","" );
    mpDefaultHost = mHostManager.getHost(QString("default_host"));


    QFont mainFont;
    mainFont = QFont("Bitstream Vera Sans Mono", 8, QFont::Courier);
    QFont mdiFont = QFont("Bitstream Vera Sans Mono", 6, QFont::Courier);
    setFont( mainFont );
    mainPane->setFont( mainFont );
    mpTabBar->setFont( mdiFont );

    mainPane->show();

    QAction * mactionMultiView = new QAction(tr("MultiView"), this);
    QAction * mactionCloseProfile = new QAction(tr("Close"), this);


    connect(mactionMultiView, SIGNAL(triggered()), this, SLOT(slot_multi_view()));
    connect(mactionCloseProfile, SIGNAL(triggered()), this, SLOT(slot_close_profile()));

    readSettings();

    mpMusicBox1 = new QMediaPlayer(this);
    mpMusicBox2 = new QMediaPlayer(this);
    mpMusicBox3 = new QMediaPlayer(this);
    mpMusicBox4 = new QMediaPlayer(this);

    addConsoleForNewHost(mpDefaultHost);
}

HostManager * mudlet::getHostManager()
{
    return &mHostManager;
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
    if( mTabMap.contains( pH->getId() ) )
    {
        mpTabBar->removeTab( tab );
        mConsoleMap.remove( pH );
        mTabMap.remove( pH->getId() );
        getHostManager()->deleteHost( pH->getId() );
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
            QString name = mpCurrentActiveHost->getId();
            Host * pH = mpCurrentActiveHost;
            mConsoleMap[mpCurrentActiveHost]->close();
            if( mTabMap.contains( pH->getId() ) )
            {
                mpTabBar->removeTab( mpTabBar->currentIndex() );
                mConsoleMap.remove( pH );
                getHostManager()->deleteHost( pH->getId() );
                mTabMap.remove( pH->getId() );
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
    setWindowTitle(mpCurrentActiveHost->getId() + " - " + version);
}

void mudlet::addConsoleForNewHost( Host * pH )
{
    if( mConsoleMap.contains( pH ) ) return;
    TConsole * pConsole = new TConsole( pH, false );
    if( ! pConsole ) return;
    pH->mpConsole = pConsole;
    pConsole->setWindowTitle( pH->getId() );
    pConsole->setObjectName( pH->getId() );
    mConsoleMap[pH] = pConsole;
    int newTabID = mpTabBar->addTab( pH->getId() );
    mTabMap[pH->getId()] = pConsole;
    if( mConsoleMap.size() > 1 )
    {
        mpTabBar->show();
    }
    else
        mpTabBar->hide();
    //update the main window title when we spawn a new tab
    setWindowTitle(pH->getId() + " - " + version);

    mainPane->layout()->addWidget( pConsole );
    if( mpCurrentActiveHost )
        mpCurrentActiveHost->mpConsole->hide();
    mpCurrentActiveHost = pH;

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
        if( pC->mpHost->getId() != "default_host" )
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
