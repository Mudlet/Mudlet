/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014, 2016-2018, 2020 by Stephen Lyons                  *
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
#include "dlgIRC.h"
#include "dlgMapper.h"
#include "dlgTriggerEditor.h"
#include "edbee/views/texteditorscrollarea.h"

#include "pre_guard.h"
#include <chrono>
#include <QtConcurrent>
#include <QColorDialog>
#include <QFileDialog>
#include <QFontDialog>
#include <QNetworkDiskCache>
#include <QPainter>
#include <QString>
#include <QTableWidget>
#include <QToolBar>
#include <QUiLoader>
#include "post_guard.h"

using namespace std::chrono_literals;

dlgProfilePreferences::dlgProfilePreferences(QWidget* pF, Host* pHost)
: QDialog(pF)
, mFontSize(10)
, mpHost(pHost)
, mpMenu(nullptr)
, mUseSharedDictionary(false)
{
    // init generated dialog
    setupUi(this);

    QPixmap holdPixmap;

    holdPixmap = *(this->notificationAreaIconLabelWarning->pixmap());
    holdPixmap.setDevicePixelRatio(5.3);
    this->notificationAreaIconLabelWarning->setPixmap(holdPixmap);

    holdPixmap = *(this->notificationAreaIconLabelError->pixmap());
    holdPixmap.setDevicePixelRatio(5.3);
    this->notificationAreaIconLabelError->setPixmap(holdPixmap);

    holdPixmap = *(this->notificationAreaIconLabelInformation->pixmap());
    holdPixmap.setDevicePixelRatio(5.3);
    this->notificationAreaIconLabelInformation->setPixmap(holdPixmap);

    // The groupBox_debug is no longer empty, (it contains
    // checkBox_showIconsOnMenus) so can no longer be "hidden until needed"
    // it still provides a location on the last (Special Options) tab where
    // temporary/development/testing controls can be placed if needed, they
    // should be added to the (QGridLayout*) returned by:
    // qobject_cast<QGridLayout*>(groupBox_debug->layout())

    mudlet* pMudlet = mudlet::self();

    // Only unhide this if it is needed
    groupBox_discordPrivacy->hide();

    checkBox_USE_SMALL_SCREEN->setChecked(pMudlet->mEnableFullScreenMode);

    // As we demonstrate the options that these next two checkboxes control in
    // the editor "preview" widget (on another tab) we will need to track
    // changes and update the edbee widget straight away. As we can have
    // multiple profiles each with a separate instance of this form open we also
    // have to respond to changes in the settings when *another* profile saves
    // them.
    checkBox_showSpacesAndTabs->setChecked(pMudlet->mEditorTextOptions & QTextOption::ShowTabsAndSpaces);
    checkBox_showLineFeedsAndParagraphs->setChecked(pMudlet->mEditorTextOptions & QTextOption::ShowLineAndParagraphSeparators);

    checkBox_reportMapIssuesOnScreen->setChecked(pMudlet->showMapAuditErrors());
    checkBox_showIconsOnMenus->setCheckState(pMudlet->mShowIconsOnMenuCheckedState);

    MainIconSize->setValue(pMudlet->mToolbarIconSize);
    TEFolderIconSize->setValue(pMudlet->mEditorTreeWidgetIconSize);

    switch (pMudlet->menuBarVisibility()) {
    case mudlet::visibleNever:
        comboBox_menuBarVisibility->setCurrentIndex(0);
        break;
    case mudlet::visibleOnlyWithoutLoadedProfile:
        comboBox_menuBarVisibility->setCurrentIndex(1);
        break;
    default:
        comboBox_menuBarVisibility->setCurrentIndex(2);
    }

    switch (pMudlet->toolBarVisibility()) {
    case mudlet::visibleNever:
        comboBox_toolBarVisibility->setCurrentIndex(0);
        break;
    case mudlet::visibleOnlyWithoutLoadedProfile:
        comboBox_toolBarVisibility->setCurrentIndex(1);
        break;
    default:
        comboBox_toolBarVisibility->setCurrentIndex(2);
    }

    // Set the properties of the log options
    lineEdit_logFileFolder->setToolTip(tr("<p>Location which will be used to store log files - matching logs will be appended to.</p>"));
    pushButton_whereToLog->setToolTip(tr("<p>Select a directory where logs will be saved.</p>"));
    pushButton_resetLogDir->setToolTip(tr("<p>Reset the directory so that logs are saved to the profile's <i>log</i> directory.</p>"));
    comboBox_logFileNameFormat->setToolTip(tr("<p>This option sets the format of the log name.</p>"
                                                                  "<p>If <i>Named file</i> is selected, you can set a custom file name. (Logs are appended "
                                                                  "if a log file of the same name already exists.)</p>"));
    lineEdit_logFileName->setToolTip(tr("<p>Set a custom name for your log. (New logs are appended if a log file of the same name "
                                                            "already exists).</p>"));
    lineEdit_logFileName->setPlaceholderText(tr("logfile",
                                                "Must be a valid default filename for a log-file and is used if the user does not enter any other value (Ensure all instances have the same translation {1 of 2})."));
    label_logFileNameExtension->setVisible(false);
    label_logFileName->setVisible(false);
    lineEdit_logFileName->setVisible(false);

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
        checkbox_noAutomaticUpdates->setToolTip(tr("<p>Automatic updates are disabled in development builds to prevent an update from overwriting your Mudlet.</p>"));
    } else {
        checkbox_noAutomaticUpdates->setChecked(!pMudlet->updater->updateAutomatically());
        // This is the extra connect(...) relating to settings' changes saved by
        // a different profile mentioned further down in this constructor:
        connect(pMudlet->updater, &Updater::signal_automaticUpdatesChanged, this, &dlgProfilePreferences::slot_changeAutomaticUpdates);
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

    // Set the tooltip on the containing widget so both the label and the
    // control have the same tool-tip:
    widget_timerDebugOutputMinimumInterval->setToolTip(tr("<p>A timer with a short interval will quickly fill up the <i>Central Debug Console</i> "
                                                          "windows with messages that it ran correctly on <i>each</i> occasion it is called.  This (per profile) "
                                                          "control adjusts a threshold that will hide those messages in just that window for those timers which "
                                                          "run <b>correctly</b> when the timer's interval is less than this setting.</p>"
                                                          "<p><u>Any timer script that has errors will still have its error messages reported whatever the setting.</u></p>"));

    pushButton_showGlyphUsage->setToolTip(tr("<p>This will bring up a display showing all the symbols used in the current "
                                             "map and whether they can be drawn using just the specifed font, any other "
                                             "font, or not at all.  It also shows the sequence of Unicode <i>code-points</i> "
                                             "that make up that symbol, so that they can be identified even if they "
                                             "cannot be displayed; also, up to the first thirty two rooms that are using "
                                             "that symbol are listed, which may help to identify any unexpected or odd cases.<p>"));
    fontComboBox_mapSymbols->setToolTip(tr("<p>Select the only or the primary font used (depending on <i>Only use symbols "
                                           "(glyphs) from chosen font</i> setting) to produce the 2D mapper room symbols.</p>"));
    checkBox_isOnlyMapSymbolFontToBeUsed->setToolTip(tr("<p>Using a single font is likely to produce a more consistent style but may "
                                                        "cause the <i>font replacement character</i> '<b>�</b>' to show if the font "
                                                        "does not have a needed glyph (a font's individual character/symbol) to represent "
                                                        "the grapheme (what is to be represented).  Clearing this checkbox will allow "
                                                        "the best alternative glyph from another font to be used to draw that grapheme.</p>"));
    checkBox_runAllKeyBindings->setToolTip(tr("<p>If <b>not</b> checked Mudlet will only react to the first matching keybinding "
                                              "(combination of key and modifiers) even if more than one of them is set to be "
                                              "active. This means that a temporary keybinding (not visible in the Editor) "
                                              "created by a script or package may be used in preference to a permanent one "
                                              "that is shown and is set to be active. If checked then all matching keybindings "
                                              "will be run.</p>"
                                              "<p><i>It is recommended to not enable this option if you need to maintain compatibility "
                                              "with scripts or packages for Mudlet versions prior to <b>3.9.0</b>.</i></p>"));
    checkBox_useWideAmbiguousEastAsianGlyphs->setToolTip(tr("<p>Some East Asian MUDs may use glyphs (characters) that Unicode classifies as being "
                                                            "of <i>Ambigous</i> width when drawn in a font with a so-called <i>fixed</i> pitch; in "
                                                            "fact such text is <i>duo-spaced</i> when not using a proportional font. These symbols can be "
                                                            "drawn using either a half or the whole space of a full character. By default Mudlet tries to "
                                                            "chose the right width automatically but you can override the setting for each profile.</p>"
                                                            "<p>This control has three settings:"
                                                            "<ul><li><b>Unchecked</b> '<i>narrow</i>' = Draw ambiguous width characters in a single 'space'.</li>"
                                                            "<li><b>Checked</b> '<i>wide</i>' = Draw ambiguous width characters two 'spaces' wide.</li>"
                                                            "<li><b>Partly checked</b> <i>(Default) 'auto'</i> = Use 'wide' setting for MUD Server "
                                                            "encodings of <b>Big5</b>, <b>GBK</b> or <b>GBK18030</b> and 'narrow' for all others.</li></ul></p>"
                                                            "<p><i>This is a temporary arrangement and will probably change when Mudlet gains "
                                                            "full support for languages other than English.</i></p>"));
    checkBox_enableTextAnalyzer->setToolTip(tr("<p>Enable a context (right click) menu action on any console/user window that, "
                                               "when the mouse cursor is hovered over it, will display the UTF-16 and UTF-8 items "
                                               "that make up each Unicode codepoint on the <b>first</b> line of any selection.</p>"
                                               "<p>This utility feature is intended to help the user identify any grapheme "
                                               "(visual equivalent to a <i>character</i>) that a Game server may send even "
                                               "if it is composed of multiple bytes as any non-ASCII character will be in the "
                                               "Lua sub-system which uses the UTF-8 encoding system.<p>"));
    checkBox_showIconsOnMenus->setToolTip(tr("<p>Some Desktop Environments tell Qt applications like Mudlet whether they should "
                                             "shown icons on menus, others, however do not. This control allows the user to override "
                                             "the setting, if needed, as follows:"
                                             "<ul><li><b>Unchecked</b> '<i>off</i>' = Prevent menus from being drawn with icons.</li>"
                                             "<li><b>Checked</b> '<i>on</i>' = Allow menus to be drawn with icons.</li>"
                                             "<li><b>Partly checked</b> <i>(Default) 'auto'</i> = Use the setting that the system provides.</li></ul></p>"
                                             "<p><i>This setting is only processed when individual menus are created and changes may not "
                                             "propogate everywhere until Mudlet is restarted.</i></p>"));


    connect(checkBox_showSpacesAndTabs, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_changeShowSpacesAndTabs);
    connect(checkBox_showLineFeedsAndParagraphs, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_changeShowLineFeedsAndParagraphs);
    connect(closeButton, &QAbstractButton::pressed, this, &dlgProfilePreferences::slot_save_and_exit);
    connect(pMudlet, &mudlet::signal_hostCreated, this, &dlgProfilePreferences::slot_handleHostAddition);
    connect(pMudlet, &mudlet::signal_hostDestroyed, this, &dlgProfilePreferences::slot_handleHostDeletion);
    // Because QComboBox::currentIndexChanged has multiple (overloaded) forms we
    // have to state which one we want to use for these two:
    connect(comboBox_menuBarVisibility, qOverload<int>(&QComboBox::currentIndexChanged), this, &dlgProfilePreferences::slot_changeShowMenuBar);
    connect(comboBox_toolBarVisibility, qOverload<int>(&QComboBox::currentIndexChanged), this, &dlgProfilePreferences::slot_changeShowToolBar);

    // This group of signal/slot connections handle updating *this* instance of
    // the "Profile preferences" form/dialog when a *different* profile saves
    // new settings from it's one - there is a further connect(...) above which
    // is also involved but it is conditional on having the updater code being
    // included in compliation:
    connect(pMudlet, &mudlet::signal_enableFulScreenModeChanged, this, &dlgProfilePreferences::slot_changeEnableFullScreenMode);
    connect(pMudlet, &mudlet::signal_editorTextOptionsChanged, this, &dlgProfilePreferences::slot_changeEditorTextOptions);
    connect(pMudlet, &mudlet::signal_showMapAuditErrorsChanged, this, &dlgProfilePreferences::slot_changeShowMapAuditErrors);
    connect(pMudlet, &mudlet::signal_setToolBarIconSize, this, &dlgProfilePreferences::slot_setToolBarIconSize);
    connect(pMudlet, &mudlet::signal_setTreeIconSize, this, &dlgProfilePreferences::slot_setTreeWidgetIconSize);
    connect(pMudlet, &mudlet::signal_menuBarVisibilityChanged, this, &dlgProfilePreferences::slot_changeMenuBarVisibility);
    connect(pMudlet, &mudlet::signal_toolBarVisibilityChanged, this, &dlgProfilePreferences::slot_changeToolBarVisibility);
    connect(pMudlet, &mudlet::signal_showIconsOnMenusChanged, this, &dlgProfilePreferences::slot_changeShowIconsOnMenus);
    connect(pMudlet, &mudlet::signal_guiLanguageChanged, this, &dlgProfilePreferences::slot_guiLanguageChanged);

    generateDiscordTooltips();

    label_languageChangeWarning->hide();
    label_invalidFontError->hide();
    label_variableWidthFontWarning->hide();

    comboBox_guiLanguage->clear();
    for (auto& code : pMudlet->getAvailableTranslationCodes()) {
        auto& translation = pMudlet->mTranslationsMap.value(code);
        auto& nativeName = translation.getNativeName();
        if (translation.fromResourceFile()) {
            auto& translatedPc = translation.getTranslatedPercentage();
            if (translatedPc >= pMudlet->mTranslationGoldStar) {
                comboBox_guiLanguage->addItem(QIcon(":/icons/rating.png"),
                                              nativeName,
                                              code);
            } else {
                // This will also be used if the percentage is set to zero
                // because it was not found in the translation statistics file
                // during compilation even though the Mudlet translation is in
                // the resources file:
                comboBox_guiLanguage->addItem(QIcon(),
                                              tr("%1 (%2% done)",
                                                 // Intentional argument to separate arguments
                                                 "%1 is the (not-translated so users of the language can read it!) language name, %2 is percentage done.")
                                              .arg(nativeName, QString::number(translatedPc)),
                                              code);
            }
        } else {
            // For translations that come from somewhere else we are not likely
            // to have the translations statistics so no icon and no extra text:
            comboBox_guiLanguage->addItem(QIcon(), nativeName, code);
        }
    }
    comboBox_guiLanguage->model()->sort(0);
    auto currentLanguage = pMudlet->getInterfaceLanguage();
    int currentIndex = comboBox_guiLanguage->findData(currentLanguage);
    if (Q_LIKELY(currentIndex != -1)) {
        // The language code has been found in the UserData role for one of the
        // entries - so select it
        comboBox_guiLanguage->setCurrentIndex(currentIndex);
        connect(comboBox_guiLanguage, QOverload<const QString &>::of(&QComboBox::currentIndexChanged), this, &dlgProfilePreferences::slot_changeGuiLanguage);
    } else {
        currentIndex = comboBox_guiLanguage->findData(QStringLiteral("en_US"));
        if (Q_LIKELY(currentIndex != -1)) {
           // The default code has been found in the UserData role for one of
           // the entries - so select it as a fallback
            comboBox_guiLanguage->setCurrentIndex(currentIndex);
            connect(comboBox_guiLanguage, QOverload<const QString &>::of(&QComboBox::currentIndexChanged), this, &dlgProfilePreferences::slot_changeGuiLanguage);
        } else if (comboBox_guiLanguage->count()) {
            // There is at least ONE entry but it is not the expected one
            // or the American English default - so select that first one as a
            // last ditch effort:
            comboBox_guiLanguage->setCurrentIndex(0);
            connect(comboBox_guiLanguage, QOverload<const QString &>::of(&QComboBox::currentIndexChanged), this, &dlgProfilePreferences::slot_changeGuiLanguage);
        } else {
            // Nothing available - so disable the control:
            comboBox_guiLanguage->setEnabled(false);
            // And insert an Engineering English warning text - this is probably
            // a sign of significant borkage in the translation system!
            comboBox_guiLanguage->addItem(QStringLiteral("No translations available!"));
        }
    }

    setupPasswordsMigration();
}

void dlgProfilePreferences::setupPasswordsMigration()
{
    hidePasswordMigrationLabelTimer = std::make_unique<QTimer>(this);
    hidePasswordMigrationLabelTimer->setSingleShot(true);

    connect(hidePasswordMigrationLabelTimer.get(), &QTimer::timeout, this, &dlgProfilePreferences::hidePasswordMigrationLabel);

    connect(mudlet::self(), &mudlet::signal_passwordsMigratedToSecure, [=]() {
        label_password_migration_notification->setText(tr("Migrated all passwords to secure storage."));
        comboBox_store_passwords_in->setEnabled(true);
        hidePasswordMigrationLabelTimer->start(10s);
    });

    connect(mudlet::self(), &mudlet::signal_passwordMigratedToSecure, [=](const QString& profile) {
        label_password_migration_notification->setText(
                tr("Migrated %1...", "This notifies the user that progress is being made on profile migration by saying what profile was just migrated to store passwords securely").arg(profile));
    });

    connect(mudlet::self(), &mudlet::signal_passwordsMigratedToProfiles, [=]() {
        label_password_migration_notification->setText(tr("Migrated all passwords to profile storage."));
        comboBox_store_passwords_in->setEnabled(true);
        hidePasswordMigrationLabelTimer->start(10s);
    });

    if (mudlet::self()->storingPasswordsSecurely()) {
        comboBox_store_passwords_in->setCurrentIndex(0);
    } else {
        comboBox_store_passwords_in->setCurrentIndex(1);
    }

    connect(comboBox_store_passwords_in, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &dlgProfilePreferences::slot_passwords_location_changed);
}

void dlgProfilePreferences::disableHostDetails()
{
    // The Host pointer is a nullptr so disable every control that depends on it

    // on tab_general:
    // groupBox_iconsAndToolbars is NOT dependent on pHost - so leave it alone
    label_encoding->setEnabled(false);
    comboBox_encoding->setEnabled(false);
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
    checkBox_enableTextAnalyzer->setEnabled(false);
    checkBox_echoLuaErrors->setEnabled(false);
    checkBox_useWideAmbiguousEastAsianGlyphs->setEnabled(false);
    widget_timerDebugOutputMinimumInterval->setEnabled(false);

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
    hidePasswordMigrationLabel();
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

    // on groupBox_logOptions:
    groupBox_logOptions->setEnabled(false);
    lineEdit_logFileName->setVisible(false);
    label_logFileName->setVisible(false);
    label_logFileNameExtension->setVisible(false);

    // on groupBox_specialOptions:
    groupBox_specialOptions->setEnabled(false);

    // it is possible to connect using the IRC client off of the
    // "default" host even without a normal profile loaded so leave
    // groupBox_ircOptions enabled...
    need_reconnect_for_specialoption->hide();
    groupbox_searchEngineSelection->setEnabled(false);

    groupBox_discordPrivacy->hide();

    // tab security
    groupBox_ssl->setEnabled(false);
}

void dlgProfilePreferences::enableHostDetails()
{
    // on tab_general:
    label_encoding->setEnabled(true);
    comboBox_encoding->setEnabled(true);
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
    checkBox_enableTextAnalyzer->setEnabled(true);
    checkBox_echoLuaErrors->setEnabled(true);
    checkBox_useWideAmbiguousEastAsianGlyphs->setEnabled(true);
    widget_timerDebugOutputMinimumInterval->setEnabled(true);

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
    groupBox_logOptions->setEnabled(true);

    // on groupBox_specialOptions:
    groupBox_specialOptions->setEnabled(true);
    // it is possible to connect using the IRC client off of the
    // "default" host even without a normal profile loaded so leave
    // groupBox_ircOptions enabled...
    groupbox_searchEngineSelection->setEnabled(true);

#if defined(QT_NO_SSL)
    groupBox_ssl->setEnabled(false);
#else
    groupBox_ssl->setEnabled(QSslSocket::supportsSsl());
#endif
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
    acceptServerMedia->setChecked(pHost->mAcceptServerMedia);

    ircHostName->setText(dlgIRC::readIrcHostName(pHost));
    ircHostPort->setText(QString::number(dlgIRC::readIrcHostPort(pHost)));
    ircChannels->setText(dlgIRC::readIrcChannels(pHost).join(" "));
    ircNick->setText(dlgIRC::readIrcNickName(pHost));

    dictList->setSelectionMode(QAbstractItemView::SingleSelection);
    dictList->clear();
    // Disable sorting whilst populating the widget:
    dictList->setSortingEnabled(false);
    checkBox_spellCheck->setChecked(pHost->mEnableSpellCheck);
    bool useUserDictionary = false;
    pHost->getUserDictionaryOptions(useUserDictionary, mUseSharedDictionary);
    // Always set the true radio button first - avoids any problems with
    // exclusivity of radio buttons:
    if (mUseSharedDictionary) {
        radioButton_userDictionary_common->setChecked(true);
        radioButton_userDictionary_profile->setChecked(false);
    } else {
        radioButton_userDictionary_profile->setChecked(true);
        radioButton_userDictionary_common->setChecked(false);
    }
    checkBox_echoLuaErrors->setChecked(pHost->mEchoLuaErrors);
    checkBox_useWideAmbiguousEastAsianGlyphs->setCheckState(pHost->getWideAmbiguousEAsianGlyphsControlState());

    // On the first run for a profile this will be the "English (American)"
    // dictionary "en_US".
    const QString& currentDictionary = pHost->getSpellDic();
    // This will also set mudlet::mUsingMudletDictionaries as appropriate:
    QString path = mudlet::getMudletPath(mudlet::hunspellDictionaryPath, currentDictionary);

    // Tweak the label for the provided spelling dictionaries depending on where
    // they come from:
    if (mudlet::self()->mUsingMudletDictionaries) {
        checkBox_spellCheck->setText(tr("Mudlet dictionaries:", "On Windows and MacOs, we have to bundle our own dictionaries with our application - and we also use them on *nix systems where we do not find the system ones."));
    } else {
        checkBox_spellCheck->setText(tr("System dictionaries:", "On *nix systems where we find the system ones we use them."));
    }

    QDir dir(path);
    QStringList entries = dir.entryList(QDir::Files, QDir::Time);
    // QRegularExpression rex(QStringLiteral(R"(\.dic$)"));
    // Use the affix file as that may eliminate supplimental dictionaries:
    QRegularExpression rex(QStringLiteral(R"(\.aff$)"));
    entries = entries.filter(rex);
    // Don't emit signals - like (void) QListWidget::currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
    // whilst populating the widget, it reduces noise about:
    // "qt.accessibility.core: Cannot create accessible child interface for object:  QListWidget(0x############, name = "dictList")  index:  ##
    dictList->blockSignals(true);
    if (entries.count()) {
        QListWidgetItem* scrollToItem = nullptr;
        for (int i = 0, total = entries.size(); i < total; ++i) {
            // This is a file name and to support macOs platforms should not be case sensitive:
            entries[i].remove(QLatin1String(".aff"), Qt::CaseInsensitive);

            if (entries.at(i).endsWith(QStringLiteral("med"), Qt::CaseInsensitive)) {
                // Skip medical dictionaries - there may be others  we also want to hide:
                continue;
            }

            auto item = new QListWidgetItem();
            item->setTextAlignment(Qt::AlignCenter);
            auto key = entries.at(i).toLower();
            // In some cases '-' will be used as a separator and in others '_' so convert all to one form:
            key.replace(QLatin1String("-"), QLatin1String("_"));
            if (mudlet::self()->mDictionaryLanguageCodeMap.contains(key)) {
                item->setText(mudlet::self()->mDictionaryLanguageCodeMap.value(key));
                item->setToolTip(tr("<p>From the dictionary file <tt>%1.dic</tt> (and its companion affix <tt>.aff</tt> file).</p>").arg(dir.absoluteFilePath(entries.at(i))));
            } else {
                item->setText(tr("%1 - not recognised").arg(entries.at(i)));
                item->setToolTip(tr("<p>Mudlet does not recognise the code \"%1\", please report it to the Mudlet developers so we can describe it properly in future Mudlet versions!</p>"
                                    "<p>The file <tt>%2.dic</tt> (and its companion affix <tt>.aff</tt> file) is still usable.</p>").arg(entries.at(i), dir.absoluteFilePath(entries.at(i))));
            }
            item->setData(Qt::UserRole, entries.at(i));
            dictList->addItem(item);
            if (entries.at(i) == currentDictionary) {
                scrollToItem = item;
            }
        }

        // Reenable sorting now we have populated the widget:
        dictList->setSortingEnabled(true);
        // Actually do the sort:
        dictList->sortItems();

        if (scrollToItem) {
            // As the selection mode is set to
            // QAbstractItemView::SingleSelection this also selects this item:
            dictList->setCurrentItem(scrollToItem);
            // And scroll to it:
            dictList->scrollToItem(scrollToItem);
        }

    } else {
        dictList->setEnabled(false);
        dictList->setToolTip(tr("No Hunspell dictionary files found, spell-checking will not be available."));
    }
    dictList->blockSignals(false);

    if (!pHost->getMmpMapLocation().isEmpty()) {
        groupBox_downloadMapOptions->setVisible(true);
        connect(buttonDownloadMap, &QAbstractButton::clicked, this, &dlgProfilePreferences::downloadMap);
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

    fontComboBox->setCurrentFont(pHost->getDisplayFont());
    mFontSize = pHost->getDisplayFont().pointSize();
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

    checkBox_USE_IRE_DRIVER_BUGFIX->setChecked(pHost->mUSE_IRE_DRIVER_BUGFIX);
    checkBox_enableTextAnalyzer->setChecked(pHost->mEnableTextAnalyzer);
    checkBox_mUSE_FORCE_LF_AFTER_PROMPT->setChecked(pHost->mUSE_FORCE_LF_AFTER_PROMPT);
    USE_UNIX_EOL->setChecked(pHost->mUSE_UNIX_EOL);

    if (mudlet::self()->mDiscord.libraryLoaded()) {
        Host::DiscordOptionFlags discordFlags = pHost->mDiscordAccessFlags;
        groupBox_discordPrivacy->show();
        checkBox_discordLuaAPI->setChecked(discordFlags & Host::DiscordLuaAccessEnabled);

        if ((discordFlags & Host::DiscordSetLargeIcon) && (discordFlags & Host::DiscordSetLargeIconText)) {
            comboBox_discordLargeIconPrivacy->setCurrentIndex(0);
        } else if ((discordFlags & Host::DiscordSetLargeIcon) && !(discordFlags & Host::DiscordSetLargeIconText)) {
            comboBox_discordLargeIconPrivacy->setCurrentIndex(1);
        } else {
            comboBox_discordLargeIconPrivacy->setCurrentIndex(2);
        }

        if ((discordFlags & Host::DiscordSetSmallIcon) && (discordFlags & Host::DiscordSetSmallIconText)) {
            comboBox_discordSmallIconPrivacy->setCurrentIndex(0);
        } else if ((discordFlags & Host::DiscordSetSmallIcon) && !(discordFlags & Host::DiscordSetSmallIconText)) {
            comboBox_discordSmallIconPrivacy->setCurrentIndex(1);
        } else {
            comboBox_discordSmallIconPrivacy->setCurrentIndex(2);
        }

        checkBox_discordServerAccessToDetail->setChecked(!(discordFlags & Host::DiscordSetDetail));
        checkBox_discordServerAccessToState->setChecked(!(discordFlags & Host::DiscordSetState));
        checkBox_discordServerAccessToPartyInfo->setChecked(!(discordFlags & Host::DiscordSetPartyInfo));
        checkBox_discordServerAccessToTimerInfo->setChecked(!(discordFlags & Host::DiscordSetTimeInfo));
        lineEdit_discordUserName->setText(pHost->mRequiredDiscordUserName);
        lineEdit_discordUserDiscriminator->setText(pHost->mRequiredDiscordUserDiscriminator);
    }

    checkBox_runAllKeyBindings->setChecked(pHost->getKeyUnit()->mRunAllKeyMatches);

    topBorderHeight->setValue(pHost->mBorderTopHeight);
    bottomBorderHeight->setValue(pHost->mBorderBottomHeight);
    leftBorderWidth->setValue(pHost->mBorderLeftWidth);
    rightBorderWidth->setValue(pHost->mBorderRightWidth);

    // Set the properties in groupBox_logOptions
    mIsLoggingTimestamps->setChecked(pHost->mIsLoggingTimestamps);
    mIsToLogInHtml->setChecked(pHost->mIsNextLogFileInHtmlFormat);

    bool isLogFileNameEntryShown = pHost->mLogFileNameFormat.isEmpty();
    QString logExtension = pHost->mIsNextLogFileInHtmlFormat ? ".html" : ".txt";
    label_logFileNameExtension->setVisible(isLogFileNameEntryShown);
    lineEdit_logFileName->setVisible(isLogFileNameEntryShown);
    label_logFileName->setVisible(isLogFileNameEntryShown);
    label_logFileNameExtension->setText(logExtension);

    // This is the previous standard:
    comboBox_logFileNameFormat->addItem(tr("yyyy-MM-dd#HH-mm-ss (e.g., 1970-01-01#00-00-00%1)").arg(logExtension), QStringLiteral("yyyy-MM-dd#HH-mm-ss"));
    // The ISO standard for this uses T as the date/time separator
    comboBox_logFileNameFormat->addItem(tr("yyyy-MM-ddTHH-mm-ss (e.g., 1970-01-01T00-00-00%1)").arg(logExtension), QStringLiteral("yyyy-MM-ddTHH-mm-ss"));
    comboBox_logFileNameFormat->addItem(tr("yyyy-MM-dd (concatenate daily logs in, e.g. 1970-01-01%1)").arg(logExtension), QStringLiteral("yyyy-MM-dd"));
    // It might be possible to use QDateTime::weekNumber but that number is not
    // available from the QDateTime::toString(...) method
    comboBox_logFileNameFormat->addItem(tr("yyyy-MM (concatenate month logs in, e.g. 1970-01%1)").arg(logExtension), QStringLiteral("yyyy-MM"));
    comboBox_logFileNameFormat->addItem(tr("Named file (concatenate logs in one file)"), QString());
    comboBox_logFileNameFormat->setCurrentIndex(comboBox_logFileNameFormat->findData(pHost->mLogFileNameFormat));

    lineEdit_logFileName->setText(pHost->mLogFileName);

    // pHost->mLogDir should be empty for the default location:
    mLogDirPath = pHost->mLogDir;
    lineEdit_logFileFolder->setText(mLogDirPath);
    lineEdit_logFileFolder->setPlaceholderText(mudlet::getMudletPath(mudlet::profileReplayAndLogFilesPath, pHost->getName()));
    // set the cursor position to the end of the lineEdit's text property.
    lineEdit_logFileFolder->setCursorPosition(lineEdit_logFileFolder->text().length());
    // Enable the reset button if the current location is not the default one:
    pushButton_resetLogDir->setEnabled(mLogDirPath.length() > 0);


    commandLineMinimumHeight->setValue(pHost->commandLineMinimumHeight);
    mNoAntiAlias->setChecked(!pHost->mNoAntiAlias);
    mFORCE_MCCP_OFF->setChecked(pHost->mFORCE_NO_COMPRESSION);
    mFORCE_GA_OFF->setChecked(pHost->mFORCE_GA_OFF);
    mAlertOnNewData->setChecked(pHost->mAlertOnNewData);
    //encoding->setCurrentIndex( pHost->mEncoding );
    mFORCE_SAVE_ON_EXIT->setChecked(pHost->mFORCE_SAVE_ON_EXIT);
    mEnableGMCP->setChecked(pHost->mEnableGMCP);
    mEnableMSDP->setChecked(pHost->mEnableMSDP);
    mEnableMSSP->setChecked(pHost->mEnableMSSP);
    mEnableMSP->setChecked(pHost->mEnableMSP);

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
        if (s.isEmpty() || !s.compare(pHost->getName())) {
            // Do not include THIS profile in the list - it will
            // automatically get saved - as the file to copy to the other
            // profiles!
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

    hidePasswordMigrationLabel();

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
        auto * pdebugLayout = qobject_cast<QGridLayout*>(groupBox_debug->layout());
        if (pdebugLayout) {
            int existingRows = pdebugLayout->rowCount();
            pdebugLayout->addWidget(pLabel_mapSymbolFontFudge, existingRows, 0);
            pdebugLayout->addWidget(mpDoubleSpinBox_mapSymbolFontFudge, existingRows, 1);
        } else {
            qWarning() << "dlgProfilePreferences::initWithHost(...) WARNING - Unable to cast groupBox_debug layout to expected QGridLayout - someone has messed with the profile_preferences.ui file and the contents of the groupBox can not be shown...!";
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
        connect(pushButton_showGlyphUsage, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_showMapGlyphUsage, Qt::UniqueConnection);
        connect(fontComboBox_mapSymbols, &QFontComboBox::currentFontChanged, this, &dlgProfilePreferences::slot_setMapSymbolFont, Qt::UniqueConnection);
        connect(checkBox_isOnlyMapSymbolFontToBeUsed, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapSymbolFontStrategy, Qt::UniqueConnection);

        widget_playerRoomStyle->show();
        comboBox_playerRoomStyle->setCurrentIndex(pHost->mpMap->mPlayerRoomStyle);
        // Custom colours only available in style '3' (of '0' to '3'):
        pushButton_playerRoomPrimaryColor->setEnabled(pHost->mpMap->mPlayerRoomStyle == 3);
        pushButton_playerRoomSecondaryColor->setEnabled(pHost->mpMap->mPlayerRoomStyle == 3);
        spinBox_playerRoomOuterDiameter->setValue(pHost->mpMap->mPlayerRoomOuterDiameterPercentage);
        spinBox_playerRoomInnerDiameter->setValue(pHost->mpMap->mPlayerRoomInnerDiameterPercentage);
        // Adjustable inner diameter not available for style '0' (original):
        spinBox_playerRoomInnerDiameter->setEnabled(pHost->mpMap->mPlayerRoomStyle != 0);
        setButtonColor(pushButton_playerRoomPrimaryColor, pHost->mpMap->mPlayerRoomOuterColor);
        setButtonColor(pushButton_playerRoomSecondaryColor, pHost->mpMap->mPlayerRoomInnerColor);
        connect(comboBox_playerRoomStyle, qOverload<int>(&QComboBox::currentIndexChanged), this, &dlgProfilePreferences::slot_changePlayerRoomStyle);
        connect(pushButton_playerRoomPrimaryColor, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setPlayerRoomPrimaryColor);
        connect(pushButton_playerRoomSecondaryColor, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setPlayerRoomSecondaryColor);
        connect(spinBox_playerRoomOuterDiameter, qOverload<int>(&QSpinBox::valueChanged), this, &dlgProfilePreferences::slot_setPlayerRoomOuterDiameter);
        connect(spinBox_playerRoomInnerDiameter, qOverload<int>(&QSpinBox::valueChanged), this, &dlgProfilePreferences::slot_setPlayerRoomInnerDiameter);
    } else {
        label_mapSymbolsFont->setEnabled(false);
        fontComboBox_mapSymbols->setEnabled(false);
        checkBox_isOnlyMapSymbolFontToBeUsed->setEnabled(false);
        pushButton_showGlyphUsage->setEnabled(false);

        checkBox_showDefaultArea->hide();
        widget_playerRoomStyle->hide();
    }

    comboBox_encoding->addItem(mudlet::self()->getEncodingNamesMap().value(QByteArray("ASCII")), QByteArray("ASCII"));
    for (auto encoding : pHost->mTelnet.getEncodingsList()) {
        auto encodingTitle = mudlet::self()->getEncodingNamesMap().value(encoding, tr("%1 (*Error, report to Mudlet Makers*)",
                                                                                      // Intentional comment to separate arguments
                                                                                      "The encoder code name is not in the mudlet class mEncodingNamesMap when it should be and the Mudlet Makers need to fix it!")
                                                                         .arg(QLatin1String(encoding)));
        comboBox_encoding->addItem(encodingTitle, encoding);
    }
    if (pHost->mTelnet.getEncoding().isEmpty()) {
        // cTelnet::mEncoding is (or should be) empty for the default 7-bit
        // ASCII case, so need to set the control specially to its (the
        // first) value
        comboBox_encoding->setCurrentIndex(0);
    } else {
        int currentIndex = comboBox_encoding->findData(pHost->mTelnet.getEncoding());
        if (currentIndex >=0) {
            comboBox_encoding->setCurrentIndex(currentIndex);
        } else {
            // invalid or not found - so reset to ASCII:
            comboBox_encoding->setCurrentIndex(0);
        }
    }

    timeEdit_timerDebugOutputMinimumInterval->setTime(pHost->mTimerDebugOutputSuppressionInterval);
    notificationArea->hide();
    notificationAreaIconLabelWarning->hide();
    notificationAreaIconLabelError->hide();
    notificationAreaIconLabelInformation->hide();
    notificationAreaMessageBox->hide();

#if !defined(QT_NO_SSL)
    if (QSslSocket::supportsSsl() && pHost->mSslTsl) {
        QSslCertificate cert = pHost->mTelnet.getPeerCertificate();
        if (cert.isNull()) {
            groupBox_ssl_certificate->hide();
        } else {
            ssl_issuer_label->setText(cert.issuerInfo(QSslCertificate::CommonName).join(","));
            ssl_issued_label->setText(cert.subjectInfo(QSslCertificate::CommonName).join(","));
            ssl_expires_label->setText(cert.expiryDate().toString(Qt::LocalDate));
            ssl_serial_label->setText(QString::fromStdString(cert.serialNumber().toStdString()));
            checkBox_self_signed->setStyleSheet("");
            checkBox_expired->setStyleSheet("");
            ssl_issuer_label->setStyleSheet("");
            ssl_expires_label->setStyleSheet("");

            if (!pHost->mTelnet.getSslErrors().empty()) {
                // handle ssl errors
                notificationAreaIconLabelWarning->show();
                notificationArea->show();
                notificationAreaMessageBox->show();
                //notificationAreaMessageBox->setText(pHost->mTelnet.errorString());

                QList<QSslError> sslErrors = pHost->mTelnet.getSslErrors();

                for (int a = 0; a < sslErrors.count(); a++) {
                    QString thisError = QStringLiteral("<li>%1</li>").arg(sslErrors.at(a).errorString());
                    notificationAreaMessageBox->setText(QStringLiteral("%1\n%2").arg(notificationAreaMessageBox->text(), thisError));

                    if (sslErrors.at(a).error() == QSslError::SelfSignedCertificate) {
                        checkBox_self_signed->setStyleSheet(QStringLiteral("font-weight: bold; background: yellow"));
                        ssl_issuer_label->setStyleSheet(QStringLiteral("font-weight: bold; color: red; background: yellow"));
                    }
                    if (sslErrors.at(a).error() == QSslError::CertificateExpired) {
                        checkBox_expired->setStyleSheet(QStringLiteral("font-weight: bold; background: yellow"));
                        ssl_expires_label->setStyleSheet(QStringLiteral("font-weight: bold; color: red; background: yellow"));
                    }
                }

            } else if (pHost->mTelnet.error() == QAbstractSocket::SslHandshakeFailedError) {
                // handle failed handshake, likely not ssl socket
                notificationAreaIconLabelError->show();
                notificationArea->show();
                notificationAreaMessageBox->show();
                notificationAreaMessageBox->setText(pHost->mTelnet.errorString());
            }
            if (pHost->mTelnet.error() == QAbstractSocket::SslInternalError) {
                // handle ssl library error
                notificationAreaIconLabelError->show();
                notificationArea->show();
                notificationAreaMessageBox->show();
                notificationAreaMessageBox->setText(pHost->mTelnet.errorString());
            }
            if (pHost->mTelnet.error() == QAbstractSocket::SslInvalidUserDataError) {
                // handle invalid data (certificate, key, cypher, etc.)
                notificationAreaIconLabelError->show();
                notificationArea->show();
                notificationAreaMessageBox->show();
                notificationAreaMessageBox->setText(pHost->mTelnet.errorString());
            }
        }
    }
#endif

    groupBox_ssl->setChecked(pHost->mSslTsl);
    checkBox_self_signed->setChecked(pHost->mSslIgnoreSelfSigned);
    checkBox_expired->setChecked(pHost->mSslIgnoreExpired);
    checkBox_ignore_all->setChecked(pHost->mSslIgnoreAll);

    groupBox_proxy->setEnabled(true);
    groupBox_proxy->setChecked(pHost->mUseProxy);
    lineEdit_proxyAddress->setText(pHost->mProxyAddress);
    if (pHost->mProxyPort != 0) {
        lineEdit_proxyPort->setText(QString::number(pHost->mProxyPort));
    }
    lineEdit_proxyUsername->setText(pHost->mProxyUsername);
    lineEdit_proxyPassword->setText(pHost->mProxyPassword);

    checkBox_expectCSpaceIdInColonLessMColorCode->setChecked(pHost->getHaveColorSpaceId());
    checkBox_allowServerToRedefineColors->setChecked(pHost->getMayRedefineColors());

    // Enable the controls that would be disabled if there wasn't a Host instance
    // on tab_general:
    // groupBox_iconsAndToolbars is NOT dependent on pHost - leave it alone
    enableHostDetails();

    // Identify which Profile we are showing the settings for:
    setWindowTitle(tr("Profile preferences - %1").arg(pHost->getName()));

    // CHECKME: Have moved ALL the connects, where possible, to the end so that
    // none are triggered by the setup operations...
    connect(pushButton_command_line_foreground_color, &QAbstractButton::clicked, this, &dlgProfilePreferences::setCommandLineFgColor);
    connect(pushButton_command_line_background_color, &QAbstractButton::clicked, this, &dlgProfilePreferences::setCommandLineBgColor);

    connect(pushButton_black, &QAbstractButton::clicked, this, &dlgProfilePreferences::setColorBlack);
    connect(pushButton_lBlack, &QAbstractButton::clicked, this, &dlgProfilePreferences::setColorLightBlack);
    connect(pushButton_red, &QAbstractButton::clicked, this, &dlgProfilePreferences::setColorRed);
    connect(pushButton_lRed, &QAbstractButton::clicked, this, &dlgProfilePreferences::setColorLightRed);
    connect(pushButton_green, &QAbstractButton::clicked, this, &dlgProfilePreferences::setColorGreen);
    connect(pushButton_lGreen, &QAbstractButton::clicked, this, &dlgProfilePreferences::setColorLightGreen);
    connect(pushButton_yellow, &QAbstractButton::clicked, this, &dlgProfilePreferences::setColorYellow);
    connect(pushButton_lYellow, &QAbstractButton::clicked, this, &dlgProfilePreferences::setColorLightYellow);
    connect(pushButton_blue, &QAbstractButton::clicked, this, &dlgProfilePreferences::setColorBlue);
    connect(pushButton_lBlue, &QAbstractButton::clicked, this, &dlgProfilePreferences::setColorLightBlue);
    connect(pushButton_magenta, &QAbstractButton::clicked, this, &dlgProfilePreferences::setColorMagenta);
    connect(pushButton_lMagenta, &QAbstractButton::clicked, this, &dlgProfilePreferences::setColorLightMagenta);
    connect(pushButton_cyan, &QAbstractButton::clicked, this, &dlgProfilePreferences::setColorCyan);
    connect(pushButton_lCyan, &QAbstractButton::clicked, this, &dlgProfilePreferences::setColorLightCyan);
    connect(pushButton_white, &QAbstractButton::clicked, this, &dlgProfilePreferences::setColorWhite);
    connect(pushButton_lWhite, &QAbstractButton::clicked, this, &dlgProfilePreferences::setColorLightWhite);

    connect(pushButton_foreground_color, &QAbstractButton::clicked, this, &dlgProfilePreferences::setFgColor);
    connect(pushButton_background_color, &QAbstractButton::clicked, this, &dlgProfilePreferences::setBgColor);
    connect(pushButton_command_foreground_color, &QAbstractButton::clicked, this, &dlgProfilePreferences::setCommandFgColor);
    connect(pushButton_command_background_color, &QAbstractButton::clicked, this, &dlgProfilePreferences::setCommandBgColor);

    connect(pushButton_resetColors, &QAbstractButton::clicked, this, &dlgProfilePreferences::resetColors);
    connect(reset_colors_button_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::resetColors2);

    connect(fontComboBox, &QFontComboBox::currentFontChanged, this, &dlgProfilePreferences::setDisplayFont);
    connect(fontSize, qOverload<int>(&QComboBox::currentIndexChanged), this, &dlgProfilePreferences::setFontSize);

    connect(pushButton_black_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::setColorBlack2);
    connect(pushButton_Lblack_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::setColorLightBlack2);
    connect(pushButton_green_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::setColorGreen2);
    connect(pushButton_Lgreen_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::setColorLightGreen2);
    connect(pushButton_red_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::setColorRed2);
    connect(pushButton_Lred_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::setColorLightRed2);
    connect(pushButton_blue_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::setColorBlue2);
    connect(pushButton_Lblue_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::setColorLightBlue2);
    connect(pushButton_yellow_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::setColorYellow2);
    connect(pushButton_Lyellow_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::setColorLightYellow2);
    connect(pushButton_cyan_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::setColorCyan2);
    connect(pushButton_Lcyan_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::setColorLightCyan2);
    connect(pushButton_magenta_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::setColorMagenta2);
    connect(pushButton_Lmagenta_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::setColorLightMagenta2);
    connect(pushButton_white_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::setColorWhite2);
    connect(pushButton_Lwhite_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::setColorLightWhite2);

    connect(pushButton_foreground_color_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::setFgColor2);
    connect(pushButton_background_color_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::setBgColor2);

    connect(mEnableGMCP, &QAbstractButton::clicked, need_reconnect_for_data_protocol, &QWidget::show);
    connect(mEnableMSDP, &QAbstractButton::clicked, need_reconnect_for_data_protocol, &QWidget::show);
    connect(mEnableMSSP, &QAbstractButton::clicked, need_reconnect_for_data_protocol, &QWidget::show);
    connect(mEnableMSP, &QAbstractButton::clicked, need_reconnect_for_data_protocol, &QWidget::show);

    connect(mFORCE_MCCP_OFF, &QAbstractButton::clicked, need_reconnect_for_specialoption, &QWidget::show);
    connect(mFORCE_GA_OFF, &QAbstractButton::clicked, need_reconnect_for_specialoption, &QWidget::show);
    connect(mpMenu.data(), &QMenu::triggered, this, &dlgProfilePreferences::slot_chooseProfilesChanged);

    connect(pushButton_copyMap, &QAbstractButton::clicked, this, &dlgProfilePreferences::copyMap);
    connect(pushButton_loadMap, &QAbstractButton::clicked, this, &dlgProfilePreferences::loadMap);
    connect(pushButton_saveMap, &QAbstractButton::clicked, this, &dlgProfilePreferences::saveMap);
    connect(comboBox_encoding, qOverload<int>(&QComboBox::currentIndexChanged), this, &dlgProfilePreferences::slot_setEncoding);

    connect(pushButton_whereToLog, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setLogDir);
    connect(pushButton_resetLogDir, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_resetLogDir);
    connect(comboBox_logFileNameFormat, qOverload<int>(&QComboBox::currentIndexChanged), this, &dlgProfilePreferences::slot_logFileNameFormatChange);
    connect(mIsToLogInHtml, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_changeLogFileAsHtml);

    //Security tab


}

void dlgProfilePreferences::disconnectHostRelatedControls()
{
    // The "new" style connect(...) does not have the same range of overloaded
    // disconnect(...) counterparts - so we need to provide the "dummy"
    // arguments to get the wanted wild-card behaviour for them:

    disconnect(buttonDownloadMap, &QAbstractButton::clicked, nullptr, nullptr);

    disconnect(pushButton_foreground_color, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_background_color, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_command_line_foreground_color, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_command_line_background_color, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_command_foreground_color, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_command_background_color, &QAbstractButton::clicked, nullptr, nullptr);

    disconnect(pushButton_black, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_lBlack, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_red, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_lRed, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_green, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_lGreen, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_yellow, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_lYellow, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_blue, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_lBlue, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_magenta, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_lMagenta, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_cyan, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_lCyan, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_white, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_lWhite, &QAbstractButton::clicked, nullptr, nullptr);

    disconnect(pushButton_resetColors, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(reset_colors_button_2, &QAbstractButton::clicked, nullptr, nullptr);

    disconnect(fontComboBox, qOverload<const QFont&>(&QFontComboBox::currentFontChanged), nullptr, nullptr);
    disconnect(fontSize, qOverload<int>(&QComboBox::currentIndexChanged), nullptr, nullptr);

    disconnect(pushButton_black_2, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_Lblack_2, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_green_2, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_Lgreen_2, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_red_2, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_Lred_2, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_blue_2, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_Lblue_2, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_yellow_2, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_Lyellow_2, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_cyan_2, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_Lcyan_2, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_magenta_2, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_Lmagenta_2, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_white_2, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_Lwhite_2, &QAbstractButton::clicked, nullptr, nullptr);

    disconnect(pushButton_foreground_color_2, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_background_color_2, &QAbstractButton::clicked, nullptr, nullptr);

    disconnect(mEnableGMCP, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(mEnableMSSP, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(mEnableMSP, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(mEnableMSDP, &QAbstractButton::clicked, nullptr, nullptr);

    disconnect(mFORCE_MCCP_OFF, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(mFORCE_GA_OFF, &QAbstractButton::clicked, nullptr, nullptr);

    disconnect(mpMenu.data(), &QMenu::triggered, nullptr, nullptr);
    disconnect(pushButton_copyMap, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_loadMap, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_saveMap, &QAbstractButton::clicked, nullptr, nullptr);

    disconnect(comboBox_encoding, &QComboBox::currentTextChanged, nullptr, nullptr);
    disconnect(pushButton_whereToLog, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_resetLogDir, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(comboBox_logFileNameFormat, qOverload<int>(&QComboBox::currentIndexChanged), nullptr, nullptr);
    disconnect(mIsToLogInHtml, &QAbstractButton::clicked, nullptr, nullptr);

    widget_playerRoomStyle->hide();
    disconnect(comboBox_playerRoomStyle, qOverload<int>(&QComboBox::currentIndexChanged), nullptr, nullptr);
    disconnect(pushButton_playerRoomPrimaryColor, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_playerRoomSecondaryColor, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(spinBox_playerRoomOuterDiameter, qOverload<int>(&QSpinBox::valueChanged), nullptr, nullptr);
    disconnect(spinBox_playerRoomInnerDiameter, qOverload<int>(&QSpinBox::valueChanged), nullptr, nullptr);
}

void dlgProfilePreferences::clearHostDetails()
{
    code_editor_theme_selection_combobox->clear();
    script_preview_combobox->clear();
    edbeePreviewWidget->textDocument()->setText(QString());

    mFORCE_MXP_NEGOTIATION_OFF->setChecked(false);
    mMapperUseAntiAlias->setChecked(false);
    acceptServerGUI->setChecked(false);
    acceptServerMedia->setChecked(false);

    // Given that the IRC sub-system can handle there NOT being an active host
    // this may need revising
    ircHostName->clear();
    ircHostPort->clear();
    ircChannels->clear();
    ircNick->clear();

    dictList->clear();
    checkBox_spellCheck->setChecked(false);
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
    checkBox_enableTextAnalyzer->setChecked(false);
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
    mEnableMSSP->setChecked(false);
    mEnableMSP->setChecked(false);
    mEnableMSDP->setChecked(false);

    pushButton_chooseProfiles->setEnabled(false);
    pushButton_copyMap->setEnabled(false);
    if (mpMenu) {
        mpMenu->deleteLater();
        mpMenu = nullptr;
    }

    pushButton_chooseProfiles->setEnabled(false);

    label_mapFileActionResult->hide();

    hidePasswordMigrationLabel();

    doubleclick_ignore_lineedit->clear();

    comboBox_mapFileSaveFormatVersion->clear();
    comboBox_mapFileSaveFormatVersion->setEnabled(true);
    label_mapFileSaveFormatVersion->setEnabled(false);
    checkBox_showDefaultArea->setChecked(false);
    checkBox_showDefaultArea->hide();

    comboBox_encoding->clear();

    mSearchEngineMap.clear();
    search_engine_combobox->clear();

    checkBox_discordLuaAPI->setChecked(false);
    comboBox_discordLargeIconPrivacy->setCurrentIndex(0);
    comboBox_discordSmallIconPrivacy->setCurrentIndex(0);
    checkBox_discordServerAccessToDetail->setChecked(false);
    checkBox_discordServerAccessToState->setChecked(false);
    checkBox_discordServerAccessToPartyInfo->setChecked(false);
    checkBox_discordServerAccessToTimerInfo->setChecked(false);
    lineEdit_discordUserName->clear();
    lineEdit_discordUserDiscriminator->clear();

    groupBox_ssl_certificate->hide();
    notificationArea->hide();
    groupBox_proxy->setDisabled(true);

    // Remove the reference to the Host/profile in the title:
    setWindowTitle(tr("Profile preferences"));
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
    config->setUseTabChar(false); // when you press Enter for a newline, pad with spaces and not tabs
    config->setCaretBlinkRate(200);
    config->setIndentSize(2);
    config->setThemeName(pHost->mEditorTheme);
    config->setCaretWidth(1);
    config->setShowWhitespaceMode((mudlet::self()->mEditorTextOptions & QTextOption::ShowTabsAndSpaces)
                                  ? edbee::TextEditorConfig::ShowWhitespaces
                                  : edbee::TextEditorConfig::HideWhitespaces);
    config->setUseLineSeparator(mudlet::self()->mEditorTextOptions & QTextOption::ShowLineAndParagraphSeparators);
    config->setFont(pHost->getDisplayFont());
    config->setAutocompleteAutoShow(pHost->mEditorAutoComplete);
    config->endChanges();
    edbeePreviewWidget->textDocument()->setLanguageGrammar(edbee::Edbee::instance()->grammarManager()->detectGrammarWithFilename(QStringLiteral("Buck.lua")));
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

    checkBox_autocompleteLuaCode->setChecked(pHost->mEditorAutoComplete);

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
        setButtonColor(pushButton_foreground_color, pHost->mFgColor);
        setButtonColor(pushButton_background_color, pHost->mBgColor);
        setButtonColor(pushButton_command_line_foreground_color, pHost->mCommandLineFgColor);
        setButtonColor(pushButton_command_line_background_color, pHost->mCommandLineBgColor);
        setButtonColor(pushButton_command_foreground_color, pHost->mCommandFgColor);
        setButtonColor(pushButton_command_background_color, pHost->mCommandBgColor);
        setButtonColor(pushButton_black, pHost->mBlack);
        setButtonColor(pushButton_lBlack, pHost->mLightBlack);
        setButtonColor(pushButton_red, pHost->mRed);
        setButtonColor(pushButton_lRed, pHost->mLightRed);
        setButtonColor(pushButton_green, pHost->mGreen);
        setButtonColor(pushButton_lGreen, pHost->mLightGreen);
        setButtonColor(pushButton_blue, pHost->mBlue);
        setButtonColor(pushButton_lBlue, pHost->mLightBlue);
        setButtonColor(pushButton_yellow, pHost->mYellow);
        setButtonColor(pushButton_lYellow, pHost->mLightYellow);
        setButtonColor(pushButton_cyan, pHost->mCyan);
        setButtonColor(pushButton_lCyan, pHost->mLightCyan);
        setButtonColor(pushButton_magenta, pHost->mMagenta);
        setButtonColor(pushButton_lMagenta, pHost->mLightMagenta);
        setButtonColor(pushButton_white, pHost->mWhite);
        setButtonColor(pushButton_lWhite, pHost->mLightWhite);
    } else {
        pushButton_foreground_color->setStyleSheet(QString());
        pushButton_background_color->setStyleSheet(QString());
        pushButton_command_line_foreground_color->setStyleSheet(QString());
        pushButton_command_line_background_color->setStyleSheet(QString());
        pushButton_command_foreground_color->setStyleSheet(QString());
        pushButton_command_background_color->setStyleSheet(QString());
        pushButton_black->setStyleSheet(QString());
        pushButton_lBlack->setStyleSheet(QString());
        pushButton_red->setStyleSheet(QString());
        pushButton_lRed->setStyleSheet(QString());
        pushButton_green->setStyleSheet(QString());
        pushButton_lGreen->setStyleSheet(QString());
        pushButton_yellow->setStyleSheet(QString());
        pushButton_lYellow->setStyleSheet(QString());
        pushButton_blue->setStyleSheet(QString());
        pushButton_lBlue->setStyleSheet(QString());
        pushButton_magenta->setStyleSheet(QString());
        pushButton_lMagenta->setStyleSheet(QString());
        pushButton_cyan->setStyleSheet(QString());
        pushButton_lCyan->setStyleSheet(QString());
        pushButton_white->setStyleSheet(QString());
        pushButton_lWhite->setStyleSheet(QString());
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

void dlgProfilePreferences::setTab(QString tab)
{
    for (auto child : tabWidget->findChildren<QWidget*>()) {
        if (child->objectName().contains(tab, Qt::CaseInsensitive)) {
            tabWidget->setCurrentIndex(tabWidget->indexOf(child));
            return;
        }
    }
    tabWidget->setCurrentIndex(0);
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
    // If these get changed, ensure TBuffer::resetColors() is updated to match
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

    // Copy across the colors to the Lua "color_table"
    pHost->updateAnsi16ColorsInTable();
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
            // update the display properly when color selections change.
            mudlet::self()->mConsoleMap[pHost]->mUpperPane->updateScreenView();
            mudlet::self()->mConsoleMap[pHost]->mUpperPane->forceUpdate();
            if (!mudlet::self()->mConsoleMap[pHost]->mUpperPane->mIsTailMode) {
                // The upper pane having mIsTailMode true means lower pane is hidden
                mudlet::self()->mConsoleMap[pHost]->mLowerPane->updateScreenView();
                mudlet::self()->mConsoleMap[pHost]->mLowerPane->forceUpdate();
            }
        }

        if (b == pushButton_black || b == pushButton_lBlack
                || b == pushButton_red || b == pushButton_lRed
                || b == pushButton_green || b == pushButton_lGreen
                || b == pushButton_yellow || b == pushButton_lYellow
                || b == pushButton_blue || b == pushButton_lBlue
                || b == pushButton_magenta || b == pushButton_lMagenta
                || b == pushButton_cyan || b == pushButton_lCyan
                || b == pushButton_white || b == pushButton_lWhite) {

            pHost->updateAnsi16ColorsInTable();
        }

        // Also set a contrasting foreground color so text will always be
        // visible - if the button is disabled the colors will be somewhat
        // "greyed-out":
        setButtonColor(b, color);
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
    QFont newFont = fontComboBox->currentFont();
    newFont.setPointSize(mFontSize);

    if (pHost->getDisplayFont() == newFont) {
        return;
    }

    label_invalidFontError->hide();
    label_variableWidthFontWarning->hide();
    if (auto [validFont, errorMessage] = pHost->setDisplayFont(newFont); !validFont) {
        label_invalidFontError->show();
        return;
    } else if (!QFontInfo(newFont).fixedPitch()) {
        label_variableWidthFontWarning->show();
    }

#if defined(Q_OS_LINUX)
    // On Linux ensure that emojis are displayed in colour even if this font
    // doesn't support it:
    QFont::insertSubstitution(pHost->mDisplayFont.family(), QStringLiteral("Noto Color Emoji"));
#endif

    auto* mainConsole = mudlet::self()->mConsoleMap.value(pHost);
    if (!mainConsole) {
        return;
    }

    // update the display properly when font or size selections change.
    mainConsole->changeColors();
    mainConsole->mUpperPane->updateScreenView();
    mainConsole->mUpperPane->forceUpdate();
    mainConsole->mLowerPane->updateScreenView();
    mainConsole->mLowerPane->forceUpdate();
    mainConsole->refresh();

    auto config = edbeePreviewWidget->config();
    config->beginChanges();
    config->setFont(newFont);
    config->endChanges();
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
        setColor(pushButton_lBlack, pHost->mLightBlack);
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
        setColor(pushButton_lRed, pHost->mLightRed);
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
        setColor(pushButton_lGreen, pHost->mLightGreen);
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
        setColor(pushButton_lYellow, pHost->mLightYellow);
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
        setColor(pushButton_lBlue, pHost->mLightBlue);
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
        setColor(pushButton_lMagenta, pHost->mLightMagenta);
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
        setColor(pushButton_lCyan, pHost->mLightCyan);
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
        setColor(pushButton_lWhite, pHost->mLightWhite);
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
                              "Do not change extensions (in braces) as they are used programmatically"));
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
    QTimer::singleShot(10 * 1000, this, &dlgProfilePreferences::hideActionLabel);

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

    // Ensure the setting is already made as the saveMap(...) uses the set value
    bool showAuditErrors = mudlet::self()->showMapAuditErrors();
    mudlet::self()->setShowMapAuditErrors(checkBox_reportMapIssuesOnScreen->isChecked());

    if (pHost->mpConsole->saveMap(fileName, comboBox_mapFileSaveFormatVersion->currentData().toInt())) {
        label_mapFileActionResult->setText(tr("Saved map to %1.").arg(fileName));
    } else {
        label_mapFileActionResult->setText(tr("Could not save map to %1.").arg(fileName));
    }
    mudlet::self()->setShowMapAuditErrors(showAuditErrors);

    QTimer::singleShot(10 * 1000, this, &dlgProfilePreferences::hideActionLabel);
}

void dlgProfilePreferences::hideActionLabel()
{
    label_mapFileActionResult->hide();
}

void dlgProfilePreferences::hidePasswordMigrationLabel()
{
    label_password_migration_notification->hide();
}

void dlgProfilePreferences::slot_passwords_location_changed(int index)
{
    // index 0 = use secure storage, index 1 = use profile storage
    if (index == 0) {
        if (mudlet::self()->migratePasswordsToSecureStorage()) {
            label_password_migration_notification->setText(tr("Migrating passwords to secure storage..."));
            label_password_migration_notification->show();
            comboBox_store_passwords_in->setDisabled(true);
            hidePasswordMigrationLabelTimer->stop();
        }
    } else {
        if (mudlet::self()->migratePasswordsToProfileStorage()) {
            label_password_migration_notification->setText(tr("Migrating passwords to profiles..."));
            label_password_migration_notification->show();
            comboBox_store_passwords_in->setDisabled(true);
            hidePasswordMigrationLabelTimer->stop();
        }
    }
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
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    QList<Host*> activeHostsList{mudlet::self()->mConsoleMap.keys()};
    QSet<Host*> activeHosts{activeHostsList.begin(), activeHostsList.end()};
#else
    QSet<Host*> activeHosts{mudlet::self()->mConsoleMap.keys().toSet()};
#endif
    QMap<QString, Host*> activeOtherHostMap;
    QSetIterator<Host*> itActiveHost(activeHosts);
    while (itActiveHost.hasNext()) {
        Host* pOtherHost = itActiveHost.next();
        if (pOtherHost && (pOtherHost != pHost)) {
            const auto otherHostName = pOtherHost->getName();
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
        if (pHost->mpMap->retrieveMapFileStats(itOtherProfile.key(),
                                               &otherProfileFileUsed,
                                               &otherProfileVersion,
                                               &otherProfileCurrentRoomId,
                                               &otherProfileAreaCount,
                                               &otherProfileRoomCount)) {

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
        QTimer::singleShot(10 * 1000, this, &dlgProfilePreferences::hideActionLabel);
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
        QTimer::singleShot(10 * 1000, this, &dlgProfilePreferences::hideActionLabel);
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
            QTimer::singleShot(10 * 1000, this, &dlgProfilePreferences::hideActionLabel);
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
    QTimer::singleShot(10 * 1000, this, &dlgProfilePreferences::hideActionLabel);

    // CHECK: Race condition? We might be changing this whilst other profile
    // are accessing it...
    mudlet::self()->setShowMapAuditErrors(savedOldAuditErrorsToConsoleEnabledSetting);
}

void dlgProfilePreferences::slot_setLogDir()
{
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }

    /*
     * To show the files even though we are looking for a directory so that the
     * user can see the files that may get appended to depending on the format
     * selection, we need to use QFileDialog::DontUseNativeDialog because on
     * Windows the native one does not show files when selecting a directory.
     *
     * Also from Qt Docs:
     * "On Windows, the dialog will spin a blocking modal event loop that will
     * not dispatch any QTimers, and if parent is not 0 then it will position
     * the dialog just below the parent's title bar.
     *
     * Warning: Do not delete parent during the execution of the dialog. If you
     * want to do this, you should create the dialog yourself using one of the
     * QFileDialog constructors."
     *
     * That warning suggests *bad things* would happen if the "Save" button or
     * the widget title bar close button was pressed on the Profile Preferrences
     * dialog while the directory selector is open...!
     */
    // Seems to return "." when Cancel is hit:
    QString currentLogDir = QFileDialog::getExistingDirectory(
            this, tr("Where should Mudlet save log files?"), (mLogDirPath.isEmpty() ? lineEdit_logFileFolder->placeholderText() : mLogDirPath), QFileDialog::DontUseNativeDialog);

    if (!currentLogDir.isEmpty() && currentLogDir != nullptr) {
        // Disable pushButton_resetLogDir and clear
        // lineEdit_logFileFolder if the directory is set to the
        // default path
        if (currentLogDir == mudlet::getMudletPath(mudlet::profileReplayAndLogFilesPath, pHost->getName())) {
            // clear mLogDirPath, which sets the directory where logs are saved
            // to Mudlet's default log path.
            mLogDirPath.clear();
            lineEdit_logFileFolder->clear();
            pushButton_resetLogDir->setEnabled(false);
        } else {
            // set mLogDirPath to the selected directory
            mLogDirPath = currentLogDir;
            // If the directory is anything other than the default log
            // directory, set the text of lineEdit_logFileFolder to the selected
            // directory.
            lineEdit_logFileFolder->setText(mLogDirPath);
            // Set the cursor position to the end of the text.
            lineEdit_logFileFolder->setCursorPosition(lineEdit_logFileFolder->text().length());
            pushButton_resetLogDir->setEnabled(true);
        }
    }
    // If 'Cancel' is pushed, do nothing and keep mLogDirPath as its current value.
    return;
}

void dlgProfilePreferences::slot_resetLogDir()
{
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }

    mLogDirPath.clear();
    lineEdit_logFileFolder->clear();
    lineEdit_logFileFolder->setCursorPosition(lineEdit_logFileFolder->placeholderText().length());
    pushButton_resetLogDir->setEnabled(false);

    return;
}

void dlgProfilePreferences::slot_logFileNameFormatChange(const int index)
{
    Q_UNUSED(index);

    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }

    bool isShown = comboBox_logFileNameFormat->currentData().toString().isEmpty();
    lineEdit_logFileName->setVisible(isShown);
    label_logFileName->setVisible(isShown);
    label_logFileNameExtension->setVisible(isShown);
}

void dlgProfilePreferences::slot_save_and_exit()
{
    if (mpDialogMapGlyphUsage) {
        mpDialogMapGlyphUsage->close();
        mpDialogMapGlyphUsage = nullptr;
    }

    mudlet* pMudlet = mudlet::self();
    Host* pHost = mpHost;
    if (pHost) {
        if (dictList->isEnabled() && dictList->currentItem()) {
            pHost->setSpellDic(dictList->currentItem()->data(Qt::UserRole).toString());
        }

        pHost->mEnableSpellCheck = checkBox_spellCheck->isChecked();
        if (radioButton_userDictionary_common->isChecked()) {
            pHost->setUserDictionaryOptions(true, true);
        } else {
            pHost->setUserDictionaryOptions(true, false);
        }
        pHost->mWrapAt = wrap_at_spinBox->value();
        pHost->mWrapIndentCount = indent_wrapped_spinBox->value();
        pHost->mPrintCommand = show_sent_text_checkbox->isChecked();
        pHost->mAutoClearCommandLineAfterSend = auto_clear_input_line_checkbox->isChecked();
        pHost->mCommandSeparator = command_separator_lineedit->text();
        pHost->mAcceptServerGUI = acceptServerGUI->isChecked();
        pHost->mAcceptServerMedia = acceptServerMedia->isChecked();
        pHost->mUSE_IRE_DRIVER_BUGFIX = checkBox_USE_IRE_DRIVER_BUGFIX->isChecked();
        pHost->set_USE_IRE_DRIVER_BUGFIX(checkBox_USE_IRE_DRIVER_BUGFIX->isChecked());
        pHost->mEnableTextAnalyzer = checkBox_enableTextAnalyzer->isChecked();
        pHost->mUSE_FORCE_LF_AFTER_PROMPT = checkBox_mUSE_FORCE_LF_AFTER_PROMPT->isChecked();
        pHost->mUSE_UNIX_EOL = USE_UNIX_EOL->isChecked();
        pHost->getKeyUnit()->mRunAllKeyMatches = checkBox_runAllKeyBindings->isChecked();
        pHost->mFORCE_NO_COMPRESSION = mFORCE_MCCP_OFF->isChecked();
        pHost->mFORCE_GA_OFF = mFORCE_GA_OFF->isChecked();
        pHost->mFORCE_SAVE_ON_EXIT = mFORCE_SAVE_ON_EXIT->isChecked();
        pHost->mEnableGMCP = mEnableGMCP->isChecked();
        pHost->mEnableMSSP = mEnableMSSP->isChecked();
        pHost->mEnableMSP = mEnableMSP->isChecked();
        pHost->mEnableMSDP = mEnableMSDP->isChecked();
        pHost->mMapperUseAntiAlias = mMapperUseAntiAlias->isChecked();
        if (pHost->mpMap && pHost->mpMap->mpMapper) {
            pHost->mpMap->mpMapper->mp2dMap->mMapperUseAntiAlias = mMapperUseAntiAlias->isChecked();
            bool isAreaWidgetInNeedOfResetting = false;
            if ((!pHost->mpMap->mpMapper->getDefaultAreaShown()) && (checkBox_showDefaultArea->isChecked()) && (pHost->mpMap->mpMapper->mp2dMap->mAreaID == -1)) {
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
        pHost->mNoAntiAlias = !mNoAntiAlias->isChecked();
        pHost->mAlertOnNewData = mAlertOnNewData->isChecked();

        pHost->mUseProxy = groupBox_proxy->isChecked();
        pHost->mProxyAddress = lineEdit_proxyAddress->text();
        pHost->mProxyPort = lineEdit_proxyPort->text().toUInt();
        pHost->mProxyUsername = lineEdit_proxyUsername->text();
        pHost->mProxyPassword = lineEdit_proxyPassword->text();

        //tab security
        pHost->mSslTsl = groupBox_ssl->isChecked();
        pHost->mSslIgnoreExpired = checkBox_expired->isChecked();
        pHost->mSslIgnoreSelfSigned = checkBox_self_signed->isChecked();
        pHost->mSslIgnoreAll = checkBox_ignore_all->isChecked();


        if (pMudlet->mConsoleMap.contains(pHost)) {
            pMudlet->mConsoleMap[pHost]->changeColors();
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
            if (pMudlet->mpIrcClientMap.contains(pHost)) {
                pMudlet->mpIrcClientMap[pHost]->connection->setNickName(newIrcNick);
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
        if (restartIrcClient && pMudlet->mpIrcClientMap.contains(pHost)) {
            pMudlet->mpIrcClientMap[pHost]->ircRestart();
        }

        setDisplayFont();

        if (pMudlet->mConsoleMap.contains(pHost)) {
            int x = pMudlet->mConsoleMap[pHost]->width();
            int y = pMudlet->mConsoleMap[pHost]->height();
            QSize s = QSize(x, y);
            QResizeEvent event(s, s);
            QApplication::sendEvent(pMudlet->mConsoleMap[pHost], &event);
        }

        pHost->mEchoLuaErrors = checkBox_echoLuaErrors->isChecked();
        pHost->setWideAmbiguousEAsianGlyphs(checkBox_useWideAmbiguousEastAsianGlyphs->checkState());
        pHost->mEditorTheme = code_editor_theme_selection_combobox->currentText();
        pHost->mEditorThemeFile = code_editor_theme_selection_combobox->currentData().toString();
        pHost->mEditorAutoComplete = checkBox_autocompleteLuaCode->isChecked();
        if (pHost->mpEditorDialog) {
            pHost->mpEditorDialog->setThemeAndOtherSettings(pHost->mEditorTheme);
        }

        auto data = script_preview_combobox->currentData().value<QPair<QString, int>>();
        pHost->mThemePreviewItemID = data.second;
        pHost->mThemePreviewType = data.first;

        pHost->mSearchEngineName = search_engine_combobox->currentText();

        pHost->mTimerDebugOutputSuppressionInterval = timeEdit_timerDebugOutputMinimumInterval->time();

        auto hideSmallIcon = false, hideSmallIconText = false;
        if (comboBox_discordSmallIconPrivacy->currentIndex() == 0) {
            hideSmallIcon = false;
            hideSmallIconText = false;
        } else if (comboBox_discordSmallIconPrivacy->currentIndex() == 1) {
            hideSmallIcon = false;
            hideSmallIconText = true;
        } else {
            hideSmallIcon = true;
            hideSmallIconText = true;
        }

        auto hideLargeIcon = false, hideLargeIconText = false;
        if (comboBox_discordLargeIconPrivacy->currentIndex() == 0) {
            hideLargeIcon = false;
            hideLargeIconText = false;
        } else if (comboBox_discordLargeIconPrivacy->currentIndex() == 1) {
            hideLargeIcon = false;
            hideLargeIconText = true;
        } else {
            hideLargeIcon = true;
            hideLargeIconText = true;
        }

        pHost->mDiscordAccessFlags = static_cast<Host::DiscordOptionFlags>(
                                         (hideLargeIcon ? Host::DiscordNoOption : Host::DiscordSetLargeIcon)
                                         | (hideLargeIconText ? Host::DiscordNoOption : Host::DiscordSetLargeIconText)
                                         | (hideSmallIcon ? Host::DiscordNoOption : Host::DiscordSetSmallIcon)
                                         | (hideSmallIconText ? Host::DiscordNoOption : Host::DiscordSetSmallIconText)
                                         | (checkBox_discordServerAccessToDetail->isChecked() ? Host::DiscordNoOption : Host::DiscordSetDetail)
                                         | (checkBox_discordServerAccessToState->isChecked() ? Host::DiscordNoOption : Host::DiscordSetState)
                                         | (checkBox_discordServerAccessToPartyInfo->isChecked() ? Host::DiscordNoOption : Host::DiscordSetPartyInfo)
                                         | (checkBox_discordServerAccessToTimerInfo->isChecked() ? Host::DiscordNoOption : Host::DiscordSetTimeInfo)
                                         | (checkBox_discordLuaAPI->isChecked() ? Host::DiscordLuaAccessEnabled : Host::DiscordNoOption));

        pHost->mRequiredDiscordUserName = lineEdit_discordUserName->text().trimmed();
        if (lineEdit_discordUserDiscriminator->hasAcceptableInput()) {
            // The input mask specifies 4 digits [0-9]
            pHost->mRequiredDiscordUserDiscriminator = lineEdit_discordUserDiscriminator->text();
        } else {
            pHost->mRequiredDiscordUserDiscriminator.clear();
        }

        pHost->setHaveColorSpaceId(checkBox_expectCSpaceIdInColonLessMColorCode->isChecked());
        pHost->setMayRedefineColors(checkBox_allowServerToRedefineColors->isChecked());

        if (widget_playerRoomStyle->isVisible()) {
            // Although the controls have been interactively modifying the
            // TMap cached values for these, they were not being committed to
            // the master values in the Host instance - but now we should write
            // those - whilst we can get the first three (quint8) values
            // directly from controls on this form/dialogue, the last two
            // (QColors) are easiest to retrieve from the TMap instance as the
            // colours are not directly stored here (as for some styles they
            // show a partly "grey-ed out" colour as they are disabled for those
            // styles):
            pHost->setPlayerRoomStyleDetails(static_cast<quint8>(comboBox_playerRoomStyle->currentIndex()),
                                             static_cast<quint8>(spinBox_playerRoomOuterDiameter->value()),
                                             static_cast<quint8>(spinBox_playerRoomInnerDiameter->value()),
                                             pHost->mpMap->mPlayerRoomOuterColor,
                                             pHost->mpMap->mPlayerRoomInnerColor);
        }
    }

#if defined(INCLUDE_UPDATER)
    if (mudlet::scmIsReleaseVersion || mudlet::scmIsPublicTestVersion) {
        pMudlet->updater->setAutomaticUpdates(!checkbox_noAutomaticUpdates->isChecked());
    }
#endif

    pMudlet->setToolBarIconSize(MainIconSize->value());
    pMudlet->setEditorTreeWidgetIconSize(TEFolderIconSize->value());
    switch (comboBox_menuBarVisibility->currentIndex()) {
    case 0:
        pMudlet->setMenuBarVisibility(mudlet::visibleNever);
        break;
    case 1:
        pMudlet->setMenuBarVisibility(mudlet::visibleOnlyWithoutLoadedProfile);
        break;
    default:
        pMudlet->setMenuBarVisibility(mudlet::visibleAlways);
    }
    switch (comboBox_toolBarVisibility->currentIndex()) {
    case 0:
        pMudlet->setToolBarVisibility(mudlet::visibleNever);
        break;
    case 1:
        pMudlet->setToolBarVisibility(mudlet::visibleOnlyWithoutLoadedProfile);
        break;
    default:
        pMudlet->setToolBarVisibility(mudlet::visibleAlways);
    }

    pMudlet->setEnableFullScreenMode(checkBox_USE_SMALL_SCREEN->isChecked());
    pMudlet->setEditorTextoptions(checkBox_showSpacesAndTabs->isChecked(), checkBox_showLineFeedsAndParagraphs->isChecked());
    pMudlet->setShowMapAuditErrors(checkBox_reportMapIssuesOnScreen->isChecked());
    pMudlet->setShowIconsOnMenu(checkBox_showIconsOnMenus->checkState());

    mudlet::self()->mDiscord.UpdatePresence();

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

void dlgProfilePreferences::slot_setEncoding(const int newEncodingIndex)
{
    Host* pHost = mpHost;
    if (pHost) {
        pHost->mTelnet.setEncoding(comboBox_encoding->itemData(newEncodingIndex).toByteArray());

        if (checkBox_useWideAmbiguousEastAsianGlyphs->checkState() == Qt::PartiallyChecked) {
            // We are linking the Server encoding to this setting currently
            // - eventually it would move to the locale/language control when it
            // goes in, but we only need to change the setting for this if it is
            // set to be automatic changed as necessary:

            pHost->setWideAmbiguousEAsianGlyphs(Qt::PartiallyChecked);
        }
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
    std::list<TTrigger*>* childTriggers = pTriggerParent->getChildrenList();
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
    std::list<TAlias*>* childrenList = pAliasParent->getChildrenList();
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
    std::list<TTimer*>* childrenList = pTimerParent->getChildrenList();
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
    std::list<TKey*>* childrenList = pKeyParent->getChildrenList();
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
    std::list<TScript*>* childrenList = pScriptParent->getChildrenList();
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
    std::list<TAction*>* childrenList = pActionParent->getChildrenList();
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
    // bail out if this is not the editor tab - or if the Host has gone away
    Host* pHost = mpHost;
    if (tabIndex != 3 || !pHost) {
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
    pHost->updateProxySettings(manager);
    QNetworkReply* getReply = manager->get(request);

    connect(getReply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), this, [=](QNetworkReply::NetworkError) {
        theme_download_label->setText(tr("Could not update themes: %1").arg(getReply->errorString()));
        QTimer::singleShot(5000, theme_download_label, [label = theme_download_label] {
            label->hide();
            label->setText(tr("Updating themes from colorsublime.github.io..."));
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
                        auto future = QtConcurrent::run(mudlet::unzip, tempThemesArchive->fileName(), mudlet::getMudletPath(mudlet::mainDataItemPath, QStringLiteral("edbee/")), temporaryDir.path());
                        auto watcher = new QFutureWatcher<bool>;
                        QObject::connect(watcher, &QFutureWatcher<bool>::finished, this, [=]() {
                            if (future.result()) {
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
        for (auto theme : qAsConst(unsortedThemes)) {
            QString themeText = theme.toObject()["Title"].toString();
            QString themeFileName = theme.toObject()["FileName"].toString();

            if (!themeText.isEmpty() && !themeFileName.isEmpty()) {
                sortedThemes << std::make_pair(themeText, themeFileName);
            }
        }
    }
    sortedThemes << std::make_pair(QStringLiteral("Mudlet"), QStringLiteral("Mudlet.tmTheme"));

    std::sort(sortedThemes.begin(), sortedThemes.end(), [](const auto& a, const auto& b) { return QString::localeAwareCompare(a.first, b.first) < 0; });

    // temporary disable painting and event updates while we refill the list
    code_editor_theme_selection_combobox->setUpdatesEnabled(false);
    code_editor_theme_selection_combobox->blockSignals(true);

    auto currentSelection = code_editor_theme_selection_combobox->currentText();
    code_editor_theme_selection_combobox->clear();
    for (auto key : qAsConst(sortedThemes)) {
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
    config->setShowWhitespaceMode(state
                                  ? edbee::TextEditorConfig::ShowWhitespaces
                                  : edbee::TextEditorConfig::HideWhitespaces);
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

/*
 * This is to deal particularly with the case where the preferences dialog is
 * opened without a host instance being around - and then the user starts up
 * a profile and one gets created.
 * In that situation we detect the signal that the mudlet class sends out
 * when a host is created and wire it up into the controls that until then
 * have been disabled/greyed-out.
 */
void dlgProfilePreferences::slot_handleHostAddition(Host* pHost, const quint8 count)
{
    if (!mpHost && pHost && count < 2) {
        // We have not been constructed with a valid Host pointer,
        // AND a real Host instance has just been created
        // AND there is only one Host instance around.
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
    anyFont.setStyleStrategy(static_cast<QFont::StyleStrategy>(mpHost->mpMap->mMapSymbolFont.styleStrategy() & ~(QFont::NoFontMerging)));

    int row = -1;
    QHashIterator<QString, QSet<int>> itUsedSymbol(roomSymbolsHash);
    while (itUsedSymbol.hasNext()) {
        itUsedSymbol.next();
        QString symbol = itUsedSymbol.key();
        QList<int> roomsWithSymbol = itUsedSymbol.value().toList();
        if (roomsWithSymbol.count() > 1) {
            std::sort(roomsWithSymbol.begin(), roomsWithSymbol.end());
        }
        auto * pSymbolInFont = new QTableWidgetItem();
        pSymbolInFont->setTextAlignment(Qt::AlignCenter);
        pSymbolInFont->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                  .arg(tr("<p>The room symbol will appear like this if only symbols (glyphs) from the specfic font are used.</p>")));
        pSymbolInFont->setFont(selectedFont);

        auto * pSymbolAnyFont = new QTableWidgetItem();
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
        bool isSingleFontUsable = true;
        bool isAllFontUsable = true;
        QStringList codePointsString;
        for (uint i = 0, total = pCodePoints.size(); i < total; ++i) {
            codePointsString << QStringLiteral("U+%1").arg(pCodePoints.at(i), 4, 16, QChar('0')).toUpper();
            if (!SymbolAnyFontMetrics.inFontUcs4(pCodePoints.at(i))) {
                isAllFontUsable = false;
                // By definition if all the fonts together cannot render the
                // glyph then the specified one cannot either
                isSingleFontUsable = false;
            } else if (!SymbolInFontMetrics.inFontUcs4(pCodePoints.at(i))) {
                isSingleFontUsable = false;
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

        auto * pDummyButton = new QToolButton();
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

void dlgProfilePreferences::generateDiscordTooltips()
{
    if (!mpHost) {
        return;
    }

    auto* mudlet = mudlet::self();

    auto detail = mudlet->mDiscord.getDetailText(mpHost);
    if (!detail.isEmpty()) {
        detail = QStringLiteral("<br/>(\"%1\")").arg(detail);
    }

    auto state = mudlet->mDiscord.getStateText(mpHost);
    if (!state.isEmpty()) {
        state = QStringLiteral("<br/>(\"%1\")").arg(state);
    }

    auto setToolTip = [=](QWidget* widget, const QString& highlight) {
        QString tooltip = QStringLiteral(R"(
  <style type="text/css">
    .tg  {border-collapse:collapse;border-spacing:0;}
    .tg td{font-size:12px;padding:10px 5px;border-style:solid;border-width:1px;overflow:hidden;word-break:normal;border-color:black;}
    .tg th{font-size:12px;font-weight:normal;padding:10px 5px;border-style:solid;border-width:1px;overflow:hidden;word-break:normal;border-color:black;}
    .tg .tg-jn9l{background-color:#2f3135;border-color:#2f3135;text-align:left;vertical-align:top;}
    .detail {color: #C79698; background-color: #66373A;}
    .state {color: #CBB38B; background-color: #69522E;}
    .party-size {color: #80B5CC; background-color: #24556B;}
    .party-max {color: #94B7AA; background-color: #35564A;}
    .time {color: #AB93B7; background-color: #4D3659;}

    #%1 {font-size:17px; font-weight:bold;}
  </style>
  <table class="tg">
    <tr>
      <td colspan="2">
      <img src=":/icons/discord-rich-presence.png"/>
      </td>
    </tr>
    <tr>
      <td class="tg-jn9l">
        <img src=":/icons/discord-rich-presence-large-icon.png"/>
        <p style="color: #989A9F;" id="large-icon">%2</p>
      </td>
      <td class="tg-jn9l"><p class="detail" id="detail">%3 %4</p></td>
    </tr>
    <tr>
      <td class="tg-jn9l">
        <img src=":/icons/discord-rich-presence-small-icon.png"/>
        <p style="color: #989A9F;" id="small-icon">%5</p>
      </td>
      <td class="tg-jn9l"><p class="state" id="state">%6 %7</p></td>
    </tr>
    <tr>
      <td class="tg-jn9l"><p class="party-size" id="party">%8</p></td>
      <td class="tg-jn9l"><p class="party-max" id="party">%9</p>
    </tr>
    <tr>
      <td class="tg-jn9l" colspan="2"><p class="time" id="time">%10</p></td>
    </tr>
  </table>
      )")
                                  .arg(highlight,
                                       tr("Large icon", "Discord Rich Presence large icon"),
                                       tr("Detail", "Discord Rich Presence detail"),
                                       detail,
                                       tr("Small icon", "Discord Rich Presence small icon"),
                                       tr("State", "Discord Rich Presence state"),
                                       state,
                                       tr("Party size", "Discord Rich Presence party size"),
                                       tr("Party max", "Discord Rich Presence maximum party size"))
                                  .arg(tr("Time", "Discord Rich Presence time until or time elapsed"));
        widget->setToolTip(tooltip);
    };

    setToolTip(checkBox_discordServerAccessToDetail, QStringLiteral("detail"));
    setToolTip(checkBox_discordServerAccessToState, QStringLiteral("state"));
    setToolTip(checkBox_discordServerAccessToPartyInfo, QStringLiteral("party"));
    setToolTip(checkBox_discordServerAccessToTimerInfo, QStringLiteral("time"));
    setToolTip(comboBox_discordLargeIconPrivacy, QStringLiteral("large-icon"));
    setToolTip(comboBox_discordSmallIconPrivacy, QStringLiteral("small-icon"));
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
void dlgProfilePreferences::slot_changeShowMenuBar(int newIndex)
{
    if (!newIndex && !comboBox_toolBarVisibility->currentIndex()) {
        // This control has been set to the "Never" setting but so is the other
        // control - so force it back to the "Only if no profile one
        comboBox_menuBarVisibility->setCurrentIndex(1);
    }
}

void dlgProfilePreferences::slot_changeShowToolBar(int newIndex)
{
    if (!newIndex && !comboBox_menuBarVisibility->currentIndex()) {
        // This control has been set to the "Never" setting but so is the other
        // control - so force it back to the "Only if no profile one
        comboBox_toolBarVisibility->setCurrentIndex(1);
    }
}

void dlgProfilePreferences::slot_changeLogFileAsHtml(const bool isHtml)
{
    if (isHtml) {
        comboBox_logFileNameFormat->setItemText(comboBox_logFileNameFormat->findData(QStringLiteral("yyyy-MM-dd#HH-mm-ss")), tr("yyyy-MM-dd#HH-mm-ss (e.g., 1970-01-01#00-00-00.html)"));
        comboBox_logFileNameFormat->setItemText(comboBox_logFileNameFormat->findData(QStringLiteral("yyyy-MM-ddTHH-mm-ss")), tr("yyyy-MM-ddTHH-mm-ss (e.g., 1970-01-01T00-00-00.html)"));
        comboBox_logFileNameFormat->setItemText(comboBox_logFileNameFormat->findData(QStringLiteral("yyyy-MM-dd")), tr("yyyy-MM-dd (concatenate daily logs in, e.g. 1970-01-01.html)"));
        comboBox_logFileNameFormat->setItemText(comboBox_logFileNameFormat->findData(QStringLiteral("yyyy-MM")), tr("yyyy-MM (concatenate month logs in, e.g. 1970-01.html)"));
        label_logFileNameExtension->setText(QStringLiteral(".html"));
    } else {
        comboBox_logFileNameFormat->setItemText(comboBox_logFileNameFormat->findData(QStringLiteral("yyyy-MM-dd#HH-mm-ss")), tr("yyyy-MM-dd#HH-mm-ss (e.g., 1970-01-01#00-00-00.txt)"));
        comboBox_logFileNameFormat->setItemText(comboBox_logFileNameFormat->findData(QStringLiteral("yyyy-MM-ddTHH-mm-ss")), tr("yyyy-MM-ddTHH-mm-ss (e.g., 1970-01-01T00-00-00.txt)"));
        comboBox_logFileNameFormat->setItemText(comboBox_logFileNameFormat->findData(QStringLiteral("yyyy-MM-dd")), tr("yyyy-MM-dd (concatenate daily logs in, e.g. 1970-01-01.txt)"));
        comboBox_logFileNameFormat->setItemText(comboBox_logFileNameFormat->findData(QStringLiteral("yyyy-MM")), tr("yyyy-MM (concatenate month logs in, e.g. 1970-01.txt)"));
        label_logFileNameExtension->setText(QStringLiteral(".txt"));
    }
}

void dlgProfilePreferences::setButtonColor(QPushButton* button, const QColor& color)
{
    if (color.isValid()) {
        if (button->isEnabled()) {
            if (button == pushButton_playerRoomPrimaryColor || button == pushButton_playerRoomSecondaryColor) {

                // These two buttons show a color that may have transparency; so,
                // instead of colouring the background, we include a generated
                // black/white checkerboard pattern overlaid with the colour which
                // when its alpha is not a 100% opaque will (partly) show the
                // checkerboard.

                // Ensure the icon has a 3:1 aspect ratio:
                if (auto iconWidth{button->iconSize().width()}, iconHeight{button->iconSize().height()}; iconWidth != iconHeight * 3) {
                    button->setIconSize(QSize(iconHeight * 3, iconHeight));
                }

                // Create a black/white checker background and overlay
                QPixmap labelBackground(1 + (button->iconSize().height() * 3), 1 + (button->iconSize().height()));
                labelBackground.fill(Qt::black);
                QPainter painter(&labelBackground);
                painter.drawImage(QRect(0, 0, labelBackground.width(), labelBackground.height()),
                                  QImage(QStringLiteral(":/icons/black_white_transparent_check_1x3_ratio.png"))
                                          .scaled(labelBackground.width(), labelBackground.height(), Qt::KeepAspectRatioByExpanding));
                painter.fillRect(0, 0, labelBackground.width(), labelBackground.height(), color);
                painter.end();
                button->setIcon(QIcon(labelBackground));
            } else {
                button->setStyleSheet(QStringLiteral("QPushButton {color: %1; background-color: %2; }")
                                              .arg(color.lightness() > 127 ? QLatin1String("black") : QLatin1String("white"),
                                                   color.name()));
            }
            return;
        }

        QColor disabledColor = QColor::fromHsl(color.hslHue(), color.hslSaturation()/4, color.lightness(), color.alpha());
        if (button == pushButton_playerRoomPrimaryColor || button == pushButton_playerRoomSecondaryColor) {

            // These two buttons show a color that may have transparency; so,
            // instead of colouring the background, we include a generated
            // black/white checkerboard pattern overlaid with the colour which
            // when its alpha is not a 100% opaque will (partly) show the
            // checkerboard.

            // Ensure the icon has a 3:1 aspect ratio:
            if (auto iconWidth{button->iconSize().width()}, iconHeight{button->iconSize().height()}; iconWidth != iconHeight * 3) {
                button->setIconSize(QSize(iconHeight * 3, iconHeight));
            }

            QPixmap iconBackground(1 + (button->iconSize().height() * 3), 1 + (button->iconSize().height()));
            iconBackground.fill(Qt::black);
            QPainter painter(&iconBackground);
            painter.drawImage(QRect(0, 0, iconBackground.width(), iconBackground.height()),
                              QImage(QStringLiteral(":/icons/black_white_transparent_check_1x3_ratio.png"))
                                      .scaled(iconBackground.width(), iconBackground.height(), Qt::KeepAspectRatioByExpanding));
            painter.fillRect(0, 0, iconBackground.width(), iconBackground.height(), disabledColor);
            painter.end();
            // Because the button is disabled we have to explictly force our
            // icon to be used for that state otherwise the built-in icon engine
            // will assume our image is for the normal state and grey it out
            // completely by automagic means instead of making use of the
            // partial (desaturating) effect that we want to use:
            QIcon icon;
            icon.addPixmap(iconBackground, QIcon::Disabled, QIcon::Off);
            button->setIcon(icon);
        } else {
            button->setStyleSheet(QStringLiteral("QPushButton {color: %1; background-color: %2; }")
                              .arg(QLatin1String("darkGray"), disabledColor.name()));
        }
        return;
    }

    button->setIcon(QIcon());
    button->setStyleSheet(QString());
}

// These next eight slots are so that if there are multiple profile preferences
// opened for different Profiles then common (application wide) settings changed
// in one of them is immediately updated in the others (so they do not get out
// of sync):
void dlgProfilePreferences::slot_changeEnableFullScreenMode(const bool state)
{
    if (checkBox_USE_SMALL_SCREEN->isChecked() != state) {
        checkBox_USE_SMALL_SCREEN->setChecked(state);
    }
}

// Connected to mudlet::signal_editorTextOptionsChanged which is emitted when
// (void) mudlet::setEditorTextoptions(...) is called from this or another
// instance:
void dlgProfilePreferences::slot_changeEditorTextOptions(const QTextOption::Flags state)
{
    if (checkBox_showSpacesAndTabs->isChecked() != (state & QTextOption::ShowTabsAndSpaces)) {
        // Changing the state of the checkbox with setChecked() does NOT fire
        // the slot_changeShowSpacesAndTabs() because that is connected to the
        // clicked() rather than the toggled() signal:
        checkBox_showSpacesAndTabs->setChecked(state & QTextOption::ShowTabsAndSpaces);
        // So we need to call the slot ourselves:
        slot_changeShowSpacesAndTabs(state & QTextOption::ShowTabsAndSpaces);
    }

    if (checkBox_showLineFeedsAndParagraphs->isChecked() != (state & QTextOption::ShowLineAndParagraphSeparators)) {
        checkBox_showLineFeedsAndParagraphs->setChecked(state & QTextOption::ShowLineAndParagraphSeparators);
        slot_changeShowLineFeedsAndParagraphs(state & QTextOption::ShowLineAndParagraphSeparators);
    }
}

void dlgProfilePreferences::slot_changeShowMapAuditErrors(const bool state)
{
    if (checkBox_reportMapIssuesOnScreen->isChecked() != state) {
        checkBox_reportMapIssuesOnScreen->setChecked(state);
    }
}

// We do not use the QSpinBox::valueChanged() signal and it is only emitted if
// the new value is different - so there is no need to worry about if we are or
// are not changing the value in the next two methods:
void dlgProfilePreferences::slot_setToolBarIconSize(const int s)
{
    MainIconSize->setValue(s);
}

void dlgProfilePreferences::slot_setTreeWidgetIconSize(const int s)
{
    TEFolderIconSize->setValue(s);
}

void dlgProfilePreferences::slot_changeAutomaticUpdates(const bool state)
{
    if (checkbox_noAutomaticUpdates->isChecked() != state) {
        checkbox_noAutomaticUpdates->setChecked(state);
    }
}

void dlgProfilePreferences::slot_changeMenuBarVisibility(const mudlet::controlsVisibility state)
{
    switch (state) {
    case mudlet::visibleNever:
        if (comboBox_menuBarVisibility->currentIndex() != 0) {
            comboBox_menuBarVisibility->setCurrentIndex(0);
        }
        break;
    case mudlet::visibleOnlyWithoutLoadedProfile:
        if (comboBox_menuBarVisibility->currentIndex() != 1) {
            comboBox_menuBarVisibility->setCurrentIndex(1);
        }
        break;
    default:
        if (comboBox_menuBarVisibility->currentIndex() != 2) {
            comboBox_menuBarVisibility->setCurrentIndex(2);
        }
    }
}

void dlgProfilePreferences::slot_changeToolBarVisibility(const mudlet::controlsVisibility state)
{
    switch (state) {
    case mudlet::visibleNever:
        if (comboBox_toolBarVisibility->currentIndex() != 0) {
            comboBox_toolBarVisibility->setCurrentIndex(0);
        }
        break;
    case mudlet::visibleOnlyWithoutLoadedProfile:
        if (comboBox_toolBarVisibility->currentIndex() != 1) {
            comboBox_toolBarVisibility->setCurrentIndex(1);
        }
        break;
    default:
        if (comboBox_toolBarVisibility->currentIndex() != 2) {
            comboBox_toolBarVisibility->setCurrentIndex(2);
        }
    }
}

void dlgProfilePreferences::slot_changeShowIconsOnMenus(const Qt::CheckState state)
{
    if (checkBox_showIconsOnMenus->checkState() != state) {
        checkBox_showIconsOnMenus->setCheckState(state);
    }
}

// This slot is called when the QComboBox for the locale/language in this dialog
// is changed by the user.
void dlgProfilePreferences::slot_changeGuiLanguage(const QString& language)
{
    Q_UNUSED(language);

    auto languageCode = comboBox_guiLanguage->currentData().toString();
    mudlet::self()->setInterfaceLanguage(languageCode);
    label_languageChangeWarning->show();
}

// This slot is called when the mudlet singleton tells everything that the
// locale/language selection has been changed (new translators installed)
// It probably came about because the control for it on THIS dialog was changed
// but it need not - the most obvious example would be if multi-playing and
// the preferences were open for more than one profile and the control was
// changed in another profile's preferences.
void dlgProfilePreferences::slot_guiLanguageChanged(const QString& language)
{
    // First ensure our QComboBox is set to the given value:
    if (comboBox_guiLanguage->currentData().toString() != language) {
        // Ah, it wasn't us who changed it - so we must adopt the new value
        // but not signal anything to prevent endless loops:
        comboBox_guiLanguage->blockSignals(true);
        comboBox_guiLanguage->setCurrentIndex(comboBox_guiLanguage->findData(language));
        comboBox_guiLanguage->blockSignals(false);
    }

    // Now change the displayed texts that are translated - importantly this
    // is done so that the message that says "restart Mudlet to finish changing
    // the language" is shown in the newly selected langauge - on the basis that
    // it is the one the user understands rather than the currently used one.
    retranslateUi(this);

    // Re identify which Profile we are showing the settings for (otherwise if
    // multiple profiles have this dialog open they revert to a plain
    // "Profile preferences" dialog title except that duplicates get a " <#>"
    // suffix to the title to tell them apart which is not good for telling
    // which profile is represented by each dialog when we were previously
    // showing the profile name as well):
    if (mpHost) {
        setWindowTitle(tr("Profile preferences - %1").arg(mpHost->getName()));
    }

    // If we wanted to support changing the locale/language without having to
    // restart then the above: retranslateUi(...) + regenerate texts that are
    // assembled after the class instance was created {i.e. outside of the
    // setupUi(...) call in the constructor} would be needed in every class with
    // persistent UI texts - this is not trivial and has been deemed NWIH...!
}

void dlgProfilePreferences::slot_changePlayerRoomStyle(const int index)
{
    Host* pHost = mpHost;
    if (!pHost || !pHost->mpMap) {
        return;
    }

    int style = index;
    switch (index) {
    case 1: // Red ring
        pushButton_playerRoomPrimaryColor->setEnabled(false);
        pushButton_playerRoomSecondaryColor->setEnabled(false);
        spinBox_playerRoomInnerDiameter->setEnabled(true);
        break;

    case 2: // Blue-yellow ring
        pushButton_playerRoomPrimaryColor->setEnabled(false);
        pushButton_playerRoomSecondaryColor->setEnabled(false);
        spinBox_playerRoomInnerDiameter->setEnabled(true);
        break;

    case 3: // Custom ring
        pushButton_playerRoomPrimaryColor->setEnabled(true);
        pushButton_playerRoomSecondaryColor->setEnabled(true);
        spinBox_playerRoomInnerDiameter->setEnabled(true);
        break;

    default:
        style = 0;
        [[fallthrough]];
    case 0: // "Original"
        pushButton_playerRoomPrimaryColor->setEnabled(false);
        pushButton_playerRoomSecondaryColor->setEnabled(false);
        spinBox_playerRoomInnerDiameter->setEnabled(false);
    }
    setButtonColor(pushButton_playerRoomPrimaryColor, pHost->mpMap->mPlayerRoomOuterColor);
    setButtonColor(pushButton_playerRoomSecondaryColor, pHost->mpMap->mPlayerRoomInnerColor);
    pHost->mpMap->mPlayerRoomStyle = static_cast<quint8>(style);
    if (!pHost->mpMap->mpMapper || !pHost->mpMap->mpMapper->mp2dMap) {
        return;
    }
    pHost->mpMap->mpMapper->mp2dMap->setPlayerRoomStyle(style);
    // And update the displayed map:
    pHost->mpMap->mpMapper->mp2dMap->update();
}

void dlgProfilePreferences::slot_setPlayerRoomPrimaryColor()
{
    Host* pHost = mpHost;
    if (!pHost || !mpHost->mpMap || !mpHost->mpMap->mpMapper || !mpHost->mpMap->mpMapper->mp2dMap) {
        return;
    }

    setPlayerRoomColor(pushButton_playerRoomPrimaryColor, mpHost->mpMap->mPlayerRoomOuterColor);
    if (comboBox_playerRoomStyle->currentIndex() != 3) {
        return;
    }

    // The current setting IS for the custom color - so use it straight away:
    mpHost->mpMap->mpMapper->mp2dMap->setPlayerRoomStyle(3);
    // And update the displayed map:
    mpHost->mpMap->mpMapper->mp2dMap->update();
}

void dlgProfilePreferences::slot_setPlayerRoomSecondaryColor()
{
    Host* pHost = mpHost;
    if (!pHost || !mpHost->mpMap || !mpHost->mpMap->mpMapper || !mpHost->mpMap->mpMapper->mp2dMap) {
        return;
    }

    setPlayerRoomColor(pushButton_playerRoomSecondaryColor, mpHost->mpMap->mPlayerRoomInnerColor);
    if (comboBox_playerRoomStyle->currentIndex() != 3) {
        return;
    }

    // The current setting IS for the custom color - so use it straight away:
    mpHost->mpMap->mpMapper->mp2dMap->setPlayerRoomStyle(3);
    // And update the displayed map:
    mpHost->mpMap->mpMapper->mp2dMap->update();
}

void dlgProfilePreferences::slot_setPlayerRoomOuterDiameter(const int value)
{
    Host* pHost = mpHost;
    if (!pHost || !mpHost->mpMap || !mpHost->mpMap->mpMapper || !mpHost->mpMap->mpMapper->mp2dMap) {
        return;
    }

    if (value < 256 && mpHost->mpMap->mPlayerRoomOuterDiameterPercentage != value) {
        mpHost->mpMap->mPlayerRoomOuterDiameterPercentage = static_cast<quint8>(value);
        mpHost->mPlayerRoomOuterDiameterPercentage = static_cast<quint8>(value);
        // And update the displayed map:
        mpHost->mpMap->mpMapper->mp2dMap->update();
    }
}

void dlgProfilePreferences::slot_setPlayerRoomInnerDiameter(const int value)
{
    Host* pHost = mpHost;
    if (!pHost || !mpHost->mpMap || !mpHost->mpMap->mpMapper || !mpHost->mpMap->mpMapper->mp2dMap) {
        return;
    }

    if (value < 256 && mpHost->mpMap->mPlayerRoomInnerDiameterPercentage != value) {
        mpHost->mpMap->mPlayerRoomInnerDiameterPercentage = static_cast<quint8>(value);
        mpHost->mPlayerRoomInnerDiameterPercentage = static_cast<quint8>(value);
        // Redefine the QGradientStops
        mpHost->mpMap->mpMapper->mp2dMap->setPlayerRoomStyle(qBound(0, comboBox_playerRoomStyle->currentIndex(), 3));
        // And update the displayed map:
        mpHost->mpMap->mpMapper->mp2dMap->update();
    }
}

void dlgProfilePreferences::setPlayerRoomColor(QPushButton* b, QColor& c)
{
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }

    auto color = QColorDialog::getColor(c, this, (b == pushButton_playerRoomPrimaryColor
                                                          ? tr("Set outer color of player room mark.")
                                                          : tr("Set inner color of player room mark.")),
                                        QColorDialog::ShowAlphaChannel);
    if (color.isValid()) {
        c = color;

        // Also sets a contrasting foreground color so text will always be
        // visible and adjusts the saturation of a disabled button:
        setButtonColor(b, color);
    }
}
