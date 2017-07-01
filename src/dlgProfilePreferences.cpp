/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014, 2016-2017 by Stephen Lyons                        *
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
#include <QRegExp>
#include <QStandardPaths>
#include <QTextOption>
#include <QToolBar>
#include <QVariant>
#include "post_guard.h"


dlgProfilePreferences::dlgProfilePreferences(QWidget* pF, Host* pH) : QDialog(pF), mFontSize(10), mpHost(pH)
{
    // init generated dialog
    setupUi(this);

    // This is currently empty so can be hidden until needed, but provides a
    // location on the last (Special Options) tab where temporary/development
    // /testing controls can be placed if needed...
    groupBox_Debug->hide();

    loadEditorTab();

    mFORCE_MXP_NEGOTIATION_OFF->setChecked(mpHost->mFORCE_MXP_NEGOTIATION_OFF);
    mMapperUseAntiAlias->setChecked(mpHost->mMapperUseAntiAlias);
    acceptServerGUI->setChecked(mpHost->mAcceptServerGUI);

    ircHostName->setText(dlgIRC::readIrcHostName(mpHost));
    ircHostPort->setText(QString::number(dlgIRC::readIrcHostPort(mpHost)));
    ircChannels->setText(dlgIRC::readIrcChannels(mpHost).join(" "));
    ircNick->setText(dlgIRC::readIrcNickName(mpHost));

    dictList->setSelectionMode(QAbstractItemView::SingleSelection);
    enableSpellCheck->setChecked(pH->mEnableSpellCheck);
    checkBox_echoLuaErrors->setChecked(pH->mEchoLuaErrors);
    checkBox_showSpacesAndTabs->setChecked(mudlet::self()->mEditorTextOptions & QTextOption::ShowTabsAndSpaces);
    checkBox_showLineFeedsAndParagraphs->setChecked(mudlet::self()->mEditorTextOptions & QTextOption::ShowLineAndParagraphSeparators);
    // As we reflect the state of the above two checkboxes in the preview widget
    // on another tab we have to track their changes in state and update that
    // edbee widget straight away - however we do not need to update any open
    // widgets of the same sort in use in ANY profile's editor until we hit
    // the save button...
    connect(checkBox_showSpacesAndTabs, SIGNAL(clicked(bool)), this, SLOT(slot_changeShowSpacesAndTabs(const bool)));
    connect(checkBox_showLineFeedsAndParagraphs, SIGNAL(clicked(bool)), this, SLOT(slot_changeShowLineFeedsAndParagraphs(const bool)));

    QString path;
#ifdef Q_OS_LINUX
    if (QFile::exists("/usr/share/hunspell/" + mpHost->mSpellDic + ".aff")) {
        path = "/usr/share/hunspell/";
    } else {
        path = "./";
    }
#elif defined(Q_OS_MAC)
    path = QCoreApplication::applicationDirPath() + "/../Resources/";
#else
    path = "./";
#endif

    QDir dir(path);

    QStringList entries = dir.entryList(QDir::Files, QDir::Time);
    QRegExp rex(R"(\.dic$)");
    entries = entries.filter(rex);
    for (int i = 0; i < entries.size(); i++) {
        QString n = entries[i].replace(".dic", "");
        auto item = new QListWidgetItem(entries[i]);
        dictList->addItem(item);
        if (entries[i] == mpHost->mSpellDic) {
            item->setSelected(true);
        }
    }

    if (pH->mUrl.contains(QStringLiteral("achaea.com"), Qt::CaseInsensitive) || pH->mUrl.contains(QStringLiteral("aetolia.com"), Qt::CaseInsensitive)
        || pH->mUrl.contains(QStringLiteral("imperian.com"), Qt::CaseInsensitive)
        || pH->mUrl.contains(QStringLiteral("lusternia.com"), Qt::CaseInsensitive)) {
        downloadMapOptions->setVisible(true);
        connect(buttonDownloadMap, SIGNAL(clicked()), this, SLOT(downloadMap()));
    } else {
        downloadMapOptions->setVisible(false);
    }

    connect(closeButton, &QAbstractButton::pressed, this, &dlgProfilePreferences::slot_save_and_exit);

    pushButton_command_line_foreground_color->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mCommandLineFgColor.name()));
    pushButton_command_line_background_color->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mCommandLineBgColor.name()));

    pushButton_black->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mBlack.name()));
    pushButton_Lblack->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mLightBlack.name()));
    pushButton_green->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mGreen.name()));
    pushButton_Lgreen->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mLightGreen.name()));
    pushButton_red->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mRed.name()));
    pushButton_Lred->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mLightRed.name()));
    pushButton_blue->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mBlue.name()));
    pushButton_Lblue->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mLightBlue.name()));
    pushButton_yellow->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mYellow.name()));
    pushButton_Lyellow->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mLightYellow.name()));
    pushButton_cyan->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mCyan.name()));
    pushButton_Lcyan->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mLightCyan.name()));
    pushButton_magenta->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mMagenta.name()));
    pushButton_Lmagenta->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mLightMagenta.name()));
    pushButton_white->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mWhite.name()));
    pushButton_Lwhite->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mLightWhite.name()));

    pushButton_foreground_color->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mFgColor.name()));
    pushButton_background_color->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mBgColor.name()));
    pushButton_command_foreground_color->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mCommandFgColor.name()));
    pushButton_command_background_color->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mCommandBgColor.name()));

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
    QStringList sizeList;
    for (int i = 1; i < 40; i++) {
        sizeList << QString::number(i);
    }
    fontSize->insertItems(1, sizeList);
    connect(fontSize, SIGNAL(currentIndexChanged(int)), this, SLOT(setFontSize()));

    setColors2();

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

    // the GMCP warning is hidden by default and is only enabled when the value is toggled
    need_reconnect_for_data_protocol->hide();
    connect(mEnableGMCP, SIGNAL(clicked()), need_reconnect_for_data_protocol, SLOT(show()));
    connect(mEnableMSDP, SIGNAL(clicked()), need_reconnect_for_data_protocol, SLOT(show()));

    // same with special connection warnings
    need_reconnect_for_specialoption->hide();
    connect(mFORCE_MCCP_OFF, SIGNAL(clicked()), need_reconnect_for_specialoption, SLOT(show()));
    connect(mFORCE_GA_OFF, SIGNAL(clicked()), need_reconnect_for_specialoption, SLOT(show()));

    comboBox_statusBarSetting->addItem(tr("Off"), QVariant(mudlet::self()->statusBarHidden));
    comboBox_statusBarSetting->addItem(tr("Auto"), QVariant(mudlet::self()->statusBarAutoShown));
    comboBox_statusBarSetting->addItem(tr("On"), QVariant(mudlet::self()->statusBarAlwaysShown));
    comboBox_statusBarSetting->setMaxCount(3);
    comboBox_statusBarSetting->setInsertPolicy(QComboBox::NoInsert);
    comboBox_statusBarSetting->setMaxVisibleItems(3);
    int _indexForStatusBarSetting = comboBox_statusBarSetting->findData(QVariant(mudlet::self()->mStatusBarState), Qt::UserRole);
    if (_indexForStatusBarSetting >= 0) {
        comboBox_statusBarSetting->setCurrentIndex(_indexForStatusBarSetting);
    }

    checkBox_reportMapIssuesOnScreen->setChecked(mudlet::self()->showMapAuditErrors());
    Host* pHost = mpHost;
    if (pHost) {
        mFontSize = pHost->mDisplayFont.pointSize();
        fontComboBox->setCurrentFont(pHost->mDisplayFont);
        if (mFontSize < 0) {
            mFontSize = 10;
        }
        if (mFontSize <= 40) {
            fontSize->setCurrentIndex(mFontSize);
        }

        setColors();

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
        QFile file_use_smallscreen(QDir::homePath() + "/.config/mudlet/mudlet_option_use_smallscreen");
        if (file_use_smallscreen.exists()) {
            checkBox_USE_SMALL_SCREEN->setChecked(true);
        } else {
            checkBox_USE_SMALL_SCREEN->setChecked(false);
        }
        topBorderHeight->setValue(pHost->mBorderTopHeight);
        bottomBorderHeight->setValue(pHost->mBorderBottomHeight);
        leftBorderWidth->setValue(pHost->mBorderLeftWidth);
        qDebug() << "loading: left border width:" << pHost->mBorderLeftWidth;
        rightBorderWidth->setValue(pHost->mBorderRightWidth);
        MainIconSize->setValue(mudlet::self()->mMainIconSize);
        TEFolderIconSize->setValue(mudlet::self()->mTEFolderIconSize);
        showMenuBar->setChecked(mudlet::self()->mShowMenuBar);
        if (!showMenuBar->isChecked()) {
            showToolbar->setChecked(true);
        } else {
            showToolbar->setChecked(mudlet::self()->mShowToolbar);
        }
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
        QStringList profileList = QDir(QStringLiteral("%1/.config/mudlet/profiles").arg(QDir::homePath())).entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time); // sort by profile "hotness"
        pushButton_chooseProfiles->setEnabled(false);
        pushButton_copyMap->setEnabled(false);
        QMenu* pMenu = new QMenu(tr("Other profiles to Map to:"));
        for (unsigned int i = 0, total = profileList.size(); i < total; ++i) {
            QString s = profileList.at(i);
            if (s.isEmpty() || !s.compare(pHost->getName()) || !s.compare(QStringLiteral("default_host"))) {
                // Do not include THIS profile in the list - it will
                // automatically get saved - as the file to copy to the other
                // profiles!  Also exclude the dummy "default_host" one
                continue;
            }

            auto pItem = new QAction(s, 0);
            pItem->setCheckable(true);
            pItem->setChecked(false);
            pMenu->addAction(pItem);
            //Enable it as we now have at least one profile to copy to
            pushButton_chooseProfiles->setEnabled(true);
        }

        pushButton_chooseProfiles->setMenu(pMenu);
        connect(pMenu, SIGNAL(triggered(QAction*)), this, SLOT(slot_chooseProfilesChanged(QAction*)));

        connect(pushButton_copyMap, SIGNAL(clicked()), this, SLOT(copyMap()));

        // label to show on successful map file action
        label_mapFileActionResult->hide();

        connect(pushButton_loadMap, SIGNAL(clicked()), this, SLOT(loadMap()));
        connect(pushButton_saveMap, SIGNAL(clicked()), this, SLOT(saveMap()));

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
            checkBox_showDefaultArea->show();
            checkBox_showDefaultArea->setText(tr(R"(Show "%1" in the map area selection)").arg(pHost->mpMap->mpRoomDB->getDefaultAreaName()));
            checkBox_showDefaultArea->setChecked(pHost->mpMap->mpMapper->getDefaultAreaShown());
        } else {
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
        connect(comboBox_encoding, SIGNAL(currentTextChanged(const QString&)), this, SLOT(slot_setEncoding(const QString&)));
    }
}

void dlgProfilePreferences::loadEditorTab()
{
    connect(tabWidgeta, &QTabWidget::currentChanged, this, &dlgProfilePreferences::slot_editor_tab_selected);

    auto config = edbeePreviewWidget->config();
    config->beginChanges();
    config->setSmartTab(true);
    config->setCaretBlinkRate(200);
    config->setIndentSize(2);
    config->setThemeName(mpHost->mEditorTheme);
    config->setCaretWidth(1);
    config->setShowWhitespaceMode(mudlet::self()->mEditorTextOptions & QTextOption::ShowTabsAndSpaces ? 1 : 0);
    config->setUseLineSeparator(mudlet::self()->mEditorTextOptions & QTextOption::ShowLineAndParagraphSeparators);
    config->setFont(mpHost->mDisplayFont);
    config->endChanges();
    edbeePreviewWidget->textDocument()->setLanguageGrammar(edbee::Edbee::instance()->grammarManager()->detectGrammarWithFilename(QLatin1Literal("Buck.lua")));
    // disable shadows as their purpose (notify there is more text) is performed by scrollbars already
    edbeePreviewWidget->textScrollArea()->enableShadowWidget(false);

    populateThemesList();
    mudlet::loadEdbeeTheme(mpHost->mEditorTheme, mpHost->mEditorThemeFile);
    populateScriptsList();

    // pre-select the current theme
    code_editor_theme_selection_combobox->lineEdit()->setPlaceholderText(QStringLiteral("Select theme"));
    auto themeIndex = code_editor_theme_selection_combobox->findText(mpHost->mEditorTheme);
    code_editor_theme_selection_combobox->setCurrentIndex(themeIndex);
    slot_theme_selected(themeIndex);

    code_editor_theme_selection_combobox->setInsertPolicy(QComboBox::NoInsert);
    code_editor_theme_selection_combobox->setMaxVisibleItems(20);

    // pre-select the last shown script to preview
    script_preview_combobox->lineEdit()->setPlaceholderText(QStringLiteral("Select script to preview"));
    auto scriptIndex = script_preview_combobox->findData(QVariant::fromValue(QPair<QString, int>(mpHost->mThemePreviewType, mpHost->mThemePreviewItemID)));
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
    if (tabWidgeta->currentIndex() == 3) {
        slot_editor_tab_selected(3);
    }
}

void dlgProfilePreferences::setColors()
{
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }

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

    pushButton_command_line_foreground_color->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mCommandLineFgColor.name()));
    pushButton_command_line_background_color->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mCommandLineBgColor.name()));
    pushButton_command_foreground_color->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mCommandFgColor.name()));
    pushButton_command_background_color->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mCommandBgColor.name()));
}

void dlgProfilePreferences::setColors2()
{
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }

    pushButton_black_2->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mBlack_2.name()));
    pushButton_Lblack_2->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mLightBlack_2.name()));
    pushButton_green_2->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mGreen_2.name()));
    pushButton_Lgreen_2->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mLightGreen_2.name()));
    pushButton_red_2->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mRed_2.name()));
    pushButton_Lred_2->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mLightRed_2.name()));
    pushButton_blue_2->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mBlue_2.name()));
    pushButton_Lblue_2->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mLightBlue_2.name()));
    pushButton_yellow_2->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mYellow_2.name()));
    pushButton_Lyellow_2->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mLightYellow_2.name()));
    pushButton_cyan_2->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mCyan_2.name()));
    pushButton_Lcyan_2->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mLightCyan_2.name()));
    pushButton_magenta_2->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mMagenta_2.name()));
    pushButton_Lmagenta_2->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mLightMagenta_2.name()));
    pushButton_white_2->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mWhite_2.name()));
    pushButton_Lwhite_2->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mLightWhite_2.name()));

    pushButton_foreground_color_2->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mFgColor_2.name()));
    pushButton_background_color_2->setStyleSheet(QStringLiteral("QPushButton{background-color: %1;}").arg(mpHost->mBgColor_2.name()));
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
    mFontSize = fontSize->currentIndex();
    setDisplayFont();
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
            this, tr("Load Mudlet map"), QDir::homePath(), tr("Mudlet map (*.dat);;Xml map data (*.xml);;Any file (*)", "Do not change extensions (in braces) they are used programmatically!"));
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
            QString toProfileDirPathString = QStringLiteral("%1/.config/mudlet/profiles/%2/map").arg(QDir::homePath(), toProfileName);
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
    // current running profile, > 0 on sucess or 0 if not running as another profile

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
    QString sourceMapFolder(QStringLiteral("%1/.config/mudlet/profiles/%2/map").arg(QDir::homePath(), pHost->getName()));
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

        if (!thisProfileLatestMapFile.copy(QStringLiteral("%1/.config/mudlet/profiles/%2/map/%3").arg(QDir::homePath(), otherHostName, thisProfileLatestMapPathFileName))) {
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

void dlgProfilePreferences::slot_save_and_exit()
{
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }
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
    //pHost->mDisableAutoCompletion = disable_auto_completion_checkbox->isChecked();
    pHost->mUSE_IRE_DRIVER_BUGFIX = checkBox_USE_IRE_DRIVER_BUGFIX->isChecked();
    pHost->set_USE_IRE_DRIVER_BUGFIX(checkBox_USE_IRE_DRIVER_BUGFIX->isChecked());
    //pHost->set_LF_ON_GA( checkBox_LF_ON_GA->isChecked() );
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
        pHost->mpMap->mpMapper->mp2dMap->repaint(); // Forceably redraw it as we ARE currently showing default area
        pHost->mpMap->mpMapper->update();
    }
    pHost->mBorderTopHeight = topBorderHeight->value();
    pHost->mBorderBottomHeight = bottomBorderHeight->value();
    pHost->mBorderLeftWidth = leftBorderWidth->value();
    pHost->mBorderRightWidth = rightBorderWidth->value();
    //qDebug()<<"Left border width:"<<pHost->mBorderLeftWidth<<" right:"<<pHost->mBorderRightWidth;
    pHost->commandLineMinimumHeight = commandLineMinimumHeight->value();
    //pHost->mMXPMode = mMXPMode->currentIndex();
    pHost->mFORCE_MXP_NEGOTIATION_OFF = mFORCE_MXP_NEGOTIATION_OFF->isChecked();
    mudlet::self()->mMainIconSize = MainIconSize->value();
    mudlet::self()->mTEFolderIconSize = TEFolderIconSize->value();
    mudlet::self()->setIcoSize(MainIconSize->value());
    pHost->mpEditorDialog->setTBIconSize(0);
    mudlet::self()->mShowMenuBar = showMenuBar->isChecked();
    if (showMenuBar->isChecked()) {
        mudlet::self()->menuBar()->show();
    } else {
        mudlet::self()->menuBar()->hide();
    }
    mudlet::self()->mShowToolbar = showToolbar->isChecked();
    if (showToolbar->isChecked()) {
        mudlet::self()->mpMainToolBar->show();
    } else {
        mudlet::self()->mpMainToolBar->hide();
    }
    pHost->mIsNextLogFileInHtmlFormat = mIsToLogInHtml->isChecked();
    pHost->mIsLoggingTimestamps = mIsLoggingTimestamps->isChecked();
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

    mudlet::self()->mStatusBarState = mudlet::StatusBarOptions(comboBox_statusBarSetting->currentData().toInt());
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

    if (checkBox_USE_SMALL_SCREEN->isChecked()) {
        QFile file_use_smallscreen(QDir::homePath() + "/.config/mudlet/mudlet_option_use_smallscreen");
        file_use_smallscreen.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream out(&file_use_smallscreen);
        file_use_smallscreen.close();
    } else {
        QFile file_use_smallscreen(QDir::homePath() + "/.config/mudlet/mudlet_option_use_smallscreen");
        file_use_smallscreen.remove();
    }
    if (mudlet::self()->mConsoleMap.contains(pHost)) {
        mudlet::self()->mConsoleMap[pHost]->console->updateScreenView();
        mudlet::self()->mConsoleMap[pHost]->console->forceUpdate();
        mudlet::self()->mConsoleMap[pHost]->refresh();
        int x = mudlet::self()->mConsoleMap[pHost]->width();
        int y = mudlet::self()->mConsoleMap[pHost]->height();
        QSize s = QSize(x, y);
        QResizeEvent event(s, s);
        QApplication::sendEvent(mudlet::self()->mConsoleMap[pHost], &event);
        //qDebug()<<"after console refresh: Left border width:"<<pHost->mBorderLeftWidth<<" right:"<<pHost->mBorderRightWidth;
    }

    // These are only sent on saving because they are application wide and
    // will affect all editors even the ones of other profiles so, if two
    // profile both had their preferences open they would fight each other if
    // they changed things at the same time:
    mudlet::self()->setEditorTextoptions(checkBox_showSpacesAndTabs->isChecked(), checkBox_showLineFeedsAndParagraphs->isChecked());
    mudlet::self()->setShowMapAuditErrors(checkBox_reportMapIssuesOnScreen->isChecked());
    pHost->mEchoLuaErrors = checkBox_echoLuaErrors->isChecked();

    pHost->mEditorTheme = code_editor_theme_selection_combobox->currentText();
    pHost->mEditorThemeFile = code_editor_theme_selection_combobox->currentData().toString();
    if (pHost->mpEditorDialog) {
        pHost->mpEditorDialog->setThemeAndOtherSettings(pHost->mEditorTheme);
    }

    auto data = script_preview_combobox->currentData().value<QPair<QString, int>>();
    pHost->mThemePreviewItemID = data.second;
    pHost->mThemePreviewType = data.first;

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
    mpHost->mTelnet.setEncoding(mpHost->mTelnet.getComputerEncoding(newEncoding));
}

// loads available Lua scripts from triggers, aliases, scripts, etc into the
// editor tab combobox
void dlgProfilePreferences::populateScriptsList()
{
    // a items of item name ("My first alias"), item type ("alias"), and item ID
    std::vector<std::tuple<QString, QString, int>> items;

    std::list<TTrigger*> triggers = mpHost->getTriggerUnit()->getTriggerRootNodeList();
    for (auto trigger : triggers) {
        if (!trigger->getScript().isEmpty() && !trigger->isTemporary()) {
            items.push_back(std::make_tuple(trigger->getName(), QStringLiteral("trigger"), trigger->getID()));
        }
        addTriggersToPreview(trigger, items);
    }

    std::list<TAlias*> aliases = mpHost->getAliasUnit()->getAliasRootNodeList();
    for (auto alias : aliases) {
        if (!alias->getScript().isEmpty() && !alias->isTemporary()) {
            items.push_back(std::make_tuple(alias->getName(), QStringLiteral("alias"), alias->getID()));
        }
        addAliasesToPreview(alias, items);
    }

    std::list<TScript*> scripts = mpHost->getScriptUnit()->getScriptRootNodeList();
    for (auto script : scripts) {
        if (!script->getScript().isEmpty()) {
            items.push_back(std::make_tuple(script->getName(), QStringLiteral("script"), script->getID()));
        }
        addScriptsToPreview(script, items);
    }

    std::list<TTimer*> timers = mpHost->getTimerUnit()->getTimerRootNodeList();
    for (auto timer : timers) {
        if (!timer->getScript().isEmpty() && !timer->isTemporary()) {
            items.push_back(std::make_tuple(timer->getName(), QStringLiteral("timer"), timer->getID()));
        }
        addTimersToPreview(timer, items);
    }

    std::list<TKey*> keys = mpHost->getKeyUnit()->getKeyRootNodeList();
    for (auto key : keys) {
        if (!key->getScript().isEmpty() && !key->isTemporary()) {
            items.push_back(std::make_tuple(key->getName(), QStringLiteral("key"), key->getID()));
        }
        addKeysToPreview(key, items);
    }

    std::list<TAction*> actions = mpHost->getActionUnit()->getActionRootNodeList();
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

    auto themesAge = QFileInfo(QStringLiteral("%1/.config/mudlet/edbee/Colorsublime-Themes-master/themes.json").arg(QDir::homePath())).lastModified().toUTC();

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
        QTimer::singleShot(5000, theme_download_label, [label = theme_download_label] {
            label->hide();
            label->setText(tr("Updating themes from colorsublime.com..."));
        });
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
                        auto future = QtConcurrent::run(mudlet::unzip, tempThemesArchive->fileName(), QStringLiteral("%1/.config/mudlet/edbee/").arg(QDir::homePath()), temporaryDir.path());
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
    QFile themesFile(QStringLiteral("%1/.config/mudlet/edbee/Colorsublime-Themes-master/themes.json").arg(QDir::homePath()));
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

    std::sort(sortedThemes.begin(), sortedThemes.end(), [](const auto& a, const auto& b) { return QString::localeAwareCompare(a.first, b.first) < 0; });

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
    auto data = script_preview_combobox->itemData(index).value<QPair<QString, int>>();
    auto itemType = data.first;
    auto itemId = data.second;

    auto preview = edbeePreviewWidget->textDocument();
    if (itemType == QStringLiteral("trigger")) {
        preview->setText(mpHost->getTriggerUnit()->getTrigger(itemId)->getScript());
    } else if (itemType == QStringLiteral("alias")) {
        preview->setText(mpHost->getAliasUnit()->getAlias(itemId)->getScript());
    } else if (itemType == QStringLiteral("script")) {
        preview->setText(mpHost->getScriptUnit()->getScript(itemId)->getScript());
    } else if (itemType == QStringLiteral("timer")) {
        preview->setText(mpHost->getTimerUnit()->getTimer(itemId)->getScript());
    } else if (itemType == QStringLiteral("key")) {
        preview->setText(mpHost->getKeyUnit()->getKey(itemId)->getScript());
    } else if (itemType == QStringLiteral("button")) {
        preview->setText(mpHost->getActionUnit()->getAction(itemId)->getScript());
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
