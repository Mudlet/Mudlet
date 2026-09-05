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
#include "MudletApp.h"
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
#include "ctelnet.h"
#include "discord.h"
#include "dlgIRC.h"
#include "dlgMapper.h"
#include "dlgTriggerEditor.h"
#include "edbee/views/texteditorscrollarea.h"
#include "MMCP.h"
#include "widgetutils.h"
#include "utils.h"

#include <chrono>
#include <vector>
#include <QtConcurrentRun>
#include <QAbstractScrollArea>
#include <QAbstractSpinBox>
#include <QAccessible>
#include <QApplication>
#include <QBuffer>
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
#include <QLocale>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QScopedValueRollback>
#include <QScrollArea>
#include <QScrollBar>
#include <QShortcut>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QToolButton>
#include <QVariantAnimation>
#include "../3rdparty/kdtoolbox/singleshot_connect/singleshot_connect.h"

using namespace std::chrono_literals;

// A reading width: whitespace absorbs a wide window rather than the controls
// stretching across it
static constexpr int scmContentColumnWidth = 640;

// Known as numbers rather than left to the stylesheet: the selected item's
// accent bar is a gradient stop, and a gradient stop is a fraction of the item
// rather than a length.
static constexpr int scmSidebarWidth = 232;
static constexpr int scmSidebarPadding = 12;
static constexpr int scmSidebarAccentBarWidth = 3;
static constexpr int scmSidebarRailWidth = 48;
static constexpr int scmSidebarRailPadding = 6;
// How far right of the frame edge this leaves a card's title is not a constant
// to go with it: styles disagree on the room after the indicator, so
// measuredCardTitleInset() measures it.
static constexpr int scmCardIndicatorSize = 13;

static const QString scmCategory_general = qsl("general");
static const QString scmCategory_appearance = qsl("appearance");
static const QString scmCategory_mainDisplay = qsl("mainDisplay");
static const QString scmCategory_inputLine = qsl("inputLine");
static const QString scmCategory_editor = qsl("editor");
static const QString scmCategory_mapper = qsl("mapper");
static const QString scmCategory_chat = qsl("chat");
static const QString scmCategory_connection = qsl("connection");
static const QString scmCategory_privacy = qsl("privacy");
static const QString scmCategory_accessibility = qsl("accessibility");
static const QString scmCategory_shortcuts = qsl("shortcuts");
static const QString scmCategory_advanced = qsl("advanced");

static constexpr int scmRole_categoryKey = Qt::UserRole;
static constexpr int scmRole_externalUrl = Qt::UserRole + 1;

// How long the search field waits for the typing to stop. Long enough that a
// burst of keystrokes costs one search rather than one each, short enough that
// the pause at the end of a word is not a wait.
static constexpr auto scmSearchDebounce = 150ms;

// Synonyms a control is searchable by that it does not show anywhere. The
// property names the shell stylesheet selects on stay literals beside it: a
// constant cannot be interpolated into a QStringLiteral.
static constexpr char scmProp_searchKeywords[] = "searchKeywords";

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

    mpTimer_search = new QTimer(this);
    mpTimer_search->setObjectName(qsl("settingsSearchDebounce"));
    mpTimer_search->setSingleShot(true);
    mpTimer_search->setInterval(scmSearchDebounce);
    connect(mpTimer_search, &QTimer::timeout, this, [this]() {
        runSearch(mPendingSearch);
    });

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
    if (mpCard_discord) {
        mpCard_discord->hide();
    }

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
    populateApplicationSettings();

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
    if (MudletApp::development() && !qEnvironmentVariableIsSet("DEV_UPDATER")) {
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
    connect(pMudlet, &mudlet::signal_hostCreated, this, &dlgProfilePreferences::slot_handleHostAddition);
    connect(pMudlet, &mudlet::signal_hostDestroyed, this, &dlgProfilePreferences::slot_handleHostDeletion);
    // Because QComboBox::currentIndexChanged has multiple (overloaded) forms we
    // have to state which one we want to use for these two:
    connect(comboBox_menuBarVisibility, qOverload<int>(&QComboBox::currentIndexChanged), this, &dlgProfilePreferences::slot_changeShowMenuBar);
    connect(comboBox_toolBarVisibility, qOverload<int>(&QComboBox::currentIndexChanged), this, &dlgProfilePreferences::slot_changeShowToolBar);

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
    // Every entry is named in its own language, which is what someone who reads
    // it would look for - but a search for one they do not read is typed in the
    // language they do, and no English name for any of them is on the card
    QStringList languageSearchNames;
    for (int i = 0, total = comboBox_guiLanguage->count(); i < total; ++i) {
        const QLocale locale(comboBox_guiLanguage->itemData(i).toString());
        languageSearchNames << QLocale::languageToString(locale.language()) << locale.nativeLanguageName();
    }
    languageSearchNames.removeDuplicates();
    comboBox_guiLanguage->setProperty(scmProp_searchKeywords, languageSearchNames.join(qsl(", ")));

    auto currentLanguage = MudletApp::getInterfaceLanguage();
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

    connect(comboBox_crashReportPolicy, qOverload<int>(&QComboBox::currentIndexChanged), this, &dlgProfilePreferences::slot_crashReportPolicyChanged);

    setupPasswordsMigration();

    connectApplyTriggers();
    mSnapshot.take();
    guardScrollWheel();

    applyShellStyle();

    // After the stylesheet, not before it: a card's padding and the weight of
    // its text both arrive with that stylesheet, and a cap measured without
    // them is a cap of a page nobody will ever see. It is also the first moment
    // every control is on the card it belongs on, which the wrapping waits for.
    mShellReady = true;
    updateColumnWidthCaps();
    rebuildTabOrder();

    setMinimumSize(780, 560);
    const auto geometry = MudletApp::getQSettings()->value(qsl("profilePreferencesGeometry")).toByteArray();
    if (geometry.isEmpty() || !restoreGeometry(geometry)) {
        resize(1060, 760);
    }
    // resize() on a dialog that has never been shown does not always deliver a
    // resize event to read the width from
    updateSidebarMode();
}

dlgProfilePreferences::~dlgProfilePreferences()
{
    // ~QDialog hides the dialog once this destructor is done, and the widget
    // that has the keyboard focus then emits its editingFinished() - the chat
    // name field and the shortcut editors both act on that one - when this
    // object is no longer a valid receiver (#9574)
    widgetutils::disconnectChildSignals(this);
}

// QLayout::setAlignment() is documented not to look in child layouts, and a
// card's controls are nested in them
static bool alignInLayoutTree(QLayout* pLayout, const QWidget* pWidget, const Qt::Alignment alignment)
{
    for (int i = 0, total = pLayout->count(); i < total; ++i) {
        QLayoutItem* pItem = pLayout->itemAt(i);
        if (pItem->widget() == pWidget) {
            pItem->setAlignment(alignment);
            return true;
        }
        if (QLayout* pChildLayout = pItem->layout(); pChildLayout && alignInLayoutTree(pChildLayout, pWidget, alignment)) {
            return true;
        }
    }
    return false;
}

// QLayout::removeWidget() only looks at its own items, and the .ui file nests
// controls several layouts deep. Qt would find it from QLayout::addWidget()
// instead, but warns once per widget - which for a dialog's worth of moves
// buries anything else on the console.
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

// A profile's Lua stylesheet is applied to the whole dialog and reaches every
// widget it does not name, so the shell's own scaffolding carries a property
// the shell stylesheet keeps it transparent by.
static void markAsShellSurface(QWidget* pWidget)
{
    pWidget->setProperty("settingsSurface", true);
}

static QLabel* wrapLabelOf(const QCheckBox* pCheckBox)
{
    QWidget* pContainer = pCheckBox->parentWidget();
    if (!pContainer || pContainer->objectName() != qsl("settingsCheckBoxWrap")) {
        return nullptr;
    }
    return pContainer->findChild<QLabel*>(QString(), Qt::FindDirectChildrenOnly);
}

// A column narrower than its contents clips them rather than scrolling, so the
// cap is the reading width or whatever the widest card needs. The page is held
// to the column plus its scrollbar, so the bar stays beside what it scrolls.
void dlgProfilePreferences::capColumnWidth(QScrollArea* pScrollArea)
{
    QWidget* pColumn = pScrollArea ? pScrollArea->widget() : nullptr;
    if (!pColumn || !pColumn->layout()) {
        return;
    }
    // Lifted first, so what is measured is the cards rather than the last cap:
    pColumn->setMaximumWidth(QWIDGETSIZE_MAX);
    pColumn->layout()->activate();
    fitCheckBoxesToColumn(pColumn);
    const int cap = std::max(scmContentColumnWidth, pColumn->minimumSizeHint().width());
    pColumn->setMaximumWidth(cap);
    pScrollArea->setMaximumWidth(cap + pScrollArea->verticalScrollBar()->sizeHint().width());
}

// A layout tells the layouts above it that it has changed by *posting* a layout
// request, and nothing between a wrap and the measurement that judges it runs an
// event loop to deliver one. Both halves below are needed: invalidate() drops
// what a layout worked out about its items, while what it caches about a
// *widget* - the size hints, in the layout item it made for it - goes only with
// updateGeometry() on that widget.
static void invalidateLayoutsUpTo(QWidget* pWidget, const QWidget* pColumn)
{
    for (QWidget* pAncestor = pWidget; pAncestor; pAncestor = pAncestor->parentWidget()) {
        if (QLayout* pLayout = pAncestor->layout(); pLayout) {
            pLayout->invalidate();
        }
        pAncestor->updateGeometry();
        if (pAncestor == pColumn) {
            return;
        }
    }
}

// The pass starts by giving every text back to the checkbox it came from, so
// what it measures is the language on show now.
void dlgProfilePreferences::fitCheckBoxesToColumn(QWidget* pColumn)
{
    // While the shell is being put together the constructor is still moving
    // controls between cards, and a control moved out of a wrap it was given a
    // moment ago would leave that wrap behind, empty, on the page it left.
    if (!mShellReady) {
        return;
    }
    QList<QCheckBox*> checkBoxes = pColumn->findChildren<QCheckBox*>();
    for (auto* pCheckBox : checkBoxes) {
        unwrapCheckBox(pCheckBox);
        invalidateLayoutsUpTo(pCheckBox, pColumn);
    }
    if (pColumn->minimumSizeHint().width() <= scmContentColumnWidth) {
        return;
    }

    std::sort(checkBoxes.begin(), checkBoxes.end(), [](const QCheckBox* pOne, const QCheckBox* pOther) {
        return pOne->sizeHint().width() > pOther->sizeHint().width();
    });
    for (auto* pCheckBox : checkBoxes) {
        const int before = pColumn->minimumSizeHint().width();
        if (before <= scmContentColumnWidth) {
            return;
        }
        if (pCheckBox->text().isEmpty()) {
            continue;
        }
        wrapCheckBox(pCheckBox);
        invalidateLayoutsUpTo(pCheckBox, pColumn);
        if (pColumn->minimumSizeHint().width() >= before) {
            unwrapCheckBox(pCheckBox);
            invalidateLayoutsUpTo(pCheckBox, pColumn);
        }
    }
}

// The QCheckBox stays the control of record - the object the apply triggers,
// the snapshot and the tests know - and gives up only its text, which a QLabel
// can wrap and it cannot. Its accessible name keeps that text.
void dlgProfilePreferences::wrapCheckBox(QCheckBox* pCheckBox)
{
    QLabel* pLabel = wrapLabelOf(pCheckBox);
    if (!pLabel) {
        QWidget* pParent = pCheckBox->parentWidget();
        QLayout* pParentLayout = pParent ? pParent->layout() : nullptr;
        if (!pParentLayout) {
            return;
        }
        auto* pContainer = new QWidget(pParent);
        pContainer->setObjectName(qsl("settingsCheckBoxWrap"));
        // Without this the label's height-for-width stops at the container, and
        // the row is given one line's worth of room whatever it has to say
        QSizePolicy policy = pContainer->sizePolicy();
        policy.setHeightForWidth(true);
        pContainer->setSizePolicy(policy);
        QLayoutItem* pTakenOut = pParentLayout->replaceWidget(pCheckBox, pContainer, Qt::FindChildrenRecursively);
        if (!pTakenOut) {
            delete pContainer;
            return;
        }
        delete pTakenOut;
        // Without an alignment the container is stretched to whatever the
        // tallest control on the row is, and the checkbox - held against the
        // label's first line below - floats to the top of that instead of
        // sitting level with what it shares the row with. Only the vertical
        // half is given, so the words still have the whole row to wrap in.
        alignInLayoutTree(pParentLayout, pContainer, Qt::AlignVCenter);
        auto* pRowLayout = new QHBoxLayout(pContainer);
        pRowLayout->setContentsMargins(0, 0, 0, 0);
        pRowLayout->setSpacing(style()->pixelMetric(QStyle::PM_CheckBoxLabelSpacing, nullptr, pCheckBox));
        pLabel = new QLabel(pContainer);
        pLabel->setObjectName(qsl("settingsWrappedLabel"));
        pLabel->setWordWrap(true);
        // Clicking the words is how a checkbox is used - see eventFilter()
        pLabel->installEventFilter(this);
        pLabel->setBuddy(pCheckBox);
        // The host enable/disable lists name the checkbox, which no longer draws
        // the words that would have greyed out with it - so the label follows
        // the checkbox's enabled state instead
        pCheckBox->installEventFilter(this);
        // Against the first line, not the middle of however many the label needs
        pRowLayout->addWidget(pCheckBox, 0, Qt::AlignTop);
        pRowLayout->addWidget(pLabel, 1);
    }
    if (pCheckBox->text().isEmpty()) {
        return;
    }
    pLabel->show();
    pLabel->setText(pCheckBox->text());
    pLabel->setToolTip(pCheckBox->toolTip());
    pLabel->setEnabled(pCheckBox->isEnabled());
    pLabel->setProperty(scmProp_searchKeywords, pCheckBox->property(scmProp_searchKeywords));
    pCheckBox->setProperty(scmProp_searchKeywords, QVariant());
    pCheckBox->setAccessibleName(pCheckBox->text());
    pCheckBox->setText(QString());
}

// The container is left in place: a checkbox that fits one language may not fit
// the next. An unwrapped one is a checkbox with its text again and an empty
// label beside it.
void dlgProfilePreferences::unwrapCheckBox(QCheckBox* pCheckBox)
{
    QLabel* pLabel = wrapLabelOf(pCheckBox);
    if (!pLabel || pLabel->text().isEmpty()) {
        return;
    }
    // Handed back only where the checkbox has nothing of its own to say: a
    // language change writes the new words straight onto it, and the label is
    // holding the *previous* language by then.
    if (pCheckBox->text().isEmpty()) {
        pCheckBox->setText(pLabel->text());
    }
    if (pCheckBox->property(scmProp_searchKeywords).toString().isEmpty()) {
        pCheckBox->setProperty(scmProp_searchKeywords, pLabel->property(scmProp_searchKeywords));
    }
    pCheckBox->setAccessibleName(QString());
    pLabel->setProperty(scmProp_searchKeywords, QVariant());
    pLabel->clear();
    pLabel->setToolTip(QString());
    pLabel->hide();
}

// Every control setupUi() made is *moved* onto the shell rather than recreated:
// the host enable/disable lists, the translations and the tests hold pointers
// to those widgets.
void dlgProfilePreferences::buildShell()
{
    while (tabWidget->count()) {
        tabWidget->removeTab(0);
    }
    vBoxLayout_main->removeWidget(tabWidget);
    tabWidget->hide();
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
    mpWidget_titleRow = pTitleRow;
    markAsShellSurface(pTitleRow);
    auto* pTitleRowLayout = new QHBoxLayout(pTitleRow);
    pTitleRowLayout->setContentsMargins(0, 0, 0, 0);
    pTitleRowLayout->setSpacing(10);
    // Hidden on a category page, where it would push that page's title sideways
    mpButton_searchBack = new QToolButton(pTitleRow);
    mpButton_searchBack->setObjectName(qsl("settingsSearchBack"));
    mpButton_searchBack->setArrowType(Qt::LeftArrow);
    mpButton_searchBack->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    mpButton_searchBack->setAutoRaise(true);
    // ...and reachable by keyboard, which a tool button is not by default
    mpButton_searchBack->setFocusPolicy(Qt::StrongFocus);
    mpButton_searchBack->hide();
    connect(mpButton_searchBack, &QAbstractButton::clicked, this, [this]() {
        if (const QString subpage = mSubpageBeforeSearch; !subpage.isEmpty()) {
            showSubpage(subpage.section(QLatin1Char('/'), 0, 0), subpage.section(QLatin1Char('/'), 1));
            return;
        }
        showCategory(mCategoryBeforeSearch.isEmpty() ? scmCategory_general : mCategoryBeforeSearch);
    });
    pTitleRowLayout->addWidget(mpButton_searchBack);
    mpButton_subpageBack = new QToolButton(pTitleRow);
    mpButton_subpageBack->setObjectName(qsl("settingsSubpageBack"));
    mpButton_subpageBack->setArrowType(Qt::LeftArrow);
    mpButton_subpageBack->setAutoRaise(true);
    mpButton_subpageBack->setFocusPolicy(Qt::StrongFocus);
    mpButton_subpageBack->hide();
    connect(mpButton_subpageBack, &QAbstractButton::clicked, this, &dlgProfilePreferences::leaveSubpage);
    pTitleRowLayout->addWidget(mpButton_subpageBack);
    mpLabel_pageTitleIcon = new QLabel(pTitleRow);
    mpLabel_pageTitleIcon->setObjectName(qsl("settingsPageTitleIcon"));
    // Fixed, so the title starts at the same x on every category page whatever
    // the shape of that category's icon
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
    buildCategoryPage(scmCategory_general, {groupBox_miscellaneous, groupBox_encoding, groupBox_logOptions, groupbox_searchEngineSelection, groupBox_updates, pCard_systemIntegration});

    auto* pCard_theme = createCard(qsl("card_theme"));
    addCardRow(pCard_theme, label_appearance, comboBox_appearance);
    auto* pCard_profileTabs = createCard(qsl("card_profileTabs"));
    moveIntoCard(pCard_profileTabs, {checkBox_showTabConnectionIndicators});
    buildCategoryPage(scmCategory_appearance, {pCard_theme, groupBox_iconsAndToolbars, pCard_profileTabs});

    // Its two controls read as one more display option rather than a card of their own
    moveIntoCard(groupBox_displayOptions, {doubleclick_ignore_label, doubleclick_ignore_lineedit, checkBox_enableOSC8Hyperlinks});
    groupBox_doubleClick->hide();
    reflowDisplayOptionsCard();
    buildCategoryPage(scmCategory_mainDisplay, {groupBox_font, groupBox_displayColors, groupBox_borders, groupBox_wrapping, groupBox_consoleBuffer, groupBox_displayOptions});

    buildCategoryPage(scmCategory_inputLine, {groupBox_input, groupBox_spellCheck});

    moveIntoCard(groupBox_autoComplete, {checkBox_echoLuaErrors});
    buildCategoryPage(scmCategory_editor, {groupbox_codeEditorThemeSelection, groupBox_autoComplete, groupBox_editorDisplayOptions});

    buildCategoryPage(scmCategory_mapper, {groupBox_mapFiles, groupBox_downloadMapOptions, groupBox_mapViewOptions, groupBox_mapperColors, groupBox_playerRoomStyle});

    buildDiscordSummaryCard();
    buildCategoryPage(scmCategory_chat, {mpCard_discord, groupBox_MMCPOptions});
    addSubpage(scmCategory_chat, qsl("discord"), mpCard_discord, {groupBox_discordPrivacy});

    auto* pCard_dataEncoding = createCard(qsl("card_dataEncoding"));
    addCardRow(pCard_dataEncoding, label_encoding, comboBox_encoding);
    moveIntoCard(groupBox_specialOptions, {checkBox_USE_IRE_DRIVER_BUGFIX});
    reflowCompatibilityCard();
    auto* pCard_network = createCard(qsl("card_network"));
    addCardRow(pCard_network, label_networkPacketTimeout, doubleSpinBox_networkPacketTimeout);
    buildProtocolsSubpage();
    buildCategoryPage(scmCategory_connection, {groupBox_protocols, pCard_dataEncoding, groupBox_specialOptions, pCard_network});
    addSubpage(scmCategory_connection, qsl("protocols"), groupBox_protocols, {mpCard_protocolList});

    auto* pCard_passwords = createCard(qsl("card_passwords"));
    addCardRow(pCard_passwords, label_store_passwords_in, comboBox_store_passwords_in);
    moveIntoCard(pCard_passwords, {label_password_migration_notification, pushButton_forgetSavedSignIn});
    auto* pCard_serverPermissions = createCard(qsl("card_serverPermissions"));
    moveIntoCard(pCard_serverPermissions, {acceptServerGUI, acceptServerMedia});
    auto* pCard_crashReports = createCard(qsl("card_crashReports"));
    addCardRow(pCard_crashReports, label_crashReportPolicy, comboBox_crashReportPolicy);
    // Beside groupBox_ssl rather than inside it: a checkable group box disables
    // its children when unchecked, and asking about an available secure
    // connection is exactly what someone with TLS switched off wants.
    auto* pCard_secureReminder = createCard(qsl("card_secureConnectionReminder"));
    pCard_secureReminder->setProperty("settingsCardPlain", true);
    moveIntoCard(pCard_secureReminder, {checkBox_askTlsAvailable});
    buildSecurityStatusCard();
    buildCategoryPage(scmCategory_privacy,
                      {mpCard_securityStatus, groupBox_ssl, pCard_secureReminder, groupBox_proxy, pCard_passwords, pCard_serverPermissions, groupBox_purgeMediaCache, pCard_crashReports});

    auto* pCard_captions = createCard(qsl("card_accessibilityText"));
    addCardRow(pCard_captions, label_blankLinesBehaviour, comboBox_blankLinesBehaviour);
    moveIntoCard(pCard_captions, {checkBox_enableBlinkText, checkBox_enableClosedCaption});
    auto* pCard_keyboard = createCard(qsl("card_accessibilityKeyboard"));
    addCardRow(pCard_keyboard, label_caretModeKey, comboBox_caretModeKey);
    moveIntoCard(pCard_keyboard, {checkBox_f3SearchEnabled});
    buildCategoryPage(scmCategory_accessibility, {groupBox_accessibility, pCard_captions, pCard_keyboard});

    buildCategoryPage(scmCategory_shortcuts, {groupBox_main_window_shortcuts});

    buildCategoryPage(scmCategory_advanced, {groupBox_debug});

    buildSearchResultsPage();

    connect(mpListWidget_categories, &QListWidget::currentRowChanged, this, &dlgProfilePreferences::slot_categorySelected);
    // Return on the support link is how a keyboard user follows it
    connect(mpListWidget_categories, &QListWidget::itemActivated, this, &dlgProfilePreferences::slot_sidebarItemClicked);
    connect(mpListWidget_categories, &QListWidget::itemClicked, this, [this](QListWidgetItem* pItem) {
        // Where the style counts a single click as an activation, the connection
        // above has already opened the browser
        if (!mpListWidget_categories->style()->styleHint(QStyle::SH_ItemView_ActivateItemOnSingleClick, nullptr, mpListWidget_categories)) {
            slot_sidebarItemClicked(pItem);
        }
    });
    connect(mpLineEdit_search, &QLineEdit::textChanged, this, &dlgProfilePreferences::queueSearch);

    retranslateShell();

    // Otherwise the stack shows its first page with nothing selected over it;
    // setTab() overrides this for a deep link
    showCategory(scmCategory_general);
}

// Left null once dismissed, which every page takes in its stride
void dlgProfilePreferences::buildMigrationBanner()
{
    if (MudletApp::getQSettings()->value(qsl("settingsRedesignBannerSeen"), false).toBool()) {
        return;
    }

    mpFrame_migrationBanner = new QFrame(this);
    mpFrame_migrationBanner->setObjectName(qsl("settingsMigrationBanner"));
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
        MudletApp::getQSettings()->setValue(qsl("settingsRedesignBannerSeen"), true);
        // Off the page and out of the member, so no later page switch brings it
        // back. Alive rather than deleted: the click is still being delivered.
        placeBannerOn(nullptr);
        mpFrame_migrationBanner = nullptr;
    });
}

// Pinned above the stack it would eat 130px of every page's height at the
// 780x560 minimum, so it rides at the top of whichever page is showing. It
// comes off while the search owns the stack, as a card's place in the index is
// its position in its column and a banner above would make each one too low.
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

void dlgProfilePreferences::buildSearchResultsPage()
{
    mpScrollArea_searchResults = createScrollPage(qsl("searchResults"));
    QWidget* pColumn = mpScrollArea_searchResults->widget();
    mpLayout_searchResults = qobject_cast<QVBoxLayout*>(pColumn->layout());

    mpLabel_searchEmpty = new QLabel(pColumn);
    mpLabel_searchEmpty->setObjectName(qsl("settingsSearchEmpty"));
    mpLabel_searchEmpty->setAlignment(Qt::AlignCenter);
    mpLabel_searchEmpty->setWordWrap(true);
    mpLabel_searchEmpty->setTextFormat(Qt::RichText);
    mpLabel_searchEmpty->setOpenExternalLinks(true);
    mpLabel_searchEmpty->hide();
    // Between two stretches so the empty state lands in the middle of the
    // viewport; results are inserted between the label and the trailing stretch
    mpLayout_searchResults->addStretch(0);
    mpLayout_searchResults->addWidget(mpLabel_searchEmpty);
    mpLayout_searchResults->addStretch(1);
    mSearchResultsPageIndex = mpStackedWidget_categories->addWidget(mpScrollArea_searchResults);
}

// Collapsed, the sidebar shows a category as its icon alone. Emptying the item's
// text would do that too, but the text is what a screen reader announces the row
// as - so it is the drawing that leaves it out rather than the data.
namespace {
class SidebarItemDelegate : public QStyledItemDelegate
{
public:
    explicit SidebarItemDelegate(QListWidget* pList)
    : QStyledItemDelegate(pList)
    , mpList(pList)
    {
    }

    void initStyleOption(QStyleOptionViewItem* pOption, const QModelIndex& index) const override
    {
        QStyledItemDelegate::initStyleOption(pOption, index);
        if (!mpList->property("settingsRail").toBool()) {
            return;
        }
        pOption->text.clear();
        pOption->features &= ~QStyleOptionViewItem::HasDisplay;
        pOption->decorationAlignment = Qt::AlignCenter;
    }

private:
    QListWidget* mpList = nullptr;
};
} // namespace

QList<dlgProfilePreferences::CategoryDefinition> dlgProfilePreferences::categoryDefinitions() const
{
    return {//: Sidebar category in the settings dialog, holding saving, language, logging, web search and update options
            {scmCategory_general, qsl("settings-general.png"), tr("General")},
            //: Sidebar category in the settings dialog, holding the theme, icon sizes and profile tab options
            {scmCategory_appearance, qsl("settings-appearance.png"), tr("Appearance")},
            //: Sidebar category in the settings dialog, holding the font, colors, borders and wrapping of the game's text window
            {scmCategory_mainDisplay, qsl("settings-display.png"), tr("Main display")},
            //: Sidebar category in the settings dialog, holding the options of the command line the player types into
            {scmCategory_inputLine, qsl("settings-input.png"), tr("Input line")},
            //: Sidebar category in the settings dialog, holding the script editor's options
            {scmCategory_editor, qsl("settings-editor.png"), tr("Editor")},
            //: Sidebar category in the settings dialog, holding the map's files, view and colors
            {scmCategory_mapper, qsl("settings-mapper.png"), tr("Mapper")},
            //: Sidebar category in the settings dialog, holding the Discord Rich Presence and MudMaster chat options
            {scmCategory_chat, qsl("settings-chat.png"), tr("Chat and sharing")},
            //: Sidebar category in the settings dialog, holding the game protocol, encoding and compatibility options
            {scmCategory_connection, qsl("settings-connection.png"), tr("Connection"), true},
            //: Sidebar category in the settings dialog, holding the secure connection, proxy, password and permission options
            {scmCategory_privacy, qsl("settings-privacy.png"), tr("Privacy and security")},
            //: Sidebar category in the settings dialog, holding the screen reader and other accessibility options
            {scmCategory_accessibility, qsl("settings-accessibility.png"), tr("Accessibility")},
            //: Sidebar category in the settings dialog, holding the main window's keyboard shortcuts
            {scmCategory_shortcuts, qsl("settings-shortcuts.png"), tr("Shortcuts")},
            //: Sidebar category in the settings dialog, holding development and diagnostic options
            {scmCategory_advanced, qsl("settings-advanced.png"), tr("Advanced")}};
}

QWidget* dlgProfilePreferences::buildSidebar()
{
    auto* pSidebar = new QWidget(mpWidget_shell);
    mpWidget_sidebar = pSidebar;
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
    mpListWidget_categories->setItemDelegate(new SidebarItemDelegate(mpListWidget_categories));
    mpListWidget_categories->installEventFilter(this);
    pSidebarLayout->addWidget(mpListWidget_categories, 1);

    // The names come from retranslateShell()
    for (const auto& category : categoryDefinitions()) {
        if (category.separatorAbove) {
            addSidebarSeparator();
        }
        addCategory(category.key, category.iconFile);
    }

    addSidebarSeparator();
    mpItem_support = new QListWidgetItem(mpListWidget_categories);
    mpItem_support->setData(scmRole_externalUrl, qsl("https://wiki.mudlet.org"));
    // Clickable but not selectable: it opens a browser rather than a page
    mpItem_support->setFlags(Qt::ItemIsEnabled);
    mpItem_support->setSizeHint(QSize(0, 36));

    return pSidebar;
}

void dlgProfilePreferences::addCategory(const QString& key, const QString& iconFile)
{
    // No icon yet: the set is single-colour, so which colour is not known until
    // applyShellStyle() has read the theme off the palette
    auto* pItem = new QListWidgetItem(QString(), mpListWidget_categories);
    pItem->setData(scmRole_categoryKey, key);
    pItem->setSizeHint(QSize(0, 36));
    CategoryPlace& place = mCategories[key];
    place.row = mpListWidget_categories->row(pItem);
    place.iconFile = iconFile;
}

int dlgProfilePreferences::categoryRow(const QString& key) const
{
    return mCategories.value(key).row;
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
    //: Tooltip and accessible name of the chevron beside a settings subpage's breadcrumb, leading back to the category the subpage belongs to
    const QString backToCategory = tr("Back to the category this page belongs to");
    mpButton_subpageBack->setToolTip(backToCategory);
    mpButton_subpageBack->setAccessibleName(backToCategory);
    //: Sidebar link at the bottom of the settings dialog, opening the Mudlet wiki in a browser
    mpItem_support->setText(tr("Mudlet support"));

    for (const auto& category : categoryDefinitions()) {
        if (QListWidgetItem* pItem = mpListWidget_categories->item(categoryRow(category.key)); pItem) {
            pItem->setText(category.name);
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
    //: Card title on the Chat and sharing settings page, above the row leading to the Discord Rich Presence settings
    cardTitles.append({qsl("card_discord"), tr("Discord Rich Presence")});
    //: Card title on the game protocols subpage, above the ten protocols Mudlet can offer the game
    cardTitles.append({qsl("card_protocolList"), tr("Protocols to offer the game")});
    //: Card title on the Accessibility settings page, above the options for blank lines, blinking text and captions
    cardTitles.append({qsl("card_accessibilityText"), tr("Text and media")});
    //: Card title on the Accessibility settings page, above the options for moving around Mudlet from the keyboard
    cardTitles.append({qsl("card_accessibilityKeyboard"), tr("Keyboard")});
    for (const auto& [objectName, title] : cardTitles) {
        if (auto* pCard = findChild<QGroupBox*>(objectName); pCard) {
            pCard->setTitle(title);
        }
    }

    //: Breadcrumb name of the subpage holding the telnet protocols, reached from the Connection settings page
    mSubpageTitles.insert(qsl("connection/protocols"), tr("Game protocols"));
    //: Breadcrumb name of the subpage holding the Discord Rich Presence settings, reached from the Chat and sharing settings page
    mSubpageTitles.insert(qsl("chat/discord"), tr("Discord Rich Presence"));

    QList<std::tuple<QCheckBox*, QString, QString>> protocols;
    //: A telnet protocol on the game protocols subpage: its name, then one line of what it does for the player
    protocols.append({mEnableCHARSET, tr("CHARSET: Character Encoding Standard"), tr("Lets Mudlet and the game agree on how letters are spelled out, so accented and non-Latin text arrives intact.")});
    //: A telnet protocol on the game protocols subpage: its name, then one line of what it does for the player
    protocols.append({mEnableGMCP,
                      tr("GMCP: Generic Mud Communication Protocol"),
                      tr("Lets the game send your health, room and inventory as data, which is what most modern packages and user interfaces are built on.")});
    //: A telnet protocol on the game protocols subpage: its name, then one line of what it does for the player
    protocols.append({mEnableMNES, tr("MNES: Mud New-Environ Standard"), tr("Tells the game a short list of facts about Mudlet, such as its name and version.")});
    //: A telnet protocol on the game protocols subpage: its name, then one line of what it does for the player
    protocols.append({mEnableMSDP, tr("MSDP: Mud Server Data Protocol"), tr("An older way for the game to send data about your character, used where GMCP is not offered.")});
    //: A telnet protocol on the game protocols subpage: its name, then one line of what it does for the player
    protocols.append({mEnableMSP, tr("MSP: Mud Sound Protocol"), tr("Lets the game play sound effects and music through Mudlet.")});
    //: A telnet protocol on the game protocols subpage: its name, then one line of what it does for the player
    protocols.append({mEnableMSSP, tr("MSSP: Mud Server Status Protocol"), tr("Lets the game tell Mudlet about itself - how many players are on, what it is about - for game listings.")});
    //: A telnet protocol on the game protocols subpage: its name, then one line of what it does for the player
    protocols.append({mEnableMTTS,
                      tr("MTTS: Mud Terminal Type Standard"),
                      tr("Tells the game which client you are using and what it can display, so it can send colour and Unicode when Mudlet supports them.")});
    //: A telnet protocol on the game protocols subpage: its name, then one line of what it does for the player
    protocols.append({mEnableMXP, tr("MXP: Mud eXtension Protocol"), tr("Lets the game mark up its text with clickable links, commands and pop-up menus.")});
    //: A telnet protocol on the game protocols subpage: its name, then one line of what it does for the player
    protocols.append({mEnableNAWS, tr("NAWS: Negotiate About Window Size"), tr("Tells the game how wide your window is, so it can wrap its text to fit rather than guessing.")});
    //: A telnet protocol on the game protocols subpage: its name, then one line of what it does for the player
    protocols.append({mEnableNEWENVIRON, tr("NEW-ENVIRON: Client Variables Standard"), tr("Tells the game more about Mudlet than MNES does, including support for clickable links in plain text.")});
    for (const auto& [pCheckBox, name, description] : protocols) {
        if (!pCheckBox) {
            continue;
        }
        pCheckBox->setText(name);
        if (auto* pLabel = findChild<QLabel*>(qsl("%1_description").arg(pCheckBox->objectName())); pLabel) {
            pLabel->setText(description);
        }
    }
    //: Tooltip for MNES protocol option explaining mutual exclusivity with NEW-ENVIRON
    mEnableMNES->setToolTip(tr("MNES uses the same telnet option as NEW-ENVIRON, so only one can be active. MNES sends a minimal set of variables, while NEW-ENVIRON sends extended variables "
                               "including OSC link support."));
    //: Tooltip for NEW-ENVIRON protocol option explaining mutual exclusivity with MNES
    mEnableNEWENVIRON->setToolTip(
            tr("NEW-ENVIRON uses the same telnet option as MNES, so only one can be active. NEW-ENVIRON sends extended variables including OSC link support, while MNES sends a minimal set."));
    updateProtocolSummary();
    updateDiscordSummary();
    updateSecurityStatus();
    setCardDescriptions();

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

    // Nothing is current while the shell is being built, and the search's own
    // title comes back with the next query
    if (const QListWidgetItem* pCurrent = mpListWidget_categories->currentItem(); pCurrent && !mSearchActive) {
        if (mCurrentSubpage.isEmpty()) {
            mpLabel_pageTitle->setText(pCurrent->text());
        } else {
            mpLabel_pageTitle->setText(tr("%1 › %2").arg(pCurrent->text(), mSubpageTitles.value(mCurrentSubpage)));
        }
    }
}

// What a player types when they do not know what Mudlet calls a setting: folded
// into the text of the card the control sits on, and highlighting that control
void dlgProfilePreferences::setSearchKeywords()
{
    // The subpage this row leads to is indexed too, but a result there is a way
    // in rather than the setting itself. Not translated: these are the protocol
    // names as the games and their documentation spell them.
    pushButton_chooseProtocols->setProperty(scmProp_searchKeywords, qsl("GMCP MSDP MSSP MSP MXP MTTS MNES NAWS CHARSET NEW-ENVIRON telnet"));

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
    synonyms.append({mpCard_discord, tr("Discord, rich presence, status, what I am playing")});
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
        pControl->setProperty(scmProp_searchKeywords, words);
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

QScrollArea* dlgProfilePreferences::createScrollPage(const QString& objectSuffix)
{
    auto* pScrollArea = new QScrollArea(mpStackedWidget_categories);
    pScrollArea->setObjectName(qsl("settingsPage_%1").arg(objectSuffix));
    pScrollArea->setFrameShape(QFrame::NoFrame);
    pScrollArea->setWidgetResizable(true);
    markAsShellSurface(pScrollArea);

    auto* pColumn = new QWidget(pScrollArea);
    pColumn->setObjectName(qsl("settingsColumn_%1").arg(objectSuffix));
    auto* pColumnLayout = new QVBoxLayout(pColumn);
    pColumnLayout->setContentsMargins(0, 0, 0, 0);
    pColumnLayout->setSpacing(16);

    pScrollArea->setWidget(pColumn);
    // setWidget() turns the column into an opaque one filled from its own
    // palette, and the page background belongs to the content area behind it
    pColumn->setAutoFillBackground(false);
    pScrollArea->viewport()->setAutoFillBackground(false);
    markAsShellSurface(pColumn);
    markAsShellSurface(pScrollArea->viewport());
    return pScrollArea;
}

QScrollArea* dlgProfilePreferences::buildPage(const QString& objectSuffix, const QList<QWidget*>& cards)
{
    QScrollArea* pScrollArea = createScrollPage(objectSuffix);
    auto* pColumnLayout = qobject_cast<QVBoxLayout*>(pScrollArea->widget()->layout());
    for (auto* pCard : cards) {
        if (!pCard) {
            continue;
        }
        detachFromLayout(pCard);
        pCard->setProperty("settingsCard", true);
        // A checkable card's title starts after its check indicator, a plain
        // one's at the frame edge - twenty-odd pixels apart, which reads as the
        // titles of a page wandering. A property rather than a stylesheet rule,
        // because a stylesheet cannot ask whether a group box is checkable.
        auto* pGroupBox = qobject_cast<QGroupBox*>(pCard);
        pCard->setProperty("settingsCardTitleInset", pGroupBox && !pGroupBox->isCheckable());
        pColumnLayout->addWidget(pCard);
    }
    pColumnLayout->addStretch(1);
    capColumnWidth(pScrollArea);
    return pScrollArea;
}

void dlgProfilePreferences::buildCategoryPage(const QString& key, const QList<QWidget*>& cards)
{
    mCategories[key].pageIndex = mpStackedWidget_categories->addWidget(buildPage(key, cards));
}

// An ordinary page of the same stack with no sidebar row: the ways in are the
// card that opens it, a deep link naming "category/sub", and a search result.
void dlgProfilePreferences::addSubpage(const QString& categoryKey, const QString& subKey, QWidget* pOpenerCard, const QList<QWidget*>& cards)
{
    const QString key = qsl("%1/%2").arg(categoryKey, subKey);
    QScrollArea* pPage = buildPage(qsl("%1_%2").arg(categoryKey, subKey), cards);
    mSubpageIndexes.insert(key, mpStackedWidget_categories->addWidget(pPage));
    mSubpageOfPage.insert(pPage, key);
    mSubpageOpeners.insert(key, pOpenerCard);
}

QString dlgProfilePreferences::subpageHolding(const QWidget* pWidget) const
{
    for (const QWidget* pAncestor = pWidget; pAncestor; pAncestor = pAncestor->parentWidget()) {
        if (const auto it = mSubpageOfPage.constFind(pAncestor); it != mSubpageOfPage.constEnd()) {
            return *it;
        }
    }
    return {};
}

void dlgProfilePreferences::showSubpage(const QString& categoryKey, const QString& subKey, QWidget* pSpotlightTarget)
{
    const QString key = qsl("%1/%2").arg(categoryKey, subKey);
    if (!mSubpageIndexes.contains(key)) {
        // Every way in is written in C++, so a name leading nowhere is a typo
        qWarning() << "dlgProfilePreferences::showSubpage(...) WARNING - there is no settings subpage" << key << "- showing the category instead.";
        showCategory(categoryKey, pSpotlightTarget);
        return;
    }
    if (mSearchActive) {
        // Clearing the field is what sends every borrowed card home, and has to
        // finish before the stack is pointed anywhere else
        mSubpageBeforeSearch.clear();
        mpLineEdit_search->clear();
    }
    // Before the sidebar moves: the row-changed slot takes a sidebar move to
    // mean a category page is what is being shown
    mCurrentSubpage.clear();
    mpListWidget_categories->setCurrentRow(qMax(0, categoryRow(categoryKey)));
    mCurrentSubpage = key;

    auto* pPage = qobject_cast<QScrollArea*>(mpStackedWidget_categories->widget(mSubpageIndexes.value(key)));
    mpStackedWidget_categories->setCurrentWidget(pPage);
    capColumnWidth(pPage);
    // As on a category page, the cap above measured cards without their padding
    QTimer::singleShot(0, this, [this, pPage]() {
        if (pPage && mpStackedWidget_categories->currentWidget() == pPage) {
            capColumnWidth(pPage);
        }
    });

    mpLabel_pageTitleIcon->hide();
    mpButton_searchBack->hide();
    mpButton_subpageBack->show();
    mpLabel_pageTitle->setText(breadcrumbFor(key));
    spotlight(pSpotlightTarget);
}

// The title row is measured against this as well as shown it - see sidebarWidths()
QString dlgProfilePreferences::breadcrumbFor(const QString& subpageKey) const
{
    const QString categoryKey = subpageKey.section(QLatin1Char('/'), 0, 0);
    const QListWidgetItem* pItem = mpListWidget_categories->item(categoryRow(categoryKey));
    //: Breadcrumb over a settings subpage: %1 is the category it belongs to, %2 the subpage's own name
    return tr("%1 › %2").arg(pItem ? pItem->text() : categoryKey, mSubpageTitles.value(subpageKey));
}

void dlgProfilePreferences::leaveSubpage()
{
    if (mCurrentSubpage.isEmpty()) {
        return;
    }
    const int row = qMax(0, categoryRow(mCurrentSubpage.section(QLatin1Char('/'), 0, 0)));
    // The sidebar never left the parent category, so there is no row change to
    // carry the page back and the slot one would have run is called outright
    if (mpListWidget_categories->currentRow() == row) {
        slot_categorySelected(row);
        return;
    }
    mpListWidget_categories->setCurrentRow(row);
}

// Its title comes from retranslateShell(), by object name
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
        // A card reusing a .ui group box keeps that group box's grid
        if (auto* pGridLayout = qobject_cast<QGridLayout*>(pCard->layout()); pGridLayout) {
            pGridLayout->addWidget(pControl, pGridLayout->rowCount(), 0, 1, std::max(1, pGridLayout->columnCount()));
        } else {
            // A card with a different layout would take the control in but lay
            // it out nowhere, which shows up as a setting silently missing
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
    // A number field stretched to the card's width reads as a text field
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

// A grid has no notion of inserting a row, so every item is taken out and put
// back one row lower, carrying its row properties. The columns are untouched,
// which keeps a .ui file's column stretches meaning what they said.
static void insertGridRowAtTop(QGridLayout* pGrid, QWidget* pWidget)
{
    const int rows = pGrid->rowCount();
    const int columns = std::max(1, pGrid->columnCount());
    QList<std::pair<int, int>> rowProperties;
    rowProperties.reserve(rows);
    for (int row = 0; row < rows; ++row) {
        rowProperties.append({pGrid->rowStretch(row), pGrid->rowMinimumHeight(row)});
    }

    QList<std::tuple<QLayoutItem*, int, int, int, int>> items;
    items.reserve(pGrid->count());
    while (pGrid->count()) {
        int row = 0;
        int column = 0;
        int rowSpan = 1;
        int columnSpan = 1;
        pGrid->getItemPosition(0, &row, &column, &rowSpan, &columnSpan);
        items.append({pGrid->takeAt(0), row, column, rowSpan, columnSpan});
    }

    pGrid->addWidget(pWidget, 0, 0, 1, columns);
    for (const auto& [pItem, row, column, rowSpan, columnSpan] : items) {
        pGrid->addItem(pItem, row + 1, column, rowSpan, columnSpan, pItem->alignment());
    }
    pGrid->setRowStretch(0, 0);
    pGrid->setRowMinimumHeight(0, 0);
    for (int row = 0; row < rows; ++row) {
        pGrid->setRowStretch(row + 1, rowProperties.at(row).first);
        pGrid->setRowMinimumHeight(row + 1, rowProperties.at(row).second);
    }
}

// A row that leads somewhere rather than setting something; the chevron at its
// right edge is drawn by the shell stylesheet from the property this puts on.
static void makeChevronRow(QAbstractButton* pButton)
{
    pButton->setProperty("settingsChevronRow", true);
    pButton->setCursor(Qt::PointingHandCursor);
    pButton->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
}

// Created on the first call and only re-worded afterwards, so that a language
// change does not leave a page with two of them.
void dlgProfilePreferences::setCardDescription(QGroupBox* pCard, const QString& description, const QString& learnMoreUrl)
{
    if (!pCard) {
        return;
    }
    QLabel* pLabel = pCard->findChild<QLabel*>(qsl("settingsCardDescription"), Qt::FindDirectChildrenOnly);
    if (!pLabel) {
        pLabel = new QLabel(pCard);
        pLabel->setObjectName(qsl("settingsCardDescription"));
        pLabel->setWordWrap(true);
        pLabel->setTextFormat(Qt::RichText);
        pLabel->setOpenExternalLinks(true);
        pLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        if (auto* pGrid = qobject_cast<QGridLayout*>(pCard->layout()); pGrid) {
            insertGridRowAtTop(pGrid, pLabel);
        } else if (auto* pBox = qobject_cast<QBoxLayout*>(pCard->layout()); pBox) {
            if (pBox->direction() == QBoxLayout::LeftToRight || pBox->direction() == QBoxLayout::RightToLeft) {
                // A card laid out as one row has no above to put the line in
                auto* pRow = new QHBoxLayout();
                pRow->setSpacing(pBox->spacing());
                while (pBox->count()) {
                    pRow->addItem(pBox->takeAt(0));
                }
                pBox->setDirection(QBoxLayout::TopToBottom);
                pBox->addWidget(pLabel);
                pBox->addLayout(pRow);
            } else {
                pBox->insertWidget(0, pLabel);
            }
        } else {
            qWarning() << "dlgProfilePreferences::setCardDescription(...) WARNING - the card" << pCard->objectName() << "has no layout its description line can go into.";
            return;
        }
    }
    if (learnMoreUrl.isEmpty()) {
        pLabel->setText(description.toHtmlEscaped());
        return;
    }
    //: Link at the end of a settings card's description line, opening the Mudlet wiki page about that setting
    pLabel->setText(qsl("%1 <a href=\"%2\">%3</a>").arg(description.toHtmlEscaped(), learnMoreUrl, tr("Learn more").toHtmlEscaped()));
}

// Called from retranslateShell(), so a language change re-words them
void dlgProfilePreferences::setCardDescriptions()
{
    //: Description line under the "System integration" card title on the General settings page
    setCardDescription(findChild<QGroupBox*>(qsl("card_systemIntegration")), tr("How Mudlet fits in with the rest of your desktop."));
    //: Description line under the "Web search" card title on the General settings page
    setCardDescription(groupbox_searchEngineSelection, tr("The site Mudlet opens when you pick \"search on the web\" after selecting some text in the game."));
    //: Description line under the "Scrollback" card title on the Main display settings page
    setCardDescription(groupBox_consoleBuffer, tr("How much of what the game has already sent stays available to scroll back through."));
    //: Description line under the "Scripting" card title on the Editor settings page
    setCardDescription(groupBox_autoComplete, tr("What the script editor offers while you write Lua, and where mistakes in it are reported."), qsl("https://wiki.mudlet.org/w/Manual:Scripting"));
    //: Description line under the "Download map" card title on the Mapper settings page
    setCardDescription(
            groupBox_downloadMapOptions, tr("Some games publish a ready-made map that Mudlet can fetch for you instead of you walking it yourself."), qsl("https://wiki.mudlet.org/w/Manual:Mapper"));
    //: Description line under the "Discord Rich Presence" card title on the Chat and sharing settings page
    setCardDescription(
            mpCard_discord, tr("Shows what you are playing on your Discord profile, and decides how much of it other people get to see."), qsl("https://wiki.mudlet.org/w/Standards:Discord_GMCP"));
    //: Description line under the "MMCP" card title on the Chat and sharing settings page
    setCardDescription(groupBox_MMCPOptions, tr("Chat directly with other players' clients, without the messages going through the game."));
    //: Description line under the "Game protocols" card title on the Connection settings page
    setCardDescription(groupBox_protocols,
                       tr("Extras Mudlet offers the game beyond plain text - sound, map data, your window size and the like. The game decides which of them it uses."),
                       qsl("https://wiki.mudlet.org/w/Manual:Supported_Protocols"));
    //: Description line under the "Data encoding" card title on the Connection settings page
    setCardDescription(findChild<QGroupBox*>(qsl("card_dataEncoding")),
                       tr("How the bytes the game sends are turned into letters. Use what the game's own documentation asks for."),
                       qsl("https://wiki.mudlet.org/w/Manual:Unicode"));
    //: Description line under the "Compatibility" card title on the Connection settings page
    setCardDescription(groupBox_specialOptions, tr("Workarounds for games whose servers do things their own way. Leave these off unless the game asks you to turn one on."));
    //: Description line under the "Network" card title on the Connection settings page
    setCardDescription(findChild<QGroupBox*>(qsl("card_network")), tr("How long Mudlet waits for the rest of a slow message before drawing what it already has."));
    //: Description line under the "Secure connection" card title on the Privacy and security settings page
    setCardDescription(groupBox_ssl, tr("Encrypts everything travelling between Mudlet and the game, so nobody in between can read it. The game has to offer a secure port of its own."));
    //: Description line under the "Proxy" card title on the Privacy and security settings page
    setCardDescription(groupBox_proxy, tr("Sends Mudlet's traffic through another server first - needed on networks that block games directly."));
    //: Description line under the "Passwords" card title on the Privacy and security settings page
    setCardDescription(findChild<QGroupBox*>(qsl("card_passwords")), tr("Where Mudlet keeps the passwords you have let it remember for you."));
    //: Description line under the "Server permissions" card title on the Privacy and security settings page
    setCardDescription(findChild<QGroupBox*>(qsl("card_serverPermissions")), tr("What the game is allowed to put on your screen or play through your speakers without asking first."));
    //: Description line under the "Media cache" card title on the Privacy and security settings page
    setCardDescription(groupBox_purgeMediaCache,
                       tr("Sounds and music the game sends are kept on disk so they only have to be downloaded once."),
                       qsl("https://wiki.mudlet.org/w/Standards:MUD_Client_Media_Protocol"));
    //: Description line under the "Crash reports" card title on the Privacy and security settings page
    setCardDescription(findChild<QGroupBox*>(qsl("card_crashReports")),
                       tr("If Mudlet stops unexpectedly it can tell the developers what went wrong. A report says where Mudlet was in its own code - never what you typed or what the game sent."));
    //: Description line under the "Developer" card title on the Advanced settings page
    setCardDescription(groupBox_debug, tr("Diagnostics for people writing packages and scripts. Leave these off for ordinary play."));
}

void dlgProfilePreferences::buildProtocolsSubpage()
{
    mpCard_protocolList = createCard(qsl("card_protocolList"));
    auto* pCardLayout = qobject_cast<QVBoxLayout*>(mpCard_protocolList->layout());
    pCardLayout->setSpacing(4);

    const auto addProtocol = [this, pCardLayout](const QString& objectName) {
        auto* pCheckBox = new QCheckBox(mpCard_protocolList);
        pCheckBox->setObjectName(objectName);
        pCardLayout->addWidget(pCheckBox);
        auto* pDescription = new QLabel(mpCard_protocolList);
        pDescription->setObjectName(qsl("%1_description").arg(objectName));
        pDescription->setProperty("settingsControlDescription", true);
        pDescription->setWordWrap(true);
        pCardLayout->addWidget(pDescription);
        return pCheckBox;
    };

    mEnableCHARSET = addProtocol(qsl("checkBox_enableCHARSET"));
    mEnableGMCP = addProtocol(qsl("checkBox_enableGMCP"));
    mEnableMNES = addProtocol(qsl("checkBox_enableMNES"));
    mEnableMSDP = addProtocol(qsl("checkBox_enableMSDP"));
    mEnableMSP = addProtocol(qsl("checkBox_enableMSP"));
    mEnableMSSP = addProtocol(qsl("checkBox_enableMSSP"));
    mEnableMTTS = addProtocol(qsl("checkBox_enableMTTS"));
    mEnableMXP = addProtocol(qsl("checkBox_enableMXP"));
    mEnableNAWS = addProtocol(qsl("checkBox_enableNAWS"));
    mEnableNEWENVIRON = addProtocol(qsl("checkBox_enableNEWENVIRON"));

    // The warning belongs on the page the change is made on
    moveIntoCard(mpCard_protocolList, {need_reconnect_for_data_protocol});

    // Made once here rather than on each initWithHost(), which would stack a
    // second copy of each every time a profile came and went
    for (auto* pCheckBox : {mEnableCHARSET.data(),
                            mEnableGMCP.data(),
                            mEnableMNES.data(),
                            mEnableMSDP.data(),
                            mEnableMSP.data(),
                            mEnableMSSP.data(),
                            mEnableMTTS.data(),
                            mEnableMXP.data(),
                            mEnableNAWS.data(),
                            mEnableNEWENVIRON.data()}) {
        connect(pCheckBox, &QAbstractButton::toggled, this, [this]() {
            // Reading a profile's settings into the controls is not a change
            // anyone has to reconnect for:
            if (!mPopulating) {
                need_reconnect_for_data_protocol->show();
            }
            updateProtocolSummary();
        });
    }
    connect(mEnableGMCP, &QAbstractButton::toggled, pushButton_forgetSavedSignIn, &QWidget::setEnabled);
    connect(mEnableMNES, &QAbstractButton::toggled, this, [this](const bool checked) {
        if (!mPopulating && checked && mEnableNEWENVIRON->isChecked()) {
            mEnableNEWENVIRON->setChecked(false);
        }
    });
    connect(mEnableNEWENVIRON, &QAbstractButton::toggled, this, [this](const bool checked) {
        if (!mPopulating && checked && mEnableMNES->isChecked()) {
            mEnableMNES->setChecked(false);
        }
    });

    // The button keeps its object name, its tab stop and its place in the four
    // host enable/disable lists: all that changes is where it leads
    pushButton_chooseProtocols->setMenu(nullptr);
    makeChevronRow(pushButton_chooseProtocols);
    connect(pushButton_chooseProtocols, &QAbstractButton::clicked, this, [this]() {
        showSubpage(scmCategory_connection, qsl("protocols"));
    });
}

void dlgProfilePreferences::updateProtocolSummary()
{
    int enabled = 0;
    int total = 0;
    for (const auto& pCheckBox : {mEnableCHARSET, mEnableGMCP, mEnableMNES, mEnableMSDP, mEnableMSP, mEnableMSSP, mEnableMTTS, mEnableMXP, mEnableNAWS, mEnableNEWENVIRON}) {
        if (!pCheckBox) {
            continue;
        }
        ++total;
        if (pCheckBox->isChecked()) {
            ++enabled;
        }
    }
    // Written as two numbers rather than as a plural form: an untranslated
    // %n string still shows its "(s)" in English, and this row is too
    // prominent to read as "9 protocol(s) on"
    //: Text of the row on the Connection page's game protocols card that opens the list of protocols; %1 is how many are switched on, %2 how many there are
    pushButton_chooseProtocols->setText(tr("%1 of %2 turned on").arg(QString::number(enabled), QString::number(total)));
}

void dlgProfilePreferences::buildDiscordSummaryCard()
{
    mpCard_discord = createCard(qsl("card_discord"));
    mpButton_discordSubpage = new QPushButton(mpCard_discord);
    mpButton_discordSubpage->setObjectName(qsl("pushButton_discordSettings"));
    makeChevronRow(mpButton_discordSubpage);
    qobject_cast<QVBoxLayout*>(mpCard_discord->layout())->addWidget(mpButton_discordSubpage);
    connect(mpButton_discordSubpage, &QAbstractButton::clicked, this, [this]() {
        showSubpage(scmCategory_chat, qsl("discord"));
    });
    for (auto* pRadioButton : {radioButton_discordDisabled, radioButton_discordMudletOnly, radioButton_discordGameDetails}) {
        connect(pRadioButton, &QAbstractButton::toggled, this, &dlgProfilePreferences::updateDiscordSummary);
    }
}

void dlgProfilePreferences::updateDiscordSummary()
{
    if (!mpButton_discordSubpage) {
        return;
    }
    QString state;
    if (radioButton_discordDisabled->isChecked()) {
        //: Summary on the Chat and sharing page's Discord card, on the row that opens the Discord settings
        state = tr("Off - Discord is told nothing");
    } else if (radioButton_discordMudletOnly->isChecked()) {
        //: Summary on the Chat and sharing page's Discord card, on the row that opens the Discord settings
        state = tr("On - Discord is told you are using Mudlet");
    } else {
        //: Summary on the Chat and sharing page's Discord card, on the row that opens the Discord settings
        state = tr("On - Discord is told which game you are playing");
    }
    mpButton_discordSubpage->setText(state);
}

void dlgProfilePreferences::buildSecurityStatusCard()
{
    mpCard_securityStatus = createCard(qsl("card_securityStatus"));
    mpCard_securityStatus->setProperty("settingsHero", true);
    // It carries no setting, so it needs no title and no room above the frame
    // for one either
    mpCard_securityStatus->setProperty("settingsCardPlain", true);
    auto* pLayout = qobject_cast<QVBoxLayout*>(mpCard_securityStatus->layout());

    mpLabel_securityHeadline = new QLabel(mpCard_securityStatus);
    mpLabel_securityHeadline->setObjectName(qsl("settingsHeroHeadline"));
    mpLabel_securityHeadline->setWordWrap(true);
    pLayout->addWidget(mpLabel_securityHeadline);

    mpLabel_securityDetail = new QLabel(mpCard_securityStatus);
    mpLabel_securityDetail->setObjectName(qsl("settingsHeroDetail"));
    mpLabel_securityDetail->setWordWrap(true);
    pLayout->addWidget(mpLabel_securityDetail);

    mpLabel_securityLink = new QLabel(mpCard_securityStatus);
    mpLabel_securityLink->setObjectName(qsl("settingsHeroLink"));
    mpLabel_securityLink->setTextFormat(Qt::RichText);
    // The hero holds no setting of its own, so the link leads to the card that does
    connect(mpLabel_securityLink, &QLabel::linkActivated, this, [this]() {
        showCategory(scmCategory_privacy, groupBox_ssl);
    });
    pLayout->addWidget(mpLabel_securityLink);
}

void dlgProfilePreferences::updateSecurityStatus()
{
    if (!mpCard_securityStatus) {
        return;
    }
    Host* pHost = mpHost;
    // With no profile loaded there is no connection to report on
    mpCard_securityStatus->setVisible(pHost != nullptr);
    if (!pHost) {
        return;
    }

    const bool connected = pHost->mTelnet.getConnectionState() == QAbstractSocket::ConnectedState;
    QString headline;
    QString detail;
    if (!connected) {
        //: Headline of the security status card on the Privacy and security settings page, when the profile is not connected to its game
        headline = tr("Not connected");
        //: Detail line of the security status card on the Privacy and security settings page, when the profile is not connected to its game
        detail = tr("Connect to the game to see whether this connection is encrypted.");
    } else if (pHost->mTelnet.currentlySecure()) {
        //: Headline of the security status card on the Privacy and security settings page, when the connection to the game is encrypted; %1 is the game's address
        headline = tr("Your connection to %1 is encrypted").arg(pHost->getUrl());
#if !defined(QT_NO_SSL)
        if (const QSslCertificate certificate = pHost->mTelnet.getPeerCertificate(); !certificate.isNull()) {
            const QString issuer = certificate.issuerInfo(QSslCertificate::CommonName).join(qsl(", "));
            const QString expiry = certificate.expiryDate().toString(mudlet::self()->getUserLocale().dateFormat(QLocale::ShortFormat));
            //: Detail line of the security status card on the Privacy and security settings page; %1 is who issued the game's certificate, %2 the date it stops being valid
            detail = tr("The game's certificate was issued by %1 and is valid until %2.").arg(issuer.isEmpty() ? tr("an unnamed authority") : issuer, expiry);
        }
#endif
        if (detail.isEmpty()) {
            //: Detail line of the security status card on the Privacy and security settings page, when the connection is encrypted but the game presented no certificate details
            detail = tr("Nobody between you and the game can read what you send.");
        }
    } else {
        //: Headline of the security status card on the Privacy and security settings page, when the connection to the game is not encrypted; %1 is the game's address
        headline = tr("Your connection to %1 is not encrypted").arg(pHost->getUrl());
        //: Detail line of the security status card on the Privacy and security settings page, when the connection is not encrypted
        detail = tr("Everything you send, your password included, travels in the clear. Games that offer a secure port let you turn this around below.");
    }
    mpLabel_securityHeadline->setText(headline);
    mpLabel_securityDetail->setText(detail);
    //: Link on the security status card of the Privacy and security settings page, leading to the "Secure connection" card below it
    mpLabel_securityLink->setText(qsl("<a href=\"#secureConnection\">%1</a>").arg(tr("Secure connection settings").toHtmlEscaped()));
}

// The .ui file titles these group boxes after the tab they sat on, and
// retranslateUi() puts those titles back on every language change
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
    //: Card title on the Accessibility settings page, above the two options about what the system screen reader is told
    groupBox_accessibility->setTitle(tr("Screen reader"));
}

// A grid cell takes one widget, so a card is emptied before it is refilled
static void takeOutOfLayout(QLayout* pLayout, const QList<QWidget*>& widgets)
{
    for (auto* pWidget : widgets) {
        pLayout->removeWidget(pWidget);
    }
}

// The .ui file laid these grids out across the full width of a tab, in rows of
// three and four cells that do not fit the reading column the cards live in now
void dlgProfilePreferences::reflowWideCards()
{
    takeOutOfLayout(horizontalLayout_groupBox_wrapping, {frame_wrap_at, frame_indent_wrapped, frame_hanging_indent_wrapped});
    int wrappingRow = 0;
    for (auto* pFrame : {frame_wrap_at, frame_indent_wrapped, frame_hanging_indent_wrapped}) {
        // As wide as the card now, so without a trailing stretch the spin box grows to fill it
        qobject_cast<QHBoxLayout*>(pFrame->layout())->addStretch(1);
        verticalLayout_groupBox_wrapping->insertWidget(wrappingRow++, pFrame);
    }

    const QList<QWidget*> mapViewOptions{
            mMapperUseAntiAlias, checkBox_drawUpperLowerLevels, checkbox_mMapperShowRoomBorders, checkBox_invertMapZoom, checkBox_largeAreaExitArrows, checkBox_showDefaultArea};
    takeOutOfLayout(gridLayout_groupBox_mapViewOptions, mapViewOptions + QList<QWidget*>{gridGroupBox, groupBox_mapSymbols});
    for (int i = 0, total = mapViewOptions.size(); i < total; ++i) {
        gridLayout_groupBox_mapViewOptions->addWidget(mapViewOptions.at(i), i / 2, i % 2);
    }
    gridLayout_groupBox_mapViewOptions->addWidget(gridGroupBox, 3, 0, 1, 2);
    gridLayout_groupBox_mapViewOptions->addWidget(groupBox_mapSymbols, 4, 0, 1, 2);

    // A column of three, leaving the two cells initWithHost() appends the
    // scaling factor to free
    const QList<QWidget*> mapSymbolRows{label_mapSymbolsFont, fontComboBox_mapSymbols, pushButton_showGlyphUsage, checkBox_isOnlyMapSymbolFontToBeUsed};
    takeOutOfLayout(gridLayout_groupBox_mapSymbols, mapSymbolRows);
    gridLayout_groupBox_mapSymbols->addWidget(label_mapSymbolsFont, 0, 0);
    gridLayout_groupBox_mapSymbols->addWidget(fontComboBox_mapSymbols, 0, 1);
    gridLayout_groupBox_mapSymbols->addWidget(checkBox_isOnlyMapSymbolFontToBeUsed, 1, 0, 1, 2);
    gridLayout_groupBox_mapSymbols->addWidget(pushButton_showGlyphUsage, 2, 0, 1, 1, Qt::AlignLeft);

    const QList<QWidget*> featureSizes{label_roomSize, spinBox_roomSize, label_exitSize, spinBox_exitSize, label_borderSize, spinBox_borderSize, label_gridSize, doubleSpinBox_gridSize};
    takeOutOfLayout(groupBox_sizing, featureSizes);
    // Two pairs to a row, the second starting halfway across, so they line up
    // with the two columns of checkboxes above instead of bunching to the left
    for (int i = 0, total = featureSizes.size(); i < total; ++i) {
        const int pair = i / 2;
        groupBox_sizing->addWidget(featureSizes.at(i), pair / 2, (pair % 2) * 3 + (i % 2));
    }
    // The spin boxes ask to expand, and across a card's width that stretches
    // four of them into text fields; the empty column ending each half takes
    // the slack instead, and equal shares of it put the halves side by side
    groupBox_sizing->setColumnStretch(2, 1);
    groupBox_sizing->setColumnStretch(5, 1);

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
    takeOutOfLayout(gridLayout_groupBox_discordRichPresence, discordRows);
    // Stacked rather than between two columns, so the divider lies across them
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

    // Framed like an input in the .ui file, but it only ever reports a name -
    // and with no Discord user to report, an empty box on the card
    label_data_discordCurrentUser->setFrameShape(QFrame::NoFrame);

    // Side by side these two filled the reading column in English and overran
    // it by 200px in German
    const QList<QWidget*> logOptionRows{mIsToLogInHtml,
                                        mIsLoggingTimestamps,
                                        label_whereToLog,
                                        lineEdit_logFileFolder,
                                        pushButton_whereToLog,
                                        pushButton_resetLogDir,
                                        label_logFileNameFormat,
                                        comboBox_logFileNameFormat,
                                        label_logFileName,
                                        lineEdit_logFileName,
                                        label_logFileNameExtension};
    takeOutOfLayout(gridLayout_groupBox_logOptions, logOptionRows);
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

    // Two label-and-control pairs abreast, whose German labels are half again
    // as long as the English ones, become four rows
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

    // Alone on its row now, and adrift on the right unless it spans both columns
    gridLayout_groupBox_debug->removeWidget(checkBox_expectCSpaceIdInColonLessMColorCode);
    gridLayout_groupBox_debug->addWidget(checkBox_expectCSpaceIdInColonLessMColorCode, 0, 0, 1, 2);
    // The time edit had the row's stretch, which put a four-field clock control
    // across the whole card
    horizontalLayout_timerDebugOutputMinimumInterval->setStretch(1, 0);
    horizontalLayout_timerDebugOutputMinimumInterval->addStretch(1);

    // Both report into a line of runtime text - a profile name, a file path -
    // longer than any column; wrapped, they stop being a floor under its width
    label_mapFileActionResult->setWordWrap(true);
    label_password_migration_notification->setWordWrap(true);
}

// Three of this card's checkboxes have gone to the pages they belong on and the
// two double-click controls have arrived stacked, which between them left the
// .ui grid holding lone widgets in half-empty rows. Laid out again as a pair of
// checkboxes, then a row for each label-and-control.
void dlgProfilePreferences::reflowDisplayOptionsCard()
{
    const QList<QWidget*> displayOptions{checkBox_useWideAmbiguousEastAsianGlyphs,
                                         checkBox_enableTextAnalyzer,
                                         checkBox_enableOSC8Hyperlinks,
                                         label_controlCharacterHandling,
                                         comboBox_controlCharacterHandling,
                                         doubleclick_ignore_label,
                                         doubleclick_ignore_lineedit};
    takeOutOfLayout(gridLayout_groupBox_displayOptions, displayOptions);
    // Both are about which characters the display makes something of
    gridLayout_groupBox_displayOptions->addWidget(checkBox_useWideAmbiguousEastAsianGlyphs, 0, 0);
    gridLayout_groupBox_displayOptions->addWidget(checkBox_enableTextAnalyzer, 0, 1);
    gridLayout_groupBox_displayOptions->addWidget(checkBox_enableOSC8Hyperlinks, 1, 0, 1, 2);
    gridLayout_groupBox_displayOptions->addWidget(label_controlCharacterHandling, 2, 0);
    gridLayout_groupBox_displayOptions->addWidget(comboBox_controlCharacterHandling, 2, 1);
    // Beside its field rather than over it, which is a row saved and puts the
    // two settings that take an answer on matching lines
    gridLayout_groupBox_displayOptions->addWidget(doubleclick_ignore_label, 3, 0);
    gridLayout_groupBox_displayOptions->addWidget(doubleclick_ignore_lineedit, 3, 1);
}

// The reconnect notice closes this card in the .ui, but the workaround moved
// onto this page lands after it, which strands the notice between two
// checkboxes now that it stays on screen long enough to be read.
void dlgProfilePreferences::reflowCompatibilityCard()
{
    takeOutOfLayout(gridLayout_groupBox_specialOptions, {checkBox_USE_IRE_DRIVER_BUGFIX, need_reconnect_for_specialoption});
    gridLayout_groupBox_specialOptions->addWidget(checkBox_USE_IRE_DRIVER_BUGFIX, 2, 1);
    gridLayout_groupBox_specialOptions->addWidget(need_reconnect_for_specialoption, 3, 0, 1, 2);
}

void dlgProfilePreferences::updateColumnWidthCaps()
{
    for (const auto& place : std::as_const(mCategories)) {
        capColumnWidth(qobject_cast<QScrollArea*>(mpStackedWidget_categories->widget(place.pageIndex)));
    }
    // ...and the pages the sidebar never selects, which hold cards like any other
    for (const int page : std::as_const(mSubpageIndexes)) {
        capColumnWidth(qobject_cast<QScrollArea*>(mpStackedWidget_categories->widget(page)));
    }
    capColumnWidth(mpScrollArea_searchResults);
}

// Measured rather than a number in the source: an interface font, a platform's
// scrollbar or a translation's longer category names all move it
dlgProfilePreferences::SidebarWidths dlgProfilePreferences::sidebarWidths() const
{
    SidebarWidths widths;
    QWidget* pContent = mpWidget_titleRow ? mpWidget_titleRow->parentWidget() : nullptr;
    // Zero collapses nothing, which is the right answer for a half-built shell
    if (!pContent || !pContent->layout() || !mpLabel_pageTitle || !mpStackedWidget_categories) {
        return widths;
    }
    const QMargins contentMargins = pContent->layout()->contentsMargins();

    const auto* pPage = qobject_cast<const QScrollArea*>(mpStackedWidget_categories->currentWidget());
    const int scrollBarWidth = pPage ? pPage->verticalScrollBar()->sizeHint().width() : 0;

    // Measured off the row itself - icon, spacings, margins - with the title it
    // happens to be showing swapped for the widest one it could be asked for.
    // Its chevrons are hidden most of the time and a hidden widget asks a layout
    // for nothing, so the wider of the two is allowed for rather than read off.
    const QFontMetrics titleMetrics = mpLabel_pageTitle->fontMetrics();
    const int chevron = std::max(mpButton_searchBack->sizeHint().width(), mpButton_subpageBack->sizeHint().width()) + mpWidget_titleRow->layout()->spacing();
    const int titleRowChrome = mpWidget_titleRow->sizeHint().width() - titleMetrics.horizontalAdvance(mpLabel_pageTitle->text()) + chevron;
    int widestTitle = 0;
    for (int row = 0, rows = mpListWidget_categories->count(); row < rows; ++row) {
        widestTitle = std::max(widestTitle, titleMetrics.horizontalAdvance(mpListWidget_categories->item(row)->text()));
    }
    for (auto it = mSubpageIndexes.constBegin(); it != mSubpageIndexes.constEnd(); ++it) {
        widestTitle = std::max(widestTitle, titleMetrics.horizontalAdvance(breadcrumbFor(it.key())));
    }

    // The reading column is a floor, not the answer: a page whose controls do
    // not fit it is capped wider (capColumnWidth()). Taken over every page
    // rather than the one on show, because a width moving with the category
    // would resize the window on every click in the sidebar.
    int contentColumn = scmContentColumnWidth;
    int widestPageMinimum = 0;
    for (int page = 0, pages = mpStackedWidget_categories->count(); page < pages; ++page) {
        const auto* pStacked = qobject_cast<const QScrollArea*>(mpStackedWidget_categories->widget(page));
        const QWidget* pColumn = pStacked ? pStacked->widget() : nullptr;
        if (!pColumn) {
            continue;
        }
        // An uncapped page still offers the default maximum, which nothing asked for
        if (pColumn->maximumWidth() < QWIDGETSIZE_MAX) {
            contentColumn = std::max(contentColumn, pColumn->maximumWidth());
        }
        widestPageMinimum = std::max(widestPageMinimum, pColumn->minimumSizeHint().width());
    }

    const int sidebarAndMargins = scmSidebarWidth + contentMargins.left() + contentMargins.right();
    widths.fullyExpanded = sidebarAndMargins + std::max(contentColumn + scrollBarWidth, titleRowChrome + widestTitle);
    // Deliberately not the same number as the width above: held equal, the
    // sidebar had its names at exactly one window width and the first pixel of
    // a drag inwards took them away, with no way back short of hitting that
    // width again. It is instead the narrowest the widest page can be drawn at
    // without scrolling sideways, and never more than the reading column - so a
    // page that wants more than a comfortable line, or a re-measure after a
    // change of language, moves what the window can grow to without moving the
    // point at which the sidebar gives up its names.
    widths.collapseBelow = sidebarAndMargins + std::min(scmContentColumnWidth, widestPageMinimum) + scrollBarWidth;
    return widths;
}

void dlgProfilePreferences::updateSidebarMode()
{
    if (!mpWidget_sidebar) {
        return;
    }
    // Zero is neither a breakpoint to test against nor a width to hold a window to
    const SidebarWidths widths = sidebarWidths();
    if (!widths.fullyExpanded) {
        return;
    }
    // The window's width rather than the space left over: the threshold is what
    // the *expanded* sidebar needs, so collapsing cannot flip the test that
    // collapsed it and start it oscillating
    setSidebarCollapsed(width() < widths.collapseBelow);
    // Nothing on a page grows past its column, so every pixel of window past
    // fullyExpanded is empty strip, all of it on one side as a layout puts a
    // widget narrower than its cell to the left. Refusing the width is what
    // keeps it out: centring would strand the sidebar away from the settings it
    // selects, and stretching the controls is the line length the column stops.
    setMaximumWidth(std::max(minimumWidth(), widths.fullyExpanded));
}

void dlgProfilePreferences::setSidebarCollapsed(const bool collapsed)
{
    if (collapsed == mSidebarCollapsed && mpWidget_sidebar->width() == (collapsed ? scmSidebarRailWidth : scmSidebarWidth)) {
        return;
    }
    mSidebarCollapsed = collapsed;
    const int padding = collapsed ? scmSidebarRailPadding : scmSidebarPadding;
    mpWidget_sidebar->setFixedWidth(collapsed ? scmSidebarRailWidth : scmSidebarWidth);
    mpWidget_sidebar->layout()->setContentsMargins(padding, 16, padding, 16);
    mpLabel_wordmark->setVisible(!collapsed);
    // What the delegate leaves the names out by, and the stylesheet draws the
    // narrower selection pill from
    mpListWidget_categories->setProperty("settingsRail", collapsed);
    mpListWidget_categories->style()->unpolish(mpListWidget_categories);
    mpListWidget_categories->style()->polish(mpListWidget_categories);
    for (auto* pSeparator : mpListWidget_categories->findChildren<QFrame*>(qsl("settingsSidebarSeparator"))) {
        pSeparator->setProperty("settingsRail", collapsed);
        pSeparator->style()->unpolish(pSeparator);
        pSeparator->style()->polish(pSeparator);
    }
    // A name no longer drawn is still what the row is: the item keeps its text -
    // its accessible name - and offers it as a tooltip while nothing shows it
    for (int row = 0, rows = mpListWidget_categories->count(); row < rows; ++row) {
        QListWidgetItem* pItem = mpListWidget_categories->item(row);
        pItem->setToolTip(collapsed ? pItem->text() : QString());
    }
    mpListWidget_categories->doItemsLayout();
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
// came from. Only the widgets of the page on show take part in a traversal, so
// one chain in sidebar order puts that right.
void dlgProfilePreferences::rebuildTabOrder()
{
    // A hidden chevron is skipped by a traversal rather than trapping it
    QList<QWidget*> chain{mpLineEdit_search, mpButton_searchBack, mpButton_subpageBack, mpListWidget_categories};
    const auto collectPage = [&chain, this](const int pageIndex) {
        auto* pScrollArea = qobject_cast<QScrollArea*>(mpStackedWidget_categories->widget(pageIndex));
        QWidget* pColumn = pScrollArea ? pScrollArea->widget() : nullptr;
        if (pColumn && pColumn->layout()) {
            collectFocusableInLayoutOrder(pColumn->layout(), chain);
        }
    };
    for (int row = 0, rows = mpListWidget_categories->count(); row < rows; ++row) {
        const QString key = mpListWidget_categories->item(row)->data(scmRole_categoryKey).toString();
        collectPage(mCategories.value(key).pageIndex);
        // ...and each subpage right after the category it belongs to
        const QString prefix = key + QLatin1Char('/');
        for (auto it = mSubpageIndexes.constBegin(); it != mSubpageIndexes.constEnd(); ++it) {
            if (it.key().startsWith(prefix)) {
                collectPage(it.value());
            }
        }
    }
    for (int i = 1, total = chain.size(); i < total; ++i) {
        setTabOrder(chain.at(i - 1), chain.at(i));
    }
}

// Instant apply turns a wheel passing over a spin box or a combo box on the way
// down a page into a silent change of that setting. Both stop answering the
// wheel unless focused, and stop taking focus from a wheel - see eventFilter().
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

// The one piece of shell state no preference decides: the sidebar is a rail
// whenever the window is too narrow to hold it, and a list of names when not.
void dlgProfilePreferences::resizeEvent(QResizeEvent* pEvent)
{
    QDialog::resizeEvent(pEvent);
    updateSidebarMode();
}

// Coming back to the window is the moment the settings can be re-read without
// interrupting anything - see refreshFromSettings()
bool dlgProfilePreferences::event(QEvent* pEvent)
{
    const bool handled = QDialog::event(pEvent);
    if (pEvent->type() == QEvent::WindowActivate) {
        refreshFromSettings();
    }
    return handled;
}

bool dlgProfilePreferences::eventFilter(QObject* pObject, QEvent* pEvent)
{
    // A checkbox whose text was wrapped into a QLabel beside it keeps the whole
    // of that label as its click target, as its own text would have been
    if (pEvent->type() == QEvent::EnabledChange) {
        if (auto* pCheckBox = qobject_cast<QCheckBox*>(pObject); pCheckBox) {
            if (QLabel* pLabel = wrapLabelOf(pCheckBox); pLabel) {
                pLabel->setEnabled(pCheckBox->isEnabled());
            }
        }
    }
    if (pEvent->type() == QEvent::MouseButtonRelease) {
        if (auto* pLabel = qobject_cast<QLabel*>(pObject); pLabel && pLabel->objectName() == qsl("settingsWrappedLabel")) {
            auto* pMouseEvent = static_cast<QMouseEvent*>(pEvent);
            QCheckBox* pCheckBox = pLabel->parentWidget()->findChild<QCheckBox*>(QString(), Qt::FindDirectChildrenOnly);
            if (pCheckBox && pCheckBox->isEnabled() && pMouseEvent->button() == Qt::LeftButton && pLabel->rect().contains(pMouseEvent->position().toPoint())) {
                pCheckBox->click();
                return true;
            }
        }
    }
    if (pEvent->type() == QEvent::Wheel) {
        auto* pControl = qobject_cast<QWidget*>(pObject);
        if (pControl && !pControl->hasFocus()) {
            // QApplication carries an unaccepted wheel event up the parent
            // chain only while nothing has *handled* it, and a filter answering
            // true has - so without this the wheel would do nothing at all
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
    // them; a property puts it back, since a QSS rule cannot ask whether the
    // widget a subcontrol belongs to has the focus
    if (pObject == mpListWidget_categories && (pEvent->type() == QEvent::FocusIn || pEvent->type() == QEvent::FocusOut)) {
        mpListWidget_categories->setProperty("settingsFocused", pEvent->type() == QEvent::FocusIn);
        mpListWidget_categories->style()->polish(mpListWidget_categories);
    }
    return QDialog::eventFilter(pObject, pEvent);
}

void dlgProfilePreferences::showCategory(const QString& key, QWidget* pSpotlightTarget)
{
    // Leaving the results is carried out by the selection *changing*, and a deep
    // link naming the category the sidebar is already on changes nothing
    if (mSearchActive) {
        mSubpageBeforeSearch.clear();
        mpLineEdit_search->clear();
    }
    QString category = key;
    if (!mCategories.contains(category)) {
        // Every deep link is written in C++, so one naming nothing is a typo
        qWarning() << "dlgProfilePreferences::showCategory(...) WARNING - there is no settings category" << key << "- showing General instead.";
        category = scmCategory_general;
    }
    mpListWidget_categories->setCurrentRow(categoryRow(category));
    spotlight(pSpotlightTarget);
}

static QString spotlightStyleSheet(const QColor& accent, const qreal strength)
{
    return qsl("#settingsSpotlight { border: 2px solid rgba(%1, %2, %3, %4); border-radius: 8px; background-color: rgba(%1, %2, %3, %5); }")
            .arg(QString::number(accent.red()), QString::number(accent.green()), QString::number(accent.blue()), QString::number(strength, 'f', 3), QString::number(strength * 0.08, 'f', 3));
}

// Deferred because setTab() runs before the dialog is shown, when nothing has
// been laid out yet
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
        // The y margin is the viewport to keep around the widget, so half of one
        // is as close to centred as the page allows
        pScrollArea->ensureWidgetVisible(pTarget, 0, pScrollArea->viewport()->height() / 2);

        // A second deep link supersedes the first rather than fading on top of it
        delete mpWidget_spotlight.data();
        const QColor accent = palette().color(QPalette::Highlight);
        auto* pPulse = new QWidget(pColumn);
        mpWidget_spotlight = pPulse;
        pPulse->setObjectName(qsl("settingsSpotlight"));
        pPulse->setAttribute(Qt::WA_TransparentForMouseEvents);
        pPulse->setAttribute(Qt::WA_StyledBackground);
        // A card fills the column's width, so an outline drawn around one would
        // be clipped on three sides - the intersection lands them on the card
        const QRect targetRect(pTarget->mapTo(pColumn, QPoint(0, 0)), pTarget->size());
        pPulse->setGeometry(targetRect.adjusted(-2, -2, 2, 2).intersected(pColumn->rect()));
        // The animation only ever takes the pulse away, and its first tick can be
        // a long time coming on a busy event loop - which is when a deep link opens
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

// Rich text, the & of keyboard accelerators, accents and case are all folded
// away, so that "fonte" finds "Fonté" and "save" finds "&Save"
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

// A combo box is not here: what it shows is one of its items, and the two
// callers want its whole list or nothing at all
static QString visibleTextOf(const QWidget* pWidget)
{
    if (const auto* pLabel = qobject_cast<const QLabel*>(pWidget); pLabel) {
        return pLabel->text();
    }
    if (const auto* pGroupBox = qobject_cast<const QGroupBox*>(pWidget); pGroupBox) {
        return pGroupBox->title();
    }
    if (const auto* pButton = qobject_cast<const QAbstractButton*>(pWidget); pButton) {
        return pButton->text();
    }
    return QString();
}

// What it shows, what its tooltip says, and any synonyms it was given
static void collectSearchText(const QWidget* pWidget, QStringList& parts)
{
    parts << pWidget->property(scmProp_searchKeywords).toString() << pWidget->toolTip();
    const auto* pComboBox = qobject_cast<const QComboBox*>(pWidget);
    if (!pComboBox) {
        parts << visibleTextOf(pWidget);
        return;
    }
    // ...but not what a font picker lists: those are the fonts installed on this
    // machine, and they make any card holding one a result for "color" or "mono"
    if (qobject_cast<const QFontComboBox*>(pWidget)) {
        return;
    }
    for (int i = 0, total = pComboBox->count(); i < total; ++i) {
        parts << pComboBox->itemText(i);
    }
}

// Synonyms count too: a card found by a keyword still shows which control carries it
static QString highlightTextOf(const QWidget* pWidget)
{
    const QString text = visibleTextOf(pWidget);
    if (text.isEmpty()) {
        return QString();
    }
    const QString keywords = pWidget->property(scmProp_searchKeywords).toString();
    return keywords.isEmpty() ? text : qsl("%1 %2").arg(text, keywords);
}

// Walked off the real widget tree rather than kept as a hand-written list, so
// a control added to the .ui file later is searchable without anyone saying so
void dlgProfilePreferences::buildSearchIndex()
{
    mSearchCards.clear();
    // A category's own cards are indexed before its subpages', so a query
    // matching both meets the card that leads in first
    const auto indexPage = [this](const int pageIndex, const QString& categoryKey, const QString& subpageKey) {
        auto* pScrollArea = qobject_cast<QScrollArea*>(mpStackedWidget_categories->widget(pageIndex));
        auto* pColumnLayout = pScrollArea ? qobject_cast<QVBoxLayout*>(pScrollArea->widget()->layout()) : nullptr;
        if (!pColumnLayout) {
            return;
        }
        for (int item = 0, items = pColumnLayout->count(); item < items; ++item) {
            // The banner is off every page while this runs - see placeBannerOn()
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
            entry.categoryKey = categoryKey;
            entry.subpageKey = subpageKey;
            entry.text = foldForSearch(parts.join(QLatin1Char(' ')));
            entry.pHomeLayout = pColumnLayout;
            entry.homeIndex = item;
            mSearchCards.append(entry);
        }
    };

    for (int row = 0, rows = mpListWidget_categories->count(); row < rows; ++row) {
        const QString key = mpListWidget_categories->item(row)->data(scmRole_categoryKey).toString();
        if (key.isEmpty()) {
            continue;
        }
        indexPage(mCategories.value(key).pageIndex, key, QString());
        const QString prefix = key + QLatin1Char('/');
        for (auto it = mSubpageIndexes.constBegin(); it != mSubpageIndexes.constEnd(); ++it) {
            if (it.key().startsWith(prefix)) {
                indexPage(it.value(), key, it.key());
            }
        }
    }
}

// A search is answered by moving every matching card onto the results page and
// every card already there back, which is most of the dialog while the query is
// still one or two letters: measured on this page, "s" costs 63ms and "sh" 76ms
// against the 6ms the finished "shortcuts" costs. Typing is a burst, so only the
// pause at the end of one is worth answering.
void dlgProfilePreferences::queueSearch(const QString& query)
{
    mPendingSearch = query;
    // Emptying the field is what hands the borrowed cards back, and a dialog
    // that keeps them for another moment reads as one that has stuck
    if (query.trimmed().isEmpty()) {
        mpTimer_search->stop();
        runSearch(query);
        return;
    }
    mpTimer_search->start();
}

// One ideograph is a word where one Latin letter is not, so it is a query worth
// running; a lone letter matches most of the dialog and answers nothing.
static bool wordEnoughToSearch(const QStringList& needles)
{
    for (const QString& needle : needles) {
        if (needle.size() >= 2) {
            return true;
        }
        switch (needle.at(0).script()) {
        case QChar::Script_Han:
        case QChar::Script_Hiragana:
        case QChar::Script_Katakana:
        case QChar::Script_Hangul:
            return true;
        default:
            break;
        }
    }
    return false;
}

void dlgProfilePreferences::runSearch(const QString& query)
{
    const QStringList needles = foldForSearch(query).split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (needles.isEmpty() || !wordEnoughToSearch(needles)) {
        exitSearchMode();
        return;
    }
    // Before the index is built, as a banner shifts every card on its page
    placeBannerOn(nullptr);
    if (mSearchCards.isEmpty()) {
        buildSearchIndex();
    }
    if (!mSearchActive) {
        mSearchActive = true;
        const QListWidgetItem* pCurrent = mpListWidget_categories->currentItem();
        mCategoryBeforeSearch = pCurrent ? pCurrent->data(scmRole_categoryKey).toString() : QString();
        // A search started on a subpage comes back to that subpage
        mSubpageBeforeSearch = mCurrentSubpage;
        mCurrentSubpage.clear();
        mpButton_subpageBack->hide();
        // Only the selection goes, never the current row: an item view answers a
        // focus-in that finds no current index by taking the first one, which
        // reads as choosing General and ends the search. That focus arrives
        // whenever a borrowed card is handed back holding it, as reparenting a
        // widget clears the focus it carries.
        const QSignalBlocker blocker(mpListWidget_categories);
        mpListWidget_categories->clearSelection();
    }

    setUpdatesEnabled(false);
    // Every keystroke starts from the pages as they really are
    returnSearchedCardsHome();
    clearSearchHighlights();

    // Moving a card fires no change signal, but the control that had the focus
    // reports its editing finished as focus leaves it. Lowered by hand, because
    // the tail below is deliberately outside the guard.
    const bool wasPopulating = mPopulating;
    mPopulating = true;

    QString lastCategory;
    int matchCount = 0;
    QList<const QWidget*> matchedCards;
    QStringList linkedSubpages;
    for (auto& entry : mSearchCards) {
        // A card the profile's state has hidden is no option, so no result either
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
        // Taking a subpage's card would leave the page behind the row that opens
        // it empty, so the results offer that way in instead
        if (!entry.subpageKey.isEmpty()) {
            if (linkedSubpages.contains(entry.subpageKey) || matchedCards.contains(mSubpageOpeners.value(entry.subpageKey).data())) {
                continue;
            }
            linkedSubpages.append(entry.subpageKey);
            if (entry.categoryKey != lastCategory) {
                lastCategory = entry.categoryKey;
                QLabel* pHeader = searchCategoryHeader(entry.categoryKey);
                mpLayout_searchResults->insertWidget(mpLayout_searchResults->count() - 1, pHeader);
                pHeader->show();
            }
            QPushButton* pLink = searchSubpageLink(entry.subpageKey, entry.pCard);
            mpLayout_searchResults->insertWidget(mpLayout_searchResults->count() - 1, pLink);
            pLink->show();
            ++matchCount;
            continue;
        }
        matchedCards.append(entry.pCard);

        if (entry.categoryKey != lastCategory) {
            lastCategory = entry.categoryKey;
            QLabel* pHeader = searchCategoryHeader(entry.categoryKey);
            mpLayout_searchResults->insertWidget(mpLayout_searchResults->count() - 1, pHeader);
            pHeader->show();
        }
        // Not that it is still at homeIndex - taking an earlier card shifts the
        // ones below - but that this is still the layout it goes back to
        Q_ASSERT_X(entry.pHomeLayout->indexOf(entry.pCard) >= 0, "dlgProfilePreferences::runSearch", "a card is no longer in the layout the search index recorded it under");
        entry.pHomeLayout->removeWidget(entry.pCard);
        mpLayout_searchResults->insertWidget(mpLayout_searchResults->count() - 1, entry.pCard);
        // Reparenting hides a widget, and the results page may not be showing yet
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
        const QString help = tr("Need help? Visit %1").arg(qsl("<a href=\"%1\">%2</a>").arg(mpItem_support->data(scmRole_externalUrl).toString(), mpItem_support->text()));
        mpLabel_searchEmpty->setText(qsl("%1<br>%2").arg(message, help));
    }
    mpLabel_searchEmpty->setVisible(!matchCount);
    mpLayout_searchResults->setStretch(0, matchCount ? 0 : 1);
    //: Title shown in place of a category name while the settings search is showing its results
    mpLabel_pageTitle->setText(tr("Search results"));
    // The back chevron stands where the category icon does on every other page
    mpLabel_pageTitleIcon->hide();
    mpButton_searchBack->show();
    mpStackedWidget_categories->setCurrentIndex(mSearchResultsPageIndex);
    // As on a category page, a card needing more than the reading width gets it:
    capColumnWidth(mpScrollArea_searchResults);
    setUpdatesEnabled(true);
}

// The index is in ascending position order per page and the cards that never
// left kept theirs, so re-inserting at the indexed position restores the page
void dlgProfilePreferences::returnSearchedCardsHome()
{
    const QScopedValueRollback<bool> populating(mPopulating, true);
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
    for (auto* pLink : std::as_const(mSearchSubpageLinks)) {
        mpLayout_searchResults->removeWidget(pLink);
        pLink->hide();
    }
}

// The field is cleared before the index, because clearing it is what sends
// every borrowed card home and that needs the index to say where home is
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
        // The query was cleared rather than a category chosen, so the page the
        // search interrupted comes back
        mpListWidget_categories->setCurrentRow(qMax(0, categoryRow(mCategoryBeforeSearch)));
    } else {
        // Only the selection was taken away, so the row is still current and
        // setCurrentRow() would report no change
        mpListWidget_categories->item(row)->setSelected(true);
        slot_categorySelected(row);
    }
    // ...and for a subpage, its category page is only half the way back
    if (const QString subpage = mSubpageBeforeSearch; !subpage.isEmpty()) {
        mSubpageBeforeSearch.clear();
        showSubpage(subpage.section(QLatin1Char('/'), 0, 0), subpage.section(QLatin1Char('/'), 1));
    }
    setUpdatesEnabled(true);
}

// A stylesheet rule selecting on a property only takes effect on a re-polish
static void setSearchMatch(QWidget* pWidget, const QVariant& matched)
{
    pWidget->setProperty("searchMatch", matched);
    pWidget->style()->unpolish(pWidget);
    pWidget->style()->polish(pWidget);
    pWidget->update();
}

void dlgProfilePreferences::clearSearchHighlights()
{
    for (const auto& pWidget : std::as_const(mHighlightedWidgets)) {
        if (!pWidget) {
            continue;
        }
        setSearchMatch(pWidget, QVariant());
    }
    mHighlightedWidgets.clear();
}

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
        setSearchMatch(pWidget, true);
        mHighlightedWidgets.append(pWidget);
    }
}

QLabel* dlgProfilePreferences::searchCategoryHeader(const QString& key)
{
    QLabel* pHeader = mSearchCategoryHeaders.value(key, nullptr);
    if (!pHeader) {
        pHeader = new QLabel(mpScrollArea_searchResults->widget());
        pHeader->setObjectName(qsl("settingsSearchHeader"));
        // Rich text is the only way one label draws a picture beside a word
        pHeader->setTextFormat(Qt::RichText);
        pHeader->hide();
        mSearchCategoryHeaders.insert(key, pHeader);
    }
    // Every time, as a language change replaces the name this map is holding
    const QListWidgetItem* pItem = mpListWidget_categories->item(categoryRow(key));
    const QString name = (pItem ? pItem->text() : key).toHtmlEscaped();
    // The sidebar row's own icon, inline rather than by path because the colour
    // is put on at runtime - see restyleSidebarIcons()
    const QString iconMarkup = mCategoryIconMarkup.value(key);
    pHeader->setText(iconMarkup.isEmpty() ? name : qsl("%1&nbsp;%2").arg(iconMarkup, name));
    return pHeader;
}

// Not the card - taking that empties the page behind the row that opens it -
// but the way in, landing on the subpage with the card it found outlined
QPushButton* dlgProfilePreferences::searchSubpageLink(const QString& subpageKey, QWidget* pCard)
{
    QPushButton* pLink = mSearchSubpageLinks.value(subpageKey, nullptr);
    if (!pLink) {
        pLink = new QPushButton(mpScrollArea_searchResults->widget());
        pLink->setObjectName(qsl("settingsSearchSubpageResult"));
        makeChevronRow(pLink);
        pLink->hide();
        mSearchSubpageLinks.insert(subpageKey, pLink);
    }
    // Re-worded every time, for the language change
    pLink->setText(mSubpageTitles.value(subpageKey));
    // ...and re-aimed, as which card matched is this query's answer
    disconnect(pLink, &QAbstractButton::clicked, this, nullptr);
    connect(pLink, &QAbstractButton::clicked, this, [this, subpageKey, pCard = QPointer<QWidget>(pCard)]() {
        showSubpage(subpageKey.section(QLatin1Char('/'), 0, 0), subpageKey.section(QLatin1Char('/'), 1), pCard);
    });
    return pLink;
}

void dlgProfilePreferences::slot_categorySelected(const int row)
{
    QListWidgetItem* pItem = mpListWidget_categories->item(row);
    if (!pItem) {
        return;
    }
    const QString key = pItem->data(scmRole_categoryKey).toString();
    if (key.isEmpty()) {
        return;
    }
    if (mSearchActive) {
        // Clearing the field is what puts every borrowed card back, and whatever
        // the query interrupted, this category is the one being asked for now
        mSubpageBeforeSearch.clear();
        mpLineEdit_search->clear();
    }
    // What comes out of this slot is a category page, so a subpage is being left
    mCurrentSubpage.clear();
    mpButton_subpageBack->hide();
    // QStackedLayout hands the keyboard focus from the outgoing page to the
    // incoming one, and taking it off a control scrolled out of sight scrolls
    // that page back to the top on the way past
    QWidget* pCurrentPage = mpStackedWidget_categories->currentWidget();
    if (QWidget* pFocus = QApplication::focusWidget(); pFocus && pCurrentPage && pCurrentPage->isAncestorOf(pFocus)) {
        mpListWidget_categories->setFocus(Qt::OtherFocusReason);
    }
    mpStackedWidget_categories->setCurrentIndex(mCategories.contains(key) ? mCategories.value(key).pageIndex : mCategories.value(scmCategory_general).pageIndex);
    auto* pShownPage = qobject_cast<QScrollArea*>(mpStackedWidget_categories->currentWidget());
    // Before the width is capped, so the page is measured as it will be shown
    placeBannerOn(pShownPage ? pShownPage->widget() : nullptr);
    capColumnWidth(pShownPage);
    // A card's padding arrives with the stylesheet, applied as the page is first
    // shown - after the cap above measured it without. Left there, a page needing
    // more than the reading width is capped 34px short and clips the difference.
    QTimer::singleShot(0, this, [this, pShownPage]() {
        if (pShownPage && mpStackedWidget_categories->currentWidget() == pShownPage) {
            capColumnWidth(pShownPage);
            // A cap that grew moved both the sidebar's breakpoint and the width
            // the window is held to, neither re-taken until the next resize
            updateSidebarMode();
        }
    });
    mpLabel_pageTitle->setText(pItem->text());
    mpLabel_pageTitleIcon->setPixmap(pItem->icon().pixmap(QSize(20, 20), devicePixelRatioF()));
    mpLabel_pageTitleIcon->show();

    if (key == scmCategory_editor && !mEditorThemesChecked) {
        mEditorThemesChecked = true;
        maybeDownloadEditorThemes();
    }
}

void dlgProfilePreferences::slot_sidebarItemClicked(QListWidgetItem* pItem)
{
    if (!pItem) {
        return;
    }
    if (const QString url = pItem->data(scmRole_externalUrl).toString(); !url.isEmpty()) {
        QDesktopServices::openUrl(QUrl(url));
        return;
    }
    // Choosing the category a subpage belongs to is no row change, so the
    // row-changed slot never runs and the subpage would stay put
    if (const QString key = pItem->data(scmRole_categoryKey).toString(); !key.isEmpty() && mCurrentSubpage.startsWith(key + QLatin1Char('/'))) {
        leaveSubpage();
    }
}

// Every surface is blended from the palette rather than written out as hex, so
// the shell follows whichever theme it is handed
static QColor blend(const QColor& from, const QColor& to, const qreal amount)
{
    return QColor::fromRgbF(from.redF() + (to.redF() - from.redF()) * amount, from.greenF() + (to.greenF() - from.greenF()) * amount, from.blueF() + (to.blueF() - from.blueF()) * amount);
}

static QString rgba(const QColor& color, const qreal alpha)
{
    return qsl("rgba(%1, %2, %3, %4)").arg(QString::number(color.red()), QString::number(color.green()), QString::number(color.blue()), QString::number(alpha, 'f', 3));
}

// The shape lives in the alpha channel: filling through it keeps the
// antialiased edges that recolouring the pixels would harden into a staircase
static QPixmap tintedGlyph(const QPixmap& source, const QColor& color)
{
    QPixmap glyph = source;
    QPainter painter(&glyph);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(glyph.rect(), color);
    painter.end();
    return glyph;
}

// Measured off a throwaway pair rather than added up from the indicator's
// width, because what a style leaves after an indicator is the style's business:
// Fusion allows 6px and the macOS style 8.
static int measuredCardTitleInset(QWidget* pParent, const QString& indicatorRules)
{
    const auto titleLeft = [&](const bool checkable) {
        QGroupBox box(pParent);
        box.setProperty("settingsCard", true);
        box.setCheckable(checkable);
        // Never shown or read, but a box with no title has no label to place
        box.setTitle(qsl("Aa"));
        // Its own rather than the shell's, which is the string being built
        box.setStyleSheet(indicatorRules);
        QStyleOptionGroupBox option;
        option.initFrom(&box);
        option.subControls = QStyle::SC_GroupBoxFrame | QStyle::SC_GroupBoxLabel;
        if (checkable) {
            option.subControls |= QStyle::SC_GroupBoxCheckBox;
            option.state |= QStyle::State_On;
        }
        option.text = box.title();
        option.textAlignment = Qt::AlignLeft;
        option.lineWidth = 0;
        option.midLineWidth = 0;
        return box.style()->subControlRect(QStyle::CC_GroupBox, &option, QStyle::SC_GroupBoxLabel, &box).left();
    };
    return qMax(0, titleLeft(true) - titleLeft(false));
}

// A QLabel's rich text reaches a picture only through a URL, and a glyph tinted
// at runtime has no path - so it travels inline
static QString inlineGlyph(const QPixmap& glyph)
{
    QByteArray png;
    QBuffer buffer(&png);
    buffer.open(QIODevice::WriteOnly);
    glyph.save(&buffer, "PNG");
    return qsl(R"(<img src="data:image/png;base64,%1" width="18" height="18">)").arg(QString::fromLatin1(png.toBase64()));
}

// Called from applyShellStyle() alone, which is both where the colours come
// from and the one thing an appearance change runs again
void dlgProfilePreferences::restyleSidebarIcons(const QColor& normal, const QColor& selected)
{
    if (!mpListWidget_categories) {
        return;
    }
    mCategoryIconMarkup.clear();
    for (auto it = mCategories.cbegin(), end = mCategories.cend(); it != end; ++it) {
        QListWidgetItem* pItem = mpListWidget_categories->item(it.value().row);
        if (!pItem || it.value().iconFile.isEmpty()) {
            continue;
        }
        const QPixmap source(qsl(":/icons/%1").arg(it.value().iconFile));
        const QPixmap quiet = tintedGlyph(source, normal);
        QIcon icon(quiet);
        // Otherwise the view makes one by washing the icon in the highlight colour
        icon.addPixmap(tintedGlyph(source, selected), QIcon::Selected);
        pItem->setIcon(icon);
        mCategoryIconMarkup.insert(it.key(), inlineGlyph(quiet));
    }
    if (mpItem_support) {
        mpItem_support->setIcon(QIcon(tintedGlyph(QPixmap(qsl(":/icons/settings-support.png")), normal)));
    }
    // A header built under the previous theme holds that theme's picture, and
    // re-wording it is what puts the new one in
    const QStringList builtHeaders = mSearchCategoryHeaders.keys();
    for (const QString& key : builtHeaders) {
        searchCategoryHeader(key);
    }
    // The title over the page took a copy of the row's icon when the page was
    // shown, which the first time round is before this has run at all. Hidden
    // means a subpage or the search results, neither of which shows one.
    if (mpLabel_pageTitleIcon && !mpLabel_pageTitleIcon->isHidden()) {
        if (const QListWidgetItem* pCurrent = mpListWidget_categories->currentItem()) {
            mpLabel_pageTitleIcon->setPixmap(pCurrent->icon().pixmap(QSize(20, 20), devicePixelRatioF()));
        }
    }
}

// Kept on the shell widget rather than on the dialog, whose stylesheet
// mudlet::showOptionsDialog() assigns the profile's Lua one to on every show.
// Every selector is scoped by objectName or by the card property, so a profile
// stylesheet still reaches the controls inside the pages.
void dlgProfilePreferences::applyShellStyle()
{
    if (!mpWidget_shell) {
        return;
    }
    // The application's palette rather than this dialog's own: a stylesheet
    // freezes the palette of the widget it is assigned to, so the profile's Lua
    // stylesheet leaves the dialog holding the theme it was shown in - and even
    // without one, the palette change is an event still undelivered when
    // signal_appearanceChanged() arrives. qApp's palette is swapped
    // synchronously by mudlet::setAppearance(), so it is already the new one.
    const QPalette themePalette = QApplication::palette();
    const QColor cardColor = themePalette.color(QPalette::Base);
    const QColor textColor = themePalette.color(QPalette::WindowText);
    const QColor accentColor = themePalette.color(QPalette::Highlight);
    // Off the palette rather than mudlet::inDarkMode(), so a dark system theme
    // under "follow the system" gets the dark treatment too
    const bool darkPage = cardColor.lightness() < 128;

    // Measured from the card and text colours, the one pair a palette must keep
    // apart to be usable at all: Mudlet's light appearance has window, base and
    // mid within three levels, so a border mixed from those is invisible
    const QColor pageColor = blend(cardColor, QColor(Qt::black), darkPage ? 0.35 : 0.04);
    const QColor borderColor = blend(cardColor, textColor, darkPage ? 0.28 : 0.16);
    const QString hoverSoft = rgba(textColor, 0.07);
    const QString accentSoft = rgba(accentColor, 0.14);
    // A saturated highlight colour rarely holds its own against both pages
    const QColor accentText = darkPage ? blend(accentColor, QColor(Qt::white), 0.45) : blend(accentColor, QColor(Qt::black), 0.2);
    const QColor mutedText = blend(cardColor, textColor, 0.7);
    // A marker pen whose lightness is chosen for the page it lies on: an opaque
    // pale wash under dark text, a darker one light text still shows through
    const QColor markerColor = QColor::fromHslF(0.13, 0.9, darkPage ? 0.34 : 0.72);
    const QString markerSoft = rgba(markerColor, darkPage ? 0.75 : 0.95);
    const QColor scrollHandle = blend(pageColor, textColor, 0.22);
    const QColor scrollHandleHover = blend(pageColor, textColor, 0.40);

    if (mpAction_searchIcon) {
        mpAction_searchIcon->setIcon(QIcon(tintedGlyph(QPixmap(qsl(":/icons/settings-search.png")), mutedText)));
    }
    // Quieter than the name beside them, and the accent under a selected one
    restyleSidebarIcons(mutedText, accentText);

    // A border-left accent bar is drawn as an arc where the pill's corner radius
    // is, pinching the bar to nothing at both ends. A gradient is clipped by the
    // radius instead, so the bar keeps its width and the pill's rounded corners.
    const qreal accentBarStop = static_cast<qreal>(scmSidebarAccentBarWidth) / (scmSidebarWidth - 2 * scmSidebarPadding);
    // ...and the same bar on a collapsed sidebar's narrower item is a different
    // fraction of a different width
    const qreal railAccentBarStop = static_cast<qreal>(scmSidebarAccentBarWidth) / (scmSidebarRailWidth - 2 * scmSidebarRailPadding);

    // Fusion draws a group box's check indicator from palette(window) darkened
    // by 40%, which on a dark card is a 1.1:1 outline - and the palette pass
    // below cannot rescue it, as that role also carries the card's title band.
    // A styled indicator gets no check mark of its own, so the checked state
    // has to be drawn out in full.
    const QColor indicatorOutline = blend(cardColor, textColor, darkPage ? 0.55 : 0.45);
    const QString cardIndicatorRules = qsl("QGroupBox[settingsCard=\"true\"]::indicator { width: %1px; height: %1px; border: 1px solid %2; border-radius: 3px; background-color: %3; }"
                                           "QGroupBox[settingsCard=\"true\"]::indicator:hover { border: 1px solid %4; }"
                                           // A fixed green check mark, so the fill under it is the
                                           // card rather than an accent that could be orange
                                           "QGroupBox[settingsCard=\"true\"]::indicator:checked { border: 1px solid %4; image: url(:/icons/dialog-ok-apply_small.png); }")
                                               .arg(QString::number(scmCardIndicatorSize), indicatorOutline.name(), cardColor.name(), accentColor.name());
    // Where a checkable card's title starts is only known once the rules above
    // are the ones being laid out under
    const QString cardTitleRule = qsl("QGroupBox[settingsCardTitleInset=\"true\"]::title { left: %1px; }").arg(QString::number(measuredCardTitleInset(mpWidget_shell, cardIndicatorRules)));

    mpWidget_shell->setStyleSheet(qsl("#settingsShell, #settingsSidebar, #settingsContent { background-color: %1; }"
                                      // Or the platform style draws its own selection as a square
                                      // box inside the rounded pill the rules below draw
                                      "#settingsCategoryList { background: transparent; border: none; outline: none; show-decoration-selected: 1;"
                                      " selection-background-color: transparent; selection-color: %6; }"
                                      // The transparent left border keeps a selected item's text
                                      // from stepping sideways under its accent bar; outline:none
                                      // drops a focus rectangle drawn square inside a round pill
                                      "#settingsCategoryList::item { border-radius: 8px; border-left: 3px solid transparent; padding-left: 7px; color: %2; outline: none; }"
                                      "#settingsCategoryList::item:hover { background-color: %3; }"
                                      "#settingsCategoryList::item:selected { color: %6; font-weight: bold; background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
                                      " stop:0 %5, stop:%11 %5, stop:%12 %4, stop:1 %4); }"
                                      // Keyboard focus is otherwise indistinguishable from the
                                      // selection; eventFilter() puts the property on
                                      "#settingsCategoryList[settingsFocused=\"true\"]::item:selected { border: 1px solid %5; border-left: 3px solid %5; padding-left: 5px; }"
                                      // On a rail the item is only as wide as its icon, so the
                                      // padding goes and the bar is a different fraction
                                      "#settingsCategoryList[settingsRail=\"true\"]::item { padding-left: 0px; }"
                                      "#settingsCategoryList[settingsRail=\"true\"]::item:selected { background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
                                      " stop:0 %5, stop:%15 %5, stop:%16 %4, stop:1 %4); }"
                                      "#settingsSidebarSeparator { border: none; background-color: %7; margin: 8px 16px; }"
                                      "#settingsSidebarSeparator[settingsRail=\"true\"] { margin: 8px 2px; }"
                                      "#settingsStack { background: transparent; }"
                                      // The shell's own surfaces keep the page colour even when a
                                      // profile stylesheet paints every QWidget it can reach.
                                      // Painted rather than left transparent, as a transparent
                                      // surface falls back to the palette that stylesheet changed.
                                      "QWidget[settingsSurface=\"true\"] { background-color: %1; border: none; }"
                                      // A scroll area's bars answer only to a descendant selector
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
                                      // The top margin lifts the title clear of the frame, rather
                                      // than leaving it cutting through the card's border
                                      "QGroupBox[settingsCard=\"true\"] { background-color: %8; border: 1px solid %7; border-radius: 8px; margin-top: 24px; padding: 16px; font-weight: bold; }"
                                      "QGroupBox[settingsCard=\"true\"]::title { subcontrol-origin: margin; subcontrol-position: top left; left: 0px; padding: 0px; }"
                                      // ...but only the title is bold, not everything the card holds:
                                      "QGroupBox[settingsCard=\"true\"] > * { font-weight: normal; }"
                                      // A card carrying a single option needs no heading, nor room above for one
                                      "QGroupBox[settingsCardPlain=\"true\"] { margin-top: 0px; }"
                                      // A group box the .ui file nests inside what is now a card
                                      // would draw a second frame; a heading alone divides them
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
                                      // Drawn as a piece of the heading it sits beside, not a button
                                      "#settingsSearchBack, #settingsSubpageBack { border: 1px solid transparent; border-radius: 6px; padding: 2px 6px; color: %2; background: transparent; }"
                                      "#settingsSearchBack:hover, #settingsSubpageBack:hover { background-color: %3; }"
                                      "#settingsSearchBack:focus, #settingsSubpageBack:focus { border: 1px solid %5; }"
                                      // Quieter than what they describe, and indented under it
                                      "#settingsCardDescription { color: %9; }"
                                      "QLabel[settingsControlDescription=\"true\"] { color: %9; margin-left: 20px; margin-bottom: 6px; }"
                                      // The wrap holder shows the card through it, named outright
                                      // so a profile stylesheet cannot paint a band across it
                                      "#settingsCheckBoxWrap { background: transparent; border: none; }"
                                      // A row that leads somewhere, clickable across the card's width
                                      "QAbstractButton[settingsChevronRow=\"true\"] { text-align: left; padding: 8px 30px 8px 10px; border: 1px solid %7; border-radius: 6px;"
                                      // Qt's stylesheets cannot scale a background image, so this
                                      // is the 16px copy of the icon rather than the 48px one
                                      " background-color: transparent; background-image: url(:/icons/arrow-right_grey-16x.png); background-repeat: no-repeat;"
                                      " background-position: right center; background-origin: padding; }"
                                      "QAbstractButton[settingsChevronRow=\"true\"]:hover { background-color: %3; }"
                                      "QAbstractButton[settingsChevronRow=\"true\"]:focus { border: 1px solid %5; }"
                                      // Carries no setting, so tinted rather than framed like a card
                                      "QGroupBox[settingsHero=\"true\"] { background-color: %4; border: 1px solid %5; }"
                                      "#settingsHeroHeadline { font-weight: bold; font-size: 115%; }"
                                      "#settingsHeroDetail { color: %9; }")
                                          .arg(pageColor.name(), textColor.name(), hoverSoft, accentSoft, accentColor.name(), accentText.name(), borderColor.name(), cardColor.name(), mutedText.name())
                                          .arg(markerSoft, QString::number(accentBarStop, 'f', 5), QString::number(accentBarStop + 0.0001, 'f', 5), scrollHandle.name(), scrollHandleHover.name())
                                          .arg(QString::number(railAccentBarStop, 'f', 5), QString::number(railAccentBarStop + 0.0001, 'f', 5))
                                  + cardIndicatorRules + cardTitleRule);

    // Fusion draws every control outline - checkbox and radio indicators
    // included - as palette(window) darkened by 40%, within 1.1:1 of a dark
    // card. Nothing in the shell paints with that role, so raising it costs
    // nothing. Per control, because a stylesheet freezes the palette of every
    // widget it polishes - and after the stylesheet, because assigning one
    // re-polishes the subtree back to the palette it was first polished with.
    const QColor controlOutlineSource = darkPage ? blend(cardColor, textColor, 0.55) : themePalette.color(QPalette::Window);
    const QColor placeholderText = blend(cardColor, textColor, 0.45);
    for (auto* pControl : mpWidget_shell->findChildren<QWidget*>()) {
        if (!qobject_cast<QAbstractButton*>(pControl) && !qobject_cast<QLineEdit*>(pControl) && !qobject_cast<QAbstractSpinBox*>(pControl) && !qobject_cast<QComboBox*>(pControl)) {
            continue;
        }
        QPalette controlPalette = pControl->palette();
        controlPalette.setColor(QPalette::Window, controlOutlineSource);
        // The dark theme leaves PlaceholderText at the light default, which is
        // all but black on a dark field
        controlPalette.setColor(QPalette::PlaceholderText, placeholderText);
        pControl->setPalette(controlPalette);
    }

    // A rich-text anchor takes its colour from the palette rather than the
    // stylesheet, and the theme's default is not chosen against a card
    QList<QLabel*> linkLabels{mpLabel_searchEmpty, mpLabel_securityLink.data()};
    for (auto* pDescription : findChildren<QLabel*>(qsl("settingsCardDescription"))) {
        linkLabels.append(pDescription);
    }
    for (auto* pLabel : linkLabels) {
        if (!pLabel) {
            continue;
        }
        QPalette linkPalette = pLabel->palette();
        linkPalette.setColor(QPalette::Link, accentText);
        pLabel->setPalette(linkPalette);
    }
}

// Found by type rather than listed by hand, since a list would silently miss
// whatever gets added to the .ui file next. Qt::UniqueConnection makes this
// safe to call again once a profile brings its own controls with it. Push
// buttons are left out: what they do, they already do for themselves.
void dlgProfilePreferences::connectApplyTriggers()
{
    for (auto* pButton : findChildren<QAbstractButton*>()) {
        // Check boxes go to the loop below, whose signal says more than toggled()
        if (qobject_cast<QPushButton*>(pButton) || qobject_cast<QToolButton*>(pButton) || qobject_cast<QCheckBox*>(pButton)) {
            continue;
        }
        connect(pButton, &QAbstractButton::toggled, this, &dlgProfilePreferences::slot_scheduleApply, Qt::UniqueConnection);
    }
    for (auto* pCheckBox : findChildren<QCheckBox*>()) {
        // toggled() says nothing about a tri-state box moving between its
        // partially checked and checked states
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
}

// The types are the ones connectApplyTriggers() listens to, so everything able
// to schedule an apply can also be told apart from how it was populated
static QVariant controlValue(const QObject* pControl)
{
    if (const auto* pGroupBox = qobject_cast<const QGroupBox*>(pControl)) {
        return pGroupBox->isCheckable() ? QVariant(pGroupBox->isChecked()) : QVariant();
    }
    if (const auto* pCheckBox = qobject_cast<const QCheckBox*>(pControl)) {
        // The check state rather than isChecked(), for the tri-state boxes
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
    return {};
}

// In the order the .ui file lists them
static enums::controlsVisibility visibilityFromComboIndex(const int index)
{
    switch (index) {
    case 0:
        return enums::visibleNever;
    case 1:
        return enums::visibleOnlyWithoutLoadedProfile;
    default:
        return enums::visibleAlways;
    }
}

// Qt sets the modified flag on the first keystroke and slot_lineEditFinished()
// clears it again, so until then the field holds half a word rather than a
// setting - which neither the apply nor the snapshot below takes it for.
static bool beingTypedInto(const QObject* pControl)
{
    const auto* pLineEdit = qobject_cast<const QLineEdit*>(pControl);
    return pLineEdit && pLineEdit->hasFocus() && pLineEdit->isModified();
}

SettingsSnapshot::SettingsSnapshot(const QWidget& owner, const QMap<QString, QKeySequence>& shortcuts)
: mOwner(owner)
, mCurrentShortcuts(shortcuts)
{
}

bool SettingsSnapshot::carriesValue(const QObject* pControl)
{
    return controlValue(pControl).isValid();
}

void SettingsSnapshot::take()
{
    const QHash<const QObject*, QVariant> previous = mValues;
    mValues.clear();
    for (const auto* pWidget : mOwner.findChildren<QWidget*>()) {
        const QVariant value = controlValue(pWidget);
        if (!value.isValid()) {
            continue;
        }
        // The apply this snapshot follows left a half-typed field alone, so what
        // it was last populated with has to stand until that edit finishes
        if (const auto it = previous.constFind(pWidget); beingTypedInto(pWidget) && it != previous.constEnd()) {
            mValues.insert(pWidget, *it);
            continue;
        }
        mValues.insert(pWidget, value);
    }
    mShortcuts = mCurrentShortcuts;
}

void SettingsSnapshot::take(const QObject* pControl)
{
    mValues.insert(pControl, controlValue(pControl));
}

bool SettingsSnapshot::dirty(const QObject* pControl) const
{
    // The debounce is shared, so the apply about to read this was very likely
    // started by another control's edit
    if (beingTypedInto(pControl)) {
        return false;
    }
    const auto it = mValues.constFind(pControl);
    if (it == mValues.constEnd()) {
        // A control that came into being after the last snapshot:
        return true;
    }
    return *it != controlValue(pControl);
}

// For a setting spread over several controls - the borders, the Discord privacy
// flags - one of them changing means the write happens. What is written is
// still composed control by control: an undirty control contributes the value
// the Host holds now rather than what it shows, which a script may have moved
// on from (#10165). Members that are separate settings take their own guard.
bool SettingsSnapshot::anyDirty(const QList<const QObject*>& controls) const
{
    for (const auto* pControl : controls) {
        if (dirty(pControl)) {
            return true;
        }
    }
    return false;
}

bool SettingsSnapshot::shortcutsDirty() const
{
    return mCurrentShortcuts != mShortcuts;
}

bool SettingsSnapshot::shortcutDirty(const QString& key) const
{
    return mCurrentShortcuts.value(key) != mShortcuts.value(key);
}

bool SettingsSnapshot::pendingEdits(const QTimer* pApplyTimer, const QLineEdit* pSearchField) const
{
    // Whatever the settings say, what the controls hold is the user's until the
    // apply has run - and the refresh at the end of it re-reads them anyway
    if (pApplyTimer && pApplyTimer->isActive()) {
        return true;
    }
    for (const auto* pWidget : mOwner.findChildren<QWidget*>()) {
        if (pWidget == pSearchField || !carriesValue(pWidget)) {
            continue;
        }
        // dirty() answers false for a field being typed into, which is exactly
        // the edit that must not be written over here - so it is asked separately
        if (beingTypedInto(pWidget) || dirty(pWidget)) {
            return true;
        }
    }
    if (shortcutsDirty()) {
        return true;
    }
    // A shortcut editor holds a capture until editingFinished, so one showing
    // anything other than what it last committed is an edit in progress
    for (auto it = mEditors.cbegin(), end = mEditors.cend(); it != end; ++it) {
        if (it.value() && it.value()->keySequence() != mCurrentShortcuts.value(it.key())) {
            return true;
        }
    }
    return false;
}

TKeySequenceEdit* SettingsSnapshot::editorFor(const QString& key) const
{
    return mEditors.value(key).data();
}

void SettingsSnapshot::addEditor(const QString& key, TKeySequenceEdit* pEditor)
{
    mEditors.insert(key, pEditor);
}

// The dialog stays open for hours while scripts and other dialogs move the
// settings on underneath it, so coming back to its window is when it re-reads
// them. Re-reading is indistinguishable from discarding whatever was in the way
// of it, so an edit anywhere in the dialog skips this run and the activation
// after that edit is applied does the work instead.
void dlgProfilePreferences::refreshFromSettings()
{
    // Activation arrives repeatedly - a dialog opened over this one and closed
    // again is two of them - and the population below raises mPopulating
    if (mPopulating || mClosing || !mShellReady) {
        return;
    }
    if (mSearchActive) {
        // Repopulating replaces the text the search index was built from, and
        // the only way to fix that up is to clear the query, which is the user's
        return;
    }
    if (mSnapshot.pendingEdits(mpTimer_apply, mpLineEdit_search)) {
        return;
    }

    // Nothing is borrowed with no query standing, so this only throws the index
    // away for the next query to rebuild
    invalidateSearch();

    // Both notices say a change the player has just made will not reach the game
    // until they reconnect, and both are hidden by initWithHost() below. That is
    // right when a profile is being read in, but here the reading is Mudlet
    // writing the controls back after an instant apply - which is the very
    // change the notice is about, so it has to survive it.
    const bool reconnectForDataProtocol = !need_reconnect_for_data_protocol->isHidden();
    const bool reconnectForSpecialOption = !need_reconnect_for_specialoption->isHidden();

    // On the two paths that build the dialog, population happens before the
    // write-through connections are made; here they are already there. Written
    // silently, or a setting that has moved travels straight back out of the
    // control just told about it - and one whose control cannot hold it exactly
    // (the room size is a 1-11 scale over a qreal) comes back rounded.
    std::vector<QSignalBlocker> blockers;
    const auto controls = findChildren<QWidget*>();
    blockers.reserve(controls.size());
    for (auto* pControl : controls) {
        if (SettingsSnapshot::carriesValue(pControl)) {
            blockers.emplace_back(pControl);
        }
    }

    mPopulating = true;
    populateApplicationSettings();
    if (Host* pHost = mpHost; pHost) {
        initWithHost(pHost);
    }
    mPopulating = false;
    need_reconnect_for_data_protocol->setVisible(reconnectForDataProtocol);
    need_reconnect_for_specialoption->setVisible(reconnectForSpecialOption);
    // Released before the re-measuring below, which moves checkboxes between parents
    blockers.clear();

    // The pairing every population ends with: what the controls hold now is what
    // the settings say, so only what changes after this is the user's next edit
    connectApplyTriggers();
    mSnapshot.take();
    // A label can have been replaced by a longer or shorter one, and a checkbox
    // that needed wrapping may no longer. Caps after whatever wrote the
    // controls, tab order after the caps, as a wrapped checkbox sits a widget deeper.
    updateColumnWidthCaps();
    rebuildTabOrder();
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
    // The protocols themselves are on a page of their own, which this card only leads to
    mpCard_protocolList->setEnabled(false);
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
    if (mpCard_discord) {
        mpCard_discord->hide();
    }

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
    mpCard_protocolList->setEnabled(true);

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
    mpCard_discord->show();

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

// Every write here is blocked from signalling, because this is the dialog
// reading a setting: a control answering by writing the same value straight
// back would, on a language or appearance setting, undo what another dialog
// just did.
void dlgProfilePreferences::populateApplicationSettings()
{
    mudlet* pMudlet = mudlet::self();

    // As we demonstrate the options that these next two checkboxes control in
    // the editor "preview" widget (on another tab) we will need to track
    // changes and update the edbee widget straight away. As we can have
    // multiple profiles each with a separate instance of this form open we also
    // have to respond to changes in the settings when *another* profile saves
    // them.
    checkBox_showSpacesAndTabs->setChecked(pMudlet->mEditorTextOptions & QTextOption::ShowTabsAndSpaces);
    checkBox_showLineFeedsAndParagraphs->setChecked(pMudlet->mEditorTextOptions & QTextOption::ShowLineAndParagraphSeparators);

    checkBox_reportMapIssuesOnScreen->setChecked(TMap::smShowMapAuditErrors);
    checkBox_showIconsOnMenus->setCheckState(pMudlet->mShowIconsOnMenuCheckedState);

    MainIconSize->setValue(pMudlet->mToolbarIconSize);
    TEFolderIconSize->setValue(pMudlet->mEditorTreeWidgetIconSize);

    {
        const QSignalBlocker menuBarBlocker(comboBox_menuBarVisibility);
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

        const QSignalBlocker toolBarBlocker(comboBox_toolBarVisibility);
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
    }

    // Sync "Never" item deactivation so the dialog opens with consistent state
    // if either visibility was already "Never" on previous save (issue #7079).
    slot_syncMenuToolBarNeverItem();

    {
        const QSignalBlocker blocker(checkBox_showTabConnectionIndicators);
        checkBox_showTabConnectionIndicators->setChecked(pMudlet->mShowTabConnectionIndicators);
    }
    {
        const QSignalBlocker blocker(comboBox_appearance);
        comboBox_appearance->setCurrentIndex(pMudlet->mAppearance);
    }
    {
        // The one setting here that lives in its own QSettings group rather than
        // in the mudlet instance, so the only one with no change to announce
        const QSignalBlocker blocker(comboBox_crashReportPolicy);
        const QSettings settings("Mudlet", "CrashReporter");
        const QVariant storedOption = settings.value("autoSendCrashReports", QVariant());
        comboBox_crashReportPolicy->setCurrentIndex(storedOption.isValid() ? storedOption.toInt() - 1 : 2);
    }
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

    connect(fontComboBox_displayFont, &QFontComboBox::currentFontChanged, this, &dlgProfilePreferences::slot_displayFontChanged, Qt::UniqueConnection);
    connect(spinBox_displayFontSize, qOverload<int>(&QSpinBox::valueChanged), this, &dlgProfilePreferences::slot_displayFontSizeChanged, Qt::UniqueConnection);
    connect(checkBox_antiAlias, &QCheckBox::clicked, this, &dlgProfilePreferences::slot_displayFontAliasingChanged, Qt::UniqueConnection);

    // search engine load - emptied first so a second run replaces the list
    {
        const QSignalBlocker blocker(search_engine_combobox);
        search_engine_combobox->clear();
        search_engine_combobox->addItems(QStringList(pHost->mSearchEngineData.keys()));

        // set to saved value or default to Google
        const int savedText = search_engine_combobox->findText(pHost->getSearchEngine().first);
        search_engine_combobox->setCurrentIndex(savedText == -1 ? 1 : savedText);
    }

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
    // This will also settle MudletApp::usingMudletDictionaries():
    const QString path = MudletApp::getMudletPath(enums::hunspellDictionaryPath, currentDictionary);
    // Tweak the label for the provided spelling dictionaries depending on where
    // they come from:
    if (MudletApp::usingMudletDictionaries()) {
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
        connect(buttonDownloadMap, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_downloadMap, Qt::UniqueConnection);
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
    connect(checkBox_advertiseScreenReader, &QCheckBox::toggled, this, &dlgProfilePreferences::slot_toggleAdvertiseScreenReader, Qt::UniqueConnection);
    checkBox_enableOSC8Hyperlinks->setChecked(pHost->mEnableOSC8Hyperlinks);
    connect(checkBox_enableOSC8Hyperlinks, &QCheckBox::toggled, this, &dlgProfilePreferences::slot_toggleEnableOSC8Hyperlinks, Qt::UniqueConnection);

    checkBox_enableClosedCaption->setChecked(pHost->mEnableClosedCaption);
    connect(checkBox_enableClosedCaption, &QCheckBox::toggled, this, &dlgProfilePreferences::slot_toggleEnableClosedCaption, Qt::UniqueConnection);

    // Block signals before setting initial state to prevent toggled signal
    checkBox_f3SearchEnabled->blockSignals(true);
    checkBox_f3SearchEnabled->setChecked(pHost->getF3SearchEnabled());
    checkBox_f3SearchEnabled->blockSignals(false);
    // Now connect the signal
    connect(checkBox_f3SearchEnabled, &QCheckBox::toggled, pHost, &Host::setF3SearchEnabled, Qt::UniqueConnection);

    checkBox_enableBlinkText->setChecked(pHost->getEnableBlinkText());

    // same with special connection warnings
    need_reconnect_for_specialoption->hide();

    wrap_at_spinBox->setValue(pHost->mWrapAt);
    indent_wrapped_spinBox->setValue(pHost->mWrapIndentCount);
    hanging_indent_wrapped_spinBox->setValue(pHost->mWrapHangingIndentCount);
    checkBox_undoServerWrap->setChecked(pHost->mUndoServerWrap);
    undo_server_wrap_width_spinBox->setValue(pHost->mUndoServerWrapWidth);
    undo_server_wrap_width_spinBox->setEnabled(pHost->mUndoServerWrap);
    connect(checkBox_undoServerWrap, &QCheckBox::toggled, undo_server_wrap_width_spinBox, &QWidget::setEnabled, Qt::UniqueConnection);
    // The note is only worth its space to someone actually running the option:
    label_undo_server_wrap_experimental->setVisible(pHost->mUndoServerWrap);
    connect(checkBox_undoServerWrap, &QCheckBox::toggled, label_undo_server_wrap_experimental, &QWidget::setVisible, Qt::UniqueConnection);

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
        } else {
            // ...and back on for a profile that has stopped using it, which the
            // checkbox's own slot would otherwise be the only way to hear about
            console_buffer_size_spinBox->setEnabled(true);
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

    if (Discord::self()->libraryLoaded()) {
        Host::DiscordOptionFlags const discordFlags = pHost->mDiscordAccessFlags;
        groupBox_discordPrivacy->show();
        mpCard_discord->show();

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

    {
        // Rebuilt rather than added to, and silently: the momentary empty list
        // in the middle of it is not a log format anyone chose
        const QSignalBlocker blocker(comboBox_logFileNameFormat);
        comboBox_logFileNameFormat->clear();
        // This is the previous standard:
        comboBox_logFileNameFormat->addItem(tr("yyyy-MM-dd#HH-mm-ss (e.g., 1970-01-01#00-00-00%1)").arg(logExtension), qsl("yyyy-MM-dd#HH-mm-ss"));
        // The ISO standard for this uses T as the date/time separator
        comboBox_logFileNameFormat->addItem(tr("yyyy-MM-ddTHH-mm-ss (e.g., 1970-01-01T00-00-00%1)").arg(logExtension), qsl("yyyy-MM-ddTHH-mm-ss"));
        comboBox_logFileNameFormat->addItem(tr("yyyy-MM-dd (concatenate daily logs in, e.g. 1970-01-01%1)").arg(logExtension), qsl("yyyy-MM-dd"));
        // It might be possible to use QDateTime::weekNumber but that number is
        // not available from the QDateTime::toString(...) method
        comboBox_logFileNameFormat->addItem(tr("yyyy-MM (concatenate month logs in, e.g. 1970-01%1)").arg(logExtension), qsl("yyyy-MM"));
        comboBox_logFileNameFormat->addItem(tr("Named file (concatenate logs in one file)"), QString());
        comboBox_logFileNameFormat->setCurrentIndex(comboBox_logFileNameFormat->findData(pHost->mLogFileNameFormat));
    }

    lineEdit_logFileName->setText(pHost->mLogFileName);

    // pHost->mLogDir should be empty for the default location:
    mLogDirPath = pHost->mLogDir;
    lineEdit_logFileFolder->setText(mLogDirPath);
    lineEdit_logFileFolder->setPlaceholderText(MudletApp::getMudletPath(enums::profileReplayAndLogFilesPath, pHost->getName()));
    // set the cursor position to the end of the lineEdit's text property.
    lineEdit_logFileFolder->setCursorPosition(lineEdit_logFileFolder->text().length());
    // Enable the reset button if the current location is not the default one:
    pushButton_resetLogDir->setEnabled(mLogDirPath.length() > 0);


    commandLineMinimumHeight->setValue(pHost->commandLineMinimumHeight);
    mFORCE_MCCP_OFF->setChecked(pHost->mFORCE_NO_COMPRESSION);
    mFORCE_GA_OFF->setChecked(pHost->mFORCE_GA_OFF);
    mAlertOnNewData->setChecked(pHost->mAlertOnNewData);
    telnetHandlerEnabled->setChecked(MudletApp::getQSettings()->value("telnetHandlerEnabled", false).toBool());
    //encoding->setCurrentIndex( pHost->mEncoding );
    mFORCE_SAVE_ON_EXIT->setChecked(pHost->mFORCE_SAVE_ON_EXIT);

    // buildProtocolsSubpage() built and wired these once; a profile only fills them
    mEnableCHARSET->setChecked(pHost->mEnableCHARSET);
    mEnableGMCP->setChecked(pHost->mEnableGMCP);
    mEnableMNES->setChecked(pHost->mEnableMNES);
    mEnableMSDP->setChecked(pHost->mEnableMSDP);
    mEnableMSP->setChecked(pHost->mEnableMSP);
    mEnableMSSP->setChecked(pHost->mEnableMSSP);
    mEnableMTTS->setChecked(pHost->mEnableMTTS);
    mEnableMXP->setChecked(pHost->mEnableMXP);
    mEnableNAWS->setChecked(pHost->mEnableNAWS);
    mEnableNEWENVIRON->setChecked(pHost->mEnableNEWENVIRON);
    updateProtocolSummary();

    groupBox_purgeMediaCache->setVisible(true);
    connect(buttonPurgeMediaCache, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_purgeMediaCache, Qt::UniqueConnection);

    // load profiles into mappers "copy map to profile" combobox
    // this feature should work seamlessly both for online and offline profiles
    const QStringList profileList = QDir(MudletApp::getMudletPath(enums::profilesPath)).entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time); // sort by profile "hotness"
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

        // The one control on this page the .ui file does not carry, so built
        // rather than filled - and built once, however many profiles come and go
        if (!mpDoubleSpinBox_mapSymbolFontFudge) {
            QLabel* pLabel_mapSymbolFontFudge = new QLabel(tr("2D Map Room Symbol scaling factor:"), groupBox_mapSymbols);
            mpDoubleSpinBox_mapSymbolFontFudge = new QDoubleSpinBox(groupBox_mapSymbols);
            mpDoubleSpinBox_mapSymbolFontFudge->setPrefix(qsl("×"));
            mpDoubleSpinBox_mapSymbolFontFudge->setRange(TMap::scmMinimumSymbolFontFudgeFactor, TMap::scmMaximumSymbolFontFudgeFactor);
            mpDoubleSpinBox_mapSymbolFontFudge->setSingleStep(0.01);
            // Qt's default of two decimals would show a factor set from Lua as
            // something it is not. Both this and the range have to be in place
            // before the value, which a spin box rounds and clamps as given.
            mpDoubleSpinBox_mapSymbolFontFudge->setDecimals(3);
            auto* pSymbolsLayout = qobject_cast<QGridLayout*>(groupBox_mapSymbols->layout());
            if (pSymbolsLayout) {
                const int existingRows = pSymbolsLayout->rowCount();
                pSymbolsLayout->addWidget(pLabel_mapSymbolFontFudge, existingRows, 0);
                pSymbolsLayout->addWidget(mpDoubleSpinBox_mapSymbolFontFudge, existingRows, 1);
            } else {
                qWarning()
                        << "dlgProfilePreferences::initWithHost(...) WARNING - Unable to cast groupBox_mapSymbols layout to expected QGridLayout - someone has messed with the profile_preferences.ui "
                           "file and the contents of the groupBox can not be shown...!";
            }
        }
        {
            // Whether just built or left here by a previous profile, writing this
            // is the dialog reading the map rather than the user turning a dial
            const QSignalBlocker blocker(mpDoubleSpinBox_mapSymbolFontFudge);
            mpDoubleSpinBox_mapSymbolFontFudge->setValue(pHost->mpMap->getSymbolFontFudgeFactor());
        }
        connect(mpDoubleSpinBox_mapSymbolFontFudge, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &dlgProfilePreferences::slot_mapSymbolFontFudgeChanged, Qt::UniqueConnection);

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

        connect(pushButton_deleteMap, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_deleteMap, Qt::UniqueConnection);
        connect(comboBox_playerRoomStyle, qOverload<int>(&QComboBox::currentIndexChanged), this, &dlgProfilePreferences::slot_changePlayerRoomStyle, Qt::UniqueConnection);
        connect(pushButton_playerRoomPrimaryColor, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setPlayerRoomPrimaryColor, Qt::UniqueConnection);
        connect(pushButton_playerRoomSecondaryColor, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setPlayerRoomSecondaryColor, Qt::UniqueConnection);
        connect(spinBox_playerRoomOuterDiameter, qOverload<int>(&QSpinBox::valueChanged), this, &dlgProfilePreferences::slot_setPlayerRoomOuterDiameter, Qt::UniqueConnection);
        connect(spinBox_playerRoomInnerDiameter, qOverload<int>(&QSpinBox::valueChanged), this, &dlgProfilePreferences::slot_setPlayerRoomInnerDiameter, Qt::UniqueConnection);

        // Initialize room, exit, and border size controls
        spinBox_roomSize->setValue(pHost->mRoomSize * 10);
        // mLineSize/mRoomBorderSize are inversely proportional to thickness
        // (exitWidth = 1/eSize * ...), convert to a direct 1-11 scale
        // using a simple reciprocal: mLineSize = 50 / spinner, spinner = 50 / mLineSize
        spinBox_exitSize->setValue(qBound(1, qRound(50.0 / pHost->mLineSize), 11));
        spinBox_borderSize->setValue(qBound(1, qRound(50.0 / pHost->mRoomBorderSize), 11));
        doubleSpinBox_gridSize->setValue(pHost->mMapGridLineSize);
        connect(spinBox_roomSize, qOverload<int>(&QSpinBox::valueChanged), this, &dlgProfilePreferences::slot_roomSizeChanged, Qt::UniqueConnection);
        connect(spinBox_exitSize, qOverload<int>(&QSpinBox::valueChanged), this, &dlgProfilePreferences::slot_exitSizeChanged, Qt::UniqueConnection);
        connect(spinBox_borderSize, qOverload<int>(&QSpinBox::valueChanged), this, &dlgProfilePreferences::slot_borderSizeChanged, Qt::UniqueConnection);
        connect(doubleSpinBox_gridSize, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &dlgProfilePreferences::slot_gridSizeChanged, Qt::UniqueConnection);
        connect(checkbox_mMapperShowRoomBorders, &QCheckBox::toggled, this, &dlgProfilePreferences::slot_changeMapperShowRoomBorders, Qt::UniqueConnection);
        connect(checkBox_drawUpperLowerLevels, &QCheckBox::toggled, this, &dlgProfilePreferences::slot_changeDrawUpperLowerLevels, Qt::UniqueConnection);
        connect(mMapperUseAntiAlias, &QCheckBox::toggled, this, &dlgProfilePreferences::slot_changeMapperUseAntiAlias, Qt::UniqueConnection);
    } else {
        label_mapSymbolsFont->setEnabled(false);
        fontComboBox_mapSymbols->setEnabled(false);
        checkBox_isOnlyMapSymbolFontToBeUsed->setEnabled(false);
        pushButton_showGlyphUsage->setEnabled(false);

        checkBox_showDefaultArea->hide();
        groupBox_playerRoomStyle->setEnabled(false);
    }

    {
        // Which encodings there are is the profile's connection talking
        const QSignalBlocker blocker(comboBox_encoding);
        comboBox_encoding->clear();
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
    }

    comboBox_controlCharacterHandling->setItemData(0, QVariant::fromValue(ControlCharacterMode::AsIs));
    comboBox_controlCharacterHandling->setItemData(1, QVariant::fromValue(ControlCharacterMode::Picture));
    comboBox_controlCharacterHandling->setItemData(2, QVariant::fromValue(ControlCharacterMode::OEM));
    auto cch_index = comboBox_controlCharacterHandling->findData(static_cast<int>(pHost->getControlCharacterMode()));
    comboBox_controlCharacterHandling->setCurrentIndex((cch_index > 0) ? cch_index : 0);
    connect(comboBox_controlCharacterHandling, qOverload<int>(&QComboBox::currentIndexChanged), this, &dlgProfilePreferences::slot_changeControlCharacterHandling, Qt::UniqueConnection);

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
    pushButton_forgetSavedSignIn->setEnabled(mEnableGMCP->isChecked());
    // Once per profile rather than once per run: reading the keychain can cost
    // the user a prompt on some platforms, for an answer that hardly changes
    if (mSignInTokenCheckedFor != pHost->getName()) {
        mSignInTokenCheckedFor = pHost->getName();
        pushButton_forgetSavedSignIn->setVisible(false);
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
    }

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
    updateSecurityStatus();
    updateDiscordSummary();

    // CHECKME: Have moved ALL the connects, where possible, to the end so that
    // none are triggered by the setup operations...
    connect(pushButton_command_line_foreground_color, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setCommandLineFgColor, Qt::UniqueConnection);
    connect(pushButton_command_line_background_color, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setCommandLineBgColor, Qt::UniqueConnection);

    connect(pushButton_black, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setColorBlack, Qt::UniqueConnection);
    connect(pushButton_lBlack, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setColorLightBlack, Qt::UniqueConnection);
    connect(pushButton_red, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setColorRed, Qt::UniqueConnection);
    connect(pushButton_lRed, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setColorLightRed, Qt::UniqueConnection);
    connect(pushButton_green, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setColorGreen, Qt::UniqueConnection);
    connect(pushButton_lGreen, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setColorLightGreen, Qt::UniqueConnection);
    connect(pushButton_yellow, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setColorYellow, Qt::UniqueConnection);
    connect(pushButton_lYellow, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setColorLightYellow, Qt::UniqueConnection);
    connect(pushButton_blue, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setColorBlue, Qt::UniqueConnection);
    connect(pushButton_lBlue, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setColorLightBlue, Qt::UniqueConnection);
    connect(pushButton_magenta, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setColorMagenta, Qt::UniqueConnection);
    connect(pushButton_lMagenta, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setColorLightMagenta, Qt::UniqueConnection);
    connect(pushButton_cyan, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setColorCyan, Qt::UniqueConnection);
    connect(pushButton_lCyan, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setColorLightCyan, Qt::UniqueConnection);
    connect(pushButton_white, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setColorWhite, Qt::UniqueConnection);
    connect(pushButton_lWhite, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setColorLightWhite, Qt::UniqueConnection);

    connect(pushButton_foreground_color, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setFgColor, Qt::UniqueConnection);
    connect(pushButton_background_color, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setBgColor, Qt::UniqueConnection);
    connect(pushButton_command_foreground_color, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setCommandFgColor, Qt::UniqueConnection);
    connect(pushButton_command_background_color, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setCommandBgColor, Qt::UniqueConnection);

    connect(pushButton_resetColors, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_resetColors, Qt::UniqueConnection);
    connect(reset_colors_button_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_resetMapColors, Qt::UniqueConnection);
    connect(pushButton_black_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapColorBlack, Qt::UniqueConnection);
    connect(pushButton_Lblack_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapColorLightBlack, Qt::UniqueConnection);
    connect(pushButton_green_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapColorGreen, Qt::UniqueConnection);
    connect(pushButton_Lgreen_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapColorLightGreen, Qt::UniqueConnection);
    connect(pushButton_red_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapColorRed, Qt::UniqueConnection);
    connect(pushButton_Lred_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapColorLightRed, Qt::UniqueConnection);
    connect(pushButton_blue_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapColorBlue, Qt::UniqueConnection);
    connect(pushButton_Lblue_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapColorLightBlue, Qt::UniqueConnection);
    connect(pushButton_yellow_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapColorYellow, Qt::UniqueConnection);
    connect(pushButton_Lyellow_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapColorLightYellow, Qt::UniqueConnection);
    connect(pushButton_cyan_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapColorCyan, Qt::UniqueConnection);
    connect(pushButton_Lcyan_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapColorLightCyan, Qt::UniqueConnection);
    connect(pushButton_magenta_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapColorMagenta, Qt::UniqueConnection);
    connect(pushButton_Lmagenta_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapColorLightMagenta, Qt::UniqueConnection);
    connect(pushButton_white_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapColorWhite, Qt::UniqueConnection);
    connect(pushButton_Lwhite_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapColorLightWhite, Qt::UniqueConnection);

    connect(pushButton_foreground_color_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapExitsColor, Qt::UniqueConnection);
    connect(pushButton_background_color_2, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapBgColor, Qt::UniqueConnection);
    connect(pushButton_lowerLevelColor, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setLowerLevelColor, Qt::UniqueConnection);
    connect(pushButton_upperLevelColor, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setUpperLevelColor, Qt::UniqueConnection);
    connect(pushButton_roomBorderColor, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapRoomBorderColor, Qt::UniqueConnection);
    connect(pushButton_mapInfoBg, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapInfoBgColor, Qt::UniqueConnection);
    connect(pushButton_roomCollisionBorderColor, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapRoomCollisionBorderColor, Qt::UniqueConnection);
    connect(pushButton_mapGridColor, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setMapGridColor, Qt::UniqueConnection);

    connect(pushButton_forgetSavedSignIn, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_forgetSavedSignIn, Qt::UniqueConnection);

    // The hero reports the connection as it is now, so it hears it come and go
    connect(&pHost->mTelnet, &cTelnet::signal_connecting, this, &dlgProfilePreferences::updateSecurityStatus, Qt::UniqueConnection);
    connect(&pHost->mTelnet, &cTelnet::signal_connected, this, &dlgProfilePreferences::updateSecurityStatus, Qt::UniqueConnection);
    connect(&pHost->mTelnet, &cTelnet::signal_disconnected, this, &dlgProfilePreferences::updateSecurityStatus, Qt::UniqueConnection);

    connect(mFORCE_MCCP_OFF, &QAbstractButton::clicked, need_reconnect_for_specialoption, &QWidget::show, Qt::UniqueConnection);
    connect(mFORCE_GA_OFF, &QAbstractButton::clicked, need_reconnect_for_specialoption, &QWidget::show, Qt::UniqueConnection);
    connect(mpMenu.data(), &QMenu::triggered, this, &dlgProfilePreferences::slot_chosenProfilesChanged, Qt::UniqueConnection);

    connect(pushButton_copyMap, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_copyMap, Qt::UniqueConnection);
    connect(pushButton_loadMap, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_loadMap, Qt::UniqueConnection);
    connect(pushButton_saveMap, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_saveMap, Qt::UniqueConnection);
    connect(comboBox_encoding, qOverload<int>(&QComboBox::currentIndexChanged), this, &dlgProfilePreferences::slot_setEncoding, Qt::UniqueConnection);

    connect(comboBox_caretModeKey, qOverload<int>(&QComboBox::currentIndexChanged), this, &dlgProfilePreferences::slot_caretModeKeyChanged, Qt::UniqueConnection);

    connect(pushButton_whereToLog, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_setLogDir, Qt::UniqueConnection);
    connect(pushButton_resetLogDir, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_resetLogDir, Qt::UniqueConnection);
    connect(comboBox_logFileNameFormat, qOverload<int>(&QComboBox::currentIndexChanged), this, &dlgProfilePreferences::slot_logFileNameFormatChange, Qt::UniqueConnection);
    connect(mIsToLogInHtml, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_changeLogFileAsHtml, Qt::UniqueConnection);
    connect(doubleSpinBox_networkPacketTimeout, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &dlgProfilePreferences::slot_setPostingTimeout, Qt::UniqueConnection);
    connect(checkBox_largeAreaExitArrows, &QCheckBox::toggled, this, &dlgProfilePreferences::slot_changeLargeAreaExitArrows, Qt::UniqueConnection);
    connect(checkBox_invertMapZoom, &QCheckBox::toggled, this, &dlgProfilePreferences::slot_changeInvertMapZoom, Qt::UniqueConnection);

    // Console buffer settings
    connect(checkBox_useMaxBufferSize, &QCheckBox::toggled, this, &dlgProfilePreferences::slot_toggleUseMaxBufferSize, Qt::UniqueConnection);

    //Shortcuts tab
    auto shortcutKeys = mudlet::self()->mpShortcutsManager->iterator();
    int shortcutsRow = 0;
    while (shortcutKeys.hasNext()) {
        auto key = shortcutKeys.next();
        auto shortcutIt = pHost->profileShortcuts.find(key);
        QKeySequence currentSequence = (shortcutIt != pHost->profileShortcuts.end()) ? QKeySequence(*shortcutIt->second) : QKeySequence();
        currentShortcuts.insert(key, currentSequence);
        // The editors outlive the profile that first filled them, so a second
        // profile re-reads the ones already in this grid. Building them again
        // would leave the first set below the second, still writing through.
        if (auto* pExistingEdit = mSnapshot.editorFor(key); pExistingEdit) {
            const QSignalBlocker blocker(pExistingEdit);
            pExistingEdit->setKeySequence(currentSequence);
            shortcutsRow++;
            continue;
        }
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
        mSnapshot.addEditor(key, sequenceEdit);
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

    // The protocol checkboxes are not in this list: what they are wired to says
    // how the controls relate to each other rather than anything about a Host

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
    // ...so the next profile to arrive asks the keychain again
    mSignInTokenCheckedFor.clear();
    groupBox_proxy->setDisabled(true);
    // With no profile there is no connection for the hero to report on
    updateSecurityStatus();

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
    connect(code_editor_theme_selection_combobox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &dlgProfilePreferences::slot_themeSelected, Qt::UniqueConnection);

    // allows people to select a script of theirs to preview
    connect(script_preview_combobox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &dlgProfilePreferences::slot_scriptSelected, Qt::UniqueConnection);

    // A deep link can reach the Editor page before there is a Host to build from
    if (!mEditorThemesChecked && mpStackedWidget_categories->currentIndex() == mCategories.value(scmCategory_editor).pageIndex) {
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

// The twelve tab objectNames callers may still pass are remapped onto the
// categories holding their contents now. A new-style target is either a
// category key or "category/cardObjectName".
void dlgProfilePreferences::setTab(QString tab)
{
    static const QHash<QString, QString> legacyTabs{{qsl("tab_general"), scmCategory_general},
                                                    {qsl("tab_inputLine"), scmCategory_inputLine},
                                                    {qsl("tab_display"), scmCategory_mainDisplay},
                                                    {qsl("tab_displayColors"), scmCategory_mainDisplay},
                                                    {qsl("tab_codeEditor"), scmCategory_editor},
                                                    {qsl("tab_mapper"), scmCategory_mapper},
                                                    {qsl("tab_mapperColors"), scmCategory_mapper},
                                                    {qsl("tab_chat"), scmCategory_chat},
                                                    {qsl("tab_connection"), scmCategory_privacy},
                                                    {qsl("tab_shortcuts"), scmCategory_shortcuts},
                                                    {qsl("tab_accessibility"), scmCategory_accessibility},
                                                    {qsl("tab_specialOptions"), scmCategory_connection}};

    QString category = tab;
    QWidget* pSpotlightTarget = nullptr;
    if (const auto it = legacyTabs.constFind(tab); it != legacyTabs.constEnd()) {
        category = it.value();
        if (tab == qsl("tab_connection")) {
            // A TLS failure wants the certificate controls, not the page top
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
        const QString target = tab.mid(separator + 1);
        if (mSubpageIndexes.contains(tab)) {
            showSubpage(category, target);
            return;
        }
        pSpotlightTarget = findChild<QWidget*>(target);
    }

    // A card on a subpage is only reachable by going into it, so a link naming
    // one takes that way in rather than landing on the category page
    if (const QString subpage = subpageHolding(pSpotlightTarget); !subpage.isEmpty()) {
        showSubpage(subpage.section(QLatin1Char('/'), 0, 0), subpage.section(QLatin1Char('/'), 1), pSpotlightTarget);
        return;
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

    // What map files are on disk changes while the dialog is open, so this is a
    // rebuild - and the enabled state goes back to where an empty list leaves it
    {
        const QSignalBlocker blocker(comboBox_mapHistory);
        comboBox_mapHistory->clear();
    }
    comboBox_mapHistory->setEnabled(false);
    pushButton_loadHistoricMap->setEnabled(false);

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
    QDir mapSaveDir(MudletApp::getMudletPath(enums::profileMapsPath, profile_name).append(QLatin1Char('/')));
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
        connect(pushButton_loadHistoricMap, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_loadHistoryMap, Qt::UniqueConnection);
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
    const bool showAuditErrors = TMap::smShowMapAuditErrors;
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
    QSettings& settings = *MudletApp::getQSettings();
    QString lastDir = settings.value("lastFileDialogLocation", MudletApp::getMudletPath(enums::profileHomePath, pHost->getName())).toString();
    dialog->setDirectory(lastDir);
    dialog->setNameFilter(loadExtensions.join(qsl(";;")));
    connect(dialog, &QDialog::finished, this, [=, this](int result) {
        if (result == QDialog::Rejected) {
            return;
        }

        auto fileName = dialog->selectedFiles().constFirst();
        loadMap(fileName);
        QSettings& settings = *MudletApp::getQSettings();
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
    QSettings& settings = *MudletApp::getQSettings();
    QString lastDir = settings.value("lastFileDialogLocation", MudletApp::getMudletPath(enums::profileHomePath, pHost->getName())).toString();
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

        QSettings& settings = *MudletApp::getQSettings();
        QString lastDir = QFileInfo(fileName).absolutePath();
        settings.setValue("lastFileDialogLocation", lastDir);

        label_mapFileActionResult->show();
        label_mapFileActionResult->setText(tr("Saving map - please wait..."));
        qApp->processEvents(); // Copied from "Loading map - please wait..." case
        // Just in case is needed to make the above message
        // show up when saving big maps

        // Ensure the setting is already made as the saveMap(...) uses the set value
        const bool showAuditErrors = TMap::smShowMapAuditErrors;
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
    const QString mapsPath = MudletApp::getMudletPath(enums::profileMapsPath, pHost->getName());
    const QDir mapsDir = QDir(mapsPath);
    return mapsDir.exists() ? mapsPath : MudletApp::getMudletPath(enums::profileHomePath, pHost->getName());
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
            const QString toProfileDirPathString = MudletApp::getMudletPath(enums::profileHomePath, pHost->getName());
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
    const bool savedOldAuditErrorsToConsoleEnabledSetting = TMap::smShowMapAuditErrors;
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
    const QString sourceMapFolder(MudletApp::getMudletPath(enums::profileMapsPath, pHost->getName()));
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

        if (!thisProfileLatestMapFile.copy(MudletApp::getMudletPath(enums::profileMapPathFileName, otherHostName, thisProfileLatestMapPathFileName))) {
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

    QSettings& settings = *MudletApp::getQSettings();
    QString lastDir = settings.value("lastFileDialogLocation", MudletApp::getMudletPath(enums::profileHomePath, pHost->getName())).toString();

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
        if (currentLogDir == MudletApp::getMudletPath(enums::profileReplayAndLogFilesPath, pHost->getName())) {
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
        // The line edit is read-only, so it emits nothing instant apply listens to
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

// Off a 400ms debounce after any control changes, and once more from
// closeEvent(). Only the settings whose controls the user actually changed: the
// dialog stays open while scripts run, and writing back a control merely
// showing a stale value would revert what a script had just set (#10165).
void dlgProfilePreferences::applyAll()
{
    if (mPopulating) {
        return;
    }

    mudlet* pMudlet = mudlet::self();
    Host* pHost = mpHost;
    if (pHost) {
        auto console = pHost->mpConsole;
        if (mSnapshot.dirty(comboBox_dictionary) && comboBox_dictionary->isEnabled() && comboBox_dictionary->currentIndex() >= 0) {
            pHost->setSpellDic(comboBox_dictionary->currentData().toString());
        }

        if (mSnapshot.dirty(checkBox_spellCheck)) {
            pHost->mEnableSpellCheck = checkBox_spellCheck->isChecked();
        }
        if (mSnapshot.anyDirty({radioButton_userDictionary_common, radioButton_userDictionary_profile})) {
            if (radioButton_userDictionary_common->isChecked()) {
                pHost->setUserDictionaryOptions(true, true);
            } else {
                pHost->setUserDictionaryOptions(true, false);
            }
        }

        if (mSnapshot.dirty(wrap_at_spinBox)) {
            const int priorWrapAt = pHost->mWrapAt;
            pHost->mWrapAt = wrap_at_spinBox->value();

            if (priorWrapAt != pHost->mWrapAt) {
                slot_changeWrapAt();
            }
        }

        pHost->updateDisplayDimensions();
        if (mSnapshot.dirty(indent_wrapped_spinBox)) {
            pHost->mWrapIndentCount = indent_wrapped_spinBox->value();
        }
        if (mSnapshot.dirty(hanging_indent_wrapped_spinBox)) {
            pHost->mWrapHangingIndentCount = hanging_indent_wrapped_spinBox->value();
        }
        if (mSnapshot.dirty(checkBox_undoServerWrap)) {
            pHost->mUndoServerWrap = checkBox_undoServerWrap->isChecked();
        }
        if (mSnapshot.dirty(undo_server_wrap_width_spinBox)) {
            pHost->mUndoServerWrapWidth = undo_server_wrap_width_spinBox->value();
        }

        // Save console buffer settings and apply them
        if (mSnapshot.anyDirty({checkBox_useMaxBufferSize, console_buffer_size_spinBox})) {
            const bool useMaxBuffer = mSnapshot.dirty(checkBox_useMaxBufferSize) ? checkBox_useMaxBufferSize->isChecked() : pHost->getUseMaxConsoleBufferSize();
            int newBufferSize;

            if (useMaxBuffer && pHost->mpConsole) {
                newBufferSize = pHost->mpConsole->buffer.getMaxBufferSize();
            } else {
                newBufferSize = mSnapshot.dirty(console_buffer_size_spinBox) ? console_buffer_size_spinBox->value() : pHost->getConsoleBufferSize();
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

        if (mSnapshot.dirty(show_sent_text_combobox)) {
            pHost->mCommandEchoMode = static_cast<Host::CommandEchoMode>(show_sent_text_combobox->currentIndex());
        }
        if (mSnapshot.dirty(auto_clear_input_line_checkbox)) {
            pHost->mAutoClearCommandLineAfterSend = auto_clear_input_line_checkbox->isChecked();
        }
        if (mSnapshot.dirty(disable_password_masking_checkbox)) {
            pHost->mDisablePasswordMasking = disable_password_masking_checkbox->isChecked();
        }
        if (mSnapshot.dirty(checkBox_highlightHistory)) {
            pHost->mHighlightHistory = checkBox_highlightHistory->isChecked();
        }
        if (mSnapshot.dirty(command_separator_lineedit)) {
            pHost->mCommandSeparator = command_separator_lineedit->text();
        }
        if (mSnapshot.dirty(acceptServerGUI)) {
            pHost->mAcceptServerGUI = acceptServerGUI->isChecked();
        }
        if (mSnapshot.dirty(acceptServerMedia)) {
            pHost->mAcceptServerMedia = acceptServerMedia->isChecked();
        }
        if (mSnapshot.dirty(checkBox_USE_IRE_DRIVER_BUGFIX)) {
            pHost->set_USE_IRE_DRIVER_BUGFIX(checkBox_USE_IRE_DRIVER_BUGFIX->isChecked());
        }
        if (mSnapshot.dirty(checkBox_enableTextAnalyzer)) {
            pHost->mEnableTextAnalyzer = checkBox_enableTextAnalyzer->isChecked();
        }
        if (mSnapshot.dirty(checkBox_mUSE_FORCE_LF_AFTER_PROMPT)) {
            pHost->mUSE_FORCE_LF_AFTER_PROMPT = checkBox_mUSE_FORCE_LF_AFTER_PROMPT->isChecked();
        }
        if (mSnapshot.dirty(USE_UNIX_EOL)) {
            pHost->mUSE_UNIX_EOL = USE_UNIX_EOL->isChecked();
        }
        if (mSnapshot.dirty(checkBox_runAllKeyBindings)) {
            pHost->getKeyUnit()->mRunAllKeyMatches = checkBox_runAllKeyBindings->isChecked();
        }
        if (mSnapshot.dirty(mFORCE_MCCP_OFF)) {
            pHost->mFORCE_NO_COMPRESSION = mFORCE_MCCP_OFF->isChecked();
        }
        if (mSnapshot.dirty(mFORCE_GA_OFF)) {
            pHost->mFORCE_GA_OFF = mFORCE_GA_OFF->isChecked();
        }
        if (mSnapshot.dirty(mFORCE_SAVE_ON_EXIT)) {
            pHost->mFORCE_SAVE_ON_EXIT = mFORCE_SAVE_ON_EXIT->isChecked();
        }
        if (mSnapshot.dirty(mEnableGMCP)) {
            pHost->mEnableGMCP = mEnableGMCP->isChecked();
        }
        if (mSnapshot.dirty(mEnableMSSP)) {
            pHost->mEnableMSSP = mEnableMSSP->isChecked();
        }
        if (mSnapshot.dirty(mEnableMSDP)) {
            pHost->mEnableMSDP = mEnableMSDP->isChecked();
        }
        if (mSnapshot.dirty(mEnableMSP)) {
            pHost->mEnableMSP = mEnableMSP->isChecked();
        }
        if (mSnapshot.dirty(mEnableMXP)) {
            pHost->mEnableMXP = mEnableMXP->isChecked();
        }
        if (mSnapshot.dirty(mEnableMTTS)) {
            pHost->mEnableMTTS = mEnableMTTS->isChecked();
        }
        if (mSnapshot.dirty(mEnableMNES)) {
            pHost->mEnableMNES = mEnableMNES->isChecked();
        }
        if (mSnapshot.dirty(mEnableNAWS)) {
            pHost->mEnableNAWS = mEnableNAWS->isChecked();
        }
        if (mSnapshot.dirty(mEnableCHARSET)) {
            pHost->mEnableCHARSET = mEnableCHARSET->isChecked();
        }
        if (mSnapshot.dirty(mEnableNEWENVIRON)) {
            pHost->mEnableNEWENVIRON = mEnableNEWENVIRON->isChecked();
        }
        if (mSnapshot.dirty(mMapperUseAntiAlias)) {
            pHost->mMapperUseAntiAlias = mMapperUseAntiAlias->isChecked();
        }
        if (mSnapshot.dirty(checkbox_mMapperShowRoomBorders)) {
            pHost->mMapperShowRoomBorders = checkbox_mMapperShowRoomBorders->isChecked();
        }
        if (mSnapshot.dirty(checkBox_drawUpperLowerLevels)) {
            pMudlet->mDrawUpperLowerLevels = checkBox_drawUpperLowerLevels->isChecked();
        }
        if (pHost->mpMap) {
            if (mSnapshot.dirty(checkBox_showDefaultArea)) {
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
            if (mSnapshot.dirty(mMapperUseAntiAlias) && pHost->mpMap->mpMapper) {
                pHost->mpMap->mpMapper->mp2dMap->mMapperUseAntiAlias = mMapperUseAntiAlias->isChecked();
            }

            // Only when the spin-box is what holds the newer value. It carries
            // no more precision than it displays, so writing it back whenever
            // Save is clicked would round off a factor a script had set:
            if (mpDoubleSpinBox_mapSymbolFontFudge && mSnapshot.dirty(mpDoubleSpinBox_mapSymbolFontFudge)
                && !spinBoxShows(mpDoubleSpinBox_mapSymbolFontFudge, pHost->mpMap->getSymbolFontFudgeFactor())) {
                pHost->mpMap->setSymbolFontFudgeFactor(mpDoubleSpinBox_mapSymbolFontFudge->value());
            }

            if (pHost->mpMap->mpMapper) {
                pHost->mpMap->mpMapper->mp2dMap->repaint(); // Forceably redraw it as we ARE currently showing default area
                pHost->mpMap->mpMapper->update();
            }
        }
        if (mSnapshot.anyDirty({leftBorderWidth, topBorderHeight, rightBorderWidth, bottomBorderHeight})) {
            const QMargins liveBorders = pHost->userBorders();
            const QMargins newBorders{mSnapshot.dirty(leftBorderWidth) ? leftBorderWidth->value() : liveBorders.left(),
                                      mSnapshot.dirty(topBorderHeight) ? topBorderHeight->value() : liveBorders.top(),
                                      mSnapshot.dirty(rightBorderWidth) ? rightBorderWidth->value() : liveBorders.right(),
                                      mSnapshot.dirty(bottomBorderHeight) ? bottomBorderHeight->value() : liveBorders.bottom()};
            pHost->setUserBorders(newBorders);
        }
        if (mSnapshot.dirty(commandLineMinimumHeight)) {
            pHost->commandLineMinimumHeight = commandLineMinimumHeight->value();
        }
        if (mSnapshot.dirty(checkBox_mVersionInTTYPE)) {
            pHost->mVersionInTTYPE = checkBox_mVersionInTTYPE->isChecked();
        }
        if (mSnapshot.dirty(checkBox_mForceMXPProcessorOn)) {
            pHost->setForceMXPProcessorOn(checkBox_mForceMXPProcessorOn->isChecked());
        }
        if (mSnapshot.dirty(mIsToLogInHtml)) {
            pHost->mIsNextLogFileInHtmlFormat = mIsToLogInHtml->isChecked();
        }
        if (mSnapshot.dirty(mIsLoggingTimestamps)) {
            pHost->mIsLoggingTimestamps = mIsLoggingTimestamps->isChecked();
        }
        // The button that picks it has no value of its own, only the text it puts here
        if (mSnapshot.dirty(lineEdit_logFileFolder)) {
            pHost->mLogDir = mLogDirPath;
        }
        if (mSnapshot.dirty(lineEdit_logFileName)) {
            pHost->mLogFileName = lineEdit_logFileName->text();
        }
        if (mSnapshot.dirty(comboBox_logFileNameFormat)) {
            pHost->mLogFileNameFormat = comboBox_logFileNameFormat->currentData().toString();
        }
        if (mSnapshot.dirty(checkBox_antiAlias)) {
            pHost->mNoAntiAlias = !checkBox_antiAlias->isChecked();
        }
        if (mSnapshot.dirty(mAlertOnNewData)) {
            pHost->mAlertOnNewData = mAlertOnNewData->isChecked();
        }

        if (mSnapshot.dirty(telnetHandlerEnabled)) {
            QSettings* settings = MudletApp::getQSettings();
            if (settings->value("telnetHandlerEnabled", false).toBool() != telnetHandlerEnabled->isChecked()) {
                settings->setValue("telnetHandlerEnabled", telnetHandlerEnabled->isChecked());
            }
        }

        if (mSnapshot.dirty(groupBox_proxy)) {
            pHost->mUseProxy = groupBox_proxy->isChecked();
        }
        if (mSnapshot.dirty(lineEdit_proxyAddress)) {
            pHost->mProxyAddress = lineEdit_proxyAddress->text();
        }
        if (mSnapshot.dirty(lineEdit_proxyPort)) {
            pHost->mProxyPort = lineEdit_proxyPort->text().toUInt();
        }
        if (mSnapshot.dirty(lineEdit_proxyUsername)) {
            pHost->mProxyUsername = lineEdit_proxyUsername->text();
        }
        if (mSnapshot.dirty(lineEdit_proxyPassword)) {
            pHost->mProxyPassword = lineEdit_proxyPassword->text();
        }

        //tab security
        if (mSnapshot.dirty(groupBox_ssl)) {
            pHost->mSslTsl = groupBox_ssl->isChecked();
        }
        if (mSnapshot.dirty(checkBox_expired)) {
            pHost->mSslIgnoreExpired = checkBox_expired->isChecked();
        }
        if (mSnapshot.dirty(checkBox_self_signed)) {
            pHost->mSslIgnoreSelfSigned = checkBox_self_signed->isChecked();
        }
        if (mSnapshot.dirty(checkBox_ignore_all)) {
            pHost->mSslIgnoreAll = checkBox_ignore_all->isChecked();
        }
        if (mSnapshot.dirty(checkBox_askTlsAvailable)) {
            pHost->mAskTlsAvailable = checkBox_askTlsAvailable->isChecked();
        }

        if (console) {
            console->changeColors();
        }

        if (mSnapshot.dirty(doubleclick_ignore_lineedit)) {
            const QString lIgnore = doubleclick_ignore_lineedit->text();
            pHost->mDoubleClickIgnore.clear();
            for (auto character : lIgnore) {
                pHost->mDoubleClickIgnore.insert(character);
            }
        }

        if (mSnapshot.dirty(comboBox_mapFileSaveFormatVersion)) {
            pHost->mpMap->mSaveVersion = comboBox_mapFileSaveFormatVersion->currentData().toInt();
        }


        if (console) {
            const int x = console->width();
            const int y = console->height();
            const QSize s = QSize(x, y);
            QResizeEvent event(s, s);
            QApplication::sendEvent(console, &event);
        }

        if (mSnapshot.dirty(checkBox_echoLuaErrors)) {
            pHost->mEchoLuaErrors = checkBox_echoLuaErrors->isChecked();
        }
        if (mSnapshot.dirty(checkBox_useWideAmbiguousEastAsianGlyphs)) {
            pHost->setWideAmbiguousEAsianGlyphs(checkBox_useWideAmbiguousEastAsianGlyphs->checkState());
        }
        if (mSnapshot.dirty(checkBox_enableBlinkText)) {
            pHost->setEnableBlinkText(checkBox_enableBlinkText->isChecked());
        }
        if (mSnapshot.dirty(code_editor_theme_selection_combobox)) {
            if (pMudlet->inDarkMode()) {
                pHost->mEditorThemeDark = code_editor_theme_selection_combobox->currentText();
                pHost->mEditorThemeFileDark = code_editor_theme_selection_combobox->currentData().toString();
            } else {
                pHost->mEditorTheme = code_editor_theme_selection_combobox->currentText();
                pHost->mEditorThemeFile = code_editor_theme_selection_combobox->currentData().toString();
            }
        }
        if (mSnapshot.dirty(checkBox_autocompleteLuaCode)) {
            pHost->mEditorAutoComplete = checkBox_autocompleteLuaCode->isChecked();
        }
        if (mSnapshot.dirty(checkBox_showBidi)) {
            pHost->setEditorShowBidi(checkBox_showBidi->isChecked());
        }
        if (mSnapshot.dirty(checkBox_showIdNumbers)) {
            pHost->setShowIdsInEditor(checkBox_showIdNumbers->isChecked());
        }
        // Re-theming an open script editor is a full edbee reconfiguration, so it
        // waits for one of the settings it carries to actually move
        if (pHost->mpEditorDialog
            && mSnapshot.anyDirty({code_editor_theme_selection_combobox, checkBox_showSpacesAndTabs, checkBox_showLineFeedsAndParagraphs, checkBox_autocompleteLuaCode, checkBox_showBidi})) {
            // The write above settled the choice into the Host, so the name comes
            // from there rather than a box a script may have moved on from
            pHost->mpEditorDialog->setThemeAndOtherSettings(pMudlet->inDarkMode() ? pHost->mEditorThemeDark : pHost->mEditorTheme);
        }

        if (mSnapshot.dirty(script_preview_combobox)) {
            auto data = script_preview_combobox->currentData().value<QPair<QString, int>>();
            pHost->mThemePreviewItemID = data.second;
            pHost->mThemePreviewType = data.first;
        }

        if (mSnapshot.dirty(search_engine_combobox)) {
            pHost->mSearchEngineName = search_engine_combobox->currentText();
        }

        if (mSnapshot.dirty(timeEdit_timerDebugOutputMinimumInterval)) {
            pHost->mTimerDebugOutputSuppressionInterval = timeEdit_timerDebugOutputMinimumInterval->time();
        }

        if (mSnapshot.dirty(comboBox_blankLinesBehaviour)) {
            pHost->mBlankLineBehaviour = static_cast<Host::BlankLineBehaviour>(comboBox_blankLinesBehaviour->currentIndex());
        }

        if (mSnapshot.anyDirty({comboBox_discordSmallIconPrivacy,
                                comboBox_discordLargeIconPrivacy,
                                checkBox_discordServerAccessToDetail,
                                checkBox_discordServerAccessToState,
                                checkBox_discordServerAccessToPartyInfo,
                                checkBox_discordServerAccessToTimerInfo})) {
            // Six controls, one flags word: only the bits whose own control was
            // edited move, so the rest keep what a script has set them to
            Host::DiscordOptionFlags discordFlags = pHost->mDiscordAccessFlags;

            // A privacy combo box carries two bits: "show it" is its first two
            // entries, "show the text with it" only the first
            if (mSnapshot.dirty(comboBox_discordLargeIconPrivacy)) {
                const int privacy = comboBox_discordLargeIconPrivacy->currentIndex();
                discordFlags.setFlag(Host::DiscordSetLargeIcon, privacy == 0 || privacy == 1);
                discordFlags.setFlag(Host::DiscordSetLargeIconText, privacy == 0);
            }
            if (mSnapshot.dirty(comboBox_discordSmallIconPrivacy)) {
                const int privacy = comboBox_discordSmallIconPrivacy->currentIndex();
                discordFlags.setFlag(Host::DiscordSetSmallIcon, privacy == 0 || privacy == 1);
                discordFlags.setFlag(Host::DiscordSetSmallIconText, privacy == 0);
            }
            // These four are ticked to *withhold* the item from the server
            if (mSnapshot.dirty(checkBox_discordServerAccessToDetail)) {
                discordFlags.setFlag(Host::DiscordSetDetail, !checkBox_discordServerAccessToDetail->isChecked());
            }
            if (mSnapshot.dirty(checkBox_discordServerAccessToState)) {
                discordFlags.setFlag(Host::DiscordSetState, !checkBox_discordServerAccessToState->isChecked());
            }
            if (mSnapshot.dirty(checkBox_discordServerAccessToPartyInfo)) {
                discordFlags.setFlag(Host::DiscordSetPartyInfo, !checkBox_discordServerAccessToPartyInfo->isChecked());
            }
            if (mSnapshot.dirty(checkBox_discordServerAccessToTimerInfo)) {
                discordFlags.setFlag(Host::DiscordSetTimeInfo, !checkBox_discordServerAccessToTimerInfo->isChecked());
            }

            pHost->mDiscordAccessFlags = discordFlags;
        }

        if (mSnapshot.anyDirty({radioButton_discordDisabled, radioButton_discordMudletOnly, radioButton_discordGameDetails})) {
            Host::DiscordMode newMode = Host::DiscordShowGameDetails;
            if (radioButton_discordDisabled->isChecked()) {
                newMode = Host::DiscordDisabled;
            } else if (radioButton_discordMudletOnly->isChecked()) {
                newMode = Host::DiscordShowMudletOnly;
            }
            pHost->setDiscordMode(newMode);
        }

        if (mSnapshot.dirty(lineEdit_discordUserName)) {
            const QString newDiscordUserName = lineEdit_discordUserName->text().trimmed().toLower();
            if (pHost->mRequiredDiscordUserName != newDiscordUserName) {
                pHost->mRequiredDiscordUserName = newDiscordUserName;
                Discord::self()->UpdatePresence();
            }
        }

        // Save chat options so they are written to XML upon export
        if (mSnapshot.dirty(lineEdit_mmcpChatName)) {
            pHost->setMMCPChatName(lineEdit_mmcpChatName->text().trimmed());
        }
        if (mSnapshot.dirty(lineEdit_mmcpChatMessagePrefix)) {
            pHost->mMMCPChatPrefix = lineEdit_mmcpChatMessagePrefix->text().trimmed();
        }
        if (mSnapshot.dirty(lineEdit_mmcpPort)) {
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

        if (mSnapshot.dirty(checkBox_mmcpPrefixEmotes)) {
            pHost->mMMCPPrefixEmotes = checkBox_mmcpPrefixEmotes->isChecked();
        }
        if (mSnapshot.dirty(checkBox_mmcpAddChatMessageNewline)) {
            pHost->mMMCPAddChatMessageNewline = checkBox_mmcpAddChatMessageNewline->isChecked();
        }
        if (mSnapshot.dirty(checkBox_mmcpSnoopInMainConsole)) {
            pHost->mMMCPShowSnoopInMainConsole = checkBox_mmcpSnoopInMainConsole->isChecked();
        }
        if (mSnapshot.dirty(checkBox_announceIncomingText)) {
            pHost->mAnnounceIncomingText = checkBox_announceIncomingText->isChecked();
        }
        if (mSnapshot.dirty(checkBox_advertiseScreenReader)) {
            pHost->mAdvertiseScreenReader = checkBox_advertiseScreenReader->isChecked();
        }
        if (mSnapshot.dirty(checkBox_enableOSC8Hyperlinks)) {
            pHost->mEnableOSC8Hyperlinks = checkBox_enableOSC8Hyperlinks->isChecked();
        }
        if (mSnapshot.dirty(checkBox_enableClosedCaption)) {
            pHost->mEnableClosedCaption = checkBox_enableClosedCaption->isChecked();
        }

        if (mSnapshot.dirty(checkBox_expectCSpaceIdInColonLessMColorCode)) {
            pHost->setHaveColorSpaceId(checkBox_expectCSpaceIdInColonLessMColorCode->isChecked());
        }
        if (mSnapshot.dirty(checkBox_allowServerToRedefineColors)) {
            pHost->setMayRedefineColors(checkBox_allowServerToRedefineColors->isChecked());
        }
        if (mSnapshot.dirty(checkBox_debugShowAllCodepointProblems)) {
            pHost->setDebugShowAllProblemCodepoints(checkBox_debugShowAllCodepointProblems->isChecked());
        }
        if (mSnapshot.dirty(comboBox_caretModeKey)) {
            pHost->mCaretShortcut = static_cast<Host::CaretShortcut>(comboBox_caretModeKey->currentIndex());
        }
        if (groupBox_playerRoomStyle->isEnabled() && mSnapshot.anyDirty({comboBox_playerRoomStyle, spinBox_playerRoomOuterDiameter, spinBox_playerRoomInnerDiameter})) {
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
            quint8 styleCode = 0;
            quint8 outerDiameter = 0;
            quint8 innerDiameter = 0;
            QColor liveOuterColor;
            QColor liveInnerColor;
            pHost->getPlayerRoomStyleDetails(styleCode, outerDiameter, innerDiameter, liveOuterColor, liveInnerColor);
            pHost->setPlayerRoomStyleDetails(mSnapshot.dirty(comboBox_playerRoomStyle) ? static_cast<quint8>(comboBox_playerRoomStyle->currentIndex()) : styleCode,
                                             mSnapshot.dirty(spinBox_playerRoomOuterDiameter) ? static_cast<quint8>(spinBox_playerRoomOuterDiameter->value()) : outerDiameter,
                                             mSnapshot.dirty(spinBox_playerRoomInnerDiameter) ? static_cast<quint8>(spinBox_playerRoomInnerDiameter->value()) : innerDiameter,
                                             pHost->mpMap->mPlayerRoomOuterColor,
                                             pHost->mpMap->mPlayerRoomInnerColor);
        }

        if (mSnapshot.shortcutsDirty()) {
            auto iterator = pMudlet->mpShortcutsManager->iterator();
            while (iterator.hasNext()) {
                auto key = iterator.next();
                // Per key for the same reason the value snapshot is per control:
                // the others show what this dialog was populated with
                if (!mSnapshot.shortcutDirty(key)) {
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
    if (mSnapshot.dirty(checkbox_noAutomaticUpdates) && (MudletApp::release() || MudletApp::publicTest() || qEnvironmentVariableIsSet("DEV_UPDATER"))) {
        pMudlet->pUpdater->setAutomaticUpdates(!checkbox_noAutomaticUpdates->isChecked());
    }
#endif

    if (mSnapshot.dirty(MainIconSize)) {
        pMudlet->setToolBarIconSize(MainIconSize->value());
    }
    if (mSnapshot.dirty(TEFolderIconSize)) {
        pMudlet->setEditorTreeWidgetIconSize(TEFolderIconSize->value());
    }
    if (mSnapshot.dirty(comboBox_menuBarVisibility)) {
        pMudlet->setMenuBarVisibility(visibilityFromComboIndex(comboBox_menuBarVisibility->currentIndex()));
    }
    if (mSnapshot.dirty(comboBox_toolBarVisibility)) {
        pMudlet->setToolBarVisibility(visibilityFromComboIndex(comboBox_toolBarVisibility->currentIndex()));
    }

    if (mSnapshot.anyDirty({checkBox_showSpacesAndTabs, checkBox_showLineFeedsAndParagraphs})) {
        const QTextOption::Flags liveOptions = pMudlet->mEditorTextOptions;
        pMudlet->setEditorTextoptions(mSnapshot.dirty(checkBox_showSpacesAndTabs) ? checkBox_showSpacesAndTabs->isChecked() : liveOptions.testFlag(QTextOption::ShowTabsAndSpaces),
                                      mSnapshot.dirty(checkBox_showLineFeedsAndParagraphs) ? checkBox_showLineFeedsAndParagraphs->isChecked()
                                                                                           : liveOptions.testFlag(QTextOption::ShowLineAndParagraphSeparators));
    }
    if (mSnapshot.dirty(checkBox_reportMapIssuesOnScreen)) {
        pMudlet->setShowMapAuditErrors(checkBox_reportMapIssuesOnScreen->isChecked());
    }
    if (mSnapshot.dirty(checkBox_showIconsOnMenus)) {
        pMudlet->setShowIconsOnMenu(checkBox_showIconsOnMenus->checkState());
    }
    if (mSnapshot.dirty(comboBox_appearance)) {
        pMudlet->setAppearance(static_cast<enums::Appearance>(comboBox_appearance->currentIndex()));
    }

    Discord::self()->UpdatePresence();

    emit signal_preferencesSaved();

    // Only what changes after this point is the user's next edit
    mSnapshot.take();

    // ...and with nothing of the user's outstanding, the other moment the dialog
    // can re-read: a write can be refused (a font with no metrics, a value the
    // Host clamps) or answered by a script, so what lands is not what was sent
    refreshFromSettings();
}

void dlgProfilePreferences::slot_scheduleApply()
{
    // A control wired up inside buildShell() arrives before the timer is made
    if (mPopulating || !mpTimer_apply) {
        return;
    }
    // Restarting, so a burst of changes costs one apply rather than one each
    mpTimer_apply->start();
}

void dlgProfilePreferences::slot_lineEditFinished()
{
    // Clearing the modified flag marks this edit finished - see beingTypedInto()
    if (auto* pLineEdit = qobject_cast<QLineEdit*>(sender()); pLineEdit) {
        pLineEdit->setModified(false);
    }
    slot_scheduleApply();
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
    std::list<Tree<TTrigger>*>* childTriggers = pTriggerParent->getChildrenList();
    for (auto* triggerNode : *childTriggers) {
        auto* trigger = static_cast<TTrigger*>(triggerNode);
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
    std::list<Tree<TAlias>*>* childrenList = pAliasParent->getChildrenList();
    for (auto* aliasNode : *childrenList) {
        auto* alias = static_cast<TAlias*>(aliasNode);
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
    std::list<Tree<TTimer>*>* childrenList = pTimerParent->getChildrenList();
    for (auto* timerNode : *childrenList) {
        auto* timer = static_cast<TTimer*>(timerNode);
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
    std::list<Tree<TKey>*>* childrenList = pKeyParent->getChildrenList();
    for (auto* keyNode : *childrenList) {
        auto* key = static_cast<TKey*>(keyNode);
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
    std::list<Tree<TScript>*>* childrenList = pScriptParent->getChildrenList();
    for (auto* scriptNode : *childrenList) {
        auto* script = static_cast<TScript*>(scriptNode);
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
    std::list<Tree<TAction>*>* childrenList = pActionParent->getChildrenList();
    for (auto* actionNode : *childrenList) {
        auto* action = static_cast<TAction*>(actionNode);
        if (!action->getScript().isEmpty()) {
            items.push_back({action->getName(), qsl("button"), action->getID()});
        }

        if (action->hasChildren()) {
            addActionsToPreview(action, items);
        }
    }
}

// A network round trip, so it is made on the first visit to the Editor category only
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

    QSettings& settings = *MudletApp::getQSettings();
    const QString themesURL = settings.value("colorSublimeThemesURL", qsl("https://github.com/Colorsublime/Colorsublime-Themes/archive/master.zip")).toString();
    // a default update period is 24h
    // it would be nice to use C++14's numeric separator but Qt Creator still
    // does not like them for its Clang code model analyser (and the built in
    // one is even less receptive to): 86'400'000
    const int themesUpdatePeriod = settings.value("themesUpdatePeriod", 86400000).toInt();
    // save the defaults in settings so the field is visible for editing in config file if needed
    settings.setValue("colorSublimeThemesURL", themesURL);
    settings.setValue("themesUpdatePeriod", themesUpdatePeriod);

    auto themesAge = QFileInfo(MudletApp::getMudletPath(enums::editorWidgetThemeJsonFile)).lastModified().toUTC();

    // A test visiting the Editor category is otherwise one file modification
    // time away from a live fetch that fails slowly rather than red
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
    request.setRawHeader(QByteArray("User-Agent"), QByteArray(qsl("Mozilla/5.0 (Mudlet/%1%2)").arg(APP_VERSION, MudletApp::buildSuffix()).toUtf8().constData()));
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
                        auto future = QtConcurrent::run(mudlet::unzip, tempThemesArchive->fileName(), MudletApp::getMudletPath(enums::mainDataItemPath, qsl("edbee/")), temporaryDir.path());
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
    QFile themesFile(MudletApp::getMudletPath(enums::editorWidgetThemeJsonFile));
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
    // The list is rebuilt long after the dialog was populated, which is the list
    // changing rather than the user picking anything: one that no longer offers
    // the profile's theme leaves the box on no item, and writing that back
    // would wipe the theme the profile had.
    mSnapshot.take(code_editor_theme_selection_combobox);
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
        // ...and the controls that only exist once there is a profile
        connectApplyTriggers();
        mSnapshot.take();
        guardScrollWheel();
        // ...including the palette fix-ups, for controls built since
        applyShellStyle();
        // ...and after it, so the caps measure cards as the stylesheet leaves them
        updateColumnWidthCaps();
        rebuildTabOrder();
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
        // The wildcard disconnects above took instant apply's connections off
        // with the rest, and Qt::UniqueConnection makes remaking them safe
        connectApplyTriggers();
        // Nothing of what the controls now hold is worth writing back:
        mpTimer_apply->stop();
        mSnapshot.take();

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

    auto detail = Discord::self()->getDetailText(mpHost);
    if (!detail.isEmpty()) {
        detail = qsl("<br/>(\"%1\")").arg(detail);
    }

    auto state = Discord::self()->getStateText(mpHost);
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
    // ...and so has every text the column widths were measured from, including
    // the checkboxes that fit the reading width in the language just left
    updateColumnWidthCaps();
    // ...which can move the width the sidebar needs to stand beside them
    updateSidebarMode();

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

void dlgProfilePreferences::slot_mapSymbolFontFudgeChanged(const double factor)
{
    Host* pHost = mpHost;
    if (!pHost || !pHost->mpMap) {
        return;
    }

    pHost->mpMap->setSymbolFontFudgeFactor(factor);
}

void dlgProfilePreferences::slot_changeMapperShowRoomBorders(const bool state)
{
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }

    pHost->mMapperShowRoomBorders = state;
    if (pHost->mpMap && pHost->mpMap->mpMapper && pHost->mpMap->mpMapper->mp2dMap) {
        pHost->mpMap->mpMapper->mp2dMap->update();
    }
}

void dlgProfilePreferences::slot_changeDrawUpperLowerLevels(const bool state)
{
    mudlet::self()->mDrawUpperLowerLevels = state;
    Host* pHost = mpHost;
    if (pHost && pHost->mpMap && pHost->mpMap->mpMapper && pHost->mpMap->mpMapper->mp2dMap) {
        pHost->mpMap->mpMapper->mp2dMap->update();
    }
}

void dlgProfilePreferences::slot_changeMapperUseAntiAlias(const bool state)
{
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }

    pHost->mMapperUseAntiAlias = state;
    if (pHost->mpMap && pHost->mpMap->mpMapper && pHost->mpMap->mpMapper->mp2dMap) {
        pHost->mpMap->mpMapper->mp2dMap->mMapperUseAntiAlias = state;
        pHost->mpMap->mpMapper->mp2dMap->update();
    }
}

// Progressive disclosure for screen-reader users: surface the hyperlink
// navigation/activation/menu shortcuts at the moment the user picks a
// pane-switching key, so they don't have to consult the wiki to discover them.
// Picking Tab additionally warns about the shared binding.
void dlgProfilePreferences::slot_caretModeKeyChanged(const int index)
{
    if (index < 0 || !QAccessible::isActive()) {
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
}

bool dlgProfilePreferences::updateDisplayFont(const Host::DisplayFontChange change)
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
    mpHost->setDisplayFont(displayFont, change);

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
    // Only fires from QFontComboBox::currentFontChanged, so the family really is
    // one the user just picked out of the list
    if (!mpHost.isNull() && updateDisplayFont(Host::DisplayFontChange::UserChoice)) {
        mpHost->mTelnet.sendInfoNewEnvironValue(qsl("FONT"));
    }
}

void dlgProfilePreferences::slot_displayFontSizeChanged()
{
    if (!mpHost.isNull() && updateDisplayFont(Host::DisplayFontChange::Adjustment)) {
        mpHost->mTelnet.sendInfoNewEnvironValue(qsl("FONT_SIZE"));
    }
}

void dlgProfilePreferences::slot_displayFontAliasingChanged()
{
    updateDisplayFont(Host::DisplayFontChange::Adjustment);
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
    // Esc goes up a level before it goes out of the dialog, and search results
    // are a level of their own. Clearing the query is what leaves them: it hands
    // the borrowed cards back and returns to the page the search interrupted.
    if (!mClosing && mSearchActive) {
        mpLineEdit_search->clear();
        return;
    }
    // ...and on a subpage it means the same as the back chevron
    if (!mClosing && !mCurrentSubpage.isEmpty()) {
        leaveSubpage();
        return;
    }
    // QDialog::reject() hides the dialog without ever sending a close event, so
    // Esc is routed through close() - which arrives back here from
    // QDialog::closeEvent() with nothing left to do but the base class
    if (!mClosing) {
        close();
        return;
    }
    QDialog::reject();
}

void dlgProfilePreferences::closeEvent(QCloseEvent* event)
{
    // Raised here so it covers the whole close: the apply below ends by
    // re-reading the settings, which a dialog on its way out has no use for
    mClosing = true;
    cancelShortcutCaptures();

    if (mpDialogMapGlyphUsage) {
        mpDialogMapGlyphUsage->close();
        mpDialogMapGlyphUsage = nullptr;
    }

    // Closing is not a discard, and the debounce will not outlive the dialog.
    // Clearing the focus makes a field being typed into report as finished.
    if (QWidget* pFocus = focusWidget(); pFocus) {
        pFocus->clearFocus();
    }
    mpTimer_apply->stop();
    applyAll();

    // The profile XML is written once per close rather than on every apply:
    if (Host* pHost = mpHost; pHost && pHost->mFORCE_SAVE_ON_EXIT) {
        pHost->saveProfile();
    }

    MudletApp::getQSettings()->setValue(qsl("profilePreferencesGeometry"), saveGeometry());

    if (mpHost) {
        emit preferencesClosing(mpHost->getName());
    }
    QDialog::closeEvent(event);
    mClosing = false;
}
