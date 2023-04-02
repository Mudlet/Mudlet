/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2013-2023 by Stephen Lyons - slysven@virginmedia.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016 by Chris Leacy - cleacy1972@gmail.com              *
 *   Copyright (C) 2016-2018 by Ian Adkins - ieadkins@gmail.com            *
 *   Copyright (C) 2017 by Tom Scheper - scheper@gmail.com                 *
 *   Copyright (C) 2011-2021 by Vadim Peretokin - vperetokin@gmail.com     *
 *   Copyright (C) 2022 by Thiago Jung Bauermann - bauermann@kolabnow.com  *
 *   Copyright (C) 2023 by Lecker Kebap - Leris@mudlet.org                 *
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

#include "AltFocusMenuBarDisable.h"
#include "EAction.h"
#include "LuaInterface.h"
#include "TCommandLine.h"
#include "TConsole.h"
#include "TDebug.h"
#include "TDockWidget.h"
#include "TEvent.h"
#include "TLabel.h"
#include "TMainConsole.h"
#include "TMap.h"
#include "TGameDetails.h"
#include "TRoomDB.h"
#include "TTabBar.h"
#include "TTextEdit.h"
#include "TToolBar.h"
#include "XMLimport.h"
#include "DarkTheme.h"
#include "dlgAboutDialog.h"
#include "dlgConnectionProfiles.h"
#include "dlgIRC.h"
#include "dlgMapper.h"
#include "dlgModuleManager.h"
#include "dlgNotepad.h"
#include "dlgPackageExporter.h"
#include "dlgPackageManager.h"
#include "dlgProfilePreferences.h"
#include "dlgTriggerEditor.h"
#include "VarUnit.h"

#include "pre_guard.h"
#include <QApplication>
#include <QtUiTools/quiloader.h>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QFile>
#include <QFileDialog>
#include <QJsonDocument>
#include <QImage>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkDiskCache>
#include <QMediaPlayer>
#include <QMessageBox>
#include <QNetworkDiskCache>
#include <QScrollBar>
#include <QShortcut>
#include <QSplitter>
#include <QStyleFactory>
#include <QTableWidget>
#include <QTextStream>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QToolTip>
#include <QVariantHash>
#include <QRandomGenerator>
#include <zip.h>
#include <QStyle>
#if defined(Q_OS_WIN32)
#include <QSettings>
#endif

#if defined(Q_OS_MAC)
// wrap in namespace since `Collection` defined in these headers will clash with Boost
namespace coreMacOS {
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
}
#endif

#include "post_guard.h"

using namespace std::chrono_literals;

bool TConsoleMonitor::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::Close) {
        mudlet::smDebugMode = false;
        mudlet::self()->refreshTabBar();
        return QObject::eventFilter(obj, event);
    } else {
        return QObject::eventFilter(obj, event);
    }
}

/*static*/ void mudlet::start()
{
    smpSelf = new mudlet;
}

/*static*/ mudlet* mudlet::self()
{
    return smpSelf;
}

mudlet::mudlet()
: QMainWindow()
{
    smFirstLaunch = !QFile::exists(mudlet::getMudletPath(mudlet::profilesPath));

    mShowIconsOnMenuOriginally = !qApp->testAttribute(Qt::AA_DontShowIconsInMenus);
    mpSettings = getQSettings();
    readEarlySettings(*mpSettings);
    if (mShowIconsOnMenuCheckedState != Qt::PartiallyChecked) {
        // If the setting is not the "tri-state" one then force the setting,
        // have to invert the sense because the attribute is a negative one:
        qApp->setAttribute(Qt::AA_DontShowIconsInMenus, (mShowIconsOnMenuCheckedState == Qt::Unchecked));
    }

    qApp->setAttribute(Qt::AA_UseHighDpiPixmaps);
    // We need to record this before we clobber it with our own substitute...
    mDefaultStyle = qApp->style()->objectName();
    // ... which is applied here:
    setAppearance(mAppearance, true);

    scanForMudletTranslations(qsl(":/lang"));
    scanForQtTranslations(getMudletPath(qtTranslationsPath));
    loadTranslators(mInterfaceLanguage);

    // Cannot assign a value in the constructor list as it requires the
    // translations to be loaded first:
    mTimeFormat = tr("hh:mm:ss",
                     // Intentional comment to separate arguments
                     "Formatting string for elapsed time display in replay playback - see QDateTime::toString(const QString&) for the gory details...!");

    if (QStringList{qsl("windowsvista"), qsl("macintosh")}.contains(mDefaultStyle, Qt::CaseInsensitive)) {
        qDebug().nospace().noquote() << "mudlet::mudlet() INFO - '" << mDefaultStyle << "' has been detected as the style factory in use - QPushButton styling fix applied!";
        mBG_ONLY_STYLESHEET = qsl("QPushButton {background-color: %1; border: 1px solid #8f8f91;}");
        mTEXT_ON_BG_STYLESHEET = qsl("QPushButton {color: %1; background-color: %2; border: 1px solid #8f8f91;}");
    } else {
        qDebug().nospace().noquote() << "mudlet::mudlet() INFO - '" << mDefaultStyle << "' has been detected as the style factory in use - no styling fixes applied.";
        mBG_ONLY_STYLESHEET = qsl("QPushButton {background-color: %1;}");
        mTEXT_ON_BG_STYLESHEET = qsl("QPushButton {color: %1; background-color: %2;}");
    }

    setupUi(this);
    setUnifiedTitleAndToolBarOnMac(true);
    setContentsMargins(0, 0, 0, 0);
    menuGames->setToolTipsVisible(true);
    menuEditor->setToolTipsVisible(true);
    menuOptions->setToolTipsVisible(true);
    menuHelp->setToolTipsVisible(true);
    menuAbout->setToolTipsVisible(true);

    setAttribute(Qt::WA_DeleteOnClose);
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setWindowTitle(scmVersion);
    if (scmIsReleaseVersion) {
        setWindowIcon(QIcon(qsl(":/icons/mudlet.png")));
    } else if (scmIsPublicTestVersion) {
        setWindowIcon(QIcon(qsl(":/icons/mudlet_ptb_256px.png")));
    } else { // scmIsDevelopmentVersion
        setWindowIcon(QIcon(qsl(":/icons/mudlet_dev_256px.png")));
    }
    mpMainToolBar = new QToolBar(this);
    mpMainToolBar->setObjectName(qsl("mpMainToolBar"));
    mpMainToolBar->setWindowTitle(tr("Main Toolbar"));
    addToolBar(mpMainToolBar);
    mpMainToolBar->setMovable(false);
    addToolBarBreak();
    auto frame = new QWidget(this);
    setCentralWidget(frame);
    mpTabBar = new TTabBar(frame);
    mpTabBar->setMaximumHeight(30);
    mpTabBar->setFocusPolicy(Qt::NoFocus);
    mpTabBar->setTabsClosable(true);
    mpTabBar->setAutoHide(true);
    connect(mpTabBar, &QTabBar::tabCloseRequested, this, &mudlet::slot_closeProfileRequested);
    mpTabBar->setMovable(true);
    // This only reports changing the tab by the user clicking on the tab
    connect(mpTabBar, &QTabBar::currentChanged, this, &mudlet::slot_tabChanged);
    connect(mpTabBar, &QTabBar::tabMoved, this, &mudlet::slot_tabMoved);
    auto layoutTopLevel = new QVBoxLayout(frame);
    layoutTopLevel->setContentsMargins(0, 0, 0, 0);
    layoutTopLevel->addWidget(mpTabBar);
    mpWidget_profileContainer = new QWidget(frame);
    QPalette mainPalette;
    mpWidget_profileContainer->setPalette(mainPalette);
    mpWidget_profileContainer->setContentsMargins(0, 0, 0, 0);
    mpWidget_profileContainer->setSizePolicy(sizePolicy);
    mpWidget_profileContainer->setAutoFillBackground(true);

    layoutTopLevel->addWidget(mpWidget_profileContainer);
    mpHBoxLayout_profileContainer = new QHBoxLayout(mpWidget_profileContainer);
    mpHBoxLayout_profileContainer->setContentsMargins(0, 0, 0, 0);

    mpSplitter_profileContainer = new QSplitter(Qt::Horizontal, mpWidget_profileContainer);
    mpSplitter_profileContainer->setContentsMargins(0, 0, 0, 0);
    mpSplitter_profileContainer->setChildrenCollapsible(false);

    mpHBoxLayout_profileContainer->addWidget(mpSplitter_profileContainer);

    mpButtonConnect = new QToolButton(this);
    mpButtonConnect->setText(tr("Connect"));
    mpButtonConnect->setObjectName(qsl("connect"));
    mpButtonConnect->setContextMenuPolicy(Qt::ActionsContextMenu);
    mpButtonConnect->setPopupMode(QToolButton::MenuButtonPopup);
    mpButtonConnect->setAutoRaise(true);
    mpMainToolBar->addWidget(mpButtonConnect);

    mpActionConnect = new QAction(tr("Connect"), this);
    mpActionConnect->setIcon(QIcon(qsl(":/icons/preferences-web-browser-cache.png")));
    mpActionConnect->setIconText(tr("Connect"));
    mpActionConnect->setObjectName(qsl("connect"));

    mpActionDisconnect = new QAction(tr("Disconnect"), this);
    mpActionDisconnect->setObjectName(qsl("disconnect"));

    mpActionCloseProfile = new QAction(tr("Close profile"), this);
    mpActionCloseProfile->setIcon(QIcon(qsl(":/icons/profile-close.png")));
    mpActionCloseProfile->setIconText(tr("Close profile"));
    mpActionCloseProfile->setObjectName(qsl("close_profile"));

    mpButtonConnect->addAction(mpActionConnect);
    mpButtonConnect->addAction(mpActionDisconnect);
    mpButtonConnect->addAction(mpActionCloseProfile);
    mpButtonConnect->setDefaultAction(mpActionConnect);

    mpActionTriggers = new QAction(QIcon(qsl(":/icons/tools-wizard.png")), tr("Triggers"), this);
    mpActionTriggers->setToolTip(utils::richText(tr("Show and edit triggers")));
    mpMainToolBar->addAction(mpActionTriggers);
    mpActionTriggers->setObjectName(qsl("triggers_action"));
    // add name to the action's widget in the toolbar, which doesn't have one by default
    // see https://stackoverflow.com/a/32460562/72944
    mpMainToolBar->widgetForAction(mpActionTriggers)->setObjectName(mpActionTriggers->objectName());

    mpActionAliases = new QAction(QIcon(qsl(":/icons/system-users.png")), tr("Aliases"), this);
    mpActionAliases->setToolTip(utils::richText(tr("Show and edit aliases")));
    mpMainToolBar->addAction(mpActionAliases);
    mpActionAliases->setObjectName(qsl("aliases_action"));
    mpMainToolBar->widgetForAction(mpActionAliases)->setObjectName(mpActionAliases->objectName());

    mpActionTimers = new QAction(QIcon(qsl(":/icons/chronometer.png")), tr("Timers"), this);
    mpActionTimers->setToolTip(utils::richText(tr("Show and edit timers")));
    mpMainToolBar->addAction(mpActionTimers);
    mpActionTimers->setObjectName(qsl("timers_action"));
    mpMainToolBar->widgetForAction(mpActionTimers)->setObjectName(mpActionTimers->objectName());

    mpActionButtons = new QAction(QIcon(qsl(":/icons/bookmarks.png")), tr("Buttons"), this);
    mpActionButtons->setToolTip(utils::richText(tr("Show and edit easy buttons")));
    mpMainToolBar->addAction(mpActionButtons);
    mpActionButtons->setObjectName(qsl("buttons_action"));
    mpMainToolBar->widgetForAction(mpActionButtons)->setObjectName(mpActionButtons->objectName());

    mpActionScripts = new QAction(QIcon(qsl(":/icons/document-properties.png")), tr("Scripts"), this);
    mpActionScripts->setToolTip(utils::richText(tr("Show and edit scripts")));
    mpMainToolBar->addAction(mpActionScripts);
    mpActionScripts->setObjectName(qsl("scripts_action"));
    mpMainToolBar->widgetForAction(mpActionScripts)->setObjectName(mpActionScripts->objectName());

    mpActionKeys = new QAction(QIcon(qsl(":/icons/preferences-desktop-keyboard.png")), tr("Keys"), this);
    mpActionKeys->setToolTip(utils::richText(tr("Show and edit keys")));
    mpMainToolBar->addAction(mpActionKeys);
    mpActionKeys->setObjectName(qsl("keys_action"));
    mpMainToolBar->widgetForAction(mpActionKeys)->setObjectName(mpActionKeys->objectName());

    mpActionVariables = new QAction(QIcon(qsl(":/icons/variables.png")), tr("Variables"), this);
    mpActionVariables->setToolTip(utils::richText(tr("Show and edit Lua variables")));
    mpMainToolBar->addAction(mpActionVariables);
    mpActionVariables->setObjectName(qsl("variables_action"));
    mpMainToolBar->widgetForAction(mpActionVariables)->setObjectName(mpActionVariables->objectName());

    mpButtonDiscord = new QToolButton(this);
    mpButtonDiscord->setText(qsl("Discord"));
    mpButtonDiscord->setObjectName(qsl("discord"));
    mpButtonDiscord->setContextMenuPolicy(Qt::ActionsContextMenu);
    mpButtonDiscord->setPopupMode(QToolButton::MenuButtonPopup);
    mpButtonDiscord->setAutoRaise(true);
    mpMainToolBar->addWidget(mpButtonDiscord);

    mpActionDiscord = new QAction(tr("Open Discord"), this);
    mpActionDiscord->setIcon(QIcon(qsl(":/icons/Discord-Logo-Color.png")));
    mpActionDiscord->setIconText(qsl("Discord"));
    mpActionDiscord->setObjectName(qsl("openDiscord"));

    mpActionMudletDiscord = new QAction(QIcon(qsl(":/icons/mudlet_discord.png")), tr("Mudlet chat"), this);
    mpActionMudletDiscord->setToolTip(utils::richText(tr("Open a link to the Mudlet server on Discord")));
    mpMainToolBar->addAction(mpActionMudletDiscord);
    mpActionMudletDiscord->setObjectName(qsl("mudlet_discord"));
    mpMainToolBar->widgetForAction(mpActionMudletDiscord)->setObjectName(mpActionMudletDiscord->objectName());
    mpActionMudletDiscord->setVisible(false); // Mudlet Discord becomes visible if game has custom invite

    mpActionIRC = new QAction(tr("Open IRC"), this);
    mpActionIRC->setIcon(QIcon(qsl(":/icons/internet-telephony.png")));
    mpActionIRC->setObjectName(qsl("openIRC"));

    mpButtonDiscord->addAction(mpActionDiscord);
    mpButtonDiscord->addAction(mpActionIRC);
    mpButtonDiscord->setDefaultAction(mpActionDiscord);

    mpActionMapper = new QAction(QIcon(qsl(":/icons/applications-internet.png")), tr("Map"), this);
    mpActionMapper->setToolTip(utils::richText(tr("Show/hide the map")));
    mpMainToolBar->addAction(mpActionMapper);
    mpActionMapper->setObjectName(qsl("map_action"));
    mpMainToolBar->widgetForAction(mpActionMapper)->setObjectName(mpActionMapper->objectName());

    mpActionHelp = new QAction(QIcon(qsl(":/icons/help-hint.png")), tr("Manual"), this);
    mpActionHelp->setToolTip(utils::richText(tr("Browse reference material and documentation")));
    mpMainToolBar->addAction(mpActionHelp);
    mpActionHelp->setObjectName(qsl("manual_action"));
    mpMainToolBar->widgetForAction(mpActionHelp)->setObjectName(mpActionHelp->objectName());

    mpActionOptions = new QAction(QIcon(qsl(":/icons/configure.png")), tr("Settings"), this);
    mpActionOptions->setToolTip(utils::richText(tr("See and edit profile preferences")));
    mpMainToolBar->addAction(mpActionOptions);
    mpActionOptions->setObjectName(qsl("settings_action"));
    mpMainToolBar->widgetForAction(mpActionOptions)->setObjectName(mpActionOptions->objectName());

    // TODO: Consider changing to ":/icons/mudlet_notepad.png" as per the icon
    // now used for the window when the visual change to the toolbar caused can
    // be managed
    mpActionNotes = new QAction(QIcon(qsl(":/icons/applications-accessories.png")), tr("Notepad"), this);
    mpActionNotes->setToolTip(utils::richText(tr("Open a notepad that you can store your notes in")));
    mpMainToolBar->addAction(mpActionNotes);
    mpActionNotes->setObjectName(qsl("notepad_action"));
    mpMainToolBar->widgetForAction(mpActionNotes)->setObjectName(mpActionNotes->objectName());

    mpButtonPackageManagers = new QToolButton(this);
    mpButtonPackageManagers->setText(tr("Packages"));
    mpButtonPackageManagers->setObjectName(qsl("package_manager"));
    mpButtonPackageManagers->setContextMenuPolicy(Qt::ActionsContextMenu);
    mpButtonPackageManagers->setPopupMode(QToolButton::MenuButtonPopup);
    mpButtonPackageManagers->setAutoRaise(true);
    mpMainToolBar->addWidget(mpButtonPackageManagers);

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
    mpMainToolBar->addAction(mpActionReplay);
    mpMainToolBar->widgetForAction(mpActionReplay)->setObjectName(mpActionReplay->objectName());

    mpActionReconnect = new QAction(QIcon(qsl(":/icons/system-restart.png")), tr("Reconnect"), this);
    mpActionReconnect->setToolTip(utils::richText(tr("Disconnects you from the game and connects once again")));
    mpMainToolBar->addAction(mpActionReconnect);
    mpActionReconnect->setObjectName(qsl("reconnect_action"));
    mpMainToolBar->widgetForAction(mpActionReconnect)->setObjectName(mpActionReconnect->objectName());

    mpActionMultiView = new QAction(QIcon(qsl(":/icons/view-split-left-right.png")), tr("MultiView"), this);
    mpActionMultiView->setToolTip(utils::richText(tr("Splits the Mudlet screen to show multiple profiles at once; disabled when less than two are loaded.",
                                                 // Intentional comment to separate arguments
                                                 "Same text is used in 2 places.")));
    mpMainToolBar->addAction(mpActionMultiView);
    mpActionMultiView->setCheckable(true);
    mpActionMultiView->setChecked(false);
    mpActionMultiView->setEnabled(false);
    mpActionMultiView->setObjectName(qsl("multiview_action"));
    mpMainToolBar->widgetForAction(mpActionMultiView)->setObjectName(mpActionMultiView->objectName());

#if defined(INCLUDE_UPDATER)
    if (scmIsPublicTestVersion) {
        mpActionReportIssue = new QAction(tr("Report issue"), this);
        QStringList issueReportIcons {"face-uncertain.png", "face-surprise.png", "face-smile.png", "face-sad.png", "face-plain.png"};
        auto randomIcon = QRandomGenerator::global()->bounded(issueReportIcons.size());
        mpActionReportIssue->setIcon(QIcon(qsl(":/icons/%1").arg(issueReportIcons.at(randomIcon))));
        mpActionReportIssue->setToolTip(utils::richText(tr("The public test build gets newer features to you quicker, and you help us find issues in them quicker. Spotted something odd? Let us know asap!")));
        mpMainToolBar->addAction(mpActionReportIssue);
        mpActionReportIssue->setObjectName(qsl("reportissue_action"));
        mpMainToolBar->widgetForAction(mpActionReportIssue)->setObjectName(mpActionReportIssue->objectName());
    }
#endif

    mpActionAbout = new QAction(QIcon(qsl(":/icons/mudlet_information.png")), tr("About"), this);
    mpActionAbout->setToolTip(utils::richText(tr("Inform yourself about this version of Mudlet, the people who made it and the licence under which you can share it.",
                                                 // Intentional comment
                                                 "Tooltip for About Mudlet sub-menu item and main toolbar button (or menu item if an update has changed that control to have a popup menu instead) (Used in 3 places - please ensure all have the same translation).")));
    mpMainToolBar->addAction(mpActionAbout);
    mpActionAbout->setObjectName(qsl("about_action"));
    mpMainToolBar->widgetForAction(mpActionAbout)->setObjectName(mpActionAbout->objectName());

    disableToolbarButtons();

    if (mEnableFullScreenMode) {
        showFullScreen();
        QAction* actionFullScreeniew = new QAction(QIcon(qsl(":/icons/dialog-cancel.png")), tr("Toggle Full Screen View"), this);
        actionFullScreeniew->setStatusTip(tr("Toggle Full Screen View"));
        mpMainToolBar->addAction(actionFullScreeniew);
        actionFullScreeniew->setObjectName(qsl("fullscreen_action"));
        mpMainToolBar->widgetForAction(actionFullScreeniew)->setObjectName(actionFullScreeniew->objectName());
        connect(actionFullScreeniew, &QAction::triggered, this, &mudlet::slot_toggleFullScreenView);
    }

    QFont mainFont = QFont(qsl("Bitstream Vera Sans Mono"), 8, QFont::Normal);
    mpWidget_profileContainer->setFont(mainFont);
    mpWidget_profileContainer->show();

    connect(mpActionConnect.data(), &QAction::triggered, this, &mudlet::slot_showConnectionDialog);
    connect(mpActionHelp.data(), &QAction::triggered, this, &mudlet::slot_showHelpDialog);
    connect(mpActionTimers.data(), &QAction::triggered, this, &mudlet::slot_showTimerDialog);
    connect(mpActionAliases.data(), &QAction::triggered, this, &mudlet::slot_showAliasDialog);
    connect(mpActionScripts.data(), &QAction::triggered, this, &mudlet::slot_showScriptDialog);
    connect(mpActionKeys.data(), &QAction::triggered, this, &mudlet::slot_showKeyDialog);
    connect(mpActionVariables.data(), &QAction::triggered, this, &mudlet::slot_showVariableDialog);
    connect(mpActionButtons.data(), &QAction::triggered, this, &mudlet::slot_showActionDialog);
    connect(mpActionOptions.data(), &QAction::triggered, this, &mudlet::slot_showPreferencesDialog);
    connect(mpActionAbout.data(), &QAction::triggered, this, &mudlet::slot_showAboutDialog);
    connect(mpActionMultiView.data(), &QAction::triggered, this, &mudlet::slot_multiView);
    connect(mpActionReconnect.data(), &QAction::triggered, this, &mudlet::slot_reconnect);
    connect(mpActionDisconnect.data(), &QAction::triggered, this, &mudlet::slot_disconnect);
    connect(mpActionCloseProfile.data(), &QAction::triggered, this, &mudlet::slot_closeCurrentProfile);
    connect(mpActionReplay.data(), &QAction::triggered, this, &mudlet::slot_replay);
    connect(mpActionNotes.data(), &QAction::triggered, this, &mudlet::slot_notes);
    connect(mpActionMapper.data(), &QAction::triggered, this, &mudlet::slot_mapper);
    connect(mpActionIRC.data(), &QAction::triggered, this, &mudlet::slot_irc);
    connect(mpActionDiscord.data(), &QAction::triggered, this, &mudlet::slot_profileDiscord);
    connect(mpActionMudletDiscord.data(), &QAction::triggered, this, &mudlet::slot_mudletDiscord);
    connect(mpActionPackageManager.data(), &QAction::triggered, this, &mudlet::slot_packageManager);
    connect(mpActionModuleManager.data(), &QAction::triggered, this, &mudlet::slot_moduleManager);
    connect(mpActionPackageExporter.data(), &QAction::triggered, this, &mudlet::slot_packageExporter);

    connect(dactionConnect, &QAction::triggered, this, &mudlet::slot_showConnectionDialog);
    connect(dactionReconnect, &QAction::triggered, this, &mudlet::slot_reconnect);
    connect(dactionDisconnect, &QAction::triggered, this, &mudlet::slot_disconnect);
    connect(dactionCloseProfile, &QAction::triggered, this, &mudlet::slot_closeCurrentProfile);
    connect(dactionNotepad, &QAction::triggered, this, &mudlet::slot_notes);
    connect(dactionReplay, &QAction::triggered, this, &mudlet::slot_replay);

    connect(dactionHelp, &QAction::triggered, this, &mudlet::slot_showHelpDialog);
    connect(dactionVideo, &QAction::triggered, this, &mudlet::slot_showHelpDialogVideo);
    connect(dactionForum, &QAction::triggered, this, &mudlet::slot_showHelpDialogForum);
    connect(dactionIRC, &QAction::triggered, this, &mudlet::slot_irc);
    connect(dactionDiscord, &QAction::triggered, this, &mudlet::slot_profileDiscord);
    connect(dactionMudletDiscord, &QAction::triggered, this, &mudlet::slot_mudletDiscord);
    connect(dactionLiveHelpChat, &QAction::triggered, this, &mudlet::slot_irc);
    connect(dactionShowErrors, &QAction::triggered, [=]() {
        auto host = getActiveHost();
        if (!host) {
            return;
        }
        host->mpEditorDialog->showCurrentTriggerItem();
        host->mpEditorDialog->raise();
        host->mpEditorDialog->showNormal();
        host->mpEditorDialog->activateWindow();
        host->mpEditorDialog->mpErrorConsole->setVisible(true);
    });

#if defined(INCLUDE_UPDATER)
    // Show the update option if the code is present AND if this is a
    // release OR a public test version:
    dactionUpdate->setVisible(scmIsReleaseVersion || scmIsPublicTestVersion);
    // Show the report issue option if the updater code is present (as it is
    // less likely to be for: {Linux} distribution packaged versions of Mudlet
    // - or people hacking their own versions and neither of those types are
    // going to want the updater to change things for them) AND only for a
    // public test version:
    if (scmIsPublicTestVersion) {
        dactionReportIssue->setVisible(true);
        connect(mpActionReportIssue.data(), &QAction::triggered, this, &mudlet::slot_reportIssue);
        connect(dactionReportIssue, &QAction::triggered, this, &mudlet::slot_reportIssue);
    } else {
        dactionReportIssue->setVisible(false);
    }
#else
    // Unconditionally hide the update and report bug menu items if the updater
    // code is not included:
    dactionUpdate->setVisible(false);
    dactionReportIssue->setVisible(false);
#endif
    connect(dactionPackageManager, &QAction::triggered, this, &mudlet::slot_packageManager);
    connect(dactionPackageExporter, &QAction::triggered, this, &mudlet::slot_packageExporter);
    connect(dactionModuleManager, &QAction::triggered, this, &mudlet::slot_moduleManager);
    connect(dactionMultiView, &QAction::triggered, this, &mudlet::slot_multiView);
    connect(dactionInputLine, &QAction::triggered, this, &mudlet::slot_compactInputLine);
    connect(mpActionTriggers.data(), &QAction::triggered, this, &mudlet::slot_showTriggerDialog);
    connect(dactionScriptEditor, &QAction::triggered, this, &mudlet::slot_showEditorDialog);
    connect(dactionShowMap, &QAction::triggered, this, &mudlet::slot_mapper);
    connect(dactionOptions, &QAction::triggered, this, &mudlet::slot_showPreferencesDialog);
    connect(dactionAbout, &QAction::triggered, this, &mudlet::slot_showAboutDialog);

    // we historically use Alt on Windows and Linux, but that is uncomfortable on macOS
#if defined(Q_OS_MACOS)
    mKeySequenceTriggers = QKeySequence(Qt::CTRL | Qt::Key_E);
    mKeySequenceShowMap = QKeySequence(Qt::CTRL | Qt::Key_M);
    mKeySequenceInputLine = QKeySequence(Qt::CTRL | Qt::Key_L);
    mKeySequenceOptions = QKeySequence(Qt::CTRL | Qt::Key_P);
    mKeySequenceNotepad = QKeySequence(Qt::CTRL | Qt::Key_N);
    mKeySequencePackages = QKeySequence(Qt::CTRL | Qt::Key_O);
    mKeySequenceModules = QKeySequence(Qt::CTRL | Qt::Key_I);
    mKeySequenceMultiView = QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_V);
    mKeySequenceConnect = QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_C);
    mKeySequenceDisconnect = QKeySequence(Qt::CTRL | Qt::Key_D);
    mKeySequenceReconnect = QKeySequence(Qt::CTRL | Qt::Key_R);
    mKeySequenceCloseProfile = QKeySequence(Qt::CTRL | Qt::Key_W);
#else
    mKeySequenceTriggers = QKeySequence(Qt::ALT | Qt::Key_E);
    mKeySequenceShowMap = QKeySequence(Qt::ALT | Qt::Key_M);
    mKeySequenceInputLine = QKeySequence(Qt::ALT | Qt::Key_L);
    mKeySequenceOptions = QKeySequence(Qt::ALT | Qt::Key_P);
    mKeySequenceNotepad = QKeySequence(Qt::ALT | Qt::Key_N);
    mKeySequencePackages = QKeySequence(Qt::ALT | Qt::Key_O);
    mKeySequenceModules = QKeySequence(Qt::ALT | Qt::Key_I);
    mKeySequenceMultiView = QKeySequence(Qt::ALT | Qt::Key_V);
    mKeySequenceConnect = QKeySequence(Qt::ALT | Qt::Key_C);
    mKeySequenceDisconnect = QKeySequence(Qt::ALT | Qt::Key_D);
    mKeySequenceReconnect = QKeySequence(Qt::ALT | Qt::Key_R);
    mKeySequenceCloseProfile = QKeySequence(Qt::ALT | Qt::Key_W);
#endif
    connect(this, &mudlet::signal_menuBarVisibilityChanged, this, &mudlet::slot_updateShortcuts);
    connect(this, &mudlet::signal_hostCreated, this, &mudlet::slot_assignShortcutsFromProfile);
    connect(this, &mudlet::signal_profileActivated, this, &mudlet::slot_assignShortcutsFromProfile);

    mpShortcutsManager = new ShortcutsManager();
    mpShortcutsManager->registerShortcut(qsl("Script editor"), tr("Script editor"), &mKeySequenceTriggers);
    mpShortcutsManager->registerShortcut(qsl("Show Map"), tr("Show Map"), &mKeySequenceShowMap);
    mpShortcutsManager->registerShortcut(qsl("Compact input line"), tr("Compact input line"), &mKeySequenceInputLine);
    mpShortcutsManager->registerShortcut(qsl("Preferences"), tr("Preferences"), &mKeySequenceOptions);
    mpShortcutsManager->registerShortcut(qsl("Notepad"), tr("Notepad"), &mKeySequenceNotepad);
    mpShortcutsManager->registerShortcut(qsl("Package manager"), tr("Package manager"), &mKeySequencePackages);
    mpShortcutsManager->registerShortcut(qsl("Module manager"), tr("Module manager"), &mKeySequenceModules);
    mpShortcutsManager->registerShortcut(qsl("MultiView"), tr("MultiView"), &mKeySequenceMultiView);
    mpShortcutsManager->registerShortcut(qsl("Play"), tr("Play"), &mKeySequenceConnect);
    mpShortcutsManager->registerShortcut(qsl("Disconnect"), tr("Disconnect"), &mKeySequenceDisconnect);
    mpShortcutsManager->registerShortcut(qsl("Reconnect"), tr("Reconnect"), &mKeySequenceReconnect);
    mpShortcutsManager->registerShortcut(qsl("Close profile"), tr("Close profile"), &mKeySequenceCloseProfile);

    mpSettings = getQSettings();
    readLateSettings(*mpSettings);
    // The previous line will set an option used in the slot method:
    connect(mpMainToolBar, &QToolBar::visibilityChanged, this, &mudlet::slot_handleToolbarVisibilityChanged);

#if defined(INCLUDE_UPDATER)
    pUpdater = new Updater(this, mpSettings);
    connect(pUpdater, &Updater::signal_updateAvailable, this, &mudlet::slot_updateAvailable);
    connect(dactionUpdate, &QAction::triggered, this, &mudlet::slot_manualUpdateCheck);
#if defined(Q_OS_MACOS)
    // ensure that 'Check for updates' is under the Applications menu per convention
    dactionUpdate->setMenuRole(QAction::ApplicationSpecificRole);
#else
    connect(pUpdater, &Updater::signal_updateInstalled, this, &mudlet::slot_updateInstalled);
#endif // !Q_OS_MACOS
#endif // INCLUDE_UPDATER

    if (!mToolbarIconSize) {
        setToolBarIconSize(mEnableFullScreenMode ? 2 : 3);
    }

#if defined(QT_GAMEPAD_LIB)
    connect(QGamepadManager::instance(), &QGamepadManager::gamepadButtonPressEvent, this, &mudlet::slot_gamepadButtonPress);
    connect(QGamepadManager::instance(), &QGamepadManager::gamepadButtonReleaseEvent, this, &mudlet::slot_gamepadButtonRelease);
    connect(QGamepadManager::instance(), &QGamepadManager::gamepadConnected, this, &mudlet::slot_gamepadConnected);
    connect(QGamepadManager::instance(), &QGamepadManager::gamepadDisconnected, this, &mudlet::slot_gamepadDisconnected);
    connect(QGamepadManager::instance(), &QGamepadManager::gamepadAxisEvent, this, &mudlet::slot_gamepadAxisEvent);
#endif // if defined(QT_GAMEPAD_LIB)
    // Edbee has a singleton that needs some initialisation
    initEdbee();

    // load bundled fonts
    mFontManager.addFonts();

    // Initialise a couple of QMaps with elements that must be translated into
    // the current GUI Language
    loadMaps();

    setupTrayIcon();

    // initialize Announcer after the window is loaded, as UIA on Windows requires it
    QTimer::singleShot(0, this, [this]() {
        mpAnnouncer = new Announcer(this);
        emit signal_adjustAccessibleNames();
    });
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
    grammarManager->readGrammarFile(QLatin1String(":/edbee_defaults/Lua.tmLanguage"));

    //Open and parse the luaFunctionList document into a stringlist for use with autocomplete
    loadLuaFunctionList();

    //QFile file(fileName);
    //if( file.exists() && file.open(QIODevice::ReadOnly) ) {

    loadEdbeeTheme(qsl("Mudlet"), qsl("Mudlet.tmTheme"));
}

void mudlet::loadMaps()
{
    // Used to identify Hunspell dictionaries (some of which are not useful -
    // the "_med" ones are suppliments and no good for Mudlet) - all keys are to
    // be lower cased so that the values can be looked up with a
    // QMap<T1, T2>::value(const T1&) where the parameter has been previously
    // been converted to all-lower case:
    // From https://en.wikipedia.org/wiki/List_of_ISO_639-1_codes:
    // More useful is the cross-referenced (Country <-> Languages):
    // https://www.unicode.org/cldr/charts/latest/supplemental/language_territory_information.html
    // Initially populated from the dictionaries provided within the Debian
    // GNU/Linux distribution:
    //: In the translation source texts the language is the leading term, with, generally, the (primary) country(ies) in the brackets, with a trailing language disabiguation after a '-' Chinese is an exception!
    mDictionaryLanguageCodeMap = {{qsl("af"), tr("Afrikaans")},
                                  {qsl("af_za"), tr("Afrikaans (South Africa)")},
                                  {qsl("an"), tr("Aragonese")},
                                  {qsl("an_es"), tr("Aragonese (Spain)")},
                                  {qsl("ar"), tr("Arabic")},
                                  {qsl("ar_ae"), tr("Arabic (United Arab Emirates)")},
                                  {qsl("ar_bh"), tr("Arabic (Bahrain)")},
                                  {qsl("ar_dz"), tr("Arabic (Algeria)")},
                                  {qsl("ar_eg"), tr("Arabic (Egypt)")},
                                  {qsl("ar_in"), tr("Arabic (India)")},
                                  {qsl("ar_iq"), tr("Arabic (Iraq)")},
                                  {qsl("ar_jo"), tr("Arabic (Jordan)")},
                                  {qsl("ar_kw"), tr("Arabic (Kuwait)")},
                                  {qsl("ar_lb"), tr("Arabic (Lebanon)")},
                                  {qsl("ar_ly"), tr("Arabic (Libya)")},
                                  {qsl("ar_ma"), tr("Arabic (Morocco)")},
                                  {qsl("ar_om"), tr("Arabic (Oman)")},
                                  {qsl("ar_qa"), tr("Arabic (Qatar)")},
                                  {qsl("ar_sa"), tr("Arabic (Saudi Arabia)")},
                                  {qsl("ar_sd"), tr("Arabic (Sudan)")},
                                  {qsl("ar_sy"), tr("Arabic (Syria)")},
                                  {qsl("ar_tn"), tr("Arabic (Tunisia)")},
                                  {qsl("ar_ye"), tr("Arabic (Yemen)")},
                                  {qsl("be"), tr("Belarusian")},
                                  {qsl("be_by"), tr("Belarusian (Belarus)")},
                                  {qsl("be_ru"), tr("Belarusian (Russia)")},
                                  {qsl("bg"), tr("Bulgarian")},
                                  {qsl("bg_bg"), tr("Bulgarian (Bulgaria)")},
                                  {qsl("bn"), tr("Bangla")},
                                  {qsl("bn_bd"), tr("Bangla (Bangladesh)")},
                                  {qsl("bn_in"), tr("Bangla (India)")},
                                  {qsl("bo"), tr("Tibetan")},
                                  {qsl("bo_cn"), tr("Tibetan (China)")},
                                  {qsl("bo_in"), tr("Tibetan (India)")},
                                  {qsl("br"), tr("Breton")},
                                  {qsl("br_fr"), tr("Breton (France)")},
                                  {qsl("bs"), tr("Bosnian")},
                                  {qsl("bs_ba"), tr("Bosnian (Bosnia/Herzegovina)")},
                                  {qsl("bs_ba_cyrl"), tr("Bosnian (Bosnia/Herzegovina - Cyrillic alphabet)")},
                                  {qsl("ca"), tr("Catalan")},
                                  {qsl("ca_es"), tr("Catalan (Spain)")},
                                  {qsl("ca_es_valencia"), tr("Catalan (Spain - Valencian)")},
                                  {qsl("ckb"), tr("Central Kurdish")},
                                  {qsl("ckb_iq"), tr("Central Kurdish (Iraq)")},
                                  {qsl("cs"), tr("Czech")},
                                  {qsl("cs_cz"), tr("Czech (Czechia)")},
                                  {qsl("cy"), tr("Welsh")},
                                  {qsl("cy_gb"), tr("Welsh (United Kingdom {Wales})")},
                                  {qsl("da"), tr("Danish")},
                                  {qsl("da_dk"), tr("Danish (Denmark)")},
                                  {qsl("de"), tr("German")},
                                  {qsl("de_at"), tr("German (Austria)")},
                                  {qsl("de_at_frami"), tr("German (Austria, revised by F M Baumann)")},
                                  {qsl("de_be"), tr("German (Belgium)")},
                                  {qsl("de_ch"), tr("German (Switzerland)")},
                                  {qsl("de_ch_frami"), tr("German (Switzerland, revised by F M Baumann)")},
                                  {qsl("de_de"), tr("German (Germany/Belgium/Luxemburg)")},
                                  {qsl("de_de_frami"), tr("German (Germany/Belgium/Luxemburg, revised by F M Baumann)")},
                                  {qsl("de_li"), tr("German (Liechtenstein)")},
                                  {qsl("de_lu"), tr("German (Luxembourg)")},
                                  {qsl("dz"), tr("Dzongkha")},
                                  {qsl("dz_bt"), tr("Dzongkha (Bhutan)")},
                                  {qsl("el"), tr("Greek")},
                                  {qsl("el_gr"), tr("Greek (Greece)")},
                                  {qsl("en"), tr("English")},
                                  {qsl("en_ag"), tr("English (Antigua/Barbuda)")},
                                  {qsl("en_au"), tr("English (Australia)")},
                                  {qsl("en_au_large"), tr("English (Australia, Large)", "This dictionary contains larger vocabulary.")},
                                  {qsl("en_bs"), tr("English (Bahamas)")},
                                  {qsl("en_bw"), tr("English (Botswana)")},
                                  {qsl("en_bz"), tr("English (Belize)")},
                                  {qsl("en_ca"), tr("English (Canada)")},
                                  {qsl("en_ca_large"), tr("English (Canada, Large)", "This dictionary contains larger vocabulary.")},
                                  {qsl("en_dk"), tr("English (Denmark)")},
                                  {qsl("en_gb"), tr("English (United Kingdom)")},
                                  {qsl("en_gb_large"), tr("English (United Kingdom, Large)", "This dictionary contains larger vocabulary.")},
                                  {qsl("en_gb_ise"), tr("English (United Kingdom - 'ise' not 'ize')", "This dictionary prefers the British 'ise' form over the American 'ize' one.")},
                                  {qsl("en_gh"), tr("English (Ghana)")},
                                  {qsl("en_hk"), tr("English (Hong Kong SAR China)")},
                                  {qsl("en_ie"), tr("English (Ireland)")},
                                  {qsl("en_in"), tr("English (India)")},
                                  {qsl("en_jm"), tr("English (Jamaica)")},
                                  {qsl("en_na"), tr("English (Namibia)")},
                                  {qsl("en_ng"), tr("English (Nigeria)")},
                                  {qsl("en_nz"), tr("English (New Zealand)")},
                                  {qsl("en_ph"), tr("English (Philippines)")},
                                  {qsl("en_sg"), tr("English (Singapore)")},
                                  {qsl("en_tt"), tr("English (Trinidad/Tobago)")},
                                  {qsl("en_us"), tr("English (United States)")},
                                  {qsl("en_us_large"), tr("English (United States, Large)", "This dictionary contains larger vocabulary.")},
                                  {qsl("en_za"), tr("English (South Africa)")},
                                  {qsl("en_zw"), tr("English (Zimbabwe)")},
                                  {qsl("es"), tr("Spanish")},
                                  {qsl("es_ar"), tr("Spanish (Argentina)")},
                                  {qsl("es_bo"), tr("Spanish (Bolivia)")},
                                  {qsl("es_cl"), tr("Spanish (Chile)")},
                                  {qsl("es_co"), tr("Spanish (Colombia)")},
                                  {qsl("es_cr"), tr("Spanish (Costa Rica)")},
                                  {qsl("es_cu"), tr("Spanish (Cuba)")},
                                  {qsl("es_do"), tr("Spanish (Dominican Republic)")},
                                  {qsl("es_ec"), tr("Spanish (Ecuador)")},
                                  {qsl("es_es"), tr("Spanish (Spain)")},
                                  {qsl("es_gt"), tr("Spanish (Guatemala)")},
                                  {qsl("es_hn"), tr("Spanish (Honduras)")},
                                  {qsl("es_mx"), tr("Spanish (Mexico)")},
                                  {qsl("es_ni"), tr("Spanish (Nicaragua)")},
                                  {qsl("es_pa"), tr("Spanish (Panama)")},
                                  {qsl("es_pe"), tr("Spanish (Peru)")},
                                  {qsl("es_pr"), tr("Spanish (Puerto Rico)")},
                                  {qsl("es_py"), tr("Spanish (Paraguay)")},
                                  {qsl("es_sv"), tr("Spanish (El Savador)")},
                                  {qsl("es_us"), tr("Spanish (United States)")},
                                  {qsl("es_uy"), tr("Spanish (Uruguay)")},
                                  {qsl("es_ve"), tr("Spanish (Venezuela)")},
                                  {qsl("et"), tr("Estonian")},
                                  {qsl("et_ee"), tr("Estonian (Estonia)")},
                                  {qsl("eu"), tr("Basque")},
                                  {qsl("eu_es"), tr("Basque (Spain)")},
                                  {qsl("eu_fr"), tr("Basque (France)")},
                                  {qsl("fi"), tr("Finnish")},
                                  {qsl("fi_fi"), tr("Finnish")},
                                  {qsl("fr"), tr("French")},
                                  // On OpenBSD this seems to be the "usual spellings of French,
                                  // with, in addition, some new spellings rectifying past
                                  // inconsistencies":
                                  {qsl("fr_xx_classique"), tr("French")},
                                  {qsl("fr_be"), tr("French (Belgium)")},
                                  {qsl("fr_ca"), tr("French (Catalan)")},
                                  {qsl("fr_ch"), tr("French (Switzerland)")},
                                  {qsl("fr_fr"), tr("French (France)")},
                                  {qsl("fr_lu"), tr("French (Luxemburg)")},
                                  {qsl("fr_mc"), tr("French (Monaco)")},
                                  {qsl("ga"), tr("Irish")},
                                  {qsl("gd"), tr("Gaelic")},
                                  {qsl("gd_gb"), tr("Gaelic (United Kingdom {Scots})")},
                                  {qsl("gl"), tr("Galician")},
                                  {qsl("gl_es"), tr("Galician (Spain)")},
                                  {qsl("gn"), tr("Guarani")},
                                  {qsl("gn_py"), tr("Guarani (Paraguay)")},
                                  {qsl("gu"), tr("Gujarati")},
                                  {qsl("gu_in"), tr("Gujarati (India)")},
                                  // Debian uses gug instead of gn for some reason:
                                  {qsl("gug"), tr("Guarani")},
                                  {qsl("gug_py"), tr("Guarani (Paraguay)")},
                                  {qsl("he"), tr("Hebrew")},
                                  {qsl("he_il"), tr("Hebrew (Israel)")},
                                  {qsl("hi"), tr("Hindi")},
                                  {qsl("hi_in"), tr("Hindi (India)")},
                                  {qsl("hr"), tr("Croatian")},
                                  {qsl("hr_hr"), tr("Croatian (Croatia)")},
                                  {qsl("hu"), tr("Hungarian")},
                                  {qsl("hu_hu"), tr("Hungarian (Hungary)")},
                                  {qsl("hy"), tr("Armenian")},
                                  {qsl("hy_am"), tr("Armenian (Armenia)")},
                                  {qsl("id"), tr("Indonesian")},
                                  {qsl("id_id"), tr("Indonesian (Indonesia)")},
                                  {qsl("ie"), tr("Interlingue", "formerly known as Occidental, and not to be mistaken for Interlingua")},
                                  {qsl("is"), tr("Icelandic")},
                                  {qsl("is_is"), tr("Icelandic (Iceland)")},
                                  {qsl("it"), tr("Italian")},
                                  {qsl("it_ch"), tr("Italian (Switzerland)")},
                                  {qsl("it_it"), tr("Italian (Italy)")},
                                  {qsl("kk"), tr("Kazakh")},
                                  {qsl("kk_kz"), tr("Kazakh (Kazakhstan)")},
                                  {qsl("kmr"), tr("Kurmanji")},
                                  {qsl("kmr_latn"), tr("Kurmanji {Latin-alphabet Kurdish}")},
                                  {qsl("ko"), tr("Korean")},
                                  {qsl("ko_kr"), tr("Korean (South Korea)")},
                                  {qsl("ku"), tr("Kurdish")},
                                  {qsl("ku_sy"), tr("Kurdish (Syria)")},
                                  {qsl("ku_tr"), tr("Kurdish (Turkey)")},
                                  {qsl("la"), tr("Latin")},
                                  {qsl("lb"), tr("Luxembourgish")},
                                  {qsl("lb_lu"), tr("Luxembourgish (Luxembourg)")},
                                  {qsl("lo"), tr("Lao")},
                                  {qsl("lo_la"), tr("Lao (Laos)")},
                                  {qsl("lt"), tr("Lithuanian")},
                                  {qsl("lt_lt"), tr("Lithuanian (Lithuania)")},
                                  {qsl("lv"), tr("Latvian")},
                                  {qsl("lv_lv"), tr("Latvian (Latvia)")},
                                  {qsl("ml"), tr("Malayalam")},
                                  {qsl("ml_in"), tr("Malayalam (India)")},
                                  {qsl("nb"), tr("Norwegian Bokmål")},
                                  {qsl("nb_no"), tr("Norwegian Bokmål (Norway)")},
                                  {qsl("ne"), tr("Nepali")},
                                  {qsl("ne_np"), tr("Nepali (Nepal)")},
                                  {qsl("nl"), tr("Dutch")},
                                  {qsl("nl_an"), tr("Dutch (Netherlands Antilles)")},
                                  {qsl("nl_aw"), tr("Dutch (Aruba)")},
                                  {qsl("nl_be"), tr("Dutch (Belgium)")},
                                  {qsl("nl_nl"), tr("Dutch (Netherlands)")},
                                  {qsl("nl_sr"), tr("Dutch (Suriname)")},
                                  {qsl("nn"), tr("Norwegian Nynorsk")},
                                  {qsl("nn_no"), tr("Norwegian Nynorsk (Norway)")},
                                  {qsl("oc"), tr("Occitan")},
                                  {qsl("oc_fr"), tr("Occitan (France)")},
                                  {qsl("pl"), tr("Polish")},
                                  {qsl("pl_pl"), tr("Polish (Poland)")},
                                  {qsl("pt"), tr("Portuguese")},
                                  {qsl("pt_br"), tr("Portuguese (Brazil)")},
                                  {qsl("pt_pt"), tr("Portuguese (Portugal)")},
                                  {qsl("ro"), tr("Romanian")},
                                  {qsl("ro_ro"), tr("Romanian (Romania)")},
                                  {qsl("ru"), tr("Russian")},
                                  {qsl("ru_ru"), tr("Russian (Russia)")},
                                  {qsl("se"), tr("Northern Sami")},
                                  {qsl("se_fi"), tr("Northern Sami (Finland)")},
                                  {qsl("se_no"), tr("Northern Sami (Norway)")},
                                  {qsl("se_se"), tr("Northern Sami (Sweden)")},
                                  {qsl("sh"), tr("Shtokavian", "This code seems to be the identifier for the prestige dialect for several languages used in the region of the former Yugoslavia state without a state indication")},
                                  {qsl("sh_yu"), tr("Shtokavian (former state of Yugoslavia)", "This code seems to be the identifier for the prestige dialect for several languages used in the region of the former Yugoslavia state with a (withdrawn from ISO 3166) state indication")},
                                  {qsl("si"), tr("Sinhala")},
                                  {qsl("si_lk"), tr("Sinhala (Sri Lanka)")},
                                  {qsl("sk"), tr("Slovak")},
                                  {qsl("sk_sk"), tr("Slovak (Slovakia)")},
                                  {qsl("sl"), tr("Slovenian")},
                                  {qsl("sl_si"), tr("Slovenian (Slovenia)")},
                                  {qsl("so"), tr("Somali")},
                                  {qsl("so_so"), tr("Somali (Somalia)")},
                                  {qsl("sq"), tr("Albanian")},
                                  {qsl("sq_al"), tr("Albanian (Albania)")},
                                  {qsl("sr"), tr("Serbian")},
                                  {qsl("sr_me"), tr("Serbian (Montenegro)")},
                                  {qsl("sr_rs"), tr("Serbian (Serbia)")},
                                  {qsl("sr_latn_rs"), tr("Serbian (Serbia - Latin-alphabet)")},
                                  {qsl("sr_yu"), tr("Serbian (former state of Yugoslavia)")},
                                  {qsl("ss"), tr("Swati")},
                                  {qsl("ss_sz"), tr("Swati (Swaziland)")},
                                  {qsl("ss_za"), tr("Swati (South Africa)")},
                                  {qsl("sv"), tr("Swedish")},
                                  {qsl("sv_se"), tr("Swedish (Sweden)")},
                                  {qsl("sv_fi"), tr("Swedish (Finland)")},
                                  {qsl("sw"), tr("Swahili")},
                                  {qsl("sw_ke"), tr("Swahili (Kenya)")},
                                  {qsl("sw_tz"), tr("Swahili (Tanzania)")},
                                  {qsl("te"), tr("Telugu")},
                                  {qsl("te_in"), tr("Telugu (India)")},
                                  {qsl("th"), tr("Thai")},
                                  {qsl("th_th"), tr("Thai (Thailand)")},
                                  {qsl("ti"), tr("Tigrinya")},
                                  {qsl("ti_er"), tr("Tigrinya (Eritrea)")},
                                  {qsl("ti_et"), tr("Tigrinya (Ethiopia)")},
                                  {qsl("tk"), tr("Turkmen")},
                                  {qsl("tk_tm"), tr("Turkmen (Turkmenistan)")},
                                  {qsl("tn"), tr("Tswana")},
                                  {qsl("tn_bw"), tr("Tswana (Botswana)")},
                                  {qsl("tn_za"), tr("Tswana (South Africa)")},
                                  {qsl("tr"), tr("Turkish")},
                                  {qsl("tr_tr"), tr("Turkish (Turkey)")},
                                  {qsl("ts"), tr("Tsonga")},
                                  {qsl("ts_za"), tr("Tsonga (South Africa)")},
                                  {qsl("uk"), tr("Ukrainian")},
                                  {qsl("uk_ua"), tr("Ukrainian (Ukraine)")},
                                  {qsl("uz"), tr("Uzbek")},
                                  {qsl("uz_uz"), tr("Uzbek (Uzbekistan)")},
                                  {qsl("ve"), tr("Venda")},
                                  {qsl("vi"), tr("Vietnamese")},
                                  {qsl("vi_vn"), tr("Vietnamese (Vietnam)")},
                                  {qsl("vi_daucu"), tr("Vietnamese (DauCu variant - old-style diacritics)")},
                                  {qsl("vi_daumoi"), tr("Vietnamese (DauMoi variant - new-style diacritics)")},
                                  // OpenBSD has the old names, see:
                                  // https://github.com/1ec5/hunspell-vi/commit/fecdb355e7c114e1d5ca22fe5b301b2d54af84c9
                                  {qsl("vi_x_old"), tr("Vietnamese (DauCu variant - old-style diacritics)")},
                                  {qsl("vi_x_new"), tr("Vietnamese (DauMoi variant - new-style diacritics)")},
                                  {qsl("wa"), tr("Walloon")},
                                  {qsl("xh"), tr("Xhosa")},
                                  {qsl("yi"), tr("Yiddish")},
                                  {qsl("zh"), tr("Chinese")},
                                  {qsl("zh_cn"), tr("Chinese (China - simplified)")},
                                  {qsl("zh_tw"), tr("Chinese (Taiwan - traditional)")},
                                  {qsl("zu"), tr("Zulu")}};

    mEncodingNameMap = {{"ASCII", tr("ASCII (Basic)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"UTF-8", tr("UTF-8 (Recommended)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"EUC-KR", tr("EUC-KR (Korean)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"GBK", tr("GBK (Chinese)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"GB18030", tr("GB18030 (Chinese)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"BIG5", tr("Big5-ETen (Taiwan)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"BIG5-HKSCS", tr("Big5-HKSCS (Hong Kong)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"ISO 8859-1", tr("ISO 8859-1 (Western European)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"ISO 8859-2", tr("ISO 8859-2 (Central European)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"ISO 8859-3", tr("ISO 8859-3 (South European)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"ISO 8859-4", tr("ISO 8859-4 (Baltic)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"ISO 8859-5", tr("ISO 8859-5 (Cyrillic)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"ISO 8859-6", tr("ISO 8859-6 (Arabic)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"ISO 8859-7", tr("ISO 8859-7 (Greek)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"ISO 8859-8", tr("ISO 8859-8 (Hebrew Visual)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"ISO 8859-9", tr("ISO 8859-9 (Turkish)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"ISO 8859-10", tr("ISO 8859-10 (Nordic)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"ISO 8859-11", tr("ISO 8859-11 (Latin/Thai)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"ISO 8859-13", tr("ISO 8859-13 (Baltic)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"ISO 8859-14", tr("ISO 8859-14 (Celtic)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"ISO 8859-15", tr("ISO 8859-15 (Western)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"ISO 8859-16", tr("ISO 8859-16 (Romanian)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"CP437", tr("CP437 (OEM Font)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"M_CP437", qsl("m ") % tr("CP437 (OEM Font)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"CP667", tr("CP667 (Mazovia)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"M_CP667", qsl("m ") % tr("CP667 (Mazovia)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"CP737", tr("CP737 (DOS Greek)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"M_CP737", qsl("m ") % tr("CP737 (DOS Greek)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"CP850", tr("CP850 (Western Europe)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"CP866", tr("CP866 (Cyrillic/Russian)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"CP869", tr("CP869 (DOS Greek 2)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"M_CP869",  qsl("m ") % tr("CP869 (DOS Greek 2)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"CP1161", tr("CP1161 (Latin/Thai)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"KOI8-R", tr("KOI8-R (Cyrillic)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"KOI8-U", tr("KOI8-U (Cyrillic/Ukrainian)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"MACINTOSH", tr("MACINTOSH", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"WINDOWS-1250", tr("WINDOWS-1250 (Central European)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"WINDOWS-1251", tr("WINDOWS-1251 (Cyrillic)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"WINDOWS-1252", tr("WINDOWS-1252 (Western)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"WINDOWS-1253", tr("WINDOWS-1253 (Greek)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"WINDOWS-1254", tr("WINDOWS-1254 (Turkish)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"WINDOWS-1255", tr("WINDOWS-1255 (Hebrew)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"WINDOWS-1256", tr("WINDOWS-1256 (Arabic)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"WINDOWS-1257", tr("WINDOWS-1257 (Baltic)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")},
                        {"WINDOWS-1258", tr("WINDOWS-1258 (Vietnamese)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)")}};
}

// migrates the Central Debug Console to the next available host, if any
void mudlet::migrateDebugConsole(Host* currentHost)
{
    if (!smpDebugArea) {
        return;
    }

    const auto debugConsoleHost = smpDebugConsole->getHost();
    if (debugConsoleHost != currentHost) {
        return;
    }

    smpDebugArea->setAttribute(Qt::WA_DeleteOnClose);
    smpDebugArea->close();
}

// As we are currently only using files from a resource file we only need to
// analyse them once per application run - if we were loading from a user
// selectable location, or even from a read-only part of their computer's
// file-system we would have to do this each time they looked to change
// language/locale:
// If we ever change the usage of this to take a path other than the
// resource file's one then the code in the main.cpp file's
// (void) loadTranslationsForCommandLine() will have to be revised as well:
void mudlet::scanForMudletTranslations(const QString& path)
{
    mPathNameMudletTranslations = path;
    // qDebug().nospace().noquote() << "mudlet::scanForMudletTranslations(\"" << path << "\") INFO - Seeking Mudlet translation files:";
    mTranslationsMap.clear();

    QDir translationDir(path);
    translationDir.setNameFilters(QStringList() << qsl("mudlet_*.qm"));
    QStringList translationFilesList(translationDir.entryList(QDir::Files | QDir::Readable, QDir::Name));

    QJsonObject translationStats;
    if (path == qsl(":/lang")) {
        QFile file(qsl(":/translation-stats.json"));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QByteArray saveData = file.readAll();
            QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));
            translationStats = loadDoc.object();
            file.close();
        } else {
            qWarning() << "translation statistics file isn't available, won't show stats in preferences";
        }
    }

    for (auto& translationFileName : qAsConst(translationFilesList)) {
        QString languageCode(translationFileName);
        languageCode.remove(qsl("mudlet_"), Qt::CaseInsensitive);
        languageCode.remove(qsl(".qm"), Qt::CaseInsensitive);
        int percentageTranslated = -1;

        std::unique_ptr<QTranslator> pMudletTranslator = std::make_unique<QTranslator>();
        if (Q_LIKELY(pMudletTranslator->load(translationFileName, path))) {
            // qDebug().noquote().nospace() << "    found a Mudlet translation for locale code: \"" << languageCode << "\".";
            if (!translationStats.isEmpty()) {
                // For this to not be empty then we are reading the translations
                // from the expected resource file and the translation
                // statistics file was also found from there

                auto value = translationStats.value(languageCode).toObject().value(qsl("translated_percent"));
                if (value != QJsonValue::Undefined) {
                    percentageTranslated = value.toInt();
                } else {
                    percentageTranslated = 0;
                }
            }
            // PLACEMARKER: Start of locale codes to native language decoding - insert an entry here for any further Mudlet supported languages
            translation currentTranslation(percentageTranslated);
            if (!languageCode.compare(QLatin1String("en_US"), Qt::CaseInsensitive)) {
                currentTranslation.mNativeName = qsl("English (American)");
            } else if (!languageCode.compare(QLatin1String("en_GB"), Qt::CaseInsensitive)) {
                currentTranslation.mNativeName = qsl("English (British)");
            } else if (!languageCode.compare(QLatin1String("zh_CN"), Qt::CaseInsensitive)) {
                currentTranslation.mNativeName = qsl("简化字");
            } else if (!languageCode.compare(QLatin1String("zh_TW"), Qt::CaseInsensitive)) {
                currentTranslation.mNativeName = qsl("繁體字");
            } else if (!languageCode.compare(QLatin1String("nl_NL"), Qt::CaseInsensitive)) {
                currentTranslation.mNativeName = qsl("Nederlands");
            } else if (!languageCode.compare(QLatin1String("fr_FR"), Qt::CaseInsensitive)) {
                currentTranslation.mNativeName = qsl("Français");
            } else if (!languageCode.compare(QLatin1String("de_DE"), Qt::CaseInsensitive)) {
                currentTranslation.mNativeName = qsl("Deutsch");
            } else if (!languageCode.compare(QLatin1String("el_GR"), Qt::CaseInsensitive)) {
                currentTranslation.mNativeName = qsl("ελληνικά");
            } else if (!languageCode.compare(QLatin1String("it_IT"), Qt::CaseInsensitive)) {
                currentTranslation.mNativeName = qsl("Italiano");
            } else if (!languageCode.compare(QLatin1String("pl_PL"), Qt::CaseInsensitive)) {
                currentTranslation.mNativeName = qsl("Polski");
            } else if (!languageCode.compare(QLatin1String("ru_RU"), Qt::CaseInsensitive)) {
                currentTranslation.mNativeName = qsl("Pусский");
            } else if (!languageCode.compare(QLatin1String("es_ES"), Qt::CaseInsensitive)) {
                currentTranslation.mNativeName = qsl("Español");
            } else if (!languageCode.compare(QLatin1String("pt_PT"), Qt::CaseInsensitive)) {
                currentTranslation.mNativeName = qsl("Portugês");
            } else if (!languageCode.compare(QLatin1String("pt_BR"), Qt::CaseInsensitive)) {
                currentTranslation.mNativeName = qsl("Português (Brasil)");
            } else if (!languageCode.compare(QLatin1String("tr_TR"), Qt::CaseInsensitive)) {
                currentTranslation.mNativeName = qsl("Türkçe");
            } else if (!languageCode.compare(QLatin1String("fi_FI"), Qt::CaseInsensitive)) {
                currentTranslation.mNativeName = qsl("Suomeksi");
            } else if (!languageCode.compare(QLatin1String("ar_SA"), Qt::CaseInsensitive)) {
                currentTranslation.mNativeName = qsl("العربية");
            } else {
                currentTranslation.mNativeName = languageCode;
            }
            currentTranslation.mMudletTranslationFileName = translationFileName;
            mTranslationsMap.insert(languageCode, currentTranslation);
        } else {
            // This is very unlikely to be reached as it means that a file that
            // matched the naming to be a Mudlet translation file was not in fact
            // one...
            qDebug().noquote().nospace() << "    no Mudlet translation found for locale code: \"" << languageCode << "\".";
        }
    }
}

// To be used AFTER scanForMudletTranslations(...) has been called, this will
// insert the corresponding Qt system translation pathFileNames (path to
// filenames: qt_xx.qm or qt_xx_YY.qm) into the entries in the mTranslationsMap
// QMap.
void mudlet::scanForQtTranslations(const QString& path)
{
    mPathNameQtTranslations = path;
    // qDebug().nospace().noquote() << "mudlet::scanForQtTranslations(\"" << path << "\") INFO - Seeking Qt translation files:";
    QMutableMapIterator<QString, translation> itTranslation(mTranslationsMap);
    while (itTranslation.hasNext()) {
        itTranslation.next();
        const QString languageCode = itTranslation.key();
        std::unique_ptr<QTranslator> pQtTranslator = std::make_unique<QTranslator>();
        QString translationFileName(qsl("qt_%1.qm").arg(languageCode));
        if (pQtTranslator->load(translationFileName, path)) {
            // qDebug().noquote().nospace() << "    found a Qt translation for locale code: \"" << languageCode << "\"";
            /*
             * Unfortunately, success in this operation does not mean that
             * a qt_xx_YY.qm translation file has been located, as the
             * (bool) QTranslator::load(...)
             * call can forget about both the _YY and even the _xx if filenames
             * with those elements are not found but a less detailed filename
             * IS detected.
             *
             * So although we can note the load of a given pathFileName is
             * successful it might not be exactly what it seems to be!
             */
            translation current = itTranslation.value();
            current.mQtTranslationFileName = translationFileName;
            itTranslation.setValue(current);
        } else {
            // qDebug().noquote().nospace() << "    no Qt translation found for locale code: \"" << languageCode << "\"";
        }
    }
}

void mudlet::loadTranslators(const QString& languageCode)
{
    if (!mTranslatorsLoadedList.isEmpty()) {
        // qDebug().nospace().noquote() << "mudlet::loadTranslators(\"" << languageCode << "\") INFO - uninstalling existing translation previously loaded...";
        QMutableListIterator<QPointer<QTranslator>> itTranslator(mTranslatorsLoadedList);
        itTranslator.toBack();
        while (itTranslator.hasPrevious()) {
            QPointer<QTranslator> pTranslator = itTranslator.previous();
            if (pTranslator) {
                qApp->removeTranslator(pTranslator);
                itTranslator.remove();
                delete pTranslator;
            }
        }
    }

    translation currentTranslation = mTranslationsMap.value(languageCode);
    QPointer<QTranslator> pQtTranslator = new QTranslator;
    QString qtTranslatorFileName = currentTranslation.getQtTranslationFileName();
    if (!qtTranslatorFileName.isEmpty()) {
        // Need to use load(fileName (e.g. {qt_xx_YY.qm"}, pathName) form - Qt
        // mangles the former to find the actual best one to use, but we
        // shouldn't include the path in the first element as it seems to mess
        // up the process of locating the file:
        pQtTranslator->load(qtTranslatorFileName, mPathNameQtTranslations);
        if (!pQtTranslator->isEmpty()) {
            // qDebug().nospace().noquote() << "mudlet::loadTranslators(\"" << languageCode << "\") INFO - installing Qt libraries' translation from a path and file name specified as: \"" << mPathNameQtTranslations << "/"<< qtTranslatorFileName << "\"...";
            qApp->installTranslator(pQtTranslator);
            mTranslatorsLoadedList.append(pQtTranslator);
        }
    }

    QPointer<QTranslator> pMudletTranslator = new QTranslator;
    QString mudletTranslatorFileName = currentTranslation.getMudletTranslationFileName();
    if (!mudletTranslatorFileName.isEmpty()) {
        pMudletTranslator->load(mudletTranslatorFileName, mPathNameMudletTranslations);
        if (!pMudletTranslator->isEmpty()) {
//            qDebug().nospace().noquote() << "mudlet::loadTranslators(\"" << languageCode << "\") INFO - installing Mudlet translation from: \"" << mPathNameMudletTranslations << "/"
//                                         << mudletTranslatorFileName << "\"...";
            qApp->installTranslator(pMudletTranslator);
            mTranslatorsLoadedList.append(pMudletTranslator);
        }
    }
}

void mudlet::slot_moduleManager()
{
    Host* pH = getActiveHost();
    if (!pH) {
        return;
    }
    auto moduleManager = pH->mpModuleManager;
    if (!moduleManager){
        moduleManager = new dlgModuleManager(this, pH);
        pH->mpModuleManager = moduleManager;
    }
    moduleManager->raise();
    moduleManager->show();
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

void mudlet::slot_packageManager()
{
    Host* pH = getActiveHost();
    if (!pH) {
        return;
    }

    auto packageManager = pH->mpPackageManager;
    if (!packageManager) {
        packageManager = new dlgPackageManager(this, pH);
        pH->mpPackageManager = packageManager;
    }
    packageManager->raise();
    packageManager->showNormal();
    packageManager->activateWindow();
}

void mudlet::slot_packageExporter()
{
    Host* pH = getActiveHost();
    if (!pH) {
        return;
    }
    auto d = new dlgPackageExporter(this, pH);
    d->show();
}

void mudlet::slot_closeCurrentProfile()
{
    Host* pH = getActiveHost();
    if (!pH) {
        return;
    }
    slot_closeProfileRequested(mpTabBar->currentIndex());

    if (!getActiveHost()) {
        disableToolbarButtons();
        slot_showConnectionDialog();
    }
}

void mudlet::slot_closeProfileRequested(int tab)
{
    QString name = mpTabBar->tabData(tab).toString();
    closeHost(name);
}

void mudlet::closeHost(const QString& name)
{
    Host* pH = mHostManager.getHost(name);
    if (!pH) {
        return;
    }

    std::list<QPointer<TToolBar>> hostToolBarMap = pH->getActionUnit()->getToolBarList();
    QMap<QString, TDockWidget*>& dockWindowMap = pH->mpConsole->mDockWidgetMap;
    QMap<QString, TConsole*>& hostConsoleMap = pH->mpConsole->mSubConsoleMap;

    if (!pH->mpConsole->close()) {
        return;
    }

    pH->mpConsole->mUserAgreedToCloseConsole = true;
    pH->closingDown();

    // disconnect before removing objects from memory as sysDisconnectionEvent needs that stuff.
    if (pH->mSslTsl) {
        pH->mTelnet.abortConnection();
    } else {
        pH->mTelnet.disconnectIt();
    }

    pH->stopAllTriggers();
    pH->mpEditorDialog->setAttribute(Qt::WA_DeleteOnClose);
    pH->mpEditorDialog->close();
    pH->mpEditorDialog = nullptr;

    for (auto consoleName : hostConsoleMap.keys()) {
        if (dockWindowMap.contains(consoleName)) {
            dockWindowMap[consoleName]->setAttribute(Qt::WA_DeleteOnClose);
            dockWindowMap[consoleName]->close();
            removeDockWidget(dockWindowMap[consoleName]);
            dockWindowMap.remove(consoleName);
        }

        hostConsoleMap[consoleName]->close();
        hostConsoleMap.remove(consoleName);
    }

    if (pH->mpNotePad) {
        pH->mpNotePad->save();
        pH->mpNotePad->setAttribute(Qt::WA_DeleteOnClose);
        pH->mpNotePad->close();
        pH->mpNotePad = nullptr;
    }

    for (TToolBar* pTB : hostToolBarMap) {
        if (pTB) {
            pTB->setAttribute(Qt::WA_DeleteOnClose);
            pTB->deleteLater();
        }
    }

    // close IRC client window if it is open.
    if (pH->mpDlgIRC) {
        pH->mpDlgIRC->setAttribute(Qt::WA_DeleteOnClose);
        pH->mpDlgIRC->deleteLater();
    }

    migrateDebugConsole(pH);

    pH->mpConsole->close();

    mpTabBar->removeTab(name);
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
    emit signal_adjustAccessibleNames();
    updateMultiViewControls();
}

void mudlet::updateMultiViewControls()
{
    const bool isEnabled = (mHostManager.getHostCount() - 1);
    if (mpActionMultiView->isEnabled() != isEnabled){
        mpActionMultiView->setEnabled(isEnabled);
    }
    if (dactionMultiView->isEnabled() != isEnabled) {
        dactionMultiView->setEnabled(isEnabled);
    }
}

void mudlet::reshowRequiredMainConsoles()
{
    if (mpTabBar->count() > 1 && mMultiView) {
        for (const auto& host: mHostManager) {
            if (host->mpConsole) {
                host->mpConsole->show();
            }
        }
    }
}

// Moved as much as possible to activateProfile()...
void mudlet::slot_tabChanged(int tabID)
{
    QString hostName = mpTabBar->tabData(tabID).toString();
    activateProfile(mHostManager.getHost(hostName));
}

void mudlet::addConsoleForNewHost(Host* pH)
{
    if (pH->mpConsole) {
        return;
    }
    auto pConsole = new TMainConsole(pH);
    if (!pConsole) {
        return;
    }
    pH->mpConsole = pConsole;
    pConsole->setWindowTitle(pH->getName());
    pConsole->setObjectName(pH->getName());
    QString tabName = pH->getName();
    int newTabID = mpTabBar->addTab(tabName);
    /*
     * There is a sneaky feature on some OSes (I found it on FreeBSD but
     * it is notable switched OFF by default on MacOs) where Qt adds an
     * automatically generated accelarator to the text on the tab which - at
     * least on FreeBSD - causes the Text to be CHANGED from what is set (an
     * underscore is added to a suitably unique letter but that, being a text
     * accelerator is converted to an additional '&' in the text when it is
     * read) - this messes up identifying the tab by its name - so we now get
     * around it by also storing the text in the tab's data - see:
     * + void qt_set_sequence_auto_mnemonic(bool) in 'QKeySequence' documentation
     * + "Detailed Description" in 'QShortCut' documentation
     * + "QTabBar creates automatic mnemonic keys in the manner of
     *    QAbstractButton; e.g. if a tab's label is '&Graphics', Alt+G becomes
     *    a shortcut key for switching to that tab." in 'QTabBar' documentation"
     */
    mpTabBar->setTabData(newTabID, tabName);

    // update the window title for the currently selected profile
    // Potential to be translated in the future if the need arises, with the following disambiguation:
    // "Title for the main window when a profile is loaded or active, %1 is the name "
    // "of the profile and %2 is the Mudlet version string."
    setWindowTitle(qsl("%1 - %2")
                           .arg(pH->getName(), scmVersion));

    mpSplitter_profileContainer->addWidget(pConsole);
    if (mpCurrentActiveHost && !mMultiView) {
        mpCurrentActiveHost->mpConsole->hide();
    }

    if (pH->mLogStatus) {
        // The above flag is set/reset at the start of the TMainConsole
        // constructor - and if it is set we now need to "click" the button
        // to immediately start logging the game output as text/HTML:
        pConsole->logButton->click();
    }

    if (pH->mTimeStampStatus) {
        // This is similar to logging above, but for timestamps
        pConsole->timeStampButton->click();
    }

    pConsole->show();

    auto pEditor = new dlgTriggerEditor(pH);
    pH->mpEditorDialog = pEditor;
    connect(pH, &Host::profileSaveStarted,  pH->mpEditorDialog, &dlgTriggerEditor::slot_profileSaveStarted);
    connect(pH, &Host::profileSaveFinished,  pH->mpEditorDialog, &dlgTriggerEditor::slot_profileSaveFinished);
    pEditor->fillout_form();

    pH->getActionUnit()->updateToolbar();

    pH->mpConsole->show();
    pH->mpConsole->repaint();
    pH->mpConsole->refresh();
    pH->mpConsole->mpCommandLine->repaint();
    pH->mpConsole->mpCommandLine->setFocus();
    pH->mpConsole->show();
    // Setting mpCurrentActiveHost to pH is now done by the following
    slot_tabChanged(newTabID);

    int x = pH->mpConsole->width();
    int y = pH->mpConsole->height();
    QSize s = QSize(x, y);
    QResizeEvent event(s, s);
    updateDiscordNamedIcon();
    QApplication::sendEvent(pH->mpConsole, &event);
    // This is needed to completely show the first autoloaded profile so that
    // it can be properly hidden by a second one (without it the:
    // mpCurrentActiveHost->mpConsole->hide() does not work correctly and two
    // profiles get shown across a split screen - even though mMultiView is NOT
    // set)!
    qApp->processEvents();
}


void mudlet::slot_timerFires()
{
    QTimer* pQT = qobject_cast<QTimer*>(sender());
    if (Q_UNLIKELY(!pQT)) {
        return;
    }

    // Pull the Host name and TTimer::id from the properties:
    QString hostName(pQT->property(TTimer::scmProperty_HostName).toString());
    if (Q_UNLIKELY(hostName.isEmpty())) {
        qWarning().nospace().noquote() << "mudlet::slot_timerFires() INFO - Host name is empty - so TTimer has probably been deleted.";
        pQT->deleteLater();
        return;
    }

    Host* pHost = mHostManager.getHost(hostName);
    Q_ASSERT_X(pHost, "mudlet::slot_timerFires()", "Unable to deduce Host pointer from data in QTimer");
    int id = pQT->property(TTimer::scmProperty_TTimerId).toInt();
    if (Q_UNLIKELY(!id)) {
        qWarning().nospace().noquote() << "mudlet::slot_timerFires() INFO - TTimer ID is zero - so TTimer has probably been deleted.";
        pQT->deleteLater();
        return;
    }
    TTimer* pTT = pHost->getTimerUnit()->getTimer(id);
    if (Q_LIKELY(pTT)) {
// commented out as it will be spammy in normal situations but saved as useful
// during timer debugging... 8-)
//        qDebug().nospace().noquote() << "mudlet::slot_timerFires() INFO - Host: \"" << hostName << "\" QTimer firing for TTimer Id:" << id;
//        qDebug().nospace().noquote() << "    (objectName:\"" << pQT->objectName() << "\")";
        pTT->execute();
        if (pTT->checkRestart()) {
            pTT->start();
        }

        // Okay now we've found it we are done:
        return;
    }

    qWarning().nospace().noquote() << "mudlet::slot_timerFires() ERROR - Timer not registered, it seems to have been called: \"" << pQT->objectName() << "\" - automatically deleting it!";
    // Clean up any bogus ones:
    pQT->stop();
    pQT->deleteLater();
}

void mudlet::disableToolbarButtons()
{
    mpActionTriggers->setEnabled(false);
    mpActionAliases->setEnabled(false);
    mpActionTimers->setEnabled(false);
    mpActionButtons->setEnabled(false);
    mpActionScripts->setEnabled(false);
    mpActionKeys->setEnabled(false);
    mpActionVariables->setEnabled(false);
    mpActionMudletDiscord->setEnabled(false);
    mpActionMapper->setEnabled(false);
    mpActionNotes->setEnabled(false);
    mpButtonPackageManagers->setEnabled(false);
    mpActionIRC->setEnabled(false);
    mpActionReplay->setEnabled(false);
    mpActionReplay->setToolTip(tr("<p>Load a Mudlet replay.</p>"
                                  "<p><i>Disabled until a profile is loaded.</i></p>"));
    // The menu items will not show tool-tips unless the parent menu is set to
    // show tool-tips which is likely to be done in near future when there are
    // more texts to show {the default is to repeat the menu text which is not
    // useful} with a call to menuEditor->setToolTipsVisible(true);
    dactionReplay->setToolTip(mpActionReplay->toolTip());

    dactionReplay->setEnabled(false);
    mpActionReconnect->setEnabled(false);
    mpActionDisconnect->setEnabled(false);

    mpActionCloseProfile->setEnabled(false);
    dactionCloseProfile->setEnabled(false);
}

void mudlet::enableToolbarButtons()
{
    mpActionTriggers->setEnabled(true);
    mpActionAliases->setEnabled(true);
    mpActionTimers->setEnabled(true);
    mpActionButtons->setEnabled(true);
    mpActionScripts->setEnabled(true);
    mpActionKeys->setEnabled(true);
    mpActionVariables->setEnabled(true);
    mpActionMudletDiscord->setEnabled(true);
    mpActionMapper->setEnabled(true);
    mpActionNotes->setEnabled(true);
    mpButtonPackageManagers->setEnabled(true);
    mpActionIRC->setEnabled(true);

    if (!mpToolBarReplay) {
        // Only enable the replay button if it is not disabled because there is
        // another profile loaded and already playing a replay {when the replay
        // toolbar pointer will be non-null}:
        mpActionReplay->setEnabled(true);
        dactionReplay->setEnabled(true);
        mpActionReplay->setToolTip(utils::richText(tr("Load a Mudlet replay.")));
        // The menu items will not show tool-tips unless the parent menu is set to
        // show tool-tips which is likely to be done in near future when there are
        // more texts to show {the default is to repeat the menu text which is not
        // useful} with a call to menuEditor->setToolTipsVisible(true);
        dactionReplay->setToolTip(mpActionReplay->toolTip());
    }

    mpActionReconnect->setEnabled(true);
    mpActionDisconnect->setEnabled(true);

    mpActionCloseProfile->setEnabled(true);
    dactionCloseProfile->setEnabled(true);

    // As this is called when a profile is loaded it is time to check whether
    // we need to continue to show the main menu and/or the main toolbar
    adjustMenuBarVisibility();
    adjustToolBarVisibility();
}

bool mudlet::saveWindowLayout()
{
    qDebug() << "mudlet::saveWindowLayout() - Already-Saved:" << mHasSavedLayout;
    if (mHasSavedLayout) {
        return false;
    }

    QString layoutFilePath = getMudletPath(mainDataItemPath, qsl("windowLayout.dat"));

    QFile layoutFile(layoutFilePath);
    if (layoutFile.open(QIODevice::WriteOnly)) {
        // revert update markers to ready objects for saving.
        commitLayoutUpdates();

        QByteArray layoutData = saveState();
        QDataStream ofs(&layoutFile);
        if (scmRunTimeQtVersion >= QVersionNumber(5, 13, 0)) {
            ofs.setVersion(scmQDataStreamFormat_5_12);
        }
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

    QString layoutFilePath = getMudletPath(mainDataItemPath, qsl("windowLayout.dat"));

    QFile layoutFile(layoutFilePath);
    if (layoutFile.exists()) {
        if (layoutFile.open(QIODevice::ReadOnly)) {
            mIsLoadingLayout = true;

            QByteArray layoutData;
            QDataStream ifs(&layoutFile);
            if (scmRunTimeQtVersion >= QVersionNumber(5, 13, 0)) {
                ifs.setVersion(scmQDataStreamFormat_5_12);
            }
            ifs >> layoutData;
            layoutFile.close();

            bool rv = restoreState(layoutData);

            commitLayoutUpdates(true);
            mIsLoadingLayout = false;

            return rv;
        }
    }
    return false;
}

void mudlet::commitLayoutUpdates(bool flush)
{
    for (auto pHost : mHostManager) {
        if (pHost->commitLayoutUpdates(flush)) {
            mHasSavedLayout = false;
        }
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

std::optional<QSize> mudlet::getImageSize(const QString& imageLocation)
{
    QImage image(imageLocation);

    if (image.isNull()) {
        return {};
    }

    return image.size();
}

Host* mudlet::getActiveHost()
{
    if (mpCurrentActiveHost && mpCurrentActiveHost->mpConsole) {
        return mpCurrentActiveHost;
    } else {
        return nullptr;
    }
}

void mudlet::closeEvent(QCloseEvent* event)
{
    QVector<QString> closingHosts;

    for (auto pHost : mHostManager) {
        const auto console = pHost->mpConsole;
        if (!console) {
            continue;
        }
        if (!console->close()) {
            // close out any profiles that we have agreed to close so far
            for (const auto& hostName : qAsConst(closingHosts)) {
                closeHost(hostName);
            }

            event->ignore();
            return;
        } else {
            console->mUserAgreedToCloseConsole = true;
            closingHosts.append(pHost->getName());
        }
    }

    writeSettings();

    goingDown();
    if (smpDebugArea) {
        smpDebugArea->setAttribute(Qt::WA_DeleteOnClose);
        smpDebugArea->close();
    }

    for (auto pHost : mHostManager) {
        pHost->close();
    }

    // hide main Mudlet window once we're sure the 'do you want to save the profile?' won't come up
    hide();

    for (auto pHost : mHostManager) {
        if (pHost->currentlySavingProfile()) {
            pHost->waitForProfileSave();
        }
    }

    // pass the event on so dblsqd can perform an update
    // if automatic updates have been disabled
    event->accept();
}

void mudlet::forceClose()
{
    for (auto pHost : mHostManager) {
        auto console = pHost->mpConsole;
        if (!console) {
            continue;
        }
        pHost->saveProfile();
        console->mUserAgreedToCloseConsole = true;

        if (pHost->mSslTsl) {
            pHost->mTelnet.abortConnection();
        } else {
            pHost->mTelnet.disconnectIt();
        }

        // close script-editor
        if (pHost->mpEditorDialog) {
            pHost->mpEditorDialog->setAttribute(Qt::WA_DeleteOnClose);
            pHost->mpEditorDialog->close();
        }

        if (pHost->mpNotePad) {
            pHost->mpNotePad->save();
            pHost->mpNotePad->setAttribute(Qt::WA_DeleteOnClose);
            pHost->mpNotePad->close();
            pHost->mpNotePad = nullptr;
        }

        if (pHost->mpDlgIRC) {
            pHost->mpDlgIRC->close();
        }

        console->close();
    }

    // hide main Mudlet window once we're sure the 'do you want to save the profile?' won't come up
    hide();

    for (auto pHost : mHostManager) {
        if (pHost->currentlySavingProfile()) {
            pHost->waitForProfileSave();
        }
    }

    writeSettings();

    close();
}

// readSettings has been split into two because some settings will need to be
// known BEFORE (early) the GUI is constructed and some AFTERWARDS (late)
void mudlet::readEarlySettings(const QSettings& settings)
{
    // In the near future the user's locale preferences will need to be read
    // as soon as possible as well!

    mShowIconsOnMenuCheckedState = static_cast<Qt::CheckState>(settings.value("showIconsInMenus", QVariant(Qt::PartiallyChecked)).toInt());

    // PLACEMARKER: Full-screen mode controlled by File (1 of 2) At some point we might removal this "if" and only consider the QSetting - dropping consideration of the sentinel file:
    if (settings.contains(qsl("enableFullScreenMode"))) {
        // We have a setting stored for this
        mEnableFullScreenMode = settings.value(qsl("enableFullScreenMode"), QVariant(false)).toBool();
    } else {
        // We do not have a QSettings value stored so check for the sentinel file:
        QFile file_use_smallscreen(getMudletPath(mainDataItemPath, qsl("mudlet_option_use_smallscreen")));
        mEnableFullScreenMode = file_use_smallscreen.exists();
    }

    // PTBs had a boolean setting, migrate it to one that can respect the system setting as well
    auto oldDarkTheme = settings.value(qsl("darkTheme"), QVariant(false)).toBool();

    auto appearance = settings.value(qsl("appearance"), QVariant(0)).toInt();
    if (appearance == 0) {
        mAppearance = settings.contains(qsl("darkTheme")) ? (oldDarkTheme ? Appearance::dark : Appearance::light) : Appearance::systemSetting;
    } else {
        mAppearance = static_cast<Appearance>(appearance);
    }

    mInterfaceLanguage = settings.value("interfaceLanguage", autodetectPreferredLanguage()).toString();
    mUserLocale = QLocale(mInterfaceLanguage);
    if (mUserLocale == QLocale::c()) {
        qWarning().nospace().noquote() << "mudlet::readEarlySettings(...) WARNING - Unable to convert language code \"" << mInterfaceLanguage << "\" to a recognised locale, reverting to the POSIX 'C' one.";
        return;
    }

//    qDebug().nospace().noquote() << "mudlet::readEarlySettings(...) INFO - Using language code \"" << mInterfaceLanguage << "\" to switch to \"" << QLocale::languageToString(mUserLocale.language()) << " (" << QLocale::countryToString(mUserLocale.country()) << ")\" locale.";
}

void mudlet::readLateSettings(const QSettings& settings)
{
    QPoint pos = settings.value(qsl("pos"), QPoint(0, 0)).toPoint();
    QSize size = settings.value(qsl("size"), QSize(750, 550)).toSize();
    // A sensible default has already been set up according to whether we are on
    // a netbook or not before this gets called so only change if there is a
    // setting stored:
    if (settings.contains(qsl("mainiconsize"))) {
        setToolBarIconSize(settings.value(qsl("mainiconsize")).toInt());
    }
    setEditorTreeWidgetIconSize(settings.value("tefoldericonsize", QVariant(3)).toInt());
    // We have abandoned previous "showMenuBar" / "showToolBar" booleans
    // although we provide a backwards compatible value
    // of: (bool) showXXXXBar = (XXXXBarVisibilty != visibleNever) for, until,
    // it is suggested Mudlet 4.x:
    setMenuBarVisibility(static_cast<controlsVisibilityFlag>(settings.value("menuBarVisibility", static_cast<int>(visibleAlways)).toInt()));
    setToolBarVisibility(static_cast<controlsVisibilityFlag>(settings.value("toolBarVisibility", static_cast<int>(visibleAlways)).toInt()));
    mEditorTextOptions = static_cast<QTextOption::Flags>(settings.value("editorTextOptions", QVariant(0)).toInt());

    mShowMapAuditErrors = settings.value("reportMapIssuesToConsole", QVariant(false)).toBool();
    mStorePasswordsSecurely = settings.value("storePasswordsSecurely", QVariant(true)).toBool();


    resize(size);
    move(pos);
    if (settings.value("maximized", false).toBool()) {
        showMaximized();
    }
    mCopyAsImageTimeout = settings.value(qsl("copyAsImageTimeout"), mCopyAsImageTimeout).toInt();
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
        mpButtonConnect->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        mpButtonDiscord->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        if (!mpButtonAbout.isNull()) {
            mpButtonAbout->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        }
        mpButtonPackageManagers->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    } else {
        mpMainToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
        mpButtonConnect->setToolButtonStyle(Qt::ToolButtonIconOnly);
        mpButtonDiscord->setToolButtonStyle(Qt::ToolButtonIconOnly);
        if (!mpButtonAbout.isNull()) {
            mpButtonAbout->setToolButtonStyle(Qt::ToolButtonIconOnly);
        }
        mpButtonPackageManagers->setToolButtonStyle(Qt::ToolButtonIconOnly);
    }

    if (mpToolBarReplay) {
        mpToolBarReplay->setIconSize(mpMainToolBar->iconSize());
        mpToolBarReplay->setToolButtonStyle(mpMainToolBar->toolButtonStyle());
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

// This is used to set the menu bar visibility and adjusts that accordingly
void mudlet::setMenuBarVisibility(const controlsVisibility state)
{
    mMenuBarVisibility = state;

    adjustMenuBarVisibility();
    emit signal_menuBarVisibilityChanged(state);
}

// This only adjusts the visibility as appropriate
void mudlet::adjustMenuBarVisibility()
{
    const int hostCount = mHostManager.getHostCount();
    if ((hostCount < 1 && (mMenuBarVisibility & visibleAlways)) || (hostCount >= 1 && (mMenuBarVisibility & visibleMaskNormally))) {
        menuBar()->show();
    } else {
        menuBar()->hide();
    }
}

void mudlet::setToolBarVisibility(const controlsVisibility state)

{
    mToolbarVisibility = state;

    adjustToolBarVisibility();
    emit signal_toolBarVisibilityChanged(state);
}

// Override the main window context menu action to prevent the main tool bar
// from being hidden if we do not want it to be (or it is not safe to do so - no
// profile loaded so no TConsoles with a "rescue" context menu):
void mudlet::slot_handleToolbarVisibilityChanged(bool isVisible)
{
    if (!isVisible && mMenuBarVisibility == visibleNever) {
        // Only need to worry about it DIS-appearing if the menu bar is not showing
        const int hostCount = mHostManager.getHostCount();
        if ((hostCount < 1 && (mToolbarVisibility & visibleAlways)) || (hostCount >= 1 && (mToolbarVisibility & visibleMaskNormally))) {
            mpMainToolBar->show();
        }
    }
}

void mudlet::adjustToolBarVisibility()
{
    const int hostCount = mHostManager.getHostCount();
    if ((hostCount < 1 && (mToolbarVisibility & visibleAlways)) || (hostCount >= 1 && (mToolbarVisibility & visibleMaskNormally))) {
        mpMainToolBar->show();
    } else {
        mpMainToolBar->hide();
    }
}

bool mudlet::isControlsVisible() const
{
    // Use the real state of the controlled things in case the logic to
    // control their state (mToolbarVisibility & mMenuBarVisibility) are out of
    // sync with reality:

    return mpMainToolBar->isVisible() || menuBar()->isVisible();
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
    // This pair are only for backwards compatibility and will be ignored for
    // this and future Mudlet versions - suggest they get removed in Mudlet 4.x
    settings.setValue("showMenuBar", mMenuBarVisibility != visibleNever);
    settings.setValue("showToolbar", mToolbarVisibility != visibleNever);

    settings.setValue("menuBarVisibility", static_cast<int>(mMenuBarVisibility));
    settings.setValue("toolBarVisibility", static_cast<int>(mToolbarVisibility));
    settings.setValue("maximized", isMaximized());
    settings.setValue("editorTextOptions", static_cast<int>(mEditorTextOptions));
    settings.setValue("reportMapIssuesToConsole", mShowMapAuditErrors);
    settings.setValue("storePasswordsSecurely", mStorePasswordsSecurely);
    settings.setValue("showIconsInMenus", mShowIconsOnMenuCheckedState);
    settings.setValue("enableFullScreenMode", mEnableFullScreenMode);
    settings.setValue("copyAsImageTimeout", mCopyAsImageTimeout);
    settings.setValue("interfaceLanguage", mInterfaceLanguage);
    // value only used during PTBs, remove it to reduce confusion in the future
    settings.remove("darkTheme");
    settings.setValue("appearance", mAppearance);
}

void mudlet::slot_showConnectionDialog()
{
    if (mpConnectionDialog) {
        return;
    }
    mpConnectionDialog = new dlgConnectionProfiles(this);
    connect(mpConnectionDialog, &dlgConnectionProfiles::signal_load_profile, this, &mudlet::slot_connectionDialogueFinished);
    mpConnectionDialog->fillout_form();

    connect(mpConnectionDialog, &QDialog::accepted, this, [=]() { enableToolbarButtons(); });
    mpConnectionDialog->setAttribute(Qt::WA_DeleteOnClose);
    mpConnectionDialog->show();
}

void mudlet::slot_showEditorDialog()
{
    Host* pHost = getActiveHost();
    if (!pHost) {
        return;
    }
    dlgTriggerEditor* pEditor = pHost->mpEditorDialog;
    if (!pEditor) {
        return;
    }
    pEditor->showCurrentTriggerItem();
    pEditor->raise();
    pEditor->showNormal();
    pEditor->activateWindow();
}

void mudlet::slot_showTriggerDialog()
{
    Host* pHost = getActiveHost();
    if (!pHost) {
        return;
    }
    dlgTriggerEditor* pEditor = pHost->mpEditorDialog;
    if (!pEditor) {
        return;
    }
    pEditor->slot_showTriggers();
    pEditor->raise();
    pEditor->showNormal();
    pEditor->activateWindow();
}

void mudlet::slot_showAliasDialog()
{
    Host* pHost = getActiveHost();
    if (!pHost) {
        return;
    }
    dlgTriggerEditor* pEditor = pHost->mpEditorDialog;
    if (!pEditor) {
        return;
    }
    pEditor->slot_showAliases();
    pEditor->raise();
    pEditor->showNormal();
    pEditor->activateWindow();
}

void mudlet::slot_showTimerDialog()
{
    Host* pHost = getActiveHost();
    if (!pHost) {
        return;
    }
    dlgTriggerEditor* pEditor = pHost->mpEditorDialog;
    if (!pEditor) {
        return;
    }
    pEditor->slot_showTimers();
    pEditor->raise();
    pEditor->showNormal();
    pEditor->activateWindow();
}

void mudlet::slot_showScriptDialog()
{
    Host* pHost = getActiveHost();
    if (!pHost) {
        return;
    }
    dlgTriggerEditor* pEditor = pHost->mpEditorDialog;
    if (!pEditor) {
        return;
    }
    pEditor->slot_showScripts();
    pEditor->raise();
    pEditor->showNormal();
    pEditor->activateWindow();
}

void mudlet::slot_showKeyDialog()
{
    Host* pHost = getActiveHost();
    if (!pHost) {
        return;
    }
    dlgTriggerEditor* pEditor = pHost->mpEditorDialog;
    if (!pEditor) {
        return;
    }
    pEditor->slot_showKeys();
    pEditor->raise();
    pEditor->showNormal();
    pEditor->activateWindow();
}

void mudlet::slot_showVariableDialog()
{
    Host* pHost = getActiveHost();
    if (!pHost) {
        return;
    }
    dlgTriggerEditor* pEditor = pHost->mpEditorDialog;
    if (!pEditor) {
        return;
    }
    pEditor->slot_showVariables();
    pEditor->raise();
    pEditor->showNormal();
    pEditor->activateWindow();
}

void mudlet::slot_showActionDialog()
{
    Host* pHost = getActiveHost();
    if (!pHost) {
        return;
    }
    dlgTriggerEditor* pEditor = pHost->mpEditorDialog;
    if (!pEditor) {
        return;
    }
    pEditor->slot_showActions();
    pEditor->raise();
    pEditor->showNormal();
    pEditor->activateWindow();
}

// tab must be the "objectName" of the tab in the preferences NOT the "titleText"
void mudlet::showOptionsDialog(const QString& tab)
{
    Host* pHost = getActiveHost();

    auto pPrefs = pHost ? pHost->mpDlgProfilePreferences : mpDlgProfilePreferences;

    if (!pPrefs) {
        pPrefs = new dlgProfilePreferences(this, pHost);
        if (pHost) {
            pHost->mpDlgProfilePreferences = pPrefs;
        } else {
            mpDlgProfilePreferences = pPrefs;
        }

        connect(mpActionReconnect.data(), &QAction::triggered, pPrefs->need_reconnect_for_data_protocol, &QWidget::hide);
        connect(dactionReconnect, &QAction::triggered, pPrefs->need_reconnect_for_data_protocol, &QWidget::hide);
        connect(mpActionReconnect.data(), &QAction::triggered, pPrefs->need_reconnect_for_specialoption, &QWidget::hide);
        connect(dactionReconnect, &QAction::triggered, pPrefs->need_reconnect_for_specialoption, &QWidget::hide);
        connect(pPrefs, &dlgProfilePreferences::signal_preferencesSaved, this, [=]() {
            slot_assignShortcutsFromProfile(getActiveHost());
        });
        pPrefs->setAttribute(Qt::WA_DeleteOnClose);
    }

    if (pHost) {
        pPrefs->setStyleSheet(pHost->mProfileStyleSheet);
    }
    pPrefs->setTab(tab);
    pPrefs->raise();
    pPrefs->show();
}

void mudlet::slot_assignShortcutsFromProfile(Host* pHost)
{
    if (pHost) {
        auto iterator = mpShortcutsManager->iterator();
        while (iterator.hasNext()) {
            auto key = iterator.next();
            mpShortcutsManager->setShortcut(key, pHost->profileShortcuts.value(key));
        }
    }
    assignKeySequences();
}

void mudlet::slot_updateShortcuts()
{
    if (Q_LIKELY(mMenuVisibleState.has_value())) {
        if ((mMenuBarVisibility == visibleNever
            || (mMenuBarVisibility == visibleOnlyWithoutLoadedProfile && mHostManager.getHostCount()))
           && (!mMenuVisibleState.value()) ) {

            /*
             * IF   EITHER the menu is NOT to be shown
             *      OR the menu is only to be show when there is no profiles AND there IS one
             *      (so the menu should be hidden)
             *    AND
             *      the setting says it is hidden
             * THEN
             *    Skip doing anything
             */
            return;
        }

        if ((mMenuBarVisibility == visibleAlways
            || (mMenuBarVisibility == visibleOnlyWithoutLoadedProfile && !mHostManager.getHostCount()))
           && (mMenuVisibleState.value()) ) {

            /*
             * IF   EITHER the menu IS to be shown
             *      OR the menu is only to be show when there is no profiles AND there is NOT one
             *      (so the menu should be shown)
             *    AND
             *      the setting says it is shown
             * THEN
             *    Skip doing anything
             */
            return;
        }
    }
    assignKeySequences();
}

void mudlet::assignKeySequences()
{
    mMenuVisibleState = !(mMenuBarVisibility == visibleNever || (mMenuBarVisibility == visibleOnlyWithoutLoadedProfile && mHostManager.getHostCount()));
    if (!mMenuVisibleState.value()) {
        // The menu is hidden so wire the QKeySequences directly to the slots:

        // If there was a shortcut then get rid of it - no need for a
        // call to "disconnect(...)" as that happens on deletion and since it
        // is okay to delete a nullptr there is no need to include a non-null
        // test first:
        delete mpShortcutTriggers.data();
        mpShortcutTriggers = new QShortcut(mKeySequenceTriggers, this);
        connect(mpShortcutTriggers.data(), &QShortcut::activated, this, &mudlet::slot_showEditorDialog);
        dactionScriptEditor->setShortcut(QKeySequence());

        delete mpShortcutShowMap.data();
        mpShortcutShowMap = new QShortcut(mKeySequenceShowMap, this);
        connect(mpShortcutShowMap.data(), &QShortcut::activated, this, &mudlet::slot_mapper);
        dactionShowMap->setShortcut(QKeySequence());

        delete mpShortcutInputLine.data();
        mpShortcutInputLine = new QShortcut(mKeySequenceInputLine, this);
        connect(mpShortcutInputLine.data(), &QShortcut::activated, this, &mudlet::slot_toggleCompactInputLine);
        dactionInputLine->setShortcut(QKeySequence());

        delete mpShortcutOptions.data();
        mpShortcutOptions = new QShortcut(mKeySequenceOptions, this);
        connect(mpShortcutOptions.data(), &QShortcut::activated, this, &mudlet::slot_showPreferencesDialog);
        dactionOptions->setShortcut(QKeySequence());

        delete mpShortcutNotepad.data();
        mpShortcutNotepad = new QShortcut(mKeySequenceNotepad, this);
        connect(mpShortcutNotepad.data(), &QShortcut::activated, this, &mudlet::slot_notes);
        dactionNotepad->setShortcut(QKeySequence());

        delete mpShortcutPackages.data();
        mpShortcutPackages = new QShortcut(mKeySequencePackages, this);
        connect(mpShortcutPackages.data(), &QShortcut::activated, this, &mudlet::slot_packageManager);
        dactionPackageManager->setShortcut(QKeySequence());

        delete mpShortcutModules.data();
        mpShortcutModules = new QShortcut(mKeySequencePackages, this);
        connect(mpShortcutModules.data(), &QShortcut::activated, this, &mudlet::slot_moduleManager);
        dactionModuleManager->setShortcut(QKeySequence());

        delete mpShortcutMultiView.data();
        mpShortcutMultiView = new QShortcut(mKeySequenceMultiView, this);
        connect(mpShortcutMultiView.data(), &QShortcut::activated, this, &mudlet::slot_toggleMultiView);
        dactionMultiView->setShortcut(QKeySequence());

        delete mpShortcutConnect.data();
        mpShortcutConnect = new QShortcut(mKeySequenceConnect, this);
        connect(mpShortcutConnect.data(), &QShortcut::activated, this, &mudlet::slot_showConnectionDialog);
        dactionConnect->setShortcut(QKeySequence());

        delete mpShortcutDisconnect.data();
        mpShortcutDisconnect = new QShortcut(mKeySequenceDisconnect, this);
        connect(mpShortcutDisconnect.data(), &QShortcut::activated, this, &mudlet::slot_disconnect);
        dactionDisconnect->setShortcut(QKeySequence());

        delete mpShortcutReconnect.data();
        mpShortcutReconnect = new QShortcut(mKeySequenceReconnect, this);
        connect(mpShortcutReconnect.data(), &QShortcut::activated, this, &mudlet::slot_reconnect);
        dactionReconnect->setShortcut(QKeySequence());

        delete mpShortcutCloseProfile.data();
        mpShortcutCloseProfile = new QShortcut(mKeySequenceCloseProfile, this);
        connect(mpShortcutCloseProfile.data(), &QShortcut::activated, this, &mudlet::slot_closeCurrentProfile);
        dactionCloseProfile->setShortcut(QKeySequence());
    } else {
        // The menu is shown so tie the QKeySequences to the menu items and it
        // is those that will call the slots:

        // Because we are deleting the object that it points at
        // this will also clear() the shortcut pointer:
        delete mpShortcutTriggers.data();
        dactionScriptEditor->setShortcut(mKeySequenceTriggers);

        delete mpShortcutShowMap.data();
        dactionShowMap->setShortcut(mKeySequenceShowMap);

        delete mpShortcutInputLine.data();
        dactionInputLine->setShortcut(mKeySequenceInputLine);

        delete mpShortcutOptions.data();
        dactionOptions->setShortcut(mKeySequenceOptions);

        delete mpShortcutNotepad.data();
        dactionNotepad->setShortcut(mKeySequenceNotepad);

        delete mpShortcutPackages.data();
        dactionPackageManager->setShortcut(mKeySequencePackages);

        delete mpShortcutModules.data();
        dactionModuleManager->setShortcut(mKeySequenceModules);

        delete mpShortcutMultiView.data();
        dactionMultiView->setShortcut(mKeySequenceMultiView);

        delete mpShortcutConnect.data();
        dactionConnect->setShortcut(mKeySequenceConnect);

        delete mpShortcutDisconnect.data();
        dactionDisconnect->setShortcut(mKeySequenceDisconnect);

        delete mpShortcutReconnect.data();
        dactionReconnect->setShortcut(mKeySequenceReconnect);

        delete mpShortcutCloseProfile.data();
        dactionCloseProfile->setShortcut(mKeySequenceCloseProfile);
    }
}

void mudlet::slot_showPreferencesDialog()
{
    showOptionsDialog(qsl("tab_general"));
}

void mudlet::slot_showHelpDialog()
{
    QDesktopServices::openUrl(QUrl("https://wiki.mudlet.org/w/Manual:Contents"));
}

void mudlet::slot_showHelpDialogVideo()
{
    QDesktopServices::openUrl(QUrl("https://www.mudlet.org/media/"));
}

void mudlet::slot_showHelpDialogForum()
{
    QDesktopServices::openUrl(QUrl("https://forums.mudlet.org/"));
}

// Not used:
//void mudlet::slot_showHelpDialogIrc()
//{
//    QDesktopServices::openUrl(QUrl("https://web.libera.chat/?channel=#mudlet"));
//}

void mudlet::slot_mapper()
{
    Host* pHost = getActiveHost();
    if (!pHost) {
        return;
    }
    pHost->showHideOrCreateMapper(true);
}

void mudlet::slot_openMappingScriptsPage()
{
    QDesktopServices::openUrl(QUrl("https://forums.mudlet.org/search.php?keywords=mapping+script&terms=all&author=&sc=1&sf=titleonly&sr=topics&sk=t&sd=d&st=0&ch=400&t=0&submit=Search"));
}

void mudlet::slot_showAboutDialog()
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
        format.setFont(pHost->getDisplayFont());
        pNotes->notesEdit->setCurrentCharFormat(format);
        pNotes->setWindowTitle(tr("%1 - notes").arg(pHost->getName()));
        pNotes->setWindowIcon(QIcon(qsl(":/icons/mudlet_notepad.png")));
        pHost->mpNotePad->setStyleSheet(pHost->mProfileStyleSheet);
        pHost->mpNotePad->notesEdit->setStyleSheet(pHost->mProfileStyleSheet);
    }
    pNotes->raise();
    pNotes->show();
}

void mudlet::slot_irc()
{
    Host* pHost = getActiveHost();
    if (!pHost) {
        return;
    }

    if (!pHost->mpDlgIRC) {
        pHost->mpDlgIRC = new dlgIRC(pHost);
    }
    pHost->mpDlgIRC->raise();
    pHost->mpDlgIRC->show();
}

void mudlet::slot_profileDiscord()
{
    Host* pHost = getActiveHost();
    QString invite;
    if (pHost) {
        invite = pHost->getDiscordInviteURL();
    }
    openWebPage(invite.isEmpty() ? mMudletDiscordInvite : invite);
}

void mudlet::slot_mudletDiscord()
{
    openWebPage(mMudletDiscordInvite);
}

void mudlet::updateDiscordNamedIcon()
{
    Host* pHost = getActiveHost();
    if (!pHost) {
        return;
    }

    QString gameName = pHost->getDiscordGameName();

    bool hasCustom = !pHost->getDiscordInviteURL().isEmpty();

    mpActionDiscord->setIconText(gameName.isEmpty() ? qsl("Discord") : QFontMetrics(mpActionDiscord->font()).elidedText(gameName, Qt::ElideRight, 90));

    if (mpActionMudletDiscord->isVisible() != hasCustom) {
        mpActionMudletDiscord->setVisible(hasCustom);
    }
    if (dactionDiscord->isVisible() != hasCustom) {
        dactionDiscord->setVisible(hasCustom);
    }
}

void mudlet::slot_reconnect()
{
    Host* pHost = getActiveHost();
    if (!pHost) {
        return;
    }
    pHost->mTelnet.reconnect();
}

void mudlet::slot_disconnect()
{
    Host* pHost = getActiveHost();
    if (!pHost) {
        return;
    }
    pHost->mTelnet.disconnectIt();
}

void mudlet::slot_replay()
{
    Host* pHost = getActiveHost();
    if (!pHost) {
        return;
    }

    QString fileName = QFileDialog::getOpenFileName(this, tr("Select Replay"),
                                                    getMudletPath(profileReplayAndLogFilesPath, pHost->getName()),
                                                    tr("*.dat"));
    if (fileName.isEmpty()) {
        // Cancel was hit in QFileDialog::getOpenFileName(...)
        return;
    }

    // No third argument causes error messages to be sent to pHost's main console:
    loadReplay(pHost, fileName);
}

QString mudlet::readProfileData(const QString& profile, const QString& item)
{
    QFile file(getMudletPath(profileDataItemPath, profile, item));
    file.open(QIODevice::ReadOnly);
    if (!file.exists()) {
        return QString();
    }

    QDataStream ifs(&file);
    if (scmRunTimeQtVersion >= QVersionNumber(5, 13, 0)) {
        ifs.setVersion(scmQDataStreamFormat_5_12);
    }
    QString ret;

    ifs >> ret;
    file.close();
    return ret;
}

QPair<bool, QString> mudlet::writeProfileData(const QString& profile, const QString& item, const QString& what)
{
    auto f = getMudletPath(mudlet::profileDataItemPath, profile, item);
    QFile file(f);
    if (file.open(QIODevice::WriteOnly | QIODevice::Unbuffered)) {
        QDataStream ofs(&file);
        ofs << what;
        file.close();
    }

    if (file.error() == QFile::NoError) {
        return qMakePair(true, QString());
    } else {
        return qMakePair(false, file.errorString());
    }
}

void mudlet::deleteProfileData(const QString& profile, const QString& item)
{
    if (!QFile::remove(getMudletPath(profileDataItemPath, profile, item))) {
        qWarning() << "Couldn't delete profile data file" << item;
    }
}

// this slot is called via a timer in the constructor of mudlet::mudlet()
void mudlet::startAutoLogin(const QStringList& cliProfiles)
{
    QStringList hostList = QDir(getMudletPath(profilesPath)).entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    hostList += TGameDetails::keys();
    hostList << qsl("Mudlet self-test");
    hostList.removeDuplicates();
    bool openedProfile = false;

    for (auto& hostName : cliProfiles){
        if (hostList.contains(hostName)) {
            doAutoLogin(hostName);
            openedProfile = true;
            hostList.removeOne(hostName);
        }
    }

    for (auto& hostName : hostList) {
        QString val = readProfileData(hostName, qsl("autologin"));
        if (val.toInt() == Qt::Checked) {
            doAutoLogin(hostName);
            openedProfile = true;
        }
    }

    if (!openedProfile) {
        slot_showConnectionDialog();
    }
}

// credit to https://github.com/DigitalInBlue/Celero/blob/master/src/Memory.cpp
int64_t mudlet::getPhysicalMemoryTotal()
{
#if defined(Q_OS_WIN32)
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    return static_cast<int64_t>(memInfo.ullTotalPhys);
#elif defined(Q_OS_HURD)
    // GNU/Hurd does not have a sysinfo struct  yet:
    errno = 0;
    int64_t pageSize = sysconf(_SC_PAGESIZE);
    if (pageSize < 0) {
        if (errno) {
            qDebug().nospace().noquote() << "mudlet::getPhysicalMemoryTotal() WARNING - error returned from sysconf(_SC_PAGESIZE); errno: " << errno;
        } else {
            qDebug().nospace().noquote() << "mudlet::getPhysicalMemoryTotal() WARNING - indeterminent limit returned from sysconf(_SC_PAGESIZE).";
        }
        return -1;
    }
    int64_t pageCount = sysconf(_SC_PHYS_PAGES);
    if (pageCount < 0) {
        if (errno) {
            qDebug().nospace().noquote() << "mudlet::getPhysicalMemoryTotal() WARNING - error returned from sysconf(_SC_PHYS_PAGES); errno: " << errno;
        } else {
            qDebug().nospace().noquote() << "mudlet::getPhysicalMemoryTotal() WARNING - indeterminent limit returned from sysconf(_SC_PHYS_PAGES).";
        }
        return -1;
    }
    return pageSize * pageCount;
#elif defined(Q_OS_MACOS)
    int mib[2];
    mib[0] = CTL_HW;
    mib[1] = HW_MEMSIZE;

    int64_t memInfo{0};
    auto len = sizeof(memInfo);

    if (!sysctl(mib, 2, &memInfo, &len, nullptr, 0)) {
        return memInfo;
    }

    return -1;
#elif defined(Q_OS_OPENBSD)
    // Very similar to MacOS but uses a different second level name
    int mib[2];
    mib[0] = CTL_HW;
    mib[1] = HW_PHYSMEM64; // Or do we really want HW_USERMEM64?

    int64_t memInfo{0};
    auto len = sizeof(memInfo);

    if (!sysctl(mib, 2, &memInfo, &len, nullptr, 0)) {
        return memInfo;
    }

    return -1;
#elif defined(Q_OS_UNIX)
    // Including both GNU/Linux and FreeBSD:
    // Prefer sysctl() over sysconf() except sysctl() HW_REALMEM and HW_PHYSMEM
    // return static_cast<int64_t>(sysconf(_SC_PHYS_PAGES)) * static_cast<int64_t>(sysconf(_SC_PAGE_SIZE));
    struct sysinfo memInfo;
    sysinfo(&memInfo);
    int64_t total = memInfo.totalram;
    return total * static_cast<int64_t>(memInfo.mem_unit);
#else
    return -1;
#endif
}

// Ensure the debug area is attached to at least one Host
void mudlet::attachDebugArea(const QString& hostname)
{
    if (smpDebugArea) {
        return;
    }

    smpDebugArea = new QMainWindow(nullptr);
    const auto pHost = mHostManager.getHost(hostname);
    smpDebugConsole = new TConsole(pHost, TConsole::CentralDebugConsole);
    smpDebugConsole->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    smpDebugConsole->setWrapAt(100);
    smpDebugArea->setCentralWidget(smpDebugConsole);
    smpDebugArea->setWindowTitle(tr("Central Debug Console"));
    smpDebugArea->setWindowIcon(QIcon(qsl(":/icons/mudlet_debug.png")));

    auto consoleCloser = new TConsoleMonitor(smpDebugArea);
    smpDebugArea->installEventFilter(consoleCloser);

    QSize generalRule(qApp->desktop()->size());
    generalRule -= QSize(30, 30);
    smpDebugArea->resize(QSize(800, 600).boundedTo(generalRule));
    smpDebugArea->hide();
}

void mudlet::doAutoLogin(const QString& profile_name)
{
    if (profile_name.isEmpty()) {
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
    bool preInstallPackages = false;
    if (entries.isEmpty()) {
        preInstallPackages = true;

        const auto it = TGameDetails::findGame(profile_name);
        if (it != TGameDetails::scmDefaultGames.end()) {
            pHost->setUrl((*it).hostUrl);
            pHost->setPort((*it).port);
            pHost->mSslTsl = (*it).tlsEnabled;
        }
    } else {
        QFile file(qsl("%1/%2").arg(folder, entries.at(0)));
        file.open(QFile::ReadOnly | QFile::Text);
        XMLimport importer(pHost);
        qDebug() << "[LOADING PROFILE]:" << file.fileName();
        importer.importPackage(&file); // TODO: Missing false return value handler
        pHost->refreshPackageFonts();

        // Is this a new profile created through 'copy profile (settings only)'? install default packages into it
        if (entries.size() == 1 && entries.first() == QLatin1String("Copied profile (settings only).xml")) {
            preInstallPackages = true;
        }
    }

    pHost->setLogin(readProfileData(profile_name, qsl("login")));
    pHost->setPass(readProfileData(profile_name, qsl("password")));

    QString val = readProfileData(profile_name, qsl("autoreconnect"));
    if (!val.isEmpty() && val.toInt() == Qt::Checked) {
        pHost->setAutoReconnect(true);
    } else {
        pHost->setAutoReconnect(false);
    }

    // This settings also need to be configured, note that the only time not to
    // save the setting is on profile loading:
    pHost->mTelnet.setEncoding(readProfileData(profile_name, qsl("encoding")).toUtf8(), false);

    if (preInstallPackages) {
        mudlet::self()->setupPreInstallPackages(pHost->getUrl().toLower());
    }

    emit signal_hostCreated(pHost, mHostManager.getHostCount());
    emit signal_adjustAccessibleNames();
    slot_connectionDialogueFinished(profile_name, true);
    enableToolbarButtons();
    updateMultiViewControls();
}

void mudlet::processEventLoopHack()
{
    QTimer::singleShot(1ms, this, &mudlet::slot_processEventLoopHackTimerRun);
}

void mudlet::slot_processEventLoopHackTimerRun()
{
    Host* pH = getActiveHost();
    if (!pH) {
        return;
    }
    pH->mpConsole->refresh();
}

void mudlet::slot_connectionDialogueFinished(const QString& profile, bool connect)
{
    Host* pHost = getHostManager().getHost(profile);
    if (!pHost) {
        return;
    }
    pHost->mIsProfileLoadingSequence = true;
    addConsoleForNewHost(pHost);
    pHost->mBlockScriptCompile = false;
    pHost->mLuaInterpreter.loadGlobal();
    pHost->hideMudletsVariables();

    //do modules here
    QMapIterator<QString, int> it(pHost->mModulePriorities);
    QMap<int, QStringList> moduleOrder;
    while (it.hasNext()) {
        it.next();
        QStringList moduleEntry = moduleOrder[it.value()];
        moduleEntry << it.key();
        moduleOrder[it.value()] = moduleEntry;
    }

    //First load modules with negative number priority
    QMapIterator<int, QStringList> it2(moduleOrder);
    while (it2.hasNext() && it2.peekNext().key() < 0) {
        it2.next();
        mudlet::installModulesList(pHost, it2.value());
    }

    pHost->mBlockStopWatchCreation = false;
    pHost->getScriptUnit()->compileAll();
    pHost->updateAnsi16ColorsInTable();

    //Load rest of modules after scripts
    while (it2.hasNext()) {
        it2.next();
        QStringList modules = it2.value();
        mudlet::installModulesList(pHost, modules);
    }

    // install default packages
    for (int i = 0; i < mPackagesToInstallList.size(); i++) {
        pHost->installPackage(mPackagesToInstallList[i], 0);
    }

    mPackagesToInstallList.clear();

    pHost->mIsProfileLoadingSequence = false;

    TEvent event {};
    event.mArgumentList.append(QLatin1String("sysLoadEvent"));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    pHost->raiseEvent(event);

    //NOTE: this is a potential problem if users connect by hand quickly
    //      and one host has a slower response time as the other one, but
    //      the worst that can happen is that they have to login manually.

    if (connect) {
        pHost->mTelnet.connectIt(pHost->getUrl(), pHost->getPort());
    } else {
        QString infoMsg = tr("[  OK  ]  - Profile \"%1\" loaded in offline mode.").arg(profile);
        pHost->postMessage(infoMsg);
    }
}

void mudlet::installModulesList(Host* pHost, QStringList modules)
{
    for (int i = 0; i < modules.size(); i++) {
        QStringList entry = pHost->mInstalledModules[modules[i]];
        pHost->installPackage(entry[0], 1);
        //we repeat this step here b/c we use the same installPackage method for initial loading,
        //where we overwrite the globalSave flag.  This restores saved and loaded packages to their proper flag
        pHost->mInstalledModules[modules[i]] = entry;
    }
}

// Connected to and needed by the shortcut to trigger the menu or toolbar button
// action because it does not provide the checked state of the item to which the
// shortcut is associated:
void mudlet::slot_toggleMultiView()
{
    const bool newState = !mMultiView;
    slot_multiView(newState);
}

// Connected to a menu and toolbar button (but not a short-cut to one of them)
// as they provide their checked state directly:
void mudlet::slot_multiView(const bool state)
{
    // Ensure the state of both controls is updated to reflect the state of the
    // option:
    if (mpActionMultiView->isChecked() != state) {
        mpActionMultiView->setChecked(state);
    }
    if (dactionMultiView->isChecked() != state) {
        dactionMultiView->setChecked(state);
    }
    mMultiView = state;
    bool foundActiveHost = false;
    for (auto pHost : mHostManager) {
        auto console = pHost->mpConsole;
        if (!console) {
            continue;
        }
        if (mpCurrentActiveHost && (&*mpCurrentActiveHost == &*pHost)) {
            // After switching the option off need to redraw the, now only, main
            // TConsole to be displayed for the currently active profile:
            foundActiveHost = true;
            console->show();
        } else if (mMultiView) {
            console->show();
        } else {
            console->hide();
        }
    }
    if (!foundActiveHost && mpTabBar->count()) {
        // If there IS at least one profile still active, but none of them WAS
        // the active one then make one (the first) the active one:
        slot_tabChanged(0);
    }
}

// Called by the short-cut to the menu item that doesn't pass the checked state
// of the menu-item that it provides a short-cut to:
void mudlet::slot_toggleCompactInputLine()
{
    const bool newState = !dactionInputLine->isChecked();
    slot_compactInputLine(newState);
}

// Called by the menu-item's action itself, that DOES pass the checked state:
void mudlet::slot_compactInputLine(const bool state)
{
    if (dactionInputLine->isChecked() != state) {
        // Ensure the menu item reflectes the actual state:
        dactionInputLine->setChecked(state);
    }
    if (mpCurrentActiveHost) {
        mpCurrentActiveHost->setCompactInputLine(state);
        // Make sure players don't get confused when accidentally hiding buttons.
        if (QKeySequence* shortcut = mpShortcutsManager->getSequence(qsl("Compact input line")); state && !mpCurrentActiveHost->mTutorialForCompactLineAlreadyShown && shortcut && !shortcut->isEmpty()) {
            QString infoMsg = tr("[ INFO ]  - Compact input line set. Press %1 to show bottom-right buttons again.",
                                 "Here %1 will be replaced with the keyboard shortcut, default is ALT+L.").arg(shortcut->toString());
            mpCurrentActiveHost->postMessage(infoMsg);
            mpCurrentActiveHost->mTutorialForCompactLineAlreadyShown = true;
        }
    }
}

mudlet::~mudlet()
{
    // There may be a corner case if a replay is running AND the application is
    // closing down AND the updater on a particular platform pauses the
    // application destruction...?
    delete (mpTimerReplay);
    mpTimerReplay = nullptr;

    if (mpHunspell_sharedDictionary) {
        saveDictionary(getMudletPath(mainDataItemPath, qsl("mudlet")), mWordSet_shared);
        mpHunspell_sharedDictionary = nullptr;
    }
    if (!mTranslatorsLoadedList.isEmpty()) {
        qDebug().nospace().noquote() << "mudlet::~mudlet() INFO - uninstalling translation...";
        QMutableListIterator<QPointer<QTranslator>> itTranslator(mTranslatorsLoadedList);
        itTranslator.toBack();
        while (itTranslator.hasPrevious()) {
            QPointer<QTranslator> pTranslator = itTranslator.previous();
            if (pTranslator) {
                qApp->removeTranslator(pTranslator);
                itTranslator.remove();
                delete pTranslator;
            }
        }
    }
    mudlet::smpSelf = nullptr;
}

void mudlet::slot_toggleFullScreenView()
{
    if (isFullScreen()) {
        showNormal();
    } else {
        showFullScreen();
    }
}

// Called from the ctelnet instance for the host concerned:
bool mudlet::replayStart()
{
    // Do not proceed if there is a problem with the main toolbar (it isn't there)
    // OR if there is already a replay toolbar in existence (a replay is already
    // in progress)...
    if (!mpMainToolBar || mpToolBarReplay) {
        return false;
    }

    // Lock the replay button and menu item down until the replay is over
    mpActionReplay->setCheckable(true);
    mpActionReplay->setChecked(true);
    mpActionReplay->setEnabled(false);
    dactionReplay->setEnabled(false);
    mpActionReplay->setToolTip(utils::richText(tr("Cannot load a replay as one is already in progress in this or another profile.")));
    dactionReplay->setToolTip(mpActionReplay->toolTip());

    mpToolBarReplay = new QToolBar(this);
    mpToolBarReplay->setIconSize(QSize(8 * mToolbarIconSize, 8 * mToolbarIconSize));
    mpToolBarReplay->setToolButtonStyle(mpMainToolBar->toolButtonStyle());

    mReplaySpeed = 1;
    mReplayTime.setHMS(0, 0, 0, 1); // Since Qt5.0 adding anything to a zero
                                    // (invalid) time leaves the time value
                                    // STILL being regarded as invalid - so to
                                    // get a valid time we have to use a very
                                    // small, NON-zero time to initiase it...!

    mpLabelReplayTime = new QLabel(this);
    mpActionReplayTime = mpToolBarReplay->addWidget(mpLabelReplayTime);

    mpActionReplaySpeedUp = new QAction(QIcon(qsl(":/icons/export.png")), tr("Faster"), this);
    mpActionReplaySpeedUp->setObjectName(qsl("replay_speed_up_action"));
    mpActionReplaySpeedUp->setToolTip(utils::richText(tr("Replay each step with a shorter time interval between steps.")));
    mpToolBarReplay->addAction(mpActionReplaySpeedUp);
    mpToolBarReplay->widgetForAction(mpActionReplaySpeedUp)->setObjectName(mpActionReplaySpeedUp->objectName());

    mpActionReplaySpeedDown = new QAction(QIcon(qsl(":/icons/import.png")), tr("Slower"), this);
    mpActionReplaySpeedDown->setObjectName(qsl("replay_speed_down_action"));
    mpActionReplaySpeedDown->setToolTip(utils::richText(tr("Replay each step with a longer time interval between steps.")));
    mpToolBarReplay->addAction(mpActionReplaySpeedDown);
    mpToolBarReplay->widgetForAction(mpActionReplaySpeedDown)->setObjectName(mpActionReplaySpeedDown->objectName());

    mpLabelReplaySpeedDisplay = new QLabel(this);
    mpActionSpeedDisplay = mpToolBarReplay->addWidget(mpLabelReplaySpeedDisplay);

    connect(mpActionReplaySpeedUp.data(), &QAction::triggered, this, &mudlet::slot_replaySpeedUp);
    connect(mpActionReplaySpeedDown.data(), &QAction::triggered, this, &mudlet::slot_replaySpeedDown);

    mpLabelReplaySpeedDisplay->setText(qsl("<font size=25><b>%1</b></font>").arg(tr("Speed: X%1").arg(mReplaySpeed)));

    mpTimerReplay = new QTimer(this);
    mpTimerReplay->setInterval(1000);
    mpTimerReplay->setSingleShot(false);
    connect(mpTimerReplay.data(), &QTimer::timeout, this, &mudlet::slot_replayTimeChanged);

    mpLabelReplayTime->setText(qsl("<font size=25><b>%1</b></font>").arg(tr("Time: %1").arg(mReplayTime.toString(mTimeFormat))));

    mpLabelReplaySpeedDisplay->show();
    mpLabelReplayTime->show();

    insertToolBar(mpMainToolBar, mpToolBarReplay);

    mpToolBarReplay->show();
    mpTimerReplay->start();
    return true;
}

void mudlet::slot_replayTimeChanged()
{
    // This can get called by a QTimer after mpLabelReplayTime has been destroyed:
    if (mpLabelReplayTime) {
        mpLabelReplayTime->setText(qsl("<font size=25><b>%1</b></font>")
                                   .arg(tr("Time: %1").arg(mReplayTime.toString(mTimeFormat))));
        mpLabelReplayTime->show();
    }
}

void mudlet::replayOver()
{
    if ((!mpMainToolBar) || (!mpToolBarReplay)) {
        return;
    }

    disconnect(mpActionReplaySpeedUp.data(), &QAction::triggered, this, &mudlet::slot_replaySpeedUp);
    disconnect(mpActionReplaySpeedDown.data(), &QAction::triggered, this, &mudlet::slot_replaySpeedDown);
    mpToolBarReplay->removeAction(mpActionReplaySpeedUp);
    mpToolBarReplay->removeAction(mpActionReplaySpeedDown);
    mpToolBarReplay->removeAction(mpActionSpeedDisplay);
    removeToolBar(mpToolBarReplay);
    mpActionReplaySpeedUp->deleteLater(); // Had previously omitted these, causing a resource leak!
    mpActionReplaySpeedUp = nullptr;
    mpActionReplaySpeedDown->deleteLater();
    mpActionReplaySpeedDown = nullptr;
    mpActionSpeedDisplay->deleteLater();
    mpActionSpeedDisplay = nullptr;
    mpActionReplayTime->deleteLater();
    mpActionReplayTime = nullptr;
    mpLabelReplaySpeedDisplay->deleteLater();
    mpLabelReplaySpeedDisplay = nullptr;
    mpLabelReplayTime->deleteLater();
    mpLabelReplayTime = nullptr;
    mpToolBarReplay->deleteLater();
    mpToolBarReplay = nullptr;

    // Unlock/uncheck the replay button/menu item
    mpActionReplay->setChecked(false);
    mpActionReplay->setCheckable(false);
    mpActionReplay->setEnabled(true);
    dactionReplay->setEnabled(true);
    mpActionReplay->setToolTip(utils::richText(tr("Load a Mudlet replay.")));
    dactionReplay->setToolTip(mpActionReplay->toolTip());
}

void mudlet::slot_replaySpeedUp()
{
    if (mpLabelReplaySpeedDisplay) {
        mReplaySpeed = qMin(1024, mReplaySpeed * 2);
        mpLabelReplaySpeedDisplay->setText(qsl("<font size=25><b>%1</b></font>").arg(tr("Speed: X%1").arg(mReplaySpeed)));
        mpLabelReplaySpeedDisplay->show();
    }
}

void mudlet::slot_replaySpeedDown()
{
    if (mpLabelReplaySpeedDisplay) {
        mReplaySpeed = qMax(1, mReplaySpeed / 2);
        mpLabelReplaySpeedDisplay->setText(qsl("<font size=25><b>%1</b></font>").arg(tr("Speed: X%1").arg(mReplaySpeed)));
        mpLabelReplaySpeedDisplay->show();
    }
}

void mudlet::setEditorTextoptions(const bool isTabsAndSpacesToBeShown, const bool isLinesAndParagraphsToBeShown)
{
    mEditorTextOptions = QTextOption::Flags((isTabsAndSpacesToBeShown ? QTextOption::ShowTabsAndSpaces : QTextOption::Flag())
                                           |(isLinesAndParagraphsToBeShown ? QTextOption::ShowLineAndParagraphSeparators : QTextOption::Flag()));
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
            QString entryInArchive(zs.name);
            QString pathInArchive(entryInArchive.section(qsl("/"), 0, -2));
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
        QString folderToCreate = qsl("%1%2").arg(destination, itPath.value());
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
        QString entryInArchive(zs.name);
        if (!entryInArchive.endsWith(QLatin1Char('/'))) {
            // TODO: check that zs.size is valid ( zs.valid & ZIP_STAT_SIZE )
            zf = zip_fopen_index(archive, static_cast<zip_uint64_t>(i), 0);
            if (!zf) {
                zip_close(archive);
                return false;
            }

            QFile fd(qsl("%1%2").arg(destination, entryInArchive));

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

//loads the luaFunctionList for use by the edbee Autocompleter
bool mudlet::loadLuaFunctionList()
{
    auto jsonFile = QFile(qsl(":/lua-function-list.json"));
    if (!jsonFile.open(QFile::ReadOnly)) {
        return false;
    }

    const QByteArray data = jsonFile.readAll();
    jsonFile.close();

    auto json_doc = QJsonDocument::fromJson(data);

    if (json_doc.isNull() || !json_doc.isObject()) {
        return false;
    }

    QJsonObject json_obj = json_doc.object();

    if (json_obj.isEmpty()) {
        return false;
    }

    mudlet::smLuaFunctionNames = json_obj.toVariantHash();

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
    TEvent event {};
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
    TEvent event {};
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
    TEvent event {};
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
    TEvent event {};
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
    TEvent event {};
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

// Convenience helper - may aide things if we want to put files in a different
// place...!
QString mudlet::getMudletPath(const mudletPathType mode, const QString& extra1, const QString& extra2)
{
    switch (mode) {
    case mainPath:
        // The root of all mudlet data for the user - does not end in a '/'
        return qsl("%1/.config/mudlet").arg(QDir::homePath());
    case mainDataItemPath:
        // Takes one extra argument as a file (or directory) relating to
        // (profile independent) mudlet data - may end with a '/' if the extra
        // argument does:
        return qsl("%1/.config/mudlet/%2").arg(QDir::homePath(), extra1);
    case mainFontsPath:
        // (Added for 3.5.0) a revised location to store Mudlet provided fonts
        return qsl("%1/.config/mudlet/fonts").arg(QDir::homePath());
    case profilesPath:
        // The directory containing all the saved user's profiles - does not end
        // in '/'
        return qsl("%1/.config/mudlet/profiles").arg(QDir::homePath());
    case profileHomePath:
        // Takes one extra argument (profile name) that returns the base
        // directory for that profile - does NOT end in a '/' unless the
        // supplied profle name does:
        return qsl("%1/.config/mudlet/profiles/%2").arg(QDir::homePath(), extra1);
    case profileMediaPath:
        // Takes one extra argument (profile name) that returns the directory
        // for the profile's cached media files - does NOT end in a '/'
        return qsl("%1/.config/mudlet/profiles/%2/media").arg(QDir::homePath(), extra1);
    case profileMediaPathFileName:
        // Takes two extra arguments (profile name, mediaFileName) that returns
        // the pathFile name for any media file:
        return qsl("%1/.config/mudlet/profiles/%2/media/%3").arg(QDir::homePath(), extra1, extra2);
    case profileXmlFilesPath:
        // Takes one extra argument (profile name) that returns the directory
        // for the profile game save XML files - ends in a '/'
        return qsl("%1/.config/mudlet/profiles/%2/current/").arg(QDir::homePath(), extra1);
    case profileMapsPath:
        // Takes one extra argument (profile name) that returns the directory
        // for the profile game save maps files - does NOT end in a '/'
        return qsl("%1/.config/mudlet/profiles/%2/map").arg(QDir::homePath(), extra1);
    case profileDateTimeStampedMapPathFileName:
        // Takes two extra arguments (profile name, dataTime stamp) that returns
        // the pathFile name for a dateTime stamped map file:
        return qsl("%1/.config/mudlet/profiles/%2/map/%3map.dat").arg(QDir::homePath(), extra1, extra2);
    case profileDateTimeStampedJsonMapPathFileName:
        // Takes two extra arguments (profile name, dataTime stamp) that returns
        // the pathFile name for a dateTime stamped JSON map file:
        return qsl("%1/.config/mudlet/profiles/%2/map/%3map.json").arg(QDir::homePath(), extra1, extra2);
    case profileMapPathFileName:
        // Takes two extra arguments (profile name, mapFileName) that returns
        // the pathFile name for any map file:
        return qsl("%1/.config/mudlet/profiles/%2/map/%3").arg(QDir::homePath(), extra1, extra2);
    case profileXmlMapPathFileName:
        // Takes one extra argument (profile name) that returns the pathFile
        // name for the downloaded IRE Server provided XML map:
        return qsl("%1/.config/mudlet/profiles/%2/map.xml").arg(QDir::homePath(), extra1);
    case profileDataItemPath:
        // Takes two extra arguments (profile name, data item) that gives a
        // path file name for, typically a data item stored as a single item
        // (binary) profile data) file (ideally these can be moved to a per
        // profile QSettings file but that is a future pipe-dream on my part
        // SlySven):
        return qsl("%1/.config/mudlet/profiles/%2/%3").arg(QDir::homePath(), extra1, extra2);
    case profilePackagePath:
        // Takes two extra arguments (profile name, package name) returns the
        // per profile directory used to store (unpacked) package contents
        // - ends with a '/':
        return qsl("%1/.config/mudlet/profiles/%2/%3/").arg(QDir::homePath(), extra1, extra2);
    case profilePackagePathFileName:
        // Takes two extra arguments (profile name, package name) returns the
        // filename of the XML file that contains the (per profile, unpacked)
        // package mudlet items in that package/module:
        return qsl("%1/.config/mudlet/profiles/%2/%3/%3.xml").arg(QDir::homePath(), extra1, extra2);
    case profileReplayAndLogFilesPath:
        // Takes one extra argument (profile name) that returns the directory
        // that contains replays (*.dat files) and logs (*.html or *.txt) files
        // for that profile - does NOT end in '/':
        return qsl("%1/.config/mudlet/profiles/%2/log").arg(QDir::homePath(), extra1);
    case profileLogErrorsFilePath:
        // Takes one extra argument (profile name) that returns the pathFileName
        // to the map auditing report file that is appended to each time a
        // map is loaded:
        return qsl("%1/.config/mudlet/profiles/%2/log/errors.txt").arg(QDir::homePath(), extra1);
    case editorWidgetThemePathFile:
        // Takes two extra arguments (profile name, theme name) that returns the
        // pathFileName of the theme file used by the edbee editor - also
        // handles the special case of the default theme "mudlet.tmTheme" that
        // is carried internally in the resource file:
        if (extra1.compare(qsl("Mudlet.tmTheme"), Qt::CaseSensitive)) {
            // No match
            return qsl("%1/.config/mudlet/edbee/Colorsublime-Themes-master/themes/%2").arg(QDir::homePath(), extra1);
        } else {
            // Match - return path to copy held in resource file
            return qsl(":/edbee_defaults/Mudlet.tmTheme");
        }
    case editorWidgetThemeJsonFile:
        // Returns the pathFileName to the external JSON file needed to process
        // an edbee editor widget theme:
        return qsl("%1/.config/mudlet/edbee/Colorsublime-Themes-master/themes.json").arg(QDir::homePath());
    case moduleBackupsPath:
        // Returns the directory used to store module backups that is used in
        // when saving/resyncing packages/modules - ends in a '/'
        return qsl("%1/.config/mudlet/moduleBackups/").arg(QDir::homePath());
    case qtTranslationsPath:
        return QLibraryInfo::location(QLibraryInfo::TranslationsPath);
    case hunspellDictionaryPath:
        // Added for 3.18.0 when user dictionary capability added
#if defined(Q_OS_MACOS)
        mudlet::self()->mUsingMudletDictionaries = true;
        return qsl("%1/../Resources/").arg(QCoreApplication::applicationDirPath());
#elif defined(Q_OS_FREEBSD)
        if (QFile::exists(qsl("/usr/local/share/hunspell/%1.aff").arg(extra1))) {
            mudlet::self()->mUsingMudletDictionaries = false;
            return QLatin1String("/usr/local/share/hunspell/");
        } else if (QFile::exists(qsl("/usr/share/hunspell/%1.aff").arg(extra1))) {
            mudlet::self()->mUsingMudletDictionaries = false;
            return QLatin1String("/usr/share/hunspell/");
        } else if (QFile::exists(qsl("%1/../../src/%2.aff").arg(QCoreApplication::applicationDirPath(), extra1))) {
            // From debug or release subdirectory of a shadow build directory alongside the ./src one:
            mudlet::self()->mUsingMudletDictionaries = true;
            return qsl("%1/../../src/").arg(QCoreApplication::applicationDirPath());
        } else if (QFile::exists(qsl("%1/../src/%2.aff").arg(QCoreApplication::applicationDirPath(), extra1))) {
            // From shadow build directory alongside the ./src one:
            mudlet::self()->mUsingMudletDictionaries = true;
            return qsl("%1/../src/").arg(QCoreApplication::applicationDirPath());
        } else {
            // From build within ./src
            mudlet::self()->mUsingMudletDictionaries = true;
            return qsl("%1/").arg(QCoreApplication::applicationDirPath());
        }
#elif defined(Q_OS_OPENBSD)
        // OpenBSD uses dictionary files from Mozilla rather than direct from,
        // Hunspell, but it does not ship a en_us one so we cannot use that on
        // the first run to find the rest - instead try for the en_GB one
        // - some of the entries for some of the locale/language/other parts of
        // the filesnames seem to be a bit random:
        if (QFile::exists(qsl("/usr/local/share/mozilla-dicts/%1.aff").arg(extra1))) {
            mudlet::self()->mUsingMudletDictionaries = false;
            return QLatin1String("/usr/local/share/mozilla-dicts/");
        } else if (QFile::exists(qsl("/usr/share/mozilla-dicts/%1.aff").arg(extra1))) {
            mudlet::self()->mUsingMudletDictionaries = false;
            return QLatin1String("/usr/share/mozilla-dicts/");
        } else if (QFile::exists(qsl("%1/../../src/%2.aff").arg(QCoreApplication::applicationDirPath(), extra1))) {
            // From debug or release subdirectory of a shadow build directory alongside the ./src one:
            mudlet::self()->mUsingMudletDictionaries = true;
            return qsl("%1/../../src/").arg(QCoreApplication::applicationDirPath());
        } else if (QFile::exists(qsl("%1/../src/%2.aff").arg(QCoreApplication::applicationDirPath(), extra1))) {
            // From shadow build directory alongside the ./src one:
            mudlet::self()->mUsingMudletDictionaries = true;
            return qsl("%1/../src/").arg(QCoreApplication::applicationDirPath());
        } else {
            // From build within ./src
            mudlet::self()->mUsingMudletDictionaries = true;
            return qsl("%1/").arg(QCoreApplication::applicationDirPath());
        }
#elif defined(Q_OS_LINUX)
        if (QFile::exists(qsl("/usr/share/hunspell/%1.aff").arg(extra1))) {
            mudlet::self()->mUsingMudletDictionaries = false;
            return QLatin1String("/usr/share/hunspell/");
        } else if (QFile::exists(qsl("%1/../../src/%2.aff").arg(QCoreApplication::applicationDirPath(), extra1))) {
            // From debug or release subdirectory of a shadow build directory
            // alongside the ./src one. {Typically QMake builds from Qtcreator
            // with CONFIG containing both 'debug_and_release' and
            // 'debug_and_release_target' (this is normal also on Windows):
            mudlet::self()->mUsingMudletDictionaries = true;
            return qsl("%1/../../src/").arg(QCoreApplication::applicationDirPath());
        } else if (QFile::exists(qsl("%1/../src/%2.aff").arg(QCoreApplication::applicationDirPath(), extra1))) {
            // From shadow build directory alongside the ./src one. {Typically
            // QMake builds from Qtcreator with CONFIG NOT containing both
            // 'debug_and_release' and 'debug_and_release_target':
            mudlet::self()->mUsingMudletDictionaries = true;
            return qsl("%1/../src/").arg(QCoreApplication::applicationDirPath());
        } else if (QFile::exists(qsl("%1/../../mudlet/src/%2.aff").arg(QCoreApplication::applicationDirPath(), extra1))) {
            // From shadow build directory above the ./src one. {Typically
            // CMake builds from Qtcreator which are outside of the unpacked
            // source code from a git repo or tarball - which has to have been
            // unpacked/placed in a directory called 'mudlet'}:
            mudlet::self()->mUsingMudletDictionaries = true;
            return qsl("%1/../../mudlet/src/").arg(QCoreApplication::applicationDirPath());
        } else {
            // From build within ./src AND installer builds that bundle
            // dictionaries in the same directory as the executable:
            mudlet::self()->mUsingMudletDictionaries = true;
            return qsl("%1/").arg(QCoreApplication::applicationDirPath());
        }
#else
        // Probably Windows!
        mudlet::self()->mUsingMudletDictionaries = true;
        if (QFile::exists(qsl("%1/../../src/%2.aff").arg(QCoreApplication::applicationDirPath(), extra1))) {
            // From debug or release subdirectory of a shadow build directory alongside the ./src one:
            return qsl("%1/../../src/").arg(QCoreApplication::applicationDirPath());
        } else if (QFile::exists(qsl("%1/../src/%2.aff").arg(QCoreApplication::applicationDirPath(), extra1))) {
            // From shadow build directory alongside the ./src one:
            return qsl("%1/../src/").arg(QCoreApplication::applicationDirPath());
        } else {
            // From build within ./src
            return qsl("%1/").arg(QCoreApplication::applicationDirPath());
        }
#endif
    }
    Q_UNREACHABLE();
    return QString();
}

#if defined(INCLUDE_UPDATER)
void mudlet::checkUpdatesOnStart()
{
    if (scmIsReleaseVersion || scmIsPublicTestVersion) {
        // Only try and create an updater (which checks for updates online) if
        // this is a release/public test version:
        pUpdater->checkUpdatesOnStart();
    }
}

void mudlet::slot_manualUpdateCheck()
{
    pUpdater->manuallyCheckUpdates();
}

void mudlet::slot_reportIssue()
{
    QDesktopServices::openUrl(QUrl(qsl("https://github.com/Mudlet/Mudlet/issues/new")));
}

// Means to turn-off the hard coded popup delay in QActions provided by:
// https://stackoverflow.com/a/30126063/4805858
void mudlet::slot_updateAvailable(const int updateCount)
{
    // Removes the normal click to activate "About Mudlet" action and move it
    // to a new menu which also contains a "goto updater" option

    if (mpActionAboutWithUpdates) {
        mpMainToolBar->removeAction(mpActionAboutWithUpdates);
    }

    // First change the existing button:
    if (!qApp->testAttribute(Qt::AA_DontShowIconsInMenus)) {
        mpActionAbout->setIcon(QIcon(":/icons/mudlet_information.png"));
    } else {
        mpActionAbout->setIcon(QIcon());
    }
    mpActionAbout->setToolTip(utils::richText(tr("Inform yourself about this version of Mudlet, the people who made it and the licence under which you can share it.",
                                                 // Intentional comment
                                                 "Tooltip for About Mudlet sub-menu item and main toolbar button (or menu item if an update has changed that control to have a popup menu instead) (Used in 3 places - "
                                                 "please ensure all have the same translation).")));

    // Create a new button (QActions actually turn into QToolButtons when they
    // are placed on a QToolBar - but we need to generate one ourselves so we
    // can modify the popupMode away from the delayed one that is hardwared into
    // the transmuted QAction-to-QToolButton ones):
    mpButtonAbout = new QToolButton();
    mpButtonAbout->setIcon(QIcon(qsl(":/icons/mudlet_important.png")));
    mpButtonAbout->setToolTip(tr("<p>About Mudlet</p>"
                                 "<p><i>%n update(s) is/are now available!</i><p>",
                                 // Intentional comment
                                 "This is the tooltip text for the 'About' Mudlet main toolbar button when it has been changed by adding a menu which now contains the original 'About Mudlet' action "
                                 "and a new one to access the manual update process",
                                 updateCount));
    mpButtonAbout->setText(tr("About"));
    mpButtonAbout->setPopupMode(QToolButton::InstantPopup);
    // Now insert our new button after the current QAction/QToolButton
    mpActionAboutWithUpdates = mpMainToolBar->insertWidget(mpActionAbout, mpButtonAbout);
    // And quickly pull out the old QAction/QToolButton:
    mpMainToolBar->removeAction(mpActionAbout);

    // Create the new menu
    QMenu* pUpdateMenu = new QMenu();
    // Stuff the QAction/QToolButton we just pulled into the new menu
    pUpdateMenu->insertAction(nullptr, mpActionAbout);
    // We can then add in the new item to give access the update(s)
    auto pActionReview = pUpdateMenu->addAction(tr("Review %n update(s)...",
                                                   // Intentional comment
                                                   "Review update(s) menu item, %n is the count of how many updates are available",
                                                   updateCount),
                                                this, &mudlet::slot_manualUpdateCheck);
    pActionReview->setToolTip(utils::richText(tr("Review the update(s) available...",
                                                 // Intentional comment
                                                 "Tool-tip for review update(s) menu item, given that the count of how many updates are available is already shown in the menu, the %n parameter that is that number need not be used here",
                                                 updateCount)));
    // Override the default hide tooltips state:
    pUpdateMenu->setToolTipsVisible(true);
    // Screw the menu onto the button
    mpButtonAbout->setMenu(pUpdateMenu);
    // And force the button to adopt the current style of the existing buttons
    // - it doesn't by default:
    mpButtonAbout->setToolButtonStyle(mpMainToolBar->toolButtonStyle());
}

void mudlet::slot_updateInstalled()
{
// can't comment out method entirely as moc chokes on it, so leave a stub
#if !defined(Q_OS_MACOS)
    // disable existing functionality to show the updates window
    disconnect(dactionUpdate, &QAction::triggered, this, nullptr);

    // rejig to restart Mudlet instead
    QObject::connect(dactionUpdate, &QAction::triggered, this, [=]() {
        forceClose();
        QProcess::startDetached(qApp->arguments()[0], qApp->arguments());
    });
    dactionUpdate->setText(tr("Update installed - restart to apply"));
#endif // !Q_OS_MACOS
}

void mudlet::showChangelogIfUpdated()
{
    if (!pUpdater->shouldShowChangelog()) {
        return;
    }

    pUpdater->showChangelog();
}
#endif // INCLUDE_UPDATER

// Can be called from lua sub-system OR from slot_replay(), the presence of a
// non-NULLPTR pErrMsg indicates the former; also the replayFileName CAN be
// relative (to the profiles ./log sub-directory where replays are stored) if
// sourced from the lua sub-system.
bool mudlet::loadReplay(Host* pHost, const QString& replayFileName, QString* pErrMsg)
{
    // Do not proceed if there is a problem with the main toolbar (it isn't there)
    // OR if there is already a replay toolbar in existence (a replay is already
    // in progress)...
    if (!mpMainToolBar || mpToolBarReplay) {
        // This was in (bool) ctelnet::loadReplay(const QString&, QString*)
        // but is needed here to prevent getting into there otherwise a lua call
        // to start a replay would mess up (QFile) ctelnet::replayFile for a
        // replay already in progress in the SAME profile.  Technically there
        // could be a very small chance of a race condition if a lua call of
        // loadReplay happens at the same time as a file was selected for a
        // replay after the toolbar/menu command to do a reaply for the same
        // profile - but the window for this is likely to be a fraction of a
        // second...
        if (pErrMsg) {
            *pErrMsg = qsl("cannot perform replay, another one seems to already be in progress; try again when it has finished.");
        } else {
            pHost->postMessage(tr("[ WARN ]  - Cannot perform replay, another one may already be in progress,\n"
                                  "try again when it has finished."));
        }
        return false;
    }

    QString absoluteReplayFileName;
    if (QFileInfo(replayFileName).isRelative()) {
        absoluteReplayFileName = qsl("%1/%2").arg(mudlet::getMudletPath(mudlet::profileReplayAndLogFilesPath, pHost->getName()), replayFileName);
    } else {
        absoluteReplayFileName = replayFileName;
    }

    return pHost->mTelnet.loadReplay(absoluteReplayFileName, pErrMsg);
}

void mudlet::slot_newDataOnHost(const QString& hostName, const bool isLowerPriorityChange)
{
    if (mMultiView) {
        // We do not need to mark tabs with activity if they are all on show anyhow:
        return;
    }
    Host* pHost = mHostManager.getHost(hostName);
    if (pHost && pHost != mpCurrentActiveHost) {
        if (mpTabBar->count() > 1) {
            if (!isLowerPriorityChange) {
                mpTabBar->setTabBold(hostName, true);
                mpTabBar->setTabItalic(hostName, false);
                mpTabBar->update();
            } else if (isLowerPriorityChange && !mpTabBar->tabBold(hostName)) {
                // Local, lower priority change so only change the
                // styling if it is not already modified - so that the
                // higher priority remote change indication will not
                // get changed by a later local one:
                mpTabBar->setTabItalic(hostName, true);
                mpTabBar->update();
            }
        }
    }
}

QStringList mudlet::getAvailableFonts()
{
    QFontDatabase database;

    return database.families(QFontDatabase::Any);
}

std::string mudlet::replaceString(std::string subject, const std::string& search, const std::string& replace)
{
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
         subject.replace(pos, search.length(), replace);
         pos += replace.length();
    }
    return subject;
}

void mudlet::setEnableFullScreenMode(const bool state)
{
    // PLACEMARKER: Full-screen mode controlled by File (2 of 2) At some point we might consider removal of all but the first line of the "if" branch of code and drop maintaining the sentinel file presence/absence:
    if (state != mEnableFullScreenMode) {
        mEnableFullScreenMode = state;
        QFile file_use_smallscreen(mudlet::getMudletPath(mudlet::mainDataItemPath, qsl("mudlet_option_use_smallscreen")));
        if (state) {
            file_use_smallscreen.open(QIODevice::WriteOnly | QIODevice::Text);
            QTextStream out(&file_use_smallscreen);
            Q_UNUSED(out);
            file_use_smallscreen.close();
        } else {
            file_use_smallscreen.remove();
        }
    }

    // Emit the signal whatever the stored value is - so that if there are
    // multiple profile preference dialogs open they all update themselves:
    emit signal_enableFulScreenModeChanged(state);
}

bool mudlet::migratePasswordsToSecureStorage()
{
    if (!mProfilePasswordsToMigrate.isEmpty()) {
        qWarning() << "mudlet::migratePasswordsToSecureStorage() warning: password migration is already in progress, won't start another.";
        return false;
    }
    mStorePasswordsSecurely = true;

    QStringList profiles = QDir(mudlet::getMudletPath(mudlet::profilesPath))
                                   .entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

    for (const auto& profile : profiles) {
        const auto password = readProfileData(profile, qsl("password"));
        if (password.isEmpty()) {
            continue;
        }

        auto *job = new QKeychain::WritePasswordJob(qsl("Mudlet profile"));
        job->setAutoDelete(false);
        job->setInsecureFallback(false);

        job->setKey(profile);
        job->setTextData(password);
        job->setProperty("profile", profile);

        mProfilePasswordsToMigrate.append(profile);

        connect(job, &QKeychain::WritePasswordJob::finished, this, &mudlet::slot_passwordMigratedToSecureStorage);

        job->start();
    }

    if (mProfilePasswordsToMigrate.isEmpty()) {
        QTimer::singleShot(0, this, [this]() {
            emit signal_passwordsMigratedToProfiles();
        });
    }

    return true;
}

void mudlet::slot_passwordMigratedToSecureStorage(QKeychain::Job* job)
{
    const auto profileName = job->property("profile").toString();
    if (job->error()) {
        qWarning().nospace().noquote() << "mudlet::slot_passwordMigratedToSecureStorage(...) ERROR - could not migrate for \"" << profileName << "\"; error was: \"" << job->errorString() << "\".";
    } else {
        deleteProfileData(profileName, qsl("password"));
    }
    mProfilePasswordsToMigrate.removeAll(profileName);
    job->deleteLater();

    if (mProfilePasswordsToMigrate.isEmpty()) {
        emit signal_passwordsMigratedToSecure();
    } else {
        emit signal_passwordMigratedToSecure(profileName);
    }
}

bool mudlet::migratePasswordsToProfileStorage()
{
    if (!mProfilePasswordsToMigrate.isEmpty()) {
        qWarning() << "mudlet::migratePasswordsToProfileStorage() WARNING - password migration is already in progress, so not starting a duplicate action.";
        return false;
    }
    mStorePasswordsSecurely = false;

    QStringList profiles = QDir(mudlet::getMudletPath(mudlet::profilesPath)).entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

    for (const auto& profile : profiles) {
        auto* job = new QKeychain::ReadPasswordJob(qsl("Mudlet profile"));
        job->setAutoDelete(false);
        job->setInsecureFallback(false);
        job->setKey(profile);
        job->setProperty("profile", profile);
        mProfilePasswordsToMigrate.append(profile);

        connect(job, &QKeychain::ReadPasswordJob::finished, this, &mudlet::slot_passwordMigratedToPortableStorage);
        job->start();
    }

    if (mProfilePasswordsToMigrate.isEmpty()) {
        QTimer::singleShot(0, this, [this]() {
            emit signal_passwordsMigratedToProfiles();
        });
    }
    return true;
}

void mudlet::slot_passwordMigratedToPortableStorage(QKeychain::Job* job)
{
    const auto profileName = job->property("profile").toString();

    if (job->error()) {
        const auto error = job->errorString();
        if (error != qsl("Entry not found") && error != qsl("No match")) {
            qWarning().nospace().noquote() << "mudlet::slot_passwordMigratedToPortableStorage(...) ERROR - could not migrate for \"" << profileName << "\"; error was: " << error << ".";
        }

    } else {
        auto readJob = static_cast<QKeychain::ReadPasswordJob*>(job);
        writeProfileData(profileName, qsl("password"), readJob->textData());

        // delete from secure storage
        auto *job = new QKeychain::DeletePasswordJob(qsl("Mudlet profile"));
        job->setAutoDelete(true);
        job->setKey(profileName);
        job->setProperty("profile", profileName);
        job->start();
    }
    mProfilePasswordsToMigrate.removeAll(profileName);
    job->deleteLater();

    if (mProfilePasswordsToMigrate.isEmpty()) {
        emit signal_passwordsMigratedToProfiles();
    }
}

void mudlet::setShowMapAuditErrors(const bool state)
{
    if (mShowMapAuditErrors != state) {
        mShowMapAuditErrors = state;

        emit signal_showMapAuditErrorsChanged(state);
    }
}

void mudlet::setShowIconsOnMenu(const Qt::CheckState state)
{
    if (mShowIconsOnMenuCheckedState != state) {
        mShowIconsOnMenuCheckedState = state;

        switch (state) {
        case Qt::Unchecked:
            qApp->setAttribute(Qt::AA_DontShowIconsInMenus, true);
            break;
        case Qt::Checked:
            qApp->setAttribute(Qt::AA_DontShowIconsInMenus, false);
            break;
        case Qt::PartiallyChecked:
            qApp->setAttribute(Qt::AA_DontShowIconsInMenus, !mShowIconsOnMenuOriginally);
        }

        emit signal_showIconsOnMenusChanged(state);
    }
}

void mudlet::setAppearance(const Appearance state, const bool& loading)
{
    if (state == mAppearance && !loading) {
        return;
    }

    mDarkMode = false;
    if (state == Appearance::dark || (state == Appearance::systemSetting && desktopInDarkMode())) {
        mDarkMode = true;
    }

    if (mDarkMode) {
        // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
        qApp->setStyle(new DarkTheme);
        getHostManager().changeAllHostColour(getActiveHost());
    } else {
        // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
        qApp->setStyle(new AltFocusMenuBarDisable(mDefaultStyle));
        getHostManager().changeAllHostColour(getActiveHost());
    }
    mAppearance = state;
    emit signal_appearanceChanged(state);
}

void mudlet::setInterfaceLanguage(const QString& languageCode)
{
    if (mInterfaceLanguage != languageCode) {
        mInterfaceLanguage = languageCode;
        mUserLocale = QLocale(mInterfaceLanguage);
        if (mUserLocale == QLocale::c()) {
            qWarning().nospace().noquote() << "mudlet::setInterfaceLanguage(\"" << languageCode
                                           << "\") WARNING - Unable to convert given language code to a recognised locale, reverting to the POSIX 'C' one.";
        } else {
            qDebug().nospace().noquote() << "mudlet::setInterfaceLanguage(\"" << languageCode
                                         << "\") INFO - switching to \"" << QLocale::languageToString(mUserLocale.language()) << " (" << QLocale::countryToString(mUserLocale.country()) << ")\" locale.";
        }
        loadTranslators(languageCode);
        // For full dynamic language change support (no restart necessary) we
        // would also need a call here to do the same in this class that the
        // signal_guiLanguageChanged call will do in classes wired up to it
        // {run retranslateUi(), and regenerate (persistent) texts generated
        // within the Mudlet application code}...
        emit signal_guiLanguageChanged(languageCode);
    }
}

// return the user's desktop language if we have a quality translation for it
// or a back-up language they've specified
QString mudlet::autodetectPreferredLanguage()
{
    // en_GB is a temporary special exception due to its likeness to en_US, while its
    // translation is still only at 20%
    QVector<QString> availableQualityTranslations {qsl("en_GB")};
    for (auto& code : getAvailableTranslationCodes()) {
        auto& translation = mTranslationsMap.value(code);
        if (translation.fromResourceFile()) {
            auto& translatedPc = translation.getTranslatedPercentage();
            if (translatedPc >= scmTranslationGoldStar) {
                availableQualityTranslations.append(code);
            }
        }
    }

    QStringList systemUiLanguages = QLocale::system().uiLanguages();
    // The code in the loop may modify the value anyway so explicity detach
    // from the original:
    systemUiLanguages.detach();
    for (auto& language : systemUiLanguages) {
        if (availableQualityTranslations.contains(language.replace(qsl("-"), qsl("_")))) {
            return language;
        }
    }

    return qsl("en_US");
}

// Returns false on significant failure (where the caller will have to bail out)
bool mudlet::scanDictionaryFile(QFile& dict, int& oldWC, QHash<QString, unsigned int>&gc, QStringList& wl)
{
    if (!dict.exists()) {
        return true;
    }

    // First update the line count in the list of words
    if (!dict.open(QFile::ReadOnly|QFile::Text)) {
        qWarning().nospace().noquote() << "mudlet::scanDictionaryFile(...) ERROR - failed to open dictionary file (for reading): \"" << dict.fileName() << "\" reason: " << dict.errorString();
        return false;
    }

    QTextStream ds(&dict);
    ds.setCodec(QTextCodec::codecForName("UTF-8"));
    QString dictionaryLine;
    ds.readLineInto(&dictionaryLine);

    bool isOk = false;
    oldWC = dictionaryLine.toInt(&isOk);
    do {
        ds.readLineInto(&dictionaryLine);
        if (!dictionaryLine.isEmpty()) {
            // qDebug().nospace().noquote() << "    " << dictionaryLine;
            wl << dictionaryLine;
            QTextBoundaryFinder graphemeFinder(QTextBoundaryFinder::Grapheme, dictionaryLine);
            // The finder will be at the start of the string
            int startPos = 0;
            int endPos = graphemeFinder.toNextBoundary();
            do {
                if (endPos > 0) {
                    QString grapheme(dictionaryLine.mid(startPos, endPos - startPos));
                    if (gc.contains(grapheme)) {
                        ++gc[grapheme];
                    } else {
                        gc[grapheme] = 1;
                    }
                    startPos = endPos;
                    endPos = graphemeFinder.toNextBoundary();
                }
            } while (endPos > 0);
        }
    } while (!ds.atEnd() && ds.status() == QTextStream::Ok);

    if (ds.status() != QTextStream::Ok) {
        qWarning().nospace().noquote() << "mudlet::scanDictionaryFile(\"" << dict.fileName() << "\") ERROR - failed to completely read dictionary file, status: " << ds.status();
        return false;
    }

    dict.close();

    qDebug().nospace().noquote() << "Loaded custom dictionary \"" << dict.fileName() << "\" with " << wl.count() << " words.";
    if (oldWC != wl.count()) {
        qDebug().nospace().noquote() << "Previously, there were " << oldWC << " words recorded instead.";
    }
    if (wl.count() > 1) {
        // This will use the system default locale - it might be better to use
        // the Mudlet one...
        QCollator sorter;
        sorter.setCaseSensitivity(Qt::CaseSensitive);
        std::sort(wl.begin(), wl.end(), sorter);
        int dupCount = wl.removeDuplicates();
        if (dupCount) {
            qDebug().nospace().noquote() << "  Removed " << dupCount << " duplicates.";
        }
    }

    return true;
}

// Returns false on significant failure (where the caller will have to bail out)
bool mudlet::overwriteDictionaryFile(QFile& dict, const QStringList& wl)
{
    // (Re)Open the file to write out the cleaned/new contents
    // QFile::WriteOnly automatically implies QFile::Truncate in the absence of
    // certain other flags:
    if (!dict.open(QFile::WriteOnly|QFile::Text)) {
        qWarning().nospace().noquote() << "mudlet::overwriteDictionaryFile(...) ERROR - failed to open dictionary file (for writing): \"" << dict.fileName() << "\" reason: " << dict.errorString();
        return false;
    }

    QTextStream ds(&dict);
    ds.setCodec(QTextCodec::codecForName("UTF-8"));
    ds << qMax(0, wl.count());
    if (!wl.isEmpty()) {
      ds << QChar(QChar::LineFeed);
      ds << wl.join(QChar::LineFeed).toUtf8();
    }
    ds.flush();
    if (dict.error() != QFile::NoError) {
        qWarning().nospace().noquote() << "mudlet::overwriteDictionaryFile(...) ERROR - failed to completely write dictionary file: \"" << dict.fileName() << "\" status: " << dict.errorString();
        return false;
    }

    return true;
}

// Returns -1 on significant failure (where the caller will have to bail out)
int mudlet::getDictionaryWordCount(QFile &dict)
{
    if (!dict.open(QFile::ReadOnly|QFile::Text)) {
        qWarning().nospace().noquote() << "mudlet::saveDictionary(...) ERROR - failed to open dictionary file (for reading): \"" << dict.fileName() << "\" reason: " << dict.errorString();
        return -1;
    }

    QTextStream ds(&dict);
    ds.setCodec(QTextCodec::codecForName("UTF-8"));
    QString dictionaryLine;
    // Read the header line containing the word count:
    ds.readLineInto(&dictionaryLine);
    bool isOk = false;
    int oldWordCount = dictionaryLine.toInt(&isOk);
    dict.close();
    if (isOk) {
        return oldWordCount;
    }

    return -1;
}

// Returns false on significant failure (where the caller will have to bail out)
bool mudlet::overwriteAffixFile(QFile& aff, QHash<QString, unsigned int>& gc)
{
    QMultiMap<unsigned int, QString> sortedGraphemeCounts;
    // Sort the graphemes into a descending order list:
    if (!gc.isEmpty()) {
        QHashIterator<QString, unsigned int> itGraphemeCount(gc);
        while (itGraphemeCount.hasNext()) {
            itGraphemeCount.next();
            sortedGraphemeCounts.insert(itGraphemeCount.value(), itGraphemeCount.key());
        }
    }

    // Generate TRY line:
    QString tryLine = qsl("TRY ");
    QMapIterator<unsigned int, QString> itGrapheme(sortedGraphemeCounts);
    itGrapheme.toBack();
    while (itGrapheme.hasPrevious()) {
        itGrapheme.previous();
        tryLine.append(itGrapheme.value());
    }

    QStringList affixLines;
    affixLines << qsl("SET UTF-8");
    affixLines << tryLine;

    // Finally, having got the needed content, write it out:
    if (!aff.open(QFile::WriteOnly|QFile::Text)) {
        qWarning().nospace().noquote() << "mudlet::overwriteAffixFile(...) ERROR - failed to open affix file (for writing): \"" << aff.fileName() << "\" reason: " << aff.errorString();
        return false;
    }

    QTextStream as(&aff);
    as.setCodec(QTextCodec::codecForName("UTF-8"));
    as << affixLines.join(QChar::LineFeed).toUtf8();
    as << QChar(QChar::LineFeed);
    as.flush();
    if (aff.error() != QFile::NoError) {
        qWarning().nospace().noquote() << "mudlet::overwriteAffixFile(...) ERROR - failed to completely write affix file: \"" << aff.fileName() << "\" status: " << aff.errorString();
        return false;
    }

    aff.close();
    return true;
}

// Returns the count of words in the first argument:
int mudlet::scanWordList(QStringList& wl, QHash<QString, unsigned int>& gc)
{
    int wordCount = wl.count();
    if (wordCount > 1) {
        // This will use the system default locale - it might be better to use
        // the Mudlet one...
        QCollator sorter;
        sorter.setCaseSensitivity(Qt::CaseSensitive);
        std::sort(wl.begin(), wl.end(), sorter);
    }

    for (int index = 0; index < wordCount; ++index) {
        // qDebug().nospace().noquote() << "    " << wordList.at(index);
        QTextBoundaryFinder graphemeFinder(QTextBoundaryFinder::Grapheme, wl.at(index));
        // The finder will be at the start of the string
        int startPos = 0;
        int endPos = graphemeFinder.toNextBoundary();
        do {
            if (endPos > 0) {
                QString grapheme(wl.at(index).mid(startPos, endPos - startPos));
                if (gc.contains(grapheme)) {
                    ++gc[grapheme];
                } else {
                    gc[grapheme] = 1;
                }
                startPos = endPos;
                endPos = graphemeFinder.toNextBoundary();
            }
        } while (endPos > 0);
    }

    return wordCount;
}

// This will load up the spelling dictionary for the profile - and handles the
// absence of files for the first run in a new profile or from an older
// Mudlet version - it processes any changes made by the user in the ".dic" file
// and regenerates (deduplicates and sorts) it and rebuilds (the "TRY" line in)
// the ".aff" file:
Hunhandle* mudlet::prepareProfileDictionary(const QString& hostName, QSet<QString>& wordSet)
{
    // Need to check that the files exist first:
    QString dictionaryPathFileName(getMudletPath(mudlet::profileDataItemPath, hostName, qsl("profile.dic")));
    QString affixPathFileName(getMudletPath(mudlet::profileDataItemPath, hostName, qsl("profile.aff")));
    QFile dictionary(dictionaryPathFileName);
    QFile affix(affixPathFileName);
    int oldWordCount = 0;
    QStringList wordList;
    QHash<QString, unsigned int> graphemeCounts;

    if (!scanDictionaryFile(dictionary, oldWordCount, graphemeCounts, wordList)) {
        return nullptr;
    }

    if (!overwriteDictionaryFile(dictionary, wordList)) {
        return nullptr;
    }

    // We have read, sorted (and deduplicated if it was) the wordlist
    int wordCount = wordList.count();
    if (wordCount > oldWordCount) {
        qDebug().nospace().noquote() << "  Considered an extra " << wordCount - oldWordCount << " words.";
    } else if (wordCount < oldWordCount) {
        qDebug().nospace().noquote() << "  Considered " << oldWordCount - wordCount << " fewer words.";
    } else {
        qDebug().nospace().noquote() << "  No change in the number of words in dictionary.";
    }

    if (!overwriteAffixFile(affix, graphemeCounts)) {
        return nullptr;
    }

    // The pair of files are now usable by hunspell library and being use to make
    // suggestions - they are also capable of being munched - but since we are
    // using this on our own profiles' dictionaries we will not know the
    // language that the Mud uses and thus which locale's affixes are suitable.

    // Also, given how we are using the dictionary, any affix rules are going
    // to confuse our add/remove code.  We just need the SET line to force the
    // Hunspell API to be UTF-8 and the TRY line to allow for searching for
    // completions. Anyhow we now need to keep the copy of the word list ourself
    // to allow for persistent editing of it as it is not possible to obtain it
    // from the Hunspell library:

    wordSet = QSet<QString>(wordList.begin(), wordList.end());

#if defined(Q_OS_WIN32)
    mudlet::self()->sanitizeUtf8Path(affixPathFileName, qsl("profile.dic"));
    mudlet::self()->sanitizeUtf8Path(dictionaryPathFileName, qsl("profile.aff"));
#endif
    return Hunspell_create(affixPathFileName.toUtf8().constData(), dictionaryPathFileName.toUtf8().constData());
}

// This will load up the shared spelling dictionary for profiles that want it
// - and handles the absence of files for the first run from an older Mudlet
// version - it processes any changes made by the user in the ".dic" file and
// regenerates (deduplicates and sorts) it and (rebuilds the "TRY" line) in
// the ".aff" file:
Hunhandle* mudlet::prepareSharedDictionary()
{
    if (mpHunspell_sharedDictionary) {
        return mpHunspell_sharedDictionary;
    }

    // Need to check that the files exist first:
    QString dictionaryPathFileName(getMudletPath(mudlet::mainDataItemPath, qsl("mudlet.dic")));
    QString affixPathFileName(getMudletPath(mudlet::mainDataItemPath, qsl("mudlet.aff")));
    QFile dictionary(dictionaryPathFileName);
    QFile affix(affixPathFileName);
    int oldWordCount = 0;
    QStringList wordList;
    QHash<QString, unsigned int> graphemeCounts;

    if (!scanDictionaryFile(dictionary, oldWordCount, graphemeCounts, wordList)) {
        return nullptr;
    }

    if (!overwriteDictionaryFile(dictionary, wordList)) {
        return nullptr;
    }

    // We have read, sorted (and deduplicated if it was) the wordlist
    int wordCount = wordList.count();
    if (wordCount > oldWordCount) {
        qDebug().nospace().noquote() << "  Considered an extra " << wordCount - oldWordCount << " words.";
    } else if (wordCount < oldWordCount) {
        qDebug().nospace().noquote() << "  Considered " << oldWordCount - wordCount << " fewer words.";
    } else {
        qDebug().nospace().noquote() << "  No change in the number of words in dictionary.";
    }

    if (!overwriteAffixFile(affix, graphemeCounts)) {
        return nullptr;
    }

    mWordSet_shared = QSet<QString>(wordList.begin(), wordList.end());

#if defined(Q_OS_WIN32)
    mudlet::self()->sanitizeUtf8Path(affixPathFileName, qsl("profile.dic"));
    mudlet::self()->sanitizeUtf8Path(dictionaryPathFileName, qsl("profile.aff"));
#endif
    mpHunspell_sharedDictionary = Hunspell_create(affixPathFileName.toUtf8().constData(), dictionaryPathFileName.toUtf8().constData());
    return mpHunspell_sharedDictionary;
}

// This commits any changes noted in the wordSet into the ".dic" file and
// regenerates the ".aff" file.
bool mudlet::saveDictionary(const QString& pathFileBaseName, QSet<QString>& wordSet)
{
    // First update the line count in the list of words
    QString dictionaryPathFileName(qsl("%1.dic").arg(pathFileBaseName));
    QString affixPathFileName(qsl("%1.aff").arg(pathFileBaseName));
    QFile dictionary(dictionaryPathFileName);
    QFile affix(affixPathFileName);
    QHash<QString, unsigned int> graphemeCounts;

    // The file will have previously been created - for it to be missing now is
    // not expected - thought it shouldn't really be fatal...
    int oldWordCount = getDictionaryWordCount(dictionary);
    if (oldWordCount == -1) {
        return false;
    }

    QStringList wordList{wordSet.begin(), wordSet.end()};

    // This also sorts wordList as a wanted side-effect:
    int wordCount = scanWordList(wordList, graphemeCounts);
    // We have sorted and scanned the wordlist
    if (wordCount > oldWordCount) {
        qDebug().nospace().noquote() << "  Saved an extra " << wordCount - oldWordCount << " words in dictionary.";
    } else if (wordCount < oldWordCount) {
        qDebug().nospace().noquote() << "  Saved " << oldWordCount - wordCount  << " fewer words in dictionary.";
    } else {
        qDebug().nospace().noquote() << "  No change in the number of words saved in dictionary.";
    }

    if (!overwriteDictionaryFile(dictionary, wordList)) {
        return false;
    }

    if (!overwriteAffixFile(affix, graphemeCounts)) {
        return false;
    }

    return true;
}

QPair<bool, bool> mudlet::addWordToSet(const QString& word)
{
    bool isAdded = false;
    Hunspell_add(mpHunspell_sharedDictionary, word.toUtf8().constData());
    if (!mWordSet_shared.contains(word)) {
        mWordSet_shared.insert(word);
        qDebug().noquote().nospace() << "mudlet::addWordToSet(\"" << word << "\") INFO - word added to shared mWordSet.";
        isAdded = true;
    }
    return qMakePair(true, isAdded);
}

QPair<bool, bool> mudlet::removeWordFromSet(const QString& word)
{
    bool isRemoved = false;
    Hunspell_remove(mpHunspell_sharedDictionary, word.toUtf8().constData());
    if (mWordSet_shared.remove(word)) {
        qDebug().noquote().nospace() << "mudlet::removeWordFromSet(\"" << word << "\") INFO - word removed from shared mWordSet.";
        isRemoved = true;
    }
    return qMakePair(true, isRemoved);
}

QSet<QString> mudlet::getWordSet()
{
    QSet<QString> wordSet;
    // Got read lock within the timeout:
    wordSet = mWordSet_shared;
    // Ensure we make a deep copy of it so the caller is not affected by
    // other profiles' edits.
    wordSet.detach();
    // Now we can unlock it:
    return wordSet;
}

std::pair<bool, QString> mudlet::setProfileIcon(const QString& profile, const QString& newIconPath)
{
    QDir dir;
    auto profileIconPath = mudlet::getMudletPath(mudlet::profileDataItemPath, profile, qsl("profileicon"));
    if (QFileInfo::exists(profileIconPath) && !dir.remove(profileIconPath)) {
        qWarning() << "mudlet::setProfileIcon() ERROR: couldn't remove existing icon" << profileIconPath;
        return {false, qsl("couldn't remove existing icon file")};
    }

    if (!QFile::copy(newIconPath, profileIconPath)) {
        qWarning() << "mudlet::setProfileIcon() ERROR: couldn't copy new icon" << newIconPath<< " to" << profileIconPath;
        return {false, qsl("couldn't copy icon file into new location")};
    }

    return {true, QString()};
}

std::pair<bool, QString> mudlet::resetProfileIcon(const QString& profile)
{
    QDir dir;
    auto profileIconPath = mudlet::getMudletPath(mudlet::profileDataItemPath, profile, qsl("profileicon"));
    if (QFileInfo::exists(profileIconPath) && !dir.remove(profileIconPath)) {
        qWarning() << "mudlet::resetProfileIcon() ERROR: couldn't remove existing icon" << profileIconPath;
        return {false, qsl("couldn't remove existing icon file")};
    }

    return {true, QString()};
}

#if defined(Q_OS_WIN32)
// credit to Qt Creator (https://github.com/qt-creator/qt-creator/blob/50d93a656789d6e776ecca4adc2e5b487bac0dbc/src/libs/utils/fileutils.cpp)
static QString getShortPathName(const QString& name)
{
    if (name.isEmpty()) {
        return name;
    }

    // Determine length, then convert.
    const LPCTSTR nameC = reinterpret_cast<LPCTSTR>(name.utf16()); // MinGW
    const DWORD length = GetShortPathNameW(nameC, NULL, 0);
    if (length == 0) {
        return name;
    }
    QScopedArrayPointer<TCHAR> buffer(new TCHAR[length]);
    GetShortPathNameW(nameC, buffer.data(), length);
    const QString rc = QString::fromUtf16(reinterpret_cast<const ushort*>(buffer.data()), length - 1);
    return rc;
}

// 'strip' non-ASCII characters from the path by copying it to a location without them
// this is only an issue for the Win32 API; macOS and Linux don't have such issues
void mudlet::sanitizeUtf8Path(QString& originalLocation, const QString& fileName) const
{
    static auto findNonAscii = QRegularExpression(qsl("([^ -~])"));

    auto nonAscii = findNonAscii.match(originalLocation);
    if (!nonAscii.hasMatch()) {
        return;
    }

    const auto shortPath = getShortPathName(originalLocation);
    // short path name might not always work: https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getshortpathnamew#remarks
    if (shortPath != originalLocation) {
        originalLocation = shortPath;
        return;
    }

    const QString pureANSIpath = qsl("C:\\Windows\\Temp\\mudlet_%1").arg(fileName);
    if (!QFileInfo::exists(pureANSIpath)) {
        if (!QFile::copy(originalLocation, pureANSIpath)) {
            qWarning() << "mudlet::sanitizeUtf8Path() ERROR: couldn't copy" << originalLocation << "to location without ASCII characters";
        } else {
            originalLocation = pureANSIpath;
        }
    }
}
#endif

// Enable redirects and HTTPS support for a given url
void mudlet::setNetworkRequestDefaults(const QUrl& url, QNetworkRequest& request)
{
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    request.setRawHeader(QByteArray("User-Agent"), QByteArray(qsl("Mozilla/5.0 (Mudlet/%1%2)").arg(APP_VERSION, APP_BUILD).toUtf8().constData()));
#if !defined(QT_NO_SSL)
    if (url.scheme() == qsl("https")) {
        QSslConfiguration config(QSslConfiguration::defaultConfiguration());
        request.setSslConfiguration(config);
    }
#endif
}

void mudlet::activateProfile(Host* pHost)
{
    QMap<QString, int> hostNameToTabMap;
    for (int i = 0, total = mpTabBar->count(); i < total; ++i) {
        hostNameToTabMap.insert(mpTabBar->tabData(i).toString(), i);
    }
    QString oldActiveHostName;
    if (mpCurrentActiveHost) {
        oldActiveHostName = mpCurrentActiveHost->getName();
    }

    if (!pHost || !pHost->mpConsole) {
        // Ah, we do not seem to have a profile anymore:
        mpCurrentActiveHost = nullptr;
        // Nothing else to do if the host to activate doesn't exist
        return;
    }

    if (mpCurrentActiveHost && mpCurrentActiveHost == pHost) {
        // Nothing to do if the caller was the current foreground host, but on
        // the initial start up mpCurrentActiveHost WILL be a nullptr
        return;
    }

    const QString newActiveHostName{pHost->getName()};
    int newActiveTabIndex = hostNameToTabMap.value(newActiveHostName, -1);

    if (mpCurrentActiveHost && mpCurrentActiveHost->mpConsole) {
        // Tell the old profile that it is losing focus:
        TEvent focusLostEvent {};
        focusLostEvent.mArgumentList << QLatin1String("sysProfileFocusChangeEvent");
        // Boolean arguments are carried as "0" for false or "1" for true,
        // This is for the profile that is losing focus:
        focusLostEvent.mArgumentList << QLatin1String("0");
        focusLostEvent.mArgumentTypeList << ARGUMENT_TYPE_STRING << ARGUMENT_TYPE_BOOLEAN;
        mpCurrentActiveHost->raiseEvent(focusLostEvent);

        if (!mMultiView) {
            // We only have to hide the current profile under the tab if NOT
            // in multi-view mode:
            mpCurrentActiveHost->mpConsole->hide();
        }
    }

    // Need to change to the right tab without triggering slot_tabChanged(int)
    // as that might even be the caller of this method!
    if (newActiveTabIndex >= 0) {
        mpTabBar->blockSignals(true);
        mpTabBar->setCurrentIndex(newActiveTabIndex);
        mpTabBar->blockSignals(false);
    }

    // Reset the tab back to "normal" to undo the effect of it having its style
    // changed on new data:
    mpTabBar->setTabBold(newActiveTabIndex, false);
    mpTabBar->setTabItalic(newActiveTabIndex, false);
    mpTabBar->setTabUnderline(newActiveTabIndex, false);

    mpCurrentActiveHost = pHost;
    mpCurrentActiveHost->mpConsole->show();
    mpCurrentActiveHost->mpConsole->repaint();
    mpCurrentActiveHost->mpConsole->refresh();
    mpCurrentActiveHost->mpConsole->mpCommandLine->repaint();

    // Regenerate the multi-view mode if it is enabled:
    reshowRequiredMainConsoles();

    // Reset the styles to reflect those of the now active profile:
    mpMainToolBar->setStyleSheet(mpCurrentActiveHost->mProfileStyleSheet);
    mpTabBar->setStyleSheet(mpCurrentActiveHost->mProfileStyleSheet);
    menuBar()->setStyleSheet(mpCurrentActiveHost->mProfileStyleSheet);

    // Tell the new profile that it is gaining focus via a Mudlet event:
    TEvent focusGainedEvent {};
    focusGainedEvent.mArgumentList << QLatin1String("sysProfileFocusChangeEvent");
    focusGainedEvent.mArgumentList << QLatin1String("1");
    focusGainedEvent.mArgumentTypeList << ARGUMENT_TYPE_STRING << ARGUMENT_TYPE_BOOLEAN;
    mpCurrentActiveHost->raiseEvent(focusGainedEvent);

    // Tell the new profile's main window that it might be resize via a Qt event:
    int x = mpCurrentActiveHost->mpConsole->width();
    int y = mpCurrentActiveHost->mpConsole->height();
    QSize s = QSize(x, y);
    QResizeEvent event(s, s);
    QApplication::sendEvent(mpCurrentActiveHost->mpConsole, &event);

    // update the main application window title for the currently selected profile
    // Potential to be translated in the future if the need arises, with the following disambiguation:
    // "Title for the main window when a profile is loaded or active, %1 is the name "
    // "of the profile and %2 is the Mudlet version string."
    setWindowTitle(qsl("%1 - %2")
                           .arg(mpCurrentActiveHost->getName(), scmVersion));

    dactionInputLine->setChecked(mpCurrentActiveHost->getCompactInputLine());

    updateDiscordNamedIcon();

    updateMultiViewControls();

    mpCurrentActiveHost->updateDisplayDimensions();

    // Currently used to update the Discord Rich Presence
    emit signal_tabChanged(mpCurrentActiveHost->getName());
    // Currently used to assign the shortcuts for the activated profile:-
    emit signal_profileActivated(pHost, newActiveTabIndex);

    mpCurrentActiveHost->setFocusOnHostActiveCommandLine();
}

void mudlet::setGlobalStyleSheet(const QString& styleSheet)
{
    mpMainToolBar->setStyleSheet(styleSheet);
    mpTabBar->setStyleSheet(styleSheet);
    menuBar()->setStyleSheet(styleSheet);
}

void mudlet::setupTrayIcon()
{
    mTrayIcon.setIcon(windowIcon());
    auto menu = new QMenu(this);
    auto hideTrayAction = new QAction(tr("Hide tray icon"), this);
    connect(hideTrayAction, &QAction::triggered, this, [=]() {
       mTrayIcon.hide();
    });
    menu->addAction(hideTrayAction);
    auto exitAction = new QAction(tr("Quit Mudlet"), this);
    connect(exitAction, &QAction::triggered, this, &mudlet::close);
    menu->addAction(exitAction);
    mTrayIcon.setContextMenu(menu);
}

void mudlet::slot_tabMoved(const int oldPos, const int newPos)
{
    const QStringList& tabNamesInOrder = mpTabBar->tabNames();
    int itemsCount = mpSplitter_profileContainer->count();
    Q_ASSERT_X(itemsCount == tabNamesInOrder.count(), "mudlet::slot_tabMoved(...)", "mismatch in count of tabs and TMainConsoles");
    QMap<QString, QWidget*> widgetMap;
    // Gather the QWidget pointers for each TMainConsole and store them
    // against their profile name:
    for (int profileIndex = 0; profileIndex < itemsCount; ++profileIndex) {
        auto pWidget = mpSplitter_profileContainer->widget(profileIndex);
        if (pWidget) {
            auto name = pWidget->property("HostName").toString();
            widgetMap.insert(name, pWidget);
        } else {
            qWarning().nospace().noquote() << "mudlet::slot_tabMoved(" << oldPos<< ", " << newPos << ") WARNING - nullptr for pointer to TMainConsole at 'profileIndex': " << profileIndex << ".";
        }
    }
    // Now go through all the names, pull the associated TMainConsoles from the
    // splitter and then re-add each of them at the end in turn - once we have
    // gone through them all it will mean that they are in the same order as the
    // tabs:
    for (int index = 0; index < itemsCount; ++index) {
        const auto& wantedTabName = tabNamesInOrder.at(index);
        mpSplitter_profileContainer->addWidget(widgetMap.value(wantedTabName));
    }
}

void mudlet::refreshTabBar()
{
    for (const auto& pHost : mHostManager) {
        QString hostName = pHost->getName();
        if (smDebugMode) {
            mpTabBar->applyPrefixToDisplayedText(hostName, TDebug::getTag(pHost.data()));
        } else {
            mpTabBar->applyPrefixToDisplayedText(hostName);
        }
    }
}

//NOLINT(readability-convert-member-functions-to-static)
// doesn't make sense to make it static since it modifies a class variable
void mudlet::setupPreInstallPackages(const QString& gameUrl)
{
    const QHash<QString, QStringList> defaultScripts = {
        // clang-format off
        // scripts to pre-install for a profile      games this applies to, * means all games
        {qsl(":/run-lua-code-v4.xml"),    {qsl("*")}},
        {qsl(":/echo.xml"),               {qsl("*")}},
        {qsl(":/deleteOldProfiles.xml"),  {qsl("*")}},
        {qsl(":/mudlet-lua/lua/enable-accessibility/enable-accessibility.xml"), {qsl("*")}},
        {qsl(":/CF-loader.xml"),          {qsl("carrionfields.net")}},
        {qsl(":/mg-loader.xml"),          {qsl("mg.mud.de")}},
        {qsl(":/run-tests.xml"),          {qsl("mudlet.org")}},
        {qsl(":/mudlet-lua/lua/stressinator/StressinatorDisplayBench.xml"), {qsl("mudlet.org")}},
        {qsl(":/mudlet-mapper.xml"),      {qsl("aetolia.com"),
                                                      qsl("achaea.com"),
                                                      qsl("lusternia.com"),
                                                      qsl("imperian.com"),
                                                      qsl("starmourn.com"),
                                                      qsl("stickmud.com")}},
        // clang-format on
    };

    QHashIterator<QString, QStringList> i(defaultScripts);
    while (i.hasNext()) {
        i.next();
        if (i.value().first() == QLatin1String("*") || i.value().contains(gameUrl)) {
            mudlet::self()->mPackagesToInstallList.append(i.key());
        }
    }

    if (!mudlet::self()->mPackagesToInstallList.contains(qsl(":/mudlet-mapper.xml"))) {
        mudlet::self()->mPackagesToInstallList.append(qsl(":/mudlet-lua/lua/generic-mapper/generic_mapper.xml"));
    }
}

// Referenced from github.com/keepassxreboot/keepassxc. Licensed under GPL2/3.
// Copyright (C) 2020 KeePassXC Team <team@keepassxc.org>
bool mudlet::desktopInDarkMode()
{
#if defined(Q_OS_WIN32)
    QSettings settings(R"(HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Themes\Personalize)", QSettings::NativeFormat);
    return settings.value("AppsUseLightTheme", 1).toInt() == 0;
#elif defined(Q_OS_MAC)
    bool isDark = false;
    CFStringRef uiStyleKey = CFSTR("AppleInterfaceStyle");
    CFStringRef uiStyle = nullptr;
    CFStringRef darkUiStyle = CFSTR("Dark");
    if (uiStyle = (CFStringRef) coreMacOS::CFPreferencesCopyAppValue(uiStyleKey, coreMacOS::kCFPreferencesCurrentApplication); uiStyle)
    {
        isDark = (coreMacOS::kCFCompareEqualTo == coreMacOS::CFStringCompare(uiStyle, darkUiStyle, 0));
        coreMacOS::CFRelease(uiStyle);
    }
    return isDark;
#elif defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD)
    QProcess process;
    process.start(qsl("gsettings"), QStringList() << qsl("get") << qsl("org.gnome.desktop.interface") << qsl("gtk-theme"));
    process.waitForFinished();
    QString output = QString::fromUtf8(process.readAllStandardOutput());
    return output.contains(qsl("-dark"), Qt::CaseInsensitive);
#endif

    return false;
}

void mudlet::announce(const QString& text, const QString& processing)
{
    mpAnnouncer->announce(text, processing);
}

void mudlet::onlyShowProfiles(const QStringList& predefinedProfiles)
{
    // As we are processing argument(s) passed on the command line check in a
    // non-case sensitive manner:
    for (const QString& predefinedProfile : predefinedProfiles) {
        if (TGameDetails::keys().contains(predefinedProfile, Qt::CaseInsensitive)) {
            auto details = TGameDetails::findGame(predefinedProfile, Qt::CaseInsensitive);
            Q_ASSERT_X(details != TGameDetails::scmDefaultGames.constEnd(), "mudlet::onlyShowProfiles(const QStringList&)", "failed to find matching game after initial search said there was a match");
            mOnlyShownPredefinedProfiles.append((*details).name);
        }
        // Don't do anything if it was NOT found
    }
}

/*static*/ QImage mudlet::getSplashScreen()
{
#if defined(INCLUDE_VARIABLE_SPLASH_SCREEN)
    auto now = QDateTime::currentDateTime();
    // clang-format off
#if defined(DEBUG_EASTER_EGGS)
    if (bool layEasterEgg = (now.time().second() < 30); layEasterEgg) {
        // Only do it in the first half of any minute:
#else
    if (bool layEasterEgg = (now.date().month() == 4
                        && now.date().day() == 1); layEasterEgg) {
#endif // ! DEBUG_EASTER_EGGS
        // clang-format on
        // Set to one more than the highest number Mudlet_splashscreen_other_NN.png:
        auto egg = QRandomGenerator::global()->bounded(24);
        if (egg) {
            auto eggFileName = qsl(":/splash/Mudlet_splashscreen_other_%1.png").arg(egg, 2, 10, QLatin1Char('0'));
            return QImage(eggFileName);
        } else {
            // For the zeroth case just rotate the picture 180 degrees:
            QImage original(mudlet::scmIsReleaseVersion
                                    ? qsl(":/splash/Mudlet_splashscreen_main.png")
                                    : mudlet::scmIsPublicTestVersion ? qsl(":/splash/Mudlet_splashscreen_ptb.png")
                                                                     : qsl(":/splash/Mudlet_splashscreen_development.png"));
            return original.mirrored(true, true);
        }
    } else {
        return QImage(mudlet::scmIsReleaseVersion
                              ? qsl(":/splash/Mudlet_splashscreen_main.png")
                              : mudlet::scmIsPublicTestVersion ? qsl(":/splash/Mudlet_splashscreen_ptb.png")
                                                               : qsl(":/splash/Mudlet_splashscreen_development.png"));
    }
#else
    return QImage(qsl(":/splash/Mudlet_splashscreen_main.png"));
#endif // INCLUDE_VARIABLE_SPLASH_SCREEN
}
