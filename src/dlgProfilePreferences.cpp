/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014, 2016-2018, 2020-2023, 2025-2026 by Stephen Lyons  *
 *                                               - slysven@virginmedia.com *
 *   Copyright (C) 2016 by Ian Adkins - ieadkins@gmail.com                 *
 *   Copyright (C) 2025 by Lecker Kebap - Leris@mudlet.org                 *
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

#include "CredentialManager.h"
#include "GMCPAuthenticator.h"
#include "Host.h"
#include "TAction.h"
#include "TAlias.h"
#include "TConsole.h"
#include "TKey.h"
#include "TMainConsole.h"
#include "TMap.h"
#include "TKeySequenceEdit.h"
#include "TMedia.h"
#include "TRoomDB.h"
#include "TScript.h"
#include "TTextEdit.h"
#include "TTimer.h"
#include "TTrigger.h"
#include "dlgIRC.h"
#include "dlgMapper.h"
#include "dlgTriggerEditor.h"
#include "edbee/views/texteditorscrollarea.h"
#include "MMCP.h"
#include "utils.h"

#include <chrono>
#include <QtConcurrentRun>
#include <QAbstractScrollArea>
#include <QAbstractSpinBox>
#include <QAccessible>
#include <QApplication>
#include <QCloseEvent>
#include <QColorDialog>
#include <QDesktopServices>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFontDialog>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QMessageBox>
#include <QNetworkDiskCache>
#include <QPainter>
#include <QPointer>
#include <QSettings>
#include <QStandardPaths>
#include <QStandardItemModel>
#include <QString>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QToolBar>
#include <QtMath>
#include <QUiLoader>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QListWidget>
#include <QScrollArea>
#include <QScrollBar>
#include <QShortcut>
#include <QStackedWidget>
#include <QStyle>
#include <QToolButton>
#include <QVariantAnimation>
#include "../3rdparty/kdtoolbox/singleshot_connect/singleshot_connect.h"

using namespace std::chrono_literals;

// Firefox caps its settings content at a reading width and lets the whitespace
// absorb a wide window instead of stretching the controls across it:
static constexpr int scmContentColumnWidth = 640;

// The sidebar's fixed width and the padding its layout puts around the category
// list, which together give the width of a category item - needed as a number
// because the selected item's accent bar is a gradient stop, and a gradient
// stop is a fraction of the item rather than a length.
static constexpr int scmSidebarWidth = 232;
static constexpr int scmSidebarPadding = 12;
static constexpr int scmSidebarAccentBarWidth = 3;
// The check indicator a checkable card draws in its title, and how far to the
// right of the frame edge that leaves the title itself - measured, because the
// second follows from the first through the style rather than by arithmetic
static constexpr int scmCardIndicatorSize = 13;
static constexpr int scmCardTitleInset = 21;

// A QDoubleSpinBox rounds whatever it is given to the number of decimals it
// displays, so it holds no more precision than that - but TMap and the Lua API
// both keep the room symbol scaling factor to the full qreal. Both directions
// of the exchange between the two ask whether the box is already showing a
// value rather than comparing the two numbers outright: the refresh must not
// rewrite the box between two keystrokes of a factor being typed into it, and
// Save must not write a rounded copy back over the factor a script set.
static bool spinBoxShows(const QDoubleSpinBox* pSpinBox, const qreal value)
{
    const qreal scale = qPow(10.0, pSpinBox->decimals());
    return qFuzzyCompare(pSpinBox->value(), qRound64(value * scale) / scale);
}

dlgProfilePreferences::dlgProfilePreferences(QWidget* pParentWidget, Host* pHost)
: QDialog(pParentWidget)
, mpHost(pHost)
{
    // init generated dialog
    setupUi(this);
    buildShell();

    mpTimer_apply = new QTimer(this);
    mpTimer_apply->setSingleShot(true);
    mpTimer_apply->setInterval(400ms);
    connect(mpTimer_apply, &QTimer::timeout, this, &dlgProfilePreferences::applyAll);

    QPixmap holdPixmap;
    holdPixmap = notificationAreaIconLabelWarning->pixmap(Qt::ReturnByValue);
    holdPixmap.setDevicePixelRatio(5.3);
    notificationAreaIconLabelWarning->setPixmap(holdPixmap);

    holdPixmap = notificationAreaIconLabelError->pixmap(Qt::ReturnByValue);
    holdPixmap.setDevicePixelRatio(5.3);
    notificationAreaIconLabelError->setPixmap(holdPixmap);

    holdPixmap = notificationAreaIconLabelInformation->pixmap(Qt::ReturnByValue);
    holdPixmap.setDevicePixelRatio(5.3);
    notificationAreaIconLabelInformation->setPixmap(holdPixmap);

    // The "Developer" card (groupBox_debug) on the Advanced page is where
    // temporary/development/testing controls can be placed if needed, they
    // should be added to the (QGridLayout*) returned by:
    // qobject_cast<QGridLayout*>(groupBox_debug->layout())

    mudlet* pMudlet = mudlet::self();

    // Only unhide this if it is needed
    groupBox_discordPrivacy->hide();

    auto updateDiscordPrivacyControls = [this]() {
        const bool enablePrivacy = radioButton_discordGameDetails->isChecked();
        label_discordLargeIcon->setEnabled(enablePrivacy);
        comboBox_discordLargeIconPrivacy->setEnabled(enablePrivacy);
        label_discordSmallIcon->setEnabled(enablePrivacy);
        comboBox_discordSmallIconPrivacy->setEnabled(enablePrivacy);
        checkBox_discordServerAccessToDetail->setEnabled(enablePrivacy);
        checkBox_discordServerAccessToState->setEnabled(enablePrivacy);
        checkBox_discordServerAccessToPartyInfo->setEnabled(enablePrivacy);
        checkBox_discordServerAccessToTimerInfo->setEnabled(enablePrivacy);
    };
    connect(radioButton_discordGameDetails, &QRadioButton::toggled, this, updateDiscordPrivacyControls);
    connect(radioButton_discordMudletOnly, &QRadioButton::toggled, this, updateDiscordPrivacyControls);
    connect(radioButton_discordDisabled, &QRadioButton::toggled, this, updateDiscordPrivacyControls);

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
    case enums::visibleNever:
        comboBox_menuBarVisibility->setCurrentIndex(0);
        break;
    case enums::visibleOnlyWithoutLoadedProfile:
        comboBox_menuBarVisibility->setCurrentIndex(1);
        break;
    default:
        comboBox_menuBarVisibility->setCurrentIndex(2);
    }

    switch (pMudlet->toolBarVisibility()) {
    case enums::visibleNever:
        comboBox_toolBarVisibility->setCurrentIndex(0);
        break;
    case enums::visibleOnlyWithoutLoadedProfile:
        comboBox_toolBarVisibility->setCurrentIndex(1);
        break;
    default:
        comboBox_toolBarVisibility->setCurrentIndex(2);
    }

    // Sync "Never" item deactivation so the dialog opens with consistent state
    // if either visibility was already "Never" on previous save (issue #7079).
    slot_syncMenuToolBarNeverItem();

    checkBox_showTabConnectionIndicators->setChecked(pMudlet->mShowTabConnectionIndicators);
    connect(checkBox_showTabConnectionIndicators, &QCheckBox::toggled, this, [=](bool checked) {
        mudlet::self()->setShowTabConnectionIndicators(checked);
    });

    // Set the properties of the log options
    lineEdit_logFileFolder->setToolTip(utils::richText(tr("Location which will be used to store log files - matching logs will be appended to.")));
    pushButton_whereToLog->setToolTip(utils::richText(tr("Select a directory where logs will be saved.")));
    pushButton_resetLogDir->setToolTip(utils::richText(tr("Reset the directory so that logs are saved to the profile's <i>log</i> directory.")));
    comboBox_logFileNameFormat->setToolTip(tr("<p>This option sets the format of the log name.</p>"
                                              "<p>If <i>Named file</i> is selected, you can set a custom file name. (Logs are appended "
                                              "if a log file of the same name already exists.)</p>"));
    lineEdit_logFileName->setToolTip(utils::richText(tr("Set a custom name for your log. (New logs are appended if a log file of the same name "
                                                        "already exists).")));
    //: Must be a valid default filename for a log-file and is used if the user does not enter any other value (Ensure all instances have the same translation {one of two copies}).
    lineEdit_logFileName->setPlaceholderText(tr("logfile"));
    label_logFileNameExtension->setVisible(false);
    label_logFileName->setVisible(false);
    lineEdit_logFileName->setVisible(false);
    /*:
    text on button to put the map from this profile into the other profiles to
    receive the map from this profile, %n is the number of other profiles that
    have already been selected to receive it and will be zero or more. The button
    will also be disabled (greyed out) in the zero case but the text will still be
    visible.
    */
    pushButton_copyMap->setText(tr("copy to %n destination(s)", nullptr, 0));

    // Must be explicitly hidden before initWithHost(...) can show it again for
    // duplicates that were saved in a previous session:
    label_shortcutsConflictWarning->hide();

    mPopulating = true;
    if (pHost) {
        initWithHost(pHost);
    } else {
        disableHostDetails();
        clearHostDetails();
    }
    mPopulating = false;

#if defined(INCLUDE_UPDATER)
    if (mudlet::self()->developmentVersion && !qEnvironmentVariableIsSet("DEV_UPDATER")) {
        // tick the box and make it be "un-untickable" as automatic updates are
        // disabled in dev builds
        checkbox_noAutomaticUpdates->setChecked(true);
        checkbox_noAutomaticUpdates->setDisabled(true);
        checkbox_noAutomaticUpdates->setToolTip(utils::richText(tr("Automatic updates are disabled in development builds to prevent an update from overwriting your Mudlet.")));
    } else if (!pMudlet->pUpdater->ready()) {
        // Nothing to show a setting for until the platform updater is set up,
        // and a checkbox that silently does nothing is worse than no checkbox
        groupBox_updates->hide();
    } else {
        checkbox_noAutomaticUpdates->setChecked(!pMudlet->pUpdater->updateAutomatically());
        // This is the extra connect(...) relating to settings' changes saved by
        // a different profile mentioned further down in this constructor:
        connect(pMudlet->pUpdater, &Updater::signal_automaticUpdatesChanged, this, &dlgProfilePreferences::slot_changeAutomaticUpdates);
    }
#else
    groupBox_updates->hide();
#endif

    // To be moved to a slot that is used on GUI language change when that gets
    // implemented:

    // Set the tooltip on the containing widget so both the label and the
    // control have the same tool-tip:
    //: Tooltip for timer debug output minimum interval
    widget_timerDebugOutputMinimumInterval->setToolTip(tr("<p>Hide success messages in Central Debug Console for timers with intervals below this threshold. "
                                                          "Error messages always display.</p>"));

    //: Tooltip for show glyph usage button
    pushButton_showGlyphUsage->setToolTip(utils::richText(tr("Show all map symbols, their Unicode code-points, font availability, and which rooms use them.")));
    fontComboBox_mapSymbols->setToolTip(utils::richText(tr("Select the only or the primary font used (depending on <i>Only use symbols "
                                                           "(glyphs) from chosen font</i> setting) to produce the 2D mapper room symbols.")));
    //: Tooltip for map symbol font usage option
    checkBox_isOnlyMapSymbolFontToBeUsed->setToolTip(utils::richText(tr("Use only the selected font (may show \uFFFD for missing symbols) or allow fallback fonts for better coverage.")));
    //: Tooltip for run all keybindings option
    checkBox_runAllKeyBindings->setToolTip(tr("<p>Run all matching keybindings instead of just the first one. "
                                              "Disable for compatibility with pre-3.9.0 scripts.</p>"));
    //: Tooltip for East Asian ambiguous width character option
    checkBox_useWideAmbiguousEastAsianGlyphs->setToolTip(tr("<p>Controls display width for ambiguous East Asian characters. "
                                                            "Auto-detects correct width for most encodings (default), or choose narrow/wide.</p>"));
    //: Tooltip for text analyzer option
    checkBox_enableTextAnalyzer->setToolTip(tr("<p>Enable context menu to analyze UTF-16/UTF-8 encoding of selected text. "
                                               "Useful for identifying multi-byte characters.</p>"));
    //: Tooltip for show icons on menus option
    checkBox_showIconsOnMenus->setToolTip(tr("<p>Control menu icon display: on, off, or auto (system default). "
                                             "May require restart.</p>"));
    lineEdit_mmcpPort->setPlaceholderText(QString::number(csDefaultMMCPHostPort));
    lineEdit_mmcpChatName->setPlaceholderText(csDefaultMMCPChatName);
    connect(lineEdit_mmcpChatName, &QLineEdit::editingFinished, this, &dlgProfilePreferences::slot_mmcpChatNameChanged);
    lineEdit_mmcpChatMessagePrefix->setPlaceholderText(csDefaultChatPrefix);

    // Add validator for MMCP Chatname, disallow ~ and , characters
    QRegularExpression rx("^[^~,]*$");
    QValidator* validator = new QRegularExpressionValidator(rx, this);
    lineEdit_mmcpChatName->setValidator(validator);

    connect(checkBox_showSpacesAndTabs, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_changeShowSpacesAndTabs);
    connect(checkBox_showLineFeedsAndParagraphs, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_changeShowLineFeedsAndParagraphs);
    connect(closeButton, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_saveAndClose);
    connect(pMudlet, &mudlet::signal_hostCreated, this, &dlgProfilePreferences::slot_handleHostAddition);
    connect(pMudlet, &mudlet::signal_hostDestroyed, this, &dlgProfilePreferences::slot_handleHostDeletion);
    // Because QComboBox::currentIndexChanged has multiple (overloaded) forms we
    // have to state which one we want to use for these two:
    connect(comboBox_menuBarVisibility, qOverload<int>(&QComboBox::currentIndexChanged), this, &dlgProfilePreferences::slot_changeShowMenuBar);
    connect(comboBox_toolBarVisibility, qOverload<int>(&QComboBox::currentIndexChanged), this, &dlgProfilePreferences::slot_changeShowToolBar);

    comboBox_appearance->setCurrentIndex(pMudlet->mAppearance);

    // This group of signal/slot connections handles updating *this* instance of
    // the "Profile preferences" form/dialog when a *different* profile saves
    // new settings from this one - there is a further connect(...) above which
    // is also involved but it is conditional on having the updater code being
    // included in compilation:
    connect(pMudlet, &mudlet::signal_editorTextOptionsChanged, this, &dlgProfilePreferences::slot_changeEditorTextOptions);
    connect(pMudlet, &mudlet::signal_showMapAuditErrorsChanged, this, &dlgProfilePreferences::slot_changeShowMapAuditErrors);
    connect(pMudlet, &mudlet::signal_setToolBarIconSize, this, &dlgProfilePreferences::slot_setToolBarIconSize);
    connect(pMudlet, &mudlet::signal_setTreeIconSize, this, &dlgProfilePreferences::slot_setTreeWidgetIconSize);
    connect(pMudlet, &mudlet::signal_menuBarVisibilityChanged, this, &dlgProfilePreferences::slot_changeMenuBarVisibility);
    connect(pMudlet, &mudlet::signal_toolBarVisibilityChanged, this, &dlgProfilePreferences::slot_changeToolBarVisibility);
    connect(pMudlet, &mudlet::signal_showIconsOnMenusChanged, this, &dlgProfilePreferences::slot_changeShowIconsOnMenus);
    connect(pMudlet, &mudlet::signal_guiLanguageChanged, this, &dlgProfilePreferences::slot_guiLanguageChanged);
    connect(pMudlet, &mudlet::signal_appearanceChanged, this, &dlgProfilePreferences::slot_setAppearance);
    connect(pMudlet, &mudlet::signal_showTabConnectionIndicatorsChanged, this, &dlgProfilePreferences::slot_changeShowTabConnectionIndicators);
    connect(comboBox_appearance, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        dlgProfilePreferences::slot_setAppearance(enums::Appearance(index));
    });
    connect(toolButton_resetMainWindowShortcuts, &QPushButton::released, this, [=, this]() {
        emit signal_resetMainWindowShortcutsToDefaults();
        // Recompute once after every editor has been reset, rather than per
        // editor, so transient clashes part-way through do not raise warnings:
        updateShortcutConflictWarning();
        slot_scheduleApply();
    });

    generateDiscordTooltips();

    label_languageChangeWarning->hide();
    label_invalidFontError->hide();
    label_variableWidthFontWarning->hide();

    comboBox_guiLanguage->clear();
    for (auto& code : pMudlet->getAvailableTranslationCodes()) {
        auto& translation = pMudlet->mTranslationsMap[code];
        auto& nativeName = translation.getNativeName();
        if (translation.fromResourceFile()) {
            auto& translatedPc = translation.getTranslatedPercentage();
            if (translatedPc >= pMudlet->scmTranslationGoldStar) {
                comboBox_guiLanguage->addItem(QIcon(":/icons/rating.png"), nativeName, code);
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
        connect(comboBox_guiLanguage, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &dlgProfilePreferences::slot_changeGuiLanguage);
    } else {
        currentIndex = comboBox_guiLanguage->findData(qsl("en_US"));
        if (Q_LIKELY(currentIndex != -1)) {
            // The default code has been found in the UserData role for one of
            // the entries - so select it as a fallback
            comboBox_guiLanguage->setCurrentIndex(currentIndex);
            connect(comboBox_guiLanguage, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &dlgProfilePreferences::slot_changeGuiLanguage);
        } else if (comboBox_guiLanguage->count()) {
            // There is at least ONE entry but it is not the expected one
            // or the American English default - so select that first one as a
            // last ditch effort:
            comboBox_guiLanguage->setCurrentIndex(0);
            connect(comboBox_guiLanguage, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &dlgProfilePreferences::slot_changeGuiLanguage);
        } else {
            // Nothing available - so disable the control:
            comboBox_guiLanguage->setEnabled(false);
            // And insert an Engineering English warning text - this is probably
            // a sign of significant borkage in the translation system!
            comboBox_guiLanguage->addItem(qsl("No translations available!"));
        }
    }

    QSettings settings("Mudlet", "CrashReporter");
    QVariant storedOption = settings.value("autoSendCrashReports", QVariant());
    int option = 2;
    if (storedOption.isValid()) {
        option = storedOption.toInt() - 1;
    }
    comboBox_crashReportPolicy->setCurrentIndex(option);
    connect(comboBox_crashReportPolicy, qOverload<int>(&QComboBox::currentIndexChanged), this, &dlgProfilePreferences::slot_crashReportPolicyChanged);

    setupPasswordsMigration();

    connectApplyTriggers();
    snapshotValues();
    guardScrollWheel();
    // Both want the controls a profile brought with it, so they run after
    // initWithHost() rather than from buildShell()
    updateColumnWidthCaps();
    rebuildTabOrder();

    applyShellStyle();

    setMinimumSize(780, 560);
    const auto geometry = mudlet::getQSettings()->value(qsl("profilePreferencesGeometry")).toByteArray();
    if (geometry.isEmpty() || !restoreGeometry(geometry)) {
        resize(1060, 760);
    }
}

dlgProfilePreferences::~dlgProfilePreferences()
{
    // ~QDialog hides the dialog once this destructor is done, and the widget
    // that has the keyboard focus then emits its editingFinished() - the chat
    // name field and the shortcut editors both act on that one - when this
    // object is no longer a valid receiver (#9574)
    utils::disconnectChildSignals(this);
}

// The .ui file nests controls several layouts deep, and QLayout::removeWidget()
// only looks at its own items, so a widget being re-homed onto a card has to be
// hunted down through the layout tree. Qt would do this itself from
// QLayout::addWidget() but warns about it once per widget, which for a whole
// dialog's worth of moves buries anything else on the console.
static bool removeFromLayoutTree(QLayout* pLayout, QWidget* pWidget)
{
    for (int i = 0, total = pLayout->count(); i < total; ++i) {
        QLayoutItem* pItem = pLayout->itemAt(i);
        if (pItem->widget() == pWidget) {
            delete pLayout->takeAt(i);
            pLayout->invalidate();
            return true;
        }
        if (QLayout* pChildLayout = pItem->layout(); pChildLayout && removeFromLayoutTree(pChildLayout, pWidget)) {
            return true;
        }
    }
    return false;
}

static void detachFromLayout(QWidget* pWidget)
{
    QWidget* pParent = pWidget->parentWidget();
    if (QLayout* pLayout = pParent ? pParent->layout() : nullptr; pLayout) {
        removeFromLayoutTree(pLayout, pWidget);
    }
}

// The shell's own scaffolding - pages, viewports, columns, the rows the
// sidebar and the title sit in. Nothing paints them; the page colour behind
// them shows through. The property is what the shell stylesheet keeps them
// transparent by, which matters because a profile's Lua stylesheet is applied
// to the whole dialog and reaches every widget it does not name.
static void markAsShellSurface(QWidget* pWidget)
{
    pWidget->setProperty("settingsSurface", true);
}

// A column narrower than its contents clips them rather than scrolling, so the
// cap is the reading width or whatever the widest card needs. The page is held
// to the column plus its scrollbar so that the bar stays beside what it scrolls
// instead of at the far edge of a wide window.
static void capColumnWidth(QScrollArea* pScrollArea)
{
    QWidget* pColumn = pScrollArea ? pScrollArea->widget() : nullptr;
    if (!pColumn || !pColumn->layout()) {
        return;
    }
    // Lifted first, so what is measured is the cards rather than the last cap:
    pColumn->setMaximumWidth(QWIDGETSIZE_MAX);
    pColumn->layout()->activate();
    const int cap = std::max(scmContentColumnWidth, pColumn->minimumSizeHint().width());
    pColumn->setMaximumWidth(cap);
    pScrollArea->setMaximumWidth(cap + pScrollArea->verticalScrollBar()->sizeHint().width());
}

// Every control setupUi() made is *moved* onto the shell rather than recreated:
// the four hand-maintained host enable/disable lists, the translations and the
// tests all hold pointers to those widgets.
void dlgProfilePreferences::buildShell()
{
    while (tabWidget->count()) {
        tabWidget->removeTab(0);
    }
    vBoxLayout_main->removeWidget(tabWidget);
    tabWidget->hide();
    // Kept alive, parented to the dialog and out of every layout: the hidden
    // Save button is still what MapSymbolFontTest clicks to apply and close:
    vBoxLayout_main->removeWidget(widget_bottom);
    widget_bottom->hide();
    vBoxLayout_main->setContentsMargins(0, 0, 0, 0);
    vBoxLayout_main->setSpacing(0);

    mpWidget_shell = new QWidget(this);
    mpWidget_shell->setObjectName(qsl("settingsShell"));
    auto* pShellLayout = new QHBoxLayout(mpWidget_shell);
    pShellLayout->setContentsMargins(0, 0, 0, 0);
    pShellLayout->setSpacing(0);
    vBoxLayout_main->addWidget(mpWidget_shell);

    pShellLayout->addWidget(buildSidebar());

    auto* pContent = new QWidget(mpWidget_shell);
    pContent->setObjectName(qsl("settingsContent"));
    auto* pContentLayout = new QVBoxLayout(pContent);
    pContentLayout->setContentsMargins(24, 24, 24, 24);
    pContentLayout->setSpacing(16);
    pShellLayout->addWidget(pContent, 1);

    mpLineEdit_search = new QLineEdit(pContent);
    mpLineEdit_search->setObjectName(qsl("settingsSearchField"));
    mpLineEdit_search->setClearButtonEnabled(true);
    mpLineEdit_search->setMinimumHeight(36);
    mpLineEdit_search->setMaximumWidth(scmContentColumnWidth);
    mpAction_searchIcon = mpLineEdit_search->addAction(QIcon(), QLineEdit::LeadingPosition);
    pContentLayout->addWidget(mpLineEdit_search);

    auto* pTitleRow = new QWidget(pContent);
    markAsShellSurface(pTitleRow);
    auto* pTitleRowLayout = new QHBoxLayout(pTitleRow);
    pTitleRowLayout->setContentsMargins(0, 0, 0, 0);
    pTitleRowLayout->setSpacing(10);
    // Only ever seen beside the "Search results" title, so it takes no room on
    // a category page and cannot push that page's own title sideways
    mpButton_searchBack = new QToolButton(pTitleRow);
    mpButton_searchBack->setObjectName(qsl("settingsSearchBack"));
    mpButton_searchBack->setArrowType(Qt::LeftArrow);
    mpButton_searchBack->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    mpButton_searchBack->setAutoRaise(true);
    // ...and reachable by keyboard, which a tool button is not by default
    mpButton_searchBack->setFocusPolicy(Qt::StrongFocus);
    mpButton_searchBack->hide();
    connect(mpButton_searchBack, &QAbstractButton::clicked, this, [this]() {
        // The same door the sidebar uses, so leaving the results by either
        // route cannot end anywhere different
        showCategory(mCategoryBeforeSearch.isEmpty() ? qsl("general") : mCategoryBeforeSearch);
    });
    pTitleRowLayout->addWidget(mpButton_searchBack);
    mpLabel_pageTitleIcon = new QLabel(pTitleRow);
    mpLabel_pageTitleIcon->setObjectName(qsl("settingsPageTitleIcon"));
    // A fixed width, so that the title starts at the same x on every category
    // page whatever the shape of that category's icon. The search results have
    // the back chevron in this place instead, and hide it.
    mpLabel_pageTitleIcon->setFixedWidth(20);
    pTitleRowLayout->addWidget(mpLabel_pageTitleIcon);
    mpLabel_pageTitle = new QLabel(pTitleRow);
    mpLabel_pageTitle->setObjectName(qsl("settingsPageTitle"));
    pTitleRowLayout->addWidget(mpLabel_pageTitle);
    pTitleRowLayout->addStretch(1);
    pContentLayout->addWidget(pTitleRow);

    mpStackedWidget_categories = new QStackedWidget(pContent);
    mpStackedWidget_categories->setObjectName(qsl("settingsStack"));
    pContentLayout->addWidget(mpStackedWidget_categories, 1);

    auto* pFindShortcut = new QShortcut(QKeySequence::Find, this);
    connect(pFindShortcut, &QShortcut::activated, this, [this]() {
        mpLineEdit_search->setFocus(Qt::ShortcutFocusReason);
        mpLineEdit_search->selectAll();
    });

    retitleCards();
    reflowWideCards();

    auto* pCard_systemIntegration = createCard(qsl("card_systemIntegration"));
    moveIntoCard(pCard_systemIntegration, {telnetHandlerEnabled, checkBox_showIconsOnMenus});
    buildMigrationBanner();
    buildCategoryPage(qsl("general"), {groupBox_miscellaneous, groupBox_encoding, groupBox_logOptions, groupbox_searchEngineSelection, groupBox_updates, pCard_systemIntegration});

    auto* pCard_theme = createCard(qsl("card_theme"));
    addCardRow(pCard_theme, label_appearance, comboBox_appearance);
    auto* pCard_profileTabs = createCard(qsl("card_profileTabs"));
    moveIntoCard(pCard_profileTabs, {checkBox_showTabConnectionIndicators});
    buildCategoryPage(qsl("appearance"), {pCard_theme, groupBox_iconsAndToolbars, pCard_profileTabs});

    // groupBox_doubleClick is left behind empty: its two controls read as one
    // more display option rather than as a card of their own.
    moveIntoCard(groupBox_displayOptions, {doubleclick_ignore_label, doubleclick_ignore_lineedit, checkBox_enableOSC8Hyperlinks});
    groupBox_doubleClick->hide();
    buildCategoryPage(qsl("mainDisplay"), {groupBox_font, groupBox_displayColors, groupBox_borders, groupBox_wrapping, groupBox_consoleBuffer, groupBox_displayOptions});

    buildCategoryPage(qsl("inputLine"), {groupBox_input, groupBox_spellCheck});

    moveIntoCard(groupBox_autoComplete, {checkBox_echoLuaErrors});
    buildCategoryPage(qsl("editor"), {groupbox_codeEditorThemeSelection, groupBox_autoComplete, groupBox_editorDisplayOptions});

    buildCategoryPage(qsl("mapper"), {groupBox_mapFiles, groupBox_downloadMapOptions, groupBox_mapViewOptions, groupBox_mapperColors, groupBox_playerRoomStyle});

    buildCategoryPage(qsl("chat"), {groupBox_discordPrivacy, groupBox_MMCPOptions});

    auto* pCard_dataEncoding = createCard(qsl("card_dataEncoding"));
    addCardRow(pCard_dataEncoding, label_encoding, comboBox_encoding);
    moveIntoCard(groupBox_specialOptions, {checkBox_USE_IRE_DRIVER_BUGFIX});
    auto* pCard_network = createCard(qsl("card_network"));
    addCardRow(pCard_network, label_networkPacketTimeout, doubleSpinBox_networkPacketTimeout);
    buildCategoryPage(qsl("connection"), {groupBox_protocols, pCard_dataEncoding, groupBox_specialOptions, pCard_network});

    auto* pCard_passwords = createCard(qsl("card_passwords"));
    addCardRow(pCard_passwords, label_store_passwords_in, comboBox_store_passwords_in);
    moveIntoCard(pCard_passwords, {label_password_migration_notification, pushButton_forgetSavedSignIn});
    auto* pCard_serverPermissions = createCard(qsl("card_serverPermissions"));
    moveIntoCard(pCard_serverPermissions, {acceptServerGUI, acceptServerMedia});
    auto* pCard_crashReports = createCard(qsl("card_crashReports"));
    addCardRow(pCard_crashReports, label_crashReportPolicy, comboBox_crashReportPolicy);
    // checkBox_askTlsAvailable gets a card of its own, beside groupBox_ssl
    // rather than inside it: a checkable group box disables its children when
    // unchecked, and asking about an available secure connection is exactly
    // what someone with TLS switched off wants. Untitled, because the checkbox
    // already says what it is.
    auto* pCard_secureReminder = createCard(qsl("card_secureConnectionReminder"));
    pCard_secureReminder->setProperty("settingsCardPlain", true);
    moveIntoCard(pCard_secureReminder, {checkBox_askTlsAvailable});
    buildCategoryPage(qsl("privacy"), {groupBox_ssl, pCard_secureReminder, groupBox_proxy, pCard_passwords, pCard_serverPermissions, groupBox_purgeMediaCache, pCard_crashReports});

    buildCategoryPage(qsl("accessibility"), {groupBox_accessibility});

    buildCategoryPage(qsl("shortcuts"), {groupBox_main_window_shortcuts});

    buildCategoryPage(qsl("advanced"), {groupBox_debug});

    buildSearchResultsPage();

    connect(mpListWidget_categories, &QListWidget::currentRowChanged, this, &dlgProfilePreferences::slot_categorySelected);
    // Return on the support link is how a keyboard user follows it, and the
    // view reports that as an activation
    connect(mpListWidget_categories, &QListWidget::itemActivated, this, &dlgProfilePreferences::slot_sidebarItemClicked);
    connect(mpListWidget_categories, &QListWidget::itemClicked, this, [this](QListWidgetItem* pItem) {
        // ...and where the style counts a single click as an activation too,
        // the connection above has already opened the browser
        if (!mpListWidget_categories->style()->styleHint(QStyle::SH_ItemView_ActivateItemOnSingleClick, nullptr, mpListWidget_categories)) {
            slot_sidebarItemClicked(pItem);
        }
    });
    connect(mpLineEdit_search, &QLineEdit::textChanged, this, &dlgProfilePreferences::runSearch);

    retranslateShell();

    // Without this the stack would show its first page with no category
    // selected and no title over it; setTab() overrides it for a deep link.
    showCategory(qsl("general"));
}

// Left null once dismissed, which every page takes in its stride
void dlgProfilePreferences::buildMigrationBanner()
{
    if (mudlet::getQSettings()->value(qsl("settingsRedesignBannerSeen"), false).toBool()) {
        return;
    }

    mpFrame_migrationBanner = new QFrame(this);
    mpFrame_migrationBanner->setObjectName(qsl("settingsMigrationBanner"));
    // Shown by placeBannerOn() once it is on a page; parented to the dialog
    // until then so that nothing draws it over the shell
    mpFrame_migrationBanner->hide();
    auto* pBannerLayout = new QVBoxLayout(mpFrame_migrationBanner);
    pBannerLayout->setSpacing(8);
    // Lines the banner's text up with the text inside the cards below it: a
    // card is inset by the stylesheet's padding plus its own layout's margins
    pBannerLayout->setContentsMargins(28, 16, 28, 16);

    auto* pTitle = new QLabel(mpFrame_migrationBanner);
    pTitle->setObjectName(qsl("settingsMigrationBannerTitle"));
    pBannerLayout->addWidget(pTitle);

    auto* pBody = new QLabel(mpFrame_migrationBanner);
    pBody->setObjectName(qsl("settingsMigrationBannerBody"));
    pBody->setWordWrap(true);
    pBannerLayout->addWidget(pBody);

    auto* pButtonRow = new QHBoxLayout();
    auto* pDismissButton = new QPushButton(mpFrame_migrationBanner);
    pDismissButton->setObjectName(qsl("settingsMigrationBannerDismiss"));
    pButtonRow->addWidget(pDismissButton);
    pButtonRow->addStretch(1);
    pBannerLayout->addLayout(pButtonRow);

    connect(pDismissButton, &QAbstractButton::clicked, this, [this]() {
        mudlet::getQSettings()->setValue(qsl("settingsRedesignBannerSeen"), true);
        // Off the page it is on and out of the member, so that no later page
        // switch brings it back. Kept alive rather than deleted: the click that
        // dismissed it is still being delivered to a button inside it.
        placeBannerOn(nullptr);
        mpFrame_migrationBanner = nullptr;
    });
}

// The banner is not a card of any one category, and it is not pinned above the
// stack either - pinned, it would eat about 130px of every page's height at the
// dialog's 780x560 minimum. It rides at the top of whichever page is showing
// instead, and comes off every page while the search owns the stack: a card's
// place in the search index is the position it holds in its column, and a
// banner sitting above it would make every one of those a place too low.
void dlgProfilePreferences::placeBannerOn(QWidget* pColumn)
{
    QWidget* pDestination = pColumn ? pColumn : static_cast<QWidget*>(this);
    if (!mpFrame_migrationBanner || mpFrame_migrationBanner->parentWidget() == pDestination) {
        return;
    }
    detachFromLayout(mpFrame_migrationBanner);
    auto* pColumnLayout = pColumn ? qobject_cast<QVBoxLayout*>(pColumn->layout()) : nullptr;
    if (!pColumnLayout) {
        mpFrame_migrationBanner->setParent(this);
        mpFrame_migrationBanner->hide();
        return;
    }
    pColumnLayout->insertWidget(0, mpFrame_migrationBanner);
    // Reparenting hides a widget, and the page may not be the one on show yet
    mpFrame_migrationBanner->show();
}

// The pseudo-category the search results live on: category subheaders and the
// cards themselves, lent here by their own pages for as long as they match.
void dlgProfilePreferences::buildSearchResultsPage()
{
    mpScrollArea_searchResults = new QScrollArea(mpStackedWidget_categories);
    mpScrollArea_searchResults->setObjectName(qsl("settingsPage_searchResults"));
    mpScrollArea_searchResults->setFrameShape(QFrame::NoFrame);
    mpScrollArea_searchResults->setWidgetResizable(true);
    markAsShellSurface(mpScrollArea_searchResults);

    auto* pColumn = new QWidget(mpScrollArea_searchResults);
    pColumn->setObjectName(qsl("settingsColumn_searchResults"));
    mpLayout_searchResults = new QVBoxLayout(pColumn);
    mpLayout_searchResults->setContentsMargins(0, 0, 0, 0);
    mpLayout_searchResults->setSpacing(16);

    mpLabel_searchEmpty = new QLabel(pColumn);
    mpLabel_searchEmpty->setObjectName(qsl("settingsSearchEmpty"));
    mpLabel_searchEmpty->setAlignment(Qt::AlignCenter);
    mpLabel_searchEmpty->setWordWrap(true);
    mpLabel_searchEmpty->setTextFormat(Qt::RichText);
    mpLabel_searchEmpty->setOpenExternalLinks(true);
    mpLabel_searchEmpty->hide();
    // The empty state stands between two stretches so that it lands in the
    // middle of the viewport rather than at the top of it; the leading one is
    // collapsed whenever there is anything to show, and everything a search
    // produces is inserted between the label and the trailing stretch.
    mpLayout_searchResults->addStretch(0);
    mpLayout_searchResults->addWidget(mpLabel_searchEmpty);
    mpLayout_searchResults->addStretch(1);

    mpScrollArea_searchResults->setWidget(pColumn);
    pColumn->setAutoFillBackground(false);
    mpScrollArea_searchResults->viewport()->setAutoFillBackground(false);
    markAsShellSurface(pColumn);
    markAsShellSurface(mpScrollArea_searchResults->viewport());
    mSearchResultsPageIndex = mpStackedWidget_categories->addWidget(mpScrollArea_searchResults);
}

QWidget* dlgProfilePreferences::buildSidebar()
{
    auto* pSidebar = new QWidget(mpWidget_shell);
    pSidebar->setObjectName(qsl("settingsSidebar"));
    pSidebar->setFixedWidth(scmSidebarWidth);
    auto* pSidebarLayout = new QVBoxLayout(pSidebar);
    pSidebarLayout->setContentsMargins(scmSidebarPadding, 16, scmSidebarPadding, 16);
    pSidebarLayout->setSpacing(4);

    auto* pHeader = new QWidget(pSidebar);
    markAsShellSurface(pHeader);
    auto* pHeaderLayout = new QHBoxLayout(pHeader);
    pHeaderLayout->setContentsMargins(8, 0, 8, 8);
    pHeaderLayout->setSpacing(10);
    auto* pIconLabel = new QLabel(pHeader);
    pIconLabel->setPixmap(QPixmap(qsl(":/icons/mudlet_main_32px.png")).scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    pHeaderLayout->addWidget(pIconLabel);
    mpLabel_wordmark = new QLabel(pHeader);
    mpLabel_wordmark->setObjectName(qsl("settingsWordmark"));
    pHeaderLayout->addWidget(mpLabel_wordmark);
    pHeaderLayout->addStretch(1);
    pSidebarLayout->addWidget(pHeader);

    mpListWidget_categories = new QListWidget(pSidebar);
    mpListWidget_categories->setObjectName(qsl("settingsCategoryList"));
    mpListWidget_categories->setFrameShape(QFrame::NoFrame);
    mpListWidget_categories->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mpListWidget_categories->setIconSize(QSize(18, 18));
    mpListWidget_categories->installEventFilter(this);
    pSidebarLayout->addWidget(mpListWidget_categories, 1);

    // The names come from retranslateShell()
    addCategory(qsl("general"), qsl("configure.png"));
    addCategory(qsl("appearance"), qsl("applications-accessories.png"));
    addCategory(qsl("mainDisplay"), qsl("view-split-left-right.png"));
    addCategory(qsl("inputLine"), qsl("edit-select-all.png"));
    addCategory(qsl("editor"), qsl("accessories-text-editor.png"));
    addCategory(qsl("mapper"), qsl("mudlet_room_exits.png"));
    addCategory(qsl("chat"), qsl("internet-telephony.png"));

    addSidebarSeparator();
    addCategory(qsl("connection"), qsl("applications-internet.png"));
    addCategory(qsl("privacy"), qsl("document-encrypt.png"));
    addCategory(qsl("accessibility"), qsl("system-users.png"));
    addCategory(qsl("shortcuts"), qsl("preferences-desktop-keyboard.png"));
    addCategory(qsl("advanced"), qsl("tools-report-bug.png"));

    addSidebarSeparator();
    mpItem_support = new QListWidgetItem(mpListWidget_categories);
    mpItem_support->setIcon(QIcon(qsl(":/icons/help-hint.png")));
    mpItem_support->setData(Qt::UserRole + 1, qsl("https://wiki.mudlet.org"));
    // Enabled so it can be clicked, but not selectable: it opens a browser
    // rather than switching to a page of its own
    mpItem_support->setFlags(Qt::ItemIsEnabled);
    mpItem_support->setSizeHint(QSize(0, 36));

    return pSidebar;
}

void dlgProfilePreferences::addCategory(const QString& key, const QString& iconFile)
{
    auto* pItem = new QListWidgetItem(QIcon(qsl(":/icons/%1").arg(iconFile)), QString(), mpListWidget_categories);
    pItem->setData(Qt::UserRole, key);
    pItem->setSizeHint(QSize(0, 36));
    mCategoryRows.insert(key, mpListWidget_categories->row(pItem));
    mCategoryIconFiles.insert(key, iconFile);
}

void dlgProfilePreferences::retranslateShell()
{
    //: Wordmark at the top of the settings dialog's category sidebar, beside the Mudlet icon
    mpLabel_wordmark->setText(tr("Settings"));
    //: Placeholder text of the search field at the top of the settings dialog
    mpLineEdit_search->setPlaceholderText(tr("Find in settings"));
    //: Accessible name of the list that switches between the settings dialog's categories
    mpListWidget_categories->setAccessibleName(tr("Settings categories"));
    //: Button at the left of the "Search results" heading, leading back to the settings category the search was started from
    mpButton_searchBack->setText(tr("Back"));
    //: Tooltip and accessible name of the button that leaves the settings search results
    const QString backToSettings = tr("Back to the settings you were on");
    mpButton_searchBack->setToolTip(backToSettings);
    mpButton_searchBack->setAccessibleName(backToSettings);
    //: Sidebar link at the bottom of the settings dialog, opening the Mudlet wiki in a browser
    mpItem_support->setText(tr("Mudlet support"));

    QList<std::pair<QString, QString>> categoryNames;
    //: Sidebar category in the settings dialog, holding saving, language, logging, web search and update options
    categoryNames.append({qsl("general"), tr("General")});
    //: Sidebar category in the settings dialog, holding the theme, icon sizes and profile tab options
    categoryNames.append({qsl("appearance"), tr("Appearance")});
    //: Sidebar category in the settings dialog, holding the font, colors, borders and wrapping of the game's text window
    categoryNames.append({qsl("mainDisplay"), tr("Main display")});
    //: Sidebar category in the settings dialog, holding the options of the command line the player types into
    categoryNames.append({qsl("inputLine"), tr("Input line")});
    //: Sidebar category in the settings dialog, holding the script editor's options
    categoryNames.append({qsl("editor"), tr("Editor")});
    //: Sidebar category in the settings dialog, holding the map's files, view and colors
    categoryNames.append({qsl("mapper"), tr("Mapper")});
    //: Sidebar category in the settings dialog, holding the Discord Rich Presence and MudMaster chat options
    categoryNames.append({qsl("chat"), tr("Chat and sharing")});
    //: Sidebar category in the settings dialog, holding the game protocol, encoding and compatibility options
    categoryNames.append({qsl("connection"), tr("Connection")});
    //: Sidebar category in the settings dialog, holding the secure connection, proxy, password and permission options
    categoryNames.append({qsl("privacy"), tr("Privacy and security")});
    //: Sidebar category in the settings dialog, holding the screen reader and other accessibility options
    categoryNames.append({qsl("accessibility"), tr("Accessibility")});
    //: Sidebar category in the settings dialog, holding the main window's keyboard shortcuts
    categoryNames.append({qsl("shortcuts"), tr("Shortcuts")});
    //: Sidebar category in the settings dialog, holding development and diagnostic options
    categoryNames.append({qsl("advanced"), tr("Advanced")});
    for (const auto& [key, name] : categoryNames) {
        if (QListWidgetItem* pItem = mpListWidget_categories->item(mCategoryRows.value(key, -1)); pItem) {
            pItem->setText(name);
        }
    }

    QList<std::pair<QString, QString>> cardTitles;
    //: Card title on the General settings page, above the options that tie Mudlet into the rest of the desktop
    cardTitles.append({qsl("card_systemIntegration"), tr("System integration")});
    //: Card title on the Appearance settings page, above the light/dark theme selector
    cardTitles.append({qsl("card_theme"), tr("Theme")});
    //: Card title on the Appearance settings page, above the options for the tabs that switch between open profiles
    cardTitles.append({qsl("card_profileTabs"), tr("Profile tabs")});
    //: Card title on the Connection settings page, above the character encoding used to talk to the game
    cardTitles.append({qsl("card_dataEncoding"), tr("Data encoding")});
    //: Card title on the Connection settings page, above the network packet timeout
    cardTitles.append({qsl("card_network"), tr("Network")});
    //: Card title on the Privacy and security settings page, above where game passwords are kept
    cardTitles.append({qsl("card_passwords"), tr("Passwords")});
    //: Card title on the Privacy and security settings page, above what the game's server is allowed to do
    cardTitles.append({qsl("card_serverPermissions"), tr("Server permissions")});
    //: Card title on the Privacy and security settings page, above the crash report sending policy
    cardTitles.append({qsl("card_crashReports"), tr("Crash reports")});
    for (const auto& [objectName, title] : cardTitles) {
        if (auto* pCard = findChild<QGroupBox*>(objectName); pCard) {
            pCard->setTitle(title);
        }
    }

    if (mpFrame_migrationBanner) {
        //: Title of the banner explaining that the settings dialog has been reorganised
        mpFrame_migrationBanner->findChild<QLabel*>(qsl("settingsMigrationBannerTitle"))->setText(tr("Same settings, new look!"));
        //: Body of the banner explaining that the settings dialog has been reorganised
        mpFrame_migrationBanner->findChild<QLabel*>(qsl("settingsMigrationBannerBody"))
                ->setText(tr("Settings are reorganised so they are easier to scan and search. Everything is still here - use search to jump straight to what you need."));
        //: Button that dismisses the "Same settings, new look!" banner for good
        mpFrame_migrationBanner->findChild<QPushButton*>(qsl("settingsMigrationBannerDismiss"))->setText(tr("Got it"));
    }

    setSearchKeywords();

    // Nothing is current while the shell is still being built, and the search's
    // own title comes back with the next query:
    if (const QListWidgetItem* pCurrent = mpListWidget_categories->currentItem(); pCurrent && !mSearchActive) {
        mpLabel_pageTitle->setText(pCurrent->text());
    }
}

// What a player types when they do not know what Mudlet calls a setting: the
// acronym for it, the name another client uses, the thing it is for. Each list
// is folded into the text of the card the control sits on, and highlights that
// control when one of its words is what matched.
void dlgProfilePreferences::setSearchKeywords()
{
    // The protocols themselves are only named inside the menu this button pops
    // up, where a search over the widget tree cannot see them. Not translated:
    // these are the protocol names as the games and their documentation spell
    // them.
    pushButton_chooseProtocols->setProperty("searchKeywords", qsl("GMCP MSDP MSSP MSP MXP MTTS MNES NAWS CHARSET NEW-ENVIRON telnet"));

    QList<std::pair<QWidget*, QString>> synonyms;
    //: Comma-separated synonyms for the settings search. Translate them into the words a player of your language would type when looking for this setting, rather than transliterating the English ones; acronyms and protocol names that your language uses untranslated can be left as they are. This one is for the secure connection settings.
    synonyms.append({groupBox_ssl, tr("TLS, SSL, secure connection, encryption, certificate")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for the reminder offered when the game supports a secure connection.
    synonyms.append({checkBox_askTlsAvailable, tr("TLS, SSL, secure connection, reminder")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for the proxy server settings.
    synonyms.append({groupBox_proxy, tr("proxy, SOCKS, tunnel, firewall")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for where game passwords are kept.
    synonyms.append({label_store_passwords_in, tr("password, keyring, keychain, credentials, sign in, two-factor, 2FA")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for showing the password as it is typed.
    synonyms.append({disable_password_masking_checkbox, tr("password, masking, hidden characters")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for telling the game that a screen reader is in use.
    synonyms.append({checkBox_advertiseScreenReader, tr("screen reader, NVDA, JAWS, VoiceOver, Orca, accessibility")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for reading incoming text out loud.
    synonyms.append({checkBox_announceIncomingText, tr("text to speech, TTS, speech, spoken, screen reader")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for putting the time in front of each logged line.
    synonyms.append({mIsLoggingTimestamps, tr("timestamps, time, date, transcript")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for the format logs are written in.
    synonyms.append({mIsToLogInHtml, tr("transcript, HTML, plain text, log format")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for where long lines are broken.
    synonyms.append({groupBox_wrapping, tr("wrap, word wrap, line length, columns, indent")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for how much past text is kept.
    synonyms.append({groupBox_consoleBuffer, tr("scrollback, history, buffer, lines kept")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for fetching a map the game offers.
    synonyms.append({groupBox_downloadMapOptions, tr("download map, fetch map, map from the game")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for links in the game's text being clickable.
    synonyms.append({checkBox_enableOSC8Hyperlinks, tr("hyperlink, link, clickable URL, OSC8")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for showing script errors in the game window.
    synonyms.append({checkBox_echoLuaErrors, tr("echo, error messages, script errors, Lua errors")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for the character encoding used to talk to the game.
    synonyms.append({label_encoding, tr("encoding, character set, charset, UTF-8, Unicode, Latin-1")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for when the menu bar is shown.
    synonyms.append({label_menuBarVisiblity, tr("menu bar, hide menus, fullscreen, distraction free")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for when the toolbar is shown.
    synonyms.append({label_toolBarVisibility, tr("toolbar, hide buttons, fullscreen, distraction free")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for checking spelling as the player types.
    synonyms.append({groupBox_spellCheck, tr("spelling, spell check, dictionary, typos")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for the language Mudlet's own interface is in.
    synonyms.append({label_guiLanguage, tr("language, locale, translation, interface language")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for the light or dark look of Mudlet.
    synonyms.append({label_appearance, tr("dark mode, light mode, night mode, theme, colour scheme")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for whether crash reports are sent.
    synonyms.append({label_crashReportPolicy, tr("crash, telemetry, diagnostics, error reports")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for what Discord is told about the game being played.
    synonyms.append({groupBox_discordPrivacy, tr("Discord, rich presence, status, what I am playing")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for the MudMaster chat protocol.
    synonyms.append({groupBox_MMCPOptions, tr("MMCP, chat, MudMaster, player to player")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for the telnet protocols Mudlet negotiates with the game.
    synonyms.append({groupBox_protocols, tr("protocols, compression, MCCP, negotiation, telnet options")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for throwing away downloaded sounds and music.
    synonyms.append({groupBox_purgeMediaCache, tr("cache, sounds, music, downloaded media, clear")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for the main window's keyboard shortcuts.
    synonyms.append({groupBox_main_window_shortcuts, tr("keyboard shortcuts, hotkeys, key bindings, accelerators")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for saving the profile when Mudlet is closed.
    synonyms.append({mFORCE_SAVE_ON_EXIT, tr("autosave, save on exit, backup")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for the font the game's text is drawn in.
    synonyms.append({groupBox_font, tr("font, typeface, size, monospace, antialiasing")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for how long Mudlet waits for the game to answer.
    synonyms.append({label_networkPacketTimeout, tr("timeout, lag, latency, slow connection")});
    for (const auto& [pControl, words] : synonyms) {
        pControl->setProperty("searchKeywords", words);
    }
}

void dlgProfilePreferences::addSidebarSeparator()
{
    auto* pItem = new QListWidgetItem(mpListWidget_categories);
    pItem->setFlags(Qt::NoItemFlags);
    pItem->setSizeHint(QSize(0, 17));
    auto* pLine = new QFrame(mpListWidget_categories);
    pLine->setObjectName(qsl("settingsSidebarSeparator"));
    pLine->setFrameShape(QFrame::HLine);
    mpListWidget_categories->setItemWidget(pItem, pLine);
}

void dlgProfilePreferences::buildCategoryPage(const QString& key, const QList<QWidget*>& cards)
{
    auto* pScrollArea = new QScrollArea(mpStackedWidget_categories);
    pScrollArea->setObjectName(qsl("settingsPage_%1").arg(key));
    pScrollArea->setFrameShape(QFrame::NoFrame);
    pScrollArea->setWidgetResizable(true);
    markAsShellSurface(pScrollArea);

    auto* pColumn = new QWidget(pScrollArea);
    pColumn->setObjectName(qsl("settingsColumn_%1").arg(key));
    auto* pColumnLayout = new QVBoxLayout(pColumn);
    pColumnLayout->setContentsMargins(0, 0, 0, 0);
    pColumnLayout->setSpacing(16);
    for (auto* pCard : cards) {
        if (!pCard) {
            continue;
        }
        detachFromLayout(pCard);
        pCard->setProperty("settingsCard", true);
        // A checkable card's title starts after its check indicator, a plain
        // one's at the frame edge - 19px apart, which reads as the titles of a
        // page wandering. Landmine 11 forbids taking the checkability away, so
        // the plain ones are given the same inset instead. Named as a property
        // because a stylesheet cannot ask whether a group box is checkable.
        auto* pGroupBox = qobject_cast<QGroupBox*>(pCard);
        pCard->setProperty("settingsCardTitleInset", pGroupBox && !pGroupBox->isCheckable());
        pColumnLayout->addWidget(pCard);
    }
    pColumnLayout->addStretch(1);
    pScrollArea->setWidget(pColumn);
    // setWidget() turns the column into an opaque one filled from its own
    // palette; the page background belongs to the content area behind it:
    pColumn->setAutoFillBackground(false);
    pScrollArea->viewport()->setAutoFillBackground(false);
    markAsShellSurface(pColumn);
    markAsShellSurface(pScrollArea->viewport());
    capColumnWidth(pScrollArea);

    mCategoryPageIndexes.insert(key, mpStackedWidget_categories->addWidget(pScrollArea));
}

// A card that no group box in the .ui file corresponds to. Its title comes from
// retranslateShell(), by object name.
QGroupBox* dlgProfilePreferences::createCard(const QString& objectName)
{
    auto* pCard = new QGroupBox(this);
    pCard->setObjectName(objectName);
    auto* pLayout = new QVBoxLayout(pCard);
    pLayout->setSpacing(8);
    return pCard;
}

void dlgProfilePreferences::moveIntoCard(QGroupBox* pCard, const QList<QWidget*>& controls)
{
    auto* pLayout = qobject_cast<QVBoxLayout*>(pCard->layout());
    for (auto* pControl : controls) {
        detachFromLayout(pControl);
        if (pLayout) {
            pLayout->addWidget(pControl);
            continue;
        }
        // A card reusing a .ui group box keeps that group box's grid; an empty
        // row costs nothing, so incoming controls simply go below what is there
        if (auto* pGridLayout = qobject_cast<QGridLayout*>(pCard->layout()); pGridLayout) {
            pGridLayout->addWidget(pControl, pGridLayout->rowCount(), 0, 1, std::max(1, pGridLayout->columnCount()));
        } else {
            // Every card this is called with has one of the two layouts above.
            // One that grew a different one would take the control in but lay
            // it out nowhere, which shows up as a setting silently missing:
            Q_ASSERT_X(false, "dlgProfilePreferences::moveIntoCard", "card has neither a vertical nor a grid layout to take the control into");
            qWarning() << "dlgProfilePreferences::moveIntoCard(...) WARNING - the card" << pCard->objectName() << "has no layout that" << pControl->objectName()
                       << "can be added to, so it will not be shown.";
            pControl->setParent(pCard);
        }
    }
}

void dlgProfilePreferences::addCardRow(QGroupBox* pCard, QWidget* pLabel, QWidget* pControl)
{
    auto* pRowLayout = new QHBoxLayout();
    pRowLayout->setSpacing(8);
    detachFromLayout(pLabel);
    detachFromLayout(pControl);
    pLabel->setParent(pCard);
    pControl->setParent(pCard);
    pRowLayout->addWidget(pLabel);
    // A combo box put to the width of the card can show more of its longest
    // item, but a number field stretched the same way reads as a text field
    if (qobject_cast<QAbstractSpinBox*>(pControl)) {
        pRowLayout->addWidget(pControl);
        pRowLayout->addStretch(1);
    } else {
        pRowLayout->addWidget(pControl, 1);
    }
    auto* pCardLayout = qobject_cast<QVBoxLayout*>(pCard->layout());
    Q_ASSERT_X(pCardLayout, "dlgProfilePreferences::addCardRow", "rows can only be added to a card that createCard() made");
    if (!pCardLayout) {
        return;
    }
    pCardLayout->addLayout(pRowLayout);
}

// The .ui file titles these group boxes after the tab they sat on, and
// retranslateUi() puts those titles back on every language change.
void dlgProfilePreferences::retitleCards()
{
    //: Card title on the General settings page, above the "save profile on exit" and "notify on new data" options
    groupBox_miscellaneous->setTitle(tr("Saving and notifications"));
    //: Card title on the General settings page, above the interface language selector
    groupBox_encoding->setTitle(tr("Language"));
    //: Card title on the General settings page, above the search engine used by the "search on the web" context menu entry
    groupbox_searchEngineSelection->setTitle(tr("Web search"));
    //: Card title on the Appearance settings page, above the icon size and menu/toolbar visibility options
    groupBox_iconsAndToolbars->setTitle(tr("Icons and toolbars"));
    //: Card title on the Main display settings page, above the colors used for the game's text
    groupBox_displayColors->setTitle(tr("Colors"));
    //: Card title on the Main display settings page, above the width of the borders around the game's text
    groupBox_borders->setTitle(tr("Borders"));
    //: Card title on the Editor settings page, above the Lua autocomplete and error echo options
    groupBox_autoComplete->setTitle(tr("Scripting"));
    //: Card title on the Editor settings page, above the options showing whitespace and other invisible marks
    groupBox_editorDisplayOptions->setTitle(tr("Whitespace and marks"));
    //: Card title on the Mapper settings page, above the colors used to draw the map
    groupBox_mapperColors->setTitle(tr("Map colors"));
    //: Card title on the Connection settings page, above the options needed by some older game drivers
    groupBox_specialOptions->setTitle(tr("Compatibility"));
    //: Card title on the Advanced settings page, above development and diagnostic options
    groupBox_debug->setTitle(tr("Developer"));
}

// The .ui file laid these grids out across the full width of a tab: rows of
// three and four cells side by side, which do not fit the reading column the
// cards live in now. Each is re-flowed into fewer, taller columns.
void dlgProfilePreferences::reflowWideCards()
{
    // Word wrapping: three "label - spin box - characters" groups in a row
    horizontalLayout_groupBox_wrapping->removeWidget(frame_wrap_at);
    horizontalLayout_groupBox_wrapping->removeWidget(frame_indent_wrapped);
    horizontalLayout_groupBox_wrapping->removeWidget(frame_hanging_indent_wrapped);
    int wrappingRow = 0;
    for (auto* pFrame : {frame_wrap_at, frame_indent_wrapped, frame_hanging_indent_wrapped}) {
        // Each frame is now as wide as the card, so its own row needs a trailing
        // stretch or the spin box grows to fill it
        qobject_cast<QHBoxLayout*>(pFrame->layout())->addStretch(1);
        verticalLayout_groupBox_wrapping->insertWidget(wrappingRow++, pFrame);
    }

    // Map view: two rows of three checkboxes become three rows of two
    const QList<QWidget*> mapViewOptions{
            mMapperUseAntiAlias, checkBox_drawUpperLowerLevels, checkbox_mMapperShowRoomBorders, checkBox_invertMapZoom, checkBox_largeAreaExitArrows, checkBox_showDefaultArea};
    for (auto* pOption : mapViewOptions) {
        gridLayout_groupBox_mapViewOptions->removeWidget(pOption);
    }
    gridLayout_groupBox_mapViewOptions->removeWidget(gridGroupBox);
    gridLayout_groupBox_mapViewOptions->removeWidget(groupBox_mapSymbols);
    for (int i = 0, total = mapViewOptions.size(); i < total; ++i) {
        gridLayout_groupBox_mapViewOptions->addWidget(mapViewOptions.at(i), i / 2, i % 2);
    }
    gridLayout_groupBox_mapViewOptions->addWidget(gridGroupBox, 3, 0, 1, 2);
    gridLayout_groupBox_mapViewOptions->addWidget(groupBox_mapSymbols, 4, 0, 1, 2);

    // ...the symbol font's row of three becomes a column of three, leaving the
    // two cells that initWithHost() appends the scaling factor to free
    const QList<QWidget*> mapSymbolRows{label_mapSymbolsFont, fontComboBox_mapSymbols, pushButton_showGlyphUsage, checkBox_isOnlyMapSymbolFontToBeUsed};
    for (auto* pWidget : mapSymbolRows) {
        gridLayout_groupBox_mapSymbols->removeWidget(pWidget);
    }
    gridLayout_groupBox_mapSymbols->addWidget(label_mapSymbolsFont, 0, 0);
    gridLayout_groupBox_mapSymbols->addWidget(fontComboBox_mapSymbols, 0, 1);
    gridLayout_groupBox_mapSymbols->addWidget(checkBox_isOnlyMapSymbolFontToBeUsed, 1, 0, 1, 2);
    gridLayout_groupBox_mapSymbols->addWidget(pushButton_showGlyphUsage, 2, 0, 1, 1, Qt::AlignLeft);

    // ...and the four feature sizes go from one row of eight cells to two of four
    const QList<QWidget*> featureSizes{label_roomSize, spinBox_roomSize, label_exitSize, spinBox_exitSize, label_borderSize, spinBox_borderSize, label_gridSize, doubleSpinBox_gridSize};
    for (auto* pWidget : featureSizes) {
        groupBox_sizing->removeWidget(pWidget);
    }
    for (int i = 0, total = featureSizes.size(); i < total; ++i) {
        groupBox_sizing->addWidget(featureSizes.at(i), i / 4, i % 4);
    }
    // The spin boxes ask to expand, and across a card's width that stretches
    // four of them into text fields. An empty column takes the slack instead.
    groupBox_sizing->setColumnStretch(4, 1);

    // Discord: the three modes, a divider and then a two-column block, instead
    // of the modes and two more blocks all standing side by side
    const QList<QWidget*> discordRows{radioButton_discordGameDetails,
                                      radioButton_discordMudletOnly,
                                      radioButton_discordDisabled,
                                      frame_discordDivider,
                                      label_discordUserName,
                                      lineEdit_discordUserName,
                                      label_discordCurrentUser,
                                      label_data_discordCurrentUser,
                                      label_discordLargeIcon,
                                      comboBox_discordLargeIconPrivacy,
                                      label_discordSmallIcon,
                                      comboBox_discordSmallIconPrivacy,
                                      checkBox_discordServerAccessToDetail,
                                      checkBox_discordServerAccessToState,
                                      checkBox_discordServerAccessToPartyInfo,
                                      checkBox_discordServerAccessToTimerInfo};
    for (auto* pWidget : discordRows) {
        gridLayout_groupBox_discordRichPresence->removeWidget(pWidget);
    }
    // The divider used to stand between two columns; stacked, it lies across them
    frame_discordDivider->setFrameShape(QFrame::HLine);
    int discordRow = 0;
    for (auto* pWidget : {radioButton_discordGameDetails, radioButton_discordMudletOnly, radioButton_discordDisabled}) {
        gridLayout_groupBox_discordRichPresence->addWidget(pWidget, discordRow++, 0, 1, 2);
    }
    gridLayout_groupBox_discordRichPresence->addWidget(frame_discordDivider, discordRow++, 0, 1, 2);
    gridLayout_groupBox_discordRichPresence->addWidget(label_discordUserName, discordRow, 0);
    gridLayout_groupBox_discordRichPresence->addWidget(lineEdit_discordUserName, discordRow++, 1);
    gridLayout_groupBox_discordRichPresence->addWidget(label_discordCurrentUser, discordRow, 0);
    gridLayout_groupBox_discordRichPresence->addWidget(label_data_discordCurrentUser, discordRow++, 1);
    gridLayout_groupBox_discordRichPresence->addWidget(label_discordLargeIcon, discordRow, 0);
    gridLayout_groupBox_discordRichPresence->addWidget(comboBox_discordLargeIconPrivacy, discordRow++, 1);
    gridLayout_groupBox_discordRichPresence->addWidget(label_discordSmallIcon, discordRow, 0);
    gridLayout_groupBox_discordRichPresence->addWidget(comboBox_discordSmallIconPrivacy, discordRow++, 1);
    for (auto* pWidget : {checkBox_discordServerAccessToDetail, checkBox_discordServerAccessToState, checkBox_discordServerAccessToPartyInfo, checkBox_discordServerAccessToTimerInfo}) {
        gridLayout_groupBox_discordRichPresence->addWidget(pWidget, discordRow++, 0, 1, 2);
    }
    gridLayout_groupBox_discordRichPresence->setColumnStretch(1, 1);

    // The .ui file frames this one like an input, but it only ever reports a
    // name - and with no Discord user to report it is an empty box on the card
    label_data_discordCurrentUser->setFrameShape(QFrame::NoFrame);

    // Log options: the two checkboxes stood side by side, which already filled
    // the reading column in English and overran it by 200px in German
    gridLayout_groupBox_logOptions->removeWidget(mIsToLogInHtml);
    gridLayout_groupBox_logOptions->removeWidget(mIsLoggingTimestamps);
    const QList<QWidget*> logOptionRows{label_whereToLog,
                                        lineEdit_logFileFolder,
                                        pushButton_whereToLog,
                                        pushButton_resetLogDir,
                                        label_logFileNameFormat,
                                        comboBox_logFileNameFormat,
                                        label_logFileName,
                                        lineEdit_logFileName,
                                        label_logFileNameExtension};
    for (auto* pRow : logOptionRows) {
        gridLayout_groupBox_logOptions->removeWidget(pRow);
    }
    gridLayout_groupBox_logOptions->addWidget(mIsToLogInHtml, 0, 0, 1, 4);
    gridLayout_groupBox_logOptions->addWidget(mIsLoggingTimestamps, 1, 0, 1, 4);
    gridLayout_groupBox_logOptions->addWidget(label_whereToLog, 2, 0, Qt::AlignRight);
    gridLayout_groupBox_logOptions->addWidget(lineEdit_logFileFolder, 2, 1);
    gridLayout_groupBox_logOptions->addWidget(pushButton_whereToLog, 2, 2);
    gridLayout_groupBox_logOptions->addWidget(pushButton_resetLogDir, 2, 3);
    gridLayout_groupBox_logOptions->addWidget(label_logFileNameFormat, 3, 0, Qt::AlignRight);
    gridLayout_groupBox_logOptions->addWidget(comboBox_logFileNameFormat, 3, 1, 1, 3);
    gridLayout_groupBox_logOptions->addWidget(label_logFileName, 4, 0, Qt::AlignRight);
    gridLayout_groupBox_logOptions->addWidget(lineEdit_logFileName, 4, 1, 1, 2);
    gridLayout_groupBox_logOptions->addWidget(label_logFileNameExtension, 4, 3);

    // Icons and toolbars: two label-and-control pairs abreast, whose German
    // labels are half again as long as the English ones, become four rows
    const QList<QPair<QWidget*, QWidget*>> iconRows{
            {label_mainIconSize, MainIconSize}, {label_33, TEFolderIconSize}, {label_menuBarVisiblity, comboBox_menuBarVisibility}, {label_toolBarVisibility, comboBox_toolBarVisibility}};
    for (const auto& [pLabel, pControl] : iconRows) {
        gridLayout_groupBox_iconsAndToolbars->removeWidget(pLabel);
        gridLayout_groupBox_iconsAndToolbars->removeWidget(pControl);
    }
    int iconRow = 0;
    for (const auto& [pLabel, pControl] : iconRows) {
        gridLayout_groupBox_iconsAndToolbars->addWidget(pLabel, iconRow, 0, Qt::AlignRight);
        gridLayout_groupBox_iconsAndToolbars->addWidget(pControl, iconRow++, 1, Qt::AlignLeft);
    }
    gridLayout_groupBox_iconsAndToolbars->setColumnStretch(2, 1);

    // Alone on its grid row now, and adrift on the right of an otherwise
    // left-aligned card unless it spans both columns
    gridLayout_groupBox_debug->removeWidget(checkBox_expectCSpaceIdInColonLessMColorCode);
    gridLayout_groupBox_debug->addWidget(checkBox_expectCSpaceIdInColonLessMColorCode, 0, 0, 1, 2);
    // ...and the time edit beside it had the row's stretch, which put a
    // four-field clock control across the whole card
    horizontalLayout_timerDebugOutputMinimumInterval->setStretch(1, 0);
    horizontalLayout_timerDebugOutputMinimumInterval->addStretch(1);

    // These two report what just happened into a line of runtime text - a
    // profile name, a file path - that is longer than any column. Wrapped, they
    // stop being a floor under the width of the page they are on.
    label_mapFileActionResult->setWordWrap(true);
    label_password_migration_notification->setWordWrap(true);
}

void dlgProfilePreferences::updateColumnWidthCaps()
{
    for (const int page : std::as_const(mCategoryPageIndexes)) {
        capColumnWidth(qobject_cast<QScrollArea*>(mpStackedWidget_categories->widget(page)));
    }
    capColumnWidth(mpScrollArea_searchResults);
}

static void collectFocusableInLayoutOrder(const QLayout* pLayout, QList<QWidget*>& chain)
{
    for (int i = 0, total = pLayout->count(); i < total; ++i) {
        QLayoutItem* pItem = pLayout->itemAt(i);
        if (QWidget* pWidget = pItem->widget(); pWidget) {
            if ((pWidget->focusPolicy() & Qt::TabFocus) == Qt::TabFocus) {
                chain.append(pWidget);
            }
            if (const QLayout* pChildLayout = pWidget->layout(); pChildLayout) {
                collectFocusableInLayoutOrder(pChildLayout, chain);
            }
        } else if (const QLayout* pChildLayout = pItem->layout(); pChildLayout) {
            collectFocusableInLayoutOrder(pChildLayout, chain);
        }
    }
}

// Qt appends a reparented widget to the end of the dialog's focus chain, so
// after the moves the .ui file's ~190 tab stops describe the tabs the controls
// came from rather than the cards they sit on. One chain over the search field,
// the sidebar and then every page in sidebar order puts that right: only the
// widgets of the page on show take part in a traversal, so where a page's
// widgets fall among another page's does not matter.
void dlgProfilePreferences::rebuildTabOrder()
{
    // The back chevron only exists while the results are showing, and a hidden
    // widget is skipped by a traversal rather than trapping it
    QList<QWidget*> chain{mpLineEdit_search, mpButton_searchBack, mpListWidget_categories};
    for (int row = 0, rows = mpListWidget_categories->count(); row < rows; ++row) {
        const QString key = mpListWidget_categories->item(row)->data(Qt::UserRole).toString();
        auto* pScrollArea = qobject_cast<QScrollArea*>(mpStackedWidget_categories->widget(mCategoryPageIndexes.value(key, -1)));
        QWidget* pColumn = pScrollArea ? pScrollArea->widget() : nullptr;
        if (!pColumn || !pColumn->layout()) {
            continue;
        }
        collectFocusableInLayoutOrder(pColumn->layout(), chain);
    }
    for (int i = 1, total = chain.size(); i < total; ++i) {
        setTabOrder(chain.at(i - 1), chain.at(i));
    }
}

// Instant apply turns a wheel that happens to pass over a spin box or a combo
// box on the way down a page into a silent change of that setting. Both stop
// answering the wheel unless they hold the keyboard focus, and stop taking it
// from a wheel alone - see eventFilter(), which hands the event on to the page.
void dlgProfilePreferences::guardScrollWheel()
{
    for (auto* pSpinBox : findChildren<QAbstractSpinBox*>()) {
        pSpinBox->setFocusPolicy(Qt::StrongFocus);
        pSpinBox->installEventFilter(this);
    }
    for (auto* pComboBox : findChildren<QComboBox*>()) {
        pComboBox->setFocusPolicy(Qt::StrongFocus);
        pComboBox->installEventFilter(this);
    }
}

bool dlgProfilePreferences::eventFilter(QObject* pObject, QEvent* pEvent)
{
    if (pEvent->type() == QEvent::Wheel) {
        auto* pControl = qobject_cast<QWidget*>(pObject);
        if (pControl && !pControl->hasFocus()) {
            // Refusing the wheel is only half of it: QApplication carries an
            // unaccepted wheel event up the parent chain only while nothing has
            // *handled* it, and an event filter answering true has. Without
            // handing the event to the page here the wheel does nothing at all.
            for (QWidget* pAncestor = pControl->parentWidget(); pAncestor; pAncestor = pAncestor->parentWidget()) {
                if (auto* pScrollArea = qobject_cast<QAbstractScrollArea*>(pAncestor); pScrollArea) {
                    QCoreApplication::sendEvent(pScrollArea->viewport(), pEvent);
                    break;
                }
            }
            return true;
        }
    }
    // Styling the sidebar's items takes its native focus rectangle away with
    // them, so a keyboard user reaching the list has nothing to tell them the
    // arrow keys now move the selection. A property the stylesheet keys off
    // puts that back, since a QSS rule cannot ask whether the widget a
    // subcontrol belongs to has the focus.
    if (pObject == mpListWidget_categories && (pEvent->type() == QEvent::FocusIn || pEvent->type() == QEvent::FocusOut)) {
        mpListWidget_categories->setProperty("settingsFocused", pEvent->type() == QEvent::FocusIn);
        mpListWidget_categories->style()->polish(mpListWidget_categories);
    }
    return QDialog::eventFilter(pObject, pEvent);
}

void dlgProfilePreferences::showCategory(const QString& key, QWidget* pSpotlightTarget)
{
    // Choosing a category is one of the ways out of the search results, but it
    // is the selection *changing* that carries it out - and a deep link naming
    // the category the sidebar is already on changes nothing. So the query goes
    // here rather than being left to the selection to clear.
    if (mSearchActive) {
        mpLineEdit_search->clear();
    }
    QString category = key;
    if (!mCategoryRows.contains(category)) {
        // Every deep link is written in C++, so one that names nothing is a
        // typo rather than anything a user can do:
        qWarning() << "dlgProfilePreferences::showCategory(...) WARNING - there is no settings category" << key << "- showing General instead.";
        category = qsl("general");
    }
    mpListWidget_categories->setCurrentRow(mCategoryRows.value(category));
    spotlight(pSpotlightTarget);
}

static QString spotlightStyleSheet(const QColor& accent, const qreal strength)
{
    return qsl("#settingsSpotlight { border: 2px solid rgba(%1, %2, %3, %4); border-radius: 8px; background-color: rgba(%1, %2, %3, %5); }")
            .arg(QString::number(accent.red()), QString::number(accent.green()), QString::number(accent.blue()), QString::number(strength, 'f', 3), QString::number(strength * 0.08, 'f', 3));
}

// Brings a deep link's target into view and outlines the card it meant. All of
// it is deferred because setTab() runs before the dialog is shown, when nothing
// has been laid out yet.
void dlgProfilePreferences::spotlight(QWidget* pTarget)
{
    if (!pTarget) {
        return;
    }
    QTimer::singleShot(0, this, [this, pTarget = QPointer<QWidget>(pTarget)]() {
        if (!pTarget) {
            return;
        }
        auto* pScrollArea = qobject_cast<QScrollArea*>(mpStackedWidget_categories->currentWidget());
        QWidget* pColumn = pScrollArea ? pScrollArea->widget() : nullptr;
        if (!pColumn || !pColumn->isAncestorOf(pTarget)) {
            return;
        }
        // The y margin is how much of the viewport to keep around the widget,
        // so half a viewport is as close to centred as the page allows:
        pScrollArea->ensureWidgetVisible(pTarget, 0, pScrollArea->viewport()->height() / 2);

        // Only ever one pulse at a time - a second deep link supersedes the
        // first rather than fading on top of it:
        delete mpWidget_spotlight.data();
        const QColor accent = palette().color(QPalette::Highlight);
        auto* pPulse = new QWidget(pColumn);
        mpWidget_spotlight = pPulse;
        pPulse->setObjectName(qsl("settingsSpotlight"));
        pPulse->setAttribute(Qt::WA_TransparentForMouseEvents);
        pPulse->setAttribute(Qt::WA_StyledBackground);
        // A card fills the column's whole width, so an outline drawn around the
        // outside of one would be clipped away by the column on three sides -
        // hence the intersection, which lands those edges on the card instead:
        const QRect targetRect(pTarget->mapTo(pColumn, QPoint(0, 0)), pTarget->size());
        pPulse->setGeometry(targetRect.adjusted(-2, -2, 2, 2).intersected(pColumn->rect()));
        // Painted at full strength before the animation is started: the
        // animation only ever takes the pulse away, and its first tick can be a
        // long time coming when the event loop is busy - which is exactly when
        // a deep link opens the dialog.
        pPulse->setStyleSheet(spotlightStyleSheet(accent, 1.0));
        pPulse->raise();
        pPulse->show();

        auto* pAnimation = new QVariantAnimation(pPulse);
        pAnimation->setDuration(2500);
        pAnimation->setKeyValueAt(0.0, 1.0);
        pAnimation->setKeyValueAt(0.8, 1.0);
        pAnimation->setKeyValueAt(1.0, 0.0);
        connect(pAnimation, &QVariantAnimation::valueChanged, pPulse, [pPulse, accent](const QVariant& value) {
            pPulse->setStyleSheet(spotlightStyleSheet(accent, value.toReal()));
        });
        connect(pAnimation, &QVariantAnimation::finished, pPulse, &QWidget::deleteLater);
        pAnimation->start(QAbstractAnimation::DeleteWhenStopped);
    });
}

// Search text is compared with the rich text of tooltips, the & of keyboard
// accelerators, accents and case all folded away, so that "fonte" finds
// "Fonté" and "save" finds "&Save".
static QString foldForSearch(const QString& text)
{
    QString plain;
    plain.reserve(text.size());
    bool inTag = false;
    for (const QChar character : text) {
        if (character == QLatin1Char('<')) {
            inTag = true;
        } else if (character == QLatin1Char('>')) {
            inTag = false;
        } else if (!inTag && character != QLatin1Char('&')) {
            plain.append(character);
        }
    }

    const QString decomposed = plain.normalized(QString::NormalizationForm_KD);
    QString folded;
    folded.reserve(decomposed.size());
    for (const QChar character : decomposed) {
        if (character.category() != QChar::Mark_NonSpacing) {
            folded.append(character);
        }
    }
    return folded.simplified().toCaseFolded();
}

// Everything one widget contributes to the text of the card it sits on: what it
// shows, what its tooltip says, and any synonyms it was given that are
// deliberately not shown anywhere.
static void collectSearchText(const QWidget* pWidget, QStringList& parts)
{
    parts << pWidget->property("searchKeywords").toString() << pWidget->toolTip();
    if (const auto* pLabel = qobject_cast<const QLabel*>(pWidget); pLabel) {
        parts << pLabel->text();
    } else if (const auto* pGroupBox = qobject_cast<const QGroupBox*>(pWidget); pGroupBox) {
        parts << pGroupBox->title();
    } else if (const auto* pButton = qobject_cast<const QAbstractButton*>(pWidget); pButton) {
        parts << pButton->text();
    } else if (const auto* pComboBox = qobject_cast<const QComboBox*>(pWidget); pComboBox) {
        // ...but not what a font picker lists: those are the fonts installed on
        // this machine rather than anything the settings say, and they turn any
        // card holding one into a result for words like "color" or "mono".
        if (qobject_cast<const QFontComboBox*>(pWidget)) {
            return;
        }
        for (int i = 0, total = pComboBox->count(); i < total; ++i) {
            parts << pComboBox->itemText(i);
        }
    }
}

// Synonyms count as highlight text too: a card found by a keyword should still
// show which of its controls carries that keyword.
static QString highlightTextOf(const QWidget* pWidget)
{
    QString text;
    if (const auto* pLabel = qobject_cast<const QLabel*>(pWidget); pLabel) {
        text = pLabel->text();
    } else if (const auto* pGroupBox = qobject_cast<const QGroupBox*>(pWidget); pGroupBox) {
        text = pGroupBox->title();
    } else if (const auto* pButton = qobject_cast<const QAbstractButton*>(pWidget); pButton) {
        text = pButton->text();
    } else {
        return QString();
    }
    const QString keywords = pWidget->property("searchKeywords").toString();
    return keywords.isEmpty() ? text : qsl("%1 %2").arg(text, keywords);
}

// Built on the first search rather than up front, and walked off the real
// widget tree rather than kept as a hand-written list, so that a control added
// to the .ui file later is searchable without anyone remembering to say so.
// Sidebar order in, sidebar order out.
void dlgProfilePreferences::buildSearchIndex()
{
    mSearchCards.clear();
    for (int row = 0, rows = mpListWidget_categories->count(); row < rows; ++row) {
        const QString key = mpListWidget_categories->item(row)->data(Qt::UserRole).toString();
        if (key.isEmpty()) {
            continue;
        }
        auto* pScrollArea = qobject_cast<QScrollArea*>(mpStackedWidget_categories->widget(mCategoryPageIndexes.value(key, -1)));
        auto* pColumnLayout = pScrollArea ? qobject_cast<QVBoxLayout*>(pScrollArea->widget()->layout()) : nullptr;
        if (!pColumnLayout) {
            continue;
        }
        for (int item = 0, items = pColumnLayout->count(); item < items; ++item) {
            // The migration banner is not on any page while this runs, so
            // every widget a column holds here is a card - see placeBannerOn()
            QWidget* pCard = pColumnLayout->itemAt(item)->widget();
            if (!pCard) {
                continue;
            }
            QStringList parts;
            collectSearchText(pCard, parts);
            for (const auto* pChild : pCard->findChildren<QWidget*>()) {
                collectSearchText(pChild, parts);
            }

            SearchCard entry;
            entry.pCard = pCard;
            entry.categoryKey = key;
            entry.text = foldForSearch(parts.join(QLatin1Char(' ')));
            entry.pHomeLayout = pColumnLayout;
            entry.homeIndex = item;
            mSearchCards.append(entry);
        }
    }
}

void dlgProfilePreferences::runSearch(const QString& query)
{
    const QStringList needles = foldForSearch(query).split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (needles.isEmpty()) {
        exitSearchMode();
        return;
    }
    // Before the index is built and before any card is lent out, because a
    // banner at the top of a page shifts every card on it by one place
    placeBannerOn(nullptr);
    if (mSearchCards.isEmpty()) {
        buildSearchIndex();
    }
    if (!mSearchActive) {
        mSearchActive = true;
        const QListWidgetItem* pCurrent = mpListWidget_categories->currentItem();
        mCategoryBeforeSearch = pCurrent ? pCurrent->data(Qt::UserRole).toString() : QString();
        // Only the selection goes, never the current row: an item view answers
        // a focus-in that finds no current index by taking the first one, which
        // reads as the user having chosen General and ends the search. That
        // focus arrives whenever a card the results borrowed is handed back
        // with one of its controls holding it, because reparenting a widget
        // clears the focus it is carrying.
        const QSignalBlocker blocker(mpListWidget_categories);
        mpListWidget_categories->clearSelection();
    }

    setUpdatesEnabled(false);
    // Every keystroke starts from the pages as they really are, so the results
    // can never accumulate a card twice or lose one:
    returnSearchedCardsHome();
    clearSearchHighlights();

    // Moving a card fires no change signal of its own, but the control that had
    // the keyboard focus reports that its editing finished as focus leaves it:
    const bool wasPopulating = mPopulating;
    mPopulating = true;

    QString lastCategory;
    int matchCount = 0;
    for (auto& entry : mSearchCards) {
        // A card the profile's state has hidden - the updater's, Discord's - is
        // not an option anyone can take up, so it is not a result either:
        if (!entry.pCard || entry.pCard->isHidden()) {
            continue;
        }
        bool matched = true;
        for (const QString& needle : needles) {
            if (!entry.text.contains(needle)) {
                matched = false;
                break;
            }
        }
        if (!matched) {
            continue;
        }

        if (entry.categoryKey != lastCategory) {
            lastCategory = entry.categoryKey;
            QLabel* pHeader = searchCategoryHeader(entry.categoryKey);
            mpLayout_searchResults->insertWidget(mpLayout_searchResults->count() - 1, pHeader);
            pHeader->show();
        }
        // Not that it is still at homeIndex - taking an earlier card from the
        // same page shifts the ones below it - but that this is still the
        // layout it goes back to: a card that has drifted to another one would
        // be stranded off its own page for good.
        Q_ASSERT_X(entry.pHomeLayout->indexOf(entry.pCard) >= 0, "dlgProfilePreferences::runSearch", "a card is no longer in the layout the search index recorded it under");
        entry.pHomeLayout->removeWidget(entry.pCard);
        mpLayout_searchResults->insertWidget(mpLayout_searchResults->count() - 1, entry.pCard);
        // Reparenting hides a widget, and the results page may not even be the
        // one showing yet, so nothing here can be left to the layout to do:
        entry.pCard->show();
        entry.onResultsPage = true;
        highlightMatches(entry.pCard, needles);
        ++matchCount;
    }

    mPopulating = wasPopulating;

    if (!matchCount) {
        // Rich text, so the query has to be escaped before it can be pasted in
        //: Empty state of the settings search; %1 is what the user typed
        const QString message = tr("No results in settings for \"%1\"").arg(query.trimmed().toHtmlEscaped());
        //: Offered under the settings search empty state; %1 is a link labelled "Mudlet support"
        const QString help = tr("Need help? Visit %1").arg(qsl("<a href=\"%1\">%2</a>").arg(mpItem_support->data(Qt::UserRole + 1).toString(), mpItem_support->text()));
        mpLabel_searchEmpty->setText(qsl("%1<br>%2").arg(message, help));
    }
    mpLabel_searchEmpty->setVisible(!matchCount);
    mpLayout_searchResults->setStretch(0, matchCount ? 0 : 1);
    //: Title shown in place of a category name while the settings search is showing its results
    mpLabel_pageTitle->setText(tr("Search results"));
    // The back chevron stands where the category icon does on every other page,
    // so the icon's placeholder goes rather than leaving a gap between the two
    mpLabel_pageTitleIcon->hide();
    mpButton_searchBack->show();
    mpStackedWidget_categories->setCurrentIndex(mSearchResultsPageIndex);
    // As on a category page, a card needing more than the reading width gets it:
    capColumnWidth(mpScrollArea_searchResults);
    setUpdatesEnabled(true);
}

// The index is in ascending position order per page, and the cards that never
// left kept their order, so inserting each one back at the position it was
// indexed at puts every page back exactly as it was.
void dlgProfilePreferences::returnSearchedCardsHome()
{
    const bool wasPopulating = mPopulating;
    mPopulating = true;
    for (auto& entry : mSearchCards) {
        if (!entry.onResultsPage) {
            continue;
        }
        entry.onResultsPage = false;
        if (!entry.pCard || !entry.pHomeLayout) {
            continue;
        }
        mpLayout_searchResults->removeWidget(entry.pCard);
        entry.pHomeLayout->insertWidget(entry.homeIndex, entry.pCard);
        entry.pCard->show();
    }
    for (auto* pHeader : std::as_const(mSearchCategoryHeaders)) {
        mpLayout_searchResults->removeWidget(pHeader);
        pHeader->hide();
    }
    mPopulating = wasPopulating;
}

// A change underneath the search - a profile arriving or leaving, a language
// change - invalidates the index. The field is cleared before the index because
// clearing the field is what sends every borrowed card home, and that needs the
// index to still say where home is.
void dlgProfilePreferences::invalidateSearch()
{
    mpLineEdit_search->clear();
    mSearchCards.clear();
}

void dlgProfilePreferences::exitSearchMode()
{
    if (!mSearchActive) {
        return;
    }
    mSearchActive = false;
    setUpdatesEnabled(false);
    returnSearchedCardsHome();
    clearSearchHighlights();
    mpButton_searchBack->hide();
    mpLabel_searchEmpty->hide();
    mpLayout_searchResults->setStretch(0, 0);

    const int row = mpListWidget_categories->currentRow();
    if (row < 0) {
        // The query was cleared rather than a category chosen, so the page that
        // the search interrupted comes back:
        mpListWidget_categories->setCurrentRow(mCategoryRows.value(mCategoryBeforeSearch, 0));
    } else {
        // Only the selection was taken away when the search began, so the row is
        // still current and setCurrentRow() would report no change:
        mpListWidget_categories->item(row)->setSelected(true);
        slot_categorySelected(row);
    }
    setUpdatesEnabled(true);
}

void dlgProfilePreferences::clearSearchHighlights()
{
    for (const auto& pWidget : std::as_const(mHighlightedWidgets)) {
        if (!pWidget) {
            continue;
        }
        pWidget->setProperty("searchMatch", QVariant());
        pWidget->style()->unpolish(pWidget);
        pWidget->style()->polish(pWidget);
        pWidget->update();
    }
    mHighlightedWidgets.clear();
}

// The property is what the shell stylesheet paints the soft highlight from
void dlgProfilePreferences::highlightMatches(QWidget* pCard, const QStringList& needles)
{
    QList<QWidget*> candidates = pCard->findChildren<QWidget*>();
    candidates.prepend(pCard);
    for (auto* pWidget : candidates) {
        const QString text = highlightTextOf(pWidget);
        if (text.isEmpty()) {
            continue;
        }
        const QString folded = foldForSearch(text);
        bool matched = false;
        for (const QString& needle : needles) {
            if (folded.contains(needle)) {
                matched = true;
                break;
            }
        }
        if (!matched) {
            continue;
        }
        pWidget->setProperty("searchMatch", true);
        pWidget->style()->unpolish(pWidget);
        pWidget->style()->polish(pWidget);
        pWidget->update();
        mHighlightedWidgets.append(pWidget);
    }
}

QLabel* dlgProfilePreferences::searchCategoryHeader(const QString& key)
{
    QLabel* pHeader = mSearchCategoryHeaders.value(key, nullptr);
    if (!pHeader) {
        pHeader = new QLabel(mpScrollArea_searchResults->widget());
        pHeader->setObjectName(qsl("settingsSearchHeader"));
        // The icon rides in the text as rich text, which is the only way one
        // label draws a picture and a word side by side
        pHeader->setTextFormat(Qt::RichText);
        pHeader->hide();
        mSearchCategoryHeaders.insert(key, pHeader);
    }
    // Set every time rather than once: the header says what the sidebar says,
    // and a language change replaces that under a header this map is holding
    const QListWidgetItem* pItem = mpListWidget_categories->item(mCategoryRows.value(key, -1));
    const QString name = (pItem ? pItem->text() : key).toHtmlEscaped();
    const QString iconFile = mCategoryIconFiles.value(key);
    // The same icon the sidebar row carries, so a result is tied back to where
    // it lives by more than its name
    pHeader->setText(iconFile.isEmpty() ? name : qsl("<img src=\":/icons/%1\" width=\"18\" height=\"18\">&nbsp;%2").arg(iconFile, name));
    return pHeader;
}

void dlgProfilePreferences::slot_categorySelected(const int row)
{
    QListWidgetItem* pItem = mpListWidget_categories->item(row);
    if (!pItem) {
        return;
    }
    const QString key = pItem->data(Qt::UserRole).toString();
    if (key.isEmpty()) {
        return;
    }
    if (mSearchActive) {
        // Picking a category is one of the ways out of the results, and
        // clearing the field is what puts every borrowed card back:
        mpLineEdit_search->clear();
    }
    // QStackedLayout hands the keyboard focus from the outgoing page to the
    // incoming one, and taking it off a control the page has scrolled out of
    // sight scrolls that page back to the top on the way past. A sidebar click
    // has already parked the focus on the list; a deep link arriving mid-edit
    // has not.
    QWidget* pCurrentPage = mpStackedWidget_categories->currentWidget();
    if (QWidget* pFocus = QApplication::focusWidget(); pFocus && pCurrentPage && pCurrentPage->isAncestorOf(pFocus)) {
        mpListWidget_categories->setFocus(Qt::OtherFocusReason);
    }
    mpStackedWidget_categories->setCurrentIndex(mCategoryPageIndexes.value(key, mCategoryPageIndexes.value(qsl("general"))));
    auto* pShownPage = qobject_cast<QScrollArea*>(mpStackedWidget_categories->currentWidget());
    // Before the width is capped, so that what is measured is the page as it
    // will be shown
    placeBannerOn(pShownPage ? pShownPage->widget() : nullptr);
    capColumnWidth(pShownPage);
    // A card's padding arrives with the stylesheet, which is applied as the page
    // is first shown - after the cap above has measured it without. Left there,
    // a page whose cards need more than the reading width is capped 34px short
    // of what its first paint asks for and clips the difference.
    QTimer::singleShot(0, this, [this, pShownPage]() {
        if (pShownPage && mpStackedWidget_categories->currentWidget() == pShownPage) {
            capColumnWidth(pShownPage);
        }
    });
    mpLabel_pageTitle->setText(pItem->text());
    mpLabel_pageTitleIcon->setPixmap(pItem->icon().pixmap(QSize(20, 20), devicePixelRatioF()));
    mpLabel_pageTitleIcon->show();

    if (key == qsl("editor") && !mEditorThemesChecked) {
        mEditorThemesChecked = true;
        maybeDownloadEditorThemes();
    }
}

void dlgProfilePreferences::slot_sidebarItemClicked(QListWidgetItem* pItem)
{
    const QString url = pItem ? pItem->data(Qt::UserRole + 1).toString() : QString();
    if (!url.isEmpty()) {
        QDesktopServices::openUrl(QUrl(url));
    }
}

// Every surface below is blended from the palette rather than written out as
// hex, so the shell follows whichever theme it is handed without needing a
// second stylesheet.
static QColor blend(const QColor& from, const QColor& to, const qreal amount)
{
    return QColor::fromRgbF(from.redF() + (to.redF() - from.redF()) * amount, from.greenF() + (to.greenF() - from.greenF()) * amount, from.blueF() + (to.blueF() - from.blueF()) * amount);
}

static QString rgba(const QColor& color, const qreal alpha)
{
    return qsl("rgba(%1, %2, %3, %4)").arg(QString::number(color.red()), QString::number(color.green()), QString::number(color.blue()), QString::number(alpha, 'f', 3));
}

// The shell's own look, kept on the shell widget rather than on the dialog:
// mudlet::showOptionsDialog() assigns the profile's Lua stylesheet to the
// dialog on every show, which would otherwise throw this away. Every selector
// is scoped by objectName or by the card property so a profile stylesheet still
// reaches the controls inside the pages.
void dlgProfilePreferences::applyShellStyle()
{
    if (!mpWidget_shell) {
        return;
    }
    const QPalette dialogPalette = palette();
    const QColor cardColor = dialogPalette.color(QPalette::Base);
    const QColor textColor = dialogPalette.color(QPalette::WindowText);
    const QColor accentColor = dialogPalette.color(QPalette::Highlight);
    // Read off the palette rather than from mudlet::inDarkMode(), so that a dark
    // system theme under the "follow the system" appearance gets the dark
    // treatment as well as Mudlet's own dark mode does
    const bool darkPage = cardColor.lightness() < 128;

    // Everything is measured from the card colour and the text colour, the one
    // pair a palette must keep apart to be usable at all. Mudlet's own light
    // appearance has window, base and mid within three levels of each other, so
    // a page or a border mixed from those comes out invisible.
    const QColor pageColor = blend(cardColor, QColor(Qt::black), darkPage ? 0.35 : 0.04);
    const QColor borderColor = blend(cardColor, textColor, darkPage ? 0.28 : 0.16);
    const QString hoverSoft = rgba(textColor, 0.07);
    const QString accentSoft = rgba(accentColor, 0.14);
    // Accent-coloured text has to hold its own against the page it is on, and a
    // saturated highlight colour rarely does at both ends
    const QColor accentText = darkPage ? blend(accentColor, QColor(Qt::white), 0.45) : blend(accentColor, QColor(Qt::black), 0.2);
    const QColor mutedText = blend(cardColor, textColor, 0.7);
    // A search hit keeps Firefox's yellow marker pen, but its lightness is
    // chosen for the page it lies on: an opaque pale wash under dark text, a
    // darker translucent one that light text still shows through
    const QColor markerColor = QColor::fromHslF(0.13, 0.9, darkPage ? 0.34 : 0.72);
    const QString markerSoft = rgba(markerColor, darkPage ? 0.75 : 0.95);
    const QColor scrollHandle = blend(pageColor, textColor, 0.22);
    const QColor scrollHandleHover = blend(pageColor, textColor, 0.40);

    if (mpAction_searchIcon) {
        // The one glyph the icon resource has for searching is the editor's
        // binoculars, which is monochrome - so it can simply be recoloured to
        // whatever the placeholder text beside it is using
        QPixmap glyph(qsl(":/icons/searchOptions-none.png"));
        QPainter painter(&glyph);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(glyph.rect(), mutedText);
        painter.end();
        mpAction_searchIcon->setIcon(QIcon(glyph));
    }

    // A border-left accent bar is drawn as an arc where the pill's corner radius
    // is, which pinches the bar to nothing at both ends and leaves a notch of
    // page colour outside it. A gradient is clipped by the radius instead, so
    // the bar keeps its width and takes the pill's own rounded corners.
    const qreal accentBarStop = static_cast<qreal>(scmSidebarAccentBarWidth) / (scmSidebarWidth - 2 * scmSidebarPadding);

    // Fusion draws a group box's check indicator from palette(window) darkened
    // by 40%, which on a dark card is a 1.1:1 outline - the one control whose
    // contrast the palette pass below cannot rescue, because a stylesheet
    // background-color lands on the same role and would take the card's title
    // band with it. Drawn from the stylesheet instead, its outline is named
    // outright, and the checked state has to be drawn out in full because a
    // styled indicator gets no check mark of its own.
    const QColor indicatorOutline = blend(cardColor, textColor, darkPage ? 0.55 : 0.45);
    const QString cardIndicatorRules = qsl("QGroupBox[settingsCard=\"true\"]::indicator { width: %1px; height: %1px; border: 1px solid %2; border-radius: 3px; background-color: %3; }"
                                           "QGroupBox[settingsCard=\"true\"]::indicator:hover { border: 1px solid %4; }"
                                           // The check mark is a fixed green rather than anything drawn
                                           // from the accent, so the fill it lands on has to be the card
                                           // and not the accent: a profile whose highlight colour is
                                           // orange would otherwise put green on orange
                                           "QGroupBox[settingsCard=\"true\"]::indicator:checked { border: 1px solid %4; image: url(:/icons/dialog-ok-apply_small.png); }"
                                           // ...and the other half of lining the titles up: a plain card's
                                           // title starts where a checkable one's indicator does
                                           "QGroupBox[settingsCardTitleInset=\"true\"]::title { left: %5px; }")
                                               .arg(QString::number(scmCardIndicatorSize), indicatorOutline.name(), cardColor.name(), accentColor.name(), QString::number(scmCardTitleInset));

    mpWidget_shell->setStyleSheet(qsl("#settingsShell, #settingsSidebar, #settingsContent { background-color: %1; }"
                                      // The view's own selection paint has to be turned off, or the
                                      // platform style draws it over the text of the item as a square
                                      // box inside the rounded pill the rules below draw:
                                      "#settingsCategoryList { background: transparent; border: none; outline: none; show-decoration-selected: 1;"
                                      " selection-background-color: transparent; selection-color: %6; }"
                                      // The transparent left border on every item is what keeps the
                                      // text of the selected one from stepping sideways under its
                                      // accent bar; outline:none drops the native focus rectangle,
                                      // which is drawn square and inset from the pill it lands on.
                                      "#settingsCategoryList::item { border-radius: 8px; border-left: 3px solid transparent; padding-left: 7px; color: %2; outline: none; }"
                                      "#settingsCategoryList::item:hover { background-color: %3; }"
                                      "#settingsCategoryList::item:selected { color: %6; font-weight: bold; background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
                                      " stop:0 %5, stop:%11 %5, stop:%12 %4, stop:1 %4); }"
                                      // Keyboard focus on the sidebar is otherwise indistinguishable
                                      // from the selection it already draws; eventFilter() is what
                                      // puts the property on:
                                      "#settingsCategoryList[settingsFocused=\"true\"]::item:selected { border: 1px solid %5; border-left: 3px solid %5; padding-left: 5px; }"
                                      "#settingsSidebarSeparator { border: none; background-color: %7; margin: 8px 16px; }"
                                      "#settingsStack { background: transparent; }"
                                      // The pages, their viewports and their columns are the shell's
                                      // own surfaces rather than page content, so they keep the page
                                      // colour even when a profile stylesheet paints every QWidget it
                                      // can reach. Painted rather than left transparent, because a
                                      // transparent surface falls back to its palette colour - which
                                      // is what that stylesheet has just changed.
                                      "QWidget[settingsSurface=\"true\"] { background-color: %1; border: none; }"
                                      // A scroll area's bars answer only to a descendant selector, not
                                      // to the child combinator:
                                      "QScrollArea[settingsSurface=\"true\"] QScrollBar:vertical, #settingsCategoryList QScrollBar:vertical"
                                      " { background-color: %1; width: 12px; margin: 0px; border: none; }"
                                      "QScrollArea[settingsSurface=\"true\"] QScrollBar:horizontal"
                                      " { background-color: %1; height: 12px; margin: 0px; border: none; }"
                                      "QScrollArea[settingsSurface=\"true\"] QScrollBar::handle:vertical, #settingsCategoryList QScrollBar::handle:vertical"
                                      " { background-color: %13; border-radius: 5px; min-height: 32px; margin: 1px; }"
                                      "QScrollArea[settingsSurface=\"true\"] QScrollBar::handle:horizontal"
                                      " { background-color: %13; border-radius: 5px; min-width: 32px; margin: 1px; }"
                                      "QScrollArea[settingsSurface=\"true\"] QScrollBar::handle:hover, #settingsCategoryList QScrollBar::handle:hover"
                                      " { background-color: %14; }"
                                      "QScrollArea[settingsSurface=\"true\"] QScrollBar::add-line, QScrollArea[settingsSurface=\"true\"] QScrollBar::sub-line,"
                                      "#settingsCategoryList QScrollBar::add-line, #settingsCategoryList QScrollBar::sub-line"
                                      " { width: 0px; height: 0px; }"
                                      "QScrollArea[settingsSurface=\"true\"] QScrollBar::add-page, QScrollArea[settingsSurface=\"true\"] QScrollBar::sub-page,"
                                      "#settingsCategoryList QScrollBar::add-page, #settingsCategoryList QScrollBar::sub-page"
                                      " { background-color: %1; }"
                                      "#settingsWordmark { font-weight: bold; font-size: 125%; }"
                                      "#settingsPageTitle { font-weight: bold; font-size: 145%; }"
                                      "#settingsSearchField { border: 1px solid %7; border-radius: 8px; padding-left: 6px; background-color: %8; }"
                                      "#settingsSearchField:focus { border: 1px solid %5; }"
                                      // The top margin lifts the title clear of the frame, so the card
                                      // is a rounded rectangle with a heading above it rather than a
                                      // box with its own title cutting through its border:
                                      "QGroupBox[settingsCard=\"true\"] { background-color: %8; border: 1px solid %7; border-radius: 8px; margin-top: 24px; padding: 16px; font-weight: bold; }"
                                      "QGroupBox[settingsCard=\"true\"]::title { subcontrol-origin: margin; subcontrol-position: top left; left: 0px; padding: 0px; }"
                                      // ...but only the title is bold, not everything the card holds:
                                      "QGroupBox[settingsCard=\"true\"] > * { font-weight: normal; }"
                                      // A card carrying a single option needs no heading, and without
                                      // one it needs no room above the frame for it either:
                                      "QGroupBox[settingsCardPlain=\"true\"] { margin-top: 0px; }"
                                      // Group boxes the .ui file nests inside what is now a card draw
                                      // a second frame within the first; a heading alone divides
                                      // them, in a top margin deep enough to draw that heading in:
                                      "QGroupBox[settingsCard=\"true\"] QGroupBox { border: none; background: transparent; margin-top: 20px; padding: 0px 0px 0px 8px; font-weight: bold; }"
                                      "QGroupBox[settingsCard=\"true\"] QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 0px; padding: 0px; }"
                                      "#settingsMigrationBanner { background-color: %4; border: 1px solid %7; border-radius: 8px; }"
                                      "#settingsMigrationBannerTitle { font-weight: bold; }"
                                      "#settingsSearchHeader { font-weight: bold; font-size: 110%; color: %2; }"
                                      "#settingsSearchEmpty { padding: 32px; color: %9; }"
                                      // The property is put on and taken off by the search itself:
                                      "QLabel[searchMatch=\"true\"], QCheckBox[searchMatch=\"true\"], QRadioButton[searchMatch=\"true\"], QPushButton[searchMatch=\"true\"]"
                                      " { background-color: %10; border-radius: 3px; }"
                                      "QGroupBox[searchMatch=\"true\"]::title { background-color: %10; border-radius: 3px; }"
                                      // Only ever seen beside the "Search results" title, so it is
                                      // drawn as a piece of that heading rather than as a button
                                      "#settingsSearchBack { border: 1px solid transparent; border-radius: 6px; padding: 2px 6px; color: %2; background: transparent; }"
                                      "#settingsSearchBack:hover { background-color: %3; }"
                                      "#settingsSearchBack:focus { border: 1px solid %5; }")
                                          .arg(pageColor.name(), textColor.name(), hoverSoft, accentSoft, accentColor.name(), accentText.name(), borderColor.name(), cardColor.name(), mutedText.name())
                                          .arg(markerSoft, QString::number(accentBarStop, 'f', 5), QString::number(accentBarStop + 0.0001, 'f', 5), scrollHandle.name(), scrollHandleHover.name())
                                  + cardIndicatorRules);

    // Fusion draws every control outline - checkbox and radio indicators
    // included - as palette(window) darkened by 40%, which in the dark theme
    // lands within 1.1:1 of a card. Nothing in the shell paints with that role,
    // every surface above coming from the stylesheet, so raising it costs
    // nothing. Per control, because a stylesheet freezes the palette of every
    // widget it polishes - and after the stylesheet, because assigning one
    // re-polishes the subtree back to the palette each widget was first
    // polished with.
    const QColor controlOutlineSource = darkPage ? blend(cardColor, textColor, 0.55) : dialogPalette.color(QPalette::Window);
    const QColor placeholderText = blend(cardColor, textColor, 0.45);
    for (auto* pControl : mpWidget_shell->findChildren<QWidget*>()) {
        if (!qobject_cast<QAbstractButton*>(pControl) && !qobject_cast<QLineEdit*>(pControl) && !qobject_cast<QAbstractSpinBox*>(pControl) && !qobject_cast<QComboBox*>(pControl)) {
            continue;
        }
        QPalette controlPalette = pControl->palette();
        controlPalette.setColor(QPalette::Window, controlOutlineSource);
        // The dark theme leaves PlaceholderText at the light default, so a
        // placeholder is drawn all but black on a dark field
        controlPalette.setColor(QPalette::PlaceholderText, placeholderText);
        pControl->setPalette(controlPalette);
    }

    // A rich-text anchor takes its colour from the palette rather than from the
    // stylesheet, and the theme's default is not chosen against a card
    QPalette emptyStatePalette = mpLabel_searchEmpty->palette();
    emptyStatePalette.setColor(QPalette::Link, accentText);
    mpLabel_searchEmpty->setPalette(emptyStatePalette);
}

// Controls are found by type rather than listed by hand, since a list would
// silently miss whatever gets added to the .ui file next. Qt::UniqueConnection
// makes this safe to call again once a profile appears and brings its own
// controls with it. Line edits deliberately report on editingFinished rather
// than per keystroke, and push buttons are left out: what they do, they already
// do for themselves.
void dlgProfilePreferences::connectApplyTriggers()
{
    for (auto* pButton : findChildren<QAbstractButton*>()) {
        // Check boxes are left to the loop below, whose signal says everything
        // toggled() does and more:
        if (qobject_cast<QPushButton*>(pButton) || qobject_cast<QToolButton*>(pButton) || qobject_cast<QCheckBox*>(pButton)) {
            continue;
        }
        connect(pButton, &QAbstractButton::toggled, this, &dlgProfilePreferences::slot_scheduleApply, Qt::UniqueConnection);
    }
    for (auto* pCheckBox : findChildren<QCheckBox*>()) {
        // toggled() says nothing about a tri-state box moving between its
        // partially checked and checked states:
        connect(pCheckBox, &QCheckBox::checkStateChanged, this, &dlgProfilePreferences::slot_scheduleApply, Qt::UniqueConnection);
    }
    for (auto* pGroupBox : findChildren<QGroupBox*>()) {
        if (pGroupBox->isCheckable()) {
            connect(pGroupBox, &QGroupBox::toggled, this, &dlgProfilePreferences::slot_scheduleApply, Qt::UniqueConnection);
        }
    }
    for (auto* pComboBox : findChildren<QComboBox*>()) {
        connect(pComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &dlgProfilePreferences::slot_scheduleApply, Qt::UniqueConnection);
    }
    for (auto* pSpinBox : findChildren<QSpinBox*>()) {
        connect(pSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &dlgProfilePreferences::slot_scheduleApply, Qt::UniqueConnection);
    }
    for (auto* pDoubleSpinBox : findChildren<QDoubleSpinBox*>()) {
        connect(pDoubleSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &dlgProfilePreferences::slot_scheduleApply, Qt::UniqueConnection);
    }
    for (auto* pDateTimeEdit : findChildren<QDateTimeEdit*>()) {
        connect(pDateTimeEdit, &QDateTimeEdit::dateTimeChanged, this, &dlgProfilePreferences::slot_scheduleApply, Qt::UniqueConnection);
    }
    for (auto* pLineEdit : findChildren<QLineEdit*>()) {
        if (pLineEdit == mpLineEdit_search) {
            continue;
        }
        connect(pLineEdit, &QLineEdit::editingFinished, this, &dlgProfilePreferences::slot_lineEditFinished, Qt::UniqueConnection);
    }
    // The telnet protocols are the one setting that is edited through a menu
    // rather than a control on a page:
    if (protocolMenu) {
        for (auto* pAction : protocolMenu->actions()) {
            if (pAction->isCheckable()) {
                connect(pAction, &QAction::toggled, this, &dlgProfilePreferences::slot_scheduleApply, Qt::UniqueConnection);
            }
        }
    }
}

// An invalid QVariant for anything holding no value a setting is written from.
// The types are the ones connectApplyTriggers() listens to, so everything able
// to schedule an apply can also be told apart from how it was populated.
static QVariant controlValue(const QObject* pControl)
{
    if (const auto* pGroupBox = qobject_cast<const QGroupBox*>(pControl)) {
        return pGroupBox->isCheckable() ? QVariant(pGroupBox->isChecked()) : QVariant();
    }
    if (const auto* pCheckBox = qobject_cast<const QCheckBox*>(pControl)) {
        // Tri-state boxes have three of them, so the check state and not just
        // whether it is the checked one:
        return QVariant::fromValue(pCheckBox->checkState());
    }
    if (const auto* pButton = qobject_cast<const QAbstractButton*>(pControl)) {
        if (qobject_cast<const QPushButton*>(pControl) || qobject_cast<const QToolButton*>(pControl)) {
            return {};
        }
        return pButton->isChecked();
    }
    if (const auto* pFontComboBox = qobject_cast<const QFontComboBox*>(pControl)) {
        return pFontComboBox->currentFont();
    }
    if (const auto* pComboBox = qobject_cast<const QComboBox*>(pControl)) {
        return pComboBox->currentIndex();
    }
    if (const auto* pSpinBox = qobject_cast<const QSpinBox*>(pControl)) {
        return pSpinBox->value();
    }
    if (const auto* pDoubleSpinBox = qobject_cast<const QDoubleSpinBox*>(pControl)) {
        return pDoubleSpinBox->value();
    }
    if (const auto* pDateTimeEdit = qobject_cast<const QDateTimeEdit*>(pControl)) {
        return pDateTimeEdit->dateTime();
    }
    if (const auto* pLineEdit = qobject_cast<const QLineEdit*>(pControl)) {
        return pLineEdit->text();
    }
    if (const auto* pAction = qobject_cast<const QAction*>(pControl)) {
        return pAction->isCheckable() ? QVariant(pAction->isChecked()) : QVariant();
    }
    return {};
}

// A line edit the user is in the middle of: Qt sets the modified flag on the
// first keystroke, and slot_lineEditFinished() clears it again once the edit is
// finished with. Until then what the field holds is half a word rather than a
// setting, so neither the apply nor the snapshot below takes it for one.
static bool beingTypedInto(const QObject* pControl)
{
    const auto* pLineEdit = qobject_cast<const QLineEdit*>(pControl);
    return pLineEdit && pLineEdit->hasFocus() && pLineEdit->isModified();
}

// Called once the controls hold what the settings say - after population, and
// after each apply - so that anything differing from this afterwards is the
// user's own edit.
void dlgProfilePreferences::snapshotValues()
{
    const QHash<const QObject*, QVariant> previous = mValueSnapshot;
    mValueSnapshot.clear();
    for (const auto* pWidget : findChildren<QWidget*>()) {
        const QVariant value = controlValue(pWidget);
        if (!value.isValid()) {
            continue;
        }
        // The apply this snapshot follows left a half-typed field alone, so the
        // value it was last populated with has to stand until that edit
        // finishes - or the apply which follows would find nothing to write.
        if (const auto it = previous.constFind(pWidget); beingTypedInto(pWidget) && it != previous.constEnd()) {
            mValueSnapshot.insert(pWidget, *it);
            continue;
        }
        mValueSnapshot.insert(pWidget, value);
    }
    if (protocolMenu) {
        for (const auto* pAction : protocolMenu->actions()) {
            const QVariant value = controlValue(pAction);
            if (value.isValid()) {
                mValueSnapshot.insert(pAction, value);
            }
        }
    }
    mShortcutsSnapshot = currentShortcuts;
}

bool dlgProfilePreferences::dirty(const QObject* pControl) const
{
    // The debounce is shared, so the apply about to read this was very likely
    // started by some other control's edit - no reason to commit a word someone
    // is halfway through typing:
    if (beingTypedInto(pControl)) {
        return false;
    }
    const auto it = mValueSnapshot.constFind(pControl);
    if (it == mValueSnapshot.constEnd()) {
        // A control that came into being after the last snapshot:
        return true;
    }
    return *it != controlValue(pControl);
}

// For a setting that is spread over several controls - the borders, the
// Discord privacy flags - one of them changing means the write happens. What is
// written is still composed control by control: a control that is not itself
// dirty contributes the value the Host holds at this moment rather than what it
// is showing, which may be a value a script has since moved on from (#10165).
// Where the group's members are separate settings rather than one composed
// value, each takes its own dirty() guard instead of appearing here.
bool dlgProfilePreferences::anyDirty(const QList<const QObject*>& controls) const
{
    for (const auto* pControl : controls) {
        if (dirty(pControl)) {
            return true;
        }
    }
    return false;
}

void dlgProfilePreferences::setupPasswordsMigration()
{
    hidePasswordMigrationLabelTimer = std::make_unique<QTimer>(this);
    hidePasswordMigrationLabelTimer->setSingleShot(true);

    connect(hidePasswordMigrationLabelTimer.get(), &QTimer::timeout, this, &dlgProfilePreferences::slot_hidePasswordMigrationLabel);

    connect(mudlet::self(), &mudlet::signal_passwordsMigratedToSecure, this, [=, this]() {
        label_password_migration_notification->setText(tr("Migrated all passwords to secure storage."));
        comboBox_store_passwords_in->setEnabled(true);
        hidePasswordMigrationLabelTimer->start(10s);
    });

    connect(mudlet::self(), &mudlet::signal_passwordMigratedToSecure, this, [=, this](const QString& profile) {
        //: This notifies the user that progress is being made on profile migration by saying what profile was just migrated to store passwords securely
        label_password_migration_notification->setText(tr("Migrated %1...").arg(profile));
    });

    connect(mudlet::self(), &mudlet::signal_passwordsMigratedToProfiles, this, [=, this]() {
        label_password_migration_notification->setText(tr("Migrated all passwords to profile storage."));
        comboBox_store_passwords_in->setEnabled(true);
        hidePasswordMigrationLabelTimer->start(10s);
    });

    if (mudlet::self()->storingPasswordsSecurely()) {
        comboBox_store_passwords_in->setCurrentIndex(0);
    } else {
        comboBox_store_passwords_in->setCurrentIndex(1);
    }

    connect(comboBox_store_passwords_in, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &dlgProfilePreferences::slot_passwordStorageLocationChanged);
}

void dlgProfilePreferences::disableHostDetails()
{
    // The Host pointer is a nullptr so disable every control that depends on it

    // Controls are (or should be) sorted by tab and then group in which they appear:
    // ===== tab_general =====
    // groupBox_iconsAndToolbars is NOT dependent on pHost - so leave it alone
    // ----- groupBox_encoding -----
    label_encoding->setEnabled(false);
    comboBox_encoding->setEnabled(false);

    // ----- groupBox_miscellaneous -----
    mAlertOnNewData->setEnabled(false);
    acceptServerGUI->setEnabled(false);
    mFORCE_SAVE_ON_EXIT->setEnabled(false);
    acceptServerMedia->setEnabled(false);

    // ----- groupBox_protocols -----
    groupBox_protocols->setEnabled(false);
    pushButton_chooseProtocols->setEnabled(false);
    need_reconnect_for_data_protocol->hide();

    // ----- groupBox_logOptions -----
    groupBox_logOptions->setEnabled(false);
    lineEdit_logFileName->setVisible(false);
    label_logFileName->setVisible(false);
    label_logFileNameExtension->setVisible(false);

    // ===== tab_inputLine =====
    groupBox_input->setEnabled(false);

    groupBox_spellCheck->setEnabled(false);

    // ===== tab_display =====
    groupBox_font->setEnabled(false);

    groupBox_borders->setEnabled(false);

    groupBox_wrapping->setEnabled(false);

    groupBox_consoleBuffer->setEnabled(false);

    // Some of groupBox_displayOptions are usable, so must pick out and
    // disable the others:
    // ----- groupBox_displayOptions -----
    checkBox_USE_IRE_DRIVER_BUGFIX->setEnabled(false);
    checkBox_enableTextAnalyzer->setEnabled(false);
    checkBox_echoLuaErrors->setEnabled(false);
    checkBox_useWideAmbiguousEastAsianGlyphs->setEnabled(false);
    label_controlCharacterHandling->setEnabled(false);
    comboBox_controlCharacterHandling->setEnabled(false);
    doubleclick_ignore_label->setEnabled(false);
    doubleclick_ignore_lineedit->setEnabled(false);
    checkBox_enableOSC8Hyperlinks->setEnabled(false);

    // ===== tab_codeEditor =====
    groupbox_codeEditorThemeSelection->setEnabled(false);
    // ----- groupbox_codeEditorThemeSelection -----
    theme_download_label->hide();

    groupBox_autoComplete->setEnabled(false);
    groupBox_editorDisplayOptions->setEnabled(false);

    // ===== tab_displayColors =====
    groupBox_displayColors->setEnabled(false);

    // ===== tab_mapper =====
    // most of groupBox_mapFiles is disabled but there is ONE checkBox that
    // is accessible because it is application wide - so disable EVERYTHING
    // else that is not already disabled:
    // ----- groupBox_mapFiles -----
    label_saveMap->setEnabled(false);
    pushButton_saveMap->setEnabled(false);
    label_loadMap->setEnabled(false);
    pushButton_loadMap->setEnabled(false);
    label_deleteMap->setEnabled(false);
    pushButton_deleteMap->setEnabled(false);
    label_copyMap->setEnabled(false);
    label_mapFileSaveFormatVersion->setEnabled(false);
    label_loadHistoricMap->setEnabled(false);
    comboBox_mapHistory->setEnabled(false);
    comboBox_mapHistory->clear();
    pushButton_loadHistoricMap->setEnabled(false);
    comboBox_mapFileSaveFormatVersion->setEnabled(false);
    comboBox_mapFileSaveFormatVersion->clear();
    label_mapFileActionResult->hide();

    // This is hidden until we have a valid map download location:
    groupBox_downloadMapOptions->setVisible(false);

    // ----- groupBox_mapViewOptions -----
    groupBox_mapViewOptions->setEnabled(false);

    // This is normally hidden until a map is loaded:
    checkBox_showDefaultArea->hide();

    // ===== tab_mapperColors =====
    groupBox_mapperColors->setEnabled(false);

    groupBox_playerRoomStyle->setEnabled(false);

    // ===== tab security =====
    groupBox_ssl->setEnabled(false);
    checkBox_askTlsAvailable->setEnabled(false);

    groupBox_discordPrivacy->hide();

    // ===== tab_shortcuts =====
    groupBox_main_window_shortcuts->setEnabled(false);

    // ===== tab_accessibility =====
    label_blankLinesBehaviour->setEnabled(false);
    label_caretModeKey->setEnabled(false);
    checkBox_announceIncomingText->setEnabled(false);
    checkBox_advertiseScreenReader->setEnabled(false);
    checkBox_enableClosedCaption->setEnabled(false);
    checkBox_enableBlinkText->setEnabled(false);
    comboBox_blankLinesBehaviour->setEnabled(false);
    comboBox_caretModeKey->setEnabled(false);

    // ===== tab_specialOptions =====
    groupBox_specialOptions->setEnabled(false);
    groupBox_purgeMediaCache->setEnabled(false);
    // ----- groupBox_specialOptions -----
    need_reconnect_for_specialoption->hide();

    groupbox_searchEngineSelection->setEnabled(false);
    // ----- groupBox_debug -----
    checkBox_expectCSpaceIdInColonLessMColorCode->setEnabled(false);
    // This acts on a label within this groupBox:
    slot_hidePasswordMigrationLabel();
    checkBox_debugShowAllCodepointProblems->setEnabled(false);
    widget_timerDebugOutputMinimumInterval->setEnabled(false);
    label_networkPacketTimeout->setEnabled(false);
    doubleSpinBox_networkPacketTimeout->setEnabled(false);
}

void dlgProfilePreferences::enableHostDetails()
{
    // ===== tab_general =====
    // ----- groupBox_encoding -----
    label_encoding->setEnabled(true);
    comboBox_encoding->setEnabled(true);

    // ----- groupBox_miscellaneous -----
    mAlertOnNewData->setEnabled(true);
    acceptServerGUI->setEnabled(true);
    mFORCE_SAVE_ON_EXIT->setEnabled(true);
    acceptServerMedia->setEnabled(true);

    groupBox_protocols->setEnabled(true);
    pushButton_chooseProtocols->setEnabled(true);

    groupBox_logOptions->setEnabled(true);

    // ===== tab_inputLine =====
    groupBox_input->setEnabled(true);

    groupBox_spellCheck->setEnabled(true);

    // ===== tab_display =====
    groupBox_font->setEnabled(true);

    groupBox_borders->setEnabled(true);

    groupBox_wrapping->setEnabled(true);

    groupBox_consoleBuffer->setEnabled(true);

    // ----- groupBox_displayOptions -----
    checkBox_USE_IRE_DRIVER_BUGFIX->setEnabled(true);
    checkBox_enableTextAnalyzer->setEnabled(true);
    checkBox_echoLuaErrors->setEnabled(true);
    checkBox_useWideAmbiguousEastAsianGlyphs->setEnabled(true);
    label_controlCharacterHandling->setEnabled(true);
    comboBox_controlCharacterHandling->setEnabled(true);
    doubleclick_ignore_label->setEnabled(true);
    doubleclick_ignore_lineedit->setEnabled(true);
    checkBox_enableOSC8Hyperlinks->setEnabled(true);

    // ===== tab_codeEditor =====
    groupbox_codeEditorThemeSelection->setEnabled(true);

    groupBox_autoComplete->setEnabled(true);
    groupBox_editorDisplayOptions->setEnabled(true);

    // ===== tab_displayColors =====
    groupBox_displayColors->setEnabled(true);

    // ===== tab_mapper =====
    // most of groupBox_mapFiles is disabled but there is ONE checkBox that
    // is accessible because it is application wide - so enable EVERYTHING
    // else:
    // ----- groupBox_mapFiles -----
    label_saveMap->setEnabled(true);
    pushButton_saveMap->setEnabled(true);
    label_loadMap->setEnabled(true);
    pushButton_loadMap->setEnabled(true);
    label_deleteMap->setEnabled(true);
    pushButton_deleteMap->setEnabled(true);
    label_copyMap->setEnabled(true);
    label_mapFileSaveFormatVersion->setEnabled(true);
    label_loadHistoricMap->setEnabled(true);
    comboBox_mapHistory->setEnabled(true);
    pushButton_loadHistoricMap->setEnabled(true);

    groupBox_mapViewOptions->setEnabled(true);

    // ===== tab_mapperColors =====
    groupBox_mapperColors->setEnabled(true);
    groupBox_playerRoomStyle->setEnabled(true);


    // ===== tab security =====
#if defined(QT_NO_SSL)
    groupBox_ssl->setEnabled(false);
    checkBox_askTlsAvailable->setEnabled(false);
#else
    groupBox_ssl->setEnabled(QSslSocket::supportsSsl());
    checkBox_askTlsAvailable->setEnabled(true);
#endif

    // ===== tab_chat =====
    groupBox_discordPrivacy->show();

    // ===== tab_shortcuts =====
    groupBox_main_window_shortcuts->setEnabled(true);

    // ===== tab_accessibility =====
    label_blankLinesBehaviour->setEnabled(true);
    label_caretModeKey->setEnabled(true);
    checkBox_announceIncomingText->setEnabled(true);
    checkBox_advertiseScreenReader->setEnabled(true);
    checkBox_enableClosedCaption->setEnabled(true);
    checkBox_enableBlinkText->setEnabled(true);
    comboBox_blankLinesBehaviour->setEnabled(true);
    comboBox_caretModeKey->setEnabled(true);

    // ===== tab_specialOptions =====
    groupBox_specialOptions->setEnabled(true);
    groupBox_purgeMediaCache->setEnabled(true);
    groupbox_searchEngineSelection->setEnabled(true);
    // ----- groupBox_debug -----
    checkBox_expectCSpaceIdInColonLessMColorCode->setEnabled(true);
    widget_timerDebugOutputMinimumInterval->setEnabled(true);
    checkBox_debugShowAllCodepointProblems->setEnabled(true);
    label_networkPacketTimeout->setEnabled(true);
    doubleSpinBox_networkPacketTimeout->setEnabled(true);
}

void dlgProfilePreferences::initWithHost(Host* pHost)
{
    loadEditorTab();

    fontComboBox_displayFont->setCurrentFont(pHost->getDisplayFont());
    // Accommodate an initial font size being larger than expected - and ensure
    // it is a positive value:
    spinBox_displayFontSize->setMaximum(std::max(pHost->getDisplayFont().pointSize(), 40));
    spinBox_displayFontSize->setValue(std::max(1, pHost->getDisplayFont().pointSize()));
    checkBox_antiAlias->setChecked(!pHost->mNoAntiAlias);

    connect(fontComboBox_displayFont, &QFontComboBox::currentFontChanged, this, &dlgProfilePreferences::slot_displayFontChanged);
    connect(spinBox_displayFontSize, qOverload<int>(&QSpinBox::valueChanged), this, &dlgProfilePreferences::slot_displayFontSizeChanged);
    connect(checkBox_antiAlias, &QCheckBox::clicked, this, &dlgProfilePreferences::slot_displayFontAliasingChanged);

    // search engine load
    search_engine_combobox->addItems(QStringList(mpHost->mSearchEngineData.keys()));

    // set to saved value or default to Google
    const int savedText = search_engine_combobox->findText(mpHost->getSearchEngine().first);
    search_engine_combobox->setCurrentIndex(savedText == -1 ? 1 : savedText);

    checkBox_mVersionInTTYPE->setChecked(pHost->mVersionInTTYPE);
    checkBox_mForceMXPProcessorOn->setChecked(pHost->getForceMXPProcessorOn());
    mMapperUseAntiAlias->setChecked(pHost->mMapperUseAntiAlias);
    checkbox_mMapperShowRoomBorders->setChecked(pHost->mMapperShowRoomBorders);
    checkBox_drawUpperLowerLevels->setChecked(mudlet::self()->mDrawUpperLowerLevels);
    acceptServerGUI->setChecked(pHost->mAcceptServerGUI);
    acceptServerMedia->setChecked(pHost->mAcceptServerMedia);


    comboBox_dictionary->clear();
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
    // Unfortunately OpenBSD does not ship a dictionary for THAT language which
    // prevents us using it to find any system ones
    const QString& currentDictionary = pHost->getSpellDic();
    // This will also set mudlet::mUsingMudletDictionaries as appropriate:
    const QString path = mudlet::getMudletPath(enums::hunspellDictionaryPath, currentDictionary);
    // Tweak the label for the provided spelling dictionaries depending on where
    // they come from:
    if (mudlet::self()->mUsingMudletDictionaries) {
        //: On Windows and MacOs, we have to bundle our own dictionaries with our application - and we also use them on *nix systems where we do not find the system ones
        checkBox_spellCheck->setText(tr("Enable spell check using Mudlet dictionary:"));
    } else {
        //: On *nix systems where we find the system ones we use them
        checkBox_spellCheck->setText(tr("Enable spell check using System dictionary:"));
    }

    const QDir dir(path);
    QStringList entries = dir.entryList(QDir::Files, QDir::Time);
    // QRegularExpression rex(qsl(R"(\.dic$)"));
    // Use the affix file as that may eliminate supplimental dictionaries:
    const QRegularExpression rex(qsl(R"(\.aff$)"));
    entries = entries.filter(rex);
    // Don't emit signals - like (void) QListWidget::currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
    // while populating the widget, it reduces noise about:
    // "qt.accessibility.core: Cannot create accessible child interface for object:  QListWidget(0x############, name = "dictList")  index:  ##
    comboBox_dictionary->blockSignals(true);
    if (!entries.isEmpty()) {
        int currentIndex = -1;
        // Because the list needs to be sorted by translated "display" name
        // which will not follow the order of files seen we need to build a
        // sorted map and then insert items into the QComboBox in that order:
        QMap<QString, int> translatedNameToEntriesMap;
        for (int i = 0, total = entries.size(); i < total; ++i) {
            entries[i].remove(QLatin1String(".aff"), Qt::CaseInsensitive);

            if (entries.at(i).endsWith(qsl("med"), Qt::CaseInsensitive)) {
                continue;
            }

            auto key = entries.at(i).toLower();
            key.replace(QLatin1String("-"), QLatin1String("_"));

            QString displayText = mudlet::self()->mDictionaryLanguageCodeMap.contains(key) ? mudlet::self()->mDictionaryLanguageCodeMap.value(key) : tr("%1 - not recognised").arg(entries.at(i));
            translatedNameToEntriesMap.insert(displayText, i);
        }

        QMapIterator<QString, int> itTranslatedName(translatedNameToEntriesMap);
        while (itTranslatedName.hasNext()) {
            itTranslatedName.next();
            const auto displayText = itTranslatedName.key();
            const auto i = itTranslatedName.value();
            auto key = entries.at(i).toLower();
            key.replace(QLatin1String("-"), QLatin1String("_"));

            QString toolTip = mudlet::self()->mDictionaryLanguageCodeMap.contains(key)
                                      ? utils::richText(tr("From the dictionary file <tt>%1.dic</tt> (and its companion affix <tt>.aff</tt> file).").arg(dir.absoluteFilePath(entries.at(i))))
                                      : tr("<p>Mudlet does not recognise the code \"%1\", please report it to the Mudlet developers so we can describe it properly in future Mudlet versions!</p>"
                                           "<p>The file <tt>%2.dic</tt> (and its companion affix <tt>.aff</tt> file) is still usable.</p>")
                                                .arg(entries.at(i), dir.absoluteFilePath(entries.at(i)));

            comboBox_dictionary->addItem(displayText, entries.at(i));
            comboBox_dictionary->setItemData(comboBox_dictionary->count() - 1, toolTip, Qt::ToolTipRole);

            if (entries.at(i) == currentDictionary) {
                currentIndex = comboBox_dictionary->count() - 1;
            }
        }

        if (currentIndex >= 0) {
            comboBox_dictionary->setCurrentIndex(currentIndex);
        }
    } else {
        comboBox_dictionary->setEnabled(false);
        comboBox_dictionary->setToolTip(utils::richText(tr("No Hunspell dictionary files found, spell-checking will not be available.")));
    }
    comboBox_dictionary->blockSignals(false);

    if (!pHost->getMmpMapLocation().isEmpty()) {
        groupBox_downloadMapOptions->setVisible(true);
        connect(buttonDownloadMap, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_downloadMap);
    } else {
        groupBox_downloadMapOptions->setVisible(false);
    }

#if defined(DEBUG_CODEPOINT_PROBLEMS)
    checkBox_debugShowAllCodepointProblems->setChecked(pHost->debugShowAllProblemCodepoints());
#else
    checkBox_debugShowAllCodepointProblems->hide();
#endif
    // the GMCP warning is hidden by default and is only enabled when the value is toggled
    need_reconnect_for_data_protocol->hide();

    checkBox_announceIncomingText->setChecked(pHost->mAnnounceIncomingText);
    checkBox_advertiseScreenReader->setChecked(pHost->mAdvertiseScreenReader);
    connect(checkBox_advertiseScreenReader, &QCheckBox::toggled, this, &dlgProfilePreferences::slot_toggleAdvertiseScreenReader);
    checkBox_enableOSC8Hyperlinks->setChecked(pHost->mEnableOSC8Hyperlinks);
    connect(checkBox_enableOSC8Hyperlinks, &QCheckBox::toggled, this, &dlgProfilePreferences::slot_toggleEnableOSC8Hyperlinks);

    checkBox_enableClosedCaption->setChecked(pHost->mEnableClosedCaption);
    connect(checkBox_enableClosedCaption, &QCheckBox::toggled, this, &dlgProfilePreferences::slot_toggleEnableClosedCaption);

    // Block signals before setting initial state to prevent toggled signal
    checkBox_f3SearchEnabled->blockSignals(true);
    checkBox_f3SearchEnabled->setChecked(pHost->getF3SearchEnabled());
    checkBox_f3SearchEnabled->blockSignals(false);
    // Now connect the signal
    connect(checkBox_f3SearchEnabled, &QCheckBox::toggled, pHost, &Host::setF3SearchEnabled);

    checkBox_enableBlinkText->setChecked(pHost->getEnableBlinkText());

    // same with special connection warnings
    need_reconnect_for_specialoption->hide();

    wrap_at_spinBox->setValue(pHost->mWrapAt);
    indent_wrapped_spinBox->setValue(pHost->mWrapIndentCount);
    hanging_indent_wrapped_spinBox->setValue(pHost->mWrapHangingIndentCount);
    checkBox_undoServerWrap->setChecked(pHost->mUndoServerWrap);
    undo_server_wrap_width_spinBox->setValue(pHost->mUndoServerWrapWidth);
    undo_server_wrap_width_spinBox->setEnabled(pHost->mUndoServerWrap);
    connect(checkBox_undoServerWrap, &QCheckBox::toggled, undo_server_wrap_width_spinBox, &QWidget::setEnabled);
    // The note is only worth its space to someone actually running the option:
    label_undo_server_wrap_experimental->setVisible(pHost->mUndoServerWrap);
    connect(checkBox_undoServerWrap, &QCheckBox::toggled, label_undo_server_wrap_experimental, &QWidget::setVisible);

    console_buffer_size_spinBox->setValue(pHost->getConsoleBufferSize());
    checkBox_useMaxBufferSize->setChecked(pHost->getUseMaxConsoleBufferSize());

    // Set maximum buffer size based on system capabilities and update tooltip
    if (pHost->mpConsole) {
        const int maxBufferSize = pHost->mpConsole->buffer.getMaxBufferSize();
        console_buffer_size_spinBox->setMaximum(maxBufferSize);
        checkBox_useMaxBufferSize->setToolTip(tr("<p>Use the maximum buffer size your system can handle (%1 lines). This will be calculated based on available memory.</p>").arg(maxBufferSize));

        // If using max buffer size, disable the spinbox and set it to max
        if (pHost->getUseMaxConsoleBufferSize()) {
            console_buffer_size_spinBox->setValue(maxBufferSize);
            console_buffer_size_spinBox->setEnabled(false);
        }
    }

    show_sent_text_combobox->setCurrentIndex(static_cast<int>(pHost->mCommandEchoMode));
    auto_clear_input_line_checkbox->setChecked(pHost->mAutoClearCommandLineAfterSend);
    disable_password_masking_checkbox->setChecked(pHost->mDisablePasswordMasking);
    checkBox_highlightHistory->setChecked(pHost->mHighlightHistory);
    command_separator_lineedit->setText(pHost->mCommandSeparator);
    checkBox_USE_IRE_DRIVER_BUGFIX->setChecked(pHost->mUSE_IRE_DRIVER_BUGFIX);
    checkBox_enableTextAnalyzer->setChecked(pHost->mEnableTextAnalyzer);
    checkBox_mUSE_FORCE_LF_AFTER_PROMPT->setChecked(pHost->mUSE_FORCE_LF_AFTER_PROMPT);
    USE_UNIX_EOL->setChecked(pHost->mUSE_UNIX_EOL);

    switch (pHost->mDiscordMode) {
    case Host::DiscordDisabled:
        radioButton_discordDisabled->setChecked(true);
        break;
    case Host::DiscordShowMudletOnly:
        radioButton_discordMudletOnly->setChecked(true);
        break;
    case Host::DiscordShowGameDetails:
        [[fallthrough]];
    default:
        radioButton_discordGameDetails->setChecked(true);
        break;
    }

    if (mudlet::self()->mDiscord.libraryLoaded()) {
        Host::DiscordOptionFlags const discordFlags = pHost->mDiscordAccessFlags;
        groupBox_discordPrivacy->show();

        const bool enablePrivacy = (pHost->mDiscordMode == Host::DiscordShowGameDetails);
        comboBox_discordLargeIconPrivacy->setEnabled(enablePrivacy);
        comboBox_discordSmallIconPrivacy->setEnabled(enablePrivacy);
        checkBox_discordServerAccessToDetail->setEnabled(enablePrivacy);
        checkBox_discordServerAccessToState->setEnabled(enablePrivacy);
        checkBox_discordServerAccessToPartyInfo->setEnabled(enablePrivacy);
        checkBox_discordServerAccessToTimerInfo->setEnabled(enablePrivacy);

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
        lineEdit_discordUserName->setToolTip(utils::richText(tr("Mudlet will only show Rich Presence information while you use this Discord username (useful if you have multiple Discord accounts). "
                                                                "Leave empty to show it for any Discord account you log in to. This must be the unique Discord username that uses a restricted "
                                                                "lowercase ASCII character set and not any \"Nickname\" that you may have set for a particular Server.")));
        lineEdit_discordUserName->setAccessibleDescription(tr("Mudlet will only show Rich Presence information while you use this Discord username (useful if you have multiple Discord accounts). "
                                                              "Leave empty to show it for any Discord account you log in to. This must be the unique Discord username that uses a restricted lowercase "
                                                              "ASCII character set and not any \"Nickname\" that you may have set for a particular Server."));

        const QString currentDiscordUser = Discord::getLoggedInUserName();
        if (!currentDiscordUser.isEmpty()) {
            //: Shows which Discord account is logged in:
            label_data_discordCurrentUser->setText(currentDiscordUser);
            label_data_discordCurrentUser->setToolTip(utils::richText(
                    tr("This is the unique username using a restricted character set for the Discord account, and not necessarily the nickname that you might have set for a particular Server.")));
        } else {
            label_data_discordCurrentUser->setText(tr("(Not connected)"));
            //: Tooltip shown when Discord Rich Presence cannot detect a logged-in user
            label_data_discordCurrentUser->setToolTip(utils::richText(tr("The Discord desktop app must be running for Rich Presence to work. Browser and mobile clients are not supported.")));
        }
    }

    lineEdit_mmcpChatName->setText(pHost->getMMCPChatName());
    lineEdit_mmcpPort->setText(QString::number(pHost->getMMCPPort()));
    lineEdit_mmcpChatMessagePrefix->setText(pHost->getMMCPChatPrefix());

    /* Possible inclusion in 4.20.1
    checkBox_mmcpAutostartServer->setChecked(pHost->mMMCPAutostartServer);
    checkBox_mmcpAllowPeekReq->setChecked(pHost->mMMCPAllowPeekRequests);
    checkBox_mmcpAutoAcceptCalls->setChecked(pHost->getMMCPAutoAcceptCalls());
    */

    checkBox_mmcpAddChatMessageNewline->setChecked(pHost->getMMCPAddChatMessageNewline());
    checkBox_mmcpPrefixEmotes->setChecked(pHost->getMMCPPrefixEmotes());
    checkBox_mmcpSnoopInMainConsole->setChecked(pHost->getMMCPShowSnoopInMainConsole());
    checkBox_runAllKeyBindings->setChecked(pHost->getKeyUnit()->mRunAllKeyMatches);

    auto originalBorders = pHost->userBorders();
    topBorderHeight->setValue(originalBorders.top());
    bottomBorderHeight->setValue(originalBorders.bottom());
    leftBorderWidth->setValue(originalBorders.left());
    rightBorderWidth->setValue(originalBorders.right());

    // Set the properties in groupBox_logOptions
    mIsLoggingTimestamps->setChecked(pHost->mIsLoggingTimestamps);
    mIsToLogInHtml->setChecked(pHost->mIsNextLogFileInHtmlFormat);

    const bool isLogFileNameEntryShown = pHost->mLogFileNameFormat.isEmpty();
    const QString logExtension = pHost->mIsNextLogFileInHtmlFormat ? ".html" : ".txt";
    label_logFileNameExtension->setVisible(isLogFileNameEntryShown);
    lineEdit_logFileName->setVisible(isLogFileNameEntryShown);
    label_logFileName->setVisible(isLogFileNameEntryShown);
    label_logFileNameExtension->setText(logExtension);

    // This is the previous standard:
    comboBox_logFileNameFormat->addItem(tr("yyyy-MM-dd#HH-mm-ss (e.g., 1970-01-01#00-00-00%1)").arg(logExtension), qsl("yyyy-MM-dd#HH-mm-ss"));
    // The ISO standard for this uses T as the date/time separator
    comboBox_logFileNameFormat->addItem(tr("yyyy-MM-ddTHH-mm-ss (e.g., 1970-01-01T00-00-00%1)").arg(logExtension), qsl("yyyy-MM-ddTHH-mm-ss"));
    comboBox_logFileNameFormat->addItem(tr("yyyy-MM-dd (concatenate daily logs in, e.g. 1970-01-01%1)").arg(logExtension), qsl("yyyy-MM-dd"));
    // It might be possible to use QDateTime::weekNumber but that number is not
    // available from the QDateTime::toString(...) method
    comboBox_logFileNameFormat->addItem(tr("yyyy-MM (concatenate month logs in, e.g. 1970-01%1)").arg(logExtension), qsl("yyyy-MM"));
    comboBox_logFileNameFormat->addItem(tr("Named file (concatenate logs in one file)"), QString());
    comboBox_logFileNameFormat->setCurrentIndex(comboBox_logFileNameFormat->findData(pHost->mLogFileNameFormat));

    lineEdit_logFileName->setText(pHost->mLogFileName);

    // pHost->mLogDir should be empty for the default location:
    mLogDirPath = pHost->mLogDir;
    lineEdit_logFileFolder->setText(mLogDirPath);
    lineEdit_logFileFolder->setPlaceholderText(mudlet::getMudletPath(enums::profileReplayAndLogFilesPath, pHost->getName()));
    // set the cursor position to the end of the lineEdit's text property.
    lineEdit_logFileFolder->setCursorPosition(lineEdit_logFileFolder->text().length());
    // Enable the reset button if the current location is not the default one:
    pushButton_resetLogDir->setEnabled(mLogDirPath.length() > 0);


    commandLineMinimumHeight->setValue(pHost->commandLineMinimumHeight);
    mFORCE_MCCP_OFF->setChecked(pHost->mFORCE_NO_COMPRESSION);
    mFORCE_GA_OFF->setChecked(pHost->mFORCE_GA_OFF);
    mAlertOnNewData->setChecked(pHost->mAlertOnNewData);
    telnetHandlerEnabled->setChecked(mudlet::getQSettings()->value("telnetHandlerEnabled", false).toBool());
    //encoding->setCurrentIndex( pHost->mEncoding );
    mFORCE_SAVE_ON_EXIT->setChecked(pHost->mFORCE_SAVE_ON_EXIT);

    if (!protocolMenu) {
        protocolMenu = new QMenu(tr("Protocols"), this);
    }
    protocolMenu->clear();

    mEnableCHARSET = new QAction(tr("CHARSET: Character Encoding Standard"), protocolMenu);
    mEnableCHARSET->setCheckable(true);
    mEnableCHARSET->setChecked(pHost->mEnableCHARSET);
    protocolMenu->addAction(mEnableCHARSET);

    mEnableGMCP = new QAction(tr("GMCP: Generic Mud Communication Protocol"), protocolMenu);
    mEnableGMCP->setCheckable(true);
    mEnableGMCP->setChecked(pHost->mEnableGMCP);
    protocolMenu->addAction(mEnableGMCP);

    mEnableMNES = new QAction(tr("MNES: Mud New-Environ Standard"), protocolMenu);
    mEnableMNES->setCheckable(true);
    mEnableMNES->setChecked(pHost->mEnableMNES);
    //: Tooltip for MNES protocol option explaining mutual exclusivity with NEW-ENVIRON
    mEnableMNES->setToolTip(tr("MNES uses the same telnet option as NEW-ENVIRON, so only one can be active. MNES sends a minimal set of variables, while NEW-ENVIRON sends extended variables "
                               "including OSC link support."));
    protocolMenu->addAction(mEnableMNES);

    mEnableMSDP = new QAction(tr("MSDP: Mud Server Data Protocol"), protocolMenu);
    mEnableMSDP->setCheckable(true);
    mEnableMSDP->setChecked(pHost->mEnableMSDP);
    protocolMenu->addAction(mEnableMSDP);

    mEnableMSP = new QAction(tr("MSP: Mud Sound Protocol"), protocolMenu);
    mEnableMSP->setCheckable(true);
    mEnableMSP->setChecked(pHost->mEnableMSP);
    protocolMenu->addAction(mEnableMSP);

    mEnableMSSP = new QAction(tr("MSSP: Mud Server Status Protocol"), protocolMenu);
    mEnableMSSP->setCheckable(true);
    mEnableMSSP->setChecked(pHost->mEnableMSSP);
    protocolMenu->addAction(mEnableMSSP);

    mEnableMTTS = new QAction(tr("MTTS: Mud Terminal Type Standard"), protocolMenu);
    mEnableMTTS->setCheckable(true);
    mEnableMTTS->setChecked(pHost->mEnableMTTS);
    protocolMenu->addAction(mEnableMTTS);

    mEnableMXP = new QAction(tr("MXP: Mud eXtension Protocol"), protocolMenu);
    mEnableMXP->setCheckable(true);
    mEnableMXP->setChecked(pHost->mEnableMXP);
    protocolMenu->addAction(mEnableMXP);

    mEnableNAWS = new QAction(tr("NAWS: Negotiate About Window Size"), protocolMenu);
    mEnableNAWS->setCheckable(true);
    mEnableNAWS->setChecked(pHost->mEnableNAWS);
    protocolMenu->addAction(mEnableNAWS);

    mEnableNEWENVIRON = new QAction(tr("NEW-ENVIRON: Client Variables Standard"), protocolMenu);
    mEnableNEWENVIRON->setCheckable(true);
    mEnableNEWENVIRON->setChecked(pHost->mEnableNEWENVIRON);
    //: Tooltip for NEW-ENVIRON protocol option explaining mutual exclusivity with MNES
    mEnableNEWENVIRON->setToolTip(
            tr("NEW-ENVIRON uses the same telnet option as MNES, so only one can be active. NEW-ENVIRON sends extended variables including OSC link support, while MNES sends a minimal set."));
    protocolMenu->addAction(mEnableNEWENVIRON);

    pushButton_chooseProtocols->setMenu(protocolMenu);

    groupBox_purgeMediaCache->setVisible(true);
    connect(buttonPurgeMediaCache, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_purgeMediaCache);

    // load profiles into mappers "copy map to profile" combobox
    // this feature should work seamlessly both for online and offline profiles
    const QStringList profileList = QDir(mudlet::getMudletPath(enums::profilesPath)).entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time); // sort by profile "hotness"
    pushButton_chooseProfiles->setEnabled(false);
    pushButton_copyMap->setEnabled(false);
    if (!mpMenu) {
        mpMenu = new QMenu(tr("Other profiles to Map to:"), this);
    }

    mpMenu->clear();
    for (unsigned int i = 0, total = profileList.size(); i < total; ++i) {
        const QString s = profileList.at(i);
        if (s.isEmpty() || !s.compare(pHost->getName())) {
            // Do not include THIS profile in the list - it will
            // automatically get saved - as the file to copy to the other
            // profiles!
            continue;
        }

        auto pItem = new QAction(s, mpMenu);
        pItem->setCheckable(true);
        pItem->setChecked(false);
        mpMenu->addAction(pItem);
        //Enable it as we now have at least one profile to copy to
        pushButton_chooseProfiles->setEnabled(true);
    }

    pushButton_chooseProfiles->setMenu(mpMenu);

    fillOutMapHistory();

    // label to show on successful map file action
    label_mapFileActionResult->hide();

    slot_hidePasswordMigrationLabel();

    //double-click ignore
    QString ignore;
    QSetIterator<QChar> it(pHost->mDoubleClickIgnore);
    while (it.hasNext()) {
        ignore = ignore.append(it.next());
    }
    doubleclick_ignore_lineedit->setText(ignore);

    // FIXME: Check this each time that it is appropriate for THIS build version
    comboBox_mapFileSaveFormatVersion->clear();
    // Add default version:
    comboBox_mapFileSaveFormatVersion->addItem(tr("%1 {Default}").arg(pHost->mpMap->mDefaultVersion), QVariant(pHost->mpMap->mDefaultVersion));
    comboBox_mapFileSaveFormatVersion->setEnabled(false);
    label_mapFileSaveFormatVersion->setEnabled(false);
    if (pHost->mpMap) {
        if (pHost->mpMap->mMaxVersion > pHost->mpMap->mDefaultVersion || pHost->mpMap->mMinVersion < pHost->mpMap->mDefaultVersion) {
            for (int i = pHost->mpMap->mMinVersion; i <= pHost->mpMap->mMaxVersion; ++i) {
                if (i == pHost->mpMap->mDefaultVersion) {
                    continue;
                }
                comboBox_mapFileSaveFormatVersion->setEnabled(true);
                label_mapFileSaveFormatVersion->setEnabled(true);
                if (i > pHost->mpMap->mDefaultVersion) {
                    comboBox_mapFileSaveFormatVersion->addItem(tr("%1 {Experimental}").arg(i), QVariant(i));
                } else {
                    comboBox_mapFileSaveFormatVersion->addItem(tr("%1 {For older versions}").arg(i), QVariant(i));
                }
            }
            const int _indexForCurrentSaveFormat = comboBox_mapFileSaveFormatVersion->findData(pHost->mpMap->mSaveVersion, Qt::UserRole);
            if (_indexForCurrentSaveFormat >= 0) {
                comboBox_mapFileSaveFormatVersion->setCurrentIndex(_indexForCurrentSaveFormat);
            }
        }

        QLabel* pLabel_mapSymbolFontFudge = new QLabel(tr("2D Map Room Symbol scaling factor:"), groupBox_mapSymbols);
        mpDoubleSpinBox_mapSymbolFontFudge = new QDoubleSpinBox(groupBox_mapSymbols);
        mpDoubleSpinBox_mapSymbolFontFudge->setPrefix(qsl("×"));
        mpDoubleSpinBox_mapSymbolFontFudge->setRange(TMap::scmMinimumSymbolFontFudgeFactor, TMap::scmMaximumSymbolFontFudgeFactor);
        mpDoubleSpinBox_mapSymbolFontFudge->setSingleStep(0.01);
        // Qt's default of two decimals would show a factor set from Lua as
        // something it is not - the API takes any value in the range. Both this
        // and the range have to be in place before the value, which a spin-box
        // rounds and clamps as it is given:
        mpDoubleSpinBox_mapSymbolFontFudge->setDecimals(3);
        mpDoubleSpinBox_mapSymbolFontFudge->setValue(pHost->mpMap->getSymbolFontFudgeFactor());
        auto* pSymbolsLayout = qobject_cast<QGridLayout*>(groupBox_mapSymbols->layout());
        if (pSymbolsLayout) {
            const int existingRows = pSymbolsLayout->rowCount();
            pSymbolsLayout->addWidget(pLabel_mapSymbolFontFudge, existingRows, 0);
            pSymbolsLayout->addWidget(mpDoubleSpinBox_mapSymbolFontFudge, existingRows, 1);
        } else {
            qWarning() << "dlgProfilePreferences::initWithHost(...) WARNING - Unable to cast groupBox_mapSymbols layout to expected QGridLayout - someone has messed with the profile_preferences.ui "
                          "file and the contents of the groupBox can not be shown...!";
        }
        connect(mpDoubleSpinBox_mapSymbolFontFudge, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
            Host* pHost = mpHost;
            if (!pHost || !pHost->mpMap) {
                return;
            }
            pHost->mpMap->setSymbolFontFudgeFactor(value);
        });

        label_mapSymbolsFont->setEnabled(true);
        fontComboBox_mapSymbols->setEnabled(true);
        checkBox_isOnlyMapSymbolFontToBeUsed->setEnabled(true);

        checkBox_showDefaultArea->show();
        checkBox_showDefaultArea->setText(tr(R"(Show "%1" in the map area selection)").arg(pHost->mpMap->getDefaultAreaName()));
        checkBox_showDefaultArea->setChecked(pHost->mpMap->getDefaultAreaShown());

        pushButton_showGlyphUsage->setEnabled(true);
        fontComboBox_mapSymbols->setCurrentFont(pHost->mpMap->mMapSymbolFont);
        checkBox_isOnlyMapSymbolFontToBeUsed->setChecked(pHost->mpMap->mIsOnlyMapSymbolFontToBeUsed);
        connect(pushButton_showGlyphUsage, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_showMapGlyphUsage, Qt::UniqueConnection);
        connect(fontComboBox_mapSymbols, &QFontComboBox::currentFontChanged, this, &dlgProfilePreferences::slot_setMapSymbolFont, Qt::UniqueConnection);
        connect(checkBox_isOnlyMapSymbolFontToBeUsed, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapSymbolFontStrategy, Qt::UniqueConnection);
        connect(pHost->mpMap.data(), &TMap::signal_mapSymbolFontChanged, this, &dlgProfilePreferences::slot_mapSymbolFontChanged, Qt::UniqueConnection);

        groupBox_playerRoomStyle->setEnabled(true);
        comboBox_playerRoomStyle->setCurrentIndex(pHost->mpMap->mPlayerRoomStyle);
        // Custom colours only available in style '3' (of '0' to '3'):
        pushButton_playerRoomPrimaryColor->setEnabled(pHost->mpMap->mPlayerRoomStyle == 3);
        pushButton_playerRoomSecondaryColor->setEnabled(pHost->mpMap->mPlayerRoomStyle == 3);
        spinBox_playerRoomOuterDiameter->setValue(pHost->mpMap->mPlayerRoomOuterDiameterPercentage);
        spinBox_playerRoomInnerDiameter->setValue(pHost->mpMap->mPlayerRoomInnerDiameterPercentage);
        // Adjustable inner diameter not available for style '0' (original):
        spinBox_playerRoomInnerDiameter->setEnabled(pHost->mpMap->mPlayerRoomStyle != 0);
        setButtonColor(pushButton_playerRoomPrimaryColor, pHost->mpMap->mPlayerRoomOuterColor, true);
        setButtonColor(pushButton_playerRoomSecondaryColor, pHost->mpMap->mPlayerRoomInnerColor, true);

        connect(pushButton_deleteMap, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_deleteMap);
        connect(comboBox_playerRoomStyle, qOverload<int>(&QComboBox::currentIndexChanged), this, &dlgProfilePreferences::slot_changePlayerRoomStyle);
        connect(pushButton_playerRoomPrimaryColor, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setPlayerRoomPrimaryColor);
        connect(pushButton_playerRoomSecondaryColor, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setPlayerRoomSecondaryColor);
        connect(spinBox_playerRoomOuterDiameter, qOverload<int>(&QSpinBox::valueChanged), this, &dlgProfilePreferences::slot_setPlayerRoomOuterDiameter);
        connect(spinBox_playerRoomInnerDiameter, qOverload<int>(&QSpinBox::valueChanged), this, &dlgProfilePreferences::slot_setPlayerRoomInnerDiameter);

        // Initialize room, exit, and border size controls
        spinBox_roomSize->setValue(pHost->mRoomSize * 10);
        // mLineSize/mRoomBorderSize are inversely proportional to thickness
        // (exitWidth = 1/eSize * ...), convert to a direct 1-11 scale
        // using a simple reciprocal: mLineSize = 50 / spinner, spinner = 50 / mLineSize
        spinBox_exitSize->setValue(qBound(1, qRound(50.0 / pHost->mLineSize), 11));
        spinBox_borderSize->setValue(qBound(1, qRound(50.0 / pHost->mRoomBorderSize), 11));
        doubleSpinBox_gridSize->setValue(pHost->mMapGridLineSize);
        connect(spinBox_roomSize, qOverload<int>(&QSpinBox::valueChanged), this, &dlgProfilePreferences::slot_roomSizeChanged);
        connect(spinBox_exitSize, qOverload<int>(&QSpinBox::valueChanged), this, &dlgProfilePreferences::slot_exitSizeChanged);
        connect(spinBox_borderSize, qOverload<int>(&QSpinBox::valueChanged), this, &dlgProfilePreferences::slot_borderSizeChanged);
        connect(doubleSpinBox_gridSize, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &dlgProfilePreferences::slot_gridSizeChanged);
        connect(checkbox_mMapperShowRoomBorders, &QCheckBox::toggled, this, [this](bool checked) {
            Host* pHost = mpHost;
            if (!pHost) {
                return;
            }
            pHost->mMapperShowRoomBorders = checked;
            if (pHost->mpMap && pHost->mpMap->mpMapper && pHost->mpMap->mpMapper->mp2dMap) {
                pHost->mpMap->mpMapper->mp2dMap->update();
            }
        });
        connect(checkBox_drawUpperLowerLevels, &QCheckBox::toggled, this, [this](bool checked) {
            mudlet::self()->mDrawUpperLowerLevels = checked;
            Host* pHost = mpHost;
            if (pHost && pHost->mpMap && pHost->mpMap->mpMapper && pHost->mpMap->mpMapper->mp2dMap) {
                pHost->mpMap->mpMapper->mp2dMap->update();
            }
        });
        connect(mMapperUseAntiAlias, &QCheckBox::toggled, this, [this](bool checked) {
            Host* pHost = mpHost;
            if (!pHost) {
                return;
            }
            pHost->mMapperUseAntiAlias = checked;
            if (pHost->mpMap && pHost->mpMap->mpMapper && pHost->mpMap->mpMapper->mp2dMap) {
                pHost->mpMap->mpMapper->mp2dMap->mMapperUseAntiAlias = checked;
                pHost->mpMap->mpMapper->mp2dMap->update();
            }
        });
    } else {
        label_mapSymbolsFont->setEnabled(false);
        fontComboBox_mapSymbols->setEnabled(false);
        checkBox_isOnlyMapSymbolFontToBeUsed->setEnabled(false);
        pushButton_showGlyphUsage->setEnabled(false);

        checkBox_showDefaultArea->hide();
        groupBox_playerRoomStyle->setEnabled(false);
    }

    comboBox_encoding->addItem(mudlet::self()->getEncodingNamesMap().value(QByteArray("ASCII")), QByteArray("ASCII"));
    for (const auto& encoding : pHost->mTelnet.getEncodingsList()) {
        auto encodingTitle =
                mudlet::self()->getEncodingNamesMap().value(encoding,
                                                            tr("%1 (*Error, report to Mudlet Makers*)",
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
        const int currentIndex = comboBox_encoding->findData(pHost->mTelnet.getEncoding());
        if (currentIndex >= 0) {
            comboBox_encoding->setCurrentIndex(currentIndex);
        } else {
            // invalid or not found - so reset to ASCII:
            comboBox_encoding->setCurrentIndex(0);
        }
    }

    comboBox_controlCharacterHandling->setItemData(0, QVariant::fromValue(ControlCharacterMode::AsIs));
    comboBox_controlCharacterHandling->setItemData(1, QVariant::fromValue(ControlCharacterMode::Picture));
    comboBox_controlCharacterHandling->setItemData(2, QVariant::fromValue(ControlCharacterMode::OEM));
    auto cch_index = comboBox_controlCharacterHandling->findData(static_cast<int>(pHost->getControlCharacterMode()));
    comboBox_controlCharacterHandling->setCurrentIndex((cch_index > 0) ? cch_index : 0);
    connect(comboBox_controlCharacterHandling, qOverload<int>(&QComboBox::currentIndexChanged), this, &dlgProfilePreferences::slot_changeControlCharacterHandling);

    timeEdit_timerDebugOutputMinimumInterval->setTime(pHost->mTimerDebugOutputSuppressionInterval);
    frame_notificationArea->hide();
    notificationAreaIconLabelWarning->hide();
    notificationAreaIconLabelError->hide();
    notificationAreaIconLabelInformation->hide();
    notificationAreaMessageBox->hide();

#if !defined(QT_NO_SSL)
    if (QSslSocket::supportsSsl() && pHost->mTelnet.currentlySecure()) {
        const QSslCertificate cert = pHost->mTelnet.getPeerCertificate();
        if (cert.isNull()) {
            groupBox_ssl_certificate->hide();
        } else {
            ssl_issuer_label->setText(cert.issuerInfo(QSslCertificate::CommonName).join(","));
            ssl_issued_label->setText(cert.subjectInfo(QSslCertificate::CommonName).join(","));
            ssl_expires_label->setText(cert.expiryDate().toString(mudlet::self()->getUserLocale().dateFormat(QLocale::ShortFormat)));
            ssl_serial_label->setText(QString::fromStdString(cert.serialNumber().toStdString()));
            checkBox_self_signed->setStyleSheet(QString());
            checkBox_expired->setStyleSheet(QString());
            ssl_issuer_label->setStyleSheet(QString());
            ssl_expires_label->setStyleSheet(QString());

            const QList<QSslError> sslErrors = pHost->mTelnet.getSslErrors();
            if (!sslErrors.isEmpty()) {
                // handle ssl errors
                notificationAreaIconLabelWarning->show();
                frame_notificationArea->show();
                notificationAreaMessageBox->show();

                QStringList errorTexts;
                for (const auto& sslError : sslErrors) {
                    errorTexts.append(qsl("<li>%1</li>").arg(sslError.errorString()));
                    if (QSslError::SelfSignedCertificate == sslError.error()) {
                        checkBox_self_signed->setStyleSheet(certificateWarningCheckBoxStyle());
                        ssl_issuer_label->setStyleSheet(certificateWarningLabelStyle());
                    }
                    if (QSslError::CertificateExpired == sslError.error()) {
                        checkBox_expired->setStyleSheet(certificateWarningCheckBoxStyle());
                        ssl_expires_label->setStyleSheet(certificateWarningLabelStyle());
                    }
                }
                notificationAreaMessageBox->setText(qsl("<ul>%1</ul>").arg(errorTexts.join(QChar::LineFeed)));

            } else {
                // Check for other errors
                if (pHost->mTelnet.error().has_value()) {
                    switch (pHost->mTelnet.error().value()) {
                    case QAbstractSocket::SslHandshakeFailedError:
                        // handle failed handshake, likely not ssl socket
                        notificationAreaIconLabelError->show();
                        frame_notificationArea->show();
                        notificationAreaMessageBox->show();
                        notificationAreaMessageBox->setText(pHost->mTelnet.errorString());
                        break;
                    case QAbstractSocket::SslInternalError:
                        // handle ssl library error
                        notificationAreaIconLabelError->show();
                        frame_notificationArea->show();
                        notificationAreaMessageBox->show();
                        notificationAreaMessageBox->setText(pHost->mTelnet.errorString());
                        break;
                    case QAbstractSocket::SslInvalidUserDataError:
                        // handle invalid data (certificate, key, cypher, etc.)
                        notificationAreaIconLabelError->show();
                        frame_notificationArea->show();
                        notificationAreaMessageBox->show();
                        notificationAreaMessageBox->setText(pHost->mTelnet.errorString());
                        break;
                    default: {
                    } // There are a significant number of other errors
                        // that are not handled here!
                    }
                }
            }
        }
    }
#endif

    if (!pHost->mLoadedOk) {
        notificationAreaIconLabelWarning->show();
        frame_notificationArea->show();
        notificationAreaMessageBox->show();
        QString errorDetails = pHost->mProfileLoadError.isEmpty() ? tr("unknown error") : pHost->mProfileLoadError;
        notificationAreaMessageBox->setText(tr("This profile could not be loaded correctly (%1). "
                                               "Settings cannot be saved. Close the profile and try loading an older version from "
                                               "'Connect - Options - Profile history'.")
                                                    .arg(errorDetails));
    }

    groupBox_ssl->setChecked(pHost->mSslTsl);
    checkBox_self_signed->setChecked(pHost->mSslIgnoreSelfSigned);
    checkBox_expired->setChecked(pHost->mSslIgnoreExpired);
    checkBox_ignore_all->setChecked(pHost->mSslIgnoreAll);

    checkBox_askTlsAvailable->setChecked(pHost->mAskTlsAvailable);

    // The "forget saved sign-in" button is gated on a reconnect token actually existing, not on the
    // sign-in-choice flag: an oauth-only game never sets that flag yet still mints tokens, and a token
    // is the only thing the button acts on. The keychain check is asynchronous, so start hidden and
    // reveal on a hit; the QPointer guards against the dialog closing before the store answers.
    // credentialExists() collapses a read failure (locked/denied/timed-out keychain) to "no token", so
    // the button deliberately stays hidden on any read failure - the only cost is not offering to forget
    // a token that could not be read, and clicking would just yield a graceful "could not remove" warning.
    pushButton_forgetSavedSignIn->setVisible(false);
    pushButton_forgetSavedSignIn->setEnabled(mEnableGMCP->isChecked());
    QPointer<dlgProfilePreferences> safeDialog = this;
    QPointer<CredentialManager> credentialManager = new CredentialManager();
    credentialManager->credentialExists(pHost->getName(), qsl("reconnect"), [safeDialog, credentialManager](bool exists) {
        if (credentialManager) {
            credentialManager->deleteLater();
        }
        if (safeDialog && exists) {
            safeDialog->pushButton_forgetSavedSignIn->setVisible(true);
        }
    });

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
    doubleSpinBox_networkPacketTimeout->setValue(pHost->mTelnet.getPostingTimeout() / 1000.0);
    {
        const QSignalBlocker blocker(comboBox_caretModeKey);
        comboBox_caretModeKey->setCurrentIndex(static_cast<int>(pHost->mCaretShortcut));
    }
    checkBox_largeAreaExitArrows->setChecked(pHost->getLargeAreaExitArrows());
    checkBox_invertMapZoom->setChecked(mudlet::self()->invertMapZoom());
    comboBox_blankLinesBehaviour->setCurrentIndex(static_cast<int>(pHost->mBlankLineBehaviour));

    // Enable the controls that would be disabled if there wasn't a Host instance
    // on tab_general:
    // groupBox_iconsAndToolbars is NOT dependent on pHost - leave it alone
    enableHostDetails();

    /* These require the color controls to be correctly enabled/disabled before
     * they are called:*/
    setColors();
    setColors2();
    setButtonColor(pushButton_playerRoomPrimaryColor, pHost->mpMap->mPlayerRoomOuterColor, true);
    setButtonColor(pushButton_playerRoomSecondaryColor, pHost->mpMap->mPlayerRoomInnerColor, true);

    // Identify which Profile we are showing the settings for:
    setWindowTitle(tr("Profile preferences - %1").arg(pHost->getName()));

    // CHECKME: Have moved ALL the connects, where possible, to the end so that
    // none are triggered by the setup operations...
    connect(pushButton_command_line_foreground_color, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setCommandLineFgColor);
    connect(pushButton_command_line_background_color, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setCommandLineBgColor);

    connect(pushButton_black, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setColorBlack);
    connect(pushButton_lBlack, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setColorLightBlack);
    connect(pushButton_red, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setColorRed);
    connect(pushButton_lRed, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setColorLightRed);
    connect(pushButton_green, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setColorGreen);
    connect(pushButton_lGreen, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setColorLightGreen);
    connect(pushButton_yellow, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setColorYellow);
    connect(pushButton_lYellow, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setColorLightYellow);
    connect(pushButton_blue, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setColorBlue);
    connect(pushButton_lBlue, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setColorLightBlue);
    connect(pushButton_magenta, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setColorMagenta);
    connect(pushButton_lMagenta, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setColorLightMagenta);
    connect(pushButton_cyan, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setColorCyan);
    connect(pushButton_lCyan, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setColorLightCyan);
    connect(pushButton_white, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setColorWhite);
    connect(pushButton_lWhite, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setColorLightWhite);

    connect(pushButton_foreground_color, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setFgColor);
    connect(pushButton_background_color, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setBgColor);
    connect(pushButton_command_foreground_color, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setCommandFgColor);
    connect(pushButton_command_background_color, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setCommandBgColor);

    connect(pushButton_resetColors, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_resetColors);
    connect(reset_colors_button_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_resetMapColors);
    connect(pushButton_black_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapColorBlack);
    connect(pushButton_Lblack_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapColorLightBlack);
    connect(pushButton_green_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapColorGreen);
    connect(pushButton_Lgreen_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapColorLightGreen);
    connect(pushButton_red_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapColorRed);
    connect(pushButton_Lred_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapColorLightRed);
    connect(pushButton_blue_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapColorBlue);
    connect(pushButton_Lblue_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapColorLightBlue);
    connect(pushButton_yellow_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapColorYellow);
    connect(pushButton_Lyellow_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapColorLightYellow);
    connect(pushButton_cyan_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapColorCyan);
    connect(pushButton_Lcyan_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapColorLightCyan);
    connect(pushButton_magenta_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapColorMagenta);
    connect(pushButton_Lmagenta_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapColorLightMagenta);
    connect(pushButton_white_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapColorWhite);
    connect(pushButton_Lwhite_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapColorLightWhite);

    connect(pushButton_foreground_color_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapExitsColor);
    connect(pushButton_background_color_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapBgColor);
    connect(pushButton_lowerLevelColor, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setLowerLevelColor);
    connect(pushButton_upperLevelColor, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setUpperLevelColor);
    connect(pushButton_roomBorderColor, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapRoomBorderColor);
    connect(pushButton_mapInfoBg, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapInfoBgColor);
    connect(pushButton_roomCollisionBorderColor, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapRoomCollisionBorderColor);
    connect(pushButton_mapGridColor, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapGridColor);

    connect(mEnableGMCP, &QAction::toggled, need_reconnect_for_data_protocol, &QWidget::show);
    // The GMCP Char.Login "forget saved sign-in" control is only meaningful when GMCP is on.
    connect(mEnableGMCP, &QAction::toggled, pushButton_forgetSavedSignIn, &QWidget::setEnabled);
    connect(pushButton_forgetSavedSignIn, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_forgetSavedSignIn);
    connect(mEnableMSDP, &QAction::toggled, need_reconnect_for_data_protocol, &QWidget::show);
    connect(mEnableMSSP, &QAction::toggled, need_reconnect_for_data_protocol, &QWidget::show);
    connect(mEnableMSP, &QAction::toggled, need_reconnect_for_data_protocol, &QWidget::show);
    connect(mEnableMXP, &QAction::toggled, need_reconnect_for_data_protocol, &QWidget::show);
    connect(mEnableMTTS, &QAction::toggled, need_reconnect_for_data_protocol, &QWidget::show);
    connect(mEnableMNES, &QAction::toggled, need_reconnect_for_data_protocol, &QWidget::show);
    connect(mEnableNAWS, &QAction::toggled, need_reconnect_for_data_protocol, &QWidget::show);
    connect(mEnableCHARSET, &QAction::toggled, need_reconnect_for_data_protocol, &QWidget::show);
    connect(mEnableNEWENVIRON, &QAction::toggled, need_reconnect_for_data_protocol, &QWidget::show);

    // MNES and NEW-ENVIRON both use telnet option 39, so they are mutually exclusive
    connect(mEnableMNES, &QAction::toggled, this, [this](bool checked) {
        if (checked && mEnableNEWENVIRON->isChecked()) {
            mEnableNEWENVIRON->setChecked(false);
        }
    });
    connect(mEnableNEWENVIRON, &QAction::toggled, this, [this](bool checked) {
        if (checked && mEnableMNES->isChecked()) {
            mEnableMNES->setChecked(false);
        }
    });

    connect(mFORCE_MCCP_OFF, &QAbstractButton::clicked, need_reconnect_for_specialoption, &QWidget::show);
    connect(mFORCE_GA_OFF, &QAbstractButton::clicked, need_reconnect_for_specialoption, &QWidget::show);
    connect(mpMenu.data(), &QMenu::triggered, this, &dlgProfilePreferences::slot_chosenProfilesChanged);

    connect(pushButton_copyMap, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_copyMap);
    connect(pushButton_loadMap, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_loadMap);
    connect(pushButton_saveMap, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_saveMap);
    connect(comboBox_encoding, qOverload<int>(&QComboBox::currentIndexChanged), this, &dlgProfilePreferences::slot_setEncoding);

    // Progressive disclosure for screen-reader users: surface the hyperlink
    // navigation/activation/menu shortcuts at the moment the user picks a
    // pane-switching key, so they don't have to consult the wiki to discover
    // them. Picking Tab additionally warns about the shared binding.
    connect(comboBox_caretModeKey, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (index < 0) {
            return;
        }
        if (!QAccessible::isActive()) {
            return;
        }
        auto* app = mudlet::self();
        if (!app) {
            return;
        }
        QString announcement;
        const auto choice = static_cast<Host::CaretShortcut>(index);
        if (choice == Host::CaretShortcut::Tab) {
            //: Screen-reader hint when the user picks Tab as the caret-mode pane-switching key, warning Tab is shared with hyperlink navigation and explaining how to activate links, open their menu, and jump to latest content. Do not translate the key names "Tab", "Ctrl+]", "Ctrl+[", "Enter", "Space", "Menu", "Shift+F10", "Ctrl+End" or "Ctrl+Home".
            announcement = tr("Tab will switch between the input line and main window, and also step through hyperlinks while in caret mode. Ctrl+] and Ctrl+[ navigate links without conflicting with "
                              "pane-switching. Press Enter or Space to activate the focused link, and the Menu key or Shift+F10 to open its context menu. Press Ctrl+End to jump to the latest "
                              "content or Ctrl+Home to jump to the start of the buffer.");
        } else {
            //: Screen-reader hint when the user picks any caret-mode pane-switching key other than Tab, explaining how to navigate, activate and open menus on hyperlinks, and jump to latest content. Do not translate the key names "Ctrl+]", "Ctrl+[", "Enter", "Space", "Menu", "Shift+F10", "Ctrl+End" or "Ctrl+Home".
            announcement = tr("In caret mode, use Ctrl+] for the next hyperlink and Ctrl+[ for the previous hyperlink. Press Enter or Space to activate the focused link, and the Menu key or "
                              "Shift+F10 to open its context menu. Press Ctrl+End to jump to the latest content or Ctrl+Home to jump to the start of the buffer.");
        }
        app->announce(announcement, QString(), true);
    });

    connect(pushButton_whereToLog, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setLogDir);
    connect(pushButton_resetLogDir, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_resetLogDir);
    connect(comboBox_logFileNameFormat, qOverload<int>(&QComboBox::currentIndexChanged), this, &dlgProfilePreferences::slot_logFileNameFormatChange);
    connect(mIsToLogInHtml, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_changeLogFileAsHtml);
    connect(doubleSpinBox_networkPacketTimeout, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &dlgProfilePreferences::slot_setPostingTimeout);
    connect(checkBox_largeAreaExitArrows, &QCheckBox::toggled, this, &dlgProfilePreferences::slot_changeLargeAreaExitArrows);
    connect(checkBox_invertMapZoom, &QCheckBox::toggled, this, &dlgProfilePreferences::slot_changeInvertMapZoom);

    // Console buffer settings
    connect(checkBox_useMaxBufferSize, &QCheckBox::toggled, this, &dlgProfilePreferences::slot_toggleUseMaxBufferSize);

    //Shortcuts tab
    auto shortcutKeys = mudlet::self()->mpShortcutsManager->iterator();
    int shortcutsRow = 0;
    while (shortcutKeys.hasNext()) {
        auto key = shortcutKeys.next();
        auto shortcutIt = pHost->profileShortcuts.find(key);
        QKeySequence currentSequence = (shortcutIt != pHost->profileShortcuts.end()) ? QKeySequence(*shortcutIt->second) : QKeySequence();
        currentShortcuts.insert(key, currentSequence);
        const QString labelText = mudlet::self()->mpShortcutsManager->getLabel(key);
        auto sequenceEdit = new TKeySequenceEdit(currentSequence, labelText);
        auto label = new QLabel(labelText);
        // Point the buddy at the control that actually receives focus (the
        // editor's inner line edit, reached via its focus proxy) rather than the
        // wrapper, so the accessible label attaches to the single announced
        // node; naming the wrapper as well made screen readers read the label
        // twice (#9322). The proxy is only null in the degraded fallback, where
        // the wrapper itself is the focus target:
        QWidget* const labelTarget = sequenceEdit->focusProxy() ? sequenceEdit->focusProxy() : sequenceEdit;
        label->setBuddy(labelTarget);

        gridLayout_groupBox_shortcuts->addWidget(label, floor(shortcutsRow / 2), (shortcutsRow % 2) * 2 + 1);
        gridLayout_groupBox_shortcuts->addWidget(sequenceEdit, floor(shortcutsRow / 2), (shortcutsRow % 2) * 2 + 2);
        shortcutsRow++;
        connect(sequenceEdit, &QKeySequenceEdit::editingFinished, this, [=]() {
            QKeySequence newSequence;
            if (!sequenceEdit->keySequence().isEmpty() && !sequenceEdit->keySequence().matches(QKeySequence(Qt::Key_Escape))) {
                newSequence = sequenceEdit->keySequence();
            }
            sequenceEdit->setKeySequence(newSequence);
            currentShortcuts[key] = newSequence;
            updateShortcutConflictWarning();
            slot_scheduleApply();
        });
        connect(this, &dlgProfilePreferences::signal_resetMainWindowShortcutsToDefaults, sequenceEdit, [=]() {
            const auto defaultSequence = *mudlet::self()->mpShortcutsManager->getDefault(key);
            sequenceEdit->setKeySequence(defaultSequence);
            currentShortcuts[key] = defaultSequence;
        });
    }
    updateShortcutConflictWarning();
}

// Recomputes the duplicate state of the whole shortcut map, not just the last
// edited entry, so the warning always names every clash and disappears once
// none remain. Duplicates are deliberately still accepted - taking away the
// ability to set one would make rearranging shortcuts awkward - but Qt
// disables ambiguous shortcuts, so the user has to be told about the clash.
void dlgProfilePreferences::updateShortcutConflictWarning()
{
    ShortcutsManager* manager = mudlet::self()->mpShortcutsManager;
    QStringList keys;
    auto keysIterator = manager->iterator();
    while (keysIterator.hasNext()) {
        keys.append(keysIterator.next());
    }

    QStringList warnings;
    QList<int> reported;
    for (int i = 0; i < keys.size(); ++i) {
        if (reported.contains(i)) {
            continue;
        }
        const QKeySequence sequence = currentShortcuts.value(keys.at(i));
        if (sequence.isEmpty()) {
            continue;
        }
        QStringList labels{manager->getLabel(keys.at(i))};
        for (int j = i + 1; j < keys.size(); ++j) {
            const QKeySequence other = currentShortcuts.value(keys.at(j));
            if (!other.isEmpty() && sequence.matches(other) == QKeySequence::ExactMatch) {
                labels.append(manager->getLabel(keys.at(j)));
                reported.append(j);
            }
        }
        if (labels.size() < 2) {
            continue;
        }
        const QString sequenceText = sequence.toString(QKeySequence::NativeText);
        if (labels.size() == 2) {
            //: Inline warning on the shortcuts preferences page when exactly two actions have been given the same shortcut. %1 and %2 are the action names, %3 is the shortcut itself.
            warnings.append(tr("Warning: '%1' and '%2' now share the shortcut %3 - neither will work until one of them is changed.").arg(labels.at(0), labels.at(1), sequenceText));
        } else {
            QStringList quotedLabels;
            for (const auto& label : labels) {
                quotedLabels.append(qsl("'%1'").arg(label));
            }
            //: Inline warning on the shortcuts preferences page when three or more actions have been given the same shortcut. %1 is the list of action names (each already quoted), %2 is the shortcut itself.
            warnings.append(tr("Warning: %1 now share the shortcut %2 - none of them will work until they are changed.").arg(quotedLabels.join(qsl(", ")), sequenceText));
        }
    }

    const QString warningText = warnings.join(QChar::LineFeed);
    if (warningText.isEmpty()) {
        if (!label_shortcutsConflictWarning->isHidden()) {
            label_shortcutsConflictWarning->hide();
            label_shortcutsConflictWarning->clear();
            if (QAccessible::isActive()) {
                //: Screen-reader announcement when editing the shortcuts removed the last duplicated assignment.
                mudlet::self()->announce(tr("Shortcut conflict resolved."), QString(), true);
            }
        }
        return;
    }

    label_shortcutsConflictWarning->setStyleSheet(qsl("color: %1; font-weight: bold;").arg(mudlet::self()->inDarkMode() ? qsl("#ff8080") : qsl("#aa0000")));
    if (!label_shortcutsConflictWarning->isHidden() && warningText == label_shortcutsConflictWarning->text()) {
        return;
    }
    label_shortcutsConflictWarning->setText(warningText);
    label_shortcutsConflictWarning->show();
    if (QAccessible::isActive()) {
        mudlet::self()->announce(warningText, QString(), true);
    }
}

void dlgProfilePreferences::disconnectHostRelatedControls()
{
    // The "new" style connect(...) does not have the same range of overloaded
    // disconnect(...) counterparts - so we need to provide the "dummy"
    // arguments to get the wanted wild-card behaviour for them:

    disconnect(fontComboBox_displayFont, &QFontComboBox::currentFontChanged, nullptr, nullptr);
    disconnect(spinBox_displayFontSize, qOverload<int>(&QSpinBox::valueChanged), nullptr, nullptr);
    disconnect(checkBox_antiAlias, &QCheckBox::clicked, nullptr, nullptr);

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
    disconnect(pushButton_lowerLevelColor, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_upperLevelColor, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_roomBorderColor, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_mapInfoBg, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_roomCollisionBorderColor, &QAbstractButton::clicked, nullptr, nullptr);

    disconnect(mEnableGMCP, &QAction::toggled, nullptr, nullptr);
    disconnect(mEnableMSSP, &QAction::toggled, nullptr, nullptr);
    disconnect(mEnableMSDP, &QAction::toggled, nullptr, nullptr);
    disconnect(mEnableMSP, &QAction::toggled, nullptr, nullptr);
    disconnect(mEnableMXP, &QAction::toggled, nullptr, nullptr);
    disconnect(mEnableMTTS, &QAction::toggled, nullptr, nullptr);
    disconnect(mEnableMNES, &QAction::toggled, nullptr, nullptr);
    disconnect(mEnableNAWS, &QAction::toggled, nullptr, nullptr);
    disconnect(mEnableCHARSET, &QAction::toggled, nullptr, nullptr);
    disconnect(mEnableNEWENVIRON, &QAction::toggled, nullptr, nullptr);

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

    disconnect(comboBox_playerRoomStyle, qOverload<int>(&QComboBox::currentIndexChanged), nullptr, nullptr);
    disconnect(pushButton_playerRoomPrimaryColor, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(pushButton_playerRoomSecondaryColor, &QAbstractButton::clicked, nullptr, nullptr);
    disconnect(spinBox_playerRoomOuterDiameter, qOverload<int>(&QSpinBox::valueChanged), nullptr, nullptr);
    disconnect(spinBox_playerRoomInnerDiameter, qOverload<int>(&QSpinBox::valueChanged), nullptr, nullptr);
    disconnect(spinBox_roomSize, qOverload<int>(&QSpinBox::valueChanged), nullptr, nullptr);
    disconnect(spinBox_exitSize, qOverload<int>(&QSpinBox::valueChanged), nullptr, nullptr);
    disconnect(spinBox_borderSize, qOverload<int>(&QSpinBox::valueChanged), nullptr, nullptr);
    disconnect(doubleSpinBox_gridSize, qOverload<double>(&QDoubleSpinBox::valueChanged), nullptr, nullptr);
    disconnect(checkBox_largeAreaExitArrows, &QCheckBox::toggled, nullptr, nullptr);
    disconnect(checkBox_invertMapZoom, &QCheckBox::toggled, nullptr, nullptr);
    disconnect(checkbox_mMapperShowRoomBorders, &QCheckBox::toggled, nullptr, nullptr);
    disconnect(checkBox_drawUpperLowerLevels, &QCheckBox::toggled, nullptr, nullptr);
    disconnect(mMapperUseAntiAlias, &QCheckBox::toggled, nullptr, nullptr);
    if (mpDoubleSpinBox_mapSymbolFontFudge) {
        disconnect(mpDoubleSpinBox_mapSymbolFontFudge, qOverload<double>(&QDoubleSpinBox::valueChanged), nullptr, nullptr);
    }

    // Console buffer settings
    disconnect(checkBox_useMaxBufferSize, &QCheckBox::toggled, nullptr, nullptr);
}

void dlgProfilePreferences::clearHostDetails()
{
    code_editor_theme_selection_combobox->clear();
    script_preview_combobox->clear();
    edbeePreviewWidget->textDocument()->setText(QString());

    label_shortcutsConflictWarning->hide();
    label_shortcutsConflictWarning->clear();
    // Drop the stale shortcut data as well, otherwise a later call to
    // updateShortcutConflictWarning() (e.g. from slot_setAppearance()) would
    // re-show the warning for the no longer active profile:
    currentShortcuts.clear();

    checkBox_mVersionInTTYPE->setChecked(false);
    checkBox_mForceMXPProcessorOn->setChecked(false);
    mMapperUseAntiAlias->setChecked(false);
    checkbox_mMapperShowRoomBorders->setChecked(false);
    checkBox_drawUpperLowerLevels->setChecked(false);
    acceptServerGUI->setChecked(false);
    acceptServerMedia->setChecked(false);


    comboBox_dictionary->clear();
    checkBox_spellCheck->setChecked(false);
    checkBox_echoLuaErrors->setChecked(false);

    groupBox_downloadMapOptions->setVisible(false);

    need_reconnect_for_data_protocol->hide();

    need_reconnect_for_specialoption->hide();

    wrap_at_spinBox->clear();
    indent_wrapped_spinBox->clear();
    checkBox_undoServerWrap->setChecked(false);
    undo_server_wrap_width_spinBox->clear();

    show_sent_text_combobox->setCurrentIndex(static_cast<int>(Host::CommandEchoMode::ScriptControl));
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
    fontComboBox_displayFont->clear();
    spinBox_displayFontSize->setValue(14);
    checkBox_antiAlias->setChecked(false);
    mFORCE_MCCP_OFF->setChecked(false);
    mFORCE_GA_OFF->setChecked(false);
    mAlertOnNewData->setChecked(false);
    mFORCE_SAVE_ON_EXIT->setChecked(false);

    pushButton_chooseProfiles->setEnabled(false);
    pushButton_copyMap->setEnabled(false);
    if (mpMenu) {
        mpMenu->deleteLater();
        mpMenu = nullptr;
    }

    label_mapFileActionResult->hide();

    slot_hidePasswordMigrationLabel();

    doubleclick_ignore_lineedit->clear();

    comboBox_mapFileSaveFormatVersion->clear();
    comboBox_mapFileSaveFormatVersion->setEnabled(true);
    label_mapFileSaveFormatVersion->setEnabled(false);
    checkBox_showDefaultArea->setChecked(false);
    checkBox_showDefaultArea->hide();

    comboBox_encoding->clear();

    mSearchEngineMap.clear();
    search_engine_combobox->clear();

    radioButton_discordGameDetails->setChecked(true);
    comboBox_discordLargeIconPrivacy->setCurrentIndex(0);
    comboBox_discordSmallIconPrivacy->setCurrentIndex(0);
    checkBox_discordServerAccessToDetail->setChecked(false);
    checkBox_discordServerAccessToState->setChecked(false);
    checkBox_discordServerAccessToPartyInfo->setChecked(false);
    checkBox_discordServerAccessToTimerInfo->setChecked(false);
    lineEdit_discordUserName->clear();
    label_data_discordCurrentUser->clear();

    lineEdit_mmcpChatName->clear();
    lineEdit_mmcpPort->clear();
    lineEdit_mmcpChatMessagePrefix->clear();
    checkBox_mmcpAddChatMessageNewline->setChecked(true);
    checkBox_mmcpPrefixEmotes->setChecked(false);
    checkBox_mmcpSnoopInMainConsole->setChecked(true);

    checkBox_debugShowAllCodepointProblems->setChecked(false);
    checkBox_announceIncomingText->setChecked(false);
    checkBox_advertiseScreenReader->setChecked(false);
    checkBox_enableClosedCaption->setChecked(false);
    checkBox_enableBlinkText->setChecked(false);
    comboBox_blankLinesBehaviour->setCurrentIndex(0);

    groupBox_ssl_certificate->hide();
    frame_notificationArea->hide();
    checkBox_askTlsAvailable->setChecked(false);
    pushButton_forgetSavedSignIn->setEnabled(false);
    pushButton_forgetSavedSignIn->setVisible(false);
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

    auto config = edbeePreviewWidget->config();
    config->beginChanges();
    config->setSmartTab(true);
    config->setUseTabChar(false); // when you press Enter for a newline, pad with spaces and not tabs
    config->setCaretBlinkRate(200);
    config->setIndentSize(2);
    config->setThemeName(pHost->getEditorTheme());
    config->setCaretWidth(1);
    config->setShowWhitespaceMode((mudlet::self()->mEditorTextOptions & QTextOption::ShowTabsAndSpaces) ? edbee::TextEditorConfig::ShowWhitespaces : edbee::TextEditorConfig::HideWhitespaces);
    config->setUseLineSeparator(mudlet::self()->mEditorTextOptions & QTextOption::ShowLineAndParagraphSeparators);
    config->setFont(pHost->getDisplayFont());
    config->setAutocompleteAutoShow(pHost->mEditorAutoComplete);
    config->setRenderBidiContolCharacters(pHost->getEditorShowBidi());
    config->setAutocompleteMinimalCharacters(3);
    config->endChanges();
    edbeePreviewWidget->textDocument()->setLanguageGrammar(edbee::Edbee::instance()->grammarManager()->detectGrammarWithFilename(qsl("Buck.lua")));
    // disable shadows as their purpose (notify there is more text) is performed by scrollbars already
    edbeePreviewWidget->textScrollArea()->enableShadowWidget(false);

    populateThemesList();
    mudlet::loadEdbeeTheme(pHost->getEditorTheme(), pHost->getEditorThemeFile());
    populateScriptsList();

    // pre-select the current theme
    code_editor_theme_selection_combobox->lineEdit()->setPlaceholderText(qsl("Select theme"));
    auto themeIndex = code_editor_theme_selection_combobox->findText(pHost->getEditorTheme());
    code_editor_theme_selection_combobox->setCurrentIndex(themeIndex);
    slot_themeSelected(themeIndex);

    code_editor_theme_selection_combobox->setInsertPolicy(QComboBox::NoInsert);
    code_editor_theme_selection_combobox->setMaxVisibleItems(20);

    // pre-select the last shown script to preview
    script_preview_combobox->lineEdit()->setPlaceholderText(qsl("Select script to preview"));
    auto scriptIndex = script_preview_combobox->findData(QVariant::fromValue(QPair<QString, int>(pHost->mThemePreviewType, pHost->mThemePreviewItemID)));
    script_preview_combobox->setCurrentIndex(scriptIndex == -1 ? 1 : scriptIndex);
    slot_scriptSelected(scriptIndex == -1 ? 1 : scriptIndex);

    script_preview_combobox->setInsertPolicy(QComboBox::NoInsert);
    script_preview_combobox->setMaxVisibleItems(20);
    script_preview_combobox->setDuplicatesEnabled(true);

    theme_download_label->hide();

    checkBox_autocompleteLuaCode->setChecked(pHost->mEditorAutoComplete);
    checkBox_showBidi->setChecked(pHost->getEditorShowBidi());
    checkBox_showIdNumbers->setChecked(pHost->showIdsInEditor());

    // changes the theme being previewed
    connect(code_editor_theme_selection_combobox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &dlgProfilePreferences::slot_themeSelected);

    // allows people to select a script of theirs to preview
    connect(script_preview_combobox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &dlgProfilePreferences::slot_scriptSelected);

    // A deep link can put the dialog on the Editor page before there was a Host
    // to build this from, in which case that first visit has already happened:
    if (!mEditorThemesChecked && mpStackedWidget_categories->currentIndex() == mCategoryPageIndexes.value(qsl("editor"), -1)) {
        mEditorThemesChecked = true;
        maybeDownloadEditorThemes();
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
        setButtonColor(pushButton_black_2, pHost->mBlack_2);
        setButtonColor(pushButton_Lblack_2, pHost->mLightBlack_2);
        setButtonColor(pushButton_green_2, pHost->mGreen_2);
        setButtonColor(pushButton_Lgreen_2, pHost->mLightGreen_2);
        setButtonColor(pushButton_red_2, pHost->mRed_2);
        setButtonColor(pushButton_Lred_2, pHost->mLightRed_2);
        setButtonColor(pushButton_blue_2, pHost->mBlue_2);
        setButtonColor(pushButton_Lblue_2, pHost->mLightBlue_2);
        setButtonColor(pushButton_yellow_2, pHost->mYellow_2);
        setButtonColor(pushButton_Lyellow_2, pHost->mLightYellow_2);
        setButtonColor(pushButton_cyan_2, pHost->mCyan_2);
        setButtonColor(pushButton_Lcyan_2, pHost->mLightCyan_2);
        setButtonColor(pushButton_magenta_2, pHost->mMagenta_2);
        setButtonColor(pushButton_Lmagenta_2, pHost->mLightMagenta_2);
        setButtonColor(pushButton_white_2, pHost->mWhite_2);
        setButtonColor(pushButton_Lwhite_2, pHost->mLightWhite_2);

        setButtonColor(pushButton_foreground_color_2, pHost->mFgColor_2);
        setButtonColor(pushButton_background_color_2, pHost->mBgColor_2);
        setButtonColor(pushButton_lowerLevelColor, pHost->mLowerLevelColor);
        setButtonColor(pushButton_upperLevelColor, pHost->mUpperLevelColor);
        setButtonColor(pushButton_roomBorderColor, pHost->mRoomBorderColor);
        setButtonColor(pushButton_mapInfoBg, pHost->mMapInfoBg, true);
        setButtonColor(pushButton_roomCollisionBorderColor, pHost->mRoomCollisionBorderColor);
        setButtonColor(pushButton_mapGridColor, pHost->mMapGridColor, true);
    } else {
        // Using QColor() gives an "invalid" color:
        setButtonColor(pushButton_black_2, QColor());
        setButtonColor(pushButton_Lblack_2, QColor());
        setButtonColor(pushButton_green_2, QColor());
        setButtonColor(pushButton_Lgreen_2, QColor());
        setButtonColor(pushButton_red_2, QColor());
        setButtonColor(pushButton_Lred_2, QColor());
        setButtonColor(pushButton_blue_2, QColor());
        setButtonColor(pushButton_Lblue_2, QColor());
        setButtonColor(pushButton_yellow_2, QColor());
        setButtonColor(pushButton_Lyellow_2, QColor());
        setButtonColor(pushButton_cyan_2, QColor());
        setButtonColor(pushButton_Lcyan_2, QColor());
        setButtonColor(pushButton_magenta_2, QColor());
        setButtonColor(pushButton_Lmagenta_2, QColor());
        setButtonColor(pushButton_white_2, QColor());
        setButtonColor(pushButton_Lwhite_2, QColor());

        setButtonColor(pushButton_foreground_color_2, QColor());
        setButtonColor(pushButton_background_color_2, QColor());
        setButtonColor(pushButton_lowerLevelColor, QColor());
        setButtonColor(pushButton_upperLevelColor, QColor());
        setButtonColor(pushButton_roomBorderColor, QColor());
        setButtonColor(pushButton_mapInfoBg, QColor());
        setButtonColor(pushButton_roomCollisionBorderColor, QColor());
        setButtonColor(pushButton_mapGridColor, QColor());
    }
}

// The twelve tab objectNames that callers in mudlet.cpp, ctelnet.cpp and the
// functional tests may still pass are remapped onto the categories holding
// their contents now, a few of them spotlighting the card the caller meant. A
// new-style target is either a category key or "category/cardObjectName".
void dlgProfilePreferences::setTab(QString tab)
{
    const QHash<QString, QString> legacyTabs{{qsl("tab_general"), qsl("general")},
                                             {qsl("tab_inputLine"), qsl("inputLine")},
                                             {qsl("tab_display"), qsl("mainDisplay")},
                                             {qsl("tab_displayColors"), qsl("mainDisplay")},
                                             {qsl("tab_codeEditor"), qsl("editor")},
                                             {qsl("tab_mapper"), qsl("mapper")},
                                             {qsl("tab_mapperColors"), qsl("mapper")},
                                             {qsl("tab_chat"), qsl("chat")},
                                             {qsl("tab_connection"), qsl("privacy")},
                                             {qsl("tab_shortcuts"), qsl("shortcuts")},
                                             {qsl("tab_accessibility"), qsl("accessibility")},
                                             {qsl("tab_specialOptions"), qsl("connection")}};

    QString category = tab;
    QWidget* pSpotlightTarget = nullptr;
    if (const auto it = legacyTabs.constFind(tab); it != legacyTabs.constEnd()) {
        category = it.value();
        if (tab == qsl("tab_connection")) {
            // The one live deep link: a TLS failure wants the certificate
            // controls, not the top of the page
            pSpotlightTarget = groupBox_ssl;
        } else if (tab == qsl("tab_specialOptions")) {
            pSpotlightTarget = groupBox_specialOptions;
        } else if (tab == qsl("tab_displayColors")) {
            pSpotlightTarget = groupBox_displayColors;
        } else if (tab == qsl("tab_mapperColors")) {
            pSpotlightTarget = groupBox_mapperColors;
        }
    } else if (const int separator = tab.indexOf(QLatin1Char('/')); separator > 0) {
        category = tab.left(separator);
        pSpotlightTarget = findChild<QWidget*>(tab.mid(separator + 1));
    }

    showCategory(category, pSpotlightTarget);
}

void dlgProfilePreferences::slot_purgeMediaCache()
{
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }

    const auto [purged, message] = pHost->mpMedia->purgeMediaCache();

    if (!purged) {
        //: Shown after the "Clear stored media" button in preferences fails to empty the profile's media directory. %1 is the reason, which is not translated.
        pHost->postMessage(tr("[ WARN ]  - Could not clear the stored media: %1.").arg(message));
        return;
    }

    //: Shown after the "Clear stored media" button in preferences empties the profile's media directory.
    pHost->postMessage(tr("[  OK  ]  - The stored media files for this profile have been cleared."));
}

void dlgProfilePreferences::slot_resetColors()
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
    if (pHost->mpConsole) {
        pHost->mpConsole->resetConsoleBackgroundImage();
        pHost->mpConsole->changeColors();
    }

    // Copy across the colors to the Lua "color_table"
    pHost->updateAnsi16ColorsInTable();
}

void dlgProfilePreferences::slot_resetMapColors()
{
    Host* pHost = mpHost;

    if (!pHost) {
        return;
    }

    // As per values in Host.h:
    pHost->mFgColor_2 = QColorConstants::LightGray;
    pHost->mBgColor_2 = QColorConstants::Black;
    pHost->mLowerLevelColor = QColorConstants::DarkGray;
    pHost->mUpperLevelColor = QColorConstants::White;
    pHost->mRoomBorderColor = QColorConstants::LightGray;
    pHost->mRoomCollisionBorderColor = QColorConstants::Yellow;
    pHost->mBlack_2 = QColorConstants::Black;
    pHost->mLightBlack_2 = QColorConstants::DarkGray;
    pHost->mRed_2 = QColorConstants::DarkRed;
    pHost->mLightRed_2 = QColorConstants::Red;
    pHost->mGreen_2 = QColorConstants::DarkGreen;
    pHost->mLightGreen_2 = QColorConstants::Green;
    pHost->mBlue_2 = QColorConstants::DarkBlue;
    pHost->mLightBlue_2 = QColorConstants::Blue;
    pHost->mYellow_2 = QColorConstants::DarkYellow;
    pHost->mLightYellow_2 = QColorConstants::Yellow;
    pHost->mCyan_2 = QColorConstants::DarkCyan;
    pHost->mLightCyan_2 = QColorConstants::Cyan;
    pHost->mMagenta_2 = QColorConstants::DarkMagenta;
    pHost->mLightMagenta_2 = QColorConstants::Magenta;
    pHost->mWhite_2 = QColorConstants::LightGray;
    pHost->mLightWhite_2 = QColorConstants::White;
    pHost->mMapInfoBg = QColor(150, 150, 150, 120);
    pHost->mMapGridColor = QColor(211, 211, 211, 64);

    // This aplies the above colors to the buttons on display:
    setColors2();

    if (pHost->mpMap) {
        pHost->mpMap->updateArea(-1);
    }
}

void dlgProfilePreferences::setButtonAndProfileColor(QPushButton* button, QColor& presentColor, bool allowAlpha)
{
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }

    //: Generic pick color dialog title
    auto color = QColorDialog::getColor(presentColor, this, tr("Pick color"), allowAlpha ? QColorDialog::ShowAlphaChannel : QColorDialog::ColorDialogOptions());
    if (color.isValid()) {
        presentColor = color;

        auto console = pHost->mpConsole;
        if (console) {
            console->changeColors();
            // update the display properly when color selections change.
            console->mUpperPane->updateScreenView();
            console->mUpperPane->forceUpdate();
            if (console->mUpperPane->mIsTailMode) {
                // The upper pane having mIsTailMode true means lower pane is hidden
                console->mLowerPane->updateScreenView();
                console->mLowerPane->forceUpdate();
            }
        }

        if (button == pushButton_black || button == pushButton_lBlack || button == pushButton_red || button == pushButton_lRed || button == pushButton_green || button == pushButton_lGreen
            || button == pushButton_yellow || button == pushButton_lYellow || button == pushButton_blue || button == pushButton_lBlue || button == pushButton_magenta || button == pushButton_lMagenta
            || button == pushButton_cyan || button == pushButton_lCyan || button == pushButton_white || button == pushButton_lWhite) {
            pHost->updateAnsi16ColorsInTable();
        }

        const bool isAMapEnvColor = (button == pushButton_black_2 || button == pushButton_Lblack_2 || button == pushButton_red_2 || button == pushButton_Lred_2 || button == pushButton_green_2
                                     || button == pushButton_Lgreen_2 || button == pushButton_yellow_2 || button == pushButton_Lyellow_2 || button == pushButton_blue_2 || button == pushButton_Lblue_2
                                     || button == pushButton_magenta_2 || button == pushButton_Lmagenta_2 || button == pushButton_cyan_2 || button == pushButton_Lcyan_2 || button == pushButton_white_2
                                     || button == pushButton_Lwhite_2);

        if (isAMapEnvColor || button == pushButton_foreground_color_2 || button == pushButton_background_color_2 || button == pushButton_lowerLevelColor || button == pushButton_upperLevelColor
            || button == pushButton_mapInfoBg || button == pushButton_roomBorderColor || button == pushButton_roomCollisionBorderColor) {
            if (pHost->mpMap) {
                // Update the custom environment (room colors)
                if (isAMapEnvColor) {
                    pHost->mpMap->restore16ColorSet();
                }
                // Redraw the map with the modified color:
                pHost->mpMap->updateArea(-1);
            }
        }

        // Also set a contrasting foreground color so text will always be
        // visible:
        setButtonColor(button, color, allowAlpha);
    }
}

void dlgProfilePreferences::slot_setFgColor()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_foreground_color, pHost->mFgColor);
    }
}

void dlgProfilePreferences::slot_setBgColor()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_background_color, pHost->mBgColor, true);
    }
}

void dlgProfilePreferences::slot_setCommandFgColor()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_command_foreground_color, pHost->mCommandFgColor);
    }
}

void dlgProfilePreferences::slot_setCommandLineFgColor()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_command_line_foreground_color, pHost->mCommandLineFgColor);
    }
}

void dlgProfilePreferences::slot_setCommandLineBgColor()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_command_line_background_color, pHost->mCommandLineBgColor);
    }
}

void dlgProfilePreferences::slot_setCommandBgColor()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_command_background_color, pHost->mCommandBgColor);
    }
}

void dlgProfilePreferences::slot_setColorBlack()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_black, pHost->mBlack);
    }
}

void dlgProfilePreferences::slot_setColorLightBlack()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_lBlack, pHost->mLightBlack);
    }
}

void dlgProfilePreferences::slot_setColorRed()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_red, pHost->mRed);
    }
}

void dlgProfilePreferences::slot_setColorLightRed()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_lRed, pHost->mLightRed);
    }
}

void dlgProfilePreferences::slot_setColorGreen()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_green, pHost->mGreen);
    }
}

void dlgProfilePreferences::slot_setColorLightGreen()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_lGreen, pHost->mLightGreen);
    }
}

void dlgProfilePreferences::slot_setColorYellow()

{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_yellow, pHost->mYellow);
    }
}

void dlgProfilePreferences::slot_setColorLightYellow()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_lYellow, pHost->mLightYellow);
    }
}

void dlgProfilePreferences::slot_setColorBlue()

{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_blue, pHost->mBlue);
    }
}

void dlgProfilePreferences::slot_setColorLightBlue()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_lBlue, pHost->mLightBlue);
    }
}

void dlgProfilePreferences::slot_setColorMagenta()

{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_magenta, pHost->mMagenta);
    }
}

void dlgProfilePreferences::slot_setColorLightMagenta()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_lMagenta, pHost->mLightMagenta);
    }
}

void dlgProfilePreferences::slot_setColorCyan()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_cyan, pHost->mCyan);
    }
}

void dlgProfilePreferences::slot_setColorLightCyan()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_lCyan, pHost->mLightCyan);
    }
}

void dlgProfilePreferences::slot_setColorWhite()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_white, pHost->mWhite);
    }
}

void dlgProfilePreferences::slot_setColorLightWhite()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_lWhite, pHost->mLightWhite);
    }
}

void dlgProfilePreferences::slot_setMapExitsColor()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_foreground_color_2, pHost->mFgColor_2);
    }
}

void dlgProfilePreferences::slot_setMapBgColor()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_background_color_2, pHost->mBgColor_2, true);
// if 3D map, update transparency flags
#if defined(INCLUDE_3DMAPPER)
        if (pHost->mpMap->mpMapper->glWidget) {
            QOpenGLWidget* map = pHost->mpMap->mpMapper->glWidget;
            if (pHost->mBgColor_2.alpha() < 255) {
                map->setAttribute(Qt::WA_OpaquePaintEvent, false);
                map->setAttribute(Qt::WA_AlwaysStackOnTop, true);
            } else {
                map->setAttribute(Qt::WA_OpaquePaintEvent, true);
                map->setAttribute(Qt::WA_AlwaysStackOnTop, false);
            }
        }
#endif
    }
}

void dlgProfilePreferences::slot_setLowerLevelColor()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_lowerLevelColor, pHost->mLowerLevelColor);
    }
}

void dlgProfilePreferences::slot_setUpperLevelColor()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_upperLevelColor, pHost->mUpperLevelColor);
    }
}

void dlgProfilePreferences::slot_setMapRoomBorderColor()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_roomBorderColor, pHost->mRoomBorderColor);
    }
}

void dlgProfilePreferences::slot_setMapRoomCollisionBorderColor()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_roomCollisionBorderColor, pHost->mRoomCollisionBorderColor);
    }
}

void dlgProfilePreferences::slot_setMapGridColor()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_mapGridColor, pHost->mMapGridColor, true);
    }
}

void dlgProfilePreferences::slot_forgetSavedSignIn()
{
    Host* pHost = mpHost;
    if (!pHost || !pHost->mpAuth) {
        return;
    }

    const auto reply = QMessageBox::question(this,
                                             //: Title of the dialog asking the user to confirm removing their saved sign-in.
                                             tr("Forget saved sign-in?"),
                                             //: Body of the dialog asking the user to confirm removing their saved sign-in; they will need to sign in again next time.
                                             tr("This will remove the saved sign-in for this profile. You will need to sign in again next time. Continue?"),
                                             QMessageBox::Yes | QMessageBox::No,
                                             QMessageBox::No);
    frame_notificationArea->show();
    notificationAreaIconLabelInformation->show();
    notificationAreaMessageBox->show();
    if (reply == QMessageBox::Yes) {
        // forgetSavedSignIn() removes the token asynchronously; only report success (and disable the
        // button) once the removal actually resolves, so a failed keychain removal cannot leave a stale
        // reconnect token behind while the UI claims it is gone. QPointers guard against the dialog or
        // host closing before the removal answers.
        QPointer<dlgProfilePreferences> safeDialog = this;
        QPointer<Host> safeHost = pHost;
        pHost->mpAuth->forgetSavedSignIn([safeDialog, safeHost](bool success) {
            if (success) {
                if (safeDialog) {
                    // Nothing is left to forget until a fresh sign-in mints a new token.
                    safeDialog->pushButton_forgetSavedSignIn->setEnabled(false);
                    //: Shown after the user's saved sign-in has actually been removed.
                    safeDialog->notificationAreaMessageBox->setText(dlgProfilePreferences::tr("The saved sign-in has been forgotten."));
                }
                if (safeHost) {
                    //: Shown in the main console after the user's saved sign-in has actually been removed.
                    safeHost->postMessage(dlgProfilePreferences::tr("[  OK  ]  - The saved sign-in for this profile has been forgotten."));
                }
            } else {
                if (safeDialog) {
                    //: Shown when removing the saved sign-in failed, so it may still be present.
                    safeDialog->notificationAreaMessageBox->setText(dlgProfilePreferences::tr("Could not remove the saved sign-in; it may still be present."));
                }
                if (safeHost) {
                    //: Shown in the main console when removing the saved sign-in failed, so it may still be present.
                    safeHost->postMessage(dlgProfilePreferences::tr("[ WARN ]  - Could not remove the saved sign-in; it may still be present."));
                }
            }
        });
    } else {
        //: Shown when the user cancels removing their saved sign-in.
        notificationAreaMessageBox->setText(tr("No changes were made to the saved sign-in."));
        //: Shown in the main console when the user cancels removing their saved sign-in.
        pHost->postMessage(tr("[ INFO ]  - Cancelled: no changes were made to the saved sign-in."));
    }
}

void dlgProfilePreferences::slot_setMapInfoBgColor()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_mapInfoBg, pHost->mMapInfoBg, true);
    }
}

void dlgProfilePreferences::slot_setMapColorBlack()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_black_2, pHost->mBlack_2);
    }
}

void dlgProfilePreferences::slot_setMapColorLightBlack()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_Lblack_2, pHost->mLightBlack_2);
    }
}

void dlgProfilePreferences::slot_setMapColorRed()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_red_2, pHost->mRed_2);
    }
}

void dlgProfilePreferences::slot_setMapColorLightRed()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_Lred_2, pHost->mLightRed_2);
    }
}

void dlgProfilePreferences::slot_setMapColorGreen()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_green_2, pHost->mGreen_2);
    }
}

void dlgProfilePreferences::slot_setMapColorLightGreen()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_Lgreen_2, pHost->mLightGreen_2);
    }
}

void dlgProfilePreferences::slot_setMapColorBlue()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_blue_2, pHost->mBlue_2);
    }
}

void dlgProfilePreferences::slot_setMapColorLightBlue()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_Lblue_2, pHost->mLightBlue_2);
    }
}

void dlgProfilePreferences::slot_setMapColorYellow()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_yellow_2, pHost->mYellow_2);
    }
}

void dlgProfilePreferences::slot_setMapColorLightYellow()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_Lyellow_2, pHost->mLightYellow_2);
    }
}

void dlgProfilePreferences::slot_setMapColorCyan()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_cyan_2, pHost->mCyan_2);
    }
}

void dlgProfilePreferences::slot_setMapColorLightCyan()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_Lcyan_2, pHost->mLightCyan_2);
    }
}

void dlgProfilePreferences::slot_setMapColorMagenta()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_magenta_2, pHost->mMagenta_2);
    }
}

void dlgProfilePreferences::slot_setMapColorLightMagenta()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_Lmagenta_2, pHost->mLightMagenta_2);
    }
}

void dlgProfilePreferences::slot_setMapColorWhite()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_white_2, pHost->mWhite_2);
    }
}

void dlgProfilePreferences::slot_setMapColorLightWhite()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_Lwhite_2, pHost->mLightWhite_2);
    }
}

void dlgProfilePreferences::slot_downloadMap()
{
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }
    if (!pHost->mpMap->mpMapper) {
        // CHECK: What happens if we are NOT the current profile anymore?
        pHost->showHideOrCreateMapper(false);
    }

    pHost->mpMap->downloadMap();
}

void dlgProfilePreferences::fillOutMapHistory()
{
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }

    const QString profile_name = pHost->getName();
    auto const locale = mudlet::self()->getUserLocale();
    int longestMapHistoryLength = 0;
    const QIcon icon_autoSave{QIcon::fromTheme(qsl("document-save"), QIcon(qsl(":/icons/document-save.png")))};
    const QIcon icon_xmlFile{QIcon::fromTheme(qsl("application-xml"), QIcon(qsl(":/icons/application-xml.png")))};
    const QIcon icon_jsonFile{QIcon::fromTheme(qsl("application-json"), QIcon(qsl(":/icons/application-json.png")))};
    QString dateTimeFormat = mudlet::self()->getUserLocale().dateTimeFormat();
    if (dateTimeFormat.contains(QLatin1Char('t'))) {
        // There is a timezone identifier in there - which (apart from perhaps
        // the period around DST changes) we don't really need and which takes
        // up space:
        if (dateTimeFormat.contains(QLatin1String(" t"))) {
            // Deal with the space if the time zone is appended to the end of
            // the string:
            dateTimeFormat.remove(QLatin1String(" t"), Qt::CaseSensitive);
        } else {
            dateTimeFormat.remove(QLatin1Char('t'), Qt::CaseSensitive);
        }
    }
    const QRegularExpression mapSaveRegularExpression{qsl("(\\d+)\\-(\\d+)\\-(\\d+)#(\\d+)\\-(\\d+)\\-(\\d+)(?:map)?\\.(dat|xml|json)"), QRegularExpression::CaseInsensitiveOption};
    QDir mapSaveDir(mudlet::getMudletPath(enums::profileMapsPath, profile_name).append(QLatin1Char('/')));
    mapSaveDir.setSorting(QDir::Time);
    const QStringList mapSaveEntries = mapSaveDir.entryList(QDir::Files | QDir::NoDotAndDotDot, QDir::Time);
    for (const auto& entry : mapSaveEntries) {
        const QRegularExpressionMatch match = mapSaveRegularExpression.match(entry);
        const QString mapPathFileName = mapSaveDir.absoluteFilePath(entry);
        if (match.capturedStart() != -1) {
            // A recognised date-time stamp file name of any Mudlet map file type:
            QString day;
            const QString month = match.captured(2);
            QString year;
            const QString hour = match.captured(4);
            const QString minute = match.captured(5);
            const QString second = match.captured(6);
            if (match.captured(1).toInt() > 31 && match.captured(3).toInt() >= 1 && match.captured(3).toInt() <= 31) {
                year = match.captured(1);
                day = match.captured(3);
            } else {
                day = match.captured(1);
                year = match.captured(3);
            }
            const QString extension = match.captured(7);
            const QDateTime datetime(QDate(year.toInt(), month.toInt(), day.toInt()), QTime(hour.toInt(), minute.toInt(), second.toInt()));
            const QString itemText = locale.toString(datetime, dateTimeFormat);
            longestMapHistoryLength = qMax(longestMapHistoryLength, itemText.size());
            if (!extension.compare(QLatin1String("xml"), Qt::CaseInsensitive)) {
                comboBox_mapHistory->addItem(icon_xmlFile, itemText, QVariant(mapPathFileName));
            } else {
                if (!extension.compare(QLatin1String("json"), Qt::CaseInsensitive)) {
                    comboBox_mapHistory->addItem(icon_jsonFile, itemText, QVariant(mapPathFileName));
                } else {
                    // Must be a .dat
                    comboBox_mapHistory->addItem(itemText, QVariant(mapPathFileName));
                }
            }
        } else if (!entry.compare(QLatin1String("autosave.dat"), Qt::CaseInsensitive)) {
            // The auto-saved file:
            const QFileInfo fileInfo(mapSaveDir, entry);
            auto lastModified = fileInfo.lastModified();
            const QString itemText = locale.toString(lastModified, dateTimeFormat);
            longestMapHistoryLength = qMax(longestMapHistoryLength, itemText.size());
            comboBox_mapHistory->addItem(icon_autoSave, itemText, QVariant(mapPathFileName));
        } else {
            // Some other file name with a recognised extension:
            longestMapHistoryLength = qMax(longestMapHistoryLength, entry.size());
            if (entry.endsWith(QLatin1String("xml"), Qt::CaseInsensitive)) {
                comboBox_mapHistory->addItem(icon_xmlFile, entry, QVariant(mapPathFileName));
            } else {
                if (entry.endsWith(QLatin1String("json"), Qt::CaseInsensitive)) {
                    comboBox_mapHistory->addItem(icon_jsonFile, entry, QVariant(mapPathFileName));
                } else {
                    comboBox_mapHistory->addItem(entry, QVariant(mapPathFileName)); // if it has a custom name, use it as it is
                }
            }
        }
    }
    if (comboBox_mapHistory->count()) {
        comboBox_mapHistory->setEnabled(true);
        pushButton_loadHistoricMap->setEnabled(true);
        connect(pushButton_loadHistoricMap, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_loadHistoryMap);
    }
}

void dlgProfilePreferences::loadMap(const QString& fileName)
{
    auto pHost = mpHost;
    if (!pHost) {
        return;
    }
    label_mapFileActionResult->show();

    // Ensure the setting is already made as the TConsole::loadMap(...) uses
    // the set value:
    const bool showAuditErrors = mudlet::self()->showMapAuditErrors();
    mudlet::self()->setShowMapAuditErrors(checkBox_reportMapIssuesOnScreen->isChecked());

    bool success = false;
    label_mapFileActionResult->setText(tr("Loading map - please wait..."));
    qApp->processEvents(); // Needed to make the above message show up when loading big maps
    if (fileName.endsWith(qsl(".xml"), Qt::CaseInsensitive)) {
        qApp->processEvents(); // Needed to make the above message show up when loading big maps
        success = pHost->mpConsole->importMap(fileName);

    } else {
        if (fileName.endsWith(qsl(".json"), Qt::CaseInsensitive)) {
            auto [localSuccess, errorMessage] = pHost->mpMap->readJsonMapFile(fileName);
            success = localSuccess;
            if (!localSuccess) {
                pHost->postMessage(tr("[ ERROR ] - Unable to load JSON map file: %1\n"
                                      "reason: %2.")
                                           .arg(fileName, errorMessage));
            }

        } else {
            success = pHost->mpConsole->loadMap(fileName);
        }
    }

    if (success) {
        // TMap::audit() is what paints up the "[  OK  ]" message for the map load.
        pHost->mpMap->audit();
        label_mapFileActionResult->setText(tr("Loaded map from %1.").arg(fileName.toHtmlEscaped()));
    } else {
        label_mapFileActionResult->setText(tr("Could not load map from %1.").arg(fileName.toHtmlEscaped()));
    }

    QTimer::singleShot(10s, this, &dlgProfilePreferences::slot_hideActionLabel);

    // Restore setting immediately before we used it
    mudlet::self()->setShowMapAuditErrors(showAuditErrors);
}

void dlgProfilePreferences::slot_loadHistoryMap()
{
    auto mapPathFileData = comboBox_mapHistory->currentData();
    if (!mapPathFileData.isValid() || mapPathFileData.toString().isEmpty()) {
        return;
    }
    loadMap(mapPathFileData.toString());
}

void dlgProfilePreferences::slot_loadMap()
{
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }

    auto loadExtensions(QStringList() << tr("Any map file (*.dat *.json *.xml)", "Do not change extensions (in braces) as they are used programmatically")
                                      << tr("Mudlet binary map (*.dat)", "Do not change extensions (in braces) as they are used programmatically")
                                      << tr("Mudlet JSON map (*.json)", "Do not change extensions (in braces) as they are used programmatically")
                                      << tr("Mudlet XML map (*.xml)", "Do not change extensions (in braces) as they are used programmatically")
                                      << tr("Any file (*)", "Do not change extensions (in braces) as they are used programmatically"));


    QFileDialog* dialog = new QFileDialog(this);
    dialog->setWindowTitle(tr("Load Mudlet map"));
    QSettings& settings = *mudlet::getQSettings();
    QString lastDir = settings.value("lastFileDialogLocation", mudlet::getMudletPath(enums::profileHomePath, pHost->getName())).toString();
    dialog->setDirectory(lastDir);
    dialog->setNameFilter(loadExtensions.join(qsl(";;")));
    connect(dialog, &QDialog::finished, this, [=, this](int result) {
        if (result == QDialog::Rejected) {
            return;
        }

        auto fileName = dialog->selectedFiles().constFirst();
        loadMap(fileName);
        QSettings& settings = *mudlet::getQSettings();
        QString lastDir = QFileInfo(fileName).absolutePath();
        settings.setValue("lastFileDialogLocation", lastDir);
    });
    dialog->open();
}

void dlgProfilePreferences::slot_saveMap()
{
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }

    auto datFilter = tr("Mudlet binary map (*.dat)", "Do not change extensions (in braces) as they are used programmatically");
    auto jsonFilter = tr("Mudlet JSON map (*.json)", "Do not change extensions (in braces) as they are used programmatically");
    auto saveExtensions(QStringList() << datFilter << jsonFilter);

    QFileDialog* dialog = new QFileDialog(this);
    dialog->setWindowTitle(tr("Save Mudlet map"));
    QSettings& settings = *mudlet::getQSettings();
    QString lastDir = settings.value("lastFileDialogLocation", mudlet::getMudletPath(enums::profileHomePath, pHost->getName())).toString();
    dialog->setDirectory(lastDir);
    dialog->setNameFilter(saveExtensions.join(qsl(";;")));
    dialog->setAcceptMode(QFileDialog::AcceptSave);
    dialog->setDefaultSuffix(qsl("dat"));
    connect(dialog, &QFileDialog::filterSelected, this, [=](const QString& filter) {
        if (filter == datFilter) {
            dialog->setDefaultSuffix(qsl("dat"));
        }
        if (filter == jsonFilter) {
            dialog->setDefaultSuffix(qsl("json"));
        }
    });

    connect(dialog, &QFileDialog::finished, this, [=, this](int result) {
        if (result == QDialog::Rejected) {
            return;
        }

        auto fileName = dialog->selectedFiles().constFirst();

        QSettings& settings = *mudlet::getQSettings();
        QString lastDir = QFileInfo(fileName).absolutePath();
        settings.setValue("lastFileDialogLocation", lastDir);

        label_mapFileActionResult->show();
        label_mapFileActionResult->setText(tr("Saving map - please wait..."));
        qApp->processEvents(); // Copied from "Loading map - please wait..." case
        // Just in case is needed to make the above message
        // show up when saving big maps

        // Ensure the setting is already made as the saveMap(...) uses the set value
        const bool showAuditErrors = mudlet::self()->showMapAuditErrors();
        mudlet::self()->setShowMapAuditErrors(checkBox_reportMapIssuesOnScreen->isChecked());

        bool success = false;
        if (!fileName.endsWith(qsl(".json"), Qt::CaseInsensitive)) {
            success = pHost->mpConsole->saveMap(fileName, comboBox_mapFileSaveFormatVersion->currentData().toInt());
        } else {
            success = pHost->mpMap->writeJsonMapFile(fileName).first;
        }

        if (success) {
            label_mapFileActionResult->setText(tr("Saved map to %1.").arg(fileName.toHtmlEscaped()));
        } else {
            label_mapFileActionResult->setText(tr("Could not save map to %1.").arg(fileName.toHtmlEscaped()));
        }
        mudlet::self()->setShowMapAuditErrors(showAuditErrors);

        QTimer::singleShot(10s, this, &dlgProfilePreferences::slot_hideActionLabel);
    });
    dialog->open();
}

QString dlgProfilePreferences::mapSaveLoadDirectory(Host* pHost)
{
    const QString mapsPath = mudlet::getMudletPath(enums::profileMapsPath, pHost->getName());
    const QDir mapsDir = QDir(mapsPath);
    return mapsDir.exists() ? mapsPath : mudlet::getMudletPath(enums::profileHomePath, pHost->getName());
}

void dlgProfilePreferences::slot_hideActionLabel()
{
    label_mapFileActionResult->hide();
}

void dlgProfilePreferences::slot_hidePasswordMigrationLabel()
{
    label_password_migration_notification->hide();
}

void dlgProfilePreferences::slot_passwordStorageLocationChanged(int index)
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

void dlgProfilePreferences::slot_copyMap()
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
            const QString toProfileName = _action->text();
            toProfilesRoomIdMap.insert(toProfileName, 0);
            // 0 is used as sentinel value that we don't have a valid Id yet
            // for the given Host - the contents of this map will be used to
            // update, or rather REPLACE TMap::mRoomIdHash

            // Check for the destination directory for the other profiles
            const QDir toProfileDir;
            const QString toProfileDirPathString = mudlet::getMudletPath(enums::profileHomePath, pHost->getName());
            if (!toProfileDir.exists(toProfileDirPathString)) {
                if (!toProfileDir.mkpath(toProfileDirPathString)) {
                    const QString errMsg = tr("[ ERROR ] - Unable to use or create directory to store map for other profile \"%1\".\n"
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
    QMap<QString, QSharedPointer<Host>> activeOtherHostMap;
    for (auto pOtherHost : mudlet::self()->getHostManager()) {
        if (pOtherHost->mpConsole && (pOtherHost != pHost)) {
            const auto& otherHostName = pOtherHost->getName();
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
    const bool savedOldAuditErrorsToConsoleEnabledSetting = mudlet::self()->showMapAuditErrors();
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
            qDebug() << "dlgProfilePreference::slot_copyMap() in other ACTIVE profile:" << itOtherProfile.key() << "\n    the player was located in:" << itOtherProfile.value();
            if (pHost->mpMap->mpRoomDB->getRoom(itOtherProfile.value())) {
                // That room IS in the map we are copying across, so update the
                // local record of it for the map for that profile:
                pHost->mpMap->mRoomIdHash[itOtherProfile.key()] = itOtherProfile.value();
            }
            continue;
        }

        // Most of these we'll just get for debugging!
        QString otherProfileFileUsed;
        qsizetype otherProfileRoomCount;
        qsizetype otherProfileAreaCount;
        int otherProfileVersion;
        int otherProfileCurrentRoomId; // What we are looking for!
        if (pHost->mpMap->retrieveMapFileStats(itOtherProfile.key(), &otherProfileFileUsed, &otherProfileVersion, &otherProfileCurrentRoomId, &otherProfileAreaCount, &otherProfileRoomCount)) {
            qDebug() << "dlgProfilePreference::slot_copyMap() in other INACTIVE profile:" << itOtherProfile.key() << "\n    the file examined was:" << otherProfileFileUsed
                     << "\n    it was of version:" << otherProfileVersion << "\n    it had an area count of:" << otherProfileAreaCount << "\n    it had a room count of:" << otherProfileRoomCount
                     << "\n    the player was located in:" << otherProfileCurrentRoomId;
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
    const int oldSaveVersionFormat = pHost->mpMap->mSaveVersion;
    pHost->mpMap->mSaveVersion = comboBox_mapFileSaveFormatVersion->currentData().toInt();

    if (!pHost->mpConsole->saveMap(QString())) {
        label_mapFileActionResult->setText(tr("Could not backup the map - saving it failed."));
        QTimer::singleShot(10s, this, &dlgProfilePreferences::slot_hideActionLabel);
        return;
    }

    // Then restore prior version
    pHost->mpMap->mSaveVersion = oldSaveVersionFormat;

    // work out which map is latest in THIS profile - which SHOULD be the one
    // we just saved!
    QString thisProfileLatestMapPathFileName;
    QFile thisProfileLatestMapFile;
    const QString sourceMapFolder(mudlet::getMudletPath(enums::profileMapsPath, pHost->getName()));
    const QStringList mProfileList = QDir(sourceMapFolder).entryList(QDir::Files | QDir::NoDotAndDotDot, QDir::Time);
    for (unsigned int i = 0, total = mProfileList.size(); i < total; ++i) {
        thisProfileLatestMapPathFileName = mProfileList.at(i);
        if (thisProfileLatestMapPathFileName.isEmpty()) {
            continue;
        }

        thisProfileLatestMapFile.setFileName(qsl("%1/%2").arg(sourceMapFolder, thisProfileLatestMapPathFileName));
        break;
    }

    if (thisProfileLatestMapFile.fileName().isEmpty()) {
        label_mapFileActionResult->setText(tr("Could not copy the map - failed to work out which map file we just saved the map as!"));
        QTimer::singleShot(10s, this, &dlgProfilePreferences::slot_hideActionLabel);
        return;
    }

    // Make the copies into the destination profiles (for all to profiles whether
    // in use or not):
    itOtherProfile.toFront();
    while (itOtherProfile.hasNext()) {
        itOtherProfile.next();
        const QString otherHostName = itOtherProfile.key();
        // Copy over into the profiles map folder, so it is loaded first when map is open - this covers the offline case
        label_mapFileActionResult->setText(tr("Copying over map to %1 - please wait...").arg(otherHostName));
        qApp->processEvents(); // Copied from "Loading map - please wait..." case
                               // Just in case is needed to make the above message
                               // show up when saving big maps

        if (!thisProfileLatestMapFile.copy(mudlet::getMudletPath(enums::profileMapPathFileName, otherHostName, thisProfileLatestMapPathFileName))) {
            label_mapFileActionResult->setText(tr("Could not copy the map to %1 - unable to copy the new map file over.").arg(otherHostName));
            QTimer::singleShot(10s, this, &dlgProfilePreferences::slot_hideActionLabel);
            continue; // Try again with next profile
        }
        label_mapFileActionResult->setText(tr("Map copied successfully to other profile %1.").arg(otherHostName));
        qApp->processEvents(); // Copied from "Loading map - please wait..." case
                               // Just in case is needed to make the above message
                               // show up when saving big maps
    }

    // Finally, signal the other profiles to reload their maps:
    mudlet::self()->requestProfilesToReloadMaps(toProfilesRoomIdMap.keys());
    // GOTCHA: keys() is a QList<QString>, however, though it IS equivalent to a
    // QStringList in many ways, the SLOT/SIGNAL system treats them as different
    // - I thinK - so use QList<QString> throughout the SIGNAL/SLOT links Slysven!
    label_mapFileActionResult->setText(tr("Map copied, now signalling other profiles to reload it."));
    QTimer::singleShot(10s, this, &dlgProfilePreferences::slot_hideActionLabel);

    // CHECK: Race condition? We might be changing this while other profile
    // are accessing it...
    mudlet::self()->setShowMapAuditErrors(savedOldAuditErrorsToConsoleEnabledSetting);
}

void dlgProfilePreferences::slot_setLogDir()
{
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }

    QSettings& settings = *mudlet::getQSettings();
    QString lastDir = settings.value("lastFileDialogLocation", mudlet::getMudletPath(enums::profileHomePath, pHost->getName())).toString();

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
     * the widget title bar close button was pressed on the Profile Preferences
     * dialog while the directory selector is open...!
     */
    // Seems to return "." when Cancel is hit:
    const QString currentLogDir = QFileDialog::getExistingDirectory(this, tr("Where should Mudlet save log files?"), (mLogDirPath.isEmpty() ? lastDir : mLogDirPath), QFileDialog::DontUseNativeDialog);

    if (!currentLogDir.isEmpty() && currentLogDir != nullptr) {
        settings.setValue("lastFileDialogLocation", currentLogDir);
        // Disable pushButton_resetLogDir and clear
        // lineEdit_logFileFolder if the directory is set to the
        // default path
        if (currentLogDir == mudlet::getMudletPath(enums::profileReplayAndLogFilesPath, pHost->getName())) {
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
        // The line edit is read-only, so it emits none of the signals instant
        // apply listens to:
        slot_scheduleApply();
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
    slot_scheduleApply();

    return;
}

void dlgProfilePreferences::slot_logFileNameFormatChange(const int index)
{
    Q_UNUSED(index)

    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }

    const bool isShown = comboBox_logFileNameFormat->currentData().toString().isEmpty();
    lineEdit_logFileName->setVisible(isShown);
    label_logFileName->setVisible(isShown);
    label_logFileNameExtension->setVisible(isShown);
}

// Writes every setting back, off a 400ms debounce after any control changes and
// once more from closeEvent(). Only the settings whose controls the user
// actually changed: the dialog stays open while scripts run, and writing back a
// control that is merely showing a stale value would revert what a script had
// just set (#10165). Hence the dirty(...) guard on every write - the values
// they are compared against come from snapshotValues().
void dlgProfilePreferences::applyAll()
{
    if (mPopulating) {
        return;
    }

    mudlet* pMudlet = mudlet::self();
    Host* pHost = mpHost;
    if (pHost) {
        auto console = pHost->mpConsole;
        if (dirty(comboBox_dictionary) && comboBox_dictionary->isEnabled() && comboBox_dictionary->currentIndex() >= 0) {
            pHost->setSpellDic(comboBox_dictionary->currentData().toString());
        }

        if (dirty(checkBox_spellCheck)) {
            pHost->mEnableSpellCheck = checkBox_spellCheck->isChecked();
        }
        if (anyDirty({radioButton_userDictionary_common, radioButton_userDictionary_profile})) {
            if (radioButton_userDictionary_common->isChecked()) {
                pHost->setUserDictionaryOptions(true, true);
            } else {
                pHost->setUserDictionaryOptions(true, false);
            }
        }

        if (dirty(wrap_at_spinBox)) {
            const int priorWrapAt = pHost->mWrapAt;
            pHost->mWrapAt = wrap_at_spinBox->value();

            if (priorWrapAt != pHost->mWrapAt) {
                slot_changeWrapAt();
            }
        }

        pHost->updateDisplayDimensions();
        if (dirty(indent_wrapped_spinBox)) {
            pHost->mWrapIndentCount = indent_wrapped_spinBox->value();
        }
        if (dirty(hanging_indent_wrapped_spinBox)) {
            pHost->mWrapHangingIndentCount = hanging_indent_wrapped_spinBox->value();
        }
        if (dirty(checkBox_undoServerWrap)) {
            pHost->mUndoServerWrap = checkBox_undoServerWrap->isChecked();
        }
        if (dirty(undo_server_wrap_width_spinBox)) {
            pHost->mUndoServerWrapWidth = undo_server_wrap_width_spinBox->value();
        }

        // Save console buffer settings and apply them
        if (anyDirty({checkBox_useMaxBufferSize, console_buffer_size_spinBox})) {
            const bool useMaxBuffer = dirty(checkBox_useMaxBufferSize) ? checkBox_useMaxBufferSize->isChecked() : pHost->getUseMaxConsoleBufferSize();
            int newBufferSize;

            if (useMaxBuffer && pHost->mpConsole) {
                newBufferSize = pHost->mpConsole->buffer.getMaxBufferSize();
            } else {
                newBufferSize = dirty(console_buffer_size_spinBox) ? console_buffer_size_spinBox->value() : pHost->getConsoleBufferSize();
            }

            // Calculate batch delete size as 5% of buffer size (minimum 100)
            const int newBatchDeleteSize = std::max(100, newBufferSize / 5);

            if (pHost->getConsoleBufferSize() != newBufferSize || pHost->getUseMaxConsoleBufferSize() != useMaxBuffer) {
                pHost->setConsoleBufferSize(newBufferSize);
                pHost->setUseMaxConsoleBufferSize(useMaxBuffer);

                // Apply the new buffer size to the main console
                if (pHost->mpConsole) {
                    pHost->mpConsole->buffer.setBufferSize(newBufferSize, newBatchDeleteSize);
                }
            }
        }

        if (dirty(show_sent_text_combobox)) {
            pHost->mCommandEchoMode = static_cast<Host::CommandEchoMode>(show_sent_text_combobox->currentIndex());
        }
        if (dirty(auto_clear_input_line_checkbox)) {
            pHost->mAutoClearCommandLineAfterSend = auto_clear_input_line_checkbox->isChecked();
        }
        if (dirty(disable_password_masking_checkbox)) {
            pHost->mDisablePasswordMasking = disable_password_masking_checkbox->isChecked();
        }
        if (dirty(checkBox_highlightHistory)) {
            pHost->mHighlightHistory = checkBox_highlightHistory->isChecked();
        }
        if (dirty(command_separator_lineedit)) {
            pHost->mCommandSeparator = command_separator_lineedit->text();
        }
        if (dirty(acceptServerGUI)) {
            pHost->mAcceptServerGUI = acceptServerGUI->isChecked();
        }
        if (dirty(acceptServerMedia)) {
            pHost->mAcceptServerMedia = acceptServerMedia->isChecked();
        }
        if (dirty(checkBox_USE_IRE_DRIVER_BUGFIX)) {
            pHost->set_USE_IRE_DRIVER_BUGFIX(checkBox_USE_IRE_DRIVER_BUGFIX->isChecked());
        }
        if (dirty(checkBox_enableTextAnalyzer)) {
            pHost->mEnableTextAnalyzer = checkBox_enableTextAnalyzer->isChecked();
        }
        if (dirty(checkBox_mUSE_FORCE_LF_AFTER_PROMPT)) {
            pHost->mUSE_FORCE_LF_AFTER_PROMPT = checkBox_mUSE_FORCE_LF_AFTER_PROMPT->isChecked();
        }
        if (dirty(USE_UNIX_EOL)) {
            pHost->mUSE_UNIX_EOL = USE_UNIX_EOL->isChecked();
        }
        if (dirty(checkBox_runAllKeyBindings)) {
            pHost->getKeyUnit()->mRunAllKeyMatches = checkBox_runAllKeyBindings->isChecked();
        }
        if (dirty(mFORCE_MCCP_OFF)) {
            pHost->mFORCE_NO_COMPRESSION = mFORCE_MCCP_OFF->isChecked();
        }
        if (dirty(mFORCE_GA_OFF)) {
            pHost->mFORCE_GA_OFF = mFORCE_GA_OFF->isChecked();
        }
        if (dirty(mFORCE_SAVE_ON_EXIT)) {
            pHost->mFORCE_SAVE_ON_EXIT = mFORCE_SAVE_ON_EXIT->isChecked();
        }
        if (dirty(mEnableGMCP)) {
            pHost->mEnableGMCP = mEnableGMCP->isChecked();
        }
        if (dirty(mEnableMSSP)) {
            pHost->mEnableMSSP = mEnableMSSP->isChecked();
        }
        if (dirty(mEnableMSDP)) {
            pHost->mEnableMSDP = mEnableMSDP->isChecked();
        }
        if (dirty(mEnableMSP)) {
            pHost->mEnableMSP = mEnableMSP->isChecked();
        }
        if (dirty(mEnableMXP)) {
            pHost->mEnableMXP = mEnableMXP->isChecked();
        }
        if (dirty(mEnableMTTS)) {
            pHost->mEnableMTTS = mEnableMTTS->isChecked();
        }
        if (dirty(mEnableMNES)) {
            pHost->mEnableMNES = mEnableMNES->isChecked();
        }
        if (dirty(mEnableNAWS)) {
            pHost->mEnableNAWS = mEnableNAWS->isChecked();
        }
        if (dirty(mEnableCHARSET)) {
            pHost->mEnableCHARSET = mEnableCHARSET->isChecked();
        }
        if (dirty(mEnableNEWENVIRON)) {
            pHost->mEnableNEWENVIRON = mEnableNEWENVIRON->isChecked();
        }
        if (dirty(mMapperUseAntiAlias)) {
            pHost->mMapperUseAntiAlias = mMapperUseAntiAlias->isChecked();
        }
        if (dirty(checkbox_mMapperShowRoomBorders)) {
            pHost->mMapperShowRoomBorders = checkbox_mMapperShowRoomBorders->isChecked();
        }
        if (dirty(checkBox_drawUpperLowerLevels)) {
            mudlet::self()->mDrawUpperLowerLevels = checkBox_drawUpperLowerLevels->isChecked();
        }
        if (pHost->mpMap) {
            if (dirty(checkBox_showDefaultArea)) {
                // Need to save the original value in case we change it in the line
                // following this one:
                const bool defaultAreaWasNotShown = pHost->mpMap->getDefaultAreaShown();
                pHost->mpMap->setDefaultAreaShown(checkBox_showDefaultArea->isChecked());
                if (pHost->mpMap->mpMapper && !defaultAreaWasNotShown && checkBox_showDefaultArea->isChecked() && pHost->mpMap->mpMapper->mp2dMap->mAreaID == -1) {
                    // Corner case fixup, user has asked for the default area
                    // to be shown and it wasn't - so it can now be:
                    pHost->mpMap->mpMapper->comboBox_showArea->setCurrentText(pHost->mpMap->getDefaultAreaName());
                }
            }
            if (dirty(mMapperUseAntiAlias) && pHost->mpMap->mpMapper) {
                pHost->mpMap->mpMapper->mp2dMap->mMapperUseAntiAlias = mMapperUseAntiAlias->isChecked();
            }

            // Only when the spin-box is what holds the newer value. It carries
            // no more precision than it displays, so writing it back whenever
            // Save is clicked would round off a factor a script had set:
            if (mpDoubleSpinBox_mapSymbolFontFudge && dirty(mpDoubleSpinBox_mapSymbolFontFudge) && !spinBoxShows(mpDoubleSpinBox_mapSymbolFontFudge, pHost->mpMap->getSymbolFontFudgeFactor())) {
                pHost->mpMap->setSymbolFontFudgeFactor(mpDoubleSpinBox_mapSymbolFontFudge->value());
            }

            if (pHost->mpMap->mpMapper) {
                pHost->mpMap->mpMapper->mp2dMap->repaint(); // Forceably redraw it as we ARE currently showing default area
                pHost->mpMap->mpMapper->update();
            }
        }
        if (anyDirty({leftBorderWidth, topBorderHeight, rightBorderWidth, bottomBorderHeight})) {
            const QMargins liveBorders = pHost->userBorders();
            const QMargins newBorders{dirty(leftBorderWidth) ? leftBorderWidth->value() : liveBorders.left(),
                                      dirty(topBorderHeight) ? topBorderHeight->value() : liveBorders.top(),
                                      dirty(rightBorderWidth) ? rightBorderWidth->value() : liveBorders.right(),
                                      dirty(bottomBorderHeight) ? bottomBorderHeight->value() : liveBorders.bottom()};
            pHost->setUserBorders(newBorders);
        }
        if (dirty(commandLineMinimumHeight)) {
            pHost->commandLineMinimumHeight = commandLineMinimumHeight->value();
        }
        if (dirty(checkBox_mVersionInTTYPE)) {
            pHost->mVersionInTTYPE = checkBox_mVersionInTTYPE->isChecked();
        }
        if (dirty(checkBox_mForceMXPProcessorOn)) {
            pHost->setForceMXPProcessorOn(checkBox_mForceMXPProcessorOn->isChecked());
        }
        if (dirty(mIsToLogInHtml)) {
            pHost->mIsNextLogFileInHtmlFormat = mIsToLogInHtml->isChecked();
        }
        if (dirty(mIsLoggingTimestamps)) {
            pHost->mIsLoggingTimestamps = mIsLoggingTimestamps->isChecked();
        }
        // The directory is picked with a button that has no value of its own,
        // showing up only as the text it puts in lineEdit_logFileFolder
        if (dirty(lineEdit_logFileFolder)) {
            pHost->mLogDir = mLogDirPath;
        }
        if (dirty(lineEdit_logFileName)) {
            pHost->mLogFileName = lineEdit_logFileName->text();
        }
        if (dirty(comboBox_logFileNameFormat)) {
            pHost->mLogFileNameFormat = comboBox_logFileNameFormat->currentData().toString();
        }
        if (dirty(checkBox_antiAlias)) {
            pHost->mNoAntiAlias = !checkBox_antiAlias->isChecked();
        }
        if (dirty(mAlertOnNewData)) {
            pHost->mAlertOnNewData = mAlertOnNewData->isChecked();
        }

        if (dirty(telnetHandlerEnabled)) {
            QSettings* settings = mudlet::getQSettings();
            if (settings->value("telnetHandlerEnabled", false).toBool() != telnetHandlerEnabled->isChecked()) {
                settings->setValue("telnetHandlerEnabled", telnetHandlerEnabled->isChecked());
            }
        }

        if (dirty(groupBox_proxy)) {
            pHost->mUseProxy = groupBox_proxy->isChecked();
        }
        if (dirty(lineEdit_proxyAddress)) {
            pHost->mProxyAddress = lineEdit_proxyAddress->text();
        }
        if (dirty(lineEdit_proxyPort)) {
            pHost->mProxyPort = lineEdit_proxyPort->text().toUInt();
        }
        if (dirty(lineEdit_proxyUsername)) {
            pHost->mProxyUsername = lineEdit_proxyUsername->text();
        }
        if (dirty(lineEdit_proxyPassword)) {
            pHost->mProxyPassword = lineEdit_proxyPassword->text();
        }

        //tab security
        if (dirty(groupBox_ssl)) {
            pHost->mSslTsl = groupBox_ssl->isChecked();
        }
        if (dirty(checkBox_expired)) {
            pHost->mSslIgnoreExpired = checkBox_expired->isChecked();
        }
        if (dirty(checkBox_self_signed)) {
            pHost->mSslIgnoreSelfSigned = checkBox_self_signed->isChecked();
        }
        if (dirty(checkBox_ignore_all)) {
            pHost->mSslIgnoreAll = checkBox_ignore_all->isChecked();
        }
        if (dirty(checkBox_askTlsAvailable)) {
            pHost->mAskTlsAvailable = checkBox_askTlsAvailable->isChecked();
        }

        if (console) {
            console->changeColors();
        }

        if (dirty(doubleclick_ignore_lineedit)) {
            const QString lIgnore = doubleclick_ignore_lineedit->text();
            pHost->mDoubleClickIgnore.clear();
            for (auto character : lIgnore) {
                pHost->mDoubleClickIgnore.insert(character);
            }
        }

        if (dirty(comboBox_mapFileSaveFormatVersion)) {
            pHost->mpMap->mSaveVersion = comboBox_mapFileSaveFormatVersion->currentData().toInt();
        }


        if (console) {
            const int x = console->width();
            const int y = console->height();
            const QSize s = QSize(x, y);
            QResizeEvent event(s, s);
            QApplication::sendEvent(console, &event);
        }

        if (dirty(checkBox_echoLuaErrors)) {
            pHost->mEchoLuaErrors = checkBox_echoLuaErrors->isChecked();
        }
        if (dirty(checkBox_useWideAmbiguousEastAsianGlyphs)) {
            pHost->setWideAmbiguousEAsianGlyphs(checkBox_useWideAmbiguousEastAsianGlyphs->checkState());
        }
        if (dirty(checkBox_enableBlinkText)) {
            pHost->setEnableBlinkText(checkBox_enableBlinkText->isChecked());
        }
        if (dirty(code_editor_theme_selection_combobox)) {
            if (mudlet::self()->inDarkMode()) {
                pHost->mEditorThemeDark = code_editor_theme_selection_combobox->currentText();
                pHost->mEditorThemeFileDark = code_editor_theme_selection_combobox->currentData().toString();
            } else {
                pHost->mEditorTheme = code_editor_theme_selection_combobox->currentText();
                pHost->mEditorThemeFile = code_editor_theme_selection_combobox->currentData().toString();
            }
        }
        if (dirty(checkBox_autocompleteLuaCode)) {
            pHost->mEditorAutoComplete = checkBox_autocompleteLuaCode->isChecked();
        }
        if (dirty(checkBox_showBidi)) {
            pHost->setEditorShowBidi(checkBox_showBidi->isChecked());
        }
        if (dirty(checkBox_showIdNumbers)) {
            pHost->setShowIdsInEditor(checkBox_showIdNumbers->isChecked());
        }
        // Re-theming an open script editor is a full edbee reconfiguration that
        // repaints every pattern field, so it waits for one of the settings it
        // carries to actually move:
        if (pHost->mpEditorDialog
            && anyDirty({code_editor_theme_selection_combobox, checkBox_showSpacesAndTabs, checkBox_showLineFeedsAndParagraphs, checkBox_autocompleteLuaCode, checkBox_showBidi})) {
            // The theme write above has already settled the user's choice into
            // the Host, so the name comes from there rather than from a combo
            // box that may be showing a theme a script has since replaced
            pHost->mpEditorDialog->setThemeAndOtherSettings(mudlet::self()->inDarkMode() ? pHost->mEditorThemeDark : pHost->mEditorTheme);
        }

        if (dirty(script_preview_combobox)) {
            auto data = script_preview_combobox->currentData().value<QPair<QString, int>>();
            pHost->mThemePreviewItemID = data.second;
            pHost->mThemePreviewType = data.first;
        }

        if (dirty(search_engine_combobox)) {
            pHost->mSearchEngineName = search_engine_combobox->currentText();
        }

        if (dirty(timeEdit_timerDebugOutputMinimumInterval)) {
            pHost->mTimerDebugOutputSuppressionInterval = timeEdit_timerDebugOutputMinimumInterval->time();
        }

        if (dirty(comboBox_blankLinesBehaviour)) {
            pHost->mBlankLineBehaviour = static_cast<Host::BlankLineBehaviour>(comboBox_blankLinesBehaviour->currentIndex());
        }

        if (anyDirty({comboBox_discordSmallIconPrivacy,
                      comboBox_discordLargeIconPrivacy,
                      checkBox_discordServerAccessToDetail,
                      checkBox_discordServerAccessToState,
                      checkBox_discordServerAccessToPartyInfo,
                      checkBox_discordServerAccessToTimerInfo})) {
            // Six controls, one flags word: start from what the Host holds and
            // move only the bits whose own control was edited, so the others
            // keep whatever a script has set them to since population
            Host::DiscordOptionFlags discordFlags = pHost->mDiscordAccessFlags;

            // A privacy combo box carries two bits: "show it" is its first two
            // entries, "show the text with it" only the first
            if (dirty(comboBox_discordLargeIconPrivacy)) {
                const int privacy = comboBox_discordLargeIconPrivacy->currentIndex();
                discordFlags.setFlag(Host::DiscordSetLargeIcon, privacy == 0 || privacy == 1);
                discordFlags.setFlag(Host::DiscordSetLargeIconText, privacy == 0);
            }
            if (dirty(comboBox_discordSmallIconPrivacy)) {
                const int privacy = comboBox_discordSmallIconPrivacy->currentIndex();
                discordFlags.setFlag(Host::DiscordSetSmallIcon, privacy == 0 || privacy == 1);
                discordFlags.setFlag(Host::DiscordSetSmallIconText, privacy == 0);
            }
            // These four are ticked to *withhold* the item from the server
            if (dirty(checkBox_discordServerAccessToDetail)) {
                discordFlags.setFlag(Host::DiscordSetDetail, !checkBox_discordServerAccessToDetail->isChecked());
            }
            if (dirty(checkBox_discordServerAccessToState)) {
                discordFlags.setFlag(Host::DiscordSetState, !checkBox_discordServerAccessToState->isChecked());
            }
            if (dirty(checkBox_discordServerAccessToPartyInfo)) {
                discordFlags.setFlag(Host::DiscordSetPartyInfo, !checkBox_discordServerAccessToPartyInfo->isChecked());
            }
            if (dirty(checkBox_discordServerAccessToTimerInfo)) {
                discordFlags.setFlag(Host::DiscordSetTimeInfo, !checkBox_discordServerAccessToTimerInfo->isChecked());
            }

            pHost->mDiscordAccessFlags = discordFlags;
        }

        if (anyDirty({radioButton_discordDisabled, radioButton_discordMudletOnly, radioButton_discordGameDetails})) {
            Host::DiscordMode newMode = Host::DiscordShowGameDetails;
            if (radioButton_discordDisabled->isChecked()) {
                newMode = Host::DiscordDisabled;
            } else if (radioButton_discordMudletOnly->isChecked()) {
                newMode = Host::DiscordShowMudletOnly;
            }
            pHost->setDiscordMode(newMode);
        }

        if (dirty(lineEdit_discordUserName)) {
            const QString newDiscordUserName = lineEdit_discordUserName->text().trimmed().toLower();
            if (pHost->mRequiredDiscordUserName != newDiscordUserName) {
                pHost->mRequiredDiscordUserName = newDiscordUserName;
                mudlet::self()->mDiscord.UpdatePresence();
            }
        }

        // Save chat options so they are written to XML upon export
        if (dirty(lineEdit_mmcpChatName)) {
            pHost->setMMCPChatName(lineEdit_mmcpChatName->text().trimmed());
        }
        if (dirty(lineEdit_mmcpChatMessagePrefix)) {
            pHost->mMMCPChatPrefix = lineEdit_mmcpChatMessagePrefix->text().trimmed();
        }
        if (dirty(lineEdit_mmcpPort)) {
            bool ok;
            const quint16 port = lineEdit_mmcpPort->text().toUShort(&ok);
            pHost->mMMCPChatPort = ok ? port : csDefaultMMCPHostPort;
        }

        /* Possible inclusion in 4.21
        pHost->mMMCPAutostartServer = checkBox_mmcpAutostartServer->isChecked();
        pHost->mMMCPAutoAcceptCalls = checkBox_mmcpAutoAcceptCalls->isChecked();
        pHost->mMMCPAllowPeekRequests = checkBox_mmcpAllowPeekReq->isChecked();
        */
        // remove these when the above is restored
        pHost->mMMCPAutostartServer = false;
        pHost->mMMCPAutoAcceptCalls = false;
        pHost->mMMCPAllowPeekRequests = false;

        if (dirty(checkBox_mmcpPrefixEmotes)) {
            pHost->mMMCPPrefixEmotes = checkBox_mmcpPrefixEmotes->isChecked();
        }
        if (dirty(checkBox_mmcpAddChatMessageNewline)) {
            pHost->mMMCPAddChatMessageNewline = checkBox_mmcpAddChatMessageNewline->isChecked();
        }
        if (dirty(checkBox_mmcpSnoopInMainConsole)) {
            pHost->mMMCPShowSnoopInMainConsole = checkBox_mmcpSnoopInMainConsole->isChecked();
        }
        if (dirty(checkBox_announceIncomingText)) {
            pHost->mAnnounceIncomingText = checkBox_announceIncomingText->isChecked();
        }
        if (dirty(checkBox_advertiseScreenReader)) {
            pHost->mAdvertiseScreenReader = checkBox_advertiseScreenReader->isChecked();
        }
        if (dirty(checkBox_enableOSC8Hyperlinks)) {
            pHost->mEnableOSC8Hyperlinks = checkBox_enableOSC8Hyperlinks->isChecked();
        }
        if (dirty(checkBox_enableClosedCaption)) {
            pHost->mEnableClosedCaption = checkBox_enableClosedCaption->isChecked();
        }

        if (dirty(checkBox_expectCSpaceIdInColonLessMColorCode)) {
            pHost->setHaveColorSpaceId(checkBox_expectCSpaceIdInColonLessMColorCode->isChecked());
        }
        if (dirty(checkBox_allowServerToRedefineColors)) {
            pHost->setMayRedefineColors(checkBox_allowServerToRedefineColors->isChecked());
        }
        if (dirty(checkBox_debugShowAllCodepointProblems)) {
            pHost->setDebugShowAllProblemCodepoints(checkBox_debugShowAllCodepointProblems->isChecked());
        }
        if (dirty(comboBox_caretModeKey)) {
            pHost->mCaretShortcut = static_cast<Host::CaretShortcut>(comboBox_caretModeKey->currentIndex());
        }
        if (groupBox_playerRoomStyle->isEnabled() && anyDirty({comboBox_playerRoomStyle, spinBox_playerRoomOuterDiameter, spinBox_playerRoomInnerDiameter})) {
            // Although the controls have been interactively modifying the
            // TMap cached values for these, they were not being committed to
            // the master values in the Host instance - but now we should write
            // those - while we can get the first three (quint8) values
            // directly from controls on this form/dialogue, the last two
            // (QColors) are easiest to retrieve from the TMap instance as the
            // colours are not directly stored here (as for some styles they
            // show a partly "grey-ed out" colour as they are disabled for those
            // styles). The three that do come from controls are taken one at a
            // time, so an untouched one contributes what the Host holds now
            // rather than what its control is showing:
            quint8 styleCode = 0;
            quint8 outerDiameter = 0;
            quint8 innerDiameter = 0;
            QColor liveOuterColor;
            QColor liveInnerColor;
            pHost->getPlayerRoomStyleDetails(styleCode, outerDiameter, innerDiameter, liveOuterColor, liveInnerColor);
            pHost->setPlayerRoomStyleDetails(dirty(comboBox_playerRoomStyle) ? static_cast<quint8>(comboBox_playerRoomStyle->currentIndex()) : styleCode,
                                             dirty(spinBox_playerRoomOuterDiameter) ? static_cast<quint8>(spinBox_playerRoomOuterDiameter->value()) : outerDiameter,
                                             dirty(spinBox_playerRoomInnerDiameter) ? static_cast<quint8>(spinBox_playerRoomInnerDiameter->value()) : innerDiameter,
                                             pHost->mpMap->mPlayerRoomOuterColor,
                                             pHost->mpMap->mPlayerRoomInnerColor);
        }

        if (currentShortcuts != mShortcutsSnapshot) {
            auto iterator = mudlet::self()->mpShortcutsManager->iterator();
            while (iterator.hasNext()) {
                auto key = iterator.next();
                // Per key for the same reason the value snapshot is per
                // control: the editors for the other shortcuts are showing what
                // this dialog was populated with, which may no longer be what
                // the profile holds
                if (currentShortcuts.value(key) == mShortcutsSnapshot.value(key)) {
                    continue;
                }
                QKeySequence sequence = currentShortcuts.value(key);
                auto it = pHost->profileShortcuts.find(key);
                if (it != pHost->profileShortcuts.end()) {
                    it->second->swap(sequence);
                }
            }
        }
    }

#if defined(INCLUDE_UPDATER)
    if (dirty(checkbox_noAutomaticUpdates) && (mudlet::self()->releaseVersion || mudlet::self()->publicTestVersion || qEnvironmentVariableIsSet("DEV_UPDATER"))) {
        pMudlet->pUpdater->setAutomaticUpdates(!checkbox_noAutomaticUpdates->isChecked());
    }
#endif

    if (dirty(MainIconSize)) {
        pMudlet->setToolBarIconSize(MainIconSize->value());
    }
    if (dirty(TEFolderIconSize)) {
        pMudlet->setEditorTreeWidgetIconSize(TEFolderIconSize->value());
    }
    if (dirty(comboBox_menuBarVisibility)) {
        switch (comboBox_menuBarVisibility->currentIndex()) {
        case 0:
            pMudlet->setMenuBarVisibility(enums::visibleNever);
            break;
        case 1:
            pMudlet->setMenuBarVisibility(enums::visibleOnlyWithoutLoadedProfile);
            break;
        default:
            pMudlet->setMenuBarVisibility(enums::visibleAlways);
        }
    }
    if (dirty(comboBox_toolBarVisibility)) {
        switch (comboBox_toolBarVisibility->currentIndex()) {
        case 0:
            pMudlet->setToolBarVisibility(enums::visibleNever);
            break;
        case 1:
            pMudlet->setToolBarVisibility(enums::visibleOnlyWithoutLoadedProfile);
            break;
        default:
            pMudlet->setToolBarVisibility(enums::visibleAlways);
        }
    }

    if (anyDirty({checkBox_showSpacesAndTabs, checkBox_showLineFeedsAndParagraphs})) {
        const QTextOption::Flags liveOptions = pMudlet->mEditorTextOptions;
        pMudlet->setEditorTextoptions(dirty(checkBox_showSpacesAndTabs) ? checkBox_showSpacesAndTabs->isChecked() : liveOptions.testFlag(QTextOption::ShowTabsAndSpaces),
                                      dirty(checkBox_showLineFeedsAndParagraphs) ? checkBox_showLineFeedsAndParagraphs->isChecked()
                                                                                 : liveOptions.testFlag(QTextOption::ShowLineAndParagraphSeparators));
    }
    if (dirty(checkBox_reportMapIssuesOnScreen)) {
        pMudlet->setShowMapAuditErrors(checkBox_reportMapIssuesOnScreen->isChecked());
    }
    if (dirty(checkBox_showIconsOnMenus)) {
        pMudlet->setShowIconsOnMenu(checkBox_showIconsOnMenus->checkState());
    }
    if (dirty(comboBox_appearance)) {
        pMudlet->setAppearance(static_cast<enums::Appearance>(comboBox_appearance->currentIndex()));
    }

    mudlet::self()->mDiscord.UpdatePresence();

    emit signal_preferencesSaved();

    // What the controls hold now is what the settings say, so only what changes
    // after this point is the user's next edit:
    snapshotValues();
}

void dlgProfilePreferences::slot_scheduleApply()
{
    // A control wiring itself up inside buildShell() can arrive here before the
    // constructor has made the timer:
    if (mPopulating || !mpTimer_apply) {
        return;
    }
    // Restarting rather than starting: a burst of changes - dragging a spin
    // box, or a combo box rebuilding itself - costs one apply, not one each
    mpTimer_apply->start();
}

void dlgProfilePreferences::slot_lineEditFinished()
{
    // Clearing the modified flag is what marks this edit finished with - see
    // beingTypedInto():
    if (auto* pLineEdit = qobject_cast<QLineEdit*>(sender()); pLineEdit) {
        pLineEdit->setModified(false);
    }
    slot_scheduleApply();
}

// Instant apply has already written every change; closeEvent() does the rest
void dlgProfilePreferences::slot_saveAndClose()
{
    close();
}

void dlgProfilePreferences::slot_chosenProfilesChanged(QAction* _action)
{
    Q_UNUSED(_action)

    QMenu* _menu = pushButton_chooseProfiles->menu();
    QListIterator<QAction*> itAction(_menu->actions());
    unsigned int selectionCount = 0;
    while (itAction.hasNext()) {
        QAction* _currentAction = itAction.next();
        if (_currentAction->isChecked()) {
            ++selectionCount;
        }
    }
    /*:
    text on button to put the map from this profile into the other profiles to
    receive the map from this profile, %n is the number of other profiles that
    have already been selected to receive it and will be zero or more. The button
    will also be disabled (greyed out) in the zero case but the text will still be
    visible.
    */
    pushButton_copyMap->setText(tr("copy to %n destination(s)", nullptr, selectionCount));
    if (selectionCount) {
        pushButton_copyMap->setEnabled(true);
        /*: text on button to select other profiles to receive the map from this profile,
 %n is the number of other profiles that have already been selected to receive it and will always be 1 or more*/
        pushButton_chooseProfiles->setText(tr("%n selected - change destinations...", nullptr, selectionCount));
    } else {
        pushButton_copyMap->setEnabled(false);
        /*: text on button to select other profiles to receive the map from this profile,
 this is used when no profiles have been selected*/
        pushButton_chooseProfiles->setText(tr("pick destinations..."));
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

    std::list<TTrigger*> const triggers = pHost->getTriggerUnit()->getTriggerRootNodeList();
    for (auto trigger : triggers) {
        if (!trigger->getScript().isEmpty() && !trigger->isTemporary()) {
            items.push_back({trigger->getName(), qsl("trigger"), trigger->getID()});
        }
        addTriggersToPreview(trigger, items);
    }

    std::list<TAlias*> const aliases = pHost->getAliasUnit()->getAliasRootNodeList();
    for (auto alias : aliases) {
        if (!alias->getScript().isEmpty() && !alias->isTemporary()) {
            items.push_back({alias->getName(), qsl("alias"), alias->getID()});
        }
        addAliasesToPreview(alias, items);
    }

    std::list<TScript*> const scripts = pHost->getScriptUnit()->getScriptRootNodeList();
    for (auto script : scripts) {
        if (!script->getScript().isEmpty()) {
            items.push_back({script->getName(), qsl("script"), script->getID()});
        }
        addScriptsToPreview(script, items);
    }

    std::list<TTimer*> const timers = pHost->getTimerUnit()->getTimerRootNodeList();
    for (auto timer : timers) {
        if (!timer->getScript().isEmpty() && !timer->isTemporary()) {
            items.push_back({timer->getName(), qsl("timer"), timer->getID()});
        }
        addTimersToPreview(timer, items);
    }

    std::list<TKey*> const keys = pHost->getKeyUnit()->getKeyRootNodeList();
    for (auto key : keys) {
        if (!key->getScript().isEmpty() && !key->isTemporary()) {
            items.push_back({key->getName(), qsl("key"), key->getID()});
        }
        addKeysToPreview(key, items);
    }

    std::list<TAction*> const actions = pHost->getActionUnit()->getActionRootNodeList();
    for (auto action : actions) {
        if (!action->getScript().isEmpty()) {
            items.push_back({action->getName(), qsl("button"), action->getID()});
        }
        addActionsToPreview(action, items);
    }

    auto combobox = script_preview_combobox;
    combobox->setUpdatesEnabled(false);
    combobox->clear();

    for (const auto& [name, type, id] : items) {
        combobox->addItem(qsl("%1 (%2)").arg(name, type),
                          // store the item type and ID in data so we can pull up the script for it later
                          QVariant::fromValue(QPair<QString, int>(type, id)));
    }
    combobox->setUpdatesEnabled(true);
}

// adds trigger name ID to the list of them for the theme preview combobox, recursing down all of them
void dlgProfilePreferences::addTriggersToPreview(TTrigger* pTriggerParent, std::vector<std::tuple<QString, QString, int>>& items)
{
    std::list<TTrigger*>* childTriggers = pTriggerParent->getChildrenList();
    for (auto trigger : *childTriggers) {
        if (!trigger->getScript().isEmpty()) {
            items.push_back({trigger->getName(), qsl("trigger"), trigger->getID()});
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
            items.push_back({alias->getName(), qsl("alias"), alias->getID()});
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
            items.push_back({timer->getName(), qsl("timer"), timer->getID()});
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
            items.push_back({key->getName(), qsl("key"), key->getID()});
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
            items.push_back({script->getName(), qsl("script"), script->getID()});
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
            items.push_back({action->getName(), qsl("button"), action->getID()});
        }

        if (action->hasChildren()) {
            addActionsToPreview(action, items);
        }
    }
}

// Refreshes the edbee themes from colorsublime. A network round trip, so it is
// made on the first visit to the Editor category only.
void dlgProfilePreferences::maybeDownloadEditorThemes()
{
    Host* pHost = mpHost;
    if (!pHost) {
        // Nothing to refresh against yet, so let a later visit try again:
        mEditorThemesChecked = false;
        return;
    }

    const QDir dir;
    const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (!dir.mkpath(cacheDir)) {
        qWarning() << "Couldn't create cache directory for edbee themes: " << cacheDir;
        return;
    }

    QSettings& settings = *mudlet::getQSettings();
    const QString themesURL = settings.value("colorSublimeThemesURL", qsl("https://github.com/Colorsublime/Colorsublime-Themes/archive/master.zip")).toString();
    // a default update period is 24h
    // it would be nice to use C++14's numeric separator but Qt Creator still
    // does not like them for its Clang code model analyser (and the built in
    // one is even less receptive to): 86'400'000
    const int themesUpdatePeriod = settings.value("themesUpdatePeriod", 86400000).toInt();
    // save the defaults in settings so the field is visible for editing in config file if needed
    settings.setValue("colorSublimeThemesURL", themesURL);
    settings.setValue("themesUpdatePeriod", themesUpdatePeriod);

    auto themesAge = QFileInfo(mudlet::getMudletPath(enums::editorWidgetThemeJsonFile)).lastModified().toUTC();

    // A functional test that visits the Editor category would otherwise be one
    // file modification time away from a live fetch of github.com, which fails
    // slowly and intermittently rather than red. Set, this takes the same route
    // a themes file that is still fresh does.
    const bool downloadSuppressed = qEnvironmentVariableIsSet("MUDLET_TEST_NO_THEME_DOWNLOAD");
    // if the cache file exists and is younger than the specified age (24h by default), don't refresh it
    if (downloadSuppressed || (themesAge.isValid() && themesAge.msecsTo(QDateTime::currentDateTimeUtc()) / (themesUpdatePeriod) < 1)) {
        populateThemesList();
        return;
    }

    theme_download_label->show();

    auto manager = new QNetworkAccessManager(this);
    auto diskCache = new QNetworkDiskCache(this);
    diskCache->setCacheDirectory(cacheDir);
    manager->setCache(diskCache);


    const QUrl url(themesURL);
    QNetworkRequest request(url);
    request.setRawHeader(QByteArray("User-Agent"), QByteArray(qsl("Mozilla/5.0 (Mudlet/%1%2)").arg(APP_VERSION, mudlet::self()->mAppBuild).toUtf8().constData()));
    // github uses redirects
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    // load from cache if possible
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
    pHost->updateProxySettings(manager);
    QNetworkReply* getReply = manager->get(request);

    connect(getReply, &QNetworkReply::errorOccurred, this, [=, this](QNetworkReply::NetworkError) {
        theme_download_label->setText(tr("Could not update themes: %1").arg(getReply->errorString()));
        QTimer::singleShot(5s, theme_download_label, [label = theme_download_label] {
            label->hide();
            label->setText(tr("Updating themes from colorsublime.github.io..."));
        });
        getReply->deleteLater();
    });

    connect(getReply,
            &QNetworkReply::finished,
            this,
            std::bind(
                    [=, this](QNetworkReply* reply) {
                        // don't do anything if there was an error
                        if (reply->error() != QNetworkReply::NoError) {
                            return;
                        }

                        const QByteArray downloadedArchive = reply->readAll();

                        tempThemesArchive = new QTemporaryFile(this);
                        if (!tempThemesArchive->open()) {
                            return;
                        }
                        tempThemesArchive->write(downloadedArchive);
                        tempThemesArchive->close();

                        const QTemporaryDir temporaryDir;
                        if (!temporaryDir.isValid()) {
                            return;
                        }

                        // perform unzipping in a worker thread so as not to freeze the UI
                        auto future = QtConcurrent::run(mudlet::unzip, tempThemesArchive->fileName(), mudlet::getMudletPath(enums::mainDataItemPath, qsl("edbee/")), temporaryDir.path());
                        auto watcher = new QFutureWatcher<bool>(this);
                        connect(watcher, &QFutureWatcher<bool>::finished, this, [=, this]() {
                            if (future.result()) {
                                populateThemesList();

                                emit signal_themeUpdateCompleted();
                            }

                            theme_download_label->hide();
                            tempThemesArchive->deleteLater();
                            watcher->deleteLater();
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
    QFile themesFile(mudlet::getMudletPath(enums::editorWidgetThemeJsonFile));
    QList<std::pair<QString, QString>> sortedThemes;
    QJsonArray unsortedThemes;

    if (themesFile.open(QIODevice::ReadOnly)) {
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(themesFile.readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qWarning() << "dlgProfilePreferences::populateThemesList() ERROR - failed to parse themes JSON file:" << themesFile.fileName() << "reason:" << parseError.errorString();
        } else if (!doc.isArray()) {
            qWarning() << "dlgProfilePreferences::populateThemesList() ERROR - themes JSON file does not contain an array:" << themesFile.fileName();
        } else {
            unsortedThemes = doc.array();
            for (auto theme : std::as_const(unsortedThemes)) {
                const QString themeText = theme.toObject()["Title"].toString();
                const QString themeFileName = theme.toObject()["FileName"].toString();

                if (!themeText.isEmpty() && !themeFileName.isEmpty()) {
                    sortedThemes << std::make_pair(themeText, themeFileName);
                }
            }
        }
    }
    sortedThemes << std::make_pair(qsl("Mudlet"), qsl("Mudlet.tmTheme"));

    std::sort(sortedThemes.begin(), sortedThemes.end(), [](const auto& a, const auto& b) {
        return QString::localeAwareCompare(a.first, b.first) < 0;
    });

    // temporary disable painting and event updates while we refill the list
    code_editor_theme_selection_combobox->setUpdatesEnabled(false);
    code_editor_theme_selection_combobox->blockSignals(true);

    auto currentSelection = code_editor_theme_selection_combobox->currentText();
    code_editor_theme_selection_combobox->clear();
    for (const auto& key : std::as_const(sortedThemes)) {
        // store the actual theme file as data because edbee needs that,
        // not the name, for choosing the theme even after the theme file was loaded
        code_editor_theme_selection_combobox->addItem(key.first, key.second);
    }

    code_editor_theme_selection_combobox->setCurrentIndex(code_editor_theme_selection_combobox->findText(currentSelection));
    code_editor_theme_selection_combobox->setUpdatesEnabled(true);
    code_editor_theme_selection_combobox->blockSignals(false);
    // The list is rebuilt long after the dialog was populated - on the first
    // visit to the Editor page, and again when the colorsublime download lands.
    // That is the list changing under the dialog rather than the user picking
    // anything, so it counts as a fresh population: a refreshed list that no
    // longer offers the profile's theme leaves the box on no item at all, and
    // writing that back would wipe the theme the profile had.
    mValueSnapshot.insert(code_editor_theme_selection_combobox, controlValue(code_editor_theme_selection_combobox));
}

// Given a theme name, try to find its dark or light counterpart in the combobox.
// Handles naming patterns like "Solarized light" <-> "Solarized dark",
// "Kimbie (light)" <-> "Kimbie (dark)", "Kary Foundation - Light" <-> "Kary Foundation - Dark".
// Returns the counterpart theme name if found, or empty string if not.
QString dlgProfilePreferences::findThemeCounterpart(const QString& themeName, const QComboBox* themeComboBox, bool toDark)
{
    const QString from = toDark ? qsl("light") : qsl("dark");
    const QString to = toDark ? qsl("dark") : qsl("light");

    // Try case-insensitive replacement of "light" with "dark" (or vice versa)
    const QRegularExpression re(QRegularExpression::escape(from), QRegularExpression::CaseInsensitiveOption);
    auto match = re.match(themeName);
    if (match.hasMatch()) {
        QString candidate = themeName;
        // Preserve the case style of the original: if the matched text starts uppercase, capitalize the replacement
        const QString matched = match.captured(0);
        const QString replacement = matched[0].isUpper() ? (to[0].toUpper() + to.mid(1)) : to;
        candidate.replace(match.capturedStart(), match.capturedLength(), replacement);

        if (themeComboBox->findText(candidate) != -1) {
            return candidate;
        }
    }

    return {};
}

// Switches the editor theme combobox to the given theme name, triggering the
// preview update. If themes haven't loaded yet, defers until they're available.
void dlgProfilePreferences::switchEditorTheme(const QString& themeName)
{
    auto index = code_editor_theme_selection_combobox->findText(themeName);
    if (index != -1) {
        code_editor_theme_selection_combobox->setCurrentIndex(index);
        return;
    }

    // theme may not be in the list yet (still downloading), so switch once the download completes
    KDToolBox::connectSingleShot(this, &dlgProfilePreferences::signal_themeUpdateCompleted, this, [=, this]() {
        auto deferredIndex = code_editor_theme_selection_combobox->findText(themeName);
        if (deferredIndex != -1) {
            code_editor_theme_selection_combobox->setCurrentIndex(deferredIndex);
        } else {
            qWarning() << "dlgProfilePreferences::switchEditorTheme() - theme" << themeName << "not found after theme update completed";
        }
    });
}

// user has picked a different theme to preview, so apply it
void dlgProfilePreferences::slot_themeSelected(int index)
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
void dlgProfilePreferences::slot_scriptSelected(int index)
{
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }

    auto data = script_preview_combobox->itemData(index).value<QPair<QString, int>>();
    auto itemType = data.first;
    auto itemId = data.second;

    auto preview = edbeePreviewWidget->textDocument();
    if (itemType == qsl("trigger")) {
        auto pT = pHost->getTriggerUnit()->getTrigger(itemId);
        preview->setText(pT ? pT->getScript() : tr("{missing, possibly recently deleted trigger item}"));
    } else if (itemType == qsl("alias")) {
        auto pT = pHost->getAliasUnit()->getAlias(itemId);
        preview->setText(pT ? pT->getScript() : tr("{missing, possibly recently deleted alias item}"));
    } else if (itemType == qsl("script")) {
        auto pT = pHost->getScriptUnit()->getScript(itemId);
        preview->setText(pT ? pT->getScript() : tr("{missing, possibly recently deleted script item}"));
    } else if (itemType == qsl("timer")) {
        auto pT = pHost->getTimerUnit()->getTimer(itemId);
        preview->setText(pT ? pT->getScript() : tr("{missing, possibly recently deleted timer item}"));
    } else if (itemType == qsl("key")) {
        auto pT = pHost->getKeyUnit()->getKey(itemId);
        preview->setText(pT ? pT->getScript() : tr("{missing, possibly recently deleted key item}"));
    } else if (itemType == qsl("button")) {
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
    config->setShowWhitespaceMode(state ? edbee::TextEditorConfig::ShowWhitespaces : edbee::TextEditorConfig::HideWhitespaces);
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

        // The profile brings controls of its own with it:
        invalidateSearch();

        mpHost = pHost;
        // So make connections to the details of the real Host instance:
        mPopulating = true;
        initWithHost(pHost);
        mPopulating = false;
        // ...and pick up the controls that only exist once there is a profile
        // (the shortcut editors, the map symbol scaling spin-box):
        connectApplyTriggers();
        snapshotValues();
        guardScrollWheel();
        updateColumnWidthCaps();
        rebuildTabOrder();
        // ...including the palette fix-ups, which the constructor only applied
        // to the controls that existed then:
        applyShellStyle();
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

        // Before the profile's controls are cleared and greyed out:
        invalidateSearch();

        // Forget about the host:
        mpHost = nullptr;
        // Remove connections to the details of the real Host instance (we
        // have to throw them away as it is too late to save them - the profile
        // has already been saved - or not):
        disconnectHostRelatedControls();
        mPopulating = true;
        clearHostDetails();
        // and we can then use the following to disable the Host specific controls:
        disableHostDetails();
        mPopulating = false;
        // The wildcard disconnects above take every connection off the signals
        // they name, instant apply's among them, so those are made again -
        // Qt::UniqueConnection is what makes that safe to do:
        connectApplyTriggers();
        // Nothing of what the controls now hold is worth writing back:
        mpTimer_apply->stop();
        snapshotValues();

        // And redraw the color controls in their cleared state
        setColors();
        setColors2();

        setButtonColor(pushButton_playerRoomPrimaryColor, QColor(), true);
        setButtonColor(pushButton_playerRoomSecondaryColor, QColor(), true);
    }
}

void dlgProfilePreferences::generateMapGlyphDisplay()
{
    QHash<QString, QSet<int>> const roomSymbolsHash(mpHost->mpMap->roomSymbolsHash());
    QPointer<QTableWidget> const pTableWidget = mpDialogMapGlyphUsage->findChild<QTableWidget*>(QLatin1String("tableWidget"));
    if (!pTableWidget) {
        return;
    }
    mGlyphDisplayFont = mpHost->mpMap->getSymbolFont();

    // Must turn off sorting at least while inserting items...
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
        const QString symbol = itUsedSymbol.key();
        QList<int> roomsWithSymbol = itUsedSymbol.value().values();
        if (roomsWithSymbol.count() > 1) {
            std::sort(roomsWithSymbol.begin(), roomsWithSymbol.end());
        }
        auto* pSymbolInFont = new QTableWidgetItem();
        pSymbolInFont->setTextAlignment(Qt::AlignCenter);
        pSymbolInFont->setToolTip(utils::richText(tr("The room symbol will appear like this if only symbols (glyphs) from the specific font are used.")));
        pSymbolInFont->setFont(selectedFont);

        auto* pSymbolAnyFont = new QTableWidgetItem();
        pSymbolAnyFont->setTextAlignment(Qt::AlignCenter);
        pSymbolAnyFont->setToolTip(utils::richText(tr("The room symbol will appear like this if symbols (glyphs) from any font can be used.")));
        pSymbolAnyFont->setFont(anyFont);

        const QFontMetrics SymbolInFontMetrics(selectedFont);
        const QFontMetrics SymbolAnyFontMetrics(anyFont);

        // pCodePoints is the sequence of UTF-32 codepoints in the symbol and
        // this ought to be what is needed to check that a font or set of fonts
        // can render the codepoints:
        const QVector<quint32> pCodePoints = symbol.toUcs4();
        // These can be used to flag symbols that cannot be reproduced
        bool isSingleFontUsable = true;
        bool isAllFontUsable = true;
        QStringList codePointsString;
        for (uint i = 0, total = pCodePoints.size(); i < total; ++i) {
            codePointsString << qsl("U+%1").arg(pCodePoints.at(i), 4, 16, QChar('0')).toUpper();
            if (!SymbolAnyFontMetrics.inFontUcs4(pCodePoints.at(i))) {
                isAllFontUsable = false;
                // By definition if all the fonts together cannot render the
                // glyph then the specified one cannot either
                isSingleFontUsable = false;
            } else if (!SymbolInFontMetrics.inFontUcs4(pCodePoints.at(i))) {
                isSingleFontUsable = false;
            }
        }

        QTableWidgetItem* pCodePointDisplay = new QTableWidgetItem(codePointsString.join(qsl(", ")));
        pCodePointDisplay->setTextAlignment(Qt::AlignCenter);
        pCodePointDisplay->setToolTip(tr("<p>These are the sequence of hexadecimal numbers that are used by the Unicode consortium "
                                         "to identify the graphemes needed to create the symbol.  These numbers can be utilised "
                                         "to determine precisely what is to be drawn even if some fonts have glyphs that are the "
                                         "same for different codepoints or combination of codepoints.</p>"
                                         "<p>Character entry utilities such as <i>charmap.exe</i> on <i>Windows</i> or <i>gucharmap</i> "
                                         "on many Unix type operating systems will also use these numbers which cover "
                                         "everything from U+0020 {Space} to U+10FFFD the last usable number in the <i>Private Use "
                                         "Plane 16</i> via most of the written marks that humanity has ever made.</p>"));

        // Need to pad the numbers with spaces so that sorting works correctly:
        QTableWidgetItem* pUsageCount = new QTableWidgetItem(qsl("%1").arg(roomsWithSymbol.count(), 5, 10, QChar(' ')));
        pUsageCount->setTextAlignment(Qt::AlignCenter);
        pUsageCount->setToolTip(utils::richText(tr("How many rooms in the whole map have this symbol.")));

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
        QTableWidgetItem* pRoomNumbers = new QTableWidgetItem(roomNumberStringList.join(qsl(", ")));
        pRoomNumbers->setToolTip(utils::richText(tr("The rooms with this symbol, up to a maximum of thirty-two, if there are more "
                                                    "than this, it is indicated but they are not shown.")));

        auto* pDummyButton = new QToolButton();
        if (isSingleFontUsable) {
            pSymbolInFont->setText(symbol);
            pSymbolAnyFont->setText(symbol);
            pDummyButton->setIcon(QIcon(qsl(":/icons/dialog-ok-apply.png")));
            pDummyButton->setToolTip(utils::richText(tr("The symbol can be made entirely from glyphs in the specified font.")));
        } else {
            // Need to switch to a different font as it is possible that the
            // single font may not have the replacement glyph either...!
            pSymbolInFont->setFont(anyFont);
            pSymbolInFont->setText(QString(QChar::ReplacementCharacter));
            if (isAllFontUsable) {
                pSymbolAnyFont->setText(symbol);
                pDummyButton->setIcon(QIcon(qsl(":/icons/dialog-warning.png")));
                pDummyButton->setToolTip(tr("<p>The symbol cannot be made entirely from glyphs in the specified font, but, "
                                            "using other fonts in the system, it can. Either un-check the <i>Only use symbols "
                                            "(glyphs) from chosen font</i> option or try and choose another font that does "
                                            "have the needed glyphs.</p>"
                                            "<p><i>You need not close this table to try another font, changing it on the main "
                                            "preferences dialogue will update this table after a slight delay.</i></p>"));
            } else {
                pSymbolAnyFont->setText(QString(QChar::ReplacementCharacter));
                pDummyButton->setIcon(QIcon(qsl(":/icons/dialog-error.png")));
                pDummyButton->setToolTip(utils::richText(tr("The symbol cannot be drawn using any of the fonts in the system, either an "
                                                            "invalid string was entered as the symbol for the indicated rooms or the map was "
                                                            "created on a different systems with a different set of fonts available to use. "
                                                            "You may be able to correct this by installing an additional font using whatever "
                                                            "method is appropriate for this system or by editing the map to use a different "
                                                            "symbol. It may be possible to do the latter via a lua script using the "
                                                            "<i>getRoomChar</i> and <i>setRoomChar</i> functions.")));
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

// The setToolTip() calls at the end of this method use the lambda method
// defined within this method and NOT the Qt method with the same name but
// different signature:
void dlgProfilePreferences::generateDiscordTooltips()
{
    if (!mpHost) {
        return;
    }

    auto* mudlet = mudlet::self();

    auto detail = mudlet->mDiscord.getDetailText(mpHost);
    if (!detail.isEmpty()) {
        detail = qsl("<br/>(\"%1\")").arg(detail);
    }

    auto state = mudlet->mDiscord.getStateText(mpHost);
    if (!state.isEmpty()) {
        state = qsl("<br/>(\"%1\")").arg(state);
    }

    auto setToolTip = [=](QWidget* widget, const QString& highlight) {
        const QString tooltip = qsl(R"(
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
                                             //: Discord Rich Presence large icon
                                             tr("Large icon"),
                                             //: Discord Rich Presence detail
                                             tr("Detail"),
                                             detail,
                                             //: Discord Rich Presence small icon"
                                             tr("Small icon"),
                                             //: Discord Rich Presence state
                                             tr("State"),
                                             state,
                                             //: Discord Rich Presence party size
                                             tr("Party size"),
                                             //: Discord Rich Presence maximum party size
                                             tr("Party max"))
                                        //: Discord Rich Presence time until or time elapsed
                                        .arg(tr("Time"));
        widget->setToolTip(tooltip);
    };

    setToolTip(checkBox_discordServerAccessToDetail, qsl("detail"));
    setToolTip(checkBox_discordServerAccessToState, qsl("state"));
    setToolTip(checkBox_discordServerAccessToPartyInfo, qsl("party"));
    setToolTip(checkBox_discordServerAccessToTimerInfo, qsl("time"));
    setToolTip(comboBox_discordLargeIconPrivacy, qsl("large-icon"));
    setToolTip(comboBox_discordSmallIconPrivacy, qsl("small-icon"));
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
    QFile file(qsl(":/ui/glyph_usage.ui"));
    if (!file.open(QFile::ReadOnly)) {
        qWarning() << "dlgProfilePreferences: failed to open UI file for reading:" << file.errorString();
        return;
    }
    mpDialogMapGlyphUsage = qobject_cast<QDialog*>(loader.load(&file, this));
    file.close();
    if (!mpDialogMapGlyphUsage) {
        qWarning() << "dlgProfilePreferences::slot_showMapGlyphUsage() ERROR: failed to create the dialog!";
        return;
    }

    mpDialogMapGlyphUsage->setWindowIcon(QIcon(qsl(":/icons/place_of_interest.png")));
    mpDialogMapGlyphUsage->setWindowTitle(tr("Map symbol usage - %1").arg(mpHost->getName()));
    mpDialogMapGlyphUsage->setAttribute(Qt::WA_DeleteOnClose);
    generateMapGlyphDisplay();
}

void dlgProfilePreferences::slot_setMapSymbolFontStrategy(const bool isToOnlyUseSelectedFont)
{
    Host* pHost = mpHost;
    if (!pHost || !pHost->mpMap) {
        return;
    }

    pHost->mpMap->setOnlySymbolFontUsed(isToOnlyUseSelectedFont);
}

void dlgProfilePreferences::slot_setMapSymbolFont(const QFont& font)
{
    Host* pHost = mpHost;
    if (!pHost || !pHost->mpMap) {
        return;
    }

    pHost->mpMap->setSymbolFont(font);
}

// Keeps the controls (and the glyph usage dialog, when open) in step with the
// map's symbol settings however they were changed - including from Lua:
void dlgProfilePreferences::slot_mapSymbolFontChanged()
{
    Host* pHost = mpHost;
    if (!pHost || !pHost->mpMap) {
        return;
    }

    // Only a control that is actually out of step gets written. Blocking the
    // signals stops the recursion but not the side effects of setting a control
    // to what it already holds: a spin-box also rewrites its line edit, which
    // between two keystrokes of a factor being typed in costs the second one.
    const QFont symbolFont = pHost->mpMap->getSymbolFont();
    if (fontComboBox_mapSymbols->currentFont().family() != symbolFont.family()) {
        const QSignalBlocker blocker(fontComboBox_mapSymbols);
        fontComboBox_mapSymbols->setCurrentFont(symbolFont);
    }

    const bool onlyUseSelectedFont = pHost->mpMap->getOnlySymbolFontUsed();
    if (checkBox_isOnlyMapSymbolFontToBeUsed->isChecked() != onlyUseSelectedFont) {
        const QSignalBlocker blocker(checkBox_isOnlyMapSymbolFontToBeUsed);
        checkBox_isOnlyMapSymbolFontToBeUsed->setChecked(onlyUseSelectedFont);
    }

    const qreal fudgeFactor = pHost->mpMap->getSymbolFontFudgeFactor();
    if (mpDoubleSpinBox_mapSymbolFontFudge && !spinBoxShows(mpDoubleSpinBox_mapSymbolFontFudge, fudgeFactor)) {
        const QSignalBlocker blocker(mpDoubleSpinBox_mapSymbolFontFudge);
        mpDoubleSpinBox_mapSymbolFontFudge->setValue(fudgeFactor);
    }

    // Rebuilding the glyph usage table walks every room in the map, so it is
    // only done when what it shows has actually changed. That is the font
    // alone - which of its glyphs the symbols need, and (through the style
    // strategy) whether other fonts may fill in - and never the scaling
    // factor, whose spin-box would otherwise mean one whole-map scan per
    // auto-repeat tick of its arrows.
    if (mpDialogMapGlyphUsage && symbolFont != mGlyphDisplayFont) {
        generateMapGlyphDisplay();
    }
}

// These next two prevent BOTH controls being set to never to prevent the lose
// of access to the setting/controls completely - once there is a profile loaded
// access to the settings/controls can be overridden by a context menu action on
// any TConsole instance:
//
// Additionally the "Never" entry in the other toolbar-visibility comboBox is
// greyed out (deactivated) while this control is set to "Never", so the user
// cannot even temporarily select "Never" for both - see issue #7079.
void dlgProfilePreferences::slot_changeShowMenuBar(int newIndex)
{
    if (!newIndex && !comboBox_toolBarVisibility->currentIndex()) {
        // This control has been set to the "Never" setting but so is the other
        // control - so force it back to the "Only if no profile one
        comboBox_menuBarVisibility->setCurrentIndex(1);
    }
    slot_syncMenuToolBarNeverItem();
}

void dlgProfilePreferences::slot_changeShowToolBar(int newIndex)
{
    if (!newIndex && !comboBox_menuBarVisibility->currentIndex()) {
        // This control has been set to the "Never" setting but so is the other
        // control - so force it back to the "Only if no profile one
        comboBox_toolBarVisibility->setCurrentIndex(1);
    }
    slot_syncMenuToolBarNeverItem();
}

// Deactivate (grey out) the "Never" item of comboBox_toolBarVisibility when
// the menu bar is set to "Never", and vice versa. This makes the existing
// mutual-exclusion visible to the user instead of silently snapping the
// selected value back (issue #7079).
void dlgProfilePreferences::slot_syncMenuToolBarNeverItem()
{
    const int menuIndex = comboBox_menuBarVisibility->currentIndex();
    const int toolIndex = comboBox_toolBarVisibility->currentIndex();
    const bool menuIsNever = (menuIndex == 0);
    const bool toolIsNever = (toolIndex == 0);

    if (auto* toolModel = qobject_cast<QStandardItemModel*>(comboBox_toolBarVisibility->model())) {
        if (QStandardItem* item = toolModel->item(0)) {
            item->setEnabled(!menuIsNever);
        }
    }
    if (auto* menuModel = qobject_cast<QStandardItemModel*>(comboBox_menuBarVisibility->model())) {
        if (QStandardItem* item = menuModel->item(0)) {
            item->setEnabled(!toolIsNever);
        }
    }
}

void dlgProfilePreferences::slot_changeLogFileAsHtml(const bool isHtml)
{
    if (isHtml) {
        comboBox_logFileNameFormat->setItemText(comboBox_logFileNameFormat->findData(qsl("yyyy-MM-dd#HH-mm-ss")), tr("yyyy-MM-dd#HH-mm-ss (e.g., 1970-01-01#00-00-00.html)"));
        comboBox_logFileNameFormat->setItemText(comboBox_logFileNameFormat->findData(qsl("yyyy-MM-ddTHH-mm-ss")), tr("yyyy-MM-ddTHH-mm-ss (e.g., 1970-01-01T00-00-00.html)"));
        comboBox_logFileNameFormat->setItemText(comboBox_logFileNameFormat->findData(qsl("yyyy-MM-dd")), tr("yyyy-MM-dd (concatenate daily logs in, e.g. 1970-01-01.html)"));
        comboBox_logFileNameFormat->setItemText(comboBox_logFileNameFormat->findData(qsl("yyyy-MM")), tr("yyyy-MM (concatenate month logs in, e.g. 1970-01.html)"));
        label_logFileNameExtension->setText(qsl(".html"));
    } else {
        comboBox_logFileNameFormat->setItemText(comboBox_logFileNameFormat->findData(qsl("yyyy-MM-dd#HH-mm-ss")), tr("yyyy-MM-dd#HH-mm-ss (e.g., 1970-01-01#00-00-00.txt)"));
        comboBox_logFileNameFormat->setItemText(comboBox_logFileNameFormat->findData(qsl("yyyy-MM-ddTHH-mm-ss")), tr("yyyy-MM-ddTHH-mm-ss (e.g., 1970-01-01T00-00-00.txt)"));
        comboBox_logFileNameFormat->setItemText(comboBox_logFileNameFormat->findData(qsl("yyyy-MM-dd")), tr("yyyy-MM-dd (concatenate daily logs in, e.g. 1970-01-01.txt)"));
        comboBox_logFileNameFormat->setItemText(comboBox_logFileNameFormat->findData(qsl("yyyy-MM")), tr("yyyy-MM (concatenate month logs in, e.g. 1970-01.txt)"));
        label_logFileNameExtension->setText(qsl(".txt"));
    }
}

/**
 * Update the chatname lineEdit when the chat name changes.
 * This may be called redundantly when the change originates from this
 * dialog (since the signal is emitted after the lineEdit was already
 * edited), but setText() does not emit editingFinished so no loop occurs.
 */
void dlgProfilePreferences::slot_setMMCPChatName(const QString& name)
{
    lineEdit_mmcpChatName->setText(name);
}

/**
 * Notify connected clients that our chatname has been changed (via GUI)
 *
 */
void dlgProfilePreferences::slot_mmcpChatNameChanged()
{
    if (mpHost) {
        if (!mpHost->setMMCPChatName(lineEdit_mmcpChatName->text().trimmed())) {
            // Validation failed — revert lineEdit to the current stored name
            lineEdit_mmcpChatName->setText(mpHost->getMMCPChatName());
        }
    }
}

void dlgProfilePreferences::setButtonColor(QPushButton* button, const QColor& color, const bool hasAlpha)
{
    if (color.isValid()) {
        if (button->isEnabled()) {
            if (hasAlpha) {
                // This is for buttons that show a color that may have
                // transparency; so,instead of colouring the background, we
                // include a generated black/white checkerboard pattern overlaid
                // with the colour which, when its alpha is not 100% opaque,
                // will (partly) show the checkerboard.

                // Ensure the icon has a 3:1 aspect ratio:
                if (auto iconWidth{button->iconSize().width()}, iconHeight{button->iconSize().height()}; iconWidth != iconHeight * 3) {
                    button->setIconSize(QSize(iconHeight * 3, iconHeight));
                }

                // Create a black/white checker background and overlay
                QPixmap labelBackground(1 + (button->iconSize().height() * 3), 1 + (button->iconSize().height()));
                labelBackground.fill(Qt::black);
                QPainter painter(&labelBackground);
                painter.drawImage(QRect(0, 0, labelBackground.width(), labelBackground.height()),
                                  QImage(qsl(":/icons/black_white_transparent_check_1x3_ratio.png")).scaled(labelBackground.width(), labelBackground.height(), Qt::KeepAspectRatioByExpanding));
                painter.fillRect(0, 0, labelBackground.width(), labelBackground.height(), color);
                painter.end();
                button->setIcon(QIcon(labelBackground));
            } else {
                button->setStyleSheet(mudlet::self()->mTEXT_ON_BG_STYLESHEET.arg(color.lightness() > 127 ? QLatin1String("black") : QLatin1String("white"), color.name()));
            }
            return;
        }

        const QColor disabledColor = QColor::fromHsl(color.hslHue(), color.hslSaturation() / 4, color.lightness(), color.alpha());
        if (hasAlpha) {
            // As above for buttons showing a potentially transparent color:
            if (auto iconWidth{button->iconSize().width()}, iconHeight{button->iconSize().height()}; iconWidth != iconHeight * 3) {
                button->setIconSize(QSize(iconHeight * 3, iconHeight));
            }

            QPixmap iconBackground(1 + (button->iconSize().height() * 3), 1 + (button->iconSize().height()));
            iconBackground.fill(Qt::black);
            QPainter painter(&iconBackground);
            painter.drawImage(QRect(0, 0, iconBackground.width(), iconBackground.height()),
                              QImage(qsl(":/icons/black_white_transparent_check_1x3_ratio.png")).scaled(iconBackground.width(), iconBackground.height(), Qt::KeepAspectRatioByExpanding));
            painter.fillRect(0, 0, iconBackground.width(), iconBackground.height(), disabledColor);
            painter.end();
            // Because the button is disabled we have to explicitly force our
            // icon to be used for that state otherwise the built-in icon engine
            // will assume our image is for the normal state and grey it out
            // completely by automagic means instead of making use of the
            // partial (desaturating) effect that we want to use:
            QIcon icon;
            icon.addPixmap(iconBackground, QIcon::Disabled, QIcon::Off);
            button->setIcon(icon);
        } else {
            button->setStyleSheet(mudlet::self()->mTEXT_ON_BG_STYLESHEET.arg(QLatin1String("darkGray"), disabledColor.name()));
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

void dlgProfilePreferences::slot_changeMenuBarVisibility(const enums::controlsVisibility state)
{
    switch (state) {
    case enums::visibleNever:
        if (comboBox_menuBarVisibility->currentIndex() != 0) {
            comboBox_menuBarVisibility->setCurrentIndex(0);
        }
        break;
    case enums::visibleOnlyWithoutLoadedProfile:
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

void dlgProfilePreferences::slot_changeToolBarVisibility(const enums::controlsVisibility state)
{
    switch (state) {
    case enums::visibleNever:
        if (comboBox_toolBarVisibility->currentIndex() != 0) {
            comboBox_toolBarVisibility->setCurrentIndex(0);
        }
        break;
    case enums::visibleOnlyWithoutLoadedProfile:
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
void dlgProfilePreferences::slot_changeGuiLanguage(int languageIndex)
{
    Q_UNUSED(languageIndex)

    auto languageCode = comboBox_guiLanguage->currentData().toString();
    mudlet::self()->setInterfaceLanguage(languageCode);
    label_languageChangeWarning->show();

    Host* pHost = mpHost;

    if (!pHost) {
        return;
    }

    pHost->mTelnet.sendInfoNewEnvironValue(qsl("LANGUAGE"));
}

// same warning palette as the system message area: soft yellow in light mode,
// muted amber in dark mode
QString dlgProfilePreferences::certificateWarningCheckBoxStyle() const
{
    const bool darkMode = mudlet::self()->inDarkMode();
    return qsl("font-weight: bold; color: %1; background: %2").arg(darkMode ? qsl("rgb(230, 230, 230)") : qsl("black"), darkMode ? qsl("rgb(64, 60, 40)") : qsl("rgb(255, 254, 215)"));
}

QString dlgProfilePreferences::certificateWarningLabelStyle() const
{
    const bool darkMode = mudlet::self()->inDarkMode();
    return qsl("font-weight: bold; color: %1; background: %2").arg(darkMode ? qsl("lightsalmon") : qsl("red"), darkMode ? qsl("rgb(64, 60, 40)") : qsl("rgb(255, 254, 215)"));
}

void dlgProfilePreferences::restyleCertificateWarnings()
{
    for (auto* checkBox : {checkBox_self_signed, checkBox_expired}) {
        if (!checkBox->styleSheet().isEmpty()) {
            checkBox->setStyleSheet(certificateWarningCheckBoxStyle());
        }
    }
    for (auto* label : {ssl_issuer_label, ssl_expires_label}) {
        if (!label->styleSheet().isEmpty()) {
            label->setStyleSheet(certificateWarningLabelStyle());
        }
    }
}

void dlgProfilePreferences::slot_setAppearance(const enums::Appearance state)
{
    if (comboBox_appearance->currentIndex() != state) {
        comboBox_appearance->setCurrentIndex(state);
    }

    const bool wasDarkMode = mudlet::self()->inDarkMode();
    mudlet::self()->setAppearance(state);
    const bool isDarkMode = mudlet::self()->inDarkMode();

    if (wasDarkMode == isDarkMode) {
        return;
    }

    // The shell's colours come from the palette, which has just been swapped:
    applyShellStyle();
    restyleCertificateWarnings();

    // Restyle the shortcut clash warning (if it is showing) for the new
    // palette:
    updateShortcutConflictWarning();

    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }

    const auto currentTheme = code_editor_theme_selection_combobox->currentText();

    if (isDarkMode) {
        const auto counterpart = findThemeCounterpart(currentTheme, code_editor_theme_selection_combobox, true);
        if (!counterpart.isEmpty()) {
            // save current as the light theme before switching
            pHost->mEditorTheme = currentTheme;
            pHost->mEditorThemeFile = code_editor_theme_selection_combobox->currentData().toString();
            switchEditorTheme(counterpart);
        }
    } else {
        const auto counterpart = findThemeCounterpart(currentTheme, code_editor_theme_selection_combobox, false);
        if (!counterpart.isEmpty()) {
            // save current as the dark theme before switching
            pHost->mEditorThemeDark = currentTheme;
            pHost->mEditorThemeFileDark = code_editor_theme_selection_combobox->currentData().toString();
            switchEditorTheme(counterpart);
        }
    }
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
    // the language" is shown in the newly selected language - on the basis that
    // it is the one the user understands rather than the currently used one.
    retranslateUi(this);
    // ...which has just put the .ui file's own group box titles back:
    retitleCards();
    // ...and which never reached the shell at all:
    retranslateShell();
    // Every text the search index was built from has just been replaced:
    invalidateSearch();

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
    setButtonColor(pushButton_playerRoomPrimaryColor, pHost->mpMap->mPlayerRoomOuterColor, true);
    setButtonColor(pushButton_playerRoomSecondaryColor, pHost->mpMap->mPlayerRoomInnerColor, true);
    pHost->mpMap->mPlayerRoomStyle = static_cast<quint8>(style);
    pHost->mPlayerRoomStyle = static_cast<quint8>(style);
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
    if (!pHost || !mpHost->mpMap) {
        return;
    }

    setPlayerRoomColor(pushButton_playerRoomPrimaryColor, mpHost->mpMap->mPlayerRoomOuterColor);
    pHost->mPlayerRoomOuterColor = mpHost->mpMap->mPlayerRoomOuterColor;
    if (comboBox_playerRoomStyle->currentIndex() != 3) {
        return;
    }

    if (mpHost->mpMap->mpMapper && mpHost->mpMap->mpMapper->mp2dMap) {
        // The current setting IS for the custom color - so use it straight away:
        mpHost->mpMap->mpMapper->mp2dMap->setPlayerRoomStyle(3);
        // And update the displayed map:
        mpHost->mpMap->mpMapper->mp2dMap->update();
    }
}

void dlgProfilePreferences::slot_setPlayerRoomSecondaryColor()
{
    Host* pHost = mpHost;
    if (!pHost || !mpHost->mpMap) {
        return;
    }

    setPlayerRoomColor(pushButton_playerRoomSecondaryColor, mpHost->mpMap->mPlayerRoomInnerColor);
    pHost->mPlayerRoomInnerColor = mpHost->mpMap->mPlayerRoomInnerColor;
    if (comboBox_playerRoomStyle->currentIndex() != 3) {
        return;
    }

    if (mpHost->mpMap->mpMapper && mpHost->mpMap->mpMapper->mp2dMap) {
        // The current setting IS for the custom color - so use it straight away:
        mpHost->mpMap->mpMapper->mp2dMap->setPlayerRoomStyle(3);
        // And update the displayed map:
        mpHost->mpMap->mpMapper->mp2dMap->update();
    }
}

void dlgProfilePreferences::slot_setPlayerRoomOuterDiameter(const int value)
{
    Host* pHost = mpHost;
    if (!pHost || !pHost->mpMap) {
        return;
    }

    if (value < 256 && pHost->mpMap->mPlayerRoomOuterDiameterPercentage != value) {
        pHost->mpMap->mPlayerRoomOuterDiameterPercentage = static_cast<quint8>(value);
        pHost->mPlayerRoomOuterDiameterPercentage = static_cast<quint8>(value);
        if (pHost->mpMap->mpMapper && pHost->mpMap->mpMapper->mp2dMap) {
            // And update the displayed map:
            pHost->mpMap->mpMapper->mp2dMap->update();
        }
    }
}

void dlgProfilePreferences::slot_setPlayerRoomInnerDiameter(const int value)
{
    Host* pHost = mpHost;
    if (!pHost || !pHost->mpMap) {
        return;
    }

    if (value < 256 && pHost->mpMap->mPlayerRoomInnerDiameterPercentage != value) {
        pHost->mpMap->mPlayerRoomInnerDiameterPercentage = static_cast<quint8>(value);
        pHost->mPlayerRoomInnerDiameterPercentage = static_cast<quint8>(value);
        if (pHost->mpMap->mpMapper && pHost->mpMap->mpMapper->mp2dMap) {
            // Redefine the QGradientStops
            pHost->mpMap->mpMapper->mp2dMap->setPlayerRoomStyle(qBound(0, comboBox_playerRoomStyle->currentIndex(), 3));
            // And update the displayed map:
            pHost->mpMap->mpMapper->mp2dMap->update();
        }
    }
}

void dlgProfilePreferences::setPlayerRoomColor(QPushButton* b, QColor& c)
{
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }

    auto color = QColorDialog::getColor(
            c, this, (b == pushButton_playerRoomPrimaryColor ? tr("Set outer color of player room mark.") : tr("Set inner color of player room mark.")), QColorDialog::ShowAlphaChannel);
    if (color.isValid()) {
        c = color;

        // Also sets a contrasting foreground color so text will always be
        // visible and adjusts the saturation of a disabled button:
        setButtonColor(b, color, true);
    }
}

void dlgProfilePreferences::slot_setPostingTimeout(const double timeout)
{
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }

    pHost->mTelnet.setPostingTimeout(qRound(1000.0 * timeout));
}

void dlgProfilePreferences::slot_changeControlCharacterHandling()
{
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }

    pHost->setControlCharacterMode(comboBox_controlCharacterHandling->currentData().value<ControlCharacterMode>());
}


void dlgProfilePreferences::slot_toggleAdvertiseScreenReader(const bool state)
{
    Host* pHost = mpHost;

    if (!pHost) {
        return;
    }

    if (pHost->mAdvertiseScreenReader != state) {
        pHost->mAdvertiseScreenReader = state;
        pHost->mTelnet.sendInfoNewEnvironValue(qsl("SCREEN_READER"));
        pHost->mTelnet.sendInfoNewEnvironValue(qsl("MTTS"));
    }
}

void dlgProfilePreferences::slot_toggleEnableOSC8Hyperlinks(const bool state)
{
    Host* pHost = mpHost;

    if (!pHost) {
        return;
    }

    if (pHost->mEnableOSC8Hyperlinks != state) {
        pHost->mEnableOSC8Hyperlinks = state;
        pHost->mTelnet.sendInfoNewEnvironOSCHyperlinks();
    }
}

void dlgProfilePreferences::slot_toggleEnableClosedCaption(const bool state)
{
    if (mpHost && mpHost->mEnableClosedCaption != state) {
        mpHost->mEnableClosedCaption = state;
    }
}

void dlgProfilePreferences::slot_changeWrapAt()
{
    Host* pHost = mpHost;

    if (!pHost) {
        return;
    }

    pHost->mTelnet.sendInfoNewEnvironValue(qsl("WORD_WRAP"));
}

void dlgProfilePreferences::slot_toggleUseMaxBufferSize(bool checked)
{
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }

    if (checked) {
        // When max is enabled, set spinbox to max value and disable it
        if (pHost->mpConsole) {
            const int maxBufferSize = pHost->mpConsole->buffer.getMaxBufferSize();
            console_buffer_size_spinBox->setValue(maxBufferSize);
        }
        console_buffer_size_spinBox->setEnabled(false);
    } else {
        // When max is disabled, enable the spinbox and set to stored value
        console_buffer_size_spinBox->setEnabled(true);
        console_buffer_size_spinBox->setValue(pHost->getConsoleBufferSize());
    }
}

void dlgProfilePreferences::slot_deleteMap()
{
    Host* pHost = mpHost;
    if (!pHost || !pHost->mpMap) {
        return;
    }

    // Disable the button, but set it to be down until process is complete
    pushButton_deleteMap->setCheckable(true);
    pushButton_deleteMap->setChecked(true);

    // Move the focus to the load map button, otherwise it will jump down to
    // the next button in the tab stop sequence (Copy map to other profiles)
    // which is not really appropriate:
    pushButton_loadMap->setFocus(Qt::OtherFocusReason);

    label_mapFileActionResult->show();
    label_mapFileActionResult->setText(tr("Deleting map - please wait..."));
    qApp->processEvents(); // Allow the above message to show up when erasing big maps
    pHost->mpMap->mapClear();
    pHost->mpMap->updateArea(-1);

    // Reset the button but leave it disabled
    pushButton_deleteMap->setChecked(false);
    pushButton_deleteMap->setCheckable(false);

    label_mapFileActionResult->setText(tr("Deleted map."));
    qApp->processEvents(); // Allow the above message to show up when erasing big maps

    QTimer::singleShot(10s, this, &dlgProfilePreferences::slot_hideActionLabel);
}

void dlgProfilePreferences::slot_changeLargeAreaExitArrows(const bool state)
{
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }

    pHost->setLargeAreaExitArrows(state);
}

void dlgProfilePreferences::slot_changeInvertMapZoom(const bool state)
{
    mudlet::self()->setInvertMapZoom(state);
}

bool dlgProfilePreferences::updateDisplayFont()
{
    if (mpHost.isNull() || (mpHost.data()->mpConsole.isNull())) {
        return false;
    }

    QFont displayFont = fontComboBox_displayFont->currentFont();
    displayFont.setPointSize(spinBox_displayFontSize->value());
    displayFont.setStyleHint(QFont::AnyStyle,
                             checkBox_antiAlias->isChecked() ? static_cast<QFont::StyleStrategy>(QFont::PreferAntialias | QFont::PreferQuality)
                                                             : static_cast<QFont::StyleStrategy>(QFont::NoAntialias | QFont::PreferQuality));

    if (TFontAttributes(mpHost->getDisplayFont()) == TFontAttributes(displayFont)) {
        // No change!
        return false;
    }

    const QFontMetrics metrics(displayFont);
    if (metrics.averageCharWidth() == 0) {
        label_invalidFontError->show();
        return false;
    }
    label_invalidFontError->hide();

    if (!QFontInfo(displayFont).fixedPitch()) {
        label_variableWidthFontWarning->show();
    } else {
        label_variableWidthFontWarning->hide();
    }

#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
#if QT_VERSION < QT_VERSION_CHECK(6, 9, 0)
    // On GNU/Linux or FreeBSD ensure that emojis are displayed in colour even
    // if this font doesn't support it:
    QFont::insertSubstitution(mpHost->getDisplayFont().family(), qsl("Noto Color Emoji"));
#endif
    // For Qt 6.9+, emoji font support is handled globally in FontManager::addEmojiFont()
#endif

    // update the display properly when font or size or antiAliasing selections
    // change.
    mpHost->setDisplayFont(displayFont);

    auto config = edbeePreviewWidget->config();
    config->beginChanges();
    config->setFont(displayFont);
    config->endChanges();

    return true;
}

void dlgProfilePreferences::cancelShortcutCaptures()
{
    const auto sequenceEdits = findChildren<QKeySequenceEdit*>();
    for (auto* sequenceEdit : sequenceEdits) {
        if (!sequenceEdit) {
            continue;
        }

        if (sequenceEdit->hasFocus()) {
            sequenceEdit->clearFocus();
        }

        sequenceEdit->releaseKeyboard();
    }
}

void dlgProfilePreferences::slot_displayFontChanged()
{
    if (!mpHost.isNull() && updateDisplayFont()) {
        mpHost->mTelnet.sendInfoNewEnvironValue(qsl("FONT"));
    }
}

void dlgProfilePreferences::slot_displayFontSizeChanged()
{
    if (!mpHost.isNull() && updateDisplayFont()) {
        mpHost->mTelnet.sendInfoNewEnvironValue(qsl("FONT_SIZE"));
    }
}

void dlgProfilePreferences::slot_displayFontAliasingChanged()
{
    updateDisplayFont();
}

void dlgProfilePreferences::slot_changeShowTabConnectionIndicators(bool state)
{
    if (checkBox_showTabConnectionIndicators->isChecked() != state) {
        checkBox_showTabConnectionIndicators->setChecked(state);
    }
}

void dlgProfilePreferences::slot_roomSizeChanged(int size)
{
    if (mpHost) {
        mpHost->mRoomSize = static_cast<float>(size) / 10.0f;
        if (mpHost->mpMap && mpHost->mpMap->mpMapper && mpHost->mpMap->mpMapper->mp2dMap) {
            mpHost->mpMap->mpMapper->mp2dMap->setRoomSize(static_cast<float>(size) / 10.0f);
            mpHost->mpMap->mpMapper->mp2dMap->update();
        }
    }
}

void dlgProfilePreferences::slot_crashReportPolicyChanged(int index)
{
    QSettings settings("Mudlet", "CrashReporter");
    settings.setValue("autoSendCrashReports", index + 1);
}

void dlgProfilePreferences::slot_exitSizeChanged(int size)
{
    if (mpHost) {
        const double internalSize = 50.0 / size;
        mpHost->mLineSize = internalSize;
        if (mpHost->mpMap && mpHost->mpMap->mpMapper && mpHost->mpMap->mpMapper->mp2dMap) {
            mpHost->mpMap->mpMapper->mp2dMap->setExitSize(internalSize);
        }
    }
}

void dlgProfilePreferences::slot_borderSizeChanged(int size)
{
    if (mpHost) {
        const double internalSize = 50.0 / size;
        mpHost->mRoomBorderSize = internalSize;
        if (mpHost->mpMap && mpHost->mpMap->mpMapper && mpHost->mpMap->mpMapper->mp2dMap) {
            mpHost->mpMap->mpMapper->mp2dMap->setBorderSize(internalSize);
        }
    }
}

void dlgProfilePreferences::slot_gridSizeChanged(double size)
{
    if (mpHost) {
        mpHost->mMapGridLineSize = size;
        if (mpHost->mpMap && mpHost->mpMap->mpMapper && mpHost->mpMap->mpMapper->mp2dMap) {
            mpHost->mpMap->mpMapper->mp2dMap->update();
        }
    }
}

void dlgProfilePreferences::reject()
{
    // Esc has to mean what the window's close button means, and QDialog's own
    // reject() hides the dialog without ever sending a close event - so it is
    // routed through close(), which arrives back here from
    // QDialog::closeEvent() with nothing left to do but the base class.
    if (!mClosing) {
        close();
        return;
    }
    QDialog::reject();
}

void dlgProfilePreferences::closeEvent(QCloseEvent* event)
{
    cancelShortcutCaptures();

    if (mpDialogMapGlyphUsage) {
        mpDialogMapGlyphUsage->close();
        mpDialogMapGlyphUsage = nullptr;
    }

    // Closing is not a discard: the last edit gets its write-back here rather
    // than waiting out a debounce the dialog will not live to see. Clearing the
    // focus is what makes a field still being typed into report as finished.
    if (QWidget* pFocus = focusWidget(); pFocus) {
        pFocus->clearFocus();
    }
    mpTimer_apply->stop();
    applyAll();

    // The profile XML is written once per close rather than on every apply:
    if (Host* pHost = mpHost; pHost && pHost->mFORCE_SAVE_ON_EXIT) {
        pHost->saveProfile();
    }

    mudlet::getQSettings()->setValue(qsl("profilePreferencesGeometry"), saveGeometry());

    if (mpHost) {
        emit preferencesClosing(mpHost->getName());
    }
    mClosing = true;
    QDialog::closeEvent(event);
    mClosing = false;
}
