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


#include <QtGui>
#include <QResizeEvent>
#include "mudlet.h"
#include "TConsole.h"

#include <QTextEdit>
#include <QTextStream>
#include <QCloseEvent>
#include <QFileDialog>
#include <QtUiTools/quiloader.h>
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "dlgTriggerEditor.h"
#include "dlgPackageExporter.h"
#include "dlgAboutDialog.h"
#include "TCommandLine.h"
#include "EAction.h"
#include "dlgProfilePreferences.h"
#include "TDebug.h"
#include "XMLimport.h"
#include "EAction.h"
#include "TTextEdit.h"
#include "dlgNotepad.h"

//#ifdef Q_CC_GNU
    #include "dlgIRC.h"
//#endif

//#define NDEBUG
#include <assert.h>


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

TConsole *  mudlet::mpDebugConsole = 0;
QMainWindow * mudlet::mpDebugArea = 0;
bool mudlet::debugMode = false;

mudlet * mudlet::_self = 0;

mudlet * mudlet::self()
{
    if( ! _self )
    {
        _self = new mudlet;
    }
    return _self;
}


mudlet::mudlet()
: QMainWindow()
, mShowMenuBar( false )
, mShowToolbar( true )
, mWindowMinimized( false )
, mReplaySpeed( 1 )
, mIsGoingDown( false )
, mpCurrentActiveHost( 0 )
, actionReplaySpeedDown( 0 )
, actionReplaySpeedUp( 0 )
, mpIRC( 0 )
, mpMusicBox1(Phonon::createPlayer(Phonon::MusicCategory) )
, mpMusicBox2(Phonon::createPlayer(Phonon::MusicCategory) )
, mpMusicBox3(Phonon::createPlayer(Phonon::MusicCategory) )
, mpMusicBox4(Phonon::createPlayer(Phonon::MusicCategory) )
#ifdef Q_OS_LINUX
    , version( "Mudlet 2.1" )
#endif
#ifdef Q_OS_MAC
    , version( "Mudlet 2.1" )
#endif
#ifdef Q_OS_WIN
    , version( "Mudlet 2.1" )
#endif
{
    setupUi(this);
    setUnifiedTitleAndToolBarOnMac( true );
    setContentsMargins(0,0,0,0);
    mudlet::debugMode = false;
    setAttribute( Qt::WA_DeleteOnClose );
    QSizePolicy sizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding);
    setWindowTitle(version);
    setWindowIcon(QIcon(":/icons/mudlet_main_16px.png"));
    mpMainToolBar = new QToolBar( this );
    addToolBar( mpMainToolBar );
    //restoreBar = menuBar()->addMenu( "" );
    mpMainToolBar->setMovable( false );
    addToolBarBreak();
    QWidget * frame = new QWidget( this );
    frame->setFocusPolicy( Qt::NoFocus );
    setCentralWidget( frame );
    mpTabBar = new QTabBar( frame );
    mpTabBar->setMaximumHeight(30);
    mpTabBar->setFocusPolicy( Qt::NoFocus );
#if QT_VERSION >= 0x040500
    mpTabBar->setTabsClosable ( true );
    connect( mpTabBar, SIGNAL(tabCloseRequested(int)), this, SLOT(slot_close_profile_requested(int)));
#endif
    connect( mpTabBar, SIGNAL(currentChanged(int)), this, SLOT(slot_tab_changed(int)));
    QVBoxLayout * layoutTopLevel = new QVBoxLayout(frame);
    layoutTopLevel->setContentsMargins(0,0,0,0);
    layoutTopLevel->setContentsMargins(0,0,0,0);
    layoutTopLevel->addWidget( mpTabBar );
    mainPane = new QWidget( frame );
    QPalette mainPalette;
    //mainPalette.setColor( QPalette::Text, QColor(100,255,100) );
    //mainPalette.setColor( QPalette::Highlight, QColor(55,55,255) );
    //mainPalette.setColor( QPalette::Window, QColor(250,150,0,50) );
    mainPane->setPalette( mainPalette );
    mainPane->setAutoFillBackground(true);
    mainPane->setFocusPolicy( Qt::NoFocus );
    layoutTopLevel->addWidget( mainPane );
    QHBoxLayout * layout = new QHBoxLayout( mainPane );
    layout->setContentsMargins(0,0,0,0);
    layout->setContentsMargins(0,0,0,0);
    //layout->addWidget(mainPane);

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
    //restoreBar = new QMenu( this );
    QAction * actionConnect = new QAction(QIcon(":/icons/preferences-web-browser-cache.png"), tr("Connect"), this);
    actionConnect->setStatusTip(tr("Connect To Server"));
    mpMainToolBar->addAction( actionConnect );

    QAction * actionTriggers = new QAction(QIcon(":/icons/tools-wizard.png"), tr("Triggers"), this);
    actionTriggers->setStatusTip(tr("Show Triggers"));
    mpMainToolBar->addAction( actionTriggers );

    QAction * actionAlias = new QAction(QIcon(":/icons/system-users.png"), tr("Aliases"), this);
    actionAlias->setStatusTip(tr("Show Aliases"));
    actionAlias->setEnabled( true );
    mpMainToolBar->addAction( actionAlias );

    QAction * actionTimers = new QAction(QIcon(":/icons/chronometer.png"), tr("Timers"), this);
    actionTimers->setStatusTip(tr("Show Timers"));
    mpMainToolBar->addAction( actionTimers );

    QAction * actionButtons = new QAction(QIcon(":/icons/bookmarks.png"), tr("Buttons"), this);
    actionButtons->setStatusTip(tr("Show Easy Buttons"));
    mpMainToolBar->addAction( actionButtons );

    QAction * actionScripts = new QAction(QIcon(":/icons/document-properties.png"), tr("Scripts"), this);
    actionScripts->setEnabled( true );
    actionScripts->setStatusTip(tr("Show Scripts"));
    mpMainToolBar->addAction( actionScripts );

    QAction * actionKeys = new QAction(QIcon(":/icons/preferences-desktop-keyboard.png"), tr("Keys"), this);
    actionKeys->setStatusTip(tr("Options"));
    actionKeys->setEnabled( true );
    mpMainToolBar->addAction( actionKeys );

    QAction * actionIRC = new QAction(QIcon(":/icons/internet-telephony.png"), tr("Help Chat"), this);
    actionIRC->setStatusTip(tr("help chat on IRC"));
    mpMainToolBar->addAction( actionIRC );

    QAction * actionMapper = new QAction(QIcon(":/icons/applications-internet.png"), tr("Map"), this);
    actionMapper->setStatusTip(tr("show map"));
    mpMainToolBar->addAction( actionMapper );

    QAction * actionHelp = new QAction(QIcon(":/icons/help-hint.png"), tr("Manual"), this);
    actionHelp->setStatusTip(tr("Browse Reference Material and Documentation"));
    mpMainToolBar->addAction( actionHelp );

    QAction * actionOptions = new QAction(QIcon(":/icons/configure.png"), tr("Settings"), this);
    actionOptions->setStatusTip(tr("Settings, Options and Preferences"));
    mpMainToolBar->addAction( actionOptions );

    QAction * actionNotes = new QAction(QIcon(":/icons/applications-accessories.png"), tr("Notepad"), this);
    actionNotes->setStatusTip(tr("take notes"));
    mpMainToolBar->addAction( actionNotes );

    QAction * actionPackageM = new QAction(QIcon(":/icons/utilities-file-archiver.png"), tr("Package Manager"), this);
    actionPackageM->setStatusTip(tr("Package Manager"));
    mpMainToolBar->addAction( actionPackageM );
//    QAction * menuActionPackageM = new QAction("Package Manager", this);
//    menuActionPackageM->setStatusTip(tr("Package Manager"));
//    connect(menuActionPackageM, SIGNAL(triggered()), this, SLOT(slot_package_manager()));
//    QMenu * _miscMenu = new QMenu("Misc", this);
//    _miscMenu->addAction(menuActionPackageM);
//    menuBar()->addMenu(_miscMenu);



    QAction * actionReplay = new QAction(QIcon(":/icons/media-optical.png"), tr("Replay"), this);
    actionNotes->setStatusTip(tr("load replay"));
    mpMainToolBar->addAction( actionReplay );

    actionReconnect = new QAction(QIcon(":/icons/system-restart.png"), tr("Reconnect"), this);
    actionNotes->setStatusTip(tr("reconnect"));
    mpMainToolBar->addAction( actionReconnect );



    QAction * actionMultiView = new QAction(QIcon(":/icons/view-split-left-right.png"), tr("MultiView"), this);
    actionMultiView->setStatusTip(tr("MultiView"));
    mpMainToolBar->addAction( actionMultiView );

    QAction * actionStopAllTriggers = new QAction(QIcon(":/icons/edit-bomb.png"), tr("Stop All Triggers"), this);
    actionStopAllTriggers->setStatusTip(tr("stop all triggers, alias, actions, timers and scripts"));
    //mpMainToolBar->addAction( actionStopAllTriggers );


    /* QAction * actionProfileBackup = new QAction(QIcon(":/icons/utilities-file-archiver.png"), tr("Backup Profile"), this);
    actionProfileBackup->setStatusTip(tr("Backup Profile"));
    mpMainToolBar->addAction( actionProfileBackup );*/


    QAction * actionAbout = new QAction(QIcon(":/icons/mudlet_main_32px.png"), tr("About"), this);
    actionAbout->setStatusTip(tr("About"));
    mpMainToolBar->addAction( actionAbout );

    QAction * actionCloseProfile = new QAction(QIcon(":/icons/window-close.png"), tr("Close"), this);
    actionScripts->setEnabled( true );
    actionScripts->setStatusTip(tr("close connection"));
    //mpMainToolBar->addAction( actionCloseProfile );



    disableToolbarButtons();

    mpDebugArea = new QMainWindow(0);
    HostManager::self()->addHost("default_host", "", "","" );
    mpDefaultHost = HostManager::self()->getHost(QString("default_host"));
    mpDebugConsole = new TConsole( mpDefaultHost, true );
    mpDebugConsole->setWindowTitle("Central Debug Console");
    mpDebugConsole->setSizePolicy( sizePolicy );
    mpDebugConsole->setWrapAt(100);
    mpDebugArea->setCentralWidget( mpDebugConsole );
    TConsoleMonitor * consoleCloser = new TConsoleMonitor;
    mpDebugArea->installEventFilter(consoleCloser);

    QSize generalRule( qApp->desktop()->size() );
    generalRule -= QSize( 30, 30 );
    mpDebugArea->resize( QSize( 800, 600 ).boundedTo( generalRule ) );
    mpDebugArea->hide();
    QFont mainFont;
    if( file_use_smallscreen.exists() )
    {
        mainFont = QFont("Bitstream Vera Sans Mono", 8, QFont::Courier);
        showFullScreen();
        QAction * actionFullScreeniew = new QAction(QIcon(":/icons/dialog-cancel.png"), tr("Toggle Full Screen View"), this);
        actionFullScreeniew->setStatusTip(tr("Toggle Full Screen View"));
        mpMainToolBar->addAction( actionFullScreeniew );
        connect(actionFullScreeniew, SIGNAL(triggered()), this, SLOT(toggleFullScreenView()));
    }
    else
    {
        mainFont = QFont("Bitstream Vera Sans Mono", 8, QFont::Courier);
    }
    QFont mdiFont = QFont("Bitstream Vera Sans Mono", 6, QFont::Courier);
    setFont( mainFont );
    mainPane->setFont( mainFont );
    mpTabBar->setFont( mdiFont );
    QIcon noIcon;
    mainPane->show();
    connect(actionConnect, SIGNAL(triggered()), this, SLOT(connectToServer()));
    connect(actionHelp, SIGNAL(triggered()), this, SLOT(show_help_dialog()));
    connect(actionTriggers, SIGNAL(triggered()), this, SLOT(show_trigger_dialog()));
    connect(actionTimers, SIGNAL(triggered()), this, SLOT(show_timer_dialog()));
    connect(actionAlias, SIGNAL(triggered()), this, SLOT(show_alias_dialog()));
    connect(actionScripts, SIGNAL(triggered()), this, SLOT(show_script_dialog()));
    connect(actionKeys, SIGNAL(triggered()),this,SLOT(show_key_dialog()));
    connect(actionButtons, SIGNAL(triggered()), this, SLOT(show_action_dialog()));
    connect(actionOptions, SIGNAL(triggered()), this, SLOT(show_options_dialog()));
    connect(actionAbout, SIGNAL(triggered()), this, SLOT(slot_show_about_dialog()));
    connect(actionMultiView, SIGNAL(triggered()), this, SLOT(slot_multi_view()));
    connect(actionCloseProfile, SIGNAL(triggered()), this, SLOT(slot_close_profile()));
    connect(actionReconnect, SIGNAL(triggered()), this, SLOT(slot_reconnect()));
    connect(actionReplay, SIGNAL(triggered()), this, SLOT(slot_replay()));
    connect(actionNotes, SIGNAL(triggered()), this, SLOT(slot_notes()));
    connect(actionMapper, SIGNAL(triggered()), this, SLOT(slot_mapper()));
    connect(actionIRC, SIGNAL(triggered()), this, SLOT(slot_irc()));
    connect(actionPackageM, SIGNAL(triggered()), this, SLOT(slot_package_manager()));



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
   /* QMenu * menu;
    menuBar()->addAction(mactionConnect);
    menuBar()->addAction(mactionTriggers);
    menuBar()->addAction(mactionTimers);
    menuBar()->addAction(mactionAlias);
    menuBar()->addAction(mactionScripts);
    menuBar()->addAction(mactionKeys);
    menuBar()->addAction(mactionButtons);
    menuBar()->addAction(mactionOptions);
    menuBar()->addAction(mactionAbout);
    menuBar()->addAction(mactionHelp);*/
    readSettings();

    QTimer * timerAutologin = new QTimer( this );
    timerAutologin->setSingleShot( true );
    connect(timerAutologin, SIGNAL(timeout()), this, SLOT(startAutoLogin()));
    timerAutologin->start( 1000 );

    //qApp->setStyleSheet("QMainWindow::separator{border: 0px;width: 0px; height: 0px; padding: 0px;} QMainWindow::separator:hover {background: red;}");

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
    moduleTable->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
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
            QTableWidgetItem *masterModule = new QTableWidgetItem ();
            QTableWidgetItem *itemEntry = new QTableWidgetItem ();
            QTableWidgetItem *itemLocation = new QTableWidgetItem ();
            QTableWidgetItem *itemPriority = new QTableWidgetItem ();
            masterModule->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled|Qt::ItemIsSelectable);
            QStringList moduleInfo = pH->mInstalledModules[pModules[i]];
            if (moduleInfo[1].toInt()){
                masterModule->setCheckState(Qt::Checked);//Qt::Checked);
                masterModule->setText("sync");
            }
            else{
                masterModule->setCheckState(Qt::Unchecked);//Qt::Checked);
                masterModule->setText("don't sync");
            }

            masterModule->setToolTip(QString("Checking this box will cause the module to be saved & resync'd across all open sessions."));
            if (moduleInfo[0].endsWith(".zip") || moduleInfo[0].endsWith(".mpackage")){
                masterModule->setCheckState(Qt::Unchecked);
                masterModule->setFlags(Qt::NoItemFlags);
                masterModule->setText("don't sync");
                moduleInfo[1] = "0";
                masterModule->setToolTip(QString("mpackage and zip packages are currently unabled to be synced across packages."));
            }
            QString moduleName = pModules[i];
            itemEntry->setText(moduleName);
            itemEntry->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
            itemLocation->setText(moduleInfo[0]);
            //itemPriority->setText(QString::number(pH->mModulePriorities[moduleName]));
            //moduleTable->setItem(row, 0, masterModule);
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
    //moduleTable->sortItems(1, Qt::AscendingOrder);
}

void mudlet::slot_module_manager(){
    Host * pH = getActiveHost();
    if( ! pH ) return;
    QUiLoader loader;
    QFile file(":/ui/module_manager.ui");
    file.open(QFile::ReadOnly);
    QDialog * d = dynamic_cast<QDialog *>(loader.load(&file, this));
    file.close();
    if( ! d ) return;
    moduleTable = d->findChild<QTableWidget *>("moduleTable");
    moduleUninstallButton = d->findChild<QPushButton *>("uninstallButton");
    moduleInstallButton = d->findChild<QPushButton *>("installButton");
    moduleHelpButton = d->findChild<QPushButton *>("helpButton");
    if( !moduleTable || !moduleUninstallButton || !moduleHelpButton) return;
    layoutModules();
    connect(moduleUninstallButton, SIGNAL(clicked()), this, SLOT(slot_uninstall_module()));
    connect(moduleInstallButton, SIGNAL(clicked()), this, SLOT(slot_install_module()));
    connect(moduleHelpButton, SIGNAL(clicked()), this, SLOT(slot_help_module()));
    connect(moduleTable, SIGNAL(itemClicked(QTableWidgetItem*)), this, SLOT(slot_module_clicked(QTableWidgetItem*)));
    connect(moduleTable, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(slot_module_changed(QTableWidgetItem*)));
    d->setWindowTitle("Module Manager");
    d->show();
    d->raise();

}

bool mudlet::openWebPage(QString path){
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
        checkStatus->setCheckState(Qt::Unchecked);
        checkStatus->setFlags(Qt::NoItemFlags);
        return;
    }
    if (pH->moduleHelp.contains(entry->text()))
        moduleHelpButton->setDisabled((!pH->moduleHelp[entry->text()].contains("helpURL") || pH->moduleHelp[entry->text()]["helpURL"].isEmpty()));
    else
        moduleHelpButton->setDisabled(true);
    if (itemPath->text().endsWith(".zip") || itemPath->text().endsWith(".mpackage")){
        checkStatus->setCheckState(Qt::Unchecked);
        checkStatus->setFlags(Qt::NoItemFlags);
        checkStatus->setText("don't sync");
    }

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
                             .arg(fileName)
                             .arg(file.errorString()));
        return;
    }

    Host * pH = getActiveHost();
    if( ! pH ) return;
    pH->installPackage( fileName, 1);
    //moduleTable->clearContents();
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
    if( ! pH ) return;
    QUiLoader loader;
    QFile file(":/ui/package_manager.ui");
    file.open(QFile::ReadOnly);
    QDialog * d = dynamic_cast<QDialog *>(loader.load(&file, this));
    file.close();
    if( ! d ) return;
    packageList = d->findChild<QListWidget *>("packageList");
    uninstallButton = d->findChild<QPushButton *>("uninstallButton");
    installButton = d->findChild<QPushButton *>("installButton");
    if( ! packageList || ! uninstallButton ) return;
    packageList->addItems( pH->mInstalledPackages );
    connect(uninstallButton, SIGNAL(clicked()), this, SLOT(slot_uninstall_package()));
    connect(installButton, SIGNAL(clicked()), this, SLOT(slot_install_package()));
    d->setWindowTitle("Package Manager");
    d->show();
    d->raise();
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
                             .arg(fileName)
                             .arg(file.errorString()));
        return;
    }

    Host * pH = getActiveHost();
    if( ! pH ) return;

    pH->installPackage( fileName, 0);
    packageList->clear();
    packageList->addItems( pH->mInstalledPackages );
}

void mudlet::showUnzipProgress( QString txt )
{
    Host * pH = getActiveHost();
    if( ! pH ) return;
    pH->showUnpackingProgress( txt );
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
    dlgPackageExporter *d = new dlgPackageExporter(this, pH);
    d->show();
}


void mudlet::slot_close_profile_requested( int tab )
{
    QString name = mpTabBar->tabText( tab );
    Host * pH = HostManager::self()->getHost( name );
    if( ! pH ) return;

    if( ! pH->mpConsole->close() )
        return;
    else
        pH->mpConsole->mUserAgreedToCloseConsole = true;
    pH->stopAllTriggers();
    pH->mpEditorDialog->close();
    mConsoleMap[pH]->close();
    if( mTabMap.contains( pH->getName() ) )
    {
        mpTabBar->removeTab( tab );
        mConsoleMap.remove( pH );
        HostManager::self()->deleteHost( pH->getName() );
        mTabMap.remove( pH->getName() );
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
            QString name = mpCurrentActiveHost->getName();
            Host * pH = mpCurrentActiveHost;
            mpCurrentActiveHost->mpEditorDialog->close();
            mConsoleMap[mpCurrentActiveHost]->close();
            if( mTabMap.contains( pH->getName() ) )
            {
                mpTabBar->removeTab( mpTabBar->currentIndex() );
                mConsoleMap.remove( pH );
                HostManager::self()->deleteHost( pH->getName() );
                mTabMap.remove( pH->getName() );
            }
            if( !mpCurrentActiveHost ) disableToolbarButtons();
        }
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
    TConsole * pConsole = new TConsole( pH, false );
    if( ! pConsole ) return;
    pH->mpConsole = pConsole;
    pConsole->setWindowTitle( pH->getName() );
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

    dlgTriggerEditor * pEditor = new dlgTriggerEditor( pH );
    pH->mpEditorDialog = pEditor;
    pEditor->fillout_form();

    pH->getActionUnit()->updateToolbar();
    QMap<QString, TConsole *> miniConsoleMap;
    mHostConsoleMap[mpCurrentActiveHost] = miniConsoleMap;
    QMap<QString, TLabel *> labelMap;
    mHostLabelMap[mpCurrentActiveHost] = labelMap;
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
    //qDebug()<<"removing QTIMER"<<pQT<<" size="<<mTimerMap.size();
    if( mTimerMap.contains( pQT ) )
    {
        mTimerMap.remove( pQT );
    }
    else
    {
        qDebug()<<"MUDLET CRITICAL ERROR: trying to unregister Timer but it is not registered!";
    }
    //qDebug()<<"---> AFTER size="<<mTimerMap.size();
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
    mpMainToolBar->actions()[8]->setEnabled( false );
    mpMainToolBar->actions()[9]->setEnabled( false );
    mpMainToolBar->actions()[10]->setEnabled( false );
    mpMainToolBar->actions()[11]->setEnabled( false );
    mpMainToolBar->actions()[12]->setEnabled( false );
}

void mudlet::enableToolbarButtons()
{
    mpMainToolBar->actions()[1]->setEnabled( true );
    mpMainToolBar->actions()[2]->setEnabled( true );
    mpMainToolBar->actions()[3]->setEnabled( true );
    mpMainToolBar->actions()[4]->setEnabled( true );
    mpMainToolBar->actions()[5]->setEnabled( true );
    mpMainToolBar->actions()[6]->setEnabled( true );
    mpMainToolBar->actions()[8]->setEnabled( true );
    mpMainToolBar->actions()[9]->setEnabled( true );
    mpMainToolBar->actions()[10]->setEnabled( true );
    mpMainToolBar->actions()[11]->setEnabled( true );
    mpMainToolBar->actions()[12]->setEnabled( true );
}

bool mudlet::openWindow( Host * pHost, QString & name )
{
    if( ! dockWindowMap.contains( name ) )
    {
        QDockWidget * pD = new QDockWidget;
        pD->setContentsMargins(0,0,0,0);
        pD->setFeatures( QDockWidget::AllDockWidgetFeatures );
        pD->setWindowTitle( name );
        dockWindowMap[name] = pD;
        TConsole * pC = new TConsole( pHost, false );
        pC->setContentsMargins(0,0,0,0);
        pD->setWidget( pC );
        pC->show();
        pC->layerCommandLine->hide();
        pC->mpScrollBar->hide();
        pC->setUserWindow();
        QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
        dockWindowConsoleMap[name] = pC;
        addDockWidget(Qt::RightDockWidgetArea, pD);
        return true;
    }
    else return false;
}

bool mudlet::createMiniConsole( Host * pHost, QString & name, int x, int y, int width, int height )
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
            return true;
        }
    }
    return false;
}

bool mudlet::createLabel( Host * pHost, QString & name, int x, int y, int width, int height, bool fillBg )
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

bool mudlet::createBuffer( Host * pHost, QString & name )
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

bool mudlet::setBackgroundColor( Host * pHost, QString & name, int r, int g, int b, int alpha )
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

bool mudlet::setBackgroundImage( Host * pHost, QString & name, QString & path )
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

bool mudlet::setTextFormat( Host * pHost, QString & name, int r1, int g1, int b1, int r2, int g2, int b2, bool bold, bool underline, bool italics )
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
        pC->mFormatCurrent.bold = bold;
        pC->mFormatCurrent.underline = underline;
        pC->mFormatCurrent.italics = italics;
        return true;
    }
    else
        return false;
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

bool mudlet::clearWindow( Host * pHost, QString & name )
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

bool mudlet::showWindow( Host * pHost, QString & name )
{
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
        dockWindowConsoleMap[name]->console->show();
        dockWindowConsoleMap[name]->console->forceUpdate();
        return true;
    }

    return false;
}

bool mudlet::paste( Host * pHost, QString & name )
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

bool mudlet::hideWindow( Host * pHost, QString & name )
{
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
        dockWindowConsoleMap[name]->console->hide();
        dockWindowConsoleMap[name]->console->forceUpdate();
        return true;
    }

    return false;
}

bool mudlet::resizeWindow( Host * pHost, QString & name, int x1, int y1 )
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

bool mudlet::setConsoleBufferSize( Host * pHost, QString & name, int x1, int y1 )
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
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->reset();
        return true;
    }
    else
        return false;
}

bool mudlet::moveWindow( Host * pHost, QString & name, int x1, int y1 )
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

bool mudlet::closeWindow( Host * pHost, QString & name )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->console->close();
        return true;
    }
    else
        return false;
}

bool mudlet::setLabelClickCallback( Host * pHost, QString & name, QString & func, TEvent * pA )
{
    QMap<QString, TLabel *> & labelMap = mHostLabelMap[pHost];
    if( labelMap.contains( name ) )
    {
        labelMap[name]->setScript( pHost, func, pA );
        return true;
    }
    else
        return false;
}

bool mudlet::setLabelOnEnter( Host * pHost, QString & name, QString & func, TEvent * pA )
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

bool mudlet::setLabelOnLeave( Host * pHost, QString & name, QString & func, TEvent * pA )
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

int mudlet::getLastLineNumber( Host * pHost, QString & name )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowMap.contains( name ) )
    {
        return dockWindowConsoleMap[name]->getLastLineNumber();
    }
    else
    {
        TDebug(QColor(Qt::white),QColor(Qt::red))<<"ERROR: window doesnt exit\n" >> 0;
    }
    return -1;
}

bool mudlet::moveCursorEnd( Host * pHost, QString & name )
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

bool mudlet::moveCursor( Host * pHost, QString & name, int x, int y )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        return dockWindowConsoleMap[name]->moveCursor( x, y );
    }
    else
        return false;
}

void mudlet::deleteLine( Host * pHost, QString & name )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->skipLine();
    }
}

void mudlet::insertText( Host * pHost, QString & name, QString text )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->insertText( text );
    }
}

void mudlet::insertLink( Host * pHost, QString & name, QString text, QStringList & func, QStringList & hint, bool customFormat )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->insertLink( text, func, hint, customFormat );
    }
}

void mudlet::replace( Host * pHost, QString & name, QString text )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->replace( text );
    }
}

void mudlet::setLink( Host * pHost, QString & name, QString & linkText, QStringList & linkFunction, QStringList & linkHint )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        TConsole * pC = dockWindowConsoleMap[name];
        pC->setLink( linkText, linkFunction, linkHint );
    }
}

void mudlet::setBold( Host * pHost, QString & name, bool b )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        TConsole * pC = dockWindowConsoleMap[name];
        pC->setBold( b );
    }
}

void mudlet::setItalics( Host * pHost, QString & name, bool b )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        TConsole * pC = dockWindowConsoleMap[name];
        pC->setItalics( b );
    }
}

void mudlet::setUnderline( Host * pHost, QString & name, bool b )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        TConsole * pC = dockWindowConsoleMap[name];
        pC->setUnderline( b );
    }
}

void mudlet::setFgColor( Host * pHost, QString & name, int r, int g, int b )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        TConsole * pC = dockWindowConsoleMap[name];
        pC->setFgColor( r, g, b );
    }
}

void mudlet::setBgColor( Host * pHost, QString & name, int r, int g, int b )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        TConsole * pC = dockWindowConsoleMap[name];
        pC->setBgColor( r, g, b );
    }
}

int mudlet::selectString( Host * pHost, QString & name, QString text, int num )
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

int mudlet::selectSection( Host * pHost, QString & name, int f, int t )
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

void mudlet::deselect( Host * pHost, QString & name )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        TConsole * pC = dockWindowConsoleMap[name];
        pC->deselect();
    }
}

bool mudlet::setWindowWrap( Host * pHost, QString & name, int & wrap )
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

bool mudlet::setWindowWrapIndent( Host * pHost, QString & name, int & wrap )
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

bool mudlet::echoWindow( Host * pHost, QString & name, QString & text )
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

bool mudlet::echoLink( Host * pHost, QString & name, QString & text, QStringList & func, QStringList & hint, bool customFormat )
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

bool mudlet::copy( Host * pHost, QString & name )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->copy();
        return true;
    }
    else return false;
}

bool mudlet::pasteWindow( Host * pHost, QString & name )
{
    QMap<QString, TConsole *> & dockWindowConsoleMap = mHostConsoleMap[pHost];
    if( dockWindowConsoleMap.contains( name ) )
    {
        dockWindowConsoleMap[name]->pasteWindow( mConsoleMap[pHost]->mClipboard );
        return true;
    }
    else return false;
}

bool mudlet::appendBuffer( Host * pHost, QString & name )
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
                //qDebug()<<"saving notepad";
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


void mudlet::readSettings()
{
    QSettings settings("Mudlet", "Mudlet 1.0");
    QPoint pos = settings.value("pos", QPoint(0, 0)).toPoint();
    QSize size = settings.value("size", QSize(750, 550)).toSize();
    mMainIconSize = settings.value("mainiconsize",QVariant(3)).toInt();
    mTEFolderIconSize = settings.value("tefoldericonsize", QVariant(3)).toInt();
    mShowMenuBar = settings.value("showMenuBar",QVariant(0)).toBool();
    mShowToolbar = settings.value("showToolbar",QVariant(0)).toBool();
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
    QSettings settings("Mudlet", "Mudlet 1.0");
    settings.setValue("pos", pos());
    settings.setValue("size", size());
    settings.setValue("mainiconsize", mMainIconSize);
    settings.setValue("tefoldericonsize",mTEFolderIconSize);
    settings.setValue("showMenuBar", mShowMenuBar );
    settings.setValue("showToolbar", mShowToolbar );
    settings.setValue("maximized", isMaximized());
}

void mudlet::connectToServer()
{
    dlgConnectionProfiles * pDlg = new dlgConnectionProfiles(this);
    connect (pDlg, SIGNAL (signal_establish_connection( QString, int )), this, SLOT (slot_connection_dlg_finnished(QString, int)));
    pDlg->fillout_form();
    pDlg->exec();
    enableToolbarButtons();
}

void mudlet::show_trigger_dialog()
{
    Host * pHost = getActiveHost();
    if( ! pHost ) return;
    dlgTriggerEditor * pEditor = pHost->mpEditorDialog;
    if( ! pEditor ) return;
    pEditor->slot_show_triggers();
    pEditor->raise();
    pEditor->show();
}

void mudlet::show_alias_dialog()
{
    Host * pHost = getActiveHost();
    if( ! pHost ) return;
    dlgTriggerEditor * pEditor = pHost->mpEditorDialog;
    if( ! pEditor ) return;
    pEditor->slot_show_aliases();
    pEditor->raise();
    pEditor->show();
}

void mudlet::show_timer_dialog()
{
    Host * pHost = getActiveHost();
    if( ! pHost ) return;
    dlgTriggerEditor * pEditor = pHost->mpEditorDialog;
    if( ! pEditor ) return;
    pEditor->slot_show_timers();
    pEditor->raise();
    pEditor->show();
}

void mudlet::show_script_dialog()
{
    Host * pHost = getActiveHost();
    if( ! pHost ) return;
    dlgTriggerEditor * pEditor = pHost->mpEditorDialog;
    if( ! pEditor ) return;
    pEditor->slot_show_scripts();
    pEditor->raise();
    pEditor->show();
}

void mudlet::show_key_dialog()
{
    Host * pHost = getActiveHost();
    if( ! pHost ) return;
    dlgTriggerEditor * pEditor = pHost->mpEditorDialog;
    if( ! pEditor ) return;
    pEditor->slot_show_keys();
    pEditor->raise();
    pEditor->show();
}

void mudlet::show_action_dialog()
{
    Host * pHost = getActiveHost();
    if( ! pHost ) return;
    dlgTriggerEditor * pEditor = pHost->mpEditorDialog;
    if( ! pEditor ) return;
    pEditor->slot_show_actions();
    pEditor->raise();
    pEditor->show();
}

void mudlet::show_options_dialog()
{
    Host * pHost = getActiveHost();
    if( ! pHost ) return;
    dlgProfilePreferences * pDlg = new dlgProfilePreferences( this, pHost );
    connect(actionReconnect, SIGNAL(triggered()), pDlg->need_reconnect_for_gmcp, SLOT(hide()));
    connect(dactionReconnect, SIGNAL(triggered()), pDlg->need_reconnect_for_gmcp, SLOT(hide()));
    connect(actionReconnect, SIGNAL(triggered()), pDlg->need_reconnect_for_specialoption, SLOT(hide()));
    connect(dactionReconnect, SIGNAL(triggered()), pDlg->need_reconnect_for_specialoption, SLOT(hide()));
    if( ! pDlg ) return;
    pDlg->show();
}


void mudlet::show_help_dialog()
{
    QString p = qApp->applicationDirPath();
    p.append("/Mudlet_API_Reference_HTML.html");
    QDesktopServices::openUrl(QUrl::fromLocalFile(p));//("file://./Mudlet_API_Reference_HTML.html"));//"http://mudlet.sourceforge.net/wordpress/?page_id=40"));
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

#include "dlgMapper.h"

void mudlet::slot_mapper()
{
    Host * pHost = getActiveHost();
    if( ! pHost ) return;
    if( pHost->mpMap->mpMapper )
    {
        bool visStatus = pHost->mpMap->mpMapper->isVisible();
        if ( pHost->mpMap->mpMapper->parentWidget()->inherits("QDockWidget") )
            pHost->mpMap->mpMapper->parentWidget()->setVisible( !visStatus );
        pHost->mpMap->mpMapper->setVisible( !visStatus );
        return;
    }

    QDockWidget * pDock = new QDockWidget("Mudlet Mapper");
    pHost->mpMap->mpMapper = new dlgMapper( pDock, pHost, pHost->mpMap );//FIXME: mpHost definieren
    pHost->mpMap->mpM = pHost->mpMap->mpMapper->glWidget;
    pDock->setWidget( pHost->mpMap->mpMapper );

    if( pHost->mpMap->rooms.size() < 1 )
    {
        pHost->mpMap->restore("");
        pHost->mpMap->init( pHost );
        pHost->mpMap->mpMapper->mp2dMap->init();
        pHost->mpMap->mpMapper->show();
    }
    else
    {
        if( pHost->mpMap->mpMapper )
        {
            pHost->mpMap->mpMapper->show();
        }
    }
    addDockWidget(Qt::RightDockWidgetArea, pDock);

    check_for_mappingscript();
    TEvent mapOpenEvent;
    mapOpenEvent.mArgumentList.append( "mapOpenEvent" );
    mapOpenEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    pHost->raiseEvent( & mapOpenEvent );
}

void mudlet::check_for_mappingscript()
{
    Host * pHost = getActiveHost();
    if( ! pHost ) return;

    if (!pHost->check_for_mappingscript()) {
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
    dlgAboutDialog * pDlg = new dlgAboutDialog( this );
    pDlg->raise();
    pDlg->show();
}

#include <QTextCharFormat>

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
        pNotes->setWindowTitle( pHost->getName()+" notes" );
    }
    pNotes->raise();
    pNotes->show();
}

void mudlet::slot_irc()
{
//#ifdef Q_CC_GNU
    if( ! mpIRC )
    {
        mpIRC = new dlgIRC();
        mpIRC->setWindowTitle( "Mudlet live IRC Help Channel #mudlet-help on irc.freenode.net" );
        mpIRC->resize(660,380);
    }

    mpIRC->raise();
    mpIRC->show();
//#endif
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
                             .arg(fileName)
                             .arg(file.errorString()));
        return;
    }
    //QString directoryLogFile = QDir::homePath()+"/.config/mudlet/profiles/"+profile_name+"/log";
    //QString fileName = directoryLogFile + "/"+QString(n.c_str());
    pHost->mTelnet.loadReplay( fileName );
}

void mudlet::printSystemMessage( Host * pH, QString & s )
{
    mConsoleMap[pH]->printSystemMessage( s );
}

void mudlet::print( Host * pH, QString & s )
{
    mConsoleMap[pH]->print( s );
}


QString mudlet::readProfileData( QString profile, QString item )
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
/*
QString mudlet::readProfileData( QString profile, QString item )
{
    QFile file( QDir::homePath()+"/.config/mudlet/profiles/"+profile+"/"+item );
    file.open( QIODevice::ReadOnly );
    QDataStream ifs( & file );
    QString ret;
    ifs >> ret;
    file.close();
    return ret;
}*/

void mudlet::doAutoLogin( QString & profile_name )
{
    if( profile_name.size() < 1 )
        return;

    Host * pOH = HostManager::self()->getHost( profile_name );
    if( pOH )
    {
        pOH->mTelnet.connectIt( pOH->getUrl(), pOH->getPort() );
        return;
    }
    // load an old profile if there is any
    HostManager::self()->addHost( profile_name, "", "", "" );
    Host * pHost = HostManager::self()->getHost( profile_name );

    if( ! pHost ) return;

    QString folder = QDir::homePath()+"/.config/mudlet/profiles/"+profile_name+"/current/";
    QDir dir( folder );
    dir.setSorting(QDir::Time);
    QStringList entries = dir.entryList( QDir::Files, QDir::Time );
    //for( int i=0;i<entries.size(); i++ )
    //    qDebug()<<i<<"#"<<entries[i];
    if( entries.size() > 0 )
    {
        QFile file(folder+"/"+entries[0]);
        file.open(QFile::ReadOnly | QFile::Text);
        XMLimport importer( pHost );
        qDebug()<<"[LOADING PROFILE]:"<<file.fileName();
        importer.importPackage( & file );
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
/*
    QList<QString> hostList = HostManager::self()->getHostList();
    for( int i=0; i<hostList.size(); i++ )
    {
        Host * pH = HostManager::self()->getHost( hostList[i] );
        QString profile = pH->getName();
        if( profile.size() < 1 )
        {
            HostManager::self()->deleteHost( profile );
            continue;
        }

        QString item = "autologin";
        QString val = readProfileData( profile, item );
        if( val.toInt() == Qt::Checked )
        {
            qDebug()<<"----> Host:"<<pH->getName()<<" URL:"<<pH->getUrl()<<"Login:"<<pH->getLogin();
            addConsoleForNewHost( pH );
            pH->connectToServer();
        }
        else
        {
            // remove Hosts that don't have autologin defined from HostPool
            qDebug() << "----> [ EXPIRED ] " << profile.toLatin1().data() << " Host ist no longer an autoloader. Due to user decision.";
            HostManager::self()->deleteHost( profile );
        }
    }
    if( hostList.size() < 1 )
        qDebug() << "----> [ OK ] nothing to be done (no autologin profiles defined)";
    else
        qDebug() << "----> [ OK ] autologin finished";
    qDebug()<<"[ AUTOLOGIN END ] currently loaded hosts after removal of non-autoloaders:"<<HostManager::self()->getHostList();
}*/

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

void mudlet::slot_connection_dlg_finnished( QString profile, int historyVersion )
{
    Host * pHost = HostManager::self()->getHost( profile );
    if( ! pHost ) return;
    pHost->mIsProfileLoadingSequence = true;
    addConsoleForNewHost( pHost );
    pHost->mBlockScriptCompile = false;
    pHost->mLuaInterpreter.loadGlobal();
    pHost->getScriptUnit()->compileAll();
    pHost->mIsProfileLoadingSequence = false;

    //do modules here
    qDebug()<<"loading modules now";
    //QMapIterator<QString, QStringList > it (pHost->mInstalledModules);
    QMapIterator<QString, int> it (pHost->mModulePriorities);
    QMap<int, QStringList> moduleOrder;
    while( it.hasNext() ){
        it.next();
        QStringList moduleEntry = moduleOrder[it.value()];
        moduleEntry << it.key();
        moduleOrder[it.value()] = moduleEntry;
        /*QStringList entry = it.value();
        pHost->installPackage(entry[0],1);
        qDebug()<<entry[0]<<","<<entry[1];
        //we repeat this step here b/c we use the same installPackage method for initial loading,
        //where we overwrite the globalSave flag.  This restores saved and loaded packages to their proper flag
        pHost->mInstalledModules[it.key()] = entry;*/
    }
    QMapIterator<int, QStringList> it2 (moduleOrder);
    while (it2.hasNext()){
        it2.next();
        QStringList modules = it2.value();
        for (int i=0;i<modules.size();i++){
            QStringList entry = pHost->mInstalledModules[modules[i]];
            pHost->installPackage(entry[0],1);
            qDebug()<<entry[0]<<","<<entry[1];
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
    event.mArgumentList.append( "sysLoadEvent" );
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    pHost->raiseEvent( & event );

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
    /*
    foreach( TConsole * pC, mConsoleMap )
    {
        pC->mpHost->stopAllTriggers();
    }*/
}

mudlet::~mudlet()
{
}

void mudlet::toggleFullScreenView()
{
    if( isFullScreen() )
        showNormal();
    else
        showFullScreen();
}

QLabel * replaySpeedDisplay = 0;
QLabel * replayTime = 0;
QAction * actionSpeedDisplay = 0;
QAction * actionReplayTime = 0;
QToolBar * replayToolBar = 0;
const QString timeFormat = "hh:mm:ss";
QTimer * replayTimer = 0;

void mudlet::replayStart()
{
    if( ! mpMainToolBar ) return;
    replayToolBar = new QToolBar( this );
    mReplaySpeed = 1;
    replayTime = new QLabel( this );
    actionReplayTime = replayToolBar->addWidget( replayTime );

    actionReplaySpeedUp = new QAction(QIcon(":/icons/export.png"), tr("faster"), this);
    actionReplaySpeedUp->setStatusTip(tr("Replay Speed Up"));
    replayToolBar->addAction( actionReplaySpeedUp );

    actionReplaySpeedDown = new QAction(QIcon(":/icons/port.png"), tr("slower"), this);
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

    QString txt2 = "<font size=25><b>Time:";
    txt2.append( mReplayTime.toString( timeFormat ) );
    txt2.append("</b></font>");
    replayTime->setText( txt2 );

    replaySpeedDisplay->show();
    replayTime->show();
    insertToolBar( mpMainToolBar, replayToolBar );
    replayToolBar->show();
    replayTimer = new QTimer( this );
    replayTimer->setInterval(1000);
    replayTimer->setSingleShot( false );
    connect( replayTimer, SIGNAL( timeout() ), this, SLOT(slot_replayTimeChanged()));
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

void mudlet::playSound( QString s )
{
    if( mpMusicBox1->remainingTime() == 0 )
    {
        mpMusicBox1->setCurrentSource( s );
        mpMusicBox1->play();
    }
    else if( mpMusicBox2->remainingTime() == 0 )
    {
        mpMusicBox2->setCurrentSource( s );
        mpMusicBox2->play();
    }
    else if( mpMusicBox3->remainingTime() == 0 )
    {
        mpMusicBox3->setCurrentSource( s );
        mpMusicBox3->play();
    }
    else
    {
        mpMusicBox4->clear();
        mpMusicBox4->setCurrentSource( s );
        mpMusicBox4->play();
    }
}
