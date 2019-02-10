/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2013-2019 by Stephen Lyons - slysven@virginmedia.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016 by Chris Leacy - cleacy1972@gmail.com              *
 *   Copyright (C) 2016-2018 by Ian Adkins - ieadkins@gmail.com            *
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
#include "LuaInterface.h"
#include "TCommandLine.h"
#include "TConsole.h"
#include "TDebug.h"
#include "TDockWidget.h"
#include "TEvent.h"
#include "TLabel.h"
#include "TMap.h"
#include "TRoomDB.h"
#include "TTabBar.h"
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
#include "VarUnit.h"

#include "pre_guard.h"
#include <QtUiTools/quiloader.h>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QScrollBar>
#include <QTableWidget>
#include <QToolBar>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QVariantHash>

#include <zip.h>
#include "post_guard.h"

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
const bool mudlet::scmIsDevelopmentVersion = !QByteArray(APP_BUILD).isEmpty();
QVariantHash mudlet::mLuaFunctionNames;

QPointer<mudlet> mudlet::_self = nullptr;

void mudlet::start()
{
    _self = new mudlet;
}

mudlet* mudlet::self()
{
    return _self;
}

void mudlet::loadLanguagesMap()
{
    mLanguageCodeMap = {
            {"en_US", make_pair(tr("English [American]", "Name of language. Please translate with the English description intact, like this: Nederlands (Dutch)"), 100)},
            {"en_GB", make_pair(tr("English [British]", "Name of language. Please translate with the English description intact, like this: Nederlands (Dutch)"), 0)},
            {"zh_CN", make_pair(tr("Chinese [Simplified]", "Name of language. Please translate with the English description intact, like this: Nederlands (Dutch)"), 0)},
            {"zh_TW", make_pair(tr("Chinese [Traditional]", "Name of language. Please translate with the English description intact, like this: Nederlands (Dutch)"), 0)},
            {"nl_NL", make_pair(tr("Dutch", "Name of language. Please translate with the English description intact, like this: Nederlands (Dutch)"), 0)},
            {"fr_FR", make_pair(tr("French", "Name of language. Please translate with the English description intact, like this: Nederlands (Dutch)"), 0)},
            {"de_DE", make_pair(tr("German", "Name of language. Please translate with the English description intact, like this: Nederlands (Dutch)"), 0)},
            {"el_GR", make_pair(tr("Greek", "Name of language. Please translate with the English description intact, like this: Nederlands (Dutch)"), 0)},
            {"it_IT", make_pair(tr("Italian", "Name of language. Please translate with the English description intact, like this: Nederlands (Dutch)"), 0)},
            {"pl_PL", make_pair(tr("Polish", "Name of language. Please translate with the English description intact, like this: Nederlands (Dutch)"), 0)},
            {"ru_RU", make_pair(tr("Russian", "Name of language. Please translate with the English description intact, like this: Nederlands (Dutch)"), 0)},
            {"es_ES", make_pair(tr("Spanish", "Name of language. Please translate with the English description intact, like this: Nederlands (Dutch)"), 0)},
            {"pt_PT", make_pair(tr("Portuguese", "Name of language. Please translate with the English description intact, like this: Nederlands (Dutch)"), 0)},
    };

    QFile file(QStringLiteral(":/translation-stats.json"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "translation statistics file isn't available, won't show stats in preferences";
        return;
    }

    QByteArray saveData = file.readAll();
    QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));
    QJsonObject translationStats = loadDoc.object();

    for (auto& languageKey : translationStats.keys()) {
        auto languageData = mLanguageCodeMap.value(languageKey);

        auto value = translationStats.value(languageKey).toObject().value(QStringLiteral("translatedpc"));
        if (value == QJsonValue::Undefined) {
            continue;
        }
        auto translatedpc = value.toInt();

        // show translation % for languages with less than 95%
        // for languages above 95%, show a gold star
        if (translatedpc < mTranslationStar) {
            mLanguageCodeMap.insert(
                    languageKey,
                    make_pair(tr("%1 (%2% done)", "%1 is the language name, %2 is the amount of texts in percent that is translated in Mudlet").arg(languageData.first).arg(translatedpc),
                              translatedpc));
        } else {
            mLanguageCodeMap.insert(languageKey, make_pair(languageData.first, translatedpc));
        }
    }
 }

mudlet::mudlet()
: QMainWindow()
, mFontManager()
, mToolbarIconSize(0)
, mEditorTreeWidgetIconSize(0)
, mWindowMinimized(false)
, mReplaySpeed(1)
, version(QString("Mudlet ") + QString(APP_VERSION) + QString(APP_BUILD))
, mpCurrentActiveHost(nullptr)
, mIsGoingDown(false)
, mMenuBarVisibility(visibleAlways)
, mToolbarVisibility(visibleAlways)
, mpActionReplaySpeedDown(nullptr)
, mpActionReplaySpeedUp(nullptr)
, mpActionSpeedDisplay(nullptr)
, mpActionReplayTime(nullptr)
, mpLabelReplaySpeedDisplay(nullptr)
, mpLabelReplayTime(nullptr)
, mpTimerReplay(nullptr)
, mIsLoadingLayout(false)
, mHasSavedLayout(false)
, mpToolBarReplay(nullptr)
, moduleTable(nullptr)
, mCompactInputLine(false)
, mpAboutDlg(nullptr)
, mpModuleDlg(nullptr)
, mpPackageManagerDlg(nullptr)
, mshowMapAuditErrors(false)
, mTimeFormat(tr("hh:mm:ss",
                 "Formatting string for elapsed time display in replay playback - see QDateTime::toString(const QString&) for the gory details...!"))
, mDiscord()
, mShowIconsOnDialogs(true)
, mCopyAsImageTimeout{3}
, mInterfaceLanguage(QStringLiteral("en_US"))
{
    mShowIconsOnMenuOriginally = !qApp->testAttribute(Qt::AA_DontShowIconsInMenus);
    mpSettings = getQSettings();
    readEarlySettings(*mpSettings);
    if (mShowIconsOnMenuCheckedState != Qt::PartiallyChecked) {
        // If the setting is not the "tri-state" one then force the setting,
        // have to invert the sense because the attribute is a negative one:
        qApp->setAttribute(Qt::AA_DontShowIconsInMenus, (mShowIconsOnMenuCheckedState == Qt::Unchecked));
    }
    
    qApp->setAttribute(Qt::AA_UseHighDpiPixmaps);

    loadTranslators();

    setupUi(this);
    setUnifiedTitleAndToolBarOnMac(true);
    setContentsMargins(0, 0, 0, 0);
    menuGames->setToolTipsVisible(true);
    menuEditor->setToolTipsVisible(true);
    menuOptions->setToolTipsVisible(true);
    menuHelp->setToolTipsVisible(true);
    menuAbout->setToolTipsVisible(true);

    mudlet::debugMode = false;
    setAttribute(Qt::WA_DeleteOnClose);
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setWindowTitle(version);
    setWindowIcon(QIcon(QStringLiteral(":/icons/mudlet_main_48px.png")));
    mpMainToolBar = new QToolBar(this);
    mpMainToolBar->setObjectName("mpMainToolBar");
    mpMainToolBar->setWindowTitle(tr("Main Toolbar"));
    addToolBar(mpMainToolBar);
    mpMainToolBar->setMovable(false);
    addToolBarBreak();
    auto frame = new QWidget(this);
    frame->setFocusPolicy(Qt::NoFocus);
    setCentralWidget(frame);
    mpTabBar = new TTabBar(frame);
    mpTabBar->setMaximumHeight(30);
    mpTabBar->setFocusPolicy(Qt::NoFocus);
    mpTabBar->setTabsClosable(true);
    connect(mpTabBar, &QTabBar::tabCloseRequested, this, &mudlet::slot_close_profile_requested);
    mpTabBar->setMovable(true);
    connect(mpTabBar, &QTabBar::currentChanged, this, &mudlet::slot_tab_changed);
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

    mpActionConnect = new QAction(QIcon(QStringLiteral(":/icons/preferences-web-browser-cache.png")), tr("Connect"), this);
    mpActionConnect->setToolTip(tr("Connect to a game"));
    mpMainToolBar->addAction(mpActionConnect);

    // add name to the action's widget in the toolbar, which doesn't have one by default
    // see https://stackoverflow.com/a/32460562/72944
    mpActionConnect->setObjectName(QStringLiteral("connect_action"));
    mpMainToolBar->widgetForAction(mpActionConnect)->setObjectName(mpActionConnect->objectName());

    mpActionTriggers = new QAction(QIcon(QStringLiteral(":/icons/tools-wizard.png")), tr("Triggers"), this);
    mpActionTriggers->setToolTip(tr("Show and edit triggers"));
    mpMainToolBar->addAction(mpActionTriggers);
    mpActionTriggers->setObjectName(QStringLiteral("triggers_action"));
    mpMainToolBar->widgetForAction(mpActionTriggers)->setObjectName(mpActionTriggers->objectName());

    mpActionAliases = new QAction(QIcon(QStringLiteral(":/icons/system-users.png")), tr("Aliases"), this);
    mpActionAliases->setToolTip(tr("Show and edit aliases"));
    mpMainToolBar->addAction(mpActionAliases);
    mpActionAliases->setObjectName(QStringLiteral("aliases_action"));
    mpMainToolBar->widgetForAction(mpActionAliases)->setObjectName(mpActionAliases->objectName());

    mpActionTimers = new QAction(QIcon(QStringLiteral(":/icons/chronometer.png")), tr("Timers"), this);
    mpActionTimers->setToolTip(tr("Show and edit timers"));
    mpMainToolBar->addAction(mpActionTimers);
    mpActionTimers->setObjectName(QStringLiteral("timers_action"));
    mpMainToolBar->widgetForAction(mpActionTimers)->setObjectName(mpActionTimers->objectName());

    mpActionButtons = new QAction(QIcon(QStringLiteral(":/icons/bookmarks.png")), tr("Buttons"), this);
    mpActionButtons->setToolTip(tr("Show and edit easy buttons"));
    mpMainToolBar->addAction(mpActionButtons);
    mpActionButtons->setObjectName(QStringLiteral("buttons_action"));
    mpMainToolBar->widgetForAction(mpActionButtons)->setObjectName(mpActionButtons->objectName());

    mpActionScripts = new QAction(QIcon(QStringLiteral(":/icons/document-properties.png")), tr("Scripts"), this);
    mpActionScripts->setToolTip(tr("Show and edit scripts"));
    mpMainToolBar->addAction(mpActionScripts);
    mpActionScripts->setObjectName(QStringLiteral("scripts_action"));
    mpMainToolBar->widgetForAction(mpActionScripts)->setObjectName(mpActionScripts->objectName());

    mpActionKeys = new QAction(QIcon(QStringLiteral(":/icons/preferences-desktop-keyboard.png")), tr("Keys"), this);
    mpActionKeys->setToolTip(tr("Show and edit keys"));
    mpMainToolBar->addAction(mpActionKeys);
    mpActionKeys->setObjectName(QStringLiteral("keys_action"));
    mpMainToolBar->widgetForAction(mpActionKeys)->setObjectName(mpActionKeys->objectName());

    mpActionVariables = new QAction(QIcon(QStringLiteral(":/icons/variables.png")), tr("Variables"), this);
    mpActionVariables->setToolTip(tr("Show and edit Lua variables"));
    mpMainToolBar->addAction(mpActionVariables);
    mpActionVariables->setObjectName(QStringLiteral("variables_action"));
    mpMainToolBar->widgetForAction(mpActionVariables)->setObjectName(mpActionVariables->objectName());

    mpActionIRC = new QAction(QIcon(QStringLiteral(":/icons/internet-telephony.png")), tr("IRC"), this);
    mpActionIRC->setToolTip(tr("Open the Mudlet IRC client"));
    mpMainToolBar->addAction(mpActionIRC);
    mpActionIRC->setObjectName(QStringLiteral("irc_action"));
    mpMainToolBar->widgetForAction(mpActionIRC)->setObjectName(mpActionIRC->objectName());

    mpActionMapper = new QAction(QIcon(QStringLiteral(":/icons/applications-internet.png")), tr("Map"), this);
    mpActionMapper->setToolTip(tr("Show/hide the map"));
    mpMainToolBar->addAction(mpActionMapper);
    mpActionMapper->setObjectName(QStringLiteral("map_action"));
    mpMainToolBar->widgetForAction(mpActionMapper)->setObjectName(mpActionMapper->objectName());

    mpActionHelp = new QAction(QIcon(QStringLiteral(":/icons/help-hint.png")), tr("Manual"), this);
    mpActionHelp->setToolTip(tr("Browse reference material and documentation"));
    mpMainToolBar->addAction(mpActionHelp);
    mpActionHelp->setObjectName(QStringLiteral("manual_action"));
    mpMainToolBar->widgetForAction(mpActionHelp)->setObjectName(mpActionHelp->objectName());

    mpActionOptions = new QAction(QIcon(QStringLiteral(":/icons/configure.png")), tr("Settings"), this);
    mpActionOptions->setToolTip(tr("See and edit profile preferences"));
    mpMainToolBar->addAction(mpActionOptions);
    mpActionOptions->setObjectName(QStringLiteral("settings_action"));
    mpMainToolBar->widgetForAction(mpActionOptions)->setObjectName(mpActionOptions->objectName());

    // TODO: Consider changing to ":/icons/mudlet_notepad.png" as per the icon
    // now used for the window when the visual change to the toolbar caused can
    // be managed
    mpActionNotes = new QAction(QIcon(QStringLiteral(":/icons/applications-accessories.png")), tr("Notepad"), this);
    mpActionNotes->setToolTip(tr("Open a notepad that you can store your notes in"));
    mpMainToolBar->addAction(mpActionNotes);
    mpActionNotes->setObjectName(QStringLiteral("notepad_action"));
    mpMainToolBar->widgetForAction(mpActionNotes)->setObjectName(mpActionNotes->objectName());

    mpActionPackageManager = new QAction(QIcon(QStringLiteral(":/icons/package-manager.png")), tr("Package Manager"), this);
    mpActionPackageManager->setToolTip(tr("Package Manager - allows you to install xmls, .mpackages"));
    mpMainToolBar->addAction(mpActionPackageManager);
    mpActionPackageManager->setObjectName(QStringLiteral("package_action"));
    mpMainToolBar->widgetForAction(mpActionPackageManager)->setObjectName(mpActionPackageManager->objectName());

    mpActionModuleManager = new QAction(QIcon(QStringLiteral(":/icons/module-manager.png")), tr("Module Manager"), this);
    mpActionModuleManager->setToolTip(tr("Module Manager - allows you to install xmls, .mpackages that are syncronized across multiple profile (good for scripts that you use on several profiles)"));
    mpMainToolBar->addAction(mpActionModuleManager);
    mpActionModuleManager->setObjectName(QStringLiteral("module_action"));
    mpMainToolBar->widgetForAction(mpActionModuleManager)->setObjectName(mpActionModuleManager->objectName());

    mpActionReplay = new QAction(QIcon(QStringLiteral(":/icons/media-optical.png")), tr("Replay"), this);
    mpActionReplay->setObjectName(QStringLiteral("replay_action"));
    mpMainToolBar->addAction(mpActionReplay);
    mpMainToolBar->widgetForAction(mpActionReplay)->setObjectName(mpActionReplay->objectName());

    mpActionReconnect = new QAction(QIcon(QStringLiteral(":/icons/system-restart.png")), tr("Reconnect"), this);
    mpActionReconnect->setToolTip(tr("Disconnects you from the game and connects once again"));
    mpMainToolBar->addAction(mpActionReconnect);
    mpActionReconnect->setObjectName(QStringLiteral("reconnect_action"));
    mpMainToolBar->widgetForAction(mpActionReconnect)->setObjectName(mpActionReconnect->objectName());

    mpActionMultiView = new QAction(QIcon(QStringLiteral(":/icons/view-split-left-right.png")), tr("MultiView"), this);
    mpActionMultiView->setToolTip(tr("If you've got multiple profiles open, splits Mudlet screen to show them all at once"));
    mpMainToolBar->addAction(mpActionMultiView);
    mpActionMultiView->setObjectName(QStringLiteral("multiview_action"));
    mpMainToolBar->widgetForAction(mpActionMultiView)->setObjectName(mpActionMultiView->objectName());

    mpActionAbout = new QAction(QIcon(QStringLiteral(":/icons/mudlet_information.png")), tr("About"), this);
    mpActionAbout->setToolTip(tr("<p>Inform yourself about this version of Mudlet, the people who made it and the licence under which you can share it.</p>",
                                 // Intentional comment
                                 "Tooltip for About Mudlet sub-menu item and main toolbar button (or menu item if an update has changed that control to have a popup menu instead) (Used in 3 places - please ensure all have the same translation)."));
    mpMainToolBar->addAction(mpActionAbout);
    mpActionAbout->setObjectName(QStringLiteral("about_action"));
    mpMainToolBar->widgetForAction(mpActionAbout)->setObjectName(mpActionAbout->objectName());

    disableToolbarButtons();

    mpDebugArea = new QMainWindow(nullptr);
    // PLACEMARKER: Host creation (1) - "default_host" case
    QString defaultHost(QStringLiteral("default_host"));
    // We DO NOT emit a signal_hostCreated for THIS case:
    mHostManager.addHost(defaultHost, QString(), QString(), QString());
    mpDefaultHost = mHostManager.getHost(defaultHost);
    mpDebugConsole = new TConsole(mpDefaultHost, TConsole::CentralDebugConsole);
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
    QFont mainFont = QFont(QStringLiteral("Bitstream Vera Sans Mono"), 8, QFont::Normal);
    if (mEnableFullScreenMode) {
        showFullScreen();
        QAction* actionFullScreeniew = new QAction(QIcon(QStringLiteral(":/icons/dialog-cancel.png")), tr("Toggle Full Screen View"), this);
        actionFullScreeniew->setStatusTip(tr("Toggle Full Screen View"));
        mpMainToolBar->addAction(actionFullScreeniew);
        actionFullScreeniew->setObjectName(QStringLiteral("fullscreen_action"));
        mpMainToolBar->widgetForAction(actionFullScreeniew)->setObjectName(actionFullScreeniew->objectName());
        connect(actionFullScreeniew, &QAction::triggered, this, &mudlet::toggleFullScreenView);
    }
    // This is the only place the tabBar font is set and it influences the
    // height of the tabs used - since we now want to adjust the appearance of
    // the tab if it is not the active one and new data has arrived to show in
    // the related profile - make the font size a little larger that the 6 it
    // once was so that it is a bit more obvious when it changes:
    QFont mdiFont = QFont(QStringLiteral("Bitstream Vera Sans Mono"), 8, QFont::Normal);
    setFont(mainFont);
    mainPane->setFont(mainFont);
    mpTabBar->setFont(mdiFont);

    mainPane->show();

    connect(mpActionConnect.data(), &QAction::triggered, this, &mudlet::slot_show_connection_dialog);
    connect(mpActionHelp.data(), &QAction::triggered, this, &mudlet::show_help_dialog);
    connect(mpActionTimers.data(), &QAction::triggered, this, &mudlet::show_timer_dialog);
    connect(mpActionAliases.data(), &QAction::triggered, this, &mudlet::show_alias_dialog);
    connect(mpActionScripts.data(), &QAction::triggered, this, &mudlet::show_script_dialog);
    connect(mpActionKeys.data(), &QAction::triggered, this, &mudlet::show_key_dialog);
    connect(mpActionVariables.data(), &QAction::triggered, this, &mudlet::show_variable_dialog);
    connect(mpActionButtons.data(), &QAction::triggered, this, &mudlet::show_action_dialog);
    connect(mpActionOptions.data(), &QAction::triggered, this, &mudlet::slot_show_options_dialog);
    connect(mpActionAbout.data(), &QAction::triggered, this, &mudlet::slot_show_about_dialog);
    connect(mpActionMultiView.data(), &QAction::triggered, this, &mudlet::slot_multi_view);
    connect(mpActionReconnect.data(), &QAction::triggered, this, &mudlet::slot_reconnect);
    connect(mpActionReplay.data(), &QAction::triggered, this, &mudlet::slot_replay);
    connect(mpActionNotes.data(), &QAction::triggered, this, &mudlet::slot_notes);
    connect(mpActionMapper.data(), &QAction::triggered, this, &mudlet::slot_mapper);
    connect(mpActionIRC.data(), &QAction::triggered, this, &mudlet::slot_irc);
    connect(mpActionPackageManager.data(), &QAction::triggered, this, &mudlet::slot_package_manager);
    connect(mpActionModuleManager.data(), &QAction::triggered, this, &mudlet::slot_module_manager);

    // PLACEMARKER: Save for later restoration (1 of 2) (by adding a "Close" (profile) option to first menu on menu bar:
    // QAction* mactionCloseProfile = new QAction(tr("Close"), this);

    connect(dactionConnect, &QAction::triggered, this, &mudlet::slot_show_connection_dialog);
    connect(dactionReconnect, &QAction::triggered, this, &mudlet::slot_reconnect);
    connect(dactionDisconnect, &QAction::triggered, this, &mudlet::slot_disconnect);
    connect(dactionNotepad, &QAction::triggered, this, &mudlet::slot_notes);
    connect(dactionReplay, &QAction::triggered, this, &mudlet::slot_replay);

    connect(dactionHelp, &QAction::triggered, this, &mudlet::show_help_dialog);
    connect(dactionVideo, &QAction::triggered, this, &mudlet::slot_show_help_dialog_video);
    connect(dactionForum, &QAction::triggered, this, &mudlet::slot_show_help_dialog_forum);
    connect(dactionIRC, &QAction::triggered, this, &mudlet::slot_irc);
    connect(dactionLiveHelpChat, &QAction::triggered, this, &mudlet::slot_irc);
#if !defined(INCLUDE_UPDATER)
    // Hide the update menu item if the code is not included
    dactionUpdate->setVisible(false);
#else
    // Also, only show it if this is a release version
    dactionUpdate->setVisible(!scmIsDevelopmentVersion);
#endif
    connect(dactionPackageManager, &QAction::triggered, this, &mudlet::slot_package_manager);
    connect(dactionPackageExporter, &QAction::triggered, this, &mudlet::slot_package_exporter);
    connect(dactionModuleManager, &QAction::triggered, this, &mudlet::slot_module_manager);
    connect(dactionMultiView, &QAction::triggered, this, &mudlet::slot_multi_view);
    connect(dactionInputLine, &QAction::triggered, this, &mudlet::slot_toggle_compact_input_line);
    connect(mpActionTriggers.data(), &QAction::triggered, this, &mudlet::show_trigger_dialog);
    connect(dactionScriptEditor, &QAction::triggered, this, &mudlet::show_trigger_dialog);
    connect(dactionShowMap, &QAction::triggered, this, &mudlet::slot_mapper);
    connect(dactionOptions, &QAction::triggered, this, &mudlet::slot_show_options_dialog);
    connect(dactionAbout, &QAction::triggered, this, &mudlet::slot_show_about_dialog);
    // PLACEMARKER: Save for later restoration (2 of 2) (by adding a "Close" (profile) option to first menu on menu bar:
    // connect(mactionCloseProfile, &QAction::triggered, this, &mudlet::slot_close_profile);

    // we historically use Alt on Windows and Linux, but that is uncomfortable on macOS
#if defined(Q_OS_MACOS)
    triggersKeySequence = QKeySequence(Qt::CTRL | Qt::Key_E);
    showMapKeySequence = QKeySequence(Qt::CTRL | Qt::Key_M);
    inputLineKeySequence = QKeySequence(Qt::CTRL | Qt::Key_L);
    optionsKeySequence = QKeySequence(Qt::CTRL | Qt::Key_P);
    notepadKeySequence = QKeySequence(Qt::CTRL | Qt::Key_N);
    packagesKeySequence = QKeySequence(Qt::CTRL | Qt::Key_O);
    modulesKeySequence = QKeySequence(Qt::CTRL | Qt::Key_I);
    multiViewKeySequence = QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_V);
    connectKeySequence = QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_C);
    disconnectKeySequence = QKeySequence(Qt::CTRL | Qt::Key_D);
    reconnectKeySequence = QKeySequence(Qt::CTRL | Qt::Key_R);
#else
    triggersKeySequence = QKeySequence(Qt::ALT | Qt::Key_E);
    showMapKeySequence = QKeySequence(Qt::ALT | Qt::Key_M);
    inputLineKeySequence = QKeySequence(Qt::ALT | Qt::Key_L);
    optionsKeySequence = QKeySequence(Qt::ALT | Qt::Key_P);
    notepadKeySequence = QKeySequence(Qt::ALT | Qt::Key_N);
    packagesKeySequence = QKeySequence(Qt::ALT | Qt::Key_O);
    modulesKeySequence = QKeySequence(Qt::ALT | Qt::Key_I);
    multiViewKeySequence = QKeySequence(Qt::ALT | Qt::Key_V);
    connectKeySequence = QKeySequence(Qt::ALT | Qt::Key_C);
    disconnectKeySequence = QKeySequence(Qt::ALT | Qt::Key_D);
    reconnectKeySequence = QKeySequence(Qt::ALT | Qt::Key_R);
#endif
    connect(this, &mudlet::signal_menuBarVisibilityChanged, this, &mudlet::slot_update_shortcuts);

    mpSettings = getQSettings();
    readLateSettings(*mpSettings);
    // The previous line will set an option used in the slot method:
    connect(mpMainToolBar, &QToolBar::visibilityChanged, this, &mudlet::slot_handleToolbarVisibilityChanged);

#if defined(INCLUDE_UPDATER)
    updater = new Updater(this, mpSettings);
    connect(updater, &Updater::signal_updateAvailable, this, &mudlet::slot_updateAvailable);
    connect(dactionUpdate, &QAction::triggered, this, &mudlet::slot_check_manual_update);
#if defined(Q_OS_MACOS)
    // ensure that 'Check for updates' is under the Applications menu per convention
    dactionUpdate->setMenuRole(QAction::ApplicationSpecificRole);
#else
    connect(updater, &Updater::signal_updateInstalled, this, &mudlet::slot_update_installed);
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
    loadLanguagesMap();
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

    //Open and parse the luaFunctionList document into a stringlist for use with autocomplete
    loadLuaFunctionList();

    //QFile file(fileName);
    //if( file.exists() && file.open(QIODevice::ReadOnly) ) {

    loadEdbeeTheme(QStringLiteral("Mudlet"), QStringLiteral("Mudlet.tmTheme"));
}

void mudlet::loadTranslationFile(const QString& fileName, const QString& filePath, QString& languageCode)
{
    QPointer<QTranslator> pMudletTranslator = new QTranslator();
    auto translatorList = mTranslatorsMap.value(languageCode);

    if (pMudletTranslator->load(fileName, filePath)) {
        translatorList.append(pMudletTranslator);

        if (!mTranslatorsMap.contains(languageCode)) {
            mTranslatorsMap.insert(languageCode, translatorList);
        }
    } else {
        qDebug() << "mudlet::mudlet() Failed to load translation file" << fileName << "from" << filePath;
    }

    if (languageCode != mInterfaceLanguage) {
        return;
    }

    qDebug() << "mudlet::mudlet() INFO - loading Mudlet:" << languageCode << "translations from:" << fileName;

    for (auto& translator : qAsConst(translatorList)) {
        if (!qApp->installTranslator(translator)) {
            qDebug() << "mudlet::mudlet() ERROR - Failed to directly load a translator for:" << languageCode << "a translation to the specified language will not be available";
        } else {
            mTranslatorsLoadedList.append(translator);
        }
    }
}

void mudlet::loadTranslators()
{
    auto loadTranslations =
            [=](const QString& path) {
                qDebug() << "mudlet::mudlet() INFO - Seeking Mudlet translations files in:" << path;

                QDir translationDir(path);
                translationDir.setNameFilters(QStringList() << QStringLiteral("mudlet_*.qm"));
                QStringList translationFilesList(translationDir.entryList(QDir::Files | QDir::Readable, QDir::Name));

                for (auto& translationFileName : qAsConst(translationFilesList)) {
                    QString languageCode(translationFileName);

                    languageCode.remove(QStringLiteral("mudlet_"), Qt::CaseInsensitive);
                    languageCode.remove(QStringLiteral(".qm"), Qt::CaseInsensitive);

                    loadTranslationFile(translationFileName, path, languageCode);
                }
            };

    QPointer<QTranslator> pMudletTranslator = new QTranslator();
    auto translatorList = mTranslatorsMap.value(QStringLiteral("en_US"));
    translatorList.append(pMudletTranslator);
    mTranslatorsMap.insert(QStringLiteral("en_US"), translatorList);

    // Qt translations are not loaded properly at the moment
    loadTranslations(getMudletPath(qtTranslationsPath));
    loadTranslations(QStringLiteral(":/lang"));


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
        connect(moduleUninstallButton.data(), &QAbstractButton::clicked, this, &mudlet::slot_uninstall_module);
        connect(moduleInstallButton.data(), &QAbstractButton::clicked, this, &mudlet::slot_install_module);
        connect(moduleHelpButton.data(), &QAbstractButton::clicked, this, &mudlet::slot_help_module);
        connect(moduleTable.data(), &QTableWidget::itemClicked, this, &mudlet::slot_module_clicked);
        connect(moduleTable.data(), &QTableWidget::itemChanged, this, &mudlet::slot_module_changed);
        connect(mpModuleDlg.data(), &QObject::destroyed, this, &mudlet::slot_module_manager_destroyed);
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
        connect(uninstallButton.data(), &QAbstractButton::clicked, this, &mudlet::slot_uninstall_package);
        connect(installButton.data(), &QAbstractButton::clicked, this, &mudlet::slot_install_package);
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
    QString name = mpTabBar->tabData(tab).toString();
    Host* pH = mHostManager.getHost(name);
    if (!pH) {
        return;
    }

    list<QPointer<TToolBar>> hostToolBarMap = pH->getActionUnit()->getToolBarList();
    QMap<QString, TDockWidget*>& dockWindowMap = pH->mpConsole->mDockWidgetMap;
    QMap<QString, TConsole*>& hostConsoleMap = pH->mpConsole->mSubConsoleMap;

    if (!pH->mpConsole->close()) {
        return;
    }

    pH->mpConsole->mUserAgreedToCloseConsole = true;
    pH->closingDown();

    // disconnect before removing objects from memory as sysDisconnectionEvent needs that stuff.
#if defined (Q_OS_WIN32)
    if (pH->mSslTsl) {
        pH->mTelnet.abortConnection();
    } else {
        pH->mTelnet.disconnect();
    }
#else
    pH->mTelnet.disconnect();
#endif

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
                QMap<QString, TDockWidget*>& dockWindowMap = pH->mpConsole->mDockWidgetMap;
                QMap<QString, TConsole*>& hostConsoleMap = pH->mpConsole->mSubConsoleMap;
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

                if (pH->mpNotePad) {
                    pH->mpNotePad->save();
                    pH->mpNotePad->setAttribute(Qt::WA_DeleteOnClose);
                    pH->mpNotePad->close();
                    pH->mpNotePad = nullptr;
                }

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
    if ((tabID != -1) && (!mTabMap.contains(mpTabBar->tabData(tabID).toString()))) {
        mpCurrentActiveHost = nullptr;
        return;
    }

    // Reset the tab back to "normal" to undo the effect of it having it's style
    // changed on new data:
    mpTabBar->setTabBold(tabID, false);
    mpTabBar->setTabItalic(tabID, false);
    mpTabBar->setTabUnderline(tabID, false);

    if (mConsoleMap.contains(mpCurrentActiveHost)) {
        mpCurrentActiveHost->mpConsole->hide();
        QString host = mpTabBar->tabData(tabID).toString();
        if (mTabMap.contains(host)) {
            mpCurrentActiveHost = mTabMap[host]->mpHost;
        } else {
            mpCurrentActiveHost = nullptr;
            return;
        }
    } else {
        if (!mTabMap.empty()) {
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

    emit signal_tabChanged(mpCurrentActiveHost->getName());
}

void mudlet::addConsoleForNewHost(Host* pH)
{
    if (mConsoleMap.contains(pH)) {
        return;
    }
    pH->mLogStatus = mAutolog;
    auto pConsole = new TConsole(pH, TConsole::MainConsole);
    if (!pConsole) {
        return;
    }
    pH->mpConsole = pConsole;
    pConsole->setWindowTitle(pH->getName());
    pConsole->setObjectName(pH->getName());
    mConsoleMap[pH] = pConsole;
    QString tabName = pH->getName();
    int newTabID = mpTabBar->addTab(tabName);
    /*
     * There is a sneaky feature on some OSes (I found it on FreeBSD but
     * it is notable switched OFF by default on MacOs) where Qt adds an
     * automatically generated accelarator to the text on the tab which - at
     * least on FreeBSD - causes the Text to be CHANGED from what is set (an
     * underscore is added to a suitably unique letter but that, being a text
     * accelerator is converted to an additional '&' in the text when it is
     * read) - this messes up identifying the tab by it's name - so we now get
     * around it by also storing the text in the tab's data - see:
     * + void qt_set_sequence_auto_mnemonic(bool) in 'QKeySequence' documentation
     * + "Detailed Description" in 'QShortCut' documentation
     * + "QTabBar creates automatic mnemonic keys in the manner of
     *    QAbstractButton; e.g. if a tab's label is '&Graphics', Alt+G becomes
     *    a shortcut key for switching to that tab." in 'QTabBar' documentation"
     */
    mpTabBar->setTabData(newTabID, tabName);
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
    connect(pH, &Host::profileSaveStarted,  pH->mpEditorDialog, &dlgTriggerEditor::slot_profileSaveStarted);
    connect(pH, &Host::profileSaveFinished,  pH->mpEditorDialog, &dlgTriggerEditor::slot_profileSaveFinished);
    pEditor->fillout_form();

    pH->getActionUnit()->updateToolbar();
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
        connect(pQT, &QTimer::timeout, this, &mudlet::slot_timer_fires);
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

    mpActionReplay->setEnabled(false);
    mpActionReplay->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                               .arg(tr("<p>Load a Mudlet replay.</p>"
                                       "<p><i>Disabled until a profile is loaded.</i></p>")));
    // The menu items will not show tool-tips unless the parent menu is set to
    // show tool-tips which is likely to be done in near future when there are
    // more texts to show {the default is to repeat the menu text which is not
    // useful} with a call to menuEditor->setToolTipsVisible(true);
    dactionReplay->setToolTip(mpActionReplay->toolTip());

    dactionReplay->setEnabled(false);
    mpActionReconnect->setEnabled(false);
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

    if (!mpToolBarReplay) {
        // Only enable the replay button if it is not disabled because there is
        // another profile loaded and already playing a replay {when the replay
        // toolbar pointer will be non-null}:
        mpActionReplay->setEnabled(true);
        dactionReplay->setEnabled(true);
        mpActionReplay->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                   .arg(tr("<p>Load a Mudlet replay.</p>")));
        // The menu items will not show tool-tips unless the parent menu is set to
        // show tool-tips which is likely to be done in near future when there are
        // more texts to show {the default is to repeat the menu text which is not
        // useful} with a call to menuEditor->setToolTipsVisible(true);
        dactionReplay->setToolTip(mpActionReplay->toolTip());
    }

    mpActionReconnect->setEnabled(true);

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
    if (!pHost || !pHost->mpConsole) {
        return;
    }

    auto pD = pHost->mpConsole->mDockWidgetMap.value(name);
    if (!mHostDockLayoutChangeMap.value(pHost).contains(name) && pD) {
        pD->setObjectName(QStringLiteral("%1_changed").arg(pD->objectName()));
        mHostDockLayoutChangeMap[pHost].append(name);

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
        QMap<QString, TDockWidget*>& dockWindowMap = pHost->mpConsole->mDockWidgetMap;
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

bool mudlet::setWindowFont(Host* pHost, const QString& window, const QString& font)
{
    if (!pHost || !pHost->mpConsole) {
        return false;
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(window);
    if (pC) {
        pC->setMiniConsoleFont(font);
        return true;
    } else {
        return false;
    }
}

QString mudlet::getWindowFont(Host* pHost, const QString& name)
{
    if (!pHost || !pHost->mpConsole) {
        return QString();
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
    if (pC) {
        return pC->mUpperPane->fontInfo().family();
    } else {
        return QString();
    }
}

bool mudlet::setWindowFontSize(Host *pHost, const QString &name, int size)
{
    if (!pHost || !pHost->mpConsole) {
        return false;
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
    if (pC) {
        pC->setMiniConsoleFontSize(size);
        return true;
    } else {
        return false;
    }
}

int mudlet::getFontSize(Host* pHost, const QString& name)
{
    if (!pHost || !pHost->mpConsole) {
        return -1;
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
    if (pC) {
        return pC->mUpperPane->mDisplayFont.pointSize();
    } else {
        return -1;
    }
}

QSize mudlet::calcFontSize(Host* pHost, const QString& windowName)
{
    if (!pHost || !pHost->mpConsole) {
        return QSize(-1, -1);
    }

    QFont font;
    if (windowName.isEmpty() || windowName.compare(QStringLiteral("main"), Qt::CaseSensitive) == 0) {
        font = pHost->mDisplayFont;
    } else {
        auto pC = pHost->mpConsole->mSubConsoleMap.value(windowName);
        if (pC) {
            Q_ASSERT_X(pC->mUpperPane, "calcFontSize", "located console does not have the upper pane available");
            font = pC->mUpperPane->mDisplayFont;
        } else {
            return QSize(-1, -1);
        }
    }

    auto fontMetrics = QFontMetrics(font);
    return QSize(fontMetrics.width(QChar('W')), fontMetrics.height());
}

bool mudlet::openWindow(Host* pHost, const QString& name, bool loadLayout)
{
    if (!pHost || !pHost->mpConsole) {
        return false;
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
    auto pD = pHost->mpConsole->mDockWidgetMap.value(name);

    if (!pC && !pD) {
        // The name is not used in either the QMaps of all user created TConsole
        // or TDockWidget instances - so we can make a NEW one:
        auto pD = new TDockWidget(pHost, name);
        pD->setObjectName(QString("dockWindow_%1_%2").arg(pHost->getName(), name));
        pD->setContentsMargins(0, 0, 0, 0);
        pD->setFeatures(QDockWidget::AllDockWidgetFeatures);
        pD->setWindowTitle(tr("User window - %1 - %2").arg(pHost->getName(), name));
        pHost->mpConsole->mDockWidgetMap.insert(name, pD);
        // It wasn't obvious but the parent passed to the TConsole constructor
        // is sliced down to a QWidget and is NOT a TDockWidget pointer:
        auto pC = new TConsole(pHost, TConsole::UserWindow, pD->widget());
        pC->setContentsMargins(0, 0, 0, 0);
        pD->setTConsole(pC);
        pC->show();
        pC->layerCommandLine->hide();
        pC->mpScrollBar->hide();
        pHost->mpConsole->mSubConsoleMap.insert(name, pC);
        // TODO: Allow user to specify alternate dock locations - and for it to be floating and not docked initially!
        addDockWidget(Qt::RightDockWidgetArea, pD);

        setWindowFontSize(pHost, name, 10);

        if (loadLayout && !pD->hasLayoutAlready) {
            loadWindowLayout();
            pD->hasLayoutAlready = true;
        }

        return true;
    } else if (pC && pD) {
        // The name is used in BOTH the QMaps of all user created TConsole
        // and TDockWidget instances - so we HAVE an existing user window,
        // Lets confirm this:
        Q_ASSERT_X(pC->getType()==TConsole::UserWindow, "mudlet::openWindow(...)", "An existing TConsole was expected to be marked as a User Window type but it isn't");
        pD->update();
        //do not change the ->show() order! Otherwise, it will automatically minimize the floating/dock window(!!)
        pC->show();
        pD->show();
        pC->showWindow(name);

        if (loadLayout && !pD->hasLayoutAlready) {
            loadWindowLayout();
            pD->hasLayoutAlready = true;
        }

        return true;
    }

    return false;
}

bool mudlet::createMiniConsole(Host* pHost, const QString& name, int x, int y, int width, int height)
{
    if (!pHost || !pHost->mpConsole) {
        return false;
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
    if (!pC) {
        TConsole* pC = pHost->mpConsole->createMiniConsole(name, x, y, width, height);
        if (pC) {
            pC->setMiniConsoleFontSize(12);
            return true;
        }
    } else {
        // CHECK: The absence of an explict return statement in this block means that
        // reusing an existing mini console causes the lua function to seem to
        // fail - is this as per Wiki?
        pC->resize(width, height);
        pC->move(x, y);
    }
    return false;
}

bool mudlet::createLabel(Host* pHost, const QString& name, int x, int y, int width, int height, bool fillBg,
                         bool clickthrough)
{
    if (!pHost || !pHost->mpConsole) {
        return false;
    }

    auto pL = pHost->mpConsole->mLabelMap.value(name);
    if (!pL) {
        pL = pHost->mpConsole->createLabel(name, x, y, width, height, fillBg, clickthrough);
        if (pL) {
            return true;
        }
    }
    return false;
}

bool mudlet::createBuffer(Host* pHost, const QString& name)
{
    if (!pHost || !pHost->mpConsole) {
        return false;
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
    if (!pC) {
        pC = pHost->mpConsole->createBuffer(name);
        if (pC) {
            return true;
        }
    }
    return false;
}

bool mudlet::setBackgroundColor(Host* pHost, const QString& name, int r, int g, int b, int alpha)
{
    if (!pHost || !pHost->mpConsole) {
        return false;
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
    auto pL = pHost->mpConsole->mLabelMap.value(name);
    if (pC) {
        pC->setConsoleBgColor(r, g, b);
        return true;
    } else if (pL) {
        QPalette mainPalette;
        mainPalette.setColor(QPalette::Window, QColor(r, g, b, alpha));
        pL->setPalette(mainPalette);
        return true;
    }

    return false;
}

bool mudlet::setBackgroundImage(Host* pHost, const QString& name, QString& path)
{
    if (!pHost || !pHost->mpConsole) {
        return false;
    }

    auto pL = pHost->mpConsole->mLabelMap.value(name);
    if (pL) {
        if (QDir::homePath().contains('\\')) {
            path.replace('/', R"(\)");
        } else {
            path.replace('\\', "/");
        }
        QPixmap bgPixmap(path);
        pL->setPixmap(bgPixmap);
        return true;
    } else {
        return false;
    }
}

bool mudlet::setTextFormat(Host* pHost, const QString& name, const QColor& bgColor, const QColor& fgColor, const TChar::AttributeFlags attributes)
{
    if (!pHost || !pHost->mpConsole) {
        return false;
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
    if (pC) {
        pC->mFormatCurrent.setTextFormat(fgColor, bgColor, attributes);
        return true;
    } else {
        return false;
    }
}

bool mudlet::setDisplayAttributes(Host* pHost, const QString& name, const TChar::AttributeFlags attributes, const bool state)
{
    if (!pHost || !pHost->mpConsole) {
        return false;
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
    if (pC) {
        // Set or reset all the specified attributes (but leave others unchanged)
        pC->mFormatCurrent.setAllDisplayAttributes((pC->mFormatCurrent.allDisplayAttributes() &~(attributes)) | (state ? attributes : TChar::None));
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
    if (!pHost || !pHost->mpConsole) {
        return false;
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
    if (pC) {
        pC->buffer.clear();
        pC->mUpperPane->update();
        return true;
    } else {
        return false;
    }
}

bool mudlet::showWindow(Host* pHost, const QString& name)
{
    if (!pHost || !pHost->mpConsole) {
        return false;
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
    auto pL = pHost->mpConsole->mLabelMap.value(name);
    // check labels first as they are shown/hidden more often
    if (pL) {
        pL->show();
        return true;
    } else if (pC) {
        auto pD = pHost->mpConsole->mDockWidgetMap.value(name);
        if (pD) {
            pD->update();
            pD->show();
            // TODO: conside refactoring TConsole::showWindow(name) so that there is a TConsole::showWindow() that can be called directly on the TConsole concerned?
            return pHost->mpConsole->showWindow(name);
        } else {
            return pHost->mpConsole->showWindow(name);
        }
    }

    return false;
}

bool mudlet::paste(Host* pHost, const QString& name)
{
    if (!pHost || !pHost->mpConsole) {
        return false;
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
    if (pC) {
        pC->paste();
        return true;
    } else {
        return false;
    }
}

bool mudlet::hideWindow(Host* pHost, const QString& name)
{
    if (!pHost || !pHost->mpConsole) {
        return false;
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
    auto pL = pHost->mpConsole->mLabelMap.value(name);
    // check labels first as they are shown/hidden more often
    if (pL) {
        pL->hide();
        return true;
    } else if (pC) {
        auto pD = pHost->mpConsole->mDockWidgetMap.value(name);
        if (pD) {
            pD->hide();
            pD->update();
        }
        return pHost->mpConsole->hideWindow(name);
    }

    return false;
}

bool mudlet::resizeWindow(Host* pHost, const QString& name, int x1, int y1)
{
    if (!pHost || !pHost->mpConsole) {
        return false;
    }

    auto pL = pHost->mpConsole->mLabelMap.value(name);
    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
    auto pD = pHost->mpConsole->mDockWidgetMap.value(name);
    if (pL) {
        pL->resize(x1, y1);
        return true;
    } else if (pC) {
        if (pD) {
            if (!pD->isFloating()) {
                // Can't resize a docked window...?
                return false;
            } else {
                pD->resize(x1, y1);
                return true;
            }
        } else {
            pC->resize(x1, y1);
            return true;
        }
    } else {
        return false;
    }
}

bool mudlet::setConsoleBufferSize(Host* pHost, const QString& name, int x1, int y1)
{
    if (!pHost || !pHost->mpConsole) {
        return false;
    }

    if (name.isEmpty() || name == QLatin1String("main")) {
        pHost->mpConsole->buffer.setBufferSize(x1, y1);
        return true;
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
    if (pC) {
        pC->buffer.setBufferSize(x1, y1);
        return true;
    } else {
        return false;
    }
}

bool mudlet::setScrollBarVisible(Host* pHost, const QString& name, bool isVisible)
{
    if (!pHost || !pHost->mpConsole) {
        return false;
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
    if (pC) {
        pC->setScrollBarVisible(isVisible);
        return true;
    } else {
        return false;
    }
}

bool mudlet::resetFormat(Host* pHost, QString& name)
{
    if (!pHost || !pHost->mpConsole) {
        return false;
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
    if (pC) {
        pC->reset();
        return true;
    } else {
        return false;
    }
}

bool mudlet::moveWindow(Host* pHost, const QString& name, int x1, int y1)
{
    if (!pHost || !pHost->mpConsole) {
        return false;
    }

    auto pL = pHost->mpConsole->mLabelMap.value(name);
    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
    auto pD = pHost->mpConsole->mDockWidgetMap.value(name);
    if (pL) {
        pL->move(x1, y1);
        return true;
    } else if (pC && !pD) {
        // NOT a floatable/dockable "user window"
        pC->move(x1, y1);
        pC->mOldX = x1;
        pC->mOldY = y1;
        return true;
    } if (pC && pD) {
        if (!pD->isFloating()) {
            // Undock a docked window
            pD->setFloating(true);
        }

        pD->move(x1, y1);
        return true;
    }

    return false;
}

bool mudlet::closeWindow(Host* pHost, const QString& name)
{
    if (!pHost || !pHost->mpConsole) {
        return false;
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
    if (pC) {
        auto pD = pHost->mpConsole->mDockWidgetMap.value(name);
        if (pD) {
            pD->hide();
            pD->update();
        }
        return pHost->mpConsole->hideWindow(name);

    } else {
        return false;
    }
}

bool mudlet::setLabelClickCallback(Host* pHost, const QString& name, const QString& func, const TEvent& pA)
{
    if (!pHost || !pHost->mpConsole) {
        return false;
    }

    auto pL = pHost->mpConsole->mLabelMap.value(name);
    if (pL) {
        pL->setClick(pHost, func, pA);
        return true;
    } else {
        return false;
    }
}

bool mudlet::setLabelDoubleClickCallback(Host* pHost, const QString& name, const QString& func, const TEvent& pA)
{
    if (!pHost || !pHost->mpConsole) {
        return false;
    }

    auto pL = pHost->mpConsole->mLabelMap.value(name);
    if (pL) {
        pL->setDoubleClick(pHost, func, pA);
        return true;
    } else {
        return false;
    }
}

bool mudlet::setLabelReleaseCallback(Host* pHost, const QString& name, const QString& func, const TEvent& pA)
{
    if (!pHost || !pHost->mpConsole) {
        return false;
    }

    auto pL = pHost->mpConsole->mLabelMap.value(name);
    if (pL) {
        pL->setRelease(pHost, func, pA);
        return true;
    } else {
        return false;
    }
}

bool mudlet::setLabelMoveCallback(Host* pHost, const QString& name, const QString& func, const TEvent& pA)
{
    if (!pHost || !pHost->mpConsole) {
        return false;
    }

    auto pL = pHost->mpConsole->mLabelMap.value(name);
    if (pL) {
        pL->setMove(pHost, func, pA);
        return true;
    } else {
        return false;
    }
}

bool mudlet::setLabelWheelCallback(Host* pHost, const QString& name, const QString& func, const TEvent& pA)
{
    if (!pHost || !pHost->mpConsole) {
        return false;
    }

    auto pL = pHost->mpConsole->mLabelMap.value(name);
    if (pL) {
        pL->setWheel(pHost, func, pA);
        return true;
    } else {
        return false;
    }
}

bool mudlet::setLabelOnEnter(Host* pHost, const QString& name, const QString& func, const TEvent& pA)
{
    if (!pHost || !pHost->mpConsole) {
        return false;
    }

    auto pL = pHost->mpConsole->mLabelMap.value(name);
    if (pL) {
        pL->setEnter(pHost, func, pA);
        return true;
    } else {
        return false;
    }
}

bool mudlet::setLabelOnLeave(Host* pHost, const QString& name, const QString& func, const TEvent& pA)
{
    if (!pHost || !pHost->mpConsole) {
        return false;
    }

    auto pL = pHost->mpConsole->mLabelMap.value(name);
    if (pL) {
        pL->setLeave(pHost, func, pA);
        return true;
    } else {
        return false;
    }
}

std::pair<bool, int> mudlet::getLineNumber(Host* pHost, QString& windowName)
{
    if (!pHost || !pHost->mpConsole) {
        return make_pair(false, -1);
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(windowName);
    if (pC) {
        return make_pair(true, pC->getLineNumber());
    } else {
        TDebug(QColor(Qt::white), QColor(Qt::red)) << QStringLiteral("ERROR: window doesn't exist\n") >> 0;
        return make_pair(false, -1);
    }
}

int mudlet::getColumnNumber(Host* pHost, QString& name)
{
    if (!pHost || !pHost->mpConsole) {
        return -1;
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
    if (pC) {
        return pC->getColumnNumber();
    } else {
        TDebug(QColor(Qt::white), QColor(Qt::red)) << "ERROR: window doesn't exist\n" >> 0;
        return -1;
    }
}

int mudlet::getLastLineNumber(Host* pHost, const QString& name)
{
    if (!pHost || !pHost->mpConsole) {
        return -1;
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
    if (pC) {
        return pC->getLastLineNumber();
    } else {
        TDebug(QColor(Qt::white), QColor(Qt::red)) << "ERROR: window doesn't exist\n" >> 0;
        return -1;
    }
}

bool mudlet::moveCursorEnd(Host* pHost, const QString& name)
{
    if (!pHost || !pHost->mpConsole) {
        return false;
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
    if (pC) {
        pC->moveCursorEnd();
        return true;
    } else {
        return false;
    }
}

bool mudlet::moveCursor(Host* pHost, const QString& name, int x, int y)
{
    if (!pHost || !pHost->mpConsole) {
        return false;
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
    if (pC) {
        return pC->moveCursor(x, y);
    } else {
        return false;
    }
}

void mudlet::deleteLine(Host* pHost, const QString& name)
{
    if (!pHost || !pHost->mpConsole) {
        return;
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
    if (pC) {
        pC->skipLine();
    }
}

bool mudlet::insertText(Host* pHost, const QString& windowName, const QString& text)
{
    if (!pHost || !pHost->mpConsole) {
        return false;
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(windowName);
    if (pC) {
        pC->insertText(text);
        return true;
    } else {
        return false;
    }
}

void mudlet::insertLink(Host* pHost, const QString& name, const QString& text, QStringList& func, QStringList& hint, bool customFormat)
{
    if (!pHost || !pHost->mpConsole) {
        return;
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
    if (pC) {
        pC->insertLink(text, func, hint, customFormat);
    }
}

void mudlet::replace(Host* pHost, const QString& name, const QString& text)
{
    if (!pHost || !pHost->mpConsole) {
        return;
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
    if (pC) {
        pC->replace(text);
    }
}

void mudlet::setLink(Host* pHost, const QString& name, QStringList& linkFunction, QStringList& linkHint)
{
    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
    if (pC) {
        pC->setLink(linkFunction, linkHint);
    }
}

void mudlet::setFgColor(Host* pHost, const QString& name, int r, int g, int b)
{
    if (!pHost || !pHost->mpConsole) {
        return;
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
    if (pC) {
        pC->setFgColor(r, g, b);
    }
}

void mudlet::setBgColor(Host* pHost, const QString& name, int r, int g, int b)
{
    if (!pHost || !pHost->mpConsole) {
        return;
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
    if (pC) {
        pC->setBgColor(r, g, b);
    }
}

int mudlet::selectString(Host* pHost, const QString& name, const QString& text, int num)
{
    if (!pHost || !pHost->mpConsole) {
        return -1;
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
    if (pC) {
        return pC->select(text, num);
    } else {
        return -1;
    }
}

int mudlet::selectSection(Host* pHost, const QString& name, int f, int t)
{
    if (!pHost || !pHost->mpConsole) {
        return -1;
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
    if (pC) {
        return pC->selectSection(f, t);
    } else {
        return -1;
    }
}

std::tuple<bool, QString, int, int> mudlet::getSelection(Host* pHost, const QString& windowName)
{
    if (!pHost || !pHost->mpConsole) {
        return make_tuple(false, QStringLiteral(R"(internal error, Host pointer had nullptr value)"), 0, 0);
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(windowName);
    if (pC) {
        return pC->getSelection();
    } else {
        return make_tuple(false, QStringLiteral(R"(window "%s" not found)").arg(windowName.toUtf8().constData()), 0, 0);
    }
}

// Added a return value to indicate whether the given windows name was found
bool mudlet::deselect(Host* pHost, const QString& name)
{
    if (!pHost || !pHost->mpConsole) {
        return -1;
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
    if (pC) {
        pC->deselect();
        return true;
    } else {
        return false;
    }
}

bool mudlet::setWindowWrap(Host* pHost, const QString& name, int& wrap)
{
    if (!pHost || !pHost->mpConsole) {
        return -1;
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
    if (pC) {
        pC->setWrapAt(wrap);
        return true;
    } else {
        return false;
    }
}

bool mudlet::setWindowWrapIndent(Host* pHost, const QString& name, int& wrap)
{
    if (!pHost || !pHost->mpConsole) {
        return -1;
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
    if (pC) {
        pC->setIndentCount(wrap);
        return true;
    } else {
        return false;
    }
}

bool mudlet::echoWindow(Host* pHost, const QString& name, const QString& text)
{
    if (!pHost || !pHost->mpConsole) {
        return -1;
    }

    auto pL = pHost->mpConsole->mLabelMap.value(name);
    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
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

bool mudlet::echoLink(Host* pHost, const QString& name, const QString& text, QStringList& func, QStringList& hint, bool customFormat)
{
    if (!pHost || !pHost->mpConsole) {
        return -1;
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
    if (pC) {
        pC->echoLink(text, func, hint, customFormat);
        return true;
    } else {
        return false;
    }
}

bool mudlet::copy(Host* pHost, const QString& name)
{
    if (!pHost || !pHost->mpConsole) {
        return -1;
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
    if (pC) {
        pC->copy();
        return true;
    } else {
        return false;
    }
}

bool mudlet::pasteWindow(Host* pHost, const QString& name)
{
    if (!pHost || !pHost->mpConsole) {
        return -1;
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
    if (pC) {
        pC->pasteWindow(mConsoleMap[pHost]->mClipboard);
        return true;
    } else {
        return false;
    }
}

bool mudlet::appendBuffer(Host* pHost, const QString& name)
{
    if (!pHost || !pHost->mpConsole) {
        return -1;
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
    if (pC) {
        pC->appendBuffer(mConsoleMap[pHost]->mClipboard);
        return true;
    } else {
        return false;
    }
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
        if (pC->mpHost->getName() != QStringLiteral("default_host")) {
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
                pC->mpHost->mpNotePad = nullptr;
            }

            // close console
            pC->close();
        }
    }

    // hide main Mudlet window once we're sure the 'do you want to save the profile?' won't come up
    hide();

    for (auto& hostName: mudlet::self()->getHostManager().getHostList()) {
        auto host = mHostManager.getHost(hostName);
        if (host->currentlySavingProfile()) {
            host->waitForProfileSave();
        }
    }

    // pass the event on so dblsqd can perform an update
    // if automatic updates have been disabled
    event->accept();
}

void mudlet::forceClose()
{
    for (auto& console : mConsoleMap) {
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
                host->mpNotePad = nullptr;
            }

            if (mpIrcClientMap.contains(host)) {
                mpIrcClientMap.value(host)->close();
            }
        }

        console->close();
    }    

    // hide main Mudlet window once we're sure the 'do you want to save the profile?' won't come up
    hide();

    for (auto& hostName: mudlet::self()->getHostManager().getHostList()) {
        auto host = mHostManager.getHost(hostName);
        if (host->currentlySavingProfile()) {
            host->waitForProfileSave();
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
    if (settings.contains(QStringLiteral("enableFullScreenMode"))) {
        // We have a setting stored for this
        mEnableFullScreenMode = settings.value(QStringLiteral("enableFullScreenMode"), QVariant(false)).toBool();
    } else {
        // We do not have a QSettings value stored so check for the sentinel file:
        QFile file_use_smallscreen(getMudletPath(mainDataItemPath, QStringLiteral("mudlet_option_use_smallscreen")));
        mEnableFullScreenMode = file_use_smallscreen.exists();
    }

    mInterfaceLanguage = settings.value("interfaceLanguage", QStringLiteral("en_US")).toString();
}

void mudlet::readLateSettings(const QSettings& settings)
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
    // We have abandoned previous "showMenuBar" / "showToolBar" booleans
    // although we provide a backwards compatible value
    // of: (bool) showXXXXBar = (XXXXBarVisibilty != visibleNever) for, until,
    // it is suggested Mudlet 4.x:
    setMenuBarVisibility(static_cast<controlsVisibilityFlag>(settings.value("menuBarVisibility", static_cast<int>(visibleAlways)).toInt()));
    setToolBarVisibility(static_cast<controlsVisibilityFlag>(settings.value("toolBarVisibility", static_cast<int>(visibleAlways)).toInt()));
    mEditorTextOptions = static_cast<QTextOption::Flags>(settings.value("editorTextOptions", QVariant(0)).toInt());

    mshowMapAuditErrors = settings.value("reportMapIssuesToConsole", QVariant(false)).toBool();
    mCompactInputLine = settings.value("compactInputLine", QVariant(false)).toBool();


    resize(size);
    move(pos);
    if (settings.value("maximized", false).toBool()) {
        showMaximized();
    }
    mCopyAsImageTimeout = settings.value(QStringLiteral("copyAsImageTimeout"), mCopyAsImageTimeout).toInt();
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
    // Are there any profiles loaded - note that the dummy "default_host" counts
    // as the first one
    if ((mHostManager.getHostCount() < 2 && mMenuBarVisibility & visibleAlways)
      ||(mMenuBarVisibility & visibleMaskNormally)) {

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
        int hostCount = mHostManager.getHostCount();
        if ((hostCount < 2 && (mToolbarVisibility & visibleAlways)) || (hostCount >= 2 && (mToolbarVisibility & visibleMaskNormally))) {
            mpMainToolBar->show();
        }
    }
}

void mudlet::adjustToolBarVisibility()
{
    // Are there any profiles loaded - note that the dummy "default_host" counts
    // as the first one
    if ((mHostManager.getHostCount() < 2 && mToolbarVisibility & visibleAlways) || (mToolbarVisibility & visibleMaskNormally)) {
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
    settings.setValue("reportMapIssuesToConsole", mshowMapAuditErrors);
    settings.setValue("compactInputLine", mCompactInputLine);
    settings.setValue("showIconsInMenus", mShowIconsOnMenuCheckedState);
    settings.setValue("enableFullScreenMode", mEnableFullScreenMode);
    settings.setValue("copyAsImageTimeout", mCopyAsImageTimeout);
    settings.setValue("interfaceLanguage", mInterfaceLanguage);
}

void mudlet::slot_show_connection_dialog()
{
    auto pDlg = new dlgConnectionProfiles(this);
    connect(pDlg, &dlgConnectionProfiles::signal_establish_connection, this, &mudlet::slot_connection_dlg_finished);
    pDlg->fillout_form();

    connect(pDlg, &QDialog::accepted, this, [=]() { enableToolbarButtons(); });
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


void mudlet::show_options_dialog(QString tab)
{
    Host* pHost = getActiveHost();

    // value will automatically return a nullptr if there is NO entry for this
    // Host in the QMap
    if (!mpProfilePreferencesDlgMap.value(pHost)) {
        mpProfilePreferencesDlgMap[pHost] = new dlgProfilePreferences(this, pHost);

        connect(mpActionReconnect.data(), &QAction::triggered, mpProfilePreferencesDlgMap.value(pHost)->need_reconnect_for_data_protocol, &QWidget::hide);
        connect(dactionReconnect, &QAction::triggered, mpProfilePreferencesDlgMap.value(pHost)->need_reconnect_for_data_protocol, &QWidget::hide);
        connect(mpActionReconnect.data(), &QAction::triggered, mpProfilePreferencesDlgMap.value(pHost)->need_reconnect_for_specialoption, &QWidget::hide);
        connect(dactionReconnect, &QAction::triggered, mpProfilePreferencesDlgMap.value(pHost)->need_reconnect_for_specialoption, &QWidget::hide);
        mpProfilePreferencesDlgMap.value(pHost)->setAttribute(Qt::WA_DeleteOnClose);
    }
    mpProfilePreferencesDlgMap.value(pHost)->setTab(tab);
    mpProfilePreferencesDlgMap.value(pHost)->raise();
    mpProfilePreferencesDlgMap.value(pHost)->show();
}

void mudlet::slot_update_shortcuts()
{
    if (mpMainToolBar->isVisible()) {
        triggersShortcut = new QShortcut(triggersKeySequence, this);
        connect(triggersShortcut.data(), &QShortcut::activated, this, &mudlet::show_trigger_dialog);
        dactionScriptEditor->setShortcut(QKeySequence());

        showMapShortcut = new QShortcut(showMapKeySequence, this);
        connect(showMapShortcut.data(), &QShortcut::activated, this, &mudlet::slot_mapper);
        dactionShowMap->setShortcut(QKeySequence());

        inputLineShortcut = new QShortcut(inputLineKeySequence, this);
        connect(inputLineShortcut.data(), &QShortcut::activated, this, &mudlet::slot_toggle_compact_input_line);
        dactionInputLine->setShortcut(QKeySequence());

        optionsShortcut = new QShortcut(optionsKeySequence, this);
        connect(optionsShortcut.data(), &QShortcut::activated, this, &mudlet::slot_show_options_dialog);
        dactionOptions->setShortcut(QKeySequence());

        notepadShortcut = new QShortcut(notepadKeySequence, this);
        connect(notepadShortcut.data(), &QShortcut::activated, this, &mudlet::slot_notes);
        dactionNotepad->setShortcut(QKeySequence());

        packagesShortcut = new QShortcut(packagesKeySequence, this);
        connect(packagesShortcut.data(), &QShortcut::activated, this, &mudlet::slot_show_options_dialog);
        dactionPackageManager->setShortcut(QKeySequence());

        modulesShortcut = new QShortcut(packagesKeySequence, this);
        connect(modulesShortcut.data(), &QShortcut::activated, this, &mudlet::slot_module_manager);
        dactionModuleManager->setShortcut(QKeySequence());

        multiViewShortcut = new QShortcut(multiViewKeySequence, this);
        connect(multiViewShortcut.data(), &QShortcut::activated, this, &mudlet::slot_multi_view);
        dactionMultiView->setShortcut(QKeySequence());

        connectShortcut = new QShortcut(connectKeySequence, this);
        connect(connectShortcut.data(), &QShortcut::activated, this, &mudlet::slot_show_connection_dialog);
        dactionConnect->setShortcut(QKeySequence());

        disconnectShortcut = new QShortcut(disconnectKeySequence, this);
        connect(disconnectShortcut.data(), &QShortcut::activated, this, &mudlet::slot_disconnect);
        dactionDisconnect->setShortcut(QKeySequence());

        reconnectShortcut = new QShortcut(reconnectKeySequence, this);
        connect(reconnectShortcut.data(), &QShortcut::activated, this, &mudlet::slot_reconnect);
        dactionReconnect->setShortcut(QKeySequence());
    } else {
        triggersShortcut.clear();
        dactionScriptEditor->setShortcut(triggersKeySequence);

        showMapShortcut.clear();
        dactionShowMap->setShortcut(showMapKeySequence);

        inputLineShortcut.clear();
        dactionInputLine->setShortcut(inputLineKeySequence);

        optionsShortcut.clear();
        dactionOptions->setShortcut(optionsKeySequence);

        notepadShortcut.clear();
        dactionNotepad->setShortcut(notepadKeySequence);

        packagesShortcut.clear();
        dactionPackageManager->setShortcut(packagesKeySequence);

        modulesShortcut.clear();
        dactionModuleManager->setShortcut(modulesKeySequence);

        multiViewShortcut.clear();
        dactionMultiView->setShortcut(multiViewKeySequence);

        connectShortcut.clear();
        dactionConnect->setShortcut(connectKeySequence);

        disconnectShortcut.clear();
        dactionDisconnect->setShortcut(disconnectKeySequence);

        reconnectShortcut.clear();
        dactionReconnect->setShortcut(reconnectKeySequence);
    }
}

void mudlet::slot_show_options_dialog()
{
    show_options_dialog("General");
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

        connect(dialog, &QDialog::accepted, this, &mudlet::slot_open_mappingscripts_page);

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

    for (auto& host : hostList) {
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
        pHost->refreshPackageFonts();
    }

    pHost->setLogin(readProfileData(profile_name, QStringLiteral("login")));
    pHost->setPass(readProfileData(profile_name, QStringLiteral("password")));

    // This settings also need to be configured, note that the only time not to
    // save the setting is on profile loading:
    pHost->mTelnet.setEncoding(readProfileData(profile_name, QLatin1String("encoding")), false);

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
    pHost->mTelnet.sendData(login);
}

void mudlet::slot_send_pass()
{
    if (tempHostQueue.isEmpty()) {
        return;
    }
    Host* pHost = tempHostQueue.dequeue();
    QString pass = pHost->getPass();
    pHost->mTelnet.sendData(pass);
}
//////////////////////////////////////////////////////////////////////////////


void mudlet::processEventLoopHack()
{
    QTimer::singleShot(1, this, &mudlet::processEventLoopHack_timerRun);
}

void mudlet::processEventLoopHack_timerRun()
{
    Host* pH = getActiveHost();
    if (!pH) {
        return;
    }
    pH->mpConsole->refresh();
}

void mudlet::hideMudletsVariables(Host* pHost)
{
    auto varUnit = pHost->getLuaInterface()->getVarUnit();

    // remember which users' saved variables shouldn't be hidden
    QVector<QString> unhideSavedVars;
    for (const auto& variable : qAsConst(varUnit->savedVars)) {
        if (!varUnit->isHidden(variable)) {
            unhideSavedVars.append(variable);
        }
    }

    // mark all currently loaded Lua variables as hidden
    // this covers entirety of Mudlets API (good!) and but unfortunately
    // user's saved variables as well
    LuaInterface* lI = pHost->getLuaInterface();
    lI->getVars(true);

    // unhide user's saved variables
    for (const auto& variable : qAsConst(unhideSavedVars)) {
        varUnit->removeHidden(variable);
    }
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
    hideMudletsVariables(pHost);

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
    // There may be a corner case if a replay is running AND the application is
    // closing down AND the updater on a particular platform pauses the
    // application destruction...?
    delete (mpTimerReplay);
    mpTimerReplay = nullptr;

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

// Called from the ctelnet instance for the host concerned:
bool mudlet::replayStart()
{
    // Do not proceed if there is a problem with the main toolbar (it isn't there)
    // OR if there is already a replay toolbar in existance (a replay is already
    // in progress)...
    if (!mpMainToolBar || mpToolBarReplay) {
        return false;
    }

    // Lock the replay button and menu item down until the replay is over
    mpActionReplay->setCheckable(true);
    mpActionReplay->setChecked(true);
    mpActionReplay->setEnabled(false);
    dactionReplay->setEnabled(false);
    mpActionReplay->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                               .arg(tr("<p>Cannot load a replay as one is already in progress in this or another profile.</p>")));
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

    mpActionReplaySpeedUp = new QAction(QIcon(QStringLiteral(":/icons/export.png")), tr("Faster"), this);
    mpActionReplaySpeedUp->setObjectName(QStringLiteral("replay_speed_up_action"));
    mpActionReplaySpeedUp->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                      .arg(tr("<p>Replay each step with a shorter time interval between steps.</p>")));
    mpToolBarReplay->addAction(mpActionReplaySpeedUp);
    mpToolBarReplay->widgetForAction(mpActionReplaySpeedUp)->setObjectName(mpActionReplaySpeedUp->objectName());

    mpActionReplaySpeedDown = new QAction(QIcon(QStringLiteral(":/icons/import.png")), tr("Slower"), this);
    mpActionReplaySpeedDown->setObjectName(QStringLiteral("replay_speed_down_action"));
    mpActionReplaySpeedDown->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                        .arg(tr("<p>Replay each step with a longer time interval between steps.</p>")));
    mpToolBarReplay->addAction(mpActionReplaySpeedDown);
    mpToolBarReplay->widgetForAction(mpActionReplaySpeedDown)->setObjectName(mpActionReplaySpeedDown->objectName());

    mpLabelReplaySpeedDisplay = new QLabel(this);
    mpActionSpeedDisplay = mpToolBarReplay->addWidget(mpLabelReplaySpeedDisplay);

    connect(mpActionReplaySpeedUp.data(), &QAction::triggered, this, &mudlet::slot_replaySpeedUp);
    connect(mpActionReplaySpeedDown.data(), &QAction::triggered, this, &mudlet::slot_replaySpeedDown);

    mpLabelReplaySpeedDisplay->setText(QStringLiteral("<font size=25><b>%1</b></font>").arg(tr("Speed: X%1").arg(mReplaySpeed)));

    mpTimerReplay = new QTimer(this);
    mpTimerReplay->setInterval(1000);
    mpTimerReplay->setSingleShot(false);
    connect(mpTimerReplay.data(), &QTimer::timeout, this, &mudlet::slot_replayTimeChanged);

    mpLabelReplayTime->setText(QStringLiteral("<font size=25><b>%1</b></font>").arg(tr("Time: %1").arg(mReplayTime.toString(mTimeFormat))));

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
        mpLabelReplayTime->setText(QStringLiteral("<font size=25><b>%1</b></font>")
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
    mpActionReplay->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>").arg(tr("<p>Load a Mudlet replay.</p>")));
    dactionReplay->setToolTip(mpActionReplay->toolTip());
}

void mudlet::slot_replaySpeedUp()
{
    if (mpLabelReplaySpeedDisplay) {
        mReplaySpeed = mReplaySpeed * 2;
        mpLabelReplaySpeedDisplay->setText(QStringLiteral("<font size=25><b>%1</b></font>").arg(tr("Speed: X%1").arg(mReplaySpeed)));

        mpLabelReplaySpeedDisplay->show();
    }
}

void mudlet::slot_replaySpeedDown()
{
    if (mpLabelReplaySpeedDisplay) {
        mReplaySpeed = mReplaySpeed / 2;
        if (mReplaySpeed < 1) {
            mReplaySpeed = 1;
        }
        mpLabelReplaySpeedDisplay->setText(QStringLiteral("<font size=25><b>%1</b></font>").arg(tr("Speed: X%1").arg(mReplaySpeed)));
        mpLabelReplaySpeedDisplay->show();
    }
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

//loads the luaFunctionList for use by the edbee Autocompleter
bool mudlet::loadLuaFunctionList()
{
    QFile* jsonFile = new QFile(QStringLiteral(":/lua-function-list.json"));
    if (!jsonFile->open(QFile::ReadOnly)) {
        return false;
    }

    const QByteArray data = jsonFile->readAll();

    jsonFile->close();

    auto json_doc = QJsonDocument::fromJson(data);

    if (json_doc.isNull() || !json_doc.isObject()) {
        return false;
    }

    QJsonObject json_obj = json_doc.object();

    if (json_obj.isEmpty()) {
        return false;
    }

    mudlet::mLuaFunctionNames = json_obj.toVariantHash();

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
        // an edbee editor widget theme:
        return QStringLiteral("%1/.config/mudlet/edbee/Colorsublime-Themes-master/themes.json").arg(QDir::homePath());
    case moduleBackupsPath:
        // Returns the directory used to store module backups that is used in
        // when saving/resyncing packages/modules - ends in a '/'
        return QStringLiteral("%1/.config/mudlet/moduleBackups/").arg(QDir::homePath());
    case qtTranslationsPath:
        return QLibraryInfo::location(QLibraryInfo::TranslationsPath);
    }
    Q_UNREACHABLE();
    return QString();
}

#if defined(INCLUDE_UPDATER)
void mudlet::checkUpdatesOnStart()
{
    if (!scmIsDevelopmentVersion) {
        // Only try and create an updater (which checks for updates online) if
        // this is a release version:
        updater->checkUpdatesOnStart();
    }
}

void mudlet::slot_check_manual_update()
{
    updater->manuallyCheckUpdates();
}

// Means to turn-off the hard coded popup delay in QActions provided by:
// https://stackoverflow.com/a/30126063/4805858
void mudlet::slot_updateAvailable(const int updateCount)
{
    // Removes the normal click to activate "About Mudlet" action and move it
    // to a new menu which also contains a "goto updater" option

    // First change the existing button:
    if (!qApp->testAttribute(Qt::AA_DontShowIconsInMenus)) {
        mpActionAbout->setIcon(QIcon(":/icons/mudlet_information.png"));
    } else {
        mpActionAbout->setIcon(QIcon());
    }
    mpActionAbout->setToolTip(tr("<p>Inform yourself about this version of Mudlet, the people who made it and the licence under which you can share it.</p>",
                                 // Intentional comment
                                 "Tooltip for About Mudlet sub-menu item and main toolbar button (or menu item if an update has changed that control to have a popup menu instead) (Used in 3 places - "
                                 "please ensure all have the same translation)."));

    // Create a new button (QActions actually turn into QToolButtons when they
    // are placed on a QToolBar - but we need to generate one ourselves so we
    // can modify the popupMode away from the delayed one that is hardwared into
    // the transmuted QAction-to-QToolButton ones):
    mpButtonAbout = new QToolButton();
    mpButtonAbout->setIcon(QIcon(QStringLiteral(":/icons/mudlet_important.png")));
    mpButtonAbout->setToolTip(tr("<p>About Mudlet</p>"
                                 "<p><i>%n update(s) is/are now available!</i><p>",
                                 // Intentional comment
                                 "This is the tooltip text for the 'About' Mudlet main toolbar button when it has been changed by adding a menu which now contains the original 'About Mudlet' action "
                                 "and a new one to access the manual update process",
                                 updateCount));
    mpButtonAbout->setText(tr("About"));
    mpButtonAbout->setPopupMode(QToolButton::InstantPopup);
    // Now insert our new button after the current QAction/QToolButton
    mpMainToolBar->insertWidget(mpActionAbout, mpButtonAbout);
    // And quickly pull out the old QAction/QToolButton:
    mpMainToolBar->removeAction(mpActionAbout);

    // Create the new menu
    QMenu* pUpdateMenu = new QMenu();
    // Stuff the QAction/QToolButton we just pulled into the new menu
    pUpdateMenu->insertAction(nullptr, mpActionAbout);
    // We can then add in the new item to give access the update(s)
    auto pActionReview = pUpdateMenu->addAction(tr("Review %n update(s)...",
                                                   // Intentional comment
                                                   "Review update(s) menu item, %n is the the count of how many updates are available",
                                                   updateCount),
                                                this, &mudlet::slot_check_manual_update);
    pActionReview->setToolTip(tr("<p>Review the update(s) available...</p>",
                                 // Intentional comment
                                 "Tool-tip for review update(s) menu item, given that the count of how many updates are available is already shown in the menu, the %n parameter that is that number need not be used here",
                                 updateCount));
    // Override the default hide tooltips state:
    pUpdateMenu->setToolTipsVisible(true);
    // Screw the menu onto the button
    mpButtonAbout->setMenu(pUpdateMenu);
    // And force the button to adopt the current style of the existing buttons
    // - it doesn't by default:
    mpButtonAbout->setToolButtonStyle(mpMainToolBar->toolButtonStyle());
}

void mudlet::slot_update_installed()
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
    if (!pHost || !pHost->mpConsole) {
        return -1;
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
    if (pC) {
        return pC->mUpperPane->getColumnCount();
    } else {
        TDebug(QColor(Qt::white), QColor(Qt::red)) << "ERROR: window doesn't exist\n" >> 0;
        return -1;
    }
}

int mudlet::getRowCount(Host* pHost, QString& name)
{
    if (!pHost || !pHost->mpConsole) {
        return -1;
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(name);
    if (pC) {
        return pC->mUpperPane->getRowCount();
    } else {
        TDebug(QColor(Qt::white), QColor(Qt::red)) << "ERROR: window doesn't exist\n" >> 0;
        return -1;
    }
}

// Can be called from lua sub-system OR from slot_replay(), the presence of a
// non-NULLPTR pErrMsg indicates the former; also the replayFileName CAN be
// relative (to the profiles ./log sub-directory where replays are stored) if
// sourced from the lua sub-system.
bool mudlet::loadReplay(Host* pHost, const QString& replayFileName, QString* pErrMsg)
{
    // Do not proceed if there is a problem with the main toolbar (it isn't there)
    // OR if there is already a replay toolbar in existance (a replay is already
    // in progress)...
    if (!mpMainToolBar || mpToolBarReplay) {
        // This was in (bool) ctelnet::loadReplay(const QString&, QString*)
        // but is needed here to prevent getting into there otherwise a lua call
        // to start a replay would mess up (QFile) ctelnet::replayFile for a
        // replay already in progess in the SAME profile.  Technically there
        // could be a very small chance of a race condition if a lua call of
        // loadRawFile happens at the same time as a file was selected for a
        // replay after the toolbar/menu command to do a reaply for the same
        // profile - but the window for this is likely to be a fraction of a
        // second...
        if (pErrMsg) {
            *pErrMsg = QStringLiteral("cannot perform replay, another one seems to already be in progress; try again when it has finished.");
        } else {
            pHost->postMessage(tr("[ WARN ]  - Cannot perform replay, another one may already be in progress,\n"
                                  "try again when it has finished."));
        }
        return false;
    }

    QString absoluteReplayFileName;
    if (QFileInfo(replayFileName).isRelative()) {
        absoluteReplayFileName = QStringLiteral("%1/%2").arg(mudlet::getMudletPath(mudlet::profileReplayAndLogFilesPath, pHost->getName()), replayFileName);
    } else {
        absoluteReplayFileName = replayFileName;
    }

    return pHost->mTelnet.loadReplay(absoluteReplayFileName, pErrMsg);
}

void mudlet::slot_newDataOnHost(const QString& hostName, const bool isLowerPriorityChange)
{
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
        QFile file_use_smallscreen(mudlet::getMudletPath(mudlet::mainDataItemPath, QStringLiteral("mudlet_option_use_smallscreen")));
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

void mudlet::setShowMapAuditErrors(const bool state)
{
    if (mshowMapAuditErrors != state) {
        mshowMapAuditErrors = state;

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

void mudlet::setInterfaceLanguage(const QString& languageCode)
{
    mInterfaceLanguage = languageCode;
}

bool mudlet::setClickthrough(Host* pHost, const QString& name, bool clickthrough)
{
    if (!pHost || !pHost->mpConsole) {
        return false;
    }

    auto pL = pHost->mpConsole->mLabelMap.value(name);
    if (pL) {
        pL->setClickThrough(clickthrough);
        return true;
    }

    return false;
}

QPair<bool, QStringList> mudlet::getLines(Host* pHost, const QString& windowName, const int lineFrom, const int lineTo)
{
    if (!pHost || !pHost->mpConsole) {
        QStringList failMessage;
        failMessage << QStringLiteral("internal error: the Host class pointer or its pointer to the main TConsole was a nullptr - please report").arg(windowName);
        return qMakePair(false, failMessage);
    }

    if (windowName.isEmpty() || windowName == QLatin1String("main")) {
        return qMakePair(true, pHost->mpConsole->getLines(lineFrom, lineTo));
    }

    auto pC = pHost->mpConsole->mSubConsoleMap.value(windowName);
    if (pC) {
        return qMakePair(true, pC->getLines(lineFrom, lineTo));
    } else {
        QStringList failMessage;
        failMessage << QStringLiteral("mini console, user window or buffer \"%1\" not found").arg(windowName);
        return qMakePair(false, failMessage);
    }
}
