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

#include "AlphaColourWell.h"
#include "CredentialManager.h"
#include "EditorAutoCompleteFocusHandler.h"
#include "FlowLayout.h"
#include "GMCPAuthenticator.h"
#include "Host.h"
#include "HostDialogs.h"
#include "HostManager.h"
#include "MapStyle.h"
#include "MudletApp.h"
#include "MudletProxyStyle.h"
#include "PairGridLayout.h"
#include "RoomAppearance.h"
#include "SidebarItemDelegate.h"
#include "SidebarToggle.h"
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
#include "TriggerMatchPool.h"
#include "TTrigger.h"
#include "ctelnet.h"
#include "discord.h"
#include "dlgMapper.h"
#include "dlgTriggerEditor.h"
#include "edbee/views/texteditorscrollarea.h"
#include "MMCP.h"
#include "uiDesign.h"
#include "utils.h"
#include "widgetutils.h"

#ifdef INCLUDE_MCPSERVER
#include "TMCPBridge.h"
#include "TMCPServer.h"
#include <QClipboard>
#include <QGuiApplication>
#endif

#include <chrono>
#include <filesystem>
#include <vector>
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
#include <QFileInfo>
#include <QFontDatabase>
#include <QFontDialog>
#include <QFutureWatcher>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QMenu>
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
#include <QThread>
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
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QShortcut>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QStyle>
#include <QToolButton>
#include <QVariantAnimation>
#include "../3rdparty/kdtoolbox/singleshot_connect/singleshot_connect.h"

using namespace std::chrono_literals;

using uiDesign::alignInLayoutTree;
using uiDesign::blend;
using uiDesign::buttonStyleSheet;
using uiDesign::cardIndicatorStyleSheet;
using uiDesign::CardMetrics;
using uiDesign::cardStyleSheet;
using uiDesign::choiceStyleSheet;
using uiDesign::collectFocusableInLayoutOrder;
using uiDesign::collectSearchText;
using uiDesign::detachFromLayout;
using uiDesign::foldForSearch;
using uiDesign::highlightTextOf;
using uiDesign::inlineGlyph;
using uiDesign::inputStyleSheet;
using uiDesign::insertGridRowAtTop;
using uiDesign::invalidateLayoutsUpTo;
using uiDesign::keepClickFocusOffControls;
using uiDesign::letPopupsTakeTheFieldsCorner;
using uiDesign::makeChevronRow;
using uiDesign::markAsShellSurface;
using uiDesign::measuredCardTitleHeight;
using uiDesign::menuStyleSheet;
using uiDesign::readableOn;
using uiDesign::rgba;
using uiDesign::scmProp_focused;
using uiDesign::scmProp_searchKeywords;
using uiDesign::scmProp_settingsCard;
using uiDesign::scmProp_settingsCardPlain;
using uiDesign::scmRadiusPanel;
using uiDesign::scmRadiusProminentInput;
using uiDesign::scrollBarStyleSheet;
using uiDesign::setSearchMatch;
using uiDesign::SettingsSnapshot;
using uiDesign::SidebarItemDelegate;
using uiDesign::sidebarStyleSheet;
using uiDesign::spotlightStyleSheet;
using uiDesign::themeTokens;
using uiDesign::ThemeTokens;
using uiDesign::tintedGlyph;
using uiDesign::typeSize;
using uiDesign::TypeStep;
using uiDesign::withLinkColour;
using uiDesign::wordEnoughToSearch;

// The text a rich-text label was given, before the colour of any link in it was
// written in. A QLabel bakes an anchor's colour into its document at the moment
// the text is set, from the application palette of that moment, so the only way
// to re-ink one when the appearance moves is to set the text again - which
// means keeping what it was.
static constexpr char scmProp_settingsRichText[] = "settingsRichText";

// A reading width: a wide window gains whitespace rather than stretching the controls
static constexpr int scmContentColumnWidth = 640;
// The narrowest the window goes, however little its pages need
static constexpr int scmMinimumWindowWidth = 780;

// Above and below the sidebar's rows. The one measurement this dialog's sidebar
// keeps of its own: it is the inset this window's own columns start at, the way
// the editor's is the inset of the editor's - and this sidebar leads with the
// wordmark row rather than with its first name. Everything else comes from the
// numbers the two windows share in uiDesign.h.
static constexpr int scmSidebarVerticalPadding = 16;
// The widest the names are worth drawing, which is the flat width this sidebar
// was held to before it measured itself. A ceiling now rather than the answer.
static constexpr int scmSidebarMaximumWidth = 232;

// The one measurement that is not a constant is what the sidebar is drawn at
// with the names showing, since that is the longest of the category names - see
// measuredSidebarWidth()
static uiDesign::SidebarMetrics settingsSidebarMetrics(const int expandedWidth)
{
    return {.expandedWidth = expandedWidth,
            .railWidth = uiDesign::scmSidebarRailWidth,
            .padding = uiDesign::scmSidebarPadding,
            .railPadding = uiDesign::scmSidebarRailPadding,
            .verticalPadding = scmSidebarVerticalPadding,
            .separatorInset = uiDesign::scmSidebarSeparatorInset};
}
// What a card leaves round what it holds - and, since the title is the first
// line inside the frame rather than a heading above it, how far in from the
// frame the title starts as well
static constexpr int scmCardPadding = 16;

// ...which, with the property the rules select on and the two variants this
// dialog has that the editor's column does not, is the whole of what its cards
// differ from the editor's by; everything else about them is drawn by
// uiDesign::cardStyleSheet(). The height of the title's own line is measured
// under the font the dialog is running at, so it is the one runtime number here.
static CardMetrics cardMetrics(const int titleHeight)
{
    return {.cardProperty = scmProp_settingsCard, .plainProperty = scmProp_settingsCardPlain, .padding = scmCardPadding, .titleHeight = titleHeight, .flattenNestedGroupBoxes = true};
}

// Between a label and the control it names on a row that flows, and between
// two lines of one
static constexpr int scmFlowRowGap = 8;
// ...between two columns of check boxes, so they read as two columns rather
// than as one line of words running across
static constexpr int scmChoiceGroupGap = 40;
// ...and between two label-and-stepper pairs, wider than the gap inside a pair
// so each label is read with its own stepper
static constexpr int scmFieldPairGap = 24;

// A label and the controls it names, each at its own width, on one line while
// they fit and wrapping after the last piece that does. A grid sizes every
// column by its longest member, so one long translation widened every row.
static QWidget* makeFlowRow(QWidget* pParent,
                            const QString& objectName,
                            const QList<QWidget*>& pieces,
                            const int gapAcross = scmFlowRowGap,
                            const uiDesign::FlowLayout::LineAlignment alignment = uiDesign::FlowLayout::LineAlignment::Centre)
{
    auto* pRow = new QWidget(pParent);
    pRow->setObjectName(objectName);
    auto* pFlow = new uiDesign::FlowLayout(pRow, gapAcross, scmFlowRowGap);
    pFlow->setContentsMargins(0, 0, 0, 0);
    // A row in the reading column must not widen it by the length of the line
    pFlow->setWidthPolicy(uiDesign::FlowLayout::Width::WidestItem);
    pFlow->setLineAlignment(alignment);
    // A layout whose height depends on its width is only asked for one if the widget carrying it says so
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    policy.setHeightForWidth(true);
    pRow->setSizePolicy(policy);
    for (auto* pPiece : pieces) {
        detachFromLayout(pPiece);
        pFlow->addWidget(pPiece);
    }
    return pRow;
}

// Controls that belong together - a label and its stepper, a column of check
// boxes - as one piece, which a flow moves to the next line whole
static QWidget* makeFlowPiece(QWidget* pParent, const QString& objectName, const QBoxLayout::Direction direction, const QList<QWidget*>& controls)
{
    auto* pPiece = new QWidget(pParent);
    pPiece->setObjectName(objectName);
    auto* pLayout = new QBoxLayout(direction, pPiece);
    pLayout->setContentsMargins(0, 0, 0, 0);
    pLayout->setSpacing(scmFlowRowGap);
    for (auto* pControl : controls) {
        detachFromLayout(pControl);
        pLayout->addWidget(pControl);
    }
    return pPiece;
}

// What the shortcut conflict warning is written in: the one red anything broken
// is written in, rather than the raw hue this used to take with no readability
// walk at all. Walked on against the card it lies on, since that surface is
// lighter than the one the red was measured against and these are words.
static QString shortcutConflictStyleSheet()
{
    const ThemeTokens tokens = themeTokens();
    return qsl("color: %1; font-weight: bold;").arg(readableOn(tokens.card, uiDesign::errorInk(tokens), tokens.text, uiDesign::scmTextMinimumRatio).name());
}

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

// Long enough that a burst of keystrokes costs one search, short enough not to lag at the end of a word
static constexpr auto scmSearchDebounce = 150ms;

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

    // The theme/font preview below has autocomplete switched on like the script
    // editor's own widget, so it needs the same treatment: without it edbee's
    // completion list takes the keyboard focus while its popup is open
    // (see #5310). Done here rather than in loadEditorTab() so it holds even
    // when that returns early for want of a profile.
    new EditorAutoCompleteFocusHandler(edbeePreviewWidget, this);
    // The preview completes Lua like the editor, which may not have been built yet
    dlgTriggerEditor::ensureLuaAutoCompleteProvider();

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

#ifdef INCLUDE_MCPSERVER
    connect(checkBox_enableMCPServer, &QCheckBox::toggled, this, &dlgProfilePreferences::slot_updateMCPServerEndpoint);
    connect(spinBox_mcpServerPort, qOverload<int>(&QSpinBox::valueChanged), this, &dlgProfilePreferences::slot_updateMCPServerEndpoint);
    connect(pushButton_connectClaudeDesktop, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_connectClaudeDesktop);
    connect(pushButton_connectChatGpt, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_connectChatGpt);
    // With Mudlet registered a button opens this menu instead of emitting clicked()
    auto* pMenu_claudeDesktop = new QMenu(pushButton_connectClaudeDesktop);
    pMenu_claudeDesktop->setObjectName(qsl("menu_claudeDesktop"));
    connect(pMenu_claudeDesktop->addAction(QString()), &QAction::triggered, this, &dlgProfilePreferences::slot_removeClaudeDesktop);
    auto* pMenu_chatGpt = new QMenu(pushButton_connectChatGpt);
    pMenu_chatGpt->setObjectName(qsl("menu_chatGpt"));
    connect(pMenu_chatGpt->addAction(QString()), &QAction::triggered, this, &dlgProfilePreferences::slot_removeChatGpt);
    connect(pushButton_copyMCPServerAddress, &QAbstractButton::clicked, this, &dlgProfilePreferences::slot_copyMCPServerAddress);
    connect(pushButton_mcpConnectManually, &QAbstractButton::toggled, widget_mcpManualConnect, &QWidget::setVisible);
    connect(pushButton_mcpAdvanced, &QAbstractButton::toggled, this, [this](const bool revealed) {
        widget_mcpAdvanced->setVisible(revealed);
        pushButton_mcpAdvanced->setArrowType(revealed ? Qt::DownArrow : Qt::RightArrow);
    });
#else
    groupBox_mcpServer->hide();
#endif

    // As we demonstrate the options that these next two checkboxes control in
    // the editor "preview" widget (on another tab) we will need to track
    // changes and update the edbee widget straight away. As we can have
    // multiple profiles each with a separate instance of this form open we also
    // have to respond to changes in the settings when *another* profile saves
    // them.
    populateApplicationSettings();

    connect(checkBox_monospacedFontsOnly, &QCheckBox::toggled, this, [this]() {
        showDisplayFont(fontComboBox_displayFont->currentFont());
    });

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
        connect(pHost, &Host::signal_profileStyleSheetChanged, this, &dlgProfilePreferences::setStyleSheet);
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
    connect(pMudlet, &mudlet::signal_colouredIconsChanged, this, &dlgProfilePreferences::slot_changeColouredIcons);
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
    label_editorFontInvalid->hide();
    label_editorFontVariableWidth->hide();

    comboBox_guiLanguage->clear();
    for (auto& code : pMudlet->getAvailableTranslationCodes()) {
        auto& translation = pMudlet->mTranslationsMap[code];
        auto& nativeName = translation.getNativeName();
        if (translation.fromResourceFile()) {
            auto& translatedPc = translation.getTranslatedPercentage();
            if (translatedPc >= MudletApp::scmTranslationGoldStar) {
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
    // Entries are named in their own language, but a search for a language the user does not read is typed
    // in one they do, and the card shows no English names
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

    // After the stylesheet, which brings the card padding and text weight the wrapping must measure. It is
    // also the first point every control is on its final card, which the wrapping waits for.
    mShellReady = true;
    fitColumns();
    rebuildTabOrder();

    setMinimumSize(scmMinimumWindowWidth, 560);
    const auto geometry = MudletApp::getQSettings()->value(qsl("profilePreferencesGeometry")).toByteArray();
    if (geometry.isEmpty() || !restoreGeometry(geometry)) {
        // No wider than the names beside the widest page's column, the width the window was held to when that
        // was as wide as it went
        const int fullyExpanded = sidebarWidths().fullyExpanded;
        resize(fullyExpanded ? std::min(1060, fullyExpanded) : 1060, 760);
    }
    // resize() on a never-shown dialog does not always deliver a resize event
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

static QLabel* wrapLabelOf(const QCheckBox* pCheckBox)
{
    QWidget* pContainer = pCheckBox->parentWidget();
    if (!pContainer || pContainer->objectName() != qsl("settingsCheckBoxWrap")) {
        return nullptr;
    }
    return pContainer->findChild<QLabel*>(QString(), Qt::FindDirectChildrenOnly);
}

// The column is never capped: a page grows with the window and its cards use the room, and the reading column
// is only the floor sidebarWidths() folds the names at. What is left to do when the cards change is the check
// box wrapping, measured against that floor.
void dlgProfilePreferences::fitColumn(QScrollArea* pScrollArea)
{
    QWidget* pColumn = pScrollArea ? pScrollArea->widget() : nullptr;
    if (!pColumn || !pColumn->layout()) {
        return;
    }
    pColumn->layout()->activate();
    fitCheckBoxesToColumn(pColumn);
}

// Unwraps everything first, so the language now showing is what gets measured
void dlgProfilePreferences::fitCheckBoxesToColumn(QWidget* pColumn)
{
    // The constructor is still moving controls between cards, and one moved out of a fresh wrap would
    // leave the empty wrap behind
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

// The QCheckBox stays the control the apply triggers, snapshot and tests know, and only hands its text
// to a QLabel, which can wrap. Its accessible name keeps the text.
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
        // Otherwise the label's height-for-width stops at the container and the row gets one line of height
        QSizePolicy policy = pContainer->sizePolicy();
        policy.setHeightForWidth(true);
        pContainer->setSizePolicy(policy);
        QLayoutItem* pTakenOut = pParentLayout->replaceWidget(pCheckBox, pContainer, Qt::FindChildrenRecursively);
        if (!pTakenOut) {
            delete pContainer;
            return;
        }
        delete pTakenOut;
        // Otherwise the container stretches to the row's tallest control and the top-aligned checkbox floats
        // above its neighbours. Vertical only, so the label keeps the whole row width to wrap in.
        alignInLayoutTree(pParentLayout, pContainer, Qt::AlignVCenter);
        auto* pRowLayout = new QHBoxLayout(pContainer);
        pRowLayout->setContentsMargins(0, 0, 0, 0);
        pRowLayout->setSpacing(style()->pixelMetric(QStyle::PM_CheckBoxLabelSpacing, nullptr, pCheckBox));
        pLabel = new QLabel(pContainer);
        pLabel->setObjectName(qsl("settingsWrappedLabel"));
        pLabel->setWordWrap(true);
        // Clicking the label toggles the checkbox - see eventFilter()
        pLabel->installEventFilter(this);
        pLabel->setBuddy(pCheckBox);
        // The host enable/disable lists name the checkbox, so the label follows its enabled state
        pCheckBox->installEventFilter(this);
        // Level with the label's first line, not its middle
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

// The container stays, as a checkbox that fits one language may not fit the next
void dlgProfilePreferences::unwrapCheckBox(QCheckBox* pCheckBox)
{
    QLabel* pLabel = wrapLabelOf(pCheckBox);
    if (!pLabel || pLabel->text().isEmpty()) {
        return;
    }
    // Only if the checkbox has no text: a language change writes the new text straight onto it, while the
    // label still holds the previous language
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

// setupUi()'s controls are moved into the shell, not recreated: the host enable/disable lists,
// translations and tests hold pointers to them.
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

    // The control that gives the sidebar's names up rides on the seam between
    // the sidebar and the settings beside it, so it is a child of the shell
    // holding both rather than of either of them - which would clip the half of
    // it that overlaps the other
    mpToggle_sidebar = new uiDesign::SidebarToggle(mpWidget_sidebar, mpWidget_shell);
    mpToggle_sidebar->setObjectName(qsl("settingsSidebarToggle"));
    connect(mpToggle_sidebar, &QAbstractButton::clicked, this, [this]() {
        setSidebarLabelsShown(!mSidebarLabelsShown);
    });

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
    // Hidden on a category page, where it would push the title sideways
    mpButton_searchBack = new QToolButton(pTitleRow);
    mpButton_searchBack->setObjectName(qsl("settingsSearchBack"));
    mpButton_searchBack->setArrowType(Qt::LeftArrow);
    mpButton_searchBack->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    mpButton_searchBack->setAutoRaise(true);
    // A tool button is not keyboard-focusable by default
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
    // Fixed, so the title starts at the same x whatever the icon's shape
    mpLabel_pageTitleIcon->setFixedWidth(20);
    pTitleRowLayout->addWidget(mpLabel_pageTitleIcon);
    mpLabel_pageTitle = new QLabel(pTitleRow);
    mpLabel_pageTitle->setObjectName(qsl("settingsPageTitle"));
    pTitleRowLayout->addWidget(mpLabel_pageTitle);
    pTitleRowLayout->addStretch(1);
    pContentLayout->addWidget(pTitleRow);

    // Tab skips disabled controls and screen readers only call them unavailable, so with no profile the
    // explanation has to be a Tab stop itself
    mpLabel_noProfileNotice = new QLabel(pContent);
    mpLabel_noProfileNotice->setObjectName(qsl("settingsNoProfileNotice"));
    mpLabel_noProfileNotice->setWordWrap(true);
    mpLabel_noProfileNotice->setMaximumWidth(scmContentColumnWidth);
    mpLabel_noProfileNotice->setFocusPolicy(Qt::TabFocus);
    mpLabel_noProfileNotice->hide();
    pContentLayout->addWidget(mpLabel_noProfileNotice);

    mpStackedWidget_categories = new QStackedWidget(pContent);
    mpStackedWidget_categories->setObjectName(qsl("settingsStack"));
    // The page on show is part of the breakpoint - see sidebarWidths()
    connect(mpStackedWidget_categories, &QStackedWidget::currentChanged, this, &dlgProfilePreferences::updateSidebarMode);
    pContentLayout->addWidget(mpStackedWidget_categories, 1);

    auto* pFindShortcut = new QShortcut(QKeySequence::Find, this);
    connect(pFindShortcut, &QShortcut::activated, this, [this]() {
        mpLineEdit_search->setFocus(Qt::ShortcutFocusReason);
        mpLineEdit_search->selectAll();
    });

    retitleCards();
    reflowWideCards();
    reflowMapperCards();
    reflowColourCards();
    reflowGeneralCards();
    reflowPrivacyCards();

    auto* pCard_systemIntegration = createCard(qsl("card_systemIntegration"));
    moveIntoCard(pCard_systemIntegration, {telnetHandlerEnabled, checkBox_showIconsOnMenus});
    buildMigrationBanner();
    QList<QWidget*> generalCards{groupBox_miscellaneous, groupBox_encoding, groupBox_logOptions, groupbox_searchEngineSelection, groupBox_updates, pCard_systemIntegration};
#ifdef INCLUDE_MCPSERVER
    buildMcpSummaryCard();
    generalCards.insert(generalCards.indexOf(pCard_systemIntegration), mpCard_mcpAssistant);
#endif
    buildCategoryPage(scmCategory_general, generalCards);
#ifdef INCLUDE_MCPSERVER
    addSubpage(scmCategory_general, qsl("mcp"), mpCard_mcpAssistant, {groupBox_mcpServer});
#endif

    auto* pCard_theme = createCard(qsl("card_theme"));
    addCardRow(pCard_theme, label_appearance, comboBox_appearance);
    auto* pCard_profileTabs = createCard(qsl("card_profileTabs"));
    moveIntoCard(pCard_profileTabs, {checkBox_showTabConnectionIndicators});
    buildCategoryPage(scmCategory_appearance, {pCard_theme, groupBox_iconsAndToolbars, pCard_profileTabs});

    // groupBox_doubleClick's two controls read better as one more display option than as a card
    moveIntoCard(groupBox_displayOptions, {doubleclick_ignore_label, doubleclick_ignore_lineedit, checkBox_enableOSC8Hyperlinks});
    groupBox_doubleClick->hide();
    reflowDisplayOptionsCard();
    buildCategoryPage(scmCategory_mainDisplay, {groupBox_font, groupBox_displayColors, groupBox_borders, groupBox_wrapping, groupBox_consoleBuffer, groupBox_displayOptions});

    buildCategoryPage(scmCategory_inputLine, {groupBox_input, groupBox_spellCheck});

    moveIntoCard(groupBox_autoComplete, {checkBox_echoLuaErrors});
    buildCategoryPage(scmCategory_editor, {groupbox_codeEditorThemeSelection, groupBox_editorFont, groupBox_autoComplete, groupBox_editorDisplayOptions});

    buildCategoryPage(scmCategory_mapper, {groupBox_mapFiles, groupBox_downloadMapOptions, groupBox_mapViewOptions, groupBox_mapperColors, groupBox_playerRoomStyle});

    buildDiscordSummaryCard();
    buildCategoryPage(scmCategory_chat, {mpCard_discord, groupBox_MMCPOptions});
    addSubpage(scmCategory_chat, qsl("discord"), mpCard_discord, {groupBox_discordPrivacy});

    auto* pCard_dataEncoding = createCard(qsl("card_dataEncoding"));
    addCardRow(pCard_dataEncoding, label_encoding, comboBox_encoding);
    moveIntoCard(groupBox_specialOptions, {checkBox_USE_IRE_DRIVER_BUGFIX, checkBox_expectCSpaceIdInColonLessMColorCode});
    reflowCompatibilityCard();
    auto* pCard_network = createCard(qsl("card_network"));
    addCardRow(pCard_network, label_networkPacketTimeout, doubleSpinBox_networkPacketTimeout);
    buildProtocolsSubpage();
    buildCategoryPage(scmCategory_connection, {groupBox_protocols, pCard_dataEncoding, groupBox_specialOptions, pCard_network});
    addSubpage(scmCategory_connection, qsl("protocols"), groupBox_protocols, {mpCard_protocolList});

    auto* pCard_passwords = createCard(qsl("card_passwords"));
    pCard_passwords->layout()->addWidget(makeFlowRow(pCard_passwords, qsl("settingsPrivacyRow_storePasswordsIn"), {label_store_passwords_in, comboBox_store_passwords_in}));
    moveIntoCard(pCard_passwords, {label_password_migration_notification, pushButton_forgetSavedSignIn});
    auto* pCard_serverPermissions = createCard(qsl("card_serverPermissions"));
    moveIntoCard(pCard_serverPermissions, {acceptServerGUI, acceptServerMedia});
    auto* pCard_crashReports = createCard(qsl("card_crashReports"));
    pCard_crashReports->layout()->addWidget(makeFlowRow(pCard_crashReports, qsl("settingsPrivacyRow_crashReportPolicy"), {label_crashReportPolicy, comboBox_crashReportPolicy}));
    // Not inside groupBox_ssl: a checkable group box disables its children when unchecked, and this
    // reminder matters most with TLS off
    auto* pCard_secureReminder = createCard(qsl("card_secureConnectionReminder"));
    pCard_secureReminder->setProperty(scmProp_settingsCardPlain, true);
    moveIntoCard(pCard_secureReminder, {checkBox_askTlsAvailable});
    buildSecurityStatusCard();
    buildCategoryPage(scmCategory_privacy,
                      {mpCard_securityStatus, groupBox_ssl, pCard_secureReminder, groupBox_proxy, pCard_passwords, pCard_serverPermissions, groupBox_purgeMediaCache, pCard_crashReports});

    auto* pCard_captions = createCard(qsl("card_accessibilityText"));
    addCardRow(pCard_captions, label_blankLinesBehaviour, comboBox_blankLinesBehaviour);
    moveIntoCard(pCard_captions, {checkBox_enableBlinkText, checkBox_enableClosedCaption});
    auto* pCard_keyboard = createCard(qsl("card_accessibilityKeyboard"));
    addCardRow(pCard_keyboard, label_caretModeKey, comboBox_caretModeKey);
    addCardRow(pCard_keyboard, label_menuBarAltKeyNavigation, comboBox_menuBarAltKeyNavigation);
#if defined(Q_OS_MACOS)
    // The native menu bar takes no part in Alt key navigation
    label_menuBarAltKeyNavigation->hide();
    comboBox_menuBarAltKeyNavigation->hide();
#endif
    moveIntoCard(pCard_keyboard, {checkBox_f3SearchEnabled});
    buildCategoryPage(scmCategory_accessibility, {groupBox_accessibility, pCard_captions, pCard_keyboard});

    buildCategoryPage(scmCategory_shortcuts, {groupBox_main_window_shortcuts});

    auto* pCard_performance = createCard(qsl("card_performance"));
    moveIntoCard(pCard_performance, {checkBox_lazyCaptureGlobals});
    createTriggerMatchPoolControls(pCard_performance);
    buildExperimentsCard();
    buildCategoryPage(scmCategory_advanced, {pCard_performance, groupBox_debug, mpCard_experiments});

    buildSearchResultsPage();

    connect(mpListWidget_categories, &QListWidget::currentRowChanged, this, &dlgProfilePreferences::slot_categorySelected);
    // Lets a keyboard user follow the support link with Return
    connect(mpListWidget_categories, &QListWidget::itemActivated, this, &dlgProfilePreferences::slot_sidebarItemClicked);
    connect(mpListWidget_categories, &QListWidget::itemClicked, this, [this](QListWidgetItem* pItem) {
        // Where a single click activates, the connection above has already opened the browser
        if (!mpListWidget_categories->style()->styleHint(QStyle::SH_ItemView_ActivateItemOnSingleClick, nullptr, mpListWidget_categories)) {
            slot_sidebarItemClicked(pItem);
        }
    });
    connect(mpLineEdit_search, &QLineEdit::textChanged, this, &dlgProfilePreferences::queueSearch);

    retranslateShell();

    // Otherwise the first page shows with nothing selected; setTab() overrides this for a deep link
    showCategory(scmCategory_general);
}

// mpFrame_migrationBanner stays null once dismissed
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
    // Aligns with the cards' text, which is inset by stylesheet padding plus layout margins
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
        // Cleared so no later page switch brings it back. Not deleted: the click is still being delivered.
        placeBannerOn(nullptr);
        mpFrame_migrationBanner = nullptr;
    });
}

// Pinned above the stack it would take 130px of every page at the 780x560 minimum, so it rides at the top
// of the page showing. Removed while search owns the stack: the index records each card's column
// position, which a banner above would offset.
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
    // Reparenting hides a widget, and the page may not be showing yet
    mpFrame_migrationBanner->show();
}

void dlgProfilePreferences::updateNoProfileNotice()
{
    mpLabel_noProfileNotice->setVisible(!mpHost);
    // Screen readers can read a dialog's description out as it opens, before Tab would reach the notice
    setAccessibleDescription(mpHost ? QString() : mpLabel_noProfileNotice->text());
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
    // Stretches either side centre the empty state; results go between the label and the trailing stretch
    mpLayout_searchResults->addStretch(0);
    mpLayout_searchResults->addWidget(mpLabel_searchEmpty);
    mpLayout_searchResults->addStretch(1);
    mSearchResultsPageIndex = mpStackedWidget_categories->addWidget(mpScrollArea_searchResults);
}

QList<dlgProfilePreferences::CategoryDefinition> dlgProfilePreferences::categoryDefinitions() const
{
    return {//: Sidebar category in the settings dialog, holding saving, language, logging, web search and update options
            {scmCategory_general, qsl("settings-general.svg"), tr("General")},
            //: Sidebar category in the settings dialog, holding the theme, icon sizes and profile tab options
            {scmCategory_appearance, qsl("settings-appearance.svg"), tr("Appearance")},
            //: Sidebar category in the settings dialog, holding the font, colors, borders and wrapping of the game's text window
            {scmCategory_mainDisplay, qsl("settings-display.svg"), tr("Main display")},
            //: Sidebar category in the settings dialog, holding the options of the command line the player types into
            {scmCategory_inputLine, qsl("settings-input.svg"), tr("Input line")},
            //: Sidebar category in the settings dialog, holding the script editor's options
            {scmCategory_editor, qsl("settings-editor.svg"), tr("Editor")},
            //: Sidebar category in the settings dialog, holding the map's files, view and colors
            {scmCategory_mapper, qsl("settings-mapper.svg"), tr("Mapper")},
            //: Sidebar category in the settings dialog, holding the Discord Rich Presence and MudMaster chat options
            {scmCategory_chat, qsl("settings-chat.svg"), tr("Chat and sharing")},
            //: Sidebar category in the settings dialog, holding the game protocol, encoding and compatibility options
            {scmCategory_connection, qsl("settings-connection.svg"), tr("Connection"), true},
            //: Sidebar category in the settings dialog, holding the secure connection, proxy, password and permission options
            {scmCategory_privacy, qsl("settings-privacy.svg"), tr("Privacy and security")},
            //: Sidebar category in the settings dialog, holding the screen reader and other accessibility options
            {scmCategory_accessibility, qsl("settings-accessibility.svg"), tr("Accessibility")},
            //: Sidebar category in the settings dialog, holding the main window's keyboard shortcuts
            {scmCategory_shortcuts, qsl("settings-shortcuts.svg"), tr("Shortcuts")},
            //: Sidebar category in the settings dialog, holding development and diagnostic options
            {scmCategory_advanced, qsl("settings-advanced.svg"), tr("Advanced")}};
}

QWidget* dlgProfilePreferences::buildSidebar()
{
    // Named for what it is: the sidebar has no closed state to remember, only
    // whether its rows are labelled. A dialog that has never been told opens
    // with the names, as the editor does, and the chevron on the seam minimises
    // them to a rail of icons for whoever wants the window for the settings.
    mSidebarLabelsShown = MudletApp::getQSettings()->value(qsl("settingsSidebarLabelsShown"), true).toBool();

    auto* pSidebar = new QWidget(mpWidget_shell);
    mpWidget_sidebar = pSidebar;
    pSidebar->setObjectName(qsl("settingsSidebar"));
    // The width with the names showing is measured off those names, which are
    // put there by retranslateShell() further down the build - so the pane
    // starts as a rail and updateSidebarMode() gives it the width it wants
    pSidebar->setFixedWidth(uiDesign::scmSidebarRailWidth);
    auto* pSidebarLayout = new QVBoxLayout(pSidebar);
    pSidebarLayout->setContentsMargins(uiDesign::scmSidebarPadding, scmSidebarVerticalPadding, uiDesign::scmSidebarPadding, scmSidebarVerticalPadding);
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
    mpListWidget_categories->setIconSize(QSize(uiDesign::scmSidebarIconSize, uiDesign::scmSidebarIconSize));
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
    mpItem_support->setSizeHint(QSize(0, uiDesign::scmSidebarRowHeight));

    return pSidebar;
}

void dlgProfilePreferences::addCategory(const QString& key, const QString& iconFile)
{
    // No icon yet: the single-colour set is tinted once applyShellStyle() reads the theme off the palette
    auto* pItem = new QListWidgetItem(QString(), mpListWidget_categories);
    pItem->setData(scmRole_categoryKey, key);
    pItem->setSizeHint(QSize(0, uiDesign::scmSidebarRowHeight));
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
    // The names the sidebar's width is measured from have just been replaced,
    // and so has the wordmark over them
    invalidateSidebarWidth();
    // ...and the chevron beside them says what it does in words
    updateSidebarToggle();

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
    //: Card title on the Advanced settings page, above options that trade compatibility for speed
    cardTitles.append({qsl("card_performance"), tr("Performance")});
    //: Card title on the Advanced settings page, above the checkboxes that switch unfinished features on for this profile
    cardTitles.append({qsl("card_experiments"), tr("Experiments (unsupported)")});
    //: Card title on the Chat and sharing settings page, above the row leading to the Discord Rich Presence settings
    cardTitles.append({qsl("card_discord"), tr("Discord Rich Presence")});
    //: Card title on the game protocols subpage, above the ten protocols Mudlet can offer the game
    cardTitles.append({qsl("card_protocolList"), tr("Protocols to offer the game")});
    //: Card title on the Accessibility settings page, above the options for blank lines, blinking text and captions
    cardTitles.append({qsl("card_accessibilityText"), tr("Text and media")});
    //: Card title on the Accessibility settings page, above the options for moving around Mudlet from the keyboard
    cardTitles.append({qsl("card_accessibilityKeyboard"), tr("Keyboard")});
#ifdef INCLUDE_MCPSERVER
    //: Card title on the General settings page, above the row leading to the AI assistant settings. MCP is the name of the Model Context Protocol standard, leave it as-is.
    cardTitles.append({qsl("card_mcpAssistant"), tr("AI assistant access (MCP)")});
#endif
    for (const auto& [objectName, title] : cardTitles) {
        if (auto* pCard = findChild<QGroupBox*>(objectName); pCard) {
            pCard->setTitle(title);
        }
    }

    //: Breadcrumb name of the subpage holding the telnet protocols, reached from the Connection settings page
    mSubpageTitles.insert(qsl("connection/protocols"), tr("Game protocols"));
    //: Breadcrumb name of the subpage holding the Discord Rich Presence settings, reached from the Chat and sharing settings page
    mSubpageTitles.insert(qsl("chat/discord"), tr("Discord Rich Presence"));
#ifdef INCLUDE_MCPSERVER
    //: Breadcrumb name of the subpage holding the AI assistant settings, reached from the General settings page. MCP is the name of the Model Context Protocol standard, leave it as-is.
    mSubpageTitles.insert(qsl("general/mcp"), tr("AI assistant access (MCP)"));
#endif

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
#ifdef INCLUDE_MCPSERVER
    updateMcpSummary();
#endif
    updateSecurityStatus();
    setCardDescriptions();
    retranslateExperiments();

    if (mpFrame_migrationBanner) {
        //: Title of the banner explaining that the settings dialog has been reorganised
        mpFrame_migrationBanner->findChild<QLabel*>(qsl("settingsMigrationBannerTitle"))->setText(tr("Same settings, new look!"));
        //: Body of the banner explaining that the settings dialog has been reorganised
        mpFrame_migrationBanner->findChild<QLabel*>(qsl("settingsMigrationBannerBody"))
                ->setText(tr("Settings are reorganised so they are easier to scan and search. Everything is still here - use search to jump straight to what you need."));
        //: Button that dismisses the "Same settings, new look!" banner for good
        mpFrame_migrationBanner->findChild<QPushButton*>(qsl("settingsMigrationBannerDismiss"))->setText(tr("Got it"));
    }

    //: Notice above the settings page when the dialog has no profile - it was opened before any was loaded, or its profile has since closed - explaining why most settings are greyed out. Screen readers may also read it out as the dialog opens.
    mpLabel_noProfileNotice->setText(tr("These settings are not linked to an open profile, so the ones that belong to a profile are greyed out. "
                                        "To change those, including the accessibility options, open Settings from a loaded profile."));
    updateNoProfileNotice();

    retranslateTriggerMatchPoolControls();
    setSearchKeywords();

    // Nothing is current while the shell is being built, and search sets its own title on the next query
    if (const QListWidgetItem* pCurrent = mpListWidget_categories->currentItem(); pCurrent && !mSearchActive) {
        if (mCurrentSubpage.isEmpty()) {
            mpLabel_pageTitle->setText(pCurrent->text());
        } else {
            mpLabel_pageTitle->setText(tr("%1 › %2").arg(pCurrent->text(), mSubpageTitles.value(mCurrentSubpage)));
        }
    }
}

void dlgProfilePreferences::createTriggerMatchPoolControls(QGroupBox* pCard)
{
    mpCheckBox_triggerMatchPool = new QCheckBox(pCard);
    mpCheckBox_triggerMatchPool->setObjectName(qsl("checkBox_triggerMatchPool"));
    moveIntoCard(pCard, {mpCheckBox_triggerMatchPool});
    auto* pCardLayout = qobject_cast<QVBoxLayout*>(pCard->layout());
    Q_ASSERT_X(pCardLayout, "dlgProfilePreferences::createTriggerMatchPoolControls", "the controls can only be added to a card that createCard() made");
    if (!pCardLayout) {
        return;
    }
    // One grid so the fields line up, indented to the check box's text as they only apply while it is on
    auto* pGrid = new QGridLayout();
    pGrid->setContentsMargins(style()->pixelMetric(QStyle::PM_IndicatorWidth, nullptr, mpCheckBox_triggerMatchPool)
                                      + style()->pixelMetric(QStyle::PM_CheckBoxLabelSpacing, nullptr, mpCheckBox_triggerMatchPool),
                              0,
                              0,
                              0);
    pGrid->setSpacing(8);
    // A number field stretched to the card's width reads as a text field
    pGrid->setColumnStretch(2, 1);
    pCardLayout->addLayout(pGrid);
    const auto addRow = [pCard, pGrid](QLabel*& pLabel, QSpinBox*& pSpinBox, const QString& name, const int minimum, const int maximum) {
        pLabel = new QLabel(pCard);
        pSpinBox = new QSpinBox(pCard);
        pSpinBox->setObjectName(qsl("spinBox_%1").arg(name));
        pSpinBox->setRange(minimum, maximum);
        pLabel->setBuddy(pSpinBox);
        const int row = pGrid->count() / 2;
        pGrid->addWidget(pLabel, row, 0);
        pGrid->addWidget(pSpinBox, row, 1);
    };
    addRow(mpLabel_triggerMatchThreads, mpSpinBox_triggerMatchThreads, qsl("triggerMatchThreads"), 1, std::max(1, QThread::idealThreadCount() - 1));
    addRow(mpLabel_triggerMatchThreshold, mpSpinBox_triggerMatchThreshold, qsl("triggerMatchThreshold"), 1, 100000);
    addRow(mpLabel_triggerMatchMissesPerMatch, mpSpinBox_triggerMatchMissesPerMatch, qsl("triggerMatchMissesPerMatch"), 0, 1000);
    addRow(mpLabel_triggerMatchFloodLines, mpSpinBox_triggerMatchFloodLines, qsl("triggerMatchFloodLines"), 1, 100000);
    addRow(mpLabel_triggerMatchSpin, mpSpinBox_triggerMatchSpin, qsl("triggerMatchSpinMicroseconds"), 0, 100000);
    connect(mpCheckBox_triggerMatchPool, &QCheckBox::toggled, this, &dlgProfilePreferences::enableTriggerMatchPoolTuning);
}

void dlgProfilePreferences::enableTriggerMatchPoolTuning(const bool enabled)
{
    for (auto* pControl : std::initializer_list<QWidget*>{mpLabel_triggerMatchThreads,
                                                          mpSpinBox_triggerMatchThreads,
                                                          mpLabel_triggerMatchThreshold,
                                                          mpSpinBox_triggerMatchThreshold,
                                                          mpLabel_triggerMatchMissesPerMatch,
                                                          mpSpinBox_triggerMatchMissesPerMatch,
                                                          mpLabel_triggerMatchFloodLines,
                                                          mpSpinBox_triggerMatchFloodLines,
                                                          mpLabel_triggerMatchSpin,
                                                          mpSpinBox_triggerMatchSpin}) {
        pControl->setEnabled(enabled);
    }
}

void dlgProfilePreferences::retranslateTriggerMatchPoolControls()
{
    //: Check box in the Performance settings; the triggers' regular expressions are then searched on several CPU cores while game text floods in
    mpCheckBox_triggerMatchPool->setText(tr("Match triggers on several threads during floods of text (experimental)"));
    //: Tooltip of the check box that matches triggers on several threads
    mpCheckBox_triggerMatchPool->setToolTip(tr("<p>Mudlet checks each line of game text against your triggers one at a time. Regex triggers are the slow ones, and on a busy "
                                               "profile most of them do not match most lines.</p>"
                                               "<p>When the game sends many lines at once, this has other CPU cores check the regex triggers first and tell Mudlet which ones "
                                               "cannot match, so it skips them. Your triggers still fire in the same order as before.</p>"
                                               "<p>On most profiles it makes no difference and uses more CPU, so it is off unless you switch it on. Once it is on, the Statistics "
                                               "report in the trigger editor (Ctrl+9) tells you whether it helps your profile.</p>"));
    const TriggerMatchPool::Settings defaults;
    //: Label of a number field in the Performance settings: how many threads help the main one match triggers
    mpLabel_triggerMatchThreads->setText(tr("Extra threads helping the main one:"));
    //: Tooltip of the number of extra threads that match triggers; %1 is the default number of them
    mpSpinBox_triggerMatchThreads->setToolTip(tr("<p>How many other CPU cores help the main one check triggers. More than three rarely helps: they then spend longer "
                                                 "waiting for each other than checking.</p><p>Default: %1</p>")
                                                      .arg(TriggerMatchPool::automaticThreads() - 1));
    //: Label of a number field in the Performance settings: how many regex searches that find nothing a line needs before other threads help with it
    mpLabel_triggerMatchThreshold->setText(tr("Failed regex searches a line needs before the threads help:"));
    //: Tooltip of the threshold of failed regex searches per line; %1 is the default
    mpSpinBox_triggerMatchThreshold->setToolTip(tr("<p>The extra threads help only if the previous line had at least this many regex triggers that did not match. "
                                                   "With fewer, handing the work out costs more than it saves.</p><p>Default: %1</p>")
                                                        .arg(defaults.threshold));
    //: Label of a number field in the Performance settings: how many failed regex searches a line needs for every one that matched
    mpLabel_triggerMatchMissesPerMatch->setText(tr("Failed searches needed for each one that matched:"));
    //: Tooltip of the failed searches per match ratio; %1 is the default
    mpSpinBox_triggerMatchMissesPerMatch->setToolTip(tr("<p>The extra threads help only if, on the previous line, at least this many regex triggers did not match for "
                                                        "each one that did. Lines where many triggers match are quicker on one thread. 0 turns this check off.</p>"
                                                        "<p>Default: %1</p>")
                                                             .arg(defaults.missesPerMatch));
    //: Label of a number field in the Performance settings: how many lines must arrive at once to count as a flood
    mpLabel_triggerMatchFloodLines->setText(tr("Lines arriving at once that make a flood:"));
    //: Tooltip of the lines that make a flood; %1 is the default
    mpSpinBox_triggerMatchFloodLines->setToolTip(tr("<p>The extra threads help only when the game sends at least this many lines at once. For a line or two at a time "
                                                    "they would use CPU for nothing.</p><p>Default: %1</p>")
                                                         .arg(defaults.floodChunkLines));
    //: Label of a number field in the Performance settings: how long, in microseconds, the helper threads stay awake waiting for the next line
    mpLabel_triggerMatchSpin->setText(tr("Microseconds the threads stay awake for the next line:"));
    //: Tooltip of the helper threads' spin time; %1 is the default, in microseconds
    mpSpinBox_triggerMatchSpin->setToolTip(tr("<p>After a line, the extra threads stay awake this long waiting for the next one before they go to sleep, because "
                                              "waking a sleeping thread costs more than the work it is given.</p><p>Default: %1</p>")
                                                   .arg(defaults.spinMicroseconds));
    for (const auto& [pLabel, pSpinBox] : std::initializer_list<std::pair<QLabel*, QSpinBox*>>{{mpLabel_triggerMatchThreads, mpSpinBox_triggerMatchThreads},
                                                                                               {mpLabel_triggerMatchThreshold, mpSpinBox_triggerMatchThreshold},
                                                                                               {mpLabel_triggerMatchMissesPerMatch, mpSpinBox_triggerMatchMissesPerMatch},
                                                                                               {mpLabel_triggerMatchFloodLines, mpSpinBox_triggerMatchFloodLines},
                                                                                               {mpLabel_triggerMatchSpin, mpSpinBox_triggerMatchSpin}}) {
        pLabel->setToolTip(pSpinBox->toolTip());
    }
}

// App-wide like the pool, so read from Mudlet.ini with or without a profile
void dlgProfilePreferences::populateTriggerMatchPoolSettings()
{
    // Validated as the pool validates them, so a field shows the value the pool would use
    const TriggerMatchPool::Settings saved = TriggerMatchPool::savedSettings();
    const bool enabled = saved.threads >= 2;
    {
        const QSignalBlocker blocker(mpCheckBox_triggerMatchPool);
        mpCheckBox_triggerMatchPool->setChecked(enabled);
    }
    enableTriggerMatchPoolTuning(enabled);
    // One core leaves no other thread to share the matching with
    mpCheckBox_triggerMatchPool->setEnabled(QThread::idealThreadCount() >= 2 || enabled);
    for (const auto& [pSpinBox, value] : std::initializer_list<std::pair<QSpinBox*, int>>{{mpSpinBox_triggerMatchThreads, (enabled ? saved.threads : TriggerMatchPool::automaticThreads()) - 1},
                                                                                          {mpSpinBox_triggerMatchThreshold, saved.threshold},
                                                                                          {mpSpinBox_triggerMatchMissesPerMatch, saved.missesPerMatch},
                                                                                          {mpSpinBox_triggerMatchFloodLines, saved.floodChunkLines},
                                                                                          {mpSpinBox_triggerMatchSpin, saved.spinMicroseconds}}) {
        const QSignalBlocker spinBlocker(pSpinBox);
        pSpinBox->setValue(value);
    }
}

void dlgProfilePreferences::applyTriggerMatchPoolSettings()
{
    // Only what the player changed: a key left unwritten keeps following the defaults of later releases
    QSettings* settings = MudletApp::getQSettings();
    bool changed = false;
    if (mSnapshot.anyDirty({mpCheckBox_triggerMatchPool, mpSpinBox_triggerMatchThreads})) {
        settings->setValue(qsl("triggerMatchThreads"), mpCheckBox_triggerMatchPool->isChecked() ? mpSpinBox_triggerMatchThreads->value() + 1 : 0);
        changed = true;
    }
    for (const auto& [pSpinBox, key] : std::initializer_list<std::pair<QSpinBox*, QString>>{{mpSpinBox_triggerMatchThreshold, qsl("triggerMatchThreshold")},
                                                                                            {mpSpinBox_triggerMatchMissesPerMatch, qsl("triggerMatchMissesPerMatch")},
                                                                                            {mpSpinBox_triggerMatchFloodLines, qsl("triggerMatchFloodLines")},
                                                                                            {mpSpinBox_triggerMatchSpin, qsl("triggerMatchSpinMicroseconds")}}) {
        if (mSnapshot.anyDirty({pSpinBox})) {
            settings->setValue(key, pSpinBox->value());
            changed = true;
        }
    }
    if (changed) {
        // Through settingsFromConfig() rather than from the controls, so an environment override still wins
        TriggerMatchPool::instance().configure(TriggerMatchPool::settingsFromConfig());
    }
}

// Folded into the text of the control's card; a match highlights that control
void dlgProfilePreferences::setSearchKeywords()
{
    // The subpage is indexed too, but this row is a way in. Not translated: these are the protocol names
    // as games and their documentation spell them.
    pushButton_chooseProtocols->setProperty(scmProp_searchKeywords, qsl("GMCP MSDP MSSP MSP MXP MTTS MNES NAWS CHARSET NEW-ENVIRON telnet"));

    QList<std::pair<QWidget*, QString>> synonyms;
    //: Comma-separated synonyms for the settings search. Translate them into the words a player of your language would type when looking for this setting, rather than transliterating the English ones; acronyms and protocol names that your language uses untranslated can be left as they are. This one is for the secure connection settings.
    synonyms.append({groupBox_ssl, tr("TLS, SSL, secure connection, encryption, certificate")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for the reminder offered when the game supports a secure connection.
    synonyms.append({checkBox_askTlsAvailable, tr("TLS, SSL, secure connection, reminder")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for preparing the matches, multimatches and line trigger variables only when a script reads them.
    synonyms.append({checkBox_lazyCaptureGlobals, tr("performance, speed, fast, slow, lag, lazy, matches, multimatches, line, _G, global variables")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for matching triggers on several threads (CPU cores) during floods of game text.
    synonyms.append({mpCheckBox_triggerMatchPool, tr("performance, speed, fast, slow, lag, threads, cores, CPU, multithreading, parallel, triggers, regex, flood, spam")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for the proxy server settings.
    synonyms.append({groupBox_proxy, tr("proxy, SOCKS, tunnel, firewall")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for where game passwords are kept.
    synonyms.append({label_store_passwords_in, tr("password, keyring, keychain, credentials, sign in, two-factor, 2FA")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for showing the password as it is typed.
    synonyms.append({disable_password_masking_checkbox, tr("password, masking, hidden characters")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for telling the game that a screen reader is in use.
    synonyms.append({checkBox_advertiseScreenReader, tr("screen reader, NVDA, JAWS, VoiceOver, Orca, accessibility")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for reaching buttons in scripts and packages with the keyboard.
    synonyms.append({checkBox_accessibleLabels, tr("keyboard, tab, focus, buttons, labels, screen reader, accessibility")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for reading incoming text out loud.
    synonyms.append({checkBox_announceIncomingText, tr("text to speech, TTS, speech, spoken, screen reader")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for whether pressing Alt on its own moves focus to the menu bar.
    synonyms.append({label_menuBarAltKeyNavigation, tr("Alt key, menu bar, menus, keyboard, screen reader")});
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
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for showing the time each line arrived in the main window.
    synonyms.append({checkBox_showTimeStamps, tr("timestamps, time stamps, clock, when lines arrived")});
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
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for the colours the game's text and background are drawn in.
    synonyms.append({groupBox_displayColors, tr("colour, colours, palette, ANSI colours, background")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for the colours the map is drawn in.
    synonyms.append({groupBox_mapperColors, tr("colour, colours, map colours, palette, background")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for the font the game's text is drawn in.
    synonyms.append({groupBox_font, tr("font, typeface, size, monospace, antialiasing")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for the font the script editor's code is written in.
    synonyms.append({groupBox_editorFont, tr("editor font, code font, script font, typeface, size, monospace")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for how long Mudlet waits for the game to answer.
    synonyms.append({label_networkPacketTimeout, tr("timeout, lag, latency, slow connection")});
#ifdef INCLUDE_MCPSERVER
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for letting an AI assistant control Mudlet. Claude, ChatGPT and Codex are product names, leave them as-is.
    synonyms.append({groupBox_mcpServer, tr("AI, assistant, MCP, Claude, ChatGPT, Codex, LLM, chatbot")});
#endif
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for workarounds needed by some games, which used to be on a tab called "Special options".
    synonyms.append({groupBox_specialOptions, tr("special options, workaround, old game, compatibility")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for script debugging options, which used to be on a tab called "Special options".
    synonyms.append({groupBox_debug, tr("special options, advanced, debug, developer")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for the unfinished features that can be switched on for a profile.
    synonyms.append({mpCard_experiments.data(), tr("experiment, experimental, beta, preview, unsupported, 3D map, renderer")});
    //: Comma-separated synonyms for the settings search - translate to what a player would type, do not transliterate. This one is for showing the toolbar's and the script editor's icons in color.
    synonyms.append({mpCheckBox_colouredIcons.data(), tr("colour, color, coloured, colored, icons, toolbar, script editor, old icons")});
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

namespace {
// A wrapping card's minimum height is measured at the column's narrowest width, many lines taller than at the
// width it is given, and QScrollArea keeps its bar until the viewport holds the taller of that and the
// height-for-width it lays the column out at
class SettingsColumn : public QWidget
{
public:
    using QWidget::QWidget;

    QSize minimumSizeHint() const override
    {
        QSize hint = QWidget::minimumSizeHint();
        if (layout() && layout()->hasHeightForWidth()) {
            hint.setHeight(0);
        }
        return hint;
    }
};
} // namespace

QScrollArea* dlgProfilePreferences::createScrollPage(const QString& objectSuffix)
{
    auto* pScrollArea = new QScrollArea(mpStackedWidget_categories);
    pScrollArea->setObjectName(qsl("settingsPage_%1").arg(objectSuffix));
    pScrollArea->setFrameShape(QFrame::NoFrame);
    pScrollArea->setWidgetResizable(true);
    markAsShellSurface(pScrollArea);

    auto* pColumn = new SettingsColumn(pScrollArea);
    pColumn->setObjectName(qsl("settingsColumn_%1").arg(objectSuffix));
    auto* pColumnLayout = new QVBoxLayout(pColumn);
    pColumnLayout->setContentsMargins(0, 0, 0, 0);
    pColumnLayout->setSpacing(16);

    pScrollArea->setWidget(pColumn);
    // setWidget() makes the column fill from its own palette; the page background belongs to the area behind
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
        pCard->setProperty(scmProp_settingsCard, true);
        // What a card leaves round what it holds is its padding, and nothing
        // else: a layout's own default margins are a second helping of it, and
        // they put the controls a style's worth of pixels right of the title
        // now drawn on the padding edge above them
        if (QLayout* pCardLayout = pCard->layout(); pCardLayout) {
            pCardLayout->setContentsMargins(0, 0, 0, 0);
        }
        pColumnLayout->addWidget(pCard);
    }
    pColumnLayout->addStretch(1);
    fitColumn(pScrollArea);
    return pScrollArea;
}

void dlgProfilePreferences::buildCategoryPage(const QString& key, const QList<QWidget*>& cards)
{
    mCategories[key].pageIndex = mpStackedWidget_categories->addWidget(buildPage(key, cards));
}

// A stack page with no sidebar row, reached by its opener card, a "category/sub" deep link, or search
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
        // Every caller is C++, so an unknown key is a typo
        qWarning() << "dlgProfilePreferences::showSubpage(...) WARNING - there is no settings subpage" << key << "- showing the category instead.";
        showCategory(categoryKey, pSpotlightTarget);
        return;
    }
    if (mSearchActive) {
        // Clearing the field sends borrowed cards home, which must finish before the stack switches
        mSubpageBeforeSearch.clear();
        mpLineEdit_search->clear();
    }
    // Before the sidebar moves: the row-changed slot takes that to mean a category page is showing
    mCurrentSubpage.clear();
    mpListWidget_categories->setCurrentRow(qMax(0, categoryRow(categoryKey)));
    mCurrentSubpage = key;

    auto* pPage = qobject_cast<QScrollArea*>(mpStackedWidget_categories->widget(mSubpageIndexes.value(key)));
    mpStackedWidget_categories->setCurrentWidget(pPage);
    fitColumn(pPage);
    // As on a category page, the fit above measured cards without their padding
    QTimer::singleShot(0ms, this, [this, pPage]() {
        if (pPage && mpStackedWidget_categories->currentWidget() == pPage) {
            fitColumn(pPage);
            updateSidebarMode();
        }
    });

    mpLabel_pageTitleIcon->hide();
    mpButton_searchBack->hide();
    mpButton_subpageBack->show();
    mpLabel_pageTitle->setText(breadcrumbFor(key));
    spotlight(pSpotlightTarget);
}

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
    // The sidebar never left the parent category, so no row change fires; call the slot directly
    if (mpListWidget_categories->currentRow() == row) {
        slot_categorySelected(row);
        return;
    }
    mpListWidget_categories->setCurrentRow(row);
}

// Titled by retranslateShell(), via the object name
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
            // The control would be parented but never laid out - a setting silently missing
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

// Created on the first call and only reworded afterwards, so a language change does not add a second line
void dlgProfilePreferences::setLinkedText(QLabel* pLabel, const QString& richText)
{
    // The first label may be written before the shell is styled, so the ink is
    // taken here if nothing has settled it yet
    if (!mInkedLinkColour.isValid()) {
        mInkedLinkColour = themeTokens().accentText;
    }
    pLabel->setProperty(scmProp_settingsRichText, richText);
    pLabel->setText(withLinkColour(richText, mInkedLinkColour));
}

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
                // A single-row card has nowhere above to put the line
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
        setLinkedText(pLabel, description.toHtmlEscaped());
        return;
    }
    //: Link at the end of a settings card's description line, opening the Mudlet wiki page about that setting
    setLinkedText(pLabel, qsl("%1 <a href=\"%2\">%3</a>").arg(description.toHtmlEscaped(), learnMoreUrl, tr("Learn more").toHtmlEscaped()));
}

void dlgProfilePreferences::setCardDescriptions()
{
    //: Description line under the "System integration" card title on the General settings page
    setCardDescription(findChild<QGroupBox*>(qsl("card_systemIntegration")), tr("How Mudlet fits in with the rest of your desktop."));
    //: Description line under the "Web search" card title on the General settings page
    setCardDescription(groupbox_searchEngineSelection, tr("The site Mudlet opens when you pick \"search on the web\" after selecting some text in the game."));
#ifdef INCLUDE_MCPSERVER
    //: Description line under the "AI assistant access (MCP)" card title on the General settings page, and again at the top of the subpage it leads to
    const QString mcpDescription = tr("Lets an AI assistant run Lua inside Mudlet for you. Lua can reach the rest of this computer, so only connect an assistant you trust.");
    setCardDescription(mpCard_mcpAssistant, mcpDescription);
    setCardDescription(groupBox_mcpServer, mcpDescription);
#endif
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
    //: Description line under the "Performance" card title on the Advanced settings page
    setCardDescription(findChild<QGroupBox*>(qsl("card_performance")),
                       tr("Ways Mudlet speeds up your scripts. Leave the first on unless a script or package misbehaves; matching on several threads is off unless you switch it on."));
    //: Description line under the "Developer" card title on the Advanced settings page
    setCardDescription(groupBox_debug, tr("How much Mudlet's debugging tools report while you work on scripts."));
    //: Description line under the "Experiments (unsupported)" card title on the Advanced settings page
    setCardDescription(mpCard_experiments,
                       tr("Unfinished features that are still being tried out. They are not supported: they may misbehave, change or be removed in any release. "
                          "Each one is switched for this profile only, unless marked as applying to every profile."));
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

    // Connected here rather than in initWithHost(), which would stack a copy every time a profile came and went
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
            // Populating from a profile needs no reconnect
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

    // The button keeps its object name, tab stop and place in the four host enable/disable lists;
    // only where it leads changes
    pushButton_chooseProtocols->setMenu(nullptr);
    makeChevronRow(pushButton_chooseProtocols);
    connect(pushButton_chooseProtocols, &QAbstractButton::clicked, this, [this]() {
        showSubpage(scmCategory_connection, qsl("protocols"));
    });
}

void dlgProfilePreferences::buildExperimentsCard()
{
    mpCard_experiments = createCard(qsl("card_experiments"));
    auto* pCardLayout = qobject_cast<QVBoxLayout*>(mpCard_experiments->layout());
    pCardLayout->setSpacing(4);

    mpCheckBox_colouredIcons = new QCheckBox(mpCard_experiments);
    mpCheckBox_colouredIcons->setObjectName(qsl("checkBox_colouredIcons"));
    pCardLayout->addWidget(mpCheckBox_colouredIcons);
    auto* pColouredDescription = new QLabel(mpCard_experiments);
    pColouredDescription->setObjectName(qsl("checkBox_colouredIcons_description"));
    pColouredDescription->setProperty("settingsControlDescription", true);
    pColouredDescription->setWordWrap(true);
    pCardLayout->addWidget(pColouredDescription);

    for (const auto& key : Host::getValidExperiments()) {
        auto* pCheckBox = new QCheckBox(mpCard_experiments);
        pCheckBox->setObjectName(qsl("checkBox_%1").arg(key));
        pCardLayout->addWidget(pCheckBox);
        auto* pDescription = new QLabel(mpCard_experiments);
        pDescription->setObjectName(qsl("checkBox_%1_description").arg(key));
        pDescription->setProperty("settingsControlDescription", true);
        pDescription->setWordWrap(true);
        pCardLayout->addWidget(pDescription);
        mExperimentCheckBoxes.insert(key, pCheckBox);
    }
#if !defined(INCLUDE_3DMAPPER)
    // Every profile experiment so far only changes the 3D map, which this build leaves out
    for (const auto& pCheckBox : std::as_const(mExperimentCheckBoxes)) {
        pCheckBox->hide();
        if (auto* pDescription = mpCard_experiments->findChild<QLabel*>(qsl("%1_description").arg(pCheckBox->objectName())); pDescription) {
            pDescription->hide();
        }
    }
#endif

    // Mirrors Host::setExperimentEnabled(), which switches the rest of a group off, so the
    // dialog never shows two of one group ticked
    for (const auto& [key, pCheckBox] : std::as_const(mExperimentCheckBoxes).asKeyValueRange()) {
        const QString group = Host::experimentGroup(key);
        if (group.isEmpty()) {
            continue;
        }
        connect(pCheckBox, &QAbstractButton::toggled, this, [this, ownKey = key, group](const bool checked) {
            if (mPopulating || !checked) {
                return;
            }
            for (const auto& [siblingKey, pSibling] : std::as_const(mExperimentCheckBoxes).asKeyValueRange()) {
                if (siblingKey != ownKey && pSibling && Host::experimentGroup(siblingKey) == group) {
                    pSibling->setChecked(false);
                }
            }
        });
    }
}

void dlgProfilePreferences::retranslateExperiments()
{
    //: Checkbox on the "Experiments (unsupported)" card of the Advanced settings page
    mpCheckBox_colouredIcons->setText(tr("Colored icons"));
    if (auto* pDescription = mpCard_experiments->findChild<QLabel*>(qsl("checkBox_colouredIcons_description")); pDescription) {
        //: Description under the "Colored icons" checkbox on the "Experiments (unsupported)" card of the Advanced settings page
        pDescription->setText(tr("Gives the icons on Mudlet's toolbar and in the script editor the colors of the pictures they replaced, so a blue map or a red delete is where you remember it. "
                                 "Applies to every profile."));
    }

    QHash<QString, QString> labels;
    QHash<QString, QString> descriptions;
    //: Checkbox on the "Experiments (unsupported)" card of the Advanced settings page
    labels.insert(qsl("experiment.3dmap.modernmapper"), tr("Modern 3D mapper"));
    //: Description under the "Modern 3D mapper" checkbox on the "Experiments (unsupported)" card of the Advanced settings page
    descriptions.insert(qsl("experiment.3dmap.modernmapper"), tr("Draws the 3D map with the new renderer instead of the original one. The other 3D map experiments only work with it."));
    //: Checkbox on the "Experiments (unsupported)" card of the Advanced settings page
    labels.insert(qsl("experiment.3d-player-icon"), tr("3D player marker"));
    //: Description under the "3D player marker" checkbox on the "Experiments (unsupported)" card of the Advanced settings page
    descriptions.insert(qsl("experiment.3d-player-icon"), tr("Shows a marker above the room you are in on the modern 3D map."));
    //: Checkbox on the "Experiments (unsupported)" card of the Advanced settings page
    labels.insert(qsl("experiment.render-in-out-exits"), tr("In and out exits on the 3D map"));
    //: Description under the "In and out exits on the 3D map" checkbox on the "Experiments (unsupported)" card of the Advanced settings page
    descriptions.insert(qsl("experiment.render-in-out-exits"), tr("Draws in and out exits on the modern 3D map, and marks the rooms that have them with arrows."));
    //: Checkbox on the "Experiments (unsupported)" card of the Advanced settings page
    labels.insert(qsl("experiment.rendering.more-transparent"), tr("Darken other levels"));
    //: Description under the "Darken other levels" checkbox on the "Experiments (unsupported)" card of the Advanced settings page
    descriptions.insert(qsl("experiment.rendering.more-transparent"),
                        tr("Draws rooms on the modern 3D map darker the further their level is from yours, instead of making the rooms above you see-through."));
    //: Checkbox on the "Experiments (unsupported)" card of the Advanced settings page
    labels.insert(qsl("experiment.rendering.originalish"), tr("Original level shading"));
    //: Description under the "Original level shading" checkbox on the "Experiments (unsupported)" card of the Advanced settings page
    descriptions.insert(qsl("experiment.rendering.originalish"), tr("Draws the rooms on other levels of the modern 3D map the usual way, with the rooms above you see-through."));

    for (const auto& [key, pCheckBox] : std::as_const(mExperimentCheckBoxes).asKeyValueRange()) {
        if (!pCheckBox) {
            continue;
        }
        pCheckBox->setText(labels.value(key, key));

        QStringList siblings;
        const QString group = Host::experimentGroup(key);
        for (const auto& siblingKey : mExperimentCheckBoxes.keys()) {
            if (!group.isEmpty() && siblingKey != key && Host::experimentGroup(siblingKey) == group) {
                siblings << labels.value(siblingKey, siblingKey);
            }
        }
        QString description = descriptions.value(key);
        if (!siblings.isEmpty()) {
            //: Added to the description of an experiment on the Advanced settings page that cannot be on together with others; %1 is the list of their names
            const QString exclusive = tr("Turning this on turns off: %1.").arg(siblings.join(qsl(", ")));
            description = description.isEmpty() ? exclusive : qsl("%1 %2").arg(description, exclusive);
        }
        if (auto* pDescription = findChild<QLabel*>(qsl("%1_description").arg(pCheckBox->objectName())); pDescription) {
            pDescription->setText(description);
            pDescription->setVisible(!description.isEmpty());
        }
        pCheckBox->setAccessibleDescription(description);
        //: Tooltip line on an experiment's checkbox on the Advanced settings page; %1 is the name a script passes to setConfig() to switch it, which is not translated
        const QString scriptName = tr("Name for scripts: %1").arg(qsl("<tt>%1</tt>").arg(key.toHtmlEscaped()));
        pCheckBox->setToolTip(description.isEmpty() ? qsl("<p>%1</p>").arg(scriptName) : qsl("<p>%1</p><p>%2</p>").arg(description.toHtmlEscaped(), scriptName));
    }
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
    // Two numbers rather than a %n plural: an untranslated %n string shows "(s)" in English, and this row
    // is too prominent to read "9 protocol(s) on"
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

#ifdef INCLUDE_MCPSERVER
void dlgProfilePreferences::buildMcpSummaryCard()
{
    mpCard_mcpAssistant = createCard(qsl("card_mcpAssistant"));
    mpButton_mcpSubpage = new QPushButton(mpCard_mcpAssistant);
    mpButton_mcpSubpage->setObjectName(qsl("pushButton_mcpSettings"));
    // The words on the row are a status, so the things a user would look for by
    // name are only on the page it leads to
    mpButton_mcpSubpage->setProperty(scmProp_searchKeywords, qsl("MCP AI assistant Claude ChatGPT Codex Lua port token"));
    makeChevronRow(mpButton_mcpSubpage);
    qobject_cast<QVBoxLayout*>(mpCard_mcpAssistant->layout())->addWidget(mpButton_mcpSubpage);
    connect(mpButton_mcpSubpage, &QAbstractButton::clicked, this, [this]() {
        showSubpage(scmCategory_general, qsl("mcp"));
    });
}

// The same four states as the status line on the subpage, said in one line: the
// checkbox rather than mcpEnabled(), so the row moves with the tick rather than
// with the apply that follows it 400ms later
void dlgProfilePreferences::updateMcpSummary()
{
    if (!mpButton_mcpSubpage) {
        return;
    }
    mudlet* pMudlet = mudlet::self();
    if (!checkBox_enableMCPServer->isChecked()) {
        //: Summary on the General page's AI assistant card, on the row that opens the assistant settings
        mpButton_mcpSubpage->setText(tr("Off - no assistant can reach Mudlet"));
        return;
    }
    if (!pMudlet->mcpEndpoint().isEmpty()) {
        //: Summary on the General page's AI assistant card while the server is running. %1 is a TCP port number.
        mpButton_mcpSubpage->setText(tr("On - listening on port %1").arg(pMudlet->mcpServerPort()));
        return;
    }
    if (!pMudlet->mcpLastError().isEmpty()) {
        //: Summary on the General page's AI assistant card when the server was switched on but could not start
        mpButton_mcpSubpage->setText(tr("On, but not running - open to see why"));
        return;
    }
    //: Summary on the General page's AI assistant card in the moment between the server being switched on and it answering
    mpButton_mcpSubpage->setText(tr("Starting..."));
}
#endif

void dlgProfilePreferences::buildSecurityStatusCard()
{
    mpCard_securityStatus = createCard(qsl("card_securityStatus"));
    mpCard_securityStatus->setProperty("settingsHero", true);
    // It carries no setting, so needs no title or room above the frame for one
    mpCard_securityStatus->setProperty(scmProp_settingsCardPlain, true);
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
    // The hero holds no setting, so the link leads to the card that does
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
    setLinkedText(mpLabel_securityLink, qsl("<a href=\"#secureConnection\">%1</a>").arg(tr("Secure connection settings").toHtmlEscaped()));
}

// The .ui titles these after their old tabs, and retranslateUi() restores those titles on every language change
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
#ifdef INCLUDE_MCPSERVER
    // The only card on its subpage, whose breadcrumb already names it
    groupBox_mcpServer->setTitle(QString());
    groupBox_mcpServer->setProperty("settingsCardPlain", true);
#endif
}

// A grid cell takes one widget, so a card is emptied before it is refilled
static void takeOutOfLayout(QLayout* pLayout, const QList<QWidget*>& widgets)
{
    for (auto* pWidget : widgets) {
        pLayout->removeWidget(pWidget);
    }
}

// The .ui laid these grids across a tab's full width, in rows of three or four cells too wide for the reading column
void dlgProfilePreferences::reflowWideCards()
{
    takeOutOfLayout(horizontalLayout_groupBox_wrapping, {frame_wrap_at, frame_indent_wrapped, frame_hanging_indent_wrapped});
    int wrappingRow = 0;
    for (auto* pFrame : {frame_wrap_at, frame_indent_wrapped, frame_hanging_indent_wrapped}) {
        // Without a trailing stretch the spin box grows to the card's width
        qobject_cast<QHBoxLayout*>(pFrame->layout())->addStretch(1);
        verticalLayout_groupBox_wrapping->insertWidget(wrappingRow++, pFrame);
    }

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
    // The rows are stacked rather than in two columns, so the divider runs across them
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

    // Framed like an input in the .ui, but it only shows a name, and with no Discord user is an empty box
    label_data_discordCurrentUser->setFrameShape(QFrame::NoFrame);

    // Two label-and-control pairs per row do not fit German labels, so each pair gets a row
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

    // Otherwise the time edit takes the row's stretch and spans the whole card
    horizontalLayout_timerDebugOutputMinimumInterval->setStretch(1, 0);
    horizontalLayout_timerDebugOutputMinimumInterval->addStretch(1);

    // Both show runtime text (a profile name, a file path) that can exceed any column; wrapping stops them
    // setting the column's minimum width
    label_mapFileActionResult->setWordWrap(true);
    label_password_migration_notification->setWordWrap(true);
}

// The .ui's grids sized every column by its longest member, which in German widened the Mapper page's cards far
// past the reading column. Each card's grid is kept as a single column of rows that flow.
void dlgProfilePreferences::reflowMapperCards()
{
    takeOutOfLayout(gridLayout_groupBox_mapFiles, {line, label_mapFileActionResult});
    const QList<QWidget*> mapFileRows{makeFlowRow(groupBox_mapFiles, qsl("settingsMapperRow_saveMap"), {label_saveMap, pushButton_saveMap, checkBox_reportMapIssuesOnScreen}),
                                      makeFlowRow(groupBox_mapFiles, qsl("settingsMapperRow_loadMap"), {label_loadMap, pushButton_loadMap}),
                                      makeFlowRow(groupBox_mapFiles, qsl("settingsMapperRow_loadHistoricMap"), {label_loadHistoricMap, comboBox_mapHistory, pushButton_loadHistoricMap}),
                                      line,
                                      makeFlowRow(groupBox_mapFiles, qsl("settingsMapperRow_deleteMap"), {label_deleteMap, pushButton_deleteMap}),
                                      makeFlowRow(groupBox_mapFiles, qsl("settingsMapperRow_copyMap"), {label_copyMap, pushButton_chooseProfiles, pushButton_copyMap}),
                                      makeFlowRow(groupBox_mapFiles, qsl("settingsMapperRow_mapFileSaveFormatVersion"), {label_mapFileSaveFormatVersion, comboBox_mapFileSaveFormatVersion}),
                                      label_mapFileActionResult};
    for (int row = 0, rows = mapFileRows.size(); row < rows; ++row) {
        gridLayout_groupBox_mapFiles->addWidget(mapFileRows.at(row), row, 0);
    }
    gridLayout_groupBox_mapFiles->setVerticalSpacing(scmFlowRowGap);

    // Turned on its side here rather than by setCardDescription(), which would nest the row in a row of its own
    hBoxLayout_groupBox_downloadMapOptions->setDirection(QBoxLayout::TopToBottom);
    hBoxLayout_groupBox_downloadMapOptions->addWidget(makeFlowRow(groupBox_downloadMapOptions, qsl("settingsMapperRow_downloadMap"), {label_11, buttonDownloadMap}));

    // Two columns of three, each as wide as its own longest words; the second goes under the first, whole,
    // once they no longer fit side by side, and lines up with the top of the first while they do
    QWidget* pLeadingChoices =
            makeFlowPiece(groupBox_mapViewOptions, qsl("settingsMapViewLeadingChoices"), QBoxLayout::TopToBottom, {mMapperUseAntiAlias, checkBox_drawUpperLowerLevels, checkBox_invertMapZoom});
    QWidget* pTrailingChoices = makeFlowPiece(
            groupBox_mapViewOptions, qsl("settingsMapViewTrailingChoices"), QBoxLayout::TopToBottom, {checkbox_mMapperShowRoomBorders, checkBox_largeAreaExitArrows, checkBox_showDefaultArea});
    QList<QWidget*> mapViewRows{makeFlowRow(groupBox_mapViewOptions, qsl("settingsMapViewChoices"), {pLeadingChoices, pTrailingChoices}, scmChoiceGroupGap, uiDesign::FlowLayout::LineAlignment::Top)};
#if defined(INCLUDE_3DMAPPER)
    mapViewRows.append(makeFlowRow(groupBox_mapViewOptions, qsl("settingsMapperRow_mapStyle"), {label_mapStyle, comboBox_mapStyle}));
#else
    takeOutOfLayout(gridLayout_groupBox_mapViewOptions, {label_mapStyle, comboBox_mapStyle});
    label_mapStyle->hide();
    comboBox_mapStyle->hide();
#endif
    takeOutOfLayout(gridLayout_groupBox_mapViewOptions, {gridGroupBox, groupBox_mapSymbols});
    mapViewRows.append({gridGroupBox, groupBox_mapSymbols});
    for (int row = 0, rows = mapViewRows.size(); row < rows; ++row) {
        gridLayout_groupBox_mapViewOptions->addWidget(mapViewRows.at(row), row, 0);
    }
    gridLayout_groupBox_mapViewOptions->setVerticalSpacing(scmFlowRowGap);

    const QList<QWidget*> featureSizes{makeFlowPiece(gridGroupBox, qsl("settingsMapperFeatureSize_room"), QBoxLayout::LeftToRight, {label_roomSize, spinBox_roomSize}),
                                       makeFlowPiece(gridGroupBox, qsl("settingsMapperFeatureSize_exit"), QBoxLayout::LeftToRight, {label_exitSize, spinBox_exitSize}),
                                       makeFlowPiece(gridGroupBox, qsl("settingsMapperFeatureSize_border"), QBoxLayout::LeftToRight, {label_borderSize, spinBox_borderSize}),
                                       makeFlowPiece(gridGroupBox, qsl("settingsMapperFeatureSize_grid"), QBoxLayout::LeftToRight, {label_gridSize, doubleSpinBox_gridSize})};
    groupBox_sizing->addWidget(makeFlowRow(gridGroupBox, qsl("settingsMapperFeatureSizes"), featureSizes, scmFieldPairGap), 0, 0);

    // initWithHost() appends the scaling factor's row under these two
    QWidget* pSymbolFontRow = makeFlowRow(groupBox_mapSymbols, qsl("settingsMapperRow_mapSymbolsFont"), {label_mapSymbolsFont, fontComboBox_mapSymbols, pushButton_showGlyphUsage});
    takeOutOfLayout(gridLayout_groupBox_mapSymbols, {checkBox_isOnlyMapSymbolFontToBeUsed});
    gridLayout_groupBox_mapSymbols->addWidget(pSymbolFontRow, 0, 0);
    gridLayout_groupBox_mapSymbols->addWidget(checkBox_isOnlyMapSymbolFontToBeUsed, 1, 0);
    gridLayout_groupBox_mapSymbols->setVerticalSpacing(scmFlowRowGap);

    // A row like the Map files rows, so the list can drop under its label: glued into one piece, the two were
    // wider than the reading column in German
    gridLayout_groupBox_playerRoomStyle->addWidget(makeFlowRow(groupBox_playerRoomStyle, qsl("settingsMapperRow_playerRoomStyle"), {label_playerRoomStyle, comboBox_playerRoomStyle}), 0, 0);
    gridLayout_groupBox_playerRoomStyle->addWidget(
            makeFlowRow(groupBox_playerRoomStyle,
                        qsl("settingsPlayerRoomMarker"),
                        {pushButton_playerRoomPrimaryColor, pushButton_playerRoomSecondaryColor, spinBox_playerRoomOuterDiameter, spinBox_playerRoomInnerDiameter},
                        scmFieldPairGap),
            1,
            0);
    gridLayout_groupBox_playerRoomStyle->setVerticalSpacing(scmFlowRowGap);
}

// The General page's rows and pairs of check boxes, laid out as the Mapper page's are: the log options' grid held
// the page at 860px in German, its path field and format list sharing columns with everything else on the card
void dlgProfilePreferences::reflowGeneralCards()
{
    takeOutOfLayout(gridLayout_groupBox_miscellaneous, {mFORCE_SAVE_ON_EXIT, mAlertOnNewData});
    gridLayout_groupBox_miscellaneous->addWidget(
            makeFlowRow(groupBox_miscellaneous, qsl("settingsSavingChoices"), {mFORCE_SAVE_ON_EXIT, mAlertOnNewData}, scmChoiceGroupGap, uiDesign::FlowLayout::LineAlignment::Top), 1, 0, 1, 2);

    gridLayout_groupBox_encoding->addWidget(makeFlowRow(groupBox_encoding, qsl("settingsGeneralRow_guiLanguage"), {label_guiLanguage, comboBox_guiLanguage}), 0, 0, 1, 2);

    takeOutOfLayout(gridLayout_groupBox_logOptions, {mIsToLogInHtml, mIsLoggingTimestamps});
    // The extension is the end of the file name it follows, so the two never part
    QWidget* pLogFileName = makeFlowPiece(groupBox_logOptions, qsl("settingsLogFileNameField"), QBoxLayout::LeftToRight, {lineEdit_logFileName, label_logFileNameExtension});
    QWidget* pLogFolderRow = makeFlowRow(groupBox_logOptions, qsl("settingsGeneralRow_logFolder"), {label_whereToLog, lineEdit_logFileFolder, pushButton_whereToLog, pushButton_resetLogDir});
    // A path is read rather than chosen, so the field showing it takes the rest of its line
    static_cast<uiDesign::FlowLayout*>(pLogFolderRow->layout())->setStretchWidget(lineEdit_logFileFolder);
    const QList<QWidget*> logOptionRows{mIsToLogInHtml,
                                        mIsLoggingTimestamps,
                                        pLogFolderRow,
                                        makeFlowRow(groupBox_logOptions, qsl("settingsGeneralRow_logFileNameFormat"), {label_logFileNameFormat, comboBox_logFileNameFormat}),
                                        makeFlowRow(groupBox_logOptions, qsl("settingsGeneralRow_logFileName"), {label_logFileName, pLogFileName})};
    for (int row = 0, rows = logOptionRows.size(); row < rows; ++row) {
        gridLayout_groupBox_logOptions->addWidget(logOptionRows.at(row), row, 0, 1, 4);
    }
    gridLayout_groupBox_logOptions->setVerticalSpacing(scmFlowRowGap);
}

// The Privacy page's .ui cards, laid out as the Mapper page's are; the two built by the shell take their flow rows
// where they are made
void dlgProfilePreferences::reflowPrivacyCards()
{
    // Two columns, each as wide as its own longest words, the second under the first once they no longer fit
    QWidget* pLeadingChoices = makeFlowPiece(groupBox_ssl, qsl("settingsSslLeadingChoices"), QBoxLayout::TopToBottom, {checkBox_self_signed, checkBox_ignore_all});
    QWidget* pTrailingChoices = makeFlowPiece(groupBox_ssl, qsl("settingsSslTrailingChoices"), QBoxLayout::TopToBottom, {checkBox_expired});
    gridLayout_groupBox_ssl->addWidget(
            makeFlowRow(groupBox_ssl, qsl("settingsSslChoices"), {pLeadingChoices, pTrailingChoices}, scmChoiceGroupGap, uiDesign::FlowLayout::LineAlignment::Top), 1, 0, 1, 2);

    // Turned on its side here rather than by setCardDescription(), which would nest the row in a row of its own
    hBoxLayout_groupBox_purgeMediaCache->setDirection(QBoxLayout::TopToBottom);
    hBoxLayout_groupBox_purgeMediaCache->addWidget(makeFlowRow(groupBox_purgeMediaCache, qsl("settingsPrivacyRow_purgeMediaCache"), {label_purgeMediaCache, buttonPurgeMediaCache}));
}

// What a colour card holds: the colours of its own, a divider with whatever is said under it, the sixteen ANSI
// colours a colour and its light twin to a row, and a footer under each half
struct ColourCardContents
{
    QList<std::pair<QWidget*, QWidget*>> namedColours;
    QList<QWidget*> divider;
    QList<std::pair<QWidget*, QWidget*>> ansiColours;
    QWidget* pFooterLeading = nullptr;
    QWidget* pFooterTrailing = nullptr;
};

// The Main display's colours and the map's are one recipe: label, well, label, well, every well the one width so
// they line up down the card, and one pair to a row once two no longer fit the column. The card keeps its .ui grid,
// emptied, with the colours in its one cell.
static void layOutColourCard(QGroupBox* pCard, QGridLayout* pCardGrid, const QString& objectName, const ColourCardContents& contents)
{
    auto* pColours = new QWidget(pCard);
    pColours->setObjectName(objectName);
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    policy.setHeightForWidth(true);
    pColours->setSizePolicy(policy);
    // The spacing the card's grid was drawn with, which on a style that spaces by the kinds of control is decided
    // per pair of them and left to the layout
    auto* pLayout = new uiDesign::PairGridLayout(pColours, pCardGrid->horizontalSpacing(), pCardGrid->verticalSpacing());
    pLayout->setContentsMargins(0, 0, 0, 0);
    const auto take = [](QWidget* pWidget) {
        if (pWidget) {
            detachFromLayout(pWidget);
        }
        return pWidget;
    };
    for (const auto& [pLabel, pWell] : contents.namedColours) {
        pLayout->addPair(take(pLabel), take(pWell));
    }
    for (auto* pWidget : contents.divider) {
        pLayout->addSpanningWidget(take(pWidget));
    }
    for (const auto& [pLabel, pWell] : contents.ansiColours) {
        pLayout->addPair(take(pLabel), take(pWell));
    }
    pLayout->addFooter(take(contents.pFooterLeading), take(contents.pFooterTrailing));
    pCardGrid->addWidget(pColours, 0, 0);
}

void dlgProfilePreferences::reflowColourCards()
{
    layOutColourCard(groupBox_displayColors,
                     gridLayout_groupBox_displayColors,
                     qsl("settingsDisplayColours"),
                     {.namedColours = {{label_foreground_color, pushButton_foreground_color},
                                       {label_background_color, pushButton_background_color},
                                       {label_command_line_foreground_color, pushButton_command_line_foreground_color},
                                       {label_command_line_background_color, pushButton_command_line_background_color},
                                       {label_command_foreground_color, pushButton_command_foreground_color},
                                       {label_command_background_color, pushButton_command_background_color}},
                      .divider = {line_colorsSeparator, label_ANSI_color_buttons_introduction},
                      .ansiColours = {{label_black, pushButton_black},
                                      {label_lBlack, pushButton_lBlack},
                                      {label_red, pushButton_red},
                                      {label_lRed, pushButton_lRed},
                                      {label_green, pushButton_green},
                                      {label_lGreen, pushButton_lGreen},
                                      {label_yellow, pushButton_yellow},
                                      {label_lYellow, pushButton_lYellow},
                                      {label_blue, pushButton_blue},
                                      {label_lBlue, pushButton_lBlue},
                                      {label_magenta, pushButton_magenta},
                                      {label_lMagenta, pushButton_lMagenta},
                                      {label_cyan, pushButton_cyan},
                                      {label_lCyan, pushButton_lCyan},
                                      {label_white, pushButton_white},
                                      {label_lWhite, pushButton_lWhite}},
                      .pFooterLeading = checkBox_allowServerToRedefineColors,
                      .pFooterTrailing = pushButton_resetColors});

    layOutColourCard(groupBox_mapperColors,
                     gridLayout_groupBox_mapperColors,
                     qsl("settingsMapColours"),
                     {.namedColours = {{label_39, pushButton_foreground_color_2},
                                       {label_40, pushButton_background_color_2},
                                       {label_4, pushButton_roomBorderColor},
                                       {label_mapInfoBg, pushButton_mapInfoBg},
                                       {label_lowerLevelColor, pushButton_lowerLevelColor},
                                       {label_upperLevelColor, pushButton_upperLevelColor},
                                       {label_pushButton_roomCollisionBorderColor, pushButton_roomCollisionBorderColor},
                                       {label_mapGridColor, pushButton_mapGridColor}},
                      .divider = {line_mapperColorsSeparator},
                      .ansiColours = {{label_17, pushButton_black_2},
                                      {label_49, pushButton_Lblack_2},
                                      {label_42, pushButton_red_2},
                                      {label_50, pushButton_Lred_2},
                                      {label_43, pushButton_green_2},
                                      {label_51, pushButton_Lgreen_2},
                                      {label_48, pushButton_yellow_2},
                                      {label_52, pushButton_Lyellow_2},
                                      {label_44, pushButton_blue_2},
                                      {label_53, pushButton_Lblue_2},
                                      {label_45, pushButton_magenta_2},
                                      {label_54, pushButton_Lmagenta_2},
                                      {label_46, pushButton_cyan_2},
                                      {label_55, pushButton_Lcyan_2},
                                      {label_47, pushButton_white_2},
                                      {label_56, pushButton_Lwhite_2}},
                      .pFooterTrailing = reset_colors_button_2});

    // The four map colours that may be see-through: a swatch over a checkerboard and how opaque the colour is
    for (QPushButton* pWell : {pushButton_mapInfoBg, pushButton_lowerLevelColor, pushButton_upperLevelColor, pushButton_mapGridColor}) {
        uiDesign::AlphaColourWell::on(pWell);
    }
}

// Other pages took three of this card's checkboxes and the double-click controls joined it, leaving the .ui
// grid with half-empty rows; re-laid as a checkbox pair, a row for each long-labelled checkbox, then one row per
// label-and-control.
void dlgProfilePreferences::reflowDisplayOptionsCard()
{
    const QList<QWidget*> displayOptions{checkBox_showTimeStamps,
                                         checkBox_enableTextAnalyzer,
                                         checkBox_useWideAmbiguousEastAsianGlyphs,
                                         checkBox_enableOSC8Hyperlinks,
                                         label_controlCharacterHandling,
                                         comboBox_controlCharacterHandling,
                                         doubleclick_ignore_label,
                                         doubleclick_ignore_lineedit};
    takeOutOfLayout(gridLayout_groupBox_displayOptions, displayOptions);
    gridLayout_groupBox_displayOptions->addWidget(checkBox_showTimeStamps, 0, 0);
    gridLayout_groupBox_displayOptions->addWidget(checkBox_enableTextAnalyzer, 0, 1);
    gridLayout_groupBox_displayOptions->addWidget(checkBox_useWideAmbiguousEastAsianGlyphs, 1, 0, 1, 2);
    gridLayout_groupBox_displayOptions->addWidget(checkBox_enableOSC8Hyperlinks, 2, 0, 1, 2);
    gridLayout_groupBox_displayOptions->addWidget(label_controlCharacterHandling, 3, 0);
    gridLayout_groupBox_displayOptions->addWidget(comboBox_controlCharacterHandling, 3, 1);
    // Label beside its field rather than above, saving a row and lining up with the combo box above
    gridLayout_groupBox_displayOptions->addWidget(doubleclick_ignore_label, 4, 0);
    gridLayout_groupBox_displayOptions->addWidget(doubleclick_ignore_lineedit, 4, 1);
}

// The .ui ends this card with the reconnect notice, but the workarounds moved here land after it,
// stranding the notice between checkboxes.
void dlgProfilePreferences::reflowCompatibilityCard()
{
    takeOutOfLayout(gridLayout_groupBox_specialOptions, {checkBox_USE_IRE_DRIVER_BUGFIX, checkBox_expectCSpaceIdInColonLessMColorCode, need_reconnect_for_specialoption});
    gridLayout_groupBox_specialOptions->addWidget(checkBox_USE_IRE_DRIVER_BUGFIX, 2, 1);
    // Its label is too long to share a row
    gridLayout_groupBox_specialOptions->addWidget(checkBox_expectCSpaceIdInColonLessMColorCode, 3, 0, 1, 2);
    gridLayout_groupBox_specialOptions->addWidget(need_reconnect_for_specialoption, 4, 0, 1, 2);
}

void dlgProfilePreferences::fitColumns()
{
    for (const auto& place : std::as_const(mCategories)) {
        fitColumn(qobject_cast<QScrollArea*>(mpStackedWidget_categories->widget(place.pageIndex)));
    }
    for (const int page : std::as_const(mSubpageIndexes)) {
        fitColumn(qobject_cast<QScrollArea*>(mpStackedWidget_categories->widget(page)));
    }
    fitColumn(mpScrollArea_searchResults);
}

void dlgProfilePreferences::invalidateSidebarWidth()
{
    mSidebarWidthKnown = false;
}

// What the sidebar is drawn at with its names showing, measured the way the
// editor measures its own: the widest of the rows it holds, at the names it
// holds. An interface font, a translation's longer category names and the style
// drawing the rows all move it, so it is taken from the rows rather than
// written down here. Kept once measured, as a resize asks for
// the answer on every frame of a drag and nothing that would change it can
// happen in the middle of one.
int dlgProfilePreferences::measuredSidebarWidth() const
{
    if (mSidebarWidthKnown) {
        return mSidebarWidth;
    }
    // A rail collapses nothing, which is the right answer for a half-built
    // shell - and not one to keep, as the shell will not stay half-built
    if (!mpListWidget_categories) {
        return uiDesign::scmSidebarRailWidth;
    }

    // A list takes its platform's item view font when it is polished, a point smaller than the window's on macOS,
    // and the first measurement comes while the dialog is still being built
    mpListWidget_categories->ensurePolished();

    // What the style in force says each row needs at its own name and glyph,
    // which is a different number in each appearance - see
    // uiDesign::sidebarRowWidth()
    int widestRow = 0;
    for (int row = 0, rows = mpListWidget_categories->count(); row < rows; ++row) {
        widestRow = std::max(widestRow, uiDesign::sidebarRowWidth(mpListWidget_categories, mpListWidget_categories->item(row)->text()));
    }
    int wanted = 2 * uiDesign::scmSidebarPadding + widestRow;
    // ...and the row of names is not the only thing in the pane: the wordmark
    // over it is the Mudlet icon, the gap after it and a word set larger than
    // the names are, and it is drawn out rather than elided
    if (const QWidget* pHeader = mpLabel_wordmark ? mpLabel_wordmark->parentWidget() : nullptr; pHeader) {
        wanted = std::max(wanted, 2 * uiDesign::scmSidebarPadding + pHeader->sizeHint().width());
    }

    mSidebarWidth = std::clamp(wanted, uiDesign::scmSidebarRailWidth, scmSidebarMaximumWidth);
    mSidebarWidthKnown = true;
    return mSidebarWidth;
}

// Measured, not constant: the interface font, platform scrollbar and translated names all move it
dlgProfilePreferences::SidebarWidths dlgProfilePreferences::sidebarWidths() const
{
    SidebarWidths widths;
    QWidget* pContent = mpWidget_titleRow ? mpWidget_titleRow->parentWidget() : nullptr;
    // Zero collapses nothing, which is right for a half-built shell
    if (!pContent || !pContent->layout() || !mpLabel_pageTitle || !mpStackedWidget_categories) {
        return widths;
    }
    const QMargins contentMargins = pContent->layout()->contentsMargins();

    const auto* pPage = qobject_cast<const QScrollArea*>(mpStackedWidget_categories->currentWidget());
    const int scrollBarWidth = pPage ? pPage->verticalScrollBar()->sizeHint().width() : 0;

    // The row's own chrome (icon, spacings, margins) plus the widest title it could show. The chevrons are
    // usually hidden, and a hidden widget reports no size, so the wider one is added explicitly.
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

    // Taken over every page, as a width that followed the category would resize the window on every sidebar
    // click
    int widestPageMinimum = 0;
    for (int page = 0, pages = mpStackedWidget_categories->count(); page < pages; ++page) {
        const auto* pStacked = qobject_cast<const QScrollArea*>(mpStackedWidget_categories->widget(page));
        const QWidget* pColumn = pStacked ? pStacked->widget() : nullptr;
        if (pColumn) {
            widestPageMinimum = std::max(widestPageMinimum, pColumn->minimumSizeHint().width());
        }
    }

    const int sidebarAndMargins = measuredSidebarWidth() + contentMargins.left() + contentMargins.right();
    // The reading column, or the widest page where that is wider
    const int contentColumn = std::max(scmContentColumnWidth, widestPageMinimum);
    widths.fullyExpanded = sidebarAndMargins + std::max(contentColumn + scrollBarWidth, titleRowChrome + widestTitle);
    // The widest page's minimum held to the reading column, so a wide page does not move the collapse point for
    // the others - raised to what the page on show needs, so a page wider than the column takes the names rather
    // than scrolling sideways. Longer names move it, and updateSidebarMode() widens the window after them.
    const QWidget* pShownColumn = pPage ? pPage->widget() : nullptr;
    const int shownPageMinimum = pShownColumn ? pShownColumn->minimumSizeHint().width() : 0;
    widths.collapseBelow = sidebarAndMargins + std::max(std::min(scmContentColumnWidth, widestPageMinimum), shownPageMinimum) + scrollBarWidth;
    widths.narrowest = uiDesign::scmSidebarRailWidth + contentMargins.left() + contentMargins.right() + widestPageMinimum + scrollBarWidth;
    return widths;
}

// Two answers decide the sidebar's mode, and only one of them is the user's:
// the toggle's stored preference says whether the names are wanted, and the
// window's width says whether there is room to grant it. A window with no room
// takes them away without writing anything down, so widening it again is
// settled by the preference alone.
void dlgProfilePreferences::updateSidebarMode()
{
    // While the shell is being put together the constructor is still measuring
    // pages and restoring the window's geometry in an order of its own, and the
    // width a window is held to is one of the things it settles - so this waits
    // for the call the constructor makes once all of that is done
    if (!mpWidget_sidebar || !mShellReady) {
        return;
    }
    // Zero means nothing has been measured yet
    const SidebarWidths widths = sidebarWidths();
    if (!widths.fullyExpanded) {
        return;
    }
    // Below narrowest even the rail leaves the widest page too little, so the window stops there rather than
    // cutting it off. There is no widest: a page grows with the window.
    setMinimumWidth(std::max(scmMinimumWindowWidth, widths.narrowest));

    const int sidebarWidth = measuredSidebarWidth();
    const bool namesRemeasured = mSidebarWidthDecidedAt && sidebarWidth != mSidebarWidthDecidedAt;
    mSidebarWidthDecidedAt = sidebarWidth;
    // The window width, not the space left over: the threshold is what the expanded sidebar needs, so
    // collapsing cannot flip the test and oscillate
    int decidingWidth = width();
    // Longer names moved the threshold past a window that was showing them, which nobody resized: re-fit the
    // window to them the way first show sizes it, and give them up only if the screen cannot hold that width
    if (namesRemeasured && mSidebarNamesFit && mSidebarLabelsShown && decidingWidth < widths.collapseBelow && refitWindowTo(widths.collapseBelow)) {
        decidingWidth = widths.collapseBelow;
    }
    mSidebarNamesFit = decidingWidth >= widths.collapseBelow;
    setSidebarCollapsed(!(mSidebarNamesFit && mSidebarLabelsShown));
    updateSidebarToggle();
}

// The resize may arrive later than this returns on a platform that sizes windows asynchronously, so the
// caller decides with the width asked for rather than reading it back
bool dlgProfilePreferences::refitWindowTo(const int wantedWidth)
{
    const QScreen* pScreen = screen();
    if (!pScreen) {
        return false;
    }
    const QRect available = pScreen->availableGeometry();
    const QRect frame = frameGeometry();
    const int frameWidth = wantedWidth + frame.width() - width();
    if (frameWidth > available.width()) {
        return false;
    }
    // Grown in place where it fits, otherwise moved back far enough to keep its right edge on the screen
    if (const int overflow = frame.left() + frameWidth - 1 - available.right(); overflow > 0) {
        move(std::max(available.left(), frame.left() - overflow), frame.top());
    }
    resize(wantedWidth, height());
    return true;
}

// The one place the preference changes, so that the toggle and the dialog hold
// the same answer. The space-driven collapse in updateSidebarMode()
// deliberately does not come through here.
void dlgProfilePreferences::setSidebarLabelsShown(const bool shown)
{
    mSidebarLabelsShown = shown;
    // Written as it is chosen rather than on close, the way every other choice
    // in this dialog is applied as it is made
    MudletApp::getQSettings()->setValue(qsl("settingsSidebarLabelsShown"), shown);
    updateSidebarMode();
}

// The chevron on the line between the sidebar and the settings. What it is
// called is what will become of the names - never of the sidebar, which stays:
// a rail turns every row's name into its tooltip, so a reader who cannot see
// the control has to be told that much in words.
void dlgProfilePreferences::updateSidebarToggle()
{
    if (!mpToggle_sidebar) {
        return;
    }
    const bool namesShowing = mSidebarLabelsShown && mSidebarNamesFit;
    // Pointing the way the sidebar will go: into the seam while the names are
    // there to be given up, out of it while they are not
    mpToggle_sidebar->setPointingLeft(namesShowing);

    //: Tooltip and accessible name of the chevron on the line down the right of the settings dialog's sidebar while the category names are showing. Pressing it leaves the sidebar in place as a rail of icons - it closes nothing - so the wording is about the labels rather than the sidebar.
    const QString minimise = tr("Minimise the sidebar to icons");
    //: Tooltip and accessible name of that same chevron while the sidebar is a rail of icons. Pressing it puts the category names back beside the icons.
    const QString showLabels = tr("Show the sidebar's labels");
    const QString wording = namesShowing ? minimise : showLabels;
    // The chevron is drawn rather than written, so its text is the name a
    // screen reader is given rather than anything on show
    mpToggle_sidebar->setText(wording);
    mpToggle_sidebar->setAccessibleName(wording);
    // A window with no room for the names cannot be talked into them, so rather
    // than doing nothing when it is pressed the control says why it will not
    mpToggle_sidebar->setEnabled(mSidebarNamesFit);
    //: Tooltip on that same chevron when the settings window is too narrow to fit the category names, which is why pressing it is not offered.
    mpToggle_sidebar->setToolTip(utils::richText(mSidebarNamesFit ? wording : tr("The window is too narrow for the sidebar's labels")));
    // The sidebar's width is what the seam is, and this is called from every
    // path that can have moved it
    mpToggle_sidebar->reposition();
}

void dlgProfilePreferences::setSidebarCollapsed(const bool collapsed)
{
    if (!uiDesign::setSidebarCollapsed(mpWidget_sidebar, mpListWidget_categories, qsl("settingsSidebarSeparator"), collapsed, settingsSidebarMetrics(measuredSidebarWidth()))) {
        return;
    }
    mpLabel_wordmark->setVisible(!collapsed);
    // The item keeps its text as its accessible name, and offers it as a tooltip while it is not drawn.
    // The editor's rows carry one of their own, naming the shortcut as well, so
    // this half stays here rather than going with the collapse.
    for (int row = 0, rows = mpListWidget_categories->count(); row < rows; ++row) {
        QListWidgetItem* pItem = mpListWidget_categories->item(row);
        pItem->setToolTip(collapsed ? pItem->text() : QString());
    }
}

// Qt appends a reparented widget to the end of the focus chain, leaving the .ui's ~190 tab stops in old tab
// order. Only the showing page's widgets are traversed, so one chain in sidebar order fixes it.
void dlgProfilePreferences::rebuildTabOrder()
{
    // Traversal skips a hidden chevron or notice rather than getting trapped on it. The
    // sidebar's own chevron comes right after the list it belongs to, which is
    // where it is drawn - the editor leaves its copy in the window's natural
    // chain, and this is that place in a chain the dialog writes out.
    QList<QWidget*> chain{mpLineEdit_search, mpButton_searchBack, mpButton_subpageBack, mpListWidget_categories, mpToggle_sidebar, mpLabel_noProfileNotice};
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

// With instant apply, scrolling the page over a spin or combo box would silently change the setting.
// Both ignore the wheel unless focused, and do not take focus from it - see eventFilter().
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

// The half of the sidebar's mode no preference decides: the names go whenever
// the window is too narrow to hold them, whatever the toggle was told.
void dlgProfilePreferences::resizeEvent(QResizeEvent* pEvent)
{
    QDialog::resizeEvent(pEvent);
    updateSidebarMode();
}

// Each of these moves what a sidebar row measures: its name, the font it is
// drawn in, or the metrics the style hands out for it - and the mode that
// measurement decides is re-taken here rather than left to the next resize
void dlgProfilePreferences::changeEvent(QEvent* pEvent)
{
    QDialog::changeEvent(pEvent);
    if (pEvent->type() == QEvent::LanguageChange || pEvent->type() == QEvent::StyleChange || pEvent->type() == QEvent::FontChange) {
        invalidateSidebarWidth();
        updateSidebarMode();
    }
}

// Returning to the window is when settings can be re-read without interrupting anything - see
// refreshFromSettings()
bool dlgProfilePreferences::event(QEvent* pEvent)
{
    const bool handled = QDialog::event(pEvent);
    if (pEvent->type() == QEvent::WindowActivate) {
        refreshFromSettings();
    }
    // Sent when this dialog's stylesheet or the application's is replaced, and by an appearance change, which is
    // slot_setAppearance()'s: restyled here first, the shell would look drawn for the new mode to it and it would
    // stop before the rest of what it redraws
    if (pEvent->type() == QEvent::StyleChange && mpWidget_shell && MudletApp::darkMode() == mShellStyledForDarkPage) {
        bool themeSurface = false;
        if (shellPalette(themeSurface) != mShellPalette) {
            applyShellStyle();
        }
    }
    return handled;
}

bool dlgProfilePreferences::eventFilter(QObject* pObject, QEvent* pEvent)
{
    // A wrapped checkbox's label greys out with it and toggles it when clicked, as its own text would
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
            // QApplication only propagates a wheel event no filter has handled, and returning true counts
            // as handled, so without this the wheel would do nothing
            for (QWidget* pAncestor = pControl->parentWidget(); pAncestor; pAncestor = pAncestor->parentWidget()) {
                if (auto* pScrollArea = qobject_cast<QAbstractScrollArea*>(pAncestor); pScrollArea) {
                    QCoreApplication::sendEvent(pScrollArea->viewport(), pEvent);
                    break;
                }
            }
            return true;
        }
    }
    // Styling the items removes the native focus rectangle; a property restores it, as QSS cannot select
    // on whether a subcontrol's widget has focus
    if (pObject == mpListWidget_categories && (pEvent->type() == QEvent::FocusIn || pEvent->type() == QEvent::FocusOut)) {
        mpListWidget_categories->setProperty(scmProp_focused, pEvent->type() == QEvent::FocusIn);
        mpListWidget_categories->style()->polish(mpListWidget_categories);
    }
    return QDialog::eventFilter(pObject, pEvent);
}

void dlgProfilePreferences::showCategory(const QString& key, QWidget* pSpotlightTarget)
{
    // Leaving the results is driven by the selection changing, which a deep link to the current category
    // does not do
    if (mSearchActive) {
        mSubpageBeforeSearch.clear();
        mpLineEdit_search->clear();
    }
    QString category = key;
    if (!mCategories.contains(category)) {
        // Every deep link is C++, so an unknown one is a typo
        qWarning() << "dlgProfilePreferences::showCategory(...) WARNING - there is no settings category" << key << "- showing General instead.";
        category = scmCategory_general;
    }
    mpListWidget_categories->setCurrentRow(categoryRow(category));
    spotlight(pSpotlightTarget);
}

// Deferred because setTab() runs before the dialog is shown and laid out
void dlgProfilePreferences::spotlight(QWidget* pTarget)
{
    if (!pTarget) {
        return;
    }
    QTimer::singleShot(0ms, this, [this, pTarget = QPointer<QWidget>(pTarget)]() {
        if (!pTarget) {
            return;
        }
        auto* pScrollArea = qobject_cast<QScrollArea*>(mpStackedWidget_categories->currentWidget());
        QWidget* pColumn = pScrollArea ? pScrollArea->widget() : nullptr;
        if (!pColumn || !pColumn->isAncestorOf(pTarget)) {
            return;
        }
        // A y margin of half the viewport is as close to centred as the page allows
        pScrollArea->ensureWidgetVisible(pTarget, 0, pScrollArea->viewport()->height() / 2);

        // A second deep link replaces the first's pulse rather than fading on top of it
        delete mpWidget_spotlight.data();
        const QColor accent = palette().color(QPalette::Highlight);
        auto* pPulse = new QWidget(pColumn);
        mpWidget_spotlight = pPulse;
        pPulse->setObjectName(qsl("settingsSpotlight"));
        pPulse->setAttribute(Qt::WA_TransparentForMouseEvents);
        pPulse->setAttribute(Qt::WA_StyledBackground);
        // A card fills the column's width, so an outline around it would be clipped on three sides;
        // the intersection keeps them on the card
        const QRect targetRect(pTarget->mapTo(pColumn, QPoint(0, 0)), pTarget->size());
        pPulse->setGeometry(targetRect.adjusted(-2, -2, 2, 2).intersected(pColumn->rect()));
        // Start at full strength: the animation only fades it, and its first tick can be slow on the busy
        // event loop a deep link opens into
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

// Walks the real widget tree rather than a hand-written list, so controls added to the .ui are searchable
void dlgProfilePreferences::buildSearchIndex()
{
    mSearchCards.clear();
    // A category's cards are indexed before its subpages', so a query matching both shows the opener first
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

// A search moves every match onto the results page and the previous ones back - most of the dialog for a
// one- or two-letter query ("s" 63ms, "sh" 76ms, "shortcuts" 6ms) - so only the pause after typing is answered.
void dlgProfilePreferences::queueSearch(const QString& query)
{
    mPendingSearch = query;
    // Emptying the field returns the borrowed cards; any delay would read as the dialog sticking
    if (query.trimmed().isEmpty()) {
        mpTimer_search->stop();
        runSearch(query);
        return;
    }
    mpTimer_search->start();
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
        mSubpageBeforeSearch = mCurrentSubpage;
        mCurrentSubpage.clear();
        mpButton_subpageBack->hide();
        // Clear the selection, never the current row: an item view focused with no current index takes the
        // first, which reads as choosing General and ends the search. Handing back a borrowed card that
        // held focus does exactly that, as reparenting clears focus.
        const QSignalBlocker blocker(mpListWidget_categories);
        mpListWidget_categories->clearSelection();
    }

    setUpdatesEnabled(false);
    returnSearchedCardsHome();
    clearSearchHighlights();

    // Moving a card fires no change signal, but the focused control reports editingFinished as focus leaves.
    // Restored by hand, because the tail below is deliberately outside the guard.
    const bool wasPopulating = mPopulating;
    mPopulating = true;

    QString lastCategory;
    int matchCount = 0;
    QList<const QWidget*> matchedCards;
    QStringList linkedSubpages;
    for (auto& entry : mSearchCards) {
        // A card hidden by the profile's state is not an option, so not a result
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
        // Taking a subpage's card would empty the page its opener leads to, so the results link in instead
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
        // Not that it is still at homeIndex (taking an earlier card shifts it), only that this is its home layout
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
        // Rich text, so the query must be escaped
        //: Empty state of the settings search; %1 is what the user typed
        const QString message = tr("No results in settings for \"%1\"").arg(query.trimmed().toHtmlEscaped());
        //: Offered under the settings search empty state; %1 is a link labelled "Mudlet support"
        const QString help = tr("Need help? Visit %1").arg(qsl("<a href=\"%1\">%2</a>").arg(mpItem_support->data(scmRole_externalUrl).toString(), mpItem_support->text()));
        setLinkedText(mpLabel_searchEmpty, qsl("%1<br>%2").arg(message, help));
    }
    mpLabel_searchEmpty->setVisible(!matchCount);
    mpLayout_searchResults->setStretch(0, matchCount ? 0 : 1);
    //: Title shown in place of a category name while the settings search is showing its results
    mpLabel_pageTitle->setText(tr("Search results"));
    // The back chevron takes the category icon's place
    mpLabel_pageTitleIcon->hide();
    mpButton_searchBack->show();
    mpStackedWidget_categories->setCurrentIndex(mSearchResultsPageIndex);
    fitColumn(mpScrollArea_searchResults);
    setUpdatesEnabled(true);
}

// The index is in ascending position per page and unmoved cards kept theirs, so re-inserting at the indexed
// position restores each page
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

// The field is cleared before the index: clearing it sends borrowed cards home, which needs the index
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
        // The query was cleared rather than a category chosen, so restore the page the search interrupted
        mpListWidget_categories->setCurrentRow(qMax(0, categoryRow(mCategoryBeforeSearch)));
    } else {
        // Only the selection was cleared, so the row is still current and setCurrentRow() would not signal
        mpListWidget_categories->item(row)->setSelected(true);
        slot_categorySelected(row);
    }
    // For a subpage, its category page is only halfway back
    if (const QString subpage = mSubpageBeforeSearch; !subpage.isEmpty()) {
        mSubpageBeforeSearch.clear();
        showSubpage(subpage.section(QLatin1Char('/'), 0, 0), subpage.section(QLatin1Char('/'), 1));
    }
    setUpdatesEnabled(true);
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
#ifdef INCLUDE_MCPSERVER
        // The MCP port is the one setting this dialog folds out of sight, and a
        // search result nobody can see answers nothing
        if (pWidget == label_mcpServerPort) {
            pushButton_mcpAdvanced->setChecked(true);
        }
#endif
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
    // On every call, as a language change renames the category
    const QListWidgetItem* pItem = mpListWidget_categories->item(categoryRow(key));
    const QString name = (pItem ? pItem->text() : key).toHtmlEscaped();
    // Inline rather than by path because the colour is applied at runtime - see restyleSidebarIcons()
    const QString iconMarkup = mCategoryIconMarkup.value(key);
    pHeader->setText(iconMarkup.isEmpty() ? name : qsl("%1&nbsp;%2").arg(iconMarkup, name));
    return pHeader;
}

// Not the card, as taking it would empty the page behind its opener, but a link to the subpage that
// spotlights the matched card
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
    // Reworded on every call, for a language change
    pLink->setText(mSubpageTitles.value(subpageKey));
    // Re-aimed at this query's matching card
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
        // Clearing the field returns borrowed cards; this category, not whatever the search interrupted, is wanted
        mSubpageBeforeSearch.clear();
        mpLineEdit_search->clear();
    }
    mCurrentSubpage.clear();
    mpButton_subpageBack->hide();
    // QStackedLayout passes focus on from the outgoing page, and taking it off a control scrolled out of
    // sight scrolls that page back to the top
    QWidget* pCurrentPage = mpStackedWidget_categories->currentWidget();
    if (QWidget* pFocus = QApplication::focusWidget(); pFocus && pCurrentPage && pCurrentPage->isAncestorOf(pFocus)) {
        mpListWidget_categories->setFocus(Qt::OtherFocusReason);
    }
    mpStackedWidget_categories->setCurrentIndex(mCategories.contains(key) ? mCategories.value(key).pageIndex : mCategories.value(scmCategory_general).pageIndex);
    auto* pShownPage = qobject_cast<QScrollArea*>(mpStackedWidget_categories->currentWidget());
    // Before the width is capped, so the page is measured as it will be shown
    placeBannerOn(pShownPage ? pShownPage->widget() : nullptr);
    fitColumn(pShownPage);
    // Card padding arrives with the stylesheet when the page is first shown, after the fit above. Without a
    // second fit, the check boxes are wrapped against a page measured 34px short.
    QTimer::singleShot(0ms, this, [this, pShownPage]() {
        if (pShownPage && mpStackedWidget_categories->currentWidget() == pShownPage) {
            fitColumn(pShownPage);
            // A grown cap moves the sidebar breakpoint and the window's maximum width, otherwise not
            // re-taken until the next resize
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
    const QString key = pItem->data(scmRole_categoryKey).toString();
    if (key.isEmpty()) {
        return;
    }
    // A search keeps the sidebar's current row, so choosing that category again is no row change either
    if (mSearchActive) {
        slot_categorySelected(mpListWidget_categories->row(pItem));
        return;
    }
    // Choosing a subpage's own category is no row change, so the row-changed slot would not leave the subpage
    if (mCurrentSubpage.startsWith(key + QLatin1Char('/'))) {
        leaveSubpage();
    }
}

// With the colored icons on, a page's colour: what a toolbar button also opens takes that button's, and the input
// line and accessibility have their own. The rest stay grey.
static uiDesign::IdentityColour settingsPageIdentity(const QString& iconFile)
{
    static const QHash<QString, uiDesign::IdentityColour> identities{{qsl("settings-input.svg"), uiDesign::scmIdentity_input},
                                                                     {qsl("settings-mapper.svg"), uiDesign::scmIdentity_map},
                                                                     {qsl("settings-chat.svg"), uiDesign::scmIdentity_discord},
                                                                     {qsl("settings-connection.svg"), uiDesign::scmIdentity_connect},
                                                                     {qsl("settings-accessibility.svg"), uiDesign::scmIdentity_accessibility},
                                                                     {qsl("settings-support.svg"), uiDesign::scmIdentity_manual}};
    return mudlet::self() && mudlet::self()->colouredIcons() ? identities.value(iconFile) : uiDesign::IdentityColour();
}

// Only called from applyShellStyle(), which supplies the colours and is what an appearance change reruns
void dlgProfilePreferences::restyleSidebarIcons(const uiDesign::ThemeTokens& tokens)
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
        const QString file = qsl(":/icons/%1").arg(it.value().iconFile);
        if (const uiDesign::IdentityColour identity = settingsPageIdentity(it.value().iconFile); identity.hue >= 0.0) {
            pItem->setIcon(uiDesign::identityIcon(file, file, identity, tokens, tokens.disabledText));
            mCategoryIconMarkup.insert(it.key(), inlineGlyph(uiDesign::identityGlyph(file, identity, tokens)));
            continue;
        }
        const QPixmap source = uiDesign::glyphPixmap(file);
        QPixmap quiet = tintedGlyph(source, tokens.mutedText);
        QPixmap chosen = tintedGlyph(source, tokens.accentText);
        if (mudlet::self() && mudlet::self()->colouredIcons()) {
            uiDesign::addPaintDots(quiet, file, tokens);
            uiDesign::addPaintDots(chosen, file, tokens);
        }
        QIcon icon(quiet);
        // Otherwise the view makes one by washing the icon in the highlight colour
        icon.addPixmap(chosen, QIcon::Selected);
        pItem->setIcon(icon);
        mCategoryIconMarkup.insert(it.key(), inlineGlyph(quiet));
    }
    if (mpItem_support) {
        const QString file = qsl(":/icons/settings-support.svg");
        const uiDesign::IdentityColour identity = settingsPageIdentity(qsl("settings-support.svg"));
        mpItem_support->setIcon(identity.hue >= 0.0 ? uiDesign::identityIcon(file, file, identity, tokens, tokens.disabledText) : QIcon(tintedGlyph(uiDesign::glyphPixmap(file), tokens.mutedText)));
    }
    // Built headers hold the previous theme's icon; rewording them puts the new one in
    const QStringList builtHeaders = mSearchCategoryHeaders.keys();
    for (const QString& key : builtHeaders) {
        searchCategoryHeader(key);
    }
    // The title copied the row's icon when the page was shown, which the first time precedes this.
    // Hidden means a subpage or the search results, which show none.
    if (mpLabel_pageTitleIcon && !mpLabel_pageTitleIcon->isHidden()) {
        if (const QListWidgetItem* pCurrent = mpListWidget_categories->currentItem()) {
            mpLabel_pageTitleIcon->setPixmap(pCurrent->icon().pixmap(QSize(20, 20), devicePixelRatioF()));
        }
    }
}

// The application's palette, not the dialog's: a stylesheet freezes its widget's palette, and the dialog's is
// still stale when signal_appearanceChanged() arrives. A stylesheet painting plain widgets paints the controls
// on the cards, so it decides; read off an unparented probe, which starts from qApp's palette.
QPalette dlgProfilePreferences::shellPalette(bool& themeSurface) const
{
    themeSurface = false;
    if (qApp->styleSheet().isEmpty() && styleSheet().isEmpty()) {
        return QApplication::palette();
    }
    QWidget probe;
    probe.setStyleSheet(styleSheet());
    probe.ensurePolished();
    QPalette palette = probe.palette();
    const QBrush window = palette.brush(QPalette::Active, QPalette::Window);
    if (palette.isBrushSet(QPalette::Active, QPalette::Window) && window.style() == Qt::SolidPattern && window.color().alpha() == 255) {
        palette.setColor(QPalette::Base, window.color());
        themeSurface = true;
        return palette;
    }
    // A gradient or translucent background is no colour to draw a surface in, and Qt copies it into Base too
    const QPalette appPalette = QApplication::palette();
    for (const auto group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
        for (const auto role : {QPalette::Window, QPalette::Base}) {
            palette.setBrush(group, role, appPalette.brush(group, role));
        }
    }
    return palette;
}

// Kept on the shell widget rather than on the dialog, whose stylesheet
// mudlet::showOptionsDialog() assigns the profile's Lua one to on every show.
// Every selector is scoped by objectName, by the card property, or by the stack
// the pages sit in - so a profile stylesheet still reaches what the shell does
// not draw itself, which on a page is the labels, the check boxes and the
// buttons rather than the fields a setting is typed into.
void dlgProfilePreferences::applyShellStyle()
{
    if (!mpWidget_shell) {
        return;
    }
    // A style or a font is what the names were measured against, and the sheet
    // below carries the sidebar's width into the fraction of an item its accent
    // bar is - so the measurement is dropped before that sheet is built rather
    // than after it has been written from a stale one
    invalidateSidebarWidth();
    bool themeSurface = false;
    const QPalette themePalette = shellPalette(themeSurface);
    mShellPalette = themePalette;
    // The shared recipes, mixed from the application palette or the theme a
    // stylesheet paints - see shellPalette() for why it is one of those and not
    // this dialog's own
    ThemeTokens tokens = themeTokens(themePalette);
    // A theme paints every label and control on a page and a card as well, so a
    // surface of any other tone would box each of them
    if (themeSurface) {
        tokens.page = themePalette.color(QPalette::Window);
        tokens.card = tokens.page;
        tokens.pane = tokens.page;
    }
    const QColor fieldColor = tokens.field;
    const QColor textColor = tokens.text;
    const QColor accentColor = tokens.accent;
    const bool darkPage = tokens.darkPage;
    const QColor pageColor = tokens.page;
    const QColor borderColor = tokens.border;
    const QString hoverSoft = tokens.hoverSoft;
    const QString accentSoft = tokens.accentSoft;
    // The theme paints the labels on a banner too, so a tint would leave each of them in a box
    const QString bannerFill = themeSurface ? tokens.card.name() : accentSoft;
    const QColor accentText = tokens.accentText;
    const QColor mutedText = tokens.mutedText;
    // How far the pen is taken over what it is drawn on is this dialog's own:
    // the search wants a wash the words still read through, not a block of ink
    const QString markerSoft = rgba(tokens.marker, darkPage ? 0.75 : 0.95);
    // The two steps of the type scale this window names, in the points a
    // stylesheet is the only thing that reads
    const QString titleSize = QString::number(typeSize(TypeStep::Title));
    const QString displaySize = QString::number(typeSize(TypeStep::Display));

    if (mpAction_searchIcon) {
        mpAction_searchIcon->setIcon(QIcon(tintedGlyph(uiDesign::glyphPixmap(qsl(":/icons/settings-search.svg")), mutedText)));
    }
    restyleSidebarIcons(tokens);

    // The check indicator a checkable card's title begins with, drawn the one
    // way the editor's cards draw theirs
    const QString cardIndicatorRules = cardIndicatorStyleSheet(scmProp_settingsCard, tokens);
    // How much of the card's top padding is the title's rather than the gap
    // under it is a line of the type the title is drawn in, and only known once
    // the rules above are the ones being laid out under
    const int cardTitleHeight = measuredCardTitleHeight(mpWidget_shell, cardIndicatorRules);

    mpWidget_shell->setStyleSheet(
            qsl("#settingsShell, #settingsSidebar, #settingsContent { background-color: %1; }").arg(pageColor.name())
            // The seam down the sidebar's trailing edge, drawn the one way the
            // editor's pane draws its own - and what the chevron rides on
            + qsl("#settingsSidebar { border-right: 1px solid %1; }").arg(tokens.separator.name())
            // The panel down the left, drawn the one way the editor's is
            + sidebarStyleSheet(qsl("settingsCategoryList"), qsl("settingsSidebarSeparator"), textColor, settingsSidebarMetrics(measuredSidebarWidth()), tokens)
            + qsl("#settingsStack { background: transparent; }"
                  // The shell's own surfaces keep the page colour even when a
                  // profile stylesheet paints every QWidget it can reach.
                  // Painted rather than left transparent, as a transparent
                  // surface falls back to the palette that stylesheet changed.
                  "QWidget[settingsSurface=\"true\"] { background-color: %1; border: none; }"
                  "#settingsWordmark { font-weight: bold; font-size: %6pt; }"
                  "#settingsPageTitle { font-weight: bold; font-size: %7pt; }"
                  // Taller than the controls on a page and the one thing the
                  // panel it heads is worked from, so it takes a panel's corner
                  "#settingsSearchField { border: 1px solid %2; border-radius: %4px; padding-left: 6px; background-color: %3; }"
                  "#settingsSearchField:focus { border: 1px solid %5; }")
                      // %3 is the surface the search box is sunk into, as
                      // against the card the rest of the shell is laid out on
                      .arg(pageColor.name(), borderColor.name(), fieldColor.name(), QString::number(scmRadiusProminentInput), accentColor.name(), titleSize, displaySize)
            // The cards the pages are laid out in, drawn the one way the
            // editor's options column draws its own
            + cardStyleSheet(cardMetrics(cardTitleHeight), tokens)
            + qsl("#settingsMigrationBanner { background-color: %1; border: 1px solid %2; border-radius: %3px; }"
                  "#settingsMigrationBannerTitle { font-weight: bold; }"
                  "#settingsSearchHeader { font-weight: bold; font-size: %10pt; color: %4; }"
                  "#settingsSearchEmpty { padding: 32px; color: %5; }"
                  // The property is put on and taken off by the search itself.
                  // Under the stack, the way the shared field, mark and button
                  // rules are: a selector carrying that name outranks one that
                  // does not, so a rule meant to beat them has to carry it too.
                  "#settingsStack QLabel[searchMatch=\"true\"], #settingsStack QCheckBox[searchMatch=\"true\"],"
                  " #settingsStack QRadioButton[searchMatch=\"true\"], #settingsStack QPushButton[searchMatch=\"true\"]"
                  " { background-color: %6; border-radius: 3px; }"
                  "#settingsStack QGroupBox[searchMatch=\"true\"]::title { background-color: %6; border-radius: 3px; }"
                  // Drawn as a piece of the heading it sits beside, not a button
                  "#settingsSearchBack, #settingsSubpageBack { border: 1px solid transparent; border-radius: 6px; padding: 2px 6px; color: %4; background: transparent; }"
                  "#settingsSearchBack:hover, #settingsSubpageBack:hover { background-color: %7; }"
                  "#settingsSearchBack:focus, #settingsSubpageBack:focus { border: 1px solid %8; }"
                  // Quieter than what they describe, and indented under it
                  "#settingsCardDescription { color: %5; }"
                  // An unavailable word in the design's own quiet tone rather
                  // than the platform's, which on a light theme is a shade off
                  // the card it is written on
                  "QLabel:disabled, QCheckBox:disabled, QRadioButton:disabled, QGroupBox:disabled, QPushButton:disabled, QToolButton:disabled { color: %9; }"
                  "QLabel[settingsControlDescription=\"true\"] { color: %5; margin-left: 20px; margin-bottom: 6px; }"
                  // The wrap holder shows the card through it, named outright
                  // so a profile stylesheet cannot paint a band across it
                  "#settingsCheckBoxWrap { background: transparent; border: none; }"
                  // A row that leads somewhere, clickable across the card's width.
                  // It is a push button, so it restates everything the shared
                  // button rule would otherwise leave on it - the height of a
                  // form control included, which would add itself to this row's
                  // own padding.
                  "#settingsStack QAbstractButton[settingsChevronRow=\"true\"] { text-align: left; padding: 8px 30px 8px 10px; border: 1px solid %2; border-radius: 6px; min-height: 0px;"
                  // Qt's stylesheets cannot scale a background image, so this
                  // is the 16px copy of the icon rather than the 48px one
                  " background-color: transparent; background-image: url(:/icons/arrow-right_grey-16x.png); background-repeat: no-repeat;"
                  " background-position: right center; background-origin: padding; }"
                  "#settingsStack QAbstractButton[settingsChevronRow=\"true\"]:hover { background-color: %7; border: 1px solid %2; }"
                  "#settingsStack QAbstractButton[settingsChevronRow=\"true\"]:focus { border: 1px solid %8; }"
                  // Carries no setting, so tinted rather than framed like a card
                  "QGroupBox[settingsHero=\"true\"] { background-color: %1; border: 1px solid %8; }"
                  "#settingsHeroHeadline { font-weight: bold; font-size: %10pt; }"
                  "#settingsHeroDetail { color: %5; }")
                      .arg(bannerFill, borderColor.name(), QString::number(scmRadiusPanel), textColor.name(), mutedText.name(), markerSoft, hoverSoft, accentColor.name())
                      .arg(tokens.disabledText.name())
                      .arg(titleSize)
            // A Tab stop, and the notice's own hairline is already the accent,
            // so having the focus thickens it rather than recolouring it
            + uiDesign::noticeStyleSheet(qsl("#settingsNoProfileNotice"), tokens)
            + qsl("#settingsNoProfileNotice { color: %1; padding: %2px %3px; }"
                  "#settingsNoProfileNotice:focus { border: 2px solid %4; padding: %5px %6px; }")
                      .arg(mutedText.name(),
                           QString::number(uiDesign::scmNoticePaddingVertical),
                           QString::number(uiDesign::scmNoticePaddingHorizontal),
                           accentColor.name(),
                           QString::number(uiDesign::scmNoticePaddingVertical - 1),
                           QString::number(uiDesign::scmNoticePaddingHorizontal - 1))
            + cardIndicatorRules
            // A scroll area's bars answer only to a descendant selector
            + scrollBarStyleSheet(qsl("QScrollArea[settingsSurface=\"true\"]"), tokens)
            + scrollBarStyleSheet(qsl("#settingsCategoryList"), tokens)
            // Every control a setting is typed into or picked in is drawn the
            // one way the editor draws its own, and under the stack alone:
            // named on the shell instead, the same rules would take the search
            // box above it and the category list beside it as well.
            + inputStyleSheet(tokens, qsl("#settingsStack"))
            // ...and every choice made on a page, and every button pressed
            // on one, under that same stack. The check boxes in the search
            // header and the sidebar stay outside on purpose: they are
            // pieces of the shell rather than settings.
            + choiceStyleSheet(tokens, qsl("#settingsStack")) + buttonStyleSheet(tokens, qsl("#settingsStack")));

    // Mixed over the field rather than the card - what a placeholder is read
    // against is the inside of the control it stands in - and then moved as far
    // as it has to be to clear the floor a word that is not typed yet is held to
    const QColor placeholderText = readableOn(fieldColor, blend(fieldColor, textColor, 0.45), textColor, uiDesign::scmQuietMinimumRatio);
    for (auto* pControl : mpWidget_shell->findChildren<QWidget*>()) {
        if (!qobject_cast<QAbstractButton*>(pControl) && !qobject_cast<QLineEdit*>(pControl) && !qobject_cast<QAbstractSpinBox*>(pControl) && !qobject_cast<QComboBox*>(pControl)) {
            continue;
        }
        // A dialog makes one of its push buttons the default, which the style
        // then fills with the highlight colour and writes in white - 2.8:1 on a
        // dark theme, on whichever button happened to be first. Nothing here is
        // a default action: a setting is applied as it is changed, so Return has
        // nothing of the sort to fire.
        if (auto* pPushButton = qobject_cast<QPushButton*>(pControl)) {
            pPushButton->setAutoDefault(false);
            pPushButton->setDefault(false);
        }

        QPalette controlPalette = pControl->palette();
        // The dark theme leaves PlaceholderText at the light default, nearly black on a dark field
        controlPalette.setColor(QPalette::PlaceholderText, placeholderText);
        pControl->setPalette(controlPalette);
    }

    // A QLabel bakes the colour of an anchor into its document when the text is
    // set, from the *application* palette of that moment - so writing
    // QPalette::Link to the label afterwards, as this used to, changed nothing
    // at all and every link here was painted in the palette's own blue. The
    // colour travels on the anchor instead, and each label kept the text it was
    // given so that it can be set again in the new ink.
    mInkedLinkColour = accentText;
    for (QLabel* pLabel : findChildren<QLabel*>()) {
        const QVariant raw = pLabel->property(scmProp_settingsRichText);
        if (raw.isValid()) {
            pLabel->setText(withLinkColour(raw.toString(), accentText));
        }
    }

    // The shortcut conflict warning is written in a state colour, which is
    // mixed against the page - so a warning already on show has to be re-inked
    // rather than left in the lightness the previous page called for
    label_shortcutsConflictWarning->setStyleSheet(shortcutConflictStyleSheet());
    // ...and so is the wash under a certificate the connection complained
    // about, for the same reason
    restyleCertificateWarnings();

    // The accent the rules above give a mark and a button on focus is the
    // keyboard's. Under the stack, the same scope those rules were set on, and
    // after them - which is also after the pages a profile brings with it are
    // built, since this runs again when one arrives.
    keepClickFocusOffControls(mpStackedWidget_categories);
    // ...and the corner the same rules give a dropped-down list, on the same
    // scope and for the same reason: the pages a profile brings with it are
    // built by the time this runs again
    letPopupsTakeTheFieldsCorner(mpStackedWidget_categories);
    styleProfileMapMenu();

    // A different font is a different width for the names, so the sidebar is
    // given the width it wants under the look it has just been handed
    updateSidebarMode();

    // Which side of the line the application was on when the sheet above was
    // mixed, which is what an appearance change compares itself against rather
    // than a before/after reading of the mode - not a theme's own darkPage
    mShellStyledForDarkPage = themeSurface ? themeTokens().darkPage : darkPage;
    ++mShellStyleApplications;
}

void dlgProfilePreferences::styleProfileMapMenu()
{
    if (!mpMenu) {
        return;
    }
    mpMenu->setStyleSheet(menuStyleSheet(themeTokens()));
    letPopupsTakeTheFieldsCorner(mpMenu);
}

// Found by type, as a hand-written list would miss later .ui additions. Qt::UniqueConnection makes it safe to
// rerun once a profile brings its own controls. Push buttons act for themselves, so are skipped.
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
        // toggled() misses a tri-state box moving between partially checked and checked
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
    for (auto* pAbstractSpinBox : findChildren<QAbstractSpinBox*>()) {
        // Return on a QDateTimeEdit leaves its line edit modified and emits nothing from it
        connect(pAbstractSpinBox, &QAbstractSpinBox::editingFinished, this, &dlgProfilePreferences::slot_lineEditFinished, Qt::UniqueConnection);
    }
    for (auto* pLineEdit : findChildren<QLineEdit*>()) {
        if (pLineEdit == mpLineEdit_search) {
            continue;
        }
        connect(pLineEdit, &QLineEdit::editingFinished, this, &dlgProfilePreferences::slot_lineEditFinished, Qt::UniqueConnection);
    }
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

// The dialog stays open for hours while scripts and other dialogs change settings, so it re-reads them on
// reactivation. Re-reading would discard an edit in progress, so any edit skips this run and the
// activation after it is applied does the work.
void dlgProfilePreferences::refreshFromSettings()
{
    // Activation arrives repeatedly (a dialog opened over this one and closed is two), and the population
    // below sets mPopulating
    if (mPopulating || mClosing || !mShellReady) {
        return;
    }
    if (mSearchActive) {
        // Repopulating replaces the text the index was built from, and only clearing the user's query fixes that
        return;
    }
    if (mSnapshot.pendingEdits(mpTimer_apply, mpLineEdit_search)) {
        return;
    }

    // With no query standing nothing is borrowed, so this only drops the index for the next query to rebuild
    invalidateSearch();

    // initWithHost() hides both reconnect notices, right when a profile is read in, but here it writes the
    // controls back after an instant apply - the very change the notice is about, so it must survive.
    const bool reconnectForDataProtocol = !need_reconnect_for_data_protocol->isHidden();
    const bool reconnectForSpecialOption = !need_reconnect_for_specialoption->isHidden();

    // Unlike when the dialog is built, the write-through connections already exist. Written silently, or a
    // moved setting flows straight back out of its control, rounded where the control cannot hold it exactly
    // (room size is a 1-11 scale over a qreal).
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

    // Every population ends with this pair: the controls now match the settings, so later changes are user edits
    connectApplyTriggers();
    mSnapshot.take();
    // Labels may have changed length, so checkboxes may need (un)wrapping. The fit after the controls are
    // written, tab order after the fit, as a wrapped checkbox sits a widget deeper.
    fitColumns();
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
        comboBox_store_passwords_in->setEnabled(true);
        const QStringList notMigrated = mudlet::self()->passwordsNotMigratedToProfiles();
        if (!notMigrated.isEmpty()) {
            //: Shown in the preferences when some passwords could not be saved into their profiles while switching to profile storage; %1 is a list of profile names
            label_password_migration_notification->setText(tr("Could not save the passwords of these profiles into them, so they are still in secure storage: %1").arg(notMigrated.join(qsl(", "))));
            return;
        }
        label_password_migration_notification->setText(tr("Migrated all passwords to profile storage."));
        hidePasswordMigrationLabelTimer->start(10s);
    });

    if (MudletApp::storingPasswordsSecurely()) {
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
    // The protocols themselves are on their own subpage
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
    checkBox_showTimeStamps->setEnabled(false);
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

    groupBox_editorFont->setEnabled(false);
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
    groupBox_MMCPOptions->setEnabled(false);

    // ===== tab_shortcuts =====
    groupBox_main_window_shortcuts->setEnabled(false);

    // ===== tab_accessibility =====
    label_blankLinesBehaviour->setEnabled(false);
    label_caretModeKey->setEnabled(false);
    checkBox_announceIncomingText->setEnabled(false);
    checkBox_advertiseScreenReader->setEnabled(false);
    checkBox_accessibleLabels->setEnabled(false);
    checkBox_enableClosedCaption->setEnabled(false);
    checkBox_enableBlinkText->setEnabled(false);
    checkBox_f3SearchEnabled->setEnabled(false);
    comboBox_blankLinesBehaviour->setEnabled(false);
    comboBox_caretModeKey->setEnabled(false);

    // ===== tab_specialOptions =====
    groupBox_specialOptions->setEnabled(false);
    groupBox_purgeMediaCache->setEnabled(false);
    // ----- groupBox_specialOptions -----
    need_reconnect_for_specialoption->hide();
    checkBox_expectCSpaceIdInColonLessMColorCode->setEnabled(false);

    groupbox_searchEngineSelection->setEnabled(false);
    slot_hidePasswordMigrationLabel();
    // ----- groupBox_debug -----
    checkBox_debugShowAllCodepointProblems->setEnabled(false);
    widget_timerDebugOutputMinimumInterval->setEnabled(false);
    checkBox_lazyCaptureGlobals->setEnabled(false);
    for (const auto& pCheckBox : std::as_const(mExperimentCheckBoxes)) {
        pCheckBox->setEnabled(false);
    }
    label_networkPacketTimeout->setEnabled(false);
    doubleSpinBox_networkPacketTimeout->setEnabled(false);

    updateNoProfileNotice();
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
    checkBox_showTimeStamps->setEnabled(true);
    checkBox_echoLuaErrors->setEnabled(true);
    checkBox_useWideAmbiguousEastAsianGlyphs->setEnabled(true);
    label_controlCharacterHandling->setEnabled(true);
    comboBox_controlCharacterHandling->setEnabled(true);
    doubleclick_ignore_label->setEnabled(true);
    doubleclick_ignore_lineedit->setEnabled(true);
    checkBox_enableOSC8Hyperlinks->setEnabled(true);

    // ===== tab_codeEditor =====
    groupbox_codeEditorThemeSelection->setEnabled(true);

    groupBox_editorFont->setEnabled(true);
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
    // initWithHost() only fills these in when the library loaded
    const bool discordAvailable = Discord::self()->libraryLoaded();
    groupBox_discordPrivacy->setVisible(discordAvailable);
    mpCard_discord->setVisible(discordAvailable);
    groupBox_MMCPOptions->setEnabled(true);

    // ===== tab_shortcuts =====
    groupBox_main_window_shortcuts->setEnabled(true);

    // ===== tab_accessibility =====
    label_blankLinesBehaviour->setEnabled(true);
    label_caretModeKey->setEnabled(true);
    checkBox_announceIncomingText->setEnabled(true);
    checkBox_advertiseScreenReader->setEnabled(true);
    checkBox_accessibleLabels->setEnabled(true);
    checkBox_enableClosedCaption->setEnabled(true);
    checkBox_enableBlinkText->setEnabled(true);
    checkBox_f3SearchEnabled->setEnabled(true);
    comboBox_blankLinesBehaviour->setEnabled(true);
    comboBox_caretModeKey->setEnabled(true);

    // ===== tab_specialOptions =====
    groupBox_specialOptions->setEnabled(true);
    groupBox_purgeMediaCache->setEnabled(true);
    // ----- groupBox_specialOptions -----
    checkBox_expectCSpaceIdInColonLessMColorCode->setEnabled(true);
    groupbox_searchEngineSelection->setEnabled(true);
    // ----- groupBox_debug -----
    widget_timerDebugOutputMinimumInterval->setEnabled(true);
    checkBox_debugShowAllCodepointProblems->setEnabled(true);
    checkBox_lazyCaptureGlobals->setEnabled(true);
    for (const auto& pCheckBox : std::as_const(mExperimentCheckBoxes)) {
        pCheckBox->setEnabled(true);
    }
    label_networkPacketTimeout->setEnabled(true);
    doubleSpinBox_networkPacketTimeout->setEnabled(true);

    updateNoProfileNotice();
}

// Every write is signal-blocked: a control writing the value straight back could undo a language or
// appearance change another dialog just made.
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
    mpCheckBox_colouredIcons->setChecked(pMudlet->colouredIcons());

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
    comboBox_menuBarAltKeyNavigation->setCurrentIndex(static_cast<int>(MudletProxyStyle::altKeyNavigation()));
    {
        const QSignalBlocker blocker(telnetHandlerEnabled);
        telnetHandlerEnabled->setChecked(MudletApp::getQSettings()->value("telnetHandlerEnabled", false).toBool());
    }
    {
        const QSignalBlocker blocker(checkBox_monospacedFontsOnly);
        checkBox_monospacedFontsOnly->setChecked(MudletApp::getQSettings()->value(qsl("showOnlyMonospacedFonts"), false).toBool());
    }
    populateTriggerMatchPoolSettings();
    {
        // The one setting here that lives in its own QSettings group rather than
        // in the mudlet instance, so the only one with no change to announce
        const QSignalBlocker blocker(comboBox_crashReportPolicy);
        const QSettings settings("Mudlet", "CrashReporter");
        const QVariant storedOption = settings.value("autoSendCrashReports", QVariant());
        comboBox_crashReportPolicy->setCurrentIndex(storedOption.isValid() ? storedOption.toInt() - 1 : 2);
    }
#ifdef INCLUDE_MCPSERVER
    {
        const QSignalBlocker enableBlocker(checkBox_enableMCPServer);
        checkBox_enableMCPServer->setChecked(pMudlet->mcpEnabled());
        const QSignalBlocker portBlocker(spinBox_mcpServerPort);
        spinBox_mcpServerPort->setValue(pMudlet->mcpServerPort());
    }
    if (label_mcpConnectResult->text().isEmpty()) {
        // A start refused at launch has no dialog open to report to, so the reason waits
        // here for the first one. Anything already written is a fresher answer than this.
        mMCPServerStateMessage = pMudlet->mcpLastError();
        setMCPConnectResult(mMCPServerStateMessage);
    }
    slot_updateMCPServerEndpoint();
    refreshMCPAppButtons();
#endif
}

void dlgProfilePreferences::initWithHost(Host* pHost)
{
    loadEditorTab();

    showDisplayFont(pHost->getDisplayFont());
    rememberDisplayFontFamilyShown();
    // Accommodate an initial font size being larger than expected - and ensure
    // it is a positive value:
    spinBox_displayFontSize->setMaximum(std::max(pHost->getDisplayFont().pointSize(), 40));
    spinBox_displayFontSize->setValue(std::max(1, pHost->getDisplayFont().pointSize()));
    checkBox_antiAlias->setChecked(pHost->fontsAntiAlias());

    connect(fontComboBox_displayFont, &QFontComboBox::currentFontChanged, this, &dlgProfilePreferences::slot_displayFontChanged, Qt::UniqueConnection);
    connect(comboBox_dictionary, &QComboBox::activated, this, &dlgProfilePreferences::slot_dictionaryPicked, Qt::UniqueConnection);
    // currentFontChanged is not emitted when the family already shown is picked again
    connect(fontComboBox_displayFont, &QComboBox::activated, this, &dlgProfilePreferences::slot_displayFontChanged, Qt::UniqueConnection);
    connect(spinBox_displayFontSize, qOverload<int>(&QSpinBox::valueChanged), this, &dlgProfilePreferences::slot_displayFontSizeChanged, Qt::UniqueConnection);
    connect(checkBox_antiAlias, &QCheckBox::clicked, this, &dlgProfilePreferences::slot_displayFontAliasingChanged, Qt::UniqueConnection);

    connect(pHost, &Host::signal_consoleFontChanged, this, &dlgProfilePreferences::slot_hostDisplayFontChanged, Qt::UniqueConnection);

    // Emptied first so a second run replaces the list
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
    checkBox_spellCheck->setChecked(pHost->getEnableSpellCheck());
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
    const bool currentDictionaryFound = TSpellChecker::systemDictionaryFound(currentDictionary);
    // Also settles MudletApp::usingMudletDictionaries(), read below
    const QString path = TSpellChecker::dictionaryFolderToList(currentDictionary);
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
            // Listed by the affix file, but only loaded when both files can be read
            if (!TSpellChecker::dictionaryUsable(path, entries.at(i))) {
                continue;
            }

            translatedNameToEntriesMap.insert(dictionaryDisplayName(entries.at(i)), i);
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

        // Shown rather than left to fall back to the first entry, which would read as the profile's choice.
        // Unlisted need not mean missing: "med" names are left out, and the file system may ignore case.
        if (currentIndex < 0 && !currentDictionary.isEmpty()) {
            //: %1 is the code of the spell-check dictionary the profile uses, such as "de_DE", when no dictionary file of that name is on this computer
            comboBox_dictionary->addItem(currentDictionaryFound ? dictionaryDisplayName(currentDictionary) : tr("%1 - not available").arg(currentDictionary), currentDictionary);
            currentIndex = comboBox_dictionary->count() - 1;
        }

        if (currentIndex >= 0) {
            comboBox_dictionary->setCurrentIndex(currentIndex);
        }
    } else {
        comboBox_dictionary->setEnabled(false);
        comboBox_dictionary->setToolTip(utils::richText(tr("No Hunspell dictionary files found, spell-checking will not be available.")));
    }
    comboBox_dictionary->blockSignals(false);
    // Installed since it was looked for, the dictionary is read now rather than shown as there while checking nothing
    if (currentDictionaryFound && pHost->spellChecker().systemDictionaryMissing()) {
        pHost->setSpellDic(currentDictionary);
    }

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
    checkBox_accessibleLabels->setChecked(pHost->mAccessibleLabels);
    connect(checkBox_accessibleLabels, &QCheckBox::toggled, this, &dlgProfilePreferences::slot_toggleAccessibleLabels, Qt::UniqueConnection);
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
    mBufferSizeBeforeMax = pHost->getConsoleBufferSize();
    checkBox_useMaxBufferSize->setChecked(pHost->getUseMaxConsoleBufferSize());

    // Set maximum buffer size based on system capabilities and update tooltip
    if (pHost->mainConsoleView()) {
        const int maxBufferSize = pHost->mainConsoleView()->buffer.getMaxBufferSize();
        console_buffer_size_spinBox->setMaximum(maxBufferSize);
        checkBox_useMaxBufferSize->setToolTip(tr("<p>Use the maximum buffer size your system can handle (%1 lines). This will be calculated based on available memory.</p>").arg(maxBufferSize));

        // If using max buffer size, disable the spinbox and set it to max
        if (pHost->getUseMaxConsoleBufferSize()) {
            console_buffer_size_spinBox->setValue(maxBufferSize);
            console_buffer_size_spinBox->setEnabled(false);
        } else {
            // A previous profile may have disabled it, and otherwise only the checkbox's slot re-enables it
            console_buffer_size_spinBox->setEnabled(true);
        }
    }

    show_sent_text_combobox->setCurrentIndex(static_cast<int>(pHost->mCommandEchoMode));
    auto_clear_input_line_checkbox->setChecked(pHost->mAutoClearCommandLineAfterSend);
    disable_password_masking_checkbox->setChecked(pHost->disablePasswordMasking());
    checkBox_highlightHistory->setChecked(pHost->mHighlightHistory);
    command_separator_lineedit->setText(pHost->mCommandSeparator);
    checkBox_USE_IRE_DRIVER_BUGFIX->setChecked(pHost->mUSE_IRE_DRIVER_BUGFIX);
    checkBox_enableTextAnalyzer->setChecked(pHost->mEnableTextAnalyzer);
    checkBox_showTimeStamps->setChecked(pHost->mainConsoleShowsTimeStamps());
    disconnect(mTimeStampsConnection);
    mTimeStampsConnection = connect(&pHost->mainConsoleModel().mNotifier, &TConsoleModelNotifier::timeStampsToggled, this, &dlgProfilePreferences::followMainConsoleTimeStamps);
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
        lineEdit_discordUserName->setText(pHost->getRequiredDiscordUserName());
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
    checkBox_mmcpAutostartServer->setChecked(pHost->getMMCPAutoStartServer());
    checkBox_mmcpAllowPeekReq->setChecked(pHost->getMMCPAllowPeekRequests());
    checkBox_mmcpAutoAcceptCalls->setChecked(pHost->getMMCPAutoAcceptCalls());
    */

    checkBox_mmcpAddChatMessageNewline->setChecked(pHost->getMMCPAddChatMessageNewline());
    checkBox_mmcpPrefixEmotes->setChecked(pHost->getMMCPPrefixEmotes());
    checkBox_mmcpSnoopInMainConsole->setChecked(pHost->getMMCPShowSnoopInMainConsole());
    checkBox_runAllKeyBindings->setChecked(pHost->getKeyUnit()->mRunAllKeyMatches);

    for (QSpinBox* pSpinBox : {topBorderHeight, bottomBorderHeight, leftBorderWidth, rightBorderWidth}) {
        pSpinBox->setRange(-Host::scmMaxBorderSize, Host::scmMaxBorderSize);
    }
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
        // Rebuilt silently: the momentary empty list is not a log format anyone chose
        const QSignalBlocker blocker(comboBox_logFileNameFormat);
        comboBox_logFileNameFormat->clear();
        // This is the previous standard:
        comboBox_logFileNameFormat->addItem(tr("yyyy-MM-dd#HH-mm-ss (e.g., 1970-01-01#00-00-00%1)").arg(logExtension), qsl("yyyy-MM-dd#HH-mm-ss"));
        // The ISO standard for this uses T as the date/time separator
        comboBox_logFileNameFormat->addItem(tr("yyyy-MM-ddTHH-mm-ss (e.g., 1970-01-01T00-00-00%1)").arg(logExtension), qsl("yyyy-MM-ddTHH-mm-ss"));
        comboBox_logFileNameFormat->addItem(tr("yyyy-MM-dd (concatenate daily logs in, e.g. 1970-01-01%1)").arg(logExtension), qsl("yyyy-MM-dd"));
        // No weekly option: QDateTime::toString() cannot format a week number
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
        // A host can be loaded before the shell has been styled or long after,
        // so whichever of the two comes second is what draws this menu
        styleProfileMapMenu();
    }

    // Rebuilt on every refresh, which must not drop the destinations already ticked
    QSet<QString> chosenProfiles;
    for (const auto* pAction : mpMenu->actions()) {
        if (pAction->isChecked()) {
            chosenProfiles.insert(pAction->text());
        }
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
        pItem->setChecked(chosenProfiles.contains(s));
        mpMenu->addAction(pItem);
        //Enable it as we now have at least one profile to copy to
        pushButton_chooseProfiles->setEnabled(true);
    }

    pushButton_chooseProfiles->setMenu(mpMenu);
    slot_chosenProfilesChanged(nullptr);

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

        // The one control here the .ui lacks, so built - once, however many profiles come and go
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
                pSymbolsLayout->addWidget(
                        makeFlowRow(groupBox_mapSymbols, qsl("settingsMapperRow_mapSymbolFontFudge"), {pLabel_mapSymbolFontFudge, mpDoubleSpinBox_mapSymbolFontFudge}), pSymbolsLayout->rowCount(), 0);
            } else {
                qWarning()
                        << "dlgProfilePreferences::initWithHost(...) WARNING - Unable to cast groupBox_mapSymbols layout to expected QGridLayout - someone has messed with the profile_preferences.ui "
                           "file and the contents of the groupBox can not be shown...!";
            }
        }
        {
            // Whether new or left by a previous profile, this is the dialog reading the map, not a user edit
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
        disconnect(mMapStyleConnection);
        mMapStyleConnection = connect(pHost->mpMap.data(), &TMap::signal_mapperSettingChanged, this, [this](const TMap::MapperSetting setting) {
            if (setting == TMap::MapperSetting::Style) {
                followMapStyle();
            }
        });

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
        spinBox_roomSize->setValue(qRound(pHost->mRoomSize * 10));
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
        // The list of encodings comes from the profile's connection
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

    // The "forget saved sign-in" button is gated on either of the stored sign-in's keys existing, not on
    // the sign-in-choice flag: an oauth-only game never sets that flag yet still mints tokens. The
    // metadata alone is not enough to test. Mudlet 5.0.1 keeps the whole sign-in under the
    // metadata key, so forgetting it there leaves this build's token key behind with nothing to offer
    // its removal. A token-less resume hint - {account, provider}, left behind when a rejected token was
    // dropped - is still something the player may want to forget, too. The keychain check is
    // asynchronous, so start hidden and reveal on a hit; the QPointer guards against the dialog closing
    // before the store answers. credentialExists() collapses a read failure (locked/denied/timed-out
    // keychain) to "nothing stored", so the button deliberately stays hidden on any read failure - the
    // only cost is not offering to forget an entry that could not be read. Every store, because the
    // forget clears every store: a token saved to the keychain before the player chose to keep
    // passwords in the profile is still there, and offering nothing would leave it for good.
    pushButton_forgetSavedSignIn->setEnabled(mEnableGMCP->isChecked());
    // Once per profile, not per refresh: reading the keychain can prompt the user on some platforms
    if (mSignInTokenCheckedFor != pHost->getName()) {
        mSignInTokenCheckedFor = pHost->getName();
        pushButton_forgetSavedSignIn->setVisible(false);
        QPointer<dlgProfilePreferences> safeDialog = this;
        const QString profileName = pHost->getName();

        // The saved sign-in's own record lives in the profile rather than the credential store, so
        // asking whether there is one to forget costs nothing - no keychain job, and on macOS no
        // prompt for an entry whose access list does not name this build. Anything still in the
        // store predates that move, and the read below is what finds those.
        //
        // Whether the file is there, rather than whether it reads back: one that cannot be read is
        // still a sign-in the player has, and hiding the control would leave them no way to revoke
        // a token that is live. It is also what the removal keys off, so the two agree.
        const bool signInRecordedInProfile = QFileInfo::exists(GMCPAuthenticator::savedSignInRecordPath(profileName));
        pushButton_forgetSavedSignIn->setVisible(signInRecordedInProfile);

        QPointer<CredentialManager> credentialManager = signInRecordedInProfile ? nullptr : new CredentialManager();
        if (credentialManager) {
            credentialManager->credentialExists(
                    profileName,
                    qsl("reconnect"),
                    [safeDialog, credentialManager, profileName](bool exists) {
                        if (credentialManager) {
                            credentialManager->deleteLater();
                        }
                        if (!safeDialog) {
                            return;
                        }
                        if (exists) {
                            safeDialog->pushButton_forgetSavedSignIn->setVisible(true);
                            return;
                        }
                        QPointer<CredentialManager> tokenChecker = new CredentialManager();
                        tokenChecker->credentialExists(
                                profileName,
                                qsl("reconnect-token"),
                                [safeDialog, tokenChecker](bool tokenExists) {
                                    if (tokenChecker) {
                                        tokenChecker->deleteLater();
                                    }
                                    if (safeDialog && tokenExists) {
                                        safeDialog->pushButton_forgetSavedSignIn->setVisible(true);
                                    }
                                },
                                CredentialManager::StoreScope::EveryStore);
                    },
                    CredentialManager::StoreScope::EveryStore);
        }
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
    checkBox_lazyCaptureGlobals->setChecked(pHost->lazyCaptureGlobals());
    for (const auto& [key, pCheckBox] : std::as_const(mExperimentCheckBoxes).asKeyValueRange()) {
        if (pCheckBox) {
            pCheckBox->setChecked(pHost->experimentEnabled(key));
        }
    }
    checkBox_allowServerToRedefineColors->setChecked(pHost->getMayRedefineColors());
    doubleSpinBox_networkPacketTimeout->setValue(pHost->mTelnet.getPostingTimeout() / 1000.0);
    {
        const QSignalBlocker blocker(comboBox_caretModeKey);
        comboBox_caretModeKey->setCurrentIndex(static_cast<int>(pHost->mCaretShortcut));
    }
    checkBox_largeAreaExitArrows->setChecked(pHost->getLargeAreaExitArrows());
    {
        const QSignalBlocker blocker(comboBox_mapStyle);
        // Kept from the last time round, as this runs again each time the dialog is activated and the pictures
        // take a few milliseconds to paint; clearHostDetails() empties it
        if (comboBox_mapStyle->count() != MapStyle::all().size()) {
            comboBox_mapStyle->clear();
            // Wider than tall, as the combo box has room across for the preview's rooms to show
            const QSize previewSize(72, 48);
            comboBox_mapStyle->setIconSize(previewSize);
            for (const MapStyle& style : MapStyle::all()) {
                comboBox_mapStyle->addItem(style.previewIcon(previewSize), style.displayName(), style.id);
            }
        }
        comboBox_mapStyle->setCurrentIndex(comboBox_mapStyle->findData(pHost->mapStyle()));
    }
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
        // The editors outlive the profile that filled them; rebuilding them for a second profile would leave
        // the first set below the second, still writing through
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
            currentShortcuts[key] = sequenceEdit->keySequence();
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
        // addCommand() refuses a key Mudlet holds, but nothing stops a Mudlet shortcut moving onto a command's key
        const QStringList holders = mudlet::self()->addonCommandsUsingShortcut(sequence, mpHost, true);
        if (labels.size() + holders.size() < 2) {
            continue;
        }
        const QString sequenceText = sequence.toString(QKeySequence::NativeText);
        if (labels.size() == 2 && holders.isEmpty()) {
            //: Inline warning on the shortcuts preferences page when exactly two actions have been given the same shortcut. %1 and %2 are the action names, %3 is the shortcut itself.
            warnings.append(tr("Warning: '%1' and '%2' now share the shortcut %3 - neither will work until one of them is changed.").arg(labels.at(0), labels.at(1), sequenceText));
        } else if (labels.size() == 1 && holders.size() == 1) {
            //: Inline warning on the shortcuts preferences page when one of Mudlet's actions has been given a shortcut an add-on command already holds. %1 is the action name, %2 the shortcut itself, %3 a comma separated list of the commands holding it: each command's name in quotes, or "a command from another profile".
            warnings.append(tr("Warning: '%1' now shares the shortcut %2 with %3 - neither will work until one of them is changed.").arg(labels.at(0), sequenceText, holders.join(qsl(", "))));
        } else {
            QStringList quotedLabels;
            for (const auto& label : labels) {
                quotedLabels.append(qsl("'%1'").arg(label));
            }
            quotedLabels.append(holders);
            //: Inline warning on the shortcuts preferences page when three or more actions or add-on commands have been given the same shortcut. %1 is the list of them, each already quoted (an add-on command from another profile appears as "a command from another profile"), %2 is the shortcut itself.
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

    label_shortcutsConflictWarning->setStyleSheet(shortcutConflictStyleSheet());
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
    disconnect(comboBox_dictionary, &QComboBox::activated, nullptr, nullptr);
    disconnect(fontComboBox_displayFont, &QComboBox::activated, nullptr, nullptr);
    disconnect(spinBox_displayFontSize, qOverload<int>(&QSpinBox::valueChanged), nullptr, nullptr);
    disconnect(checkBox_antiAlias, &QCheckBox::clicked, nullptr, nullptr);
    disconnect(checkBox_editorFontFollowsDisplay, &QCheckBox::toggled, nullptr, nullptr);
    disconnect(fontComboBox_editorFont, &QFontComboBox::currentFontChanged, nullptr, nullptr);
    disconnect(spinBox_editorFontSize, qOverload<int>(&QSpinBox::valueChanged), nullptr, nullptr);

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

    // Not the protocol checkboxes: their wiring relates the controls to each other, not to a Host

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
    disconnect(mTimeStampsConnection);
    checkBox_showTimeStamps->setChecked(false);
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
    checkBox_editorFontFollowsDisplay->setChecked(true);
    fontComboBox_editorFont->clear();
    spinBox_editorFontSize->setValue(14);
    label_editorFontInvalid->hide();
    label_editorFontVariableWidth->hide();
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
    disconnect(mMapStyleConnection);
    comboBox_mapStyle->clear();
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
    checkBox_lazyCaptureGlobals->setChecked(true);
    for (const auto& pCheckBox : std::as_const(mExperimentCheckBoxes)) {
        if (pCheckBox) {
            pCheckBox->setChecked(false);
        }
    }
    checkBox_announceIncomingText->setChecked(false);
    checkBox_advertiseScreenReader->setChecked(false);
    checkBox_accessibleLabels->setChecked(false);
    checkBox_enableClosedCaption->setChecked(false);
    checkBox_enableBlinkText->setChecked(false);
    comboBox_blankLinesBehaviour->setCurrentIndex(0);

    groupBox_ssl_certificate->hide();
    frame_notificationArea->hide();
    checkBox_askTlsAvailable->setChecked(false);
    pushButton_forgetSavedSignIn->setEnabled(false);
    pushButton_forgetSavedSignIn->setVisible(false);
    // So the next profile asks the keychain again
    mSignInTokenCheckedFor.clear();
    groupBox_proxy->setDisabled(true);
    // Hides the hero, as there is no connection to report on
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

    mudlet::loadEdbeeTheme(pHost->getEditorTheme(), pHost->getEditorThemeFile());
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
    config->setFont(pHost->getEditorFont());
    config->setAutocompleteAutoShow(pHost->mEditorAutoComplete);
    config->setRenderBidiContolCharacters(pHost->getEditorShowBidi());
    config->setAutocompleteMinimalCharacters(3);
    config->endChanges();
    edbeePreviewWidget->textDocument()->setLanguageGrammar(edbee::Edbee::instance()->grammarManager()->detectGrammarWithFilename(qsl("Buck.lua")));
    // disable shadows as their purpose (notify there is more text) is performed by scrollbars already
    edbeePreviewWidget->textScrollArea()->enableShadowWidget(false);

    populateThemesList();
    // Walking every scripted item stalls a large profile, too long to repeat on every refresh; a profile
    // change empties the list in clearHostDetails()
    if (!script_preview_combobox->count()) {
        populateScriptsList();
    }

    // pre-select the current theme. What the profile recorded rather than what
    // the editor is drawn with: those differ when the recorded theme is not on
    // this machine, and the control is where the profile's own choice is shown
    // and written back from. Showing the fallback here would offer to save it
    // over the choice.
    QString storedThemeName;
    QString storedThemeFile;
    pHost->storedEditorTheme(storedThemeName, storedThemeFile);
    //: Placeholder of the editor colour theme picker on the Editor page of the settings
    code_editor_theme_selection_combobox->lineEdit()->setPlaceholderText(tr("Select theme"));
    auto themeIndex = code_editor_theme_selection_combobox->findText(storedThemeName);
    code_editor_theme_selection_combobox->setCurrentIndex(themeIndex);
    slot_themeSelected(themeIndex);

    code_editor_theme_selection_combobox->setInsertPolicy(QComboBox::NoInsert);
    code_editor_theme_selection_combobox->setMaxVisibleItems(20);

    // pre-select the last shown script to preview
    //: Placeholder of the picker for the script shown as the theme preview on the Editor page of the settings
    script_preview_combobox->lineEdit()->setPlaceholderText(tr("Select script to preview"));
    auto scriptIndex = script_preview_combobox->findData(QVariant::fromValue(QPair<QString, int>(pHost->mThemePreviewType, pHost->mThemePreviewItemID)));
    script_preview_combobox->setCurrentIndex(scriptIndex == -1 ? 1 : scriptIndex);
    slot_scriptSelected(scriptIndex == -1 ? 1 : scriptIndex);

    script_preview_combobox->setInsertPolicy(QComboBox::NoInsert);
    script_preview_combobox->setMaxVisibleItems(20);
    script_preview_combobox->setDuplicatesEnabled(true);
    // The list is no wider than the box, which is held to a fixed length so the profile's longest item name
    // cannot widen the page; eliding in the middle keeps the item's kind at the end of its name in view
    script_preview_combobox->view()->setTextElideMode(Qt::ElideMiddle);

    // Once fetched, the label reports how that went for as long as the dialog is open, and this runs again
    // every time the dialog is reactivated
    if (!mEditorThemesChecked) {
        theme_download_label->hide();
    }

    checkBox_autocompleteLuaCode->setChecked(pHost->mEditorAutoComplete);
    checkBox_showBidi->setChecked(pHost->getEditorShowBidi());
    checkBox_showIdNumbers->setChecked(pHost->showIdsInEditor());
    checkBox_folderView->setChecked(pHost->folderViewInEditor());

    checkBox_editorFontFollowsDisplay->setChecked(pHost->editorFontFollowsDisplay());
    showEditorFont(pHost->getEditorFont());
    connect(checkBox_editorFontFollowsDisplay, &QCheckBox::toggled, this, &dlgProfilePreferences::reflectEditorFontControls, Qt::UniqueConnection);
    connect(fontComboBox_editorFont, &QFontComboBox::currentFontChanged, this, &dlgProfilePreferences::reflectEditorFontControls, Qt::UniqueConnection);
    connect(spinBox_editorFontSize, qOverload<int>(&QSpinBox::valueChanged), this, &dlgProfilePreferences::reflectEditorFontControls, Qt::UniqueConnection);
    connect(pHost, &Host::signal_editorFontChanged, this, &dlgProfilePreferences::slot_hostEditorFontChanged, Qt::UniqueConnection);

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
        setButtonColor(pushButton_lowerLevelColor, pHost->mLowerLevelColor, true);
        setButtonColor(pushButton_upperLevelColor, pHost->mUpperLevelColor, true);
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

// The twelve legacy tab objectNames callers may still pass map onto categories; otherwise a target is a
// category key or "category/cardObjectName"
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

    // A subpage card is only reachable through its subpage, so go there rather than to the category page
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
    // theme-fixed: the console's own default palette down to the blank line
    // below - the colours the game's text is drawn in, rather than the window's
    // chrome
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
    if (pHost->mainConsoleView()) {
        pHost->mainConsoleView()->resetConsoleBackgroundImage();
        pHost->mainConsoleView()->changeColors();
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
    // theme-fixed: the map's own default palette down to the blank line below -
    // what a room, a level and a grid line are drawn in is the picture the map
    // is, not chrome of the window holding it, so it stays where it is put
    // whichever appearance the application is in.
    pHost->mFgColor_2 = QColorConstants::LightGray;
    pHost->mBgColor_2 = QColorConstants::Black;
    pHost->mLowerLevelColor = Host::scmDefaultLowerLevelColor;
    pHost->mUpperLevelColor = Host::scmDefaultUpperLevelColor;
    pHost->mRoomBorderColor = Host::scmDefaultRoomBorderColor;
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
    pHost->mMapInfoBg = Host::scmDefaultMapInfoBg;
    pHost->mMapGridColor = QColor(211, 211, 211, 64);

    // This aplies the above colors to the buttons on display:
    setColors2();

    if (pHost->mpMap) {
        pHost->mpMap->restore16ColorSet();
        pHost->mpMap->announceMapperSettingChanged(TMap::MapperSetting::BackgroundColor);
        pHost->mpMap->updateArea(-1);
    }
}

void dlgProfilePreferences::setButtonAndProfileColor(QPushButton* button, QColor& presentColor, bool allowAlpha)
{
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }

    // The dialog runs an event loop of its own, in which the button or the whole window can go
    const QPointer<dlgProfilePreferences> pSelf(this);
    const QPointer<QPushButton> pClicked(button);
    //: Generic pick color dialog title
    auto color = QColorDialog::getColor(presentColor, this, tr("Pick color"), allowAlpha ? QColorDialog::ShowAlphaChannel : QColorDialog::ColorDialogOptions());
    if (!pSelf) {
        return;
    }
    returnFromColourDialog(pClicked);
    if (color.isValid()) {
        presentColor = color;

        QPointer<TMainConsole> console = pHost->mainConsoleView();
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
            || button == pushButton_mapInfoBg || button == pushButton_roomBorderColor || button == pushButton_roomCollisionBorderColor || button == pushButton_mapGridColor) {
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
        if (pHost->mpMap) {
            pHost->mpMap->announceMapperSettingChanged(TMap::MapperSetting::BackgroundColor);
        }
    }
}

void dlgProfilePreferences::slot_setLowerLevelColor()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_lowerLevelColor, pHost->mLowerLevelColor, true);
    }
}

void dlgProfilePreferences::slot_setUpperLevelColor()
{
    Host* pHost = mpHost;
    if (pHost) {
        setButtonAndProfileColor(pushButton_upperLevelColor, pHost->mUpperLevelColor, true);
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
        // Disable for the duration, not only on success. The removal is asynchronous and can take tens
        // of seconds behind a slow or prompting keychain, during which the button stayed live and a
        // second click was easy. That second request supersedes the first, and a superseded forget
        // reports failure - so the player got a "could not remove" warning for a forget that was about
        // to succeed, immediately followed by the success. Re-enabled below on any outcome that leaves
        // something still to forget.
        pushButton_forgetSavedSignIn->setEnabled(false);
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
                    // Something may still be stored, so let the player try again.
                    safeDialog->pushButton_forgetSavedSignIn->setEnabled(true);
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
    if (!pHost->mpMap->mapper()) {
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
    // An autosave still being written would be listed as its temporary file wherever that file has a name
    pHost->waitForMapAutosave();

    // Map files change while the dialog is open, so rebuild, resetting the enabled state to an empty list's.
    // The rebuild follows every change, so it keeps an older map that was picked to load; the newest entry
    // is not kept, so a map saved since takes its place at the top.
    const QVariant pickedMapFile = comboBox_mapHistory->currentIndex() > 0 ? comboBox_mapHistory->currentData() : QVariant();
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
    if (const int pickedIndex = comboBox_mapHistory->findData(pickedMapFile); pickedMapFile.isValid() && pickedIndex >= 0) {
        const QSignalBlocker blocker(comboBox_mapHistory);
        comboBox_mapHistory->setCurrentIndex(pickedIndex);
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

    // Ensure the setting is already made as the Host::loadMapFile(...) uses
    // the set value:
    const bool showAuditErrors = TMap::smShowMapAuditErrors;
    mudlet::self()->setShowMapAuditErrors(checkBox_reportMapIssuesOnScreen->isChecked());

    bool success = false;
    const bool isJson = fileName.endsWith(qsl(".json"), Qt::CaseInsensitive);
    label_mapFileActionResult->setText(tr("Loading map - please wait..."));
    qApp->processEvents(); // Needed to make the above message show up when loading big maps
    if (fileName.endsWith(qsl(".xml"), Qt::CaseInsensitive)) {
        qApp->processEvents(); // Needed to make the above message show up when loading big maps
        success = pHost->importMapFile(fileName);

    } else {
        if (isJson) {
            auto [localSuccess, errorMessage] = pHost->mpMap->importJsonMapFile(fileName);
            success = localSuccess;
            if (!localSuccess) {
                pHost->postMessage(tr("[ ERROR ] - Unable to load JSON map file: %1\n"
                                      "reason: %2.")
                                           .arg(fileName, errorMessage));
            }

        } else {
            success = pHost->loadMapFile(fileName);
        }
    }

    if (success) {
        // TMap::audit() is what paints up the "[  OK  ]" message for the map load.
        if (!isJson) {
            pHost->mpMap->audit();
        }
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
            success = pHost->saveMapFile(fileName, comboBox_mapFileSaveFormatVersion->currentData().toInt());
        } else {
            auto [localSuccess, errorMessage] = pHost->mpMap->writeJsonMapFile(fileName);
            success = localSuccess;
            if (!localSuccess) {
                //: %1 is the file the map was being exported to, %2 why it could not be
                pHost->postMessage(tr("[ ERROR ] - Unable to save JSON map file: %1\n"
                                      "reason: %2.")
                                           .arg(fileName, errorMessage));
            }
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
            const QString toProfileDirPathString = MudletApp::getMudletPath(enums::profileMapsPath, toProfileName);
            // A profile deleted while this dialog was open is left for its copy to fail, rather than brought back holding only a map
            if (toProfileDir.exists(MudletApp::getMudletPath(enums::profileHomePath, toProfileName)) && !toProfileDir.exists(toProfileDirPathString)) {
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
    for (auto pOtherHost : *HostManager::self()) {
        if (pOtherHost->mainConsoleView() && (pOtherHost != pHost)) {
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

    // On Windows and macOS a copy keeps the time of the map saved here, so that save has to come
    // after each destination's autosave
    const QStringList toProfileNames = toProfilesRoomIdMap.keys();
    for (const QString& otherHostName : toProfileNames) {
        if (Host* pOtherHost = HostManager::self()->getHost(otherHostName)) {
            pOtherHost->waitForMapAutosave();
        }
    }
    if (!pHost->saveMapFile(QString())) {
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

    QList<QString> copiedToProfiles;
    QStringList failedProfiles;
    // Make the copies into the destination profiles (for all to profiles whether
    // in use or not):
    itOtherProfile.toFront();
    while (itOtherProfile.hasNext()) {
        itOtherProfile.next();
        const QString otherHostName = itOtherProfile.key();
        const QString destination = MudletApp::getMudletPath(enums::profileMapPathFileName, otherHostName, thisProfileLatestMapPathFileName);
        // A map folder shared through a link or junction already holds the save just made, which a remove would delete
        std::error_code notComparable;
        if (std::filesystem::equivalent(QFileInfo(destination).filesystemFilePath(), QFileInfo(thisProfileLatestMapFile).filesystemFilePath(), notComparable)) {
            copiedToProfiles.append(otherHostName);
            continue;
        }
        // A copy made earlier in the same second may have left a file of this name, which QFile::copy() will not replace;
        // it is set aside rather than removed, so a copy that fails can put it back
        const QString setAside = qsl("%1.replaced").arg(destination);
        bool replacing = false;
        if (QFileInfo(destination).isFile()) {
            QFile::remove(setAside);
            replacing = QFile::rename(destination, setAside);
        }
        // Copy over into the profiles map folder, so it is loaded first when map is open - this covers the offline case
        label_mapFileActionResult->setText(tr("Copying over map to %1 - please wait...").arg(otherHostName));
        qApp->processEvents(); // Copied from "Loading map - please wait..." case
                               // Just in case is needed to make the above message
                               // show up when saving big maps

        if (Host* pOtherHost = HostManager::self()->getHost(otherHostName)) {
            pOtherHost->waitForMapAutosave();
        }
        if (!thisProfileLatestMapFile.copy(MudletApp::getMudletPath(enums::profileMapPathFileName, otherHostName, thisProfileLatestMapPathFileName))) {
            //: %1 is the profile the map could not be copied to, %2 the system's reason, e.g. "Destination file exists"
            pHost->postMessage(tr("[ ERROR ] - Unable to copy the map to profile \"%1\".\n"
                                  "reason: %2.")
                                       .arg(otherHostName, thisProfileLatestMapFile.errorString()));
            failedProfiles.append(otherHostName);
            if (replacing) {
                QFile::rename(setAside, destination);
            }
            continue; // Try again with next profile
        }
        copiedToProfiles.append(otherHostName);
        if (replacing) {
            QFile::remove(setAside);
        }
        label_mapFileActionResult->setText(tr("Map copied successfully to other profile %1.").arg(otherHostName));
        qApp->processEvents(); // Copied from "Loading map - please wait..." case
                               // Just in case is needed to make the above message
                               // show up when saving big maps
    }

    // Finally, signal the other profiles to reload their maps:
    mudlet::self()->requestProfilesToReloadMaps(copiedToProfiles, thisProfileLatestMapPathFileName);
    if (failedProfiles.isEmpty()) {
        label_mapFileActionResult->setText(tr("Map copied, now signalling other profiles to reload it."));
    } else {
        //: %1 is a comma-separated list of the profiles the map could not be copied to
        label_mapFileActionResult->setText(tr("Could not copy the map to %1 - see the main console for why.").arg(failedProfiles.join(qsl(", "))));
    }
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

// Writes only settings whose controls the user changed: the dialog stays open while scripts run, and writing
// back a control showing a stale value would revert what a script just set.
void dlgProfilePreferences::applyAll()
{
    if (mPopulating) {
        return;
    }

    mudlet* pMudlet = mudlet::self();
    Host* pHost = mpHost;
    if (pHost) {
        QPointer<TMainConsole> console = pHost->mainConsoleView();
        if (mSnapshot.dirty(comboBox_dictionary) && comboBox_dictionary->isEnabled() && comboBox_dictionary->currentIndex() >= 0) {
            pHost->setSpellDic(comboBox_dictionary->currentData().toString());
        }

        if (mSnapshot.dirty(checkBox_spellCheck)) {
            pHost->setEnableSpellCheck(checkBox_spellCheck->isChecked());
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
            // The profile keeps the size the user chose while the maximum is in
            // charge, so unticking the maximum has a size to go back to
            int chosenBufferSize = pHost->getConsoleBufferSize();
            if (useMaxBuffer) {
                chosenBufferSize = mBufferSizeBeforeMax;
            } else if (mSnapshot.dirty(console_buffer_size_spinBox)) {
                chosenBufferSize = console_buffer_size_spinBox->value();
            }

            if (pHost->getConsoleBufferSize() != chosenBufferSize || pHost->getUseMaxConsoleBufferSize() != useMaxBuffer) {
                pHost->setConsoleBufferSize(chosenBufferSize);
                pHost->setUseMaxConsoleBufferSize(useMaxBuffer);

                if (pHost->mainConsoleView()) {
                    const int newBufferSize = useMaxBuffer ? pHost->mainConsoleView()->buffer.getMaxBufferSize() : chosenBufferSize;
                    // Calculate batch delete size as 5% of buffer size (minimum 100)
                    const int newBatchDeleteSize = std::max(100, newBufferSize / 5);
                    pHost->mainConsoleView()->buffer.setBufferSize(newBufferSize, newBatchDeleteSize);
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
            pHost->setDisablePasswordMasking(disable_password_masking_checkbox->isChecked());
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
        if (mSnapshot.dirty(checkBox_showTimeStamps)) {
            pHost->setWindowTimeStamps(qsl("main"), checkBox_showTimeStamps->isChecked());
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
            pMudlet->setDrawUpperLowerLevels(checkBox_drawUpperLowerLevels->isChecked());
        }
        if (pHost->mpMap) {
            if (mSnapshot.dirty(checkBox_showDefaultArea)) {
                // Need to save the original value in case we change it in the line
                // following this one:
                const bool defaultAreaWasNotShown = pHost->mpMap->getDefaultAreaShown();
                pHost->mpMap->setDefaultAreaShown(checkBox_showDefaultArea->isChecked());
                if (pHost->mpMap->mapper() && !defaultAreaWasNotShown && checkBox_showDefaultArea->isChecked() && pHost->mpMap->mapper()->mp2dMap->mAreaID == -1) {
                    // Corner case fixup, user has asked for the default area
                    // to be shown and it wasn't - so it can now be:
                    pHost->mpMap->mapper()->comboBox_showArea->setCurrentText(pHost->mpMap->getDefaultAreaName());
                }
            }
            if (mSnapshot.dirty(mMapperUseAntiAlias) && pHost->mpMap->mapper()) {
                pHost->mpMap->mapper()->mp2dMap->mMapperUseAntiAlias = mMapperUseAntiAlias->isChecked();
            }

            // Only when the spin-box is what holds the newer value. It carries
            // no more precision than it displays, so writing it back whenever
            // Save is clicked would round off a factor a script had set:
            if (mpDoubleSpinBox_mapSymbolFontFudge && mSnapshot.dirty(mpDoubleSpinBox_mapSymbolFontFudge)
                && !spinBoxShows(mpDoubleSpinBox_mapSymbolFontFudge, pHost->mpMap->getSymbolFontFudgeFactor())) {
                pHost->mpMap->setSymbolFontFudgeFactor(mpDoubleSpinBox_mapSymbolFontFudge->value());
            }

            if (pHost->mpMap->mapper()) {
                pHost->mpMap->mapper()->mp2dMap->repaint(); // Forceably redraw it as we ARE currently showing default area
                pHost->mpMap->mapper()->update();
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
        // Set by the folder button, which carries no value of its own
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
            pHost->setFontsAntiAlias(checkBox_antiAlias->isChecked());
        }
        if (mSnapshot.dirty(mAlertOnNewData)) {
            pHost->mAlertOnNewData = mAlertOnNewData->isChecked();
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
            // The connection dialog seeds its Secure checkbox from this file, and hands that back
            // to the profile on connect
            if (const auto [written, error] = pHost->writeProfileData(qsl("ssl_tsl"), QString::number(groupBox_ssl->isChecked() ? Qt::Checked : Qt::Unchecked)); !written) {
                //: %1 is the reason the file could not be written, from the operating system
                pHost->postMessage(tr("[ WARN ]  - Could not save the secure connection setting for the Connect dialog: %1. "
                                      "Connecting from that dialog may change the setting back.")
                                           .arg(error));
            }
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
            if (MudletApp::darkMode()) {
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
        if (mSnapshot.dirty(checkBox_folderView)) {
            pHost->setFolderViewInEditor(checkBox_folderView->isChecked());
        }
        // Ahead of the switch: a family or size picked straight after unticking it is the font the editor
        // leaves the display font for, and switching first would put the Host's own back into the controls
        if (!checkBox_editorFontFollowsDisplay->isChecked() && mSnapshot.anyDirty({fontComboBox_editorFont, spinBox_editorFontSize})) {
            const QFont base = pHost->editorFontBase();
            const QString family = mSnapshot.dirty(fontComboBox_editorFont) ? fontComboBox_editorFont->currentFont().family() : base.family();
            const int size = mSnapshot.dirty(spinBox_editorFontSize) ? spinBox_editorFontSize->value() : base.pointSize();
            const QFont font = pHost->createFontWithSettings(family, size);
            if (QFontMetrics(font).averageCharWidth() > 0) {
                pHost->setEditorFont(font);
            }
        }
        if (mSnapshot.dirty(checkBox_editorFontFollowsDisplay)) {
            pHost->setEditorFontFollowsDisplay(checkBox_editorFontFollowsDisplay->isChecked());
        }
        // Re-theming an open script editor fully reconfigures edbee, so only when one of these changed
        auto* pDialogs = HostDialogs::find(pHost);
        if (auto* pEditor = pDialogs ? pDialogs->mpEditorDialog.data() : nullptr;
            pEditor && mSnapshot.anyDirty({code_editor_theme_selection_combobox, checkBox_showSpacesAndTabs, checkBox_showLineFeedsAndParagraphs, checkBox_autocompleteLuaCode, checkBox_showBidi})) {
            // From the Host, which the write above updated, not a box a script may have moved on from - and through
            // the getter, which falls back to the light choice for a profile that never picked a dark theme. The dark
            // one read directly handed edbee an empty name, which is edbee's own default theme rather than the profile's.
            pEditor->setThemeAndOtherSettings(pHost->getEditorTheme());
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
            // Six controls, one flags word: only bits whose control was edited change, keeping what a script set
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
            if (pHost->getRequiredDiscordUserName() != newDiscordUserName) {
                pHost->setRequiredDiscordUserName(newDiscordUserName);
                Discord::self()->UpdatePresence();
            }
        }

        // Save chat options so they are written to XML upon export
        if (mSnapshot.dirty(lineEdit_mmcpChatName)) {
            pHost->setMMCPChatName(lineEdit_mmcpChatName->text().trimmed());
        }
        if (mSnapshot.dirty(lineEdit_mmcpChatMessagePrefix)) {
            pHost->setMMCPChatPrefix(lineEdit_mmcpChatMessagePrefix->text().trimmed());
        }
        if (mSnapshot.dirty(lineEdit_mmcpPort)) {
            bool ok;
            const quint16 port = lineEdit_mmcpPort->text().toUShort(&ok);
            pHost->setMMCPPort(ok ? port : csDefaultMMCPHostPort);
        }

        // These MMCP options' check boxes are commented out of profile_preferences.ui, so there is nothing to read
        /* restore these along with the check boxes, and give Host the setters:
        pHost->setMMCPAutoStartServer(checkBox_mmcpAutostartServer->isChecked());
        pHost->setMMCPAutoAcceptCalls(checkBox_mmcpAutoAcceptCalls->isChecked());
        pHost->setMMCPAllowPeekRequests(checkBox_mmcpAllowPeekReq->isChecked());
        */

        if (mSnapshot.dirty(checkBox_mmcpPrefixEmotes)) {
            pHost->setMMCPPrefixEmotes(checkBox_mmcpPrefixEmotes->isChecked());
        }
        if (mSnapshot.dirty(checkBox_mmcpAddChatMessageNewline)) {
            pHost->setMMCPAddChatMessageNewline(checkBox_mmcpAddChatMessageNewline->isChecked());
        }
        if (mSnapshot.dirty(checkBox_mmcpSnoopInMainConsole)) {
            pHost->setMMCPShowSnoopInMainConsole(checkBox_mmcpSnoopInMainConsole->isChecked());
        }
        if (mSnapshot.dirty(checkBox_enableOSC8Hyperlinks)) {
            pHost->mEnableOSC8Hyperlinks = checkBox_enableOSC8Hyperlinks->isChecked();
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
        if (mSnapshot.dirty(checkBox_lazyCaptureGlobals)) {
            pHost->setLazyCaptureGlobals(checkBox_lazyCaptureGlobals->isChecked());
        }
        for (const auto& [key, pCheckBox] : std::as_const(mExperimentCheckBoxes).asKeyValueRange()) {
            if (pCheckBox && mSnapshot.dirty(pCheckBox)) {
                pHost->setExperimentEnabled(key, pCheckBox->isChecked());
            }
        }
        if (mSnapshot.dirty(comboBox_caretModeKey)) {
            pHost->mCaretShortcut = static_cast<Host::CaretShortcut>(comboBox_caretModeKey->currentIndex());
        }
        if (mSnapshot.dirty(comboBox_mapStyle) && comboBox_mapStyle->currentIndex() >= 0) {
            pHost->setMapStyle(comboBox_mapStyle->currentData().toString());
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
                // Per key, like the value snapshot: the others show what the dialog was populated with
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

        // Last, because these setters run script handlers synchronously, which
        // may do anything to the Host this block is still writing to
        if (mSnapshot.dirty(checkBox_announceIncomingText)) {
            pHost->setAnnounceIncomingText(checkBox_announceIncomingText->isChecked());
        }
        if (mSnapshot.dirty(checkBox_advertiseScreenReader)) {
            pHost->setAdvertiseScreenReader(checkBox_advertiseScreenReader->isChecked());
        }
        if (mSnapshot.dirty(checkBox_accessibleLabels)) {
            pHost->setAccessibleLabels(checkBox_accessibleLabels->isChecked());
        }
        if (mSnapshot.dirty(checkBox_enableClosedCaption)) {
            pHost->setEnableClosedCaption(checkBox_enableClosedCaption->isChecked());
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

#ifdef INCLUDE_MCPSERVER
    if (mSnapshot.anyDirty({checkBox_enableMCPServer, spinBox_mcpServerPort})) {
        QString mcpError;
        const auto requestedMcpPort = static_cast<quint16>(spinBox_mcpServerPort->value());
        if (pMudlet->setMCPEnabled(checkBox_enableMCPServer->isChecked(), requestedMcpPort, mcpError)) {
            if (!mMCPServerStateMessage.isEmpty() && label_mcpConnectResult->text() == mMCPServerStateMessage) {
                // The buttons' own outcomes share this line and are not ours to take back
                setMCPConnectResult(QString());
            }
            mMCPServerStateMessage.clear();
        } else {
            mMCPServerStateMessage = mcpError;
            setMCPConnectResult(mcpError);
        }
        // refreshFromSettings() is the usual way back to the widgets, but it stands down
        // while the search is open or the port box is still being typed into - both of
        // which are exactly when this runs.
        slot_updateMCPServerEndpoint();
    }
#endif

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
    if (mSnapshot.dirty(mpCheckBox_colouredIcons)) {
        pMudlet->setColouredIcons(mpCheckBox_colouredIcons->isChecked());
    }
    if (mSnapshot.dirty(comboBox_appearance)) {
        pMudlet->setAppearance(static_cast<enums::Appearance>(comboBox_appearance->currentIndex()));
    }
    if (mSnapshot.dirty(comboBox_menuBarAltKeyNavigation)) {
        MudletProxyStyle::setAltKeyNavigation(static_cast<enums::MenuBarAltKeyNavigation>(comboBox_menuBarAltKeyNavigation->currentIndex()));
    }
    if (mSnapshot.dirty(telnetHandlerEnabled)) {
        QSettings* settings = MudletApp::getQSettings();
        if (settings->value("telnetHandlerEnabled", false).toBool() != telnetHandlerEnabled->isChecked()) {
            settings->setValue("telnetHandlerEnabled", telnetHandlerEnabled->isChecked());
        }
    }
    if (mSnapshot.dirty(checkBox_monospacedFontsOnly)) {
        QSettings* settings = MudletApp::getQSettings();
        if (settings->value(qsl("showOnlyMonospacedFonts"), false).toBool() != checkBox_monospacedFontsOnly->isChecked()) {
            settings->setValue(qsl("showOnlyMonospacedFonts"), checkBox_monospacedFontsOnly->isChecked());
        }
    }
    applyTriggerMatchPoolSettings();

    Discord::self()->UpdatePresence();

    emit signal_preferencesSaved();

    mSnapshot.take();

    // With nothing outstanding, re-read: a write can be refused (a font with no metrics, a value the Host
    // clamps) or answered by a script, so what lands may differ from what was sent
    refreshFromSettings();
}

void dlgProfilePreferences::slot_scheduleApply()
{
    // A control wired up inside buildShell() arrives before the timer is made
    if (mPopulating || !mpTimer_apply) {
        return;
    }
    mpTimer_apply->start();
}

void dlgProfilePreferences::slot_lineEditFinished()
{
    // Clearing the modified flag marks this edit finished - see beingTypedInto()
    QObject* pEditor = sender();
    if (auto* pSpinBox = qobject_cast<QAbstractSpinBox*>(pEditor)) {
        pEditor = pSpinBox->findChild<QLineEdit*>(QString(), Qt::FindDirectChildrenOnly);
    }
    if (auto* pLineEdit = qobject_cast<QLineEdit*>(pEditor); pLineEdit) {
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
        // No Host yet, so let a later visit try again
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

    // Otherwise a test visiting the Editor category may make a live fetch that fails slowly rather than red
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

    auto failThemeUpdate = [this](const QString& reason, const QString& loggedReason = QString()) {
        qWarning() << "dlgProfilePreferences: could not update the editor themes:" << (loggedReason.isEmpty() ? reason : loggedReason);
        //: Shown under the editor theme list in the preferences; %1 is why the update failed: a network or file system error from the system, or "the downloaded themes could not be unpacked"
        theme_download_label->setText(tr("Could not update themes: %1").arg(reason));
        // A profile closed mid-fetch has greyed this page out and put the label away
        if (mpHost) {
            theme_download_label->show();
        }
    };

    connect(getReply, &QNetworkReply::errorOccurred, this, [=, this](QNetworkReply::NetworkError) {
        failThemeUpdate(getReply->errorString());
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
                        const QUrl fetchedUrl = reply->url();
                        reply->deleteLater();

                        tempThemesArchive = new QTemporaryFile(this);
                        if (!tempThemesArchive->open()) {
                            failThemeUpdate(tempThemesArchive->errorString());
                            tempThemesArchive->deleteLater();
                            return;
                        }
                        // Qt sets no error when only part of a write reaches the disk - in write() for a large one,
                        // in flush() for a small one it buffered - so the rest is tried again to get the reason.
                        // close() cannot report a failure at all.
                        qint64 written = 0;
                        while (written < downloadedArchive.size()) {
                            const qint64 count = tempThemesArchive->write(downloadedArchive.constData() + written, downloadedArchive.size() - written);
                            if (count <= 0) {
                                break;
                            }
                            written += count;
                        }
                        if (written != downloadedArchive.size() || !(tempThemesArchive->flush() || tempThemesArchive->flush())) {
                            failThemeUpdate(tempThemesArchive->errorString());
                            tempThemesArchive->deleteLater();
                            return;
                        }
                        tempThemesArchive->close();

                        const QTemporaryDir temporaryDir;
                        if (!temporaryDir.isValid()) {
                            failThemeUpdate(temporaryDir.errorString());
                            tempThemesArchive->deleteLater();
                            return;
                        }

                        // perform unzipping in a worker thread so as not to freeze the UI
                        auto future = QtConcurrent::run(utils::unzip, tempThemesArchive->fileName(), MudletApp::getMudletPath(enums::mainDataItemPath, qsl("edbee/")), temporaryDir.path());
                        auto watcher = new QFutureWatcher<bool>(this);
                        connect(watcher, &QFutureWatcher<bool>::finished, this, [=, this]() {
                            if (future.result()) {
                                populateThemesList();
                                theme_download_label->hide();

                                emit signal_themeUpdateCompleted();
                            } else {
                                // PreferCache would otherwise hand the same unusable download to every later dialog
                                diskCache->remove(fetchedUrl);
                                //: Shown after "Could not update themes: " when the downloaded editor themes could not be unpacked
                                const char* const unpackFailure = QT_TR_NOOP("the downloaded themes could not be unpacked");
                                failThemeUpdate(tr(unpackFailure), QString::fromLatin1(unpackFailure));
                            }

                            tempThemesArchive->deleteLater();
                            watcher->deleteLater();
                        });
                        watcher->setFuture(future);
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
    // The two Mudlet carries itself rather than downloading. Naming them as a
    // light/dark pair is all it takes for findThemeCounterpart() to carry a
    // profile from one to the other on an appearance change - and for the user
    // to say no to that by picking anything else, which is then remembered as
    // that appearance's choice.
    sortedThemes << std::make_pair(QString(MudletApp::scmEditorThemeNameLight), QString(MudletApp::scmEditorThemeFileLight));
    sortedThemes << std::make_pair(QString(MudletApp::scmEditorThemeNameDark), QString(MudletApp::scmEditorThemeFileDark));

    std::sort(sortedThemes.begin(), sortedThemes.end(), [](const auto& a, const auto& b) {
        return QString::localeAwareCompare(a.first, b.first) < 0;
    });

    // Every instant apply reads the settings back through here, and refilling an unchanged list throws an open one
    // back to wherever the chosen theme then lands
    bool unchanged = code_editor_theme_selection_combobox->count() == sortedThemes.size();
    for (qsizetype row = 0; unchanged && row < sortedThemes.size(); ++row) {
        unchanged = code_editor_theme_selection_combobox->itemText(row) == sortedThemes.at(row).first && code_editor_theme_selection_combobox->itemData(row).toString() == sortedThemes.at(row).second;
    }
    if (unchanged) {
        return;
    }

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
    // Rebuilt long after population, which is not a user choice: a list lacking the profile's theme leaves the
    // box on no item, and writing that back would wipe the profile's theme.
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

        // The profile brings its own controls
        invalidateSearch();

        mpHost = pHost;
        // So make connections to the details of the real Host instance:
        mPopulating = true;
        initWithHost(pHost);
        mPopulating = false;
        // Covers the controls that only exist once there is a profile
        connectApplyTriggers();
        mSnapshot.take();
        guardScrollWheel();
        // Palette fix-ups for the controls just built
        applyShellStyle();
        // After the stylesheet, so the fit measures styled cards
        fitColumns();
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

        // Before the profile's controls are cleared and greyed out
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
        // The wildcard disconnects above removed instant apply's connections too
        connectApplyTriggers();
        // Nothing the controls now hold should be written back
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
            if (!RoomAppearance::symbolCodePointNeedsGlyph(pCodePoints.at(i))) {
                continue;
            }
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
        // theme-fixed: a picture of Discord's own panel, in Discord's colours
        // and at Discord's sizes, shown so the user can see what the game will
        // publish - it is not a surface of this dialog and follows neither its
        // theme nor its type scale. A raw string cannot carry a comment of its
        // own, so this one marks all of it.
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

// A style set while the dialog is open, such as by a script, replaces any pick
// not yet applied, so that Save does not write the older pick back over it
void dlgProfilePreferences::followMapStyle()
{
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }
    const int index = comboBox_mapStyle->findData(pHost->mapStyle());
    if (index != comboBox_mapStyle->currentIndex()) {
        const QSignalBlocker blocker(comboBox_mapStyle);
        comboBox_mapStyle->setCurrentIndex(index);
    }
    mSnapshot.take(comboBox_mapStyle);
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
    uiDesign::AlphaColourWell* pAlphaWell = hasAlpha ? uiDesign::AlphaColourWell::find(button) : nullptr;
    if (pAlphaWell && color.isValid()) {
        // The well draws itself - its swatch, its words and its sheet - at whatever state the button is in when
        // it is drawn, available or not
        pAlphaWell->setColour(color);
        button->setIcon(QIcon());
        return;
    }
    if (pAlphaWell) {
        pAlphaWell->setColour(QColor());
    }
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
                // theme-fixed: the fill is the colour the user picked, so the word on it is chosen against that fill rather than against a page
                button->setStyleSheet(
                        mudlet::self()->mTEXT_ON_BG_STYLESHEET.arg(color.lightness() > 127 ? QLatin1String("black") : QLatin1String("white"), color.name(), uiDesign::themeTokens().border.name()));
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
            // theme-fixed: the same well, desaturated - the word still sits on the value rather than on a page
            button->setStyleSheet(mudlet::self()->mTEXT_ON_BG_STYLESHEET.arg(QLatin1String("darkGray"), disabledColor.name(), uiDesign::themeTokens().border.name()));
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

void dlgProfilePreferences::slot_changeColouredIcons(const bool state)
{
    if (mpCheckBox_colouredIcons && mpCheckBox_colouredIcons->isChecked() != state) {
        mpCheckBox_colouredIcons->setChecked(state);
    }
    // The sidebar's icons, and the search headers and page title copied from them
    applyShellStyle();
}

#ifdef INCLUDE_MCPSERVER
QString dlgProfilePreferences::mcpServerNotReadyHint() const
{
    if (!checkBox_enableMCPServer->isChecked()) {
        //: Shown beneath the AI assistant settings, next to the restart notice, after Mudlet was added to an AI app while the server checkbox above is not on yet.
        return tr("Also tick the checkbox above, or it will find nobody to talk to.");
    }
    if (mudlet::self()->mcpEndpoint().isEmpty()) {
        //: Shown beneath the AI assistant settings, next to the restart notice, after Mudlet was added to an AI app while the server is switched on but could not start. "Advanced" is the label of the button that reveals the port.
        return tr("Mudlet is not serving yet though, so check the port under Advanced.");
    }
    return {};
}

void dlgProfilePreferences::slot_updateMCPServerEndpoint()
{
    updateMcpSummary();
    mudlet* pMudlet = mudlet::self();
    const quint16 port = static_cast<quint16>(spinBox_mcpServerPort->value());
    // The address carries the access token, which only exists once the server is up, so it
    // is shown verbatim rather than rebuilt from the port box.
    const QString liveEndpoint = pMudlet->mcpEndpoint();

    lineEdit_mcpServerAddress->setText(liveEndpoint);
    pushButton_copyMCPServerAddress->setEnabled(!liveEndpoint.isEmpty());
    pushButton_mcpConnectManually->setEnabled(!liveEndpoint.isEmpty());
    if (liveEndpoint.isEmpty()) {
        const QSignalBlocker blocker(pushButton_mcpConnectManually);
        pushButton_mcpConnectManually->setChecked(false);
        widget_mcpManualConnect->hide();
    }

    if (!liveEndpoint.isEmpty()) {
        // The port it answers on, not the one in the box: a failed move leaves the two
        // disagreeing, and the box is the request while this line is the outcome.
        //: Shown beneath the AI assistant settings while the MCP server is running. %1 is a TCP port number.
        label_mcpServerEndpoint->setText(tr("Listening on port %1.").arg(pMudlet->mcpServerPort()));
        return;
    }

    if (!checkBox_enableMCPServer->isChecked()) {
        //: Shown beneath the AI assistant settings while the MCP server is switched off.
        label_mcpServerEndpoint->setText(tr("The server is off, so nothing can reach Mudlet."));
        return;
    }

    if (!pMudlet->mcpLastError().isEmpty()) {
        // Every reason the server can refuse to start is answered by the port box, so fold
        // it out - the message telling the user to change it would otherwise point at
        // something they cannot see. Never folded back in: that is theirs to close.
        pushButton_mcpAdvanced->setChecked(true);
        // The reason is on the result line below the buttons, which this must not contradict
        //: Shown beneath the AI assistant settings when the MCP server could not open its port. The reason appears below the buttons.
        label_mcpServerEndpoint->setText(tr("The server is not running."));
        return;
    }

    // The moment between the box being ticked and the change being applied
    //: Shown beneath the AI assistant settings in the moment between the server being switched on and it answering. %1 is a TCP port number.
    label_mcpServerEndpoint->setText(tr("Starting on port %1...").arg(port));
}

// An empty line would still hold its row open beneath the buttons
void dlgProfilePreferences::setMCPConnectResult(const QString& message)
{
    label_mcpConnectResult->setText(message);
    label_mcpConnectResult->setVisible(!message.isEmpty());
}

void dlgProfilePreferences::slot_connectClaudeDesktop()
{
    switch (TMCPBridge::connectClaudeDesktop()) {
    case TMCPBridge::ConnectOutcome::Written: {
        mMCPRestartClaudeDesktop = true;
        updateMCPRestartNotice();
        refreshMCPAppButtons();
        mMCPServerStateMessage = mcpServerNotReadyHint();
        setMCPConnectResult(mMCPServerStateMessage);
        break;
    }
    case TMCPBridge::ConnectOutcome::NoClientApp: {
        const QString dir = TMCPBridge::claudeDesktopConfigDir();
        if (dir.isEmpty()) {
            //: Shown beneath the AI assistant settings when the location Claude Desktop keeps its settings in could not be worked out at all. Claude Desktop is a product name, leave it as-is.
            setMCPConnectResult(tr("Could not work out where Claude Desktop keeps its settings on this system."));
            break;
        }
        //: Shown beneath the AI assistant settings when the Claude Desktop application's settings folder does not exist. %1 is the folder Mudlet looked for, claude.com/download is a URL - leave it as-is.
        setMCPConnectResult(tr("Claude Desktop does not look installed - there is no %1. Install it from claude.com/download and open it once, then try again.").arg(dir));
        break;
    }
    case TMCPBridge::ConnectOutcome::NoBinaryPath:
        //: Shown beneath the AI assistant settings when Mudlet could not work out its own location on disk. Claude Desktop is a product name, leave it as-is.
        setMCPConnectResult(tr("Could not work out where this Mudlet is installed, so Claude Desktop was not told how to launch it."));
        break;
    case TMCPBridge::ConnectOutcome::ConfigUnreadable:
        //: Shown beneath the AI assistant settings when the Claude Desktop application's settings file could not be read or understood. %1 is the file's location.
        setMCPConnectResult(tr("Claude Desktop's settings file could not be read or understood, so it was left untouched. Check %1 for problems - a stray comma is enough - and try again.")
                                    .arg(TMCPBridge::claudeDesktopConfigFilePath()));
        break;
    case TMCPBridge::ConnectOutcome::WriteFailed:
        //: Shown beneath the AI assistant settings when an AI app's settings file could not be written. %1 is the file's location.
        setMCPConnectResult(tr("Could not write to %1 - check its file permissions.").arg(TMCPBridge::claudeDesktopConfigFilePath()));
        break;
    }
}

void dlgProfilePreferences::slot_connectChatGpt()
{
    switch (TMCPBridge::connectCodex()) {
    case TMCPBridge::ConnectOutcome::Written: {
        mMCPRestartChatGpt = true;
        updateMCPRestartNotice();
        refreshMCPAppButtons();
        mMCPServerStateMessage = mcpServerNotReadyHint();
        setMCPConnectResult(mMCPServerStateMessage);
        break;
    }
    case TMCPBridge::ConnectOutcome::NoClientApp:
        //: Shown beneath the AI assistant settings when the folder the ChatGPT desktop app and Codex keep their settings in does not exist. %1 is the folder Mudlet looked for, ChatGPT and Codex are product names - leave them as-is.
        setMCPConnectResult(tr("ChatGPT does not look installed - there is no %1. Install the ChatGPT desktop app or Codex and use it once, then try again.").arg(TMCPBridge::codexConfigDir()));
        break;
    case TMCPBridge::ConnectOutcome::NoBinaryPath:
        //: Shown beneath the AI assistant settings when Mudlet could not work out its own location on disk. ChatGPT is a product name, leave it as-is.
        setMCPConnectResult(tr("Could not work out where this Mudlet is installed, so ChatGPT was not told how to launch it."));
        break;
    case TMCPBridge::ConnectOutcome::ConfigUnreadable:
        //: Shown beneath the AI assistant settings when the settings file shared by the ChatGPT desktop app and Codex could not be read or understood. %1 is the file's location.
        setMCPConnectResult(tr("ChatGPT's settings file could not be read or understood, so it was left untouched. Check %1 for problems and try again.").arg(TMCPBridge::codexConfigFilePath()));
        break;
    case TMCPBridge::ConnectOutcome::WriteFailed:
        //: Shown beneath the AI assistant settings when an AI app's settings file could not be written. %1 is the file's location.
        setMCPConnectResult(tr("Could not write to %1 - check its file permissions.").arg(TMCPBridge::codexConfigFilePath()));
        break;
    }
}

// Empty when Mudlet was taken out, which needs no line of its own beside the restart notice
static QString mcpRemoveMessage(const TMCPBridge::RemoveOutcome outcome, const QString& appName, const QString& configFilePath)
{
    switch (outcome) {
    case TMCPBridge::RemoveOutcome::Removed:
        return {};
    case TMCPBridge::RemoveOutcome::NotRegistered:
        //: Shown beneath the AI assistant settings when Mudlet was asked to remove itself from an AI app's settings but was no longer there. %1 is the app's name.
        return dlgProfilePreferences::tr("Mudlet was no longer in %1's settings, so there was nothing to remove.").arg(appName);
    case TMCPBridge::RemoveOutcome::ConfigUnreadable:
        //: Shown beneath the AI assistant settings when an AI app's settings file could not be read or understood while removing Mudlet from it. %1 is the app's name, %2 the file's location.
        return dlgProfilePreferences::tr("%1's settings file could not be read or understood, so it was left untouched. Check %2 for problems and try again.").arg(appName, configFilePath);
    case TMCPBridge::RemoveOutcome::WriteFailed:
        //: Shown beneath the AI assistant settings when an AI app's settings file could not be written. %1 is the file's location.
        return dlgProfilePreferences::tr("Could not write to %1 - check its file permissions.").arg(configFilePath);
    }
    return {};
}

void dlgProfilePreferences::slot_removeClaudeDesktop()
{
    const TMCPBridge::RemoveOutcome outcome = TMCPBridge::removeClaudeDesktop();
    mMCPRestartClaudeDesktop |= outcome == TMCPBridge::RemoveOutcome::Removed;
    //: Name of the Claude Desktop application, used in messages about removing Mudlet from its settings. Claude Desktop is a product name, leave it as-is.
    setMCPConnectResult(mcpRemoveMessage(outcome, tr("Claude Desktop"), TMCPBridge::claudeDesktopConfigFilePath()));
    updateMCPRestartNotice();
    refreshMCPAppButtons();
}

void dlgProfilePreferences::slot_removeChatGpt()
{
    const TMCPBridge::RemoveOutcome outcome = TMCPBridge::removeCodex();
    mMCPRestartChatGpt |= outcome == TMCPBridge::RemoveOutcome::Removed;
    //: Name of the ChatGPT desktop app, used in messages about removing Mudlet from its settings. ChatGPT is a product name, leave it as-is.
    setMCPConnectResult(mcpRemoveMessage(outcome, tr("ChatGPT"), TMCPBridge::codexConfigFilePath()));
    updateMCPRestartNotice();
    refreshMCPAppButtons();
}

void dlgProfilePreferences::refreshMCPAppButtons()
{
    const auto present = [](QPushButton* pButton, QMenu* pMenu, const bool registered, const QString& removeText, const QString& registeredDescription) {
        pMenu->actions().constFirst()->setText(removeText);
        pButton->setMenu(registered ? pMenu : nullptr);
        pButton->setIcon(registered ? QIcon(qsl(":/icons/dialog-ok-apply.png")) : QIcon());
        // The .ui texts describe adding; kept to put back once Mudlet is removed again
        if (!pButton->property("mcpAddToolTip").isValid()) {
            pButton->setProperty("mcpAddToolTip", pButton->toolTip());
            pButton->setProperty("mcpAddDescription", pButton->accessibleDescription());
        }
        pButton->setToolTip(registered ? qsl("<p>%1</p>").arg(registeredDescription.toHtmlEscaped()) : pButton->property("mcpAddToolTip").toString());
        pButton->setAccessibleDescription(registered ? registeredDescription : pButton->property("mcpAddDescription").toString());
    };
    //: Menu entry on the Claude Desktop button once Mudlet is in that app's settings. Claude Desktop is a product name, leave it as-is.
    present(pushButton_connectClaudeDesktop,
            findChild<QMenu*>(qsl("menu_claudeDesktop")),
            TMCPBridge::claudeDesktopRegistered(),
            tr("Remove from Claude Desktop"),
            //: Tooltip and screen reader description of the Claude Desktop button once Mudlet is in that app's settings. Claude Desktop is a product name, leave it as-is.
            tr("Mudlet is added to Claude Desktop. Opens a menu to remove it again."));
    //: Menu entry on the ChatGPT button once Mudlet is in the settings shared by the ChatGPT desktop app and Codex. ChatGPT is a product name, leave it as-is.
    present(pushButton_connectChatGpt,
            findChild<QMenu*>(qsl("menu_chatGpt")),
            TMCPBridge::codexRegistered(),
            tr("Remove from ChatGPT"),
            //: Tooltip and screen reader description of the ChatGPT button once Mudlet is in the settings shared by the ChatGPT desktop app and Codex. ChatGPT and Codex are product names, leave them as-is.
            tr("Mudlet is added to ChatGPT and Codex. Opens a menu to remove it again."));
}

void dlgProfilePreferences::updateMCPRestartNotice()
{
    QString notice;
    if (mMCPRestartClaudeDesktop && mMCPRestartChatGpt) {
        //: Shown beneath the AI assistant settings after Mudlet was added to or removed from both AI apps' settings. Claude Desktop and ChatGPT are product names, leave them as-is.
        notice = tr("Please restart Claude Desktop and ChatGPT for the change to take effect");
    } else if (mMCPRestartClaudeDesktop) {
        //: Shown beneath the AI assistant settings after Mudlet was added to or removed from the Claude Desktop application's settings. Claude Desktop is a product name, leave it as-is.
        notice = tr("Please restart Claude Desktop for the change to take effect");
    } else if (mMCPRestartChatGpt) {
        //: Shown beneath the AI assistant settings after Mudlet was added to or removed from the settings shared by the ChatGPT desktop app and Codex. ChatGPT and Codex are product names, leave them as-is.
        notice = tr("Please restart ChatGPT (or Codex) for the change to take effect");
    }
    need_restart_for_ai_assistant->setText(notice);
    need_restart_for_ai_assistant->setVisible(!notice.isEmpty());
}

void dlgProfilePreferences::slot_copyMCPServerAddress()
{
    const QString endpoint = mudlet::self()->mcpEndpoint();
    if (endpoint.isEmpty()) {
        // The button disables itself while the server is down, but a stop can race the click.
        //: Shown beneath the AI assistant settings when the Copy address button is pressed while the server is not running.
        setMCPConnectResult(tr("There is no address to copy while the server is off."));
        return;
    }
    QGuiApplication::clipboard()->setText(endpoint);
    //: Shown beneath the AI assistant settings after the Copy address button is pressed.
    setMCPConnectResult(tr("Address copied - paste it into your AI assistant."));
}
#endif

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

// A certificate the connection complained about is called out on a wash of the
// warning hue, at the weight the editor's notice washes its accent at - light
// enough that the words on it are still the card's words.
static QString certificateWarningWash(const ThemeTokens& tokens)
{
    return rgba(uiDesign::stateColor(uiDesign::scmStateHue_warning, tokens.darkPage), uiDesign::scmSoftWashStrength);
}

// ...and what that wash comes out as over the card it lies on, which is what an
// ink on it has to be read against
static QColor certificateWarningSurface(const ThemeTokens& tokens)
{
    return blend(tokens.card, uiDesign::stateColor(uiDesign::scmStateHue_warning, tokens.darkPage), uiDesign::scmSoftWashStrength);
}

QString dlgProfilePreferences::certificateWarningCheckBoxStyle() const
{
    const ThemeTokens tokens = themeTokens();
    return qsl("font-weight: bold; color: %1; background: %2").arg(tokens.text.name(), certificateWarningWash(tokens));
}

QString dlgProfilePreferences::certificateWarningLabelStyle() const
{
    const ThemeTokens tokens = themeTokens();
    // The issuer and the expiry are the reading itself rather than chrome, so
    // they carry the one red anything broken is written in - walked on against
    // this wash, which is lighter than the surface that red was measured on and
    // so the harder of the two for a light ink to be read against
    const QColor ink = readableOn(certificateWarningSurface(tokens), uiDesign::errorInk(tokens), tokens.text, uiDesign::scmTextMinimumRatio);
    return qsl("font-weight: bold; color: %1; background: %2").arg(ink.name(), certificateWarningWash(tokens));
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

// Reached both from this dialog's own combo box and from the application
// telling everything that the appearance moved - which is what a second
// profile's settings dialog hears, and the only thing any other caller of
// mudlet::setAppearance() ever gets. That signal is emitted after the mode has
// already turned over, so a before/after reading around the call below says
// "nothing changed" on precisely the path that has everything to redraw. What
// this dialog was last drawn for is the honest question, and applyShellStyle()
// is where the answer is written down.
void dlgProfilePreferences::slot_setAppearance(const enums::Appearance state)
{
    if (comboBox_appearance->currentIndex() != state) {
        comboBox_appearance->setCurrentIndex(state);
    }

    // A no-op when the change came in through the signal, since the application
    // is already in this state. When it came from the combo box it emits that
    // signal, which re-enters here and does the work below - so the outer call
    // finds the shell already drawn for the new mode and stops at the line after.
    mudlet::self()->setAppearance(state);
    const bool isDarkMode = MudletApp::darkMode();

    if (isDarkMode == mShellStyledForDarkPage) {
        return;
    }

    // The shell's colours come from the palette, which has just been swapped -
    // and the certificate warnings, which are washed in a state colour, are
    // restyled from in there
    applyShellStyle();

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
    // retranslateUi() restored the .ui file's group box titles, and does not reach the shell
    retitleCards();
    retranslateShell();
#ifdef INCLUDE_MCPSERVER
    // retranslateUi() put back the adding texts, now in the new language
    for (auto* pButton : {pushButton_connectClaudeDesktop, pushButton_connectChatGpt}) {
        pButton->setProperty("mcpAddToolTip", QVariant());
        pButton->setProperty("mcpAddDescription", QVariant());
    }
    refreshMCPAppButtons();
    updateMCPRestartNotice();
#endif
    // Every text the search index and column widths were measured from has been replaced, including
    // checkboxes fitted to the old language, which can also change the width the sidebar needs
    invalidateSearch();
    fitColumns();
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
    pHost->setPlayerRoomStyle(static_cast<quint8>(style));
    if (!pHost->mpMap->mapper() || !pHost->mpMap->mapper()->mp2dMap) {
        return;
    }
    pHost->mpMap->mapper()->mp2dMap->setPlayerRoomStyle(style);
    // And update the displayed map:
    pHost->mpMap->mapper()->repaintMaps();
}

void dlgProfilePreferences::slot_setPlayerRoomPrimaryColor()
{
    Host* pHost = mpHost;
    if (!pHost || !mpHost->mpMap) {
        return;
    }

    setPlayerRoomColor(pushButton_playerRoomPrimaryColor, mpHost->mpMap->mPlayerRoomOuterColor);
    pHost->setPlayerRoomOuterColor(mpHost->mpMap->mPlayerRoomOuterColor);
    if (comboBox_playerRoomStyle->currentIndex() != 3) {
        return;
    }

    if (mpHost->mpMap->mapper() && mpHost->mpMap->mapper()->mp2dMap) {
        // The current setting IS for the custom color - so use it straight away:
        mpHost->mpMap->mapper()->mp2dMap->setPlayerRoomStyle(3);
        // And update the displayed map:
        mpHost->mpMap->mapper()->repaintMaps();
    }
}

void dlgProfilePreferences::slot_setPlayerRoomSecondaryColor()
{
    Host* pHost = mpHost;
    if (!pHost || !mpHost->mpMap) {
        return;
    }

    setPlayerRoomColor(pushButton_playerRoomSecondaryColor, mpHost->mpMap->mPlayerRoomInnerColor);
    pHost->setPlayerRoomInnerColor(mpHost->mpMap->mPlayerRoomInnerColor);
    if (comboBox_playerRoomStyle->currentIndex() != 3) {
        return;
    }

    if (mpHost->mpMap->mapper() && mpHost->mpMap->mapper()->mp2dMap) {
        // The current setting IS for the custom color - so use it straight away:
        mpHost->mpMap->mapper()->mp2dMap->setPlayerRoomStyle(3);
        // And update the displayed map:
        mpHost->mpMap->mapper()->repaintMaps();
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
        pHost->setPlayerRoomOuterDiameter(static_cast<quint8>(value));
        if (pHost->mpMap->mapper() && pHost->mpMap->mapper()->mp2dMap) {
            // And update the displayed map:
            pHost->mpMap->mapper()->repaintMaps();
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
        pHost->setPlayerRoomInnerDiameter(static_cast<quint8>(value));
        if (pHost->mpMap->mapper() && pHost->mpMap->mapper()->mp2dMap) {
            // Redefine the QGradientStops
            pHost->mpMap->mapper()->mp2dMap->setPlayerRoomStyle(qBound(0, comboBox_playerRoomStyle->currentIndex(), 3));
            // And update the displayed map:
            pHost->mpMap->mapper()->repaintMaps();
        }
    }
}

void dlgProfilePreferences::setPlayerRoomColor(QPushButton* b, QColor& c)
{
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }

    // The dialog runs an event loop of its own, in which the button or the whole window can go
    const QPointer<dlgProfilePreferences> pSelf(this);
    const QPointer<QPushButton> pClicked(b);
    auto color = QColorDialog::getColor(
            c, this, (b == pushButton_playerRoomPrimaryColor ? tr("Set outer color of player room mark.") : tr("Set inner color of player room mark.")), QColorDialog::ShowAlphaChannel);
    if (!pSelf) {
        return;
    }
    returnFromColourDialog(pClicked);
    if (color.isValid()) {
        c = color;

        // Also sets a contrasting foreground color so text will always be
        // visible and adjusts the saturation of a disabled button:
        setButtonColor(b, color, true);
    }
}

void dlgProfilePreferences::returnFromColourDialog(QWidget* pClicked)
{
    if (!isVisible()) {
        return;
    }
    raise();
    activateWindow();
    if (pClicked) {
        pClicked->setFocus(Qt::OtherFocusReason);
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
    if (mpHost) {
        mpHost->setAdvertiseScreenReader(state);
    }
}

void dlgProfilePreferences::slot_toggleAccessibleLabels(const bool state)
{
    if (mpHost) {
        mpHost->setAccessibleLabels(state);
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
    if (mpHost) {
        mpHost->setEnableClosedCaption(state);
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
        // A size typed into the box may not have been applied yet
        mBufferSizeBeforeMax = console_buffer_size_spinBox->value();
        if (pHost->mainConsoleView()) {
            const int maxBufferSize = pHost->mainConsoleView()->buffer.getMaxBufferSize();
            console_buffer_size_spinBox->setValue(maxBufferSize);
        }
        console_buffer_size_spinBox->setEnabled(false);
    } else {
        console_buffer_size_spinBox->setEnabled(true);
        console_buffer_size_spinBox->setValue(mBufferSizeBeforeMax);
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
    if (pHost->mpMap && pHost->mpMap->mapper() && pHost->mpMap->mapper()->mp2dMap) {
        pHost->mpMap->mapper()->repaintMaps();
    }
}

void dlgProfilePreferences::slot_changeDrawUpperLowerLevels(const bool state)
{
    mudlet::self()->setDrawUpperLowerLevels(state);
}

void dlgProfilePreferences::slot_changeMapperUseAntiAlias(const bool state)
{
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }

    pHost->mMapperUseAntiAlias = state;
    if (pHost->mpMap && pHost->mpMap->mapper() && pHost->mpMap->mapper()->mp2dMap) {
        pHost->mpMap->mapper()->mp2dMap->mMapperUseAntiAlias = state;
        pHost->mpMap->mapper()->mp2dMap->update();
    }
}

// Tells screen-reader users the hyperlink shortcuts when they pick a pane-switching key, so they need not
// look them up in the wiki
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

// Under the display font's picker and the editor font's: false when Mudlet cannot draw in the font at all
static bool showFontWarnings(const QFont& font, QLabel* pInvalidLabel, QLabel* pVariableWidthLabel)
{
    const QFontMetrics metrics(font);
    if (metrics.averageCharWidth() == 0) {
        pInvalidLabel->show();
        return false;
    }
    pInvalidLabel->hide();
    pVariableWidthLabel->setVisible(!QFontInfo(font).fixedPitch());
    return true;
}

bool dlgProfilePreferences::updateDisplayFont(const Host::DisplayFontChange change)
{
    if (mpHost.isNull() || !mpHost->mainConsoleView()) {
        return false;
    }

    QFont displayFont = fontComboBox_displayFont->currentFont();
    displayFont.setPointSize(spinBox_displayFontSize->value());
    displayFont.setStyleHint(QFont::AnyStyle,
                             checkBox_antiAlias->isChecked() ? static_cast<QFont::StyleStrategy>(QFont::PreferAntialias | QFont::PreferQuality)
                                                             : static_cast<QFont::StyleStrategy>(QFont::NoAntialias | QFont::PreferQuality));

    if (TFontAttributes(mpHost->getDisplayFont()) == TFontAttributes(displayFont)) {
        // Nothing changes on screen, but a pick still has to reach the Host: re-picking
        // the family standing in for a missing font settles for it. Every pick of a new
        // family lands here as well, as activated follows currentFontChanged
        if (change == Host::DisplayFontChange::UserChoice) {
            mpHost->setDisplayFont(displayFont, change);
        }
        return false;
    }

    if (!showFontWarnings(displayFont, label_invalidFontError, label_variableWidthFontWarning)) {
        return false;
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

// A missing dictionary picked again, once installed, is no change of entry for the apply to see,
// so this is what retries it
void dlgProfilePreferences::slot_dictionaryPicked(const int index)
{
    if (!mpHost) {
        return;
    }
    const QString dictionary = comboBox_dictionary->itemData(index).toString();
    mpHost->setSpellDic(dictionary);
    if (TSpellChecker::systemDictionaryFound(dictionary)) {
        comboBox_dictionary->setItemText(index, dictionaryDisplayName(dictionary));
    }
}

QString dlgProfilePreferences::dictionaryDisplayName(const QString& dictionary)
{
    auto key = dictionary.toLower();
    key.replace(QLatin1String("-"), QLatin1String("_"));
    return mudlet::self()->mDictionaryLanguageCodeMap.value(key, tr("%1 - not recognised").arg(dictionary));
}

void dlgProfilePreferences::rememberDisplayFontFamilyShown()
{
    const QString family = fontComboBox_displayFont->currentFont().family();
    mDisplayFontFamilyShown = (mpHost && mpHost->resolveFontFamily(family).available) ? family : QString();
}

// QFontComboBox::setFontFilters() moves the list off a family it leaves out, which would put another font
// on the profile, so proportional fonts are left out only while the font shown is a monospaced one
void dlgProfilePreferences::showDisplayFont(const QFont& font)
{
    const bool monospacedOnly = checkBox_monospacedFontsOnly->isChecked() && QFontDatabase::isFixedPitch(font.family());
    const QSignalBlocker blocker(fontComboBox_displayFont);
    fontComboBox_displayFont->setFontFilters(monospacedOnly ? QFontComboBox::MonospacedFonts : QFontComboBox::AllFonts);
    fontComboBox_displayFont->setCurrentFont(font);
}

void dlgProfilePreferences::slot_displayFontChanged()
{
    // QFontComboBox moves off a family by itself when its package is uninstalled: no choice of the
    // user's, and the profile's own check is about to put the stand-in up and say so
    const QString shownBefore = mDisplayFontFamilyShown;
    rememberDisplayFontFamilyShown();
    if (mpHost.isNull() || (!shownBefore.isEmpty() && !mpHost->resolveFontFamily(shownBefore).available)) {
        return;
    }
    // Past that, only a pick out of the list gets here (currentFontChanged and activated both land
    // here, so a second run for the same font must change nothing), so the family really is the user's choice
    if (updateDisplayFont(Host::DisplayFontChange::UserChoice)) {
        mpHost->mTelnet.sendInfoNewEnvironValue(qsl("FONT"));
    }
}

void dlgProfilePreferences::slot_displayFontSizeChanged()
{
    if (!mpHost.isNull() && updateDisplayFont(Host::DisplayFontChange::Adjustment)) {
        mpHost->mTelnet.sendInfoNewEnvironValue(qsl("FONT_SIZE"));
    }
}

// The font can change behind the dialog, from a script or as a package brings a missing family
// back, and the next edit here builds the whole font from these controls
void dlgProfilePreferences::slot_hostDisplayFontChanged(const QFont& font)
{
    // The connection outlives slot_handleHostDeletion(), which has cleared these controls
    if (mpHost.isNull()) {
        return;
    }
    {
        const QSignalBlocker comboBlocker(fontComboBox_displayFont);
        const QSignalBlocker sizeBlocker(spinBox_displayFontSize);
        const QSignalBlocker antiAliasBlocker(checkBox_antiAlias);
        showDisplayFont(font);
        spinBox_displayFontSize->setMaximum(std::max(font.pointSize(), spinBox_displayFontSize->maximum()));
        spinBox_displayFontSize->setValue(std::max(1, font.pointSize()));
        // Not fontsAntiAlias(): applyAll() writes that from this box only once the apply timer fires
        checkBox_antiAlias->setChecked(!(font.styleStrategy() & QFont::NoAntialias));
    }
    rememberDisplayFontFamilyShown();
    // Not the user's edits, so they must not keep refreshFromSettings() from re-reading the rest - nor
    // the line edits inside the list and the spin box, which the snapshot holds as well. The anti-alias
    // box stays dirty, as this runs for the dialog's own toggle too and applyAll() must still store it
    for (QWidget* pControl : {static_cast<QWidget*>(fontComboBox_displayFont), static_cast<QWidget*>(spinBox_displayFontSize)}) {
        mSnapshot.take(pControl);
        for (const auto* pPart : pControl->findChildren<QWidget*>()) {
            if (SettingsSnapshot::carriesValue(pPart)) {
                mSnapshot.take(pPart);
            }
        }
    }
}

void dlgProfilePreferences::slot_displayFontAliasingChanged()
{
    updateDisplayFont(Host::DisplayFontChange::Adjustment);
}

// Writes the controls only, so the caller decides whether the snapshot is to follow
void dlgProfilePreferences::showEditorFont(const QFont& font)
{
    {
        const QSignalBlocker comboBlocker(fontComboBox_editorFont);
        const QSignalBlocker sizeBlocker(spinBox_editorFontSize);
        fontComboBox_editorFont->setCurrentFont(font);
        spinBox_editorFontSize->setMaximum(std::max(font.pointSize(), 40));
        spinBox_editorFontSize->setValue(std::max(1, font.pointSize()));
    }
    reflectEditorFontControls();
}

void dlgProfilePreferences::reflectEditorFontControls()
{
    // Disabled rather than hidden while following, so what following gives is on show
    const bool ownFont = !checkBox_editorFontFollowsDisplay->isChecked();
    fontComboBox_editorFont->setEnabled(ownFont);
    spinBox_editorFontSize->setEnabled(ownFont);
    // ...and the display font's own card says whatever there is to say about that one
    if (!ownFont) {
        label_editorFontInvalid->hide();
        label_editorFontVariableWidth->hide();
        return;
    }
    QFont font = fontComboBox_editorFont->currentFont();
    font.setPointSize(spinBox_editorFontSize->value());
    showFontWarnings(font, label_editorFontInvalid, label_editorFontVariableWidth);
}

// A script, or the display font moving while the editor follows it
void dlgProfilePreferences::slot_hostEditorFontChanged(const QFont& font)
{
    if (mpHost.isNull()) {
        return;
    }
    {
        const QSignalBlocker followBlocker(checkBox_editorFontFollowsDisplay);
        checkBox_editorFontFollowsDisplay->setChecked(mpHost->editorFontFollowsDisplay());
    }
    showEditorFont(font);
    auto config = edbeePreviewWidget->config();
    config->beginChanges();
    config->setFont(font);
    config->endChanges();
    // As in slot_hostDisplayFontChanged(): not the user's edits
    for (QWidget* pControl : {static_cast<QWidget*>(checkBox_editorFontFollowsDisplay), static_cast<QWidget*>(fontComboBox_editorFont), static_cast<QWidget*>(spinBox_editorFontSize)}) {
        mSnapshot.take(pControl);
        for (const auto* pPart : pControl->findChildren<QWidget*>()) {
            if (SettingsSnapshot::carriesValue(pPart)) {
                mSnapshot.take(pPart);
            }
        }
    }
}

// The timestamp button and scripts move this setting while the dialog is in front, and the newer choice
// has to win over a tick of the box still waiting out the apply debounce
void dlgProfilePreferences::followMainConsoleTimeStamps()
{
    if (!mpHost) {
        return;
    }
    const QSignalBlocker blocker(checkBox_showTimeStamps);
    checkBox_showTimeStamps->setChecked(mpHost->mainConsoleShowsTimeStamps());
    mSnapshot.take(checkBox_showTimeStamps);
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
        mpHost->mRoomSize = size / 10.0;
        if (mpHost->mpMap && mpHost->mpMap->mapper() && mpHost->mpMap->mapper()->mp2dMap) {
            mpHost->mpMap->mapper()->mp2dMap->setRoomSize(mpHost->mRoomSize);
            mpHost->mpMap->mapper()->mp2dMap->update();
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
        if (mpHost->mpMap && mpHost->mpMap->mapper() && mpHost->mpMap->mapper()->mp2dMap) {
            mpHost->mpMap->mapper()->mp2dMap->setExitSize(internalSize);
        }
    }
}

void dlgProfilePreferences::slot_borderSizeChanged(int size)
{
    if (mpHost) {
        const double internalSize = 50.0 / size;
        mpHost->mRoomBorderSize = internalSize;
        if (mpHost->mpMap && mpHost->mpMap->mapper() && mpHost->mpMap->mapper()->mp2dMap) {
            mpHost->mpMap->mapper()->mp2dMap->setBorderSize(internalSize);
            mpHost->mpMap->mapper()->update3DView();
        }
    }
}

void dlgProfilePreferences::slot_gridSizeChanged(double size)
{
    if (mpHost) {
        mpHost->mMapGridLineSize = size;
        if (mpHost->mpMap) {
            mpHost->mpMap->requestMapRepaint();
        }
    }
}

void dlgProfilePreferences::reject()
{
    // Esc goes up a level before leaving the dialog, and search results are a level; clearing the query
    // leaves them
    if (!mClosing && mSearchActive) {
        mpLineEdit_search->clear();
        return;
    }
    // On a subpage Esc acts as the back chevron
    if (!mClosing && !mCurrentSubpage.isEmpty()) {
        leaveSubpage();
        return;
    }
    // QDialog::reject() hides without a close event, so Esc goes through close(), which comes back here from
    // QDialog::closeEvent() with only the base class left to run
    if (!mClosing) {
        close();
        return;
    }
    QDialog::reject();
}

void dlgProfilePreferences::closeEvent(QCloseEvent* event)
{
    // First, as the apply below ends by re-reading the settings
    mClosing = true;
    cancelShortcutCaptures();

    if (mpDialogMapGlyphUsage) {
        mpDialogMapGlyphUsage->close();
        mpDialogMapGlyphUsage = nullptr;
    }

    // Closing is not a discard, and the debounce will not outlive the dialog. Clearing the focus makes a
    // field being typed into report as finished.
    if (QWidget* pFocus = focusWidget(); pFocus) {
        pFocus->clearFocus();
    }
    mpTimer_apply->stop();
    applyAll();

    // The profile XML is written once per close rather than on every apply
    if (Host* pHost = mpHost; pHost && pHost->mFORCE_SAVE_ON_EXIT) {
        pHost->saveProfileReportingFailure();
    }

    MudletApp::getQSettings()->setValue(qsl("profilePreferencesGeometry"), saveGeometry());
    // Most application-wide settings reach Mudlet.ini only at quit, so an
    // unclean exit would lose the ones changed here
    mudlet::self()->writeSettings();

    if (mpHost) {
        emit preferencesClosing(mpHost->getName());
    }
    QDialog::closeEvent(event);
    mClosing = false;
}
