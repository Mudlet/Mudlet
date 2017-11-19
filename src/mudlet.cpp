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

#include "edbee/edbee.h"
#include "edbee/models/textgrammar.h"
#include "edbee/texteditorwidget.h"
#include "edbee/views/texttheme.h"

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
QMainWindow* mudlet::mpDebugArea = nullptr;
bool mudlet::debugMode = false;

QPointer<mudlet> mudlet::_self = nullptr;

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
, mWindowMinimized(false)
, mReplaySpeed(1)
, mpCurrentActiveHost(nullptr)
, mIsGoingDown(false)
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
, mpActionFullScreenView(nullptr)
, moduleTable(nullptr)
, mshowMapAuditErrors(false)
, mCompactInputLine(false)
, mpAboutDlg(nullptr)
, mpModuleDlg(nullptr)
, mpPackageManagerDlg(nullptr)
, mpProfilePreferencesDlg(nullptr)
, mGuiLanguageSelection(QStringLiteral("none"))
, mTimeFormat(tr("hh:mm:ss",
                 "Formatting string for elapsed time display in replay playback - see QDateTime::toString(const QString&) for the gory details...!"))
{

    /*In case sensitive environments, two different config directories
       were used: "Mudlet" for QSettings, and "mudlet" anywhere else.
       Furthermore, we skip the version from the application name to follow the convention.
       For compatibility with older settings, if no config is loaded
       from the config directory "mudlet", application "Mudlet", we try to load from the config
       directory "Mudlet", application "Mudlet 1.0". */
    QString mudlet(QStringLiteral("mudlet"));
    QString Mudlet(QStringLiteral("Mudlet"));
    QSettings settings_new(mudlet, Mudlet);
    QSettings settings((settings_new.contains("pos") ? mudlet : Mudlet), (settings_new.contains("pos") ? Mudlet : QStringLiteral("Mudlet 1.0")));

    // Move this as near to start of the constructor as possible as we ideally
    // need the mGuiLanguageSelection member to select the right translation
    // BEFORE we draw the GUI...!
    readSettings(settings);

    QString translationFilesPath(getMudletPath(mudletTranslationsPath));
    QString qtTranslationsFilePath(getMudletPath(qtTranslationsPath));
    QDir translationDir(translationFilesPath);
    QStringList translationFilesFilters;
    translationFilesFilters << QStringLiteral("mudlet_*.qm");
    translationDir.setNameFilters(translationFilesFilters);
    QStringList translationFilesList(translationDir.entryList(QDir::Files | QDir::Readable, QDir::Name));
    QStringListIterator itTranslation(translationFilesList);
    qDebug() << "mudlet::mudlet() INFO - Seeking Qt library translations files in:" << qtTranslationsFilePath;
    qDebug() << "mudlet::mudlet() INFO - Seeking Mudlet library translations files in:" << translationFilesPath;
    while (itTranslation.hasNext()) {
        QString translationFileName = itTranslation.next();
        QString languageCode(translationFileName);
        QList<QPointer<QTranslator>> pNewLanguageTranslatorsList;
        languageCode.remove(QStringLiteral("mudlet_"), Qt::CaseInsensitive);
        languageCode.remove(QStringLiteral(".qm"), Qt::CaseInsensitive);
        QPointer<QTranslator> pMudletTranslator = new QTranslator();
        if (!pMudletTranslator->load(translationFileName, translationFilesPath)) {
            qDebug() << "mudlet::mudlet() Failed to load translation file" << translationFileName << "from" << translationFilesPath << "a translation to the specified language will NOT be available!";
        } else {
            pNewLanguageTranslatorsList.append(pMudletTranslator);

            QPointer<QTranslator> pQtTranslator = new QTranslator();
            QString temp = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
            if (pQtTranslator->load(QStringLiteral("qt_%1").arg(languageCode),
                                   QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {

                pNewLanguageTranslatorsList.append(pQtTranslator);
                qDebug() << "mudlet::mudlet() INFO - found QT translations for language code:" << languageCode << "in" << temp;
            } else {
                qDebug() << "mudlet::mudlet() WARN - QT translations for language code:" << languageCode << "not found in" << temp;
            }

            mTranslatorsMap[languageCode] = pNewLanguageTranslatorsList;
            qDebug() << "mudlet::mudlet() INFO - creating MUDLET translator for language code:" << languageCode << "using translations from:" << translationFileName;
        }

        // Is this the language code we are looking for
        if (!languageCode.compare(mGuiLanguageSelection, Qt::CaseInsensitive) && ! pNewLanguageTranslatorsList.isEmpty()) {
            QListIterator<QPointer<QTranslator>> itTranslator(pNewLanguageTranslatorsList);
            while (itTranslator.hasNext()) {
                QPointer<QTranslator> pTranslator = itTranslator.next();
                // The second condition will generate a QEvent::LanguageChange...
                if (! pTranslator || !qApp->installTranslator(pTranslator)) {
                    qDebug() << "mudlet::mudlet() ERROR - Failed to directly install (load) a translator for:" << languageCode << "a translation to the specified language will NOT be available!";
                } else {
                    mTranslatorsLoadedList.append(pTranslator);
                }
            }
        }
    }

    setupUi(this);
    setUnifiedTitleAndToolBarOnMac(true);
    setContentsMargins(0, 0, 0, 0);
    mudlet::debugMode = false;
    setAttribute(Qt::WA_DeleteOnClose);

    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    setWindowIcon(QIcon(QStringLiteral(":/icons/mudlet_main_48px.png")));
    mpMainToolBar = new QToolBar(this);
    mpMainToolBar->setObjectName(QStringLiteral("mpMainToolBar"));

    // This needs to be called AFTER we create mpMainToolBar as it adjusts some
    // of the settings for it...
    updateAfterReadingSettings(settings);

    version = tr("Mudlet-%1%2%3", "I suppose it is, theoretically, possible that \"Mudlet\" might need to be retitled in a different language, but it is not likely IMHO!  The arguments that followed are: version \"number\" in form #.#.#, a \'-\' separator if needed if the third, the \"build\" value is non-empty.")
            .arg(QStringLiteral(APP_VERSION).trimmed() )
            .arg(QStringLiteral(APP_BUILD).trimmed().length() ? QStringLiteral("-") : QString())
            .arg(QStringLiteral(APP_BUILD).trimmed());

    setWindowTitle(version);

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

    mAutolog = QFile(getMudletPath(mainDataItemPath, QStringLiteral("autolog"))).exists();

    bool isSmallScreenMode = QFile(getMudletPath(mainDataItemPath, QStringLiteral("mudlet_option_use_smallscreen"))).exists();
    if (isSmallScreenMode) {
        mpMainToolBar->setIconSize(QSize(16, 16));
    } else {
        mpMainToolBar->setIconSize(QSize(32, 32));
        mpMainToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    }

    // add name to the action's widget in the toolbar, which doesn't have one by default
    // see https://stackoverflow.com/a/32460562/72944

    // Enable tool-tips on each of the main menu items - they are not by default
    // (because the default tool-tip is exactly the same as the item "text")
    menuEditor->setToolTipsVisible(true);
    menuGames->setToolTipsVisible(true);
    menuOptions->setToolTipsVisible(true);
    menuHelp->setToolTipsVisible(true);

    mpActionConnect = new QAction(this);
    mpActionConnect->setIcon(QIcon(QStringLiteral(":/icons/preferences-web-browser-cache.png")));
    mpActionConnect->setObjectName(QStringLiteral("connect_action"));
    mpMainToolBar->addAction(mpActionConnect);
    mpMainToolBar->widgetForAction(mpActionConnect)->setObjectName(mpActionConnect->objectName());

    mpActionTriggers = new QAction(this);
    mpActionTriggers->setIcon(QIcon(QStringLiteral(":/icons/tools-wizard.png")));
    mpActionTriggers->setObjectName(QStringLiteral("triggers_action"));
    mpMainToolBar->addAction(mpActionTriggers);
    mpMainToolBar->widgetForAction(mpActionTriggers)->setObjectName(mpActionTriggers->objectName());

    mpActionAliases = new QAction(this);
    mpActionAliases->setIcon(QIcon(QStringLiteral(":/icons/system-users.png")));
    mpActionAliases->setObjectName(QStringLiteral("aliases_action"));
    mpMainToolBar->addAction(mpActionAliases);
    mpMainToolBar->widgetForAction(mpActionAliases)->setObjectName(mpActionAliases->objectName());

    mpActionTimers = new QAction(this);
    mpActionTimers->setIcon(QIcon(QStringLiteral(":/icons/chronometer.png")));
    mpActionTimers->setObjectName(QStringLiteral("timers_action"));
    mpMainToolBar->addAction(mpActionTimers);
    mpMainToolBar->widgetForAction(mpActionTimers)->setObjectName(mpActionTimers->objectName());

    mpActionButtons = new QAction(this);
    mpActionButtons->setIcon(QIcon(QStringLiteral(":/icons/bookmarks.png")));
    mpActionButtons->setObjectName(QStringLiteral("buttons_action"));
    mpMainToolBar->addAction(mpActionButtons);
    mpMainToolBar->widgetForAction(mpActionButtons)->setObjectName(mpActionButtons->objectName());

    mpActionScripts = new QAction(this);
    mpActionScripts->setIcon(QIcon(QStringLiteral(":/icons/document-properties.png")));
    mpActionScripts->setObjectName(QStringLiteral("scripts_action"));
    mpMainToolBar->addAction(mpActionScripts);
    mpMainToolBar->widgetForAction(mpActionScripts)->setObjectName(mpActionScripts->objectName());

    mpActionKeys = new QAction(this);
    mpActionKeys->setIcon(QIcon(QStringLiteral(":/icons/preferences-desktop-keyboard.png")));
    mpActionKeys->setObjectName(QStringLiteral("keys_action"));
    mpMainToolBar->addAction(mpActionKeys);
    mpMainToolBar->widgetForAction(mpActionKeys)->setObjectName(mpActionKeys->objectName());

    mpActionVariables = new QAction(this);
    mpActionVariables->setIcon(QIcon(QStringLiteral(":/icons/variables.png")));
    mpMainToolBar->addAction(mpActionVariables);
    mpActionVariables->setObjectName(QStringLiteral("variables_action"));
    mpMainToolBar->widgetForAction(mpActionVariables)->setObjectName(mpActionVariables->objectName());

    mpActionIRC = new QAction(this);
    mpActionIRC->setIcon(QIcon(QStringLiteral(":/icons/internet-telephony.png")));
    mpActionIRC->setObjectName(QStringLiteral("irc_action"));
    mpMainToolBar->addAction(mpActionIRC);
    mpMainToolBar->widgetForAction(mpActionIRC)->setObjectName(mpActionIRC->objectName());

    mpActionMapper = new QAction(this);
    mpActionMapper->setIcon(QIcon(QStringLiteral(":/icons/applications-internet.png")));
    mpActionMapper->setObjectName(QStringLiteral("map_action"));
    mpMainToolBar->addAction(mpActionMapper);
    mpMainToolBar->widgetForAction(mpActionMapper)->setObjectName(mpActionMapper->objectName());

    mpActionHelp = new QAction(this);
    mpActionHelp->setIcon(QIcon(QStringLiteral(":/icons/help-hint.png")));
    mpActionHelp->setObjectName(QStringLiteral("manual_action"));
    mpMainToolBar->addAction(mpActionHelp);
    mpMainToolBar->widgetForAction(mpActionHelp)->setObjectName(mpActionHelp->objectName());

    mpActionOptions = new QAction(this);
    mpActionOptions->setIcon(QIcon(QStringLiteral(":/icons/configure.png")));
    mpActionOptions->setObjectName(QStringLiteral("settings_action"));
    mpMainToolBar->addAction(mpActionOptions);
    mpMainToolBar->widgetForAction(mpActionOptions)->setObjectName(mpActionOptions->objectName());

    mpActionNotes = new QAction(this);
    mpActionNotes->setIcon(QIcon(QStringLiteral(":/icons/applications-accessories.png")));
    mpActionNotes->setObjectName(QStringLiteral("notepad_action"));
    mpMainToolBar->addAction(mpActionNotes);
    mpMainToolBar->widgetForAction(mpActionNotes)->setObjectName(mpActionNotes->objectName());

    mpActionPackageManager = new QAction(this);
    mpActionPackageManager->setIcon(QIcon(QStringLiteral(":/icons/package-manager.png")));
    mpActionPackageManager->setObjectName(QStringLiteral("package_action"));
    mpMainToolBar->addAction(mpActionPackageManager);
    mpMainToolBar->widgetForAction(mpActionPackageManager)->setObjectName(mpActionPackageManager->objectName());

    mpActionModuleManager = new QAction(this);
    mpActionModuleManager->setIcon(QIcon(QStringLiteral(":/icons/module-manager.png")));
    mpActionModuleManager->setObjectName(QStringLiteral("module_action"));
    mpMainToolBar->addAction(mpActionModuleManager);
    mpMainToolBar->widgetForAction(mpActionModuleManager)->setObjectName(mpActionModuleManager->objectName());

    mpActionReplay = new QAction(this);
    mpActionReplay->setIcon(QIcon(QStringLiteral(":/icons/media-optical.png")));
    mpActionReplay->setObjectName(QStringLiteral("replay_action"));
    mpMainToolBar->addAction(mpActionReplay);
    mpMainToolBar->widgetForAction(mpActionReplay)->setObjectName(mpActionReplay->objectName());

    mpActionDisconnect = new QAction(this);
    mpActionDisconnect->setIcon(QIcon(QStringLiteral(":/icons/network-disconnect.png")));
    mpActionDisconnect->setObjectName(QStringLiteral("disconnect_action"));
    mpMainToolBar->addAction(mpActionDisconnect);
    mpMainToolBar->widgetForAction(mpActionDisconnect)->setObjectName(mpActionDisconnect->objectName());

    mpActionReconnect = new QAction(this);
    mpActionReconnect->setIcon(QIcon(QStringLiteral(":/icons/system-restart.png")));
    mpActionReconnect->setObjectName(QStringLiteral("reconnect_action"));
    mpMainToolBar->addAction(mpActionReconnect);
    mpMainToolBar->widgetForAction(mpActionReconnect)->setObjectName(mpActionReconnect->objectName());

    mpActionMultiView = new QAction(this);
    mpActionMultiView->setIcon(QIcon(QStringLiteral(":/icons/view-split-left-right.png")));
    mpActionMultiView->setObjectName(QStringLiteral("multiview_action"));
    mpMainToolBar->addAction(mpActionMultiView);
    mpMainToolBar->widgetForAction(mpActionMultiView)->setObjectName(mpActionMultiView->objectName());

    mpActionAbout = new QAction(this);
    mpActionAbout->setIcon(QIcon(QStringLiteral(":/icons/mudlet_information.png")));
    mpActionAbout->setObjectName(QStringLiteral("about_action"));
    mpMainToolBar->addAction(mpActionAbout);
    mpMainToolBar->widgetForAction(mpActionAbout)->setObjectName(mpActionAbout->objectName());

    disableToolbarButtons();

    mpDebugArea = new QMainWindow(nullptr);
    mHostManager.addHost("default_host", "", "", "");
    mpDefaultHost = mHostManager.getHost(QString("default_host"));
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
    QFont mainFont(QStringLiteral("Bitstream Vera Sans Mono"), 8, QFont::Normal);
    if (isSmallScreenMode) {
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
    connect(mpActionConnect, SIGNAL(triggered()), this, SLOT(slot_show_connection_dialog()));
    connect(mpActionHelp, SIGNAL(triggered()), this, SLOT(show_help_dialog()));
    connect(mpActionTriggers, SIGNAL(triggered()), this, SLOT(show_trigger_dialog()));
    connect(mpActionTimers, SIGNAL(triggered()), this, SLOT(show_timer_dialog()));
    connect(mpActionAliases, SIGNAL(triggered()), this, SLOT(show_alias_dialog()));
    connect(mpActionScripts, SIGNAL(triggered()), this, SLOT(show_script_dialog()));
    connect(mpActionKeys, SIGNAL(triggered()), this, SLOT(show_key_dialog()));
    connect(mpActionVariables, SIGNAL(triggered()), this, SLOT(show_variable_dialog()));
    connect(mpActionButtons, SIGNAL(triggered()), this, SLOT(show_action_dialog()));
    connect(mpActionOptions, SIGNAL(triggered()), this, SLOT(show_options_dialog()));
    connect(mpActionAbout, SIGNAL(triggered()), this, SLOT(slot_show_about_dialog()));
    connect(mpActionMultiView, SIGNAL(triggered()), this, SLOT(slot_multi_view()));
    connect(mpActionReconnect, SIGNAL(triggered()), this, SLOT(slot_reconnect()));
    connect(mpActionReplay, SIGNAL(triggered()), this, SLOT(slot_replay()));
    connect(mpActionNotes, SIGNAL(triggered()), this, SLOT(slot_notes()));
    connect(mpActionMapper, SIGNAL(triggered()), this, SLOT(slot_mapper()));
    connect(mpActionIRC, SIGNAL(triggered()), this, SLOT(slot_irc()));
    connect(mpActionPackageManager, SIGNAL(triggered()), this, SLOT(slot_package_manager()));
    connect(mpActionModuleManager, SIGNAL(triggered()), this, SLOT(slot_module_manager()));


// There were QActions here which had a 'm' prefix but were NOT made members of
// or had pointers to saved as part of this class - which were connected up to
// various slots - BUT THEY WERE NEVER PLACED ANYWHERE WHERE THEY WOULD BE USED
// (on a toolbar, on a dialog etc.) - so I have removed them - SlySven

// There are QActions on the main_window.ui which had a 'd' {for dialog
// prefix?} I changed that prefix to "menu" - Slysven
// Also reordered to match actual menu structure
// Games menu:
    connect(menuActionConnect, SIGNAL(triggered()), this, SLOT(slot_show_connection_dialog()));
    // Separator
    connect(menuActionDisconnect, SIGNAL(triggered()), this, SLOT(slot_disconnect()));
    connect(menuActionReconnect, SIGNAL(triggered()), this, SLOT(slot_reconnect()));
    // Separator
    connect(menuActionQuit, SIGNAL(triggered()), this, SLOT(close()));

// Toolbox menu:
    connect(menuActionTriggers, SIGNAL(triggered()), this, SLOT(show_trigger_dialog()));
    connect(menuActionTimers, SIGNAL(triggered()), this, SLOT(show_timer_dialog()));
    connect(menuActionAliases, SIGNAL(triggered()), this, SLOT(show_alias_dialog()));
    connect(menuActionScripts, SIGNAL(triggered()), this, SLOT(show_script_dialog()));
    connect(menuActionKeys, SIGNAL(triggered()), this, SLOT(show_key_dialog()));
    connect(menuActionButtons, SIGNAL(triggered()), this, SLOT(show_action_dialog()));
    // Separator
    connect(menuActionShowMap, SIGNAL(triggered()), this, SLOT(slot_mapper()));
    connect(menuActionNotepad, SIGNAL(triggered()), this, SLOT(slot_notes()));
    // Separator
    connect(menuActionPackageManager, SIGNAL(triggered()), this, SLOT(slot_package_manager()));
    connect(menuActionReplay, SIGNAL(triggered()), this, SLOT(slot_replay()));
    connect(menuActionModuleManager, SIGNAL(triggered()), this, SLOT(slot_module_manager()));
    connect(menuActionPackageExporter, SIGNAL(triggered()), this, SLOT(slot_package_exporter()));
    // Separator
    connect(menuActionOptions, SIGNAL(triggered()), this, SLOT(show_options_dialog()));
    connect(menuActionMultiView, SIGNAL(triggered()), this, SLOT(slot_multi_view()));
    connect(menuActionInputLine, &QAction::triggered, this, &mudlet::slot_toggle_compact_input_line);

    // Help (and now About) menu
    connect(menuActionButtons, SIGNAL(triggered()), this, SLOT(show_help_dialog()));
    connect(menuActionForum, SIGNAL(triggered()), this, SLOT(slot_show_help_dialog_forum()));
    connect(menuActionVideo, SIGNAL(triggered()), this, SLOT(slot_show_help_dialog_video()));
    connect(menuActionIrcBuiltIn, SIGNAL(triggered()), this, SLOT(slot_irc()));
    connect(menuActionIrcWeb, SIGNAL(triggered()), this, SLOT(slot_show_help_dialog_irc()));

    connect(menuActionDownload, SIGNAL(triggered()), this, SLOT(slot_show_help_dialog_download()));

    connect(menuActionAbout, SIGNAL(triggered()), this, SLOT(slot_show_about_dialog()));
    // The aboutQt() slot is provided by QApplication and is often placed on the
    // help menu for a Qt application - it is treated specially as an entry on a
    // QMainMenu on the macOs platform...!
    connect(menuActionAboutQt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));


#ifdef QT_GAMEPAD_LIB
    //connect(QGamepadManager::instance(), &QGamepadManager::gamepadButtonPressEvent, this, slot_gamepadButtonPress);
    //connect(QGamepadManager::instance(), &QGamepadManager::gamepadButtonReleaseEvent, this, slot_gamepadButtonRelease);
    //connect(QGamepadManager::instance(), &QGamepadManager::gamepadConnected, this, slot_gamepadConnected);
    //connect(QGamepadManager::instance(), &QGamepadManager::gamepadDisconnected, this, slot_gamepadDisconnected);
    //connect(QGamepadManager::instance(), &QGamepadManager::gamepadAxisEvent, this, slot_gamepadAxisEvent);
    connect(QGamepadManager::instance(), SIGNAL(gamepadButtonPressEvent(int, QGamepadManager::GamepadButton, double)), this, SLOT(slot_gamepadButtonPress(int, QGamepadManager::GamepadButton, double)));
    connect(QGamepadManager::instance(), SIGNAL(gamepadButtonReleaseEvent(int, QGamepadManager::GamepadButton)), this, SLOT(slot_gamepadButtonRelease(int, QGamepadManager::GamepadButton)));
    connect(QGamepadManager::instance(), SIGNAL(gamepadAxisEvent(int, QGamepadManager::GamepadAxis, double)), this, SLOT(slot_gamepadAxisEvent(int, QGamepadManager::GamepadAxis, double)));
    connect(QGamepadManager::instance(), SIGNAL(gamepadConnected(int)), this, SLOT(slot_gamepadConnected(int)));
    connect(QGamepadManager::instance(), SIGNAL(gamepadDisconnected(int)), this, SLOT(slot_gamepadDisconnected(int)));
#endif

    // Edbee has a singleton that needs some initialisation
    initEdbee();

    guiLanguageChange();
}

// Unlike other classes this is NOT a SLOT - it is called directly from within
// this class:
void mudlet::guiLanguageChange()
{
    // Translate the automatically generated form/dialog stuff
    retranslateUi(this);

    mTimeFormat = tr("hh:mm:ss",
                     "Formatting string for elapsed time display in replay playback - see QDateTime::toString(const QString&) for the gory details...!");

    mpActionConnect->setText(tr("Connect", "Is there an Alt+C shortcut on the main menu"));
    mpActionConnect->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                .arg(tr("<p>Configure the connection if necessary, and then connect to a MUD Server; includes some preconfigured examples to get you started!</p>")));
    menuActionConnect->setToolTip(mpActionConnect->toolTip());

    mpActionTriggers->setText(tr("Triggers", "Is there an Alt+E shortcut on the main menu"));
    mpActionTriggers->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                 .arg(tr("<p>Opens the Editor on the view of triggers which are items that can respond to incoming MUD server content.</p>")));
    menuActionTriggers->setToolTip(mpActionTriggers->toolTip());

    mpActionAliases->setText(tr("Aliases"));
    mpActionAliases->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                .arg(tr("<p>Opens the Editor on the view of aliases which are items that can respond to what you type <i><b>(when you then press "
                                        "<key>enter</key> on the command line to send)</b></i>, possibly modifying or removing anything that would be sent out "
                                        "to the MUD server or performing something locally first.</p>")));
    menuActionAliases->setToolTip(mpActionAliases->toolTip());

    mpActionTimers->setText(tr("Timers"));
    mpActionTimers->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                               .arg(tr("<p>Opens the Editor on the view of timers which are items that can perform a task to do something locally or to send "
                                       "something to the MUD Server on a regular basis; by default they will do this repeatedly but it is quite possible for "
                                       "a timer to disable <i>itself</i> when it fires so as to provide a delay if started from another item before whatever "
                                       "else it is designed to do actually happends.  To this end there is a special mode that can be achieved using multiple "
                                       "timer items so that all but the last one in a chain become what is called <i>offset</i> timers.  Read more about this "
                                       "subject in the documentation provided as a <b>Wiki</b>.</p>")));
    menuActionTimers->setToolTip(mpActionTimers->toolTip());

    mpActionButtons->setText(tr("Buttons"));
    mpActionButtons->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                .arg(tr("<p>Opens the Editor on the view of buttons (and menus and toolbars that contain them) which are graphical user interface items that can be clicked upon with a mouse to "
                                        "perform a task to do something locally or to send something to the MUD Server.</p><p>A button by default performs this action every time it is clicked upon but it can "
                                        "also be configured so that it becomes a toggle so that one click switches it <i>on</i>(down) and a second one is needed to switch it <i>off</i>(up) again.  Several "
                                        "<i>buttons</i> can be combined into the more complact form of a <i>menu</i> which apears as a single button but displays all the buttons it contains when clicked upon; "
                                        "buttons and menus can both then be contained in a <i>toolbar</i> which can either be a simple single line of them around the left/top/right of the main screen area or as "
                                        "a grid with them divided up into a specified number of rows or columns (depending on whether it is laid out horizonatlly or vertically) in a separate widget that can "
                                        "either be docked as the user wants around the edge of the main window (with other such toolbars and other text windows and even the map display for the profile) or can "
                                        "be <i>floated</i> freely anywhere onto the whole desktop...</p>")));
    menuActionButtons->setToolTip(mpActionButtons->toolTip());

    // TODO: Fill out these texts
    mpActionScripts->setText(tr("Scripts"));
    mpActionScripts->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                .arg(tr("<p>Show and edit scripts.</p>")));
    menuActionScripts->setToolTip(mpActionScripts->toolTip());

    mpActionKeys->setText(tr("Keys"));
    mpActionKeys->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                             .arg(tr("<p>Show and edit keys.</p>")));
    menuActionKeys->setToolTip(mpActionKeys->toolTip());

    mpActionVariables->setText(tr("Variables"));
    mpActionVariables->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                  .arg(tr("<p>Show and edit lua variables.</p>")));
    menuActionVariables->setToolTip(mpActionVariables->toolTip());

    mpActionIRC->setText(tr("IRC"));
    mpActionIRC->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                            .arg(tr("<p>Join and get live help from a dedicated #mudlet help channel on the built-in IRC(<i>Internet Relay "
                                    "Chat</i>) client, this will connect to one of many (all interconnected) Freenode IRC servers. If "
                                    "nobody answers right away, give it a little time...</p>")));
    menuActionIrcBuiltIn->setToolTip(mpActionIRC->toolTip());

    menuActionIrcWeb->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                 .arg(tr("<p>Join and get live help from a dedicated #mudlet help channel using your system web-browser to connect to a special "
                                         "web-page that emulates an <i>Internet Relay Chat</i> client on http://webchat.freenode.net, this will connect to one "
                                         "of many (all interconnected) Freenode IRC servers. If nobody answers right away, give it a little time...</p>")));

    mpActionMapper->setText(tr("Map"));
    mpActionMapper->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                               .arg(tr("<p>Show/hide the map.</p>")));
    menuActionShowMap->setToolTip(mpActionMapper->toolTip());

    mpActionHelp->setText(tr("Manual"));
    mpActionHelp->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                             .arg(tr("<p>Browse reference material and documentation (opens in your system web-browser) from the (on-line) Mudlet Wiki, at the API front page.</p>")));
    menuActionHelp->setToolTip(mpActionHelp->toolTip());

    mpActionOptions->setText(tr("Preferences", "Is there an Alt+P (Preferences) shortcut on the main menu"));
    mpActionOptions->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                .arg(tr("<p>Review and edit Mudlet and currently active profile preferences.</p>")));
    menuActionOptions->setToolTip(mpActionOptions->toolTip());

    mpActionNotes->setText(tr("Notepad", "Is there an Alt+N shortcut on the main menu"));
    mpActionNotes->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                              .arg(tr("<p>Open a notepad (free form text editor window) for the active profile that is also saved "
                                      "<b>between</b> sessions - that you can store your notes in for <i>that</i> profile.</p>")));
    menuActionNotepad->setToolTip(mpActionNotes->toolTip());

    mpActionPackageManager->setText(tr("Package Manager", "Is there an Alt+O shortcut on the main menu"));
    mpActionPackageManager->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                       .arg(tr("<p>Package Manager - allows you to install (and remove) <i>.xml</i> and <i>.mpackage</i> type previously saved Mudlet resources.</p>")));
    menuActionPackageManager->setToolTip(mpActionPackageManager->toolTip());

    mpActionModuleManager->setText(tr("Module Manager"));
    mpActionModuleManager->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                      .arg(tr("<p>Module Manager - allows you to install (and remove) <i>.xml</i> and <i>.mpackage</i> type that you wish to have "
                                              "syncronized across multiple profile (good for scripts that you use on several profiles!)</p>")));
    menuActionModuleManager->setToolTip(mpActionModuleManager->toolTip());

    menuActionPackageExporter->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                          .arg(tr("<p>Gather and bundle up collections of Mudlet items (and other resources) into a module "
                                                  "that can be shared with other profiles and other users on different Mudlet clients.</p>")));

    mpActionReplay->setText(tr("Load Replay"));
    mpActionReplay->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                               .arg(tr("<p>Load a Mudlet replay. This is a recording from this or a previous MUD session that can be replayed and used to test "
                                       "and debug the Mudlet items and lua scripts that you or others have created (to avoid confusion it is best to do this "
                                       "whilst off-line i.e. <i>disconnected</i> from the MUD Server!)</p>")));
    menuActionReplay->setToolTip(mpActionReplay->toolTip());

    mpActionReconnect->setText(tr("Reconnect", "Is the Alt+R shortcut on the main menu"));
    mpActionReconnect->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                  .arg(tr("<p>Disconnects you from the game and connects once again.</p>")));
    menuActionReconnect->setToolTip(mpActionReconnect->toolTip());

    mpActionDisconnect->setText(tr("Disconnect", "Is the Alt+D shortcut on the main menu"));
    mpActionDisconnect->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                   .arg(tr("<p>Disconnects you from the MUD Server.</p>")));
    menuActionDisconnect->setToolTip(mpActionDisconnect->toolTip());

    mpActionMultiView->setText(tr("MultiView", "Is there an Alt+V shortcut on the main menu"));
    mpActionMultiView->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                  .arg(tr("<p>When you have multiple profiles (MUD sessions), whether <i>connected</i> or not, open, this splits the main Mudlet "
                                          "screen to show them all at once, one tab will still be selected and this remains the one that is regarded as the "
                                          "<i>active</i> one and is the one to which the other buttons in this toolbar will act.")));
    menuActionMultiView->setToolTip(mpActionMultiView->toolTip());

    mpActionAbout->setText(tr("About Mudlet"));
    mpActionAbout->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                              .arg(tr("<p>About Mudlet, including version information, and something about the <i>Mudlet Makers</i> who "
                                      "created this superb application <i>8-)</i> and the terms under which you may <u>give freely</u> "
                                      "copies to others.  It also gives the details of some other components that Mudlet can use due to "
                                      "their Creators' generous contributions towards the Free and Open Source Software ideal.</p>")));
    menuActionAbout->setToolTip(mpActionAbout->toolTip());

    menuActionAboutQt->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                  .arg(tr("<p>About the Qt libraries used as a framework upon which Mudlet (and many other applications) are made.</p>")));

    menuActionDownload->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                   .arg(tr("<p>Opens the Mudlet website in your system's web-browser at the place where you can download the latest version of this application.</p>")));

    menuActionForum->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                .arg(tr("<p>Opens the Mudlet website in your system's web-browser at the place where any matter relating to Mudlet can be discussed in writing.</p>")));

    menuActionVideo->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                .arg(tr("<p>Opens the Mudlet website in your system's web-browser at the place where there is a collection of \"Educational Mudlet screencasts\".</p>")));

    menuActionQuit->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                               .arg(tr("<p>Will ask to close all open profiles and then exit Mudlet if sucessful.</p>")));

    if (compactInputLine()) {
        menuActionInputLine->setText(tr("Compact input line"));
    } else {
        menuActionInputLine->setText(tr("Standard input line"));
    }

    if (mpActionFullScreenView) {
        mpActionFullScreenView->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                           .arg(tr("<p>Toggles Full Screen View (which has <b>no</b> titlebar.)</p>")));
    }

    if (mpActionReplaySpeedUp) {
        mpActionReplaySpeedUp->setText(tr("Faster"));
        mpActionReplaySpeedUp->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                          .arg(tr("<p>Replay each step with a smaller time interval between steps.</p>")));
    }

    if (mpActionReplaySpeedDown) {
        mpActionReplaySpeedDown->setText(tr("Slower"));
        mpActionReplaySpeedDown->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                            .arg(tr("<p>Replay each step with a longer time interval between steps.</p>")));
    }

    if (mpLabelReplaySpeedDisplay) {
        mpLabelReplaySpeedDisplay->setText(QStringLiteral("<font size5><b>%1</b></font>")
                                           .arg(tr("Speed: X%1").arg(mReplaySpeed)));
        mpLabelReplaySpeedDisplay->show();
    }

    if (mpLabelReplayTime) {
        mpLabelReplayTime->setText(QStringLiteral("<font size=25><b>%1</b></font>")
                                   .arg(tr("Time: %1").arg(mReplayTime.toString(mTimeFormat))));
        mpLabelReplayTime->show();
    }

    // PLACEMARKER: Redefine GUI Texts
    // TODO - regenerate the texts we have created
    if (mpCurrentActiveHost) {
        setWindowTitle(tr("Mudlet - %1", "Argument is the currently active profile's name")
                       .arg(mpCurrentActiveHost->getName()));
    } else {
        setWindowTitle(tr("Mudlet - %1", "Argument is the Mudlet version string")
                       .arg(version));
    }

    QStringList splashScreenTexts;
    // Generate the start-up Splash screen texts, the first one is a template
    // so does not have the QString::arg()s expected - those are supplied when
    // it is used...
    splashScreenTexts << tr("Version: %1%2");
    splashScreenTexts << tr("\nMudlet comes with\n"
                            "ABSOLUTELY NO WARRANTY!\n"
                            "This is free software, and you are\n"
                            "welcome to redistribute it under\n"
                            "certain conditions; select the\n"
                            "'About' item for details.\n\n"
                            "Locating profiles... Done.\n"
                            "Loading font files...",
                            "May need to insert some extra new lines to ensure text fits within width - try and copy them back to the other two texts if they have the characters where the new lines are.");
    splashScreenTexts << tr("\nMudlet comes with\n"
                            "ABSOLUTELY NO WARRANTY!\n"
                            "This is free software, and you are\n"
                            "welcome to redistribute it under\n"
                            "certain conditions; select the\n"
                            "'About' item for details.\n\n"
                            "Locating profiles... Done.\n"
                            "Loading font files... Done.\n"
                            "Initiating main application...\n",
                            "May need to insert some extra new lines to ensure text fits within width - try and copy them back to the other two texts if they have the characters where the new lines are.");
    splashScreenTexts << tr("\nMudlet comes with\n"
                            "ABSOLUTELY NO WARRANTY!\n"
                            "This is free software, and you are\n"
                            "welcome to redistribute it under\n"
                            "certain conditions; select the\n"
                            "'About' item for details.\n\n"
                            "Locating profiles... Done.\n"
                            "Loading font files... Done.\n"
                            "Initiating main application... Done.\n\n"
                            "Starting Mudlet... Have fun!",
                            "May need to insert some extra new lines to ensure text fits within width - try and copy them back to the other two texts if they have the characters where the new lines are.");

    QFile splashScreenTextsFile(mudlet::getMudletPath(mudlet::mainDataItemPath, QStringLiteral("splashScreenTexts.dat")));
    if (splashScreenTextsFile.open(QIODevice::WriteOnly | QIODevice::Unbuffered)) {
        QDataStream ofs(&splashScreenTextsFile);
        ofs << splashScreenTexts;
        splashScreenTextsFile.close();
    }

    update();
}

void mudlet::initEdbee()
{
    // We only need the single Lua lexer, probably ever
    // Optional additional themes will be added in future

    edbee::Edbee* edbee = edbee::Edbee::instance();
    edbee->autoInit();
    edbee->autoShutDownOnAppExit();

    auto grammarManager = edbee->grammarManager();
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
    sl << tr("Module Name")
       << tr("Priority")
       << tr("Sync")
       << tr("Module Location");
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
            masterModule->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                     .arg(tr("<p>Checking this box will cause the module to be saved and <i>resynchronised</i> across all sessions that share it when the <i>Save Profile</i> button is clicked in the Editor or if it is saved at the end of the session.</p>",
                                             "Ensure that the indicated <i>Save Profile</i> term has the same translation as the button that is referred to and is elsewhere on the dialogue.")));
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

        QTextEdit * pTextEdit_info = mpModuleDlg->findChild<QTextEdit*>(QStringLiteral("textEdit"));
        pTextEdit_info->setHtml(QStringLiteral("<html><head/><body>%1</body></html>")
                                .arg(tr("<p>Modules are a way to utilize a common package across many sessions - unlike packages, which are installed per-profile.</p>"
                                        "<p>Modules are loaded in ascending priority (1 will get loaded before 2 and so on), modules with the same priority will be loaded in alphabetical order.</p>"
                                        "<p>The <b><i>Sync</i></b> option, if it is enabled, will, when the module in <b>this profile</b> is saved <b>to disk</b>, cause it to be then reloaded into all profiles which also are using the same file that contains the module. "
                                        "To make several profiles use the same module, install it in each profile through this module manager (which should be opened when the particular profile is the one currently in the foreground).</p>"
                                        "<p><i>NOTE:</i> <b>.zip</b> and <b>.mpackage</b> modules are currently unable to be synced, only <b>.xml</b> packages are able to be synchronized across profiles at the moment.</p>"
                                        "<p>For each save operation, modules are backed up to a directory, <i>moduleBackups</i>, within your Mudlet profile directory.</p>")));

        moduleTable = mpModuleDlg->findChild<QTableWidget*>(QStringLiteral("moduleTable"));
        moduleUninstallButton = mpModuleDlg->findChild<QPushButton*>(QStringLiteral("uninstallButton"));
        moduleInstallButton = mpModuleDlg->findChild<QPushButton*>(QStringLiteral("installButton"));
        moduleHelpButton = mpModuleDlg->findChild<QPushButton*>(QStringLiteral("helpButton"));
        // This button should be disabled until at least a module is selected:
        moduleHelpButton->setEnabled(false);

        if (!pTextEdit_info || !moduleTable || !moduleUninstallButton || !moduleInstallButton || !moduleHelpButton) {
            qWarning() << "mudlet::slot_module_manager() ERROR - expected element not found on form - aborting!";
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
        moduleHelpButton->setDisabled(( !mpModuleTableHost->moduleHelp.value(entry->text()).contains(QStringLiteral("helpURL"))
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
    } else  {
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
    if (d->lineEdit_filePath->text().isEmpty()) {
        return;
    }

    d->show();
}


void mudlet::slot_close_profile_requested(int tab)
{
    QString name = mpTabBar->tabText(tab);
    Host* pH = getHostManager().getHost(name);
    if (!pH) {
        return;
    }

    list<QPointer<TToolBar>> hostToolBarMap = pH->getActionUnit()->getToolBarList();
    QMap<QString, TDockWidget*>& dockWindowMap = mHostDockConsoleMap[pH];
    QMap<QString, TConsole*>& hostConsoleMap = mHostConsoleMap[pH];

    if (!pH->mpConsole->close()) {
        return;
    } else {
        pH->mpConsole->mUserAgreedToCloseConsole = true;
    }
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
        getHostManager().deleteHost(pH->getName());
    }

    // hide the tab bar if we only have 1 or no tabs available. saves screen space.
    if (mConsoleMap.size() > 1) {
        mpTabBar->show();
        mpActionMultiView->setEnabled(true);
        menuActionMultiView->setEnabled(true);
    } else {
        mpTabBar->hide();
        mpActionMultiView->setEnabled(false);
        menuActionMultiView->setEnabled(false);
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
                    getHostManager().deleteHost(name);
                    mTabMap.remove(name);
                }

                if (mConsoleMap.size() > 1) {
                    mpTabBar->show();
                    mpActionMultiView->setEnabled(true);
                    menuActionMultiView->setEnabled(true);
                } else {
                    mpTabBar->hide();
                    mpActionMultiView->setEnabled(false);
                    menuActionMultiView->setEnabled(false);
                }

                mpCurrentActiveHost = Q_NULLPTR;
            }
        }

        if (!mConsoleMap.size()) {
            // If there are no consoles left then we must disable a lot of
            // menu toolbar items:
            disableToolbarButtons();
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
        mpActionMultiView->setEnabled(true);
        menuActionMultiView->setEnabled(true);
    } else {
        mpTabBar->hide();
        mpActionMultiView->setEnabled(false);
        menuActionMultiView->setEnabled(false);
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
        pConsole->mLogButton->click();
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
    mpActionTriggers->setEnabled(false);
    menuActionTriggers->setEnabled(false);

    mpActionAliases->setEnabled(false);
    menuActionAliases->setEnabled(false);

    mpActionTimers->setEnabled(false);
    menuActionTimers->setEnabled(false);

    mpActionButtons->setEnabled(false);
    menuActionButtons->setEnabled(false);

    mpActionKeys->setEnabled(false);
    menuActionKeys->setEnabled(false);

    mpActionScripts->setEnabled(false);
    menuActionScripts->setEnabled(false);

    mpActionVariables->setEnabled(false);
    menuActionVariables->setEnabled(false);

    mpActionMapper->setEnabled(false);
    menuActionShowMap->setEnabled(false);

    mpActionOptions->setEnabled(false);
    menuActionOptions->setEnabled(false);

    mpActionNotes->setEnabled(false);
    menuActionNotepad->setEnabled(false);

    mpActionPackageManager->setEnabled(false);
    menuActionPackageManager->setEnabled(false);

    mpActionModuleManager->setEnabled(false);
    menuActionModuleManager->setEnabled(false);

    menuActionPackageExporter->setEnabled(false);

    mpActionReplay->setEnabled(false);
    menuActionReplay->setEnabled(false);

    mpActionReconnect->setEnabled(false);
    menuActionReconnect->setEnabled(false);

    mpActionDisconnect->setEnabled(false);
    menuActionDisconnect->setEnabled(false);

    // This one will not want to be enabled until the tabbed main window widget is shown
    mpActionMultiView->setEnabled(false);
    menuActionMultiView->setEnabled(false);
}

void mudlet::enableToolbarButtons()
{
    mpActionTriggers->setEnabled(true);
    menuActionTriggers->setEnabled(true);

    mpActionAliases->setEnabled(true);
    menuActionAliases->setEnabled(true);

    mpActionTimers->setEnabled(true);
    menuActionTimers->setEnabled(true);

    mpActionButtons->setEnabled(true);
    menuActionButtons->setEnabled(true);

    mpActionKeys->setEnabled(true);
    menuActionKeys->setEnabled(true);

    mpActionScripts->setEnabled(true);
    menuActionScripts->setEnabled(true);

    mpActionVariables->setEnabled(true);
    menuActionVariables->setEnabled(true);

    mpActionMapper->setEnabled(true);
    menuActionShowMap->setEnabled(true);

    mpActionOptions->setEnabled(true);
    menuActionOptions->setEnabled(true);

    mpActionNotes->setEnabled(true);
    menuActionNotepad->setEnabled(true);

    mpActionPackageManager->setEnabled(true);
    menuActionPackageManager->setEnabled(true);

    mpActionModuleManager->setEnabled(true);
    menuActionModuleManager->setEnabled(true);

    menuActionPackageExporter->setEnabled(true);

    mpActionReplay->setEnabled(true);
    menuActionReplay->setEnabled(true);

    mpActionReconnect->setEnabled(true);
    menuActionReconnect->setEnabled(true);

    mpActionDisconnect->setEnabled(true);
    menuActionDisconnect->setEnabled(true);

    // The multiViuew items will not want to be enabled until the tabbed main window widget is shown
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
    QMap<QString, TDockWidget*>& dockWindowMap = mHostDockConsoleMap[pHost];

    if (dockWindowMap.contains(name) && dockWindowConsoleMap.contains(name)) {
        TConsole* pC = dockWindowConsoleMap.value(name);
        pC->console->mDisplayFont = QFont("Bitstream Vera Sans Mono", size, QFont::Normal);
        pC->console->updateScreenView();
        pC->console->forceUpdate();
        pC->console2->mDisplayFont = QFont("Bitstream Vera Sans Mono", size, QFont::Normal);
        pC->console2->updateScreenView();
        pC->console2->forceUpdate();

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
    QMap<QString, TDockWidget*>& dockWindowMap = mHostDockConsoleMap[pHost];

    if (dockWindowMap.contains(name) && dockWindowConsoleMap.contains(name)) {
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

        if (loadLayout) {
            loadWindowLayout();
        }

        return true;
    } else if (dockWindowMap.contains(name) && dockWindowConsoleMap.contains(name)) {
        dockWindowMap[name]->update();
        dockWindowMap[name]->show();
        dockWindowConsoleMap[name]->showWindow(name);

        if (loadLayout) {
            loadWindowLayout();
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
        pC->mConsoleName = name;
        if (pC) {
            dockWindowConsoleMap[name] = pC;
            std::string _n = name.toStdString();
            pC->setMiniConsoleFontSize(_n, 12);
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

    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    if (dockWindowConsoleMap.contains(name)) {
        dockWindowConsoleMap[name]->resize(x1, y1);
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

    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    if (dockWindowConsoleMap.contains(name)) {
        dockWindowConsoleMap[name]->move(x1, y1);
        dockWindowConsoleMap[name]->mOldX = x1;
        dockWindowConsoleMap[name]->mOldY = y1;
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
        TDebug(QColor(Qt::white), QColor(Qt::red)) << "ERROR: window doesnt exit\n" >> 0;
    }
    return -1;
}

int mudlet::getColumnNumber(Host* pHost, QString& name)
{
    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    if (dockWindowConsoleMap.contains(name)) {
        return dockWindowConsoleMap[name]->getColumnNumber();
    } else {
        TDebug(QColor(Qt::white), QColor(Qt::red)) << "ERROR: window doesnt exit\n" >> 0;
    }
    return -1;
}


int mudlet::getLastLineNumber(Host* pHost, const QString& name)
{
    QMap<QString, TConsole*>& dockWindowConsoleMap = mHostConsoleMap[pHost];
    if (dockWindowConsoleMap.contains(name)) {
        return dockWindowConsoleMap[name]->getLastLineNumber();
    } else {
        TDebug(QColor(Qt::white), QColor(Qt::red)) << "ERROR: window doesnt exit\n" >> 0;
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
    if (mpDebugConsole) {
        mpDebugConsole->close();
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

    event->accept();
    qApp->quit();
}

void mudlet::forceClose()
{
    for (auto console : mConsoleMap) {
        console->mUserAgreedToCloseConsole = true;
    }

    close();
}

void mudlet::readSettings(QSettings& settings)
{
    mMainIconSize = settings.value("mainiconsize", QVariant(3)).toInt();
    mTEFolderIconSize = settings.value("tefoldericonsize", QVariant(3)).toInt();
    mShowMenuBar = settings.value("showMenuBar", QVariant(0)).toBool();
    mShowToolbar = settings.value("showToolbar", QVariant(0)).toBool();
    mEditorTextOptions = QTextOption::Flags(settings.value("editorTextOptions", QVariant(0)).toInt());

    mshowMapAuditErrors = settings.value("reportMapIssuesToConsole", QVariant(false)).toBool();
    mCompactInputLine = settings.value("compactInputLine", QVariant(false)).toBool();
    mGuiLanguageSelection = settings.value("guiLanguage", QStringLiteral("en_US")).toString();

    // There was addition code here that has been split off into
    // updateAfterReadingSettings(size, pos, settings) as it needs to be done
    // after the GUI is created whereas in order to establish which language to
    // use for the GUI we need to read these details ASAP (before we create
    // the GUI).
}


void mudlet::updateAfterReadingSettings(QSettings& settings)
{
    resize(settings.value("size", QSize(750, 550)).toSize());
    move(settings.value("pos", QPoint(0, 0)).toPoint());
    setIcoSize(mMainIconSize);
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

void mudlet::setIcoSize(int s)
{
    mpMainToolBar->setIconSize(QSize(s * 8, s * 8));
    if (mMainIconSize > 2) {
        mpMainToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    } else {
        mpMainToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    }
    if (mShowMenuBar) {
        menuBar()->show();
    } else {
        menuBar()->hide();
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
    settings.setValue("mainiconsize", mMainIconSize);
    settings.setValue("tefoldericonsize", mTEFolderIconSize);
    settings.setValue("showMenuBar", mShowMenuBar);
    settings.setValue("showToolbar", mShowToolbar);
    settings.setValue("maximized", isMaximized());
    settings.setValue("editorTextOptions", static_cast<int>(mEditorTextOptions));
    settings.setValue("reportMapIssuesToConsole", mshowMapAuditErrors);
    settings.setValue("compactInputLine", mCompactInputLine);
    settings.setValue("guiLanguage", mGuiLanguageSelection );
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
    if (!pHost) {
        return;
    }

    if (!mpProfilePreferencesDlg) {
        mpProfilePreferencesDlg = new dlgProfilePreferences(this, pHost);
        connect(mpActionReconnect, SIGNAL(triggered()), mpProfilePreferencesDlg->need_reconnect_for_data_protocol, SLOT(hide()));
        connect(menuActionReconnect, SIGNAL(triggered()), mpProfilePreferencesDlg->need_reconnect_for_data_protocol, SLOT(hide()));
        connect(mpActionReconnect, SIGNAL(triggered()), mpProfilePreferencesDlg->need_reconnect_for_specialoption, SLOT(hide()));
        connect(menuActionReconnect, SIGNAL(triggered()), mpProfilePreferencesDlg->need_reconnect_for_specialoption, SLOT(hide()));
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

        // Locate the text label
        QLabel * pLabel = dialog->findChild<QLabel*>(QStringLiteral("label"));
        // Insert HTML, here is a deobfusticated copy of what WAS there:
        pLabel->setText(QStringLiteral("<html><head/><body>%1</body></html>")
                        .arg(tr("<p>It seems that you do not have any <a href=\"http://wiki.mudlet.org/w/Mapping_script\">mapping scripts</a> installed yet - the mapper needs you to have one for your MUD, so it can track where you are and autowalk you. "
                                "You can either make one yourself, or import an existing one that someone else made.</p>"
                                "<p>Would you like to see if any are available?</p>")));

        // There are two push-buttons on the form "Close" and "Find some scripts"
        // the first one is wired-up internally to the "reject" (i.e. cancel
        // action) and the second to the "accept" action - so that clicking on
        // it invokes the following:
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

void mudlet::slot_show_help_dialog_download()
{
    QDesktopServices::openUrl(QUrl("https://www.mudlet.org/download/"));
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
        pNotes->setWindowIcon(QIcon(QStringLiteral(":/icons/applications-accessories.png")));
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
    QStringList hostList = QDir(getMudletPath(profilesPath))
                           .entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
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

    Host* pOH = getHostManager().getHost(profile_name);
    if (pOH) {
        pOH->mTelnet.connectIt(pOH->getUrl(), pOH->getPort());
        return;
    }
    // load an old profile if there is any
    getHostManager().addHost(profile_name, "", "", "");
    Host* pHost = getHostManager().getHost(profile_name);

    if (!pHost) {
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

    QString login = "login";
    QString val1 = readProfileData(profile_name, login);
    pHost->setLogin(val1);
    QString pass = "password";
    QString val2 = readProfileData(profile_name, pass);
    pHost->setPass(val2);
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

    // It would be nice if we could replace the ONLY call to Host::connectToServer()
    // with pHost->mTelnet.connectIt(pHost->getUrl, pHost->getPort()); but that
    // is not possible because that is not a static member function (?)
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
        menuActionInputLine->setText(tr("Compact input line"));
        setCompactInputLine(false);
    } else {
        buttons->hide();
        menuActionInputLine->setText(tr("Standard input line"));
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
        menuActionInputLine->setText(tr("Compact input line"));
    } else {
        buttons->hide();
        menuActionInputLine->setText(tr("Standard input line"));
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

    mpToolBarReplay = new QToolBar(this);
    mpToolBarReplay->setIconSize(QSize(8 * mMainIconSize, 8 * mMainIconSize));
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
    mpMainToolBar->widgetForAction(mpActionReplaySpeedUp)->setObjectName(mpActionReplaySpeedUp->objectName());

    mpActionReplaySpeedDown = new QAction(QIcon(QStringLiteral(":/icons/import.png")), tr("Slower"), this);
    mpActionReplaySpeedDown->setObjectName(QStringLiteral("replay_speed_down_action"));
    mpActionReplaySpeedDown->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                        .arg(tr("<p>Replay each step with a longer time interval between steps.</p>")));
    mpToolBarReplay->addAction(mpActionReplaySpeedDown);
    mpMainToolBar->widgetForAction(mpActionReplaySpeedDown)->setObjectName(mpActionReplaySpeedDown->objectName());

    mpLabelReplaySpeedDisplay = new QLabel(this);
    mpActionSpeedDisplay = mpToolBarReplay->addWidget(mpLabelReplaySpeedDisplay);

    connect(mpActionReplaySpeedUp, SIGNAL(triggered()), this, SLOT(slot_replaySpeedUp()));
    connect(mpActionReplaySpeedDown, SIGNAL(triggered()), this, SLOT(slot_replaySpeedDown()));

    mpLabelReplaySpeedDisplay->setText(QStringLiteral("<font size=25><b>%1</b></font>").arg(tr("Speed: X%1").arg(mReplaySpeed)));

    mpTimerReplay = new QTimer(this);
    mpTimerReplay->setInterval(1000);
    mpTimerReplay->setSingleShot(false);
    connect(mpTimerReplay, SIGNAL(timeout()), this, SLOT(slot_replayTimeChanged()));

    mpLabelReplayTime->setText(QStringLiteral("<font size=25><b>%1</b></font>").arg(tr("Time: %1").arg(mReplayTime.toString(mTimeFormat))));

    mpLabelReplaySpeedDisplay->show();
    mpLabelReplayTime->show();
    insertToolBar(mpMainToolBar, mpToolBarReplay);
    mpToolBarReplay->show();
    mpTimerReplay->start();
}

void mudlet::slot_replayTimeChanged()
{
    mpLabelReplayTime->setText(QStringLiteral("<font size=25><b>%1</b></font>").arg(tr("Time: %1").arg(mReplayTime.toString(mTimeFormat))));
    mpLabelReplayTime->show();
}

void mudlet::replayOver()
{
    if ((!mpMainToolBar) || (!mpToolBarReplay)) {
        return;
    }

    if (mpActionReplaySpeedUp) {
        disconnect(mpActionReplaySpeedUp, SIGNAL(triggered()), this, SLOT(slot_replaySpeedUp()));
        disconnect(mpActionReplaySpeedDown, SIGNAL(triggered()), this, SLOT(slot_replaySpeedDown()));
        mpToolBarReplay->removeAction(mpActionReplaySpeedUp);
        mpToolBarReplay->removeAction(mpActionReplaySpeedDown);
        mpToolBarReplay->removeAction(mpActionSpeedDisplay);
        removeToolBar(mpToolBarReplay);
        mpActionReplaySpeedUp->deleteLater(); // Had previously ommitted these, causing a reasource leak!
        mpActionReplaySpeedUp = nullptr;
        mpActionReplaySpeedDown->deleteLater();
        mpActionReplaySpeedDown = nullptr;
        mpActionSpeedDisplay->deleteLater();
        mpActionSpeedDisplay = nullptr;
        mpActionReplayTime->deleteLater();
        mpActionReplayTime = nullptr;
        mpToolBarReplay->deleteLater();
        mpToolBarReplay = nullptr;
    }
}

void mudlet::slot_replaySpeedUp()
{
    mReplaySpeed = mReplaySpeed * 2;
    mpLabelReplaySpeedDisplay->setText(QStringLiteral("<font size=25><b>%1</b></font>").arg(tr("Speed: X%1").arg(mReplaySpeed)));

    mpLabelReplaySpeedDisplay->show();
}

void mudlet::slot_replaySpeedDown()
{
    mReplaySpeed = mReplaySpeed / 2;
    if (mReplaySpeed < 1) {
        mReplaySpeed = 1;
    }
    mpLabelReplaySpeedDisplay->setText(QStringLiteral("<font size=25><b>%1</b></font>").arg(tr("Speed: X%1").arg(mReplaySpeed)));
    mpLabelReplaySpeedDisplay->show();
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
            pHost->postMessage("[ ERROR ] - Unable to create new QMediaPlayer object\n");
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
    char buf[4096];
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
    QString themeLocation(
                getMudletPath(editorWidgetThemePathFile,
                              themeFile));
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
    switch(mode) {
    case mainPath:
        // The root of all mudlet data for the user - does not end in a '/'
        return QStringLiteral("%1/.config/mudlet")
                .arg(QDir::homePath());
    case mainDataItemPath:
        // Takes one extra argument as a file (or directory) relating to
        // (profile independent) mudlet data - may end with a '/' if the extra
        // argument does:
        return QStringLiteral("%1/.config/mudlet/%2")
                .arg(QDir::homePath(), extra1);
    case mainFontsPath:
        // (Added for 3.5.0) a revised location to store Mudlet provided fonts
        return QStringLiteral("%1/.config/mudlet/fonts")
                .arg(QDir::homePath());
    case profilesPath:
        // The directory containing all the saved user's profiles - does not end
        // in '/'
        return QStringLiteral("%1/.config/mudlet/profiles")
                .arg(QDir::homePath());
    case profileHomePath:
        // Takes one extra argument (profile name) that returns the base
        // directory for that profile - does NOT end in a '/' unless the
        // supplied profle name does:
        return QStringLiteral("%1/.config/mudlet/profiles/%2")
                .arg(QDir::homePath(), extra1);
    case profileXmlFilesPath:
        // Takes one extra argument (profile name) that returns the directory
        // for the profile game save XML files - ends in a '/'
        return QStringLiteral("%1/.config/mudlet/profiles/%2/current/")
                .arg(QDir::homePath(), extra1);
    case profileMapsPath:
        // Takes one extra argument (profile name) that returns the directory
        // for the profile game save maps files - does NOT end in a '/'
        return QStringLiteral("%1/.config/mudlet/profiles/%2/map")
                .arg(QDir::homePath(), extra1);
    case profileDateTimeStampedMapPathFileName:
        // Takes two extra arguments (profile name, dataTime stamp) that returns
        // the pathFile name for a dateTime stamped map file:
        return QStringLiteral("%1/.config/mudlet/profiles/%2/map/%3map.dat")
                .arg(QDir::homePath(), extra1, extra2);
    case profileMapPathFileName:
        // Takes two extra arguments (profile name, mapFileName) that returns
        // the pathFile name for any map file:
        return QStringLiteral("%1/.config/mudlet/profiles/%2/map/%3")
                .arg(QDir::homePath(), extra1, extra2);
    case profileXmlMapPathFileName:
        // Takes one extra argument (profile name) that returns the pathFile
        // name for the downloaded IRE Server provided XML map:
        return QStringLiteral("%1/.config/mudlet/profiles/%2/map.xml")
                .arg(QDir::homePath(), extra1);
    case profileDataItemPath:
        // Takes two extra arguments (profile name, data item) that gives a
        // path file name for, typically a data item stored as a single item
        // (binary) profile data) file (ideally these can be moved to a per
        // profile QSettings file but that is a future pipe-dream on my part
        // SlySven):
        return QStringLiteral("%1/.config/mudlet/profiles/%2/%3")
                .arg(QDir::homePath(), extra1, extra2);
    case profilePackagePath:
        // Takes two extra arguments (profile name, package name) returns the
        // per profile directory used to store (unpacked) package contents
        // - ends with a '/':
        return QStringLiteral("%1/.config/mudlet/profiles/%2/%3/")
                .arg(QDir::homePath(), extra1, extra2);
    case profilePackagePathFileName:
        // Takes two extra arguments (profile name, package name) returns the
        // filename of the XML file that contains the (per profile, unpacked)
        // package mudlet items in that package/module:
        return QStringLiteral("%1/.config/mudlet/profiles/%2/%3/%3.xml")
                .arg(QDir::homePath(), extra1, extra2);
    case profilePackageStagingPathFileName:
        // Takes two extra arguments (profile name, package name) returns the
        // path of staging area for the package/module items in that
        // package/module - ends in '/':
        return QStringLiteral("%1/.config/mudlet/profiles/%2/tmp/%3/")
                .arg(QDir::homePath(), extra1, extra2);
    case profileReplayAndLogFilesPath:
        // Takes one extra argument (profile name) that returns the directory
        // that contains replays (*.dat files) and logs (*.html or *.txt) files
        // for that profile - does NOT end in '/':
        return QStringLiteral("%1/.config/mudlet/profiles/%2/log")
                .arg(QDir::homePath(), extra1);
    case profileLogErrorsFilePath:
        // Takes one extra argument (profile name) that returns the pathFileName
        // to the map auditing report file that is appended to each time a
        // map is loaded:
        return QStringLiteral("%1/.config/mudlet/profiles/%2/log/errors.txt")
                .arg(QDir::homePath(), extra1);
    case editorWidgetThemePathFile:
        // Takes two extra arguments (profile name, theme name) that returns the
        // pathFileName of the theme file used by the edbee editor - also
        // handles the special case of the default theme "mudlet.thTheme" that
        // is carried internally in the resource file:
        if (extra1.compare(QStringLiteral("Mudlet.tmTheme"),Qt::CaseSensitive)) {
            // No match
            return QStringLiteral("%1/.config/mudlet/edbee/Colorsublime-Themes-master/themes/%2")
                    .arg(QDir::homePath(), extra1);
        } else {
            // Match - return path to copy held in resource file
            return QStringLiteral(":/edbee_defaults/Mudlet.tmTheme");
        }
    case editorWidgetThemeJsonFile:
        // Returns the pathFileName to the external JSON file needed to process
        // an edbee edtor widget theme:
        return QStringLiteral("%1/.config/mudlet/edbee/Colorsublime-Themes-master/themes.json")
                .arg(QDir::homePath());
    case moduleBackupsPath:
        // Returns the directory used to store module backups that is used in
        // when saving/resyncing packages/modules - ends in a '/'
        return QStringLiteral("%1/.config/mudlet/moduleBackups/")
                .arg(QDir::homePath());
    case mudletTranslationsPath:
#if defined( Q_OS_MAC )
        return QStringLiteral("%1/../Resources/").arg(QCoreApplication::applicationDirPath());
#else
        return QStringLiteral("%1/").arg(QCoreApplication::applicationDirPath());
#endif
    case qtTranslationsPath:
        return QLibraryInfo::location(QLibraryInfo::TranslationsPath);
    }
}

// NOT a slot_ method, it is called directly from dlgProfilePreferences and
// raises a SIGNAL that everything with a translateable GUI should listen for
// and act on.
void mudlet::setGuiLanguage(const QString& languageCode)
{
    QString oldLanguageCode = QLatin1String("none");
    if (!mGuiLanguageSelection.isEmpty()) {
        oldLanguageCode = mGuiLanguageSelection;
    }

    // Handle case where no translator is wanted
    if (languageCode.isEmpty() || languageCode == QStringLiteral("none")) {
        mGuiLanguageSelection = QStringLiteral("none");
        if (!mTranslatorsLoadedList.isEmpty()) {
            QMutableListIterator<QPointer<QTranslator>> itTranslator(mTranslatorsLoadedList);
            // Unload in reverse order to loading - may not be important but
            // way not...
            itTranslator.toBack();
            while (itTranslator.hasPrevious()) {
                // This cause a LanguageChangeEvent to be generated:
                QPointer<QTranslator> pTranslator(itTranslator.previous());
                if (pTranslator) {
                    qApp->removeTranslator(pTranslator);
                }
                itTranslator.remove();
            }
        }

        // Allow the Qt events to propogate through the system
        qApp->processEvents();

        // Translate the automatically generated form/dialog stuff and handle
        // the GUI things we made
        guiLanguageChange();

        emit signal_translatorChangeCompleted(mGuiLanguageSelection, oldLanguageCode);

        return;
    }

    // Check for whether we need to do anything:
    if (mGuiLanguageSelection == languageCode || !mTranslatorsMap.contains(languageCode)) {
        return;
    }

    mGuiLanguageSelection = languageCode;

    // Handle other cases where translators are to be used - first, unload
    // installed translators
    if (!mTranslatorsLoadedList.isEmpty()) {
        QMutableListIterator<QPointer<QTranslator>> itTranslator(mTranslatorsLoadedList);
        // Unload in reverse order to loading - may not be important but
        // way not...
        itTranslator.toBack();
        while (itTranslator.hasPrevious()) {
            // This cause a LanguageChangeEvent to be generated:
            QPointer<QTranslator> pTranslator(itTranslator.previous());
            if (pTranslator) {
                qApp->removeTranslator(pTranslator);
            }
            itTranslator.remove();
        }
    }

    QListIterator<QPointer<QTranslator>> itTranslator(mTranslatorsMap.value(languageCode));
    while (itTranslator.hasNext()) {
        QPointer<QTranslator> pTranslator(itTranslator.next());
        if (pTranslator) {
            // This cause a LanguageChangeEvent to be generated:
            if (qApp->installTranslator(pTranslator)) {
                // Managed to load translation, so add this one to loaded list
                mTranslatorsLoadedList.append(pTranslator);
            }
        }
    }

    // Allow the Qt events to propogate through the system
    qApp->processEvents();

    // Handle the GUI things we made
    guiLanguageChange();

    emit signal_translatorChangeCompleted(mGuiLanguageSelection, oldLanguageCode);
}
