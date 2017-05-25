/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2013-2017 by Stephen Lyons - slysven@virginmedia.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016 by Chris Leacy - cleacy1972@gmail.com              *
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
#include <QTextCharFormat>
#include <QToolBar>
#include "post_guard.h"


using namespace std;

bool TConsoleMonitor::eventFilter(QObject *obj, QEvent *event)
{
    if( event->type() == QEvent::Close )
    {
        mudlet::debugMode = false;
        return QObject::eventFilter(obj,event);
    }
    else
    {
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
const QString mudlet::scmMudletXmlDefaultVersion = QString::number( 1.001f, 'f', 3 );

QPointer<TConsole> mudlet::mpDebugConsole = 0;
QMainWindow* mudlet::mpDebugArea = 0;
bool mudlet::debugMode = false;
static const QString timeFormat = "hh:mm:ss";

QPointer<mudlet> mudlet::_self;

void mudlet::start()
{
    _self = new mudlet;
}

mudlet * mudlet::self()
{
    return _self;
}


mudlet::mudlet()
: QMainWindow()
, mShowMenuBar( false )
, mShowToolbar( true )
, mWindowMinimized( false )
, mReplaySpeed( 1 )
, mpIRC( 0 )
, version( QString("Mudlet ") + QString(APP_VERSION) + QString(APP_BUILD) )
, mpCurrentActiveHost( 0 )
, mIsGoingDown( false )
, actionReplaySpeedDown( 0 )
, actionReplaySpeedUp( 0 )
, actionSpeedDisplay( 0 )
, actionReplayTime( 0 )
, replaySpeedDisplay( 0 )
, replayTime( 0 )
, replayTimer( 0 )
, replayToolBar( 0 )
, moduleTable( 0 )
, mStatusBarState( statusBarAlwaysShown )
, mIsToDisplayMapAuditErrorsToConsole( false )
, mpAboutDlg( 0 )
, mpModuleDlg( 0 )
, mpPackageManagerDlg( 0 )
, mpProfilePreferencesDlg( 0 )
{
    setupUi(this);
    setUnifiedTitleAndToolBarOnMac( true );
    setContentsMargins(0,0,0,0);
    mudlet::debugMode = false;
    setAttribute( Qt::WA_DeleteOnClose );
    QSizePolicy sizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding);
    setWindowTitle(version);
    setWindowIcon( QIcon( QStringLiteral( ":/icons/mudlet_main_48px.png" ) ) );
    mpMainStatusBar = QMainWindow::statusBar();
    // On at least my platform (Linux) the status bar does not seem to exist
    // but getting the pointer to it causes it to be created automagically...
    mpMainToolBar = new QToolBar( this );
    mpMainToolBar->setObjectName("mpMainToolBar");
    addToolBar( mpMainToolBar );
    mpMainToolBar->setMovable( false );
    addToolBarBreak();
    auto frame = new QWidget( this );
    frame->setFocusPolicy( Qt::NoFocus );
    setCentralWidget( frame );
    mpTabBar = new QTabBar( frame );
    mpTabBar->setMaximumHeight(30);
    mpTabBar->setFocusPolicy( Qt::NoFocus );
    mpTabBar->setTabsClosable ( true );
    connect( mpTabBar, SIGNAL(tabCloseRequested(int)), this, SLOT(slot_close_profile_requested(int)));
    mpTabBar->setMovable(true);
    connect( mpTabBar, SIGNAL(currentChanged(int)), this, SLOT(slot_tab_changed(int)));
    auto layoutTopLevel = new QVBoxLayout(frame);
    layoutTopLevel->setContentsMargins(0,0,0,0);
    layoutTopLevel->addWidget( mpTabBar );
    mainPane = new QWidget( frame );
    QPalette mainPalette;
    mainPane->setPalette( mainPalette );
    mainPane->setAutoFillBackground(true);
    mainPane->setFocusPolicy( Qt::NoFocus );
    layoutTopLevel->addWidget( mainPane );
    auto layout = new QHBoxLayout( mainPane );
    layout->setContentsMargins(0,0,0,0);

    mainPane->setContentsMargins(0,0,0,0);
    mainPane->setSizePolicy( sizePolicy );
    mainPane->setFocusPolicy( Qt::NoFocus );

    QFile file_autolog( QDir::homePath()+"/.config/mudlet/autolog" );
    if( file_autolog.exists() )
    {
        mAutolog = true;
    }
    else
    {
        mAutolog = false;
    }

    QFile file_use_smallscreen( QDir::homePath()+"/.config/mudlet/mudlet_option_use_smallscreen" );
    if( file_use_smallscreen.exists() )
    {
        mpMainToolBar->setIconSize(QSize(16,16));
    }
    else
    {
        mpMainToolBar->setIconSize(QSize(32,32));
        mpMainToolBar->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );
    }
    QAction * actionConnect = new QAction( QIcon( QStringLiteral( ":/icons/preferences-web-browser-cache.png" ) ), tr("Connect"), this);
    actionConnect->setToolTip(tr("Connect to a MUD"));
    mpMainToolBar->addAction( actionConnect );

    QAction * actionTriggers = new QAction( QIcon( QStringLiteral( ":/icons/tools-wizard.png" ) ), tr("Triggers"), this);
    actionTriggers->setToolTip(tr("Show and edit triggers"));
    mpMainToolBar->addAction( actionTriggers );

    QAction * actionAlias = new QAction( QIcon( QStringLiteral( ":/icons/system-users.png" ) ), tr("Aliases"), this);
    actionAlias->setToolTip(tr("Show and edit aliases"));
    mpMainToolBar->addAction( actionAlias );

    QAction * actionTimers = new QAction( QIcon( QStringLiteral( ":/icons/chronometer.png" ) ), tr("Timers"), this);
    actionTimers->setToolTip(tr("Show and edit timers"));
    mpMainToolBar->addAction( actionTimers );

    QAction * actionButtons = new QAction( QIcon( QStringLiteral( ":/icons/bookmarks.png" ) ), tr("Buttons"), this);
    actionButtons->setToolTip(tr("Show and edit easy buttons"));
    mpMainToolBar->addAction( actionButtons );

    QAction * actionScripts = new QAction( QIcon( QStringLiteral( ":/icons/document-properties.png" ) ), tr("Scripts"), this);
    actionScripts->setToolTip(tr("Show and edit scripts"));
    mpMainToolBar->addAction( actionScripts );

    QAction * actionKeys = new QAction( QIcon( QStringLiteral( ":/icons/preferences-desktop-keyboard.png" ) ), tr("Keys"), this);
    actionKeys->setToolTip(tr("Show and edit keys"));
    mpMainToolBar->addAction( actionKeys );

    QAction * actionVars = new QAction( QIcon( QStringLiteral( ":/icons/variables.png" ) ), tr("Variables"), this);
    actionVars->setToolTip(tr("Show and edit lua variables"));
    mpMainToolBar->addAction( actionVars );

    QAction * actionIRC = new QAction( QIcon( QStringLiteral( ":/icons/internet-telephony.png" ) ), tr("Help Chat"), this);
    actionIRC->setToolTip(tr("Join Mudlet help chat on IRC"));
    mpMainToolBar->addAction( actionIRC );

    QAction * actionMapper = new QAction( QIcon( QStringLiteral( ":/icons/applications-internet.png" ) ), tr("Map"), this);
    actionMapper->setToolTip(tr("Show/hide the map"));
    mpMainToolBar->addAction( actionMapper );

    QAction * actionHelp = new QAction( QIcon( QStringLiteral( ":/icons/help-hint.png" ) ), tr("Manual"), this);
    actionHelp->setToolTip(tr("Browse reference material and documentation"));
    mpMainToolBar->addAction( actionHelp );

    QAction * actionOptions = new QAction( QIcon( QStringLiteral( ":/icons/configure.png" ) ), tr("Settings"), this);
    actionOptions->setToolTip(tr("See and edit profile preferences"));
    mpMainToolBar->addAction( actionOptions );

    // TODO: Consider changing to ":/icons/mudlet_notepad.png" as per the icon
    // now used for the window when the visual change to the toolbar caused can
    // be managed
    QAction * actionNotes = new QAction( QIcon( QStringLiteral( ":/icons/applications-accessories.png" ) ), tr("Notepad"), this);
    actionNotes->setToolTip(tr("Open a notepad that you can store your notes in"));
    mpMainToolBar->addAction( actionNotes );

    QAction * actionPackageM = new QAction( QIcon( QStringLiteral( ":/icons/package-manager.png" ) ), tr("Package Manager"), this);
    actionPackageM->setToolTip(tr("Package Manager - allows you to install xmls, .mpackages"));
    mpMainToolBar->addAction( actionPackageM );

    QAction * actionModuleM = new QAction( QIcon( QStringLiteral( ":/icons/module-manager.png" ) ), tr("Module Manager"), this);
    actionModuleM->setToolTip(tr("Module Manager - allows you to install xmls, .mpackages that are syncronized across multiple profile (good for scripts that you use on several profiles)"));
    mpMainToolBar->addAction( actionModuleM );

    QAction * actionReplay = new QAction( QIcon( QStringLiteral( ":/icons/media-optical.png" ) ), tr("Replay"), this);
    actionReplay->setToolTip(tr("Load a Mudlet replay"));
    mpMainToolBar->addAction( actionReplay );

    actionReconnect = new QAction( QIcon( QStringLiteral( ":/icons/system-restart.png" ) ), tr("Reconnect"), this);
    actionReconnect->setToolTip(tr("Disconnects you from the game and connects once again"));
    mpMainToolBar->addAction( actionReconnect );

    QAction * actionMultiView = new QAction( QIcon( QStringLiteral( ":/icons/view-split-left-right.png" ) ), tr("MultiView"), this);
    actionMultiView->setToolTip(tr("If you've got multiple profiles open, splits Mudlet screen to show them all at once"));
    mpMainToolBar->addAction( actionMultiView );

    QAction * actionStopAllTriggers = new QAction( QIcon( QStringLiteral( ":/icons/edit-bomb.png" ) ), tr("Stop All Triggers"), this);
    actionStopAllTriggers->setToolTip(tr("Stop all triggers, alias, actions, timers and scripts"));

    QAction * actionAbout = new QAction( QIcon( QStringLiteral( ":/icons/mudlet_information.png" ) ), tr("About"), this);
    actionAbout->setToolTip(tr("About Mudlet"));
    mpMainToolBar->addAction( actionAbout );

    disableToolbarButtons();

    mpDebugArea = new QMainWindow(0);
    mHostManager.addHost("default_host", "", "","" );
    mpDefaultHost = mHostManager.getHost(QString("default_host"));
    mpDebugConsole = new TConsole( mpDefaultHost, true );
    mpDebugConsole->setSizePolicy( sizePolicy );
    mpDebugConsole->setWrapAt(100);
    mpDebugArea->setCentralWidget( mpDebugConsole );
    mpDebugArea->setWindowTitle( tr( "Central Debug Console" ) );
    mpDebugArea->setWindowIcon( QIcon( QStringLiteral( ":/icons/mudlet_debug.png" ) ) );

    auto consoleCloser = new TConsoleMonitor(mpDebugArea);
    mpDebugArea->installEventFilter(consoleCloser);

    QSize generalRule( qApp->desktop()->size() );
    generalRule -= QSize( 30, 30 );
    mpDebugArea->resize( QSize( 800, 600 ).boundedTo( generalRule ) );
    mpDebugArea->hide();
    QFont mainFont;
    if( file_use_smallscreen.exists() ) {
        mainFont = QFont( QStringLiteral( "Bitstream Vera Sans Mono" ), 8, QFont::Normal);
        showFullScreen();
        QAction * actionFullScreeniew = new QAction( QIcon( QStringLiteral( ":/icons/dialog-cancel.png" ) ), tr("Toggle Full Screen View"), this);
        actionFullScreeniew->setStatusTip(tr("Toggle Full Screen View"));
        mpMainToolBar->addAction( actionFullScreeniew );
        connect(actionFullScreeniew, SIGNAL(triggered()), this, SLOT(toggleFullScreenView()));
    }
    else {
        mainFont = QFont( QStringLiteral( "Bitstream Vera Sans Mono" ), 8, QFont::Normal);
    }
    QFont mdiFont = QFont(QStringLiteral( "Bitstream Vera Sans Mono" ), 6, QFont::Normal);
    setFont( mainFont );
    mainPane->setFont( mainFont );
    mpTabBar->setFont( mdiFont );

    mainPane->show();
    connect(actionConnect, SIGNAL(triggered()), this, SLOT(connectToServer()));
    connect(actionHelp, SIGNAL(triggered()), this, SLOT(show_help_dialog()));
    connect(actionTriggers, SIGNAL(triggered()), this, SLOT(show_trigger_dialog()));
    connect(actionTimers, SIGNAL(triggered()), this, SLOT(show_timer_dialog()));
    connect(actionAlias, SIGNAL(triggered()), this, SLOT(show_alias_dialog()));
    connect(actionScripts, SIGNAL(triggered()), this, SLOT(show_script_dialog()));
    connect(actionKeys, SIGNAL(triggered()),this,SLOT(show_key_dialog()));
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

    QAction * mactionConnect = new QAction(tr("Connect"), this);
    QAction * mactionTriggers = new QAction(tr("Triggers"), this);
    QAction * mactionAlias = new QAction(tr("Aliases"), this);
    QAction * mactionTimers = new QAction(tr("Timers"), this);
    QAction * mactionButtons = new QAction(tr("Actions"), this);
    QAction * mactionScripts = new QAction(tr("Scripts"), this);
    QAction * mactionKeys = new QAction(tr("Keys"), this);
    QAction * mactionMapper = new QAction(tr("Map"), this);
    QAction * mactionHelp = new QAction(tr("Help"), this);
    QAction * mactionOptions = new QAction(tr("Preferences"), this);
    QAction * mactionMultiView = new QAction(tr("MultiView"), this);
    QAction * mactionAbout = new QAction(tr("About"), this);
    QAction * mactionCloseProfile = new QAction(tr("Close"), this);

    connect(mactionConnect, SIGNAL(triggered()), this, SLOT(connectToServer()));
    connect(dactionConnect, SIGNAL(triggered()), this, SLOT(connectToServer()));
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
    connect(dactionDownload, SIGNAL(triggered()), this, SLOT(slot_show_help_dialog_download()));
    connect(actionPackage_manager, SIGNAL(triggered()), this, SLOT(slot_package_manager()));
    connect(actionPackage_Exporter, SIGNAL(triggered()), this, SLOT(slot_package_exporter()));
    connect(actionModule_manager, SIGNAL(triggered()), this, SLOT(slot_module_manager()));
    connect(dactionMultiView, SIGNAL(triggered()), this, SLOT(slot_multi_view()));

    connect(mactionTriggers, SIGNAL(triggered()), this, SLOT(show_trigger_dialog()));
    connect(dactionScriptEditor, SIGNAL(triggered()), this, SLOT(show_trigger_dialog()));
    connect(mactionMapper, SIGNAL(triggered()), this, SLOT(slot_mapper()));
    connect(mactionTimers, SIGNAL(triggered()), this, SLOT(show_timer_dialog()));
    connect(mactionAlias, SIGNAL(triggered()), this, SLOT(show_alias_dialog()));
    connect(mactionScripts, SIGNAL(triggered()), this, SLOT(show_script_dialog()));
    connect(mactionKeys, SIGNAL(triggered()),this,SLOT(show_key_dialog()));
    connect(mactionButtons, SIGNAL(triggered()), this, SLOT(show_action_dialog()));

    connect(mactionOptions, SIGNAL(triggered()), this, SLOT(show_options_dialog()));
    connect(dactionOptions, SIGNAL(triggered()), this, SLOT(show_options_dialog()));

    connect(mactionAbout, SIGNAL(triggered()), this, SLOT(slot_show_about_dialog()));
    connect(dactionAbout, SIGNAL(triggered()), this, SLOT(slot_show_about_dialog()));

    connect(mactionMultiView, SIGNAL(triggered()), this, SLOT(slot_multi_view()));
    connect(mactionCloseProfile, SIGNAL(triggered()), this, SLOT(slot_close_profile()));

    readSettings();

    auto timerAutologin = new QTimer( this );
    timerAutologin->setSingleShot( true );
    connect(timerAutologin, SIGNAL(timeout()), this, SLOT(startAutoLogin()));
    timerAutologin->start( 1000 );

    connect(mpMainStatusBar, SIGNAL(messageChanged(QString)), this, SLOT(slot_statusBarMessageChanged(QString)));
    // Do something with the QStatusBar just so we "use" it (for 15 seconds)...
    if(  mStatusBarState & statusBarAlwaysShown
      || mStatusBarState & statusBarAutoShown ) {

        mpMainStatusBar->showMessage( tr( "Click on the \"Connect\" button to choose a profile to start... (status bar can be disabled via options once a profile is loaded!)" ), 15000 );
    }
    else {
        mpMainStatusBar->showMessage( tr( "Click on the \"Connect\" button to choose a profile to start... (status bar disabled via options, will not show again this session!)" ), 5000 );
    }
}

bool mudlet::moduleTableVisible()
{
    if (moduleTable)
        return moduleTable->isVisible();
    return false;
}

void mudlet::layoutModules(){
    Host * pH = getActiveHost();
    QMapIterator<QString, QStringList > it (pH->mInstalledModules);
    QStringList sl;
    // The following seems like a non-intuitive operator
    // overload but that is how they do it...
    sl << "Module Name" << "Priority" << "Save & Sync?" << "Module Location";
    moduleTable->setHorizontalHeaderLabels(sl);
    moduleTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    moduleTable->verticalHeader()->hide();
    moduleTable->setShowGrid(true);
    //clear everything
    for(int i=0;i<=moduleTable->rowCount();i++)
        moduleTable->removeRow(i);
    //order modules by priority and then alphabetically
    QMap<int, QStringList> mOrder;
    while( it.hasNext() ){
        it.next();
        int priority = pH->mModulePriorities[it.key()];
        if (mOrder.contains(priority))
            mOrder[priority].append(it.key());
        else
            mOrder[priority] = QStringList(it.key());
    }
    QMapIterator<int, QStringList> it2 (mOrder);
    while(it2.hasNext()){
        it2.next();
        QStringList pModules = it2.value();
        pModules.sort();
        for(int i=0;i<pModules.size();i++){
            int row = moduleTable->rowCount();
            moduleTable->insertRow(row);
            auto masterModule = new QTableWidgetItem ();
            auto itemEntry = new QTableWidgetItem ();
            auto itemLocation = new QTableWidgetItem ();
            auto itemPriority = new QTableWidgetItem ();
            masterModule->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled|Qt::ItemIsSelectable);
            QStringList moduleInfo = pH->mInstalledModules[pModules[i]];
            masterModule->setText("sync?");
            if (moduleInfo[1].toInt())
                masterModule->setCheckState(Qt::Checked);//Qt::Checked);
            else
                masterModule->setCheckState(Qt::Unchecked);//Qt::Checked);
            masterModule->setToolTip(QString("Checking this box will cause the module to be saved & resync'd across all open sessions."));

            QString moduleName = pModules[i];
            itemEntry->setText(moduleName);
            itemEntry->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
            itemLocation->setText(moduleInfo[0]);
            itemLocation->setToolTip(moduleInfo[0]); // show the full path in a tooltip, in case it doesn't fit in the table
            itemLocation->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled); // disallow editing of module path, because that is not saved
            itemPriority->setData(Qt::EditRole, pH->mModulePriorities[moduleName]);
            moduleTable->setItem(row, 0, itemEntry);
            moduleTable->setItem(row, 1, itemPriority);
            moduleTable->setItem(row, 2, masterModule);
            moduleTable->setItem(row, 3, itemLocation);
        }
    }
    moduleTable->resizeColumnsToContents();
}

void mudlet::slot_module_manager(){
    Host * pH = getActiveHost();
    if( ! pH ) {
        return;
    }

    if( !mpModuleDlg ) {
        QUiLoader loader;
        QFile file(":/ui/module_manager.ui");
        file.open(QFile::ReadOnly);
        mpModuleDlg = dynamic_cast<QDialog *>(loader.load(&file, this));
        file.close();

        if( !mpModuleDlg ) {
            return;
        }

        moduleTable = mpModuleDlg->findChild<QTableWidget *>("moduleTable");
        moduleUninstallButton = mpModuleDlg->findChild<QPushButton *>("uninstallButton");
        moduleInstallButton = mpModuleDlg->findChild<QPushButton *>("installButton");
        moduleHelpButton = mpModuleDlg->findChild<QPushButton *>("helpButton");

        if( !moduleTable || !moduleUninstallButton || !moduleHelpButton) {
            return;
        }

        layoutModules();
        connect(moduleUninstallButton, SIGNAL(clicked()), this, SLOT(slot_uninstall_module()));
        connect(moduleInstallButton, SIGNAL(clicked()), this, SLOT(slot_install_module()));
        connect(moduleHelpButton, SIGNAL(clicked()), this, SLOT(slot_help_module()));
        connect(moduleTable, SIGNAL(itemClicked(QTableWidgetItem*)), this, SLOT(slot_module_clicked(QTableWidgetItem*)));
        connect(moduleTable, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(slot_module_changed(QTableWidgetItem*)));
        mpModuleDlg->setWindowTitle(tr("Module Manager"));
        mpModuleDlg->setAttribute( Qt::WA_DeleteOnClose );
    }

    mpModuleDlg->raise();
    mpModuleDlg->show();
}

bool mudlet::openWebPage(const QString& path){
    if (path.isEmpty() || path.isNull())
        return false;
    QUrl url(path,QUrl::TolerantMode);
    if (!url.isValid())
        return false;
    return QDesktopServices::openUrl(url);
}

void mudlet::slot_help_module(){
    Host * pH = getActiveHost();
    if (!pH)
        return;
    int cRow = moduleTable->currentRow();
    QTableWidgetItem * pI = moduleTable->item(cRow, 0);
    if (!pI)
        return;
    if (pH->moduleHelp[pI->text()].contains("helpURL") && !pH->moduleHelp[pI->text()]["helpURL"].isEmpty()){
        if (!openWebPage(pH->moduleHelp[pI->text()]["helpURL"])){
            //failed first open, try for a module related path
            QTableWidgetItem * item = moduleTable->item(cRow,3);
            QString itemPath = item->text();
            QStringList path = itemPath.split(QDir::separator());
            path.pop_back();
            path.append(QDir::separator());
            path.append(pH->moduleHelp[pI->text()]["helpURL"]);
            QString path2 = path.join("");
            if (!openWebPage(path2))
                moduleHelpButton->setDisabled(true);
        }
    }
}


void mudlet::slot_module_clicked(QTableWidgetItem* pItem){
    int i = pItem->row();
    Host * pH = getActiveHost();
    QTableWidgetItem * entry = moduleTable->item(i,0);
    QTableWidgetItem * checkStatus = moduleTable->item(i,2);
    QTableWidgetItem * itemPriority = moduleTable->item(i,1);
    QTableWidgetItem * itemPath = moduleTable->item(i,3);
    qDebug()<<itemPath->text();
    if (!entry || !checkStatus || !itemPriority || !pH->mInstalledModules.contains(entry->text())){
        moduleHelpButton->setDisabled(true);
        if (checkStatus) {
            checkStatus->setCheckState(Qt::Unchecked);
            checkStatus->setFlags(Qt::NoItemFlags);
        }
        return;
    }
    if (pH->moduleHelp.contains(entry->text()))
        moduleHelpButton->setDisabled((!pH->moduleHelp[entry->text()].contains("helpURL") || pH->moduleHelp[entry->text()]["helpURL"].isEmpty()));
    else
        moduleHelpButton->setDisabled(true);
}

void mudlet::slot_module_changed(QTableWidgetItem* pItem){
    int i = pItem->row();
    Host * pH = getActiveHost();
    QStringList moduleStringList;
    QTableWidgetItem * entry = moduleTable->item(i,0);
    QTableWidgetItem * checkStatus = moduleTable->item(i,2);
    QTableWidgetItem * itemPriority = moduleTable->item(i,1);
    if (!entry || !checkStatus || !itemPriority || !pH->mInstalledModules.contains(entry->text()))
        return;
    moduleStringList = pH->mInstalledModules[entry->text()];
    if (checkStatus->checkState() == Qt::Unchecked){
        moduleStringList[1] = "0";
        checkStatus->setText("don't sync");
    }
    if (checkStatus->checkState() == Qt::Checked){
        moduleStringList[1] = "1";
        checkStatus->setText("sync");
    }
    pH->mInstalledModules[entry->text()] = moduleStringList;
    pH->mModulePriorities[entry->text()] = itemPriority->text().toInt();
}

void mudlet::slot_install_module()
{

    QString fileName = QFileDialog::getOpenFileName(this, tr("Load Mudlet Module"),
                                                    QDir::currentPath());
    if( fileName.isEmpty() ) return;

    QFile file(fileName);
    if( ! file.open(QFile::ReadOnly | QFile::Text) )
    {
        QMessageBox::warning(this, tr("Load Mudlet Module:"),
                             tr("Cannot read file %1:\n%2.")
                             .arg(fileName, file.errorString()));
        return;
    }

    Host * pH = getActiveHost();
    if( ! pH ) return;
    pH->installPackage( fileName, 1);
    for (int i=moduleTable->rowCount()-1; i >= 0; --i)
        moduleTable->removeRow(i);
    layoutModules();
}

void mudlet::slot_uninstall_module()
{
    Host * pH = getActiveHost();
    if( ! pH ) return;
    int cRow = moduleTable->currentRow();
    QTableWidgetItem * pI = moduleTable->item(cRow, 0);
    if( pI )
        pH->uninstallPackage( pI->text(), 1);
    for (int i=moduleTable->rowCount()-1; i >= 0; --i)
        moduleTable->removeRow(i);
    layoutModules();
}

void mudlet::slot_package_manager()
{
    Host * pH = getActiveHost();
    if( ! pH ) {
        return;
    }

    if( ! mpPackageManagerDlg ) {
        QUiLoader loader;
        QFile file(":/ui/package_manager.ui");
        file.open(QFile::ReadOnly);
        mpPackageManagerDlg = dynamic_cast<QDialog *>(loader.load(&file, this));
        file.close();

        if( !mpPackageManagerDlg ) {
            return;
        }

        packageList = mpPackageManagerDlg->findChild<QListWidget *>("packageList");
        uninstallButton = mpPackageManagerDlg->findChild<QPushButton *>("uninstallButton");
        installButton = mpPackageManagerDlg->findChild<QPushButton *>("installButton");

        if( ! packageList || ! uninstallButton ) {
            return;
        }

        packageList->addItems( pH->mInstalledPackages );
        connect(uninstallButton, SIGNAL(clicked()), this, SLOT(slot_uninstall_package()));
        connect(installButton, SIGNAL(clicked()), this, SLOT(slot_install_package()));
        mpPackageManagerDlg->setWindowTitle(tr("Package Manager"));
        mpPackageManagerDlg->setAttribute( Qt::WA_DeleteOnClose );
    }

    mpPackageManagerDlg->raise();
    mpPackageManagerDlg->show();
}

void mudlet::slot_install_package()
{

    QString fileName = QFileDialog::getOpenFileName(this, tr("Import Mudlet Package"),
                                                    QDir::currentPath());
    if( fileName.isEmpty() ) return;

    QFile file(fileName);
    if( ! file.open(QFile::ReadOnly | QFile::Text) )
    {
        QMessageBox::warning(this, tr("Import Mudlet Package:"),
                             tr("Cannot read file %1:\n%2.")
                             .arg(fileName, file.errorString()));
        return;
    }

    Host * pH = getActiveHost();
    if( ! pH ) return;

    pH->installPackage( fileName, 0);
    packageList->clear();
    packageList->addItems( pH->mInstalledPackages );
}

void mudlet::slot_uninstall_package()
{
    Host * pH = getActiveHost();
    if( ! pH ) return;
    QListWidgetItem * pI = packageList->currentItem();
    if( pI )
        pH->uninstallPackage( pI->text(), 0);
    packageList->clear();
    packageList->addItems( pH->mInstalledPackages );
}

void mudlet::slot_package_exporter(){
    Host * pH = getActiveHost();
    if( ! pH ) return;
    auto d = new dlgPackageExporter(this, pH);
    // don't show the dialog if the user cancelled the wizard
    if (d->filePath.isEmpty()) {
        return;
    }

    d->show();
}


void mudlet::slot_close_profile_requested( int tab )
{
    QString name = mpTabBar->tabText( tab );
    Host* pH = getHostManager().getHost(name);
    if( ! pH ) return;

    QMap<QString, TDockWidget *> & dockWindowMap = mHostDockConsoleMap[pH];
    QMap<QString, TConsole*> & hostConsoleMap = mHostConsoleMap[pH];

    if( ! pH->mpConsole->close() )
        return;
    else
        pH->mpConsole->mUserAgreedToCloseConsole = true;
    pH->closingDown();
    pH->stopAllTriggers();
    pH->mpEditorDialog->close();
    for( auto consoleName : hostConsoleMap.keys() ) {
        hostConsoleMap[consoleName]->close();
        hostConsoleMap.remove(consoleName);

        if( dockWindowMap.contains(consoleName) ) {
            dockWindowMap[consoleName]->setAttribute( Qt::WA_DeleteOnClose );
            dockWindowMap[consoleName]->close();
            removeDockWidget( dockWindowMap[consoleName] );
            dockWindowMap.remove(consoleName);
        }
    }
    mHostDockConsoleMap.remove(pH);
    mHostConsoleMap.remove(pH);

    mConsoleMap[pH]->close();
    if( mTabMap.contains( pH->getName() ) )
    {
        mpTabBar->removeTab( tab );
        mConsoleMap.remove( pH );
        mTabMap.remove( pH->getName() );
        getHostManager().deleteHost(pH->getName());
    }

    // hide the tab bar if we only have 1 or no tabs available. saves screen space.
    if( mConsoleMap.size() > 1 )
    {
        mpTabBar->show();
    }
    else
        mpTabBar->hide();

}

void mudlet::slot_close_profile()
{
    if( mpCurrentActiveHost )
    {
        if( mConsoleMap.contains( mpCurrentActiveHost ) )
        {
            Host * pH = mpCurrentActiveHost;
            if( pH )
            {
                QMap<QString, TDockWidget *> & dockWindowMap = mHostDockConsoleMap[pH];
                QMap<QString, TConsole*> & hostConsoleMap = mHostConsoleMap[pH];
                QString name = pH->getName();

                pH->closingDown();
                mpCurrentActiveHost->mpEditorDialog->close();
                for( auto consoleName : hostConsoleMap.keys() ) {
                    hostConsoleMap[consoleName]->close();
                    hostConsoleMap.remove(consoleName);

                    if( dockWindowMap.contains(consoleName) ) {
                        dockWindowMap[consoleName]->setAttribute( Qt::WA_DeleteOnClose );
                        dockWindowMap[consoleName]->close();
                        removeDockWidget( dockWindowMap[consoleName] );
                        dockWindowMap.remove(consoleName);
                    }
                }
                mHostDockConsoleMap.remove(pH);
                mHostConsoleMap.remove(pH);

                mConsoleMap[ pH ]->close();
                if( mTabMap.contains( name ) )
                {
                    mpTabBar->removeTab( mpTabBar->currentIndex() );
                    mConsoleMap.remove( pH );
                    getHostManager().deleteHost(name);
                    mTabMap.remove( name );
                }
                mpCurrentActiveHost = Q_NULLPTR;
            }
        }

    }
    else
    {
        disableToolbarButtons();
    }
}

void mudlet::slot_tab_changed( int tabID )
{
    if( ( ! mTabMap.contains( mpTabBar->tabText( tabID ) ) ) && ( tabID != -1 ) )
    {
        mpCurrentActiveHost = 0;
        return;
    }

    if( mConsoleMap.contains( mpCurrentActiveHost ) )
    {
        mpCurrentActiveHost->mpConsole->hide();
        QString host = mpTabBar->tabText( tabID );
        if( mTabMap.contains( host ) )
        {
            mpCurrentActiveHost = mTabMap[host]->mpHost;
        }
        else
        {
            mpCurrentActiveHost = 0;
            return;
        }
    }
    else
    {
        if( mTabMap.size() > 0 )
        {
            mpCurrentActiveHost = mTabMap.begin().value()->mpHost;
        }
        else
        {
            mpCurrentActiveHost = 0;
            return;
        }
    }

    if( ! mpCurrentActiveHost || mConsoleMap.contains( mpCurrentActiveHost ) )
    {
        if( ! mpCurrentActiveHost ) return;
        mpCurrentActiveHost->mpConsole->show();
        mpCurrentActiveHost->mpConsole->repaint();
        mpCurrentActiveHost->mpConsole->refresh();
        mpCurrentActiveHost->mpConsole->mpCommandLine->repaint();
        mpCurrentActiveHost->mpConsole->mpCommandLine->setFocus();
        mpCurrentActiveHost->mpConsole->show();

        int x = mpCurrentActiveHost->mpConsole->width();
        int y = mpCurrentActiveHost->mpConsole->height();
        QSize s = QSize(x,y);
        QResizeEvent event(s, s);
        QApplication::sendEvent( mpCurrentActiveHost->mpConsole, &event);
    }
    else
    {
        mpCurrentActiveHost = 0;
        return;
    }

    // update the window title for the currently selected profile
    setWindowTitle(mpCurrentActiveHost->getName() + " - " + version);
}

void mudlet::addConsoleForNewHost( Host * pH )
{
    if( mConsoleMap.contains( pH ) ) return;
    pH->mLogStatus = mAutolog;
    auto pConsole = new TConsole( pH, false );
    if( ! pConsole ) return;
    pH->mpConsole = pConsole;
    pConsole->setWindowTitle( pH->getName() );
    pConsole->setObjectName( pH->getName() );
    mConsoleMap[pH] = pConsole;
    int newTabID = mpTabBar->addTab( pH->getName() );
    mTabMap[pH->getName()] = pConsole;
    if( mConsoleMap.size() > 1 )
    {
        mpTabBar->show();
    }
    else
        mpTabBar->hide();
    //update the main window title when we spawn a new tab
    setWindowTitle(pH->getName() + " - " + version);

    mainPane->layout()->addWidget( pConsole );
    if( mpCurrentActiveHost )
        mpCurrentActiveHost->mpConsole->hide();
    mpCurrentActiveHost = pH;
    if( pH->mLogStatus ) pConsole->logButton->click();

    pConsole->show();
    connect( pConsole->emergencyStop, SIGNAL(pressed()), this , SLOT(slot_stopAllTriggers()));

    auto pEditor = new dlgTriggerEditor( pH );
    pH->mpEditorDialog = pEditor;
    pEditor->fillout_form();

    pH->getActionUnit()->updateToolbar();
    QMap<QString, TDockWidget*> dockConsoleMap;
    mHostDockConsoleMap[mpCurrentActiveHost] = dockConsoleMap;
    QMap<QString, TConsole *> miniConsoleMap;
    mHostConsoleMap[mpCurrentActiveHost] = miniConsoleMap;
    QMap<QString, TLabel *> labelMap;
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
    mpTabBar->setCurrentIndex( newTabID );
    int x = mpCurrentActiveHost->mpConsole->width();
    int y = mpCurrentActiveHost->mpConsole->height();
    QSize s = QSize(x,y);
    QResizeEvent event(s, s);
    QApplication::sendEvent( mpCurrentActiveHost->mpConsole, &event);
}


void mudlet::slot_timer_fires()
{
    QTimer * pQT = (QTimer*)sender();
    if( ! pQT ) return;
    if( mTimerMap.contains( pQT ) )
    {
        TTimer * pTT = mTimerMap[pQT];
        pTT->execute();
        if( pTT->checkRestart()  )
        {
            pTT->start();
        }
    }
    else
    {
        qDebug()<<"MUDLET CRITICAL ERROR: Timer not registered!";
    }
}

void mudlet::unregisterTimer( QTimer * pQT )
{
    if( mTimerMap.contains( pQT ) )
    {
        mTimerMap.remove( pQT );
    }
    else
    {
        qDebug()<<"MUDLET CRITICAL ERROR: trying to unregister Timer but it is not registered!";
    }
}

void mudlet::registerTimer( TTimer * pTT, QTimer * pQT )
{
    if( ! mTimerMap.contains( pQT ) )
    {
        mTimerMap[pQT] = pTT;
        connect(pQT, SIGNAL(timeout()), this,SLOT(slot_timer_fires()));
    }
}

void mudlet::disableToolbarButtons()
{
    mpMainToolBar->actions()[1]->setEnabled( false );
    mpMainToolBar->actions()[2]->setEnabled( false );
    mpMainToolBar->actions()[3]->setEnabled( false );
    mpMainToolBar->actions()[4]->setEnabled( false );
    mpMainToolBar->actions()[5]->setEnabled( false );
    mpMainToolBar->actions()[6]->setEnabled( false );
    mpMainToolBar->actions()[7]->setEnabled( false );
    mpMainToolBar->actions()[9]->setEnabled( false );
    mpMainToolBar->actions()[10]->setEnabled( false );
    mpMainToolBar->actions()[11]->setEnabled( false );
    mpMainToolBar->actions()[12]->setEnabled( false );
    mpMainToolBar->actions()[13]->setEnabled( false );
    mpMainToolBar->actions()[14]->setEnabled( false );
}

void mudlet::enableToolbarButtons()
{
    mpMainToolBar->actions()[1]->setEnabled( true );
    mpMainToolBar->actions()[2]->setEnabled( true );
    mpMainToolBar->actions()[3]->setEnabled( true );
    mpMainToolBar->actions()[4]->setEnabled( true );
    mpMainToolBar->actions()[5]->setEnabled( true );
    mpMainToolBar->actions()[6]->setEnabled( true );
    mpMainToolBar->actions()[7]->setEnabled( true );
    mpMainToolBar->actions()[9]->setEnabled( true );
    mpMainToolBar->actions()[10]->setEnabled( true );
    mpMainToolBar->actions()[11]->setEnabled( true );
    mpMainToolBar->actions()[12]->setEnabled( true );
    mpMainToolBar->actions()[13]->setEnabled( true );
    mpMainToolBar->actions()[14]->setEnabled( true );
}

bool mudlet::saveWindowLayout() {
    qDebug() << "mudlet::saveWindowLayout() - Already-Saved:" << mHasSavedLayout;
    if( mHasSavedLayout ) { return false; }

    QString layoutFilePath = QStringLiteral("%1/.config/mudlet/windowLayout.dat").arg(QDir::homePath());

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

bool mudlet::loadWindowLayout() {
    qDebug() << "mudlet::loadWindowLayout() - loading layout.";

    QString layoutFilePath = QStringLiteral("%1/.config/mudlet/windowLayout.dat").arg(QDir::homePath());

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

void mudlet::setDockLayoutUpdated(Host* pHost, const QString & name) {
    if( !pHost ) return;

    QMap<QString, TDockWidget *> & dockWindowMap = mHostDockConsoleMap[pHost];
    QList<QString> & mDockLayoutUpdateMap = mHostDockLayoutChangeMap[pHost];
    if( !mDockLayoutUpdateMap.contains(name) && dockWindowMap.contains(name) ) {
        dockWindowMap[name]->setObjectName( QString("%1_changed").arg(dockWindowMap[name]->objectName()) );
        mDockLayoutUpdateMap.append(name);

        mHasSavedLayout = false;
    }
}

void mudlet::setToolbarLayoutUpdated(Host* pHost, TToolBar * pTB) {
    if( !pHost ) return;

    QList<TToolBar*> & mToolbarLayoutUpdateMap = mHostToolbarLayoutChangeMap[pHost];
    if( !mToolbarLayoutUpdateMap.contains(pTB) ) {
        pTB->setObjectName( QString("%1_changed").arg(pTB->objectName()) );
        mToolbarLayoutUpdateMap.append(pTB);

        mHasSavedLayout = false;
    }
}

void mudlet::commitLayoutUpdates() {
    // commit changes for dockwidget consoles. (user windows)
    for( Host* pHost : mHostDockLayoutChangeMap.keys() ) {
        QMap<QString, TDockWidget *> & dockWindowMap = mHostDockConsoleMap[pHost];
        QList<QString> & mDockLayoutUpdateMap = mHostDockLayoutChangeMap[pHost];

        for( QString TDockName : mDockLayoutUpdateMap ) {
            if( dockWindowMap.contains(TDockName) ) {
                QString rename = QString("dockWindow_%1_%2").arg(pHost->getName(), TDockName);
                dockWindowMap[TDockName]->setObjectName(rename);
            }
            mDockLayoutUpdateMap.removeAll(TDockName);
        }
    }

    // commit changes for dockable/floating toolbars.
    for( Host* pHost : mHostToolbarLayoutChangeMap.keys()  ) {
        QList<TToolBar*> & mToolbarLayoutUpdateMap = mHostToolbarLayoutChangeMap[pHost];

        for( TToolBar* pTB : mToolbarLayoutUpdateMap ) {
            pTB->setObjectName(QString("dockToolBar_%1").arg( pTB->getName() ));
            mToolbarLayoutUpdateMap.removeAll(pTB);
        }
    }
}

bool mudlet::openWindow( Host * pHost, const QString & name, bool loadLayout )
{
    if( ! pHost ) return false;

    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    QMap<QString, TDockWidget *> & dockWindowMap = mHostDockConsoleMap[pHost];

    if( ! dockWindowMap.contains(name) && ! dockWindowConsoleMap.contains(name) )
    {
        auto pD = new TDockWidget(pHost, name);
        pD->setObjectName(QString("dockWindow_%1_%2").arg(pHost->getName(), name));
        pD->setContentsMargins(0,0,0,0);
        pD->setFeatures( QDockWidget::AllDockWidgetFeatures );
        pD->setWindowTitle( QString("%1 - %2").arg(name, pHost->getName()) );
        dockWindowMap[name] = pD;
        auto pC = new TConsole( pHost, false, pD );
        pC->setContentsMargins(0,0,0,0);
        pD->setWidget( pC );
        pC->show();
        pC->layerCommandLine->hide();
        pC->mpScrollBar->hide();
        pC->setUserWindow();
        dockWindowConsoleMap[name] = pC;
        addDockWidget(Qt::RightDockWidgetArea, pD);

        if(loadLayout)
            loadWindowLayout();

        return true;
    } else if( dockWindowMap.contains(name) && dockWindowConsoleMap.contains(name) ) {
        dockWindowMap[name]->update();
        dockWindowMap[name]->show();
        dockWindowConsoleMap[name]->showWindow(name);

        if(loadLayout)
            loadWindowLayout();

        return true;
    }

    return false;
}

bool mudlet::createMiniConsole( Host * pHost, const QString & name, int x, int y, int width, int height )
{
    if( ! pHost ) return false;
    if( ! pHost->mpConsole ) return false;

    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( ! dockWindowConsoleMap.contains( name ) )
    {
        TConsole * pC = pHost->mpConsole->createMiniConsole( name, x, y, width, height );
        pC->mConsoleName = name;
        if( pC )
        {
            dockWindowConsoleMap[name] = pC;
            std::string _n = name.toStdString();
            pC->setMiniConsoleFontSize( _n, 12 );
            return true;
        }
    }
    else
    {
        TConsole * pC = dockWindowConsoleMap[name];
        pC->resize( width, height );
        pC->move( x, y );
    }
    return false;
}

bool mudlet::createLabel( Host * pHost, const QString & name, int x, int y, int width, int height, bool fillBg )
{
    if( ! pHost ) return false;
    if( ! pHost->mpConsole ) return false;
    QMap<QString, TLabel *> & labelMap = mHostLabelMap[pHost];
    if( ! labelMap.contains( name ) )
    {
        TLabel * pL = pHost->mpConsole->createLabel( name, x, y, width, height, fillBg );
        if( pL )
        {
            labelMap[name] = pL;
            return true;
        }
    }
    return false;
}

bool mudlet::createBuffer( Host * pHost, const QString & name )
{
    if( ! pHost ) return false;
    if( ! pHost->mpConsole ) return false;

    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( ! dockWindowConsoleMap.contains( name ) )
    {
        TConsole * pC = pHost->mpConsole->createBuffer( name );
        pC->mConsoleName = name;
        if( pC )
        {
            dockWindowConsoleMap[name] = pC;
            return true;
        }
    }
    return false;
}

bool mudlet::setBackgroundColor( Host * pHost, const QString & name, int r, int g, int b, int alpha )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->setConsoleBgColor(r,g,b);
        return true;
    }
    else
    {
        QMap<QString, TLabel *> & labelMap = mHostLabelMap[pHost];
        if( labelMap.contains( name ) )
        {
            QPalette mainPalette;
            mainPalette.setColor( QPalette::Window, QColor(r, g, b, alpha) );
            labelMap[name]->setPalette( mainPalette );
            return true;
        }
    }
    return false;
}

bool mudlet::setBackgroundImage( Host * pHost, const QString & name, QString & path )
{
    QMap<QString, TLabel *> & labelMap = mHostLabelMap[pHost];
    if( labelMap.contains( name ) )
    {
        if( QDir::homePath().contains('\\') )
        {
            path.replace('/', "\\");
        }
        else
            path.replace('\\', "/");
        QPixmap bgPixmap( path );
        labelMap[name]->setPixmap( bgPixmap );
        return true;
    }
    else
        return false;
}

bool mudlet::setTextFormat( Host * pHost, const QString & name,
                            int r1, int g1, int b1,
                            int r2, int g2, int b2,
                            bool bold, bool underline, bool italics, bool strikeout )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        TConsole * pC = dockWindowConsoleMap[name];
        pC->mFormatCurrent.bgR = r1;
        pC->mFormatCurrent.bgG = g1;
        pC->mFormatCurrent.bgB = b1;
        pC->mFormatCurrent.fgR = r2;
        pC->mFormatCurrent.fgG = g2;
        pC->mFormatCurrent.fgB = b2;
        if( bold )
        {
            pC->mFormatCurrent.flags |= TCHAR_BOLD;
        }
        else
        {
            pC->mFormatCurrent.flags &= ~(TCHAR_BOLD);
        }
        if( underline )
        {
            pC->mFormatCurrent.flags |= TCHAR_UNDERLINE;
        }
        else
        {
            pC->mFormatCurrent.flags &= ~(TCHAR_UNDERLINE);
        }
        if( italics )
        {
            pC->mFormatCurrent.flags |= TCHAR_ITALICS;
        }
        else
        {
            pC->mFormatCurrent.flags &= ~(TCHAR_ITALICS);
        }
        if( strikeout )
        {
            pC->mFormatCurrent.flags |= TCHAR_STRIKEOUT;
        }
        else
        {
            pC->mFormatCurrent.flags &= ~(TCHAR_STRIKEOUT);
        }
        return true;
    }
    else
    {
        return false;
    }
}

void mudlet::showEvent( QShowEvent * event )
{
    mWindowMinimized = false;
    QMainWindow::showEvent( event );
}

void mudlet::hideEvent( QHideEvent * event )
{
    mWindowMinimized = true;
    QMainWindow::hideEvent( event );
}

bool mudlet::clearWindow( Host * pHost, const QString & name )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->buffer.clear();
        dockWindowConsoleMap[name]->console->update();
        return true;
    }
    else
        return false;
}

bool mudlet::showWindow( Host * pHost, const QString & name )
{
    if( ! pHost ) return false;
    if( ! pHost->mpConsole ) return false;

    // check labels first as they are shown/hidden more often
    QMap<QString, TLabel *> & labelMap = mHostLabelMap[pHost];
    if( labelMap.contains( name ) )
    {
        labelMap[name]->show();
        return true;
    }

    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        QMap<QString, TDockWidget *> & dockWindowMap = mHostDockConsoleMap[pHost];
        if( dockWindowMap.contains(name) ) {
            dockWindowMap[name]->update();
            dockWindowMap[name]->show();
            dockWindowConsoleMap[name]->showWindow(name);

            return true;
        }

        return pHost->mpConsole->showWindow(name);
    }

    return false;
}

bool mudlet::paste( Host * pHost, const QString & name )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->paste();
        return true;
    }
    else
        return false;
}

bool mudlet::hideWindow( Host * pHost, const QString & name )
{
    if( ! pHost ) return false;
    if( ! pHost->mpConsole ) return false;

    // check labels first as they are shown/hidden more often
    QMap<QString, TLabel *> & labelMap = mHostLabelMap[pHost];
    if( labelMap.contains( name ) )
    {
        labelMap[name]->hide();
        return true;
    }

    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        QMap<QString, TDockWidget *> & dockWindowMap = mHostDockConsoleMap[pHost];
        if( dockWindowMap.contains(name) ) {
            dockWindowMap[name]->hide();
            dockWindowMap[name]->update();
            dockWindowConsoleMap[name]->hideWindow(name);

            return true;
        }

        return pHost->mpConsole->hideWindow(name);
    }

    return false;
}

bool mudlet::resizeWindow( Host * pHost, const QString & name, int x1, int y1 )
{
    QMap<QString, TLabel *> & labelMap = mHostLabelMap[pHost];
    if( labelMap.contains( name ) )
    {
        labelMap[name]->resize( x1, y1 );
        return true;
    }

    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->resize( x1, y1 );
        return true;
    }

    return false;
}

bool mudlet::setConsoleBufferSize( Host * pHost, const QString & name, int x1, int y1 )
{
    if( name == "main" )
    {
        pHost->mpConsole->buffer.setBufferSize( x1, y1 );
        return true;
    }

    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];

    if( dockWindowConsoleMap.contains( name ) )
    {
        (dockWindowConsoleMap[name]->buffer).setBufferSize( x1, y1 );
        return true;
    }
    else
        return false;
}

bool mudlet::resetFormat( Host * pHost, QString & name )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) ) {
        dockWindowConsoleMap[name]->reset();
        return true;
    }
    else {
        return false;
    }
}

bool mudlet::moveWindow( Host * pHost, const QString & name, int x1, int y1 )
{
    QMap<QString, TLabel *> & labelMap = mHostLabelMap[pHost];
    if( labelMap.contains( name ) )
    {
        labelMap[name]->move( x1, y1 );
        return true;
    }

    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->move( x1, y1 );
        dockWindowConsoleMap[name]->mOldX = x1;
        dockWindowConsoleMap[name]->mOldY = y1;
        return true;
    }

    return false;
}

bool mudlet::closeWindow( Host * pHost, const QString & name )
{
    if( ! pHost ) return false;
    if( ! pHost->mpConsole ) return false;

    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains(name) )
    {
        QMap<QString, TDockWidget *> & dockWindowMap = mHostDockConsoleMap[pHost];
        if( dockWindowMap.contains(name) ) {
            dockWindowMap[name]->hide();
            dockWindowMap[name]->update();
            dockWindowConsoleMap[name]->hideWindow(name);

            return true;
        }

        return pHost->mpConsole->hideWindow(name);
    }
    else
        return false;
}

bool mudlet::setLabelClickCallback( Host * pHost, const QString & name, const QString & func, const TEvent & pA )
{
    QMap<QString, TLabel *> & labelMap = mHostLabelMap[pHost];
    if( labelMap.contains( name ) )
    {
        labelMap[name]->setClick( pHost, func, pA );
        return true;
    }
    else
        return false;
}

bool mudlet::setLabelReleaseCallback( Host * pHost, const QString & name, const QString & func, const TEvent & pA )
{
    QMap<QString, TLabel *> & labelMap = mHostLabelMap[pHost];
    if( labelMap.contains( name ) )
    {
        labelMap[name]->setRelease( pHost, func, pA );
        return true;
    }
    else
        return false;
}

bool mudlet::setLabelOnEnter( Host * pHost, const QString & name, const QString & func, const TEvent & pA )
{
    QMap<QString, TLabel *> & labelMap = mHostLabelMap[pHost];
    if( labelMap.contains( name ) )
    {
        labelMap[name]->setEnter( pHost, func, pA );
        return true;
    }
    else
        return false;
}

bool mudlet::setLabelOnLeave( Host * pHost, const QString & name, const QString & func, const TEvent & pA )
{
    QMap<QString, TLabel *> & labelMap = mHostLabelMap[pHost];
    if( labelMap.contains( name ) )
    {
        labelMap[name]->setLeave( pHost, func, pA );
        return true;
    }
    else
        return false;
}

int mudlet::getLineNumber( Host * pHost, QString & name )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        return dockWindowConsoleMap[name]->getLineNumber();
    }
    else
    {
        TDebug(QColor(Qt::white),QColor(Qt::red))<<"ERROR: window doesnt exit\n" >> 0;
    }
    return -1;
}

int mudlet::getColumnNumber( Host * pHost, QString & name )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        return dockWindowConsoleMap[name]->getColumnNumber();
    }
    else
    {
        TDebug(QColor(Qt::white),QColor(Qt::red))<<"ERROR: window doesnt exit\n" >> 0;
    }
    return -1;
}


int mudlet::getLastLineNumber( Host * pHost, const QString & name )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        return dockWindowConsoleMap[name]->getLastLineNumber();
    }
    else
    {
        TDebug(QColor(Qt::white),QColor(Qt::red))<<"ERROR: window doesnt exit\n" >> 0;
    }
    return -1;
}

bool mudlet::moveCursorEnd( Host * pHost, const QString & name )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->moveCursorEnd();
        return true;
    }
    else
        return false;
}

bool mudlet::moveCursor( Host * pHost, const QString & name, int x, int y )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        return dockWindowConsoleMap[name]->moveCursor( x, y );
    }
    else
        return false;
}

void mudlet::deleteLine( Host * pHost, const QString & name )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->skipLine();
    }
}

void mudlet::insertText( Host * pHost, const QString & name, const QString& text )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->insertText( text );
    }
}

void mudlet::insertLink( Host * pHost, const QString & name, const QString& text, QStringList & func, QStringList & hint, bool customFormat )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->insertLink( text, func, hint, customFormat );
    }
}

void mudlet::replace( Host * pHost, const QString & name, const QString& text )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->replace( text );
    }
}

void mudlet::setLink( Host * pHost, const QString & name, const QString & linkText, QStringList & linkFunction, QStringList & linkHint )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        TConsole * pC = dockWindowConsoleMap[name];
        pC->setLink( linkText, linkFunction, linkHint );
    }
}

void mudlet::setBold( Host * pHost, const QString & name, bool b )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        TConsole * pC = dockWindowConsoleMap[name];
        pC->setBold( b );
    }
}

void mudlet::setItalics( Host * pHost, const QString & name, bool b )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        TConsole * pC = dockWindowConsoleMap[name];
        pC->setItalics( b );
    }
}

void mudlet::setUnderline( Host * pHost, const QString & name, bool b )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        TConsole * pC = dockWindowConsoleMap[name];
        pC->setUnderline( b );
    }
}

void mudlet::setStrikeOut( Host * pHost, const QString & name, bool b )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        TConsole * pC = dockWindowConsoleMap[name];
        pC->setStrikeOut( b );
    }
}

void mudlet::setFgColor( Host * pHost, const QString & name, int r, int g, int b )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        TConsole * pC = dockWindowConsoleMap[name];
        pC->setFgColor( r, g, b );
    }
}

void mudlet::setBgColor( Host * pHost, const QString & name, int r, int g, int b )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        TConsole * pC = dockWindowConsoleMap[name];
        pC->setBgColor( r, g, b );
    }
}

int mudlet::selectString( Host * pHost, const QString & name, const QString& text, int num )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        TConsole * pC = dockWindowConsoleMap[name];
        return pC->select( text, num );
    }
    else
        return -1;
}

int mudlet::selectSection( Host * pHost, const QString & name, int f, int t )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        TConsole * pC = dockWindowConsoleMap[name];
        return pC->selectSection( f, t );
    }
    else
        return -1;
}

// Added a return value to indicate whether the given windows name was found
bool mudlet::deselect( Host * pHost, const QString & name )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) ) {
        TConsole * pC = dockWindowConsoleMap[name];
        pC->deselect();
        return true;
    }
    else {
        return false;
    }
}

bool mudlet::setWindowWrap( Host * pHost, const QString & name, int & wrap )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    QString wn = name;
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[wn]->setWrapAt( wrap );
        return true;
    }
    else
        return false;
}

bool mudlet::setWindowWrapIndent( Host * pHost, const QString & name, int & wrap )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    QString wn = name;
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[wn]->setIndentCount( wrap );
        return true;
    }
    else
        return false;
}

bool mudlet::echoWindow( Host * pHost, const QString & name, const QString & text )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    QString t = text;
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->echoUserWindow( t );
        return true;
    }
    QMap<QString, TLabel *> & labelMap = mHostLabelMap[pHost];
    if( labelMap.contains( name ) )
    {
        labelMap[name]->setText( t );
        return true;
    }
    else
        return false;
}

bool mudlet::echoLink( Host * pHost, const QString & name, const QString & text, QStringList & func, QStringList & hint, bool customFormat )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    QString t = text;
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->echoLink( text, func, hint, customFormat );
        return true;
    }
    else
        return false;
}

bool mudlet::copy( Host * pHost, const QString & name )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->copy();
        return true;
    }
    else return false;
}

bool mudlet::pasteWindow( Host * pHost, const QString & name )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->pasteWindow( mConsoleMap[pHost]->mClipboard );
        return true;
    }
    else return false;
}

bool mudlet::appendBuffer( Host * pHost, const QString & name )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->appendBuffer( mConsoleMap[pHost]->mClipboard );
        return true;
    }
    else return false;
}

void mudlet::slot_userToolBar_hovered( QAction* pA )
{
    QStringList sL;
    if( pA->menu() )
    {
        pA->menu()->exec();
    }


}

void mudlet::slot_userToolBar_orientation_changed( Qt::Orientation dir )
{
}

Host * mudlet::getActiveHost()
{
    if( mConsoleMap.contains( mpCurrentActiveHost ) )
    {
        return mpCurrentActiveHost;
    }
    else
        return 0;
}

void mudlet::addSubWindow( TConsole* pConsole )
{
    mainPane->layout()->addWidget( pConsole );
    pConsole->show();//NOTE: this is important for Apple OSX otherwise the console isnt displayed
}

void mudlet::closeEvent(QCloseEvent *event)
{
    foreach( TConsole * pC, mConsoleMap )
    {
        if( ! pC->close() )
        {
            event->ignore();
            return;
        }
        else
            pC->mUserAgreedToCloseConsole = true;
    }

    writeSettings();

    goingDown();
    if( mpDebugConsole )
    {
        mpDebugConsole->close();
    }
    foreach( TConsole * pC, mConsoleMap )
    {
        if( pC->mpHost->getName() != "default_host" )
        {
            // close script-editor
            if( pC->mpHost->mpEditorDialog )
            {
                pC->mpHost->mpEditorDialog->setAttribute( Qt::WA_DeleteOnClose );
                pC->mpHost->mpEditorDialog->close();
            }
            if( pC->mpHost->mpNotePad )
            {
                pC->mpHost->mpNotePad->save();
                pC->mpHost->mpNotePad->setAttribute( Qt::WA_DeleteOnClose );
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

void mudlet::readSettings()
{
    /*In case sensitive environments, two different config directories
    were used: "Mudlet" for QSettings, and "mudlet" anywhere else.
    Furthermore, we skip the version from the application name to follow the convention.
    For compatibility with older settings, if no config is loaded
    from the config directory "mudlet", application "Mudlet", we try to load from the config
    directory "Mudlet", application "Mudlet 1.0". */
    QSettings settings_new("mudlet","Mudlet");
    QSettings settings((settings_new.contains("pos")? "mudlet":"Mudlet"),(settings_new.contains("pos")? "Mudlet":"Mudlet 1.0"));

    QPoint pos = settings.value("pos", QPoint(0, 0)).toPoint();
    QSize size = settings.value("size", QSize(750, 550)).toSize();
    mMainIconSize = settings.value("mainiconsize",QVariant(3)).toInt();
    mTEFolderIconSize = settings.value("tefoldericonsize", QVariant(3)).toInt();
    mShowMenuBar = settings.value("showMenuBar",QVariant(0)).toBool();
    mShowToolbar = settings.value("showToolbar",QVariant(0)).toBool();
    mEditorTextOptions = QTextOption::Flags( settings.value( "editorTextOptions",QVariant(0)).toInt() );

    // By default the status bar will not be shown for new/upgraded
    // installations - if the user wants the status bar shown either all the
    // time or when it has something to show, they will have to enable that
    // themselves, but that only has to be done once! - Slysven
    mStatusBarState = StatusBarOptions( settings.value( "statusBarOptions", statusBarHidden ).toInt() );

    mIsToDisplayMapAuditErrorsToConsole = settings.value( "reportMapIssuesToConsole", QVariant(false)).toBool();
    resize( size );
    move( pos );
    setIcoSize( mMainIconSize );
    if( mShowMenuBar )
        MenuBar->show();
    else
        MenuBar->hide();
    if( mShowToolbar || ! mShowMenuBar )
    {
        mpMainToolBar->show();
    }
    else
    {
        if( mShowMenuBar )
        {
            mpMainToolBar->hide();
        }
        else
            mpMainToolBar->show();
    }
    if( settings.value("maximized", false).toBool() )
    {
        showMaximized();
    }
}

void mudlet::setIcoSize( int s )
{
    mpMainToolBar->setIconSize(QSize(s*8,s*8));
    if( mMainIconSize > 2 )
        mpMainToolBar->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );
    else
        mpMainToolBar->setToolButtonStyle( Qt::ToolButtonIconOnly );
    if( mShowMenuBar )
        menuBar()->show();
    else
        menuBar()->hide();
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
    settings.setValue("tefoldericonsize",mTEFolderIconSize);
    settings.setValue("showMenuBar", mShowMenuBar );
    settings.setValue("showToolbar", mShowToolbar );
    settings.setValue("maximized", isMaximized());
    settings.setValue("editorTextOptions", static_cast<int>(mEditorTextOptions) );
    settings.setValue("statusBarOptions", static_cast<int>(mStatusBarState) );
    settings.setValue("reportMapIssuesToConsole", mIsToDisplayMapAuditErrorsToConsole );
}

void mudlet::connectToServer()
{
    auto pDlg = new dlgConnectionProfiles(this);
    connect (pDlg, SIGNAL (signal_establish_connection( QString, int )), this, SLOT (slot_connection_dlg_finnished(QString, int)));
    pDlg->fillout_form();
    if (pDlg->exec() == QDialog::Accepted) {
         enableToolbarButtons();
    }
}

void mudlet::show_trigger_dialog()
{
    Host * pHost = getActiveHost();
    if( ! pHost ) return;
    dlgTriggerEditor * pEditor = pHost->mpEditorDialog;
    if( ! pEditor ) return;
    pEditor->slot_show_triggers();
    pEditor->raise();
    pEditor->showNormal();
}

void mudlet::show_alias_dialog()
{
    Host * pHost = getActiveHost();
    if( ! pHost ) return;
    dlgTriggerEditor * pEditor = pHost->mpEditorDialog;
    if( ! pEditor ) return;
    pEditor->slot_show_aliases();
    pEditor->raise();
    pEditor->showNormal();
}

void mudlet::show_timer_dialog()
{
    Host * pHost = getActiveHost();
    if( ! pHost ) return;
    dlgTriggerEditor * pEditor = pHost->mpEditorDialog;
    if( ! pEditor ) return;
    pEditor->slot_show_timers();
    pEditor->raise();
    pEditor->showNormal();
}

void mudlet::show_script_dialog()
{
    Host * pHost = getActiveHost();
    if( ! pHost ) return;
    dlgTriggerEditor * pEditor = pHost->mpEditorDialog;
    if( ! pEditor ) return;
    pEditor->slot_show_scripts();
    pEditor->raise();
    pEditor->showNormal();
}

void mudlet::show_key_dialog()
{
    Host * pHost = getActiveHost();
    if( ! pHost ) return;
    dlgTriggerEditor * pEditor = pHost->mpEditorDialog;
    if( ! pEditor ) return;
    pEditor->slot_show_keys();
    pEditor->raise();
    pEditor->showNormal();
}

void mudlet::show_variable_dialog()
{
    Host * pHost = getActiveHost();
    if( ! pHost ) return;
    dlgTriggerEditor * pEditor = pHost->mpEditorDialog;
    if( ! pEditor ) return;
    pEditor->slot_show_vars();
    pEditor->raise();
    pEditor->showNormal();
}

void mudlet::show_action_dialog()
{
    Host * pHost = getActiveHost();
    if( ! pHost ) return;
    dlgTriggerEditor * pEditor = pHost->mpEditorDialog;
    if( ! pEditor ) return;
    pEditor->slot_show_actions();
    pEditor->raise();
    pEditor->showNormal();
}

void mudlet::show_options_dialog()
{
    Host * pHost = getActiveHost();
    if( ! pHost ) {
        return;
    }

    if( ! mpProfilePreferencesDlg ) {
        mpProfilePreferencesDlg = new dlgProfilePreferences( this, pHost );
        connect(actionReconnect, SIGNAL(triggered()), mpProfilePreferencesDlg->need_reconnect_for_data_protocol, SLOT(hide()));
        connect(dactionReconnect, SIGNAL(triggered()), mpProfilePreferencesDlg->need_reconnect_for_data_protocol, SLOT(hide()));
        connect(actionReconnect, SIGNAL(triggered()), mpProfilePreferencesDlg->need_reconnect_for_specialoption, SLOT(hide()));
        connect(dactionReconnect, SIGNAL(triggered()), mpProfilePreferencesDlg->need_reconnect_for_specialoption, SLOT(hide()));
        mpProfilePreferencesDlg->setAttribute( Qt::WA_DeleteOnClose );
    }
    mpProfilePreferencesDlg->raise();
    mpProfilePreferencesDlg->show();
}

void mudlet::show_help_dialog()
{
    QDesktopServices::openUrl(QUrl("http://wiki.mudlet.org/w/Manual:Contents"));
}

void mudlet::slot_show_help_dialog_video()
{
    QDesktopServices::openUrl(QUrl("http://www.mudlet.org/media/"));
}

void mudlet::slot_show_help_dialog_forum()
{
    QDesktopServices::openUrl(QUrl("http://forums.mudlet.org/"));
}

void mudlet::slot_show_help_dialog_irc()
{
    QDesktopServices::openUrl(QUrl("http://webchat.freenode.net/?channels=mudlet"));
}

void mudlet::slot_mapper()
{
    createMapper( true );
}

// Needed to extract into a separate method from slot_mapper() so that we can
// use it WITHOUT loading a file - at least for the TConsole::importMap(...)
// case that may need to create a map widget before it loads/imports a
// non-default (last saved map in profile's map directory.
void mudlet::createMapper( bool isToLoadDefaultMapFile )
{
    Host * pHost = getActiveHost();
    if( ! pHost )
    {
        return;
    }
    if( pHost->mpMap->mpMapper )
    {
        bool visStatus = pHost->mpMap->mpMapper->isVisible();
        if ( pHost->mpMap->mpMapper->parentWidget()->inherits("QDockWidget") )
        {
            pHost->mpMap->mpMapper->parentWidget()->setVisible( !visStatus );
        }
        pHost->mpMap->mpMapper->setVisible( !visStatus );
        return;
    }

    pHost->mpDockableMapWidget = new QDockWidget( tr("Map - %1").arg(pHost->getName()) );
    pHost->mpDockableMapWidget->setObjectName( QString("dockMap_%1").arg(pHost->getName()) );
    pHost->mpMap->mpMapper = new dlgMapper( pHost->mpDockableMapWidget, pHost, pHost->mpMap.data() );//FIXME: mpHost definieren
    pHost->mpMap->mpM = pHost->mpMap->mpMapper->glWidget;
    pHost->mpDockableMapWidget->setWidget( pHost->mpMap->mpMapper );

    if( isToLoadDefaultMapFile && pHost->mpMap->mpRoomDB->getRoomIDList().isEmpty() )
    {
        qDebug() << "mudlet::slot_mapper() - restore map case 3.";
        pHost->mpMap->pushErrorMessagesToFile( tr( "Pre-Map loading(3) report" ), true );
        QDateTime now( QDateTime::currentDateTime() );
        if( pHost->mpMap->restore( QString() ) ) {
            pHost->mpMap->audit();
            pHost->mpMap->mpMapper->mp2dMap->init();
            pHost->mpMap->mpMapper->updateAreaComboBox();
            pHost->mpMap->mpMapper->resetAreaComboBoxToPlayerRoomArea();
            pHost->mpMap->mpMapper->show();
        }

        pHost->mpMap->pushErrorMessagesToFile( tr( "Loading map(3) at %1 report" ).arg( now.toString( Qt::ISODate ) ), true );

    }
    else
    {
        if( pHost->mpMap->mpMapper )
        {
            pHost->mpMap->mpMapper->show();
        }
    }
    addDockWidget(Qt::RightDockWidgetArea, pHost->mpDockableMapWidget);

    loadWindowLayout();

    check_for_mappingscript();
    TEvent mapOpenEvent;
    mapOpenEvent.mArgumentList.append(QLatin1String("mapOpenEvent"));
    mapOpenEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    pHost->raiseEvent( mapOpenEvent );
}

void mudlet::check_for_mappingscript()
{
    Host * pHost = getActiveHost();
    if( ! pHost ) return;

    if (!pHost->checkForMappingScript()) {
        QUiLoader loader;

        QFile file(":/ui/lacking_mapper_script.ui");
        file.open(QFile::ReadOnly);

        QDialog *dialog = dynamic_cast<QDialog *>(loader.load(&file, this));
        file.close();

        connect(dialog, SIGNAL(accepted()), this, SLOT(slot_open_mappingscripts_page()));

        dialog->show();
        dialog->raise();
        dialog->activateWindow();
    }
}

void mudlet::slot_open_mappingscripts_page()
{
    QDesktopServices::openUrl(QUrl("http://forums.mudlet.org/search.php?keywords=mapping+script&terms=all&author=&sc=1&sf=titleonly&sr=topics&sk=t&sd=d&st=0&ch=400&t=0&submit=Search"));
}

void mudlet::slot_show_help_dialog_download()
{
    QDesktopServices::openUrl(QUrl("http://sourceforge.net/projects/mudlet/files/"));
}

void mudlet::slot_show_about_dialog()
{
    if( ! mpAboutDlg ) {
        mpAboutDlg = new dlgAboutDialog( this );
        mpAboutDlg->setAttribute( Qt::WA_DeleteOnClose );
    }

    mpAboutDlg->raise();
    mpAboutDlg->show();
}

void mudlet::slot_notes()
{
    Host * pHost = getActiveHost();
    if( ! pHost ) return;
    dlgNotepad * pNotes = pHost->mpNotePad;
    if( ! pNotes )
    {
        pHost->mpNotePad = new dlgNotepad( pHost );
        pNotes = pHost->mpNotePad;

        QTextCharFormat format;
        format.setFont( pHost->mDisplayFont );
        pNotes->notesEdit->setCurrentCharFormat( format );
        pNotes->restore();
        pNotes->setWindowTitle( tr( "%1 - notes" ).arg( pHost->getName() ) );
        pNotes->setWindowIcon( QIcon( QStringLiteral( ":/icons/mudlet_notepad.png" ) ) );
    }
    pNotes->raise();
    pNotes->show();
}

void mudlet::slot_irc()
{
    if( ! mpIRC )
    {
        mpIRC = new dlgIRC();
        mpIRC->setWindowTitle( tr( "Mudlet live IRC Help Channel #mudlet-help on irc.freenode.net" ) );
        mpIRC->setWindowIcon( QIcon( QStringLiteral( ":/icons/mudlet_irc.png" ) ) );
        mpIRC->resize(660,380);
    }

    mpIRC->raise();
    mpIRC->show();
}

void mudlet::slot_reconnect()
{
    Host * pHost = getActiveHost();
    if( ! pHost ) return;
    pHost->mTelnet.connectIt( pHost->getUrl(), pHost->getPort() );
}

void mudlet::slot_disconnect()
{
    Host * pHost = getActiveHost();
    if( ! pHost ) return;
    pHost->mTelnet.disconnect();
}

void mudlet::slot_replay()
{
    Host * pHost = getActiveHost();
    if( ! pHost ) return;
    QString home = QDir::homePath() + "/.config/mudlet/profiles/";
    home.append( pHost->getName() );
    home.append( "/log/" );
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select Replay"),
                                                    home,
                                                    tr("*.dat"));
    if( fileName.isEmpty() ) return;

    QFile file(fileName);
    if( ! file.open(QFile::ReadOnly | QFile::Text) )
    {
        QMessageBox::warning(this, tr("Select Replay"),
                             tr("Cannot read file %1:\n%2.")
                             .arg(fileName, file.errorString()));
        return;
    }
    //QString directoryLogFile = QDir::homePath()+"/.config/mudlet/profiles/"+profile_name+"/log";
    //QString fileName = directoryLogFile + "/"+QString(n.c_str());
    pHost->mTelnet.loadReplay( fileName );
}

void mudlet::printSystemMessage( Host * pH, const QString & s )
{
    mConsoleMap[pH]->printSystemMessage( s );
}

void mudlet::print( Host * pH, const QString & s )
{
    mConsoleMap[pH]->print( s );
}

QString mudlet::readProfileData( const QString& profile, const QString& item )
{
    QFile file( QDir::homePath()+"/.config/mudlet/profiles/"+profile+"/"+item );
    file.open( QIODevice::ReadOnly );
    if( ! file.exists() ) return "";

    QDataStream ifs( & file );
    QString ret;

    ifs >> ret;
    file.close();
    return ret;
}

// this slot is called via a timer in the constructor of mudlet::mudlet()
void mudlet::startAutoLogin()
{
    QStringList hostList = QDir(QDir::homePath()+"/.config/mudlet/profiles").entryList(QDir::Dirs, QDir::Name);
    hostList.removeAll(".");
    hostList.removeAll("..");
    for( int i = 0; i< hostList.size(); i++ )
    {
        QString item = "autologin";
        QString val = readProfileData( hostList[i], item );
        if( val.toInt() == Qt::Checked )
        {
            doAutoLogin( hostList[i] );
        }
    }

}

void mudlet::doAutoLogin( const QString & profile_name )
{
    if( profile_name.size() < 1 )
        return;

    Host* pOH = getHostManager().getHost(profile_name);
    if( pOH )
    {
        pOH->mTelnet.connectIt( pOH->getUrl(), pOH->getPort() );
        return;
    }
    // load an old profile if there is any
    getHostManager().addHost(profile_name, "", "", "");
    Host* pHost = getHostManager().getHost(profile_name);

    if( ! pHost ) return;

    LuaInterface * lI = pHost->getLuaInterface();
    lI->getVars( true );

    QString folder = QDir::homePath()+"/.config/mudlet/profiles/"+profile_name+"/current/";
    QDir dir( folder );
    dir.setSorting(QDir::Time);
    QStringList entries = dir.entryList( QDir::Files, QDir::Time );
    if( entries.size() > 0 )
    {
        QFile file(folder+"/"+entries[0]);
        file.open(QFile::ReadOnly | QFile::Text);
        XMLimport importer( pHost );
        qDebug()<<"[LOADING PROFILE]:"<<file.fileName();
        importer.importPackage( & file ); // TODO: Missing false return value handler
    }

    QString login = "login";
    QString val1 = readProfileData( profile_name, login );
    pHost->setLogin( val1 );
    QString pass = "password";
    QString val2 = readProfileData( profile_name, pass );
    pHost->setPass( val2 );
    slot_connection_dlg_finnished( profile_name, 0 );
    enableToolbarButtons();
}

///////////////////////////////////////////////////////////////////////////////
// these two callbacks are called from cTelnet::handleConnectedToServer()
void mudlet::slot_send_login()
{
    if( tempHostQueue.isEmpty() )
        return;
    Host * pHost = tempHostQueue.dequeue();
    QString login = pHost->getLogin();
    pHost->sendRaw( login );
}

void mudlet::slot_send_pass()
{
    if( tempHostQueue.isEmpty() )
        return;
    Host * pHost = tempHostQueue.dequeue();
    QString pass = pHost->getPass();
    pHost->sendRaw( pass );
}
//////////////////////////////////////////////////////////////////////////////



void mudlet::processEventLoopHack()
{
    QTimer::singleShot(1, this, SLOT(processEventLoopHack_timerRun()));
}

void mudlet::processEventLoopHack_timerRun()
{
    Host * pH = getActiveHost();
    if( !pH ) return;
    pH->mpConsole->refresh();
}

void mudlet::slot_connection_dlg_finnished( const QString& profile, int historyVersion )
{
    Host* pHost = getHostManager().getHost(profile);
    if( ! pHost ) return;
    pHost->mIsProfileLoadingSequence = true;
    addConsoleForNewHost( pHost );
    pHost->mBlockScriptCompile = false;
    pHost->mLuaInterpreter.loadGlobal();
    LuaInterface * lI = pHost->getLuaInterface();
    lI->getVars( true );
    pHost->getScriptUnit()->compileAll();
    pHost->mIsProfileLoadingSequence = false;

    //do modules here
    QMapIterator<QString, int> it (pHost->mModulePriorities);
    QMap<int, QStringList> moduleOrder;
    while( it.hasNext() ){
        it.next();
        QStringList moduleEntry = moduleOrder[it.value()];
        moduleEntry << it.key();
        moduleOrder[it.value()] = moduleEntry;
    }
    QMapIterator<int, QStringList> it2 (moduleOrder);
    while (it2.hasNext()){
        it2.next();
        QStringList modules = it2.value();
        for (int i=0;i<modules.size();i++){
            QStringList entry = pHost->mInstalledModules[modules[i]];
            pHost->installPackage(entry[0],1);
            //we repeat this step here b/c we use the same installPackage method for initial loading,
            //where we overwrite the globalSave flag.  This restores saved and loaded packages to their proper flag
            pHost->mInstalledModules[modules[i]] = entry;
        }
    }

    // install default packages
    for( int i=0; i< packagesToInstallList.size(); i++ )
    {
        pHost->installPackage( packagesToInstallList[i], 0 );
    }

    packagesToInstallList.clear();

    TEvent event;
    event.mArgumentList.append(QLatin1String("sysLoadEvent"));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    pHost->raiseEvent( event );

    //NOTE: this is a potential problem if users connect by hand quickly
    //      and one host has a slower response time as the other one, but
    //      the worst that can happen is that they have to login manually.

    tempHostQueue.enqueue( pHost );
    tempHostQueue.enqueue( pHost );
    pHost->connectToServer();
}

void mudlet::slot_multi_view()
{
     QMapIterator<Host *, TConsole *> it( mConsoleMap );
     while( it.hasNext() )
     {
         it.next();
         it.value()->show();
     }
}

void mudlet::slot_stopAllTriggers()
{
}

mudlet::~mudlet()
{
    mudlet::_self = 0;
}

void mudlet::toggleFullScreenView()
{
    if( isFullScreen() )
        showNormal();
    else
        showFullScreen();
}

void mudlet::replayStart()
{
    if( ! mpMainToolBar ) return;
    replayToolBar = new QToolBar( this );
    mReplaySpeed = 1;
    mReplayTime.setHMS( 0, 0, 0, 1 ); // Since Qt5.0 adding anything to a zero
                                      // (invalid) time leaves the time value
                                      // STILL being regarded as invalid - so to
                                      // get a valid time we have to use a very
                                      // small, NON-zero time to initiase it...!
    replayTime = new QLabel( this );
    actionReplayTime = replayToolBar->addWidget( replayTime );

    replayToolBar->setIconSize( QSize( 8 * mMainIconSize, 8 * mMainIconSize ) );
    replayToolBar->setToolButtonStyle( mpMainToolBar->toolButtonStyle() );

    actionReplaySpeedUp = new QAction( QIcon( QStringLiteral( ":/icons/export.png" ) ), tr("Faster"), this);
    actionReplaySpeedUp->setStatusTip(tr("Replay Speed Up"));
    replayToolBar->addAction( actionReplaySpeedUp );

    actionReplaySpeedDown = new QAction( QIcon( QStringLiteral( ":/icons/import.png" ) ), tr("Slower"), this);
    actionReplaySpeedDown->setStatusTip(tr("Replay Speed Down"));
    replayToolBar->addAction( actionReplaySpeedDown );
    replaySpeedDisplay = new QLabel( this );
    actionSpeedDisplay = replayToolBar->addWidget( replaySpeedDisplay );

    connect(actionReplaySpeedUp, SIGNAL(triggered()), this, SLOT(slot_replaySpeedUp()));
    connect(actionReplaySpeedDown, SIGNAL(triggered()), this, SLOT(slot_replaySpeedDown()));

    QString txt = "<font size=25><b>speed:";
    txt.append( QString::number( mReplaySpeed ) );
    txt.append("X</b></font>");
    replaySpeedDisplay->setText(txt);

    replayTimer = new QTimer(this);
    replayTimer->setInterval(1000);
    replayTimer->setSingleShot(false);
    connect(replayTimer, SIGNAL(timeout()), this, SLOT(slot_replayTimeChanged()));

    QString txt2 = "<font size=25><b>Time:";
    txt2.append( mReplayTime.toString( timeFormat ) );
    txt2.append("</b></font>");
    replayTime->setText( txt2 );

    replaySpeedDisplay->show();
    replayTime->show();
    insertToolBar( mpMainToolBar, replayToolBar );
    replayToolBar->show();
    replayTimer->start();
}

void mudlet::slot_replayTimeChanged()
{
    QString txt2 = "<font size=25><b>Time:";
    txt2.append( mReplayTime.toString( timeFormat ) );
    txt2.append("</b></font>");
    replayTime->setText( txt2 );
}

void mudlet::replayOver()
{
    if( ! mpMainToolBar ) return;
    if( ! replayToolBar ) return;

    if( actionReplaySpeedUp )
    {
        disconnect(actionReplaySpeedUp, SIGNAL(triggered()), this, SLOT(slot_replaySpeedUp()));
        disconnect(actionReplaySpeedDown, SIGNAL(triggered()), this, SLOT(slot_replaySpeedDown()));
        replayToolBar->removeAction( actionReplaySpeedUp );
        replayToolBar->removeAction( actionReplaySpeedDown );
        replayToolBar->removeAction( actionSpeedDisplay );
        removeToolBar( replayToolBar );
        actionReplaySpeedUp = 0;
        actionReplaySpeedDown = 0;
        actionSpeedDisplay = 0;
        actionReplayTime = 0;
        replayToolBar = 0;
    }
}

void mudlet::slot_replaySpeedUp()
{
    mReplaySpeed = mReplaySpeed * 2;
    QString txt = "<font size=25><b>speed:";
    txt.append( QString::number( mReplaySpeed ) );
    txt.append("X</b></font>");
    replaySpeedDisplay->setText(txt);
    replaySpeedDisplay->show();
}

void mudlet::slot_replaySpeedDown()
{
    mReplaySpeed = mReplaySpeed / 2;
    if( mReplaySpeed < 1 ) mReplaySpeed = 1;
    QString txt = "<font size=25><b>speed:";
    txt.append( QString::number( mReplaySpeed ) );
    txt.append("X</b></font>");
    replaySpeedDisplay->setText(txt);
    replaySpeedDisplay->show();
}

/* loop through and stop all sounds */
void mudlet::stopSounds()
{
    QListIterator<QMediaPlayer *> itMusicBox( mMusicBoxList );

    while( itMusicBox.hasNext() ) {
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
    QMediaPlayer* pPlayer = 0;

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
    disconnect(pPlayer, &QMediaPlayer::stateChanged, 0, 0);

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

void mudlet::setEditorTextoptions( const bool isTabsAndSpacesToBeShown, const bool isLinesAndParagraphsToBeShown )
{
    mEditorTextOptions = QTextOption::Flags( ( isTabsAndSpacesToBeShown ? QTextOption::ShowTabsAndSpaces : 0 )
                                           | ( isLinesAndParagraphsToBeShown ? QTextOption::ShowLineAndParagraphSeparators : 0 ) );
    emit signal_editorTextOptionsChanged( mEditorTextOptions );
}

void mudlet::slot_statusBarMessageChanged( QString text )
{
    if( mStatusBarState & statusBarAutoShown ) {
        if( text.isEmpty() ) {
            mpMainStatusBar->hide();
        }
        else {
            mpMainStatusBar->show();
        }
    }
    else if( mStatusBarState & statusBarAlwaysShown ) {
        if( ! mpMainStatusBar->isVisible() ) {
            mpMainStatusBar->show();
        }
    }
    else {
        // Should be hidden
        if( mpMainStatusBar->isVisible() ) {
            mpMainStatusBar->hide();
        }
    }
}

// Originally a slot_ but it does not actually need to be - Slysven
void mudlet::requestProfilesToReloadMaps( QList<QString> affectedProfiles )
{
    emit signal_profileMapReloadRequested( affectedProfiles );
}

