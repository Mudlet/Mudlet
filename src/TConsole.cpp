/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn (KoehnHeiko@googlemail.com)    *
 *                                                                         *
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


#include <QMessageBox>
#include <QDebug>
#include "TConsole.h"
#include "mudlet.h"
#include <QScrollBar>
#include "TCommandLine.h"
#include <QVBoxLayout>
//#include <sys/time.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <time.h>
//#include <unistd.h>
#include <QTextCodec>
#include <QHostAddress>
#include <iostream>
#include <string>
#include <sstream>
#include <sys/types.h>
#include <stdio.h>
#include "TDebug.h"
#include "TTextEdit.h"
#include <QGraphicsSimpleTextItem>
#include "XMLexport.h"
#include <QShortcut>
#include "TLabel.h"
#include "TSplitter.h"
#include "TSplitterHandle.h"
#include <QDir>
#include "dlgNotepad.h"
#include <assert.h>
#include "dlgMapper.h"

using namespace std;

const QString TConsole::cmLuaLineVariable("line");

TConsole::TConsole( Host * pH, bool isDebugConsole, QWidget * parent )
: QWidget( parent )
, mpHost( pH )
, buffer( pH )
, emergencyStop( new QToolButton )
, layerCommandLine( 0 )
, mBgColor( QColor( 255, 255, 255 ) )
, mClipboard( mpHost )
, mCommandBgColor( QColor( 0, 0, 0 ) )
, mCommandFgColor( QColor( 213, 195, 0 ) )
, mConsoleName( "main" )
, mDisplayFont( QFont("Bitstream Vera Sans Mono", 10, QFont::Courier ) )//mDisplayFont( QFont("Monospace", 10, QFont::Courier ) )
, mFgColor( QColor( 0, 0, 0 ) )
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
, mSystemMessageFgColor( QColor( 255,0,0 ) )
, mTriggerEngineMode( false )
, mUserConsole( false )
, mWindowIsHidden( false )
, mWrapAt( 100 )
, networkLatency( new QLineEdit )
, mLastBufferLogLine( 0 )
, mUserAgreedToCloseConsole( false )
, mpBufferSearchBox( new QLineEdit )
, mpBufferSearchUp( new QToolButton )
, mpBufferSearchDown( new QToolButton )
, mCurrentSearchResult( 0 )
, mSearchQuery("")
{
    //mDisplayFont.setWordSpacing( 0 );
//    mDisplayFont.setLetterSpacing( QFont::AbsoluteSpacing, 0 );
    QShortcut * ps = new QShortcut(this);
    ps->setKey(Qt::CTRL + Qt::Key_W);
    ps->setContext(Qt::WidgetShortcut);

    if( mIsDebugConsole )
    {
        setWindowTitle("Debug Console");
        mWrapAt = 50;
        mIsSubConsole = false;
        mStandardFormat.bgR = mBgColor.red();
        mStandardFormat.bgG = mBgColor.green();
        mStandardFormat.bgB = mBgColor.blue();
        mStandardFormat.fgR = mFgColor.red();
        mStandardFormat.fgG = mFgColor.green();
        mStandardFormat.fgB = mFgColor.blue();
        mStandardFormat.bold = false;
        mStandardFormat.italics = false;
        mStandardFormat.underline = false;
    }
    else
    {
        setWindowTitle("keine Debug Console");
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
        mStandardFormat.bold = false;
        mStandardFormat.italics = false;
        mStandardFormat.underline = false;
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
    mainPalette.setColor( QPalette::Text, QColor(0,0,0) );
    mainPalette.setColor( QPalette::Highlight, QColor(55,55,255) );
    mainPalette.setColor( QPalette::Window, QColor(0,0,0,255) );
    QPalette splitterPalette;
    splitterPalette = mainPalette;
    splitterPalette.setColor( QPalette::Button, QColor(0,0,255,255) );
    splitterPalette.setColor( QPalette::Window, QColor(0,255,0));//,255) );
    splitterPalette.setColor( QPalette::Base, QColor(255,0,0,255) );
    splitterPalette.setColor( QPalette::Window, QColor(255,255,255) );
    //setPalette( mainPalette );

    //QVBoxLayout * layoutFrame = new QVBoxLayout( mainFrame );
    QPalette framePalette;
    framePalette.setColor( QPalette::Text, QColor(0,0,0) );
    framePalette.setColor( QPalette::Highlight, QColor(55,55,255) );
    framePalette.setColor( QPalette::Window, QColor(0,0,0,255) );
    mpMainFrame->setPalette( framePalette );
    mpMainFrame->setAutoFillBackground(true);
    mpMainFrame->setContentsMargins(0,0,0,0);
    QVBoxLayout * centralLayout = new QVBoxLayout;
    setLayout( centralLayout );
    QVBoxLayout * baseVFrameLayout = new QVBoxLayout;
    mpBaseVFrame->setLayout( baseVFrameLayout );
    baseVFrameLayout->setMargin( 0 );
    baseVFrameLayout->setSpacing( 0 );
    centralLayout->addWidget( mpBaseVFrame );
    QHBoxLayout * baseHFrameLayout = new QHBoxLayout;
    mpBaseHFrame->setLayout( baseHFrameLayout );
    baseHFrameLayout->setMargin( 0 );
    baseHFrameLayout->setSpacing( 0 );
    layout()->setSpacing( 0 );
    layout()->setMargin( 0 );
    setContentsMargins( 0, 0, 0, 0 );

    QHBoxLayout * topBarLayout = new QHBoxLayout;
    mpTopToolBar->setLayout( topBarLayout );
    mpTopToolBar->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
    mpTopToolBar->setContentsMargins(0,0,0,0);
    mpTopToolBar->setAutoFillBackground(true);
    QPalette topbarPalette;
    topbarPalette.setColor( QPalette::Text, QColor(255,255,255) );
    topbarPalette.setColor( QPalette::Highlight, QColor(55,55,255) );
    topbarPalette.setColor( QPalette::Window, QColor(0,255,0,255) );
    topbarPalette.setColor( QPalette::Base, QColor(0,255,0,255) );
    //mpTopToolBar->setPalette(topbarPalette);


    topBarLayout->setMargin( 0 );
    topBarLayout->setSpacing(0);
    QVBoxLayout * leftBarLayout = new QVBoxLayout;
    mpLeftToolBar->setLayout( leftBarLayout );
    mpLeftToolBar->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding));
    mpLeftToolBar->setAutoFillBackground(true);
    leftBarLayout->setMargin( 0 );
    leftBarLayout->setSpacing(0);
    mpLeftToolBar->setContentsMargins(0,0,0,0);
    QVBoxLayout * rightBarLayout = new QVBoxLayout;
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
    QWidget * mpCorePane = new QWidget( mpBaseHFrame );
    QVBoxLayout * coreSpreadLayout = new QVBoxLayout;
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
    QVBoxLayout * layout = new QVBoxLayout;
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

    QHBoxLayout * layoutLayer = new QHBoxLayout;
    layer->setLayout( layoutLayer );
    layoutLayer->setMargin( 0 );//neu rc1
    layoutLayer->setSpacing( 0 );//neu rc1
    layoutLayer->setMargin( 0 );//neu rc1
    QSizePolicy sizePolicyLayer(QSizePolicy::Expanding, QSizePolicy::Expanding);

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

    QHBoxLayout * layoutLayer2 = new QHBoxLayout( layerCommandLine );
    layoutLayer2->setMargin(0);
    layoutLayer2->setSpacing(0);

    QWidget * buttonMainLayer = new QWidget;//( layerCommandLine );
    buttonMainLayer->setSizePolicy(sizePolicy);
    buttonMainLayer->setContentsMargins(0,0,0,0);
    QVBoxLayout * layoutButtonMainLayer = new QVBoxLayout( buttonMainLayer );
    layoutButtonMainLayer->setMargin(0);
    layoutButtonMainLayer->setContentsMargins(0,0,0,0);

    layoutButtonMainLayer->setSpacing(0);
    /*buttonMainLayer->setMinimumHeight(31);
    buttonMainLayer->setMaximumHeight(31);*/
    QWidget * buttonLayer = new QWidget;
    QGridLayout * layoutButtonLayer = new QGridLayout( buttonLayer );
    layoutButtonLayer->setMargin(0);
    layoutButtonLayer->setSpacing(0);

    QWidget * buttonLayerSpacer = new QWidget(buttonLayer);
    buttonLayerSpacer->setSizePolicy( sizePolicy4 );
    layoutButtonMainLayer->addWidget( buttonLayerSpacer );
    layoutButtonMainLayer->addWidget( buttonLayer );

    QToolButton * timeStampButton = new QToolButton;
    timeStampButton->setCheckable( true );
    timeStampButton->setMinimumSize(QSize(30,30));
    timeStampButton->setMaximumSize(QSize(30,30));
    timeStampButton->setSizePolicy( sizePolicy5 );
    timeStampButton->setFocusPolicy( Qt::NoFocus );
    timeStampButton->setToolTip("Show Time Stamps");
    QIcon icon(":/icons/dialog-information.png");
    timeStampButton->setIcon( icon );
    connect( timeStampButton, SIGNAL(pressed()), console, SLOT(slot_toggleTimeStamps()));

    QToolButton * replayButton = new QToolButton;
    replayButton->setCheckable( true );
    replayButton->setMinimumSize(QSize(30,30));
    replayButton->setMaximumSize(QSize(30,30));
    replayButton->setSizePolicy( sizePolicy5 );
    replayButton->setFocusPolicy( Qt::NoFocus );
    replayButton->setToolTip("record a replay");
    QIcon icon4(":/icons/media-tape.png");
    replayButton->setIcon( icon4 );
    connect( replayButton, SIGNAL(pressed()), this, SLOT(slot_toggleReplayRecording()));

    logButton = new QToolButton;
    logButton->setMinimumSize(QSize(30, 30));
    logButton->setMaximumSize(QSize(30, 30));
    logButton->setCheckable( true );
    logButton->setSizePolicy( sizePolicy5 );
    logButton->setFocusPolicy( Qt::NoFocus );
    logButton->setToolTip("start logging MUD output to log file");
    QIcon icon3(":/icons/folder-downloads.png");
    logButton->setIcon( icon3 );
    connect( logButton, SIGNAL(pressed()), this, SLOT(slot_toggleLogging()));

    networkLatency->setReadOnly( true );
    networkLatency->setSizePolicy( sizePolicy4 );
    networkLatency->setFocusPolicy( Qt::NoFocus );
    networkLatency->setToolTip("<i>N:</i> is the latency of the MUD server and network (aka ping, in seconds), <br><i>S:</i> is the system processing time - how long your triggers took to process the last line(s)");
    networkLatency->setMaximumSize( 120, 30 );
    networkLatency->setMinimumSize( 120, 30 );
    networkLatency->setAutoFillBackground( true );
    networkLatency->setContentsMargins(0,0,0,0);
    QPalette basePalette;
    basePalette.setColor( QPalette::Text, QColor(0,0,0) );
    basePalette.setColor( QPalette::Base, QColor(255, 255, 255) );
    networkLatency->setPalette( basePalette );
    networkLatency->setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );



    QFont latencyFont = QFont("Bitstream Vera Sans Mono", 10, QFont::Courier);
    int width;
    int maxWidth = 120;
    width = QFontMetrics( latencyFont ).boundingRect(QString("N:0.000 S:0.000")).width();
    if( width < maxWidth )
    {
        networkLatency->setFont( latencyFont );
    }
    else
    {
         QFont latencyFont2 = QFont("Bitstream Vera Sans Mono", 9, QFont::Courier);
         width = QFontMetrics( latencyFont2 ).boundingRect(QString("N:0.000 S:0.000")).width();
         if( width < maxWidth )
         {
             networkLatency->setFont( latencyFont2 );
         }
         else
         {
             QFont latencyFont3 = QFont("Bitstream Vera Sans Mono", 8, QFont::Courier);
             width = QFontMetrics( latencyFont3 ).boundingRect(QString("N:0.000 S:0.000")).width();
             networkLatency->setFont( latencyFont3 );
         }
    }

    QIcon icon2(":/icons/edit-bomb.png");
    emergencyStop->setMinimumSize(QSize(30,30));
    emergencyStop->setMaximumSize(QSize(30,30));
    emergencyStop->setIcon( icon2 );
    emergencyStop->setSizePolicy( sizePolicy4 );
    emergencyStop->setFocusPolicy( Qt::NoFocus );
    emergencyStop->setCheckable( true );
    emergencyStop->setToolTip("Emergency Stop. Stop All Timers and Triggers");
    connect( emergencyStop, SIGNAL(clicked(bool)), this, SLOT(slot_stop_all_triggers( bool )));

    mpBufferSearchBox->setMinimumSize(QSize(100,30));
    mpBufferSearchBox->setMaximumSize(QSize(150,30));
    mpBufferSearchBox->setSizePolicy( sizePolicy5 );
    mpBufferSearchBox->setFont(mpHost->mCommandLineFont);
    mpBufferSearchBox->setFocusPolicy( Qt::ClickFocus );
#if QT_VERSION >= 0x040700
    mpBufferSearchBox->setPlaceholderText("Search ...");
#endif
    QPalette __pal;
    __pal.setColor(QPalette::Text, mpHost->mCommandLineFgColor );//QColor(0,0,192));
    __pal.setColor(QPalette::Highlight,QColor(0,0,192));
    __pal.setColor(QPalette::HighlightedText, QColor(255,255,255));
    __pal.setColor(QPalette::Base,mpHost->mCommandLineBgColor);//QColor(255,255,225));
    __pal.setColor(QPalette::Window, mpHost->mCommandLineBgColor);
    mpBufferSearchBox->setPalette( __pal );
    mpBufferSearchBox->setToolTip("Search buffer");
    connect( mpBufferSearchBox, SIGNAL(returnPressed()), this, SLOT(slot_searchBufferUp()));




    mpBufferSearchUp->setMinimumSize(QSize(30,30));
    mpBufferSearchUp->setMaximumSize(QSize(30,30));
    mpBufferSearchUp->setSizePolicy( sizePolicy5 );
    mpBufferSearchUp->setFocusPolicy( Qt::NoFocus );
    mpBufferSearchUp->setToolTip("next result");
    mpBufferSearchUp->setFocusPolicy( Qt::NoFocus );
    QIcon icon34(":/icons/export.png");
    mpBufferSearchUp->setIcon( icon34 );
    connect( mpBufferSearchUp, SIGNAL(clicked()), this, SLOT(slot_searchBufferUp()));


    mpBufferSearchDown->setMinimumSize(QSize(30,30));
    mpBufferSearchDown->setMaximumSize(QSize(30,30));
    mpBufferSearchDown->setSizePolicy( sizePolicy5 );
    mpBufferSearchDown->setFocusPolicy( Qt::NoFocus );
    mpBufferSearchDown->setToolTip("next result");
    mpBufferSearchDown->setFocusPolicy( Qt::NoFocus );
    QIcon icon35(":/icons/import.png");
    mpBufferSearchDown->setIcon( icon35 );
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


    setAttribute( Qt::WA_OpaquePaintEvent );//was disabled

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

}

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

        TEvent me;
        me.mArgumentList.append( "sysWindowResizeEvent" );
        me.mArgumentList.append( QString::number(x - mMainFrameLeftWidth - mMainFrameRightWidth) );
        me.mArgumentList.append( QString::number(y - mMainFrameTopHeight - mMainFrameBottomHeight - mpCommandLine->height()) );
        me.mArgumentList.append( mConsoleName );
        me.mArgumentTypeList.append( ARGUMENT_TYPE_STRING );
        me.mArgumentTypeList.append( ARGUMENT_TYPE_NUMBER );
        me.mArgumentTypeList.append( ARGUMENT_TYPE_NUMBER );
        me.mArgumentTypeList.append( ARGUMENT_TYPE_STRING );
        mpHost->raiseEvent( &me );
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
        if( ! mudlet::self()->isGoingDown() )
        {
            hide();
            mudlet::mpDebugArea->setVisible(false);
            mudlet::debugMode = false;
            event->ignore();
            return;
        }
        else
        {
            event->accept();
            return;
        }
    }
    if( mUserConsole )
    {
        if( ! mudlet::self()->isGoingDown() )
        {
            hide();
            event->ignore();
            return;
        }
        else
        {
            event->accept();
            return;
        }
    }

    if( profile_name != "default_host" )
    {
        TEvent conCloseEvent;
        conCloseEvent.mArgumentList.append( "sysExitEvent" );
        conCloseEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mpHost->raiseEvent( & conCloseEvent );

        if( mpHost->mFORCE_SAVE_ON_EXIT )
        {
            QString directory_xml = QDir::homePath()+"/.config/mudlet/profiles/"+profile_name+"/current";
            QString filename_xml = directory_xml + "/"+QDateTime::currentDateTime().toString("dd-MM-yyyy#hh-mm-ss")+".xml";
            QDir dir_xml;
            if( ! dir_xml.exists( directory_xml ) )
            {
                dir_xml.mkpath( directory_xml );
            }
            QFile file_xml( filename_xml );
            if( file_xml.open( QIODevice::WriteOnly ) )
            {
                mpHost->modulesToWrite.clear();
                XMLexport writer( mpHost );
                writer.exportHost( & file_xml );
                file_xml.close();
                mpHost->saveModules(0);

            }
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
        ASK: int choice = QMessageBox::question( this, "Exiting Session: Question", "Do you want to save the profile "+profile_name, QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel );
        if( choice == QMessageBox::Cancel )
        {
            event->setAccepted(false);
            event->ignore();
            return;
        }
        if( choice == QMessageBox::Yes )
        {
            QString directory_xml = QDir::homePath()+"/.config/mudlet/profiles/"+profile_name+"/current";
            QString filename_xml = directory_xml + "/"+QDateTime::currentDateTime().toString("dd-MM-yyyy#hh-mm-ss")+".xml";
            QDir dir_xml;
            if( ! dir_xml.exists( directory_xml ) )
            {
                dir_xml.mkpath( directory_xml );
            }
            QFile file_xml( filename_xml );
            if( file_xml.open( QIODevice::WriteOnly ) )
            {
                /*XMLexport writer( mpHost );
                writer.exportHost( & file_xml );
                file_xml.close();*/
                mpHost->modulesToWrite.clear();
                XMLexport writer( mpHost );
                writer.exportHost( & file_xml );
                file_xml.close();
                mpHost->saveModules(0);
                if( mpHost->mpMap && mpHost->mpMap->mpRoomDB->size() > 0 )
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
            else
            {
                QMessageBox::critical( this, "ERROR: Not closing profile.", "Failed to save "+profile_name+" to location "+filename_xml+" because of the following error: "+file_xml.errorString() );
                goto ASK;
            }

        }
        else if( choice == QMessageBox::No )
        {
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

void TConsole::slot_toggleLogging()
{
    if( mIsDebugConsole ) return;
    mLogToLogFile = ! mLogToLogFile;
    //mpHost->mLogStatus = mLogToLogFile;
    if( mLogToLogFile )
    {
        QFile file( QDir::homePath()+"/.config/mudlet/autolog" );
        file.open( QIODevice::WriteOnly | QIODevice::Text );
        QTextStream out(&file);
        file.close();
    }
    else
    {
       QFile file( QDir::homePath()+"/.config/mudlet/autolog" );
       file.remove();
    }
    if( mLogToLogFile )
    {
        mLastBufferLogLine = buffer.size();
        QString directoryLogFile = QDir::homePath()+"/.config/mudlet/profiles/"+profile_name+"/log";
        QString mLogFileName = directoryLogFile + "/"+QDateTime::currentDateTime().toString("dd-MM-yyyy#hh-mm-ss");
        if( mpHost->mRawStreamDump )
        {
            mLogFileName.append(".html");
        }
        else
            mLogFileName.append(".txt");

        QDir dirLogFile;
        if( ! dirLogFile.exists( directoryLogFile ) )
        {
            dirLogFile.mkpath( directoryLogFile );
        }
        mLogFile.setFileName( mLogFileName );
        mLogFile.open( QIODevice::WriteOnly );
        mLogStream.setDevice( &mLogFile );
        if( mpHost->mRawStreamDump ) mLogStream << "<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.01//EN' 'http://www.w3.org/TR/html4/strict.dtd'><html><head><style><!-- *{ font-family: 'Courier New', 'Monospace', 'Courier';} *{ white-space: pre-wrap; } *{color:rgb(255,255,255);} *{background-color:rgb("<<mpHost->mBgColor.red()<<","<<mpHost->mBgColor.green()<<","<<mpHost->mBgColor.blue()<<");} --></style><meta http-equiv='content-type' content='text/html; charset=utf-8'></head><body>";
        QString message = QString("Logging has started. Log file is ") + mLogFile.fileName() + "\n";
        printSystemMessage( message );
    }
    else
    {
        if( mpHost->mRawStreamDump ) mLogStream << "</pre></body></html>";
        mLogFile.close();
        QString message = QString("Logging has been stopped. Log file is ") + mLogFile.fileName() + "\n";
        printSystemMessage( message );
    }
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
        QString message = QString("Replay recording has been stopped. File: ") + mLogFile.fileName() + "\n";
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
        palette.setColor( QPalette::Base, QColor(0,0,0) );
        console->setPalette( palette );
        console2->setPalette( palette );
    }
    else if( mIsSubConsole )
    {
        mDisplayFont.setStyleStrategy( (QFont::StyleStrategy)(QFont::NoAntialias | QFont::PreferQuality ) );
#if defined(Q_OS_MAC) || (defined(Q_OS_LINUX) && QT_VERSION >= 0x040800)
        QPixmap pixmap = QPixmap( 2000, 600 );
        QPainter p(&pixmap);
        mDisplayFont.setLetterSpacing( QFont::AbsoluteSpacing, 0 );
        p.setFont(mDisplayFont);
        const QRectF r = QRectF(0,0,2000, 600);
        QRectF r2;
        const QString t = "123";
        p.drawText(r,1,t,&r2);
        int mFontHeight = QFontMetrics( mDisplayFont ).height();
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
        pal.setColor(QPalette::HighlightedText, QColor(255,255,255));
        pal.setColor(QPalette::Base,mpHost->mCommandLineBgColor);//QColor(255,255,225));
        mpCommandLine->setPalette( pal );
        mpCommandLine->mRegularPalette = pal;
        if( mpHost->mNoAntiAlias )
            mpHost->mDisplayFont.setStyleStrategy( QFont::NoAntialias );
        else
            mpHost->mDisplayFont.setStyleStrategy( (QFont::StyleStrategy)( QFont::PreferAntialias | QFont::PreferQuality ) );
        mpHost->mDisplayFont.setFixedPitch(true);
        mDisplayFont.setFixedPitch(true);
#if defined(Q_OS_MAC) || (defined(Q_OS_LINUX) && QT_VERSION >= 0x040800)
        QPixmap pixmap = QPixmap( 2000, 600 );
        QPainter p(&pixmap);
        QFont _font = mpHost->mDisplayFont;
        _font.setLetterSpacing(QFont::AbsoluteSpacing, 0);
        p.setFont(_font);
        const QRectF r = QRectF(0,0,2000, 600);
        QRectF r2;
        const QString t = "123";
        p.drawText(r,1,t,&r2);
        int mFontHeight = QFontMetrics( mpHost->mDisplayFont ).height();
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
    }
    QPalette palette;
    palette.setColor( QPalette::Button, QColor(0,0,255) );
    palette.setColor( QPalette::Window, QColor(0,255,0) );
    palette.setColor( QPalette::Base, QColor(255,0,0) );

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

void TConsole::printOnDisplay( std::string & incomingSocketData )
{
    //buffer.messen();
    QString prompt ="";//FIXME

    mProcessingTime.restart();
    mTriggerEngineMode = true;
    buffer.translateToPlainText( incomingSocketData );
    mTriggerEngineMode = false;
//    if( mLogToLogFile )
//    {
//        if( ! mIsDebugConsole )
//        {
//            if( buffer.size() < mLastBufferLogLine )
//            {
//                mLastBufferLogLine -= buffer.mBatchDeleteSize;
//                qDebug()<<"---> RESETTING mLastBufferLogLine";
//                if( mLastBufferLogLine < 0 )
//                {
//                    mLastBufferLogLine = 0;
//                }
//            }
//            if( buffer.size() > mLastBufferLogLine + 1 )
//            {
//                for( int i=mLastBufferLogLine+1; i<buffer.size(); i++ )
//                {
//                    QString toLog;
//                    if( mpHost->mRawStreamDump )
//                    {
//                        QPoint P1 = QPoint(0,i);
//                        QPoint P2 = QPoint( buffer.buffer[i].size(), i);
//                        toLog = buffer.bufferToHtml(P1, P2);
//                    }
//                    else
//                    {
//                        toLog = buffer.lineBuffer[i];
//                        toLog.append("\n");
//                    }
//                    mLogStream << toLog;
//                    qDebug()<<"LOG:"<<i<<" lastLogLine="<<mLastBufferLogLine<<" size="<<buffer.size()<<" toLog<"<<toLog<<">";
//                    mLastBufferLogLine++;
//                }
//                mLastBufferLogLine--;
//            }
//            mLogStream.flush();
//        }
//    }
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

QString TConsole::assemble_html_font_specs()
{
    QString s;
    s = "</span><span style=\"";
    if( m_LoggerfontSpecs.m_fgColorHasChanged )
    {
        s+="color: rgb("+
            QString::number(m_LoggerfontSpecs.fgColor.red())+","+
            QString::number(m_LoggerfontSpecs.fgColor.green())+","+
            QString::number(m_LoggerfontSpecs.fgColor.blue()) + ");";
    }
    if( m_LoggerfontSpecs.m_bgColorHasChanged )
    {
        s += " background: rgb("+
            QString::number(m_LoggerfontSpecs.bgColor.red())+","+
            QString::number(m_LoggerfontSpecs.bgColor.green())+","+
            QString::number(m_LoggerfontSpecs.bgColor.blue()) +");";
    }
    s += " font-weight: " + m_LoggerfontSpecs.getFontWeight() +
        "; font-style: " + m_LoggerfontSpecs.getFontStyle() +
        "; font-decoration: " + m_LoggerfontSpecs.getFontDecoration() +
        "\">";
    return s;
}

void TConsole::logger_set_text_properties( QString tags )
{
    switch( tags.toInt() )
    {
    case 0:
        m_LoggerfontSpecs.reset();
        break;
    case 1:
        m_LoggerfontSpecs.bold = true;
        break;
    case 2:
        m_LoggerfontSpecs.bold = false;
        break;
    case 3:
        m_LoggerfontSpecs.italics = true;
        break;
    case 4:
        m_LoggerfontSpecs.underline = true;
    case 5:
        break; //FIXME support blinking
    case 6:
        break; //FIXME support fast blinking
    case 7:
        break; //FIXME support inverse
    case 9:
        break; //FIXME support strikethrough
    case 22:
        m_LoggerfontSpecs.bold = false;
        break;
    case 23:
        m_LoggerfontSpecs.italics = false;
        break;
    case 24:
        m_LoggerfontSpecs.underline = false;
        break;
    case 27:
        break; //FIXME inverse off
    case 29:
        break; //FIXME
    case 30:
        m_LoggerfontSpecs.fgColor = mpHost->mBlack;
        m_LoggerfontSpecs.fgColorLight = mpHost->mLightBlack;
        m_LoggerfontSpecs.fg_color_change();
        break;
    case 31:
        m_LoggerfontSpecs.fgColor = mpHost->mRed;
        m_LoggerfontSpecs.fgColorLight = mpHost->mLightRed;
        m_LoggerfontSpecs.fg_color_change();
        break;
    case 32:
        m_LoggerfontSpecs.fgColor = mpHost->mGreen;
        m_LoggerfontSpecs.fgColorLight = mpHost->mLightGreen;
        m_LoggerfontSpecs.fg_color_change();
        break;
    case 33:
        m_LoggerfontSpecs.fgColor = mpHost->mYellow;
        m_LoggerfontSpecs.fgColorLight = mpHost->mLightYellow;
        m_LoggerfontSpecs.fg_color_change();
        break;
    case 34:
        m_LoggerfontSpecs.fgColor = mpHost->mBlue;
        m_LoggerfontSpecs.fgColorLight = mpHost->mLightBlue;
        m_LoggerfontSpecs.fg_color_change();
        break;
    case 35:
        m_LoggerfontSpecs.fgColor = mpHost->mMagenta;
        m_LoggerfontSpecs.fgColorLight = mpHost->mLightMagenta;
        m_LoggerfontSpecs.fg_color_change();
        break;
    case 36:
        m_LoggerfontSpecs.fgColor = mpHost->mCyan;
        m_LoggerfontSpecs.fgColorLight = mpHost->mLightCyan;
        m_LoggerfontSpecs.fg_color_change();
        break;
    case 37:
        m_LoggerfontSpecs.fgColor = mpHost->mWhite;
        m_LoggerfontSpecs.fgColorLight = mpHost->mLightWhite;
        m_LoggerfontSpecs.fg_color_change();
        break;
    case 39:
        m_LoggerfontSpecs.bgColor = mpHost->mBgColor;//mWhite
        m_LoggerfontSpecs.bg_color_change();
        break;
    case 40:
        m_LoggerfontSpecs.bgColor = mpHost->mBlack;
        m_LoggerfontSpecs.bg_color_change();
        break;
    case 41:
        m_LoggerfontSpecs.bgColor = mpHost->mRed;
        m_LoggerfontSpecs.bg_color_change();
        break;
    case 42:
        m_LoggerfontSpecs.bgColor = mpHost->mGreen;
        m_LoggerfontSpecs.bg_color_change();
        break;
    case 43:
        m_LoggerfontSpecs.bgColor = mpHost->mYellow;
        m_LoggerfontSpecs.bg_color_change();
        break;
    case 44:
        m_LoggerfontSpecs.bgColor = mpHost->mBlue;
        m_LoggerfontSpecs.bg_color_change();
        break;
    case 45:
        m_LoggerfontSpecs.bgColor = mpHost->mMagenta;
        m_LoggerfontSpecs.bg_color_change();
        break;
    case 46:
        m_LoggerfontSpecs.bgColor = mpHost->mCyan;
        m_LoggerfontSpecs.bg_color_change();
        break;
    case 47:
        m_LoggerfontSpecs.bgColor = mpHost->mWhite;
        m_LoggerfontSpecs.bg_color_change();
        break;
    };
}


QString TConsole::logger_translate( QString & s )
{
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
      28 strikethrough off
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


    //s.replace(QChar('\\'), "\\\\");
    s.replace(QChar('\n'), "<br />");
    s.replace(QChar('\t'), "     ");
    int sequence_begin = 0;
    int sequence_end = 0;
    QString sequence;
    while( (sequence_begin = s.indexOf(QString("\033["),0) ) != -1 )
    {
        sequence_end = s.indexOf(QChar('m'),sequence_begin);
        int sequence_length = abs(sequence_begin - sequence_end )+1;
        if( sequence_end != -1 )
        {
            sequence = s.mid(sequence_begin+2,sequence_length-3); // weil 3 elemente ausgelassen werden
            QStringList textPropertyList;
            if( sequence.indexOf(QChar(';'),0) )
            {
                textPropertyList = sequence.split(QChar(';'),QString::SkipEmptyParts);
            }
            else
            {
                textPropertyList << sequence;
            }
            for( int i=0; i<textPropertyList.size(); i++ )
            {
                m_LoggerfontSpecs.m_fgColorHasChanged = false;
                m_LoggerfontSpecs.m_bgColorHasChanged = false;
                logger_set_text_properties(textPropertyList[i]);
                //            qDebug()<<"set property:"<<textPropertyList[i];
            }
            QString html_tags = assemble_html_font_specs();
            s.replace(sequence_begin,sequence_length,html_tags);
        }
        else
        {
            break; // sequenzende befindet sich im naechsten tcp/ip packet
        }
    }

    return s;
}


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
    mFormatCurrent.bold = false;
    mFormatCurrent.italics = false;
    mFormatCurrent.underline = false;
}

void TConsole::insertLink( QString text, QStringList & func, QStringList & hint, QPoint P, bool customFormat )
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
                TChar _f = TChar(0,0,255,mBgColor.red(), mBgColor.green(), mBgColor.blue(), false, false, true );
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
                TChar _f = TChar(0,0,255,mBgColor.red(), mBgColor.green(), mBgColor.blue(), false, false, true );
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
                TChar _f = TChar(0,0,255,mBgColor.red(), mBgColor.green(), mBgColor.blue(), false, false, true );
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
                TChar _f = TChar(0,0,255,mBgColor.red(), mBgColor.green(), mBgColor.blue(), false, false, true );
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

void TConsole::insertText( QString text, QPoint P )
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
                           mFormatCurrent.bold,
                           mFormatCurrent.italics,
                           mFormatCurrent.underline );
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


void TConsole::replace( QString text )
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

bool TConsole::saveMap(QString location)
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

bool TConsole::loadMap(QString location)
{
    if( !mpHost ) return false;
    if( !mpHost->mpMap || !mpHost->mpMap->mpMapper )
    {
        mudlet::self()->slot_mapper();
    }
    if( !mpHost->mpMap || !mpHost->mpMap->mpMapper ) return false;

    mpHost->mpMap->mapClear();

    if ( mpHost->mpMap->restore(location) )
    {
        mpHost->mpMap->init( mpHost );
        mpHost->mpMap->mpMapper->mp2dMap->init();
        mpHost->mpMap->mpMapper->show();
        if( mpHost->mpMap )
            if( mpHost->mpMap->mpMapper )
                mpHost->mpMap->mpMapper->updateAreaComboBox();
        // previous selections stay, so we need to clear it
        //mpHost->mpMap->mpMapper->mp2dMap->deselect();
        return true;
    }

    return false;
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

void TConsole::insertText( QString msg )
{
    insertText( msg, mUserCursor );
}

void TConsole::insertLink( QString text, QStringList & func, QStringList & hint, bool customFormat )
{
    insertLink( text, func, hint, mUserCursor, customFormat );
}

void TConsole::insertHTML( QString text )
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
        pC->console->mDisplayFont = QFont("Bitstream Vera Sans Mono", size, QFont::Courier);
        pC->console->updateScreenView();
        pC->console->forceUpdate();
        pC->console2->mDisplayFont = QFont("Bitstream Vera Sans Mono", size, QFont::Courier);
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
        if( ! pC ) return false;
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

int TConsole::select( QString text, int numOfMatch )
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

void TConsole::setLink( QString & linkText, QStringList & linkFunction, QStringList & linkHint )
{
    buffer.applyLink( P_begin, P_end, linkText, linkFunction, linkHint );
}

void TConsole::setBold( bool b )
{
    mFormatCurrent.bold = b;
    buffer.applyBold( P_begin, P_end, b );
}

void TConsole::setItalics( bool b )
{
    mFormatCurrent.italics = b;
    buffer.applyItalics( P_begin, P_end, b );
}

void TConsole::setUnderline( bool b )
{
    mFormatCurrent.underline = b;
    buffer.applyUnderline( P_begin, P_end, b );
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
        print( msg, mCommandFgColor.red(), mCommandFgColor.green(), mCommandFgColor.blue(), mCommandBgColor.red(), mCommandBgColor.green(), mCommandBgColor.blue() );
    }
}

void TConsole::echoLink( QString & text, QStringList & func, QStringList & hint, bool customFormat )
{
    if( customFormat )
        buffer.addLink( mTriggerEngineMode, text, func, hint, mFormatCurrent );
    else
    {
        if( ! mIsSubConsole && ! mIsDebugConsole )
        {
            TChar f = TChar(0, 0, 255, mpHost->mBgColor.red(), mpHost->mBgColor.green(), mpHost->mBgColor.blue(), false, false, true);
            buffer.addLink( mTriggerEngineMode, text, func, hint, f );
        }
        else
        {
            TChar f = TChar(0, 0, 255, mBgColor.red(), mBgColor.green(), mBgColor.blue(), false, false, true);
            buffer.addLink( mTriggerEngineMode, text, func, hint, f );
        }
    }
}

void TConsole::echo( QString & msg )
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
                           mFormatCurrent.bold,
                           mFormatCurrent.italics,
                           mFormatCurrent.underline );
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
                   mFormatCurrent.bold,
                   mFormatCurrent.italics,
                   mFormatCurrent.underline );
    console->showNewLines();
    console2->showNewLines();
}

void TConsole::printDebug( QColor & c, QColor & d, QString & msg )
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
                   false );

    console->showNewLines();
    console2->showNewLines();
}

TConsole * TConsole::createBuffer( QString & name )
{
    std::string key = name.toLatin1().data();
    if( mSubConsoleMap.find( key ) == mSubConsoleMap.end() )
    {
        TConsole * pC = new TConsole( mpHost, false );
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

TConsole * TConsole::createMiniConsole( QString & name, int x, int y, int width, int height )
{
    std::string key = name.toLatin1().data();
    if( mSubConsoleMap.find( key ) == mSubConsoleMap.end() )
    {
        TConsole * pC = new TConsole(mpHost, false, mpMainFrame );
        if( ! pC )
        {
            return 0;
        }
        mSubConsoleMap[key] = pC;
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

TLabel * TConsole::createLabel( QString & name, int x, int y, int width, int height, bool fillBackground )
{
    std::string key = name.toLatin1().data();
    if( mLabelMap.find( key ) == mLabelMap.end() )
    {
        TLabel * pC = new TLabel( mpMainFrame );
        mLabelMap[key] = pC;
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
        mpMapper = new dlgMapper( mpMainFrame, mpHost, mpHost->mpMap );
        mpHost->mpMap->mpM = mpMapper->glWidget;
        mpHost->mpMap->mpHost = mpHost;
        mpHost->mpMap->mpMapper = mpMapper;
        mpMapper->mpHost = mpHost;
        mpHost->mpMap->restore("");
        mpHost->mpMap->init( mpHost );

        TEvent mapOpenEvent;
        mapOpenEvent.mArgumentList.append( "mapOpenEvent" );
        mapOpenEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mpHost->raiseEvent( & mapOpenEvent );
    }
    mpMapper->resize( width, height );
    mpMapper->move( x, y );
    //mpMapper->mp2dMap->init();
    mpMapper->mp2dMap->gridMapSizeChange = true; //mapper size has changed, but only init grid map when necessary
    mpMapper->show();
}

bool TConsole::createButton( QString & name, int x, int y, int width, int height, bool fillBackground )
{
    std::string key = name.toLatin1().data();
    if( mLabelMap.find( key ) == mLabelMap.end() )
    {
        TLabel * pC = new TLabel( mpMainFrame );
        mLabelMap[key] = pC;
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

bool TConsole::setBackgroundImage( QString & name, QString & path )
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

bool TConsole::setBackgroundColor( QString & name, int r, int g, int b, int alpha )
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

bool TConsole::showWindow( QString & name )
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

bool TConsole::hideWindow( QString & name )
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

bool TConsole::printWindow( QString & name, QString & text )
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

void TConsole::print( QString & msg )
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
                    mFormatCurrent.bold,
                    mFormatCurrent.italics,
                    mFormatCurrent.underline );
    console->showNewLines();
    console2->showNewLines();
}

void TConsole::print( QString & msg, int fgColorR, int fgColorG, int fgColorB, int bgColorR, int bgColorG, int bgColorB )
{
    buffer.append(  msg,
                    0,
                    msg.size(),
                    fgColorR,
                    fgColorG,
                    fgColorB,
                    bgColorR,
                    bgColorG,
                    bgColorB,
                    false,
                    false,
                    false );
    console->showNewLines();
    console2->showNewLines();
}


void TConsole::printSystemMessage( QString & msg )
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
                    false );
    console->showNewLines();
    console2->showNewLines();
}

void TConsole::echoUserWindow( QString & msg )
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
        QIcon icon2(":/icons/red-bomb.png");
        emergencyStop->setIcon( icon2 );
    }
    else
    {
        mpHost->reenableAllTriggers();
        QIcon icon2(":/icons/edit-bomb.png");
        emergencyStop->setIcon( icon2 );
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
    print( msg, 150, 120, 0, 0, 0, 0 );

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
    print( msg, 150, 120, 0, 0, 0, 0 );
    script = "setFgColor(190,150,0); setUnderline(true);echo([[\n\nTimer Report:\n\n]]);setBold(false);setUnderline(false);setFgColor(150,120,0)";
    mpHost->mLuaInterpreter.compileAndExecuteScript( script );
    QString r2 = mpHost->getTimerUnit()->assembleReport();
    QString footer = QString("\n+--------------------------------------------------------------+\n" );
    msg = r2;
    print( msg, 150, 120, 0, 0, 0, 0 );
    mpHost->mpConsole->print( footer, 150, 120, 0, 0, 0, 0 );
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



