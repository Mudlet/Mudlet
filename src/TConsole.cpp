/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014-2020 by Stephen Lyons - slysven@virginmedia.com    *
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
#include "TDockWidget.h"
#include "TEvent.h"
#include "TLabel.h"
#include "TMap.h"
#include "TRoomDB.h"
#include "TSplitter.h"
#include "TTextEdit.h"
#include "dlgMapper.h"
#include "mudlet.h"

#include "pre_guard.h"
#include <QLineEdit>
#include <QMessageBox>
#include <QMimeData>
#include <QRegularExpression>
#include <QScrollBar>
#include <QShortcut>
#include <QTextBoundaryFinder>
#include <QTextCodec>
#include <QPainter>
#include "post_guard.h"

const QString TConsole::cmLuaLineVariable("line");

TConsole::TConsole(Host* pH, ConsoleType type, QWidget* parent)
: QWidget(parent)
, mpHost(pH)
, mpDockWidget(nullptr)
, mpCommandLine(nullptr)
, buffer(pH)
, emergencyStop(new QToolButton)
, layerCommandLine(nullptr)
, mBgColor(QColor(Qt::black))
, mClipboard(mpHost)
, mCommandBgColor(Qt::black)
, mCommandFgColor(QColor(213, 195, 0))
, mConsoleName("main")
, mDisplayFontName("Bitstream Vera Sans Mono")
, mDisplayFontSize(14)
, mDisplayFont(QFont(mDisplayFontName, mDisplayFontSize, QFont::Normal))
, mFgColor(Qt::black)
, mIndentCount(0)
, mLogFileName(QString(""))
, mLogToLogFile(false)
, mMainFrameBottomHeight(0)
, mMainFrameLeftWidth(0)
, mMainFrameRightWidth(0)
, mMainFrameTopHeight(0)
, mOldX(0)
, mOldY(0)
, mpBaseVFrame(new QWidget(this))
, mpTopToolBar(new QWidget(mpBaseVFrame))
, mpBaseHFrame(new QWidget(mpBaseVFrame))
, mpLeftToolBar(new QWidget(mpBaseHFrame))
, mpMainFrame(new QWidget(mpBaseHFrame))
, mpRightToolBar(new QWidget(mpBaseHFrame))
, mpMainDisplay(new QWidget(mpMainFrame))
, mpMapper(nullptr)
, mpScrollBar(new QScrollBar)
, mRecordReplay(false)
, mSystemMessageBgColor(mBgColor)
, mSystemMessageFgColor(QColor(Qt::red))
, mTriggerEngineMode(false)
, mWrapAt(100)
, networkLatency(new QLineEdit)
, mProfileName(mpHost ? mpHost->getName() : QStringLiteral("debug console"))
, mIsPromptLine(false)
, mUserAgreedToCloseConsole(false)
, mpBufferSearchBox(new QLineEdit)
, mpBufferSearchUp(new QToolButton)
, mpBufferSearchDown(new QToolButton)
, mCurrentSearchResult(0)
, mSearchQuery()
, mpButtonMainLayer(nullptr)
, mType(type)
, mSpellDic()
, mpHunspell_system(nullptr)
, mpHunspell_shared(nullptr)
, mpHunspell_profile(nullptr)
{
    auto ps = new QShortcut(this);
    ps->setKey(Qt::CTRL + Qt::Key_W);
    ps->setContext(Qt::WidgetShortcut);

    if (mType & CentralDebugConsole) {
        setWindowTitle(tr("Debug Console"));
        // Probably will not show up as this is used inside a QMainWindow widget
        // which has it's own title and icon set.
        // mIsSubConsole was left false for this
        mWrapAt = 50;
        mStandardFormat.setTextFormat(mFgColor, mBgColor, TChar::None);
    } else {
        if (mType & (ErrorConsole|SubConsole|UserWindow)) {
            // Orginally this was for TConsole instances with a parent pointer
            // This branch for: UserWindows, SubConsole, ErrorConsole
            // mIsSubConsole was true for these
            mMainFrameTopHeight = 0;
            mMainFrameBottomHeight = 0;
            mMainFrameLeftWidth = 0;
            mMainFrameRightWidth = 0;
        } else if (mType & (MainConsole|Buffer)) {
            // Orginally this was for TConsole instances without a parent pointer
            // This branch for: Buffers, MainConsole
            // mIsSubConsole was false for these
            mMainFrameTopHeight = mpHost->mBorderTopHeight;
            mMainFrameBottomHeight = mpHost->mBorderBottomHeight;
            mMainFrameLeftWidth = mpHost->mBorderLeftWidth;
            mMainFrameRightWidth = mpHost->mBorderRightWidth;
            mCommandBgColor = mpHost->mCommandBgColor;
            mCommandFgColor = mpHost->mCommandFgColor;
        } else {
            Q_ASSERT_X(false, "TConsole::TConsole(...)", "invalid TConsole type detected");
        }
        mStandardFormat.setTextFormat(mpHost->mFgColor, mpHost->mBgColor, TChar::None);
    }
    setContentsMargins(0, 0, 0, 0);
    mFormatSystemMessage.setBackground(mBgColor);
    mFormatSystemMessage.setForeground(Qt::red);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_OpaquePaintEvent); //was disabled

    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QSizePolicy sizePolicy3(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QSizePolicy sizePolicy2(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QSizePolicy sizePolicy4(QSizePolicy::Fixed, QSizePolicy::Expanding);
    QSizePolicy sizePolicy5(QSizePolicy::Fixed, QSizePolicy::Fixed);
    QPalette mainPalette;
    mainPalette.setColor(QPalette::Text, QColor(Qt::black));
    mainPalette.setColor(QPalette::Highlight, QColor(55, 55, 255));
    mainPalette.setColor(QPalette::Window, QColor(0, 0, 0, 255));
    QPalette splitterPalette;
    splitterPalette = mainPalette;
    splitterPalette.setColor(QPalette::Button, QColor(0, 0, 255, 255));
    splitterPalette.setColor(QPalette::Window, QColor(Qt::green)); //,255) );
    splitterPalette.setColor(QPalette::Base, QColor(255, 0, 0, 255));
    splitterPalette.setColor(QPalette::Window, QColor(Qt::white));
    //setPalette( mainPalette );

    //QVBoxLayout * layoutFrame = new QVBoxLayout( mainFrame );
    QPalette framePalette;
    framePalette.setColor(QPalette::Text, QColor(Qt::black));
    framePalette.setColor(QPalette::Highlight, QColor(55, 55, 255));
    framePalette.setColor(QPalette::Window, QColor(0, 0, 0, 255));
    mpMainFrame->setPalette(framePalette);
    mpMainFrame->setAutoFillBackground(true);
    mpMainFrame->setContentsMargins(0, 0, 0, 0);
    auto centralLayout = new QVBoxLayout;
    setLayout(centralLayout);
    auto baseVFrameLayout = new QVBoxLayout;
    mpBaseVFrame->setLayout(baseVFrameLayout);
    baseVFrameLayout->setMargin(0);
    baseVFrameLayout->setSpacing(0);
    centralLayout->addWidget(mpBaseVFrame);
    auto baseHFrameLayout = new QHBoxLayout;
    mpBaseHFrame->setLayout(baseHFrameLayout);
    baseHFrameLayout->setMargin(0);
    baseHFrameLayout->setSpacing(0);
    layout()->setSpacing(0);
    layout()->setMargin(0);
    setContentsMargins(0, 0, 0, 0);

    auto topBarLayout = new QHBoxLayout;
    mpTopToolBar->setLayout(topBarLayout);
    mpTopToolBar->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
    mpTopToolBar->setContentsMargins(0, 0, 0, 0);
    mpTopToolBar->setAutoFillBackground(true);
    QPalette topbarPalette;
    topbarPalette.setColor(QPalette::Text, QColor(Qt::white));
    topbarPalette.setColor(QPalette::Highlight, QColor(55, 55, 255));
    topbarPalette.setColor(QPalette::Window, QColor(0, 255, 0, 255));
    topbarPalette.setColor(QPalette::Base, QColor(0, 255, 0, 255));
    //mpTopToolBar->setPalette(topbarPalette);


    topBarLayout->setMargin(0);
    topBarLayout->setSpacing(0);
    auto leftBarLayout = new QVBoxLayout;
    mpLeftToolBar->setLayout(leftBarLayout);
    mpLeftToolBar->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding));
    mpLeftToolBar->setAutoFillBackground(true);
    leftBarLayout->setMargin(0);
    leftBarLayout->setSpacing(0);
    mpLeftToolBar->setContentsMargins(0, 0, 0, 0);
    auto rightBarLayout = new QVBoxLayout;
    mpRightToolBar->setLayout(rightBarLayout);
    mpRightToolBar->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding));
    mpRightToolBar->setAutoFillBackground(true);
    rightBarLayout->setMargin(0);
    rightBarLayout->setSpacing(0);
    mpRightToolBar->setContentsMargins(0, 0, 0, 0);
    mpBaseVFrame->setContentsMargins(0, 0, 0, 0);
    baseVFrameLayout->setSpacing(0);
    baseVFrameLayout->setMargin(0);
    mpTopToolBar->setContentsMargins(0, 0, 0, 0);
    baseVFrameLayout->addWidget(mpTopToolBar);
    baseVFrameLayout->addWidget(mpBaseHFrame);
    baseHFrameLayout->addWidget(mpLeftToolBar);
    auto mpCorePane = new QWidget(mpBaseHFrame);
    auto coreSpreadLayout = new QVBoxLayout;
    mpCorePane->setLayout(coreSpreadLayout);
    mpCorePane->setContentsMargins(0, 0, 0, 0);
    coreSpreadLayout->setMargin(0);
    coreSpreadLayout->setSpacing(0);
    coreSpreadLayout->addWidget(mpMainFrame);
    mpCorePane->setSizePolicy(sizePolicy);
    baseHFrameLayout->addWidget(mpCorePane);
    baseHFrameLayout->addWidget(mpRightToolBar);
    mpTopToolBar->setContentsMargins(0, 0, 0, 0);
    mpBaseHFrame->setAutoFillBackground(true);
    baseHFrameLayout->setSpacing(0);
    baseHFrameLayout->setMargin(0);
    setContentsMargins(0, 0, 0, 0);
    mpBaseHFrame->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setMargin(0);
    mpMainDisplay->move(mMainFrameLeftWidth, mMainFrameTopHeight);
    mpMainFrame->show();
    mpMainDisplay->show();
    mpMainFrame->setContentsMargins(0, 0, 0, 0);
    mpMainDisplay->setContentsMargins(0, 0, 0, 0);
    auto layout = new QVBoxLayout;
    mpMainDisplay->setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    mpBaseVFrame->setSizePolicy(sizePolicy);
    mpBaseHFrame->setSizePolicy(sizePolicy);
    mpBaseVFrame->setFocusPolicy(Qt::NoFocus);
    mpBaseHFrame->setFocusPolicy(Qt::NoFocus);

    baseVFrameLayout->setMargin(0);
    baseHFrameLayout->setMargin(0);
    centralLayout->setMargin(0);

    if (mType == MainConsole) {
        mpCommandLine = new TCommandLine(pH, mpCommandLine->MainCommandLine, this, mpMainDisplay);
        mpCommandLine->setContentsMargins(0, 0, 0, 0);
        mpCommandLine->setSizePolicy(sizePolicy);
        mpCommandLine->setFocusPolicy(Qt::StrongFocus);
    }

    layer = new QWidget(mpMainDisplay);
    layer->setContentsMargins(0, 0, 0, 0);
    layer->setContentsMargins(0, 0, 0, 0); //neu rc1
    layer->setSizePolicy(sizePolicy);
    layer->setFocusPolicy(Qt::NoFocus);

    auto layoutLayer = new QHBoxLayout;
    layer->setLayout(layoutLayer);
    layoutLayer->setMargin(0);  //neu rc1
    layoutLayer->setSpacing(0); //neu rc1
    layoutLayer->setMargin(0);  //neu rc1

    mpScrollBar->setFixedWidth(15);

    splitter = new TSplitter(Qt::Vertical);
    splitter->setContentsMargins(0, 0, 0, 0);
    splitter->setSizePolicy(sizePolicy);
    splitter->setOrientation(Qt::Vertical);
    splitter->setHandleWidth(3);
    splitter->setPalette(splitterPalette);
    splitter->setParent(layer);

    mUpperPane = new TTextEdit(this, splitter, &buffer, mpHost, false);
    mUpperPane->setContentsMargins(0, 0, 0, 0);
    mUpperPane->setSizePolicy(sizePolicy3);
    mUpperPane->setFocusPolicy(Qt::NoFocus);

    mLowerPane = new TTextEdit(this, splitter, &buffer, mpHost, true);
    mLowerPane->setContentsMargins(0, 0, 0, 0);
    mLowerPane->setSizePolicy(sizePolicy3);
    mLowerPane->setFocusPolicy(Qt::NoFocus);

    if (mType == MainConsole) {
        setFocusProxy(mpCommandLine);
        mUpperPane->setFocusProxy(mpCommandLine);
        mLowerPane->setFocusProxy(mpCommandLine);
    } else if (mType == UserWindow) {
        setFocusProxy(mpHost->mpConsole->mpCommandLine);
        mUpperPane->setFocusProxy(mpHost->mpConsole->mpCommandLine);
        mLowerPane->setFocusProxy(mpHost->mpConsole->mpCommandLine);
    }

    splitter->addWidget(mUpperPane);
    splitter->addWidget(mLowerPane);

    splitter->setCollapsible(1, false);
    splitter->setCollapsible(0, false);
    splitter->setStretchFactor(0, 6);
    splitter->setStretchFactor(1, 1);

    layoutLayer->addWidget(splitter);
    layoutLayer->addWidget(mpScrollBar);
    layoutLayer->setContentsMargins(0, 0, 0, 0);
    layoutLayer->setSpacing(1); // nicht naeher dran, da es sonst performance probleme geben koennte beim display

    layerCommandLine = new QWidget; //( mpMainFrame );//layer );
    layerCommandLine->setContentsMargins(0, 0, 0, 0);
    layerCommandLine->setSizePolicy(sizePolicy2);
    layerCommandLine->setMaximumHeight(31);
    layerCommandLine->setMinimumHeight(31);

    layoutLayer2 = new QHBoxLayout(layerCommandLine);
    layoutLayer2->setMargin(0);
    layoutLayer2->setSpacing(0);

    mpButtonMainLayer = new QWidget;
    mpButtonMainLayer->setObjectName(QStringLiteral("mpButtonMainLayer"));
    mpButtonMainLayer->setSizePolicy(sizePolicy);
    mpButtonMainLayer->setContentsMargins(0, 0, 0, 0);
    auto layoutButtonMainLayer = new QVBoxLayout(mpButtonMainLayer);
    layoutButtonMainLayer->setObjectName(QStringLiteral("layoutButtonMainLayer"));
    layoutButtonMainLayer->setMargin(0);
    layoutButtonMainLayer->setContentsMargins(0, 0, 0, 0);

    layoutButtonMainLayer->setSpacing(0);
    /*mpButtonMainLayer->setMinimumHeight(31);
           mpButtonMainLayer->setMaximumHeight(31);*/
    auto buttonLayer = new QWidget;
    buttonLayer->setObjectName(QStringLiteral("buttonLayer"));
    auto layoutButtonLayer = new QGridLayout(buttonLayer);
    layoutButtonLayer->setObjectName(QStringLiteral("layoutButtonLayer"));
    layoutButtonLayer->setMargin(0);
    layoutButtonLayer->setSpacing(0);

    auto buttonLayerSpacer = new QWidget(buttonLayer);
    buttonLayerSpacer->setSizePolicy(sizePolicy4);
    layoutButtonMainLayer->addWidget(buttonLayerSpacer);
    layoutButtonMainLayer->addWidget(buttonLayer);

    auto timeStampButton = new QToolButton;
    timeStampButton->setCheckable(true);
    timeStampButton->setMinimumSize(QSize(30, 30));
    timeStampButton->setMaximumSize(QSize(30, 30));
    timeStampButton->setSizePolicy(sizePolicy5);
    timeStampButton->setFocusPolicy(Qt::NoFocus);
    timeStampButton->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(
        tr("Show Time Stamps.")));
    timeStampButton->setIcon(QIcon(QStringLiteral(":/icons/dialog-information.png")));
    connect(timeStampButton, &QAbstractButton::toggled, mUpperPane, &TTextEdit::slot_toggleTimeStamps);
    connect(timeStampButton, &QAbstractButton::toggled, mLowerPane, &TTextEdit::slot_toggleTimeStamps);

    auto replayButton = new QToolButton;
    replayButton->setCheckable(true);
    replayButton->setMinimumSize(QSize(30, 30));
    replayButton->setMaximumSize(QSize(30, 30));
    replayButton->setSizePolicy(sizePolicy5);
    replayButton->setFocusPolicy(Qt::NoFocus);
    replayButton->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(
        tr("Record a replay.")));
    replayButton->setIcon(QIcon(QStringLiteral(":/icons/media-tape.png")));
    connect(replayButton, &QAbstractButton::pressed, this, &TConsole::slot_toggleReplayRecording);

    logButton = new QToolButton;
    logButton->setMinimumSize(QSize(30, 30));
    logButton->setMaximumSize(QSize(30, 30));
    logButton->setCheckable(true);
    logButton->setSizePolicy(sizePolicy5);
    logButton->setFocusPolicy(Qt::NoFocus);
    logButton->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(
        tr("Start logging game output to log file.")));
    QIcon logIcon;
    logIcon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-downloads.png")), QIcon::Normal, QIcon::Off);
    logIcon.addPixmap(QPixmap(QStringLiteral(":/icons/folder-downloads-red-cross.png")), QIcon::Normal, QIcon::On);
    logButton->setIcon(logIcon);
    connect(logButton, &QAbstractButton::pressed, this, &TConsole::slot_toggleLogging);

    networkLatency->setReadOnly(true);
    networkLatency->setSizePolicy(sizePolicy4);
    networkLatency->setFocusPolicy(Qt::NoFocus);
    networkLatency->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(
        tr("<i>N:</i> is the latency of the game server and network (aka ping, in seconds), <br>"
           "<i>S:</i> is the system processing time - how long your triggers took to process the last line(s).")));
    networkLatency->setMaximumSize(120, 30);
    networkLatency->setMinimumSize(120, 30);
    networkLatency->setAutoFillBackground(true);
    networkLatency->setContentsMargins(0, 0, 0, 0);
    QPalette basePalette;
    basePalette.setColor(QPalette::Text, QColor(Qt::black));
    basePalette.setColor(QPalette::Base, QColor(Qt::white));
    networkLatency->setPalette(basePalette);
    networkLatency->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

    QFont latencyFont = QFont("Bitstream Vera Sans Mono", 10, QFont::Normal);
    int width;
    int maxWidth = 120;
    width = QFontMetrics(latencyFont).boundingRect(QString("N:0.000 S:0.000")).width();
    if (width < maxWidth) {
        networkLatency->setFont(latencyFont);
    } else {
        QFont latencyFont2 = QFont("Bitstream Vera Sans Mono", 9, QFont::Normal);
        width = QFontMetrics(latencyFont2).boundingRect(QString("N:0.000 S:0.000")).width();
        if (width < maxWidth) {
            networkLatency->setFont(latencyFont2);
        } else {
            QFont latencyFont3 = QFont("Bitstream Vera Sans Mono", 8, QFont::Normal);
            width = QFontMetrics(latencyFont3).boundingRect(QString("N:0.000 S:0.000")).width();
            networkLatency->setFont(latencyFont3);
        }
    }

    emergencyStop->setMinimumSize(QSize(30, 30));
    emergencyStop->setMaximumSize(QSize(30, 30));
    emergencyStop->setIcon(QIcon(QStringLiteral(":/icons/edit-bomb.png")));
    emergencyStop->setSizePolicy(sizePolicy4);
    emergencyStop->setFocusPolicy(Qt::NoFocus);
    emergencyStop->setCheckable(true);
    emergencyStop->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(
        tr("Emergency Stop. Stops all timers and triggers.")));
    connect(emergencyStop, &QAbstractButton::clicked, this, &TConsole::slot_stop_all_triggers);

    mpBufferSearchBox->setMinimumSize(QSize(100, 30));
    mpBufferSearchBox->setMaximumSize(QSize(150, 30));
    mpBufferSearchBox->setSizePolicy(sizePolicy5);
    mpBufferSearchBox->setFont(mpHost->mCommandLineFont);
    mpBufferSearchBox->setFocusPolicy(Qt::ClickFocus);
    mpBufferSearchBox->setPlaceholderText("Search ...");
    QPalette __pal;
    __pal.setColor(QPalette::Text, mpHost->mCommandLineFgColor);
    __pal.setColor(QPalette::Highlight, QColor(0, 0, 192));
    __pal.setColor(QPalette::HighlightedText, QColor(Qt::white));
    __pal.setColor(QPalette::Base, mpHost->mCommandLineBgColor);
    __pal.setColor(QPalette::Window, mpHost->mCommandLineBgColor);
    mpBufferSearchBox->setPalette(__pal);
    mpBufferSearchBox->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(
        tr("Search buffer.")));
    connect(mpBufferSearchBox, &QLineEdit::returnPressed, this, &TConsole::slot_searchBufferUp);


    mpBufferSearchUp->setMinimumSize(QSize(30, 30));
    mpBufferSearchUp->setMaximumSize(QSize(30, 30));
    mpBufferSearchUp->setSizePolicy(sizePolicy5);
    mpBufferSearchUp->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(
        tr("Earlier search result.")));
    mpBufferSearchUp->setFocusPolicy(Qt::NoFocus);
    mpBufferSearchUp->setIcon(QIcon(QStringLiteral(":/icons/export.png")));
    connect(mpBufferSearchUp, &QAbstractButton::clicked, this, &TConsole::slot_searchBufferUp);


    mpBufferSearchDown->setMinimumSize(QSize(30, 30));
    mpBufferSearchDown->setMaximumSize(QSize(30, 30));
    mpBufferSearchDown->setSizePolicy(sizePolicy5);
    mpBufferSearchDown->setFocusPolicy(Qt::NoFocus);
    mpBufferSearchDown->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(
        tr("Later search result.")));
    mpBufferSearchDown->setIcon(QIcon(QStringLiteral(":/icons/import.png")));
    connect(mpBufferSearchDown, &QAbstractButton::clicked, this, &TConsole::slot_searchBufferDown);

    if (mpCommandLine) {
        layoutLayer2->addWidget(mpCommandLine);
    }

    layoutLayer2->addWidget(mpButtonMainLayer);
    layoutButtonLayer->addWidget(mpBufferSearchBox, 0, 0, 0, 4);
    layoutButtonLayer->addWidget(mpBufferSearchUp, 0, 5);
    layoutButtonLayer->addWidget(mpBufferSearchDown, 0, 6);
    layoutButtonLayer->addWidget(timeStampButton, 0, 7);
    layoutButtonLayer->addWidget(replayButton, 0, 8);
    layoutButtonLayer->addWidget(logButton, 0, 9);
    layoutButtonLayer->addWidget(emergencyStop, 0, 10);
    layoutButtonLayer->addWidget(networkLatency, 0, 11);
    layoutLayer2->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(layer);
    networkLatency->setFrame(false);
    //QPalette whitePalette;
    //whitePalette.setColor( QPalette::Window, baseColor);//,255) );
    layerCommandLine->setPalette(basePalette);
    layerCommandLine->setAutoFillBackground(true);

    centralLayout->addWidget(layerCommandLine);

    QList<int> sizeList;
    sizeList << 6 << 2;
    splitter->setSizes(sizeList);

    mUpperPane->show();
    mLowerPane->show();
    mLowerPane->hide();

    connect(mpScrollBar, &QAbstractSlider::valueChanged, mUpperPane, &TTextEdit::slot_scrollBarMoved);

    if (mType & (ErrorConsole|SubConsole|UserWindow)) {
        mpScrollBar->hide();
        mLowerPane->hide();
        layerCommandLine->hide();
        mpMainFrame->move(0, 0);
        mpMainDisplay->move(0, 0);
    }
    if (mType & CentralDebugConsole) {
        layerCommandLine->hide();
    }

    mpBaseVFrame->setContentsMargins(0, 0, 0, 0);
    mpBaseHFrame->setContentsMargins(0, 0, 0, 0);
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
    mpButtonMainLayer->setMinimumWidth(400);
    mpButtonMainLayer->setMaximumWidth(400);
    setFocusPolicy(Qt::ClickFocus);
    mUpperPane->setFocusPolicy(Qt::ClickFocus);
    mLowerPane->setFocusPolicy(Qt::ClickFocus);

    buttonLayerSpacer->setAutoFillBackground(true);
    buttonLayerSpacer->setPalette(__pal);
    mpButtonMainLayer->setAutoFillBackground(true);
    mpButtonMainLayer->setPalette(__pal);

    buttonLayer->setAutoFillBackground(true);
    buttonLayer->setPalette(__pal);

    layerCommandLine->setPalette(__pal);

    changeColors();
    if (mType == MainConsole) {
        // During first use where mIsDebugConsole IS true mudlet::self() is null
        // then - but we rely on that flag to avoid having to also test for a
        // non-null mudlet::self() - the connect(...) will produce a debug
        // message and not make THAT connection should it indeed be null but it
        // is not fatal...
        // So, this SHOULD be the main profile mUpperPane - Slysven
        connect(mudlet::self(), &mudlet::signal_profileMapReloadRequested, this, &TConsole::slot_reloadMap, Qt::UniqueConnection);
        connect(this, &TConsole::signal_newDataAlert, mudlet::self(), &mudlet::slot_newDataOnHost, Qt::UniqueConnection);

        // Load up the spelling dictionary from the system:
        setSystemSpellDictionary(mpHost->getSpellDic());

        // Load up the spelling dictionary for the profile - needs to handle the
        // absence of files for the first run in a new profile or from an older
        // Mudlet version:
        setProfileSpellDictionary();
    }

    // error and debug consoles inherit font of the main console
    if (mType & (ErrorConsole | CentralDebugConsole)) {
        mDisplayFont = mpHost->getDisplayFont();
        mDisplayFontName = mDisplayFont.family();
        mDisplayFontSize = mDisplayFont.pointSize();
        refreshMiniConsole();
    }

    if (mType & (MainConsole | UserWindow)) {
        setAcceptDrops(true);
        setMouseTracking(true);
    }

    if (mType & MainConsole) {
        mpButtonMainLayer->setVisible(!mpHost->getCompactInputLine());
    }
}

TConsole::~TConsole()
{
    if (mpHunspell_system) {
        Hunspell_destroy(mpHunspell_system);
        mpHunspell_system = nullptr;
    }
    if (mpHunspell_profile) {
        Hunspell_destroy(mpHunspell_profile);
        mpHunspell_profile = nullptr;
        // Need to commit any changes to personal dictionary
        qDebug() << "TCommandLine::~TConsole(...) INFO - Saving profile's own Hunspell dictionary...";
        mudlet::self()->saveDictionary(mudlet::self()->getMudletPath(mudlet::profileDataItemPath, mProfileName, QStringLiteral("profile")), mWordSet_profile);
    }
}

Host* TConsole::getHost()
{
    return mpHost;
}

void TConsole::setLabelStyleSheet(std::string& buf, std::string& sh)
{
    QString key = QString::fromUtf8(buf.c_str());
    QString sheet = sh.c_str();
    if (mLabelMap.find(key) != mLabelMap.end()) {
        QLabel* pC = mLabelMap[key];
        if (!pC) {
            return;
        }
        pC->setStyleSheet(sheet);
        return;
    }
}

std::pair<bool, QString> TConsole::setUserWindowStyleSheet(const QString& name, const QString& userWindowStyleSheet)
{
    if (name.isEmpty()) {
        return {false, QStringLiteral("a userwindow cannot have an empty string as its name")};
    }

    auto pW = mDockWidgetMap.value(name);
    if (pW) {
        pW->setStyleSheet(userWindowStyleSheet);
        return {true, QString()};
    }
    return {false, QStringLiteral("userwindow name \"%1\" not found").arg(name)};
}


void TConsole::resizeEvent(QResizeEvent* event)
{
    if (mType & (MainConsole|Buffer)) {
        mMainFrameTopHeight = mpHost->mBorderTopHeight;
        mMainFrameBottomHeight = mpHost->mBorderBottomHeight;
        mMainFrameLeftWidth = mpHost->mBorderLeftWidth;
        mMainFrameRightWidth = mpHost->mBorderRightWidth;
    }
    int x = event->size().width();
    int y = event->size().height();


    if (mType & (MainConsole|Buffer|SubConsole|UserWindow) && mpCommandLine && !mpCommandLine->isHidden()) {
        mpMainFrame->resize(x, y);
        mpBaseVFrame->resize(x, y);
        mpBaseHFrame->resize(x, y);
        x = x - mpLeftToolBar->width() - mpRightToolBar->width();
        y = y - mpTopToolBar->height();
        mpMainDisplay->resize(x - mMainFrameLeftWidth - mMainFrameRightWidth, y - mMainFrameTopHeight - mMainFrameBottomHeight - mpCommandLine->height());
    } else {
        mpMainFrame->resize(x, y);
        mpMainDisplay->resize(x, y); //x - mMainFrameLeftWidth - mMainFrameRightWidth, y - mMainFrameTopHeight - mMainFrameBottomHeight );
    }
    mpMainDisplay->move(mMainFrameLeftWidth, mMainFrameTopHeight);

    if (mType & (CentralDebugConsole|ErrorConsole)) {
        layerCommandLine->hide();
     // do nothing for SubConsole or UserWindows
    } else if (mType & (!SubConsole|!UserWindow)) {
        //layerCommandLine->move(0,mpMainFrame->height()-layerCommandLine->height());
        layerCommandLine->move(0, mpBaseVFrame->height() - layerCommandLine->height());
    }

    QWidget::resizeEvent(event);

    if (mType & (MainConsole|Buffer)) {
        TLuaInterpreter* pLua = mpHost->getLuaInterpreter();
        QString func = "handleWindowResizeEvent";
        QString n = "WindowResizeEvent";
        pLua->call(func, n);

        TEvent mudletEvent {};
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
//create the sysUserWindowResize Event for automatic resizing with Geyser
    if (mType & (UserWindow)) {
        TLuaInterpreter* pLua = mpHost->getLuaInterpreter();
        QString func = "handleWindowResizeEvent";
        QString n = "WindowResizeEvent";
        pLua->call(func, n);

        TEvent mudletEvent {};
        mudletEvent.mArgumentList.append(QLatin1String("sysUserWindowResizeEvent"));
        mudletEvent.mArgumentList.append(QString::number(x));
        mudletEvent.mArgumentList.append(QString::number(y));
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
    if (mType & (ErrorConsole|MainConsole|SubConsole|UserWindow|Buffer)) {
        mMainFrameTopHeight = mpHost->mBorderTopHeight;
        mMainFrameBottomHeight = mpHost->mBorderBottomHeight;
        mMainFrameLeftWidth = mpHost->mBorderLeftWidth;
        mMainFrameRightWidth = mpHost->mBorderRightWidth;
    }

    int x = width();
    int y = height();

    mpBaseVFrame->resize(x, y);
    mpBaseHFrame->resize(x, y);

    x = mpBaseVFrame->width();
    if (!mpLeftToolBar->isHidden()) {
        x -= mpLeftToolBar->width();
    }
    if (!mpRightToolBar->isHidden()) {
        x -= mpRightToolBar->width();
    }

    y = mpBaseVFrame->height();
    if (!mpTopToolBar->isHidden()) {
        y -= mpTopToolBar->height();
    }

    mpMainDisplay->resize(x - mMainFrameLeftWidth - mMainFrameRightWidth, y - mMainFrameTopHeight - mMainFrameBottomHeight - mpCommandLine->height());

    mpMainDisplay->move(mMainFrameLeftWidth, mMainFrameTopHeight);
    x = width();
    y = height();
    QSize s = QSize(x, y);
    QResizeEvent event(s, s);
    QApplication::sendEvent(this, &event);
}


void TConsole::closeEvent(QCloseEvent* event)
{
    if (mType == CentralDebugConsole) {
        if (mudlet::self()->isGoingDown() || mpHost->isClosingDown()) {
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

    if (mType & (SubConsole|Buffer)) {
        if (mudlet::self()->isGoingDown() || mpHost->isClosingDown()) {
            auto pC = mpHost->mpConsole->mSubConsoleMap.take(mConsoleName);
            if (pC) {
                // As it happens pC will be identical to 'this' it is just that
                // we will have removed it from the main TConsole's
                // mSubConsoleMap:
                mUpperPane->close();
                mLowerPane->close();
            }

            event->accept();
            return;
        } else {
            hide();
            event->ignore();
            return;
        }
    }

    if (mType == UserWindow) {
        if (mudlet::self()->isGoingDown() || mpHost->isClosingDown()) {
            auto pC = mpHost->mpConsole->mSubConsoleMap.take(mConsoleName);
            auto pD = mpHost->mpConsole->mDockWidgetMap.take(mConsoleName);
            if (pC) {
                // As it happens pC will be identical to 'this' it is just that
                // we will have removed it from the main TConsole's
                // mSubConsoleMap:
                mUpperPane->close();
                mLowerPane->close();
            }
            if (!pD) {
                qDebug() << "TConsole::closeEvent(QCloseEvent*) INFO - closing a UserWindow but the TDockWidget pointer was not found to be removed...";
            }

            event->accept();
            return;
        } else {
            hide();
            event->ignore();
            return;
        }
    }

    TEvent conCloseEvent{};
    conCloseEvent.mArgumentList.append(QStringLiteral("sysExitEvent"));
    conCloseEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    mpHost->raiseEvent(conCloseEvent);

    if (mpHost->mFORCE_SAVE_ON_EXIT) {
        mudlet::self()->saveWindowLayout();
        mpHost->modulesToWrite.clear();
        mpHost->saveProfile();

        if (mpHost->mpMap->mpRoomDB->size() > 0) {
            QDir dir_map;
            QString directory_map = mudlet::getMudletPath(mudlet::profileMapsPath, mProfileName);
            // CHECKME: Consider changing datetime spec to more "sortable" "yyyy-MM-dd#HH-mm-ss" (3 of 6)
            QString filename_map = mudlet::getMudletPath(mudlet::profileDateTimeStampedMapPathFileName, mProfileName, QDateTime::currentDateTime().toString("dd-MM-yyyy#hh-mm-ss"));
            if (!dir_map.exists(directory_map)) {
                dir_map.mkpath(directory_map);
            }
            QFile file_map(filename_map);
            if (file_map.open(QIODevice::WriteOnly)) {
                QDataStream out(&file_map);
                if (mudlet::scmRunTimeQtVersion >= QVersionNumber(5, 13, 0)) {
                    out.setVersion(mudlet::scmQDataStreamFormat_5_12);
                }
                mpHost->mpMap->serialize(out);
                file_map.close();
            }
        }
        event->accept();
        return;
    }

    if (!mUserAgreedToCloseConsole) {
    ASK:
        int choice = QMessageBox::question(this, tr("Save profile?"), tr("Do you want to save the profile %1?").arg(mProfileName), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        if (choice == QMessageBox::Cancel) {
            event->setAccepted(false);
            event->ignore();
            return;
        }
        if (choice == QMessageBox::Yes) {
            mudlet::self()->saveWindowLayout();

            mpHost->modulesToWrite.clear();
            std::tuple<bool, QString, QString> result = mpHost->saveProfile();

            if (!std::get<0>(result)) {
                QMessageBox::critical(this, tr("Couldn't save profile"), tr("Sorry, couldn't save your profile - got the following error: %1").arg(std::get<2>(result)));
                goto ASK;
            } else if (mpHost->mpMap && mpHost->mpMap->mpRoomDB->size() > 0) {
                QDir dir_map;
                QString directory_map = mudlet::getMudletPath(mudlet::profileMapsPath, mProfileName);
                // CHECKME: Consider changing datetime spec to more "sortable" "yyyy-MM-dd#HH-mm-ss" (4 of 6)
                QString filename_map = mudlet::getMudletPath(mudlet::profileDateTimeStampedMapPathFileName, mProfileName, QDateTime::currentDateTime().toString(QStringLiteral("dd-MM-yyyy#hh-mm-ss")));
                if (!dir_map.exists(directory_map)) {
                    dir_map.mkpath(directory_map);
                }
                QFile file_map(filename_map);
                if (file_map.open(QIODevice::WriteOnly)) {
                    QDataStream out(&file_map);
                    if (mudlet::scmRunTimeQtVersion >= QVersionNumber(5, 13, 0)) {
                        out.setVersion(mudlet::scmQDataStreamFormat_5_12);
                    }
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
        } else {
            if (!mudlet::self()->isGoingDown()) {
                QMessageBox::warning(this, "Aborting exit", "Session exit aborted on user request.");
                event->ignore();
                return;
            } else {
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

void TConsole::toggleLogging(bool isMessageEnabled)
{
    if (mType & (CentralDebugConsole|ErrorConsole|SubConsole|UserWindow)) {
        return;
        // We don't support logging anything other than main console (at present?)
    }

    // CHECKME: This path seems suspicious, it is shared amoungst ALL profiles
    // but the action is "Per Profile"...!
    QFile file(mudlet::getMudletPath(mudlet::mainDataItemPath, QStringLiteral("autolog")));
    QDateTime logDateTime = QDateTime::currentDateTime();
    if (!mLogToLogFile) {
        file.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream out(&file);
        file.close();

        QString directoryLogFile;
        QString logFileName;
        // If no log directory is set, default to Mudlet's replay and log files path
        if (mpHost->mLogDir == nullptr || mpHost->mLogDir.isEmpty()) {
            directoryLogFile = mudlet::getMudletPath(mudlet::profileReplayAndLogFilesPath, mProfileName);
        } else {
            directoryLogFile = mpHost->mLogDir;
        }
        // The format being empty is a signal value that means use a specified
        // name:
        if (mpHost->mLogFileNameFormat.isEmpty()) {
            if (mpHost->mLogFileName.isEmpty()) {
                // If no log name is set, use the default placeholder
                logFileName = tr("logfile", "Must be a valid default filename for a log-file and is used if the user does not enter any other value (Ensure all instances have the same translation {2 of 2}).");
            } else {
                // Otherwise a specific name as one is given
                logFileName = mpHost->mLogFileName;
            }
        } else {
            logFileName = logDateTime.toString(mpHost->mLogFileNameFormat);
        }

        // The preset file name formats are derived from date/times so that
        // alphabetical filename and date sort order are the same...
        QDir dirLogFile;
        if (!dirLogFile.exists(directoryLogFile)) {
            dirLogFile.mkpath(directoryLogFile);
        }

        mpHost->mIsCurrentLogFileInHtmlFormat = mpHost->mIsNextLogFileInHtmlFormat;
        if (mpHost->mIsCurrentLogFileInHtmlFormat) {
            mLogFileName = QStringLiteral("%1/%2.html").arg(directoryLogFile, logFileName);
        } else {
            mLogFileName = QStringLiteral("%1/%2.txt").arg(directoryLogFile, logFileName);
        }
        mLogFile.setFileName(mLogFileName);
        // We do not want to use WriteOnly here:
        // Append = "The device is opened in append mode so that all data is
        // written to the end of the file."
        // WriteOnly = "The device is open for writing. Note that this mode
        // implies Truncate."
        if (mpHost->mIsCurrentLogFileInHtmlFormat) {
            mLogFile.open(QIODevice::ReadWrite);
        } else {
            mLogFile.open(QIODevice::Append);
        }
        mLogStream.setDevice(&mLogFile);
        // We have to set a codec here to convert the QString based QTextStream
        // encoding (from UTF-16) to UTF-8 - by default a local 8-Bit one would
        // be used, which is problematic on Windows for non-ASCII (or Latin1?)
        // characters:
        QTextCodec* pLogCodec = QTextCodec::codecForName("UTF-8");
        mLogStream.setCodec(pLogCodec);
        if (isMessageEnabled) {
            QString message = tr("Logging has started. Log file is %1\n").arg(mLogFile.fileName());
            printSystemMessage(message);
            // This puts text onto console that is IMMEDIATELY POSTED into log file so
            // must be done BEFORE logging starts - or actually mLogToLogFile gets set!
        }
        mLogToLogFile = true;
    } else {
        file.remove();
        mLogToLogFile = false;
        if (isMessageEnabled) {
            QString message = tr("Logging has been stopped. Log file is %1\n").arg(mLogFile.fileName());
            printSystemMessage(message);
            // This puts text onto console that is IMMEDIATELY POSTED into log file so
            // must be done AFTER logging ends - or actually mLogToLogFile gets reset!
        }
    }

    if (mLogToLogFile) {
        // Logging is being turned on
        if (mpHost->mIsCurrentLogFileInHtmlFormat) {
            QString log;
            QTextStream logStream(&log);
            // No setting a QTextCodec here, they don't work on QString based QTextStreams
            QStringList fontsList;                  // List of fonts to become the font-family entry for
                                                    // the master css in the header
            fontsList << this->fontInfo().family(); // Seems to be the best way to get the
                                                    // font in use, as different TConsole
                                                    // instances within the same profile
                                                    // might have different fonts
            fontsList << QStringLiteral("Courier New");
            fontsList << QStringLiteral("Monospace");
            fontsList << QStringLiteral("Courier");
            fontsList.removeDuplicates(); // In case the actual one is one of the defaults here

            logStream << "<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.01//EN' 'http://www.w3.org/TR/html4/strict.dtd'>\n";
            logStream << "<html>\n";
            logStream << " <head>\n";
            logStream << "  <meta http-equiv='content-type' content='text/html; charset=utf-8'>";
            // put the charset as early as possible as the parser MUST restart when it
            // switches away from the ASCII default
            logStream << "  <meta name='generator' content='" << tr("Mudlet MUD Client version: %1%2").arg(APP_VERSION, APP_BUILD) << "'>\n";
            // Nice to identify what made the file!
            logStream << "  <title>" << tr("Mudlet, log from %1 profile").arg(mProfileName) << "</title>\n";
            // Web-page title
            logStream << "  <style type='text/css'>\n";
            logStream << "   <!-- body { font-family: '" << fontsList.join("', '") << "'; font-size: 100%; line-height: 1.125em; white-space: nowrap; color:rgb("
                      << mpHost->mFgColor.red() << "," << mpHost->mFgColor.green() << "," << mpHost->mFgColor.blue()
                      << "); background-color:rgb("
                      << mpHost->mBgColor.red() << "," << mpHost->mBgColor.green() << "," << mpHost->mBgColor.blue() << ");}\n";
            logStream << "        span { white-space: pre-wrap; } -->\n";
            logStream << "  </style>\n";
            logStream << "  </head>\n";
            bool isAtBody = false;
            bool foundBody = false;
            while (!mLogStream.atEnd()) {
                QString line = mLogStream.readLine();
                if (line.contains("<body><div>")) {
                    // Begin writing old log to the current log when the body is
                    // found.
                    isAtBody = true;
                    foundBody = true;
                } else if (line.contains("</div></body>")) {
                    // Stop writing to current log once the end of the old log's
                    // <body> is reached.
                    isAtBody = false;
                }

                if (isAtBody) {
                    logStream << line << "\n";
                }
            }
            if (!foundBody) {
                logStream << "  <body><div>\n";
            } else {
                // Put a horizontal line between separate log sessions
                logStream << "  </div><hr><div>\n";
            }
            logStream << QStringLiteral("<p>%1</p>\n")
                         .arg(logDateTime.toString(tr("'Log session starting at 'hh:mm:ss' on 'dddd', 'd' 'MMMM' 'yyyy'.",
                                                      "This is the format argument to QDateTime::toString(...) and needs to follow the rules for that function {literal text must be single quoted} as well as being suitable for the translation locale")));
            // <div></div> tags required around outside of the body <span></spans> for
            // strict HTML 4 as we do not use <p></p>s or anything else

            if (!mLogFile.resize(0)) {
                qWarning() << "TConsole::toggleLogging(...) ERROR - Failed to resize HTML Logfile - it may now be corrupted...!";
            }
            mLogStream << log;
            mLogFile.flush();
        } else {
            // File is NOT an HTML one but pure text:
            // Put a horizontal line between separate log sessions
            // Unfortunately QLatin1String does not have a repeated() method,
            // but it does mean we can use non-ASCII/Latin1 characters:
            // Using 10x U+23AF Horizontal line extension from "Box drawing characters":
            if (mLogFile.size() > 5) {
                // Allow a few junk characters ("BOM"???) at the very start of
                // file to not trigger the insertion of this line:
                mLogStream << QStringLiteral("⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯").repeated(8).append(QChar::LineFeed);
            }
            mLogStream << QStringLiteral("%1\n")
                         .arg(logDateTime.toString(tr("'Log session starting at 'hh:mm:ss' on 'dddd', 'd' 'MMMM' 'yyyy'.",
                                                  "This is the format argument to QDateTime::toString(...) and needs to follow the rules for that function {literal text must be single quoted} as well as being suitable for the translation locale")));

        }
        logButton->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                              .arg(tr("<p>Stop logging game output to log file.</p>")));
    } else {
        // Logging is being turned off
        buffer.logRemainingOutput();
        QString endDateTimeLine = logDateTime.toString(tr("'Log session ending at 'hh:mm:ss' on 'dddd', 'd' 'MMMM' 'yyyy'.",
                                             "This is the format argument to QDateTime::toString(...) and needs to follow the rules for that function {literal text must be single quoted} as well as being suitable for the translation locale"));
        if (mpHost->mIsCurrentLogFileInHtmlFormat) {
            mLogStream << QStringLiteral("<p>%1</p>\n").arg(endDateTimeLine);
            mLogStream << "  </div></body>\n";
            mLogStream << "</html>\n";
        } else {
            // File is NOT an HTML one but pure text:
            mLogStream << endDateTimeLine << "\n";
        }
        mLogFile.flush();
        mLogFile.close();
        logButton->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                              .arg(tr("<p>Start logging game output to log file.</p>")));
    }
}

// Converted into a wrapper around a separate toggleLogging() method so that
// calls to turn logging on/off via the toolbar button - which go via this
// wrapper - generate messages on the console.  Requests to control logging from
// the Lua interpreter call the wrapped method directly and messages are
// generated for Lua user control by the Lua subsystem.
void TConsole::slot_toggleLogging()
{
    if (mType & (CentralDebugConsole|ErrorConsole|SubConsole|UserWindow)) {
        return;
        // We don't support logging anything other than main console (at present?)
    }

    toggleLogging(true);
}

void TConsole::slot_toggleReplayRecording()
{
    if (mType & CentralDebugConsole) {
        return;
    }
    mRecordReplay = !mRecordReplay;
    if (mRecordReplay) {
        QString directoryLogFile = mudlet::getMudletPath(mudlet::profileReplayAndLogFilesPath, mProfileName);
        // CHECKME: Consider changing datetime spec to more "sortable" "yyyy-MM-dd#HH-mm-ss" (5 of 6)
        QString mLogFileName = QStringLiteral("%1/%2.dat").arg(directoryLogFile, QDateTime::currentDateTime().toString(QStringLiteral("dd-MM-yyyy#hh-mm-ss")));
        QDir dirLogFile;
        if (!dirLogFile.exists(directoryLogFile)) {
            dirLogFile.mkpath(directoryLogFile);
        }
        mReplayFile.setFileName(mLogFileName);
        mReplayFile.open(QIODevice::WriteOnly);
        if (mudlet::scmRunTimeQtVersion >= QVersionNumber(5, 13, 0)) {
            mReplayStream.setVersion(mudlet::scmQDataStreamFormat_5_12);
        }
        mReplayStream.setDevice(&mReplayFile);
        mpHost->mTelnet.recordReplay();
        QString message = QString("Replay recording has started. File: ") + mReplayFile.fileName() + "\n";
        printSystemMessage(message);
    } else {
        mReplayFile.close();
        QString message = QString("Replay recording has been stopped. File: ") + mReplayFile.fileName() + "\n";
        printSystemMessage(message);
    }
}

void TConsole::changeColors()
{
    mDisplayFont.setFixedPitch(true);
    if (mType == CentralDebugConsole) {
        mDisplayFont.setStyleStrategy((QFont::StyleStrategy)(QFont::NoAntialias | QFont::PreferQuality));
        mDisplayFont.setFixedPitch(true);
        mUpperPane->setFont(mDisplayFont);
        mLowerPane->setFont(mDisplayFont);
        QPalette palette;
        palette.setColor(QPalette::Text, mFgColor);
        palette.setColor(QPalette::Highlight, QColor(55, 55, 255));
        palette.setColor(QPalette::Base, QColor(Qt::black));
        mUpperPane->setPalette(palette);
        mLowerPane->setPalette(palette);
    } else if (mType & (ErrorConsole|SubConsole|UserWindow|Buffer)) {
        mDisplayFont.setStyleStrategy(QFont::StyleStrategy(QFont::NoAntialias | QFont::PreferQuality));
        mDisplayFont.setFixedPitch(true);
        mUpperPane->setFont(mDisplayFont);
        mLowerPane->setFont(mDisplayFont);
        QPalette palette;
        palette.setColor(QPalette::Text, mFgColor);
        palette.setColor(QPalette::Highlight, QColor(55, 55, 255));
        palette.setColor(QPalette::Base, mBgColor);
        setPalette(palette);
        layer->setPalette(palette);
        mUpperPane->setPalette(palette);
        mLowerPane->setPalette(palette);
    } else if (mType == MainConsole) {
        if (mpCommandLine) {
            QPalette pal;
            pal.setColor(QPalette::Text, mpHost->mCommandLineFgColor); //QColor(0,0,192));
            pal.setColor(QPalette::Highlight, QColor(0, 0, 192));
            pal.setColor(QPalette::HighlightedText, QColor(Qt::white));
            pal.setColor(QPalette::Base, mpHost->mCommandLineBgColor); //QColor(255,255,225));
            mpCommandLine->setPalette(pal);
            mpCommandLine->mRegularPalette = pal;
        }
        if (mpHost->mNoAntiAlias) {
            mpHost->setDisplayFontStyle(QFont::NoAntialias);
        } else {
            mpHost->setDisplayFontStyle(QFont::StyleStrategy(QFont::PreferAntialias | QFont::PreferQuality));
        }
        mpHost->setDisplayFontFixedPitch(true);
        mDisplayFont.setFixedPitch(true);
        mUpperPane->setFont(mpHost->getDisplayFont());
        mLowerPane->setFont(mpHost->getDisplayFont());
        QPalette palette;
        palette.setColor(QPalette::Text, mpHost->mFgColor);
        palette.setColor(QPalette::Highlight, QColor(55, 55, 255));
        palette.setColor(QPalette::Base, mpHost->mBgColor);
        setPalette(palette);
        layer->setPalette(palette);
        mUpperPane->setPalette(palette);
        mLowerPane->setPalette(palette);
        mCommandFgColor = mpHost->mCommandFgColor;
        mCommandBgColor = mpHost->mCommandBgColor;
        if (mpCommandLine) {
            mpCommandLine->setFont(mpHost->getDisplayFont());
        }
        mFormatCurrent.setColors(mpHost->mFgColor, mpHost->mBgColor);
    } else {
        Q_ASSERT_X(false, "TConsole::changeColors()", "invalid TConsole type detected");
    }
    QPalette palette;
    palette.setColor(QPalette::Button, QColor(Qt::blue));
    palette.setColor(QPalette::Window, QColor(Qt::green));
    palette.setColor(QPalette::Base, QColor(Qt::red));

    if (mType & (CentralDebugConsole|MainConsole|Buffer)) {
        mUpperPane->mWrapAt = mWrapAt;
        mLowerPane->mWrapAt = mWrapAt;
        splitter->setPalette(palette);
    }

    buffer.updateColors();
    if (mType & (MainConsole|Buffer)) {
        buffer.mWrapAt = mpHost->mWrapAt;
        buffer.mWrapIndent = mpHost->mWrapIndentCount;
    }
}

void TConsole::setConsoleBgColor(int r, int g, int b)
{
    mBgColor = QColor(r, g, b);
    mUpperPane->setConsoleBgColor(r, g, b);
    mLowerPane->setConsoleBgColor(r, g, b);
    changeColors();
}

// Not used:
//void TConsole::setConsoleFgColor(int r, int g, int b)
//{
//    mFgColor = QColor(r, g, b);
//    mUpperPane->setConsoleFgColor(r, g, b);
//    mLowerPane->setConsoleFgColor(r, g, b);
//    changeColors();
//}

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

void TConsole::printOnDisplay(std::string& incomingSocketData, const bool isFromServer)
{
    mProcessingTime.restart();
    mTriggerEngineMode = true;
    buffer.translateToPlainText(incomingSocketData, isFromServer);
    mTriggerEngineMode = false;

    // dequeues MXP events and raise them through the LuaInterpreter
    // TODO: move this somewhere else more appropriate
    auto &mxpEventQueue = mpHost->mMxpClient.mMxpEvents;
    while (!mxpEventQueue.isEmpty()) {
        const auto& event = mxpEventQueue.dequeue();
        mpHost->mLuaInterpreter.signalMXPEvent(event.name, event.attrs, event.actions);
    }

    double processT = mProcessingTime.elapsed();
    if (mpHost->mTelnet.mGA_Driver) {
        networkLatency->setText(QString("N:%1 S:%2").arg(mpHost->mTelnet.networkLatency, 0, 'f', 3).arg(processT / 1000, 0, 'f', 3));
    } else {
        networkLatency->setText(QString("<no GA> S:%1").arg(processT / 1000, 0, 'f', 3));
    }
    // Modify the tab text if this is not the currently active host - this
    // method is only used on the "main" console so no need to filter depending
    // on TConsole types:

    emit signal_newDataAlert(mProfileName);
}

void TConsole::runTriggers(int line)
{
    mDeletedLines = 0;
    mUserCursor.setY(line);
    mIsPromptLine = buffer.promptBuffer.at(line);
    mEngineCursor = line;
    mUserCursor.setX(0);
    mCurrentLine = buffer.line(line);
    mpHost->getLuaInterpreter()->set_lua_string(cmLuaLineVariable, mCurrentLine);
    mCurrentLine.append('\n');

    if (mudlet::debugMode) {
        TDebug(QColor(Qt::darkGreen), QColor(Qt::black)) << "new line arrived:" >> 0;
        TDebug(QColor(Qt::lightGray), QColor(Qt::black)) << mCurrentLine << "\n" >> 0;
    }
    mpHost->incomingStreamProcessor(mCurrentLine, line);
    mIsPromptLine = false;

    //FIXME: neu schreiben: wenn lines oberhalb der aktuellen zeile gelöscht wurden->redraw clean slice
    //       ansonsten einfach löschen
}

void TConsole::finalize()
{
    mUpperPane->showNewLines();
    mLowerPane->showNewLines();
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

void TConsole::scrollDown(int lines)
{
    mUpperPane->scrollDown(lines);
    if (mUpperPane->mIsTailMode) {
        mLowerPane->mCursorY = buffer.lineBuffer.size();
        mLowerPane->hide();

        mUpperPane->mCursorY = buffer.lineBuffer.size();
        mUpperPane->updateScreenView();
        mUpperPane->forceUpdate();
    }
}

void TConsole::scrollUp(int lines)
{
    mLowerPane->mCursorY = buffer.size();
    mLowerPane->show();
    mLowerPane->updateScreenView();
    mLowerPane->forceUpdate();

    mUpperPane->scrollUp(lines);
}

void TConsole::deselect()
{
    P_begin.setX(0);
    P_begin.setY(0);
    P_end.setX(0);
    P_end.setY(0);
}

void TConsole::showEvent(QShowEvent* event)
{
    if (mType & (MainConsole|Buffer)) {
        if (mpHost) {
            mpHost->mTelnet.mAlertOnNewData = false;
        }
    }
    QWidget::showEvent(event); //FIXME-refac: might cause problems
}

void TConsole::hideEvent(QHideEvent* event)
{
    if (mType & (MainConsole|Buffer)) {
        if (mpHost) {
            if (mudlet::self()->mWindowMinimized) {
                if (mpHost->mAlertOnNewData) {
                    mpHost->mTelnet.mAlertOnNewData = true;
                }
            }
        }
    }
    QWidget::hideEvent(event); //FIXME-refac: might cause problems
}


void TConsole::reset()
{
    deselect();
    mFormatCurrent.setColors(mStandardFormat.foreground(), mStandardFormat.background());
    mFormatCurrent.setAllDisplayAttributes(TChar::None);
}

void TConsole::insertLink(const QString& text, QStringList& func, QStringList& hint, QPoint P, bool customFormat)
{
    int x = P.x();
    int y = P.y();
    QPoint P2 = P;
    P2.setX(x + text.size());

    TChar standardLinkFormat = TChar(Qt::blue, mBgColor, TChar::Underline);
    if (mTriggerEngineMode) {
        mpHost->getLuaInterpreter()->adjustCaptureGroups(x, text.size());

        if (customFormat) {
            buffer.insertInLine(P, text, mFormatCurrent);
        } else {
            buffer.insertInLine(P, text, standardLinkFormat);
        }

        buffer.applyLink(P, P2, func, hint);

        if (y < mEngineCursor) {
            mUpperPane->needUpdate(mUserCursor.y(), mUserCursor.y() + 1);
        }
        return;

    } else {
        if ((buffer.buffer.empty() && buffer.buffer[0].empty()) || mUserCursor == buffer.getEndPos()) {
            if (customFormat) {
                buffer.addLink(mTriggerEngineMode, text, func, hint, mFormatCurrent);
            } else {
                buffer.addLink(mTriggerEngineMode, text, func, hint, standardLinkFormat);
            }

            mUpperPane->showNewLines();
            mLowerPane->showNewLines();

        } else {
            if (customFormat) {
                buffer.insertInLine(mUserCursor, text, mFormatCurrent);
            } else {
                buffer.insertInLine(mUserCursor, text, standardLinkFormat);
            }

            buffer.applyLink(P, P2, func, hint);
            if (text.indexOf("\n") != -1) {
                int y_tmp = mUserCursor.y();
                int down = buffer.wrapLine(mUserCursor.y(), mpHost->mScreenWidth, mpHost->mWrapIndentCount, mFormatCurrent);
                mUpperPane->needUpdate(y_tmp, y_tmp + down + 1);
                int y_neu = y_tmp + down;
                int x_adjust = text.lastIndexOf("\n");
                int x_neu = 0;
                if (x_adjust != -1) {
                    x_neu = text.size() - x_adjust - 1 > 0 ? text.size() - x_adjust - 1 : 0;
                }
                moveCursor(x_neu, y_neu);
            } else {
                mUpperPane->needUpdate(mUserCursor.y(), mUserCursor.y() + 1);
                moveCursor(mUserCursor.x() + text.size(), mUserCursor.y());
            }
        }
    }
}

void TConsole::insertText(const QString& text, QPoint P)
{
    int x = P.x();
    int y = P.y();
    if (mTriggerEngineMode) {
        mpHost->getLuaInterpreter()->adjustCaptureGroups(x, text.size());
        if (y < mEngineCursor) {
            buffer.insertInLine(P, text, mFormatCurrent);
            mUpperPane->needUpdate(mUserCursor.y(), mUserCursor.y() + 1);
        } else if (y >= mEngineCursor) {
            buffer.insertInLine(P, text, mFormatCurrent);
        }

    } else {
        if ((buffer.buffer.empty() && buffer.buffer[0].empty()) || mUserCursor == buffer.getEndPos()) {
            buffer.append(text, 0, text.size(), mFormatCurrent);
            mUpperPane->showNewLines();
            mLowerPane->showNewLines();
        } else {
            buffer.insertInLine(mUserCursor, text, mFormatCurrent);
            int y_tmp = mUserCursor.y();
            if (text.indexOf(QChar::LineFeed) != -1) {
                int down = buffer.wrapLine(y_tmp, mpHost->mScreenWidth, mpHost->mWrapIndentCount, mFormatCurrent);
                mUpperPane->needUpdate(y_tmp, y_tmp + down + 1);
            } else {
                mUpperPane->needUpdate(y_tmp, y_tmp + 1);
            }
        }

    }
}


void TConsole::replace(const QString& text)
{
    int x = P_begin.x();
    int o = P_end.x() - P_begin.x();
    int r = text.size();

    if (mTriggerEngineMode) {
        if (hasSelection()) {
            if (r < o) {
                int a = -1 * (o - r);
                mpHost->getLuaInterpreter()->adjustCaptureGroups(x, a);
            }
            if (r > o) {
                int a = r - o;
                mpHost->getLuaInterpreter()->adjustCaptureGroups(x, a);
            }
        } else {
            mpHost->getLuaInterpreter()->adjustCaptureGroups(x, r);
        }
    }

    buffer.replaceInLine(P_begin, P_end, text, mFormatCurrent);
}

void TConsole::skipLine()
{
    if (deleteLine(mUserCursor.y())) {
        mDeletedLines++;
    }
}

// TODO: It may be worth considering moving the (now) three following methods
// to the TMap class...?
bool TConsole::saveMap(const QString& location, int saveVersion)
{
    QDir dir_map;
    QString filename_map;
    QString directory_map = mudlet::getMudletPath(mudlet::profileMapsPath, mProfileName);

    if (location.isEmpty()) {
        // CHECKME: Consider changing datetime spec to more "sortable" "yyyy-MM-dd#HH-mm-ss" (6 of 6)
        filename_map = mudlet::getMudletPath(mudlet::profileDateTimeStampedMapPathFileName, mProfileName, QDateTime::currentDateTime().toString(QStringLiteral("dd-MM-yyyy#hh-mm-ss")));
    } else {
        filename_map = location;
    }

    if (!dir_map.exists(directory_map)) {
        dir_map.mkpath(directory_map);
    }
    QFile file_map(filename_map);
    if (file_map.open(QIODevice::WriteOnly)) {
        QDataStream out(&file_map);
        if (mudlet::scmRunTimeQtVersion >= QVersionNumber(5, 13, 0)) {
            out.setVersion(mudlet::scmQDataStreamFormat_5_12);
        }
        mpHost->mpMap->serialize(out, saveVersion);
        file_map.close();
    } else {
        return false;
    }

    return true;
}

bool TConsole::loadMap(const QString& location)
{
    Host* pHost = mpHost;
    if (!pHost) {
        // Check for valid mpHost pointer (mpHost was/is/will be a QPoint<Host>
        // in later software versions and is a weak pointer until used
        // (I think - Slysven ?)
        return false;
    }

    if (!pHost->mpMap || !pHost->mpMap->mpMapper) {
        // No map or map currently loaded - so try and created mapper
        // but don't load a map here by default, we do that below and it may not
        // be the default map anyhow
        mudlet::self()->createMapper(false);
    }

    if (!pHost->mpMap || !pHost->mpMap->mpMapper) {
        // And that failed so give up
        return false;
    }

    pHost->mpMap->mapClear();

    qDebug() << "TConsole::loadMap() - restore map case 1.";
    pHost->mpMap->pushErrorMessagesToFile(tr("Pre-Map loading(1) report"), true);
    QDateTime now(QDateTime::currentDateTime());

    bool result = false;
    if (pHost->mpMap->restore(location)) {
        pHost->mpMap->audit();
        pHost->mpMap->mpMapper->mp2dMap->init();
        pHost->mpMap->mpMapper->updateAreaComboBox();
        pHost->mpMap->mpMapper->resetAreaComboBoxToPlayerRoomArea();
        pHost->mpMap->mpMapper->show();
        result = true;
    } else {
        pHost->mpMap->mpMapper->mp2dMap->init();
        pHost->mpMap->mpMapper->updateAreaComboBox();
        pHost->mpMap->mpMapper->show();
    }

    if (location.isEmpty()) {
        pHost->mpMap->pushErrorMessagesToFile(tr("Loading map(1) at %1 report").arg(now.toString(Qt::ISODate)), true);
    } else {
        pHost->mpMap->pushErrorMessagesToFile(tr(R"(Loading map(1) "%1" at %2 report)").arg(location, now.toString(Qt::ISODate)), true);
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
bool TConsole::importMap(const QString& location, QString* errMsg)
{
    Host* pHost = mpHost;
    if (!pHost) {
        // Check for valid mpHost pointer (mpHost was/is/will be a QPoint<Host>
        // in later software versions and is a weak pointer until used
        // (I think - Slysven ?)
        if (errMsg) {
            *errMsg = QStringLiteral("loadMap: NULL Host pointer {in TConsole::importMap(...)} - something is wrong!");
        }
        return false;
    }

    if (!pHost->mpMap || !pHost->mpMap->mpMapper) {
        // No map or mapper currently loaded/present - so try and create mapper
        mudlet::self()->createMapper(false);
    }

    if (!pHost->mpMap || !pHost->mpMap->mpMapper) {
        // And that failed so give up
        if (errMsg) {
            *errMsg = QStringLiteral("loadMap: unable to initialise mapper {in TConsole::importMap(...)} - something is wrong!");
        }
        return false;
    }

    // Dump any outstanding map errors from past activities that had not yet
    // been logged...
    qDebug() << "TConsole::importingMap() - importing map case 1.";
    pHost->mpMap->pushErrorMessagesToFile(tr("Pre-Map importing(1) report"), true);
    QDateTime now(QDateTime::currentDateTime());

    bool result = false;

    QFileInfo fileInfo(location);
    QString filePathNameString;
    if (!fileInfo.filePath().isEmpty()) {
        if (fileInfo.isRelative()) {
            // Resolve the name relative to the profile home directory:
            filePathNameString = QDir::cleanPath(mudlet::getMudletPath(mudlet::profileDataItemPath, mProfileName, fileInfo.filePath()));
        } else {
            if (fileInfo.exists()) {
                filePathNameString = fileInfo.canonicalFilePath(); // Cannot use cannonical path if file doesn't exist!
            } else {
                filePathNameString = fileInfo.absoluteFilePath();
            }
        }
    }

    QFile file(filePathNameString);
    if (!file.exists()) {
        if (!errMsg) {
            QString infoMsg = tr("[ ERROR ]  - Map file not found, path and name used was:\n"
                                 "%1.")
                                      .arg(filePathNameString);
            pHost->postMessage(infoMsg);
        } else {
            // error message for lua loadMap()
            *errMsg = tr("loadMap: bad argument #1 value (filename used: \n"
                         "\"%1\" was not found).")
                              .arg(filePathNameString);
        }
        return false;
    }

    if (file.open(QFile::ReadOnly | QFile::Text)) {
        if (!errMsg) {
            QString infoMsg = tr("[ INFO ]  - Map file located and opened, now parsing it...");
            pHost->postMessage(infoMsg);
        }

        result = pHost->mpMap->importMap(file, errMsg);

        file.close();
        pHost->mpMap->pushErrorMessagesToFile(tr(R"(Importing map(1) "%1" at %2 report)").arg(location, now.toString(Qt::ISODate)));
    } else {
        if (!errMsg) {
            QString infoMsg = tr(R"([ INFO ]  - Map file located but it could not opened, please check permissions on:"%1".)").arg(filePathNameString);
            pHost->postMessage(infoMsg);
        } else {
            *errMsg = tr("loadMap: bad argument #1 value (filename used: \n"
                         "\"%1\" could not be opened for reading).")
                              .arg(filePathNameString);
        }
        return false;
    }

    return result;
}

bool TConsole::deleteLine(int y)
{
    return buffer.deleteLine(y);
}

bool TConsole::hasSelection()
{
    if (P_begin != P_end) {
        return true;
    } else {
        return false;
    }
}

void TConsole::insertText(const QString& msg)
{
    insertText(msg, mUserCursor);
}

void TConsole::insertLink(const QString& text, QStringList& func, QStringList& hint, bool customFormat)
{
    insertLink(text, func, hint, mUserCursor, customFormat);
}

void TConsole::insertHTML(const QString& text)
{
    insertText(text);
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

QStringList TConsole::getLines(int from, int to)
{
    QStringList ret;
    int delta = abs(from - to);
    for (int i = 0; i < delta; i++) {
        ret << buffer.line(from + i);
    }
    return ret;
}

void TConsole::selectCurrentLine()
{
    selectSection(0, buffer.line(mUserCursor.y()).size());
}

void TConsole::selectCurrentLine(std::string& buf)
{
    QString key = QString::fromUtf8(buf.c_str());
    if (key.isEmpty() || key == QLatin1String("main")) {
        selectCurrentLine();
        return;
    }
    auto pC = mSubConsoleMap.value(key);
    if (pC) {
        pC->selectCurrentLine();
    }
}

std::list<int> TConsole::_getFgColor()
{
    std::list<int> result;
    int x = P_begin.x();
    int y = P_begin.y();
    if (y < 0) {
        return result;
    }
    if (x < 0) {
        return result;
    }
    if (y >= static_cast<int>(buffer.buffer.size())) {
        return result;
    }

    if (static_cast<int>(buffer.buffer.at(y).size()) - 1 >= x) {
        QColor color(buffer.buffer.at(y).at(x).foreground());
        result.push_back(color.red());
        result.push_back(color.green());
        result.push_back(color.blue());
    }

    return result;
}

std::list<int> TConsole::_getBgColor()
{
    std::list<int> result;
    int x = P_begin.x();
    int y = P_begin.y();
    if (y < 0) {
        return result;
    }
    if (x < 0) {
        return result;
    }
    if (y >= static_cast<int>(buffer.buffer.size())) {
        return result;
    }

    if (static_cast<int>(buffer.buffer.at(y).size()) - 1 >= x) {
        QColor color(buffer.buffer.at(y).at(x).background());
        result.push_back(color.red());
        result.push_back(color.green());
        result.push_back(color.blue());
    }
    return result;
}

std::list<int> TConsole::getFgColor(std::string& buf)
{
    QString key = QString::fromUtf8(buf.c_str());
    if (key.isEmpty() || key == QLatin1String("main")) {
        return _getFgColor();
    }
    auto pC = mSubConsoleMap.value(key);
    if (pC) {
        return pC->_getFgColor();
    }

    return {};
}

std::list<int> TConsole::getBgColor(std::string& buf)
{
    QString key = QString::fromUtf8(buf.c_str());
    if (key.isEmpty() || key == QLatin1String("main")) {
        return _getBgColor();
    }
    auto pC = mSubConsoleMap.value(key);
    if (pC) {
        return pC->_getBgColor();
    }

    return {};
}

QPair<quint8, TChar> TConsole::getTextAttributes() const
{
    int x = P_begin.x();
    int y = P_begin.y();
    if (y < 0 || x < 0 || y >= static_cast<int>(buffer.buffer.size()) || x >= (static_cast<int>(buffer.buffer.at(y).size()) - 1)) {
        return qMakePair(2, TChar());
    }

    return qMakePair(0, buffer.buffer.at(y).at(x));
}

QPair<quint8, TChar> TConsole::getTextAttributes(const QString& name) const
{
    if (name.isEmpty() || name == QLatin1String("main")) {
        return getTextAttributes();
    }

    auto pC = mSubConsoleMap.value(name);
    if (pC) {
        return pC->getTextAttributes();
    }

    return qMakePair(1, TChar());
}

void TConsole::luaWrapLine(std::string& buf, int line)
{
    QString key = QString::fromUtf8(buf.c_str());
    if (key.isEmpty() || key == QLatin1String("main")) {
        _luaWrapLine(line);
        return;
    }
    auto pC = mSubConsoleMap.value(key);
    if (pC) {
        pC->_luaWrapLine(line);
    }
}

void TConsole::_luaWrapLine(int line)
{
    if (!mpHost) {
        return;
    }
    TChar ch(mpHost);
    buffer.wrapLine(line, mWrapAt, mIndentCount, ch);
}

bool TConsole::setMiniConsoleFontSize(int size)
{
    mDisplayFontSize = size;

    refreshMiniConsole();
    return true;
}

void TConsole::setMiniConsoleCmdVisible(bool isVisible)
{
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // create MiniConsole commandline if it's not existing
    if (!mpCommandLine) {
        mpCommandLine = new TCommandLine(mpHost, mpCommandLine->ConsoleCommandLine, this, mpMainDisplay);
        mpCommandLine->setContentsMargins(0, 0, 0, 0);
        mpCommandLine->setSizePolicy(sizePolicy);
        mpCommandLine->setFocusPolicy(Qt::StrongFocus);
        // put this CommandLine in the mainConsoles SubCommandLineMap
        // name is the console name
        mpHost->mpConsole->mSubCommandLineMap[mConsoleName] = mpCommandLine;
        mpCommandLine->mCommandLineName = mConsoleName;
        mpCommandLine->setObjectName(mConsoleName);
        layoutLayer2->addWidget(mpCommandLine);
    }
    mpButtonMainLayer->setVisible(false);
    layerCommandLine->setVisible(isVisible);
    mpCommandLine->setVisible(isVisible);
    //resizes miniconsole if command line gets enabled/disabled
    QSize s = QSize(width(), height());
    QResizeEvent event(s, s);
    QApplication::sendEvent(this, &event);
}

void TConsole::refreshMiniConsole() const
{
    mUpperPane->mDisplayFont = QFont(mDisplayFontName, mDisplayFontSize, QFont::Normal);
    mUpperPane->setFont(mUpperPane->mDisplayFont);
    mUpperPane->updateScreenView();
    mUpperPane->forceUpdate();
    mLowerPane->mDisplayFont = QFont(mDisplayFontName, mDisplayFontSize, QFont::Normal);
    mLowerPane->setFont(mLowerPane->mDisplayFont);
    mLowerPane->updateScreenView();
    mLowerPane->forceUpdate();
}

bool TConsole::setMiniConsoleFont(const QString& font)
{
    mDisplayFontName = font;

    refreshMiniConsole();
    return true;
}

QString TConsole::getCurrentLine()
{
    return buffer.line(mUserCursor.y());
}

QString TConsole::getCurrentLine(std::string& buf)
{
    QString key = QString::fromUtf8(buf.c_str());
    if (key.isEmpty() || key == QLatin1String("main")) {
        return getCurrentLine();
    }
    auto pC = mSubConsoleMap.value(key);
    if (pC) {
        return pC->getCurrentLine();
    }
    return QStringLiteral("ERROR: mini console does not exist");
}

int TConsole::getLastLineNumber()
{
    return buffer.getLastLineNumber();
}

void TConsole::moveCursorEnd()
{
    int y = buffer.getLastLineNumber();
    int x = buffer.line(y).size() - 1;
    x = x >= 0 ? x : 0;
    moveCursor(x, y);
}

bool TConsole::moveCursor(int x, int y)
{
    QPoint P(x, y);
    if (buffer.moveCursor(P)) {
        mUserCursor.setX(x);
        mUserCursor.setY(y);
        return true;
    } else {
        return false;
    }
}

int TConsole::select(const QString& text, int numOfMatch)
{
    if (mUserCursor.y() < 0) {
        return -1;
    }
    if (mUserCursor.y() >= buffer.size()) {
        return -1;
    }

    if (mudlet::debugMode) {
        TDebug(QColor(Qt::darkMagenta), QColor(Qt::black)) << "\nline under current user cursor: " >> 0;
        TDebug(QColor(Qt::red), QColor(Qt::black)) << mUserCursor.y() << "#:" >> 0;
        TDebug(QColor(Qt::gray), QColor(Qt::black)) << buffer.line(mUserCursor.y()) << "\n" >> 0;
    }

    int begin = -1;
    for (int i = 0; i < numOfMatch; i++) {
        QString li = buffer.line(mUserCursor.y());
        if (li.size() < 1) {
            continue;
        }
        begin = li.indexOf(text, begin + 1);

        if (begin == -1) {
            P_begin.setX(0);
            P_begin.setY(0);
            P_end.setX(0);
            P_end.setY(0);
            return -1;
        }
    }

    int end = begin + text.size();
    P_begin.setX(begin);
    P_begin.setY(mUserCursor.y());
    P_end.setX(end);
    P_end.setY(mUserCursor.y());

    if (mudlet::debugMode) {
        TDebug(QColor(Qt::darkRed), QColor(Qt::black)) << "P_begin(" << P_begin.x() << "/" << P_begin.y() << "), P_end(" << P_end.x() << "/" << P_end.y()
                                                       << ") selectedText = " << buffer.line(mUserCursor.y()).mid(P_begin.x(), P_end.x() - P_begin.x()) << "\n"
                >> 0;
    }
    return begin;
}

bool TConsole::selectSection(int from, int to)
{
    if (mudlet::debugMode) {
        TDebug(QColor(Qt::darkMagenta), QColor(Qt::black)) << "\nselectSection(" << from << "," << to << "): line under current user cursor: " << buffer.line(mUserCursor.y()) << "\n" >> 0;
    }
    if (from < 0) {
        return false;
    }
    if (mUserCursor.y() >= static_cast<int>(buffer.buffer.size())) {
        return false;
    }
    int s = buffer.buffer[mUserCursor.y()].size();
    if (from > s || from + to > s) {
        return false;
    }
    P_begin.setX(from);
    P_begin.setY(mUserCursor.y());
    P_end.setX(from + to);
    P_end.setY(mUserCursor.y());

    if (mudlet::debugMode) {
        TDebug(QColor(Qt::darkMagenta), QColor(Qt::black)) << "P_begin(" << P_begin.x() << "/" << P_begin.y() << "), P_end(" << P_end.x() << "/" << P_end.y()
                                                           << ") selectedText = " << buffer.line(mUserCursor.y()).mid(P_begin.x(), P_end.x() - P_begin.x()) << "\n"
                >> 0;
    }
    return true;
}

// returns whenever the selection is valid, the selection text,
// start position, and the length of the seletion
std::tuple<bool, QString, int, int> TConsole::getSelection()
{
    if (mUserCursor.y() >= static_cast<int>(buffer.buffer.size())) {
        return std::make_tuple(false, QStringLiteral("the selection is no longer valid"), 0, 0);
    }

    const auto start = P_begin.x();
    const auto length = P_end.x() - P_begin.x();
    const auto line = buffer.line(mUserCursor.y());
    if (line.size() < start) {
        return std::make_tuple(false, QStringLiteral("the selection is no longer valid"), 0, 0);
    }

    const auto text = line.mid(start, length);
    return std::make_tuple(true, text, start, length);
}

void TConsole::setLink(const QStringList& linkFunction, const QStringList& linkHint)
{
    buffer.applyLink(P_begin, P_end, linkFunction, linkHint);
    mUpperPane->forceUpdate();
    mLowerPane->forceUpdate();
}

// Set or Reset ALL the specified (but not others)
void TConsole::setDisplayAttributes(const TChar::AttributeFlags attributes, const bool b)
{
    mFormatCurrent.setAllDisplayAttributes((mFormatCurrent.allDisplayAttributes() & ~(attributes)) | (b ? attributes : TChar::None));
    buffer.applyAttribute(P_begin, P_end, attributes, b);
    mUpperPane->forceUpdate();
    mLowerPane->forceUpdate();
}

void TConsole::setFgColor(int r, int g, int b)
{
    setFgColor(QColor(r, g, b));
    mUpperPane->forceUpdate();
    mLowerPane->forceUpdate();
}

void TConsole::setBgColor(int r, int g, int b)
{
    setBgColor(QColor(r, g, b));
    mUpperPane->forceUpdate();
    mLowerPane->forceUpdate();
}

void TConsole::setBgColor(const QColor& newColor)
{
    mFormatCurrent.setBackground(newColor);
    buffer.applyBgColor(P_begin, P_end, newColor);
    mUpperPane->forceUpdate();
    mLowerPane->forceUpdate();
}

void TConsole::setFgColor(const QColor& newColor)
{
    mFormatCurrent.setForeground(newColor);
    buffer.applyFgColor(P_begin, P_end, newColor);
    mUpperPane->forceUpdate();
    mLowerPane->forceUpdate();
}

void TConsole::setScrollBarVisible(bool isVisible)
{
    if (mpScrollBar) {
        mpScrollBar->setVisible(isVisible);
    }
}

void TConsole::printCommand(QString& msg)
{
    if (mTriggerEngineMode) {
        msg.append(QChar::LineFeed);
        int lineBeforeNewContent = buffer.getLastLineNumber();
        if (lineBeforeNewContent >= 0) {
            if (buffer.lineBuffer.at(lineBeforeNewContent).right(1) != QChar(QChar::LineFeed)) {
                msg.prepend(QChar::LineFeed);
            }
        }
        buffer.appendLine(msg, 0, msg.size() - 1, mCommandFgColor, mCommandBgColor);
    } else {
        int lineBeforeNewContent = buffer.size() - 2;
        if (lineBeforeNewContent >= 0) {
            int promptEnd = buffer.buffer.at(lineBeforeNewContent).size();
            if (promptEnd < 0) {
                promptEnd = 0;
            }
            if (buffer.promptBuffer[lineBeforeNewContent]) {
                QPoint P(promptEnd, lineBeforeNewContent);
                TChar format(mCommandFgColor, mCommandBgColor);
                buffer.insertInLine(P, msg, format);
                int down = buffer.wrapLine(lineBeforeNewContent, mpHost->mScreenWidth, mpHost->mWrapIndentCount, mFormatCurrent);

                mUpperPane->needUpdate(lineBeforeNewContent, lineBeforeNewContent + 1 + down);
                mLowerPane->needUpdate(lineBeforeNewContent, lineBeforeNewContent + 1 + down);
                buffer.promptBuffer[lineBeforeNewContent] = false;
                return;
            }
        }
        msg.append("\n");
        print(msg, mCommandFgColor, mCommandBgColor);
    }
}

void TConsole::echoLink(const QString& text, QStringList& func, QStringList& hint, bool customFormat)
{
    if (customFormat) {
        buffer.addLink(mTriggerEngineMode, text, func, hint, mFormatCurrent);
    } else {
        TChar f = TChar(Qt::blue, (mType == MainConsole ? mpHost->mBgColor : mBgColor), TChar::Underline);
        buffer.addLink(mTriggerEngineMode, text, func, hint, f);
    }
    mUpperPane->showNewLines();
    mLowerPane->showNewLines();
}

TConsole* TConsole::createBuffer(const QString& name)
{
    if (!mSubConsoleMap.contains(name)) {
        auto pC = new TConsole(mpHost, Buffer);
        mSubConsoleMap[name] = pC;
        pC->mConsoleName = name;
        pC->setContentsMargins(0, 0, 0, 0);
        pC->hide();
        pC->layerCommandLine->hide();
        return pC;
    } else {
        return nullptr;
    }
}

void TConsole::resetMainConsole()
{
    //resetProfile should reset also UserWindows
    QMutableMapIterator<QString, TDockWidget*> itDockWidget(mDockWidgetMap);
    while (itDockWidget.hasNext()) {
        itDockWidget.next();
        itDockWidget.value()->close();
        itDockWidget.remove();
    }

    QMutableMapIterator<QString, TCommandLine*> itCommandLine(mSubCommandLineMap);
    while (itCommandLine.hasNext()) {
        itCommandLine.next();
        itCommandLine.value()->deleteLater();
        itCommandLine.remove();
    }

    QMutableMapIterator<QString, TConsole*> itSubConsole(mSubConsoleMap);
    while (itSubConsole.hasNext()) {
        itSubConsole.next();
        // CHECK: Do we need to handle the float/dockable widgets here:
        itSubConsole.value()->close();
        itSubConsole.remove();
    }

    QMutableMapIterator<QString, TLabel*> itLabel(mLabelMap);
    while (itLabel.hasNext()) {
        itLabel.next();
        itLabel.value()->close();
        itLabel.remove();
    }
}

// This is a sub-console overlaid on to the main console
TConsole* TConsole::createMiniConsole(const QString& windowname, const QString& name, int x, int y, int width, int height)
{
    //if pW then add Console as Overlay to the Userwindow
    auto pW = mDockWidgetMap.value(windowname);
    auto pC = mSubConsoleMap.value(name);
    if (!pC) {
        if (!pW) {
            pC = new TConsole(mpHost, SubConsole, mpMainFrame);
        } else {
            pC = new TConsole(mpHost, SubConsole, pW->widget());
        }
        if (!pC) {
            return nullptr;
        }
        mSubConsoleMap[name] = pC;
        pC->setObjectName(name);
        pC->mConsoleName = name;
        pC->setFocusPolicy(Qt::NoFocus);
        const auto& hostCommandLine = mpHost->mpConsole->mpCommandLine;
        pC->setFocusProxy(hostCommandLine);
        pC->mUpperPane->setFocusProxy(hostCommandLine);
        pC->mLowerPane->setFocusProxy(hostCommandLine);
        pC->resize(width, height);
        pC->mOldX = x;
        pC->mOldY = y;
        pC->setContentsMargins(0, 0, 0, 0);
        pC->move(x, y);

        pC->setMiniConsoleFontSize(12);
        pC->show();
        return pC;
    } else {
        return nullptr;
    }
}

TLabel* TConsole::createLabel(const QString& windowname, const QString& name, int x, int y, int width, int height, bool fillBackground, bool clickThrough)
{
    //if pW put Label in Userwindow
    auto pL = mLabelMap.value(name);
    auto pW = mDockWidgetMap.value(windowname);
    if (!pL) {
        if (!pW) {
            pL = new TLabel(mpHost, mpMainFrame);
        } else {
            pL = new TLabel(mpHost, pW->widget());
        }
        mLabelMap[name] = pL;
        pL->setObjectName(name);
        pL->setAutoFillBackground(fillBackground);
        pL->setClickThrough(clickThrough);
        pL->resize(width, height);
        pL->setContentsMargins(0, 0, 0, 0);
        pL->move(x, y);
        pL->show();
        return pL;
    } else {
        return nullptr;
    }
}

std::pair<bool, QString> TConsole::deleteLabel(const QString& name)
{
    if (name.isEmpty()) {
        return {false, QLatin1String("a label cannot have an empty string as its name")};
    }

    auto pL = mLabelMap.take(name);
    if (pL) {
        // Using deleteLater() rather than delete as it seems a safer option
        // given that this item is likely to be linked to some events and
        // suchlike:
        pL->deleteLater();

        // It remains to be seen if the label has "gone" as a result of the
        // above by the time the Lua subsystem processes the following:
        TEvent mudletEvent{};
        mudletEvent.mArgumentList.append(QLatin1String("sysLabelDeleted"));
        mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mudletEvent.mArgumentList.append(name);
        mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mpHost->raiseEvent(mudletEvent);
        return {true, QString()};
    }

    // Message is of the form needed for a Lua API function call run-time error
    return {false, QStringLiteral("label name \"%1\" not found").arg(name)};
}

std::pair<bool, QString> TConsole::setLabelToolTip(const QString& name, const QString& text, double duration)
{
    if (name.isEmpty()) {
        return {false, QStringLiteral("a label cannot have an empty string as its name")};
    }

    auto pL = mLabelMap.value(name);
    if (pL) {
        duration = duration * 1000;
        pL->setToolTip(text);
        pL->setToolTipDuration(duration);
        return {true, QString()};
    }

    // Message is of the form needed for a Lua API function call run-time error
    return {false, QStringLiteral("label name \"%1\" not found").arg(name)};
}

std::pair<bool, QString> TConsole::setLabelCursor(const QString& name, int shape)
{
    if (name.isEmpty()) {
        return {false, QStringLiteral("a label cannot have an empty string as its name")};
    }

    auto pL = mLabelMap.value(name);
    if (pL) {
        if (shape > -1 && shape < 22) {
            pL->setCursor(static_cast<Qt::CursorShape>(shape));
        } else if (shape == -1) {
            pL->unsetCursor();
        } else {
            return {false, QStringLiteral("cursor shape \"%1\" not found. see https://doc.qt.io/qt-5/qt.html#CursorShape-enum").arg(shape)};
        }
        return {true, QString()};
    }
    return {false, QStringLiteral("label name \"%1\" not found").arg(name)};
}

std::pair<bool, QString> TConsole::setLabelCustomCursor(const QString& name, const QString& pixMapLocation, int hotX, int hotY)
{
    if (name.isEmpty()) {
        return {false, QStringLiteral("a label cannot have an empty string as its name")};
    }

    if (pixMapLocation.isEmpty()) {
        return {false, QStringLiteral("custom cursor location cannot be an empty string")};
    }

    auto pL = mLabelMap.value(name);
    if (pL) {
        QPixmap cursor_pixmap = QPixmap(pixMapLocation);
        if (cursor_pixmap.isNull()) {
            return {false, QStringLiteral("couldn't find custom cursor, is the location \"%1\" correct?").arg(pixMapLocation)};
        }
        QCursor custom_cursor = QCursor(cursor_pixmap, hotX, hotY);
        pL->setCursor(custom_cursor);
        return {true, QString()};
    }

    return {false, QStringLiteral("label name \"%1\" not found").arg(name)};
}

std::pair<bool, QString> TConsole::createMapper(const QString& windowname, int x, int y, int width, int height)
{
    auto pW = mDockWidgetMap.value(windowname);
    auto pM = mpHost->mpDockableMapWidget;
    if (pM) {
        return {false, QStringLiteral("cannot create mapper. Do you already use a map window?")};
    }
    if (!mpMapper) {
        // Arrange for TMap member values to be copied from the Host masters so they
        // are in place when the 2D mapper is created:
        mpHost->getPlayerRoomStyleDetails(mpHost->mpMap->mPlayerRoomStyle,
                                          mpHost->mpMap->mPlayerRoomOuterDiameterPercentage,
                                          mpHost->mpMap->mPlayerRoomInnerDiameterPercentage,
                                          mpHost->mpMap->mPlayerRoomOuterColor,
                                          mpHost->mpMap->mPlayerRoomInnerColor);
        if (!pW) {
            mpMapper = new dlgMapper(mpMainFrame, mpHost, mpHost->mpMap.data());
        } else {
            mpMapper = new dlgMapper(pW->widget(), mpHost, mpHost->mpMap.data());
        }
        mpHost->mpMap->mpHost = mpHost;
        mpHost->mpMap->mpMapper = mpMapper;
        qDebug() << "TConsole::createMapper() - restore map case 2.";
        mpHost->mpMap->pushErrorMessagesToFile(tr("Pre-Map loading(2) report"), true);
        QDateTime now(QDateTime::currentDateTime());

        if (mpHost->mpMap->restore(QString())) {
            mpHost->mpMap->audit();
            mpMapper->mp2dMap->init();
            mpMapper->updateAreaComboBox();
            mpMapper->resetAreaComboBoxToPlayerRoomArea();
        }

        mpHost->mpMap->pushErrorMessagesToFile(tr("Loading map(2) at %1 report").arg(now.toString(Qt::ISODate)), true);

        TEvent mapOpenEvent{};
        mapOpenEvent.mArgumentList.append(QLatin1String("mapOpenEvent"));
        mapOpenEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mpHost->raiseEvent(mapOpenEvent);
    }
    mpMapper->resize(width, height);
    mpMapper->move(x, y);

    // Qt bug workaround: on Windows and during profile load only, if the mapper widget is created
    // it gives a height and width to mpLeftToolBar, mpRightToolBar, and mpTopToolBar for
    // some reason. Those widgets size back down immediately after on their own (?!), however if
    // getMainWindowSize() is called right after map create, the sizes reported will be wrong
#if defined(Q_OS_WIN32)
    mpLeftToolBar->setHidden(true);
    mpRightToolBar->setHidden(true);
    mpTopToolBar->setHidden(true);
    mpMapper->show();
    mpLeftToolBar->setVisible(true);
    mpRightToolBar->setVisible(true);
    mpTopToolBar->setVisible(true);
#else
    mpMapper->show();
#endif
    return {true, QString()};
}

std::pair<bool, QString> TConsole::createCommandLine(const QString& windowname, const QString& name, int x, int y, int width, int height)
{
    if (name.isEmpty()) {
        return {false, QLatin1String("a commandLine cannot have an empty string as its name")};
    }

    auto pN = mSubCommandLineMap.value(name);
    auto pW = mDockWidgetMap.value(windowname);

    if (!pN) {
        if (!pW) {
            pN = new TCommandLine(mpHost, mpCommandLine->SubCommandLine, this, mpMainFrame);
        } else {
            pN = new TCommandLine(mpHost, mpCommandLine->SubCommandLine, this, pW->widget());
        }
        mSubCommandLineMap[name] = pN;
        pN->mCommandLineName = name;
        pN->setObjectName(name);
        pN->resize(width, height);
        pN->move(x, y);
        pN->show();
        return {true, QString()};
    }
    return {false, QLatin1String("couldn't create commandLine")};
}

bool TConsole::setBackgroundImage(const QString& name, const QString& path)
{
    auto pL = mLabelMap.value(name);
    if (pL) {
        QPixmap bgPixmap(path);
        pL->setPixmap(bgPixmap);
        return true;
    } else {
        return false;
    }
}

bool TConsole::setBackgroundColor(const QString& name, int r, int g, int b, int alpha)
{
    auto pC = mSubConsoleMap.value(name);
    auto pL = mLabelMap.value(name);
    if (pC) {
        QPalette mainPalette;
        mainPalette.setColor(QPalette::Window, QColor(r, g, b, alpha));
        pC->setPalette(mainPalette);
        pC->mUpperPane->mBgColor = QColor(r, g, b, alpha);
        pC->mLowerPane->mBgColor = QColor(r, g, b, alpha);
        // update the display properly when color selections change.
        pC->mUpperPane->updateScreenView();
        pC->mUpperPane->forceUpdate();
        if (!pC->mUpperPane->mIsTailMode) {
            // The upper pane having mIsTailMode true means lower pane is hidden
            pC->mLowerPane->updateScreenView();
            pC->mLowerPane->forceUpdate();
        }
        return true;
    } else if (pL) {
        QPalette mainPalette;
        mainPalette.setColor(QPalette::Window, QColor(r, g, b, alpha));
        pL->setPalette(mainPalette);
        return true;
    } else {
        return false;
    }
}

bool TConsole::raiseWindow(const QString& name)
{
    auto pC = mSubConsoleMap.value(name);
    auto pL = mLabelMap.value(name);
    auto pM = mpMapper;
    auto pN = mSubCommandLineMap.value(name);

    if (pC) {
        pC->raise();
        return true;
    }
    if (pL) {
        pL->raise();
        return true;
    }
    if (pM && !name.compare(QLatin1String("mapper"), Qt::CaseInsensitive)) {
        pM->raise();
        return true;
    }
    if (pN) {
        pN->raise();
        return true;
    }

    return false;
}

bool TConsole::lowerWindow(const QString& name)
{
    auto pC = mSubConsoleMap.value(name);
    auto pL = mLabelMap.value(name);
    auto pM = mpMapper;
    auto pN = mSubCommandLineMap.value(name);

    if (pC) {
        pC->lower();
        mpMainDisplay->lower();
        return true;
    }
    if (pL) {
        pL->lower();
        mpMainDisplay->lower();
        return true;
    }
    if (pM && !name.compare(QLatin1String("mapper"), Qt::CaseInsensitive)) {
        pM->lower();
        mpMainDisplay->lower();
        return true;
    }
    if (pN) {
        pN->lower();
        return true;
    }
    return false;
}

bool TConsole::showWindow(const QString& name)
{
    auto pC = mSubConsoleMap.value(name);
    auto pL = mLabelMap.value(name);
    if (pC) {
        pC->mUpperPane->updateScreenView();
        pC->mUpperPane->forceUpdate();
        pC->show();

        pC->mLowerPane->updateScreenView();
        pC->mLowerPane->forceUpdate();
        return true;
    } else if (pL) {
        pL->show();
        return true;
    } else {
        return false;
    }
}

bool TConsole::hideWindow(const QString& name)
{
    auto pC = mSubConsoleMap.value(name);
    auto pL = mLabelMap.value(name);
    if (pC) {
        pC->hide();
        return true;
    } else if (pL) {
        pL->hide();
        return true;
    } else {
        return false;
    }
}

bool TConsole::printWindow(const QString& name, const QString& text)
{
    auto pC = mSubConsoleMap.value(name);
    auto pL = mLabelMap.value(name);
    if (pC) {
        pC->print(text);
        return true;
    } else if (pL) {
        pL->setText(text);
        return true;
    } else {
        return false;
    }
}

void TConsole::print(const char* txt)
{
    QString msg(txt);
    buffer.append(msg, 0, msg.size(), mFormatCurrent.foreground(), mFormatCurrent.background(), mFormatCurrent.allDisplayAttributes());
    mUpperPane->showNewLines();
    mLowerPane->showNewLines();
}

// echoUserWindow(const QString& msg) was a redundant wrapper around this method:
void TConsole::print(const QString& msg)
{
    buffer.append(msg, 0, msg.size(), mFormatCurrent.foreground(), mFormatCurrent.background(), mFormatCurrent.allDisplayAttributes());
    mUpperPane->showNewLines();
    mLowerPane->showNewLines();
}

// printDebug(QColor& c, QColor& d, const QString& msg) was functionally the
// same as this method it was just that the arguments were in a different order
void TConsole::print(const QString& msg, const QColor fgColor, const QColor bgColor)
{
    buffer.append(msg, 0, msg.size(), fgColor, bgColor);
    mUpperPane->showNewLines();
    mLowerPane->showNewLines();
}

void TConsole::printSystemMessage(const QString& msg)
{
    QString txt = tr("System Message: %1").arg(msg);
    print(txt, mSystemMessageFgColor, mSystemMessageBgColor);
}

void TConsole::echo(const QString& msg)
{
    if (mTriggerEngineMode) {
        buffer.appendLine(msg, 0, msg.size() - 1, mFormatCurrent.foreground(), mFormatCurrent.background(), mFormatCurrent.allDisplayAttributes());
    } else {
        print(msg);
    }
}

void TConsole::copy()
{
    mpHost->mpConsole->mClipboard = buffer.copy(P_begin, P_end);
}

void TConsole::cut()
{
    mClipboard = buffer.cut(P_begin, P_end);
}

void TConsole::paste()
{
    if (buffer.size() - 1 > mUserCursor.y()) {
        buffer.paste(mUserCursor, mClipboard);
        mUpperPane->needUpdate(mUserCursor.y(), mUserCursor.y());
    } else {
        buffer.appendBuffer(mClipboard);
    }
    mUpperPane->showNewLines();
    mLowerPane->showNewLines();
}

void TConsole::pasteWindow(TBuffer bufferSlice)
{
    mClipboard = bufferSlice;
    paste();
}

void TConsole::appendBuffer()
{
    buffer.appendBuffer(mClipboard);
    mUpperPane->showNewLines();
    mLowerPane->showNewLines();
}

void TConsole::appendBuffer(TBuffer bufferSlice)
{
    buffer.appendBuffer(bufferSlice);
    mUpperPane->showNewLines();
    mLowerPane->showNewLines();
}

void TConsole::slot_stop_all_triggers(bool b)
{
    if (b) {
        mpHost->stopAllTriggers();
        emergencyStop->setIcon(QIcon(QStringLiteral(":/icons/red-bomb.png")));
    } else {
        mpHost->reenableAllTriggers();
        emergencyStop->setIcon(QIcon(QStringLiteral(":/icons/edit-bomb.png")));
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
    print(msg, QColor(150, 120, 0), Qt::black);

    QString script = "setFgColor(190,150,0); setUnderline(true);echo([[\n\nGMCP events:\n]]);setUnderline(false);setFgColor(150,120,0);display( gmcp );";
    mpHost->mLuaInterpreter.compileAndExecuteScript(script);
    script = "setFgColor(190,150,0); setUnderline(true);echo([[\n\nATCP events:\n]]);setUnderline(false);setFgColor(150,120,0); display( atcp );";
    mpHost->mLuaInterpreter.compileAndExecuteScript(script);
    script = "setFgColor(190,150,0); setUnderline(true);echo([[\n\nchannel102 events:\n]]);setUnderline(false);setFgColor(150,120,0);display( channel102 );";
    mpHost->mLuaInterpreter.compileAndExecuteScript(script);


    script = "setFgColor(190,150,0); setUnderline(true); echo([[\n\nTrigger Report:\n\n]]); setBold(false);setUnderline(false);setFgColor(150,120,0)";
    mpHost->mLuaInterpreter.compileAndExecuteScript(script);
    QString r1 = mpHost->getTriggerUnit()->assembleReport();
    msg = r1;
    print(msg, QColor(150, 120, 0), Qt::black);
    script = "setFgColor(190,150,0); setUnderline(true);echo([[\n\nTimer Report:\n\n]]);setBold(false);setUnderline(false);setFgColor(150,120,0)";
    mpHost->mLuaInterpreter.compileAndExecuteScript(script);
    QString r2 = mpHost->getTimerUnit()->assembleReport();
    msg = r2;
    print(msg, QColor(150, 120, 0), Qt::black);

    script = "setFgColor(190,150,0); setUnderline(true);echo([[\n\nKeybinding Report:\n\n]]);setBold(false);setUnderline(false);setFgColor(150,120,0)";
    mpHost->mLuaInterpreter.compileAndExecuteScript(script);
    QString r3 = mpHost->getKeyUnit()->assembleReport();
    msg = r3;
    print(msg, QColor(150, 120, 0), Qt::black);

    QString footer = QString("\n+--------------------------------------------------------------+\n");
    mpHost->mpConsole->print(footer, QColor(150, 120, 0), Qt::black);
    script = "resetFormat();";
    mpHost->mLuaInterpreter.compileAndExecuteScript(script);

    mpHost->mpConsole->raise();
}

void TConsole::slot_searchBufferUp()
{
    // The search term entry box is one widget that does not pass a mouse press
    // event up to the main TConsole and thus does not cause the focus to shift
    // to the profile's tab when in multi-view mode - so add a call to make that
    // happen:
    mudlet::self()->activateProfile(mpHost);

    QString _txt = mpBufferSearchBox->text();
    if (_txt != mSearchQuery) {
        mSearchQuery = _txt;
        mCurrentSearchResult = buffer.lineBuffer.size();
    } else {
        // make sure the line to search from does not exceed the buffer, which can grow and shrink dynamically
        mCurrentSearchResult = std::min(mCurrentSearchResult, buffer.lineBuffer.size());
    }
    if (buffer.lineBuffer.empty()) {
        return;
    }
    bool _found = false;
    for (int i = mCurrentSearchResult - 1; i >= 0; i--) {
        int begin = -1;
        do {
            begin = buffer.lineBuffer[i].indexOf(mSearchQuery, begin + 1);
            if (begin > -1) {
                int length = mSearchQuery.size();
                moveCursor(0, i);
                selectSection(begin, length);
                setBgColor(255, 255, 0);
                setFgColor(0, 0, 0);
                deselect();
                reset();
                _found = true;
            }
        } while (begin > -1);
        if (_found) {
            scrollUp(buffer.mCursorY - i - 3);
            mUpperPane->forceUpdate();
            mCurrentSearchResult = i;
            return;
        }
    }
    print(QStringLiteral("%1\n").arg(tr("No search results, sorry!")));
}

void TConsole::slot_searchBufferDown()
{
    QString _txt = mpBufferSearchBox->text();
    if (_txt != mSearchQuery) {
        mSearchQuery = _txt;
        mCurrentSearchResult = buffer.lineBuffer.size();
    }
    if (buffer.lineBuffer.empty()) {
        return;
    }
    if (mCurrentSearchResult >= buffer.lineBuffer.size()) {
        return;
    }
    bool _found = false;
    for (int i = mCurrentSearchResult + 1; i < buffer.lineBuffer.size(); i++) {
        int begin = -1;
        do {
            begin = buffer.lineBuffer[i].indexOf(mSearchQuery, begin + 1);
            if (begin > -1) {
                int length = mSearchQuery.size();
                moveCursor(0, i);
                selectSection(begin, length);
                setBgColor(255, 255, 0);
                setFgColor(0, 0, 0);
                deselect();
                reset();
                _found = true;
            }
        } while (begin > -1);
        if (_found) {
            scrollUp(buffer.mCursorY - i - 3);
            mUpperPane->forceUpdate();
            mCurrentSearchResult = i;
            return;
        }
    }
    print(QStringLiteral("%1\n").arg(tr("No search results, sorry!")));
}

QSize TConsole::getMainWindowSize() const
{
    QSize consoleSize = size();
    int toolbarWidth = mpLeftToolBar->width() + mpRightToolBar->width();
    int toolbarHeight = mpTopToolBar->height();
    int commandLineHeight = mpCommandLine->height();
    QSize mainWindowSize(consoleSize.width() - toolbarWidth, consoleSize.height() - (commandLineHeight + toolbarHeight));
    return mainWindowSize;
}
//getUserWindowSize for resizing in Geyser
QSize TConsole::getUserWindowSize(const QString& windowname) const
{
    auto pW = mDockWidgetMap.value(windowname);
    if (pW){
        QSize windowSize = pW->widget()->size();
        QSize userWindowSize(windowSize.width(), windowSize.height());
        return userWindowSize;
    }
    return getMainWindowSize();
}

void TConsole::slot_reloadMap(QList<QString> profilesList)
{
    Host* pHost = getHost();
    if (!pHost) {
        return;
    }

    if (!profilesList.contains(mProfileName)) {
        qDebug() << "TConsole::slot_reloadMap(" << profilesList << ") request received but we:" << mProfileName << "are not mentioned - so we are ignoring it...!";
        return;
    }

    QString infoMsg = tr("[ INFO ]  - Map reload request received from system...");
    pHost->postMessage(infoMsg);

    QString outcomeMsg;
    if (loadMap(QString())) {
        outcomeMsg = tr("[  OK  ]  - ... System Map reload request completed.");
    } else {
        outcomeMsg = tr("[ WARN ]  - ... System Map reload request failed.");
    }

    pHost->postMessage(outcomeMsg);
}

QPair<bool, QString> TConsole::addWordToSet(const QString& word)
{
    QString errMsg = QStringLiteral("the word \"%1\" already seems to be in the user dictionary");
    QPair<bool, QString> result{};
    if (!mEnableUserDictionary) {
        return qMakePair(false, QLatin1String("a user dictionary is not enable for this profile"));
    }

    if (!mUseSharedDictionary) {
        // The return value from this function is unclear - it does not seems to
        // indicate anything useful
        Hunspell_add(mpHunspell_profile, word.toUtf8().constData());
        if (!mWordSet_profile.contains(word)) {
            mWordSet_profile.insert(word);
            qDebug().noquote().nospace() << "TConsole::addWordToSet(\"" << word << "\") INFO - word added to profile mWordSet.";
            result.first = true;
        } else {
            result.second = errMsg.arg(word);
        }

    } else {
        auto pMudlet = mudlet::self();
        QPair<bool, bool> sharedDictionaryResult = pMudlet->addWordToSet(word);
        while (!sharedDictionaryResult.first) {
            qDebug() << "TConsole::addWordToSet(...) ALERT - failed to get a write lock to access mWordSet_shared and loaded shared hunspell dictionary, retrying...";
            sharedDictionaryResult = pMudlet->addWordToSet(word);
        }

        if (sharedDictionaryResult.second) {
            // Successfully added word:
            result.first = true;
        } else {
            // Word already present
            result.second = errMsg.arg(word);
        }
    }

    return result;
}

QPair<bool, QString> TConsole::removeWordFromSet(const QString& word)
{
    QString errMsg = QStringLiteral("the word \"%1\" does not seem to be in the user dictionary");
    QPair<bool, QString> result{};
    if (!mEnableUserDictionary) {
        return qMakePair(false, QLatin1String("a user dictionary is not enable for this profile"));
    }

    if (!mUseSharedDictionary) {
        // The return value from this function is unclear - it does not seems to
        // indicate anything useful
        Hunspell_remove(mpHunspell_profile, word.toUtf8().constData());
        if (mWordSet_profile.remove(word)) {
            qDebug().noquote().nospace() << "TConsole::removeWordFromSet(\"" << word << "\") INFO - word removed from profile mWordSet.";
            result.first = true;
        } else {
            result.second = errMsg.arg(word);
        }

    } else {
        auto pMudlet = mudlet::self();
        QPair<bool, bool> sharedDictionaryResult = pMudlet->removeWordFromSet(word);
        while (!sharedDictionaryResult.first) {
            qDebug() << "TConsole::removeWordFromSet(...) ALERT - failed to get a write lock to access mWordSet_shared and loaded shared hunspell dictionary, retrying...";
            sharedDictionaryResult = pMudlet->removeWordFromSet(word);
        }

        if (sharedDictionaryResult.second) {
            // Successfully added word:
            result.first = true;
        } else {
            // Word already present
            result.second = errMsg.arg(word);
        }
    }

    return result;
}

void TConsole::setSystemSpellDictionary(const QString& newDict)
{
    if (newDict.isEmpty() || mSpellDic == newDict) {
        return;
    }

    mSpellDic = newDict;

    QString path = mudlet::getMudletPath(mudlet::hunspellDictionaryPath, mpHost->getSpellDic());
    QString spell_aff = QStringLiteral("%1%2.aff").arg(path, newDict);
    QString spell_dic = QStringLiteral("%1%2.dic").arg(path, newDict);

    if (mpHunspell_system) {
        Hunspell_destroy(mpHunspell_system);
    }

#if defined(Q_OS_WIN32)
    // strip non-ASCII characters from the path because hunspell can't handle them
    // when compiled with MinGW 7.3.0
    mudlet::self()->sanitizeUtf8Path(spell_aff, QStringLiteral("%1.aff").arg(newDict));
    mudlet::self()->sanitizeUtf8Path(spell_dic, QStringLiteral("%1.dic").arg(newDict));
#endif

    mpHunspell_system = Hunspell_create(spell_aff.toUtf8().constData(), spell_dic.toUtf8().constData());
    if (mpHunspell_system) {
        mHunspellCodecName_system = QByteArray(Hunspell_get_dic_encoding(mpHunspell_system));
        qDebug().noquote().nospace() << "TCommandLine::setSystemSpellDictionary(\"" << newDict << "\") INFO - System Hunspell dictionary loaded for profile, it uses a \"" << Hunspell_get_dic_encoding(mpHunspell_system) << "\" encoding...";
        mpHunspellCodec_system = QTextCodec::codecForName(mHunspellCodecName_system);
    }
}

// NOTE: mEnabledUserDictionary has been wedged on (it will never be false)
void TConsole::setProfileSpellDictionary()
{
    // Determine and copy the configuration settings from the Host instance:
    mpHost->getUserDictionaryOptions(mEnableUserDictionary, mUseSharedDictionary);
    if (!mEnableUserDictionary) {
        if (mpHunspell_profile) {
            Hunspell_destroy(mpHunspell_profile);
            mpHunspell_profile = nullptr;
            // Need to commit any changes to personal dictionary
            qDebug() << "TConsole::setProfileSpellDictionary() INFO - Saving profile's own Hunspell dictionary...";
            mudlet::self()->saveDictionary(mudlet::self()->getMudletPath(mudlet::profileDataItemPath, mProfileName, QStringLiteral("profile")), mWordSet_profile);
        }
        // Nothing else to do if not using the shared one

    } else {
        if (!mUseSharedDictionary) {
            // Want to use per profile dictionary, is it loaded?
            if (!mpHunspell_profile) {
                // No - so load it
                qDebug() << "TConsole::setProfileSpellDictionary() INFO - Preparing profile's own Hunspell dictionary...";
                mpHunspell_profile = mudlet::self()->prepareProfileDictionary(mpHost->getName(), mWordSet_profile);
            }
            // Else no need to load it

        } else {
            // Want to use the shared dictionary - this will open it if needed:
            mpHunspell_shared = mudlet::self()->prepareSharedDictionary();
        }
    }
}

QSet<QString> TConsole::getWordSet() const
{
    if (!mEnableUserDictionary) {
        return QSet<QString>();
    }

    if (!mUseSharedDictionary) {
        return mWordSet_profile;
    } else {
        return mudlet::self()->getWordSet();
    }
}

void TConsole::setProfileName(const QString& newName)
{
    mProfileName = newName;
    if (mType != MainConsole) {
        return;
    }

    for (auto pC : mSubConsoleMap) {
        pC->setProfileName(newName);
    }
}


void TConsole::dragEnterEvent(QDragEnterEvent* e)
{
    if (e->mimeData()->hasUrls()) {
        e->acceptProposedAction();
    }
}

//https://amin-ahmadi.com/2016/01/04/qt-drag-drop-files-images/
void TConsole::dropEvent(QDropEvent* e)
{
    for (const auto& url : e->mimeData()->urls()) {
        QString fname = url.toLocalFile();
        QFileInfo info(fname);
        if (info.exists()) {
            QPoint pos = e->pos();
            TEvent mudletEvent{};
            mudletEvent.mArgumentList.append(QLatin1String("sysDropEvent"));
            mudletEvent.mArgumentList.append(fname);
            mudletEvent.mArgumentList.append(info.suffix().trimmed());
            mudletEvent.mArgumentList.append(QString::number(pos.x()));
            mudletEvent.mArgumentList.append(QString::number(pos.y()));
            mudletEvent.mArgumentList.append(mConsoleName);
            mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
            mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
            mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
            mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
            mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
            mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
            mpHost->raiseEvent(mudletEvent);
        }
    }
}

std::pair<bool, QString> TConsole::setUserWindowTitle(const QString& name, const QString& text)
{
    if (name.isEmpty()) {
        return {false, QStringLiteral("a user window cannot have an empty string as its name")};
    }

    auto pC = mSubConsoleMap.value(name);
    if (!pC) {
        return {false, QStringLiteral("user window name \"%1\" not found").arg(name)};
    }

    // If it does not have an mType of UserWindow then it does not in a
    // floatable/dockable widget - so it can't have a titlebar...!
    if (pC->getType() != UserWindow) {
        return {false, QStringLiteral("\"%1\" is not a user window").arg(name)};
    }

    auto pD = mDockWidgetMap.value(name);
    if (Q_LIKELY(pD)) {
        if (text.isEmpty()) {
            // Reset to default text:
            pD->setWindowTitle(tr("User window - %1 - %2").arg(mpHost->getName(), name));
            return {true, QString()};
        }

        pD->setWindowTitle(text);
        return {true, QString()};
    }

    // This should be:
    Q_UNREACHABLE();
    // as it means that the TConsole is flagged as being a user window yet
    // it does not have a TDockWidget to hold it...
    return {false, QStringLiteral("internal error: TConsole \"%1\" is marked as a user window but does not have a TDockWidget to contain it").arg(name)};
}

// This is also called from the TTextEdit mouse(Press|Release)Event()s:
void TConsole::raiseMudletMousePressOrReleaseEvent(QMouseEvent* event, const bool isPressEvent)
{
    // Ensure that this profile is the one that has it's tab selected in a
    // multi-view situation:
    mudlet::self()->activateProfile(mpHost);

    TEvent mudletEvent{};
    mudletEvent.mArgumentList.append(isPressEvent ? QStringLiteral("sysWindowMousePressEvent") : QStringLiteral("sysWindowMouseReleaseEvent"));
    switch (event->button()) {
    case Qt::LeftButton:    mudletEvent.mArgumentList.append(QString::number(1));   break;
    case Qt::RightButton:   mudletEvent.mArgumentList.append(QString::number(2));   break;
    case Qt::MidButton:     mudletEvent.mArgumentList.append(QString::number(3));   break;
    case Qt::BackButton:    mudletEvent.mArgumentList.append(QString::number(4));   break;
    case Qt::ForwardButton: mudletEvent.mArgumentList.append(QString::number(5));   break;
    case Qt::TaskButton:    mudletEvent.mArgumentList.append(QString::number(6));   break;
    case Qt::ExtraButton4:  mudletEvent.mArgumentList.append(QString::number(7));   break;
    case Qt::ExtraButton5:  mudletEvent.mArgumentList.append(QString::number(8));   break;
    case Qt::ExtraButton6:  mudletEvent.mArgumentList.append(QString::number(9));   break;
    case Qt::ExtraButton7:  mudletEvent.mArgumentList.append(QString::number(10));  break;
    case Qt::ExtraButton8:  mudletEvent.mArgumentList.append(QString::number(11));  break;
    case Qt::ExtraButton9:  mudletEvent.mArgumentList.append(QString::number(12));  break;
    case Qt::ExtraButton10: mudletEvent.mArgumentList.append(QString::number(13));  break;
    case Qt::ExtraButton11: mudletEvent.mArgumentList.append(QString::number(14));  break;
    case Qt::ExtraButton12: mudletEvent.mArgumentList.append(QString::number(15));  break;
    case Qt::ExtraButton13: mudletEvent.mArgumentList.append(QString::number(16));  break;
    case Qt::ExtraButton14: mudletEvent.mArgumentList.append(QString::number(17));  break;
    case Qt::ExtraButton15: mudletEvent.mArgumentList.append(QString::number(18));  break;
    case Qt::ExtraButton16: mudletEvent.mArgumentList.append(QString::number(19));  break;
    case Qt::ExtraButton17: mudletEvent.mArgumentList.append(QString::number(20));  break;
    case Qt::ExtraButton18: mudletEvent.mArgumentList.append(QString::number(21));  break;
    case Qt::ExtraButton19: mudletEvent.mArgumentList.append(QString::number(22));  break;
    case Qt::ExtraButton20: mudletEvent.mArgumentList.append(QString::number(23));  break;
    case Qt::ExtraButton21: mudletEvent.mArgumentList.append(QString::number(24));  break;
    case Qt::ExtraButton22: mudletEvent.mArgumentList.append(QString::number(25));  break;
    case Qt::ExtraButton23: mudletEvent.mArgumentList.append(QString::number(26));  break;
    case Qt::ExtraButton24: mudletEvent.mArgumentList.append(QString::number(27));  break;
    default:                mudletEvent.mArgumentList.append(QString::number(0));
    }
    mudletEvent.mArgumentList.append(QString::number(event->x()));
    mudletEvent.mArgumentList.append(QString::number(event->y()));
    mudletEvent.mArgumentList.append(mConsoleName);
    mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    mpHost->raiseEvent(mudletEvent);
}

// This does not tend to get the leftButton press events as they tend to be
// captured by the TTextEdit or TCommandLine or other widgets within this class:
void TConsole::mousePressEvent(QMouseEvent* event)
{
    raiseMudletMousePressOrReleaseEvent(event, true);
}

void TConsole::mouseReleaseEvent(QMouseEvent* event)
{
    raiseMudletMousePressOrReleaseEvent(event, false);
}

bool TConsole::setTextFormat(const QString& name, const QColor& fgColor, const QColor& bgColor, const TChar::AttributeFlags& flags)
{
    if (name.isEmpty() || name.compare(QStringLiteral("main"), Qt::CaseSensitive) == 0) {
        mFormatCurrent.setTextFormat(fgColor, bgColor, flags);
        return true;
    }

    auto pC = mSubConsoleMap.value(name);
    if (pC) {
        pC->mFormatCurrent.setTextFormat(fgColor, bgColor, flags);
        return true;
    }

    return false;
}
