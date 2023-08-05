/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014-2023 by Stephen Lyons - slysven@virginmedia.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016 by Ian Adkins - ieadkins@gmail.com                 *
 *   Copyright (C) 2021 by Vadim Peretokin - vperetokin@gmail.com          *
 *   Copyright (C) 2022 by Thiago Jung Bauermann - bauermann@kolabnow.com  *
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
#include "TMainConsole.h"
#include "TMap.h"
#include "TRoomDB.h"
#include "TSplitter.h"
#include "TTextEdit.h"
#include "dlgMapper.h"
#include "mudlet.h"

#include "pre_guard.h"
#include <QAccessibleInterface>
#include <QAccessibleWidget>
#include <QLineEdit>
#include <QMessageBox>
#include <QMimeData>
#include <QScrollBar>
#include <QShortcut>
#include <QTextBoundaryFinder>
#include <QTextCodec>
#include <QPainter>
#include "post_guard.h"

const QString TConsole::cmLuaLineVariable("line");

// A high-performance text widget with split screen ability for scrolling back
// Contains two TTextEdits, and is backed by a TBuffer
TConsole::TConsole(Host* pH, const QString& name, const ConsoleType type, QWidget* parent)
: QWidget(parent)
, mpHost(pH)
, buffer(pH, this)
, emergencyStop(new QToolButton)
, mConsoleName(name)
, mpBaseVFrame(new QWidget(this))
, mpTopToolBar(new QWidget(mpBaseVFrame))
, mpBaseHFrame(new QWidget(mpBaseVFrame))
, mpLeftToolBar(new QWidget(mpBaseHFrame))
, mpMainFrame(new QWidget(mpBaseHFrame))
, mpRightToolBar(new QWidget(mpBaseHFrame))
, mpMainDisplay(new QWidget(mpMainFrame))
, mpScrollBar(new QScrollBar)
, mpHScrollBar(new QScrollBar(Qt::Horizontal))
, mProfileName(mpHost ? mpHost->getName() : qsl("debug console"))
, mpBufferSearchBox(new QLineEdit)
, mpBufferSearchUp(new QToolButton)
, mpBufferSearchDown(new QToolButton)
, mControlCharacter(pH->getControlCharacterMode())
, mType(type)
{
    auto quitShortcut = new QShortcut(this);
    quitShortcut->setKey(Qt::CTRL | Qt::Key_W);
    quitShortcut->setContext(Qt::WidgetShortcut);

    if (mType == CentralDebugConsole) {
        // Probably will not show up as this is used inside a QMainWindow widget
        // which has its own title and icon set.
        setWindowTitle(tr("Debug Console"));
        mWrapAt = 50;
    } else if (mType == MainConsole) {
        mBorders = mpHost->borders();
        mCommandBgColor = mpHost->mCommandBgColor;
        mCommandFgColor = mpHost->mCommandFgColor;
    }
    setContentsMargins(0, 0, 0, 0);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_OpaquePaintEvent); //was disabled

    QSizePolicy const sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QSizePolicy const sizePolicy3(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QSizePolicy const sizePolicy2(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QSizePolicy const sizePolicy4(QSizePolicy::Fixed, QSizePolicy::Expanding);
    QSizePolicy const sizePolicy5(QSizePolicy::Fixed, QSizePolicy::Fixed);

    mpMainFrame->setContentsMargins(0, 0, 0, 0);

    QPalette framePalette;
    framePalette.setColor(QPalette::Text, QColor(Qt::black));
    framePalette.setColor(QPalette::Highlight, QColor(55, 55, 255));
    framePalette.setColor(QPalette::Window, QColor(0, 0, 0, 255));
    mpMainFrame->setPalette(framePalette);
    mpMainFrame->setAutoFillBackground(true);
    mpMainFrame->setObjectName(qsl("MainFrame"));

    auto centralLayout = new QVBoxLayout;
    setLayout(centralLayout);
    auto baseVFrameLayout = new QVBoxLayout;
    mpBaseVFrame->setLayout(baseVFrameLayout);
    baseVFrameLayout->setContentsMargins(0, 0, 0, 0);
    baseVFrameLayout->setSpacing(0);
    centralLayout->addWidget(mpBaseVFrame);
    auto baseHFrameLayout = new QHBoxLayout;
    mpBaseHFrame->setLayout(baseHFrameLayout);
    baseHFrameLayout->setContentsMargins(0, 0, 0, 0);
    baseHFrameLayout->setSpacing(0);
    layout()->setSpacing(0);
    layout()->setContentsMargins(0, 0, 0, 0);
    setContentsMargins(0, 0, 0, 0);

    auto topBarLayout = new QHBoxLayout;
    mpTopToolBar->setLayout(topBarLayout);
    mpTopToolBar->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
    mpTopToolBar->setContentsMargins(0, 0, 0, 0);
    mpTopToolBar->setAutoFillBackground(true);

    topBarLayout->setContentsMargins(0, 0, 0, 0);
    topBarLayout->setSpacing(0);
    auto leftBarLayout = new QVBoxLayout;
    mpLeftToolBar->setLayout(leftBarLayout);
    mpLeftToolBar->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding));
    mpLeftToolBar->setAutoFillBackground(true);
    leftBarLayout->setContentsMargins(0, 0, 0, 0);
    leftBarLayout->setSpacing(0);
    mpLeftToolBar->setContentsMargins(0, 0, 0, 0);
    auto rightBarLayout = new QVBoxLayout;
    mpRightToolBar->setLayout(rightBarLayout);
    mpRightToolBar->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding));
    mpRightToolBar->setAutoFillBackground(true);
    rightBarLayout->setContentsMargins(0, 0, 0, 0);
    rightBarLayout->setSpacing(0);
    mpRightToolBar->setContentsMargins(0, 0, 0, 0);
    mpBaseVFrame->setContentsMargins(0, 0, 0, 0);
    baseVFrameLayout->setSpacing(0);
    baseVFrameLayout->setContentsMargins(0, 0, 0, 0);
    mpTopToolBar->setContentsMargins(0, 0, 0, 0);
    baseVFrameLayout->addWidget(mpTopToolBar);
    baseVFrameLayout->addWidget(mpBaseHFrame);
    baseHFrameLayout->addWidget(mpLeftToolBar);
    auto mpCorePane = new QWidget(mpBaseHFrame);
    auto coreSpreadLayout = new QVBoxLayout;
    mpCorePane->setLayout(coreSpreadLayout);
    mpCorePane->setContentsMargins(0, 0, 0, 0);
    coreSpreadLayout->setContentsMargins(0, 0, 0, 0);
    coreSpreadLayout->setSpacing(0);
    coreSpreadLayout->addWidget(mpMainFrame);
    mpCorePane->setSizePolicy(sizePolicy);
    baseHFrameLayout->addWidget(mpCorePane);
    baseHFrameLayout->addWidget(mpRightToolBar);
    mpTopToolBar->setContentsMargins(0, 0, 0, 0);
    mpBaseHFrame->setAutoFillBackground(true);
    baseHFrameLayout->setSpacing(0);
    baseHFrameLayout->setContentsMargins(0, 0, 0, 0);
    setContentsMargins(0, 0, 0, 0);
    mpBaseHFrame->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    mpMainDisplay->move(mBorders.left(), mBorders.top());
    mpMainFrame->show();
    mpMainDisplay->show();
    mpMainFrame->setContentsMargins(0, 0, 0, 0);
    mpMainDisplay->setContentsMargins(0, 0, 0, 0);
    auto layout = new QVBoxLayout;
    mpMainDisplay->setObjectName(qsl("MainDisplay"));
    mpMainDisplay->setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    mpBaseVFrame->setSizePolicy(sizePolicy);
    mpBaseHFrame->setSizePolicy(sizePolicy);

    baseVFrameLayout->setContentsMargins(0, 0, 0, 0);
    baseHFrameLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setContentsMargins(0, 0, 0, 0);

    if (mType == MainConsole) {
        mpCommandLine = new TCommandLine(pH, qsl("main"), TCommandLine::MainCommandLine, this, mpMainDisplay);
        mpCommandLine->setContentsMargins(0, 0, 0, 0);
        mpCommandLine->setSizePolicy(sizePolicy);
        // Setting the focusProxy cannot be done here because things have not
        // been completed enough at this point - it has been defered to a
        // zero-timer at the end of this constructor
    }

    layer = new QWidget(mpMainDisplay);
    layer->setObjectName(qsl("layer"));
    layer->setStyleSheet("QWidget#layer{background-color: rgba(0,0,0,0)}");
    layer->setContentsMargins(0, 0, 0, 0);
    layer->setSizePolicy(sizePolicy);

    auto vLayoutLayer = new QVBoxLayout;
    auto layoutLayer = new QHBoxLayout;
    layer->setLayout(vLayoutLayer);
    layoutLayer->setContentsMargins(0, 0, 0, 0);
    layoutLayer->setSpacing(0);

    mpScrollBar->setFixedWidth(15);
    mpHScrollBar->setFixedHeight(15);

    splitter = new TSplitter(Qt::Vertical);
    splitter->setContentsMargins(0, 0, 0, 0);
    splitter->setSizePolicy(sizePolicy);
    splitter->setOrientation(Qt::Vertical);
    splitter->setHandleWidth(3);
    //QSplitter covers the background if not set to transparent and a new AppStyleSheet is set for example by DarkTheme
    auto styleSheet = qsl("QSplitter { background-color: rgba(0,0,0,0) }");
    splitter->setStyleSheet(styleSheet);
    splitter->setParent(layer);

    mUpperPane = new TTextEdit(this, splitter, &buffer, mpHost, false);
    mUpperPane->setContentsMargins(0, 0, 0, 0);
    mUpperPane->setSizePolicy(sizePolicy3);
    mUpperPane->setAccessibleName(tr("main window"));

    mLowerPane = new TTextEdit(this, splitter, &buffer, mpHost, true);
    mLowerPane->setContentsMargins(0, 0, 0, 0);
    mLowerPane->setSizePolicy(sizePolicy3);

    if (mType == MainConsole) {
        setFocusProxy(mpCommandLine);
        mUpperPane->setFocusProxy(mpCommandLine);
        mLowerPane->setFocusProxy(mpCommandLine);
    } else if (mType & (UserWindow|SubConsole)) {
        // These will need to be changed when the built in TCommandLine is
        // enabled or an additional one is added to them:
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
    layoutLayer->setSpacing(1); // not closer, otherwise there could be performance problems when displaying

    vLayoutLayer->addLayout(layoutLayer);
    vLayoutLayer->addWidget(mpHScrollBar);
    vLayoutLayer->setContentsMargins(0, 0, 0, 0);
    vLayoutLayer->setSpacing(0);

    layerCommandLine = new QWidget; //( mpMainFrame );//layer );
    layerCommandLine->setContentsMargins(0, 0, 0, 0);
    layerCommandLine->setSizePolicy(sizePolicy2);
    layerCommandLine->setMaximumHeight(31);
    layerCommandLine->setMinimumHeight(31);

    layoutLayer2 = new QHBoxLayout(layerCommandLine);
    layoutLayer2->setContentsMargins(0, 0, 0, 0);
    layoutLayer2->setSpacing(0);

    mpButtonMainLayer = new QWidget;
    mpButtonMainLayer->setObjectName(qsl("mpButtonMainLayer"));
    mpButtonMainLayer->setSizePolicy(sizePolicy);
    mpButtonMainLayer->setContentsMargins(0, 0, 0, 0);
    auto layoutButtonMainLayer = new QVBoxLayout(mpButtonMainLayer);
    layoutButtonMainLayer->setObjectName(qsl("layoutButtonMainLayer"));
    layoutButtonMainLayer->setContentsMargins(0, 0, 0, 0);

    layoutButtonMainLayer->setSpacing(0);
    /*mpButtonMainLayer->setMinimumHeight(31);
           mpButtonMainLayer->setMaximumHeight(31);*/
    auto buttonLayer = new QWidget;
    buttonLayer->setObjectName(qsl("buttonLayer"));
    auto layoutButtonLayer = new QGridLayout(buttonLayer);
    layoutButtonLayer->setObjectName(qsl("layoutButtonLayer"));
    layoutButtonLayer->setContentsMargins(0, 0, 0, 0);
    layoutButtonLayer->setSpacing(0);

    auto buttonLayerSpacer = new QWidget(buttonLayer);
    buttonLayerSpacer->setSizePolicy(sizePolicy4);
    layoutButtonMainLayer->addWidget(buttonLayerSpacer);
    layoutButtonMainLayer->addWidget(buttonLayer);

    timeStampButton = new QToolButton;
    timeStampButton->setCheckable(true);
    timeStampButton->setMinimumSize(QSize(30, 30));
    timeStampButton->setMaximumSize(QSize(30, 30));
    timeStampButton->setSizePolicy(sizePolicy5);
    timeStampButton->setFocusPolicy(Qt::NoFocus);
    timeStampButton->setToolTip(utils::richText(tr("Show Time Stamps.")));
    timeStampButton->setIcon(QIcon(qsl(":/icons/dialog-information.png")));
    connect(timeStampButton, &QAbstractButton::toggled, mUpperPane, &TTextEdit::slot_toggleTimeStamps);
    connect(timeStampButton, &QAbstractButton::toggled, mLowerPane, &TTextEdit::slot_toggleTimeStamps);

    auto replayButton = new QToolButton;
    replayButton->setCheckable(true);
    replayButton->setMinimumSize(QSize(30, 30));
    replayButton->setMaximumSize(QSize(30, 30));
    replayButton->setSizePolicy(sizePolicy5);
    replayButton->setFocusPolicy(Qt::NoFocus);
    replayButton->setToolTip(utils::richText(tr("Record a replay.")));
    replayButton->setIcon(QIcon(qsl(":/icons/media-tape.png")));
    connect(replayButton, &QAbstractButton::clicked, this, &TConsole::slot_toggleReplayRecording);

    logButton = new QToolButton;
    logButton->setMinimumSize(QSize(30, 30));
    logButton->setMaximumSize(QSize(30, 30));
    logButton->setCheckable(true);
    logButton->setSizePolicy(sizePolicy5);
    logButton->setFocusPolicy(Qt::NoFocus);
    logButton->setToolTip(utils::richText(tr("Start logging game output to log file.")));
    QIcon logIcon;
    logIcon.addPixmap(QPixmap(qsl(":/icons/folder-downloads.png")), QIcon::Normal, QIcon::Off);
    logIcon.addPixmap(QPixmap(qsl(":/icons/folder-downloads-red-cross.png")), QIcon::Normal, QIcon::On);
    logButton->setIcon(logIcon);
    connect(logButton, &QAbstractButton::clicked, this, &TConsole::slot_toggleLogging);

    if (mType == MainConsole) {
        mpLineEdit_networkLatency = new QLineEdit(this);
        mpLineEdit_networkLatency->setReadOnly(true);
        mpLineEdit_networkLatency->setSizePolicy(sizePolicy4);
        mpLineEdit_networkLatency->setFocusPolicy(Qt::NoFocus);
        mpLineEdit_networkLatency->setToolTip(utils::richText(tr("<i>N:</i> is the latency of the game server and network (aka ping, in seconds),<br>"
                                                                 "<i>S:</i> is the system processing time - how long your triggers took to process the last line(s).")));
        mpLineEdit_networkLatency->setMaximumSize(120, 30);
        mpLineEdit_networkLatency->setMinimumSize(120, 30);
        mpLineEdit_networkLatency->setAutoFillBackground(true);
        mpLineEdit_networkLatency->setContentsMargins(0, 0, 0, 0);
        mpLineEdit_networkLatency->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

        int latencyFontPointSize = 21;
        QFont latencyFont = QFont(qsl("Bitstream Vera Sans Mono"), latencyFontPointSize, QFont::Normal);
        const int latencyFontSizeMargin = 10;
        /*:
        The first argument 'N' represents the 'N'etwork latency; the second 'S' the
        'S'ystem (processing) time
        */
        const QString dummyTextA = tr("N:%1 S:%2")
                                     .arg(0.0, 0, 'f', 3)
                                     .arg(0.0, 0, 'f', 3);
        /*:
        The argument 'S' represents the 'S'ystem (processing) time, in this situation
        the Game Server is not sending "GoAhead" signals so we cannot deduce the
        network latency...
        */
        const QString dummyTextB = tr("<no GA> S:%1")
                                     .arg(0.0, 0, 'f', 3);
        do {
            latencyFont.setPointSize(--latencyFontPointSize);
        } while (latencyFontPointSize > 6
                 && qMax(QFontMetrics(latencyFont).boundingRect(dummyTextA).width(),
                         QFontMetrics(latencyFont).boundingRect(dummyTextB).width()) + latencyFontSizeMargin
                            > mpLineEdit_networkLatency->maximumWidth());

        mpLineEdit_networkLatency->setFont(latencyFont);
        mpLineEdit_networkLatency->setFrame(false);
    }

    emergencyStop->setMinimumSize(QSize(30, 30));
    emergencyStop->setMaximumSize(QSize(30, 30));
    emergencyStop->setIcon(QIcon(qsl(":/icons/edit-bomb.png")));
    emergencyStop->setSizePolicy(sizePolicy4);
    emergencyStop->setFocusPolicy(Qt::NoFocus);
    emergencyStop->setCheckable(true);
    emergencyStop->setToolTip(utils::richText(tr("Emergency Stop. Stops all timers and triggers.")));
    connect(emergencyStop, &QAbstractButton::clicked, this, &TConsole::slot_stopAllItems);

    mpBufferSearchBox->setClearButtonEnabled(true);
    for (auto child : mpBufferSearchBox->children()) {
        auto *pAction_clear(qobject_cast<QAction *>(child));
        if (pAction_clear && pAction_clear->objectName() == QLatin1String("_q_qlineeditclearaction")) {
            connect(pAction_clear, &QAction::triggered, this, &TConsole::slot_clearSearchResults, Qt::QueuedConnection);
            break;
        }
    }

    mpBufferSearchBox->setMinimumSize(QSize(150, 30));
    mpBufferSearchBox->setMaximumSize(QSize(250, 30));
    mpBufferSearchBox->setSizePolicy(sizePolicy5);
    mpBufferSearchBox->setFont(mpHost->mCommandLineFont);
    mpBufferSearchBox->setFocusPolicy(Qt::ClickFocus);
    mpBufferSearchBox->setPlaceholderText("Search ...");
    QPalette commandLinePalette;
    commandLinePalette.setColor(QPalette::Text, mpHost->mCommandLineFgColor);
    commandLinePalette.setColor(QPalette::Highlight, QColor(0, 0, 192));
    commandLinePalette.setColor(QPalette::HighlightedText, QColor(Qt::white));
    commandLinePalette.setColor(QPalette::Base, mpHost->mCommandLineBgColor);
    commandLinePalette.setColor(QPalette::Window, mpHost->mCommandLineBgColor);
    mpBufferSearchBox->setToolTip(utils::richText(tr("Search buffer.")));
    connect(mpBufferSearchBox, &QLineEdit::returnPressed, this, &TConsole::slot_searchBufferUp);

    mpAction_searchOptions = new QAction(tr("Search Options"), this);
    mpAction_searchOptions->setObjectName(qsl("mpAction_searchOptions"));

    QMenu* pMenu_searchOptions = new QMenu(tr("Search Options"), this);
    pMenu_searchOptions->setObjectName(qsl("pMenu_searchOptions"));
    pMenu_searchOptions->setToolTipsVisible(true);

    mpAction_searchCaseSensitive = new QAction(tr("Case sensitive"), this);
    mpAction_searchCaseSensitive->setObjectName(qsl("mpAction_searchCaseSensitive"));
    mpAction_searchCaseSensitive->setToolTip(utils::richText(tr("Match case precisely")));
    mpAction_searchCaseSensitive->setCheckable(true);
    pMenu_searchOptions->insertAction(nullptr, mpAction_searchCaseSensitive);

    setSearchOptions(mSearchOptions);

    connect(mpAction_searchCaseSensitive, &QAction::triggered, this, &TConsole::slot_toggleSearchCaseSensitivity);
    mpAction_searchOptions->setMenu(pMenu_searchOptions);
    mpBufferSearchBox->addAction(mpAction_searchOptions, QLineEdit::LeadingPosition);

    mpBufferSearchUp->setMinimumSize(QSize(30, 30));
    mpBufferSearchUp->setMaximumSize(QSize(30, 30));
    mpBufferSearchUp->setSizePolicy(sizePolicy5);
    mpBufferSearchUp->setToolTip(utils::richText(tr("Earlier search result.")));
    mpBufferSearchUp->setFocusPolicy(Qt::NoFocus);
    mpBufferSearchUp->setIcon(QIcon(qsl(":/icons/export.png")));
    connect(mpBufferSearchUp, &QAbstractButton::clicked, this, &TConsole::slot_searchBufferUp);


    mpBufferSearchDown->setMinimumSize(QSize(30, 30));
    mpBufferSearchDown->setMaximumSize(QSize(30, 30));
    mpBufferSearchDown->setSizePolicy(sizePolicy5);
    mpBufferSearchDown->setFocusPolicy(Qt::NoFocus);
    mpBufferSearchDown->setToolTip(utils::richText(tr("Later search result.")));
    mpBufferSearchDown->setIcon(QIcon(qsl(":/icons/import.png")));
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
    if (mType == MainConsole) {
        // In fact a whole lot more could be inside this "if"!
        layoutButtonLayer->addWidget(mpLineEdit_networkLatency, 0, 11);
    }
    layoutLayer2->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(layer);
    layerCommandLine->setAutoFillBackground(true);

    centralLayout->addWidget(layerCommandLine);

    QList<int> sizeList;
    sizeList << 6 << 2;
    splitter->setSizes(sizeList);

    mUpperPane->show();
    mLowerPane->hide();

    connect(mpScrollBar, &QAbstractSlider::valueChanged, mUpperPane, &TTextEdit::slot_scrollBarMoved);
    connect(mpHScrollBar, &QAbstractSlider::valueChanged, mUpperPane, &TTextEdit::slot_hScrollBarMoved);

    mpHScrollBar->hide();

    //enable horizontal scrollbar in ErrorConsole
    if (mType == ErrorConsole) {
        mHScrollBarEnabled = true;
    }

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
    mpBaseVFrame->layout()->setSpacing(0);
    mpBaseHFrame->layout()->setSpacing(0);


    buttonLayerSpacer->setMinimumHeight(0);
    buttonLayerSpacer->setMinimumWidth(100);
    buttonLayer->setMaximumHeight(31);
    //buttonLayer->setMaximumWidth(31);
    buttonLayer->setMinimumWidth(400);
    buttonLayer->setMaximumWidth(400);
    mpButtonMainLayer->setMinimumWidth(400);
    mpButtonMainLayer->setMaximumWidth(400);

    mpButtonMainLayer->setAutoFillBackground(true);
    mpButtonMainLayer->setPalette(commandLinePalette);

    buttonLayer->setAutoFillBackground(true);
    changeColors();

    // error and debug consoles inherit font of the main console
    if (mType & (ErrorConsole | CentralDebugConsole)) {
        mDisplayFont = mpHost->getDisplayFont();
        mDisplayFontName = mDisplayFont.family();
        mDisplayFontSize = mDisplayFont.pointSize();

        // They always use "Control Pictures" to show control characters:
        mControlCharacter = ControlCharacterMode::Picture;
        refreshView();
    } else if (mpHost) {
        connect(mpHost, &Host::signal_controlCharacterHandlingChanged, this, &TConsole::slot_changeControlCharacterHandling);
    }

    if (mType & (MainConsole | UserWindow)) {
        setAcceptDrops(true);
        setMouseTracking(true);
    }


    if (mType & MainConsole) {
        mpButtonMainLayer->setVisible(!mpHost->getCompactInputLine());
    }

    if (mType & MainConsole) {
        mpCommandLine->adjustHeight();
    }

    connect(mudlet::self(), &mudlet::signal_adjustAccessibleNames, this, &TConsole::slot_adjustAccessibleNames);
    slot_adjustAccessibleNames();
    // Need to delay doing this because it uses elements that may not have
    // been constructed yet:
    if (mType == MainConsole) {
        QTimer::singleShot(0, this, [this]() { setProxyForFocus(mpCommandLine); });
    }
}

TConsole::~TConsole()
{
    if (mType & ~CentralDebugConsole) {
        // Codepoint issues reporting is not enabled for the CDC:
        mUpperPane->reportCodepointErrors();
    }
}

Host* TConsole::getHost()
{
    return mpHost;
}

void TConsole::resizeConsole()
{
    QSize const size = QSize(width(), height());
    QResizeEvent event(size, size);
    QApplication::sendEvent(this, &event);
}


void TConsole::raiseMudletSysWindowResizeEvent(const int overallWidth, const int overallHeight)
{
    if (mpHost.isNull()) {
        return;
    }
    TEvent mudletEvent {};
    mudletEvent.mArgumentList.append(QLatin1String("sysWindowResizeEvent"));
    mudletEvent.mArgumentList.append(QString::number(overallWidth - mBorders.left() - mBorders.right()));
    mudletEvent.mArgumentList.append(QString::number(overallHeight - mBorders.top() - mBorders.bottom() - mpCommandLine->height()));
    mudletEvent.mArgumentList.append(mConsoleName);
    mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    mpHost->raiseEvent(mudletEvent);
}

void TConsole::resizeEvent(QResizeEvent* event)
{
    if (mType & MainConsole) {
        mBorders = mpHost->borders();
    }
    int x = event->size().width();
    int y = event->size().height();

    if (mType == MainConsole && !x) {
        // When multi-view is NOT active but more than one profile is loaded
        // switching between tabs causes the deselected profile to resize its
        // main console to a width of zero - but that is not useful from a NAWS
        // or event handling system point of view - so abort doing anything
        // with the event:
        return;
    }

    if (mType & (MainConsole|SubConsole|UserWindow) && mpCommandLine && !mpCommandLine->isHidden()) {
        mpMainFrame->resize(x, y);
        mpBaseVFrame->resize(x, y);
        mpBaseHFrame->resize(x, y);
        x -= (mpLeftToolBar->width() + mpRightToolBar->width());
        y -= mpTopToolBar->height();
        // The mBorders components will be all zeros for all but the MainConsole:
        mpMainDisplay->resize(x - mBorders.left() - mBorders.right(),
                              y - mBorders.top() - mBorders.bottom() - mpCommandLine->height());
    } else {
        mpMainFrame->resize(x, y);
        mpMainDisplay->resize(x, y);
    }
    mpMainDisplay->move(mBorders.left(), mBorders.top());

    if (mType & (CentralDebugConsole|ErrorConsole)) {
        layerCommandLine->hide();
    } else if (mType & ~(SubConsole|UserWindow)) {
        // does nothing for SubConsole or UserWindows
        layerCommandLine->move(0, mpBaseVFrame->height() - layerCommandLine->height());
    }

    QWidget::resizeEvent(event);

    if (mType & MainConsole) {
        // don't call event in lua if size didn't change
        const bool preventLuaEvent = (getMainWindowSize() == mOldSize);
        mOldSize = getMainWindowSize();
        if (preventLuaEvent) {
            return;
        }
        if (!mpHost.isNull()) {
            TLuaInterpreter* pLua = mpHost->getLuaInterpreter();
            const QString func = "handleWindowResizeEvent";
            const QString n = "WindowResizeEvent";
            pLua->call(func, n);

            raiseMudletSysWindowResizeEvent(x, y);
        }
    }
//create the sysUserWindowResize Event for automatic resizing with Geyser
    if (mType & (UserWindow) && !mpHost.isNull()) {
        TLuaInterpreter* pLua = mpHost->getLuaInterpreter();
        const QString func = "handleWindowResizeEvent";
        const QString n = "WindowResizeEvent";
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
    if (mType == MainConsole) {
        mBorders = mpHost->borders();
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

    mpMainDisplay->resize(x - mBorders.left() - mBorders.right(), y - mBorders.top() - mBorders.bottom() - mpCommandLine->height());

    if (mType & MainConsole) {
        mpCommandLine->adjustHeight();
    }

    mpMainDisplay->move(mBorders.left(), mBorders.top());
    x = width();
    y = height();
    QSize const s = QSize(x, y);
    QResizeEvent event(s, s);
    QApplication::sendEvent(this, &event);
}

void TConsole::clearSelection() const
{
    mLowerPane->unHighlight();
    mUpperPane->unHighlight();
    mLowerPane->mSelectedRegion = QRegion(0, 0, 0, 0);
    mUpperPane->mSelectedRegion = QRegion(0, 0, 0, 0);
    mUpperPane->forceUpdate();
    mLowerPane->forceUpdate();
}


void TConsole::closeEvent(QCloseEvent* event)
{
    if (mType == CentralDebugConsole) {
        if (mudlet::self()->isGoingDown() || mpHost->isClosingDown()) {
            event->accept();
            return;
        } else {
            hide();
            mudlet::smpDebugArea->setVisible(false);
            mudlet::smDebugMode = false;
            mudlet::self()->refreshTabBar();
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
    conCloseEvent.mArgumentList.append(qsl("sysExitEvent"));
    conCloseEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    mpHost->raiseEvent(conCloseEvent);

    if (mpHost->mFORCE_SAVE_ON_EXIT) {
        mudlet::self()->saveWindowLayout();
        mpHost->modulesToWrite.clear();
        mpHost->saveProfile();

        if (mpHost->mpMap && mpHost->mpMap->mpRoomDB) {
            // There is a map loaded - but it *could* have no rooms at all!
            const QDir dir_map;
            const QString directory_map = mudlet::getMudletPath(mudlet::profileMapsPath, mProfileName);
            const QString filename_map = mudlet::getMudletPath(mudlet::profileDateTimeStampedMapPathFileName, mProfileName, QDateTime::currentDateTime().toString("yyyy-MM-dd#HH-mm-ss"));
            if (!dir_map.exists(directory_map)) {
                dir_map.mkpath(directory_map);
            }
            QSaveFile file(filename_map);
            if (file.open(QIODevice::WriteOnly)) {
                QDataStream out(&file);
                if (mudlet::scmRunTimeQtVersion >= QVersionNumber(5, 13, 0)) {
                    out.setVersion(mudlet::scmQDataStreamFormat_5_12);
                }
                // FIXME: https://github.com/Mudlet/Mudlet/issues/6316 - unchecked return value - we are not handling a failure to save the map!
                mpHost->mpMap->serialize(out);
                if (!file.commit()) {
                    qDebug() << "TConsole::closeEvent: error saving map: " << file.errorString();
                }
            }
        }
        mpHost->waitForProfileSave();
        event->accept();
        return;
    }

    if (!mUserAgreedToCloseConsole) {
    ASK:
        const int choice = QMessageBox::question(this, tr("Save profile?"), tr("Do you want to save the profile %1?").arg(mProfileName), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        if (choice == QMessageBox::Cancel) {
            event->setAccepted(false);
            event->ignore();
            return;
        }
        if (choice == QMessageBox::Yes) {
            mudlet::self()->saveWindowLayout();

            mpHost->modulesToWrite.clear();
            auto [ok, filename, error] = mpHost->saveProfile();

            if (!ok) {
                QMessageBox::critical(this, tr("Couldn't save profile"), tr("Sorry, couldn't save your profile - got the following error: %1").arg(error));
                goto ASK;
            } else if (mpHost->mpMap && mpHost->mpMap->mpRoomDB) {
                // There is a map loaded - but it *could* have no rooms at all!
                const QDir dir_map;
                const QString directory_map = mudlet::getMudletPath(mudlet::profileMapsPath, mProfileName);
                const QString filename_map = mudlet::getMudletPath(mudlet::profileDateTimeStampedMapPathFileName, mProfileName, QDateTime::currentDateTime().toString(qsl("yyyy-MM-dd#HH-mm-ss")));
                if (!dir_map.exists(directory_map)) {
                    dir_map.mkpath(directory_map);
                }
                QSaveFile file(filename_map);
                if (file.open(QIODevice::WriteOnly)) {
                    QDataStream out(&file);
                    if (mudlet::scmRunTimeQtVersion >= QVersionNumber(5, 13, 0)) {
                        out.setVersion(mudlet::scmQDataStreamFormat_5_12);
                    }
                    // FIXME: https://github.com/Mudlet/Mudlet/issues/6316 - unchecked return value - we are not handling a failure to save the map!
                    mpHost->mpMap->serialize(out);
                    if (!file.commit()) {
                        qDebug() << "TConsole::closeEvent: error saving map: " << file.errorString();
                    }
                }
            }
            mpHost->waitForProfileSave();
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
    mpHost->mpConsole->toggleLogging(true);
}

// FIXME: This needs to move to the TMainConsole class but the button handling
// code is currently defined but not used for all TConsole instances - and some
// of them might be useful to have on the other ones...
void TConsole::slot_toggleReplayRecording()
{
    if (mType & CentralDebugConsole) {
        return;
    }
    mRecordReplay = !mRecordReplay;
    if (mRecordReplay) {
        const QString directoryLogFile = mudlet::getMudletPath(mudlet::profileReplayAndLogFilesPath, mProfileName);
        const QString mLogFileName = qsl("%1/%2.dat").arg(directoryLogFile, QDateTime::currentDateTime().toString(qsl("yyyy-MM-dd#HH-mm-ss")));
        const QDir dirLogFile;
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
        printSystemMessage(tr("Replay recording has started. File: %1").arg(mReplayFile.fileName()) % QChar::LineFeed);
    } else {
        if (!mReplayFile.commit()) {
            qDebug() << "TConsole::slot_toggleReplayRecording: error saving replay: " << mReplayFile.errorString();
            printSystemMessage(tr("Replay recording has been stopped, but couldn't be saved.") % QChar::LineFeed);
        } else {
            printSystemMessage(tr("Replay recording has been stopped. File: %1").arg(mReplayFile.fileName()) % QChar::LineFeed);
        }
    }
}

QString getColorCode(QColor color)
{
    return qsl("%1,%2,%3,%4").arg(color.red()).arg(color.green()).arg(color.blue()).arg(color.alpha());
}

void TConsole::changeColors()
{
    mDisplayFont.setFixedPitch(true);
    if (mType == CentralDebugConsole) {
        mDisplayFont.setStyleStrategy((QFont::StyleStrategy)(QFont::NoAntialias | QFont::PreferQuality));
        mDisplayFont.setFixedPitch(true);
        mUpperPane->setFont(mDisplayFont);
        mLowerPane->setFont(mDisplayFont);
    } else if (mType & (ErrorConsole|SubConsole|UserWindow|Buffer)) {
        mDisplayFont.setStyleStrategy(QFont::StyleStrategy(QFont::NoAntialias | QFont::PreferQuality));
        mDisplayFont.setFixedPitch(true);
        mUpperPane->setFont(mDisplayFont);
        mLowerPane->setFont(mDisplayFont);
        if (!mBgImageMode) {
            auto styleSheet = qsl("QWidget#MainDisplay{background-color: rgba(%1);}").arg(getColorCode(mBgColor));
            mpMainDisplay->setStyleSheet(styleSheet);
        } else {
            setConsoleBackgroundImage(mBgImagePath, mBgImageMode);
        }
    } else if (mType == MainConsole) {
        if (mpCommandLine) {
            auto styleSheet = mpCommandLine->styleSheet();
            mpCommandLine->setStyleSheet(QString());
            // CHECK: This seems to be a, possibly iffy, attempt to combine a
            // QPalette with a style-sheet - though the Qt Documentation does
            // seem to say one should not mix QPalettes with styles/stylesheets!
            QPalette commandLinePalette;
            commandLinePalette.setColor(QPalette::Text, mpHost->mCommandLineFgColor);
            commandLinePalette.setColor(QPalette::Highlight, QColor(0, 0, 192));
            commandLinePalette.setColor(QPalette::HighlightedText, QColor(Qt::white));
            commandLinePalette.setColor(QPalette::Base, mpHost->mCommandLineBgColor);
            commandLinePalette.setColor(QPalette::Window, mpHost->mCommandLineBgColor);
            mpCommandLine->setPalette(commandLinePalette);
            mpButtonMainLayer->setPalette(commandLinePalette);
            mpCommandLine->mRegularPalette = commandLinePalette;
            mpCommandLine->setStyleSheet(styleSheet);
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
        if (!mBgImageMode) {
            auto styleSheet = qsl("QWidget#MainDisplay{background-color: rgba(%1);}").arg(getColorCode(mpHost->mBgColor));
            mpMainDisplay->setStyleSheet(styleSheet);
        } else {
            setConsoleBackgroundImage(mBgImagePath, mBgImageMode);
        }
        mBgColor = mpHost->mBgColor;
        mFgColor = mpHost->mFgColor;
        mCommandFgColor = mpHost->mCommandFgColor;
        mCommandBgColor = mpHost->mCommandBgColor;
        if (mpCommandLine) {
            mpCommandLine->setFont(mpHost->getDisplayFont());
        }
        mFormatCurrent.setColors(mpHost->mFgColor, mpHost->mBgColor);
    } else {
        Q_ASSERT_X(false, "TConsole::changeColors()", "invalid TConsole type detected");
    }

    buffer.updateColors();
    if (mType & (MainConsole|Buffer)) {
        buffer.mWrapAt = mpHost->mWrapAt;
        buffer.mWrapIndent = mpHost->mWrapIndentCount;
    }
}

void TConsole::setConsoleBgColor(int r, int g, int b, int a)
{
    mBgColor = QColor(r, g, b, a);
    mUpperPane->setConsoleBgColor(r, g, b, a);
    mLowerPane->setConsoleBgColor(r, g, b, a);
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
    if ((mType & (UserWindow|SubConsole)) && !mScrollingEnabled) {
        return;
    }

    mUpperPane->scrollDown(lines);
    if (!mUpperPane->mIsTailMode &&
        (mUpperPane->imageTopLine() + mUpperPane->getScreenHeight() >= buffer.lineBuffer.size() - mLowerPane->getRowCount())) {
        mUpperPane->scrollDown(mLowerPane->getRowCount() + 100); // Gets to the bottom
        mUpperPane->scrollDown(100);                             // needs another scroll to force mIsTailMode
    }
    if (mUpperPane->mIsTailMode) {
        mLowerPane->mCursorY = buffer.lineBuffer.size();
        mLowerPane->hide();

        mUpperPane->mCursorY = buffer.lineBuffer.size();
        mUpperPane->updateScreenView();
        mUpperPane->forceUpdate();
    }
    slot_adjustAccessibleNames();
}

void TConsole::scrollUp(int lines)
{
    if ((mType & (UserWindow|SubConsole)) && !mScrollingEnabled) {
        return;
    }

    const bool lowerAppears = mLowerPane->isHidden();
    mLowerPane->mCursorY = buffer.size();
    mLowerPane->show();
    mLowerPane->updateScreenView();
    mLowerPane->forceUpdate();

    if (lowerAppears) {
        QTimer::singleShot(0, this, [this]() {  mUpperPane->scrollUp(mLowerPane->getRowCount()); });
        if (!mpHost->mTutorialForSplitscreenScrollbackAlreadyShown) {
            const QString infoMsg = tr("[ INFO ]  - Split-screen scrollback activated. Press CTRL-ENTER to cancel.");
            mpHost->postMessage(infoMsg);
            mpHost->mTutorialForSplitscreenScrollbackAlreadyShown = true;
        }
    }
    mUpperPane->scrollUp(lines);
    slot_adjustAccessibleNames();
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
    mFormatCurrent.setColors(mFgColor, mBgColor);
    mFormatCurrent.setAllDisplayAttributes(TChar::None);
}

void TConsole::insertLink(const QString& text, QStringList& func, QStringList& hint, QPoint P, bool customFormat, QVector<int> luaReference)
{
    const int x = P.x();
    const int y = P.y();
    QPoint P2 = P;
    P2.setX(x + text.size());

    const TChar standardLinkFormat = TChar(Qt::blue, mBgColor, TChar::Underline);
    if (mTriggerEngineMode) {
        mpHost->getLuaInterpreter()->adjustCaptureGroups(x, text.size());

        if (customFormat) {
            buffer.insertInLine(P, text, mFormatCurrent);
        } else {
            buffer.insertInLine(P, text, standardLinkFormat);
        }

        buffer.applyLink(P, P2, func, hint, luaReference);

        if (y < mEngineCursor) {
            mUpperPane->needUpdate(mUserCursor.y(), mUserCursor.y() + 1);
        }
        return;

    } else {
        if ((buffer.buffer.empty() && buffer.buffer[0].empty()) || mUserCursor == buffer.getEndPos()) {
            if (customFormat) {
                buffer.addLink(mTriggerEngineMode, text, func, hint, mFormatCurrent, luaReference);
            } else {
                buffer.addLink(mTriggerEngineMode, text, func, hint, standardLinkFormat, luaReference);
            }

            mUpperPane->showNewLines();
            mLowerPane->showNewLines();

        } else {
            if (customFormat) {
                buffer.insertInLine(mUserCursor, text, mFormatCurrent);
            } else {
                buffer.insertInLine(mUserCursor, text, standardLinkFormat);
            }

            buffer.applyLink(P, P2, func, hint, luaReference);
            if (text.indexOf("\n") != -1) {
                const int y_tmp = mUserCursor.y();
                const int down = buffer.wrapLine(mUserCursor.y(), mpHost->mScreenWidth, mpHost->mWrapIndentCount, mFormatCurrent);
                mUpperPane->needUpdate(y_tmp, y_tmp + down + 1);
                const int y_neu = y_tmp + down;
                const int x_adjust = text.lastIndexOf("\n");
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
    const int x = P.x();
    const int y = P.y();
    if (mTriggerEngineMode) {
        mpHost->getLuaInterpreter()->adjustCaptureGroups(x, text.size());
        buffer.insertInLine(P, text, mFormatCurrent);
        if (y < mEngineCursor) {
            mUpperPane->needUpdate(mUserCursor.y(), mUserCursor.y() + 1);
        }

    } else {
        if ((buffer.buffer.empty() && buffer.buffer[0].empty()) || mUserCursor == buffer.getEndPos()) {
            buffer.append(text, 0, text.size(), mFormatCurrent);
            mUpperPane->showNewLines();
            mLowerPane->showNewLines();
        } else {
            buffer.insertInLine(mUserCursor, text, mFormatCurrent);
            const int y_tmp = mUserCursor.y();
            if (text.indexOf(QChar::LineFeed) != -1) {
                const int down = buffer.wrapLine(y_tmp, mpHost->mScreenWidth, mpHost->mWrapIndentCount, mFormatCurrent);
                mUpperPane->needUpdate(y_tmp, y_tmp + down + 1);
            } else {
                mUpperPane->needUpdate(y_tmp, y_tmp + 1);
            }
        }

    }
}


void TConsole::replace(const QString& text)
{
    const int x = P_begin.x();
    const int o = P_end.x() - P_begin.x();
    const int r = text.size();

    if (mTriggerEngineMode) {
        if (hasSelection()) {
            if (r < o) {
                const int a = -1 * (o - r);
                mpHost->getLuaInterpreter()->adjustCaptureGroups(x, a);
            }
            if (r > o) {
                const int a = r - o;
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
    deleteLine(mUserCursor.y());
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

void TConsole::insertLink(const QString& text, QStringList& func, QStringList& hint, bool customFormat, QVector<int> luaReference)
{
    insertLink(text, func, hint, mUserCursor, customFormat, luaReference);
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

int TConsole::getWrapAt()
{
    return buffer.mWrapAt;
}

int TConsole::getLineCount()
{
    return buffer.getLastLineNumber();
}

QStringList TConsole::getLines(int from, int to)
{
    QStringList ret;
    const int delta = abs(from - to);
    for (int i = 0; i < delta; i++) {
        ret << buffer.line(from + i);
    }
    return ret;
}

void TConsole::selectCurrentLine()
{
    selectSection(0, buffer.line(mUserCursor.y()).size());
}

std::list<int> TConsole::getFgColor()
{
    std::list<int> result;
    const int x = P_begin.x();
    const int y = P_begin.y();
    if (y < 0) {
        return result;
    }
    if (x < 0) {
        return result;
    }
    if (y >= static_cast<int>(buffer.buffer.size())) {
        return result;
    }

    auto line = buffer.buffer.at(y);
    const int len = static_cast<int>(line.size());
    if (len - 1 >= x) {
        QColor const color(line.at(x).foreground());
        result.push_back(color.red());
        result.push_back(color.green());
        result.push_back(color.blue());
    }

    return result;
}

std::list<int> TConsole::getBgColor()
{
    std::list<int> result;
    const int x = P_begin.x();
    const int y = P_begin.y();
    if (y < 0) {
        return result;
    }
    if (x < 0) {
        return result;
    }
    if (y >= static_cast<int>(buffer.buffer.size())) {
        return result;
    }

    auto line = buffer.buffer.at(y);
    const int len = static_cast<int>(line.size());
    if (len - 1 >= x) {
        QColor const color(line.at(x).background());
        result.push_back(color.red());
        result.push_back(color.green());
        result.push_back(color.blue());
    }

    return result;
}

QPair<quint8, TChar> TConsole::getTextAttributes() const
{
    const int x = P_begin.x();
    const int y = P_begin.y();
    if (y < 0 || x < 0 || y >= static_cast<int>(buffer.buffer.size()) || x >= (static_cast<int>(buffer.buffer.at(y).size()) - 1)) {
        return qMakePair(2, TChar());
    }

    return qMakePair(0, buffer.buffer.at(y).at(x));
}

void TConsole::luaWrapLine(int line)
{
    if (!mpHost) {
        return;
    }
    TChar ch(this);
    buffer.wrapLine(line, mWrapAt, mIndentCount, ch);
}

bool TConsole::setFontSize(int size)
{
    mDisplayFontSize = size;

    refreshView();
    return true;
}

bool TConsole::setConsoleBackgroundImage(const QString& imgPath, int mode)
{
    QColor bgColor;
    QString styleSheet;

    if (mType == MainConsole) {
        bgColor = mpHost->mBgColor;
    } else {
        bgColor = mBgColor;
    }

    if (mode == 1) {
        styleSheet = qsl("QWidget#MainDisplay{background-color: rgba(%1); border-image: url(%2);}").arg(getColorCode(bgColor)).arg(imgPath);
    } else if (mode == 2) {
        styleSheet = qsl("QWidget#MainDisplay{background-color: rgba(%1); background-image: url(%2); background-repeat: no-repeat; background-position: center; background-origin: margin;}")
                             .arg(getColorCode(bgColor))
                             .arg(imgPath);
    } else if (mode == 3) {
        styleSheet = qsl("QWidget#MainDisplay{background-color: rgba(%1); background-image: url(%2);}").arg(getColorCode(bgColor)).arg(imgPath);
    } else if (mode == 4) {
        styleSheet = qsl("QWidget#MainDisplay{background-color: rgba(%1); %2}").arg(getColorCode(bgColor)).arg(imgPath);
    } else {
        return false;
    }
    mpMainDisplay->setStyleSheet(styleSheet);
    mBgImageMode = mode;
    mBgImagePath = imgPath;
    return true;
}

bool TConsole::resetConsoleBackgroundImage()
{
    mBgImageMode = 0;
    changeColors();
    return true;
}

void TConsole::setCmdVisible(bool isVisible)
{
    QSizePolicy const sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // create MiniConsole commandline if it's not existing
    if (!mpCommandLine) {
        if (!isVisible) {
            // If we don't have one and we are being told to hide it then
            // really there is nothing to do - so lets do nothing:
            return;
        }
        mpCommandLine = new TCommandLine(mpHost, mConsoleName, TCommandLine::ConsoleCommandLine, this, mpMainDisplay);
        mpCommandLine->setContentsMargins(0, 0, 0, 0);
        mpCommandLine->setSizePolicy(sizePolicy);
        mpCommandLine->setFocusPolicy(Qt::StrongFocus);
        // put this CommandLine in the mainConsoles SubCommandLineMap
        // name is the console name
        mpHost->mpConsole->mSubCommandLineMap[mConsoleName] = mpCommandLine;
        layoutLayer2->addWidget(mpCommandLine);
    }
    if (mType == MainConsole) {
        if (mpHost) {
            if (!mpHost->getCompactInputLine() && isVisible) {
                mpButtonMainLayer->setVisible(true);
            } else {
                mpButtonMainLayer->setVisible(false);
            }
        }

    } else {
        mpButtonMainLayer->setVisible(false);
    }
    layerCommandLine->setVisible(isVisible);
    mpCommandLine->setVisible(isVisible);
    //resizes miniconsole if command line gets enabled/disabled
    resizeConsole();
    setProxyForFocus(isVisible ? mpCommandLine : nullptr);
    // Need to remove the TCommandLine from the last used stack
    // if it has been explicitly hidden:
    if (!isVisible && mpHost) {
        mpHost->forgetCommandLine(mpCommandLine);
    }
}

void TConsole::refreshView() const
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

bool TConsole::setFont(const QString& font)
{
    mDisplayFontName = font;

    refreshView();
    return true;
}

QString TConsole::getCurrentLine()
{
    return buffer.line(mUserCursor.y());
}

int TConsole::getLastLineNumber()
{
    return buffer.getLastLineNumber();
}

void TConsole::moveCursorEnd()
{
    const int y = buffer.getLastLineNumber();
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

    if (mudlet::smDebugMode) {
        TDebug(Qt::darkMagenta, Qt::black) << "line under current user cursor: " >> mpHost;
        TDebug(Qt::red, Qt::black) << TDebug::csmContinue << mUserCursor.y() << "#:" >> mpHost;
        TDebug(Qt::gray, Qt::black) << TDebug::csmContinue << buffer.line(mUserCursor.y()) << "\n" >>  mpHost;
    }

    int begin = -1;
    for (int i = 0; i < numOfMatch; i++) {
        const QString li = buffer.line(mUserCursor.y());
        if (li.isEmpty()) {
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

    const int end = begin + text.size();
    P_begin.setX(begin);
    P_begin.setY(mUserCursor.y());
    P_end.setX(end);
    P_end.setY(mUserCursor.y());

    if (mudlet::smDebugMode) {
        TDebug(Qt::darkRed, Qt::black) << "P_begin(" << P_begin.x() << "/" << P_begin.y() << "), P_end(" << P_end.x() << "/" << P_end.y()
                                                       << ") selectedText = " << buffer.line(mUserCursor.y()).mid(P_begin.x(), P_end.x() - P_begin.x()) << "\n"
                >> mpHost;
    }
    return begin;
}

bool TConsole::selectSection(int from, int to)
{
    if (mudlet::smDebugMode) {
        TDebug(Qt::darkMagenta, Qt::black) << "selectSection(" << from << "," << to << "): line under current user cursor: " << buffer.line(mUserCursor.y()) << "\n" >> mpHost;
    }
    if (from < 0) {
        return false;
    }
    if (mUserCursor.y() >= static_cast<int>(buffer.buffer.size())) {
        return false;
    }
    const int s = buffer.buffer[mUserCursor.y()].size();
    if (from > s || from + to > s) {
        return false;
    }
    P_begin.setX(from);
    P_begin.setY(mUserCursor.y());
    P_end.setX(from + to);
    P_end.setY(mUserCursor.y());

    if (mudlet::smDebugMode) {
        TDebug(Qt::darkMagenta, Qt::black) << "P_begin(" << P_begin.x() << "/" << P_begin.y() << "), P_end(" << P_end.x() << "/" << P_end.y() << ") selectedText:\n\""
                                           << buffer.line(mUserCursor.y()).mid(P_begin.x(), P_end.x() - P_begin.x()) << "\"\n"
                >> mpHost;
    }
    return true;
}

// returns whenever the selection is valid, the selection text,
// start position, and the length of the selection
std::tuple<bool, QString, int, int> TConsole::getSelection()
{
    if (mUserCursor.y() >= static_cast<int>(buffer.buffer.size())) {
        return {false, qsl("the selection is no longer valid"), 0, 0};
    }

    const auto start = P_begin.x();
    const auto length = P_end.x() - P_begin.x();
    const auto line = buffer.line(mUserCursor.y());
    if (line.size() < start) {
        return {false, qsl("the selection is no longer valid"), 0, 0};
    }

    const auto text = line.mid(start, length);
    return {true, text, start, length};
}

void TConsole::setLink(const QStringList& linkFunction, const QStringList& linkHint, const QVector<int> linkReference)
{
    buffer.applyLink(P_begin, P_end, linkFunction, linkHint, linkReference);
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
}

void TConsole::setBgColor(int r, int g, int b, int a)
{
    setBgColor(QColor(r, g, b, a));
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

void TConsole::setCommandBgColor(int r, int g, int b, int a)
{
    setCommandBgColor(QColor(r, g, b, a));
}

void TConsole::setCommandBgColor(const QColor& newColor)
{
    mCommandBgColor = newColor;
}

void TConsole::setCommandFgColor(int r, int g, int b, int a)
{
    setCommandFgColor(QColor(r, g, b, a));
}

void TConsole::setCommandFgColor(const QColor& newColor)
{
    mCommandFgColor = newColor;
}

void TConsole::setScrollBarVisible(bool isVisible)
{
    if (mpScrollBar) {
        mpScrollBar->setVisible(isVisible);
    }
}

void TConsole::setHorizontalScrollBar(bool isEnabled)
{
    if (mpHScrollBar) {
        mHScrollBarEnabled = isEnabled;
        mpHScrollBar->setVisible(isEnabled);
    }
}

void TConsole::setScrolling(const bool state)
{
    if (mType & (UserWindow | SubConsole)) {
        mScrollingEnabled = state;
        if (!mScrollingEnabled) {
            clearSplit();
        }
    }
}

void TConsole::printCommand(QString& msg)
{
    if (mTriggerEngineMode) {
        msg.append(QChar::LineFeed);
        const int lineBeforeNewContent = buffer.getLastLineNumber();
        if (lineBeforeNewContent >= 0) {
            if (buffer.lineBuffer.at(lineBeforeNewContent).right(1) != QChar(QChar::LineFeed)) {
                msg.prepend(QChar::LineFeed);
            }
        }
        buffer.appendLine(msg, 0, msg.size() - 1, mCommandFgColor, mCommandBgColor);
    } else {
        const int lineBeforeNewContent = buffer.size() - 2;
        if (lineBeforeNewContent >= 0) {
            int promptEnd = buffer.buffer.at(lineBeforeNewContent).size();
            if (promptEnd < 0) {
                promptEnd = 0;
            }
            if (buffer.promptBuffer[lineBeforeNewContent]) {
                QPoint P(promptEnd, lineBeforeNewContent);
                const TChar format(mCommandFgColor, mCommandBgColor);
                buffer.insertInLine(P, msg, format);
                const int down = buffer.wrapLine(lineBeforeNewContent, mpHost->mScreenWidth, mpHost->mWrapIndentCount, mFormatCurrent);

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

void TConsole::echoLink(const QString& text, QStringList& func, QStringList& hint, bool customFormat, QVector<int> luaReference)
{
    if (customFormat) {
        buffer.addLink(mTriggerEngineMode, text, func, hint, mFormatCurrent, luaReference);
    } else {
        const TChar f = TChar(Qt::blue, (mType == MainConsole ? mpHost->mBgColor : mBgColor), TChar::Underline);
        buffer.addLink(mTriggerEngineMode, text, func, hint, f, luaReference);
    }
    mUpperPane->showNewLines();
    mLowerPane->showNewLines();
}

// An overload of print(const QString& msg):
void TConsole::print(const char* txt)
{
    const QString msg(txt);
    print(msg);
}

// echoUserWindow(const QString& msg) was a redundant wrapper around this method:
void TConsole::print(const QString& msg)
{
    buffer.append(msg, 0, msg.size(), mFormatCurrent.foreground(), mFormatCurrent.background(), mFormatCurrent.allDisplayAttributes());
    mUpperPane->showNewLines();
    mLowerPane->showNewLines();

    if (Q_UNLIKELY(mudlet::self()->smMirrorToStdOut)) {
        qDebug().nospace().noquote() << qsl("%1| %2").arg(mConsoleName, msg);
    }
}

// printDebug(QColor& c, QColor& d, const QString& msg) was functionally the
// same as this method it was just that the arguments were in a different order
void TConsole::print(const QString& msg, const QColor fgColor, const QColor bgColor)
{
    buffer.append(msg, 0, msg.size(), fgColor, bgColor);
    mUpperPane->showNewLines();
    mLowerPane->showNewLines();

    if (Q_UNLIKELY(mudlet::self()->smMirrorToStdOut)) {
        qDebug().nospace().noquote() << qsl("%1| %2").arg(mConsoleName, msg);
    }
}

void TConsole::printSystemMessage(const QString& msg)
{
    const QString txt = tr("System Message: %1").arg(msg);
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
    mpHost->mpConsole->mClipboard = buffer.cut(P_begin, P_end);
}

void TConsole::paste()
{
    if (buffer.size() - 1 > mUserCursor.y()) {
        buffer.paste(mUserCursor, mpHost->mpConsole->mClipboard);
        mUpperPane->needUpdate(mUserCursor.y(), mUserCursor.y());
    } else {
        buffer.appendBuffer(mpHost->mpConsole->mClipboard);
    }
    mUpperPane->showNewLines();
    mLowerPane->showNewLines();
}

void TConsole::pasteWindow(TBuffer bufferSlice)
{
    mpHost->mpConsole->mClipboard = bufferSlice;
    paste();
}

void TConsole::appendBuffer()
{
    buffer.appendBuffer(mpHost->mpConsole->mClipboard);
    mUpperPane->showNewLines();
    mLowerPane->showNewLines();
}

void TConsole::appendBuffer(const TBuffer& bufferSlice)
{
    buffer.appendBuffer(bufferSlice);
    mUpperPane->showNewLines();
    mLowerPane->showNewLines();
}

void TConsole::slot_stopAllItems(bool b)
{
    if (b) {
        mpHost->stopAllTriggers();
        emergencyStop->setIcon(QIcon(qsl(":/icons/red-bomb.png")));
    } else {
        mpHost->reenableAllTriggers();
        emergencyStop->setIcon(QIcon(qsl(":/icons/edit-bomb.png")));
    }
}

void TConsole::slot_searchBufferUp()
{
    // The search term entry box is one widget that does not pass a mouse press
    // event up to the main TConsole and thus does not cause the focus to shift
    // to the profile's tab when in multi-view mode - so add a call to make that
    // happen:
    mudlet::self()->activateProfile(mpHost);

    if (mSearchQuery != mpBufferSearchBox->text()) {
        mSearchQuery = mpBufferSearchBox->text();
        buffer.clearSearchHighlights();
        mCurrentSearchResult = buffer.lineBuffer.size();
    } else {
        // make sure the line to search from does not exceed the buffer, which can grow and shrink dynamically
        mCurrentSearchResult = std::min<qsizetype>(mCurrentSearchResult, buffer.lineBuffer.size());
    }
    if (buffer.lineBuffer.empty()) {
        return;
    }

    bool found = false;
    for (int searchY = mCurrentSearchResult - 1; searchY >= 0; --searchY) {
        int searchX = -1;
        do {
            searchX = buffer.lineBuffer[searchY].indexOf(mSearchQuery, searchX + 1, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive));
            if (searchX > -1) {
                buffer.applyAttribute(QPoint(searchX, searchY), QPoint(searchX + mSearchQuery.size(), searchY), TChar::Found, true);
                found = true;
            }
        } while (searchX > -1);

        if (found) {
            scrollUp(buffer.mCursorY - searchY - 3);
            mUpperPane->forceUpdate();
            mCurrentSearchResult = searchY;
            return;
        }
    }
    print(qsl("%1\n").arg(tr("No search results, sorry!")));
}

void TConsole::slot_searchBufferDown()
{
    if (mSearchQuery != mpBufferSearchBox->text()) {
        mSearchQuery = mpBufferSearchBox->text();
        buffer.clearSearchHighlights();
        mCurrentSearchResult = buffer.lineBuffer.size();
    }
    if (buffer.lineBuffer.empty()) {
        return;
    }
    if (mCurrentSearchResult >= buffer.lineBuffer.size()) {
        return;
    }

    bool found = false;
    for (int searchY = mCurrentSearchResult + 1; searchY < buffer.lineBuffer.size(); ++searchY) {
        int searchX = -1;
        do {
            searchX = buffer.lineBuffer[searchY].indexOf(mSearchQuery, searchX + 1, ((mSearchOptions & SearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive));
            if (searchX > -1) {
                buffer.applyAttribute(QPoint(searchX, searchY), QPoint(searchX + mSearchQuery.size(), searchY), TChar::Found, true);
                found = true;
            }
        } while (searchX > -1);

        if (found) {
            scrollUp(buffer.mCursorY - searchY - 3);
            mUpperPane->forceUpdate();
            mCurrentSearchResult = searchY;
            return;
        }
    }
    print(qsl("%1\n").arg(tr("No search results, sorry!")));
}

QSize TConsole::getMainWindowSize() const
{
    if (isHidden()) {
        return mOldSize;
    }
    QSize const consoleSize = size();
    const int toolbarWidth = mpLeftToolBar->width() + mpRightToolBar->width();
    const int toolbarHeight = mpTopToolBar->height();
    const int commandLineHeight = mpCommandLine->height();
    QSize mainWindowSize(consoleSize.width() - toolbarWidth, consoleSize.height() - (commandLineHeight + toolbarHeight));
    return mainWindowSize;
}

void TConsole::setProfileName(const QString& newName)
{
    mProfileName = newName;
}


void TConsole::dragEnterEvent(QDragEnterEvent* e)
{
    if (e->mimeData()->hasUrls() || e->mimeData()->hasText()) {
        e->acceptProposedAction();
    }
}

//https://amin-ahmadi.com/2016/01/04/qt-drag-drop-files-images/
void TConsole::dropEvent(QDropEvent* e)
{
    for (const auto& url : e->mimeData()->urls()) {
        const QString fname = url.toLocalFile();
        const QFileInfo info(fname);
        if (info.exists()) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            const QPoint pos = e->pos();
#else
            QPoint pos = e->position().toPoint();
#endif
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
    if (e->mimeData()->hasText()) {
        if (QUrl const url(e->mimeData()->text()); url.isValid()) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            const QPoint pos = e->pos();
#else
            QPoint pos = e->position().toPoint();
#endif
            TEvent mudletEvent{};
            mudletEvent.mArgumentList.append(QLatin1String("sysDropUrlEvent"));
            mudletEvent.mArgumentList.append(url.toString());
            mudletEvent.mArgumentList.append(url.scheme());
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

// This is also called from the TTextEdit mouse(Press|Release)Event()s:
void TConsole::raiseMudletMousePressOrReleaseEvent(QMouseEvent* event, const bool isPressEvent)
{
    if (mType & (CentralDebugConsole | ErrorConsole)) {
        return;
    }

    // Else if NOT the CentralDebugConsole or the ErrorConsole then bring the
    // focus to the current profile in the main application window:
    TEvent mudletEvent{};
    mudletEvent.mArgumentList.append(isPressEvent ? qsl("sysWindowMousePressEvent") : qsl("sysWindowMouseReleaseEvent"));
    switch (event->button()) {
    case Qt::LeftButton:    mudletEvent.mArgumentList.append(QString::number(1));   break;
    case Qt::RightButton:   mudletEvent.mArgumentList.append(QString::number(2));   break;
    case Qt::MiddleButton:  mudletEvent.mArgumentList.append(QString::number(3));   break;
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
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    const QPoint pos = event->pos();
#else
    QPoint pos = event->position().toPoint();
#endif
    mudletEvent.mArgumentList.append(QString::number(pos.x()));
    mudletEvent.mArgumentList.append(QString::number(pos.y()));
    mudletEvent.mArgumentList.append(mConsoleName);
    mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    mpHost->raiseEvent(mudletEvent);

    mpHost->setFocusOnHostActiveCommandLine();
}

void TConsole::mousePressEvent(QMouseEvent* event)
{
    raiseMudletMousePressOrReleaseEvent(event, true);
}

void TConsole::slot_adjustAccessibleNames()
{
    const bool multipleProfilesActive = (mudlet::self()->getHostManager().getHostCount() > 1);
    switch (mType) {
    case CentralDebugConsole:
        setAccessibleName(tr("Debug Console."));
        setAccessibleDescription(tr("Debug messages from all profiles are shown here."));
        if (mLowerPane->isVisible()) {
            //: accessibility-friendly name to describe the upper half of the Mudlet central debug window when you've scrolled up
            mUpperPane->setAccessibleName(tr("Central debug console past content."));
            //: accessibility-friendly name to describe the lower half of the Mudlet central debug when you've scrolled up
            mLowerPane->setAccessibleName(tr("Central debug console live content."));
        } else {
            //: accessibility-friendly name to describe the upper half of the Mudlet central debug window when it is not scrolled up
            mUpperPane->setAccessibleName(tr("Central debug console."));
            mLowerPane->setAccessibleName(QString());
        }
        return;
    case ErrorConsole:
        setAccessibleName(tr("Error Console in editor."));
        if (mLowerPane->isVisible()) {
            if (multipleProfilesActive) {
                //: accessibility-friendly name to describe the upper half of the Mudlet profile's editor error window when you've scrolled up, %1 is the name of the profile when more than one is loaded.
                mUpperPane->setAccessibleName(tr("Editor's error window for profile \"%1\", past content.").arg(mProfileName));
                //: accessibility-friendly name to describe the lower half of the Mudlet profile's editor error window when you've scrolled up, %1 is the name of the profile when more than one is loaded.
                mLowerPane->setAccessibleName(tr("Editor's error window for profile \"%1\", live content.").arg(mProfileName));
            } else {
                //: accessibility-friendly name to describe the upper half of the Mudlet profile's editor error window when you've scrolled up and only one profile is loaded.
                mUpperPane->setAccessibleName(tr("Editor's error window past content."));
                //: accessibility-friendly name to describe the lower half of the Mudlet profile's editor error window when you've scrolled up and only one profile is loaded.
                mLowerPane->setAccessibleName(tr("Editor's error window live content."));
            }
            setAccessibleDescription(tr("Error messages for the \"%1\" profile are shown here in the editor.").arg(mProfileName));
        } else {
            if (multipleProfilesActive) {
                //: accessibility-friendly name to describe the upper half of the Mudlet profile's editor error window when it is not scrolled up, %1 is the name of the profile when more than one is loaded.
                mUpperPane->setAccessibleName(tr("Editor's error window for profile \"%1\".").arg(mProfileName));
            } else {
                //: accessibility-friendly name to describe the upper half of the Mudlet profile's editor error window when it is not scrolled up and only one profile is loaded.
                mUpperPane->setAccessibleName(tr("Editor's error window"));
            }
            mLowerPane->setAccessibleName(QString());
            setAccessibleDescription(tr("Error messages are shown here in the editor."));
        }
        return;
    case MainConsole:
        setAccessibleDescription(tr("Game content is shown here. It may contain subconsoles and a mapper window."));
        if (multipleProfilesActive) {
            setAccessibleName(tr("Main Window for \"%1\" profile.").arg(mProfileName));
        } else {
            setAccessibleName(tr("Main Window."));
        }
        if (mLowerPane->isVisible()) {
            if (multipleProfilesActive) {
                //: accessibility-friendly name to describe the upper half of a Mudlet profile's main window when you've scrolled up, %1 is the name of the profile when more than one is loaded.
                mUpperPane->setAccessibleName(tr("Profile \"%1\" main window past content.").arg(mProfileName));
                //: accessibility-friendly name to describe the lower half of a Mudlet profile's main window when you've scrolled up, %1 is the name of the profile when more than one is loaded.
                mLowerPane->setAccessibleName(tr("Profile \"%1\" main window live content.").arg(mProfileName));
            } else {
                //: accessibility-friendly name to describe the upper half of a Mudlet profile's main window when you've scrolled up and only one profile is loaded.
                mUpperPane->setAccessibleName(tr("Profile main window past content."));
                //: accessibility-friendly name to describe the lower half of a Mudlet profile's main window when you've scrolled up and only one profile is loaded.
                mLowerPane->setAccessibleName(tr("Profile main window live content."));
            }
        } else {
            if (multipleProfilesActive) {
                //: accessibility-friendly name to describe the upper half of a Mudlet profile's main window when it is not scrolled up, %1 is the name of the profile when more than one is loaded.
                mUpperPane->setAccessibleName(tr("Profile \"%1\" main window.").arg(mProfileName));
            } else {
                //: accessibility-friendly name to describe the upper half of a Mudlet profile's main window when it is not scrolled up and only one profile is loaded.
                mUpperPane->setAccessibleName(tr("Profile main window."));
            }
            mLowerPane->setAccessibleName(QString());
        }
        return;
    case SubConsole:
        if (multipleProfilesActive) {
            setAccessibleName(tr("Embedded window \"%1\" for \"%2\" profile.").arg(mConsoleName, mProfileName));
        } else {
            setAccessibleName(tr("Embedded window \"%1\".").arg(mConsoleName));
        }
        setAccessibleDescription(tr("Game content or locally generated text may be sent here."));
        if (mLowerPane->isVisible()) {
            if (multipleProfilesActive) {
                //: accessibility-friendly name to describe the upper half of a Mudlet profile's sub-console window when you've scrolled up, %1 is the name of the profile when more than one is loaded and %2 is the name of the window.
                mUpperPane->setAccessibleName(tr("Profile \"%1\" embedded window \"%2\" past content.").arg(mProfileName, mConsoleName));
                //: accessibility-friendly name to describe the lower half of a Mudlet profile's sub-console window when you've scrolled up, %1 is the name of the profile when more than one is loaded and %2 is the name of the window.
                mLowerPane->setAccessibleName(tr("Profile \"%1\" embedded window \"%2\" live content.").arg(mProfileName, mConsoleName));
            } else {
                //: accessibility-friendly name to describe the upper half of a Mudlet profile's sub-console window when you've scrolled up, %1 is the name of the window.
                mUpperPane->setAccessibleName(tr("Profile embedded window \"%1\" past content.").arg(mConsoleName));
                //: accessibility-friendly name to describe the lower half of a Mudlet profile's sub-console window when you've scrolled up, %1 is the name of the window.
                mLowerPane->setAccessibleName(tr("Profile embedded window \"%1\" live content.").arg(mConsoleName));
            }
        } else {
            if (multipleProfilesActive) {
                //: accessibility-friendly name to describe the upper half of a Mudlet profile's sub-console window when it is not scrolled up, %1 is the name of the profile when more than one is loaded and %2 is the name of the window.
                mUpperPane->setAccessibleName(tr("Profile \"%1\" embedded window \"%2\".").arg(mProfileName, mConsoleName));
            } else {
                //: accessibility-friendly name to describe the upper half of a Mudlet profile's sub-console window when it is not scrolled up, %1 is the name of the window.
                mUpperPane->setAccessibleName(tr("Profile embedded window \"%1\".").arg(mConsoleName));
            }
            mLowerPane->setAccessibleName(QString());
        }
        return;
    case UserWindow:
        if (multipleProfilesActive) {
            setAccessibleName(tr("User window \"%1\" for \"%2\" profile.").arg(mConsoleName, mProfileName));
        } else {
            setAccessibleName(tr("User window \"%1\".").arg(mConsoleName));
        }
        setAccessibleDescription(tr("Game content or locally generated text may be sent to this window that may be floated away from the Mudlet application or docked within the main application window."));
        if (mLowerPane->isVisible()) {
            if (multipleProfilesActive) {
                //: accessibility-friendly name to describe the upper half of a Mudlet profile's floating/dockable user window when you've scrolled up, %1 is the name of the profile when more than one is loaded and %2 is the name of the window.
                mUpperPane->setAccessibleName(tr("Profile \"%1\" user window \"%2\" past content.").arg(mProfileName, mConsoleName));
                //: accessibility-friendly name to describe the lower half of a Mudlet profile's floating/dockable user window window when you've scrolled up, %1 is the name of the profile when more than one is loaded and %2 is the name of the window.
                mLowerPane->setAccessibleName(tr("Profile \"%1\" user window \"%2\" live content.").arg(mProfileName, mConsoleName));
            } else {
                //: accessibility-friendly name to describe the upper half of a Mudlet profile's sub-console window when you've scrolled up, %1 is the name of the window.
                mUpperPane->setAccessibleName(tr("Profile user window \"%1\" past content.").arg(mConsoleName));
                //: accessibility-friendly name to describe the lower half of a Mudlet profile's sub-console window when you've scrolled up, %1 is the name of the window.
                mLowerPane->setAccessibleName(tr("Profile user window \"%1\" live content.").arg(mConsoleName));
            }
        } else {
            if (multipleProfilesActive) {
                //: accessibility-friendly name to describe the upper half of a Mudlet profile's floating/dockable user window window when it is not scrolled up, %1 is the name of the profile when more than one is loaded and %2 is the name of the window.
                mUpperPane->setAccessibleName(tr("Profile \"%1\" user window \"%2\".").arg(mProfileName, mConsoleName));
            } else {
                //: accessibility-friendly name to describe the upper half of a Mudlet profile's floating/dockable user window window when it is not scrolled up, %1 is the name of the window.
                mUpperPane->setAccessibleName(tr("Profile user window \"%1\".").arg(mConsoleName));
            }
            mLowerPane->setAccessibleName(QString());
        }
        return;
    case Buffer:
        // This is not a visible thing so is not accessible to screen readers
        return;
    case UnknownType:
        // Should never be used -  and since we have now handled ALL enum values
        // we do not need a "default:" entry
        Q_UNREACHABLE();
    }
}

void TConsole::mouseReleaseEvent(QMouseEvent* event)
{
    raiseMudletMousePressOrReleaseEvent(event, false);
}

void TConsole::slot_changeControlCharacterHandling(const ControlCharacterMode mode)
{
    if (mControlCharacter != mode) {
        mControlCharacter = mode;
        refreshView();
    }
}

void TConsole::setProxyForFocus(TCommandLine* pCommandLine)
{
    if (mType == MainConsole) {
        mUpperPane->setFocusProxy(pCommandLine);
        QAccessibleEvent event(pCommandLine, QAccessible::Focus);
        QAccessible::updateAccessibility(&event);
    } else if (mType == UserWindow) {
        if (pCommandLine && pCommandLine->isVisible()) {
            mUpperPane->setFocusProxy(pCommandLine);
            QAccessibleEvent event(pCommandLine, QAccessible::Focus);
            QAccessible::updateAccessibility(&event);
        } else {
            mUpperPane->setFocusProxy(mpHost->mpConsole->mpCommandLine);
            QAccessibleEvent event(mpHost->mpConsole->mpCommandLine, QAccessible::Focus);
            QAccessible::updateAccessibility(&event);
        }
    } else if (mType == SubConsole) {
        if (pCommandLine && pCommandLine->isVisible()) {
            mUpperPane->setFocusProxy(pCommandLine);
            QAccessibleEvent event(pCommandLine, QAccessible::Focus);
            QAccessible::updateAccessibility(&event);
        } else {
            // Need to search ancestors to find the TConsole that this one
            // is inserted into - and if it has a TCommandLine
            auto parentConsole = mpHost->parentTConsole(this);
            if (!parentConsole.isNull() && parentConsole->mpCommandLine && parentConsole->mpCommandLine->isVisible()) {
                // TBH We ought to also check for any added TCommandLine but
                // that can wait for a future development...
                mUpperPane->setFocusProxy(parentConsole->mpCommandLine);
                QAccessibleEvent event(parentConsole->mpCommandLine, QAccessible::Focus);
                QAccessible::updateAccessibility(&event);
            } else {
                // Somehow that has failed so fall back to the main console
                mUpperPane->setFocusProxy(mpHost->mpConsole->mpCommandLine);
                QAccessibleEvent event(mpHost->mpConsole->mpCommandLine, QAccessible::Focus);
                QAccessible::updateAccessibility(&event);
            }
        }
    }
}

// At present this only supports/works on the main console
void TConsole::setCaretMode(bool enabled)
{
    mUpperPane->updateCaret();
    mLowerPane->updateCaret();

    if (enabled) {
        mUpperPane->initializeCaret();
        // Remove the focusProxy before setting the focusPolicy otherwise
        // the Policy gets sent to the Proxy!
        mUpperPane->setFocusProxy(nullptr);
        // This adds TabFocus to the otherwise used ClickFocus:
        mUpperPane->setFocusPolicy(Qt::StrongFocus);
#if defined(Q_OS_WIN32) || defined(Q_OS_LINUX)
        // windows & linux don't move keyboard focus to the main window without this
        mUpperPane->setFocus(Qt::MouseFocusReason);
        mUpperPane->grabKeyboard();

        QAccessibleEvent event(mUpperPane, QAccessible::Focus);
        QAccessible::updateAccessibility(&event);
#endif
        // The overload without an argument uses Qt::OtherFocusReason according
        // to the Qt source code:
        mUpperPane->setFocus();
    } else {
#if defined(Q_OS_WIN32) || defined(Q_OS_LINUX)
        // NVDA breaks focus reset, so do it on a timer
        QTimer::singleShot(0, this, [this] () {
            mUpperPane->releaseKeyboard();
        });
#endif
        Q_ASSERT_X(!mUpperPane->focusProxy(), "TConsole:setCaretMode(false) FAIL", "About to set a focusPolicy but there is a focusProxy in place that will get it instead!");
        mUpperPane->setFocusPolicy(Qt::ClickFocus);
        setProxyForFocus(mpCommandLine);
        // Carefull - if there is a FocusProxy for this element then IT gets the policy
    }
}

void TConsole::createSearchOptionIcon()
{
    // When we add new search options we must create icons for each combination
    // beforehand - which is simpler than having to do code to combine the
    // QPixMaps...
    QIcon newIcon;
    switch (mSearchOptions) {
    // Each combination must be handled here
    case SearchOptionCaseSensitive:
        newIcon.addPixmap(QPixmap(":/icons/searchOptions-caseSensitive.png"));
        break;

    case SearchOptionNone:
        // Use the grey icon as that is appropriate for the "No options set" case
        newIcon.addPixmap(QPixmap(":/icons/searchOptions-none.png"));
        break;

    default:
        // Don't grey out this one - is a diagnositic for an uncoded combination
        newIcon.addPixmap(QPixmap(":/icons/searchOptions-unspecified.png"));
    }

    mIcon_searchOptions = newIcon;
    mpAction_searchOptions->setIcon(newIcon);
}

void TConsole::setSearchOptions(const SearchOptions optionsState)
{
    mSearchOptions = optionsState;
    mpAction_searchCaseSensitive->setChecked(optionsState & SearchOptionCaseSensitive);
    createSearchOptionIcon();
}

void TConsole::slot_toggleSearchCaseSensitivity(const bool state)
{
    if ((mSearchOptions & SearchOptionCaseSensitive) != state) {
        mSearchOptions = (mSearchOptions & ~(SearchOptionCaseSensitive)) | (state ? SearchOptionCaseSensitive : SearchOptionNone);
        createSearchOptionIcon();
        mpHost->mBufferSearchOptions = mSearchOptions;
    }
}

void TConsole::slot_clearSearchResults()
{
    buffer.clearSearchHighlights();
    mUpperPane->forceUpdate();
    mLowerPane->forceUpdate();
}

void TConsole::handleLinesOverflowEvent(const int lineCount)
{
    if (mType & ~(UserWindow | SubConsole)) {
        // It isn't a type that we need to worry about the number of lines of
        // text in it:
        return;
    }

    if (mScrollingEnabled) {
        // It is capable of scrolling so a "text overflow" is not a concern:
        return;
    }

    const int linesSpare = mUpperPane->getRowCount() - lineCount;
    if (linesSpare >= 0) {
        // There IS space for all the lines
        return;
    }

    // Else we do have an overflow situation so let's raise an event for it:
    TEvent sysWindowOverflow {};
    sysWindowOverflow.mArgumentList.append(QLatin1String("sysWindowOverflowEvent"));
    sysWindowOverflow.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    sysWindowOverflow.mArgumentList.append(mConsoleName);
    sysWindowOverflow.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    sysWindowOverflow.mArgumentList.append(QString::number(-linesSpare));
    sysWindowOverflow.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    mpHost->raiseEvent(sysWindowOverflow);
}

void TConsole::clearSplit()
{
    mLowerPane->mCursorY = buffer.size();
    mLowerPane->hide();
    buffer.mCursorY = buffer.size();
    mUpperPane->mCursorY = buffer.size();
    mUpperPane->mCursorX = 0;
    mUpperPane->mIsTailMode = true;
    mUpperPane->updateScreenView();
    mUpperPane->forceUpdate();
}
