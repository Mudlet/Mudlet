/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014, 2016-2018 by Stephen Lyons                        *
 *                                               - slysven@virginmedia.com *
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


#include "dlgProfilePreferences.h"


#include "Host.h"
#include "TConsole.h"
#include "TMap.h"
#include "TRoomDB.h"
#include "TTextEdit.h"
#include "ctelnet.h"
#include "dlgIRC.h"
#include "dlgMapper.h"
#include "dlgTriggerEditor.h"
#include "edbee/views/texteditorscrollarea.h"
#include "mudlet.h"

#include "pre_guard.h"
#include <QtConcurrent>
#include <QColorDialog>
#include <QFileDialog>
#include <QFontDialog>
#include <QMainWindow>
#include <QNetworkDiskCache>
#include <QPalette>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTextOption>
#include <QToolBar>
#include <QUiLoader>
#include <QVariant>
#include "post_guard.h"


dlgProfilePreferences::dlgProfilePreferences(QWidget* pF, Host* pHost)
: QDialog(pF)
, mFontSize(10)
, mpHost(pHost)
, mpMenu(nullptr)
{
    // init generated dialog
    setupUi(this);

    // This is currently empty so can be hidden until needed, but provides a
    // location on the last (Special Options) tab where
    // temporary/development/testing controls can be placed if needed...
    groupBox_debug->hide();

    QFile file_use_smallscreen(mudlet::getMudletPath(mudlet::mainDataItemPath, QStringLiteral("mudlet_option_use_smallscreen")));
    checkBox_USE_SMALL_SCREEN->setChecked(file_use_smallscreen.exists());
    checkBox_showSpacesAndTabs->setChecked(mudlet::self()->mEditorTextOptions & QTextOption::ShowTabsAndSpaces);
    checkBox_showLineFeedsAndParagraphs->setChecked(mudlet::self()->mEditorTextOptions & QTextOption::ShowLineAndParagraphSeparators);
    // As we reflect the state of the above two checkboxes in the preview widget
    // on another tab we have to track their changes in state and update that
    // edbee widget straight away - however we do not need to update any open
    // widgets of the same sort in use in ANY profile's editor until we hit
    // the save button...
    checkBox_reportMapIssuesOnScreen->setChecked(mudlet::self()->showMapAuditErrors());

    MainIconSize->setValue(mudlet::self()->mToolbarIconSize);
    TEFolderIconSize->setValue(mudlet::self()->mEditorTreeWidgetIconSize);
    switch (mudlet::self()->menuBarVisibility()) {
    case mudlet::visibleNever:
        comboBox_menuBarVisibility->setCurrentIndex(0);
        break;
    case mudlet::visibleOnlyWithoutLoadedProfile:
        comboBox_menuBarVisibility->setCurrentIndex(1);
        break;
    default:
        comboBox_menuBarVisibility->setCurrentIndex(2);
    }

    switch (mudlet::self()->toolBarVisibility()) {
    case mudlet::visibleNever:
        comboBox_toolBarVisibility->setCurrentIndex(0);
        break;
    case mudlet::visibleOnlyWithoutLoadedProfile:
        comboBox_toolBarVisibility->setCurrentIndex(1);
        break;
    default:
        comboBox_toolBarVisibility->setCurrentIndex(2);
    }

    if (pHost) {
        initWithHost(pHost);
    } else {
        disableHostDetails();
        clearHostDetails();
    }

#if defined(INCLUDE_UPDATER)
    if (mudlet::scmIsDevelopmentVersion) {
        // tick the box and make it be "un-untickable" as automatic updates are
        // disabled in dev builds
        checkbox_noAutomaticUpdates->setChecked(true);
        checkbox_noAutomaticUpdates->setDisabled(true);
        checkbox_noAutomaticUpdates->setToolTip(tr("Automatic updates are disabled in development builds to prevent an update from overwriting your Mudlet"));
    } else {
        checkbox_noAutomaticUpdates->setChecked(!mudlet::self()->updater->updateAutomatically());
    }
#else
    groupBox_updates->hide();
#endif

    // Enforce selection of the first tab - despite any cock-ups when using the
    // Qt Designer utility when the dialog was saved with a different one
    // on top! 8-)
    tabWidget->setCurrentIndex(0);

    // To be moved to a slot that is used on GUI language change when that gets
    // implimented:
    pushButton_showGlyphUsage->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                          .arg("<p>This will bring up a display showing all the symbols used in the current "
                                               "map and whether they can be drawn using just the specifed font, any other "
                                               "font, or not at all.  It also shows the sequence of Unicode <i>code-points</i> "
                                               "that make up that symbol, so that they can be identified even if they "
                                               "cannot be displayed; also, up to the first thirty two rooms that are using "
                                               "that symbol are listed, which may help to identify any unexpected or odd cases.<p>"));
    fontComboBox_mapSymbols->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                        .arg("<p>Select the only or the primary font used (depending on <i>Only use symbols "
                                             "(glyphs) from chosen font</i> setting) to produce the 2D mapper room symbols.</p>"));
    checkBox_isOnlyMapSymbolFontToBeUsed->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                                             .arg("<p>Using a single font is likely to produce a more consistent style but may "
                                                                  "cause the <i>font replacement character</i> '<b>�</b>' to show if the font "
                                                                  "does not have a needed glyph (a font's individual character/symbol) to represent "
                                                                  "the grapheme (what is to be represented).  Clearing this checkbox will allow "
                                                                  "the best alternative glyph from another font to be used to draw that grapheme.</p>"));

    // Set the properties on tab_logging
    comboBox_logFileNameFormat->setToolTip(tr("<html><head/><body>%1</body></html>")
                                                   .arg("<p>This option sets the format of the log name.</p>"
                                                        "<p>If 'Named file' is selected, you can set a custom file name. (Logs are appended if a log file of the same name already exists.)</p>"));
    comboBox_logFileNameFormat->addItem(tr("yyyy-MM-dd#HH:mm:ss (e.g., 1970-01-01#00:00:00)"), QStringLiteral("yyyy-MM-dd#HH:mm:ss"));
    comboBox_logFileNameFormat->addItem(tr("yyyy-MM-ddTHH:mm:ss (e.g., 1970-01-01T00:00:00)"), QStringLiteral("yyyy-MM-dd#THH:mm:ss"));
    comboBox_logFileNameFormat->addItem(tr("yyyy-MM-dd (concatenate logs by day)"), QStringLiteral("yyyy-MM-dd"));
    comboBox_logFileNameFormat->addItem(tr("yyyy-MM (concatenate logs by month)"), QStringLiteral("yyyy-MM"));
    comboBox_logFileNameFormat->addItem(tr("Named file (concatenate logs in one file)"), QString());
    lineEdit_logFileName->setToolTip(tr("<html><head/><body>%1</body></html>").arg("<p>Set a custom name for your log. (Logs are appended if a log file of the same name already exists).</p>"));
    lineEdit_logFileName->setPlaceholderText(tr("logfile"));
    lineEdit_logFileName->setEnabled(false);


    connect(checkBox_showSpacesAndTabs, SIGNAL(clicked(bool)), this, SLOT(slot_changeShowSpacesAndTabs(const bool)));
    connect(checkBox_showLineFeedsAndParagraphs, SIGNAL(clicked(bool)), this, SLOT(slot_changeShowLineFeedsAndParagraphs(const bool)));
    connect(closeButton, &QAbstractButton::pressed, this, &dlgProfilePreferences::slot_save_and_exit);
    connect(mudlet::self(), SIGNAL(signal_hostCreated(Host*, quint8)), this, SLOT(slot_handleHostAddition(Host*, quint8)));
    connect(mudlet::self(), SIGNAL(signal_hostDestroyed(Host*, quint8)), this, SLOT(slot_handleHostDeletion(Host*)));
    connect(comboBox_menuBarVisibility, SIGNAL(currentIndexChanged(int)), this, SLOT(slot_changeShowMenuBar(int)));
    connect(comboBox_toolBarVisibility, SIGNAL(currentIndexChanged(int)), this, SLOT(slot_changeShowToolBar(int)));
}

void dlgProfilePreferences::disableHostDetails()
{
    // The Host pointer is a nullptr so disable every control that depends on it

    // on tab_general:
    // groupBox_iconsAndToolbars is NOT dependent on pHost - leave it alone
    groupBox_encoding->setEnabled(false);
    groupBox_miscellaneous->setEnabled(false);
    groupBox_protocols->setEnabled(false);
    need_reconnect_for_data_protocol->hide();

    // on tab_inputLine:
    groupBox_input->setEnabled(false);
    groupBox_spellCheck->setEnabled(false);

    // on tab_display:
    groupBox_font->setEnabled(false);
    groupBox_borders->setEnabled(false);
    groupBox_wrapping->setEnabled(false);
    groupBox_doubleClick->setEnabled(false);
    // Some of groupBox_displayOptions are usable, so must pick out and
    // disable the others:
    checkBox_USE_IRE_DRIVER_BUGFIX->setEnabled(false);
    checkBox_echoLuaErrors->setEnabled(false);

    // on tab_codeEditor:
    groupbox_codeEditorThemeSelection->setEnabled(false);
    theme_download_label->hide();

    // on tab_displayColors:
    groupBox_displayColors->setEnabled(false);

    // on tab_mapper:
    // most of groupBox_mapFiles is disabled but there is ONE checkBox that
    // is accessable because it is application wide - so disable EVERYTHING
    // else that is not already disabled:
    label_saveMap->setEnabled(false);
    pushButton_saveMap->setEnabled(false);
    label_loadMap->setEnabled(false);
    pushButton_loadMap->setEnabled(false);
    label_copyMap->setEnabled(false);
    label_mapFileSaveFormatVersion->setEnabled(false);
    comboBox_mapFileSaveFormatVersion->setEnabled(false);
    comboBox_mapFileSaveFormatVersion->clear();
    label_mapFileActionResult->hide();
    label_mapSymbolsFont->setEnabled(false);
    fontComboBox_mapSymbols->setEnabled(false);
    checkBox_isOnlyMapSymbolFontToBeUsed->setEnabled(false);
    pushButton_showGlyphUsage->setEnabled(false);

    groupBox_downloadMapOptions->setEnabled(false);
    // The above is actually normally hidden:
    groupBox_downloadMapOptions->hide();
    groupBox_mapOptions->setEnabled(false);
    // This is actually normally hidden until a map is loaded:
    checkBox_showDefaultArea->hide();

    // on tab_mapperColors:
    groupBox_mapperColors->setEnabled(false);

    // on tab_logging:
    pushButton_whereToLog->setEnabled(false);
    pushButton_resetLogDir->setEnabled(false);
    comboBox_logFileNameFormat->setEnabled(false);
    lineEdit_logFileName->setEnabled(false);
    mIsToLogInHtml->setEnabled(false);
    mIsLoggingTimestamps->setEnabled(false);

    // on groupBox_specialOptions:
    groupBox_specialOptions->setEnabled(false);
    // it is possible to connect using the IRC client off of the
    // "default" host even without a normal profile loaded so leave
    // groupBox_ircOptions enabled...
    need_reconnect_for_specialoption->hide();
    groupbox_searchEngineSelection->setEnabled(false);
}

void dlgProfilePreferences::enableHostDetails()
{
    groupBox_encoding->setEnabled(true);
    groupBox_miscellaneous->setEnabled(true);
    groupBox_protocols->setEnabled(true);

    // on tab_inputLine:
    groupBox_input->setEnabled(true);
    groupBox_spellCheck->setEnabled(true);

    // on tab_display:
    groupBox_font->setEnabled(true);
    groupBox_borders->setEnabled(true);
    groupBox_wrapping->setEnabled(true);
    groupBox_doubleClick->setEnabled(true);

    checkBox_USE_IRE_DRIVER_BUGFIX->setEnabled(true);
    checkBox_echoLuaErrors->setEnabled(true);

    // on tab_codeEditor:
    groupbox_codeEditorThemeSelection->setEnabled(true);

    // on tab_displayColors:
    groupBox_displayColors->setEnabled(true);

    // on tab_mapper:
    // most of groupBox_mapFiles is disabled but there is ONE checkBox that
    // is accessable because it is application wide - so disable EVERYTHING
    // else that is not already disabled:
    label_saveMap->setEnabled(true);
    pushButton_saveMap->setEnabled(true);
    label_loadMap->setEnabled(true);
    pushButton_loadMap->setEnabled(true);
    label_copyMap->setEnabled(true);
    label_mapFileSaveFormatVersion->setEnabled(true);

    groupBox_downloadMapOptions->setEnabled(true);
    groupBox_mapOptions->setEnabled(true);

    // on tab_mapperColors:
    groupBox_mapperColors->setEnabled(true);

    // on tab_logging:
    pushButton_whereToLog->setEnabled(true);
    pushButton_resetLogDir->setEnabled(true);
    comboBox_logFileNameFormat->setEnabled(true);
    lineEdit_logFileName->setEnabled(true);
    mIsToLogInHtml->setEnabled(true);
    mIsLoggingTimestamps->setEnabled(true);

    // on groupBox_specialOptions:
    groupBox_specialOptions->setEnabled(true);
    // it is possible to connect using the IRC client off of the
    // "default" host even without a normal profile loaded so leave
    // groupBox_ircOptions enabled...
    groupbox_searchEngineSelection->setEnabled(true);
}

void dlgProfilePreferences::initWithHost(Host* pHost)
{
    loadEditorTab();

    // search engine load
    search_engine_combobox->addItems(QStringList(mpHost->mSearchEngineData.keys()));

    // set to saved value or default to Google
    int savedText = search_engine_combobox->findText(mpHost->getSearchEngine().first);
    search_engine_combobox->setCurrentIndex(savedText == -1 ? 1 : savedText);

    mFORCE_MXP_NEGOTIATION_OFF->setChecked(pHost->mFORCE_MXP_NEGOTIATION_OFF);
    mMapperUseAntiAlias->setChecked(pHost->mMapperUseAntiAlias);
    acceptServerGUI->setChecked(pHost->mAcceptServerGUI);

    ircHostName->setText(dlgIRC::readIrcHostName(pHost));
    ircHostPort->setText(QString::number(dlgIRC::readIrcHostPort(pHost)));
    ircChannels->setText(dlgIRC::readIrcChannels(pHost).join(" "));
    ircNick->setText(dlgIRC::readIrcNickName(pHost));

    dictList->setSelectionMode(QAbstractItemView::SingleSelection);
    enableSpellCheck->setChecked(pHost->mEnableSpellCheck);
    checkBox_echoLuaErrors->setChecked(pHost->mEchoLuaErrors);

    QString path;
    // This is duplicated (and should be the same as) the code in:
    // TCommandLine::TCommandLine(Host*, TConsole*, QWidget*)
#if defined(Q_OS_MACOS)
    path = QStringLiteral("%1/../Resources/").arg(QCoreApplication::applicationDirPath());
#elif defined(Q_OS_FREEBSD)
    if (QFile::exists(QStringLiteral("/usr/local/share/hunspell/%1.aff").arg(pHost->mSpellDic))) {
        path = QLatin1String("/usr/local/share/hunspell/");
    } else if (QFile::exists(QStringLiteral("/usr/share/hunspell/%1.aff").arg(pHost->mSpellDic))) {
        path = QLatin1String("/usr/share/hunspell/");
    } else {
        path = QLatin1String("./");
    }
#elif defined(Q_OS_LINUX)
    if (QFile::exists(QStringLiteral("/usr/share/hunspell/%1.aff").arg(pHost->mSpellDic))) {
        path = QLatin1String("/usr/share/hunspell/");
    } else {
        path = QLatin1String("./");
    }
#else
    // Probably Windows!
    path = "./";
#endif

    QDir dir(path);
    QStringList entries = dir.entryList(QDir::Files, QDir::Time);
    QRegularExpression rex(QStringLiteral(R"(\.dic$)"));
    entries = entries.filter(rex);
    for (int i = 0; i < entries.size(); i++) {
        // This is a file name and to support macOs platforms should not be case sensitive:
        entries[i].remove(QLatin1String(".dic"), Qt::CaseInsensitive);
        auto item = new QListWidgetItem(entries[i]);
        dictList->addItem(item);
        if (entries[i] == pHost->mSpellDic) {
            item->setSelected(true);
        }
    }

    const QString url(pHost->getUrl());
    if (url.contains(QStringLiteral("achaea.com"), Qt::CaseInsensitive)
     || url.contains(QStringLiteral("aetolia.com"), Qt::CaseInsensitive)
     || url.contains(QStringLiteral("imperian.com"), Qt::CaseInsensitive)
     || url.contains(QStringLiteral("lusternia.com"), Qt::CaseInsensitive)) {

        groupBox_downloadMapOptions->setVisible(true);
        connect(buttonDownloadMap, SIGNAL(clicked()), this, SLOT(downloadMap()));
    } else {
        groupBox_downloadMapOptions->setVisible(false);
    }

    setColors();

    QStringList sizeList;
    for (int i = 1; i < 40; i++) {
        sizeList << QString::number(i);
    }
    fontSize->insertItems(1, sizeList);

    setColors2();

    // the GMCP warning is hidden by default and is only enabled when the value is toggled
    need_reconnect_for_data_protocol->hide();

    // same with special connection warnings
    need_reconnect_for_specialoption->hide();

    fontComboBox->setCurrentFont(pHost->mDisplayFont);
    mFontSize = pHost->mDisplayFont.pointSize();
    if (mFontSize < 0) {
        mFontSize = 10;
    }
    if (mFontSize < 40 && mFontSize > 0) {
        fontSize->setCurrentIndex((mFontSize - 1));
    } else {
        // if the font size set for the main console is outside the pre-set range
        // this will unfortunately reset the font to default size.
        // without this the first entry (font-size 1) is selected and on-save
        // will make the console font far too tiny to read.
        // Maybe our font-size range should be generated differently if the console
        // has a font size larger than the preset range offers?
        fontSize->setCurrentIndex(9); // default font is size 10, index 9.
    }

    wrap_at_spinBox->setValue(pHost->mWrapAt);
    indent_wrapped_spinBox->setValue(pHost->mWrapIndentCount);

    show_sent_text_checkbox->setChecked(pHost->mPrintCommand);
    auto_clear_input_line_checkbox->setChecked(pHost->mAutoClearCommandLineAfterSend);
    command_separator_lineedit->setText(pHost->mCommandSeparator);
    //disable_auto_completion_checkbox->setChecked(pHost->mDisableAutoCompletion);

    checkBox_USE_IRE_DRIVER_BUGFIX->setChecked(pHost->mUSE_IRE_DRIVER_BUGFIX);
    //this option is changed into a forced option for GA enabled drivers as triggers wont run on prompt lines otherwise
    //checkBox_LF_ON_GA->setChecked( pHost->mLF_ON_GA );
    checkBox_mUSE_FORCE_LF_AFTER_PROMPT->setChecked(pHost->mUSE_FORCE_LF_AFTER_PROMPT);
    USE_UNIX_EOL->setChecked(pHost->mUSE_UNIX_EOL);
    topBorderHeight->setValue(pHost->mBorderTopHeight);
    bottomBorderHeight->setValue(pHost->mBorderBottomHeight);
    leftBorderWidth->setValue(pHost->mBorderLeftWidth);
    rightBorderWidth->setValue(pHost->mBorderRightWidth);
    comboBox_logFileNameFormat->setCurrentIndex(pHost->mLogFileNameFormatIndex);
    lineEdit_logFileName->setText(pHost->mLogFileName);
    lineEdit_logFileName->setEnabled(pHost->mLogFileNameFormat.isEmpty());
    mIsToLogInHtml->setChecked(pHost->mIsNextLogFileInHtmlFormat);
    mIsLoggingTimestamps->setChecked(pHost->mIsLoggingTimestamps);
    commandLineMinimumHeight->setValue(pHost->commandLineMinimumHeight);
    mNoAntiAlias->setChecked(!pHost->mNoAntiAlias);
    mFORCE_MCCP_OFF->setChecked(pHost->mFORCE_NO_COMPRESSION);
    mFORCE_GA_OFF->setChecked(pHost->mFORCE_GA_OFF);
    mAlertOnNewData->setChecked(pHost->mAlertOnNewData);
    //mMXPMode->setCurrentIndex( pHost->mMXPMode );
    //encoding->setCurrentIndex( pHost->mEncoding );
    mFORCE_SAVE_ON_EXIT->setChecked(pHost->mFORCE_SAVE_ON_EXIT);
    mEnableGMCP->setChecked(pHost->mEnableGMCP);
    mEnableMSDP->setChecked(pHost->mEnableMSDP);

    // load profiles into mappers "copy map to profile" combobox
    // this feature should work seamlessly both for online and offline profiles
    QStringList profileList = QDir(mudlet::getMudletPath(mudlet::profilesPath)).entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time); // sort by profile "hotness"
    pushButton_chooseProfiles->setEnabled(false);
    pushButton_copyMap->setEnabled(false);
    if (!mpMenu) {
        mpMenu = new QMenu(tr("Other profiles to Map to:"));
    }

    mpMenu->clear();
    for (unsigned int i = 0, total = profileList.size(); i < total; ++i) {
        QString s = profileList.at(i);
        if (s.isEmpty() || !s.compare(pHost->getName()) || !s.compare(QStringLiteral("default_host"))) {
            // Do not include THIS profile in the list - it will
            // automatically get saved - as the file to copy to the other
            // profiles!  Also exclude the dummy "default_host" one
            continue;
        }

        auto pItem = new QAction(s, nullptr);
        pItem->setCheckable(true);
        pItem->setChecked(false);
        mpMenu->addAction(pItem);
        //Enable it as we now have at least one profile to copy to
        pushButton_chooseProfiles->setEnabled(true);
    }

    pushButton_chooseProfiles->setMenu(mpMenu);


    // label to show on successful map file action
    label_mapFileActionResult->hide();

    //doubleclick ignore
    QString ignore;
    QSetIterator<QChar> it(pHost->mDoubleClickIgnore);
    while (it.hasNext()) {
        ignore = ignore.append(it.next());
    }
    doubleclick_ignore_lineedit->setText(ignore);

    // FIXME: Check this each time that it is appropriate for THIS build version
    comboBox_mapFileSaveFormatVersion->clear();
    // Add default version:
    comboBox_mapFileSaveFormatVersion->addItem(tr("%1 {Default, recommended}").arg(pHost->mpMap->mDefaultVersion), QVariant(pHost->mpMap->mDefaultVersion));
    comboBox_mapFileSaveFormatVersion->setEnabled(false);
    label_mapFileSaveFormatVersion->setEnabled(false);
    if (pHost->mpMap->mMaxVersion > pHost->mpMap->mDefaultVersion || pHost->mpMap->mMinVersion < pHost->mpMap->mDefaultVersion) {
        for (short int i = pHost->mpMap->mMinVersion; i <= pHost->mpMap->mMaxVersion; ++i) {
            if (i == pHost->mpMap->mDefaultVersion) {
                continue;
            }
            comboBox_mapFileSaveFormatVersion->setEnabled(true);
            label_mapFileSaveFormatVersion->setEnabled(true);
            if (i > pHost->mpMap->mDefaultVersion) {
                comboBox_mapFileSaveFormatVersion->addItem(tr("%1 {Upgraded, experimental/testing, NOT recommended}").arg(i), QVariant(i));
            } else {
                comboBox_mapFileSaveFormatVersion->addItem(tr("%1 {Downgraded, for sharing with older version users, NOT recommended}").arg(i), QVariant(i));
            }
        }
        int _indexForCurrentSaveFormat = comboBox_mapFileSaveFormatVersion->findData(pHost->mpMap->mSaveVersion, Qt::UserRole);
        if (_indexForCurrentSaveFormat >= 0) {
            comboBox_mapFileSaveFormatVersion->setCurrentIndex(_indexForCurrentSaveFormat);
        }
    }
    if (pHost->mpMap->mpMapper) {
        QLabel* pLabel_mapSymbolFontFudge = new QLabel(tr("2D Map Room Symbol scaling factor:"), groupBox_debug);
        mpDoubleSpinBox_mapSymbolFontFudge = new QDoubleSpinBox(groupBox_debug);
        mpDoubleSpinBox_mapSymbolFontFudge->setValue(pHost->mpMap->mMapSymbolFontFudgeFactor);
        mpDoubleSpinBox_mapSymbolFontFudge->setPrefix(QStringLiteral("×"));
        mpDoubleSpinBox_mapSymbolFontFudge->setRange(0.50, 2.00);
        mpDoubleSpinBox_mapSymbolFontFudge->setSingleStep(0.01);
        QFormLayout* pdebugLayout = qobject_cast<QFormLayout*>(groupBox_debug->layout());
        if (pdebugLayout) {
            pdebugLayout->addRow(pLabel_mapSymbolFontFudge, mpDoubleSpinBox_mapSymbolFontFudge);
            groupBox_debug->show();
        } else {
            qWarning() << "dlgProfilePreferences::initWithHost(...) WARNING - Unable to cast groupBox_debug layout to expected QFormLayout - someone has messed with the profile_preferences.ui file and the contents of the groupBox can not be shown...!";
        }

        label_mapSymbolsFont->setEnabled(true);
        fontComboBox_mapSymbols->setEnabled(true);
        checkBox_isOnlyMapSymbolFontToBeUsed->setEnabled(true);

        checkBox_showDefaultArea->show();
        checkBox_showDefaultArea->setText(tr(R"(Show "%1" in the map area selection)").arg(pHost->mpMap->mpRoomDB->getDefaultAreaName()));
        checkBox_showDefaultArea->setChecked(pHost->mpMap->mpMapper->getDefaultAreaShown());

        pushButton_showGlyphUsage->setEnabled(true);
        fontComboBox_mapSymbols->setCurrentFont(pHost->mpMap->mMapSymbolFont);
        checkBox_isOnlyMapSymbolFontToBeUsed->setChecked(pHost->mpMap->mIsOnlyMapSymbolFontToBeUsed);
        connect(pushButton_showGlyphUsage, SIGNAL(clicked(bool)), this, SLOT(slot_showMapGlyphUsage()), Qt::UniqueConnection);
        connect(fontComboBox_mapSymbols, SIGNAL(currentFontChanged(const QFont&)), this, SLOT(slot_setMapSymbolFont(const QFont&)), Qt::UniqueConnection);
        connect(checkBox_isOnlyMapSymbolFontToBeUsed, SIGNAL(clicked(bool)), this, SLOT(slot_setMapSymbolFontStrategy(bool)), Qt::UniqueConnection);
    } else {
        label_mapSymbolsFont->setEnabled(false);
        fontComboBox_mapSymbols->setEnabled(false);
        checkBox_isOnlyMapSymbolFontToBeUsed->setEnabled(false);
        pushButton_showGlyphUsage->setEnabled(false);

        checkBox_showDefaultArea->hide();
    }

    comboBox_encoding->addItem(QLatin1String("ASCII"));
    comboBox_encoding->addItems(pHost->mTelnet.getFriendlyEncodingsList());
    if (pHost->mTelnet.getEncoding().isEmpty()) {
        // cTelnet::mEncoding is (or should be) empty for the default 7-bit
        // ASCII case, so need to set the control specially to its (the
        // first) value
        comboBox_encoding->setCurrentIndex(0);
    } else {
        comboBox_encoding->setCurrentText(pHost->mTelnet.getFriendlyEncoding());
    }


    // Enable the controls that would be disabled if there wasn't a Host instance
    // on tab_general:
    // groupBox_iconsAndToolbars is NOT dependent on pHost - leave it alone
    enableHostDetails();

    // CHECKME: Have moved ALL the connects, where possible, to the end so that
    // none are triggered by the setup operations...
    connect(pushButton_command_line_foreground_color, SIGNAL(clicked()), this, SLOT(setCommandLineFgColor()));
    connect(pushButton_command_line_background_color, SIGNAL(clicked()), this, SLOT(setCommandLineBgColor()));

    connect(pushButton_black, SIGNAL(clicked()), this, SLOT(setColorBlack()));
    connect(pushButton_Lblack, SIGNAL(clicked()), this, SLOT(setColorLightBlack()));
    connect(pushButton_green, SIGNAL(clicked()), this, SLOT(setColorGreen()));
    connect(pushButton_Lgreen, SIGNAL(clicked()), this, SLOT(setColorLightGreen()));
    connect(pushButton_red, SIGNAL(clicked()), this, SLOT(setColorRed()));
    connect(pushButton_Lred, SIGNAL(clicked()), this, SLOT(setColorLightRed()));
    connect(pushButton_blue, SIGNAL(clicked()), this, SLOT(setColorBlue()));
    connect(pushButton_Lblue, SIGNAL(clicked()), this, SLOT(setColorLightBlue()));
    connect(pushButton_yellow, SIGNAL(clicked()), this, SLOT(setColorYellow()));
    connect(pushButton_Lyellow, SIGNAL(clicked()), this, SLOT(setColorLightYellow()));
    connect(pushButton_cyan, SIGNAL(clicked()), this, SLOT(setColorCyan()));
    connect(pushButton_Lcyan, SIGNAL(clicked()), this, SLOT(setColorLightCyan()));
    connect(pushButton_magenta, SIGNAL(clicked()), this, SLOT(setColorMagenta()));
    connect(pushButton_Lmagenta, SIGNAL(clicked()), this, SLOT(setColorLightMagenta()));
    connect(pushButton_white, SIGNAL(clicked()), this, SLOT(setColorWhite()));
    connect(pushButton_Lwhite, SIGNAL(clicked()), this, SLOT(setColorLightWhite()));

    connect(pushButton_foreground_color, SIGNAL(clicked()), this, SLOT(setFgColor()));
    connect(pushButton_background_color, SIGNAL(clicked()), this, SLOT(setBgColor()));
    connect(pushButton_command_foreground_color, SIGNAL(clicked()), this, SLOT(setCommandFgColor()));
    connect(pushButton_command_background_color, SIGNAL(clicked()), this, SLOT(setCommandBgColor()));

    connect(reset_colors_button, &QAbstractButton::clicked, this, &dlgProfilePreferences::resetColors);
    connect(reset_colors_button_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::resetColors2);

    connect(fontComboBox, SIGNAL(currentFontChanged(const QFont&)), this, SLOT(setDisplayFont()));
    connect(fontSize, SIGNAL(currentIndexChanged(int)), this, SLOT(setFontSize()));

    connect(pushButton_black_2, SIGNAL(clicked()), this, SLOT(setColorBlack2()));
    connect(pushButton_Lblack_2, SIGNAL(clicked()), this, SLOT(setColorLightBlack2()));
    connect(pushButton_green_2, SIGNAL(clicked()), this, SLOT(setColorGreen2()));
    connect(pushButton_Lgreen_2, SIGNAL(clicked()), this, SLOT(setColorLightGreen2()));
    connect(pushButton_red_2, SIGNAL(clicked()), this, SLOT(setColorRed2()));
    connect(pushButton_Lred_2, SIGNAL(clicked()), this, SLOT(setColorLightRed2()));
    connect(pushButton_blue_2, SIGNAL(clicked()), this, SLOT(setColorBlue2()));
    connect(pushButton_Lblue_2, SIGNAL(clicked()), this, SLOT(setColorLightBlue2()));
    connect(pushButton_yellow_2, SIGNAL(clicked()), this, SLOT(setColorYellow2()));
    connect(pushButton_Lyellow_2, SIGNAL(clicked()), this, SLOT(setColorLightYellow2()));
    connect(pushButton_cyan_2, SIGNAL(clicked()), this, SLOT(setColorCyan2()));
    connect(pushButton_Lcyan_2, SIGNAL(clicked()), this, SLOT(setColorLightCyan2()));
    connect(pushButton_magenta_2, SIGNAL(clicked()), this, SLOT(setColorMagenta2()));
    connect(pushButton_Lmagenta_2, SIGNAL(clicked()), this, SLOT(setColorLightMagenta2()));
    connect(pushButton_white_2, SIGNAL(clicked()), this, SLOT(setColorWhite2()));
    connect(pushButton_Lwhite_2, SIGNAL(clicked()), this, SLOT(setColorLightWhite2()));

    connect(pushButton_foreground_color_2, SIGNAL(clicked()), this, SLOT(setFgColor2()));
    connect(pushButton_background_color_2, SIGNAL(clicked()), this, SLOT(setBgColor2()));

    connect(mEnableGMCP, SIGNAL(clicked()), need_reconnect_for_data_protocol, SLOT(show()));
    connect(mEnableMSDP, SIGNAL(clicked()), need_reconnect_for_data_protocol, SLOT(show()));

    connect(mFORCE_MCCP_OFF, SIGNAL(clicked()), need_reconnect_for_specialoption, SLOT(show()));
    connect(mFORCE_GA_OFF, SIGNAL(clicked()), need_reconnect_for_specialoption, SLOT(show()));
    connect(mpMenu, SIGNAL(triggered(QAction*)), this, SLOT(slot_chooseProfilesChanged(QAction*)));

    connect(pushButton_copyMap, SIGNAL(clicked()), this, SLOT(copyMap()));
    connect(pushButton_loadMap, SIGNAL(clicked()), this, SLOT(loadMap()));
    connect(pushButton_saveMap, SIGNAL(clicked()), this, SLOT(saveMap()));
    connect(comboBox_encoding, SIGNAL(currentTextChanged(const QString&)), this, SLOT(slot_setEncoding(const QString&)));

    connect(pushButton_whereToLog, SIGNAL(clicked()), this, SLOT(slot_setLogDir()));
    connect(pushButton_resetLogDir, SIGNAL(clicked()), this, SLOT(slot_resetLogDir()));
    connect(comboBox_logFileNameFormat, SIGNAL(currentIndexChanged(int)), this, SLOT(slot_logFileNameFormatChange(int)));
}

void dlgProfilePreferences::disconnectHostRelatedControls()
{
    disconnect(buttonDownloadMap, SIGNAL(clicked()));

    disconnect(pushButton_command_line_foreground_color, SIGNAL(clicked()));
    disconnect(pushButton_command_line_background_color, SIGNAL(clicked()));

    disconnect(pushButton_black, SIGNAL(clicked()));
    disconnect(pushButton_Lblack, SIGNAL(clicked()));
    disconnect(pushButton_green, SIGNAL(clicked()));
    disconnect(pushButton_Lgreen, SIGNAL(clicked()));
    disconnect(pushButton_red, SIGNAL(clicked()));
    disconnect(pushButton_Lred, SIGNAL(clicked()));
    disconnect(pushButton_blue, SIGNAL(clicked()));
    disconnect(pushButton_Lblue, SIGNAL(clicked()));
    disconnect(pushButton_yellow, SIGNAL(clicked()));
    disconnect(pushButton_Lyellow, SIGNAL(clicked()));
    disconnect(pushButton_cyan, SIGNAL(clicked()));
    disconnect(pushButton_Lcyan, SIGNAL(clicked()));
    disconnect(pushButton_magenta, SIGNAL(clicked()));
    disconnect(pushButton_Lmagenta, SIGNAL(clicked()));
    disconnect(pushButton_white, SIGNAL(clicked()));
    disconnect(pushButton_Lwhite, SIGNAL(clicked()));

    disconnect(pushButton_foreground_color, SIGNAL(clicked()));
    disconnect(pushButton_background_color, SIGNAL(clicked()));
    disconnect(pushButton_command_foreground_color, SIGNAL(clicked()));
    disconnect(pushButton_command_background_color, SIGNAL(clicked()));

    // The "new" style connect(...) does not have the same range of overloaded
    // disconnect(...) counterparts - so we need to provide the "dummy"
    // arguments to get the wanted wild-card behaviour for them:
    disconnect(reset_colors_button, &QAbstractButton::clicked, 0, 0);
    disconnect(reset_colors_button_2, &QAbstractButton::clicked, 0, 0);

    disconnect(fontComboBox, SIGNAL(currentFontChanged(const QFont&)));
    disconnect(fontSize, SIGNAL(currentIndexChanged(int)));

    disconnect(pushButton_black_2, SIGNAL(clicked()));
    disconnect(pushButton_Lblack_2, SIGNAL(clicked()));
    disconnect(pushButton_green_2, SIGNAL(clicked()));
    disconnect(pushButton_Lgreen_2, SIGNAL(clicked()));
    disconnect(pushButton_red_2, SIGNAL(clicked()));
    disconnect(pushButton_Lred_2, SIGNAL(clicked()));
    disconnect(pushButton_blue_2, SIGNAL(clicked()));
    disconnect(pushButton_Lblue_2, SIGNAL(clicked()));
    disconnect(pushButton_yellow_2, SIGNAL(clicked()));
    disconnect(pushButton_Lyellow_2, SIGNAL(clicked()));
    disconnect(pushButton_cyan_2, SIGNAL(clicked()));
    disconnect(pushButton_Lcyan_2, SIGNAL(clicked()));
    disconnect(pushButton_magenta_2, SIGNAL(clicked()));
    disconnect(pushButton_Lmagenta_2, SIGNAL(clicked()));
    disconnect(pushButton_white_2, SIGNAL(clicked()));
    disconnect(pushButton_Lwhite_2, SIGNAL(clicked()));

    disconnect(pushButton_foreground_color_2, SIGNAL(clicked()));
    disconnect(pushButton_background_color_2, SIGNAL(clicked()));

    disconnect(mEnableGMCP, SIGNAL(clicked()));
    disconnect(mEnableMSDP, SIGNAL(clicked()));

    disconnect(mFORCE_MCCP_OFF, SIGNAL(clicked()));
    disconnect(mFORCE_GA_OFF, SIGNAL(clicked()));

    disconnect(mpMenu, SIGNAL(triggered(QAction*)));
    disconnect(pushButton_copyMap, SIGNAL(clicked()));
    disconnect(pushButton_loadMap, SIGNAL(clicked()));
    disconnect(pushButton_saveMap, SIGNAL(clicked()));

    disconnect(comboBox_encoding, SIGNAL(currentTextChanged(const QString&)));
}

void dlgProfilePreferences::clearHostDetails()
{
    code_editor_theme_selection_combobox->clear();
    script_preview_combobox->clear();
    edbeePreviewWidget->textDocument()->setText(QString());

    mFORCE_MXP_NEGOTIATION_OFF->setChecked(false);
    mMapperUseAntiAlias->setChecked(false);
    acceptServerGUI->setChecked(false);

    // Given that the IRC sub-system can handle there NOT being an active host
    // this may need revising - but then the IRC sub-system may need to be fixed
    // to handle switching back to the "default_host" should the currently
    // active one be the one "going away" at this point...
    ircHostName->clear();
    ircHostPort->clear();
    ircChannels->clear();
    ircNick->clear();

    dictList->clear();
    enableSpellCheck->setChecked(false);
    checkBox_echoLuaErrors->setChecked(false);

    groupBox_downloadMapOptions->setVisible(false);

    fontSize->clear();

    need_reconnect_for_data_protocol->hide();

    need_reconnect_for_specialoption->hide();

    fontComboBox->clear();

    fontSize->clear();

    setColors();
    setColors2();

    wrap_at_spinBox->clear();
    indent_wrapped_spinBox->clear();

    show_sent_text_checkbox->setChecked(false);
    auto_clear_input_line_checkbox->setChecked(false);
    command_separator_lineedit->clear();

    checkBox_USE_IRE_DRIVER_BUGFIX->setChecked(false);
    checkBox_mUSE_FORCE_LF_AFTER_PROMPT->setChecked(false);
    USE_UNIX_EOL->setChecked(false);
    topBorderHeight->clear();
    bottomBorderHeight->clear();
    leftBorderWidth->clear();
    rightBorderWidth->clear();
    mIsToLogInHtml->setChecked(false);
    mIsLoggingTimestamps->setChecked(false);
    commandLineMinimumHeight->clear();
    mNoAntiAlias->setChecked(false);
    mFORCE_MCCP_OFF->setChecked(false);
    mFORCE_GA_OFF->setChecked(false);
    mAlertOnNewData->setChecked(false);
    mFORCE_SAVE_ON_EXIT->setChecked(false);
    mEnableGMCP->setChecked(false);
    mEnableMSDP->setChecked(false);

    pushButton_chooseProfiles->setEnabled(false);
    pushButton_copyMap->setEnabled(false);
    if (mpMenu) {
        mpMenu->deleteLater();
        mpMenu = nullptr;
    }

    pushButton_chooseProfiles->setEnabled(false);

    label_mapFileActionResult->hide();

    doubleclick_ignore_lineedit->clear();

    comboBox_mapFileSaveFormatVersion->clear();
    comboBox_mapFileSaveFormatVersion->setEnabled(true);
    label_mapFileSaveFormatVersion->setEnabled(false);
    checkBox_showDefaultArea->setChecked(false);
    checkBox_showDefaultArea->hide();

    comboBox_encoding->clear();

    mSearchEngineMap.clear();
    search_engine_combobox->clear();
}

void dlgProfilePreferences::loadEditorTab()
{
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }

    connect(tabWidget, &QTabWidget::currentChanged, this, &dlgProfilePreferences::slot_editor_tab_selected);

    auto config = edbeePreviewWidget->config();
    config->beginChanges();
    config->setSmartTab(true);
    config->setCaretBlinkRate(200);
    config->setIndentSize(2);
    config->setThemeName(pHost->mEditorTheme);
    config->setCaretWidth(1);
    config->setShowWhitespaceMode(mudlet::self()->mEditorTextOptions & QTextOption::ShowTabsAndSpaces ? 1 : 0);
    config->setUseLineSeparator(mudlet::self()->mEditorTextOptions & QTextOption::ShowLineAndParagraphSeparators);
    config->setFont(pHost->mDisplayFont);
    config->endChanges();
    edbeePreviewWidget->textDocument()->setLanguageGrammar(edbee::Edbee::instance()->grammarManager()->detectGrammarWithFilename(QLatin1Literal("Buck.lua")));
    // disable shadows as their purpose (notify there is more text) is performed by scrollbars already
    edbeePreviewWidget->textScrollArea()->enableShadowWidget(false);

    populateThemesList();
    mudlet::loadEdbeeTheme(pHost->mEditorTheme, pHost->mEditorThemeFile);
    populateScriptsList();

    // pre-select the current theme
    code_editor_theme_selection_combobox->lineEdit()->setPlaceholderText(QStringLiteral("Select theme"));
    auto themeIndex = code_editor_theme_selection_combobox->findText(pHost->mEditorTheme);
    code_editor_theme_selection_combobox->setCurrentIndex(themeIndex);
    slot_theme_selected(themeIndex);

    code_editor_theme_selection_combobox->setInsertPolicy(QComboBox::NoInsert);
    code_editor_theme_selection_combobox->setMaxVisibleItems(20);

    // pre-select the last shown script to preview
    script_preview_combobox->lineEdit()->setPlaceholderText(QStringLiteral("Select script to preview"));
    auto scriptIndex = script_preview_combobox->findData(QVariant::fromValue(QPair<QString, int>(pHost->mThemePreviewType, pHost->mThemePreviewItemID)));
    script_preview_combobox->setCurrentIndex(scriptIndex == -1 ? 1 : scriptIndex);
    slot_script_selected(scriptIndex == -1 ? 1 : scriptIndex);

    script_preview_combobox->setInsertPolicy(QComboBox::NoInsert);
    script_preview_combobox->setMaxVisibleItems(20);
    script_preview_combobox->setDuplicatesEnabled(true);

    theme_download_label->hide();

    // changes the theme being previewed
    connect(code_editor_theme_selection_combobox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &dlgProfilePreferences::slot_theme_selected);

    // allows people to select a script of theirs to preview
    connect(script_preview_combobox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &dlgProfilePreferences::slot_script_selected);

    // fire tab selection event manually should the dialog open on it by default
    if (tabWidget->currentIndex() == 3) {
        slot_editor_tab_selected(3);
    }
}

void dlgProfilePreferences::setColors()
{
    Host* pHost = mpHost;
    if (pHost) {
        pushButton_foreground_color->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(pHost->mFgColor.name()));
        pushButton_background_color->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(pHost->mBgColor.name()));
        pushButton_black->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(pHost->mBlack.name()));
        pushButton_Lblack->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(pHost->mLightBlack.name()));
        pushButton_red->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(pHost->mRed.name()));
        pushButton_Lred->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(pHost->mLightRed.name()));
        pushButton_green->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(pHost->mGreen.name()));
        pushButton_Lgreen->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(pHost->mLightGreen.name()));
        pushButton_blue->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(pHost->mBlue.name()));
        pushButton_Lblue->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(pHost->mLightBlue.name()));
        pushButton_yellow->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(pHost->mYellow.name()));
        pushButton_Lyellow->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(pHost->mLightYellow.name()));
        pushButton_cyan->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(pHost->mCyan.name()));
        pushButton_Lcyan->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(pHost->mLightCyan.name()));
        pushButton_magenta->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(pHost->mMagenta.name()));
        pushButton_Lmagenta->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(pHost->mLightMagenta.name()));
        pushButton_white->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(pHost->mWhite.name()));
        pushButton_Lwhite->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(pHost->mLightWhite.name()));

        pushButton_command_line_foreground_color->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(pHost->mCommandLineFgColor.name()));
        pushButton_command_line_background_color->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(pHost->mCommandLineBgColor.name()));
        pushButton_command_foreground_color->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(pHost->mCommandFgColor.name()));
        pushButton_command_background_color->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(pHost->mCommandBgColor.name()));
    } else {
        pushButton_foreground_color->setStyleSheet(QString());
        pushButton_background_color->setStyleSheet(QString());

        pushButton_black->setStyleSheet(QString());
        pushButton_Lblack->setStyleSheet(QString());
        pushButton_green->setStyleSheet(QString());
        pushButton_Lgreen->setStyleSheet(QString());
        pushButton_red->setStyleSheet(QString());
        pushButton_Lred->setStyleSheet(QString());
        pushButton_blue->setStyleSheet(QString());
        pushButton_Lblue->setStyleSheet(QString());
        pushButton_yellow->setStyleSheet(QString());
        pushButton_Lyellow->setStyleSheet(QString());
        pushButton_cyan->setStyleSheet(QString());
        pushButton_Lcyan->setStyleSheet(QString());
        pushButton_magenta->setStyleSheet(QString());
        pushButton_Lmagenta->setStyleSheet(QString());
        pushButton_white->setStyleSheet(QString());
        pushButton_Lwhite->setStyleSheet(QString());

        pushButton_command_line_foreground_color->setStyleSheet(QString());
        pushButton_command_line_background_color->setStyleSheet(QString());
        pushButton_command_foreground_color->setStyleSheet(QString());
        pushButton_command_background_color->setStyleSheet(QString());
    }
}

void dlgProfilePreferences::setColors2()
{
    Host* pHost = mpHost;
    if (pHost) {
        pushButton_black_2->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(pHost->mBlack_2.name()));
        pushButton_Lblack_2->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(pHost->mLightBlack_2.name()));
        pushButton_green_2->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(pHost->mGreen_2.name()));
        pushButton_Lgreen_2->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(pHost->mLightGreen_2.name()));
        pushButton_red_2->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(pHost->mRed_2.name()));
        pushButton_Lred_2->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(pHost->mLightRed_2.name()));
        pushButton_blue_2->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(pHost->mBlue_2.name()));
        pushButton_Lblue_2->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(pHost->mLightBlue_2.name()));
        pushButton_yellow_2->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(pHost->mYellow_2.name()));
        pushButton_Lyellow_2->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(pHost->mLightYellow_2.name()));
        pushButton_cyan_2->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(pHost->mCyan_2.name()));
        pushButton_Lcyan_2->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(pHost->mLightCyan_2.name()));
        pushButton_magenta_2->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(pHost->mMagenta_2.name()));
        pushButton_Lmagenta_2->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(pHost->mLightMagenta_2.name()));
        pushButton_white_2->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(pHost->mWhite_2.name()));
        pushButton_Lwhite_2->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(pHost->mLightWhite_2.name()));

        pushButton_foreground_color_2->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(pHost->mFgColor_2.name()));
        pushButton_background_color_2->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(pHost->mBgColor_2.name()));
    } else {
        pushButton_black_2->setStyleSheet(QString());
        pushButton_Lblack_2->setStyleSheet(QString());
        pushButton_green_2->setStyleSheet(QString());
        pushButton_Lgreen_2->setStyleSheet(QString());
        pushButton_red_2->setStyleSheet(QString());
        pushButton_Lred_2->setStyleSheet(QString());
        pushButton_blue_2->setStyleSheet(QString());
        pushButton_Lblue_2->setStyleSheet(QString());
        pushButton_yellow_2->setStyleSheet(QString());
        pushButton_Lyellow_2->setStyleSheet(QString());
        pushButton_cyan_2->setStyleSheet(QString());
        pushButton_Lcyan_2->setStyleSheet(QString());
        pushButton_magenta_2->setStyleSheet(QString());
        pushButton_Lmagenta_2->setStyleSheet(QString());
        pushButton_white_2->setStyleSheet(QString());
        pushButton_Lwhite_2->setStyleSheet(QString());

        pushButton_foreground_color_2->setStyleSheet(QString());
        pushButton_background_color_2->setStyleSheet(QString());
    }
}

void dlgProfilePreferences::resetColors()
{
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }

    pHost->mCommandLineFgColor = Qt::darkGray;
    pHost->mCommandLineBgColor = Qt::black;
    pHost->mCommandFgColor = QColor(113, 113, 0);
    pHost->mCommandBgColor = Qt::black;
    pHost->mFgColor = Qt::lightGray;
    pHost->mBgColor = Qt::black;
    pHost->mBlack = Qt::black;
    pHost->mLightBlack = Qt::darkGray;
    pHost->mRed = Qt::darkRed;
    pHost->mLightRed = Qt::red;
    pHost->mGreen = Qt::darkGreen;
    pHost->mLightGreen = Qt::green;
    pHost->mBlue = Qt::darkBlue;
    pHost->mLightBlue = Qt::blue;
    pHost->mYellow = Qt::darkYellow;
    pHost->mLightYellow = Qt::yellow;
    pHost->mCyan = Qt::darkCyan;
    pHost->mLightCyan = Qt::cyan;
    pHost->mMagenta = Qt::darkMagenta;
    pHost->mLightMagenta = Qt::magenta;
    pHost->mWhite = Qt::lightGray;
    pHost->mLightWhite = Qt::white;

    setColors();
    if (mudlet::self()->mConsoleMap.contains(pHost)) {
        mudlet::self()->mConsoleMap[pHost]->changeColors();
    }
}

void dlgProfilePreferences::resetColors2()
{
    Host* pHost = mpHost;

    if (!pHost) {
        return;
    }

    pHost->mFgColor_2 = Qt::lightGray;
    pHost->mBgColor_2 = Qt::black;
    pHost->mBlack_2 = Qt::black;
    pHost->mLightBlack_2 = Qt::darkGray;
    pHost->mRed_2 = Qt::darkRed;
    pHost->mLightRed_2 = Qt::red;
    pHost->mGreen_2 = Qt::darkGreen;
    pHost->mLightGreen_2 = Qt::green;
    pHost->mBlue_2 = Qt::darkBlue;
    pHost->mLightBlue_2 = Qt::blue;
    pHost->mYellow_2 = Qt::darkYellow;
    pHost->mLightYellow_2 = Qt::yellow;
    pHost->mCyan_2 = Qt::darkCyan;
    pHost->mLightCyan_2 = Qt::cyan;
    pHost->mMagenta_2 = Qt::darkMagenta;
    pHost->mLightMagenta_2 = Qt::magenta;
    pHost->mWhite_2 = Qt::lightGray;
    pHost->mLightWhite_2 = Qt::white;

    setColors2();
}

void dlgProfilePreferences::setColor(QPushButton* b, QColor& c)
{
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }

    auto color = QColorDialog::getColor(c, this);
    if (color.isValid()) {
        c = color;
        if (mudlet::self()->mConsoleMap.contains(pHost)) {
            mudlet::self()->mConsoleMap[pHost]->changeColors();
        }

        b->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(color.name()));
    }
}

void dlgProfilePreferences::setFgColor()
{
    Host* pHost = mpHost;
    if (pHost) {
        setColor(pushButton_foreground_color, pHost->mFgColor);
    }
}

void dlgProfilePreferences::setBgColor()
{
    Host* pHost = mpHost;
    if (pHost) {
        setColor(pushButton_background_color, pHost->mBgColor);
    }
}

void dlgProfilePreferences::setCommandFgColor()
{
    Host* pHost = mpHost;
    if (pHost) {
        setColor(pushButton_command_foreground_color, pHost->mCommandFgColor);
    }
}

void dlgProfilePreferences::setCommandLineFgColor()
{
    Host* pHost = mpHost;
    if (pHost) {
        setColor(pushButton_command_line_foreground_color, pHost->mCommandLineFgColor);
    }
}

void dlgProfilePreferences::setCommandLineBgColor()
{
    Host* pHost = mpHost;
    if (pHost) {
        setColor(pushButton_command_line_background_color, pHost->mCommandLineBgColor);
    }
}

void dlgProfilePreferences::setCommandBgColor()
{
    Host* pHost = mpHost;
    if (pHost) {
        setColor(pushButton_command_background_color, pHost->mCommandBgColor);
    }
}

void dlgProfilePreferences::setFontSize()
{
    mFontSize = fontSize->currentIndex() + 1;
    // delay setting pHost->mDisplayFont until save is clicked by the user.
    //setDisplayFont();
}

void dlgProfilePreferences::setDisplayFont()
{
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }
    QFont font = fontComboBox->currentFont();
    font.setPointSize(mFontSize);
    if (pHost->mDisplayFont != font) {
        pHost->mDisplayFont = font;
        if (mudlet::self()->mConsoleMap.contains(pHost)) {
            mudlet::self()->mConsoleMap[pHost]->changeColors();

            // update the display properly when font or size selections change.
            mudlet::self()->mConsoleMap[pHost]->console->updateScreenView();
            mudlet::self()->mConsoleMap[pHost]->console->forceUpdate();
            mudlet::self()->mConsoleMap[pHost]->console2->updateScreenView();
            mudlet::self()->mConsoleMap[pHost]->console2->forceUpdate();
            mudlet::self()->mConsoleMap[pHost]->refresh();
        }
        auto config = edbeePreviewWidget->config();
        config->beginChanges();
        config->setFont(font);
        config->endChanges();
    }
}

// Currently UNUSED!
void dlgProfilePreferences::setCommandLineFont()
{
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }
    bool ok;
    QFont font = QFontDialog::getFont(&ok, pHost->mCommandLineFont, this);
    pHost->mCommandLineFont = font;
    if (mudlet::self()->mConsoleMap.contains(pHost)) {
        mudlet::self()->mConsoleMap[pHost]->changeColors();
    }
}

void dlgProfilePreferences::setColorBlack()
{
    Host* pHost = mpHost;
    if (pHost) {
        setColor(pushButton_black, pHost->mBlack);
    }
}

void dlgProfilePreferences::setColorLightBlack()
{
    Host* pHost = mpHost;
    if (pHost) {
        setColor(pushButton_Lblack, pHost->mLightBlack);
    }
}

void dlgProfilePreferences::setColorRed()
{
    Host* pHost = mpHost;
    if (pHost) {
        setColor(pushButton_red, pHost->mRed);
    }
}

void dlgProfilePreferences::setColorLightRed()
{
    Host* pHost = mpHost;
    if (pHost) {
        setColor(pushButton_Lred, pHost->mLightRed);
    }
}

void dlgProfilePreferences::setColorGreen()
{
    Host* pHost = mpHost;
    if (pHost) {
        setColor(pushButton_green, pHost->mGreen);
    }
}

void dlgProfilePreferences::setColorLightGreen()
{
    Host* pHost = mpHost;
    if (pHost) {
        setColor(pushButton_Lgreen, pHost->mLightGreen);
    }
}

void dlgProfilePreferences::setColorBlue()
{
    Host* pHost = mpHost;
    if (pHost) {
        setColor(pushButton_blue, pHost->mBlue);
    }
}

void dlgProfilePreferences::setColorLightBlue()
{
    Host* pHost = mpHost;
    if (pHost) {
        setColor(pushButton_Lblue, pHost->mLightBlue);
    }
}

void dlgProfilePreferences::setColorYellow()
{
    Host* pHost = mpHost;
    if (pHost) {
        setColor(pushButton_yellow, pHost->mYellow);
    }
}

void dlgProfilePreferences::setColorLightYellow()
{
    Host* pHost = mpHost;
    if (pHost) {
        setColor(pushButton_Lyellow, pHost->mLightYellow);
    }
}

void dlgProfilePreferences::setColorCyan()
{
    Host* pHost = mpHost;
    if (pHost) {
        setColor(pushButton_cyan, pHost->mCyan);
    }
}

void dlgProfilePreferences::setColorLightCyan()
{
    Host* pHost = mpHost;
    if (pHost) {
        setColor(pushButton_Lcyan, pHost->mLightCyan);
    }
}

void dlgProfilePreferences::setColorMagenta()
{
    Host* pHost = mpHost;
    if (pHost) {
        setColor(pushButton_magenta, pHost->mMagenta);
    }
}

void dlgProfilePreferences::setColorLightMagenta()
{
    Host* pHost = mpHost;
    if (pHost) {
        setColor(pushButton_Lmagenta, pHost->mLightMagenta);
    }
}

void dlgProfilePreferences::setColorWhite()
{
    Host* pHost = mpHost;
    if (pHost) {
        setColor(pushButton_white, pHost->mWhite);
    }
}

void dlgProfilePreferences::setColorLightWhite()
{
    Host* pHost = mpHost;
    if (pHost) {
        setColor(pushButton_Lwhite, pHost->mLightWhite);
    }
}

void dlgProfilePreferences::setFgColor2()
{
    Host* pHost = mpHost;
    if (pHost) {
        setColor(pushButton_foreground_color_2, pHost->mFgColor_2);
    }
}

void dlgProfilePreferences::setBgColor2()
{
    Host* pHost = mpHost;
    if (pHost) {
        setColor(pushButton_background_color_2, pHost->mBgColor_2);
    }
}

void dlgProfilePreferences::setColorBlack2()
{
    Host* pHost = mpHost;
    if (pHost) {
        setColor(pushButton_black_2, pHost->mBlack_2);
    }
}

void dlgProfilePreferences::setColorLightBlack2()
{
    Host* pHost = mpHost;
    if (pHost) {
        setColor(pushButton_Lblack_2, pHost->mLightBlack_2);
    }
}

void dlgProfilePreferences::setColorRed2()
{
    Host* pHost = mpHost;
    if (pHost) {
        setColor(pushButton_red_2, pHost->mRed_2);
    }
}

void dlgProfilePreferences::setColorLightRed2()
{
    Host* pHost = mpHost;
    if (pHost) {
        setColor(pushButton_Lred_2, pHost->mLightRed_2);
    }
}

void dlgProfilePreferences::setColorGreen2()
{
    Host* pHost = mpHost;
    if (pHost) {
        setColor(pushButton_green_2, pHost->mGreen_2);
    }
}

void dlgProfilePreferences::setColorLightGreen2()
{
    Host* pHost = mpHost;
    if (pHost) {
        setColor(pushButton_Lgreen_2, pHost->mLightGreen_2);
    }
}

void dlgProfilePreferences::setColorBlue2()
{
    Host* pHost = mpHost;
    if (pHost) {
        setColor(pushButton_blue_2, pHost->mBlue_2);
    }
}

void dlgProfilePreferences::setColorLightBlue2()
{
    Host* pHost = mpHost;
    if (pHost) {
        setColor(pushButton_Lblue_2, pHost->mLightBlue_2);
    }
}

void dlgProfilePreferences::setColorYellow2()
{
    Host* pHost = mpHost;
    if (pHost) {
        setColor(pushButton_yellow_2, pHost->mYellow_2);
    }
}

void dlgProfilePreferences::setColorLightYellow2()
{
    Host* pHost = mpHost;
    if (pHost) {
        setColor(pushButton_Lyellow_2, pHost->mLightYellow_2);
    }
}

void dlgProfilePreferences::setColorCyan2()
{
    Host* pHost = mpHost;
    if (pHost) {
        setColor(pushButton_cyan_2, pHost->mCyan_2);
    }
}

void dlgProfilePreferences::setColorLightCyan2()
{
    Host* pHost = mpHost;
    if (pHost) {
        setColor(pushButton_Lcyan_2, pHost->mLightCyan_2);
    }
}

void dlgProfilePreferences::setColorMagenta2()
{
    Host* pHost = mpHost;
    if (pHost) {
        setColor(pushButton_magenta_2, pHost->mMagenta_2);
    }
}

void dlgProfilePreferences::setColorLightMagenta2()
{
    Host* pHost = mpHost;
    if (pHost) {
        setColor(pushButton_Lmagenta_2, pHost->mLightMagenta_2);
    }
}

void dlgProfilePreferences::setColorWhite2()
{
    Host* pHost = mpHost;
    if (pHost) {
        setColor(pushButton_white_2, pHost->mWhite_2);
    }
}

void dlgProfilePreferences::setColorLightWhite2()
{
    Host* pHost = mpHost;
    if (pHost) {
        setColor(pushButton_Lwhite_2, pHost->mLightWhite_2);
    }
}

void dlgProfilePreferences::downloadMap()
{
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }
    if (!pHost->mpMap->mpMapper) {
        // CHECK: What happens if we are NOT the current profile anymore?
        mudlet::self()->createMapper(false);
    }

    pHost->mpMap->downloadMap();
}

void dlgProfilePreferences::loadMap()
{
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }

    QString fileName = QFileDialog::getOpenFileName(
                           this,
                           tr("Load Mudlet map"),
                           mudlet::getMudletPath(mudlet::profileMapsPath, pHost->getName()),
                           tr("Mudlet map (*.dat);;Xml map data (*.xml);;Any file (*)",
                              "Do not change extensions (in braces) they are used programmatically!"));
    if (fileName.isEmpty()) {
        return;
    }

    label_mapFileActionResult->show();

    // Ensure the setting is already made as the loadMap(...) uses the set value
    bool showAuditErrors = mudlet::self()->showMapAuditErrors();
    mudlet::self()->setShowMapAuditErrors(checkBox_reportMapIssuesOnScreen->isChecked());

    if (fileName.endsWith(QStringLiteral(".xml"), Qt::CaseInsensitive)) {
        label_mapFileActionResult->setText(tr("Importing map - please wait..."));
        qApp->processEvents(); // Needed to make the above message show up when loading big maps

        if (pHost->mpConsole->importMap(fileName)) {
            label_mapFileActionResult->setText(tr("Imported map from %1.").arg(fileName));
        } else {
            label_mapFileActionResult->setText(tr("Could not import map from %1.").arg(fileName));
        }
    } else {
        label_mapFileActionResult->setText(tr("Loading map - please wait..."));
        qApp->processEvents(); // Needed to make the above message show up when loading big maps


        if (pHost->mpConsole->loadMap(fileName)) {
            label_mapFileActionResult->setText(tr("Loaded map from %1.").arg(fileName));
        } else {
            label_mapFileActionResult->setText(tr("Could not load map from %1.").arg(fileName));
        }
    }
    QTimer::singleShot(10 * 1000, this, SLOT(hideActionLabel()));

    // Restore setting immediately before we used it
    mudlet::self()->setShowMapAuditErrors(showAuditErrors);
}

void dlgProfilePreferences::saveMap()
{
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }

    QString fileName =
            QFileDialog::getSaveFileName(this, tr("Save Mudlet map"), QDir::homePath(), tr("Mudlet map (*.dat)", "Do not change the extension text (in braces) - it is needed programmatically!"));
    if (fileName.isEmpty()) {
        return;
    }

    if (!fileName.endsWith(QStringLiteral(".dat"), Qt::CaseInsensitive)) {
        fileName.append(QStringLiteral(".dat"));
    }

    label_mapFileActionResult->show();
    label_mapFileActionResult->setText(tr("Saving map - please wait..."));
    qApp->processEvents(); // Copied from "Loading map - please wait..." case
                           // Just in case is needed to make the above message
                           // show up when saving big maps

    // Temporarily use whatever version is currently set
    int oldSaveVersionFormat = pHost->mpMap->mSaveVersion;
    pHost->mpMap->mSaveVersion = comboBox_mapFileSaveFormatVersion->currentData().toInt();

    // Ensure the setting is already made as the saveMap(...) uses the set value
    bool showAuditErrors = mudlet::self()->showMapAuditErrors();
    mudlet::self()->setShowMapAuditErrors(checkBox_reportMapIssuesOnScreen->isChecked());

    if (pHost->mpConsole->saveMap(fileName)) {
        label_mapFileActionResult->setText(tr("Saved map to %1.").arg(fileName));
    } else {
        label_mapFileActionResult->setText(tr("Could not save map to %1.").arg(fileName));
    }
    // Then restore prior version
    pHost->mpMap->mSaveVersion = oldSaveVersionFormat;
    mudlet::self()->setShowMapAuditErrors(showAuditErrors);

    QTimer::singleShot(10 * 1000, this, SLOT(hideActionLabel()));
}

void dlgProfilePreferences::hideActionLabel()
{
    label_mapFileActionResult->hide();
}

void dlgProfilePreferences::copyMap()
{
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }

    QMap<QString, int> toProfilesRoomIdMap;
    QMenu* _menu = pushButton_chooseProfiles->menu();
    QListIterator<QAction*> itAction(_menu->actions());
    while (itAction.hasNext()) {
        QAction* _action = itAction.next();
        if (_action->isChecked()) {
            QString toProfileName = _action->text();
            toProfilesRoomIdMap.insert(toProfileName, 0);
            // 0 is used as sentinel value that we don't have a valid Id yet
            // for the given Host - the contents of this map will be used to
            // update, or rather REPLACE TMap::mRoomIdHash

            // Check for the destination directory for the other profiles
            QDir toProfileDir;
            QString toProfileDirPathString = mudlet::getMudletPath(mudlet::profileMapsPath, toProfileName);
            if (!toProfileDir.exists(toProfileDirPathString)) {
                if (!toProfileDir.mkpath(toProfileDirPathString)) {
                    QString errMsg = tr("[ ERROR ] - Unable to use or create directory to store map for other profile \"%1\".\n"
                                        "Please check that you have permissions/access to:\n"
                                        "\"%2\"\n"
                                        "and there is enough space. The copying operation has failed.")
                                             .arg(toProfileName, toProfileDirPathString);
                    pHost->postMessage(errMsg);
                    label_mapFileActionResult->show();
                    label_mapFileActionResult->setText(tr("Creating a destination directory failed..."));
                    return;
                }
            }
        }
    }

    // Identify which, if any, of the toProfilesRoomIdMap is active and get the current room
    QSet<Host*> activeHosts(mudlet::self()->mConsoleMap.keys().toSet());
    QMap<QString, Host*> activeOtherHostMap;
    QSetIterator<Host*> itActiveHost(activeHosts);
    while (itActiveHost.hasNext()) {
        Host* pOtherHost = itActiveHost.next();
        if (pOtherHost && pHost != pOtherHost && pOtherHost) {
            QString otherHostName = pOtherHost->getName();
            if (toProfilesRoomIdMap.contains(otherHostName)) {
                activeOtherHostMap.insert(otherHostName, pOtherHost);
                toProfilesRoomIdMap.insert(otherHostName, pOtherHost->mpMap->mRoomIdHash.value(otherHostName, -1));
            }
        }
    }
    // otherProfileCurrentRoomId will be -1 if tried and failed to get it from
    // current running profile, > 0 on success or 0 if not running as another profile

    // Ensure the setting is already made as the value could be used in the
    // code following after
    bool savedOldAuditErrorsToConsoleEnabledSetting = mudlet::self()->showMapAuditErrors();
    mudlet::self()->setShowMapAuditErrors(checkBox_reportMapIssuesOnScreen->isChecked());

    // We now KNOW there are places where the destination profiles will/have
    // stored their maps - if we do not already know where the player is in the
    // other profiles - because they aren't active - or have not set it - try
    // and find out what the rooms are from the last saved files - ignoring
    // other details that we have also obtained.
    QMutableMapIterator<QString, int> itOtherProfile(toProfilesRoomIdMap);
    while (itOtherProfile.hasNext()) {
        itOtherProfile.next();
        if (itOtherProfile.value() > 0) {
            // Skip the ones where we have already got the player room from the
            // active profile
            qDebug() << "dlgProfilePreference::copyMap() in other ACTIVE profile:" << itOtherProfile.key() << "\n    the player was located in:" << itOtherProfile.value();
            if (pHost->mpMap->mpRoomDB->getRoom(itOtherProfile.value())) {
                // That room IS in the map we are copying across, so update the
                // local record of it for the map for that profile:
                pHost->mpMap->mRoomIdHash[itOtherProfile.key()] = itOtherProfile.value();
            }
            continue;
        }

        // Most of these we'll just get for debugging!
        QString otherProfileFileUsed;
        int otherProfileRoomCount;
        int otherProfileAreaCount;
        int otherProfileVersion;
        int otherProfileCurrentRoomId; // What we are looking for!
        if( pHost->mpMap->retrieveMapFileStats( itOtherProfile.key(),
                                                 & otherProfileFileUsed,
                                                 & otherProfileVersion,
                                                 & otherProfileCurrentRoomId,
                                                 & otherProfileAreaCount,
                                                 & otherProfileRoomCount ) ) {

            qDebug() << "dlgProfilePreference::copyMap() in other INACTIVE profile:"
                     << itOtherProfile.key()
                     << "\n    the file examined was:"
                     << otherProfileFileUsed
                     << "\n    it was of version:"
                     << otherProfileVersion
                     << "\n    it had an area count of:"
                     << otherProfileAreaCount
                     << "\n    it had a room count of:"
                     << otherProfileRoomCount
                     << "\n    the player was located in:"
                     << otherProfileCurrentRoomId;
            itOtherProfile.setValue(otherProfileCurrentRoomId);
            // Using a mutable iterator we must modify (mutate) the data through
            // the iterator!
            if (pHost->mpMap->mpRoomDB->getRoom(otherProfileCurrentRoomId)) {
                // That room IS in the map we are copying across, so update the
                // local record of it for the map for that profile:
                pHost->mpMap->mRoomIdHash[itOtherProfile.key()] = otherProfileCurrentRoomId;
            }
        }
    }

    // Now, we can save our current map with all the profiles' player room data
    label_mapFileActionResult->show();
    label_mapFileActionResult->setText(tr("Backing up current map - please wait..."));
    qApp->processEvents(); // Copied from "Loading map - please wait..." case
                           // Just in case is needed to make the above message
                           // show up when saving big maps

    // Temporarily use whatever version is currently set
    int oldSaveVersionFormat = pHost->mpMap->mSaveVersion;
    pHost->mpMap->mSaveVersion = comboBox_mapFileSaveFormatVersion->currentData().toInt();

    if (!pHost->mpConsole->saveMap(QString())) {
        label_mapFileActionResult->setText(tr("Could not backup the map - saving it failed."));
        QTimer::singleShot(10 * 1000, this, SLOT(hideActionLabel()));
        return;
    }

    // Then restore prior version
    pHost->mpMap->mSaveVersion = oldSaveVersionFormat;

    // work out which map is latest in THIS profile - which SHOULD be the one
    // we just saved!
    QString thisProfileLatestMapPathFileName;
    QFile thisProfileLatestMapFile;
    QString sourceMapFolder(mudlet::getMudletPath(mudlet::profileMapsPath, pHost->getName()));
    QStringList mProfileList = QDir(sourceMapFolder).entryList(QDir::Files | QDir::NoDotAndDotDot, QDir::Time);
    for (unsigned int i = 0, total = mProfileList.size(); i < total; ++i) {
        thisProfileLatestMapPathFileName = mProfileList.at(i);
        if (thisProfileLatestMapPathFileName.isEmpty()) {
            continue;
        }

        thisProfileLatestMapFile.setFileName(QStringLiteral("%1/%2").arg(sourceMapFolder, thisProfileLatestMapPathFileName));
        break;
    }

    if (thisProfileLatestMapFile.fileName().isEmpty()) {
        label_mapFileActionResult->setText(tr("Could not copy the map - failed to work out which map file we just saved the map as!"));
        QTimer::singleShot(10 * 1000, this, SLOT(hideActionLabel()));
        return;
    }

    // Make the copies into the destination profiles (for all to profiles whether
    // in use or not):
    itOtherProfile.toFront();
    while (itOtherProfile.hasNext()) {
        itOtherProfile.next();
        QString otherHostName = itOtherProfile.key();
        // Copy over into the profiles map folder, so it is loaded first when map is open - this covers the offline case
        label_mapFileActionResult->setText(tr("Copying over map to %1 - please wait...").arg(otherHostName));
        qApp->processEvents(); // Copied from "Loading map - please wait..." case
                               // Just in case is needed to make the above message
                               // show up when saving big maps

        if (!thisProfileLatestMapFile.copy(mudlet::getMudletPath(mudlet::profileMapPathFileName, otherHostName, thisProfileLatestMapPathFileName))) {
            label_mapFileActionResult->setText(tr("Could not copy the map to %1 - unable to copy the new map file over.").arg(otherHostName));
            QTimer::singleShot(10 * 1000, this, SLOT(hideActionLabel()));
            continue; // Try again with next profile
        } else {
            label_mapFileActionResult->setText(tr("Map copied successfully to other profile %1.").arg(otherHostName));
            qApp->processEvents(); // Copied from "Loading map - please wait..." case
                                   // Just in case is needed to make the above message
                                   // show up when saving big maps
        }
    }

    // Finally, signal the other profiles to reload their maps:
    mudlet::self()->requestProfilesToReloadMaps(toProfilesRoomIdMap.keys());
    // GOTCHA: keys() is a QList<QString>, however, though it IS equivalent to a
    // QStringList in many ways, the SLOT/SIGNAL system treats them as different
    // - I thinK - so use QList<QString> thoughout the SIGNAL/SLOT links Slysven!
    label_mapFileActionResult->setText(tr("Map copied, now signalling other profiles to reload it."));
    QTimer::singleShot(10 * 1000, this, SLOT(hideActionLabel()));

    // CHECK: Race condition? We might be changing this whilst other profile
    // are accessing it...
    mudlet::self()->setShowMapAuditErrors(savedOldAuditErrorsToConsoleEnabledSetting);
}

void dlgProfilePreferences::slot_setLogDir()
{
    Host* pHost = mpHost;
    if (!pHost)
        return;

    QDir currentLogDir = QFileDialog::getExistingDirectory(this, tr("Select log directory"), pHost->mLogDir, QFileDialog::ShowDirsOnly);
    mLogDirPath = currentLogDir.absolutePath();

    return;
}

void dlgProfilePreferences::slot_resetLogDir()
{
    Host* pHost = mpHost;
    if (!pHost)
        return;

    mLogDirPath.clear();
    return;
}

void dlgProfilePreferences::slot_logFileNameFormatChange(const int index)
{
    Q_UNUSED(index);

    Host* pHost = mpHost;
    if (!pHost)
        return;

    lineEdit_logFileName->setEnabled(comboBox_logFileNameFormat->currentData().toString().isEmpty());
}

void dlgProfilePreferences::slot_save_and_exit()
{
    if (mpDialogMapGlyphUsage) {
        mpDialogMapGlyphUsage->close();
        mpDialogMapGlyphUsage = nullptr;
    }

    Host* pHost = mpHost;
    if (pHost) {
        if (dictList->currentItem()) {
            pHost->mSpellDic = dictList->currentItem()->text();
        }

        pHost->mEnableSpellCheck = enableSpellCheck->isChecked();
        pHost->mWrapAt = wrap_at_spinBox->value();
        pHost->mWrapIndentCount = indent_wrapped_spinBox->value();
        pHost->mPrintCommand = show_sent_text_checkbox->isChecked();
        pHost->mAutoClearCommandLineAfterSend = auto_clear_input_line_checkbox->isChecked();
        pHost->mCommandSeparator = command_separator_lineedit->text();
        pHost->mAcceptServerGUI = acceptServerGUI->isChecked();
        pHost->mUSE_IRE_DRIVER_BUGFIX = checkBox_USE_IRE_DRIVER_BUGFIX->isChecked();
        pHost->set_USE_IRE_DRIVER_BUGFIX(checkBox_USE_IRE_DRIVER_BUGFIX->isChecked());
        pHost->mUSE_FORCE_LF_AFTER_PROMPT = checkBox_mUSE_FORCE_LF_AFTER_PROMPT->isChecked();
        pHost->mUSE_UNIX_EOL = USE_UNIX_EOL->isChecked();
        pHost->mFORCE_NO_COMPRESSION = mFORCE_MCCP_OFF->isChecked();
        pHost->mFORCE_GA_OFF = mFORCE_GA_OFF->isChecked();
        pHost->mFORCE_SAVE_ON_EXIT = mFORCE_SAVE_ON_EXIT->isChecked();
        pHost->mEnableGMCP = mEnableGMCP->isChecked();
        pHost->mEnableMSDP = mEnableMSDP->isChecked();
        pHost->mMapperUseAntiAlias = mMapperUseAntiAlias->isChecked();
        if (pHost->mpMap && pHost->mpMap->mpMapper) {
            pHost->mpMap->mpMapper->mp2dMap->mMapperUseAntiAlias = mMapperUseAntiAlias->isChecked();
            bool isAreaWidgetInNeedOfResetting = false;
            if ((!pHost->mpMap->mpMapper->getDefaultAreaShown()) && (checkBox_showDefaultArea->isChecked()) && (pHost->mpMap->mpMapper->mp2dMap->mAID == -1)) {
                isAreaWidgetInNeedOfResetting = true;
            }

            pHost->mpMap->mpMapper->setDefaultAreaShown(checkBox_showDefaultArea->isChecked());
            if (isAreaWidgetInNeedOfResetting) {
                // Corner case fixup:
                pHost->mpMap->mpMapper->showArea->setCurrentText(pHost->mpMap->mpRoomDB->getDefaultAreaName());
            }

            // If a map was loaded
            if (mpDoubleSpinBox_mapSymbolFontFudge) {
                pHost->mpMap->mMapSymbolFontFudgeFactor = mpDoubleSpinBox_mapSymbolFontFudge->value();
            }

            pHost->mpMap->mpMapper->mp2dMap->repaint(); // Forceably redraw it as we ARE currently showing default area
            pHost->mpMap->mpMapper->update();
        }
        pHost->mBorderTopHeight = topBorderHeight->value();
        pHost->mBorderBottomHeight = bottomBorderHeight->value();
        pHost->mBorderLeftWidth = leftBorderWidth->value();
        pHost->mBorderRightWidth = rightBorderWidth->value();
        pHost->commandLineMinimumHeight = commandLineMinimumHeight->value();
        pHost->mFORCE_MXP_NEGOTIATION_OFF = mFORCE_MXP_NEGOTIATION_OFF->isChecked();
        pHost->mIsNextLogFileInHtmlFormat = mIsToLogInHtml->isChecked();
        pHost->mIsLoggingTimestamps = mIsLoggingTimestamps->isChecked();
        pHost->mLogDir = mLogDirPath;
        pHost->mLogFileName = lineEdit_logFileName->text();
        pHost->mLogFileNameFormat = comboBox_logFileNameFormat->currentData().toString();
        pHost->mLogFileNameFormatIndex = comboBox_logFileNameFormat->currentIndex();
        pHost->mNoAntiAlias = !mNoAntiAlias->isChecked();
        pHost->mAlertOnNewData = mAlertOnNewData->isChecked();

        if (mudlet::self()->mConsoleMap.contains(pHost)) {
            mudlet::self()->mConsoleMap[pHost]->changeColors();
        }

        QString lIgnore = doubleclick_ignore_lineedit->text();
        pHost->mDoubleClickIgnore.clear();
        for (auto character : lIgnore) {
            pHost->mDoubleClickIgnore.insert(character);
        }

        pHost->mpMap->mSaveVersion = comboBox_mapFileSaveFormatVersion->currentData().toInt();

        QString oldIrcNick = dlgIRC::readIrcNickName(pHost);
        QString oldIrcHost = dlgIRC::readIrcHostName(pHost);
        QString oldIrcPort = QString::number(dlgIRC::readIrcHostPort(pHost));
        QString oldIrcChannels = dlgIRC::readIrcChannels(pHost).join(" ");

        QString newIrcNick = ircNick->text();
        QString newIrcHost = ircHostName->text();
        QString newIrcPort = ircHostPort->text();
        QString newIrcChannels = ircChannels->text();
        QStringList newChanList;
        int nIrcPort = dlgIRC::DefaultHostPort;
        bool restartIrcClient = false;

        if (newIrcHost.isEmpty()) {
            newIrcHost = dlgIRC::DefaultHostName;
        }

        if (!newIrcPort.isEmpty()) {
            bool ok;
            nIrcPort = newIrcPort.toInt(&ok);
            if (!ok) {
                nIrcPort = dlgIRC::DefaultHostPort;
            } else if (nIrcPort > 65535 || nIrcPort < 1) {
                nIrcPort = dlgIRC::DefaultHostPort;
            }
            newIrcPort = QString::number(nIrcPort);
        }

        if (newIrcNick.isEmpty()) {
            qsrand(QTime::currentTime().msec());
            newIrcNick = QString("%1%2").arg(dlgIRC::DefaultNickName, QString::number(rand() % 10000));
        }

        if (!newIrcChannels.isEmpty()) {
            QStringList tL = newIrcChannels.split(" ", QString::SkipEmptyParts);
            for (QString s : tL) {
                if (s.startsWith("#") || s.startsWith("&") || s.startsWith("+")) {
                    newChanList << s;
                }
            }
        } else {
            newChanList = dlgIRC::DefaultChannels;
        }
        newIrcChannels = newChanList.join(" ");

        if (oldIrcNick != newIrcNick) {
            dlgIRC::writeIrcNickName(pHost, newIrcNick);

            // if the client is active, update our client nickname.
            if (mudlet::self()->mpIrcClientMap[pHost]) {
                mudlet::self()->mpIrcClientMap[pHost]->connection->setNickName(newIrcNick);
            }
        }

        if (oldIrcChannels != newIrcChannels) {
            dlgIRC::writeIrcChannels(pHost, newChanList);
        }

        if (oldIrcHost != newIrcHost) {
            dlgIRC::writeIrcHostName(pHost, newIrcHost);
            restartIrcClient = true;
        }

        if (oldIrcPort != newIrcPort) {
            dlgIRC::writeIrcHostPort(pHost, nIrcPort);
            restartIrcClient = true;
        }

        // restart the irc client if it is active and we have changed host/port.
        if (restartIrcClient && mudlet::self()->mpIrcClientMap[pHost]) {
            mudlet::self()->mpIrcClientMap[pHost]->ircRestart();
        }

        setDisplayFont();

        if (mudlet::self()->mConsoleMap.contains(pHost)) {
            int x = mudlet::self()->mConsoleMap[pHost]->width();
            int y = mudlet::self()->mConsoleMap[pHost]->height();
            QSize s = QSize(x, y);
            QResizeEvent event(s, s);
            QApplication::sendEvent(mudlet::self()->mConsoleMap[pHost], &event);
        }

        pHost->mEchoLuaErrors = checkBox_echoLuaErrors->isChecked();
        pHost->mEditorTheme = code_editor_theme_selection_combobox->currentText();
        pHost->mEditorThemeFile = code_editor_theme_selection_combobox->currentData().toString();
        if (pHost->mpEditorDialog) {
            pHost->mpEditorDialog->setThemeAndOtherSettings(pHost->mEditorTheme);
        }

        auto data = script_preview_combobox->currentData().value<QPair<QString, int>>();
        pHost->mThemePreviewItemID = data.second;
        pHost->mThemePreviewType = data.first;

        pHost->mSearchEngineName = search_engine_combobox->currentText();
    }

#if defined(INCLUDE_UPDATER)
    mudlet::self()->updater->setAutomaticUpdates(!checkbox_noAutomaticUpdates->isChecked());
#endif

    mudlet::self()->setToolBarIconSize(MainIconSize->value());
    mudlet::self()->setEditorTreeWidgetIconSize(TEFolderIconSize->value());
    switch (comboBox_menuBarVisibility->currentIndex()) {
    case 0:
        mudlet::self()->setMenuBarVisibility(mudlet::visibleNever);
        break;
    case 1:
        mudlet::self()->setMenuBarVisibility(mudlet::visibleOnlyWithoutLoadedProfile);
        break;
    default:
        mudlet::self()->setMenuBarVisibility(mudlet::visibleAlways);
    }
    switch (comboBox_toolBarVisibility->currentIndex()) {
    case 0:
        mudlet::self()->setToolBarVisibility(mudlet::visibleNever);
        break;
    case 1:
        mudlet::self()->setToolBarVisibility(mudlet::visibleOnlyWithoutLoadedProfile);
        break;
    default:
        mudlet::self()->setToolBarVisibility(mudlet::visibleAlways);
    }

    QFile file_use_smallscreen(mudlet::getMudletPath(mudlet::mainDataItemPath, QStringLiteral("mudlet_option_use_smallscreen")));
    if (checkBox_USE_SMALL_SCREEN->isChecked()) {
        file_use_smallscreen.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream out(&file_use_smallscreen);
        Q_UNUSED(out);
        file_use_smallscreen.close();
    } else {
        file_use_smallscreen.remove();
    }

    // These are only set on saving because they are application wide and
    // will affect all editors even the ones of other profiles so, if two
    // profile both had their preferences open they would fight each other if
    // they changed things at the same time:
    mudlet::self()->setEditorTextoptions(checkBox_showSpacesAndTabs->isChecked(), checkBox_showLineFeedsAndParagraphs->isChecked());
    mudlet::self()->setShowMapAuditErrors(checkBox_reportMapIssuesOnScreen->isChecked());

    close();
}

void dlgProfilePreferences::slot_chooseProfilesChanged(QAction* _action)
{
    Q_UNUSED(_action);

    QMenu* _menu = pushButton_chooseProfiles->menu();
    QListIterator<QAction*> itAction(_menu->actions());
    unsigned int selectionCount = 0;
    while (itAction.hasNext()) {
        QAction* _currentAction = itAction.next();
        if (_currentAction->isChecked()) {
            ++selectionCount;
        }
    }
    if (selectionCount) {
        pushButton_copyMap->setEnabled(true);
        pushButton_chooseProfiles->setText(tr("%1 selected - press to change").arg(selectionCount));
    } else {
        pushButton_copyMap->setEnabled(false);
        pushButton_chooseProfiles->setText(tr("Press to pick destination(s)"));
    }
}

void dlgProfilePreferences::slot_setEncoding(const QString& newEncoding)
{
    Host* pHost = mpHost;
    if (pHost) {
        pHost->mTelnet.setEncoding(pHost->mTelnet.getComputerEncoding(newEncoding));
    }
}

// loads available Lua scripts from triggers, aliases, scripts, etc into the
// editor tab combobox
void dlgProfilePreferences::populateScriptsList()
{
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }

    // a items of item name ("My first alias"), item type ("alias"), and item ID
    std::vector<std::tuple<QString, QString, int>> items;

    std::list<TTrigger*> triggers = pHost->getTriggerUnit()->getTriggerRootNodeList();
    for (auto trigger : triggers) {
        if (!trigger->getScript().isEmpty() && !trigger->isTemporary()) {
            items.push_back(std::make_tuple(trigger->getName(), QStringLiteral("trigger"), trigger->getID()));
        }
        addTriggersToPreview(trigger, items);
    }

    std::list<TAlias*> aliases = pHost->getAliasUnit()->getAliasRootNodeList();
    for (auto alias : aliases) {
        if (!alias->getScript().isEmpty() && !alias->isTemporary()) {
            items.push_back(std::make_tuple(alias->getName(), QStringLiteral("alias"), alias->getID()));
        }
        addAliasesToPreview(alias, items);
    }

    std::list<TScript*> scripts = pHost->getScriptUnit()->getScriptRootNodeList();
    for (auto script : scripts) {
        if (!script->getScript().isEmpty()) {
            items.push_back(std::make_tuple(script->getName(), QStringLiteral("script"), script->getID()));
        }
        addScriptsToPreview(script, items);
    }

    std::list<TTimer*> timers = pHost->getTimerUnit()->getTimerRootNodeList();
    for (auto timer : timers) {
        if (!timer->getScript().isEmpty() && !timer->isTemporary()) {
            items.push_back(std::make_tuple(timer->getName(), QStringLiteral("timer"), timer->getID()));
        }
        addTimersToPreview(timer, items);
    }

    std::list<TKey*> keys = pHost->getKeyUnit()->getKeyRootNodeList();
    for (auto key : keys) {
        if (!key->getScript().isEmpty() && !key->isTemporary()) {
            items.push_back(std::make_tuple(key->getName(), QStringLiteral("key"), key->getID()));
        }
        addKeysToPreview(key, items);
    }

    std::list<TAction*> actions = pHost->getActionUnit()->getActionRootNodeList();
    for (auto action : actions) {
        if (!action->getScript().isEmpty()) {
            items.push_back(std::make_tuple(action->getName(), QStringLiteral("button"), action->getID()));
        }
        addActionsToPreview(action, items);
    }

    auto combobox = script_preview_combobox;
    combobox->setUpdatesEnabled(false);
    combobox->clear();

    for (auto item : items) {
        combobox->addItem(QStringLiteral("%1 (%2)").arg(std::get<0>(item), std::get<1>(item)),
                          // store the item type and ID in data so we can pull up the script for it later
                          QVariant::fromValue(QPair<QString, int>(std::get<1>(item), std::get<2>(item))));
    }
    combobox->setUpdatesEnabled(true);
}

// adds trigger name ID to the list of them for the theme preview combobox, recursing down all of them
void dlgProfilePreferences::addTriggersToPreview(TTrigger* pTriggerParent, std::vector<std::tuple<QString, QString, int>>& items)
{
    list<TTrigger*>* childTriggers = pTriggerParent->getChildrenList();
    for (auto trigger : *childTriggers) {
        if (!trigger->getScript().isEmpty()) {
            items.push_back(std::make_tuple(trigger->getName(), QStringLiteral("trigger"), trigger->getID()));
        }

        if (trigger->hasChildren()) {
            addTriggersToPreview(trigger, items);
        }
    }
}

// adds alias name ID to the list of them for the theme preview combobox, recursing down all of them
void dlgProfilePreferences::addAliasesToPreview(TAlias* pAliasParent, std::vector<std::tuple<QString, QString, int>>& items)
{
    list<TAlias*>* childrenList = pAliasParent->getChildrenList();
    for (auto alias : *childrenList) {
        if (!alias->getScript().isEmpty()) {
            items.push_back(std::make_tuple(alias->getName(), QStringLiteral("alias"), alias->getID()));
        }

        if (alias->hasChildren()) {
            addAliasesToPreview(alias, items);
        }
    }
}

// adds timer name ID to the list of them for the theme preview combobox, recursing down all of them
void dlgProfilePreferences::addTimersToPreview(TTimer* pTimerParent, std::vector<std::tuple<QString, QString, int>>& items)
{
    list<TTimer*>* childrenList = pTimerParent->getChildrenList();
    for (auto timer : *childrenList) {
        if (!timer->getScript().isEmpty()) {
            items.push_back(std::make_tuple(timer->getName(), QStringLiteral("timer"), timer->getID()));
        }

        if (timer->hasChildren()) {
            addTimersToPreview(timer, items);
        }
    }
}

// adds key name ID to the list of them for the theme preview combobox, recursing down all of them
void dlgProfilePreferences::addKeysToPreview(TKey* pKeyParent, std::vector<std::tuple<QString, QString, int>>& items)
{
    list<TKey*>* childrenList = pKeyParent->getChildrenList();
    for (auto key : *childrenList) {
        if (!key->getScript().isEmpty()) {
            items.push_back(std::make_tuple(key->getName(), QStringLiteral("key"), key->getID()));
        }

        if (key->hasChildren()) {
            addKeysToPreview(key, items);
        }
    }
}

// adds script name ID to the list of them for the theme preview combobox, recursing down all of them
void dlgProfilePreferences::addScriptsToPreview(TScript* pScriptParent, std::vector<std::tuple<QString, QString, int>>& items)
{
    list<TScript*>* childrenList = pScriptParent->getChildrenList();
    for (auto script : *childrenList) {
        if (!script->getScript().isEmpty()) {
            items.push_back(std::make_tuple(script->getName(), QStringLiteral("script"), script->getID()));
        }

        if (script->hasChildren()) {
            addScriptsToPreview(script, items);
        }
    }
}

// adds action name ID to the list of them for the theme preview combobox, recursing down all of them
void dlgProfilePreferences::addActionsToPreview(TAction* pActionParent, std::vector<std::tuple<QString, QString, int>>& items)
{
    list<TAction*>* childrenList = pActionParent->getChildrenList();
    for (auto action : *childrenList) {
        if (!action->getScript().isEmpty()) {
            items.push_back(std::make_tuple(action->getName(), QStringLiteral("button"), action->getID()));
        }

        if (action->hasChildren()) {
            addActionsToPreview(action, items);
        }
    }
}

// updates latest edbee themes when the user opens up the editor tab
void dlgProfilePreferences::slot_editor_tab_selected(int tabIndex)
{
    // bail out if this is not an editor tab
    if (tabIndex != 3) {
        return;
    }

    QDir dir;
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (!dir.mkpath(cacheDir)) {
        qWarning() << "Couldn't create cache directory for edbee themes: " << cacheDir;
        return;
    }

    QSettings settings("mudlet", "Mudlet");
    QString themesURL = settings.value("colorSublimeThemesURL", QStringLiteral("https://github.com/Colorsublime/Colorsublime-Themes/archive/master.zip")).toString();
    // a default update period is 24h
    // it would be nice to use C++14's numeric separator but Qt Creator still
    // does not like them for its Clang code model analyser (and the built in
    // one is even less receptive to): 86'400'000
    int themesUpdatePeriod = settings.value("themesUpdatePeriod", 86400000).toInt();
    // save the defaults in settings so the field is visible for editing in config file if needed
    settings.setValue("colorSublimeThemesURL", themesURL);
    settings.setValue("themesUpdatePeriod", themesUpdatePeriod);

    auto themesAge = QFileInfo(mudlet::getMudletPath(mudlet::editorWidgetThemeJsonFile)).lastModified().toUTC();

    // if the cache file exists and is younger than the specified age (24h by default), don't refresh it
    if (themesAge.isValid() && themesAge.msecsTo(QDateTime::currentDateTimeUtc()) / (themesUpdatePeriod) < 1) {
        populateThemesList();
        return;
    }

    theme_download_label->show();

    auto manager = new QNetworkAccessManager(this);
    auto diskCache = new QNetworkDiskCache(this);
    diskCache->setCacheDirectory(cacheDir);
    manager->setCache(diskCache);


    QUrl url(themesURL);
    QNetworkRequest request(url);
    request.setRawHeader(QByteArray("User-Agent"), QByteArray(QStringLiteral("Mozilla/5.0 (Mudlet/%1%2)").arg(APP_VERSION, APP_BUILD).toUtf8().constData()));
    // github uses redirects
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    // load from cache if possible
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);

    QNetworkReply* getReply = manager->get(request);

    connect(getReply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), [=](QNetworkReply::NetworkError) {
        theme_download_label->setText(tr("Could not update themes: %1").arg(getReply->errorString()));
        QTimer::singleShot(5000, theme_download_label, [this] { slot_resetThemeUpdateLabel(); });
        getReply->deleteLater();
    });

    connect(getReply,
            &QNetworkReply::finished,
            this,
            std::bind(
                    [=](QNetworkReply* reply) {
                        // don't do anything if there was an error
                        if (reply->error() != QNetworkReply::NoError) {
                            return;
                        }

                        QByteArray downloadedArchive = reply->readAll();

                        tempThemesArchive = new QTemporaryFile();
                        if (!tempThemesArchive->open()) {
                            return;
                        }
                        tempThemesArchive->write(downloadedArchive);
                        tempThemesArchive->close();

                        QTemporaryDir temporaryDir;
                        if (!temporaryDir.isValid()) {
                            return;
                        }

                        // perform unzipping in a worker thread so as not to freeze the UI
                        auto future = QtConcurrent::run(mudlet::unzip, tempThemesArchive->fileName(), mudlet::getMudletPath(mudlet::mainDataItemPath, QStringLiteral("edbee/")), temporaryDir.path());
                        auto watcher = new QFutureWatcher<bool>;
                        QObject::connect(watcher, &QFutureWatcher<bool>::finished, [=]() {
                            if (future.result() == true) {
                                populateThemesList();
                            }

                            theme_download_label->hide();
                            tempThemesArchive->deleteLater();
                        });
                        watcher->setFuture(future);
                        reply->deleteLater();
                    },
                    getReply));
}

// reloads the latest edbee themes from disk and fills up the
// selection combobox with them
void dlgProfilePreferences::populateThemesList()
{
    QFile themesFile(mudlet::getMudletPath(mudlet::editorWidgetThemeJsonFile));
    QList<std::pair<QString, QString>> sortedThemes;
    QJsonArray unsortedThemes;

    if (themesFile.open(QIODevice::ReadOnly)) {
        unsortedThemes = QJsonDocument::fromJson(themesFile.readAll()).array();
        for (auto theme : unsortedThemes) {
            QString themeText = theme.toObject()["Title"].toString();
            QString themeFileName = theme.toObject()["FileName"].toString();

            if (!themeText.isEmpty() && !themeFileName.isEmpty()) {
                sortedThemes << make_pair(themeText, themeFileName);
            }
        }
    }
    sortedThemes << make_pair(QStringLiteral("Mudlet"), QStringLiteral("Mudlet.tmTheme"));

    std::sort(sortedThemes.begin(), sortedThemes.end(), [](const std::pair<QString, QString>& a, const std::pair<QString, QString>& b) { return QString::localeAwareCompare(a.first, b.first) < 0; });

    // temporary disable painting and event updates while we refill the list
    code_editor_theme_selection_combobox->setUpdatesEnabled(false);
    code_editor_theme_selection_combobox->blockSignals(true);

    auto currentSelection = code_editor_theme_selection_combobox->currentText();
    code_editor_theme_selection_combobox->clear();
    for (auto key : sortedThemes) {
        // store the actual theme file as data because edbee needs that,
        // not the name, for choosing the theme even after the theme file was loaded
        code_editor_theme_selection_combobox->addItem(key.first, key.second);
    }

    code_editor_theme_selection_combobox->setCurrentIndex(code_editor_theme_selection_combobox->findText(currentSelection));
    code_editor_theme_selection_combobox->setUpdatesEnabled(true);
    code_editor_theme_selection_combobox->blockSignals(false);
}

// user has picked a different theme to preview, so apply it
void dlgProfilePreferences::slot_theme_selected(int index)
{
    auto themeFileName = code_editor_theme_selection_combobox->itemData(index).toString();
    auto themeName = code_editor_theme_selection_combobox->itemText(index);

    if (!mudlet::loadEdbeeTheme(themeName, themeFileName)) {
        return;
    }

    auto config = edbeePreviewWidget->config();
    config->beginChanges();
    config->setThemeName(themeName);
    config->endChanges();
}

// user has picked a different script to preview, so show it
void dlgProfilePreferences::slot_script_selected(int index)
{
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }

    auto data = script_preview_combobox->itemData(index).value<QPair<QString, int>>();
    auto itemType = data.first;
    auto itemId = data.second;

    auto preview = edbeePreviewWidget->textDocument();
    if (itemType == QStringLiteral("trigger")) {
        auto pT = pHost->getTriggerUnit()->getTrigger(itemId);
        preview->setText(pT ? pT->getScript() : tr("{missing, possibly recently deleted trigger item}"));
    } else if (itemType == QStringLiteral("alias")) {
        auto pT = pHost->getAliasUnit()->getAlias(itemId);
        preview->setText(pT ? pT->getScript() : tr("{missing, possibly recently deleted alias item}"));
    } else if (itemType == QStringLiteral("script")) {
        auto pT = pHost->getScriptUnit()->getScript(itemId);
        preview->setText(pT ? pT->getScript() : tr("{missing, possibly recently deleted script item}"));
    } else if (itemType == QStringLiteral("timer")) {
        auto pT = pHost->getTimerUnit()->getTimer(itemId);
        preview->setText(pT ? pT->getScript() : tr("{missing, possibly recently deleted timer item}"));
    } else if (itemType == QStringLiteral("key")) {
        auto pT = pHost->getKeyUnit()->getKey(itemId);
        preview->setText(pT ? pT->getScript() : tr("{missing, possibly recently deleted key item}"));
    } else if (itemType == QStringLiteral("button")) {
        auto pT = pHost->getActionUnit()->getAction(itemId);
        preview->setText(pT ? pT->getScript() : tr("{missing, possibly recently deleted button item}"));
    }
}

/*!
 * \brief dlgProfilePreferences::slot_changeShowSpacesAndTabs
 * \param state \c true to show whitespace (dots for spaces, right arrows for tabs)
 * \c false to hide them and show just normal space
 *
 * A private slot function that adjusts the display of spaces and tab in the
 * editor preview in the "Editor" tab
 */
void dlgProfilePreferences::slot_changeShowSpacesAndTabs(const bool state)
{
    auto config = edbeePreviewWidget->config();
    config->beginChanges();
    config->setShowWhitespaceMode(state ? 1 : 0);
    config->endChanges();
}

/*!
 * \brief dlgProfilePreferences::slot_changeShowLineFeedsAndParagraphs
 * \param state \c true to show (currently) a graphic line under each line of text in editor
 * \c false to hide them.
 *
 * A private slot function that (currently) adjusts the display of a horizontal
 * "underline" acros the width of each line of text in the editor preview in the
 * "Editor" tab although it was originally intended to show line-feeds and paragraph
 * markers in the previous QTextEdit (and may in the future in the edbee) widget.
 */
void dlgProfilePreferences::slot_changeShowLineFeedsAndParagraphs(const bool state)
{
    auto config = edbeePreviewWidget->config();
    config->beginChanges();
    config->setUseLineSeparator(state);
    config->endChanges();
}

void dlgProfilePreferences::slot_resetThemeUpdateLabel()
{
    theme_download_label->hide();
    theme_download_label->setText(tr("Updating themes from colorsublime.com..."));
}

/*
 * This is to deal particularly with the case where the preferences dialog is
 * opened without a host instance (other than the dummy "default_host") being
 * around - and then the user starts up a profile and one gets created.
 * In that situation we detect the signal that the mudlet class sends out (now)
 * when a host is created and wire it up into the controls that until then
 * have been disabled/greyed-out.
 */
void dlgProfilePreferences::slot_handleHostAddition(Host* pHost, const quint8 count)
{
    // count will be 2 in the case we particularly want to handle (adding the
    // first real Host instance):
    if (!mpHost && pHost && count < 3) {
        // We have not been constructed with a valid Host pointer,
        // AND a real Host instance has just been created
        // AND there are only two Host instances (the "real" one and the
        // "default_host") around.
        mpHost = pHost;
        // So make connections to the details of the real Host instance:
        initWithHost(pHost);
    }
}

/*
 * This is to deal with the case where the preferences is opened on a profile
 * and then the user closes the profile before closing the dialog/form of this
 * class and (currently) they are multiplaying so that Mudlet itself is not
 * shutting down.  It disables/greys-out/hides the controls that are
 * particularly associated with the single host instance (without saving
 * application wide settings adjustments).
 * This was not originally planned to be done but with the addition of the
 * functionality to handle the situation of having a mainly disabled preference
 * dialog opened when no profiles were, it makes for a slightly more friendly
 * UX to also do this and adds a certain "balance" in the "code functionality".
 */
void dlgProfilePreferences::slot_handleHostDeletion(Host* pHost)
{
    if (mpHost && pHost && mpHost == pHost) {
        // We have been constructed with a valid Host pointer,
        // AND a real Host instance is being destroyed
        // AND we are working on the Host instance concerned.
        // Forget about the host:
        mpHost = nullptr;
        // Remove connections to the details of the real Host instance (we
        // have to throw them away as it is too late to save them - the profile
        // has already been saved - or not):
        disconnectHostRelatedControls();
        clearHostDetails();
        // and we can then use the following to disable the Host specific controls:
        disableHostDetails();
    }
}

void dlgProfilePreferences::generateMapGlyphDisplay()
{
    QHash<QString, QSet<int>> roomSymbolsHash(mpHost->mpMap->roomSymbolsHash());
    QPointer<QTableWidget> pTableWidget = mpDialogMapGlyphUsage->findChild<QTableWidget*>(QLatin1String("tableWidget"));
    if (!pTableWidget) {
        return;
    }

    // Must turn off sorting at least whilst inserting items...
    pTableWidget->setSortingEnabled(false);
    pTableWidget->setColumnCount(6);
    // This clears any previous contents:
    pTableWidget->setRowCount(0);
    pTableWidget->setRowCount(roomSymbolsHash.count());


    QFont selectedFont = mpHost->mpMap->mMapSymbolFont;
    selectedFont.setPointSize(16);
    selectedFont.setStyleStrategy(static_cast<QFont::StyleStrategy>(mpHost->mpMap->mMapSymbolFont.styleStrategy() | QFont::NoFontMerging));
    QFont anyFont = mpHost->mpMap->mMapSymbolFont;
    anyFont.setPointSize(16);
    anyFont.setStyleStrategy(static_cast<QFont::StyleStrategy>(mpHost->mpMap->mMapSymbolFont.styleStrategy() &~(QFont::NoFontMerging)));

    int row = -1;
    QHashIterator<QString, QSet<int>> itUsedSymbol(roomSymbolsHash);
    while (itUsedSymbol.hasNext()) {
        itUsedSymbol.next();
        QString symbol = itUsedSymbol.key();
        QList<int> roomsWithSymbol = itUsedSymbol.value().toList();
        if (roomsWithSymbol.count() > 1) {
            std::sort(roomsWithSymbol.begin(), roomsWithSymbol.end());
        }
        QTableWidgetItem* pSymbolInFont = new QTableWidgetItem();
        pSymbolInFont->setTextAlignment(Qt::AlignCenter);
        pSymbolInFont->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                  .arg(tr("<p>The room symbol will appear like this if only symbols (glyphs) from the specfic font are used.</p>")));
        pSymbolInFont->setFont(selectedFont);

        QTableWidgetItem* pSymbolAnyFont = new QTableWidgetItem();
        pSymbolAnyFont->setTextAlignment(Qt::AlignCenter);
        pSymbolAnyFont->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                   .arg(tr("<p>The room symbol will appear like this if symbols (glyphs) from any font can be used.</p>")));
        pSymbolAnyFont->setFont(anyFont);

        QFontMetrics SymbolInFontMetrics(selectedFont);
        QFontMetrics SymbolAnyFontMetrics(anyFont);

        // pCodePoints is the sequence of UTF-32 codepoints in the symbol and
        // this ought to be what is needed to check that a font or set of fonts
        // can render the codepoints:
        QVector<quint32> pCodePoints = symbol.toUcs4();
        // These can be used to flag symbols that cannot be reproduced
        bool isSingleFontUsable=true;
        bool isAllFontUsable=true;
        QStringList codePointsString;
        for (uint i = 0, total = pCodePoints.size(); i < total; ++i) {
            codePointsString << QStringLiteral("U+%1").arg(pCodePoints.at(i), 4, 16, QChar('0')).toUpper();
            if (!SymbolAnyFontMetrics.inFontUcs4(pCodePoints.at(i))) {
                isAllFontUsable=false;
                // By definition if all the fonts together cannot render the
                // glyph then the specified one cannot either
                isSingleFontUsable=false;
            } else if (!SymbolInFontMetrics.inFontUcs4(pCodePoints.at(i))) {
                isSingleFontUsable=false;
            }
        }

        QTableWidgetItem* pCodePointDisplay = new QTableWidgetItem(codePointsString.join(QStringLiteral(", ")));
        pCodePointDisplay->setTextAlignment(Qt::AlignCenter);
        pCodePointDisplay->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                      .arg(tr("<p>These are the sequence of hexadecimal numbers that are used by the Unicode consortium "
                                              "to identify the graphemes needed to create the symbol.  These numbers can be utilised "
                                              "to determine precisely what is to be drawn even if some fonts have glyphs that are the "
                                              "same for different codepoints or combination of codepoints.</p>"
                                              "<p>Character entry utilities such as <i>charmap.exe</i> on <i>Windows</i> or <i>gucharmap</i> "
                                              "on many Unix type operating systems will also use these numbers which cover "
                                              "everything from U+0020 {Space} to U+10FFFD the last usable number in the <i>Private Use "
                                              "Plane 16</i> via most of the written marks that humanity has ever made.</p>")));

        // Need to pad the numbers with spaces so that sorting works correctly:
        QTableWidgetItem* pUsageCount = new QTableWidgetItem(QStringLiteral("%1").arg(roomsWithSymbol.count(), 5, 10, QChar(' ')));
        pUsageCount->setTextAlignment(Qt::AlignCenter);
        pUsageCount->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                .arg(tr("<p>How many rooms in the whole map have this symbol.")));

        QStringList roomNumberStringList;
        QListIterator<int> itRoom(roomsWithSymbol);
        // Only show the first, say 32, rooms otherwise the whole dialog could
        // be filled completely for a symbol that is used extensively e.g. on
        // a wilderness type map:
        int roomCount = 0;
        while (itRoom.hasNext()) {
            roomNumberStringList << QString::number(itRoom.next());
            if (++roomCount == 32 && itRoom.hasNext()) {
                // There is still rooms not listed
                roomNumberStringList << tr("more - not shown...");
                // Escape from loop to truncate the listing:
                break;
            }
        }
        QTableWidgetItem* pRoomNumbers = new QTableWidgetItem(roomNumberStringList.join(QStringLiteral(", ")));
        pRoomNumbers->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                 .arg(tr("<p>The rooms with this symbol, up to a maximum of thirty-two, if there are more "
                                         "than this, it is indicated but they are not shown.</p>")));

        QToolButton * pDummyButton = new QToolButton();
        if (isSingleFontUsable) {
            pSymbolInFont->setText(symbol);
            pSymbolAnyFont->setText(symbol);
            pDummyButton->setIcon(QIcon(QStringLiteral(":/icons/dialog-ok-apply.png")));
            pDummyButton->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                     .arg(tr("<p>The symbol can be made entirely from glyphs in the specified font.</p>")));
        } else {
            // Need to switch to a different font as it is possible that the
            // single font may not have the replacement glyph either...!
            pSymbolInFont->setFont(anyFont);
            pSymbolInFont->setText(QString(QChar::ReplacementCharacter));
            if (isAllFontUsable) {
                pSymbolAnyFont->setText(symbol);
                pDummyButton->setIcon(QIcon(QStringLiteral(":/icons/dialog-warning.png")));
                pDummyButton->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                         .arg(tr("<p>The symbol cannot be made entirely from glyphs in the specified font, but, "
                                                 "using other fonts in the system, it can. Either un-check the <i>Only use symbols "
                                                 "(glyphs) from chosen font</i> option or try and choose another font that does "
                                                 "have the needed glyphs.</p><p><i>You need not close this table to try another font, "
                                                 "changing it on the main preferences dialogue will update this table after a slight "
                                                 "delay.</i></p>")));
            } else {
                pSymbolAnyFont->setText(QString(QChar::ReplacementCharacter));
                pDummyButton->setIcon(QIcon(QStringLiteral(":/icons/dialog-error.png")));
                pDummyButton->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                         .arg(tr("<p>The symbol cannot be drawn using any of the fonts in the system, either an "
                                                 "invalid string was entered as the symbol for the indicated rooms or the map was "
                                                 "created on a different systems with a different set of fonts available to use. "
                                                 "You may be able to correct this by installing an additional font using whatever "
                                                 "method is appropriate for this system or by editing the map to use a different "
                                                 "symbol. It may be possible to do the latter via a lua script using the "
                                                 "<i>getRoomChar</i> and <i>setRoomChar</i> functions.</p>")));
            }
        }
        pTableWidget->setCellWidget(++row, 0, pDummyButton);

        pTableWidget->setItem(row, 1, pSymbolInFont);
        pTableWidget->setItem(row, 2, pSymbolAnyFont);
        pTableWidget->setItem(row, 3, pCodePointDisplay);
        pTableWidget->setItem(row, 4, pUsageCount);
        pTableWidget->setItem(row, 5, pRoomNumbers);
    }
    pTableWidget->sortItems(4, Qt::DescendingOrder);
    pTableWidget->setSortingEnabled(true);
    pTableWidget->resizeColumnsToContents();
    // The room number column can contain a lot of rooms...
    pTableWidget->resizeRowsToContents();
    mpDialogMapGlyphUsage->show();
    mpDialogMapGlyphUsage->raise();
}

void dlgProfilePreferences::slot_showMapGlyphUsage()
{
    if (!mpHost || !mpHost->mpMap) {
        return;
    }

    if (mpDialogMapGlyphUsage) {
        // Already in use so just refresh the contents instead:
        generateMapGlyphDisplay();
        return;
    }

    QUiLoader loader;
    QFile file(QStringLiteral(":/ui/glyph_usage.ui"));
    file.open(QFile::ReadOnly);
    mpDialogMapGlyphUsage = qobject_cast<QDialog*>(loader.load(&file, this));
    file.close();
    if (!mpDialogMapGlyphUsage) {
        qWarning() << "dlgProfilePreferences::slot_showMapGlyphUsage() ERROR: failed to create the dialog!";
        return;
    }

    mpDialogMapGlyphUsage->setWindowIcon(QIcon(QStringLiteral(":/icons/place_of_interest.png")));
    mpDialogMapGlyphUsage->setWindowTitle(tr("Map symbol usage - %1").arg(mpHost->getName()));
    mpDialogMapGlyphUsage->setAttribute(Qt::WA_DeleteOnClose);
    generateMapGlyphDisplay();
}

void dlgProfilePreferences::slot_setMapSymbolFontStrategy(const bool isToOnlyUseSelectedFont)
{
    Host* pHost = mpHost;
    if (!pHost ||!pHost->mpMap) {
        return;
    }

    if (pHost->mpMap->mIsOnlyMapSymbolFontToBeUsed != isToOnlyUseSelectedFont) {
        pHost->mpMap->mIsOnlyMapSymbolFontToBeUsed = isToOnlyUseSelectedFont;
        if (isToOnlyUseSelectedFont) {
            pHost->mpMap->mMapSymbolFont.setStyleStrategy(static_cast<QFont::StyleStrategy>(pHost->mpMap->mMapSymbolFont.styleStrategy() | QFont::NoFontMerging));
        } else {
            pHost->mpMap->mMapSymbolFont.setStyleStrategy(static_cast<QFont::StyleStrategy>(pHost->mpMap->mMapSymbolFont.styleStrategy() &~(QFont::NoFontMerging)));
        }
        // Clear the existing cache of room symbol pixmaps:
        pHost->mpMap->mpMapper->mp2dMap->flushSymbolPixmapCache();
        pHost->mpMap->mpMapper->mp2dMap->repaint();
        pHost->mpMap->mpMapper->update();

        if (mpDialogMapGlyphUsage) {
            generateMapGlyphDisplay();
        }
    }
}

void dlgProfilePreferences::slot_setMapSymbolFont(const QFont & font)
{
    Host* pHost = mpHost;
    if (!pHost ||!pHost->mpMap) {
        return;
    }

    int pointSize = pHost->mpMap->mMapSymbolFont.pointSize();
    if (pHost->mpMap->mMapSymbolFont != font) {
        pHost->mpMap->mMapSymbolFont = font;
        pHost->mpMap->mMapSymbolFont.setPointSize(pointSize);
        // Clear the existing cache of room symbol pixmaps:
        pHost->mpMap->mpMapper->mp2dMap->flushSymbolPixmapCache();
        pHost->mpMap->mpMapper->mp2dMap->repaint();
        pHost->mpMap->mpMapper->update();

        if (mpDialogMapGlyphUsage) {
            generateMapGlyphDisplay();
        }
    }
}

// These next two prevent BOTH controls being set to never to prevent the lose
// of access to the setting/controls completely - once there is a profile loaded
// access to the settings/controls can be overriden by a context menu action on
// any TConsole instance:
void dlgProfilePreferences::slot_changeShowMenuBar(const int newIndex)
{
    if (!newIndex && !comboBox_toolBarVisibility->currentIndex()) {
        // This control has been set to the "Never" setting but so is the other
        // control - so force it back to the "Only if no profile one
        comboBox_menuBarVisibility->setCurrentIndex(1);
    }
}

void dlgProfilePreferences::slot_changeShowToolBar(const int newIndex)
{
    if (!newIndex && !comboBox_menuBarVisibility->currentIndex()) {
        // This control has been set to the "Never" setting but so is the other
        // control - so force it back to the "Only if no profile one
        comboBox_toolBarVisibility->setCurrentIndex(1);
    }
}
