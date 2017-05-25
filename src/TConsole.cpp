/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014-2016 by Stephen Lyons - slysven@virginmedia.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016 by Ian Adkins - ieadkins@gmail.com                 *
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


#include "TConsole.h"


#include "Host.h"
#include "TCommandLine.h"
#include "TDebug.h"
#include "TEvent.h"
#include "TLabel.h"
#include "TMap.h"
#include "TRoomDB.h"
#include "TSplitter.h"
#include "TTextEdit.h"
#include "XMLexport.h"
#include "dlgMapper.h"
#include "mudlet.h"

#include "pre_guard.h"
#include <QDateTime>
#include <QDir>
#include <QLineEdit>
#include <QMessageBox>
#include <QScrollBar>
#include <QShortcut>
#include <QToolButton>
#include <QVBoxLayout>
#include "post_guard.h"

#include <assert.h>


using namespace std;

const QString TConsole::cmLuaLineVariable("line");

TConsole::TConsole( Host * pH, bool isDebugConsole, QWidget * parent )
: QWidget( parent )
, mpHost( pH )
, buffer( pH )
, emergencyStop( new QToolButton )
, layerCommandLine( 0 )
, mBgColor( QColor(Qt::black) )
, mClipboard( mpHost )
, mCommandBgColor( Qt::black )
, mCommandFgColor( QColor( 213, 195, 0 ) )
, mConsoleName( "main" )
, mDisplayFont( QFont("Bitstream Vera Sans Mono", 10, QFont::Normal ) )//mDisplayFont( QFont("Monospace", 10, QFont::Courier ) )
, mFgColor( Qt::black )
, mIndentCount( 0 )
, mIsDebugConsole( isDebugConsole )
, mLogFileName(QString(""))
, mLogToLogFile( false )
, mMainFrameBottomHeight( 0 )
, mMainFrameLeftWidth( 0 )
, mMainFrameRightWidth( 0 )
, mMainFrameTopHeight( 0 )
, mOldX( 0 )
, mOldY( 0 )
, mpBaseVFrame( new QWidget( this ) )
, mpTopToolBar( new QWidget( mpBaseVFrame ) )
, mpBaseHFrame( new QWidget( mpBaseVFrame ) )
, mpLeftToolBar( new QWidget( mpBaseHFrame ) )
, mpMainFrame( new QWidget( mpBaseHFrame ) )
, mpRightToolBar( new QWidget( mpBaseHFrame ) )
, mpMainDisplay( new QWidget( mpMainFrame ) )
, mpMapper( 0 )
, mpScrollBar( new QScrollBar )

, mRecordReplay( false )
, mSystemMessageBgColor( mBgColor )
, mSystemMessageFgColor( QColor(Qt::red) )
, mTriggerEngineMode( false )
, mUserConsole( false )
, mWindowIsHidden( false )
, mWrapAt( 100 )
, networkLatency( new QLineEdit )
, mUserAgreedToCloseConsole( false )
, mpBufferSearchBox( new QLineEdit )
, mpBufferSearchUp( new QToolButton )
, mpBufferSearchDown( new QToolButton )
, mCurrentSearchResult( 0 )
, mSearchQuery("")
{
    auto ps = new QShortcut(this);
    ps->setKey(Qt::CTRL + Qt::Key_W);
    ps->setContext(Qt::WidgetShortcut);

    if( mIsDebugConsole )
    {
        setWindowTitle( tr( "Debug Console" ) );
        // Probably will not show up as this is used inside a QMainWindow widget
        // which has it's own title and icon set.
        mWrapAt = 50;
        mIsSubConsole = false;
        mStandardFormat.bgR = mBgColor.red();
        mStandardFormat.bgG = mBgColor.green();
        mStandardFormat.bgB = mBgColor.blue();
        mStandardFormat.fgR = mFgColor.red();
        mStandardFormat.fgG = mFgColor.green();
        mStandardFormat.fgB = mFgColor.blue();
        mStandardFormat.flags &= ~(TCHAR_BOLD);
        mStandardFormat.flags &= ~(TCHAR_ITALICS);
        mStandardFormat.flags &= ~(TCHAR_UNDERLINE);
        mStandardFormat.flags &= ~(TCHAR_STRIKEOUT);
    }
    else
    {
        setWindowTitle( tr( "Non Debug Console" ) );
        if( parent )
        {
            mIsSubConsole = true;
            mpHost->mpConsole->mSubConsoleList.append( this );
            mMainFrameTopHeight = 0;
            mMainFrameBottomHeight = 0;
            mMainFrameLeftWidth = 0;
            mMainFrameRightWidth = 0;
        }
        else
        {
            mIsSubConsole = false;
            mMainFrameTopHeight = mpHost->mBorderTopHeight;
            mMainFrameBottomHeight = mpHost->mBorderBottomHeight;
            mMainFrameLeftWidth = mpHost->mBorderLeftWidth;
            mMainFrameRightWidth = mpHost->mBorderRightWidth;
            mCommandBgColor = mpHost->mCommandBgColor;
            mCommandFgColor = mpHost->mCommandFgColor;
        }
        mStandardFormat.bgR = mpHost->mBgColor.red();
        mStandardFormat.bgG = mpHost->mBgColor.green();
        mStandardFormat.bgB = mpHost->mBgColor.blue();
        mStandardFormat.fgR = mpHost->mFgColor.red();
        mStandardFormat.fgG = mpHost->mFgColor.green();
        mStandardFormat.fgB = mpHost->mFgColor.blue();
        mStandardFormat.flags &= ~(TCHAR_BOLD);
        mStandardFormat.flags &= ~(TCHAR_ITALICS);
        mStandardFormat.flags &= ~(TCHAR_UNDERLINE);
        mStandardFormat.flags &= ~(TCHAR_STRIKEOUT);
    }
    setContentsMargins(0,0,0,0);
    if( mpHost )
        profile_name = mpHost->getName();
    else
        profile_name = "debug console";
    mFormatSystemMessage.bgR = mBgColor.red();
    mFormatSystemMessage.bgG = mBgColor.green();
    mFormatSystemMessage.bgB = mBgColor.blue();
    mFormatSystemMessage.fgR = 255;
    mFormatSystemMessage.fgG = 0;
    mFormatSystemMessage.fgB = 0;
    setAttribute( Qt::WA_DeleteOnClose );
    setAttribute( Qt::WA_OpaquePaintEvent );//was disabled
    mWaitingForHighColorCode = false;
    mHighColorModeForeground = false;
    mHighColorModeBackground = false;
    mIsHighColorMode = false;

    QSizePolicy sizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding);
    QSizePolicy sizePolicy3( QSizePolicy::Expanding, QSizePolicy::Expanding);
    QSizePolicy sizePolicy2( QSizePolicy::Expanding, QSizePolicy::Fixed);
    QSizePolicy sizePolicy4( QSizePolicy::Fixed, QSizePolicy::Expanding);
    QSizePolicy sizePolicy5( QSizePolicy::Fixed, QSizePolicy::Fixed);
    QPalette mainPalette;
    mainPalette.setColor( QPalette::Text, QColor(Qt::black) );
    mainPalette.setColor( QPalette::Highlight, QColor(55,55,255) );
    mainPalette.setColor( QPalette::Window, QColor(0,0,0,255) );
    QPalette splitterPalette;
    splitterPalette = mainPalette;
    splitterPalette.setColor( QPalette::Button, QColor(0,0,255,255) );
    splitterPalette.setColor( QPalette::Window, QColor(Qt::green));//,255) );
    splitterPalette.setColor( QPalette::Base, QColor(255,0,0,255) );
    splitterPalette.setColor( QPalette::Window, QColor(Qt::white) );
    //setPalette( mainPalette );

    //QVBoxLayout * layoutFrame = new QVBoxLayout( mainFrame );
    QPalette framePalette;
    framePalette.setColor( QPalette::Text, QColor(Qt::black) );
    framePalette.setColor( QPalette::Highlight, QColor(55,55,255) );
    framePalette.setColor( QPalette::Window, QColor(0,0,0,255) );
    mpMainFrame->setPalette( framePalette );
    mpMainFrame->setAutoFillBackground(true);
    mpMainFrame->setContentsMargins(0,0,0,0);
    auto centralLayout = new QVBoxLayout;
    setLayout( centralLayout );
    auto baseVFrameLayout = new QVBoxLayout;
    mpBaseVFrame->setLayout( baseVFrameLayout );
    baseVFrameLayout->setMargin( 0 );
    baseVFrameLayout->setSpacing( 0 );
    centralLayout->addWidget( mpBaseVFrame );
    auto baseHFrameLayout = new QHBoxLayout;
    mpBaseHFrame->setLayout( baseHFrameLayout );
    baseHFrameLayout->setMargin( 0 );
    baseHFrameLayout->setSpacing( 0 );
    layout()->setSpacing( 0 );
    layout()->setMargin( 0 );
    setContentsMargins( 0, 0, 0, 0 );

    auto topBarLayout = new QHBoxLayout;
    mpTopToolBar->setLayout( topBarLayout );
    mpTopToolBar->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
    mpTopToolBar->setContentsMargins(0,0,0,0);
    mpTopToolBar->setAutoFillBackground(true);
    QPalette topbarPalette;
    topbarPalette.setColor( QPalette::Text, QColor(Qt::white) );
    topbarPalette.setColor( QPalette::Highlight, QColor(55,55,255) );
    topbarPalette.setColor( QPalette::Window, QColor(0,255,0,255) );
    topbarPalette.setColor( QPalette::Base, QColor(0,255,0,255) );
    //mpTopToolBar->setPalette(topbarPalette);


    topBarLayout->setMargin( 0 );
    topBarLayout->setSpacing(0);
    auto leftBarLayout = new QVBoxLayout;
    mpLeftToolBar->setLayout( leftBarLayout );
    mpLeftToolBar->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding));
    mpLeftToolBar->setAutoFillBackground(true);
    leftBarLayout->setMargin( 0 );
    leftBarLayout->setSpacing(0);
    mpLeftToolBar->setContentsMargins(0,0,0,0);
    auto rightBarLayout = new QVBoxLayout;
    mpRightToolBar->setLayout( rightBarLayout );
    mpRightToolBar->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding));
    mpRightToolBar->setAutoFillBackground(true);
    rightBarLayout->setMargin( 0 );
    rightBarLayout->setSpacing(0);
    mpRightToolBar->setContentsMargins(0,0,0,0);
    mpBaseVFrame->setContentsMargins(0,0,0,0);
    baseVFrameLayout->setSpacing(0);
    baseVFrameLayout->setMargin( 0 );
    mpTopToolBar->setContentsMargins(0,0,0,0);
    baseVFrameLayout->addWidget( mpTopToolBar );
    baseVFrameLayout->addWidget( mpBaseHFrame );
    baseHFrameLayout->addWidget( mpLeftToolBar );
    auto mpCorePane = new QWidget( mpBaseHFrame );
    auto coreSpreadLayout = new QVBoxLayout;
    mpCorePane->setLayout( coreSpreadLayout );
    mpCorePane->setContentsMargins(0,0,0,0);
    coreSpreadLayout->setMargin(0);
    coreSpreadLayout->setSpacing(0);
    coreSpreadLayout->addWidget( mpMainFrame );
    mpCorePane->setSizePolicy( sizePolicy );
    baseHFrameLayout->addWidget( mpCorePane );
    baseHFrameLayout->addWidget( mpRightToolBar );
    mpTopToolBar->setContentsMargins(0,0,0,0);
    mpBaseHFrame->setAutoFillBackground(true);
    baseHFrameLayout->setSpacing(0);
    baseHFrameLayout->setMargin(0);
    setContentsMargins(0,0,0,0);
    mpBaseHFrame->setContentsMargins(0,0,0,0);
    centralLayout->setSpacing(0);
    centralLayout->setContentsMargins(0,0,0,0);
    centralLayout->setMargin(0);
    mpMainDisplay->move( mMainFrameLeftWidth, mMainFrameTopHeight );
    mpMainFrame->show();
    mpMainDisplay->show();
    mpMainFrame->setContentsMargins(0,0,0,0);
    mpMainDisplay->setContentsMargins(0,0,0,0);
    auto layout = new QVBoxLayout;
    mpMainDisplay->setLayout(layout);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);

    mpBaseVFrame->setSizePolicy(sizePolicy);
    mpBaseHFrame->setSizePolicy(sizePolicy);
    mpBaseVFrame->setFocusPolicy(Qt::NoFocus);
    mpBaseHFrame->setFocusPolicy(Qt::NoFocus);

    baseVFrameLayout->setMargin(0);
    baseHFrameLayout->setMargin(0);
    centralLayout->setMargin(0);

    mpCommandLine = new TCommandLine( pH, this, mpMainDisplay );
    mpCommandLine->setContentsMargins(0,0,0,0);
    mpCommandLine->setSizePolicy( sizePolicy );
    mpCommandLine->setFocusPolicy( Qt::StrongFocus );

    layer = new QWidget( mpMainDisplay );
    layer->setContentsMargins(0,0,0,0);
    layer->setContentsMargins( 0, 0, 0, 0 );//neu rc1
    layer->setSizePolicy( sizePolicy );
    layer->setFocusPolicy( Qt::NoFocus );

    auto layoutLayer = new QHBoxLayout;
    layer->setLayout( layoutLayer );
    layoutLayer->setMargin( 0 );//neu rc1
    layoutLayer->setSpacing( 0 );//neu rc1
    layoutLayer->setMargin( 0 );//neu rc1

    mpScrollBar->setFixedWidth( 15 );

    splitter = new TSplitter( Qt::Vertical );
    splitter->setContentsMargins(0,0,0,0);
    splitter->setSizePolicy( sizePolicy );
    splitter->setOrientation( Qt::Vertical );
    splitter->setHandleWidth( 3 );
    splitter->setPalette( splitterPalette );
    splitter->setParent( layer );
    setFocusProxy( mpCommandLine );

    console = new TTextEdit( this, splitter, &buffer, mpHost, isDebugConsole, false );
    console->setContentsMargins(0,0,0,0);
    console->setSizePolicy( sizePolicy3 );
    console->setFocusPolicy( Qt::NoFocus );

    console2 = new TTextEdit( this, splitter, &buffer, mpHost, isDebugConsole, true );
    console2->setContentsMargins(0,0,0,0);
    console2->setSizePolicy( sizePolicy3 );
    console2->setFocusPolicy( Qt::NoFocus );

    splitter->addWidget( console );
    splitter->addWidget( console2 );

    splitter->setCollapsible( 1, false );
    splitter->setCollapsible( 0, false );
    splitter->setStretchFactor(0,6);
    splitter->setStretchFactor(1,1);

    layoutLayer->addWidget( splitter );
    layoutLayer->addWidget( mpScrollBar );
    layoutLayer->setContentsMargins(0,0,0,0);
    layoutLayer->setSpacing( 1 );// nicht naeher dran, da es sonst performance probleme geben koennte beim display

    layerCommandLine = new QWidget;//( mpMainFrame );//layer );
    layerCommandLine->setContentsMargins(0,0,0,0);
    layerCommandLine->setSizePolicy( sizePolicy2 );
    layerCommandLine->setMaximumHeight(31);
    layerCommandLine->setMinimumHeight(31);

    auto layoutLayer2 = new QHBoxLayout( layerCommandLine );
    layoutLayer2->setMargin(0);
    layoutLayer2->setSpacing(0);

    auto buttonMainLayer = new QWidget;//( layerCommandLine );
    buttonMainLayer->setSizePolicy(sizePolicy);
    buttonMainLayer->setContentsMargins(0,0,0,0);
    auto layoutButtonMainLayer = new QVBoxLayout( buttonMainLayer );
    layoutButtonMainLayer->setMargin(0);
    layoutButtonMainLayer->setContentsMargins(0,0,0,0);

    layoutButtonMainLayer->setSpacing(0);
    /*buttonMainLayer->setMinimumHeight(31);
    buttonMainLayer->setMaximumHeight(31);*/
    auto buttonLayer = new QWidget;
    auto layoutButtonLayer = new QGridLayout( buttonLayer );
    layoutButtonLayer->setMargin(0);
    layoutButtonLayer->setSpacing(0);

    auto buttonLayerSpacer = new QWidget(buttonLayer);
    buttonLayerSpacer->setSizePolicy( sizePolicy4 );
    layoutButtonMainLayer->addWidget( buttonLayerSpacer );
    layoutButtonMainLayer->addWidget( buttonLayer );

    auto timeStampButton = new QToolButton;
    timeStampButton->setCheckable( true );
    timeStampButton->setMinimumSize(QSize(30,30));
    timeStampButton->setMaximumSize(QSize(30,30));
    timeStampButton->setSizePolicy( sizePolicy5 );
    timeStampButton->setFocusPolicy( Qt::NoFocus );
    timeStampButton->setToolTip(tr("<html><head/><body><p>Show Time Stamps.</p></body></html>"));
    timeStampButton->setIcon( QIcon( QStringLiteral( ":/icons/dialog-information.png" ) ) );
    connect( timeStampButton, SIGNAL(pressed()), console, SLOT(slot_toggleTimeStamps()));

    auto replayButton = new QToolButton;
    replayButton->setCheckable( true );
    replayButton->setMinimumSize(QSize(30,30));
    replayButton->setMaximumSize(QSize(30,30));
    replayButton->setSizePolicy( sizePolicy5 );
    replayButton->setFocusPolicy( Qt::NoFocus );
    replayButton->setToolTip(tr("<html><head/><body><p>Record a replay.</p></body></html>"));
    replayButton->setIcon( QIcon( QStringLiteral( ":/icons/media-tape.png" ) ) );
    connect( replayButton, SIGNAL(pressed()), this, SLOT(slot_toggleReplayRecording()));

    logButton = new QToolButton;
    logButton->setMinimumSize(QSize(30, 30));
    logButton->setMaximumSize(QSize(30, 30));
    logButton->setCheckable( true );
    logButton->setSizePolicy( sizePolicy5 );
    logButton->setFocusPolicy( Qt::NoFocus );
    logButton->setToolTip(tr("<html><head/><body><p>Start logging MUD output to log file.</p></body></html>"));
    QIcon logIcon;
    logIcon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-downloads.png" ) ), QIcon::Normal, QIcon::Off );
    logIcon.addPixmap( QPixmap( QStringLiteral( ":/icons/folder-downloads-red-cross.png" ) ), QIcon::Normal, QIcon::On );
    logButton->setIcon( logIcon );
    connect( logButton, SIGNAL(pressed()), this, SLOT(slot_toggleLogging()));

    networkLatency->setReadOnly( true );
    networkLatency->setSizePolicy( sizePolicy4 );
    networkLatency->setFocusPolicy( Qt::NoFocus );
    networkLatency->setToolTip(tr("<html><head/><body><p><i>N:</i> is the latency of the MUD server and network (aka ping, in seconds), <br><i>S:</i> is the system processing time - how long your triggers took to process the last line(s).</p></body></html>"));
    networkLatency->setMaximumSize( 120, 30 );
    networkLatency->setMinimumSize( 120, 30 );
    networkLatency->setAutoFillBackground( true );
    networkLatency->setContentsMargins(0,0,0,0);
    QPalette basePalette;
    basePalette.setColor( QPalette::Text, QColor(Qt::black) );
    basePalette.setColor( QPalette::Base, QColor(Qt::white) );
    networkLatency->setPalette( basePalette );
    networkLatency->setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );



    QFont latencyFont = QFont("Bitstream Vera Sans Mono", 10, QFont::Normal);
    int width;
    int maxWidth = 120;
    width = QFontMetrics( latencyFont ).boundingRect(QString("N:0.000 S:0.000")).width();
    if( width < maxWidth )
    {
        networkLatency->setFont( latencyFont );
    }
    else
    {
         QFont latencyFont2 = QFont("Bitstream Vera Sans Mono", 9, QFont::Normal);
         width = QFontMetrics( latencyFont2 ).boundingRect(QString("N:0.000 S:0.000")).width();
         if( width < maxWidth )
         {
             networkLatency->setFont( latencyFont2 );
         }
         else
         {
             QFont latencyFont3 = QFont("Bitstream Vera Sans Mono", 8, QFont::Normal);
             width = QFontMetrics( latencyFont3 ).boundingRect(QString("N:0.000 S:0.000")).width();
             networkLatency->setFont( latencyFont3 );
         }
    }

    emergencyStop->setMinimumSize(QSize(30,30));
    emergencyStop->setMaximumSize(QSize(30,30));
    emergencyStop->setIcon( QIcon( QStringLiteral( ":/icons/edit-bomb.png" ) ) );
    emergencyStop->setSizePolicy( sizePolicy4 );
    emergencyStop->setFocusPolicy( Qt::NoFocus );
    emergencyStop->setCheckable( true );
    emergencyStop->setToolTip(tr("<html><head/><body><p>Emergency Stop. Stops all timers and triggers.</p></body></html>"));
    connect( emergencyStop, SIGNAL(clicked(bool)), this, SLOT(slot_stop_all_triggers( bool )));

    mpBufferSearchBox->setMinimumSize(QSize(100,30));
    mpBufferSearchBox->setMaximumSize(QSize(150,30));
    mpBufferSearchBox->setSizePolicy( sizePolicy5 );
    mpBufferSearchBox->setFont(mpHost->mCommandLineFont);
    mpBufferSearchBox->setFocusPolicy( Qt::ClickFocus );
    mpBufferSearchBox->setPlaceholderText("Search ...");
    QPalette __pal;
    __pal.setColor(QPalette::Text, mpHost->mCommandLineFgColor );//QColor(0,0,192));
    __pal.setColor(QPalette::Highlight,QColor(0,0,192));
    __pal.setColor(QPalette::HighlightedText, QColor(Qt::white));
    __pal.setColor(QPalette::Base,mpHost->mCommandLineBgColor);//QColor(255,255,225));
    __pal.setColor(QPalette::Window, mpHost->mCommandLineBgColor);
    mpBufferSearchBox->setPalette( __pal );
    mpBufferSearchBox->setToolTip(tr("<html><head/><body><p>Search buffer.</p></body></html>"));
    connect( mpBufferSearchBox, SIGNAL(returnPressed()), this, SLOT(slot_searchBufferUp()));




    mpBufferSearchUp->setMinimumSize(QSize(30,30));
    mpBufferSearchUp->setMaximumSize(QSize(30,30));
    mpBufferSearchUp->setSizePolicy( sizePolicy5 );
    mpBufferSearchUp->setToolTip(tr("<html><head/><body><p>Earlier search result.</p></body></html>"));
    mpBufferSearchUp->setFocusPolicy( Qt::NoFocus );
    mpBufferSearchUp->setIcon( QIcon( QStringLiteral( ":/icons/export.png" ) ) );
    connect( mpBufferSearchUp, SIGNAL(clicked()), this, SLOT(slot_searchBufferUp()));


    mpBufferSearchDown->setMinimumSize(QSize(30,30));
    mpBufferSearchDown->setMaximumSize(QSize(30,30));
    mpBufferSearchDown->setSizePolicy( sizePolicy5 );
    mpBufferSearchDown->setFocusPolicy( Qt::NoFocus );
    mpBufferSearchDown->setToolTip(tr("<html><head/><body><p>Later search result.</p></body></html>"));
    mpBufferSearchDown->setIcon( QIcon( QStringLiteral( ":/icons/import.png" ) ) );
    connect( mpBufferSearchDown, SIGNAL(clicked()), this, SLOT(slot_searchBufferDown()));

    layoutLayer2->addWidget( mpCommandLine );
    layoutLayer2->addWidget( buttonMainLayer );
    layoutButtonLayer->addWidget( mpBufferSearchBox,0, 0, 0, 4 );
    layoutButtonLayer->addWidget( mpBufferSearchUp, 0, 5 );
    layoutButtonLayer->addWidget( mpBufferSearchDown, 0, 6 );
    layoutButtonLayer->addWidget( timeStampButton, 0, 7 );
    layoutButtonLayer->addWidget( replayButton, 0, 8 );
    layoutButtonLayer->addWidget( logButton, 0, 9 );
    layoutButtonLayer->addWidget( emergencyStop, 0, 10 );
    layoutButtonLayer->addWidget( networkLatency, 0, 11 );
    layoutLayer2->setContentsMargins(0,0,0,0);
    layout->addWidget( layer );
    networkLatency->setFrame( false );
    //QPalette whitePalette;
    //whitePalette.setColor( QPalette::Window, baseColor);//,255) );
    layerCommandLine->setPalette( basePalette );
    layerCommandLine->setAutoFillBackground( true );

    centralLayout->addWidget( layerCommandLine );

    QList<int> sizeList;
    sizeList << 6 << 2;
    splitter->setSizes( sizeList );

    console->show();
    console2->show();
    console2->hide();
    if( mIsDebugConsole )
        mpCommandLine->hide();

    isUserScrollBack = false;

    connect( mpScrollBar, SIGNAL(valueChanged(int)), console, SLOT(slot_scrollBarMoved(int)));

    if( mIsSubConsole )
    {
        mpScrollBar->hide();
        console2->hide();
        layerCommandLine->hide();
        mpMainFrame->move(0,0);
        mpMainDisplay->move(0,0);
    }
    if( mIsDebugConsole )
    {
        layerCommandLine->hide();
    }

    mpBaseVFrame->setContentsMargins(0,0,0,0);
    mpBaseHFrame->setContentsMargins(0,0,0,0);
    mpBaseVFrame->layout()->setMargin(0);
    mpBaseVFrame->layout()->setSpacing(0);
    mpBaseHFrame->layout()->setMargin(0);
    mpBaseHFrame->layout()->setSpacing(0);


    buttonLayerSpacer->setMinimumHeight(0);
    buttonLayerSpacer->setMinimumWidth(100);
    buttonLayer->setMaximumHeight(31);
    //buttonLayer->setMaximumWidth(31);
    buttonLayer->setMinimumWidth(400);
    buttonLayer->setMaximumWidth(400);
    buttonMainLayer->setMinimumWidth(400);
    buttonMainLayer->setMaximumWidth(400);
    setFocusPolicy(Qt::ClickFocus);
    setFocusProxy(mpCommandLine);
    console->setFocusPolicy(Qt::ClickFocus);
    console->setFocusProxy(mpCommandLine);
    console2->setFocusPolicy(Qt::ClickFocus);
    console2->setFocusProxy(mpCommandLine);

    buttonLayerSpacer->setAutoFillBackground( true );
    buttonLayerSpacer->setPalette( __pal );
    buttonMainLayer->setAutoFillBackground( true );
    buttonMainLayer->setPalette( __pal );

    buttonLayer->setAutoFillBackground( true );
    buttonLayer->setPalette( __pal );

    layerCommandLine->setPalette( __pal );

    changeColors();
    if( ! mIsSubConsole && ! mIsDebugConsole )
    {
        // During first use where mIsDebugConsole IS true mudlet::self() is null
        // then - but we rely on that flag to avoid having to also test for a
        // non-null mudlet::self() - the connect(...) will produce a debug
        // message and not make THAT connection should it indeed be null but it
        // is not fatal...
        // So, this SHOULD be the main profile console - Slysven
        connect( mudlet::self(),
                 SIGNAL( signal_profileMapReloadRequested( QList<QString> ) ),
                 this,
                 SLOT( slot_reloadMap( QList<QString> ) ),
                 Qt::UniqueConnection );
        // For some odd reason the first seems to get connected twice - the
        // last flag prevents multiple ones being made
    }
}

Host * TConsole::getHost() { return mpHost; }

void TConsole::setLabelStyleSheet( std::string & buf, std::string & sh )
{
    std::string key = buf;
    QString sheet = sh.c_str();
    if( mLabelMap.find( key ) != mLabelMap.end() )
    {
        QLabel * pC = mLabelMap[key];
        if( ! pC ) return;
        pC->setStyleSheet( sheet );
        return;
    }
}


void TConsole::resizeEvent( QResizeEvent * event )
{
    if( ! mIsDebugConsole && ! mIsSubConsole )
    {
        mMainFrameTopHeight = mpHost->mBorderTopHeight;
        mMainFrameBottomHeight = mpHost->mBorderBottomHeight;
        mMainFrameLeftWidth = mpHost->mBorderLeftWidth;
        mMainFrameRightWidth = mpHost->mBorderRightWidth;
    }
    int x = event->size().width();
    int y = event->size().height();


    if( ! mIsSubConsole && ! mIsDebugConsole )
    {
        mpMainFrame->resize(x,y);
        mpBaseVFrame->resize(x, y);
        mpBaseHFrame->resize(x, y);
        x = x-mpLeftToolBar->width()-mpRightToolBar->width();
        y = y-mpTopToolBar->height();
        mpMainDisplay->resize( x - mMainFrameLeftWidth - mMainFrameRightWidth,
                               y - mMainFrameTopHeight - mMainFrameBottomHeight - mpCommandLine->height() );
    }
    else
    {
        mpMainFrame->resize( x, y );
        mpMainDisplay->resize( x,y);//x - mMainFrameLeftWidth - mMainFrameRightWidth, y - mMainFrameTopHeight - mMainFrameBottomHeight );

    }
    mpMainDisplay->move( mMainFrameLeftWidth, mMainFrameTopHeight );

    if( mIsSubConsole || mIsDebugConsole )
    {
        layerCommandLine->hide();
        mpCommandLine->hide();
    }
    else
    {
        //layerCommandLine->move(0,mpMainFrame->height()-layerCommandLine->height());
        layerCommandLine->move(0, mpBaseVFrame->height()-layerCommandLine->height());
    }

    QWidget::resizeEvent( event );

    if( ! mIsDebugConsole && ! mIsSubConsole )
    {
        TLuaInterpreter * pLua = mpHost->getLuaInterpreter();
        QString func = "handleWindowResizeEvent";
        QString n = "WindowResizeEvent";
        pLua->call( func, n );

        TEvent mudletEvent;
        mudletEvent.mArgumentList.append(QLatin1String("sysWindowResizeEvent"));
        mudletEvent.mArgumentList.append(QString::number(x - mMainFrameLeftWidth - mMainFrameRightWidth));
        mudletEvent.mArgumentList.append(QString::number(y - mMainFrameTopHeight - mMainFrameBottomHeight - mpCommandLine->height()));
        mudletEvent.mArgumentList.append(mConsoleName);
        mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
        mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
        mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mpHost->raiseEvent(mudletEvent);
    }
}

void TConsole::refresh()
{
    if( ! mIsDebugConsole )
    {
        mMainFrameTopHeight = mpHost->mBorderTopHeight;
        mMainFrameBottomHeight = mpHost->mBorderBottomHeight;
        mMainFrameLeftWidth = mpHost->mBorderLeftWidth;
        mMainFrameRightWidth = mpHost->mBorderRightWidth;
    }

    int x = width();
    int y = height();

    mpBaseVFrame->resize( x, y );
    mpBaseHFrame->resize( x, y );

    x = mpBaseVFrame->width();
    if( !mpLeftToolBar->isHidden() ) x -= mpLeftToolBar->width();
    if( !mpRightToolBar->isHidden() ) x -= mpRightToolBar->width();

    y = mpBaseVFrame->height();
    if( !mpTopToolBar->isHidden() ) y -= mpTopToolBar->height();

    mpMainDisplay->resize( x - mMainFrameLeftWidth - mMainFrameRightWidth,
                           y - mMainFrameTopHeight - mMainFrameBottomHeight - mpCommandLine->height() );

    mpMainDisplay->move( mMainFrameLeftWidth, mMainFrameTopHeight );
    x = width();
    y = height();
    QSize s = QSize(x,y);
    QResizeEvent event(s, s);
    QApplication::sendEvent( this, &event);
}


void TConsole::closeEvent( QCloseEvent *event )
{
    if( mIsDebugConsole )
    {
        if( mudlet::self()->isGoingDown() || mpHost->isClosingDown() ) {
            event->accept();
            return;
        } else {
            hide();
            mudlet::mpDebugArea->setVisible(false);
            mudlet::debugMode = false;
            event->ignore();
            return;
        }
    }

    if( mUserConsole )
    {
        if( mudlet::self()->isGoingDown() || mpHost->isClosingDown() )
        {
            std::string key = objectName().toLatin1().data();
            TConsole * pC = mpHost->mpConsole;
            if( pC->mSubConsoleMap.find(key) != pC->mSubConsoleMap.end() ) {
                console->close();
                console2->close();

                pC->mSubConsoleMap.erase(key);
            }

            event->accept();
            return;
        } else {
            hide();
            event->ignore();
            return;
        }
    }

    if( profile_name != "default_host" )
    {
        TEvent conCloseEvent;
        conCloseEvent.mArgumentList.append(QLatin1String("sysExitEvent"));
        conCloseEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mpHost->raiseEvent( conCloseEvent );

        if( mpHost->mFORCE_SAVE_ON_EXIT )
        {
            mpHost->modulesToWrite.clear();
            mpHost->saveProfile();

            if( mpHost->mpMap->mpRoomDB->size() > 0 )
            {
                QDir dir_map;
                QString directory_map = QDir::homePath()+"/.config/mudlet/profiles/"+profile_name+"/map";
                QString filename_map = directory_map + "/"+QDateTime::currentDateTime().toString("dd-MM-yyyy#hh-mm-ss")+"map.dat";
                if( ! dir_map.exists( directory_map ) )
                {
                    dir_map.mkpath( directory_map );
                }
                QFile file_map( filename_map );
                if ( file_map.open( QIODevice::WriteOnly ) )
                {
                    QDataStream out( & file_map );
                    mpHost->mpMap->serialize( out );
                    file_map.close();
                }
            }
            event->accept();
            return;
        }
    }

    if( profile_name != "default_host" && ! mUserAgreedToCloseConsole )
    {
        ASK: int choice = QMessageBox::question( this, tr("Save profile?"), tr("Do you want to save the profile %1?").arg(profile_name), QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel );
        if( choice == QMessageBox::Cancel )
        {
            event->setAccepted(false);
            event->ignore();
            return;
        }
        if (choice == QMessageBox::Yes) {
            mudlet::self()->saveWindowLayout();

            mpHost->modulesToWrite.clear();
            std::tuple<bool, QString, QString> result = mpHost->saveProfile();

            if (std::get<0>(result) == false) {
                QMessageBox::critical(this, tr("Couldn't save profile"), tr("Sorry, couldn't save your profile - got the following error: %1").arg(std::get<2>(result)));
                goto ASK;
            } else if (mpHost->mpMap && mpHost->mpMap->mpRoomDB->size() > 0) {
                QDir dir_map;
                QString directory_map = QDir::homePath() + "/.config/mudlet/profiles/" + profile_name + "/map";
                QString filename_map = directory_map + "/" + QDateTime::currentDateTime().toString("dd-MM-yyyy#hh-mm-ss") + "map.dat";
                if (!dir_map.exists(directory_map)) {
                    dir_map.mkpath(directory_map);
                }
                QFile file_map(filename_map);
                if (file_map.open(QIODevice::WriteOnly)) {
                    QDataStream out(&file_map);
                    mpHost->mpMap->serialize(out);
                    file_map.close();
                }
            }
            event->accept();
            return;

        } else if (choice == QMessageBox::No) {
            mudlet::self()->saveWindowLayout();

            event->accept();
            return;
        }
        else
        {
            if( ! mudlet::self()->isGoingDown() )
            {
                QMessageBox::warning( this, "Aborting exit","Session exit aborted on user request." );
                event->ignore();
                return;
            }
            else
            {
                event->accept();
                return;
            }
        }
    }
}



int TConsole::getButtonState()
{
    return mButtonState;
}

void TConsole::toggleLogging( bool isMessageEnabled )
{
    if( mIsDebugConsole || mIsSubConsole ) {
        return;
        // We don't support logging anything other than main console (at present?)
    }

    QFile file( QStringLiteral( "%1/.config/mudlet/autolog" ).arg( QDir::homePath() ) );
    if( ! mLogToLogFile ) {
        file.open( QIODevice::WriteOnly | QIODevice::Text );
        QTextStream out(&file);
        file.close();

        QString directoryLogFile = QStringLiteral( "%1/.config/mudlet/profiles/%2/log" ).arg(QDir::homePath(), profile_name);
        mLogFileName = QStringLiteral( "%1/%2" ).arg(directoryLogFile, QDateTime::currentDateTime().toString( QStringLiteral( "yyyy-MM-dd#hh-mm-ss" ) ) );
        // Revised file name derived from time so that alphabetical filename and
        // date sort order are the same...
        QDir dirLogFile;
        if( ! dirLogFile.exists( directoryLogFile ) ) {
            dirLogFile.mkpath( directoryLogFile );
        }

        mpHost->mIsCurrentLogFileInHtmlFormat = mpHost->mIsNextLogFileInHtmlFormat;
        if( mpHost->mIsCurrentLogFileInHtmlFormat ) {
            mLogFileName.append( QStringLiteral( ".html" ) );
        }
        else {
            mLogFileName.append( QStringLiteral( ".txt" ) );
        }
        mLogFile.setFileName( mLogFileName );
        mLogFile.open( QIODevice::WriteOnly );
        mLogStream.setDevice( &mLogFile );
        if( isMessageEnabled ) {
            QString message = tr("Logging has started. Log file is %1\n").arg( mLogFile.fileName() );
            printSystemMessage( message );
            // This puts text onto console that is IMMEDIATELY POSTED into log file so
            // must be done BEFORE logging starts - or actually mLogToLogFile gets set!
        }
        mLogToLogFile = true;
    }
    else {
        file.remove();
        mLogToLogFile = false;
        if( isMessageEnabled ) {
            QString message = tr("Logging has been stopped. Log file is %1\n").arg( mLogFile.fileName() );
            printSystemMessage( message );
            // This puts text onto console that is IMMEDIATELY POSTED into log file so
            // must be done AFTER logging ends - or actually mLogToLogFile gets reset!
        }
    }

    if( mLogToLogFile ) {
        if( mpHost->mIsCurrentLogFileInHtmlFormat ) {
            QStringList fontsList; // List of fonts to become the font-family entry for
                                   // the master css in the header
            fontsList << this->fontInfo().family(); // Seems to be the best way to get the
                                                // font in use, as different TConsole
                                                // instances within the same profile
                                                // might have different fonts in future,
                                                // and although the font is settable for
                                                // the main profile window, it is not yet
                                                // for user miniConsoles, or the Debug one
            fontsList << QStringLiteral( "Courier New" );
            fontsList << QStringLiteral( "Monospace" );
            fontsList << QStringLiteral( "Courier" );
            fontsList.removeDuplicates(); // In case the actual one is one of the defaults here

            mLogStream << "<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.01//EN' 'http://www.w3.org/TR/html4/strict.dtd'>\n";
            mLogStream << "<html>\n";
            mLogStream << " <head>\n";
            mLogStream << "  <meta http-equiv='content-type' content='text/html; charset=utf-8'>";
            // put the charset as early as possible as the parser MUST restart when it
            // switches away from the ASCII default
            mLogStream << "  <meta name='generator' content='Mudlet MUD Client version: " << APP_VERSION << APP_BUILD << "'>\n";
            // Nice to identify what made the file!
            mLogStream << "  <title>" << tr( "Mudlet, log from %1 profile" ).arg(profile_name) << "</title>\n" ;
             // Web-page title
            mLogStream << "  <style type='text/css'>\n";
            mLogStream << "   <!-- body { font-family: '" << fontsList.join("', '") << "'; font-size: 100%; line-height: 1.125em; white-space: nowrap; color:rgb(255,255,255); background-color:rgb("<<mpHost->mBgColor.red()<<","<<mpHost->mBgColor.green()<<","<<mpHost->mBgColor.blue()<<");}\n";
            mLogStream << "        span { white-space: pre; } -->\n";
            mLogStream << "  </style>\n";
            mLogStream << "  </head>\n";
            mLogStream << "  <body><div>";
            // <div></div> tags required around outside of the body <span></spans> for
            // strict HTML 4 as we do not use <p></p>s or anything else
            mLogFile.flush();
        }
        logButton->setToolTip( tr("<html><head/><body><p>Stop logging MUD output to log file.</p></body></html>") );
    }
    else {
        if( mpHost->mIsCurrentLogFileInHtmlFormat ) {
            mLogStream << "</div></body>\n";
            mLogStream << "</html>\n";
        }
        mLogFile.flush();
        mLogFile.close();
        logButton->setToolTip( tr("<html><head/><body><p>Start logging MUD output to log file.</p></body></html>") );
    }
}

// Converted into a wrapper around a separate toggleLogging() method so that
// calls to turn logging on/off via the toolbar button - which go via this
// wrapper - generate messages on the console.  Requests to control logging from
// the Lua interpreter call the wrapped method directly and messages are
// generated for Lua user control by the Lua subsystem.
void TConsole::slot_toggleLogging()
{
    if( mIsDebugConsole || mIsSubConsole ) {
        return;
        // We don't support logging anything other than main console (at present?)
    }

    toggleLogging( true );
}

void TConsole::slot_toggleReplayRecording()
{
    if( mIsDebugConsole ) return;
    mRecordReplay = ! mRecordReplay;
    if( mRecordReplay )
    {
        QString directoryLogFile = QDir::homePath()+"/.config/mudlet/profiles/"+profile_name+"/log";
        QString mLogFileName = directoryLogFile + "/"+QDateTime::currentDateTime().toString("dd-MM-yyyy#hh-mm-ss");
        mLogFileName.append(".dat");
        QDir dirLogFile;
        if( ! dirLogFile.exists( directoryLogFile ) )
        {
            dirLogFile.mkpath( directoryLogFile );
        }
        mReplayFile.setFileName( mLogFileName );
        mReplayFile.open( QIODevice::WriteOnly );
        mReplayStream.setDevice( &mReplayFile );
        mpHost->mTelnet.recordReplay();
        QString message = QString("Replay recording has started. File: ") + mReplayFile.fileName() + "\n";
        printSystemMessage( message );
    }
    else
    {
        mReplayFile.close();
        QString message = QString("Replay recording has been stopped. File: ") + mReplayFile.fileName() + "\n";
        printSystemMessage( message );
    }
}

void TConsole::changeColors()
{
    mDisplayFont.setFixedPitch(true);
    if( mIsDebugConsole )
    {
        mDisplayFont.setStyleStrategy( (QFont::StyleStrategy)(QFont::NoAntialias | QFont::PreferQuality) );
        mDisplayFont.setFixedPitch(true);
        console->setFont( mDisplayFont );
        console2->setFont( mDisplayFont );
        QPalette palette;
        palette.setColor( QPalette::Text, mFgColor );
        palette.setColor( QPalette::Highlight, QColor(55,55,255) );
        palette.setColor( QPalette::Base, QColor(Qt::black) );
        console->setPalette( palette );
        console2->setPalette( palette );
    }
    else if( mIsSubConsole )
    {
#if defined(Q_OS_MAC) || defined(Q_OS_LINUX)
        mDisplayFont.setStyleStrategy( (QFont::StyleStrategy)(QFont::NoAntialias | QFont::PreferQuality ) );
        QPixmap pixmap = QPixmap( 2000, 600 );
        QPainter p(&pixmap);
        mDisplayFont.setLetterSpacing( QFont::AbsoluteSpacing, 0 );
        p.setFont(mDisplayFont);
        const QRectF r = QRectF(0,0,2000, 600);
        QRectF r2;
        const QString t = "123";
        p.drawText(r,1,t,&r2);
// N/U:        int mFontHeight = QFontMetrics( mDisplayFont ).height();
        int mFontWidth = QFontMetrics( mDisplayFont ).width( QChar('W') );
        qreal letterSpacing = (qreal)((qreal)mFontWidth-(qreal)(r2.width()/t.size()));
        console->mLetterSpacing = letterSpacing;
        console2->mLetterSpacing = letterSpacing;
        mpHost->mDisplayFont.setLetterSpacing( QFont::AbsoluteSpacing, letterSpacing );
        mDisplayFont.setLetterSpacing( QFont::AbsoluteSpacing, console->mLetterSpacing );
#endif
        mDisplayFont.setFixedPitch(true);
        console->setFont( mDisplayFont );
        console2->setFont( mDisplayFont );
        QPalette palette;
        palette.setColor( QPalette::Text, mFgColor );
        palette.setColor( QPalette::Highlight, QColor(55,55,255) );
        palette.setColor( QPalette::Base, mBgColor );
        setPalette( palette );
        layer->setPalette( palette );
        console->setPalette( palette );
        console2->setPalette( palette );
    }
    else
    {
        QPalette pal;
        pal.setColor(QPalette::Text, mpHost->mCommandLineFgColor );//QColor(0,0,192));
        pal.setColor(QPalette::Highlight,QColor(0,0,192));
        pal.setColor(QPalette::HighlightedText, QColor(Qt::white));
        pal.setColor(QPalette::Base,mpHost->mCommandLineBgColor);//QColor(255,255,225));
        mpCommandLine->setPalette( pal );
        mpCommandLine->mRegularPalette = pal;
        if( mpHost->mNoAntiAlias )
            mpHost->mDisplayFont.setStyleStrategy( QFont::NoAntialias );
        else
            mpHost->mDisplayFont.setStyleStrategy( (QFont::StyleStrategy)( QFont::PreferAntialias | QFont::PreferQuality ) );
        mpHost->mDisplayFont.setFixedPitch(true);
        mDisplayFont.setFixedPitch(true);
#if defined(Q_OS_MAC) || defined(Q_OS_LINUX)
        QPixmap pixmap = QPixmap( 2000, 600 );
        QPainter p(&pixmap);
        QFont _font = mpHost->mDisplayFont;
        _font.setLetterSpacing(QFont::AbsoluteSpacing, 0);
        p.setFont(_font);
        const QRectF r = QRectF(0,0,2000, 600);
        QRectF r2;
        const QString t = "123";
        p.drawText(r,1,t,&r2);
// N/U:        int mFontHeight = QFontMetrics( mpHost->mDisplayFont ).height();
        int mFontWidth = QFontMetrics( mpHost->mDisplayFont ).width( QChar('W') );
        qreal letterSpacing = (qreal)((qreal)mFontWidth-(qreal)(r2.width()/t.size()));
        console->mLetterSpacing = letterSpacing;
        console2->mLetterSpacing = letterSpacing;
        mpHost->mDisplayFont.setLetterSpacing( QFont::AbsoluteSpacing, letterSpacing );
        mDisplayFont.setLetterSpacing( QFont::AbsoluteSpacing, console->mLetterSpacing );
#endif
        console->setFont( mpHost->mDisplayFont );
        console2->setFont( mpHost->mDisplayFont );
        QPalette palette;
        palette.setColor( QPalette::Text, mpHost->mFgColor );
        palette.setColor( QPalette::Highlight, QColor(55,55,255) );
        palette.setColor( QPalette::Base, mpHost->mBgColor );
        setPalette( palette );
        layer->setPalette( palette );
        console->setPalette( palette );
        console2->setPalette( palette );
        mCommandFgColor = mpHost->mCommandFgColor;
        mCommandBgColor = mpHost->mCommandBgColor;
        mpCommandLine->setFont(mpHost->mDisplayFont);
        mFormatCurrent.bgR = mpHost->mBgColor.red();
        mFormatCurrent.bgG = mpHost->mBgColor.green();
        mFormatCurrent.bgB = mpHost->mBgColor.blue();
        mFormatCurrent.fgR = mpHost->mFgColor.red();
        mFormatCurrent.fgG = mpHost->mFgColor.green();
        mFormatCurrent.fgB = mpHost->mFgColor.blue();

    }
    QPalette palette;
    palette.setColor( QPalette::Button, QColor(Qt::blue) );
    palette.setColor( QPalette::Window, QColor(Qt::green) );
    palette.setColor( QPalette::Base, QColor(Qt::red) );

    if( ! mIsSubConsole )
    {
        console->mWrapAt = mWrapAt;
        console2->mWrapAt = mWrapAt;
        splitter->setPalette( palette );
    }

    buffer.updateColors();
    if( ! mIsDebugConsole && ! mIsSubConsole )
    {
        buffer.mWrapAt = mpHost->mWrapAt;
        buffer.mWrapIndent = mpHost->mWrapIndentCount;
    }


}

void TConsole::setConsoleBgColor( int r, int g, int b )
{
    mBgColor = QColor(r,g,b);
    console->setConsoleBgColor( r, g, b );
    changeColors();
}

void TConsole::setConsoleFgColor( int r, int g, int b )
{
    mFgColor = QColor(r,g,b);
    console->setConsoleFgColor( r, g, b );
    changeColors();
}

/*std::string TConsole::getCurrentTime()
{
    time_t t;
    time(&t);
    tm lt;
    ostringstream s;
    s.str("");
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    localtime_r( &t, &lt );
    s << "["<<lt.tm_hour<<":"<<lt.tm_min<<":"<<lt.tm_sec<<":"<<tv.tv_usec<<"]";
    string time = s.str();
    return time;
} */



void TConsole::loadRawFile( std::string n )
{
    QString directoryLogFile = QDir::homePath()+"/.config/mudlet/profiles/"+profile_name+"/log";
    QString fileName = directoryLogFile + "/"+QString(n.c_str());
    mpHost->mTelnet.loadReplay( fileName );
}

void TConsole::printOnDisplay( std::string & incomingSocketData, const bool isFromServer )
{

    mProcessingTime.restart();
    mTriggerEngineMode = true;
    buffer.translateToPlainText(incomingSocketData, isFromServer);
    mTriggerEngineMode = false;

    double processT = mProcessingTime.elapsed();
    if( mpHost->mTelnet.mGA_Driver )
    {
        networkLatency->setText( QString("N:%1 S:%2").arg(mpHost->mTelnet.networkLatency,0,'f',3)
                                                         .arg(processT/1000,0,'f',3));
    }
    else
    {
        networkLatency->setText( QString("<no GA> S:%1").arg(processT/1000,0,'f',3));
    }
}

void TConsole::runTriggers( int line )
{
    mDeletedLines = 0;
    mUserCursor.setY( line );
    mIsPromptLine = buffer.promptBuffer.at( line );
    mEngineCursor = line;
    mUserCursor.setX( 0 );
    mCurrentLine = buffer.line( line );
    mpHost->getLuaInterpreter()->set_lua_string( cmLuaLineVariable, mCurrentLine );
    mCurrentLine.append('\n');

    if( mudlet::debugMode )
    {
        TDebug(QColor(Qt::darkGreen),QColor(Qt::black)) << "new line arrived:">>0; TDebug(QColor(Qt::lightGray),QColor(Qt::black)) << mCurrentLine<<"\n">>0;
    }
    mpHost->incomingStreamProcessor( mCurrentLine, line );
    mIsPromptLine = false;

    //FIXME: neu schreiben: wenn lines oberhalb der aktuellen zeile gelöscht wurden->redraw clean slice
    //       ansonsten einfach löschen
}

void TConsole::finalize()
{
    console->showNewLines();
    console2->showNewLines();
}

     /* ANSI color codes: sequence = "ESCAPE + [ code_1; ... ; code_n m"
      -----------------------------------------
      0 reset
      1 intensity bold on
      2 intensity faint
      3 italics on
      4 underline on
      5 blink slow
      6 blink fast
      7 inverse on
      9 strikethrough
      22 intensity normal (not bold, not faint)
      23 italics off
      24 underline off
      27 inverse off
      29 strikethrough off
      30 fg black
      31 fg red
      32 fg green
      33 fg yellow
      34 fg blue
      35 fg magenta
      36 fg cyan
      37 fg white
      39 bg default white
      40 bg black
      41 bg red
      42 bg green
      43 bg yellow
      44 bg blue
      45 bg magenta
      46 bg cyan
      47 bg white
      49 bg black     */

void TConsole::scrollDown( int lines )
{
    console->scrollDown( lines );
    if( console->isTailMode() )
    {
        console2->mCursorY = buffer.lineBuffer.size();//getLastLineNumber();
        console2->hide();
        console->mCursorY = buffer.lineBuffer.size();//getLastLineNumber();
        console->mIsTailMode = true;
        console->updateScreenView();
        console->forceUpdate();

    }
}

void TConsole::scrollUp( int lines )
{
    console2->mIsTailMode = true;
    console2->mCursorY = buffer.size();//getLastLineNumber();
    console2->show();
    console2->updateScreenView();
    console2->forceUpdate();
    console->scrollUp( lines );
}

void TConsole::deselect()
{
    P_begin.setX( 0 );
    P_begin.setY( 0 );
    P_end.setX( 0 );
    P_end.setY( 0 );
}

void TConsole::showEvent( QShowEvent * event )
{
    if( ! mIsDebugConsole && ! mIsSubConsole )
    {
        if( mpHost )
        {
            mpHost->mTelnet.mAlertOnNewData = false;
        }
    }
    QWidget::showEvent( event );//FIXME-refac: might cause problems
}

void TConsole::hideEvent( QHideEvent * event )
{
    if( ! mIsDebugConsole && ! mIsSubConsole )
    {
        if( mpHost )
        {
            if( mudlet::self()->mWindowMinimized )
            {
                if( mpHost->mAlertOnNewData )
                {
                    mpHost->mTelnet.mAlertOnNewData = true;
                }
            }
        }
    }
    QWidget::hideEvent( event );//FIXME-refac: might cause problems
}


void TConsole::reset()
{
    deselect();
    mFormatCurrent.bgR = mStandardFormat.bgR;
    mFormatCurrent.bgG = mStandardFormat.bgG;
    mFormatCurrent.bgB = mStandardFormat.bgB;
    mFormatCurrent.fgR = mStandardFormat.fgR;
    mFormatCurrent.fgG = mStandardFormat.fgG;
    mFormatCurrent.fgB = mStandardFormat.fgB;
    mFormatCurrent.flags &= ~(TCHAR_BOLD);
    mFormatCurrent.flags &= ~(TCHAR_ITALICS);
    mFormatCurrent.flags &= ~(TCHAR_UNDERLINE);
    mFormatCurrent.flags &= ~(TCHAR_STRIKEOUT);
}

void TConsole::insertLink(const QString& text, QStringList & func, QStringList & hint, QPoint P, bool customFormat )
{
    int x = P.x();
    int y = P.y();
    int o = 0;//FIXME: das ist ein fehler bei mehrzeiliger selection
    int r = text.size();
    QPoint P2 = P;
    P2.setX( x+r );

    if( mTriggerEngineMode )
    {
        if( hasSelection() )
        {
            if( r < o )
            {
                int a = -1*(o-r);
                mpHost->getLuaInterpreter()->adjustCaptureGroups( x, a );
            }
            if( r > o )
            {
                int a = r-o;
                mpHost->getLuaInterpreter()->adjustCaptureGroups( x, a );
            }
        }
        else
        {
            mpHost->getLuaInterpreter()->adjustCaptureGroups( x, r );
        }
        if( y < mEngineCursor )
        {
            if( customFormat )
                buffer.insertInLine( P, text, mFormatCurrent );
            else
            {
                TChar _f = TChar( 0, 0, 255,
                                  mBgColor.red(), mBgColor.green(), mBgColor.blue(),
                                  false, false, true, false );
                buffer.insertInLine( P, text, _f );
            }
            buffer.applyLink( P, P2, text, func, hint );
            console->needUpdate( mUserCursor.y(), mUserCursor.y()+1 );
        }
        else if( y >= mEngineCursor )
        {
            if( customFormat )
                buffer.insertInLine( P, text, mFormatCurrent );
            else
            {
                TChar _f = TChar( 0, 0, 255,
                                  mBgColor.red(), mBgColor.green(), mBgColor.blue(),
                                  false, false, true, false );
                buffer.insertInLine( P, text, _f );
            }
            buffer.applyLink( P, P2, text, func, hint );
        }
        return;
    }
    else
    {
        if( ( buffer.buffer.size() == 0 && buffer.buffer[0].size() == 0 ) || mUserCursor == buffer.getEndPos() )
        {
            if( customFormat )
                buffer.addLink( mTriggerEngineMode, text, func, hint, mFormatCurrent );
            else
            {
                TChar _f = TChar( 0, 0, 255,
                                  mBgColor.red(), mBgColor.green(), mBgColor.blue(),
                                  false, false, true, false );
                buffer.addLink( mTriggerEngineMode, text, func, hint, _f );
            }

            /*buffer.append( text,
                           0,
                           text.size(),
                           mFormatCurrent.fgR,
                           mFormatCurrent.fgG,
                           mFormatCurrent.fgB,
                           mFormatCurrent.bgR,
                           mFormatCurrent.bgG,
                           mFormatCurrent.bgB,
                           mFormatCurrent.bold,
                           mFormatCurrent.italics,
                           mFormatCurrent.underline );*/
            console->showNewLines();
            console2->showNewLines();

        }
        else
        {
            if( customFormat )
                buffer.insertInLine( mUserCursor,
                                     text,
                                     mFormatCurrent );
            else
            {
                TChar _f = TChar( 0, 0, 255,
                                  mBgColor.red(), mBgColor.green(), mBgColor.blue(),
                                  false, false, true, false );
                buffer.insertInLine( mUserCursor,
                                     text,
                                     _f );
            }

            buffer.applyLink( P, P2, text, func, hint );
            if( text.indexOf("\n") != -1 )
            {
                int y_tmp = mUserCursor.y();
                int down = buffer.wrapLine( mUserCursor.y(),mpHost->mScreenWidth, mpHost->mWrapIndentCount, mFormatCurrent );
                console->needUpdate( y_tmp, y_tmp+down+1 );
                int y_neu = y_tmp+down;
                int x_adjust = text.lastIndexOf("\n");
                int x_neu = 0;
                if( x_adjust != -1 )
                {
                    x_neu = text.size()-x_adjust-1 > 0 ? text.size()-x_adjust-1 : 0;
                }
                moveCursor( x_neu, y_neu );
            }
            else
            {
                console->needUpdate( mUserCursor.y(),mUserCursor.y()+1 );
                moveCursor( mUserCursor.x()+text.size(), mUserCursor.y() );
            }
        }
    }
}

void TConsole::insertText(const QString& text, QPoint P )
{
    int x = P.x();
    int y = P.y();
    int o = 0;//FIXME: das ist ein fehler bei mehrzeiliger selection
    int r = text.size();
    if( mTriggerEngineMode )
    {
        if( hasSelection() )
        {
            if( r < o )
            {
                int a = -1*(o-r);
                mpHost->getLuaInterpreter()->adjustCaptureGroups( x, a );
            }
            if( r > o )
            {
                int a = r-o;
                mpHost->getLuaInterpreter()->adjustCaptureGroups( x, a );
            }
        }
        else
        {
            mpHost->getLuaInterpreter()->adjustCaptureGroups( x, r );
        }
        if( y < mEngineCursor )
        {
            buffer.insertInLine( P, text, mFormatCurrent );
            console->needUpdate( mUserCursor.y(), mUserCursor.y()+1 );
        }
        else if( y >= mEngineCursor )
        {
            buffer.insertInLine( P, text, mFormatCurrent );
        }
        return;
    }
    else
    {
        if( ( buffer.buffer.size() == 0 && buffer.buffer[0].size() == 0 ) || mUserCursor == buffer.getEndPos() )
        {
            buffer.append( text,
                           0,
                           text.size(),
                           mFormatCurrent.fgR,
                           mFormatCurrent.fgG,
                           mFormatCurrent.fgB,
                           mFormatCurrent.bgR,
                           mFormatCurrent.bgG,
                           mFormatCurrent.bgB,
                           mFormatCurrent.flags & TCHAR_BOLD,
                           mFormatCurrent.flags & TCHAR_ITALICS,
                           mFormatCurrent.flags & TCHAR_UNDERLINE,
                           mFormatCurrent.flags & TCHAR_STRIKEOUT );
            console->showNewLines();
            console2->showNewLines();
        }
        else
        {
            buffer.insertInLine( mUserCursor,
                                 text,
                                 mFormatCurrent );
            if( text.indexOf("\n") != -1 )
            {
                int y_tmp = mUserCursor.y();
                int down = buffer.wrapLine( mUserCursor.y(),mpHost->mScreenWidth, mpHost->mWrapIndentCount, mFormatCurrent );
                console->needUpdate( y_tmp, y_tmp+down+1 );
            }
            else
            {
                console->needUpdate( mUserCursor.y(),mUserCursor.y()+1 );
            }
        }
    }
}


void TConsole::replace(const QString& text )
{
    int x = P_begin.x();
    int o = P_end.x() - P_begin.x();
    int r = text.size();

    if( mTriggerEngineMode )
    {
        if( hasSelection() )
        {
            if( r < o )
            {
                int a = -1*(o-r);
                mpHost->getLuaInterpreter()->adjustCaptureGroups( x, a );
            }
            if( r > o )
            {
                int a = r-o;
                mpHost->getLuaInterpreter()->adjustCaptureGroups( x, a );
            }
        }
        else
        {
            mpHost->getLuaInterpreter()->adjustCaptureGroups( x, r );
        }
    }

    buffer.replaceInLine( P_begin, P_end, text, mFormatCurrent );
}

void TConsole::skipLine()
{
    if( deleteLine( mUserCursor.y() ) )
    {
        mDeletedLines++;
    }
}

// TODO: It may be worth considering moving the (now) three following methods
// to the TMap class...?
bool TConsole::saveMap(const QString& location)
{
    QDir dir_map;
    QString filename_map;
    QString directory_map = QDir::homePath()+"/.config/mudlet/profiles/"+profile_name+"/map";

    if (location == "")
        filename_map = directory_map + "/"+QDateTime::currentDateTime().toString("dd-MM-yyyy#hh-mm-ss")+"map.dat";
    else
        filename_map = location;

    if( ! dir_map.exists( directory_map ) )
    {
        dir_map.mkpath( directory_map );
    }
    QFile file_map( filename_map );
    if ( file_map.open( QIODevice::WriteOnly ) )
    {
        QDataStream out( & file_map );
        mpHost->mpMap->serialize( out );
        file_map.close();
    } else
        return false;

    return true;
}

bool TConsole::loadMap(const QString& location)
{
    Host * pHost = mpHost;
    if( ! pHost ) {
        // Check for valid mpHost pointer (mpHost was/is/will be a QPoint<Host>
        // in later software versions and is a weak pointer until used
        // (I think - Slysven ?)
        return false;
    }

    if( ! pHost->mpMap || ! pHost->mpMap->mpMapper ) {
        // No map or map currently loaded - so try and created mapper
        // but don't load a map here by default, we do that below and it may not
        // be the default map anyhow
        mudlet::self()->createMapper( false );
    }

    if( ! pHost->mpMap || ! pHost->mpMap->mpMapper ) {
        // And that failed so give up
        return false;
    }

    pHost->mpMap->mapClear();

    qDebug() << "TConsole::loadMap() - restore map case 1.";
    pHost->mpMap->pushErrorMessagesToFile( tr( "Pre-Map loading(1) report" ), true );
    QDateTime now( QDateTime::currentDateTime() );

    bool result = false;
    if( pHost->mpMap->restore( location ) ) {
        pHost->mpMap->audit();
        pHost->mpMap->mpMapper->mp2dMap->init();
        pHost->mpMap->mpMapper->updateAreaComboBox();
        pHost->mpMap->mpMapper->resetAreaComboBoxToPlayerRoomArea();
        pHost->mpMap->mpMapper->show();
        result = true;
    }
    else {
        pHost->mpMap->mpMapper->mp2dMap->init();
        pHost->mpMap->mpMapper->updateAreaComboBox();
        pHost->mpMap->mpMapper->show();
    }

    if( location.isEmpty() ) {
        pHost->mpMap->pushErrorMessagesToFile( tr( "Loading map(1) at %1 report" ).arg( now.toString( Qt::ISODate ) ), true );
    }
    else {
        pHost->mpMap->pushErrorMessagesToFile( tr( "Loading map(1) \"%1\" at %2 report" ).arg(location, now.toString( Qt::ISODate) ), true );
    }

    return result;
}

// Used by TLuaInterpreter::loadMap() and dlgProfilePreferences for import/load
// of files ending in ".xml"
// The TLuaInterpreter::loadMap() supplies a pointer to an error Message which
// it requires in the event of an error (it should be written in a structure
// to match "loadMap: XXXXX." format) - the presence of a non-null pointer here
// should be used to suppress the writing of error messages direct to the
// console - if possible!
bool TConsole::importMap( const QString & location, QString * errMsg )
{
    Host * pHost = mpHost;
    if( ! pHost ) {
        // Check for valid mpHost pointer (mpHost was/is/will be a QPoint<Host>
        // in later software versions and is a weak pointer until used
        // (I think - Slysven ?)
        if( errMsg ) {
            *errMsg = tr( "loadMap: NULL Host pointer {in TConsole::importMap(...)} - something is wrong!" );
        }
        return false;
    }

    if( ! pHost->mpMap || ! pHost->mpMap->mpMapper ) {
        // No map or mapper currently loaded/present - so try and create mapper
        mudlet::self()->createMapper( false );
    }

    if( ! pHost->mpMap || ! pHost->mpMap->mpMapper ) {
        // And that failed so give up
        if( errMsg ) {
            *errMsg = tr( "loadMap: unable to initialise mapper {in TConsole::importMap(...)} - something is wrong!" );
        }
        return false;
    }

    // Dump any outstanding map errors from past activities that had not yet
    // been logged...
    qDebug() << "TConsole::importingMap() - importing map case 1.";
    pHost->mpMap->pushErrorMessagesToFile( tr( "Pre-Map importing(1) report" ), true );
    QDateTime now( QDateTime::currentDateTime() );

    bool result = false;

    QFileInfo fileInfo( location );
    QString filePathNameString;
    if( ! fileInfo.filePath().isEmpty() ) {
        if( fileInfo.isRelative() ) {
            // Resolve the name relative to the profile home directory:
            filePathNameString = QDir::cleanPath( QStringLiteral( "%1/.config/mudlet/profiles/%2/%3" )
                                                  .arg( QDir::homePath(), pHost->getName(), fileInfo.filePath() ) );
        }
        else {
            if( fileInfo.exists() ) {
                filePathNameString = fileInfo.canonicalFilePath(); // Cannot use cannonical path if file doesn't exist!
            }
            else {
                filePathNameString = fileInfo.absoluteFilePath();
            }
        }
    }

    QFile file( filePathNameString );
    if( ! file.exists() ) {
        if( ! errMsg ) {
            QString infoMsg = tr( "[ ERROR ]  - Map file not found, path and name used was:\n"
                                               "%1." )
                              .arg( filePathNameString );
            pHost->postMessage( infoMsg );
        }
        else {
            // error message for lua loadMap()
            *errMsg = tr( "loadMap: bad argument #1 value (filename used: \n"
                         "\"%1\" was not found)." )
                     .arg( filePathNameString );
        }
        return false;
    }

    if( file.open( QFile::ReadOnly | QFile::Text ) ) {

        if( ! errMsg ) {
            QString infoMsg = tr( "[ INFO ]  - Map file located and opened, now parsing it..." );
            pHost->postMessage( infoMsg );
        }

        result = pHost->mpMap->importMap( file, errMsg );

        file.close();
        pHost->mpMap->pushErrorMessagesToFile( tr( "Importing map(1) \"%1\" at %2 report" ).arg(location, now.toString( Qt::ISODate) ) );
    }
    else {
        if( ! errMsg ) {
            QString infoMsg = tr( "[ INFO ]  - Map file located but it could not opened, please check permissions on:"
                                  "\"%1\"." )
                              .arg( filePathNameString );
            pHost->postMessage( infoMsg );
        }
        else {
            *errMsg = tr( "loadMap: bad argument #1 value (filename used: \n"
                         "\"%1\" could not be opened for reading)." )
                     .arg( filePathNameString );
        }
        return false;
    }

    return result;
}

bool TConsole::deleteLine( int y )
{
    return buffer.deleteLine( y );
}

bool TConsole::hasSelection()
{
    if( P_begin != P_end )
        return true;
    else
        return false;
}

void TConsole::insertText(const QString& msg )
{
    insertText( msg, mUserCursor );
}

void TConsole::insertLink(const QString& text, QStringList & func, QStringList & hint, bool customFormat )
{
    insertLink( text, func, hint, mUserCursor, customFormat );
}

void TConsole::insertHTML(const QString& text )
{
    insertText( text );
}

int TConsole::getLineNumber()
{
    return mUserCursor.y();
}

int TConsole::getColumnNumber()
{
    return mUserCursor.x();
}

int TConsole::getLineCount()
{
    return buffer.getLastLineNumber();
}

QStringList TConsole::getLines( int from, int to )
{
    QStringList ret;
    int delta = abs( from - to );
    for( int i=0; i<delta; i++ )
    {
        ret << buffer.line( from + i );
    }
    return ret;
}

void TConsole::selectCurrentLine()
{
    selectSection(0, buffer.line( mUserCursor.y() ).size() );
}

/*void TConsole::selectCurrentLine( std::string & buf )
{
    std::string key = buf;
    if( buf == "main" )
    {
        return selectCurrentLine();
    }
    if( mSubConsoleMap.find( key ) != mSubConsoleMap.end() )
    {
        TConsole * pC = mSubConsoleMap[key];
        if( ! pC ) return;
        pC->selectCurrentLine();
        return;
    }
    else
    {
        return;
    }
}*/
void TConsole::selectCurrentLine( std::string & buf )
{
    std::string key = buf;
    if( buf == "main" )
    {
        selectCurrentLine();
    }
    if( mSubConsoleMap.find( key ) != mSubConsoleMap.end() )
    {
        TConsole * pC = mSubConsoleMap[key];
        if( ! pC ) return;
        pC->selectCurrentLine();
    }
    else
    {
        return;
    }
}

std::list<int> TConsole::_getFgColor()
{
    std::list<int> result;
    int x = P_begin.x();
    int y = P_begin.y();
    if( y < 0 ) return result;
    if( x < 0 ) return result;
    if( y >= static_cast<int>(buffer.buffer.size()) ) return result;

    if( static_cast<int>(buffer.buffer[y].size())-1 >=  x )
    {
        result.push_back( buffer.buffer[y][x].fgR );
        result.push_back( buffer.buffer[y][x].fgG );
        result.push_back( buffer.buffer[y][x].fgB );
    }
    return result;
}

std::list<int> TConsole::_getBgColor()
{
    std::list<int> result;
    int x = P_begin.x();
    int y = P_begin.y();
    if( y < 0 ) return result;
    if( x < 0 ) return result;
    if( y >= static_cast<int>(buffer.buffer.size()) ) return result;

    if( static_cast<int>(buffer.buffer[y].size())-1 >=  x )
    {
        result.push_back( buffer.buffer[y][x].bgR );
        result.push_back( buffer.buffer[y][x].bgG );
        result.push_back( buffer.buffer[y][x].bgB );
    }
    return result;
}

std::list<int> TConsole::getFgColor( std::string & buf )
{
    std::list<int> result;
    std::string key = buf;
    if( buf == "main" )
    {
        return _getFgColor();
    }
    if( mSubConsoleMap.find( key ) != mSubConsoleMap.end() )
    {
        TConsole * pC = mSubConsoleMap[key];
        if( ! pC ) return result;
        return pC->_getFgColor();
    }
    else
    {
        return result;
    }
}

std::list<int> TConsole::getBgColor( std::string & buf )
{
    std::list<int> result;
    std::string key = buf;
    if( buf == "main" )
    {
        return _getBgColor();
    }
    if( mSubConsoleMap.find( key ) != mSubConsoleMap.end() )
    {
        TConsole * pC = mSubConsoleMap[key];
        if( ! pC ) return result;
        return pC->_getBgColor();
    }
    else
    {
        return result;
    }
}

void TConsole::luaWrapLine( std::string & buf, int line )
{
    std::string key = buf;
    if( buf == "main" )
    {
        _luaWrapLine( line );
        return;
    }
    if( mSubConsoleMap.find( key ) != mSubConsoleMap.end() )
    {
        TConsole * pC = mSubConsoleMap[key];
        if( ! pC ) return;
        pC->_luaWrapLine( line );
        return;
    }
}

void TConsole::_luaWrapLine( int line )
{
    if( !mpHost ) return;
    TChar ch(mpHost);
    buffer.wrapLine( line, mWrapAt, mIndentCount, ch );
}

bool TConsole::setMiniConsoleFontSize( std::string & buf, int size )
{
    std::string key = buf;
    if( mSubConsoleMap.find( key ) != mSubConsoleMap.end() )
    {
        TConsole * pC = mSubConsoleMap[key];
        if( ! pC ) return false;
        pC->console->mDisplayFont = QFont("Bitstream Vera Sans Mono", size, QFont::Normal);
        pC->console->updateScreenView();
        pC->console->forceUpdate();
        pC->console2->mDisplayFont = QFont("Bitstream Vera Sans Mono", size, QFont::Normal);
        pC->console2->updateScreenView();
        pC->console2->forceUpdate();
        return true;
    }
    return false;
}

QString TConsole::getCurrentLine()
{
    return buffer.line( mUserCursor.y() );
}

QString TConsole::getCurrentLine( std::string & buf )
{
    std::string key = buf;
    if( buf == "main" )
    {
        return getCurrentLine();
    }
    if( mSubConsoleMap.find( key ) != mSubConsoleMap.end() )
    {
        TConsole * pC = mSubConsoleMap[key];
        if( ! pC )
            return ""; //return value was false but a QString is needed not a boolean
        else
            return pC->getCurrentLine();
    }
    else
    {
        return QString("ERROR: mini console does not exist");
    }
}


int TConsole::getLastLineNumber()
{
    return buffer.getLastLineNumber();
}

void TConsole::moveCursorEnd()
{
    int y = buffer.getLastLineNumber();
    int x = buffer.line( y ).size()-1;
    x = x >= 0 ? x : 0;
    moveCursor( x, y );
}

bool TConsole::moveCursor( int x, int y )
{
    QPoint P( x, y );
    if( buffer.moveCursor( P ) )
    {
        mUserCursor.setX( x );
        mUserCursor.setY( y );
        return true;
    }
    else
    {
        return false;
    }
}


void TConsole::setUserWindow()
{
    mUserConsole = true;
}

int TConsole::select(const QString& text, int numOfMatch )
{
    if( mUserCursor.y()<0 ) return -1;
    if( mUserCursor.y()>=buffer.size() ) return -1;

    if( mudlet::debugMode ) {TDebug(QColor(Qt::darkMagenta), QColor(Qt::black)) << "\nline under current user cursor: ">>0; TDebug(QColor(Qt::red),QColor(Qt::black))<<mUserCursor.y()<<"#:">>0; TDebug(QColor(Qt::gray),QColor(Qt::black)) << buffer.line( mUserCursor.y() ) << "\n" >> 0;}

    int begin = -1;
    for( int i=0;i<numOfMatch; i++ )
    {
        QString li = buffer.line( mUserCursor.y() );
        if( li.size() < 1 ) continue;
        begin = li.indexOf( text, begin+1 );

        if( begin == -1 )
        {
            P_begin.setX( 0 );
            P_begin.setY( 0 );
            P_end.setX( 0 );
            P_end.setY( 0 );
            return -1;
        }
    }

    int end = begin + text.size();
    P_begin.setX( begin );
    P_begin.setY( mUserCursor.y() );
    P_end.setX( end );
    P_end.setY( mUserCursor.y() );

    if( mudlet::debugMode ) {TDebug(QColor(Qt::darkRed),QColor(Qt::black))<<"P_begin("<<P_begin.x()<<"/"<<P_begin.y()<<"), P_end("<<P_end.x()<<"/"<<P_end.y()<<") selectedText = " << buffer.line( mUserCursor.y() ).mid(P_begin.x(), P_end.x()-P_begin.x() ) <<"\n" >> 0;}
    return begin;
}

bool TConsole::selectSection( int from, int to )
{
    if( mudlet::debugMode )
    {
        if( mudlet::debugMode ) {TDebug(QColor(Qt::darkMagenta),QColor(Qt::black)) <<"\nselectSection("<<from<<","<<to<<"): line under current user cursor: " << buffer.line( mUserCursor.y() ) << "\n" >> 0;}
    }
    if( from < 0 )
    {
        return false;
    }
    if( mUserCursor.y() >= static_cast<int>(buffer.buffer.size()) )
    {
        return false;
    }
    int s = buffer.buffer[mUserCursor.y()].size();
    if( from > s || from + to > s )
    {
        return false;
    }
    P_begin.setX( from );
    P_begin.setY( mUserCursor.y() );
    P_end.setX( from + to );
    P_end.setY( mUserCursor.y() );

    if( mudlet::debugMode ){ TDebug(QColor(Qt::darkMagenta),QColor(Qt::black))<<"P_begin("<<P_begin.x()<<"/"<<P_begin.y()<<"), P_end("<<P_end.x()<<"/"<<P_end.y()<<") selectedText = " << buffer.line( mUserCursor.y() ).mid(P_begin.x(), P_end.x()-P_begin.x() ) <<"\n" >> 0;}
    return true;
}

void TConsole::setLink(const QString & linkText, QStringList & linkFunction, QStringList & linkHint )
{
    buffer.applyLink( P_begin, P_end, linkText, linkFunction, linkHint );
}

void TConsole::setBold( bool b )
{
    if( b )
        mFormatCurrent.flags |= TCHAR_BOLD;
    else
        mFormatCurrent.flags &= ~(TCHAR_BOLD);
    buffer.applyBold( P_begin, P_end, b );
}

void TConsole::setItalics( bool b )
{
    if( b )
        mFormatCurrent.flags |= TCHAR_ITALICS;
    else
        mFormatCurrent.flags &= ~(TCHAR_ITALICS);
    buffer.applyItalics( P_begin, P_end, b );
}

void TConsole::setUnderline( bool b )
{
    if( b )
        mFormatCurrent.flags |= TCHAR_UNDERLINE;
    else
        mFormatCurrent.flags &= ~(TCHAR_UNDERLINE);
    buffer.applyUnderline( P_begin, P_end, b );
}

void TConsole::setStrikeOut( bool b )
{
    if( b )
        mFormatCurrent.flags |= TCHAR_STRIKEOUT;
    else
        mFormatCurrent.flags &= ~(TCHAR_STRIKEOUT);
    buffer.applyStrikeOut( P_begin, P_end, b );
}

void TConsole::setFgColor( int r, int g, int b )
{
    mFormatCurrent.fgR = r;
    mFormatCurrent.fgG = g;
    mFormatCurrent.fgB = b;
    buffer.applyFgColor( P_begin, P_end, r,g,b );
}

void TConsole::setBgColor( int r, int g, int b )
{
    mFormatCurrent.bgR = r;
    mFormatCurrent.bgG = g;
    mFormatCurrent.bgB = b;
    buffer.applyBgColor( P_begin, P_end, r,g,b );
}

void TConsole::printCommand( QString & msg )
{
    if( mTriggerEngineMode )
    {
        msg.append("\n");
        int lineBeforeNewContent = buffer.getLastLineNumber();
        if( lineBeforeNewContent >= 0 )
        {
            if( buffer.lineBuffer[lineBeforeNewContent].right(1) != "\n" )
            {
                msg.prepend("\n");
            }
        }
        buffer.appendLine( msg,
                           0,
                           msg.size()-1,
                           mCommandFgColor.red(),
                           mCommandFgColor.green(),
                           mCommandFgColor.blue(),
                           mCommandBgColor.red(),
                           mCommandBgColor.green(),
                           mCommandBgColor.blue(),
                           false,
                           false,
                           false,
                           false );
    }
    else
    {
        int lineBeforeNewContent = buffer.size()-2;
        if( lineBeforeNewContent >= 0 )
        {
            int promptEnd = buffer.buffer[lineBeforeNewContent].size();
            if( promptEnd < 0 )
            {
                promptEnd = 0;
            }
            if( buffer.promptBuffer[lineBeforeNewContent] == true )
            {
                QPoint P( promptEnd, lineBeforeNewContent );
                TChar format;
                format.fgR = mCommandFgColor.red();
                format.fgG = mCommandFgColor.green();
                format.fgB = mCommandFgColor.blue();
                format.bgR = mCommandBgColor.red();
                format.bgG = mCommandBgColor.green();
                format.bgB = mCommandBgColor.blue();
                buffer.insertInLine( P, msg, format );
                int down = buffer.wrapLine( lineBeforeNewContent,
                                            mpHost->mScreenWidth,
                                            mpHost->mWrapIndentCount,
                                            mFormatCurrent );

                console->needUpdate( lineBeforeNewContent, lineBeforeNewContent+1+down );
                console2->needUpdate( lineBeforeNewContent, lineBeforeNewContent+1+down );
                buffer.promptBuffer[lineBeforeNewContent] = false;
                return;
            }
        }
        msg.append("\n");
        print( msg, mCommandFgColor, mCommandBgColor );
    }
}

void TConsole::echoLink(const QString & text, QStringList & func, QStringList & hint, bool customFormat )
{
    if( customFormat )
        buffer.addLink( mTriggerEngineMode, text, func, hint, mFormatCurrent );
    else
    {
        if( ! mIsSubConsole && ! mIsDebugConsole )
        {
            TChar f = TChar( 0, 0, 255,
                             mpHost->mBgColor.red(), mpHost->mBgColor.green(), mpHost->mBgColor.blue(),
                             false, false, true, false );
            buffer.addLink( mTriggerEngineMode, text, func, hint, f );
        }
        else
        {
            TChar f = TChar(0, 0, 255,
                            mBgColor.red(), mBgColor.green(), mBgColor.blue(),
                            false, false, true, false);
            buffer.addLink( mTriggerEngineMode, text, func, hint, f );
        }
    }
}

void TConsole::echo(const QString & msg )
{
    if( mTriggerEngineMode )
    {
        buffer.appendLine( msg,
                           0,
                           msg.size()-1,
                           mFormatCurrent.fgR,
                           mFormatCurrent.fgG,
                           mFormatCurrent.fgB,
                           mFormatCurrent.bgR,
                           mFormatCurrent.bgG,
                           mFormatCurrent.bgB,
                           mFormatCurrent.flags & TCHAR_BOLD,
                           mFormatCurrent.flags & TCHAR_ITALICS,
                           mFormatCurrent.flags & TCHAR_UNDERLINE,
                           mFormatCurrent.flags & TCHAR_STRIKEOUT );
    }
    else
    {
        print( msg );
    }
}

void TConsole::print( const char * txt )
{
    QString msg = txt;
    buffer.append( msg,
                   0,
                   msg.size(),
                   mFormatCurrent.fgR,
                   mFormatCurrent.fgG,
                   mFormatCurrent.fgB,
                   mFormatCurrent.bgR,
                   mFormatCurrent.bgG,
                   mFormatCurrent.bgB,
                   mFormatCurrent.flags & TCHAR_BOLD,
                   mFormatCurrent.flags & TCHAR_ITALICS,
                   mFormatCurrent.flags & TCHAR_UNDERLINE,
                   mFormatCurrent.flags & TCHAR_STRIKEOUT );
    console->showNewLines();
    console2->showNewLines();
}

void TConsole::printDebug( QColor & c, QColor & d, const QString & msg )
{
    buffer.append( msg,
                   0,
                   msg.size(),
                   c.red(),
                   c.green(),
                   c.blue(),
                   d.red(),
                   d.green(),
                   d.blue(),
                   false,
                   false,
                   false,
                   false );

    console->showNewLines();
    console2->showNewLines();
}

TConsole * TConsole::createBuffer(const QString & name )
{
    std::string key = name.toLatin1().data();
    if( mSubConsoleMap.find( key ) == mSubConsoleMap.end() )
    {
        auto pC = new TConsole( mpHost, false );
        mSubConsoleMap[key] = pC;
        pC->setWindowTitle(name);
        pC->setContentsMargins(0,0,0,0);
        pC->hide();
        pC->layerCommandLine->hide();
        //pC->mpScrollBar->hide();
        pC->setUserWindow();
        return pC;
    }
    else
    {
        return 0;
    }
}

void TConsole::resetMainConsole()
{
    std::map<string, TConsole *>::const_iterator it;
    it = mSubConsoleMap.begin();
    for( ; it != mSubConsoleMap.end(); it++ )
    {
        QMap<QString, TConsole *> & dockWindowConsoleMap = mudlet::self()->mHostConsoleMap[mpHost];
        QString n = it->first.c_str();
        dockWindowConsoleMap.remove( n );
        (*it->second).close();
    }

    std::map<string, TLabel *>::const_iterator it2;
    for( it2 = mLabelMap.begin(); it2 != mLabelMap.end(); it2++ )
    {
        QMap<QString, TLabel *> & dockWindowConsoleMap = mudlet::self()->mHostLabelMap[mpHost];
        QString n = it2->first.c_str();
        dockWindowConsoleMap.remove( n );
        (*it2->second).close();
    }
    mSubConsoleMap.clear();
    mLabelMap.clear();
}

TConsole * TConsole::createMiniConsole(const QString & name, int x, int y, int width, int height )
{
    std::string key = name.toLatin1().data();
    if( mSubConsoleMap.find( key ) == mSubConsoleMap.end() )
    {
        auto pC = new TConsole(mpHost, false, mpMainFrame );
        if( ! pC )
        {
            return 0;
        }
        mSubConsoleMap[key] = pC;
        pC->setObjectName( name );
        pC->setFocusPolicy( Qt::NoFocus );
        pC->setUserWindow();
        pC->console->setIsMiniConsole();
        pC->console2->setIsMiniConsole();
        pC->resize( width, height );
        pC->mOldX = x;
        pC->mOldY = y;
        pC->setContentsMargins(0,0,0,0);
        pC->move( x, y );
        std::string _n = name.toStdString();
        pC->setMiniConsoleFontSize( _n, 12 );
        pC->show();
        return pC;
    }
    else
    {
        return 0;
    }
}

TLabel * TConsole::createLabel(const QString & name, int x, int y, int width, int height, bool fillBackground )
{
    std::string key = name.toLatin1().data();
    if( mLabelMap.find( key ) == mLabelMap.end() )
    {
        auto pC = new TLabel( mpMainFrame );
        mLabelMap[key] = pC;
        pC->setObjectName( name );
        pC->setAutoFillBackground( fillBackground );
        pC->resize( width, height );
        pC->setContentsMargins(0,0,0,0);
        pC->move( x, y );
        pC->show();
        return pC;
    }
    else
    {
        return 0;
    }
}

void TConsole::createMapper( int x, int y, int width, int height )
{
    if( ! mpMapper )
    {
        mpMapper = new dlgMapper( mpMainFrame, mpHost, mpHost->mpMap.data() );
        mpHost->mpMap->mpM = mpMapper->glWidget;
        mpHost->mpMap->mpHost = mpHost;
        mpHost->mpMap->mpMapper = mpMapper;
        qDebug() << "TConsole::createMapper() - restore map case 2.";
        mpHost->mpMap->pushErrorMessagesToFile( tr( "Pre-Map loading(2) report" ), true );
        QDateTime now( QDateTime::currentDateTime() );

        if( mpHost->mpMap->restore( QString() ) ) {
            mpHost->mpMap->audit();
            mpMapper->mp2dMap->init();
            mpMapper->updateAreaComboBox();
            mpMapper->resetAreaComboBoxToPlayerRoomArea();
        }

        mpHost->mpMap->pushErrorMessagesToFile( tr( "Loading map(2) at %1 report" ).arg( now.toString( Qt::ISODate ) ), true );

        TEvent mapOpenEvent;
        mapOpenEvent.mArgumentList.append(QLatin1String("mapOpenEvent"));
        mapOpenEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mpHost->raiseEvent( mapOpenEvent );
    }
    mpMapper->resize( width, height );
    mpMapper->move( x, y );
    mpMapper->mp2dMap->gridMapSizeChange = true; //mapper size has changed, but only init grid map when necessary
    mpMapper->show();
}

bool TConsole::createButton(const QString & name, int x, int y, int width, int height, bool fillBackground )
{
    std::string key = name.toLatin1().data();
    if( mLabelMap.find( key ) == mLabelMap.end() )
    {
        auto pC = new TLabel( mpMainFrame );
        mLabelMap[key] = pC;
        pC->setObjectName( name );
        pC->setAutoFillBackground( fillBackground );
        pC->resize( width, height );
        pC->setContentsMargins(0,0,0,0);
        pC->move( x, y );
        pC->show();
        return true;
    }
    else
        return false;
}

bool TConsole::setBackgroundImage(const QString & name, const QString & path )
{
    std::string key = name.toLatin1().data();
    if( mLabelMap.find( key ) != mLabelMap.end() )
    {
        QPixmap bgPixmap( path );
        mLabelMap[key]->setPixmap( bgPixmap );
        return true;
    }
    else
        return false;
}

bool TConsole::setBackgroundColor(const QString & name, int r, int g, int b, int alpha )
{
    std::string key = name.toLatin1().data();
    if( mSubConsoleMap.find( key ) != mSubConsoleMap.end() )
    {
        QPalette mainPalette;
        mainPalette.setColor( QPalette::Window, QColor(r, g, b, alpha) );
        mSubConsoleMap[key]->setPalette( mainPalette );
        mSubConsoleMap[key]->console->mBgColor = QColor( r,g,b,alpha);
        mSubConsoleMap[key]->console2->mBgColor = QColor( r,g,b,alpha);
        return true;
    }
    else if( mLabelMap.find( key ) != mLabelMap.end() )
    {
        QPalette mainPalette;
        mainPalette.setColor( QPalette::Window, QColor(r, g, b, alpha) );
        mLabelMap[key]->setPalette( mainPalette );
        return true;
    }
    else
        return false;

}

bool TConsole::raiseWindow(const QString & name )
{
    std::string key = name.toLatin1().data();
    if( mSubConsoleMap.find( key ) != mSubConsoleMap.end() )
    {
        mSubConsoleMap[key]->raise();
        return true;
    }
    else if( mLabelMap.find( key ) != mLabelMap.end() )
    {
        mLabelMap[key]->raise();
        return true;
    }
    else
        return false;
}

bool TConsole::lowerWindow(const QString & name )
{
    std::string key = name.toLatin1().data();
    if( mSubConsoleMap.find( key ) != mSubConsoleMap.end() )
    {
        mSubConsoleMap[key]->lower();
        mpMainDisplay->lower();
        return true;
    }
    else if( mLabelMap.find( key ) != mLabelMap.end() )
    {
        mLabelMap[key]->lower();
        mpMainDisplay->lower();
        return true;
    }
    else
        return false;
}

bool TConsole::showWindow(const QString & name )
{
    std::string key = name.toLatin1().data();
    if( mSubConsoleMap.find( key ) != mSubConsoleMap.end() )
    {
        mSubConsoleMap[key]->console->updateScreenView();
        mSubConsoleMap[key]->console->forceUpdate();
        mSubConsoleMap[key]->show();

        mSubConsoleMap[key]->console2->updateScreenView();
        mSubConsoleMap[key]->console2->forceUpdate();
        //mSubConsoleMap[key]->move(mSubConsoleMap[key]->mOldX, mSubConsoleMap[key]->mOldY);
        return true;
    }
    else if( mLabelMap.find( key ) != mLabelMap.end() )
    {
        mLabelMap[key]->show();
        return true;
    }
    else
        return false;
}

bool TConsole::hideWindow(const QString & name )
{
    std::string key = name.toLatin1().data();
    if( mSubConsoleMap.find( key ) != mSubConsoleMap.end() )
    {
        //mSubConsoleMap[key]->move(9999,9999);
        mSubConsoleMap[key]->hide();
        return true;
    }
    else if( mLabelMap.find( key ) != mLabelMap.end() )
    {
        mLabelMap[key]->hide();
        return true;
    }
    else
        return false;
}

bool TConsole::printWindow(const QString & name, const QString & text )
{
    std::string key = name.toLatin1().data();
    if( mSubConsoleMap.find( key ) != mSubConsoleMap.end() )
    {
        mSubConsoleMap[key]->print( text );
        return true;
    }
    else if( mLabelMap.find( key ) != mLabelMap.end() )
    {
        mLabelMap[key]->setText( text );
        return true;
    }
    else
        return false;
}

void TConsole::print(const QString & msg )
{
    buffer.append(  msg,
                    0,
                    msg.size(),
                    mFormatCurrent.fgR,
                    mFormatCurrent.fgG,
                    mFormatCurrent.fgB,
                    mFormatCurrent.bgR,
                    mFormatCurrent.bgG,
                    mFormatCurrent.bgB,
                    mFormatCurrent.flags & TCHAR_BOLD,
                    mFormatCurrent.flags & TCHAR_ITALICS,
                    mFormatCurrent.flags & TCHAR_UNDERLINE,
                    mFormatCurrent.flags & TCHAR_STRIKEOUT );
    console->showNewLines();
    console2->showNewLines();
}

void TConsole::print(const QString & msg, const QColor fgColor, const QColor bgColor )
{
    buffer.append(  msg,
                    0,
                    msg.size(),
                    fgColor.red(),
                    fgColor.green(),
                    fgColor.blue(),
                    bgColor.red(),
                    bgColor.green(),
                    bgColor.blue(),
                    false,
                    false,
                    false,
                    false );
    console->showNewLines();
    console2->showNewLines();
}


void TConsole::printSystemMessage(const QString & msg )
{
    assert( mpHost );

    QColor bgColor;
    QColor fgColor;

    if( mIsDebugConsole )
    {
        bgColor = mBgColor;
        fgColor = mFgColor;
    }
    else
    {
        bgColor = mpHost->mBgColor;
    }

    //int lineBeforeNewContent = buffer.getLastLineNumber();
    QString txt = QString("System Message: ")+msg;
    buffer.append(  txt,
                    0,
                    txt.size(),
                    mSystemMessageFgColor.red(),
                    mSystemMessageFgColor.green(),
                    mSystemMessageFgColor.blue(),
                    mSystemMessageBgColor.red(),
                    mSystemMessageBgColor.green(),
                    mSystemMessageBgColor.blue(),
                    false,
                    false,
                    false,
                    false );
    console->showNewLines();
    console2->showNewLines();
}

void TConsole::echoUserWindow(const QString & msg )
{
    print( msg );
}

void TConsole::copy()
{
    mpHost->mpConsole->mClipboard = buffer.copy( P_begin, P_end );
}

void TConsole::cut()
{
    mClipboard = buffer.cut( P_begin, P_end );
}

void TConsole::paste()
{

    if( buffer.size()-1 > mUserCursor.y() )
    {
        buffer.paste( mUserCursor, mClipboard );
        console->needUpdate( mUserCursor.y(), mUserCursor.y() );
    }
    else
    {
        buffer.appendBuffer( mClipboard );
    }
    console->showNewLines();
    console2->showNewLines();
}

void TConsole::pasteWindow( TBuffer bufferSlice )
{
    mClipboard = bufferSlice;
    paste();
}

void TConsole::appendBuffer()
{
    buffer.appendBuffer( mClipboard );
    console->showNewLines();
    console2->showNewLines();
}

void TConsole::appendBuffer( TBuffer bufferSlice )
{
    buffer.appendBuffer( bufferSlice );
    console->showNewLines();
    console2->showNewLines();
}


void TConsole::slot_stop_all_triggers( bool b )
{
    if( b )
    {
        mpHost->stopAllTriggers();
        emergencyStop->setIcon( QIcon( QStringLiteral( ":/icons/red-bomb.png" ) ) );
    }
    else
    {
        mpHost->reenableAllTriggers();
        emergencyStop->setIcon( QIcon( QStringLiteral( ":/icons/edit-bomb.png" ) ) );
    }
}

void TConsole::showStatistics()
{
    QStringList header;
    header << "\n"
           << "+--------------------------------------------------------------+\n"
           << "|               system statistics                              |\n"
           << "+--------------------------------------------------------------+\n";

    QString h = header.join("");
    QString msg = h;
    print( msg, QColor(150, 120, 0), Qt::black );

    QString script = "setFgColor(190,150,0); setUnderline(true);echo([[\n\nGMCP events:\n]]);setUnderline(false);setFgColor(150,120,0);display( gmcp );";
    mpHost->mLuaInterpreter.compileAndExecuteScript( script );
    script = "setFgColor(190,150,0); setUnderline(true);echo([[\n\nATCP events:\n]]);setUnderline(false);setFgColor(150,120,0); display( atcp );";
    mpHost->mLuaInterpreter.compileAndExecuteScript( script );
    script = "setFgColor(190,150,0); setUnderline(true);echo([[\n\nchannel102 events:\n]]);setUnderline(false);setFgColor(150,120,0);display( channel102 );";
    mpHost->mLuaInterpreter.compileAndExecuteScript( script );


    script = "setFgColor(190,150,0); setUnderline(true); echo([[\n\nTrigger Report:\n\n]]); setBold(false);setUnderline(false);setFgColor(150,120,0)";
    mpHost->mLuaInterpreter.compileAndExecuteScript( script );

    QString r1 = mpHost->getTriggerUnit()->assembleReport();
    msg = r1;
    print( msg, QColor(150, 120, 0), Qt::black );
    script = "setFgColor(190,150,0); setUnderline(true);echo([[\n\nTimer Report:\n\n]]);setBold(false);setUnderline(false);setFgColor(150,120,0)";
    mpHost->mLuaInterpreter.compileAndExecuteScript( script );
    QString r2 = mpHost->getTimerUnit()->assembleReport();
    QString footer = QString("\n+--------------------------------------------------------------+\n" );
    msg = r2;
    print( msg, QColor(150, 120, 0), Qt::black );
    mpHost->mpConsole->print( footer, QColor(150, 120, 0), Qt::black );
    script = "resetFormat();";
    mpHost->mLuaInterpreter.compileAndExecuteScript( script );

    mpHost->mpConsole->raise();
}

void TConsole::slot_searchBufferUp()
{
    QString _txt = mpBufferSearchBox->text();
    if( _txt != mSearchQuery )
    {
        mSearchQuery = _txt;
        mCurrentSearchResult = buffer.lineBuffer.size();
    }
    if( buffer.lineBuffer.size() < 1 ) return;
    bool _found = false;
    for( int i=mCurrentSearchResult-1; i >= 0; i-- )
    {
        int begin = -1;
        do
        {
            begin = buffer.lineBuffer[i].indexOf(mSearchQuery, begin+1 );
            if( begin > -1 )
            {
                int length = mSearchQuery.size();
                moveCursor( 0, i );
                selectSection( begin, length );
                setBgColor( 255, 255, 0 );
                setFgColor( 0, 0, 0 );
                deselect();
                reset();
                _found = true;
            }
        }
        while( begin > -1 );
        if( _found )
        {
            scrollUp( buffer.mCursorY-i-3 );
            console->forceUpdate();
            mCurrentSearchResult = i;
            return;
        }
    }
    print("No search results, sorry!\n");
}

void TConsole::slot_searchBufferDown()
{
    QString _txt = mpBufferSearchBox->text();
    if( _txt != mSearchQuery )
    {
        mSearchQuery = _txt;
        mCurrentSearchResult = buffer.lineBuffer.size();
    }
    if( buffer.lineBuffer.size() < 1 ) return;
    if( mCurrentSearchResult >= buffer.lineBuffer.size() ) return;
    bool _found = false;
    for( int i=mCurrentSearchResult+1; i < buffer.lineBuffer.size(); i++ )
    {
        int begin = -1;
        do
        {
            begin = buffer.lineBuffer[i].indexOf(mSearchQuery, begin+1 );
            if( begin > -1 )
            {
                int length = mSearchQuery.size();
                moveCursor( 0, i );
                selectSection( begin, length );
                setBgColor( 255, 255, 0 );
                setFgColor( 0, 0, 0 );
                deselect();
                reset();
                _found = true;
            }
        }
        while( begin > -1 );
        if( _found )
        {
            scrollUp( buffer.mCursorY-i-3 );
            console->forceUpdate();
            mCurrentSearchResult = i;
            return;
        }
    }
    print("No search results, sorry!\n");
}

QSize TConsole::getMainWindowSize() const
{
    QSize consoleSize = size();
    int toolbarWidth = mpLeftToolBar->width() + mpRightToolBar->width();
    int toolbarHeight = mpTopToolBar->height();
    int commandLineHeight = mpCommandLine->height();
    QSize mainWindowSize( consoleSize.width()-toolbarWidth, consoleSize.height()-(commandLineHeight+toolbarHeight));
    return mainWindowSize;
}

void TConsole::slot_reloadMap( QList<QString> profilesList )
{
    Host * pHost = getHost();
    if( ! pHost ) {
        return;
    }

    QString ourName = pHost->getName();
    if( ! profilesList.contains( ourName ) ) {
        qDebug() << "TConsole::slot_reloadMap("
                 << profilesList
                 << ") request received but we:"
                 << ourName
                 << "are not mentioned - so we are ignoring it...!";
        return;
    }

    QString infoMsg = tr( "[ INFO ]  - Map reload request received from system..." );
    pHost->postMessage( infoMsg );

    QString outcomeMsg;
    if( loadMap( QString() ) ) {
        outcomeMsg = tr( "[  OK  ]  - ... System Map reload request completed." );
    }
    else {
        outcomeMsg = tr( "[ WARN ]  - ... System Map reload request failed." );
    }

    pHost->postMessage( outcomeMsg );
}
