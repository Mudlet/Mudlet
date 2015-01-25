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
#include "MainWindow.h"


#include "Telnet.h"
#include "Profile.h"
#include "Profiles.h"
#include "CommandLine.h"
#include "Console.h"
#include "TextEdit.h"

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
QString MainWindow::CONFIG_DIR;

QPointer<MainWindow> MainWindow::_self;

void MainWindow::start()
{
    CONFIG_DIR = QDir::homePath()+"/.config/mudlet";
    QDir dir;
    if( ! dir.exists( CONFIG_DIR ) )
    {
        dir.mkpath( CONFIG_DIR );
    }

    _self = new MainWindow;

}

MainWindow * MainWindow::self()
{
    return _self;
}


MainWindow::MainWindow()
: QMainWindow()
, mWindowMinimized( false )
, version( QString("Mudlet ") + QString(APP_VERSION) + QString(APP_BUILD) )
, activeHost( 0 )
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
    tabBar = new QTabBar( frame );
    tabBar->setMaximumHeight(30);
    tabBar->setFocusPolicy( Qt::NoFocus );

    tabBar->setTabsClosable ( true );
    connect( tabBar, SIGNAL(tabCloseRequested(int)), this, SLOT(slot_close_profile_requested(int)));
    tabBar->setMovable(true);

    connect( tabBar, SIGNAL(currentChanged(int)), this, SLOT(slot_tab_changed(int)));
    QVBoxLayout * layoutTopLevel = new QVBoxLayout(frame);
    layoutTopLevel->setContentsMargins(0,0,0,0);
    layoutTopLevel->setContentsMargins(0,0,0,0);
    layoutTopLevel->addWidget( tabBar );
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
    tabBar->setFont( mdiFont );

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

Profiles * MainWindow::getHostManager()
{
    return &mHostManager;
}

bool MainWindow::openWebPage(const QString& path){
    if (path.isEmpty() || path.isNull())
        return false;
    QUrl url(path,QUrl::TolerantMode);
    if (!url.isValid())
        return false;
    return QDesktopServices::openUrl(url);
}


void MainWindow::slot_close_profile_requested( int tab )
{
    QString name = tabBar->tabText( tab );
    Profile * pH = getHostManager()->getHost( name );
    if( ! pH ) return;

    if( ! pH->console->close() )
        return;
    else
        pH->console->mUserAgreedToCloseConsole = true;
    mConsoleMap[pH]->close();
    if( mTabMap.contains( pH->getId() ) )
    {
        tabBar->removeTab( tab );
        mConsoleMap.remove( pH );
        mTabMap.remove( pH->getId() );
        getHostManager()->deleteHost( pH->getId() );
    }

    // hide the tab bar if we only have 1 or no tabs available. saves screen space.
    if( mConsoleMap.size() > 1 )
    {
        tabBar->show();
    }
    else
        tabBar->hide();

}

void MainWindow::slot_close_profile()
{
    if( activeHost )
    {
        if( mConsoleMap.contains( activeHost ) )
        {
            QString name = activeHost->getId();
            Profile * pH = activeHost;
            mConsoleMap[activeHost]->close();
            if( mTabMap.contains( pH->getId() ) )
            {
                tabBar->removeTab( tabBar->currentIndex() );
                mConsoleMap.remove( pH );
                getHostManager()->deleteHost( pH->getId() );
                mTabMap.remove( pH->getId() );
            }

        }
    }
}

void MainWindow::slot_tab_changed( int tabID )
{
    if( ( ! mTabMap.contains( tabBar->tabText( tabID ) ) ) && ( tabID != -1 ) )
    {
        activeHost = 0;
        return;
    }

    if( mConsoleMap.contains( activeHost ) )
    {
        activeHost->console->hide();
        QString host = tabBar->tabText( tabID );
        if( mTabMap.contains( host ) )
        {
            activeHost = mTabMap[host]->mpHost;
        }
        else
        {
            activeHost = 0;
            return;
        }
    }
    else
    {
        if( mTabMap.size() > 0 )
        {
            activeHost = mTabMap.begin().value()->mpHost;
        }
        else
        {
            activeHost = 0;
            return;
        }
    }

    if( ! activeHost || mConsoleMap.contains( activeHost ) )
    {
        if( ! activeHost ) return;
        activeHost->console->show();
        activeHost->console->repaint();
        activeHost->console->refresh();
        activeHost->console->cmdLine->repaint();
        activeHost->console->cmdLine->setFocus();
        activeHost->console->show();

        int x = activeHost->console->width();
        int y = activeHost->console->height();
        QSize s = QSize(x,y);
        QResizeEvent event(s, s);
        QApplication::sendEvent( activeHost->console, &event);
    }
    else
    {
        activeHost = 0;
        return;
    }

    // update the window title for the currently selected profile
    setWindowTitle(activeHost->getId() + " - " + version);
}

void MainWindow::addConsoleForNewHost( Profile * pH )
{
    if( mConsoleMap.contains( pH ) ) return;
    Console * pConsole = new Console( pH, false );
    if( ! pConsole ) return;
    pH->console = pConsole;
    pConsole->setWindowTitle( pH->getId() );
    pConsole->setObjectName( pH->getId() );
    mConsoleMap[pH] = pConsole;
    int newTabID = tabBar->addTab( pH->getId() );
    mTabMap[pH->getId()] = pConsole;
    if( mConsoleMap.size() > 1 )
    {
        tabBar->show();
    }
    else
        tabBar->hide();
    //update the main window title when we spawn a new tab
    setWindowTitle(pH->getId() + " - " + version);

    mainPane->layout()->addWidget( pConsole );
    if( activeHost )
        activeHost->console->hide();
    activeHost = pH;

    pConsole->show();

    QMap<QString, Console *> miniConsoleMap;
    mHostConsoleMap[activeHost] = miniConsoleMap;
    QMap<QString, TLabel *> labelMap;
    mHostLabelMap[activeHost] = labelMap;
    activeHost->console->show();
    activeHost->console->repaint();
    activeHost->console->refresh();
    activeHost->console->cmdLine->repaint();
    activeHost->console->cmdLine->setFocus();
    activeHost->console->show();
    tabBar->setCurrentIndex( newTabID );
    int x = activeHost->console->width();
    int y = activeHost->console->height();
    QSize s = QSize(x,y);
    QResizeEvent event(s, s);
    QApplication::sendEvent( activeHost->console, &event);
}

bool MainWindow::openWindow( Profile * host, const QString & name )
{
    if( ! dockWindowMap.contains( name ) )
    {
        QDockWidget * pD = new QDockWidget;
        pD->setContentsMargins(0,0,0,0);
        pD->setFeatures( QDockWidget::AllDockWidgetFeatures );
        pD->setWindowTitle( name );
        dockWindowMap[name] = pD;
        Console * pC = new Console( host, false );
        pC->setContentsMargins(0,0,0,0);
        pD->setWidget( pC );
        pC->show();
        pC->layerCommandLine->hide();
        pC->mpScrollBar->hide();
        pC->setUserWindow();
        QMap<QString, Console *> & dockWindowConsoleMap = mHostConsoleMap[host];
        dockWindowConsoleMap[name] = pC;
        addDockWidget(Qt::RightDockWidgetArea, pD);
        return true;
    }
    else return false;
}

bool MainWindow::createMiniConsole( Profile * pHost, const QString & name, int x, int y, int width, int height )
{
    if( ! pHost ) return false;
    if( ! pHost->console ) return false;

    QMap<QString, Console *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( ! dockWindowConsoleMap.contains( name ) )
    {
        Console * pC = pHost->console->createMiniConsole( name, x, y, width, height );
        pC->name = name;
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
        Console * pC = dockWindowConsoleMap[name];
        pC->resize( width, height );
        pC->move( x, y );
    }
    return false;
}

bool MainWindow::createBuffer( Profile * pHost, const QString & name )
{
    if( ! pHost ) return false;
    if( ! pHost->console ) return false;

    QMap<QString, Console *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( ! dockWindowConsoleMap.contains( name ) )
    {
        Console * pC = pHost->console->createBuffer( name );
        pC->name = name;
        if( pC )
        {
            dockWindowConsoleMap[name] = pC;
            return true;
        }
    }
    return false;
}

bool MainWindow::setBackgroundColor( Profile * pHost, const QString & name, int r, int g, int b, int alpha )
{
    QMap<QString, Console *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->setConsoleBgColor(r,g,b);
        return true;
    }
    return false;
}


bool MainWindow::setTextFormat( Profile * pHost, const QString & name, int r1, int g1, int b1, int r2, int g2, int b2, bool bold, bool underline, bool italics, bool strikeout )
{
    QMap<QString, Console *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        Console * pC = dockWindowConsoleMap[name];
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

void MainWindow::showEvent( QShowEvent * event )
{
    mWindowMinimized = false;
    QMainWindow::showEvent( event );
}

void MainWindow::hideEvent( QHideEvent * event )
{
    mWindowMinimized = true;
    QMainWindow::hideEvent( event );
}

bool MainWindow::clearWindow( Profile * pHost, const QString & name )
{
    QMap<QString, Console *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->buffer.clear();
        dockWindowConsoleMap[name]->console->update();
        return true;
    }
    else
        return false;
}

bool MainWindow::showWindow( Profile * pHost, const QString & name )
{

    QMap<QString, Console *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->console->show();
        dockWindowConsoleMap[name]->console->forceUpdate();
        return true;
    }

    return false;
}

bool MainWindow::paste( Profile * pHost, const QString & name )
{
    QMap<QString, Console *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->paste();
        return true;
    }
    else
        return false;
}

bool MainWindow::hideWindow( Profile * pHost, const QString & name )
{

    QMap<QString, Console *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->console->hide();
        dockWindowConsoleMap[name]->console->forceUpdate();
        return true;
    }

    return false;
}

bool MainWindow::resizeWindow( Profile * pHost, const QString & name, int x1, int y1 )
{

    QMap<QString, Console *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->resize( x1, y1 );
        return true;
    }

    return false;
}

bool MainWindow::setConsoleBufferSize( Profile * pHost, const QString & name, int x1, int y1 )
{
    if( name == "main" )
    {
        pHost->console->buffer.setBufferSize( x1, y1 );
        return true;
    }

    QMap<QString, Console *> & dockWindowConsoleMap = mHostConsoleMap[pHost];

    if( dockWindowConsoleMap.contains( name ) )
    {
        (dockWindowConsoleMap[name]->buffer).setBufferSize( x1, y1 );
        return true;
    }
    else
        return false;
}



bool MainWindow::resetFormat( Profile * pHost, QString & name )
{
    QMap<QString, Console *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->reset();
        return true;
    }
    else
        return false;
}

bool MainWindow::moveWindow( Profile * pHost, const QString & name, int x1, int y1 )
{

    QMap<QString, Console *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->move( x1, y1 );
        dockWindowConsoleMap[name]->mOldX = x1;
        dockWindowConsoleMap[name]->mOldY = y1;
        return true;
    }

    return false;
}

bool MainWindow::closeWindow( Profile * pHost, const QString & name )
{
    QMap<QString, Console *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->console->close();
        return true;
    }
    else
        return false;
}

int MainWindow::getLineNumber( Profile * pHost, QString & name )
{
    QMap<QString, Console *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        return dockWindowConsoleMap[name]->getLineNumber();
    }
    return -1;
}

int MainWindow::getColumnNumber( Profile * pHost, QString & name )
{
    QMap<QString, Console *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        return dockWindowConsoleMap[name]->getColumnNumber();
    }
    return -1;
}


int MainWindow::getLastLineNumber( Profile * pHost, const QString & name )
{
    QMap<QString, Console *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        return dockWindowConsoleMap[name]->getLastLineNumber();
    }
    return -1;
}

bool MainWindow::moveCursorEnd( Profile * pHost, const QString & name )
{
    QMap<QString, Console *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->moveCursorEnd();
        return true;
    }
    else
        return false;
}

bool MainWindow::moveCursor( Profile * pHost, const QString & name, int x, int y )
{
    QMap<QString, Console *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        return dockWindowConsoleMap[name]->moveCursor( x, y );
    }
    else
        return false;
}

void MainWindow::deleteLine( Profile * pHost, const QString & name )
{
    QMap<QString, Console *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->skipLine();
    }
}

void MainWindow::insertText( Profile * pHost, const QString & name, const QString& text )
{
    QMap<QString, Console *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->insertText( text );
    }
}

void MainWindow::insertLink( Profile * pHost, const QString & name, const QString& text, QStringList & func, QStringList & hint, bool customFormat )
{
    QMap<QString, Console *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->insertLink( text, func, hint, customFormat );
    }
}

void MainWindow::replace( Profile * pHost, const QString & name, const QString& text )
{
    QMap<QString, Console *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->replace( text );
    }
}

void MainWindow::setLink( Profile * pHost, const QString & name, const QString & linkText, QStringList & linkFunction, QStringList & linkHint )
{
    QMap<QString, Console *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        Console * pC = dockWindowConsoleMap[name];
        pC->setLink( linkText, linkFunction, linkHint );
    }
}

void MainWindow::setBold( Profile * pHost, const QString & name, bool b )
{
    QMap<QString, Console *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        Console * pC = dockWindowConsoleMap[name];
        pC->setBold( b );
    }
}

void MainWindow::setItalics( Profile * pHost, const QString & name, bool b )
{
    QMap<QString, Console *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        Console * pC = dockWindowConsoleMap[name];
        pC->setItalics( b );
    }
}

void MainWindow::setUnderline( Profile * pHost, const QString & name, bool b )
{
    QMap<QString, Console *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        Console * pC = dockWindowConsoleMap[name];
        pC->setUnderline( b );
    }
}

void MainWindow::setStrikeOut( Profile * pHost, const QString & name, bool b )
{
    QMap<QString, Console *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        Console * pC = dockWindowConsoleMap[name];
        pC->setStrikeOut( b );
    }
}

void MainWindow::setFgColor( Profile * pHost, const QString & name, int r, int g, int b )
{
    QMap<QString, Console *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        Console * pC = dockWindowConsoleMap[name];
        pC->setFgColor( r, g, b );
    }
}

void MainWindow::setBgColor( Profile * pHost, const QString & name, int r, int g, int b )
{
    QMap<QString, Console *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        Console * pC = dockWindowConsoleMap[name];
        pC->setBgColor( r, g, b );
    }
}

int MainWindow::selectString( Profile * pHost, const QString & name, const QString& text, int num )
{
    QMap<QString, Console *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        Console * pC = dockWindowConsoleMap[name];
        return pC->select( text, num );
    }
    else
        return -1;
}

int MainWindow::selectSection( Profile * pHost, const QString & name, int f, int t )
{
    QMap<QString, Console *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        Console * pC = dockWindowConsoleMap[name];
        return pC->selectSection( f, t );
    }
    else
        return -1;
}

void MainWindow::deselect( Profile * pHost, const QString & name )
{
    QMap<QString, Console *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        Console * pC = dockWindowConsoleMap[name];
        pC->deselect();
    }
}

bool MainWindow::setWindowWrap( Profile * pHost, const QString & name, int & wrap )
{
    QMap<QString, Console *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    QString wn = name;
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[wn]->setWrapAt( wrap );
        return true;
    }
    else
        return false;
}

bool MainWindow::setWindowWrapIndent( Profile * pHost, const QString & name, int & wrap )
{
    QMap<QString, Console *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    QString wn = name;
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[wn]->setIndentCount( wrap );
        return true;
    }
    else
        return false;
}

bool MainWindow::echoWindow( Profile * pHost, const QString & name, const QString & text )
{
    QMap<QString, Console *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    QString t = text;
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->echoUserWindow( t );
        return true;
    }
    return false;
}

bool MainWindow::echoLink( Profile * pHost, const QString & name, const QString & text, QStringList & func, QStringList & hint, bool customFormat )
{
    QMap<QString, Console *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    QString t = text;
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->echoLink( text, func, hint, customFormat );
        return true;
    }
    else
        return false;
}

bool MainWindow::copy( Profile * pHost, const QString & name )
{
    QMap<QString, Console *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->copy();
        return true;
    }
    else return false;
}

bool MainWindow::pasteWindow( Profile * pHost, const QString & name )
{
    QMap<QString, Console *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->pasteWindow( mConsoleMap[pHost]->mClipboard );
        return true;
    }
    else return false;
}

bool MainWindow::appendBuffer( Profile * pHost, const QString & name )
{
    QMap<QString, Console *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->appendBuffer( mConsoleMap[pHost]->mClipboard );
        return true;
    }
    else return false;
}

Profile * MainWindow::getActiveHost()
{
    if( mConsoleMap.contains( activeHost ) )
    {
        return activeHost;
    }
    else
        return 0;
}



void MainWindow::addSubWindow( Console* pConsole )
{
    mainPane->layout()->addWidget( pConsole );
    pConsole->show();//NOTE: this is important for Apple OSX otherwise the console isnt displayed
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    foreach( Console * pC, mConsoleMap )
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
    foreach( Console * pC, mConsoleMap )
    {
        if( pC->mpHost->getId() != "default_host" )
        {
            pC->close();
        }
    }

    event->accept();
    qApp->quit();
}


void MainWindow::readSettings()
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


void MainWindow::writeSettings()
{
    QSettings settings("Mudlet", "Mudlet 1.0");
    settings.setValue("pos", pos());
    settings.setValue("size", size());
    settings.setValue("tefoldericonsize",mTEFolderIconSize);
    settings.setValue("maximized", isMaximized());
}

void MainWindow::printSystemMessage( Profile * pH, const QString & s )
{
    mConsoleMap[pH]->printSystemMessage( s );
}

void MainWindow::print( Profile * pH, const QString & s )
{
    mConsoleMap[pH]->print( s );
}


QString MainWindow::readProfileData( const QString& profile, const QString& item )
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

void MainWindow::processEventLoopHack()
{
    QTimer::singleShot(1, this, SLOT(processEventLoopHack_timerRun()));
}

void MainWindow::processEventLoopHack_timerRun()
{
    Profile * pH = getActiveHost();
    if( !pH ) return;
    pH->console->refresh();
}

void MainWindow::slot_multi_view()
{
     QMapIterator<Profile *, Console *> it( mConsoleMap );
     while( it.hasNext() )
     {
         it.next();
         it.value()->show();
     }
}

MainWindow::~MainWindow()
{
    MainWindow::_self = 0;
}

void MainWindow::toggleFullScreenView()
{
    if( isFullScreen() )
        showNormal();
    else
        showFullScreen();
}

void MainWindow::stopSounds()
{
    mpMusicBox1->stop();
    mpMusicBox2->stop();
    mpMusicBox3->stop();
    mpMusicBox4->stop();
}

void MainWindow::playSound( QString s )
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
