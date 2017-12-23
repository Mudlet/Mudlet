/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2013-2017 by Stephen Lyons - slysven@virginmedia.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016 by Chris Leacy - cleacy1972@gmail.com              *
 *   Copyright (C) 2016-2017 by Ian Adkins - ieadkins@gmail.com            *
 *   Copyright (C) 2017 by Tom Scheper - scheper@gmail.com                 *
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


#include "mudlet.h"

#include "EAction.h"
#include "Host.h"
#include "HostManager.h"
#include "LuaInterface.h"
#include "TCommandLine.h"
#include "TConsole.h"
#include "TDebug.h"
#include "TDockWidget.h"
#include "TEvent.h"
#include "TLabel.h"
#include "TMap.h"
#include "TRoomDB.h"
#include "TTextEdit.h"
#include "TToolBar.h"
#include "XMLimport.h"
#include "dlgAboutDialog.h"
#include "dlgConnectionProfiles.h"
#include "dlgIRC.h"
#include "dlgMapper.h"
#include "dlgNotepad.h"
#include "dlgPackageExporter.h"
#include "dlgProfilePreferences.h"
#include "dlgTriggerEditor.h"
#include "edbee/edbee.h"
#include "edbee/models/textgrammar.h"
#include "edbee/texteditorwidget.h"
#include "edbee/views/texttheme.h"
#if defined(INCLUDE_UPDATER)
#include "updater.h"
#endif

#include "pre_guard.h"
#include <QtEvents>
#include <QtUiTools/quiloader.h>
#include <QApplication>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QDockWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QScrollBar>
#include <QTableWidget>
#include <QTextCharFormat>
#include <QToolBar>
#include "post_guard.h"

#include <zip.h>

using namespace std;

bool TConsoleMonitor::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::Close) {
        mudlet::debugMode = false;
        return QObject::eventFilter(obj, event);
    } else {
        return QObject::eventFilter(obj, event);
    }
}

// "mMudletXmlDefaultFormat" number represents a major (integer part) and minor
// (1000ths, range 0 to 999) that is used as a "version" attribute number when
// writing the <MudletPackage ...> element of all (but maps if I ever get around
// to doing a Map Xml file exporter/writer) Xml files used to export/save Mudlet
// button/menu/toolbars; aliases. keys, scripts, timers, triggers and variables
// and collections of these as modules/packages and entire profiles as "game
// saves".  Mudlet versions up to 3.0.1 never bothered checking the version
// detail and it had been hard coded as "1.0" back as far as history can
// determine.  From that version a check was coded to test that the version
// was less than 2.000f with the intention to loudly and clearly fail if a
// higher version was encountered. Values above 1.000f have not yet been
// codified but should be accepted so it should be possible to raise the number
// a little and to use that to extend the Xml data format in a manner that older
// versions ignore (possibly with some noise) but which they can still get the
// details they can handle yet allow a later upgraded version to get extra
// information they want.
//
// Taking this number to 2.000f or more WILL prevent old versions from reading
// Xml files and should be considered a step associated with a major version
// number change in the Mudlet application itself and SHOULD NOT BE DONE WITHOUT
// agreement and consideration from the Project management, even a minor part
// increment should not be done without justification...!
// XML version Change history (what and why):
// 1.001    Added method to allow XML format to permit ASCII control codes
//          0x01-0x08, 0x0b, 0x0c, 0x0e-0x1f, 0x7f to be stored as part of the
//          "script" element for a Mudlet "item" (0x09, 0x0a, 0x0d are the only
//          ones that ARE permitted) - this is wanted so that, for instance
//          ANSI ESC codes can be included in a Lua script without breaking
//          the XML format used to store it - prior to this embedding such
//          codes would break or destroy the script that used it.
const QString mudlet::scmMudletXmlDefaultVersion = QString::number(1.001f, 'f', 3);

QPointer<TConsole> mudlet::mpDebugConsole = nullptr;
QPointer<QMainWindow> mudlet::mpDebugArea = nullptr;
bool mudlet::debugMode = false;
static const QString timeFormat = "hh:mm:ss";
const bool mudlet::scmIsDevelopmentVersion = !QByteArray(APP_BUILD).isEmpty();

QPointer<mudlet> mudlet::_self;

void mudlet::start()
{
    _self = new mudlet;
}

mudlet* mudlet::self()
{
    return _self;
}


mudlet::mudlet()
: QMainWindow()
, mShowMenuBar(false)
, mShowToolbar(true)
, mToolbarIconSize(0)
, mEditorTreeWidgetIconSize(0)
, mWindowMinimized(false)
, mReplaySpeed(1)
, version(QString("Mudlet ") + QString(APP_VERSION) + QString(APP_BUILD))
, mpCurrentActiveHost(nullptr)
, mIsGoingDown(false)
, mIsLoadingLayout(false)
, mHasSavedLayout(false)
, actionReplaySpeedDown(nullptr)
, actionReplaySpeedUp(nullptr)
, actionSpeedDisplay(nullptr)
, actionReplayTime(nullptr)
, replaySpeedDisplay(nullptr)
, replayTime(nullptr)
, replayTimer(nullptr)
, replayToolBar(nullptr)
, moduleTable(nullptr)
, mCompactInputLine(false)
, mpAboutDlg(nullptr)
, mpModuleDlg(nullptr)
, mpPackageManagerDlg(nullptr)
, mpProfilePreferencesDlg(nullptr)
, mshowMapAuditErrors(false)
{
    setupUi(this);
    setUnifiedTitleAndToolBarOnMac(true);
    setContentsMargins(0, 0, 0, 0);
    mudlet::debugMode = false;
    setAttribute(Qt::WA_DeleteOnClose);
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setWindowTitle(version);
    setWindowIcon(QIcon(QStringLiteral(":/icons/mudlet_main_48px.png")));
    mpMainToolBar = new QToolBar(this);
    mpMainToolBar->setObjectName("mpMainToolBar");
    addToolBar(mpMainToolBar);
    mpMainToolBar->setMovable(false);
    addToolBarBreak();
    auto frame = new QWidget(this);
    frame->setFocusPolicy(Qt::NoFocus);
    setCentralWidget(frame);
    mpTabBar = new QTabBar(frame);
    mpTabBar->setMaximumHeight(30);
    mpTabBar->setFocusPolicy(Qt::NoFocus);
    mpTabBar->setTabsClosable(true);
    connect(mpTabBar, SIGNAL(tabCloseRequested(int)), this, SLOT(slot_close_profile_requested(int)));
    mpTabBar->setMovable(true);
    connect(mpTabBar, SIGNAL(currentChanged(int)), this, SLOT(slot_tab_changed(int)));
    auto layoutTopLevel = new QVBoxLayout(frame);
    layoutTopLevel->setContentsMargins(0, 0, 0, 0);
    layoutTopLevel->addWidget(mpTabBar);
    mainPane = new QWidget(frame);
    QPalette mainPalette;
    mainPane->setPalette(mainPalette);
    mainPane->setAutoFillBackground(true);
    mainPane->setFocusPolicy(Qt::NoFocus);
    layoutTopLevel->addWidget(mainPane);
    auto layout = new QHBoxLayout(mainPane);
    layout->setContentsMargins(0, 0, 0, 0);

    mainPane->setContentsMargins(0, 0, 0, 0);
    mainPane->setSizePolicy(sizePolicy);
    mainPane->setFocusPolicy(Qt::NoFocus);

    QFile file_autolog(getMudletPath(mainDataItemPath, QStringLiteral("autolog")));
    if (file_autolog.exists()) {
        mAutolog = true;
    } else {
        mAutolog = false;
    }

    QAction* actionConnect = new QAction(QIcon(QStringLiteral(":/icons/preferences-web-browser-cache.png")), tr("Connect"), this);
    actionConnect->setToolTip(tr("Connect to a MUD"));
    mpMainToolBar->addAction(actionConnect);

    // add name to the action's widget in the toolbar, which doesn't have one by default
    // see https://stackoverflow.com/a/32460562/72944
    actionConnect->setObjectName(QStringLiteral("connect_action"));
    mpMainToolBar->widgetForAction(actionConnect)->setObjectName(actionConnect->objectName());

    QAction* actionTriggers = new QAction(QIcon(QStringLiteral(":/icons/tools-wizard.png")), tr("Triggers"), this);
    actionTriggers->setToolTip(tr("Show and edit triggers"));
    mpMainToolBar->addAction(actionTriggers);
    actionTriggers->setObjectName(QStringLiteral("triggers_action"));
    mpMainToolBar->widgetForAction(actionTriggers)->setObjectName(actionTriggers->objectName());

    QAction* actionAlias = new QAction(QIcon(QStringLiteral(":/icons/system-users.png")), tr("Aliases"), this);
    actionAlias->setToolTip(tr("Show and edit aliases"));
    mpMainToolBar->addAction(actionAlias);
    actionAlias->setObjectName(QStringLiteral("aliases_action"));
    mpMainToolBar->widgetForAction(actionAlias)->setObjectName(actionAlias->objectName());

    QAction* actionTimers = new QAction(QIcon(QStringLiteral(":/icons/chronometer.png")), tr("Timers"), this);
    actionTimers->setToolTip(tr("Show and edit timers"));
    mpMainToolBar->addAction(actionTimers);
    actionTimers->setObjectName(QStringLiteral("timers_action"));
    mpMainToolBar->widgetForAction(actionTimers)->setObjectName(actionTimers->objectName());

    QAction* actionButtons = new QAction(QIcon(QStringLiteral(":/icons/bookmarks.png")), tr("Buttons"), this);
    actionButtons->setToolTip(tr("Show and edit easy buttons"));
    mpMainToolBar->addAction(actionButtons);
    actionButtons->setObjectName(QStringLiteral("buttons_action"));
    mpMainToolBar->widgetForAction(actionButtons)->setObjectName(actionButtons->objectName());

    QAction* actionScripts = new QAction(QIcon(QStringLiteral(":/icons/document-properties.png")), tr("Scripts"), this);
    actionScripts->setToolTip(tr("Show and edit scripts"));
    mpMainToolBar->addAction(actionScripts);
    actionScripts->setObjectName(QStringLiteral("scripts_action"));
    mpMainToolBar->widgetForAction(actionScripts)->setObjectName(actionScripts->objectName());

    QAction* actionKeys = new QAction(QIcon(QStringLiteral(":/icons/preferences-desktop-keyboard.png")), tr("Keys"), this);
    actionKeys->setToolTip(tr("Show and edit keys"));
    mpMainToolBar->addAction(actionKeys);
    actionKeys->setObjectName(QStringLiteral("keys_action"));
    mpMainToolBar->widgetForAction(actionKeys)->setObjectName(actionKeys->objectName());

    QAction* actionVars = new QAction(QIcon(QStringLiteral(":/icons/variables.png")), tr("Variables"), this);
    actionVars->setToolTip(tr("Show and edit Lua variables"));
    mpMainToolBar->addAction(actionVars);
    actionVars->setObjectName(QStringLiteral("variables_action"));
    mpMainToolBar->widgetForAction(actionVars)->setObjectName(actionVars->objectName());

    QAction* actionIRC = new QAction(QIcon(QStringLiteral(":/icons/internet-telephony.png")), tr("IRC"), this);
    actionIRC->setToolTip(tr("Open the Mudlet IRC client"));
    mpMainToolBar->addAction(actionIRC);
    actionIRC->setObjectName(QStringLiteral("irc_action"));
    mpMainToolBar->widgetForAction(actionIRC)->setObjectName(actionIRC->objectName());

    QAction* actionMapper = new QAction(QIcon(QStringLiteral(":/icons/applications-internet.png")), tr("Map"), this);
    actionMapper->setToolTip(tr("Show/hide the map"));
    mpMainToolBar->addAction(actionMapper);
    actionMapper->setObjectName(QStringLiteral("map_action"));
    mpMainToolBar->widgetForAction(actionMapper)->setObjectName(actionMapper->objectName());

    QAction* actionHelp = new QAction(QIcon(QStringLiteral(":/icons/help-hint.png")), tr("Manual"), this);
    actionHelp->setToolTip(tr("Browse reference material and documentation"));
    mpMainToolBar->addAction(actionHelp);
    actionHelp->setObjectName(QStringLiteral("manual_action"));
    mpMainToolBar->widgetForAction(actionHelp)->setObjectName(actionHelp->objectName());

    QAction* actionOptions = new QAction(QIcon(QStringLiteral(":/icons/configure.png")), tr("Settings"), this);
    actionOptions->setToolTip(tr("See and edit profile preferences"));
    mpMainToolBar->addAction(actionOptions);
    actionOptions->setObjectName(QStringLiteral("settings_action"));
    mpMainToolBar->widgetForAction(actionOptions)->setObjectName(actionOptions->objectName());

    // TODO: Consider changing to ":/icons/mudlet_notepad.png" as per the icon
    // now used for the window when the visual change to the toolbar caused can
    // be managed
    QAction* actionNotes = new QAction(QIcon(QStringLiteral(":/icons/applications-accessories.png")), tr("Notepad"), this);
    actionNotes->setToolTip(tr("Open a notepad that you can store your notes in"));
    mpMainToolBar->addAction(actionNotes);
    actionNotes->setObjectName(QStringLiteral("notepad_action"));
    mpMainToolBar->widgetForAction(actionNotes)->setObjectName(actionNotes->objectName());

    QAction* actionPackageM = new QAction(QIcon(QStringLiteral(":/icons/package-manager.png")), tr("Package Manager"), this);
    actionPackageM->setToolTip(tr("Package Manager - allows you to install xmls, .mpackages"));
    mpMainToolBar->addAction(actionPackageM);
    actionPackageM->setObjectName(QStringLiteral("package_action"));
    mpMainToolBar->widgetForAction(actionPackageM)->setObjectName(actionPackageM->objectName());

    QAction* actionModuleM = new QAction(QIcon(QStringLiteral(":/icons/module-manager.png")), tr("Module Manager"), this);
    actionModuleM->setToolTip(tr("Module Manager - allows you to install xmls, .mpackages that are syncronized across multiple profile (good for scripts that you use on several profiles)"));
    mpMainToolBar->addAction(actionModuleM);
    actionModuleM->setObjectName(QStringLiteral("module_action"));
    mpMainToolBar->widgetForAction(actionModuleM)->setObjectName(actionModuleM->objectName());

    QAction* actionReplay = new QAction(QIcon(QStringLiteral(":/icons/media-optical.png")), tr("Replay"), this);
    actionReplay->setToolTip(tr("Load a Mudlet replay"));
    mpMainToolBar->addAction(actionReplay);
    actionReplay->setObjectName(QStringLiteral("replay_action"));
    mpMainToolBar->widgetForAction(actionReplay)->setObjectName(actionReplay->objectName());

    actionReconnect = new QAction(QIcon(QStringLiteral(":/icons/system-restart.png")), tr("Reconnect"), this);
    actionReconnect->setToolTip(tr("Disconnects you from the game and connects once again"));
    mpMainToolBar->addAction(actionReconnect);
    actionReconnect->setObjectName(QStringLiteral("reconnect_action"));
    mpMainToolBar->widgetForAction(actionReconnect)->setObjectName(actionReconnect->objectName());

    QAction* actionMultiView = new QAction(QIcon(QStringLiteral(":/icons/view-split-left-right.png")), tr("MultiView"), this);
    actionMultiView->setToolTip(tr("If you've got multiple profiles open, splits Mudlet screen to show them all at once"));
    mpMainToolBar->addAction(actionMultiView);
    actionMultiView->setObjectName(QStringLiteral("multiview_action"));
    mpMainToolBar->widgetForAction(actionMultiView)->setObjectName(actionMultiView->objectName());

    QAction* actionStopAllTriggers = new QAction(QIcon(QStringLiteral(":/icons/edit-bomb.png")), tr("Stop All Triggers"), this);
    actionStopAllTriggers->setToolTip(tr("Stop all triggers, alias, actions, timers and scripts"));

    QAction* actionAbout = new QAction(QIcon(QStringLiteral(":/icons/mudlet_information.png")), tr("About"), this);
    actionAbout->setToolTip(tr("About Mudlet"));
    mpMainToolBar->addAction(actionAbout);
    actionAbout->setObjectName(QStringLiteral("about_action"));
    mpMainToolBar->widgetForAction(actionAbout)->setObjectName(actionAbout->objectName());

    disableToolbarButtons();

    mpDebugArea = new QMainWindow(nullptr);
    // PLACEMARKER: Host creation (1) - "default_host" case
    QString defaultHost(QStringLiteral("default_host"));
    // We DO NOT emit a signal_hostCreated for THIS case:
    mHostManager.addHost(defaultHost, QString(), QString(), QString());
    mpDefaultHost = mHostManager.getHost(defaultHost);
    mpDebugConsole = new TConsole(mpDefaultHost, true);
    mpDebugConsole->setSizePolicy(sizePolicy);
    mpDebugConsole->setWrapAt(100);
    mpDebugArea->setCentralWidget(mpDebugConsole);
    mpDebugArea->setWindowTitle(tr("Central Debug Console"));
    mpDebugArea->setWindowIcon(QIcon(QStringLiteral(":/icons/mudlet_debug.png")));

    auto consoleCloser = new TConsoleMonitor(mpDebugArea);
    mpDebugArea->installEventFilter(consoleCloser);

    QSize generalRule(qApp->desktop()->size());
    generalRule -= QSize(30, 30);
    mpDebugArea->resize(QSize(800, 600).boundedTo(generalRule));
    mpDebugArea->hide();
    QFont mainFont;
    mainFont = QFont(QStringLiteral("Bitstream Vera Sans Mono"), 8, QFont::Normal);
    QFile file_use_smallscreen(getMudletPath(mainDataItemPath, QStringLiteral("mudlet_option_use_smallscreen")));
    if (file_use_smallscreen.exists()) {
        showFullScreen();
        QAction* actionFullScreeniew = new QAction(QIcon(QStringLiteral(":/icons/dialog-cancel.png")), tr("Toggle Full Screen View"), this);
        actionFullScreeniew->setStatusTip(tr("Toggle Full Screen View"));
        mpMainToolBar->addAction(actionFullScreeniew);
        actionFullScreeniew->setObjectName(QStringLiteral("fullscreen_action"));
        mpMainToolBar->widgetForAction(actionFullScreeniew)->setObjectName(actionFullScreeniew->objectName());
        connect(actionFullScreeniew, SIGNAL(triggered()), this, SLOT(toggleFullScreenView()));
    }
    QFont mdiFont = QFont(QStringLiteral("Bitstream Vera Sans Mono"), 6, QFont::Normal);
    setFont(mainFont);
    mainPane->setFont(mainFont);
    mpTabBar->setFont(mdiFont);

    mainPane->show();

    connect(actionConnect, SIGNAL(triggered()), this, SLOT(slot_show_connection_dialog()));
    connect(actionHelp, SIGNAL(triggered()), this, SLOT(show_help_dialog()));
    connect(actionTriggers, SIGNAL(triggered()), this, SLOT(show_trigger_dialog()));
    connect(actionTimers, SIGNAL(triggered()), this, SLOT(show_timer_dialog()));
    connect(actionAlias, SIGNAL(triggered()), this, SLOT(show_alias_dialog()));
    connect(actionScripts, SIGNAL(triggered()), this, SLOT(show_script_dialog()));
    connect(actionKeys, SIGNAL(triggered()), this, SLOT(show_key_dialog()));
    connect(actionVars, SIGNAL(triggered()), this, SLOT(show_variable_dialog()));
    connect(actionButtons, SIGNAL(triggered()), this, SLOT(show_action_dialog()));
    connect(actionOptions, SIGNAL(triggered()), this, SLOT(show_options_dialog()));
    connect(actionAbout, SIGNAL(triggered()), this, SLOT(slot_show_about_dialog()));
    connect(actionMultiView, SIGNAL(triggered()), this, SLOT(slot_multi_view()));
    connect(actionReconnect, SIGNAL(triggered()), this, SLOT(slot_reconnect()));
    connect(actionReplay, SIGNAL(triggered()), this, SLOT(slot_replay()));
    connect(actionNotes, SIGNAL(triggered()), this, SLOT(slot_notes()));
    connect(actionMapper, SIGNAL(triggered()), this, SLOT(slot_mapper()));
    connect(actionIRC, SIGNAL(triggered()), this, SLOT(slot_irc()));
    connect(actionPackageM, SIGNAL(triggered()), this, SLOT(slot_package_manager()));
    connect(actionModuleM, SIGNAL(triggered()), this, SLOT(slot_module_manager()));

    QAction* mactionConnect = new QAction(tr("Connect"), this);
    QAction* mactionTriggers = new QAction(tr("Triggers"), this);
    QAction* mactionAlias = new QAction(tr("Aliases"), this);
    QAction* mactionTimers = new QAction(tr("Timers"), this);
    QAction* mactionButtons = new QAction(tr("Actions"), this);
    QAction* mactionScripts = new QAction(tr("Scripts"), this);
    QAction* mactionKeys = new QAction(tr("Keys"), this);
    QAction* mactionMapper = new QAction(tr("Map"), this);
    QAction* mactionHelp = new QAction(tr("Help"), this);
    QAction* mactionOptions = new QAction(tr("Preferences"), this);
    QAction* mactionMultiView = new QAction(tr("MultiView"), this);
    QAction* mactionAbout = new QAction(tr("About"), this);
    QAction* mactionCloseProfile = new QAction(tr("Close"), this);

    connect(mactionConnect, SIGNAL(triggered()), this, SLOT(slot_show_connection_dialog()));
    connect(dactionConnect, SIGNAL(triggered()), this, SLOT(slot_show_connection_dialog()));
    connect(dactionReconnect, SIGNAL(triggered()), this, SLOT(slot_reconnect()));
    connect(dactionDisconnect, SIGNAL(triggered()), this, SLOT(slot_disconnect()));
    connect(dactionNotepad, SIGNAL(triggered()), this, SLOT(slot_notes()));
    connect(dactionReplay, SIGNAL(triggered()), this, SLOT(slot_replay()));

    connect(mactionHelp, SIGNAL(triggered()), this, SLOT(show_help_dialog()));
    connect(dactionHelp, SIGNAL(triggered()), this, SLOT(show_help_dialog()));
    connect(dactionVideo, SIGNAL(triggered()), this, SLOT(slot_show_help_dialog_video()));
    connect(dactionForum, SIGNAL(triggered()), this, SLOT(slot_show_help_dialog_forum()));
    connect(dactionIRC, SIGNAL(triggered()), this, SLOT(slot_irc()));
    connect(actionLive_Help_Chat, SIGNAL(triggered()), this, SLOT(slot_irc()));
    connect(actionShow_Map, SIGNAL(triggered()), this, SLOT(slot_mapper()));
#if !defined(INCLUDE_UPDATER)
    dactionUpdate->setVisible(false);
#endif
    connect(actionPackage_manager, SIGNAL(triggered()), this, SLOT(slot_package_manager()));
    connect(actionPackage_Exporter, SIGNAL(triggered()), this, SLOT(slot_package_exporter()));
    connect(actionModule_manager, SIGNAL(triggered()), this, SLOT(slot_module_manager()));
    connect(dactionMultiView, SIGNAL(triggered()), this, SLOT(slot_multi_view()));
    connect(dactionInputLine, &QAction::triggered, this, &mudlet::slot_toggle_compact_input_line);

    connect(mactionTriggers, SIGNAL(triggered()), this, SLOT(show_trigger_dialog()));
    connect(dactionScriptEditor, SIGNAL(triggered()), this, SLOT(show_trigger_dialog()));
    connect(mactionMapper, SIGNAL(triggered()), this, SLOT(slot_mapper()));
    connect(mactionTimers, SIGNAL(triggered()), this, SLOT(show_timer_dialog()));
    connect(mactionAlias, SIGNAL(triggered()), this, SLOT(show_alias_dialog()));
    connect(mactionScripts, SIGNAL(triggered()), this, SLOT(show_script_dialog()));
    connect(mactionKeys, SIGNAL(triggered()), this, SLOT(show_key_dialog()));
    connect(mactionButtons, SIGNAL(triggered()), this, SLOT(show_action_dialog()));

    connect(mactionOptions, SIGNAL(triggered()), this, SLOT(show_options_dialog()));
    connect(dactionOptions, SIGNAL(triggered()), this, SLOT(show_options_dialog()));

    connect(mactionAbout, SIGNAL(triggered()), this, SLOT(slot_show_about_dialog()));
    connect(dactionAbout, SIGNAL(triggered()), this, SLOT(slot_show_about_dialog()));

    connect(mactionMultiView, SIGNAL(triggered()), this, SLOT(slot_multi_view()));
    connect(mactionCloseProfile, SIGNAL(triggered()), this, SLOT(slot_close_profile()));

    mpSettings = getQSettings();
    readSettings(*mpSettings);

#if defined(INCLUDE_UPDATER)
    updater = new Updater(this, mpSettings);
    connect(dactionUpdate, &QAction::triggered, this, &mudlet::slot_check_manual_update);
#if defined(Q_OS_MACOS)
    // ensure that 'Check for updates' is under the Applications menu per convention
    dactionUpdate->setMenuRole(QAction::ApplicationSpecificRole);
#else
    connect(updater, &Updater::updateInstalled, this, &mudlet::slot_update_installed);
#endif // !Q_OS_MACOS
#endif // INCLUDE_UPDATER

    // mToolbarIconSize has been set to 0 in the initialisation list so either
    // value will be accepted:
    setToolBarIconSize(file_use_smallscreen.exists() ? 2 : 3);

    // Recover from a save in a state when both are hidden so that the toolbar
    // is shown
    // TODO: Provide a main console context menu item to restore at least one
    // of them - if both are hidden.
    if (!(mShowMenuBar || mShowToolbar)) {
        setToolBarVisible(true);
    }

#ifdef QT_GAMEPAD_LIB
    //connect(QGamepadManager::instance(), &QGamepadManager::gamepadButtonPressEvent, this, slot_gamepadButtonPress);
    //connect(QGamepadManager::instance(), &QGamepadManager::gamepadButtonReleaseEvent, this, slot_gamepadButtonRelease);
    //connect(QGamepadManager::instance(), &QGamepadManager::gamepadConnected, this, slot_gamepadConnected);
    //connect(QGamepadManager::instance(), &QGamepadManager::gamepadDisconnected, this, slot_gamepadDisconnected);
    //connect(QGamepadManager::instance(), &QGamepadManager::gamepadAxisEvent, this, slot_gamepadAxisEvent);
    connect(QGamepadManager::instance(),
            SIGNAL(gamepadButtonPressEvent(int, QGamepadManager::GamepadButton, double)),
            this,
            SLOT(slot_gamepadButtonPress(int, QGamepadManager::GamepadButton, double)));
    connect(QGamepadManager::instance(), SIGNAL(gamepadButtonReleaseEvent(int, QGamepadManager::GamepadButton)), this, SLOT(slot_gamepadButtonRelease(int, QGamepadManager::GamepadButton)));
    connect(QGamepadManager::instance(), SIGNAL(gamepadAxisEvent(int, QGamepadManager::GamepadAxis, double)), this, SLOT(slot_gamepadAxisEvent(int, QGamepadManager::GamepadAxis, double)));
    connect(QGamepadManager::instance(), SIGNAL(gamepadConnected(int)), this, SLOT(slot_gamepadConnected(int)));
    connect(QGamepadManager::instance(), SIGNAL(gamepadDisconnected(int)), this, SLOT(slot_gamepadDisconnected(int)));
#endif
    // Edbee has a singleton that needs some initialisation
    initEdbee();
}

QSettings* mudlet::getQSettings()
{
    /*In case sensitive environments, two different config directories
        were used: "Mudlet" for QSettings, and "mudlet" anywhere else.
        Furthermore, we skip the version from the application name to follow the convention.
        For compatibility with older settings, if no config is loaded
        from the config directory "mudlet", application "Mudlet", we try to load from the config
        directory "Mudlet", application "Mudlet 1.0". */
    QSettings settings_new("mudlet", "Mudlet");
    return new QSettings((settings_new.contains("pos") ? "mudlet" : "Mudlet"), (settings_new.contains("pos") ? "Mudlet" : "Mudlet 1.0"));
}

void mudlet::initEdbee()
{
    auto edbee = edbee::Edbee::instance();
    edbee->init();
    edbee->autoShutDownOnAppExit();

    auto grammarManager = edbee->grammarManager();
    // We only need the single Lua lexer, probably ever
    grammarManager->readGrammarFile(QLatin1Literal(":/edbee_defaults/Lua.tmLanguage"));

    loadEdbeeTheme(QStringLiteral("Mudlet"), QStringLiteral("Mudlet.tmTheme"));
}

bool mudlet::moduleTableVisible()
{
    if (moduleTable) {
        return moduleTable->isVisible();
    }
    return false;
}

void mudlet::layoutModules()
{
    if (!mpModuleTableHost) {
        return;
    }

    QMapIterator<QString, QStringList> it(mpModuleTableHost->mInstalledModules);
    QStringList sl;
    sl << tr("Module Name") << tr("Priority") << tr("Sync") << tr("Module Location");
    moduleTable->setHorizontalHeaderLabels(sl);
    moduleTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    moduleTable->verticalHeader()->hide();
    moduleTable->setShowGrid(true);
    //clear everything
    for (int i = 0; i <= moduleTable->rowCount(); i++) {
        moduleTable->removeRow(i);
    }
    //order modules by priority and then alphabetically
    QMap<int, QStringList> mOrder;
    while (it.hasNext()) {
        it.next();
        int priority = mpModuleTableHost->mModulePriorities[it.key()];
        if (mOrder.contains(priority)) {
            mOrder[priority].append(it.key());
        } else {
            mOrder[priority] = QStringList(it.key());
        }
    }
    QMapIterator<int, QStringList> it2(mOrder);
    while (it2.hasNext()) {
        it2.next();
        QStringList pModules = it2.value();
        pModules.sort();
        for (int i = 0; i < pModules.size(); i++) {
            int row = moduleTable->rowCount();
            moduleTable->insertRow(row);
            auto masterModule = new QTableWidgetItem();
            auto itemEntry = new QTableWidgetItem();
            auto itemLocation = new QTableWidgetItem();
            auto itemPriority = new QTableWidgetItem();
            masterModule->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            QStringList moduleInfo = mpModuleTableHost->mInstalledModules[pModules[i]];

            if (moduleInfo.at(1).toInt()) {
                masterModule->setCheckState(Qt::Checked);
            } else {
                masterModule->setCheckState(Qt::Unchecked);
            }
            masterModule->setText(QString());
            masterModule->setToolTip(QStringLiteral("<html><head/><body><p>%1</p></body></html>")
                                             .arg(tr("Checking this box will cause the module to be saved and <i>resynchronised</i> across all "
                                                     "sessions that share it when the <i>Save Profile</i> button is clicked in the Editor or if it "
                                                     "is saved at the end of the session.")));
            // Although there is now no text used here this may help to make the
            // checkbox more central in the column
            masterModule->setTextAlignment(Qt::AlignCenter);

            QString moduleName = pModules[i];
            itemEntry->setText(moduleName);
            itemEntry->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            itemLocation->setText(moduleInfo[0]);
            itemLocation->setToolTip(moduleInfo[0]);                          // show the full path in a tooltip, in case it doesn't fit in the table
            itemLocation->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled); // disallow editing of module path, because that is not saved
            itemPriority->setData(Qt::EditRole, mpModuleTableHost->mModulePriorities[moduleName]);
            moduleTable->setItem(row, 0, itemEntry);
            moduleTable->setItem(row, 1, itemPriority);
            moduleTable->setItem(row, 2, masterModule);
            moduleTable->setItem(row, 3, itemLocation);
        }
    }
    moduleTable->resizeColumnsToContents();
}

void mudlet::slot_module_manager()
{
    Host* pH = getActiveHost();
    if (!pH) {
        return;
    }

    if (!mpModuleDlg) {
        // No dialog open, note the profile concerned
        mpModuleTableHost = pH;

        QUiLoader loader;
        QFile file(":/ui/module_manager.ui");
        file.open(QFile::ReadOnly);
        mpModuleDlg = dynamic_cast<QDialog*>(loader.load(&file, this));
        file.close();

        if (!mpModuleDlg) {
            return;
        }

        moduleTable = mpModuleDlg->findChild<QTableWidget*>("moduleTable");
        moduleUninstallButton = mpModuleDlg->findChild<QPushButton*>("uninstallButton");
        moduleInstallButton = mpModuleDlg->findChild<QPushButton*>("installButton");
        moduleHelpButton = mpModuleDlg->findChild<QPushButton*>("helpButton");

        if (!moduleTable || !moduleUninstallButton || !moduleHelpButton) {
            return;
        }

        layoutModules();
        connect(moduleUninstallButton, SIGNAL(clicked()), this, SLOT(slot_uninstall_module()));
        connect(moduleInstallButton, SIGNAL(clicked()), this, SLOT(slot_install_module()));
        connect(moduleHelpButton, SIGNAL(clicked()), this, SLOT(slot_help_module()));
        connect(moduleTable, SIGNAL(itemClicked(QTableWidgetItem*)), this, SLOT(slot_module_clicked(QTableWidgetItem*)));
        connect(moduleTable, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(slot_module_changed(QTableWidgetItem*)));
        connect(mpModuleDlg, SIGNAL(destroyed()), this, SLOT(slot_module_manager_destroyed()));
        mpModuleDlg->setWindowTitle(tr("Module Manager - %1").arg(mpModuleTableHost->getName()));
        mpModuleDlg->setAttribute(Qt::WA_DeleteOnClose);
    }

    mpModuleDlg->raise();
    mpModuleDlg->show();
}

bool mudlet::openWebPage(const QString& path)
{
    if (path.isEmpty() || path.isNull()) {
        return false;
    }
    QUrl url(path, QUrl::TolerantMode);
    if (!url.isValid()) {
        return false;
    }
    return QDesktopServices::openUrl(url);
}

void mudlet::slot_help_module()
{
    if (!mpModuleTableHost) {
        return;
    }
    int cRow = moduleTable->currentRow();
    QTableWidgetItem* pI = moduleTable->item(cRow, 0);
    if (!pI) {
        return;
    }
    if (mpModuleTableHost->moduleHelp.value(pI->text()).contains(QLatin1String("helpURL")) && !mpModuleTableHost->moduleHelp.value(pI->text()).value(QLatin1String("helpURL")).isEmpty()) {
        if (!openWebPage(mpModuleTableHost->moduleHelp.value(pI->text()).value(QLatin1String("helpURL")))) {
            //failed first open, try for a module related path
            QTableWidgetItem* item = moduleTable->item(cRow, 3);
            QString itemPath = item->text();
            QStringList path = itemPath.split(QDir::separator());
            path.pop_back();
            path.append(QDir::separator());
            path.append(mpModuleTableHost->moduleHelp.value(pI->text()).value(QLatin1String("helpURL")));
            QString path2 = path.join(QString());
            if (!openWebPage(path2)) {
                moduleHelpButton->setDisabled(true);
            }
        }
    }
}


void mudlet::slot_module_clicked(QTableWidgetItem* pItem)
{
    if (!mpModuleTableHost) {
        return;
    }

    int i = pItem->row();

    QTableWidgetItem* entry = moduleTable->item(i, 0);
    QTableWidgetItem* checkStatus = moduleTable->item(i, 2);
    QTableWidgetItem* itemPriority = moduleTable->item(i, 1);
    QTableWidgetItem* itemPath = moduleTable->item(i, 3);
    qDebug() << itemPath->text();
    if (!entry || !checkStatus || !itemPriority || !mpModuleTableHost->mInstalledModules.contains(entry->text())) {
        moduleHelpButton->setDisabled(true);
        if (checkStatus) {
            checkStatus->setCheckState(Qt::Unchecked);
            checkStatus->setFlags(Qt::NoItemFlags);
        }
        return;
    }

    if (mpModuleTableHost->moduleHelp.contains(entry->text())) {
        moduleHelpButton->setDisabled((!mpModuleTableHost->moduleHelp.value(entry->text()).contains(QStringLiteral("helpURL"))
                                       || mpModuleTableHost->moduleHelp.value(entry->text()).value(QStringLiteral("helpURL")).isEmpty()));
    } else {
        moduleHelpButton->setDisabled(true);
    }
}

void mudlet::slot_module_changed(QTableWidgetItem* pItem)
{
    if (!mpModuleTableHost) {
        return;
    }

    int i = pItem->row();

    QStringList moduleStringList;
    QTableWidgetItem* entry = moduleTable->item(i, 0);
    QTableWidgetItem* checkStatus = moduleTable->item(i, 2);
    QTableWidgetItem* itemPriority = moduleTable->item(i, 1);
    if (!entry || !checkStatus || !itemPriority || !mpModuleTableHost->mInstalledModules.contains(entry->text())) {
        return;
    }
    moduleStringList = mpModuleTableHost->mInstalledModules.value(entry->text());
    if (checkStatus->checkState() == Qt::Checked) {
        moduleStringList[1] = QLatin1String("1");
    } else {
        moduleStringList[1] = QLatin1String("0");
    }
    mpModuleTableHost->mInstalledModules[entry->text()] = moduleStringList;
    mpModuleTableHost->mModulePriorities[entry->text()] = itemPriority->text().toInt();
}

void mudlet::slot_install_module()
{
    if (!mpModuleTableHost) {
        return;
    }

    QString fileName = QFileDialog::getOpenFileName(this, tr("Load Mudlet Module"), QDir::currentPath());
    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Load Mudlet Module:"), tr("Cannot read file %1:\n%2.").arg(fileName, file.errorString()));
        return;
    }

    mpModuleTableHost->installPackage(fileName, 1);
    for (int i = moduleTable->rowCount() - 1; i >= 0; --i) {
        moduleTable->removeRow(i);
    }

    layoutModules();
}

void mudlet::slot_uninstall_module()
{
    if (!mpModuleTableHost) {
        return;
    }

    int cRow = moduleTable->currentRow();
    QTableWidgetItem* pI = moduleTable->item(cRow, 0);
    if (pI) {
        mpModuleTableHost->uninstallPackage(pI->text(), 1);
    }
    for (int i = moduleTable->rowCount() - 1; i >= 0; --i) {
        moduleTable->removeRow(i);
    }
    layoutModules();
}

void mudlet::slot_package_manager()
{
    Host* pH = getActiveHost();
    if (!pH) {
        return;
    }

    if (!mpPackageManagerDlg) {
        QUiLoader loader;
        QFile file(":/ui/package_manager.ui");
        file.open(QFile::ReadOnly);
        mpPackageManagerDlg = dynamic_cast<QDialog*>(loader.load(&file, this));
        file.close();

        if (!mpPackageManagerDlg) {
            return;
        }

        packageList = mpPackageManagerDlg->findChild<QListWidget*>("packageList");
        uninstallButton = mpPackageManagerDlg->findChild<QPushButton*>("uninstallButton");
        installButton = mpPackageManagerDlg->findChild<QPushButton*>("installButton");

        if (!packageList || !uninstallButton) {
            return;
        }

        packageList->addItems(pH->mInstalledPackages);
        connect(uninstallButton, SIGNAL(clicked()), this, SLOT(slot_uninstall_package()));
        connect(installButton, SIGNAL(clicked()), this, SLOT(slot_install_package()));
        mpPackageManagerDlg->setWindowTitle(tr("Package Manager"));
        mpPackageManagerDlg->setAttribute(Qt::WA_DeleteOnClose);
    }

    mpPackageManagerDlg->raise();
    mpPackageManagerDlg->show();
}

void mudlet::slot_install_package()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Import Mudlet Package"), QDir::currentPath());
    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Import Mudlet Package:"), tr("Cannot read file %1:\n%2.").arg(fileName, file.errorString()));
        return;
    }

    Host* pH = getActiveHost();
    if (!pH) {
        return;
    }

    pH->installPackage(fileName, 0);
    packageList->clear();
    packageList->addItems(pH->mInstalledPackages);
}

void mudlet::slot_uninstall_package()
{
    Host* pH = getActiveHost();
    if (!pH) {
        return;
    }
    QListWidgetItem* pI = packageList->currentItem();
    if (pI) {
        pH->uninstallPackage(pI->text(), 0);
    }
    packageList->clear();
    packageList->addItems(pH->mInstalledPackages);
}

void mudlet::slot_package_exporter()
{
    Host* pH = getActiveHost();
    if (!pH) {
        return;
    }
    auto d = new dlgPackageExporter(this, pH);
    // don't show the dialog if the user cancelled the wizard
    if (d->filePath.isEmpty()) {
        return;
    }

    d->show();
}


void mudlet::slot_close_profile_requested(int tab)
{
    QString name = mpTabBar->tabText(tab);
    Host* pH = mHostManager.getHost(name);
    if (!pH) {
        return;
    }

    list<QPointer<TToolBar>> hostToolBarMap = pH->getActionUnit()->getToolBarList();
    QMap<QString, TDockWidget*>& dockWindowMap = mHostDockConsoleMap[pH];
    QMap<QString, TConsole*>& hostConsoleMap = mHostConsoleMap[pH];

    if (!pH->mpConsole->close()) {
        return;
    }

    pH->mpConsole->mUserAgreedToCloseConsole = true;
    pH->closingDown();

    // disconnect before removing objects from memory as sysDisconnectionEvent needs that stuff.
    pH->mTelnet.disconnect();

    pH->stopAllTriggers();
    pH->mpEditorDialog->close();
    for (auto consoleName : hostConsoleMap.keys()) {
        hostConsoleMap[consoleName]->close();
        hostConsoleMap.remove(consoleName);

        if (dockWindowMap.contains(consoleName)) {
            dockWindowMap[consoleName]->setAttribute(Qt::WA_DeleteOnClose);
            dockWindowMap[consoleName]->close();
            removeDockWidget(dockWindowMap[consoleName]);
            dockWindowMap.remove(consoleName);
        }
    }
    mHostDockConsoleMap.remove(pH);
    mHostConsoleMap.remove(pH);

    for (TToolBar* pTB : hostToolBarMap) {
        if (pTB) {
            pTB->setAttribute(Qt::WA_DeleteOnClose);
            pTB->deleteLater();
        }
    }

    // close IRC client window if it is open.
    if (mpIrcClientMap.contains(pH)) {
        mpIrcClientMap[pH]->setAttribute(Qt::WA_DeleteOnClose);
        mpIrcClientMap[pH]->deleteLater();
    }

    mConsoleMap[pH]->close();
    if (mTabMap.contains(pH->getName())) {
        mpTabBar->removeTab(tab);
        mConsoleMap.remove(pH);
        mTabMap.remove(pH->getName());
        // PLACEMARKER: Host destruction (1) - from close button on tab bar
        // Unfortunately the spaghetti nature of the code means that the profile
        // is also (maybe) saved (or not) in the TConsole::close() call prior to
        // here but because that is optional we cannot only force a "save"
        // operation in the profile preferences dialog for the Host specific
        // details BEFORE the save (so any changes make it into the save) -
        // instead we just have to accept that any profile changes will not be
        // saved if the preferences dialog is not closed before the profile is...
        int hostCount = mHostManager.getHostCount();
        emit signal_hostDestroyed(pH, --hostCount);
        mHostManager.deleteHost(pH->getName());
    }

    // hide the tab bar if we only have 1 or no tabs available. saves screen space.
    if (mConsoleMap.size() > 1) {
        mpTabBar->show();
    } else {
        mpTabBar->hide();
    }
}

void mudlet::slot_close_profile()
{
    if (mpCurrentActiveHost) {
        if (mConsoleMap.contains(mpCurrentActiveHost)) {
            Host* pH = mpCurrentActiveHost;
            if (pH) {
                list<QPointer<TToolBar>> hostTToolBarMap = pH->getActionUnit()->getToolBarList();
                QMap<QString, TDockWidget*>& dockWindowMap = mHostDockConsoleMap[pH];
                QMap<QString, TConsole*>& hostConsoleMap = mHostConsoleMap[pH];
                QString name = pH->getName();

                pH->closingDown();

                // disconnect before removing objects from memory as sysDisconnectionEvent needs that stuff.
                pH->mTelnet.disconnect();

                mpCurrentActiveHost->mpEditorDialog->close();
                for (auto consoleName : hostConsoleMap.keys()) {
                    hostConsoleMap[consoleName]->close();
                    hostConsoleMap.remove(consoleName);

                    if (dockWindowMap.contains(consoleName)) {
                        dockWindowMap[consoleName]->setAttribute(Qt::WA_DeleteOnClose);
                        dockWindowMap[consoleName]->close();
                        removeDockWidget(dockWindowMap[consoleName]);
                        dockWindowMap.remove(consoleName);
                    }
                }
                mHostDockConsoleMap.remove(pH);
                mHostConsoleMap.remove(pH);

                for (TToolBar* pTB : hostTToolBarMap) {
                    if (pTB) {
                        pTB->setAttribute(Qt::WA_DeleteOnClose);
                        pTB->deleteLater();
                    }
                }

                // close IRC client window if it is open.
                if (mpIrcClientMap.contains(pH)) {
                    mpIrcClientMap[pH]->setAttribute(Qt::WA_DeleteOnClose);
                    mpIrcClientMap[pH]->deleteLater();
                }

                mConsoleMap[pH]->close();
                if (mTabMap.contains(name)) {
                    mpTabBar->removeTab(mpTabBar->currentIndex());
                    mConsoleMap.remove(pH);
                    // PLACEMARKER: Host destruction (2) - normal case
                    int hostCount = mHostManager.getHostCount();
                    emit signal_hostDestroyed(pH, --hostCount);
                    mHostManager.deleteHost(name);
                    mTabMap.remove(name);
                }
                mpCurrentActiveHost = Q_NULLPTR;
            }
        }

    } else {
        disableToolbarButtons();
    }
}

void mudlet::slot_tab_changed(int tabID)
{
    if ((!mTabMap.contains(mpTabBar->tabText(tabID))) && (tabID != -1)) {
        mpCurrentActiveHost = nullptr;
        return;
    }

    if (mConsoleMap.contains(mpCurrentActiveHost)) {
        mpCurrentActiveHost->mpConsole->hide();
        QString host = mpTabBar->tabText(tabID);
        if (mTabMap.contains(host)) {
            mpCurrentActiveHost = mTabMap[host]->mpHost;
        } else {
            mpCurrentActiveHost = nullptr;
            return;
        }
    } else {
        if (mTabMap.size() > 0) {
            mpCurrentActiveHost = mTabMap.begin().value()->mpHost;
        } else {
            mpCurrentActiveHost = nullptr;
            return;
        }
    }

    if (!mpCurrentActiveHost || mConsoleMap.contains(mpCurrentActiveHost)) {
        if (!mpCurrentActiveHost) {
            return;
        }
        mpCurrentActiveHost->mpConsole->show();
        mpCurrentActiveHost->mpConsole->repaint();
        mpCurrentActiveHost->mpConsole->refresh();
        mpCurrentActiveHost->mpConsole->mpCommandLine->repaint();
        mpCurrentActiveHost->mpConsole->mpCommandLine->setFocus();
        mpCurrentActiveHost->mpConsole->show();

        int x = mpCurrentActiveHost->mpConsole->width();
        int y = mpCurrentActiveHost->mpConsole->height();
        QSize s = QSize(x, y);
        QResizeEvent event(s, s);
        QApplication::sendEvent(mpCurrentActiveHost->mpConsole, &event);
    } else {
        mpCurrentActiveHost = nullptr;
        return;
    }

    // update the window title for the currently selected profile
    setWindowTitle(mpCurrentActiveHost->getName() + " - " + version);
}

void mudlet::addConsoleForNewHost(Host* pH)
{
    if (mConsoleMap.contains(pH)) {
        return;
    }
    pH->mLogStatus = mAutolog;
    auto pConsole = new TConsole(pH, false);
    if (!pConsole) {
        return;
    }
    pH->mpConsole = pConsole;
    pConsole->setWindowTitle(pH->getName());
    pConsole->setObjectName(pH->getName());
    mConsoleMap[pH] = pConsole;
    int newTabID = mpTabBar->addTab(pH->getName());
    mTabMap[pH->getName()] = pConsole;
    if (mConsoleMap.size() > 1) {
        mpTabBar->show();
    } else {
        mpTabBar->hide();
    }
    //update the main window title when we spawn a new tab
    setWindowTitle(pH->getName() + " - " + version);

    mainPane->layout()->addWidget(pConsole);
    if (mpCurrentActiveHost) {
        mpCurrentActiveHost->mpConsole->hide();
    }
    mpCurrentActiveHost = pH;

    set_compact_input_line();
    if (pH->mLogStatus) {
        pConsole->logButton->click();
    }

    pConsole->show();

    auto pEditor = new dlgTriggerEditor(pH);
    pH->mpEditorDialog = pEditor;
    pEditor->fillout_form();

    pH->getActionUnit()->updateToolbar();
    QMap<QString, TDockWidget*> dockConsoleMap;
    mHostDockConsoleMap[mpCurrentActiveHost] = dockConsoleMap;
    QMap<QString, TConsole*> miniConsoleMap;
    mHostConsoleMap[mpCurrentActiveHost] = miniConsoleMap;
    QMap<QString, TLabel*> labelMap;
    mHostLabelMap[mpCurrentActiveHost] = labelMap;
    QList<QString> dockUpdateMap;
    mHostDockLayoutChangeMap[mpCurrentActiveHost] = dockUpdateMap;
    QList<TToolBar*> toolbarUpdateMap;
    mHostToolbarLayoutChangeMap[mpCurrentActiveHost] = toolbarUpdateMap;
    mpCurrentActiveHost->mpConsole->show();
    mpCurrentActiveHost->mpConsole->repaint();
    mpCurrentActiveHost->mpConsole->refresh();
    mpCurrentActiveHost->mpConsole->mpCommandLine->repaint();
    mpCurrentActiveHost->mpConsole->mpCommandLine->setFocus();
    mpCurrentActiveHost->mpConsole->show();
    mpTabBar->setCurrentIndex(newTabID);

    int x = mpCurrentActiveHost->mpConsole->width();
    int y = mpCurrentActiveHost->mpConsole->height();
    QSize s = QSize(x, y);
    QResizeEvent event(s, s);
    QApplication::sendEvent(mpCurrentActiveHost->mpConsole, &event);
}


void mudlet::slot_timer_fires()
{
    QTimer* pQT = (QTimer*)sender();
    if (!pQT) {
        return;
    }
    if (mTimerMap.contains(pQT)) {
        TTimer* pTT = mTimerMap[pQT];
        pTT->execute();
        if (pTT->checkRestart()) {
            pTT->start();
        }
    } else {
        qDebug() << "MUDLET CRITICAL ERROR: Timer not registered!";
    }
}

void mudlet::unregisterTimer(QTimer* pQT)
{
    if (mTimerMap.contains(pQT)) {
        mTimerMap.remove(pQT);
    } else {
        qDebug() << "MUDLET CRITICAL ERROR: trying to unregister Timer but it is not registered!";
    }
}

void mudlet::registerTimer(TTimer* pTT, QTimer* pQT)
{
    if (!mTimerMap.contains(pQT)) {
        mTimerMap[pQT] = pTT;
        connect(pQT, SIGNAL(timeout()), this, SLOT(slot_timer_fires()));
    }
}

void mudlet::disableToolbarButtons()
{
    mpMainToolBar->actions()[1]->setEnabled(false);
    mpMainToolBar->actions()[2]->setEnabled(false);
    mpMainToolBar->actions()[3]->setEnabled(false);
    mpMainToolBar->actions()[4]->setEnabled(false);
    mpMainToolBar->actions()[5]->setEnabled(false);
    mpMainToolBar->actions()[6]->setEnabled(false);
    mpMainToolBar->actions()[7]->setEnabled(false);
    mpMainToolBar->actions()[9]->setEnabled(false);
    mpMainToolBar->actions()[10]->setEnabled(false);
    mpMainToolBar->actions()[12]->setEnabled(false);
    mpMainToolBar->actions()[13]->setEnabled(false);
    mpMainToolBar->actions()[14]->setEnabled(false);
}

void mudlet::enableToolbarButtons()
{
    mpMainToolBar->actions()[1]->setEnabled(true);
    mpMainToolBar->actions()[2]->setEnabled(true);
    mpMainToolBar->actions()[3]->setEnabled(true);
    mpMainToolBar->actions()[4]->setEnabled(true);
    mpMainToolBar->actions()[5]->setEnabled(true);
    mpMainToolBar->actions()[6]->setEnabled(true);
    mpMainToolBar->actions()[7]->setEnabled(true);
    mpMainToolBar->actions()[9]->setEnabled(true);
    mpMainToolBar->actions()[10]->setEnabled(true);
    mpMainToolBar->actions()[12]->setEnabled(true);
    mpMainToolBar->actions()[13]->setEnabled(true);
    mpMainToolBar->actions()[14]->setEnabled(true);
}

bool mudlet::saveWindowLayout()
{
    qDebug() << "mudlet::saveWindowLayout() - Already-Saved:" << mHasSavedLayout;
    if (mHasSavedLayout) {
        return false;
    }

    QString layoutFilePath = getMudletPath(mainDataItemPath, QStringLiteral("windowLayout.dat"));

    QFile layoutFile(layoutFilePath);
    if (layoutFile.open(QIODevice::WriteOnly)) {
        // revert update markers to ready objects for saving.
        commitLayoutUpdates();

        QByteArray layoutData = saveState();
        QDataStream ofs(&layoutFile);
        ofs << layoutData;
        layoutFile.close();
        mHasSavedLayout = true;
        return true;
    } else {
        return false;
    }
}

bool mudlet::loadWindowLayout()
{
    if (mIsLoadingLayout) {
        qDebug() << "mudlet::loadWindowLayout() - already loading...";
        return false;
    }
    qDebug() << "mudlet::loadWindowLayout() - loading layout.";

    QString layoutFilePath = getMudletPath(mainDataItemPath, QStringLiteral("windowLayout.dat"));

    QFile layoutFile(layoutFilePath);
    if (layoutFile.exists()) {
        if (layoutFile.open(QIODevice::ReadOnly)) {
            mIsLoadingLayout = true;

            QByteArray layoutData;
            QDataStream ifs(&layoutFile);
            ifs >> layoutData;
            layoutFile.close();

            bool rv = restoreState(layoutData);

            mIsLoadingLayout = false;
            return rv;
        }
    }
    return false;
}

void mudlet::setDockLayoutUpdated(Host* pHost, const QString& name)
{
    if (!pHost) {
        return;
    }

    QMap<QString, TDockWidget*>& dockWindowMap = mHostDockConsoleMap[pHost];
    QList<QString>& mDockLayoutUpdateMap = mHostDockLayoutChangeMap[pHost];
    if (!mDockLayoutUpdateMap.contains(name) && dockWindowMap.contains(name)) {
        dockWindowMap[name]->setObjectName(QString("%1_changed").arg(dockWindowMap[name]->objectName()));
        mDockLayoutUpdateMap.append(name);

        mHasSavedLayout = false;
    }
}

void mudlet::setToolbarLayoutUpdated(Host* pHost, TToolBar* pTB)
{
    if (!pHost) {
        return;
    }

    QList<TToolBar*>& mToolbarLayoutUpdateMap = mHostToolbarLayoutChangeMap[pHost];
    if (!mToolbarLayoutUpdateMap.contains(pTB)) {
        pTB->setObjectName(QString("%1_changed").arg(pTB->objectName()));
        mToolbarLayoutUpdateMap.append(pTB);

        mHasSavedLayout = false;
    }
}

void mudlet::commitLayoutUpdates()
{
    // commit changes for dockwidget consoles. (user windows)
    for (Host* pHost : mHostDockLayoutChangeMap.keys()) {
        QMap<QString, TDockWidget*>& dockWindowMap = mHostDockConsoleMap[pHost];
        QList<QString>& mDockLayoutUpdateMap = mHostDockLayoutChangeMap[pHost];

        for (QString TDockName : mDockLayoutUpdateMap) {
            if (dockWindowMap.contains(TDockName)) {
                QString rename = QString("dockWindow_%1_%2").arg(pHost->getName(), TDockName);
                dockWindowMap[TDockName]->setObjectName(rename);
            }
            mDockLayoutUpdateMap.removeAll(TDockName);
        }
    }

    // commit changes for dockable/floating toolbars.
    for (Host* pHost : mHostToolbarLayoutChangeMap.keys()) {
        QList<TToolBar*>& mToolbarLayoutUpdateMap = mHostToolbarLayoutChangeMap[pHost];

        for (TToolBar* pTB : mToolbarLayoutUpdateMap) {
            pTB->setObjectName(QString("dockToolBar_%1").arg(pTB->getName()));
            mToolbarLayoutUpdateMap.removeAll(pTB);
        }
    }
}

bool mudlet::setFontSize(Host* pHost, const QString& name, int size)
{
    if (!pHost) {
        return false;
    }

    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];

    if (dockWindowConsoleMap.contains(name)) {
        TConsole* pC = dockWindowConsoleMap.value(name);
        pC->setMiniConsoleFontSize(size);

        return true;
    } else {
        return false;
    }
}

int mudlet::getFontSize(Host* pHost, const QString& name)
{
    if (!pHost) {
        return -1;
    }

    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];

    if (dockWindowConsoleMap.contains(name)) {
        return dockWindowConsoleMap.value(name)->console->mDisplayFont.pointSize();
    } else {
        return -1;
    }
}

bool mudlet::openWindow(Host* pHost, const QString& name, bool loadLayout)
{
    if (!pHost) {
        return false;
    }

    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    QMap<QString, TDockWidget*>& dockWindowMap = mHostDockConsoleMap[pHost];

    if (!dockWindowMap.contains(name) && !dockWindowConsoleMap.contains(name)) {
        auto pD = new TDockWidget(pHost, name);
        pD->setObjectName(QString("dockWindow_%1_%2").arg(pHost->getName(), name));
        pD->setContentsMargins(0, 0, 0, 0);
        pD->setFeatures(QDockWidget::AllDockWidgetFeatures);
        pD->setWindowTitle(QString("%1 - %2").arg(name, pHost->getName()));
        dockWindowMap[name] = pD;
        auto pC = new TConsole(pHost, false, pD);
        pC->setContentsMargins(0, 0, 0, 0);
        pD->setWidget(pC);
        pC->show();
        pC->layerCommandLine->hide();
        pC->mpScrollBar->hide();
        pC->setUserWindow();
        pC->console->setIsMiniConsole();
        pC->console2->setIsMiniConsole();
        dockWindowConsoleMap[name] = pC;
        addDockWidget(Qt::RightDockWidgetArea, pD);

        setFontSize(pHost, name, 10);

        if (loadLayout && !dockWindowMap[name]->hasLayoutAlready) {
            loadWindowLayout();
            dockWindowMap[name]->hasLayoutAlready = true;
        }

        return true;
    } else if (dockWindowMap.contains(name) && dockWindowConsoleMap.contains(name)) {
        dockWindowMap[name]->update();
        dockWindowMap[name]->show();
        dockWindowConsoleMap[name]->showWindow(name);

        if (loadLayout && !dockWindowMap[name]->hasLayoutAlready) {
            loadWindowLayout();
            dockWindowMap[name]->hasLayoutAlready = true;
        }

        return true;
    }

    return false;
}

bool mudlet::createMiniConsole(Host* pHost, const QString& name, int x, int y, int width, int height)
{
    if (!pHost) {
        return false;
    }
    if (!pHost->mpConsole) {
        return false;
    }

    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    if (!dockWindowConsoleMap.contains(name)) {
        TConsole* pC = pHost->mpConsole->createMiniConsole(name, x, y, width, height);
        if (pC) {
            pC->mConsoleName = name;
            dockWindowConsoleMap[name] = pC;
            std::string _n = name.toStdString();
            pC->setMiniConsoleFontSize(12);
            return true;
        }
    } else {
        TConsole* pC = dockWindowConsoleMap[name];
        pC->resize(width, height);
        pC->move(x, y);
    }
    return false;
}

bool mudlet::createLabel(Host* pHost, const QString& name, int x, int y, int width, int height, bool fillBg)
{
    if (!pHost) {
        return false;
    }
    if (!pHost->mpConsole) {
        return false;
    }
    QMap<QString, TLabel*>& labelMap = mHostLabelMap[pHost];
    if (!labelMap.contains(name)) {
        TLabel* pL = pHost->mpConsole->createLabel(name, x, y, width, height, fillBg);
        if (pL) {
            labelMap[name] = pL;
            return true;
        }
    }
    return false;
}

bool mudlet::createBuffer(Host* pHost, const QString& name)
{
    if (!pHost) {
        return false;
    }
    if (!pHost->mpConsole) {
        return false;
    }

    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    if (!dockWindowConsoleMap.contains(name)) {
        TConsole* pC = pHost->mpConsole->createBuffer(name);
        pC->mConsoleName = name;
        if (pC) {
            dockWindowConsoleMap[name] = pC;
            return true;
        }
    }
    return false;
}

bool mudlet::setBackgroundColor(Host* pHost, const QString& name, int r, int g, int b, int alpha)
{
    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    if (dockWindowConsoleMap.contains(name)) {
        dockWindowConsoleMap[name]->setConsoleBgColor(r, g, b);
        return true;
    } else {
        QMap<QString, TLabel*>& labelMap = mHostLabelMap[pHost];
        if (labelMap.contains(name)) {
            QPalette mainPalette;
            mainPalette.setColor(QPalette::Window, QColor(r, g, b, alpha));
            labelMap[name]->setPalette(mainPalette);
            return true;
        }
    }
    return false;
}

bool mudlet::setBackgroundImage(Host* pHost, const QString& name, QString& path)
{
    QMap<QString, TLabel*>& labelMap = mHostLabelMap[pHost];
    if (labelMap.contains(name)) {
        if (QDir::homePath().contains('\\')) {
            path.replace('/', R"(\)");
        } else {
            path.replace('\\', "/");
        }
        QPixmap bgPixmap(path);
        labelMap[name]->setPixmap(bgPixmap);
        return true;
    } else {
        return false;
    }
}

bool mudlet::setTextFormat(Host* pHost, const QString& name, int r1, int g1, int b1, int r2, int g2, int b2, bool bold, bool underline, bool italics, bool strikeout)
{
    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    if (dockWindowConsoleMap.contains(name)) {
        TConsole* pC = dockWindowConsoleMap[name];
        pC->mFormatCurrent.bgR = r1;
        pC->mFormatCurrent.bgG = g1;
        pC->mFormatCurrent.bgB = b1;
        pC->mFormatCurrent.fgR = r2;
        pC->mFormatCurrent.fgG = g2;
        pC->mFormatCurrent.fgB = b2;
        if (bold) {
            pC->mFormatCurrent.flags |= TCHAR_BOLD;
        } else {
            pC->mFormatCurrent.flags &= ~(TCHAR_BOLD);
        }
        if (underline) {
            pC->mFormatCurrent.flags |= TCHAR_UNDERLINE;
        } else {
            pC->mFormatCurrent.flags &= ~(TCHAR_UNDERLINE);
        }
        if (italics) {
            pC->mFormatCurrent.flags |= TCHAR_ITALICS;
        } else {
            pC->mFormatCurrent.flags &= ~(TCHAR_ITALICS);
        }
        if (strikeout) {
            pC->mFormatCurrent.flags |= TCHAR_STRIKEOUT;
        } else {
            pC->mFormatCurrent.flags &= ~(TCHAR_STRIKEOUT);
        }
        return true;
    } else {
        return false;
    }
}

void mudlet::showEvent(QShowEvent* event)
{
    mWindowMinimized = false;
    QMainWindow::showEvent(event);
}

void mudlet::hideEvent(QHideEvent* event)
{
    mWindowMinimized = true;
    QMainWindow::hideEvent(event);
}

bool mudlet::clearWindow(Host* pHost, const QString& name)
{
    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    if (dockWindowConsoleMap.contains(name)) {
        dockWindowConsoleMap[name]->buffer.clear();
        dockWindowConsoleMap[name]->console->update();
        return true;
    } else {
        return false;
    }
}

bool mudlet::showWindow(Host* pHost, const QString& name)
{
    if (!pHost) {
        return false;
    }
    if (!pHost->mpConsole) {
        return false;
    }

    // check labels first as they are shown/hidden more often
    QMap<QString, TLabel*>& labelMap = mHostLabelMap[pHost];
    if (labelMap.contains(name)) {
        labelMap[name]->show();
        return true;
    }

    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    if (dockWindowConsoleMap.contains(name)) {
        QMap<QString, TDockWidget*>& dockWindowMap = mHostDockConsoleMap[pHost];
        if (dockWindowMap.contains(name)) {
            dockWindowMap[name]->update();
            dockWindowMap[name]->show();
            dockWindowConsoleMap[name]->showWindow(name);

            return true;
        }

        return pHost->mpConsole->showWindow(name);
    }

    return false;
}

bool mudlet::paste(Host* pHost, const QString& name)
{
    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    if (dockWindowConsoleMap.contains(name)) {
        dockWindowConsoleMap[name]->paste();
        return true;
    } else {
        return false;
    }
}

bool mudlet::hideWindow(Host* pHost, const QString& name)
{
    if (!pHost) {
        return false;
    }
    if (!pHost->mpConsole) {
        return false;
    }

    // check labels first as they are shown/hidden more often
    QMap<QString, TLabel*>& labelMap = mHostLabelMap[pHost];
    if (labelMap.contains(name)) {
        labelMap[name]->hide();
        return true;
    }

    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    if (dockWindowConsoleMap.contains(name)) {
        QMap<QString, TDockWidget*>& dockWindowMap = mHostDockConsoleMap[pHost];
        if (dockWindowMap.contains(name)) {
            dockWindowMap[name]->hide();
            dockWindowMap[name]->update();
            dockWindowConsoleMap[name]->hideWindow(name);

            return true;
        }

        return pHost->mpConsole->hideWindow(name);
    }

    return false;
}

bool mudlet::resizeWindow(Host* pHost, const QString& name, int x1, int y1)
{
    QMap<QString, TLabel*>& labelMap = mHostLabelMap[pHost];
    if (labelMap.contains(name)) {
        labelMap[name]->resize(x1, y1);
        return true;
    }

    QMap<QString, TDockWidget*>& dockWindowMap = mHostDockConsoleMap[pHost];
    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    if (dockWindowConsoleMap.contains(name) && !dockWindowMap.contains(name)) {
        dockWindowConsoleMap[name]->resize(x1, y1);
        return true;
    }

    if (dockWindowMap.contains(name)) {
        if (!dockWindowMap[name]->isFloating()) {
            return false;
        }

        dockWindowMap[name]->resize(x1, y1);
        return true;
    }

    return false;
}

bool mudlet::setConsoleBufferSize(Host* pHost, const QString& name, int x1, int y1)
{
    if (name == "main") {
        pHost->mpConsole->buffer.setBufferSize(x1, y1);
        return true;
    }

    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];

    if (dockWindowConsoleMap.contains(name)) {
        (dockWindowConsoleMap[name]->buffer).setBufferSize(x1, y1);
        return true;
    } else {
        return false;
    }
}

bool mudlet::setScrollBarVisible(Host* pHost, const QString& name, bool isVisible)
{
    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];

    if (dockWindowConsoleMap.contains(name)) {
        dockWindowConsoleMap[name]->setScrollBarVisible(isVisible);
        return true;
    } else
        return false;
}

bool mudlet::resetFormat(Host* pHost, QString& name)
{
    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    if (dockWindowConsoleMap.contains(name)) {
        dockWindowConsoleMap[name]->reset();
        return true;
    } else {
        return false;
    }
}

bool mudlet::moveWindow(Host* pHost, const QString& name, int x1, int y1)
{
    QMap<QString, TLabel*>& labelMap = mHostLabelMap[pHost];
    if (labelMap.contains(name)) {
        labelMap[name]->move(x1, y1);
        return true;
    }

    QMap<QString, TDockWidget*>& dockWindowMap = mHostDockConsoleMap[pHost];
    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    if (dockWindowConsoleMap.contains(name) && !dockWindowMap.contains(name)) {
        dockWindowConsoleMap[name]->move(x1, y1);
        dockWindowConsoleMap[name]->mOldX = x1;
        dockWindowConsoleMap[name]->mOldY = y1;
        return true;
    }

    if (dockWindowMap.contains(name)) {
        if (!dockWindowMap[name]->isFloating()) {
            dockWindowMap[name]->setFloating(true);
        }

        dockWindowMap[name]->move(x1, y1);
        return true;
    }

    return false;
}

bool mudlet::closeWindow(Host* pHost, const QString& name)
{
    if (!pHost) {
        return false;
    }
    if (!pHost->mpConsole) {
        return false;
    }

    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    if (dockWindowConsoleMap.contains(name)) {
        QMap<QString, TDockWidget*>& dockWindowMap = mHostDockConsoleMap[pHost];
        if (dockWindowMap.contains(name)) {
            dockWindowMap[name]->hide();
            dockWindowMap[name]->update();
            dockWindowConsoleMap[name]->hideWindow(name);

            return true;
        }

        return pHost->mpConsole->hideWindow(name);
    } else {
        return false;
    }
}

bool mudlet::setLabelClickCallback(Host* pHost, const QString& name, const QString& func, const TEvent& pA)
{
    QMap<QString, TLabel*>& labelMap = mHostLabelMap[pHost];
    if (labelMap.contains(name)) {
        labelMap[name]->setClick(pHost, func, pA);
        return true;
    } else {
        return false;
    }
}

bool mudlet::setLabelDoubleClickCallback(Host* pHost, const QString& name, const QString& func, const TEvent& pA)
{
    QMap<QString, TLabel*>& labelMap = mHostLabelMap[pHost];
    if (labelMap.contains(name)) {
        labelMap[name]->setDoubleClick(pHost, func, pA);
        return true;
    } else {
        return false;
    }
}

bool mudlet::setLabelReleaseCallback(Host* pHost, const QString& name, const QString& func, const TEvent& pA)
{
    QMap<QString, TLabel*>& labelMap = mHostLabelMap[pHost];
    if (labelMap.contains(name)) {
        labelMap[name]->setRelease(pHost, func, pA);
        return true;
    } else {
        return false;
    }
}

bool mudlet::setLabelMoveCallback(Host* pHost, const QString& name, const QString& func, const TEvent& pA)
{
    QMap<QString, TLabel*>& labelMap = mHostLabelMap[pHost];
    if (labelMap.contains(name)) {
        labelMap[name]->setMove(pHost, func, pA);
        return true;
    } else {
        return false;
    }
}

bool mudlet::setLabelWheelCallback(Host* pHost, const QString& name, const QString& func, const TEvent& pA)
{
    QMap<QString, TLabel*>& labelMap = mHostLabelMap[pHost];
    if (labelMap.contains(name)) {
        labelMap[name]->setWheel(pHost, func, pA);
        return true;
    } else {
        return false;
    }
}

bool mudlet::setLabelOnEnter(Host* pHost, const QString& name, const QString& func, const TEvent& pA)
{
    QMap<QString, TLabel*>& labelMap = mHostLabelMap[pHost];
    if (labelMap.contains(name)) {
        labelMap[name]->setEnter(pHost, func, pA);
        return true;
    } else {
        return false;
    }
}

bool mudlet::setLabelOnLeave(Host* pHost, const QString& name, const QString& func, const TEvent& pA)
{
    QMap<QString, TLabel*>& labelMap = mHostLabelMap[pHost];
    if (labelMap.contains(name)) {
        labelMap[name]->setLeave(pHost, func, pA);
        return true;
    } else {
        return false;
    }
}

int mudlet::getLineNumber(Host* pHost, QString& name)
{
    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    if (dockWindowConsoleMap.contains(name)) {
        return dockWindowConsoleMap[name]->getLineNumber();
    } else {
        TDebug(QColor(Qt::white), QColor(Qt::red)) << "ERROR: window doesn't exist\n" >> 0;
    }
    return -1;
}

int mudlet::getColumnNumber(Host* pHost, QString& name)
{
    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    if (dockWindowConsoleMap.contains(name)) {
        return dockWindowConsoleMap[name]->getColumnNumber();
    } else {
        TDebug(QColor(Qt::white), QColor(Qt::red)) << "ERROR: window doesn't exist\n" >> 0;
    }
    return -1;
}


int mudlet::getLastLineNumber(Host* pHost, const QString& name)
{
    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    if (dockWindowConsoleMap.contains(name)) {
        return dockWindowConsoleMap[name]->getLastLineNumber();
    } else {
        TDebug(QColor(Qt::white), QColor(Qt::red)) << "ERROR: window doesn't exist\n" >> 0;
    }
    return -1;
}

bool mudlet::moveCursorEnd(Host* pHost, const QString& name)
{
    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    if (dockWindowConsoleMap.contains(name)) {
        dockWindowConsoleMap[name]->moveCursorEnd();
        return true;
    } else {
        return false;
    }
}

bool mudlet::moveCursor(Host* pHost, const QString& name, int x, int y)
{
    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    if (dockWindowConsoleMap.contains(name)) {
        return dockWindowConsoleMap[name]->moveCursor(x, y);
    } else {
        return false;
    }
}

void mudlet::deleteLine(Host* pHost, const QString& name)
{
    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    if (dockWindowConsoleMap.contains(name)) {
        dockWindowConsoleMap[name]->skipLine();
    }
}

void mudlet::insertText(Host* pHost, const QString& name, const QString& text)
{
    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    if (dockWindowConsoleMap.contains(name)) {
        dockWindowConsoleMap[name]->insertText(text);
    }
}

void mudlet::insertLink(Host* pHost, const QString& name, const QString& text, QStringList& func, QStringList& hint, bool customFormat)
{
    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    if (dockWindowConsoleMap.contains(name)) {
        dockWindowConsoleMap[name]->insertLink(text, func, hint, customFormat);
    }
}

void mudlet::replace(Host* pHost, const QString& name, const QString& text)
{
    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    if (dockWindowConsoleMap.contains(name)) {
        dockWindowConsoleMap[name]->replace(text);
    }
}

void mudlet::setLink(Host* pHost, const QString& name, const QString& linkText, QStringList& linkFunction, QStringList& linkHint)
{
    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    if (dockWindowConsoleMap.contains(name)) {
        TConsole* pC = dockWindowConsoleMap[name];
        pC->setLink(linkText, linkFunction, linkHint);
    }
}

void mudlet::setBold(Host* pHost, const QString& name, bool b)
{
    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    if (dockWindowConsoleMap.contains(name)) {
        TConsole* pC = dockWindowConsoleMap[name];
        pC->setBold(b);
    }
}

void mudlet::setItalics(Host* pHost, const QString& name, bool b)
{
    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    if (dockWindowConsoleMap.contains(name)) {
        TConsole* pC = dockWindowConsoleMap[name];
        pC->setItalics(b);
    }
}

void mudlet::setUnderline(Host* pHost, const QString& name, bool b)
{
    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    if (dockWindowConsoleMap.contains(name)) {
        TConsole* pC = dockWindowConsoleMap[name];
        pC->setUnderline(b);
    }
}

void mudlet::setStrikeOut(Host* pHost, const QString& name, bool b)
{
    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    if (dockWindowConsoleMap.contains(name)) {
        TConsole* pC = dockWindowConsoleMap[name];
        pC->setStrikeOut(b);
    }
}

void mudlet::setFgColor(Host* pHost, const QString& name, int r, int g, int b)
{
    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    if (dockWindowConsoleMap.contains(name)) {
        TConsole* pC = dockWindowConsoleMap[name];
        pC->setFgColor(r, g, b);
    }
}

void mudlet::setBgColor(Host* pHost, const QString& name, int r, int g, int b)
{
    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    if (dockWindowConsoleMap.contains(name)) {
        TConsole* pC = dockWindowConsoleMap[name];
        pC->setBgColor(r, g, b);
    }
}

int mudlet::selectString(Host* pHost, const QString& name, const QString& text, int num)
{
    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    if (dockWindowConsoleMap.contains(name)) {
        TConsole* pC = dockWindowConsoleMap[name];
        return pC->select(text, num);
    } else {
        return -1;
    }
}

int mudlet::selectSection(Host* pHost, const QString& name, int f, int t)
{
    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    if (dockWindowConsoleMap.contains(name)) {
        TConsole* pC = dockWindowConsoleMap[name];
        return pC->selectSection(f, t);
    } else {
        return -1;
    }
}

// Added a return value to indicate whether the given windows name was found
bool mudlet::deselect(Host* pHost, const QString& name)
{
    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    if (dockWindowConsoleMap.contains(name)) {
        TConsole* pC = dockWindowConsoleMap[name];
        pC->deselect();
        return true;
    } else {
        return false;
    }
}

bool mudlet::setWindowWrap(Host* pHost, const QString& name, int& wrap)
{
    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    QString wn = name;
    if (dockWindowConsoleMap.contains(name)) {
        dockWindowConsoleMap[wn]->setWrapAt(wrap);
        return true;
    } else {
        return false;
    }
}

bool mudlet::setWindowWrapIndent(Host* pHost, const QString& name, int& wrap)
{
    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    QString wn = name;
    if (dockWindowConsoleMap.contains(name)) {
        dockWindowConsoleMap[wn]->setIndentCount(wrap);
        return true;
    } else {
        return false;
    }
}

bool mudlet::echoWindow(Host* pHost, const QString& name, const QString& text)
{
    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    QString t = text;
    if (dockWindowConsoleMap.contains(name)) {
        dockWindowConsoleMap[name]->echoUserWindow(t);
        return true;
    }
    QMap<QString, TLabel*>& labelMap = mHostLabelMap[pHost];
    if (labelMap.contains(name)) {
        labelMap[name]->setText(t);
        return true;
    } else {
        return false;
    }
}

bool mudlet::echoLink(Host* pHost, const QString& name, const QString& text, QStringList& func, QStringList& hint, bool customFormat)
{
    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    if (dockWindowConsoleMap.contains(name)) {
        dockWindowConsoleMap[name]->echoLink(text, func, hint, customFormat);
        return true;
    } else {
        return false;
    }
}

bool mudlet::copy(Host* pHost, const QString& name)
{
    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    if (dockWindowConsoleMap.contains(name)) {
        dockWindowConsoleMap[name]->copy();
        return true;
    } else {
        return false;
    }
}

bool mudlet::pasteWindow(Host* pHost, const QString& name)
{
    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    if (dockWindowConsoleMap.contains(name)) {
        dockWindowConsoleMap[name]->pasteWindow(mConsoleMap[pHost]->mClipboard);
        return true;
    } else {
        return false;
    }
}

bool mudlet::appendBuffer(Host* pHost, const QString& name)
{
    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    if (dockWindowConsoleMap.contains(name)) {
        dockWindowConsoleMap[name]->appendBuffer(mConsoleMap[pHost]->mClipboard);
        return true;
    } else {
        return false;
    }
}

void mudlet::slot_userToolBar_hovered(QAction* pA)
{
    QStringList sL;
    if (pA->menu()) {
        pA->menu()->exec();
    }
}

void mudlet::slot_userToolBar_orientation_changed(Qt::Orientation dir)
{
}

Host* mudlet::getActiveHost()
{
    if (mConsoleMap.contains(mpCurrentActiveHost)) {
        return mpCurrentActiveHost;
    } else {
        return nullptr;
    }
}

void mudlet::addSubWindow(TConsole* pConsole)
{
    mainPane->layout()->addWidget(pConsole);
    pConsole->show(); //NOTE: this is important for Apple OSX otherwise the console isnt displayed
}

void mudlet::closeEvent(QCloseEvent* event)
{
    foreach (TConsole* pC, mConsoleMap) {
        if (!pC->close()) {
            event->ignore();
            return;
        } else {
            pC->mUserAgreedToCloseConsole = true;
        }
    }

    writeSettings();

    goingDown();
    if (mpDebugArea) {
        mpDebugArea->setAttribute(Qt::WA_DeleteOnClose);
        mpDebugArea->close();
    }
    foreach (TConsole* pC, mConsoleMap) {
        if (pC->mpHost->getName() != "default_host") {
            // disconnect before removing objects from memory as sysDisconnectionEvent needs that stuff.
            pC->mpHost->mTelnet.disconnect();

            // close script-editor
            if (pC->mpHost->mpEditorDialog) {
                pC->mpHost->mpEditorDialog->setAttribute(Qt::WA_DeleteOnClose);
                pC->mpHost->mpEditorDialog->close();
            }
            if (pC->mpHost->mpNotePad) {
                pC->mpHost->mpNotePad->save();
                pC->mpHost->mpNotePad->setAttribute(Qt::WA_DeleteOnClose);
                pC->mpHost->mpNotePad->close();
            }

            // close console
            pC->close();
        }
    }

    // pass the event on so dblsqd can perform an update
    // if automatic updates have been disabled
    event->accept();
}

void mudlet::forceClose()
{
    for (auto console : mConsoleMap) {
        auto host = console->getHost();
        host->saveProfile();
        console->mUserAgreedToCloseConsole = true;

        if (host->getName() != QStringLiteral("default_host")) {
            host->mTelnet.disconnect();
            // close script-editor
            if (host->mpEditorDialog) {
                host->mpEditorDialog->setAttribute(Qt::WA_DeleteOnClose);
                host->mpEditorDialog->close();
            }
            if (host->mpNotePad) {
                host->mpNotePad->save();
                host->mpNotePad->setAttribute(Qt::WA_DeleteOnClose);
                host->mpNotePad->close();
            }

            if (mpIrcClientMap.contains(host)) {
                mpIrcClientMap.value(host)->close();
            }
        }

        console->close();
    }

    writeSettings();

    close();
}

void mudlet::readSettings(const QSettings& settings)
{
    QPoint pos = settings.value("pos", QPoint(0, 0)).toPoint();
    QSize size = settings.value("size", QSize(750, 550)).toSize();
    // A sensible default has already been set up according to whether we are on
    // a netbook or not before this gets called so only change if there is a
    // setting stored:
    if (settings.contains("mainiconsize")) {
        setToolBarIconSize(settings.value("mainiconsize").toInt());
    }
    setEditorTreeWidgetIconSize(settings.value("tefoldericonsize", QVariant(3)).toInt());
    setMenuBarVisible(settings.value("showMenuBar", QVariant(false)).toBool());
    setToolBarVisible(settings.value("showToolbar", QVariant(true)).toBool());
    mEditorTextOptions = QTextOption::Flags(settings.value("editorTextOptions", QVariant(0)).toInt());

    mshowMapAuditErrors = settings.value("reportMapIssuesToConsole", QVariant(false)).toBool();
    mCompactInputLine = settings.value("compactInputLine", QVariant(false)).toBool();
    resize(size);
    move(pos);
    if (mShowMenuBar) {
        MenuBar->show();
    } else {
        MenuBar->hide();
    }
    if (mShowToolbar || !mShowMenuBar) {
        mpMainToolBar->show();
    } else {
        if (mShowMenuBar) {
            mpMainToolBar->hide();
        } else {
            mpMainToolBar->show();
        }
    }
    if (settings.value("maximized", false).toBool()) {
        showMaximized();
    }
}

void mudlet::setToolBarIconSize(const int s)
{
    if (mToolbarIconSize == s || s <= 0) {
        return;
    }

    mToolbarIconSize = s;
    mpMainToolBar->setIconSize(QSize(s * 8, s * 8));
    if (mToolbarIconSize > 2) {
        mpMainToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    } else {
        mpMainToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    }

    if (replayToolBar) {
        replayToolBar->setIconSize(mpMainToolBar->iconSize());
        replayToolBar->setToolButtonStyle(mpMainToolBar->toolButtonStyle());
    }
    emit signal_setToolBarIconSize(s);
}

void mudlet::setEditorTreeWidgetIconSize(const int s)
{
    if (mEditorTreeWidgetIconSize == s || s <= 0) {
        return;
    }

    mEditorTreeWidgetIconSize = s;
    emit signal_setTreeIconSize(s);
}

void mudlet::setMenuBarVisible(const bool state)
{
    mShowMenuBar = state;

    if (mShowMenuBar) {
        menuBar()->show();
    } else {
        menuBar()->hide();
    }
}

void mudlet::setToolBarVisible(const bool state)
{
    mShowToolbar = state;

    if (mShowToolbar) {
        mpMainToolBar->show();
    } else {
        mpMainToolBar->hide();
    }
}

void mudlet::writeSettings()
{
    /*In case sensitive environments, two different config directories
      were used: "Mudlet" for QSettings, and "mudlet" anywhere else. We change the QSettings directory to "mudlet".
      Furthermore, we skip the version from the application name to follow the convention.*/
    QSettings settings("mudlet", "Mudlet");
    settings.setValue("pos", pos());
    settings.setValue("size", size());
    settings.setValue("mainiconsize", mToolbarIconSize);
    settings.setValue("tefoldericonsize", mEditorTreeWidgetIconSize);
    settings.setValue("showMenuBar", mShowMenuBar);
    settings.setValue("showToolbar", mShowToolbar);
    settings.setValue("maximized", isMaximized());
    settings.setValue("editorTextOptions", static_cast<int>(mEditorTextOptions));
    settings.setValue("reportMapIssuesToConsole", mshowMapAuditErrors);
    settings.setValue("compactInputLine", mCompactInputLine);
}

void mudlet::slot_show_connection_dialog()
{
    auto pDlg = new dlgConnectionProfiles(this);
    connect(pDlg, SIGNAL(signal_establish_connection(QString, int)), this, SLOT(slot_connection_dlg_finished(QString, int)));
    pDlg->fillout_form();

    connect(pDlg, &QDialog::accepted, [=]() { enableToolbarButtons(); });
    pDlg->setAttribute(Qt::WA_DeleteOnClose);
    pDlg->show();
}

void mudlet::show_trigger_dialog()
{
    Host* pHost = getActiveHost();
    if (!pHost) {
        return;
    }
    dlgTriggerEditor* pEditor = pHost->mpEditorDialog;
    if (!pEditor) {
        return;
    }
    pEditor->slot_show_triggers();
    pEditor->raise();
    pEditor->showNormal();
}

void mudlet::show_alias_dialog()
{
    Host* pHost = getActiveHost();
    if (!pHost) {
        return;
    }
    dlgTriggerEditor* pEditor = pHost->mpEditorDialog;
    if (!pEditor) {
        return;
    }
    pEditor->slot_show_aliases();
    pEditor->raise();
    pEditor->showNormal();
}

void mudlet::show_timer_dialog()
{
    Host* pHost = getActiveHost();
    if (!pHost) {
        return;
    }
    dlgTriggerEditor* pEditor = pHost->mpEditorDialog;
    if (!pEditor) {
        return;
    }
    pEditor->slot_show_timers();
    pEditor->raise();
    pEditor->showNormal();
}

void mudlet::show_script_dialog()
{
    Host* pHost = getActiveHost();
    if (!pHost) {
        return;
    }
    dlgTriggerEditor* pEditor = pHost->mpEditorDialog;
    if (!pEditor) {
        return;
    }
    pEditor->slot_show_scripts();
    pEditor->raise();
    pEditor->showNormal();
}

void mudlet::show_key_dialog()
{
    Host* pHost = getActiveHost();
    if (!pHost) {
        return;
    }
    dlgTriggerEditor* pEditor = pHost->mpEditorDialog;
    if (!pEditor) {
        return;
    }
    pEditor->slot_show_keys();
    pEditor->raise();
    pEditor->showNormal();
}

void mudlet::show_variable_dialog()
{
    Host* pHost = getActiveHost();
    if (!pHost) {
        return;
    }
    dlgTriggerEditor* pEditor = pHost->mpEditorDialog;
    if (!pEditor) {
        return;
    }
    pEditor->slot_show_vars();
    pEditor->raise();
    pEditor->showNormal();
}

void mudlet::show_action_dialog()
{
    Host* pHost = getActiveHost();
    if (!pHost) {
        return;
    }
    dlgTriggerEditor* pEditor = pHost->mpEditorDialog;
    if (!pEditor) {
        return;
    }
    pEditor->slot_show_actions();
    pEditor->raise();
    pEditor->showNormal();
}

void mudlet::show_options_dialog()
{
    Host* pHost = getActiveHost();

    if (!mpProfilePreferencesDlg) {
        mpProfilePreferencesDlg = new dlgProfilePreferences(this, pHost);
        connect(actionReconnect, SIGNAL(triggered()), mpProfilePreferencesDlg->need_reconnect_for_data_protocol, SLOT(hide()));
        connect(dactionReconnect, SIGNAL(triggered()), mpProfilePreferencesDlg->need_reconnect_for_data_protocol, SLOT(hide()));
        connect(actionReconnect, SIGNAL(triggered()), mpProfilePreferencesDlg->need_reconnect_for_specialoption, SLOT(hide()));
        connect(dactionReconnect, SIGNAL(triggered()), mpProfilePreferencesDlg->need_reconnect_for_specialoption, SLOT(hide()));
        mpProfilePreferencesDlg->setAttribute(Qt::WA_DeleteOnClose);
    }
    mpProfilePreferencesDlg->raise();
    mpProfilePreferencesDlg->show();
}

void mudlet::show_help_dialog()
{
    QDesktopServices::openUrl(QUrl("https://wiki.mudlet.org/w/Manual:Contents"));
}

void mudlet::slot_show_help_dialog_video()
{
    QDesktopServices::openUrl(QUrl("https://www.mudlet.org/media/"));
}

void mudlet::slot_show_help_dialog_forum()
{
    QDesktopServices::openUrl(QUrl("https://forums.mudlet.org/"));
}

void mudlet::slot_show_help_dialog_irc()
{
    QDesktopServices::openUrl(QUrl("https://webchat.freenode.net/?channels=mudlet"));
}

void mudlet::slot_mapper()
{
    createMapper(true);
}

// Needed to extract into a separate method from slot_mapper() so that we can
// use it WITHOUT loading a file - at least for the TConsole::importMap(...)
// case that may need to create a map widget before it loads/imports a
// non-default (last saved map in profile's map directory.
void mudlet::createMapper(bool loadDefaultMap)
{
    Host* pHost = getActiveHost();
    if (!pHost) {
        return;
    }
    if (pHost->mpMap->mpMapper) {
        bool visStatus = pHost->mpMap->mpMapper->isVisible();
        if (pHost->mpMap->mpMapper->parentWidget()->inherits("QDockWidget")) {
            pHost->mpMap->mpMapper->parentWidget()->setVisible(!visStatus);
        }
        pHost->mpMap->mpMapper->setVisible(!visStatus);
        return;
    }

    pHost->mpDockableMapWidget = new QDockWidget(tr("Map - %1").arg(pHost->getName()));
    pHost->mpDockableMapWidget->setObjectName(QString("dockMap_%1").arg(pHost->getName()));
    pHost->mpMap->mpMapper = new dlgMapper(pHost->mpDockableMapWidget, pHost, pHost->mpMap.data()); //FIXME: mpHost definieren
    pHost->mpMap->mpM = pHost->mpMap->mpMapper->glWidget;
    pHost->mpDockableMapWidget->setWidget(pHost->mpMap->mpMapper);

    if (loadDefaultMap && pHost->mpMap->mpRoomDB->getRoomIDList().isEmpty()) {
        qDebug() << "mudlet::slot_mapper() - restore map case 3.";
        pHost->mpMap->pushErrorMessagesToFile(tr("Pre-Map loading(3) report"), true);
        QDateTime now(QDateTime::currentDateTime());
        if (pHost->mpMap->restore(QString())) {
            pHost->mpMap->audit();
            pHost->mpMap->mpMapper->mp2dMap->init();
            pHost->mpMap->mpMapper->updateAreaComboBox();
            pHost->mpMap->mpMapper->resetAreaComboBoxToPlayerRoomArea();
            pHost->mpMap->mpMapper->show();
        }

        pHost->mpMap->pushErrorMessagesToFile(tr("Loading map(3) at %1 report").arg(now.toString(Qt::ISODate)), true);

    } else {
        if (pHost->mpMap->mpMapper) {
            pHost->mpMap->mpMapper->show();
        }
    }
    addDockWidget(Qt::RightDockWidgetArea, pHost->mpDockableMapWidget);

    loadWindowLayout();

    check_for_mappingscript();
    TEvent mapOpenEvent;
    mapOpenEvent.mArgumentList.append(QLatin1String("mapOpenEvent"));
    mapOpenEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    pHost->raiseEvent(mapOpenEvent);
}

void mudlet::check_for_mappingscript()
{
    Host* pHost = getActiveHost();
    if (!pHost) {
        return;
    }

    if (!pHost->checkForMappingScript()) {
        QUiLoader loader;

        QFile file(":/ui/lacking_mapper_script.ui");
        file.open(QFile::ReadOnly);

        QDialog* dialog = dynamic_cast<QDialog*>(loader.load(&file, this));
        file.close();

        connect(dialog, SIGNAL(accepted()), this, SLOT(slot_open_mappingscripts_page()));

        dialog->show();
        dialog->raise();
        dialog->activateWindow();
    }
}

void mudlet::slot_open_mappingscripts_page()
{
    QDesktopServices::openUrl(QUrl("https://forums.mudlet.org/search.php?keywords=mapping+script&terms=all&author=&sc=1&sf=titleonly&sr=topics&sk=t&sd=d&st=0&ch=400&t=0&submit=Search"));
}

void mudlet::slot_show_about_dialog()
{
    if (!mpAboutDlg) {
        mpAboutDlg = new dlgAboutDialog(this);
        mpAboutDlg->setAttribute(Qt::WA_DeleteOnClose);
    }

    mpAboutDlg->raise();
    mpAboutDlg->show();
}

void mudlet::slot_notes()
{
    Host* pHost = getActiveHost();
    if (!pHost) {
        return;
    }
    dlgNotepad* pNotes = pHost->mpNotePad;
    if (!pNotes) {
        pHost->mpNotePad = new dlgNotepad(pHost);
        pNotes = pHost->mpNotePad;

        QTextCharFormat format;
        format.setFont(pHost->mDisplayFont);
        pNotes->notesEdit->setCurrentCharFormat(format);
        pNotes->restore();
        pNotes->setWindowTitle(tr("%1 - notes").arg(pHost->getName()));
        pNotes->setWindowIcon(QIcon(QStringLiteral(":/icons/mudlet_notepad.png")));
    }
    pNotes->raise();
    pNotes->show();
}

void mudlet::slot_irc()
{
    Host* pHost = getActiveHost();
    bool isDefaultHost = false;
    if (!pHost) {
        pHost = mpDefaultHost;
        isDefaultHost = true;
    }

    if (!mpIrcClientMap.contains(pHost)) {
        QPointer<dlgIRC> dlg = new dlgIRC(pHost);
        dlg->setDefaultHostClient(isDefaultHost);
        mpIrcClientMap[pHost] = dlg;
    }
    mpIrcClientMap.value(pHost)->raise();
    mpIrcClientMap.value(pHost)->show();
}

void mudlet::slot_reconnect()
{
    Host* pHost = getActiveHost();
    if (!pHost) {
        return;
    }
    pHost->mTelnet.connectIt(pHost->getUrl(), pHost->getPort());
}

void mudlet::slot_disconnect()
{
    Host* pHost = getActiveHost();
    if (!pHost) {
        return;
    }
    pHost->mTelnet.disconnect();
}

void mudlet::slot_replay()
{
    Host* pHost = getActiveHost();
    if (!pHost) {
        return;
    }

    QString home = getMudletPath(profileReplayAndLogFilesPath, pHost->getName());
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select Replay"), home, tr("*.dat"));
    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Select Replay"), tr("Cannot read file %1:\n%2.").arg(fileName, file.errorString()));
        return;
    }

    pHost->mTelnet.loadReplay(fileName);
}

void mudlet::printSystemMessage(Host* pH, const QString& s)
{
    mConsoleMap[pH]->printSystemMessage(s);
}

void mudlet::print(Host* pH, const QString& s)
{
    mConsoleMap[pH]->print(s);
}

QString mudlet::readProfileData(const QString& profile, const QString& item)
{
    QFile file(getMudletPath(profileDataItemPath, profile, item));
    file.open(QIODevice::ReadOnly);
    if (!file.exists()) {
        return "";
    }

    QDataStream ifs(&file);
    QString ret;

    ifs >> ret;
    file.close();
    return ret;
}

// this slot is called via a timer in the constructor of mudlet::mudlet()
void mudlet::startAutoLogin()
{
    QStringList hostList = QDir(getMudletPath(profilesPath)).entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    bool openedProfile = false;

    for (auto host : hostList) {
        QString val = readProfileData(host, QStringLiteral("autologin"));
        if (val.toInt() == Qt::Checked) {
            doAutoLogin(host);
            openedProfile = true;
        }
    }

    if (!openedProfile) {
        slot_show_connection_dialog();
    }
}

void mudlet::doAutoLogin(const QString& profile_name)
{
    if (profile_name.size() < 1) {
        return;
    }

    Host* pHost = mHostManager.getHost(profile_name);
    if (pHost) {
        pHost->mTelnet.connectIt(pHost->getUrl(), pHost->getPort());
        return;
    }

    // load an old profile if there is any
    // PLACEMARKER: Host creation (2) - autoload case
    if (mHostManager.addHost(profile_name, QString(), QString(), QString())) {
        pHost = mHostManager.getHost(profile_name);
        if (!pHost) {
            return;
        }
    } else {
        return;
    }

    LuaInterface* lI = pHost->getLuaInterface();
    lI->getVars(true);

    QString folder = getMudletPath(profileXmlFilesPath, profile_name);
    QDir dir(folder);
    dir.setSorting(QDir::Time);
    QStringList entries = dir.entryList(QDir::Files, QDir::Time);
    if (!entries.isEmpty()) {
        QFile file(QStringLiteral("%1/%2").arg(folder, entries.at(0)));
        file.open(QFile::ReadOnly | QFile::Text);
        XMLimport importer(pHost);
        qDebug() << "[LOADING PROFILE]:" << file.fileName();
        importer.importPackage(&file); // TODO: Missing false return value handler
    }

    pHost->setLogin(readProfileData(profile_name, QStringLiteral("login")));
    pHost->setPass(readProfileData(profile_name, QStringLiteral("password")));
    // For the first real host created the getHostCount() will return 2 because
    // there is already a "default_host"
    signal_hostCreated(pHost, mHostManager.getHostCount());
    slot_connection_dlg_finished(profile_name, 0);
    enableToolbarButtons();
}

///////////////////////////////////////////////////////////////////////////////
// these two callbacks are called from cTelnet::handleConnectedToServer()
void mudlet::slot_send_login()
{
    if (tempHostQueue.isEmpty()) {
        return;
    }
    Host* pHost = tempHostQueue.dequeue();
    QString login = pHost->getLogin();
    pHost->sendRaw(login);
}

void mudlet::slot_send_pass()
{
    if (tempHostQueue.isEmpty()) {
        return;
    }
    Host* pHost = tempHostQueue.dequeue();
    QString pass = pHost->getPass();
    pHost->sendRaw(pass);
}
//////////////////////////////////////////////////////////////////////////////


void mudlet::processEventLoopHack()
{
    QTimer::singleShot(1, this, SLOT(processEventLoopHack_timerRun()));
}

void mudlet::processEventLoopHack_timerRun()
{
    Host* pH = getActiveHost();
    if (!pH) {
        return;
    }
    pH->mpConsole->refresh();
}

void mudlet::slot_connection_dlg_finished(const QString& profile, int historyVersion)
{
    Host* pHost = getHostManager().getHost(profile);
    if (!pHost) {
        return;
    }
    pHost->mIsProfileLoadingSequence = true;
    addConsoleForNewHost(pHost);
    pHost->mBlockScriptCompile = false;
    pHost->mLuaInterpreter.loadGlobal();
    LuaInterface* lI = pHost->getLuaInterface();
    lI->getVars(true);
    pHost->getScriptUnit()->compileAll();
    pHost->mIsProfileLoadingSequence = false;

    //do modules here
    QMapIterator<QString, int> it(pHost->mModulePriorities);
    QMap<int, QStringList> moduleOrder;
    while (it.hasNext()) {
        it.next();
        QStringList moduleEntry = moduleOrder[it.value()];
        moduleEntry << it.key();
        moduleOrder[it.value()] = moduleEntry;
    }
    QMapIterator<int, QStringList> it2(moduleOrder);
    while (it2.hasNext()) {
        it2.next();
        QStringList modules = it2.value();
        for (int i = 0; i < modules.size(); i++) {
            QStringList entry = pHost->mInstalledModules[modules[i]];
            pHost->installPackage(entry[0], 1);
            //we repeat this step here b/c we use the same installPackage method for initial loading,
            //where we overwrite the globalSave flag.  This restores saved and loaded packages to their proper flag
            pHost->mInstalledModules[modules[i]] = entry;
        }
    }

    // install default packages
    for (int i = 0; i < packagesToInstallList.size(); i++) {
        pHost->installPackage(packagesToInstallList[i], 0);
    }

    packagesToInstallList.clear();

    TEvent event;
    event.mArgumentList.append(QLatin1String("sysLoadEvent"));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    pHost->raiseEvent(event);

    //NOTE: this is a potential problem if users connect by hand quickly
    //      and one host has a slower response time as the other one, but
    //      the worst that can happen is that they have to login manually.

    tempHostQueue.enqueue(pHost);
    tempHostQueue.enqueue(pHost);
    pHost->connectToServer();
}

void mudlet::slot_multi_view()
{
    QMapIterator<Host*, TConsole*> it(mConsoleMap);
    while (it.hasNext()) {
        it.next();
        it.value()->show();
    }
}


void mudlet::slot_toggle_compact_input_line()
{
    if (!mpCurrentActiveHost) {
        return;
    }

    auto buttons = mpCurrentActiveHost->mpConsole->mpButtonMainLayer;

    if (compactInputLine()) {
        buttons->show();
        dactionInputLine->setText(tr("Compact input line"));
        setCompactInputLine(false);
    } else {
        buttons->hide();
        dactionInputLine->setText(tr("Standard input line"));
        setCompactInputLine(true);
    }
}

void mudlet::set_compact_input_line()
{
    if (!mpCurrentActiveHost) {
        return;
    }

    auto buttons = mpCurrentActiveHost->mpConsole->mpButtonMainLayer;

    if (!compactInputLine()) {
        buttons->show();
        dactionInputLine->setText(tr("Compact input line"));
    } else {
        buttons->hide();
        dactionInputLine->setText(tr("Standard input line"));
    }
}

mudlet::~mudlet()
{
    mudlet::_self = nullptr;
}

void mudlet::toggleFullScreenView()
{
    if (isFullScreen()) {
        showNormal();
    } else {
        showFullScreen();
    }
}

void mudlet::replayStart()
{
    if (!mpMainToolBar) {
        return;
    }
    replayToolBar = new QToolBar(this);
    mReplaySpeed = 1;
    mReplayTime.setHMS(0, 0, 0, 1); // Since Qt5.0 adding anything to a zero
                                    // (invalid) time leaves the time value
                                    // STILL being regarded as invalid - so to
                                    // get a valid time we have to use a very
                                    // small, NON-zero time to initiase it...!
    replayTime = new QLabel(this);
    actionReplayTime = replayToolBar->addWidget(replayTime);

    replayToolBar->setIconSize(mpMainToolBar->iconSize());
    replayToolBar->setToolButtonStyle(mpMainToolBar->toolButtonStyle());

    actionReplaySpeedUp = new QAction(QIcon(QStringLiteral(":/icons/export.png")), tr("Faster"), this);
    actionReplaySpeedUp->setStatusTip(tr("Replay Speed Up"));
    replayToolBar->addAction(actionReplaySpeedUp);
    actionReplaySpeedUp->setObjectName(QStringLiteral("replay_speed_up_action"));
    replayToolBar->widgetForAction(actionReplaySpeedUp)->setObjectName(actionReplaySpeedUp->objectName());

    actionReplaySpeedDown = new QAction(QIcon(QStringLiteral(":/icons/import.png")), tr("Slower"), this);
    actionReplaySpeedDown->setStatusTip(tr("Replay Speed Down"));
    replayToolBar->addAction(actionReplaySpeedDown);
    actionReplaySpeedDown->setObjectName(QStringLiteral("replay_speed_down_action"));
    replayToolBar->widgetForAction(actionReplaySpeedDown)->setObjectName(actionReplaySpeedDown->objectName());
    replaySpeedDisplay = new QLabel(this);
    actionSpeedDisplay = replayToolBar->addWidget(replaySpeedDisplay);

    connect(actionReplaySpeedUp, SIGNAL(triggered()), this, SLOT(slot_replaySpeedUp()));
    connect(actionReplaySpeedDown, SIGNAL(triggered()), this, SLOT(slot_replaySpeedDown()));

    QString txt = "<font size=25><b>speed:";
    txt.append(QString::number(mReplaySpeed));
    txt.append("X</b></font>");
    replaySpeedDisplay->setText(txt);

    replayTimer = new QTimer(this);
    replayTimer->setInterval(1000);
    replayTimer->setSingleShot(false);
    connect(replayTimer, SIGNAL(timeout()), this, SLOT(slot_replayTimeChanged()));

    QString txt2 = "<font size=25><b>Time:";
    txt2.append(mReplayTime.toString(timeFormat));
    txt2.append("</b></font>");
    replayTime->setText(txt2);

    replaySpeedDisplay->show();
    replayTime->show();
    insertToolBar(mpMainToolBar, replayToolBar);
    replayToolBar->show();
    replayTimer->start();
}

void mudlet::slot_replayTimeChanged()
{
    QString txt2 = "<font size=25><b>Time:";
    txt2.append(mReplayTime.toString(timeFormat));
    txt2.append("</b></font>");
    replayTime->setText(txt2);
}

void mudlet::replayOver()
{
    if (!mpMainToolBar) {
        return;
    }
    if (!replayToolBar) {
        return;
    }

    if (actionReplaySpeedUp) {
        disconnect(actionReplaySpeedUp, SIGNAL(triggered()), this, SLOT(slot_replaySpeedUp()));
        disconnect(actionReplaySpeedDown, SIGNAL(triggered()), this, SLOT(slot_replaySpeedDown()));
        replayToolBar->removeAction(actionReplaySpeedUp);
        replayToolBar->removeAction(actionReplaySpeedDown);
        replayToolBar->removeAction(actionSpeedDisplay);
        removeToolBar(replayToolBar);
        actionReplaySpeedUp = nullptr;
        actionReplaySpeedDown = nullptr;
        actionSpeedDisplay = nullptr;
        actionReplayTime = nullptr;
        replayToolBar = nullptr;
    }
}

void mudlet::slot_replaySpeedUp()
{
    mReplaySpeed = mReplaySpeed * 2;
    QString txt = "<font size=25><b>speed:";
    txt.append(QString::number(mReplaySpeed));
    txt.append("X</b></font>");
    replaySpeedDisplay->setText(txt);
    replaySpeedDisplay->show();
}

void mudlet::slot_replaySpeedDown()
{
    mReplaySpeed = mReplaySpeed / 2;
    if (mReplaySpeed < 1) {
        mReplaySpeed = 1;
    }
    QString txt = "<font size=25><b>speed:";
    txt.append(QString::number(mReplaySpeed));
    txt.append("X</b></font>");
    replaySpeedDisplay->setText(txt);
    replaySpeedDisplay->show();
}

/* loop through and stop all sounds */
void mudlet::stopSounds()
{
    QListIterator<QMediaPlayer*> itMusicBox(mMusicBoxList);

    while (itMusicBox.hasNext()) {
        itMusicBox.next()->stop();
    }
}

void mudlet::playSound(QString s, int soundVolume)
{
    QPointer<Host> pHost = getActiveHost();
    if (!pHost) {
        return;
    }

    QListIterator<QMediaPlayer*> itMusicBox(mMusicBoxList);
    QMediaPlayer* pPlayer = nullptr;

    /* find first available inactive QMediaPlayer */
    while (itMusicBox.hasNext()) {
        QMediaPlayer* pTestPlayer = itMusicBox.next();

        if (pTestPlayer->state() != QMediaPlayer::PlayingState && pTestPlayer->mediaStatus() != QMediaPlayer::LoadingMedia) {
            pPlayer = pTestPlayer;
            break;
        }
    }

    /* no available QMediaPlayer, create a new one */
    if (!pPlayer) {
        pPlayer = new QMediaPlayer(this);


        if (!pPlayer) {
            /* It (should) be impossible to ever reach this */
            pHost->postMessage("\n[  ERROR  ]  - Unable to create new QMediaPlayer object\n");
            return;
        }

        mMusicBoxList.append(pPlayer);
    }

    // Remove any previous connection to the signal of this QMediaPlayer,
    // theoretically this might be movable to be within the lambda function of
    // the following connect(...) but that does seem a bit twisty and this works
    // well enough!
    disconnect(pPlayer, &QMediaPlayer::stateChanged, nullptr, nullptr);

    connect(pPlayer, &QMediaPlayer::stateChanged, [=](QMediaPlayer::State state) {
        if (state == QMediaPlayer::StoppedState) {
            TEvent soundFinished;
            soundFinished.mArgumentList.append("sysSoundFinished");
            soundFinished.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
            soundFinished.mArgumentList.append(pPlayer->media().canonicalUrl().fileName());
            soundFinished.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
            soundFinished.mArgumentList.append(pPlayer->media().canonicalUrl().path());
            soundFinished.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
            if (pHost) {
                // The host may have gone away if the sound was a long one
                // and we are multi-playing so we ought to test it...
                pHost->raiseEvent(soundFinished);
            }
        }
    });

    /* set volume and play sound */
    pPlayer->setMedia(QUrl::fromLocalFile(s));
    pPlayer->setVolume(soundVolume);
    pPlayer->play();
}

void mudlet::setEditorTextoptions(const bool isTabsAndSpacesToBeShown, const bool isLinesAndParagraphsToBeShown)
{
    mEditorTextOptions = QTextOption::Flags((isTabsAndSpacesToBeShown ? QTextOption::ShowTabsAndSpaces : 0) | (isLinesAndParagraphsToBeShown ? QTextOption::ShowLineAndParagraphSeparators : 0));
    emit signal_editorTextOptionsChanged(mEditorTextOptions);
}

// Originally a slot_ but it does not actually need to be - Slysven
void mudlet::requestProfilesToReloadMaps(QList<QString> affectedProfiles)
{
    emit signal_profileMapReloadRequested(affectedProfiles);
}

bool mudlet::unzip(const QString& archivePath, const QString& destination, const QDir& tmpDir)
{
    int err = 0;
    //from: https://gist.github.com/mobius/1759816
    struct zip_stat zs;
    struct zip_file* zf;
    zip_uint64_t bytesRead = 0;
    char buf[4096]; // Was 100 but that seems unduly stingy...!
    zip* archive = zip_open(archivePath.toStdString().c_str(), 0, &err);
    if (err != 0) {
        zip_error_to_str(buf, sizeof(buf), err, errno);
        return false;
    }

    // We now scan for directories first, and gather needed ones first, not
    // just relying on (zero length) archive entries ending in '/' as some
    // (possibly broken) archive building libraries seem to forget to
    // include them.
    QMap<QString, QString> directoriesNeededMap;
    //   Key is: relative path stored in archive
    // Value is: absolute path needed when extracting files
    for (zip_int64_t i = 0, total = zip_get_num_entries(archive, 0); i < total; ++i) {
        if (!zip_stat_index(archive, static_cast<zip_uint64_t>(i), 0, &zs)) {
            QString entryInArchive(QString::fromUtf8(zs.name));
            QString pathInArchive(entryInArchive.section(QLatin1Literal("/"), 0, -2));
            // TODO: We are supposed to validate the fields (except the
            // "valid" one itself) in zs before using them:
            // i.e. check that zs.name is valid ( zs.valid & ZIP_STAT_NAME )
            if (entryInArchive.endsWith(QLatin1Char('/'))) {
                if (!directoriesNeededMap.contains(pathInArchive)) {
                    directoriesNeededMap.insert(pathInArchive, pathInArchive);
                }
            } else {
                if (!pathInArchive.isEmpty() && !directoriesNeededMap.contains(pathInArchive)) {
                    directoriesNeededMap.insert(pathInArchive, pathInArchive);
                }
            }
        }
    }

    // Now create the needed directories:
    QMapIterator<QString, QString> itPath(directoriesNeededMap);
    while (itPath.hasNext()) {
        itPath.next();
        QString folderToCreate = QStringLiteral("%1%2").arg(destination, itPath.value());
        if (!tmpDir.exists(folderToCreate)) {
            if (!tmpDir.mkpath(folderToCreate)) {
                zip_close(archive);
                return false; // Abort reading rest of archive
            }
            tmpDir.refresh();
        }
    }

    // Now extract the files
    for (zip_int64_t i = 0, total = zip_get_num_entries(archive, 0); i < total; ++i) {
        // No need to check return value as we've already done it first time
        zip_stat_index(archive, static_cast<zip_uint64_t>(i), 0, &zs);
        QString entryInArchive(QString::fromUtf8(zs.name));
        if (!entryInArchive.endsWith(QLatin1Char('/'))) {
            // TODO: check that zs.size is valid ( zs.valid & ZIP_STAT_SIZE )
            zf = zip_fopen_index(archive, static_cast<zip_uint64_t>(i), 0);
            if (!zf) {
                zip_close(archive);
                return false;
            }

            QFile fd(QStringLiteral("%1%2").arg(destination, entryInArchive));

            if (!fd.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
                zip_fclose(zf);
                zip_close(archive);
                return false;
            }

            bytesRead = 0;
            zip_uint64_t bytesExpected = zs.size;
            while (bytesRead < bytesExpected && fd.error() == QFileDevice::NoError) {
                zip_int64_t len = zip_fread(zf, buf, sizeof(buf));
                if (len < 0) {
                    fd.close();
                    zip_fclose(zf);
                    zip_close(archive);
                    return false;
                }

                if (fd.write(buf, len) == -1) {
                    fd.close();
                    zip_fclose(zf);
                    zip_close(archive);
                    return false;
                }
                bytesRead += static_cast<zip_uint64_t>(len);
            }
            fd.close();
            zip_fclose(zf);
        }
    }

    err = zip_close(archive);
    if (err) {
        zip_error_to_str(buf, sizeof(buf), err, errno);
        return false;
    }

    return true;
}

// loads the needed edbee theme from disk for use
bool mudlet::loadEdbeeTheme(const QString& themeName, const QString& themeFile)
{
    auto edbee = edbee::Edbee::instance();
    auto themeManager = edbee->themeManager();

    // getMudletPath(...) needs the themeFile to determine if it is the
    // "default" which is stored in the resource file and not downloaded into
    // the cache:
    QString themeLocation(getMudletPath(editorWidgetThemePathFile, themeFile));
    auto result = themeManager->readThemeFile(themeLocation, themeName);
    if (result == nullptr) {
        qWarning() << themeManager->lastErrorMessage();
        return false;
    }

    return true;
}

#ifdef QT_GAMEPAD_LIB
void mudlet::slot_gamepadButtonPress(int deviceId, QGamepadManager::GamepadButton button, double value)
{
    Host* pH = getActiveHost();
    if (!pH) {
        return;
    }
    TEvent event;
    event.mArgumentList.append(QLatin1String("sysGamepadButtonPress"));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(QString::number(deviceId));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    event.mArgumentList.append(QString::number(button));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    event.mArgumentList.append(QString::number(value));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    pH->raiseEvent(event);
}

void mudlet::slot_gamepadButtonRelease(int deviceId, QGamepadManager::GamepadButton button)
{
    Host* pH = getActiveHost();
    if (!pH) {
        return;
    }
    TEvent event;
    event.mArgumentList.append(QLatin1String("sysGamepadButtonRelease"));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(QString::number(deviceId));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    event.mArgumentList.append(QString::number(button));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    pH->raiseEvent(event);
}

void mudlet::slot_gamepadConnected(int deviceId)
{
    Host* pH = getActiveHost();
    if (!pH) {
        return;
    }
    TEvent event;
    event.mArgumentList.append(QLatin1String("sysGamepadConnected"));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(QString::number(deviceId));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    pH->raiseEvent(event);
}

void mudlet::slot_gamepadDisconnected(int deviceId)
{
    Host* pH = getActiveHost();
    if (!pH) {
        return;
    }
    TEvent event;
    event.mArgumentList.append(QLatin1String("sysGamepadDisconnected"));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(QString::number(deviceId));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    pH->raiseEvent(event);
}

void mudlet::slot_gamepadAxisEvent(int deviceId, QGamepadManager::GamepadAxis axis, double value)
{
    Host* pH = getActiveHost();
    if (!pH) {
        return;
    }
    TEvent event;
    event.mArgumentList.append(QLatin1String("sysGamepadAxisMove"));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(QString::number(deviceId));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    event.mArgumentList.append(QString::number(axis));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    event.mArgumentList.append(QString::number(value));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    pH->raiseEvent(event);
}

#endif // #ifdef QT_GAMEPAD_LIB

void mudlet::slot_module_manager_destroyed()
{
    // Clear the record of the profile that had the module manager open so
    // another profile can use it...
    mpModuleTableHost = nullptr;
}

// Convenience helper - may aide things if we want to put files in a different
// place...!
QString mudlet::getMudletPath(const mudletPathType mode, const QString& extra1, const QString& extra2)
{
    switch (mode) {
    case mainPath:
        // The root of all mudlet data for the user - does not end in a '/'
        return QStringLiteral("%1/.config/mudlet").arg(QDir::homePath());
    case mainDataItemPath:
        // Takes one extra argument as a file (or directory) relating to
        // (profile independent) mudlet data - may end with a '/' if the extra
        // argument does:
        return QStringLiteral("%1/.config/mudlet/%2").arg(QDir::homePath(), extra1);
    case mainFontsPath:
        // (Added for 3.5.0) a revised location to store Mudlet provided fonts
        return QStringLiteral("%1/.config/mudlet/fonts").arg(QDir::homePath());
    case profilesPath:
        // The directory containing all the saved user's profiles - does not end
        // in '/'
        return QStringLiteral("%1/.config/mudlet/profiles").arg(QDir::homePath());
    case profileHomePath:
        // Takes one extra argument (profile name) that returns the base
        // directory for that profile - does NOT end in a '/' unless the
        // supplied profle name does:
        return QStringLiteral("%1/.config/mudlet/profiles/%2").arg(QDir::homePath(), extra1);
    case profileXmlFilesPath:
        // Takes one extra argument (profile name) that returns the directory
        // for the profile game save XML files - ends in a '/'
        return QStringLiteral("%1/.config/mudlet/profiles/%2/current/").arg(QDir::homePath(), extra1);
    case profileMapsPath:
        // Takes one extra argument (profile name) that returns the directory
        // for the profile game save maps files - does NOT end in a '/'
        return QStringLiteral("%1/.config/mudlet/profiles/%2/map").arg(QDir::homePath(), extra1);
    case profileDateTimeStampedMapPathFileName:
        // Takes two extra arguments (profile name, dataTime stamp) that returns
        // the pathFile name for a dateTime stamped map file:
        return QStringLiteral("%1/.config/mudlet/profiles/%2/map/%3map.dat").arg(QDir::homePath(), extra1, extra2);
    case profileMapPathFileName:
        // Takes two extra arguments (profile name, mapFileName) that returns
        // the pathFile name for any map file:
        return QStringLiteral("%1/.config/mudlet/profiles/%2/map/%3").arg(QDir::homePath(), extra1, extra2);
    case profileXmlMapPathFileName:
        // Takes one extra argument (profile name) that returns the pathFile
        // name for the downloaded IRE Server provided XML map:
        return QStringLiteral("%1/.config/mudlet/profiles/%2/map.xml").arg(QDir::homePath(), extra1);
    case profileDataItemPath:
        // Takes two extra arguments (profile name, data item) that gives a
        // path file name for, typically a data item stored as a single item
        // (binary) profile data) file (ideally these can be moved to a per
        // profile QSettings file but that is a future pipe-dream on my part
        // SlySven):
        return QStringLiteral("%1/.config/mudlet/profiles/%2/%3").arg(QDir::homePath(), extra1, extra2);
    case profilePackagePath:
        // Takes two extra arguments (profile name, package name) returns the
        // per profile directory used to store (unpacked) package contents
        // - ends with a '/':
        return QStringLiteral("%1/.config/mudlet/profiles/%2/%3/").arg(QDir::homePath(), extra1, extra2);
    case profilePackagePathFileName:
        // Takes two extra arguments (profile name, package name) returns the
        // filename of the XML file that contains the (per profile, unpacked)
        // package mudlet items in that package/module:
        return QStringLiteral("%1/.config/mudlet/profiles/%2/%3/%3.xml").arg(QDir::homePath(), extra1, extra2);
    case profileReplayAndLogFilesPath:
        // Takes one extra argument (profile name) that returns the directory
        // that contains replays (*.dat files) and logs (*.html or *.txt) files
        // for that profile - does NOT end in '/':
        return QStringLiteral("%1/.config/mudlet/profiles/%2/log").arg(QDir::homePath(), extra1);
    case profileLogErrorsFilePath:
        // Takes one extra argument (profile name) that returns the pathFileName
        // to the map auditing report file that is appended to each time a
        // map is loaded:
        return QStringLiteral("%1/.config/mudlet/profiles/%2/log/errors.txt").arg(QDir::homePath(), extra1);
    case editorWidgetThemePathFile:
        // Takes two extra arguments (profile name, theme name) that returns the
        // pathFileName of the theme file used by the edbee editor - also
        // handles the special case of the default theme "mudlet.thTheme" that
        // is carried internally in the resource file:
        if (extra1.compare(QStringLiteral("Mudlet.tmTheme"), Qt::CaseSensitive)) {
            // No match
            return QStringLiteral("%1/.config/mudlet/edbee/Colorsublime-Themes-master/themes/%2").arg(QDir::homePath(), extra1);
        } else {
            // Match - return path to copy held in resource file
            return QStringLiteral(":/edbee_defaults/Mudlet.tmTheme");
        }
    case editorWidgetThemeJsonFile:
        // Returns the pathFileName to the external JSON file needed to process
        // an edbee edtor widget theme:
        return QStringLiteral("%1/.config/mudlet/edbee/Colorsublime-Themes-master/themes.json").arg(QDir::homePath());
    case moduleBackupsPath:
        // Returns the directory used to store module backups that is used in
        // when saving/resyncing packages/modules - ends in a '/'
        return QStringLiteral("%1/.config/mudlet/moduleBackups/").arg(QDir::homePath());
    }
}

#if defined(INCLUDE_UPDATER)
void mudlet::checkUpdatesOnStart()
{
    updater->checkUpdatesOnStart();
}

void mudlet::slot_check_manual_update()
{
    updater->manuallyCheckUpdates();
}

void mudlet::slot_update_installed()
{
// can't comment out method entirely as moc chokes on it, so leave a stub
#if !defined(Q_OS_MACOS)
    // disable existing functionality to show the updates window
    dactionUpdate->disconnect(SIGNAL(triggered()));

    // rejig to restart Mudlet instead
    QObject::connect(dactionUpdate, &QAction::triggered, [=]() {
        forceClose();
        QProcess::startDetached(qApp->arguments()[0], qApp->arguments());
    });
    dactionUpdate->setText(QStringLiteral("Update installed - restart to apply"));
#endif // !Q_OS_MACOS
}

void mudlet::showChangelogIfUpdated()
{
    if (!updater->shouldShowChangelog()) {
        return;
    }

    updater->showChangelog();
}
#endif // INCLUDE_UPDATER

int mudlet::getColumnCount(Host* pHost, QString& name)
{
    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];

    if (!dockWindowConsoleMap.contains(name)) {
        TDebug(QColor(Qt::white), QColor(Qt::red)) << "ERROR: window doesn't exist\n" >> 0;
        return -1;
    }

    return dockWindowConsoleMap[name]->console->getColumnCount();
}

int mudlet::getRowCount(Host* pHost, QString& name)
{
    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];

    if (!dockWindowConsoleMap.contains(name)) {
        TDebug(QColor(Qt::white), QColor(Qt::red)) << "ERROR: window doesn't exist\n" >> 0;
        return -1;
    }

    return dockWindowConsoleMap[name]->console->getRowCount();
}
