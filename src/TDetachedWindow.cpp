/***************************************************************************
 *   Copyright (C) 2025 by Mike Conley - mike.conley@stickmud.com          *
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

#include "TDetachedWindow.h"
#include "TMainConsole.h"
#include "TTabBar.h"
#include "TDebug.h"
#include "Host.h"
#include "HostManager.h"
#include "mudlet.h"
#include "utils.h"
#include "dlgMapper.h"
#include "TRoomDB.h"
#include "TMap.h"
#include "dlgProfilePreferences.h"
#include "dlgNotepad.h"
#include "dlgPackageManager.h"
#include "dlgModuleManager.h"
#include <QVBoxLayout>
#include <QMenuBar>
#include <QAction>
#include <QMenu>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QHideEvent>
#include <QMimeData>
#include <QSettings>
#include <QScreen>
#include <QApplication>
#include <QToolBar>
#include <QToolButton>
#include <QTimer>
#include <QIcon>
#include <QAbstractSocket>
#include <QLabel>
#include <QSet>
#include <QStackedWidget>
#include <QSizePolicy>
#include <QWidget>
#include <QWindowStateChangeEvent>
#include <QDockWidget>
#include <QDesktopServices>
#include <QUrl>

TDetachedWindow::TDetachedWindow(const QString& profileName, TMainConsole* console, QWidget* parent, bool toolbarVisible)
    : QMainWindow(parent)
    , mCurrentProfileName(profileName)
{
    // Add the initial profile
    mProfileConsoleMap[profileName] = console;

    setupUI();
    createToolBar();
    createMenus();
    restoreWindowGeometry();

    // Set initial toolbar visibility based on main window state
    if (mpToolBar) {
        mpToolBar->setVisible(toolbarVisible);
    }

    // Set window properties
    //: This is the title of a Mudlet window which was detached from the main Mudlet window, and %1 is the name of the profile.
    setWindowTitle(tr("Mudlet - %1 (Detached)").arg(profileName));
    setWindowIcon(parent ? parent->windowIcon() : QIcon());
    setAttribute(Qt::WA_DeleteOnClose);
    setAcceptDrops(true);

    // Update toolbar state and window title for this profile
    updateToolBarActions();
    updateWindowTitle();

    // Initialize the window menu
    updateWindowMenu();
}

TDetachedWindow::~TDetachedWindow()
{
    // Set flag to prevent event handlers from accessing potentially invalid data
    mIsBeingDestroyed = true;
    
    // Immediately remove all references to this window from the main mudlet window
    if (auto mudletInstance = mudlet::self()) {
        auto& detachedWindowsMap = const_cast<QMap<QString, QPointer<TDetachedWindow>>&>(mudletInstance->getDetachedWindows());
        auto it = detachedWindowsMap.begin();
        while (it != detachedWindowsMap.end()) {
            if (it.value().data() == this) {
                it = detachedWindowsMap.erase(it);
            } else {
                ++it;
            }
        }
        
        // Update multi-view controls since detached windows changed
        mudletInstance->updateMultiViewControls();
    }

    // Clean up any docked widgets and restore main mappers
    for (auto it = mDockWidgetMap.begin(); it != mDockWidgetMap.end(); ++it) {
        if (it.value()) {
            // If this is a map dock widget, restore the main mapper
            if (it.key().startsWith("map_")) {
                QString profileName = it.key().mid(4); // Remove "map_" prefix
                if (auto mudletInstance = mudlet::self()) {
                    if (auto pHost = mudletInstance->getHostManager().getHost(profileName)) {
                        auto pMap = pHost->mpMap.data();

                        if (pMap && pHost->mpDockableMapWidget) {
                            // Find the main window's mapper
                            auto mainMapWidget = pHost->mpDockableMapWidget->widget();

                            if (auto mainMapper = qobject_cast<dlgMapper*>(mainMapWidget)) {
                                pMap->mpMapper = mainMapper;
                            }
                        }
                    }
                }
            }

            it.value()->deleteLater();
        }
    }

    mDockWidgetMap.clear();
    
    if (mpMapDockWidget) {
        // Restore main mapper if this detached window had an active map
        if (!mCurrentProfileName.isEmpty()) {
            if (auto mudletInstance = mudlet::self()) {
                if (auto pHost = mudletInstance->getHostManager().getHost(mCurrentProfileName)) {
                    auto pMap = pHost->mpMap.data();

                    if (pMap && pHost->mpDockableMapWidget) {
                        // Find the main window's mapper
                        auto mainMapWidget = pHost->mpDockableMapWidget->widget();

                        if (auto mainMapper = qobject_cast<dlgMapper*>(mainMapWidget)) {
                            pMap->mpMapper = mainMapper;
                        }
                    }
                }
            }
        }

        mpMapDockWidget->deleteLater();
        mpMapDockWidget = nullptr;
    }
}

void TDetachedWindow::setupUI()
{
    auto centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    mpMainLayout = new QVBoxLayout(centralWidget);
    mpMainLayout->setContentsMargins(0, 0, 0, 0);
    mpMainLayout->setSpacing(0);

    // Create a tab bar to show the profile names and allow reattachment
    mpTabBar = new TTabBar(centralWidget);
    mpTabBar->setMaximumHeight(30);
    mpTabBar->setTabsClosable(true);
    mpTabBar->setMovable(true);

    // Inherit font from main window's tab bar to ensure consistency
    if (auto mudletInstance = mudlet::self()) {
        if (mudletInstance->mpTabBar) {
            mpTabBar->setFont(mudletInstance->mpTabBar->font());
        }
    }

    // Add initial tab for the first profile
    if (!mCurrentProfileName.isEmpty()) {
        QString displayText = mCurrentProfileName;
        
        // Apply CDC identifier prefix if debug mode is active
        if (mudlet::smDebugMode) {
            Host* pHost = mudlet::self()->getHostManager().getHost(mCurrentProfileName);

            if (pHost) {
                QString debugTag = TDebug::getTag(pHost);

                if (!debugTag.isEmpty()) {
                    displayText = debugTag + mCurrentProfileName;
                }
            }
        }
        
        mpTabBar->addTab(displayText);
        mpTabBar->setTabData(0, mCurrentProfileName);
    }

    mpMainLayout->addWidget(mpTabBar);

    // Create a stacked widget to hold multiple consoles
    mpConsoleContainer = new QStackedWidget(centralWidget);
    mpMainLayout->addWidget(mpConsoleContainer);

    // Add the initial console
    if (!mCurrentProfileName.isEmpty() && mProfileConsoleMap.contains(mCurrentProfileName)) {
        auto console = mProfileConsoleMap[mCurrentProfileName];

        if (console) {
            mpConsoleContainer->addWidget(console);
            mpConsoleContainer->setCurrentWidget(console);
            console->show();
        }
    }

    updateTabIndicator(0);  // Set initial connection status

    // Connect signals
    connect(mpTabBar, &TTabBar::tabDetachRequested, this, &TDetachedWindow::onReattachAction);
    connect(mpTabBar, &QTabBar::tabCloseRequested, this, &TDetachedWindow::closeProfileByIndex);
    connect(mpTabBar, &QTabBar::currentChanged, this, &TDetachedWindow::slot_tabChanged);
    connect(mpTabBar, &QTabBar::tabMoved, this, &TDetachedWindow::slot_tabMoved);

    // Connect double-click to reattach
    connect(mpTabBar, &QTabBar::tabBarDoubleClicked, this, &TDetachedWindow::onReattachAction);

    // Connect context menu for right-click reattach
    mpTabBar->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(mpTabBar, &QWidget::customContextMenuRequested,
            this, &TDetachedWindow::showTabContextMenu);
}

void TDetachedWindow::createMenus()
{
    // Games menu with connection actions - matches main window order
    //: This is the name of a menu in the menubar of a detached Mudlet window.
    auto gamesMenu = menuBar()->addMenu(tr("&Games"));

    //: This is an item in the "Games" menu in the menubar of a detached Mudlet window.
    auto connectAction = new QAction(tr("&Play"), this);
#if defined(Q_OS_MACOS)
    connectAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_C));
#else
    connectAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_C));
#endif
    //: This explains the "Play" item in the "Games" menu in the menubar of a detached Mudlet window.
    connectAction->setStatusTip(tr("Configure connection details of, and make a connection to, game servers."));
    connect(connectAction, &QAction::triggered, this, &TDetachedWindow::slot_showConnectionDialog);
    gamesMenu->addAction(connectAction);

    gamesMenu->addSeparator();

    //: This is an item in the "Games" menu in the menubar of a detached Mudlet window.
    auto disconnectAction = new QAction(tr("&Disconnect"), this);
#if defined(Q_OS_MACOS)
    disconnectAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));
#else
    disconnectAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_D));
#endif
    //: This explains the "Disconnect" item in the "Games" menu in the menubar of a detached Mudlet window.
    disconnectAction->setStatusTip(tr("Disconnect from the current game server."));
    connect(disconnectAction, &QAction::triggered, this, &TDetachedWindow::slot_disconnectProfile);
    gamesMenu->addAction(disconnectAction);

    //: This is an item in the "Games" menu in the menubar of a detached Mudlet window.
    auto reconnectAction = new QAction(tr("&Reconnect"), this);
#if defined(Q_OS_MACOS)
    reconnectAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
#else
    reconnectAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_R));
#endif
    //: This explains the "Reconnect" item in the "Games" menu in the menubar of a detached Mudlet window.
    reconnectAction->setStatusTip(tr("Disconnect and then reconnect to the current game server."));
    connect(reconnectAction, &QAction::triggered, this, &TDetachedWindow::slot_reconnectProfile);
    gamesMenu->addAction(reconnectAction);

    gamesMenu->addSeparator();

    //: This is an item in the "Games" menu in the menubar of a detached Mudlet window.
    auto closeProfileAction = new QAction(tr("&Close Profile"), this);
#if defined(Q_OS_MACOS)
    closeProfileAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_W));
#else
    closeProfileAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_W));
#endif
    //: This explains the "Close Profile" item in the "Games" menu in the menubar of a detached Mudlet window.
    closeProfileAction->setStatusTip(tr("Close the current profile"));
    connect(closeProfileAction, &QAction::triggered, this, &TDetachedWindow::slot_closeCurrentProfile);
    gamesMenu->addAction(closeProfileAction);

    //: This is an item in the "Games" menu in the menubar of a detached Mudlet window.
    auto closeApplicationAction = new QAction(tr("Close &Mudlet"), this);
    //: This explains the "Close Mudlet" item in the "Games" menu in the menubar of a detached Mudlet window.
    closeApplicationAction->setStatusTip(tr("Close the entire Mudlet application"));
    connect(closeApplicationAction, &QAction::triggered, mudlet::self(), &QWidget::close);
    gamesMenu->addAction(closeApplicationAction);

    // Toolbox menu with all scripting tools - matches main window order
    //: This is the name of a menu in the menubar of a detached Mudlet window.
    auto toolboxMenu = menuBar()->addMenu(tr("&Toolbox"));

    //: This is an item in the "Toolbox" menu in the menubar of a detached Mudlet window.
    auto scriptEditorAction = new QAction(tr("&Script editor"), this);
#if defined(Q_OS_MACOS)
    scriptEditorAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E));
#else
    scriptEditorAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_E));
#endif
    //: This explains the "Script editor" item in the "Toolbox" menu in the menubar of a detached Mudlet window.
    scriptEditorAction->setStatusTip(tr("Opens the Editor for the different types of things that can be scripted by the user."));
    connect(scriptEditorAction, &QAction::triggered, this, &TDetachedWindow::slot_showTriggerDialog);
    toolboxMenu->addAction(scriptEditorAction);

    //: This is an item in the "Toolbox" menu in the menubar of a detached Mudlet window.
    auto showErrorsAction = new QAction(tr("Show &errors"), this);
    //: This explains the "Show errors" item in the "Toolbox" menu in the menubar of a detached Mudlet window.
    showErrorsAction->setStatusTip(tr("Show errors from scripts that you have running"));
    connect(showErrorsAction, &QAction::triggered, this, &TDetachedWindow::slot_showEditorDialog);
    toolboxMenu->addAction(showErrorsAction);

    //: This is an item in the "Toolbox" menu in the menubar of a detached Mudlet window.
    auto showMapAction = new QAction(tr("Show &map"), this);
#if defined(Q_OS_MACOS)
    showMapAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_M));
#else
    showMapAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_M));
#endif
    //: This explains the "Show map" item in the "Toolbox" menu in the menubar of a detached Mudlet window.
    showMapAction->setStatusTip(tr("Show or hide the game map."));
    connect(showMapAction, &QAction::triggered, this, &TDetachedWindow::slot_toggleMap);
    toolboxMenu->addAction(showMapAction);

    //: This is an item in the "Toolbox" menu in the menubar of a detached Mudlet window.
    auto inputLineAction = new QAction(tr("Compact &input line"), this);
#if defined(Q_OS_MACOS)
    inputLineAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L));
#else
    inputLineAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_L));
#endif
    //: This explains the "Compact input line" item in the "Toolbox" menu in the menubar of a detached Mudlet window.
    inputLineAction->setStatusTip(tr("Hide / show the search area and buttons at the bottom of the screen."));
    inputLineAction->setCheckable(true);
    connect(inputLineAction, &QAction::triggered, this, &TDetachedWindow::slot_toggleCompactInputLine);
    toolboxMenu->addAction(inputLineAction);

    //: This is an item in the "Toolbox" menu in the menubar of a detached Mudlet window.
    auto notepadAction = new QAction(tr("&Notepad"), this);
#if defined(Q_OS_MACOS)
    notepadAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_N));
#else
    notepadAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_N));
#endif
    //: This explains the "Notepad" item in the "Toolbox" menu in the menubar of a detached Mudlet window.
    notepadAction->setStatusTip(tr("Opens a free form text editor window for this profile that is saved between sessions."));
    connect(notepadAction, &QAction::triggered, this, &TDetachedWindow::slot_showNotesDialog);
    toolboxMenu->addAction(notepadAction);

    //: This is an item in the "Toolbox" menu in the menubar of a detached Mudlet window.
    auto ircAction = new QAction(tr("&IRC"), this);
    //: This explains the "IRC" item in the "Toolbox" menu in the menubar of a detached Mudlet window.
    ircAction->setStatusTip(tr("Opens a built-in IRC chat."));
    connect(ircAction, &QAction::triggered, mudlet::self(), &mudlet::slot_irc);
    toolboxMenu->addAction(ircAction);

    //: This is an item in the "Toolbox" menu in the menubar of a detached Mudlet window.
    auto packageManagerAction = new QAction(tr("&Package manager"), this);
#if defined(Q_OS_MACOS)
    packageManagerAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_O));
#else
    packageManagerAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_O));
#endif
    //: This explains the "Package manager" item in the "Toolbox" menu in the menubar of a detached Mudlet window.
    packageManagerAction->setStatusTip(tr("Install and remove collections of Mudlet lua items (packages)."));
    connect(packageManagerAction, &QAction::triggered, this, &TDetachedWindow::slot_showPackageManagerDialog);
    toolboxMenu->addAction(packageManagerAction);

    //: This is an item in the "Toolbox" menu in the menubar of a detached Mudlet window.
    auto replayAction = new QAction(tr("Load &replay"), this);
    //: This explains the "Load replay" item in the "Toolbox" menu in the menubar of a detached Mudlet window.
    replayAction->setStatusTip(tr("Load a previous saved game session that can be used to test Mudlet lua systems (off-line!)."));
    connect(replayAction, &QAction::triggered, this, &TDetachedWindow::slot_showReplayDialog);
    toolboxMenu->addAction(replayAction);

    //: This is an item in the "Toolbox" menu in the menubar of a detached Mudlet window.
    auto moduleManagerAction = new QAction(tr("&Module manager"), this);
#if defined(Q_OS_MACOS)
    moduleManagerAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_I));
#else
    moduleManagerAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_I));
#endif
    //: This explains the "Module manager" item in the "Toolbox" menu in the menubar of a detached Mudlet window.
    moduleManagerAction->setStatusTip(tr("Install and remove (share- & sync-able) collections of Mudlet lua items (modules)."));
    connect(moduleManagerAction, &QAction::triggered, this, &TDetachedWindow::slot_showModuleManagerDialog);
    toolboxMenu->addAction(moduleManagerAction);

    //: This is an item in the "Toolbox" menu in the menubar of a detached Mudlet window.
    auto packageExporterAction = new QAction(tr("Package &exporter"), this);
    //: This explains the "Package exporter" item in the "Toolbox" menu in the menubar of a detached Mudlet window.
    packageExporterAction->setStatusTip(tr("Gather and bundle up collections of Mudlet Lua items and other reasources into a module."));
    connect(packageExporterAction, &QAction::triggered, this, &TDetachedWindow::slot_showPackageExporterDialog);
    toolboxMenu->addAction(packageExporterAction);

    //: This is an item in the "Toolbox" menu in the menubar of a detached Mudlet window.
    auto toggleReplayAction = new QAction(tr("Record replay"), this);
#if defined(Q_OS_MACOS)
    toggleReplayAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_R));
#else
    toggleReplayAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_R));
#endif
    //: This explains the "Record replay" item in the "Toolbox" menu in the menubar of a detached Mudlet window.
    toggleReplayAction->setStatusTip(tr("Toggle recording of replays."));
    connect(toggleReplayAction, &QAction::triggered, this, &TDetachedWindow::slot_toggleReplay);
    toolboxMenu->addAction(toggleReplayAction);

    //: This is an item in the "Toolbox" menu in the menubar of a detached Mudlet window.
    auto toggleLoggingAction = new QAction(tr("Record log"), this);
#if defined(Q_OS_MACOS)
    toggleLoggingAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_L));
#else
    toggleLoggingAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_L));
#endif
    //: This explains the "Record log" item in the "Toolbox" menu in the menubar of a detached Mudlet window.
    toggleLoggingAction->setStatusTip(tr("Toggle logging facilities."));
    connect(toggleLoggingAction, &QAction::triggered, this, &TDetachedWindow::slot_toggleLogging);
    toolboxMenu->addAction(toggleLoggingAction);

    //: This is an item in the "Toolbox" menu in the menubar of a detached Mudlet window.
    auto toggleEmergencyStopAction = new QAction(tr("Emergency stop"), this);
    toggleEmergencyStopAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_S));
    //: This explains the "Emergency stop" item in the "Toolbox" menu in the menubar of a detached Mudlet window.
    toggleEmergencyStopAction->setStatusTip(tr("Toggle all triggers, aliases, timers, etc. on or off"));
    connect(toggleEmergencyStopAction, &QAction::triggered, this, &TDetachedWindow::slot_toggleEmergencyStop);
    toolboxMenu->addAction(toggleEmergencyStopAction);

    // Options menu - matches main window order
    //: This is the name of a menu in the menubar of a detached Mudlet window.
    auto optionsMenu = menuBar()->addMenu(tr("&Options"));

    //: This is an item in the "Options" menu in the menubar of a detached Mudlet window.
    auto optionsAction = new QAction(tr("&Preferences"), this);
    optionsAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_P));
    //: This explains the "Preferences" item in the "Options" menu in the menubar of a detached Mudlet window.
    optionsAction->setStatusTip(tr("Configure setting for the Mudlet application globally and for the current profile."));
    connect(optionsAction, &QAction::triggered, this, &TDetachedWindow::slot_showPreferencesDialog);
    optionsMenu->addAction(optionsAction);

    //: This is an item in the "Options" menu in the menubar of a detached Mudlet window.
    auto toggleTimeStampAction = new QAction(tr("&Timestamps"), this);
    toggleTimeStampAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_T));
    //: This explains the "Timestamps" item in the "Options" menu in the menubar of a detached Mudlet window.
    toggleTimeStampAction->setStatusTip(tr("Toggle time stamps on the main console."));
    connect(toggleTimeStampAction, &QAction::triggered, this, &TDetachedWindow::slot_toggleTimeStamp);
    optionsMenu->addAction(toggleTimeStampAction);

    optionsMenu->addSeparator();

    //: This is an item in the "Options" menu in the menubar of a detached Mudlet window.
    auto muteMediaAction = new QAction(tr("Mute all media"), this);
    muteMediaAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_K));
    //: This explains the "Mute all media" item in the "Options" menu in the menubar of a detached Mudlet window.
    muteMediaAction->setStatusTip(tr("Mutes all media played."));
    muteMediaAction->setCheckable(true);
    connect(muteMediaAction, &QAction::triggered, this, &TDetachedWindow::slot_muteMedia);
    optionsMenu->addAction(muteMediaAction);

    //: This is an item in the "Options" menu in the menubar of a detached Mudlet window.
    auto muteAPIAction = new QAction(tr("Mute sounds from Mudlet (triggers, scripts, etc.)"), this);
    //: This explains the "Mute sounds from Mudlet (triggers, scripts, etc.)" item in the "Options" menu in the menubar of a detached Mudlet window.
    muteAPIAction->setStatusTip(tr("Mutes media played by the Lua API and scripts."));
    muteAPIAction->setCheckable(true);
    connect(muteAPIAction, &QAction::triggered, this, &TDetachedWindow::slot_muteAPI);
    optionsMenu->addAction(muteAPIAction);

    //: This is an item in the "Options" menu in the menubar of a detached Mudlet window.
    auto muteGameAction = new QAction(tr("Mute sounds from the game (MCMP, MSP)"), this);
    //: This explains the "Mute sounds from the game (MCMP, MSP)" item in the "Options" menu in the menubar of a detached Mudlet window.
    muteGameAction->setStatusTip(tr("Mutes media played by the game (MCMP, MSP)."));
    muteGameAction->setCheckable(true);
    connect(muteGameAction, &QAction::triggered, this, &TDetachedWindow::slot_muteGame);
    optionsMenu->addAction(muteGameAction);

    // Window menu - matches main window order (except reattach vs detach)
    //: This is the name of a menu in the menubar of a detached Mudlet window.
    mpWindowMenu = menuBar()->addMenu(tr("&Window"));

    //: This is an item in the "Window" menu in the menubar of a detached Mudlet window.
    auto fullScreenAction = new QAction(tr("&Fullscreen"), this);
    fullScreenAction->setCheckable(true);
    //: This explains the "Fullscreen" item in the "Window" menu in the menubar of a detached Mudlet window.
    fullScreenAction->setStatusTip(tr("Toggle Full Screen View"));
    connect(fullScreenAction, &QAction::triggered, this, &TDetachedWindow::slot_toggleFullScreen);
    mpWindowMenu->addAction(fullScreenAction);

    //: This is an item in the "Window" menu in the menubar of a detached Mudlet window.
    auto multiViewAction = new QAction(tr("&Multiview"), this);
    multiViewAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_V));
    //: This explains the "Multiview" item in the "Window" menu in the menubar of a detached Mudlet window.
    multiViewAction->setStatusTip(tr("Splits the Mudlet screen to show multiple profiles at once; disabled when less than two are loaded."));
    multiViewAction->setCheckable(true);
    connect(multiViewAction, &QAction::triggered, this, &TDetachedWindow::slot_toggleMultiView);
    mpWindowMenu->addAction(multiViewAction);

    //: This is an item in the "Window" menu in the menubar of a detached Mudlet window.
    auto reattachAction = new QAction(tr("&Reattach to Main Window"), this);
    //: This explains the "Reattach to Main Window" item in the "Window" menu in the menubar of a detached Mudlet window.
    reattachAction->setStatusTip(tr("Reattach this profile window to the main Mudlet window"));
    connect(reattachAction, &QAction::triggered, this, &TDetachedWindow::onReattachAction);
    mpWindowMenu->addAction(reattachAction);

    mpWindowMenu->addSeparator();

    //: This is an item in the "Window" menu in the menubar of a detached Mudlet window.
    auto alwaysOnTopAction = new QAction(tr("Always on &Top"), this);
    alwaysOnTopAction->setCheckable(true);
    //: This explains the "Always on Top" item in the "Window" menu in the menubar of a detached Mudlet window.
    alwaysOnTopAction->setStatusTip(tr("Keep this window always on top of other windows"));
    connect(alwaysOnTopAction, &QAction::triggered, this, &TDetachedWindow::slot_toggleAlwaysOnTop);
    mpWindowMenu->addAction(alwaysOnTopAction);

    //: This is an item in the "Window" menu in the menubar of a detached Mudlet window.
    auto minimizeAction = new QAction(tr("&Minimize"), this);
    //: This explains the "Minimize" item in the "Window" menu in the menubar of a detached Mudlet window.
    minimizeAction->setStatusTip(tr("Minimize this window"));
    connect(minimizeAction, &QAction::triggered, this, &QWidget::showMinimized);
    mpWindowMenu->addAction(minimizeAction);

    // Help menu - matches main window order
    //: This is the name of a menu in the menubar of a detached Mudlet window.
    auto helpMenu = menuBar()->addMenu(tr("&Help"));

    //: This is an item in the "Help" menu in the menubar of a detached Mudlet window.
    auto helpAction = new QAction(tr("&API Reference"), this);
    //: This explains the "API Reference" item in the "Help" menu in the menubar of a detached Mudlet window.
    helpAction->setStatusTip(tr("Opens the Mudlet manual in your web browser."));
    connect(helpAction, &QAction::triggered, mudlet::self(), &mudlet::slot_showHelpDialog);
    helpMenu->addAction(helpAction);

    //: This is an item in the "Help" menu in the menubar of a detached Mudlet window.
    auto videoAction = new QAction(tr("&Video tutorials"), this);
    //: This explains the "Video tutorials" item in the "Help" menu in the menubar of a detached Mudlet window.
    videoAction->setStatusTip(tr("Opens an (on-line) collection of \"Educational Mudlet screencasts\" in your system web-browser."));
    connect(videoAction, &QAction::triggered, mudlet::self(), &mudlet::slot_showHelpDialogVideo);
    helpMenu->addAction(videoAction);

    //: This is an item in the "Help" menu in the menubar of a detached Mudlet window.
    auto discordAction = new QAction(tr("&Discord"), this);
    //: This explains the "Discord" item in the "Help" menu in the menubar of a detached Mudlet window.
    discordAction->setStatusTip(tr("Open a link to Discord."));
    connect(discordAction, &QAction::triggered, this, &TDetachedWindow::slot_profileDiscord);
    helpMenu->addAction(discordAction);

    //: This is an item in the "Help" menu in the menubar of a detached Mudlet window.
    auto mudletDiscordAction = new QAction(tr("Discord &help channel"), this);
    //: This explains the "Discord help channel" item in the "Help" menu in the menubar of a detached Mudlet window.
    mudletDiscordAction->setStatusTip(tr("Open a link to the Mudlet server on Discord."));
    connect(mudletDiscordAction, &QAction::triggered, mudlet::self(), &mudlet::slot_mudletDiscord);
    helpMenu->addAction(mudletDiscordAction);

    //: This is an item in the "Help" menu in the menubar of a detached Mudlet window.
    auto liveHelpChatAction = new QAction(tr("&Live help chat"), this);
    //: This explains the "Live help chat" item in the "Help" menu in the menubar of a detached Mudlet window.
    liveHelpChatAction->setStatusTip(tr("Opens a connect to an IRC server (LiberaChat) in your system web-browser."));
    connect(liveHelpChatAction, &QAction::triggered, mudlet::self(), &mudlet::slot_showHelpDialogIrc);
    helpMenu->addAction(liveHelpChatAction);

    //: This is an item in the "Help" menu in the menubar of a detached Mudlet window.
    auto forumAction = new QAction(tr("Online &forum"), this);
    //: This explains the "Online forum" item in the "Help" menu in the menubar of a detached Mudlet window.
    forumAction->setStatusTip(tr("Opens the (on-line) Mudlet Forum in your system web-browser."));
    connect(forumAction, &QAction::triggered, mudlet::self(), &mudlet::slot_showHelpDialogForum);
    helpMenu->addAction(forumAction);

    // About menu - matches main window order
    //: This is the name of a menu in the menubar of a detached Mudlet window.
    auto aboutMenu = menuBar()->addMenu(tr("&About"));

    //: This is an item in the "About" menu in the menubar of a detached Mudlet window.
    auto aboutAction = new QAction(tr("About &Mudlet"), this);
    //: This explains the "About Mudlet" item in the "About" menu in the menubar of a detached Mudlet window.
    aboutAction->setStatusTip(tr("Inform yourself about this version of Mudlet, the people who made it and the licence under which you can share it."));
    connect(aboutAction, &QAction::triggered, mudlet::self(), &mudlet::slot_showAboutDialog);
    aboutMenu->addAction(aboutAction);

#if defined(INCLUDE_UPDATER)
    //: This is an item in the "About" menu in the menubar of a detached Mudlet window.
    auto updateAction = new QAction(tr("&Check for updates..."), this);
    //: This explains the "Check for updates..." item in the "About" menu in the menubar of a detached Mudlet window.
    updateAction->setStatusTip(tr("Check for newer versions of Mudlet"));
    connect(updateAction, &QAction::triggered, mudlet::self(), &mudlet::slot_manualUpdateCheck);
    aboutMenu->addAction(updateAction);

    //: This is an item in the "About" menu in the menubar of a detached Mudlet window.
    auto changelogAction = new QAction(tr("Show &changelog"), this);
    //: This explains the "Show changelog" item in the "About" menu in the menubar of a detached Mudlet window.
    changelogAction->setStatusTip(tr("Show the changelog for this version"));
    connect(changelogAction, &QAction::triggered, mudlet::self(), &mudlet::slot_showFullChangelog);
    aboutMenu->addAction(changelogAction);
#endif

    //: This is an item in the "About" menu in the menubar of a detached Mudlet window.
    auto reportIssueAction = new QAction(tr("&Report an issue"), this);
    //: This explains the "Report an issue" item in the "About" menu in the menubar of a detached Mudlet window.
    reportIssueAction->setStatusTip(tr("The public test build gets newer features to you quicker, and you help us find issues in them quicker. Spotted something odd? Let us know asap!"));
    connect(reportIssueAction, &QAction::triggered, this, &TDetachedWindow::slot_reportIssue);
    aboutMenu->addAction(reportIssueAction);

    // Toolbar visibility toggle
    //: This is an item for the toolbar visibility toggle in a detached Mudlet window.
    mpActionToggleToolBar = new QAction(tr("Show &Toolbar"), this);
    mpActionToggleToolBar->setCheckable(true);
    mpActionToggleToolBar->setChecked(mpToolBar ? mpToolBar->isVisible() : true);
    //: This explains the "Show Toolbar" action for toolbar visibility in a detached Mudlet window.
    mpActionToggleToolBar->setStatusTip(tr("Show or hide the toolbar"));
    connect(mpActionToggleToolBar, &QAction::triggered, this, &TDetachedWindow::slot_toggleToolBarVisibility);

    // Connect the Window menu's aboutToShow signal to update the window list
    connect(mpWindowMenu, &QMenu::aboutToShow, this, &TDetachedWindow::updateWindowMenu);
}

void TDetachedWindow::closeEvent(QCloseEvent* event)
{
    // Ensure profiles are properly closed when window is closed
    // This prevents orphaned profiles that remain loaded but invisible
    if (!mIsReattaching) {
        // Get a copy of profile names before we start closing them
        QStringList profilesToClose = mProfileConsoleMap.keys();
        
        // Properly close each profile - this ensures proper cleanup and save prompts
        for (const QString& profileName : profilesToClose) {
#if defined(DEBUG_WINDOW_HANDLING)
            qDebug() << "TDetachedWindow::closeEvent() - Properly closing profile:" << profileName;
#endif
            mudlet::self()->slot_closeProfileByName(profileName);
        }
        
        // Remove all consoles from the stacked widget and reset their parents
        for (auto console : mProfileConsoleMap) {
            if (console) {
                mpConsoleContainer->removeWidget(console);
                console->setParent(nullptr);
            }
        }

        mProfileConsoleMap.clear();
        
        // Emit signal to notify main window for any remaining cleanup
        for (const QString& profileName : profilesToClose) {
            emit windowClosed(profileName);
        }
    }
    event->accept();
}

void TDetachedWindow::dragEnterEvent(QDragEnterEvent* event)
{
    const QMimeData* mimeData = event->mimeData();

    if (mimeData->hasFormat("application/x-mudlet-tab")) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void TDetachedWindow::dragMoveEvent(QDragMoveEvent* event)
{
    const QMimeData* mimeData = event->mimeData();

    if (mimeData->hasFormat("application/x-mudlet-tab")) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void TDetachedWindow::dropEvent(QDropEvent* event)
{
    const QMimeData* mimeData = event->mimeData();

    if (mimeData->hasFormat("application/x-mudlet-tab")) {
        const QString profileName = QString::fromUtf8(mimeData->data("application/x-mudlet-tab"));

        // Don't allow dropping a tab that's already in this window
        if (!mProfileConsoleMap.contains(profileName)) {
            // Request main window to detach this tab to this detached window
            emit profileDetachToWindowRequested(profileName, this);
        }

        event->acceptProposedAction();
    }
}

void TDetachedWindow::moveEvent(QMoveEvent* event)
{
    mLastPosition = event->pos();
    QMainWindow::moveEvent(event);

    // Check if we should offer to merge with another detached window
    QTimer::singleShot(100, this, &TDetachedWindow::checkForWindowMergeOpportunity);
}

void TDetachedWindow::resizeEvent(QResizeEvent* event)
{
    mLastSize = event->size();
    QMainWindow::resizeEvent(event);
}

void TDetachedWindow::hideEvent(QHideEvent* event)
{
    // Allow hiding if the window is currently minimized (check current state directly)
    // This handles the case where minimize happens before changeEvent sets mIsBeingMinimized
    if (isMinimized() || mIsBeingMinimized) {
        QMainWindow::hideEvent(event);
        return;
    }

    // Allow hiding if we're changing window flags (e.g., always on top)
    if (mIsChangingWindowFlags) {
        QMainWindow::hideEvent(event);
        return;
    }

    // Prevent the window from being hidden if it should stay visible (has profiles)
    // IMPORTANT: Only allow hiding if there are NO profiles remaining, regardless of mIsReattaching
    if (mShouldStayVisible && mpTabBar && mpTabBar->count() > 0) {
#if defined(DEBUG_WINDOW_HANDLING)
        qDebug() << "TDetachedWindow::hideEvent: Preventing window hide - has" << mpTabBar->count() << "profiles";
#endif
        // Force the window to stay visible - but only if not minimized
        QTimer::singleShot(0, this, [this]() {
            if (mpTabBar && mpTabBar->count() > 0 && !mIsBeingMinimized) {
                setVisible(true);
                show();
                raise();
                activateWindow();
            }
        });
        event->ignore();
        return;
    }

    QMainWindow::hideEvent(event);
}

void TDetachedWindow::onReattachAction()
{
    // Prevent duplicate calls - if we're already processing a reattach on this window, ignore
    if (mReattachInProgress) {
        return;
    }

    // Only set mIsReattaching if this is the last profile (which will close the window)
    if (getProfileCount() <= 1) {
        mIsReattaching = true;
    }

    // Reattach the currently active profile
    if (!mCurrentProfileName.isEmpty()) {
        mReattachInProgress = true;

        emit reattachRequested(mCurrentProfileName);

        // Reset the flag after a short delay to allow for the operation to complete
        QTimer::singleShot(500, this, [this]() {
            mReattachInProgress = false;
        });
    }
}

void TDetachedWindow::showTabContextMenu(const QPoint& position)
{
    // Only show context menu if click was on a tab
    int tabIndex = mpTabBar->tabAt(position);

    if (tabIndex == -1) {
        return;
    }

    QString profileName = mpTabBar->tabData(tabIndex).toString();

    QMenu contextMenu(this);

    auto reattachAction = contextMenu.addAction(QIcon(qsl(":/icons/view-restore.png")),
    //: This is an item in the context menu when clicked on a detached tab, and %1 is the name of the profile.
                                               tr("Reattach '%1' to Main Window").arg(profileName));
    connect(reattachAction, &QAction::triggered, [this, profileName] {
        // Switch to this profile first, then reattach
        switchToProfile(profileName);
        onReattachAction();
    });

    contextMenu.addSeparator();

    auto closeTabAction = contextMenu.addAction(QIcon(qsl(":/icons/profile-close.png")),
    //: This is an item in the context menu when clicked on a detached tab, and %1 is the name of the profile.
                                               tr("Close Profile '%1'").arg(profileName));
    connect(closeTabAction, &QAction::triggered, [this, tabIndex] {
        closeProfileByIndex(tabIndex);
    });

    if (mProfileConsoleMap.size() > 1) {
        contextMenu.addSeparator();

        auto closeWindowAction = contextMenu.addAction(QIcon(qsl(":/icons/dialog-close.png")),
    //: This is an item in the context menu when clicked on a detached tab.
                                                      tr("Close Window (All Profiles)"));
        connect(closeWindowAction, &QAction::triggered, this, &QWidget::close);
    }

    contextMenu.addSeparator();

    // Add toolbar visibility toggle to the context menu
    //: This is an item in the context menu when clicked on a detached tab.
    auto toolbarToggleAction = contextMenu.addAction(tr("Profile Toolbar"));
    toolbarToggleAction->setCheckable(true);
    toolbarToggleAction->setChecked(mpToolBar && mpToolBar->isVisible());
    connect(toolbarToggleAction, &QAction::triggered, this, &TDetachedWindow::slot_toggleToolBarVisibility);

    // Add connection indicator toggle
    //: This is an item in the context menu when clicked on a detached tab.
    auto connectionIndicatorToggleAction = contextMenu.addAction(tr("Show Connection Indicators on Tabs"));
    connectionIndicatorToggleAction->setCheckable(true);
    connectionIndicatorToggleAction->setChecked(mudlet::self()->showTabConnectionIndicators());
    connect(connectionIndicatorToggleAction, &QAction::triggered, this, [this](bool checked) {
        mudlet::self()->setShowTabConnectionIndicators(checked);
    });

    contextMenu.exec(mpTabBar->mapToGlobal(position));
}

void TDetachedWindow::saveWindowGeometry()
{
    QSettings& settings = *mudlet::getQSettings();
    // Use current profile name for settings key
    const QString key = QString("DetachedWindow/%1").arg(mCurrentProfileName.isEmpty() ? "Unknown" : mCurrentProfileName);
    settings.setValue(key + "/geometry", saveGeometry());
    settings.setValue(key + "/windowState", saveState());
}

void TDetachedWindow::restoreWindowGeometry()
{
    QSettings& settings = *mudlet::getQSettings();
    // Use current profile name for settings key
    const QString key = QString("DetachedWindow/%1").arg(mCurrentProfileName.isEmpty() ? "Unknown" : mCurrentProfileName);

    const QByteArray geometry = settings.value(key + "/geometry").toByteArray();
    const QByteArray windowState = settings.value(key + "/windowState").toByteArray();

    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    } else {
        // Default size and position
        resize(800, 600);

        // Center on screen containing the main window
        if (parentWidget()) {
            const QScreen* screen = QApplication::screenAt(parentWidget()->pos());

            if (screen) {
                const QRect screenGeometry = screen->availableGeometry();
                move(screenGeometry.center() - rect().center());
            }
        }
    }

    if (!windowState.isEmpty()) {
        restoreState(windowState);
    }
}

void TDetachedWindow::createToolBar()
{
    mpToolBar = new QToolBar(this);
    mpToolBar->setObjectName(qsl("detachedMainToolBar"));
    mpToolBar->setWindowTitle(tr("Profile Toolbar"));
    addToolBar(mpToolBar);
    mpToolBar->setMovable(false);
    mpToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    // Reattach action - placed first in the toolbar for prominence
    //: This is an item in the toolbar of a detached Mudlet window. It will reattach the profile to the main Mudlet window.
    mpActionReattach = new QAction(QIcon(qsl(":/icons/mudlet_main_48px.png")), tr("Reattach"), this);
    //: This explains the "Reattach" item in the toolbar of a detached Mudlet window.
    mpActionReattach->setToolTip(utils::richText(tr("Reattach this profile window to the main Mudlet window")));
    mpActionReattach->setObjectName(qsl("reattach_action"));
    mpToolBar->addAction(mpActionReattach);

    // Add separator after reattach button to make it distinct
    mpToolBar->addSeparator();

    // Connect button with dropdown actions
    mpButtonConnect = new QToolButton(this);
    //: This is an item in the toolbar of a detached Mudlet window.
    mpButtonConnect->setText(tr("Connect"));
    mpButtonConnect->setObjectName(qsl("connect"));
    mpButtonConnect->setContextMenuPolicy(Qt::ActionsContextMenu);
    mpButtonConnect->setPopupMode(QToolButton::MenuButtonPopup);
    mpButtonConnect->setAutoRaise(true);
    mpButtonConnect->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    mpToolBar->addWidget(mpButtonConnect);

    //: This is a sub-item of the "Connect" item in the toolbar of a detached Mudlet window.
    mpActionConnect = new QAction(tr("Connect"), this);
    mpActionConnect->setIcon(QIcon(qsl(":/icons/preferences-web-browser-cache.png")));
    mpActionConnect->setIconText(tr("Connect"));
    mpActionConnect->setObjectName(qsl("connect"));

    //: This is a sub-item of the "Connect" item in the toolbar of a detached Mudlet window.
    mpActionDisconnect = new QAction(tr("Disconnect"), this);
    mpActionDisconnect->setObjectName(qsl("disconnect"));

    //: This is a sub-item of the "Connect" item in the toolbar of a detached Mudlet window.
    mpActionCloseProfile = new QAction(tr("Close profile"), this);
    mpActionCloseProfile->setIcon(QIcon(qsl(":/icons/profile-close.png")));
    mpActionCloseProfile->setIconText(tr("Close profile"));
    mpActionCloseProfile->setObjectName(qsl("close_profile"));

    //: This is a sub-item of the "Connect" item in the toolbar of a detached Mudlet window.
    mpActionCloseApplication = new QAction(tr("Close Mudlet"), this);
    mpActionCloseApplication->setIcon(QIcon::fromTheme(qsl("application-exit"), QIcon(qsl(":/icons/application-exit.png"))));
    mpActionCloseApplication->setIconText(tr("Close Mudlet"));
    mpActionCloseApplication->setObjectName(qsl("close_application"));

    mpButtonConnect->addAction(mpActionConnect);
    mpButtonConnect->addAction(mpActionDisconnect);
    mpButtonConnect->addAction(mpActionCloseProfile);
    mpButtonConnect->addAction(mpActionCloseApplication);
    mpButtonConnect->setDefaultAction(mpActionConnect);

    // Script Editor Actions
    mpActionTriggers = new QAction(QIcon(qsl(":/icons/tools-wizard.png")), tr("Triggers"), this);
    mpActionTriggers->setToolTip(utils::richText(tr("Show and edit triggers")));
    mpToolBar->addAction(mpActionTriggers);
    mpActionTriggers->setObjectName(qsl("triggers_action"));

    mpActionAliases = new QAction(QIcon(qsl(":/icons/system-users.png")), tr("Aliases"), this);
    mpActionAliases->setToolTip(utils::richText(tr("Show and edit aliases")));
    mpToolBar->addAction(mpActionAliases);
    mpActionAliases->setObjectName(qsl("aliases_action"));

    mpActionTimers = new QAction(QIcon(qsl(":/icons/chronometer.png")), tr("Timers"), this);
    mpActionTimers->setToolTip(utils::richText(tr("Show and edit timers")));
    mpToolBar->addAction(mpActionTimers);
    mpActionTimers->setObjectName(qsl("timers_action"));

    mpActionButtons = new QAction(QIcon(qsl(":/icons/bookmarks.png")), tr("Buttons"), this);
    mpActionButtons->setToolTip(utils::richText(tr("Show and edit easy buttons")));
    mpToolBar->addAction(mpActionButtons);
    mpActionButtons->setObjectName(qsl("buttons_action"));

    mpActionScripts = new QAction(QIcon(qsl(":/icons/document-properties.png")), tr("Scripts"), this);
    mpActionScripts->setToolTip(utils::richText(tr("Show and edit scripts")));
    mpToolBar->addAction(mpActionScripts);
    mpActionScripts->setObjectName(qsl("scripts_action"));

    mpActionKeys = new QAction(QIcon(qsl(":/icons/preferences-desktop-keyboard.png")), tr("Keys"), this);
    mpActionKeys->setToolTip(utils::richText(tr("Show and edit keys")));
    mpToolBar->addAction(mpActionKeys);
    mpActionKeys->setObjectName(qsl("keys_action"));

    mpActionVariables = new QAction(QIcon(qsl(":/icons/variables.png")), tr("Variables"), this);
    mpActionVariables->setToolTip(utils::richText(tr("Show and edit Lua variables")));
    mpToolBar->addAction(mpActionVariables);
    mpActionVariables->setObjectName(qsl("variables_action"));

    // Mute button with dropdown
    mpButtonMute = new QToolButton(this);
    mpButtonMute->setText(tr("Mute"));
    mpButtonMute->setObjectName(qsl("mute"));
    mpButtonMute->setContextMenuPolicy(Qt::ActionsContextMenu);
    mpButtonMute->setPopupMode(QToolButton::MenuButtonPopup);
    mpButtonMute->setAutoRaise(true);
    mpButtonMute->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    mpToolBar->addWidget(mpButtonMute);

    mpActionMuteMedia = new QAction(tr("Mute all media"), this);
    mpActionMuteMedia->setIcon(QIcon(qsl(":/icons/mute.png")));
    mpActionMuteMedia->setIconText(tr("Mute all media"));
    mpActionMuteMedia->setObjectName(qsl("muteMedia"));
    mpActionMuteMedia->setCheckable(true);

    mpActionMuteAPI = new QAction(tr("Mute sounds from Mudlet (triggers, scripts, etc.)"), this);
    mpActionMuteAPI->setIcon(QIcon(qsl(":/icons/mute.png")));
    mpActionMuteAPI->setIconText(tr("Mute sounds from Mudlet (triggers, scripts, etc.)"));
    mpActionMuteAPI->setObjectName(qsl("muteAPI"));
    mpActionMuteAPI->setCheckable(true);

    mpActionMuteGame = new QAction(tr("Mute sounds from the game (MCMP, MSP)"), this);
    mpActionMuteGame->setIcon(QIcon(qsl(":/icons/mute.png")));
    mpActionMuteGame->setIconText(tr("Mute sounds from the game (MCMP, MSP)"));
    mpActionMuteGame->setObjectName(qsl("muteGame"));
    mpActionMuteGame->setCheckable(true);

    mpButtonMute->addAction(mpActionMuteMedia);
    mpButtonMute->addAction(mpActionMuteAPI);
    mpButtonMute->addAction(mpActionMuteGame);
    mpButtonMute->setDefaultAction(mpActionMuteMedia);

    // Discord button with dropdown actions
    mpButtonDiscord = new QToolButton(this);
    mpButtonDiscord->setText(qsl("Discord"));
    mpButtonDiscord->setObjectName(qsl("discord"));
    mpButtonDiscord->setContextMenuPolicy(Qt::ActionsContextMenu);
    mpButtonDiscord->setPopupMode(QToolButton::MenuButtonPopup);
    mpButtonDiscord->setAutoRaise(true);
    mpButtonDiscord->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    mpToolBar->addWidget(mpButtonDiscord);

    mpActionDiscord = new QAction(tr("Open Discord"), this);
    mpActionDiscord->setIcon(QIcon(qsl(":/icons/Discord-Logo-Color.png")));
    mpActionDiscord->setIconText(qsl("Discord"));
    mpActionDiscord->setObjectName(qsl("openDiscord"));

    mpActionMudletDiscord = new QAction(QIcon(qsl(":/icons/mudlet_discord.png")), tr("Mudlet chat"), this);
    mpActionMudletDiscord->setToolTip(utils::richText(tr("Open a link to the Mudlet server on Discord")));
    mpToolBar->addAction(mpActionMudletDiscord);
    mpActionMudletDiscord->setObjectName(qsl("mudlet_discord"));
    mpToolBar->widgetForAction(mpActionMudletDiscord)->setObjectName(mpActionMudletDiscord->objectName());
    mpActionMudletDiscord->setVisible(false); // Mudlet Discord becomes visible if game has custom invite

    mpActionIRC = new QAction(tr("Open IRC"), this);
    mpActionIRC->setIcon(QIcon(qsl(":/icons/internet-telephony.png")));
    mpActionIRC->setObjectName(qsl("openIRC"));

    mpButtonDiscord->addAction(mpActionDiscord);
    mpButtonDiscord->addAction(mpActionIRC);
    mpButtonDiscord->setDefaultAction(mpActionDiscord);

    // Map and other tools
    mpActionMapper = new QAction(QIcon(qsl(":/icons/applications-internet.png")), tr("Map"), this);
    mpActionMapper->setToolTip(utils::richText(tr("Show/hide the map")));
    mpToolBar->addAction(mpActionMapper);
    mpActionMapper->setObjectName(qsl("map_action"));

    mpActionHelp = new QAction(QIcon(qsl(":/icons/help-hint.png")), tr("Manual"), this);
    mpActionHelp->setToolTip(utils::richText(tr("Browse reference material and documentation")));
    mpToolBar->addAction(mpActionHelp);
    mpActionHelp->setObjectName(qsl("manual_action"));

    mpActionOptions = new QAction(QIcon(qsl(":/icons/configure.png")), tr("Settings"), this);
    mpActionOptions->setToolTip(utils::richText(tr("See and edit profile preferences")));
    mpToolBar->addAction(mpActionOptions);
    mpActionOptions->setObjectName(qsl("settings_action"));

    mpActionNotes = new QAction(QIcon(qsl(":/icons/applications-accessories.png")), tr("Notepad"), this);
    mpActionNotes->setToolTip(utils::richText(tr("Open a notepad that you can store your notes in")));
    mpToolBar->addAction(mpActionNotes);
    mpActionNotes->setObjectName(qsl("notepad_action"));

    // Package managers button
    mpButtonPackageManagers = new QToolButton(this);
    mpButtonPackageManagers->setText(tr("Packages"));
    mpButtonPackageManagers->setObjectName(qsl("package_manager"));
    mpButtonPackageManagers->setContextMenuPolicy(Qt::ActionsContextMenu);
    mpButtonPackageManagers->setPopupMode(QToolButton::MenuButtonPopup);
    mpButtonPackageManagers->setAutoRaise(true);
    mpButtonPackageManagers->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    mpToolBar->addWidget(mpButtonPackageManagers);

    mpActionPackageManager = new QAction(tr("Package Manager"), this);
    mpActionPackageManager->setIcon(QIcon(qsl(":/icons/package-manager.png")));
    mpActionPackageManager->setIconText(tr("Packages"));
    mpActionPackageManager->setObjectName(qsl("package_manager"));

    mpActionModuleManager = new QAction(tr("Module Manager"), this);
    mpActionModuleManager->setIcon(QIcon(qsl(":/icons/module-manager.png")));
    mpActionModuleManager->setObjectName(qsl("module_manager"));

    mpActionPackageExporter = new QAction(tr("Package Exporter"), this);
    mpActionPackageExporter->setIcon(QIcon(qsl(":/icons/package-exporter.png")));
    mpActionPackageExporter->setObjectName(qsl("package_exporter"));

    mpButtonPackageManagers->addAction(mpActionPackageManager);
    mpButtonPackageManagers->addAction(mpActionModuleManager);
    mpButtonPackageManagers->addAction(mpActionPackageExporter);
    mpButtonPackageManagers->setDefaultAction(mpActionPackageManager);

    mpActionReplay = new QAction(QIcon(qsl(":/icons/media-optical.png")), tr("Replay"), this);
    mpActionReplay->setObjectName(qsl("replay_action"));
    mpToolBar->addAction(mpActionReplay);

    // Standalone Reconnect action (like main window)
    mpActionReconnectStandalone = new QAction(QIcon(qsl(":/icons/system-restart.png")), tr("Reconnect"), this);
    mpActionReconnectStandalone->setToolTip(utils::richText(tr("Disconnects you from the game and connects once again")));
    mpActionReconnectStandalone->setObjectName(qsl("reconnect_standalone"));
    mpToolBar->addAction(mpActionReconnectStandalone);

    // About action (like main window)
    mpActionAbout = new QAction(QIcon(qsl(":/icons/mudlet_information.png")), tr("About"), this);
    mpActionAbout->setToolTip(utils::richText(tr("Inform yourself about this version of Mudlet, the people who made it and the licence under which you can share it.")));
    mpActionAbout->setObjectName(qsl("about_action"));
    mpToolBar->addAction(mpActionAbout);

    // Full screen toggle
    QIcon fullScreenIcon;
    fullScreenIcon.addPixmap(qsl(":/icons/view-fullscreen.png"), QIcon::Normal, QIcon::Off);
    fullScreenIcon.addPixmap(qsl(":/icons/view-restore.png"), QIcon::Normal, QIcon::On);
    mpActionFullScreenView = new QAction(fullScreenIcon, tr("Full Screen"), this);
    mpActionFullScreenView->setToolTip(utils::richText(tr("Toggle Full Screen View")));
    mpActionFullScreenView->setCheckable(true);
    mpActionFullScreenView->setObjectName(qsl("fullscreen_action"));
    mpToolBar->addAction(mpActionFullScreenView);

    // Setup context menu for the toolbar
    mpToolBar->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(mpToolBar, &QToolBar::customContextMenuRequested, this, &TDetachedWindow::slot_showDetachedToolBarContextMenu);

    // Connect all actions to their respective slots in the main mudlet instance
    connectToolBarActions();
}

void TDetachedWindow::connectToolBarActions()
{
    auto mudletInstance = mudlet::self();

    if (!mudletInstance) {
        return;
    }

    // Connection actions - use our custom slots to ensure correct profile context
    connect(mpActionConnect, &QAction::triggered, this, &TDetachedWindow::slot_connectProfile);
    connect(mpActionDisconnect, &QAction::triggered, this, &TDetachedWindow::slot_disconnectProfile);
    connect(mpActionCloseProfile, &QAction::triggered, this, &TDetachedWindow::slot_closeCurrentProfile);
    connect(mpActionCloseApplication, &QAction::triggered, this, &TDetachedWindow::slot_closeApplication);

    // Reattach action
    connect(mpActionReattach, &QAction::triggered, this, &TDetachedWindow::onReattachAction);

    // Script editor actions - use our custom slots to ensure correct profile context
    connect(mpActionTriggers, &QAction::triggered, this, &TDetachedWindow::slot_showTriggerDialog);
    connect(mpActionAliases, &QAction::triggered, this, &TDetachedWindow::slot_showAliasDialog);
    connect(mpActionTimers, &QAction::triggered, this, &TDetachedWindow::slot_showTimerDialog);
    connect(mpActionButtons, &QAction::triggered, this, &TDetachedWindow::slot_showActionDialog);
    connect(mpActionScripts, &QAction::triggered, this, &TDetachedWindow::slot_showScriptDialog);
    connect(mpActionKeys, &QAction::triggered, this, &TDetachedWindow::slot_showKeyDialog);
    connect(mpActionVariables, &QAction::triggered, this, &TDetachedWindow::slot_showVariableDialog);

    // Mute actions - use our custom slots to ensure correct profile context
    connect(mpActionMuteMedia, &QAction::triggered, this, &TDetachedWindow::slot_muteMedia);
    connect(mpActionMuteAPI, &QAction::triggered, this, &TDetachedWindow::slot_muteAPI);
    connect(mpActionMuteGame, &QAction::triggered, this, &TDetachedWindow::slot_muteGame);

    // Other tools - use our custom slots to ensure correct profile context
    connect(mpActionMapper, &QAction::triggered, this, &TDetachedWindow::slot_showMapperDialog);
    connect(mpActionHelp, &QAction::triggered, this, &TDetachedWindow::slot_showHelpDialog);
    connect(mpActionOptions, &QAction::triggered, this, &TDetachedWindow::slot_showPreferencesDialog);
    connect(mpActionNotes, &QAction::triggered, this, &TDetachedWindow::slot_showNotesDialog);

    // Package management - use our custom slots to ensure correct profile context
    connect(mpActionPackageManager, &QAction::triggered, this, &TDetachedWindow::slot_showPackageManagerDialog);
    connect(mpActionModuleManager, &QAction::triggered, this, &TDetachedWindow::slot_showModuleManagerDialog);
    connect(mpActionPackageExporter, &QAction::triggered, this, &TDetachedWindow::slot_showPackageExporterDialog);

    connect(mpActionReplay, &QAction::triggered, this, &TDetachedWindow::slot_showReplayDialog);
    connect(mpActionReconnectStandalone, &QAction::triggered, this, &TDetachedWindow::slot_reconnectProfile);
    connect(mpActionAbout, &QAction::triggered, this, &TDetachedWindow::slot_showAboutDialog);
    connect(mpActionFullScreenView, &QAction::triggered, this, &TDetachedWindow::slot_toggleFullScreenView);

    // Discord/IRC actions - use our custom slots to ensure correct profile context
    connect(mpActionDiscord, &QAction::triggered, this, &TDetachedWindow::slot_profileDiscord);
    connect(mpActionMudletDiscord, &QAction::triggered, this, &TDetachedWindow::slot_mudletDiscord);
    connect(mpActionIRC, &QAction::triggered, this, &TDetachedWindow::slot_irc);
}

void TDetachedWindow::updateToolBarActions()
{
    Host* pHost = nullptr;

    if (!mCurrentProfileName.isEmpty()) {
        pHost = mudlet::self()->getHostManager().getHost(mCurrentProfileName);
    }

    bool hasActiveProfile = (pHost != nullptr);

    // Enable/disable actions based on profile state
    mpActionTriggers->setEnabled(hasActiveProfile);
    mpActionAliases->setEnabled(hasActiveProfile);
    mpActionTimers->setEnabled(hasActiveProfile);
    mpActionButtons->setEnabled(hasActiveProfile);
    mpActionScripts->setEnabled(hasActiveProfile);
    mpActionKeys->setEnabled(hasActiveProfile);
    mpActionVariables->setEnabled(hasActiveProfile);
    mpActionMapper->setEnabled(hasActiveProfile);
    mpActionNotes->setEnabled(hasActiveProfile);
    mpActionOptions->setEnabled(hasActiveProfile);
    mpActionReplay->setEnabled(hasActiveProfile);
    mpActionPackageManager->setEnabled(hasActiveProfile);
    mpActionModuleManager->setEnabled(hasActiveProfile);
    mpActionPackageExporter->setEnabled(hasActiveProfile);

    // Connection-related actions
    if (hasActiveProfile) {
        // Enable/disable individual actions based on connection state
        // All actions should always be enabled to match main window behavior
        mpActionConnect->setEnabled(true);
        mpActionDisconnect->setEnabled(true);
        mpActionReconnectStandalone->setEnabled(true);
        mpActionCloseProfile->setEnabled(true);
        mpActionCloseApplication->setEnabled(true);
    } else {
        // Even when no profile is active, keep all actions enabled to match main window
        mpActionConnect->setEnabled(true);
        mpActionDisconnect->setEnabled(true);
        mpActionReconnectStandalone->setEnabled(true);
        mpActionCloseProfile->setEnabled(true);
        mpActionCloseApplication->setEnabled(true);
    }

    // Mute actions are always enabled
    mpActionMuteMedia->setEnabled(true);
    mpActionMuteAPI->setEnabled(true);
    mpActionMuteGame->setEnabled(true);

    // Help and About are always enabled
    mpActionHelp->setEnabled(true);
    mpActionAbout->setEnabled(true);

    // Reattach is always enabled when the window exists
    mpActionReattach->setEnabled(true);

    // Full screen toggle is always enabled
    mpActionFullScreenView->setEnabled(true);

    // Update Discord icon based on current profile
    updateDiscordNamedIcon();
}

void TDetachedWindow::updateDiscordNamedIcon()
{
    Host* pHost = nullptr;
    
    if (!mCurrentProfileName.isEmpty()) {
        pHost = mudlet::self()->getHostManager().getHost(mCurrentProfileName);
    }
    
    if (!pHost) {
        // No active host - reset Discord icon to default state
        mpActionDiscord->setIconText(qsl("Discord"));
        
        // Hide Mudlet Discord action as there's no game with custom invite
        if (mpActionMudletDiscord->isVisible()) {
            mpActionMudletDiscord->setVisible(false);
        }
        return;
    }

    const QString gameName = pHost->getDiscordGameName();
    const bool hasCustom = !pHost->getDiscordInviteURL().isEmpty();

    mpActionDiscord->setIconText(gameName.isEmpty() ? qsl("Discord") : QFontMetrics(mpActionDiscord->font()).elidedText(gameName, Qt::ElideRight, 90));

    // Show/hide Mudlet Discord action based on whether game has custom invite
    if (mpActionMudletDiscord->isVisible() != hasCustom) {
        mpActionMudletDiscord->setVisible(hasCustom);
    }
}

void TDetachedWindow::updateToolbarForProfile(Host* pHost)
{
    Q_UNUSED(pHost)
    // Update toolbar actions based on the provided host/profile
    // This method is called from mudlet's updateDetachedWindowToolbars()
    updateToolBarActions();
    updateWindowTitle();
    updateTabIndicator();
}

void TDetachedWindow::updateWindowTitle()
{
    QString title;

    if (mProfileConsoleMap.size() == 1) {
        // Single profile - use the simple format
        Host* pHost = nullptr;

        if (!mCurrentProfileName.isEmpty()) {
            pHost = mudlet::self()->getHostManager().getHost(mCurrentProfileName);
            //: This is the title of a Mudlet window which was detached from the main Mudlet window, and %1 is the name of the profile.
            title = tr("Mudlet - %1 (Detached)").arg(mCurrentProfileName);
        } else {
            //: This is the title of a Mudlet window which was detached from the main Mudlet window, but has no profile loaded.
            title = tr("Mudlet (Detached)");
        }

        if (pHost) {
            title += qsl(" - ");
            bool isConnected = (pHost->mTelnet.getConnectionState() == QAbstractSocket::ConnectedState);
            bool isConnecting = (pHost->mTelnet.getConnectionState() == QAbstractSocket::ConnectingState);

            if (isConnected) {
                if (!pHost->getUrl().isEmpty()) {
                    //: This text will be added to the title of a detached Mudlet window, if it is currently connected. The whole title will be like "Mudlet PROFILENAME (Detached) - Connected to GAMENAME"
                    title += tr("Connected to %1").arg(pHost->getUrl());
                } else {
                    //: This text will be part of to the title of a detached Mudlet window, if it is currently connected but we don't know to where. The whole title will be like "Mudlet PROFILENAME (Detached) - Connected"
                    title += tr("Connected");                    
                }
            } else if (isConnecting) {
                //: This text will be part of the title of a detached Mudlet window, if it is about to be connected. The whole title will be like "Mudlet PROFILENAME (Detached) - Connecting..."
                title += tr("Connecting...");
            } else {
                //: This text will be part of the title of a detached Mudlet window, if it is not connected. The whole title will be like "Mudlet PROFILENAME (Detached) - Disconnected"
                title += tr("Disconnected");
            }
        }
    } else {
        // Multiple profiles - show count and current
        //: This is the title of a Mudlet window which was detached from the main Mudlet window, and has multiple profiles opened in this window. %1 is the number of profiles, %2 is the name of the profile currently shown.
        title = tr("Mudlet (%1 profiles) - %2 (Detached)")
                .arg(mProfileConsoleMap.size())
                .arg(mCurrentProfileName);
    }

    setWindowTitle(title);
}

void TDetachedWindow::updateTabIndicator(int tabIndex)
{
    if (!mpTabBar || mpTabBar->count() == 0) {
        return;
    }

    // If no tab index specified, update current tab
    if (tabIndex == -1) {
        tabIndex = mpTabBar->currentIndex();
    }

    if (tabIndex < 0 || tabIndex >= mpTabBar->count()) {
        return;
    }

    QString profileName = mpTabBar->tabData(tabIndex).toString();

    if (profileName.isEmpty()) {
        return;
    }

    // Get the host and determine connection status
    Host* pHost = mudlet::self()->getHostManager().getHost(profileName);
    QIcon tabIcon;

    // Only show connection indicators if the global setting is enabled
    if (mudlet::self()->showTabConnectionIndicators()) {
        if (pHost) {
            bool isConnected = (pHost->mTelnet.getConnectionState() == QAbstractSocket::ConnectedState);
            bool isConnecting = (pHost->mTelnet.getConnectionState() == QAbstractSocket::ConnectingState);
            tabIcon = mudlet::createConnectionStatusIcon(isConnected, isConnecting, false);
        } else {
            tabIcon = mudlet::createConnectionStatusIcon(false, false, true);
        }
    } else {
        // No icon when indicators are disabled
        tabIcon = QIcon();
    }

    // Set the tab text and icon, accounting for CDC identifiers
    QString displayText = profileName;
    
    // Apply CDC identifier prefix if debug mode is active (like main window does)
    if (mudlet::smDebugMode && pHost) {
        QString debugTag = TDebug::getTag(pHost);

        if (!debugTag.isEmpty()) {
            displayText = debugTag + profileName;
        }
    }
    
    mpTabBar->setTabText(tabIndex, displayText);
    mpTabBar->setTabIcon(tabIndex, tabIcon);
}

void TDetachedWindow::updateAllTabIndicators()
{
    if (!mpTabBar) {
        return;
    }

    // Update all tabs in this detached window
    for (int i = 0; i < mpTabBar->count(); ++i) {
        updateTabIndicator(i);
    }
}

void TDetachedWindow::updateDockWidgetVisibilityForProfile(const QString& profileName)
{
    // Clear the global map dock widget reference first
    mpMapDockWidget = nullptr;
    
#if defined(DEBUG_WINDOW_HANDLING)
    qDebug() << "TDetachedWindow::updateDockWidgetVisibilityForProfile: Starting for profile" << profileName
             << "- mDockWidgetMap.size():" << mDockWidgetMap.size();

    // Track if we found and showed a dock widget for the current profile
    bool currentProfileHasVisibleDockWidget = false;
#endif
    
    // Collect dock widgets to process to avoid iterator invalidation
    QList<QPair<QString, QPointer<QDockWidget>>> dockWidgetsToProcess;

    for (auto it = mDockWidgetMap.begin(); it != mDockWidgetMap.end(); ++it) {
        if (it.value()) {
            dockWidgetsToProcess.append(qMakePair(it.key(), it.value()));
#if defined(DEBUG_WINDOW_HANDLING)
            qDebug() << "TDetachedWindow: Found dock widget in map:" << it.key() << "isVisible:" << it.value()->isVisible();
#endif
        } else {
#if defined(DEBUG_WINDOW_HANDLING)
            qDebug() << "TDetachedWindow: Found null dock widget in map for key:" << it.key();
#endif
        }
    }

    // Process dock widgets without iterating over the map directly
    for (const auto& dockPair : dockWidgetsToProcess) {
        const QString& dockKey = dockPair.first;
        QPointer<QDockWidget> dockWidget = dockPair.second;
        
        // Check if the dock widget still exists and is in our map
        if (!dockWidget || !mDockWidgetMap.contains(dockKey)) {
#if defined(DEBUG_WINDOW_HANDLING)
            qDebug() << "TDetachedWindow: Skipping dock widget" << dockKey << "- widget exists:" << (dockWidget != nullptr) 
                     << "in map:" << mDockWidgetMap.contains(dockKey);
#endif
            continue;
        }
        
        // Check if this docked widget belongs to the current profile
        if (dockKey.startsWith("map_")) {
            QString dockProfileName = dockKey.mid(4); // Remove "map_" prefix
#if defined(DEBUG_WINDOW_HANDLING)
            qDebug() << "TDetachedWindow: Processing map dock" << dockKey << "profile:" << dockProfileName << "current:" << profileName;
#endif
            
            if (dockProfileName == profileName) {
                // This dock widget belongs to the current profile
                // Check the user's preference for dock widget visibility
                bool shouldBeVisible = mDockWidgetUserPreference.value(dockKey, false);
#if defined(DEBUG_WINDOW_HANDLING)
                qDebug() << "TDetachedWindow: Found dock widget for current profile" << profileName 
                         << "currently visible:" << dockWidget->isVisible() 
                         << "should be visible:" << shouldBeVisible;
#endif
                
                // Show or hide based on intended visibility
                if (shouldBeVisible) {
                    dockWidget->show();
                    dockWidget->raise();
                    
                    // Set this as the global map dock widget reference
                    mpMapDockWidget = dockWidget;
#if defined(DEBUG_WINDOW_HANDLING)
                    currentProfileHasVisibleDockWidget = true;
                    qDebug() << "TDetachedWindow: Dock widget should be visible - showing and setting as active";
#endif
                } else {
                    // Block signals to prevent user preference from being updated by system-initiated change
                    dockWidget->blockSignals(true);
                    dockWidget->setVisible(false);
                    dockWidget->blockSignals(false);
#if defined(DEBUG_WINDOW_HANDLING)
                    qDebug() << "TDetachedWindow: Dock widget should be hidden - respecting user preference";
#endif
                }
                
                // Ensure the map's active mapper points to our detached instance (if visible)
                if (auto pHost = mudlet::self()->getHostManager().getHost(profileName)) {
                    if (auto pMap = pHost->mpMap.data()) {
                        auto mapWidget = dockWidget->widget();

                        if (auto detachedMapper = qobject_cast<dlgMapper*>(mapWidget)) {
                            // Only set as active mapper if the dock widget should be visible
                            if (shouldBeVisible) {
                                pMap->mpMapper = detachedMapper;
#if defined(DEBUG_WINDOW_HANDLING)
                                qDebug() << "TDetachedWindow: Set active mapper for profile" << profileName;
#endif
                            }
                        }
                    }
                }
            } else {
                // This dock widget belongs to a different profile - hide it
#if defined(DEBUG_WINDOW_HANDLING)
                qDebug() << "TDetachedWindow: Hiding dock widget for different profile" << dockProfileName;
#endif
                // Block signals to prevent user preference from being updated by system-initiated change
                dockWidget->blockSignals(true);
                dockWidget->setVisible(false);
                dockWidget->blockSignals(false);
                
                // Restore main mapper for the other profile
                if (auto pHost = mudlet::self()->getHostManager().getHost(dockProfileName)) {
                    if (auto pMap = pHost->mpMap.data()) {
                        if (pHost->mpDockableMapWidget) {
                            auto mainMapWidget = pHost->mpDockableMapWidget->widget();

                            if (auto mainMapper = qobject_cast<dlgMapper*>(mainMapWidget)) {
                                pMap->mpMapper = mainMapper;
#if defined(DEBUG_WINDOW_HANDLING)
                                qDebug() << "TDetachedWindow: Restored main mapper for profile" << dockProfileName;
#endif
                            }
                        }
                    }
                }
            }
        }
        // Add other dock widget types here as they are implemented
    }
    
    // Debug output to help track dock widget visibility changes
#if defined(DEBUG_WINDOW_HANDLING)
    qDebug() << "TDetachedWindow::updateDockWidgetVisibilityForProfile:" << profileName 
             << "- Found visible dock widget:" << currentProfileHasVisibleDockWidget
             << "- Total dock widgets:" << dockWidgetsToProcess.size()
             << "- mpMapDockWidget set:" << (mpMapDockWidget != nullptr);
#endif
}

void TDetachedWindow::slot_toggleFullScreenView()
{
    if (isFullScreen()) {
        showNormal();
        mpActionFullScreenView->setChecked(false);
    } else {
        showFullScreen();
        mpActionFullScreenView->setChecked(true);
    }
}

void TDetachedWindow::slot_toggleAlwaysOnTop()
{
    // Set flag to allow hiding during window flag changes
    mIsChangingWindowFlags = true;

    Qt::WindowFlags flags = windowFlags();

    if (flags & Qt::WindowStaysOnTopHint) {
        // Remove always on top flag
        setWindowFlags(flags & ~Qt::WindowStaysOnTopHint);
    } else {
        // Add always on top flag
        setWindowFlags(flags | Qt::WindowStaysOnTopHint);
    }

    show();  // Required after changing window flags

    // Reset flag after a short delay to allow the window operations to complete
    QTimer::singleShot(100, this, [this]() {
        mIsChangingWindowFlags = false;
    });
}

void TDetachedWindow::slot_toggleToolBarVisibility()
{
    if (!mpToolBar) {
        return;
    }

    bool newVisibility = !isToolBarVisible();
    
    // Synchronize toolbar visibility across all windows
    auto mudletInstance = mudlet::self();

    if (mudletInstance) {
        mudletInstance->synchronizeToolBarVisibility(newVisibility);
    }
}

void TDetachedWindow::slot_showDetachedToolBarContextMenu(const QPoint& position)
{
    if (!mpToolBar) {
        return;
    }

    QMenu menu(this);
    
    // Create "Profile Toolbar" toggle action
    QAction* toolbarToggleAction = menu.addAction(tr("Profile Toolbar"));
    toolbarToggleAction->setCheckable(true);
    toolbarToggleAction->setChecked(mpToolBar->isVisible());
    connect(toolbarToggleAction, &QAction::triggered, this, &TDetachedWindow::slot_toggleToolBarVisibility);
    
    // Show the context menu at the clicked position
    menu.exec(mpToolBar->mapToGlobal(position));
}

void TDetachedWindow::setToolBarVisibility(bool visible)
{
    if (mpToolBar) {
        mpToolBar->setVisible(visible);
    }

    if (mpActionToggleToolBar) {
        mpActionToggleToolBar->setChecked(visible);
    }
}

bool TDetachedWindow::isToolBarVisible() const
{
    return mpToolBar ? mpToolBar->isVisible() : false;
}

void TDetachedWindow::slot_saveProfile()
{
    if (mCurrentProfileName.isEmpty()) {
        return;
    }

    Host* pHost = mudlet::self()->getHostManager().getHost(mCurrentProfileName);

    if (pHost) {
        pHost->saveProfile();
    }
}

void TDetachedWindow::slot_exportProfile()
{
    // Use the main mudlet's package exporter functionality
    mudlet::self()->slot_packageExporter();
}

void TDetachedWindow::updateWindowMenu()
{
    // Don't update menu if the window is being destroyed
    if (mIsBeingDestroyed) {
        return;
    }
    
    // Clean up existing window list actions
    for (QAction* action : mWindowListActions) {
        mpWindowMenu->removeAction(action);
        action->deleteLater();
    }

    mWindowListActions.clear();

    // Remove separator if it exists
    if (mWindowListSeparator) {
        mpWindowMenu->removeAction(mWindowListSeparator);
        mWindowListSeparator->deleteLater();
        mWindowListSeparator = nullptr;
    }

    // Get the detached windows from mudlet
    const auto& detachedWindows = mudlet::self()->getDetachedWindows();
    
    // Count total windows (main + detached)
    int totalWindows = 1; // Main window
    totalWindows += detachedWindows.size();

    // Only show window list if there are multiple windows OR if there are multiple profiles
    bool hasMultipleProfiles = mudlet::self()->getHostManager().getHostCount() > 1;

    if (totalWindows > 1 || hasMultipleProfiles) {
        // Add separator before window list
        mWindowListSeparator = mpWindowMenu->addSeparator();

        // Add main window profiles
        QStringList mainWindowProfiles;

        for (const auto& host : mudlet::self()->getHostManager()) {
            if (host && host->mpConsole) {
                const QString profileName = host->getName();
                // Only include profiles that are in the main window (not detached)
                if (!detachedWindows.contains(profileName)) {
                    mainWindowProfiles.append(profileName);
                }
            }
        }

        // Add main window profiles
        if (!mainWindowProfiles.isEmpty()) {
            for (const QString& profileName : mainWindowProfiles) {

                //: This is an item in list of profiles in the "Window" menu of a detached Mudlet window. %1 is the name of the profile, and it is located not in the detached window, but in Mudlet's main window.
                QString actionText = tr("%1 (Main Window)").arg(profileName);
                QAction* profileAction = new QAction(actionText, this);
                profileAction->setCheckable(true);
                profileAction->setChecked(mudlet::self()->isActiveWindow() && 
                    mudlet::self()->getActiveHost() && 
                    mudlet::self()->getActiveHost()->getName() == profileName);
                profileAction->setData(profileName); // Store profile name for identification
                connect(profileAction, &QAction::triggered, this, &TDetachedWindow::slot_activateMainWindowProfile);
                mpWindowMenu->addAction(profileAction);
                mWindowListActions.append(profileAction);
            }
        }

        // Add detached window profiles
        // Collect unique detached windows to avoid duplicates
        QSet<TDetachedWindow*> uniqueDetachedWindows;

        for (auto it = detachedWindows.begin(); it != detachedWindows.end(); ++it) {
            TDetachedWindow* detachedWindow = it.value();

            if (detachedWindow) {
                uniqueDetachedWindows.insert(detachedWindow);
            }
        }
        
        // Process each unique detached window
        for (TDetachedWindow* detachedWindow : uniqueDetachedWindows) {
            // Get all profiles in this detached window
            QStringList profilesInWindow = detachedWindow->getProfileNames();

            for (const QString& windowProfileName : profilesInWindow) {
                //: This is an item in list of profiles in the "Window" menu of a detached Mudlet window. %1 is the name of the profile, and it is located not in Mudlet's main window, but in the detached window.
                QString actionText = tr("%1 (Detached)").arg(windowProfileName);
                QAction* profileAction = new QAction(actionText, this);
                profileAction->setCheckable(true);
                profileAction->setChecked(detachedWindow == this && detachedWindow->getCurrentProfileName() == windowProfileName);
                profileAction->setData(windowProfileName); // Store profile name for identification
                connect(profileAction, &QAction::triggered, this, &TDetachedWindow::slot_activateDetachedWindowProfile);
                mpWindowMenu->addAction(profileAction);
                mWindowListActions.append(profileAction);
            }
        }
    }
}

void TDetachedWindow::slot_activateMainWindow()
{
    mudlet* mainWindow = mudlet::self();

    if (mainWindow) {
        mainWindow->raise();
        mainWindow->activateWindow();
        mainWindow->show(); // Ensure it's not minimized
        updateWindowMenu(); // Refresh checkmarks
    }
}

void TDetachedWindow::slot_activateDetachedWindow()
{
    QAction* action = qobject_cast<QAction*>(sender());

    if (!action) {
        return;
    }

    QString profileName = action->data().toString();
    const auto& detachedWindows = mudlet::self()->getDetachedWindows();
    
    if (detachedWindows.contains(profileName)) {
        TDetachedWindow* detachedWindow = detachedWindows[profileName];

        if (detachedWindow) {
            detachedWindow->raise();
            detachedWindow->activateWindow();
            detachedWindow->show(); // Ensure it's not minimized
            updateWindowMenu(); // Refresh checkmarks
        }
    }
}

void TDetachedWindow::slot_activateMainWindowProfile()
{
    QAction* action = qobject_cast<QAction*>(sender());

    if (!action) {
        return;
    }

    QString profileName = action->data().toString();
    
    // Delegate to mudlet's main window profile activation
    mudlet* mainWindow = mudlet::self();

    if (mainWindow) {
        // Find the tab index for this profile in the main window
        int tabIndex = -1;

        for (int i = 0; i < mainWindow->mpTabBar->count(); ++i) {
            if (mainWindow->mpTabBar->tabData(i).toString() == profileName) {
                tabIndex = i;
                break;
            }
        }

        if (tabIndex >= 0) {
            // Activate the main window first
            mainWindow->raise();
            mainWindow->activateWindow();
            mainWindow->show(); // Ensure it's not minimized
            
            // Switch to the specific tab
            mainWindow->mpTabBar->setCurrentIndex(tabIndex);
            
            // Trigger the tab change logic to ensure the profile is properly activated
            mainWindow->slot_tabChanged(tabIndex);
            
            updateWindowMenu(); // Refresh checkmarks
        }
    }
}

void TDetachedWindow::slot_activateDetachedWindowProfile()
{
    QAction* action = qobject_cast<QAction*>(sender());

    if (!action) {
        return;
    }

    QString profileName = action->data().toString();
    
    // Find which detached window contains this profile
    const auto& detachedWindows = mudlet::self()->getDetachedWindows();

    for (auto it = detachedWindows.begin(); it != detachedWindows.end(); ++it) {
        TDetachedWindow* detachedWindow = it.value();

        if (detachedWindow && detachedWindow->getProfileNames().contains(profileName)) {
            // Activate the detached window
            detachedWindow->raise();
            detachedWindow->activateWindow();
            detachedWindow->show(); // Ensure it's not minimized
            
            // Switch to the specific profile tab in the detached window
            detachedWindow->switchToProfile(profileName);
            
            updateWindowMenu(); // Refresh checkmarks
            break;
        }
    }
}

void TDetachedWindow::closeProfile()
{
    if (mCurrentProfileName.isEmpty()) {
        return;
    }

    // Close the current profile
    closeProfileByIndex(mpTabBar->currentIndex());
}

bool TDetachedWindow::addProfile(const QString& profileName, TMainConsole* console)
{
    if (mProfileConsoleMap.contains(profileName) || !console) {
        return false;
    }

    // Add to our map
    mProfileConsoleMap[profileName] = console;

    // Ensure window should stay visible when it has profiles
    mShouldStayVisible = true;

    // Add tab
    int tabIndex = mpTabBar->addTab(profileName);
    mpTabBar->setTabData(tabIndex, profileName);

    // Add console to stacked widget
    mpConsoleContainer->addWidget(console);

    // Force console to properly resize and fill the available space
    console->setParent(mpConsoleContainer);
    console->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Clear any size constraints that might have been set in the previous window
    console->setMinimumSize(0, 0);
    console->setMaximumSize(16777215, 16777215);  // Qt's maximum size

    // Force immediate visibility and geometry updates
    console->setVisible(true);
    console->show();
    console->raise();

    // Multiple rounds of geometry updates to ensure proper sizing
    console->updateGeometry();
    console->adjustSize();

    // Force immediate redraw
    console->update();
    console->repaint();

    // Update the container to ensure proper layout
    mpConsoleContainer->updateGeometry();
    mpConsoleContainer->update();
    mpConsoleContainer->repaint();

    // Update tab indicator for the new tab
    updateTabIndicator(tabIndex);

    // Switch to the new profile
    mpTabBar->setCurrentIndex(tabIndex);

    // Explicitly switch to the new profile to ensure toolbar actions are updated
    switchToProfile(profileName);

    // Additional forced updates after switching to the profile
    console->updateGeometry();
    console->update();
    console->repaint();

    // Update the entire window layout
    updateGeometry();
    update();
    repaint();

    // Schedule a delayed update to handle any Qt layout timing issues
    QTimer::singleShot(10, this, [this, profileName]() {
        auto console = mProfileConsoleMap.value(profileName);

        if (console) {
            console->updateGeometry();
            console->adjustSize();
            console->update();
            console->repaint();
        }
    });

    return true;
}

bool TDetachedWindow::removeProfile(const QString& profileName)
{
    if (!mProfileConsoleMap.contains(profileName)) {
        return false;
    }

    // Clean up any dock widgets for this profile before removing it
    QString mapKey = qsl("map_%1").arg(profileName);

    if (mDockWidgetMap.contains(mapKey)) {
        QPointer<QDockWidget> mapDockWidget = mDockWidgetMap.value(mapKey);

        if (mapDockWidget) {
#if defined(DEBUG_WINDOW_HANDLING)
            qDebug() << "TDetachedWindow::removeProfile: Cleaning up dock widget for profile" << profileName;
#endif
            
            // If this is the currently active map dock, clear the global reference
            if (mpMapDockWidget == mapDockWidget) {
                mpMapDockWidget = nullptr;
            }
            
            // Remove the dock widget from Qt's dock widget management
            QMainWindow::removeDockWidget(mapDockWidget);
            
            // Restore the main window's mapper before deleting our dock widget
            if (auto pHost = mudlet::self()->getHostManager().getHost(profileName)) {
                if (auto pMap = pHost->mpMap.data()) {
                    if (pHost->mpDockableMapWidget) {
                        auto mainMapWidget = pHost->mpDockableMapWidget->widget();

                        if (auto mainMapper = qobject_cast<dlgMapper*>(mainMapWidget)) {
                            pMap->mpMapper = mainMapper;
#if defined(DEBUG_WINDOW_HANDLING)
                            qDebug() << "TDetachedWindow::removeProfile: Restored main mapper for profile" << profileName;
#endif
                        }
                    }
                }
            }
            
            // Clean up the dock widget
            mapDockWidget->deleteLater();
        }
        
        // Remove from our tracking maps
        mDockWidgetMap.remove(mapKey);
        mDockWidgetUserPreference.remove(mapKey);
    }

    // Find the tab index
    int tabIndex = -1;

    for (int i = 0; i < mpTabBar->count(); ++i) {
        if (mpTabBar->tabData(i).toString() == profileName) {
            tabIndex = i;
            break;
        }
    }

    if (tabIndex == -1) {
        return false;
    }

    // Remove console from stacked widget
    auto console = mProfileConsoleMap[profileName];

    if (console) {
        mpConsoleContainer->removeWidget(console);
        // DON'T set parent to nullptr here - let the receiving window handle reparenting
        // console->setParent(nullptr);
    }

    // Remove from map
    mProfileConsoleMap.remove(profileName);

    // Remember which tab was currently selected and adjust selection intelligently
    const int currentTabIndex = mpTabBar->currentIndex();
    const int tabCount = mpTabBar->count();
    int newSelectedIndex = -1;

    // Determine what tab should be selected after removal
    if (tabCount > 1) { // There will be tabs remaining after removal
        if (tabIndex == currentTabIndex) {
            // The current tab is being removed - select an adjacent tab
            if (tabIndex < tabCount - 1) {
                // Select the next tab (same index position)
                newSelectedIndex = tabIndex;
            } else {
                // We're removing the last tab, select the previous one
                newSelectedIndex = tabIndex - 1;
            }
        } else if (tabIndex < currentTabIndex) {
            // A tab before the current one is being removed - adjust current index
            newSelectedIndex = currentTabIndex - 1;
        } else {
            // A tab after the current one is being removed - keep current index
            newSelectedIndex = currentTabIndex;
        }
    }

    mpTabBar->removeTab(tabIndex);

    // Set the new selected tab if there are remaining tabs
    if (newSelectedIndex >= 0 && newSelectedIndex < mpTabBar->count()) {
        mpTabBar->setCurrentIndex(newSelectedIndex);
    }

    // Force tab bar to update and repaint
    mpTabBar->update();
    mpTabBar->repaint();

    // Update current profile if necessary
    if (mCurrentProfileName == profileName) {
        if (mpTabBar->count() > 0) {
            // Switch to the first remaining tab
            const QString newProfileName = mpTabBar->tabData(mpTabBar->currentIndex()).toString();

            if (!newProfileName.isEmpty()) {
                switchToProfile(newProfileName);
            }
        } else {
            mCurrentProfileName.clear();
            // Update window title when no profiles remain
            updateWindowTitle();
            // Allow window to be hidden since no profiles remain
            mShouldStayVisible = false;
        }
    }

    // Ensure the window remains visible if it still has profiles
    if (mpTabBar->count() > 0) {
        // Force layout update
        mpConsoleContainer->update();

        if (layout()) {
            layout()->update();
        }

        // Ensure window is not minimized
        if (isMinimized()) {
            showNormal();
        }

        // Make sure the window is visible and on top
        setVisible(true);
        show();
        raise();
        activateWindow();

        // Force window to front on macOS
#ifdef Q_OS_MACOS
        setAttribute(Qt::WA_ShowWithoutActivating, false);
#endif

        // Force repaint to ensure visibility
        repaint();

        // Additional forced update after event processing
        update();

        // Also schedule a delayed visibility restoration in case the drag operation
        // affects window state after this method returns
        QTimer::singleShot(100, this, [this, profileName]() {
            if (mpTabBar->count() > 0) {
                setVisible(true);
                show();
                raise();
                activateWindow();
                repaint();
            }
        });
    }

    return true;
}

QStringList TDetachedWindow::getProfileNames() const
{
    return mProfileConsoleMap.keys();
}

QString TDetachedWindow::getCurrentProfileName() const
{
    return mCurrentProfileName;
}

TMainConsole* TDetachedWindow::getCurrentConsole() const
{
    if (mCurrentProfileName.isEmpty()) {
        return nullptr;
    }

    return mProfileConsoleMap.value(mCurrentProfileName);
}

TMainConsole* TDetachedWindow::getConsole(const QString& profileName) const
{
    return mProfileConsoleMap.value(profileName);
}

void TDetachedWindow::switchToProfile(const QString& profileName)
{
    if (!mProfileConsoleMap.contains(profileName)) {
        return;
    }

    mCurrentProfileName = profileName;

    auto console = mProfileConsoleMap[profileName];

    if (console && mpConsoleContainer) {
        // Set the current widget in the stacked container
        mpConsoleContainer->setCurrentWidget(console);

        // Ensure console has proper size policy and constraints for the container
        console->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        console->setMinimumSize(0, 0);
        console->setMaximumSize(16777215, 16777215);  // Qt's maximum size

        // Force console visibility and proper sizing
        console->setVisible(true);
        console->show();
        console->raise();

        // Multiple rounds of geometry updates
        console->updateGeometry();
        console->adjustSize();

        // Force immediate redraw
        console->update();
        console->repaint();

        // Update container to ensure proper layout
        mpConsoleContainer->updateGeometry();
        mpConsoleContainer->update();
        mpConsoleContainer->repaint();

        // Force parent widget updates
        if (console->parentWidget()) {
            console->parentWidget()->updateGeometry();
            console->parentWidget()->update();
            console->parentWidget()->repaint();
        }
    }

    // Update tab selection to match the profile switch
    for (int i = 0; i < mpTabBar->count(); ++i) {
        if (mpTabBar->tabData(i).toString() == profileName) {
            if (mpTabBar->currentIndex() != i) {
                mpTabBar->setCurrentIndex(i);
            }
            break;
        }
    }

    // Force tab bar repainting
    mpTabBar->update();
    mpTabBar->repaint();

    // Update UI for the new active profile
    updateToolBarActions();
    updateWindowTitle();
    updateTabIndicator();
    
    // Update docked window visibility based on the new active profile
    updateDockWidgetVisibilityForProfile(profileName);

    // Force window repainting and layout update
    updateGeometry();
    update();
    repaint();

    // Schedule a delayed update to handle any Qt layout timing issues
    QTimer::singleShot(10, this, [this, profileName]() {
        auto console = mProfileConsoleMap.value(profileName);

        if (console) {
            console->updateGeometry();
            console->adjustSize();
            console->update();
            console->repaint();
        }
    });
    
    // Ensure the detached window itself gets focus and is brought to the front
    raise();
    activateWindow();
    show();
}

void TDetachedWindow::slot_tabChanged(int index)
{
    if (index < 0 || index >= mpTabBar->count()) {
        return;
    }

    QString profileName = mpTabBar->tabData(index).toString();

    if (!profileName.isEmpty() && profileName != mCurrentProfileName) {
        switchToProfile(profileName);
    }
}

void TDetachedWindow::slot_tabMoved(int oldPos, int newPos)
{
    // Get the current order of profile names from the tab bar
    const QStringList& profileNamesInOrder = mpTabBar->tabNames();
    const int itemsCount = mpConsoleContainer->count();
    
    // Validate that tab count matches console count
    if (itemsCount != profileNamesInOrder.count()) {
        qWarning().nospace().noquote() << "TDetachedWindow::slot_tabMoved(" << oldPos << ", " << newPos 
                                       << ") WARNING - mismatch in count of tabs (" << profileNamesInOrder.count() 
                                       << ") and TMainConsoles (" << itemsCount << ")";
        return;
    }

    // Create a map of profile names to console widgets
    QMap<QString, TMainConsole*> consoleWidgetMap;

    for (auto it = mProfileConsoleMap.begin(); it != mProfileConsoleMap.end(); ++it) {
        if (it.value()) {
            consoleWidgetMap.insert(it.key(), it.value());
        } else {
            qWarning().nospace().noquote() << "TDetachedWindow::slot_tabMoved(" << oldPos << ", " << newPos 
                                           << ") WARNING - nullptr for TMainConsole for profile: " << it.key();
        }
    }

    // Remember the currently active widget before reordering
    TMainConsole* currentActiveConsole = qobject_cast<TMainConsole*>(mpConsoleContainer->currentWidget());

    // Reorder the console widgets in the stacked widget to match the new tab order
    // Remove all widgets from the stacked widget and re-add them in the new order
    QList<TMainConsole*> widgetsToReAdd;

    for (const QString& profileName : profileNamesInOrder) {
        TMainConsole* console = consoleWidgetMap.value(profileName);

        if (console) {
            mpConsoleContainer->removeWidget(console);
            widgetsToReAdd.append(console);
        }
    }

    // Re-add the widgets in the new order
    for (TMainConsole* console : widgetsToReAdd) {
        mpConsoleContainer->addWidget(console);
    }

    // Restore the currently active widget
    if (currentActiveConsole && mpConsoleContainer->indexOf(currentActiveConsole) >= 0) {
        mpConsoleContainer->setCurrentWidget(currentActiveConsole);
    } else if (!mCurrentProfileName.isEmpty()) {
        // Fallback: use the current profile name to find the correct console
        TMainConsole* currentConsole = mProfileConsoleMap.value(mCurrentProfileName);

        if (currentConsole) {
            mpConsoleContainer->setCurrentWidget(currentConsole);
        }
    }
}

void TDetachedWindow::closeProfileByIndex(int index)
{
    if (index < 0 || index >= mpTabBar->count()) {
        return;
    }

    QString profileName = mpTabBar->tabData(index).toString();

    if (profileName.isEmpty()) {
        return;
    }

    // Close the specific profile
    mudlet::self()->slot_closeProfileByName(profileName);

    // Remove the profile from this detached window
    removeProfile(profileName);

    // If this was the last profile, close the detached window
    if (mProfileConsoleMap.isEmpty()) {
        // Signal that this window is closing to update multi-view controls
        // Since the profile map is now empty, closeEvent() won't emit windowClosed signals
        // so we need to notify the main window about the window closure here
        emit windowClosed(profileName);
        
        QTimer::singleShot(0, this, [this] {
            close();
        });
    }
}

void TDetachedWindow::checkForWindowMergeOpportunity()
{
    // Don't check for merges if we're in the process of closing or reattaching
    if (mIsReattaching || !isVisible() || !isActiveWindow()) {
        return;
    }

    // Don't check if we have no profiles
    if (mProfileConsoleMap.isEmpty()) {
        return;
    }

    // Get our window geometry
    const QRect ourRect = geometry();

    // Get list of all detached windows from mudlet
    auto mudletInstance = mudlet::self();

    if (!mudletInstance) {
        return;
    }

    // Look for overlapping detached windows
    const auto& detachedWindows = mudletInstance->getDetachedWindows();

    for (auto it = detachedWindows.begin(); it != detachedWindows.end(); ++it) {
        TDetachedWindow* otherWindow = it.value();

        // Skip ourselves
        if (otherWindow == this || !otherWindow || !otherWindow->isVisible()) {
            continue;
        }

        const QRect otherRect = otherWindow->geometry();

        // Check if windows significantly overlap (more than 50% of smaller window)
        const QRect intersection = ourRect.intersected(otherRect);

        if (intersection.isEmpty()) {
            continue;
        }

        const int ourArea = ourRect.width() * ourRect.height();
        const int otherArea = otherRect.width() * otherRect.height();
        const int intersectionArea = intersection.width() * intersection.height();
        const int smallerArea = qMin(ourArea, otherArea);

        // If overlap is significant (more than 60% of the smaller window)
        if (intersectionArea > (smallerArea * 0.6)) {
            // Automatically merge windows
            performWindowMerge(otherWindow);
            break; // Only merge with one window at a time
        }
    }
}

void TDetachedWindow::performWindowMerge(TDetachedWindow* otherWindow)
{
    if (!otherWindow || otherWindow == this || mIsReattaching) {
        return;
    }

    // Additional safety checks
    if (!isVisible() || !otherWindow->isVisible()) {
        return;
    }

    // Prevent multiple merge operations for the same windows
    static QSet<QPair<TDetachedWindow*, TDetachedWindow*>> activeMergeOperations;
    const auto mergePair = qMakePair(this, otherWindow);
    const auto reverseMergePair = qMakePair(otherWindow, this);

    if (activeMergeOperations.contains(mergePair) || activeMergeOperations.contains(reverseMergePair)) {
        return;
    }

    activeMergeOperations.insert(mergePair);

    // Get profile names for the merge
    const QStringList ourProfiles = getProfileNames();

    if (ourProfiles.isEmpty()) {
        activeMergeOperations.remove(mergePair);
        return;
    }

    // Automatically merge without prompting - defer the operation to avoid timing issues
    QTimer::singleShot(0, this, [this, otherWindow, ourProfiles, mergePair]() {
        // Check if the other window is still valid
        if (!otherWindow) {
            return;
        }

        // Set flag to prevent cleanup during merge
        mIsReattaching = true;

        // Merge all our profiles into the other window
        for (const QString& profileName : ourProfiles) {
            // Double-check the profile still exists in this window
            if (mProfileConsoleMap.contains(profileName)) {
                emit profileDetachToWindowRequested(profileName, otherWindow);
            }
        }
    });

    // Clean up the active merge tracking after a delay
    QTimer::singleShot(100, [mergePair]() {
        activeMergeOperations.remove(mergePair);
    });
}

void TDetachedWindow::logWindowState(const QString& context)
{
    Q_UNUSED(context)
    // Simplified debug output for critical state information only
    if (mShouldStayVisible && mpTabBar && mpTabBar->count() > 0 && !isVisible()) {
#if defined(DEBUG_WINDOW_HANDLING)
        qDebug() << "TDetachedWindow [" << context << "] WARNING: Window should stay visible but is hidden - profiles:" << getProfileCount();
#endif
    }
}

// Helper method to temporarily set the active host for actions
void TDetachedWindow::withCurrentProfileActive(const std::function<void()>& action)
{
    auto mudletInstance = mudlet::self();

    if (!mudletInstance || mCurrentProfileName.isEmpty()) {
        return;
    }

    Host* pHost = mudletInstance->getHostManager().getHost(mCurrentProfileName);

    if (!pHost) {
        return;
    }

    // Store the current active host
    Host* previousActiveHost = mudletInstance->mpCurrentActiveHost;

    // Temporarily set our profile's host as active
    mudletInstance->mpCurrentActiveHost = pHost;

    // Execute the action
    action();

    // Restore the previous active host
    mudletInstance->mpCurrentActiveHost = previousActiveHost;
}

// Helper method for script editor dialogs to reduce code duplication
void TDetachedWindow::showScriptEditorDialog(std::function<void(dlgTriggerEditor*)> showMethod)
{
    // Store the originating profile for focus restoration
    QString originatingProfile = mCurrentProfileName;
    
    withCurrentProfileActive([this, originatingProfile, showMethod]() {
        auto mudletInstance = mudlet::self();
        if (!mudletInstance) {
            return;
        }
        
        Host* pHost = mudletInstance->getActiveHost();
        if (!pHost) {
            return;
        }
        
        // Create or get the editor directly, avoiding the main window's focus restoration logic
        dlgTriggerEditor* pEditor = nullptr;
        if (pHost->mpEditorDialog != nullptr) {
            pEditor = pHost->mpEditorDialog;
        } else {
            // Create a new editor directly without using the main window's method
            pEditor = new dlgTriggerEditor(pHost);
            pHost->mpEditorDialog = pEditor;
            connect(pHost, &Host::profileSaveStarted, pHost->mpEditorDialog, &dlgTriggerEditor::slot_profileSaveStarted);
            connect(pHost, &Host::profileSaveFinished, pHost->mpEditorDialog, &dlgTriggerEditor::slot_profileSaveFinished);
            pEditor->fillout_form();
        }
        
        if (!pEditor) {
            return;
        }
        
        // Use centralized focus restoration with this detached window as target
        mudlet::setupEditorFocusRestoration(pEditor, originatingProfile, this);
        
        // Call the specific show method (slot_showAliases, slot_showTimers, etc.)
        if (showMethod) {
            showMethod(pEditor);
        }
        
        // Position dialog on the same screen as this detached window
        utils::positionDialogOnParentScreen(pEditor, this);
        
        // Show and activate the editor
        pEditor->raise();
        pEditor->showNormal();
        pEditor->activateWindow();
    });
}

// Detached window specific toolbar action slots
void TDetachedWindow::slot_connectProfile()
{
    withCurrentProfileActive([this]() {
        mudlet::self()->slot_showConnectionDialog();
    });
}

void TDetachedWindow::slot_disconnectProfile()
{
    withCurrentProfileActive([this]() {
        mudlet::self()->slot_disconnect();
    });
}

void TDetachedWindow::slot_reconnectProfile()
{
    withCurrentProfileActive([this]() {
        mudlet::self()->slot_reconnect();
    });
}

void TDetachedWindow::slot_closeCurrentProfile()
{
    // Use the detached window's own close profile logic instead of 
    // delegating to the main window, which would use the wrong tab bar
    closeProfile();
}

void TDetachedWindow::slot_closeApplication()
{
    withCurrentProfileActive([this]() {
        mudlet::self()->close();
    });
}

void TDetachedWindow::slot_showTriggerDialog()
{
    showScriptEditorDialog([](dlgTriggerEditor* pEditor) {
        pEditor->slot_showTriggers();
    });
}

void TDetachedWindow::slot_showAliasDialog()
{
    showScriptEditorDialog([](dlgTriggerEditor* pEditor) {
        pEditor->slot_showAliases();
    });
}

void TDetachedWindow::slot_showTimerDialog()
{
    showScriptEditorDialog([](dlgTriggerEditor* pEditor) {
        pEditor->slot_showTimers();
    });
}

void TDetachedWindow::slot_showActionDialog()
{
    showScriptEditorDialog([](dlgTriggerEditor* pEditor) {
        pEditor->slot_showActions();
    });
}

void TDetachedWindow::slot_showScriptDialog()
{
    showScriptEditorDialog([](dlgTriggerEditor* pEditor) {
        pEditor->slot_showScripts();
    });
}

void TDetachedWindow::slot_showKeyDialog()
{
    showScriptEditorDialog([](dlgTriggerEditor* pEditor) {
        pEditor->slot_showKeys();
    });
}

void TDetachedWindow::slot_showVariableDialog()
{
    showScriptEditorDialog([](dlgTriggerEditor* pEditor) {
        pEditor->slot_showVariables();
    });
}

void TDetachedWindow::slot_showMapperDialog()
{
    if (mCurrentProfileName.isEmpty()) {
        return;
    }

    Host* pHost = mudlet::self()->getHostManager().getHost(mCurrentProfileName);
    if (!pHost) {
        return;
    }

    auto pMap = pHost->mpMap.data();
    if (!pMap) {
        return;
    }

    // Close any existing map for this profile in other windows first
    auto mudletInstance = mudlet::self();
    QString mapKey = qsl("map_%1").arg(mCurrentProfileName);
    
    // Check if main window has a map for this profile and close it
    auto mainMapDock = mudletInstance->getMainWindowDockWidget(mapKey);

    if (mainMapDock && mainMapDock->isVisible()) {
#if defined(DEBUG_WINDOW_HANDLING)
        qDebug() << "TDetachedWindow: Closing main window map for profile" << mCurrentProfileName << "to prevent conflicts";
#endif
        // Block signals to prevent user preference from being updated by system-initiated change
        mainMapDock->blockSignals(true);
        mainMapDock->setVisible(false);
        mainMapDock->blockSignals(false);
    }
    
    // Check other detached windows for conflicting maps
    const auto& detachedWindows = mudletInstance->getDetachedWindows();

    for (auto it = detachedWindows.begin(); it != detachedWindows.end(); ++it) {
        TDetachedWindow* otherWindow = it.value();

        if (otherWindow && otherWindow != this) {
            auto otherMapDock = otherWindow->getDockWidget(mapKey);

            if (otherMapDock && otherMapDock->isVisible()) {
#if defined(DEBUG_WINDOW_HANDLING)
                qDebug() << "TDetachedWindow: Closing map in other detached window for profile" << mCurrentProfileName;
#endif
                // Block signals to prevent user preference from being updated by system-initiated change
                otherMapDock->blockSignals(true);
                otherMapDock->setVisible(false);
                otherMapDock->blockSignals(false);
            }
        }
    }

    // Check if we already have a mapper dock widget for this profile
    QPointer<QDockWidget> existingMapDock = mDockWidgetMap.value(mapKey);
    
    if (existingMapDock) {
        // Toggle visibility of existing mapper and update global reference
        bool newVisibility = !existingMapDock->isVisible();
        existingMapDock->setVisible(newVisibility);
        
        // Update mpMapDockWidget to point to the current profile's map if it's being shown
        if (newVisibility) {
            mpMapDockWidget = existingMapDock;
            
            // Ensure the map's active mapper points to our detached instance
            auto mapWidget = existingMapDock->widget();
            if (auto detachedMapper = qobject_cast<dlgMapper*>(mapWidget)) {
                pMap->mpMapper = detachedMapper;
            }
        } else if (mpMapDockWidget == existingMapDock) {
            // If we're hiding the current map, clear the global reference and restore main mapper
            mpMapDockWidget = nullptr;
            
            // Restore the main window's mapper as the active one
            if (pHost->mpDockableMapWidget) {
                auto mainMapWidget = pHost->mpDockableMapWidget->widget();
                if (auto mainMapper = qobject_cast<dlgMapper*>(mainMapWidget)) {
                    pMap->mpMapper = mainMapper;
                }
            }
        }
        return;
    }

    // Create a new docked mapper widget for this profile
    //: This is to create a new docked mapper widget for a profile in a detached Mudlet window. %1 is the name of the profile.
    auto newMapDockWidget = new QDockWidget(tr("Map - %1").arg(mCurrentProfileName), this);
    newMapDockWidget->setObjectName(qsl("dockMap_%1_detached").arg(mCurrentProfileName));
    
    // Store the main window's mapper temporarily so we can restore it later
    QPointer<dlgMapper> mainMapper = pMap->mpMapper;
    QPointer<QDockWidget> mainDockWidget = pHost->mpDockableMapWidget;
    
    // Create a new mapper instance for the detached window
    // We need to copy player room style details first
    pHost->getPlayerRoomStyleDetails(pMap->mPlayerRoomStyle,
                                   pMap->mPlayerRoomOuterDiameterPercentage,
                                   pMap->mPlayerRoomInnerDiameterPercentage,
                                   pMap->mPlayerRoomOuterColor,
                                   pMap->mPlayerRoomInnerColor);

    // Create the mapper dialog
    auto detachedMapper = new dlgMapper(newMapDockWidget, pHost, pMap);
    detachedMapper->setStyleSheet(pHost->mProfileStyleSheet);
    newMapDockWidget->setWidget(detachedMapper);

    // CRITICAL: Set the map's active mapper to our detached instance
    // This ensures map updates go to our detached window instead of the main window
    pMap->mpMapper = detachedMapper;

    // Initialize the mapper
    if (pMap->mpRoomDB && !pMap->mpRoomDB->isEmpty()) {
        detachedMapper->mp2dMap->init();
        detachedMapper->updateAreaComboBox();
        detachedMapper->resetAreaComboBoxToPlayerRoomArea();
        detachedMapper->show();
    }

    // Add the dock widget to this detached window
    QMainWindow::addDockWidget(Qt::RightDockWidgetArea, newMapDockWidget);
    
    // Store reference in our map for cleanup and profile-specific access
    mDockWidgetMap[mapKey] = newMapDockWidget;
    
    // Set user preference to true since we're initially showing this dock widget
    mDockWidgetUserPreference[mapKey] = true;
    
    // Set global reference to the currently active map
    mpMapDockWidget = newMapDockWidget;

    // Connect to handle dock widget visibility changes
    connect(newMapDockWidget, &QDockWidget::visibilityChanged, this, [this, mapKey](bool visible) {
        auto mapDockWidget = mDockWidgetMap.value(mapKey);
        if (!mapDockWidget) {
            return;
        }
        
        // Track user-initiated visibility changes - always update user preference
        // to ensure dock widget state is properly tracked regardless of which profile is active
        mDockWidgetUserPreference[mapKey] = visible;
#if defined(DEBUG_WINDOW_HANDLING)
        qDebug() << "TDetachedWindow: User changed dock widget visibility for" << mapKey << "to" << visible;
#endif
        
        // Extract profile name from mapKey to safely look up objects
        QString profileName = mapKey;
        if (profileName.startsWith("map_")) {
            profileName = profileName.mid(4); // Remove "map_" prefix
        }
        
        // Safely get the host and map - they might be null during shutdown
        auto mudletInstance = mudlet::self();
        if (!mudletInstance) {
            return;
        }
        
        Host* pHost = mudletInstance->getHostManager().getHost(profileName);
        if (!pHost) {
            return;
        }
        
        auto pMap = pHost->mpMap.data();
        if (!pMap) {
            return;
        }
        
        if (!visible) {
            // If this is the currently active map dock, clear the global reference
            if (mpMapDockWidget == mapDockWidget) {
                mpMapDockWidget = nullptr;
            }
            
            // Restore the main window's mapper as the active one when hiding
            if (pHost->mpDockableMapWidget) {
                auto mainMapWidget = pHost->mpDockableMapWidget->widget();
                if (auto mainMapper = qobject_cast<dlgMapper*>(mainMapWidget)) {
                    pMap->mpMapper = mainMapper;
                }
            }
        } else {
            // When showing, set this as the active mapper
            mpMapDockWidget = mapDockWidget;
            
            // Ensure the map's active mapper points to our detached instance
            auto mapWidget = mapDockWidget->widget();
            if (auto detachedMapper = qobject_cast<dlgMapper*>(mapWidget)) {
                pMap->mpMapper = detachedMapper;
            }
        }
        
#if defined(DEBUG_WINDOW_HANDLING)
        qDebug() << "TDetachedWindow: Map dock visibility changed for" << mapKey << "visible:" << visible;
#endif
    });

    // Show the dock widget
    newMapDockWidget->show();
}

void TDetachedWindow::slot_showHelpDialog()
{
    withCurrentProfileActive([this]() {
        mudlet::self()->slot_showHelpDialog();
    });
}

void TDetachedWindow::slot_showPreferencesDialog()
{
    withCurrentProfileActive([this]() {
        // Store originating profile for focus restoration
        QString originatingProfile = mCurrentProfileName;
        
        auto mudletInstance = mudlet::self();
        if (!mudletInstance) {
            return;
        }
        
        Host* pHost = mudletInstance->getActiveHost();
        
        // Open preferences dialog
        mudletInstance->slot_showPreferencesDialog();
        
        // Position the preferences dialog on the same screen as this detached window
        auto pPrefs = pHost ? pHost->mpDlgProfilePreferences : mudletInstance->mpDlgProfilePreferences;
        if (pPrefs) {
            utils::positionDialogOnParentScreen(pPrefs, this);
            
            // Set up focus restoration for the preferences dialog to return to this detached window
            mudletInstance->setupPreferencesFocusRestoration(pPrefs);
        }
    });
}

void TDetachedWindow::slot_showNotesDialog()
{
    withCurrentProfileActive([this]() {
        // Store originating profile for focus restoration
        QString originatingProfile = mCurrentProfileName;
        
        auto mudletInstance = mudlet::self();
        if (!mudletInstance) {
            return;
        }
        
        Host* pHost = mudletInstance->getActiveHost();
        if (!pHost) {
            return;
        }
        
        // Open notes dialog
        mudletInstance->slot_notes();
        
        // Position the notes dialog on the same screen as this detached window
        if (pHost->mpNotePad) {
            utils::positionDialogOnParentScreen(pHost->mpNotePad, this);
            
            // Set up focus restoration for the notepad to return to this detached window
            mudletInstance->setupNotepadFocusRestoration(pHost->mpNotePad);
        }
    });
}

void TDetachedWindow::slot_showReplayDialog()
{
    withCurrentProfileActive([this]() {
        mudlet::self()->slot_replay();
    });
}

void TDetachedWindow::slot_showPackageManagerDialog()
{
    withCurrentProfileActive([this]() {
        // Store originating profile for focus restoration
        QString originatingProfile = mCurrentProfileName;
        
        auto mudletInstance = mudlet::self();
        if (!mudletInstance) {
            return;
        }
        
        Host* pHost = mudletInstance->getActiveHost();
        if (!pHost) {
            return;
        }
        
        // Open package manager dialog
        mudletInstance->slot_packageManager();
        
        // Position the package manager dialog on the same screen as this detached window
        if (pHost->mpPackageManager) {
            utils::positionDialogOnParentScreen(pHost->mpPackageManager, this);
            
            // Set up focus restoration for the package manager to return to this detached window
            mudletInstance->setupPackageManagerFocusRestoration(pHost->mpPackageManager);
        }
    });
}

void TDetachedWindow::slot_showModuleManagerDialog()
{
    withCurrentProfileActive([this]() {
        // Store originating profile for focus restoration
        QString originatingProfile = mCurrentProfileName;
        
        auto mudletInstance = mudlet::self();
        if (!mudletInstance) {
            return;
        }
        
        Host* pHost = mudletInstance->getActiveHost();
        if (!pHost) {
            return;
        }
        
        // Open module manager dialog
        mudletInstance->slot_moduleManager();
        
        // Position the module manager dialog on the same screen as this detached window
        if (pHost->mpModuleManager) {
            utils::positionDialogOnParentScreen(pHost->mpModuleManager, this);
            
            // Set up focus restoration for the module manager to return to this detached window
            mudletInstance->setupModuleManagerFocusRestoration(pHost->mpModuleManager);
        }
    });
}

void TDetachedWindow::slot_showPackageExporterDialog()
{
    withCurrentProfileActive([this]() {
        mudlet::self()->slot_packageExporter();
    });
}

void TDetachedWindow::slot_showConnectionDialog()
{
    withCurrentProfileActive([this]() {
        mudlet::self()->slot_showConnectionDialog();
    });
}

void TDetachedWindow::slot_showEditorDialog()
{
    showScriptEditorDialog(nullptr); // No specific method to call, just show the general editor
}

void TDetachedWindow::slot_showAboutDialog()
{
    withCurrentProfileActive([this]() {
        mudlet::self()->slot_showAboutDialog();
    });
}

void TDetachedWindow::slot_reportIssue()
{
    withCurrentProfileActive([this]() {
        // Access the private method via QDesktopServices to open a URL
        QDesktopServices::openUrl(QUrl("https://github.com/Mudlet/Mudlet/issues"));
    });
}

void TDetachedWindow::slot_profileDiscord()
{
    auto mudletInstance = mudlet::self();
    if (!mudletInstance || mCurrentProfileName.isEmpty()) {
        return;
    }
    
    Host* pHost = mudletInstance->getHostManager().getHost(mCurrentProfileName);
    QString invite;
    if (pHost) {
        invite = pHost->getDiscordInviteURL();
    }
    
    if (invite.isEmpty()) {
        // Fall back to Mudlet Discord - call the slot directly without profile switching
        mudletInstance->slot_mudletDiscord();
    } else {
        mudletInstance->openWebPage(invite);
    }
}

void TDetachedWindow::slot_mudletDiscord()
{
    auto mudletInstance = mudlet::self();
    if (mudletInstance) {
        mudletInstance->slot_mudletDiscord();
    }
}

void TDetachedWindow::slot_irc()
{
    mudlet::self()->slot_irc();
}

void TDetachedWindow::slot_muteMedia()
{
    withCurrentProfileActive([this]() {
        mudlet::self()->slot_muteMedia();
    });
}

void TDetachedWindow::slot_muteAPI()
{
    withCurrentProfileActive([this]() {
        // Toggle the current API mute state
        bool currentState = mudlet::self()->muteAPI();
        mudlet::self()->slot_muteAPI(!currentState);
    });
}

void TDetachedWindow::slot_muteGame()
{
    withCurrentProfileActive([this]() {
        // Toggle the current game mute state
        bool currentState = mudlet::self()->muteGame();
        mudlet::self()->slot_muteGame(!currentState);
    });
}

void TDetachedWindow::changeEvent(QEvent* event)
{
    // Don't process events if the window is being destroyed
    if (mIsBeingDestroyed) {
        QMainWindow::changeEvent(event);
        return;
    }
    
    if (event->type() == QEvent::WindowStateChange) {
        // Check if window is being minimized
        if (windowState() & Qt::WindowMinimized) {
            mIsBeingMinimized = true;
            qDebug() << "TDetachedWindow::changeEvent: Window is being minimized";
        } else {
            // Reset the flag when window is no longer minimized
            if (mIsBeingMinimized && !(windowState() & Qt::WindowMinimized)) {
                mIsBeingMinimized = false;
                qDebug() << "TDetachedWindow::changeEvent: Window is no longer minimized";
            }
        }
    } else if (event->type() == QEvent::ActivationChange) {
        // Update window menu when window activation changes
        updateWindowMenu();
    }

    QMainWindow::changeEvent(event);
}

void TDetachedWindow::refreshTabBar()
{
    // Update all tab texts to account for CDC identifiers (like main window does)
    for (int i = 0; i < mpTabBar->count(); ++i) {
        QString profileName = mpTabBar->tabData(i).toString();
        if (!profileName.isEmpty()) {
            QString displayText = profileName;
            
            // Apply CDC identifier prefix if debug mode is active
            if (mudlet::smDebugMode) {
                Host* pHost = mudlet::self()->getHostManager().getHost(profileName);
                if (pHost) {
                    QString debugTag = TDebug::getTag(pHost);
                    if (!debugTag.isEmpty()) {
                        displayText = debugTag + profileName;
                    }
                }
            }
            
            mpTabBar->setTabText(i, displayText);
        }
    }
}

void TDetachedWindow::slot_closeAllProfiles()
{
    // Properly close all profiles before closing the window
    // This ensures save prompts and proper cleanup
    QStringList profilesToClose = mProfileConsoleMap.keys();
    
    qDebug() << "TDetachedWindow::slot_closeAllProfiles() - Closing" << profilesToClose.size() << "profiles";
    
    for (const QString& profileName : profilesToClose) {
        qDebug() << "TDetachedWindow::slot_closeAllProfiles() - Closing profile:" << profileName;
        mudlet::self()->slot_closeProfileByName(profileName);
    }
    
    // After all profiles are closed, close the window
    // The profiles should already be removed from mProfileConsoleMap by now,
    // but closeEvent() will handle any remaining cleanup
    close();
}

void TDetachedWindow::addTransferredDockWidget(const QString& mapKey, QDockWidget* dockWidget)
{
    // Add to the map
    mDockWidgetMap[mapKey] = dockWidget;
    
    // Set up signal connection for visibility changes - similar to what's done for new dock widgets
    connect(dockWidget, &QDockWidget::visibilityChanged, this, [this, mapKey](bool visible) {
        auto mapDockWidget = mDockWidgetMap.value(mapKey);
        if (!mapDockWidget) {
            return;
        }
        
        // Track user-initiated visibility changes - always update user preference
        // to ensure dock widget state is properly tracked regardless of which profile is active
        mDockWidgetUserPreference[mapKey] = visible;
#if defined(DEBUG_WINDOW_HANDLING)
        qDebug() << "TDetachedWindow: User changed dock widget visibility for" << mapKey << "to" << visible;
#endif
        
        // Extract profile name from mapKey to safely look up objects
        QString profileName = mapKey;

        if (profileName.startsWith("map_")) {
            profileName = profileName.mid(4); // Remove "map_" prefix
        }
        
        // Safely get the host and map - they might be null during shutdown
        auto mudletInstance = mudlet::self();

        if (!mudletInstance) {
            return;
        }
        
        Host* pHost = mudletInstance->getHostManager().getHost(profileName);

        if (!pHost) {
            return;
        }
        
        auto pMap = pHost->mpMap.data();

        if (!pMap) {
            return;
        }
        
        if (!visible) {
            // If this is the currently active map dock, clear the global reference
            if (mpMapDockWidget == mapDockWidget) {
                mpMapDockWidget = nullptr;
            }
            
            // Restore the main window's mapper as the active one when hiding
            if (pHost->mpDockableMapWidget) {
                auto mainMapWidget = pHost->mpDockableMapWidget->widget();

                if (auto mainMapper = qobject_cast<dlgMapper*>(mainMapWidget)) {
                    pMap->mpMapper = mainMapper;
                }
            }
        } else {
            // When showing, set this as the active mapper
            mpMapDockWidget = mapDockWidget;
            
            // Ensure the map's active mapper points to our detached instance
            auto mapWidget = mapDockWidget->widget();

            if (auto detachedMapper = qobject_cast<dlgMapper*>(mapWidget)) {
                pMap->mpMapper = detachedMapper;
            }
        }
        
        // Update visibility for the current profile
        updateDockWidgetVisibilityForProfile(profileName);
    });
}

// Additional slot implementations for new menu actions
void TDetachedWindow::slot_toggleMap()
{
    withCurrentProfileActive([this]() {
        mudlet::self()->slot_showMapperDialog();
    });
}

void TDetachedWindow::slot_toggleCompactInputLine()
{
    if (mCurrentProfileName.isEmpty()) {
        return;
    }
    
    auto mudletInstance = mudlet::self();
    if (!mudletInstance) {
        return;
    }
    
    auto host = mudletInstance->getHostManager().getHost(mCurrentProfileName);
    if (!host) {
        return;
    }
    
    // Toggle the compact input line state for the current profile
    bool currentState = host->getCompactInputLine();
    host->setCompactInputLine(!currentState);
}

void TDetachedWindow::slot_toggleReplay()
{
    withCurrentProfileActive([this]() {
        mudlet::self()->slot_toggleReplay();
    });
}

void TDetachedWindow::slot_toggleLogging()
{
    withCurrentProfileActive([this]() {
        mudlet::self()->slot_toggleLogging();
    });
}

void TDetachedWindow::slot_toggleEmergencyStop()
{
    withCurrentProfileActive([this]() {
        mudlet::self()->slot_toggleEmergencyStop();
    });
}

void TDetachedWindow::slot_toggleTimeStamp()
{
    withCurrentProfileActive([this]() {
        mudlet::self()->slot_toggleTimeStamp();
    });
}

void TDetachedWindow::slot_toggleMultiView()
{
    withCurrentProfileActive([this]() {
        mudlet::self()->slot_toggleMultiView();
    });
}

void TDetachedWindow::slot_toggleFullScreen()
{
    // Toggle fullscreen state for this detached window
    if (isFullScreen()) {
        showNormal();
    } else {
        showFullScreen();
    }
}
