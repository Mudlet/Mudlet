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
#include "Host.h"
#include "HostManager.h"
#include "mudlet.h"
#include "utils.h"
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
#include <QStatusBar>
#include <QStackedWidget>
#include <QSizePolicy>
#include <QWidget>
#include <QWindowStateChangeEvent>

TDetachedWindow::TDetachedWindow(const QString& profileName, TMainConsole* console, QWidget* parent)
    : QMainWindow(parent)
    , mCurrentProfileName(profileName)
{
    // Add the initial profile
    mProfileConsoleMap[profileName] = console;
    
    setupUI();
    createToolBar();
    createMenus();
    createStatusBar();
    restoreWindowGeometry();
    
    // Set window properties
    setWindowTitle(tr("Mudlet - %1 (Detached)").arg(profileName));
    setWindowIcon(parent ? parent->windowIcon() : QIcon());
    setAttribute(Qt::WA_DeleteOnClose);
    setAcceptDrops(true);
    
    // Update toolbar state and window title for this profile
    updateToolBarActions();
    updateWindowTitle();
}

TDetachedWindow::~TDetachedWindow()
{
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
    }
    
    saveWindowGeometry();
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
    
    // Add initial tab for the first profile
    if (!mCurrentProfileName.isEmpty()) {
        mpTabBar->addTab(mCurrentProfileName);
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
    
    // Connect double-click to reattach
    connect(mpTabBar, &QTabBar::tabBarDoubleClicked, this, &TDetachedWindow::onReattachAction);
    
    // Connect context menu for right-click reattach
    mpTabBar->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(mpTabBar, &QWidget::customContextMenuRequested, 
            this, &TDetachedWindow::showTabContextMenu);
}

void TDetachedWindow::createMenus()
{
    // Profile menu with quick actions
    auto profileMenu = menuBar()->addMenu(tr("&Profile"));
    
    auto saveProfileAction = new QAction(QIcon(qsl(":/icons/document-save.png")), tr("&Save Profile"), this);
    saveProfileAction->setShortcut(QKeySequence::Save);
    saveProfileAction->setStatusTip(tr("Save the current profile"));
    connect(saveProfileAction, &QAction::triggered, this, &TDetachedWindow::slot_saveProfile);
    profileMenu->addAction(saveProfileAction);
    
    profileMenu->addSeparator();
    
    auto exportProfileAction = new QAction(QIcon(qsl(":/icons/document-export.png")), tr("&Export Profile"), this);
    exportProfileAction->setStatusTip(tr("Export profile as package"));
    connect(exportProfileAction, &QAction::triggered, this, &TDetachedWindow::slot_exportProfile);
    profileMenu->addAction(exportProfileAction);
    
    auto profileSettingsAction = new QAction(QIcon(qsl(":/icons/configure.png")), tr("Profile &Settings"), this);
    profileSettingsAction->setShortcut(QKeySequence::Preferences);
    profileSettingsAction->setStatusTip(tr("Open profile settings"));
    connect(profileSettingsAction, &QAction::triggered, mudlet::self(), &mudlet::slot_showPreferencesDialog);
    profileMenu->addAction(profileSettingsAction);
    
    profileMenu->addSeparator();
    
    auto closeProfileAction = new QAction(QIcon(qsl(":/icons/profile-close.png")), tr("&Close Profile"), this);
    closeProfileAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_W));
    closeProfileAction->setStatusTip(tr("Close the current profile"));
    connect(closeProfileAction, &QAction::triggered, mudlet::self(), &mudlet::slot_closeCurrentProfile);
    profileMenu->addAction(closeProfileAction);

    auto windowMenu = menuBar()->addMenu(tr("&Window"));
    
    auto reattachAction = new QAction(tr("&Reattach to Main Window"), this);
    reattachAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    reattachAction->setStatusTip(tr("Reattach this profile window to the main Mudlet window"));
    connect(reattachAction, &QAction::triggered, this, &TDetachedWindow::onReattachAction);
    windowMenu->addAction(reattachAction);
    
    windowMenu->addSeparator();
    
    auto closeAction = new QAction(tr("&Close"), this);
    closeAction->setShortcut(QKeySequence::Close);
    connect(closeAction, &QAction::triggered, this, &QWidget::close);
    windowMenu->addAction(closeAction);
    
    windowMenu->addSeparator();
    
    // Always on top toggle
    auto alwaysOnTopAction = new QAction(tr("Always on &Top"), this);
    alwaysOnTopAction->setCheckable(true);
    alwaysOnTopAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_T));
    alwaysOnTopAction->setStatusTip(tr("Keep this window always on top of other windows"));
    connect(alwaysOnTopAction, &QAction::triggered, this, &TDetachedWindow::slot_toggleAlwaysOnTop);
    windowMenu->addAction(alwaysOnTopAction);
    
    // Minimize action
    auto minimizeAction = new QAction(tr("&Minimize"), this);
    minimizeAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_M));
    connect(minimizeAction, &QAction::triggered, this, &QWidget::showMinimized);
    windowMenu->addAction(minimizeAction);
}

void TDetachedWindow::closeEvent(QCloseEvent* event)
{
    // Only remove consoles if this is not a reattachment
    if (!mIsReattaching) {
        // Remove all consoles from the stacked widget and reset their parents
        for (auto console : mProfileConsoleMap) {
            if (console) {
                mpConsoleContainer->removeWidget(console);
                console->setParent(nullptr);
            }
        }
        mProfileConsoleMap.clear();
    }
    
    // Emit signal to notify main window only if not reattaching
    if (!mIsReattaching) {
        // Emit for each profile being closed
        for (const QString& profileName : mProfileConsoleMap.keys()) {
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
        qDebug() << "TDetachedWindow::hideEvent: Preventing window hide - has" << mpTabBar->count() << "profiles";
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
                                               tr("Reattach '%1' to Main Window").arg(profileName));
    connect(reattachAction, &QAction::triggered, [this, profileName] {
        // Switch to this profile first, then reattach
        switchToProfile(profileName);
        onReattachAction();
    });
    
    contextMenu.addSeparator();
    
    auto closeTabAction = contextMenu.addAction(QIcon(qsl(":/icons/profile-close.png")), 
                                               tr("Close Profile '%1'").arg(profileName));
    connect(closeTabAction, &QAction::triggered, [this, tabIndex] {
        closeProfileByIndex(tabIndex);
    });
    
    if (mProfileConsoleMap.size() > 1) {
        contextMenu.addSeparator();
        
        auto closeWindowAction = contextMenu.addAction(QIcon(qsl(":/icons/dialog-close.png")), 
                                                      tr("Close Window (All Profiles)"));
        connect(closeWindowAction, &QAction::triggered, this, &QWidget::close);
    }
    
    contextMenu.exec(mpTabBar->mapToGlobal(position));
}

void TDetachedWindow::saveWindowGeometry()
{
    QSettings settings;
    // Use current profile name for settings key
    const QString key = QString("DetachedWindow/%1").arg(mCurrentProfileName.isEmpty() ? "Unknown" : mCurrentProfileName);
    settings.setValue(key + "/geometry", saveGeometry());
    settings.setValue(key + "/windowState", saveState());
}

void TDetachedWindow::restoreWindowGeometry()
{
    QSettings settings;
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

    // Connect button with dropdown actions
    mpButtonConnect = new QToolButton(this);
    mpButtonConnect->setText(tr("Connect"));
    mpButtonConnect->setObjectName(qsl("connect"));
    mpButtonConnect->setContextMenuPolicy(Qt::ActionsContextMenu);
    mpButtonConnect->setPopupMode(QToolButton::MenuButtonPopup);
    mpButtonConnect->setAutoRaise(true);
    mpButtonConnect->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    mpToolBar->addWidget(mpButtonConnect);

    mpActionConnect = new QAction(tr("Connect"), this);
    mpActionConnect->setIcon(QIcon(qsl(":/icons/preferences-web-browser-cache.png")));
    mpActionConnect->setIconText(tr("Connect"));
    mpActionConnect->setObjectName(qsl("connect"));

    mpActionDisconnect = new QAction(tr("Disconnect"), this);
    mpActionDisconnect->setObjectName(qsl("disconnect"));

    mpActionReconnect = new QAction(tr("Reconnect"), this);
    mpActionReconnect->setIcon(QIcon(qsl(":/icons/system-restart.png")));
    mpActionReconnect->setObjectName(qsl("reconnect"));

    mpActionCloseProfile = new QAction(tr("Close profile"), this);
    mpActionCloseProfile->setIcon(QIcon(qsl(":/icons/profile-close.png")));
    mpActionCloseProfile->setIconText(tr("Close profile"));
    mpActionCloseProfile->setObjectName(qsl("close_profile"));

    mpButtonConnect->addAction(mpActionConnect);
    mpButtonConnect->addAction(mpActionDisconnect);
    mpButtonConnect->addAction(mpActionReconnect);
    mpButtonConnect->addAction(mpActionCloseProfile);
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

    // Full screen toggle
    QIcon fullScreenIcon;
    fullScreenIcon.addPixmap(qsl(":/icons/view-fullscreen.png"), QIcon::Normal, QIcon::Off);
    fullScreenIcon.addPixmap(qsl(":/icons/view-restore.png"), QIcon::Normal, QIcon::On);
    mpActionFullScreenView = new QAction(fullScreenIcon, tr("Full Screen"), this);
    mpActionFullScreenView->setToolTip(utils::richText(tr("Toggle Full Screen View")));
    mpActionFullScreenView->setCheckable(true);
    mpActionFullScreenView->setObjectName(qsl("fullscreen_action"));
    mpToolBar->addAction(mpActionFullScreenView);

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
    connect(mpActionReconnect, &QAction::triggered, this, &TDetachedWindow::slot_reconnectProfile);
    connect(mpActionCloseProfile, &QAction::triggered, this, &TDetachedWindow::slot_closeCurrentProfile);

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
    connect(mpActionFullScreenView, &QAction::triggered, this, &TDetachedWindow::slot_toggleFullScreenView);
}

void TDetachedWindow::createStatusBar()
{
    // Create status bar with connection and profile information
    auto statusBar = this->statusBar();
    statusBar->showMessage(tr("Ready"));
    
    // Profile information (left side)
    mpStatusBarProfileLabel = new QLabel(tr("Profile: %1").arg(mCurrentProfileName.isEmpty() ? tr("None") : mCurrentProfileName));
    mpStatusBarProfileLabel->setStyleSheet(qsl("QLabel { padding: 2px 8px; }"));
    statusBar->addWidget(mpStatusBarProfileLabel);
    
    // Add stretcher to push connection info to the right
    statusBar->addWidget(new QWidget(), 1);
    
    // Connection information (right side)
    mpStatusBarConnectionLabel = new QLabel(tr("Disconnected"));
    mpStatusBarConnectionLabel->setStyleSheet(qsl("QLabel { padding: 2px 8px; }"));
    statusBar->addPermanentWidget(mpStatusBarConnectionLabel);
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
        bool isConnected = (pHost->mTelnet.getConnectionState() == QAbstractSocket::ConnectedState);
        bool isConnecting = (pHost->mTelnet.getConnectionState() == QAbstractSocket::ConnectingState);
        
        // Update status bar
        updateStatusBar(pHost, isConnected, isConnecting);
        
        // Enable/disable individual actions based on connection state
        // All actions should always be enabled to match main window behavior
        mpActionConnect->setEnabled(true);
        mpActionDisconnect->setEnabled(true);
        mpActionReconnect->setEnabled(true);
        mpActionCloseProfile->setEnabled(true);
    } else {
        // Update status bar
        updateStatusBar(nullptr, false, false);
        
        // Even when no profile is active, keep all actions enabled to match main window
        mpActionConnect->setEnabled(true);
        mpActionDisconnect->setEnabled(true);
        mpActionReconnect->setEnabled(true);
        mpActionCloseProfile->setEnabled(true);
    }

    // Mute actions are always enabled
    mpActionMuteMedia->setEnabled(true);
    mpActionMuteAPI->setEnabled(true);
    mpActionMuteGame->setEnabled(true);

    // Help is always enabled
    mpActionHelp->setEnabled(true);

    // Full screen toggle is always enabled
    mpActionFullScreenView->setEnabled(true);
}

void TDetachedWindow::updateToolbarForProfile(Host* pHost)
{
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
            title = tr("Mudlet - %1 (Detached)").arg(mCurrentProfileName);
        } else {
            title = tr("Mudlet (Detached)");
        }
        
        if (pHost) {
            bool isConnected = (pHost->mTelnet.getConnectionState() == QAbstractSocket::ConnectedState);
            bool isConnecting = (pHost->mTelnet.getConnectionState() == QAbstractSocket::ConnectingState);
            
            if (isConnected) {
                title += tr(" - Connected");
                if (!pHost->getUrl().isEmpty()) {
                    title += tr(" to %1").arg(pHost->getUrl());
                }
            } else if (isConnecting) {
                title += tr(" - Connecting...");
            } else {
                title += tr(" - Disconnected");
            }
        }
    } else {
        // Multiple profiles - show count and current
        title = tr("Mudlet (%1 profiles) - %2 (Detached)")
                .arg(mProfileConsoleMap.size())
                .arg(mCurrentProfileName.isEmpty() ? tr("None") : mCurrentProfileName);
    }
    
    setWindowTitle(title);
}

void TDetachedWindow::updateStatusBar(Host* pHost, bool isConnected, bool isConnecting)
{
    if (!mpStatusBarConnectionLabel || !mpStatusBarProfileLabel) {
        return;
    }
    
    // Update profile label
    if (mProfileConsoleMap.size() == 1) {
        mpStatusBarProfileLabel->setText(tr("Profile: %1").arg(mCurrentProfileName));
    } else {
        mpStatusBarProfileLabel->setText(tr("Profiles: %1 | Active: %2")
                                       .arg(mProfileConsoleMap.size())
                                       .arg(mCurrentProfileName.isEmpty() ? tr("None") : mCurrentProfileName));
    }
    
    // Update connection label
    if (pHost) {
        if (isConnected) {
            mpStatusBarConnectionLabel->setText(tr("Connected to %1").arg(pHost->getUrl()));
            mpStatusBarConnectionLabel->setStyleSheet(qsl("QLabel { color: green; padding: 2px 8px; }"));
        } else if (isConnecting) {
            mpStatusBarConnectionLabel->setText(tr("Connecting to %1...").arg(pHost->getUrl()));
            mpStatusBarConnectionLabel->setStyleSheet(qsl("QLabel { color: orange; padding: 2px 8px; }"));
        } else {
            mpStatusBarConnectionLabel->setText(tr("Disconnected from %1").arg(pHost->getUrl()));
            mpStatusBarConnectionLabel->setStyleSheet(qsl("QLabel { color: red; padding: 2px 8px; }"));
        }
    } else {
        mpStatusBarConnectionLabel->setText(tr("No Profile"));
        mpStatusBarConnectionLabel->setStyleSheet(qsl("QLabel { color: gray; padding: 2px 8px; }"));
    }
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
    
    if (pHost) {
        bool isConnected = (pHost->mTelnet.getConnectionState() == QAbstractSocket::ConnectedState);
        bool isConnecting = (pHost->mTelnet.getConnectionState() == QAbstractSocket::ConnectingState);
        
        if (isConnected) {
            tabIcon = QIcon(qsl(":/icons/dialog-ok-apply.png"));
        } else if (isConnecting) {
            tabIcon = QIcon(qsl(":/icons/dialog-information.png"));
        } else {
            tabIcon = QIcon(qsl(":/icons/dialog-close.png"));
        }
    } else {
        tabIcon = QIcon(qsl(":/icons/dialog-error.png"));
    }
    
    // Set the tab text and icon
    mpTabBar->setTabText(tabIndex, profileName);
    mpTabBar->setTabIcon(tabIndex, tabIcon);
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

void TDetachedWindow::slot_saveProfile()
{
    if (mCurrentProfileName.isEmpty()) {
        return;
    }
    
    Host* pHost = mudlet::self()->getHostManager().getHost(mCurrentProfileName);
    if (pHost) {
        pHost->saveProfile();
        statusBar()->showMessage(tr("Profile '%1' saved").arg(mCurrentProfileName), 2000);
    }
}

void TDetachedWindow::slot_exportProfile()
{
    // Use the main mudlet's package exporter functionality
    mudlet::self()->slot_packageExporter();
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
            // Update window title and status bar when no profiles remain
            updateWindowTitle();
            updateStatusBar(nullptr, false, false);
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
    // Simplified debug output for critical state information only
    if (mShouldStayVisible && mpTabBar && mpTabBar->count() > 0 && !isVisible()) {
        qDebug() << "TDetachedWindow [" << context << "] WARNING: Window should stay visible but is hidden - profiles:" << getProfileCount();
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
    withCurrentProfileActive([this]() {
        mudlet::self()->slot_closeCurrentProfile();
    });
}

void TDetachedWindow::slot_showTriggerDialog()
{
    withCurrentProfileActive([this]() {
        mudlet::self()->slot_showTriggerDialog();
    });
}

void TDetachedWindow::slot_showAliasDialog()
{
    withCurrentProfileActive([this]() {
        mudlet::self()->slot_showAliasDialog();
    });
}

void TDetachedWindow::slot_showTimerDialog()
{
    withCurrentProfileActive([this]() {
        mudlet::self()->slot_showTimerDialog();
    });
}

void TDetachedWindow::slot_showActionDialog()
{
    withCurrentProfileActive([this]() {
        mudlet::self()->slot_showActionDialog();
    });
}

void TDetachedWindow::slot_showScriptDialog()
{
    withCurrentProfileActive([this]() {
        mudlet::self()->slot_showScriptDialog();
    });
}

void TDetachedWindow::slot_showKeyDialog()
{
    withCurrentProfileActive([this]() {
        mudlet::self()->slot_showKeyDialog();
    });
}

void TDetachedWindow::slot_showVariableDialog()
{
    withCurrentProfileActive([this]() {
        mudlet::self()->slot_showVariableDialog();
    });
}

void TDetachedWindow::slot_showMapperDialog()
{
    withCurrentProfileActive([this]() {
        mudlet::self()->slot_mapper();
    });
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
        mudlet::self()->slot_showPreferencesDialog();
    });
}

void TDetachedWindow::slot_showNotesDialog()
{
    withCurrentProfileActive([this]() {
        mudlet::self()->slot_notes();
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
        mudlet::self()->slot_packageManager();
    });
}

void TDetachedWindow::slot_showModuleManagerDialog()
{
    withCurrentProfileActive([this]() {
        mudlet::self()->slot_moduleManager();
    });
}

void TDetachedWindow::slot_showPackageExporterDialog()
{
    withCurrentProfileActive([this]() {
        mudlet::self()->slot_packageExporter();
    });
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
    if (event->type() == QEvent::WindowStateChange) {
        auto stateChangeEvent = static_cast<QWindowStateChangeEvent*>(event);
        
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
    }
    
    QMainWindow::changeEvent(event);
}
