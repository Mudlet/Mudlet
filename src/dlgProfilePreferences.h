#ifndef MUDLET_DLGPROFILEPREFERENCES_H
#define MUDLET_DLGPROFILEPREFERENCES_H

/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017-2018, 2022 by Stephen Lyons                        *
 *                                               - slysven@virginmedia.com *
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


#include "Host.h"
#include "mudlet.h"

#include "ui_profile_preferences.h"
#include "uiDesign.h"
#include <QDialog>
#include <QHash>
#include <QList>
#include <QMap>

class QCloseEvent;
class QDoubleSpinBox;
class QEvent;
class QFrame;
class QListWidget;
class QListWidgetItem;
class QResizeEvent;
class QScrollArea;
class QStackedWidget;
class QToolButton;
class QVBoxLayout;
class TAction;
class TAlias;
class TKey;
class TKeySequenceEdit;
class TScript;
class TTimer;
class TTrigger;

namespace uiDesign {
class SidebarToggle;
struct ThemeTokens;
}


class dlgProfilePreferences : public QDialog, public Ui::profile_preferences
{
    Q_OBJECT

    // Allow QTest-based test classes to access private members
    friend class SettingsAppearanceTest;
    friend class SettingsMapControlsTest;
    friend class SettingsPageFlowTest;
    friend class SettingsShellNavigationTest;

public:
    Q_DISABLE_COPY(dlgProfilePreferences)
    explicit dlgProfilePreferences(QWidget*, Host* pHost = nullptr);
    ~dlgProfilePreferences();
    void setTab(QString tab);

public slots:
    // Terminal colors.
    void slot_setColorBlack();
    void slot_setColorLightBlack();
    void slot_setColorRed();
    void slot_setColorLightRed();
    void slot_setColorBlue();
    void slot_setColorLightBlue();
    void slot_setColorGreen();
    void slot_setColorLightGreen();
    void slot_setColorYellow();
    void slot_setColorLightYellow();
    void slot_setColorCyan();
    void slot_setColorLightCyan();
    void slot_setColorMagenta();
    void slot_setColorLightMagenta();
    void slot_setColorWhite();
    void slot_setColorLightWhite();
    void slot_setFgColor();
    void slot_setBgColor();
    void slot_setCommandLineBgColor();
    void slot_setCommandLineFgColor();
    void slot_setCommandBgColor();
    void slot_setCommandFgColor();
    void slot_resetColors();

    // Mapper colors.
    void slot_setMapColorBlack();
    void slot_setMapColorLightBlack();
    void slot_setMapColorRed();
    void slot_setMapColorLightRed();
    void slot_setMapColorBlue();
    void slot_setMapColorLightBlue();
    void slot_setMapColorGreen();
    void slot_setMapColorLightGreen();
    void slot_setMapColorYellow();
    void slot_setMapColorLightYellow();
    void slot_setMapColorCyan();
    void slot_setMapColorLightCyan();
    void slot_setMapColorMagenta();
    void slot_setMapColorLightMagenta();
    void slot_setMapColorWhite();
    void slot_setMapColorLightWhite();
    void slot_setMapExitsColor();
    void slot_setMapBgColor();
    void slot_setMapRoomBorderColor();
    void slot_setMapInfoBgColor();
    void slot_setMapRoomCollisionBorderColor();
    void slot_setMapGridColor();
    void slot_setLowerLevelColor();
    void slot_setUpperLevelColor();
    void slot_resetMapColors();

    // Map.
    void slot_downloadMap();
    void slot_loadMap();
    void slot_saveMap();
    void slot_copyMap();
    void slot_chosenProfilesChanged(QAction*);
    void slot_showMapGlyphUsage();

    // Media
    void slot_purgeMediaCache();
    void slot_toggleEnableClosedCaption(const bool);

    // Log.
    void slot_setLogDir();
    void slot_resetLogDir();
    void slot_logFileNameFormatChange(int index);
    void slot_changeLogFileAsHtml(bool isHtml);

    // Chat
    void slot_setMMCPChatName(const QString&);
    void slot_mmcpChatNameChanged();

    void slot_hideActionLabel();
    void slot_setEncoding(const int);

    void slot_handleHostAddition(Host*, quint8);
    void slot_handleHostDeletion(Host*);

    void slot_guiLanguageChanged(const QString&);

    void reject() override;

private slots:
    void slot_forgetSavedSignIn();
    void slot_changeShowSpacesAndTabs(bool);
    void slot_changeShowLineFeedsAndParagraphs(bool);
    void slot_scriptSelected(int index);
    void slot_categorySelected(const int row);
    void slot_sidebarItemClicked(QListWidgetItem*);
    void slot_scheduleApply();
    void slot_lineEditFinished();
    void slot_themeSelected(int index);
    void slot_setMapSymbolFont(const QFont&);
    void slot_setMapSymbolFontStrategy(bool);
    void slot_mapSymbolFontChanged();
    void slot_changeShowMenuBar(int);
    void slot_changeShowToolBar(int);
    void slot_changeEditorTextOptions(const QTextOption::Flags);
    void slot_setAppearance(const enums::Appearance);
    void slot_changeShowMapAuditErrors(const bool);
    void slot_changeColouredIcons(const bool);
#ifdef INCLUDE_MCPSERVER
    void slot_updateMCPServerEndpoint();
    void slot_connectClaudeDesktop();
    void slot_removeClaudeDesktop();
    void slot_connectChatGpt();
    void slot_removeChatGpt();
    void slot_copyMCPServerAddress();
#endif
    void slot_changeAutomaticUpdates(const bool);
    void slot_setToolBarIconSize(const int);
    void slot_setTreeWidgetIconSize(const int);
    void slot_changeMenuBarVisibility(const enums::controlsVisibility);
    void slot_changeToolBarVisibility(const enums::controlsVisibility);
    // Greys out the "Never" entry in the other toolbar-visibility comboBox so
    // both bars cannot be hidden simultaneously (issue #7079).
    void slot_syncMenuToolBarNeverItem();
    void slot_changeShowIconsOnMenus(const Qt::CheckState);
    void slot_changeGuiLanguage(int);
    void slot_passwordStorageLocationChanged(int);
    void slot_changePlayerRoomStyle(const int);
    void slot_setPlayerRoomPrimaryColor();
    void slot_setPlayerRoomSecondaryColor();
    void slot_setPlayerRoomOuterDiameter(const int);
    void slot_setPlayerRoomInnerDiameter(const int);
    void slot_setPostingTimeout(const double);
    void slot_changeControlCharacterHandling();
    void slot_toggleAdvertiseScreenReader(const bool);
    void slot_toggleAccessibleLabels(const bool);
    void slot_toggleEnableOSC8Hyperlinks(const bool);
    void slot_changeWrapAt();
    void slot_toggleUseMaxBufferSize(bool checked);
    void slot_deleteMap();
    void slot_changeLargeAreaExitArrows(const bool);
    void slot_changeInvertMapZoom(const bool);
    void slot_hidePasswordMigrationLabel();
    void slot_loadHistoryMap();
    void slot_roomSizeChanged(int size);
    void slot_exitSizeChanged(int size);
    void slot_borderSizeChanged(int size);
    void slot_gridSizeChanged(double size);
    void slot_displayFontChanged();
    void slot_dictionaryPicked(const int index);
    void slot_displayFontSizeChanged();
    void slot_displayFontAliasingChanged();
    void slot_hostDisplayFontChanged(const QFont&);
    void slot_hostEditorFontChanged(const QFont&);
    void slot_changeShowTabConnectionIndicators(bool state);
    void slot_crashReportPolicyChanged(int index);
    // Named rather than lambdas so initWithHost() can connect them with Qt::UniqueConnection
    void slot_mapSymbolFontFudgeChanged(const double factor);
    void slot_changeMapperShowRoomBorders(const bool state);
    void slot_changeDrawUpperLowerLevels(const bool state);
    void slot_changeMapperUseAntiAlias(const bool state);
    void slot_caretModeKeyChanged(const int index);


signals:
    void signal_themeUpdateCompleted();
    void signal_preferencesSaved();
    void signal_resetMainWindowShortcutsToDefaults();
    void preferencesClosing(const QString& profileName);

protected:
    void closeEvent(QCloseEvent* event) override;
    bool event(QEvent* pEvent) override;
    bool eventFilter(QObject* pObject, QEvent* pEvent) override;
    void resizeEvent(QResizeEvent* pEvent) override;
    void changeEvent(QEvent* pEvent) override;

private:
    void setColors();
    void setColors2();
    void setButtonAndProfileColor(QPushButton*, QColor&, bool allowAlpha = false);
    void setPlayerRoomColor(QPushButton*, QColor&);
    // After a colour dialog, however it was closed: macOS's native colour panel hands the activation to the
    // application's main window as it closes, which leaves the settings behind it with nothing focused
    void returnFromColourDialog(QWidget* pClicked);
    void setButtonColor(QPushButton*, const QColor&, const bool hasAlpha = false);
    void loadEditorTab();
    void populateThemesList();
    void populateScriptsList();
    void addTriggersToPreview(TTrigger* pTriggerParent, std::vector<std::tuple<QString, QString, int>>& items);
    void addAliasesToPreview(TAlias* pAliasParent, std::vector<std::tuple<QString, QString, int>>& items);
    void addTimersToPreview(TTimer* pTimerParent, std::vector<std::tuple<QString, QString, int>>& items);
    void addActionsToPreview(TAction* pActionParent, std::vector<std::tuple<QString, QString, int>>& items);
    void addScriptsToPreview(TScript* pScriptParent, std::vector<std::tuple<QString, QString, int>>& items);
    void addKeysToPreview(TKey* pKeyParent, std::vector<std::tuple<QString, QString, int>>& items);
#ifdef INCLUDE_MCPSERVER
    // What still stands between a registered assistant and a reachable Mudlet,
    // or an empty string when nothing does
    QString mcpServerNotReadyHint() const;
#endif
    // Must stay re-runnable for refreshFromSettings(): build once, empty every list before filling it,
    // and make every connection Qt::UniqueConnection or build-once.
    void initWithHost(Host*);
    // Same contract, for the application-wide settings
    void populateApplicationSettings();
    void refreshFromSettings();
    QString certificateWarningCheckBoxStyle() const;
    QString certificateWarningLabelStyle() const;
    void restyleCertificateWarnings();
    void disableHostDetails();
    void enableHostDetails();
    void clearHostDetails();
    void disconnectHostRelatedControls();
    void generateMapGlyphDisplay();
    void generateDiscordTooltips();
    void setupPasswordsMigration();
    QString mapSaveLoadDirectory(Host* pHost);
    void loadMap(const QString&);
    void fillOutMapHistory();
    bool updateDisplayFont(Host::DisplayFontChange change);
    void rememberDisplayFontFamilyShown();
    void showDisplayFont(const QFont&);
    void showEditorFont(const QFont&);
    void reflectEditorFontControls();
    void cancelShortcutCaptures();
    void updateShortcutConflictWarning();
    void switchEditorTheme(const QString& themeName);
    static QString dictionaryDisplayName(const QString&);
    static QString findThemeCounterpart(const QString& themeName, const QComboBox* themeComboBox, bool toDark);

    // The sidebar-and-cards shell that replaces the .ui file's tab widget
    void buildShell();
    QWidget* buildSidebar();
    void addCategory(const QString& key, const QString& iconFile);
    // The single declaration of the categories: sidebar order, icon, name and separators.
    // Re-read on a language change to retranslate the names.
    struct CategoryDefinition
    {
        QString key;
        QString iconFile;
        QString name;
        bool separatorAbove = false;
    };
    QList<CategoryDefinition> categoryDefinitions() const;
    // -1 for an unknown key
    int categoryRow(const QString& key) const;
    void addSidebarSeparator();
    QScrollArea* createScrollPage(const QString& objectSuffix);
    QScrollArea* buildPage(const QString& objectSuffix, const QList<QWidget*>& cards);
    void buildCategoryPage(const QString& key, const QList<QWidget*>& cards);
    // The sidebar stays on the parent category; a breadcrumb with a back chevron leads out
    void addSubpage(const QString& categoryKey, const QString& subKey, QWidget* pOpenerCard, const QList<QWidget*>& cards);
    void showSubpage(const QString& categoryKey, const QString& subKey, QWidget* pSpotlightTarget = nullptr);
    void leaveSubpage();
    // "Category › Subpage" - the widest title, so sidebarWidths() measures the title row against it
    QString breadcrumbFor(const QString& subpageKey) const;
    // "category/sub", or empty for a widget on a category page
    QString subpageHolding(const QWidget* pWidget) const;
    QGroupBox* createCard(const QString& objectName);
    // Every label here that can hold a link goes through this: an anchor's
    // colour is written into the text rather than answered by a palette
    void setLinkedText(QLabel* pLabel, const QString& richText);
    // A muted line under the card's title, ending in a "Learn more" link if learnMoreUrl is given
    void setCardDescription(QGroupBox* pCard, const QString& description, const QString& learnMoreUrl = QString());
    void setCardDescriptions();
    void buildProtocolsSubpage();
    void updateProtocolSummary();
    // One checkbox per experiment the Host accepts, so a new one needs no work here
    void buildExperimentsCard();
    void retranslateExperiments();
    void buildDiscordSummaryCard();
    void updateDiscordSummary();
#ifdef INCLUDE_MCPSERVER
    // The AI assistant settings, reached from the General page's summary card
    void buildMcpSummaryCard();
    void updateMcpSummary();
#endif
    // Shows the current connection's actual security, not what the settings ask for
    void buildSecurityStatusCard();
    void updateSecurityStatus();
    // Strings setupUi() did not create, which retranslateUi() therefore cannot restore on a language change
    void retranslateShell();
    // Search synonyms not in a setting's own words; here rather than in the .ui file so they can carry
    // a translator note.
    void setSearchKeywords();
    // Matching triggers on several threads during floods: app-wide, in Mudlet.ini, built in code
    void createTriggerMatchPoolControls(QGroupBox* pCard);
    void retranslateTriggerMatchPoolControls();
    void populateTriggerMatchPoolSettings();
    void applyTriggerMatchPoolSettings();
    void enableTriggerMatchPoolTuning(bool enabled);
    void moveIntoCard(QGroupBox* pCard, const QList<QWidget*>& controls);
    void addCardRow(QGroupBox* pCard, QWidget* pLabel, QWidget* pControl);
    void retitleCards();
    void reflowWideCards();
    void reflowMapperCards();
    void reflowColourCards();
    void reflowGeneralCards();
    void reflowPrivacyCards();
    void reflowDisplayOptionsCard();
    void reflowCompatibilityCard();
    void fitColumn(QScrollArea* pScrollArea);
    // Rerun once a profile has filled the controls, which decide the widest card, and after a language change
    void fitColumns();
    // A checkbox never wraps its text, so a long translation makes the page scroll sideways. Offenders
    // get a wrapping label beside them, one at a time and only while that measurably narrows the column.
    void fitCheckBoxesToColumn(QWidget* pColumn);
    void wrapCheckBox(QCheckBox* pCheckBox);
    void unwrapCheckBox(QCheckBox* pCheckBox);
    // Window widths: below collapseBelow the sidebar becomes an icon rail; fullyExpanded has the names beside
    // the widest page's column, the most a window with no remembered size opens at; below narrowest even the
    // rail leaves the widest page too little room
    struct SidebarWidths
    {
        int collapseBelow = 0;
        int fullyExpanded = 0;
        int narrowest = 0;
    };
    SidebarWidths sidebarWidths() const;
    // What the sidebar is drawn at with its names showing, measured off the
    // widest of them the way the editor's is
    [[nodiscard]] int measuredSidebarWidth() const;
    // Called wherever the names, the font or the style they are measured
    // against change, since none of those reach a resize
    void invalidateSidebarWidth();
    void updateSidebarMode();
    void setSidebarCollapsed(bool collapsed);
    // The chevron on the seam, and the one place the preference it carries
    // is changed
    void updateSidebarToggle();
    // Widens the window to wantedWidth if its screen can hold that, keeping it on that screen
    bool refitWindowTo(const int wantedWidth);
    void setSidebarLabelsShown(const bool shown);
    void rebuildTabOrder();
    void guardScrollWheel();
    void buildMigrationBanner();
    void placeBannerOn(QWidget* pColumn);
    void updateNoProfileNotice();
    void showCategory(const QString& key, QWidget* pSpotlightTarget = nullptr);
    void spotlight(QWidget* pTarget);
    void applyShellStyle();
    QPalette shellPalette(bool& themeSurface) const;
    // The menu of profiles a map can be copied to. It hangs off the dialog
    // rather than off the shell, so the shell's sheet never reaches it - and it
    // is rebuilt whenever a host is loaded, which may be before or after the
    // style pass, so both call this.
    void styleProfileMapMenu();
    void restyleSidebarIcons(const uiDesign::ThemeTokens& tokens);
    // Search indexes the real widget tree and lends matching cards to the results page while the query stands
    void buildSearchResultsPage();
    void buildSearchIndex();
    void queueSearch(const QString& query);
    void runSearch(const QString& query);
    void invalidateSearch();
    void exitSearchMode();
    void returnSearchedCardsHome();
    void clearSearchHighlights();
    void highlightMatches(QWidget* pCard, const QStringList& needles);
    QLabel* searchCategoryHeader(const QString& key);
    QPushButton* searchSubpageLink(const QString& subpageKey, QWidget* pCard);
    void connectApplyTriggers();
    void applyAll();
    void maybeDownloadEditorThemes();

    QPointer<Host> mpHost;
    // The family the display font list was last set to while the font database had it, so that
    // the list moving off it as it leaves the database is not taken for the user's choice
    QString mDisplayFontFamilyShown;
    QPointer<QTemporaryFile> tempThemesArchive;
    QMap<QString, QString> mSearchEngineMap;
    QPointer<QMenu> mpMenu;
    QPointer<QDialog> mpDialogMapGlyphUsage;
    // The map symbol font the glyph usage table was last built from, so that
    // the other symbol settings changing does not cost a rebuild:
    QFont mGlyphDisplayFont;
    QPointer<QDoubleSpinBox> mpDoubleSpinBox_mapSymbolFontFudge;
    // Kept so that initWithHost(), which runs again for each profile, can replace it rather than stack another
    QMetaObject::Connection mMapStyleConnection;
    QMetaObject::Connection mTimeStampsConnection;
    void followMainConsoleTimeStamps();
    void followMapStyle();
    std::unique_ptr<QTimer> hidePasswordMigrationLabelTimer;
    QMap<QString, QKeySequence> currentShortcuts;
    // Must be declared after currentShortcuts, which it references
    uiDesign::SettingsSnapshot mSnapshot{*this, currentShortcuts};
    QPointer<QCheckBox> mEnableGMCP;
    QPointer<QCheckBox> mEnableMSDP;
    QPointer<QCheckBox> mEnableMSSP;
    QPointer<QCheckBox> mEnableMSP;
    QPointer<QCheckBox> mEnableMXP;
    QPointer<QCheckBox> mEnableMTTS;
    QPointer<QCheckBox> mEnableMNES;
    QPointer<QCheckBox> mEnableNAWS;
    QPointer<QCheckBox> mEnableCHARSET;
    QPointer<QCheckBox> mEnableNEWENVIRON;

    // A card's search text and the layout slot it returns to after a search. Subpage cards are never borrowed.
    struct SearchCard
    {
        QPointer<QWidget> pCard;
        QString categoryKey;
        QString subpageKey;
        QString text;
        QVBoxLayout* pHomeLayout = nullptr;
        int homeIndex = -1;
        bool onResultsPage = false;
    };

    QWidget* mpWidget_shell = nullptr;
    // What applyShellStyle() last drew from, so a StyleChange that alters nothing does not restyle the shell
    QPalette mShellPalette;
    QWidget* mpWidget_sidebar = nullptr;
    QWidget* mpWidget_titleRow = nullptr;
    QLabel* mpLabel_wordmark = nullptr;
    QCheckBox* mpCheckBox_triggerMatchPool = nullptr;
    QLabel* mpLabel_triggerMatchThreads = nullptr;
    QSpinBox* mpSpinBox_triggerMatchThreads = nullptr;
    QLabel* mpLabel_triggerMatchThreshold = nullptr;
    QSpinBox* mpSpinBox_triggerMatchThreshold = nullptr;
    QLabel* mpLabel_triggerMatchMissesPerMatch = nullptr;
    QSpinBox* mpSpinBox_triggerMatchMissesPerMatch = nullptr;
    QLabel* mpLabel_triggerMatchFloodLines = nullptr;
    QSpinBox* mpSpinBox_triggerMatchFloodLines = nullptr;
    QLabel* mpLabel_triggerMatchSpin = nullptr;
    QSpinBox* mpSpinBox_triggerMatchSpin = nullptr;
    QListWidget* mpListWidget_categories = nullptr;
    // The chevron on the seam down the sidebar's trailing edge, which is the
    // one control that sidebar has
    uiDesign::SidebarToggle* mpToggle_sidebar = nullptr;
    // Measuring the names costs a pass over every row, and a resize asks for
    // the answer on every frame of a drag
    mutable int mSidebarWidth = 0;
    mutable bool mSidebarWidthKnown = false;
    // What the user asked the sidebar for, kept across sessions under
    // settingsSidebarLabelsShown. It is a preference about the names alone: the
    // sidebar is there either way, as a list of names or as a rail of icons -
    // and a dialog that has never been told opens with the names, the way the
    // editor does.
    bool mSidebarLabelsShown = true;
    // ...and whether the window is currently wide enough to grant it. This one
    // is the window's answer rather than the user's, so it is never stored - a
    // window dragged narrow shows the rail and gives the names back when it is
    // widened, whatever it was in the middle of
    bool mSidebarNamesFit = true;
    // The sidebar width the last mode decision was taken at, so that names measured anew (a language, font or
    // style change) can be told from a resize
    int mSidebarWidthDecidedAt = 0;
    // The sidebar row that is a link rather than a category
    QListWidgetItem* mpItem_support = nullptr;
    QStackedWidget* mpStackedWidget_categories = nullptr;
    QLineEdit* mpLineEdit_search = nullptr;
    // Kept so applyShellStyle() recolours it rather than adding another on every appearance change
    QPointer<QAction> mpAction_searchIcon;
    QToolButton* mpButton_searchBack = nullptr;
    QToolButton* mpButton_subpageBack = nullptr;
    QLabel* mpLabel_pageTitle = nullptr;
    QLabel* mpLabel_pageTitleIcon = nullptr;
    QFrame* mpFrame_migrationBanner = nullptr;
    QLabel* mpLabel_noProfileNotice = nullptr;
    QScrollArea* mpScrollArea_searchResults = nullptr;
    QVBoxLayout* mpLayout_searchResults = nullptr;
    QLabel* mpLabel_searchEmpty = nullptr;
    // Invalidated whenever the cards' contents change, e.g. a profile bringing its own controls
    QList<SearchCard> mSearchCards;
    QList<QPointer<QWidget>> mHighlightedWidgets;
    QMap<QString, QLabel*> mSearchCategoryHeaders;
    // Links into a subpage whose contents matched but whose opener card did not; kept between searches
    QMap<QString, QPushButton*> mSearchSubpageLinks;
    QPointer<QWidget> mpWidget_spotlight;
    int mSearchResultsPageIndex = -1;
    bool mSearchActive = false;
    QString mCategoryBeforeSearch;
    // So leaving the results by any route returns to the subpage the query was typed on
    QString mSubpageBeforeSearch;
    // "category/sub", empty on a category page
    QString mCurrentSubpage;
    QMap<QString, int> mSubpageIndexes;
    // Breadcrumb names, written by retranslateShell()
    QMap<QString, QString> mSubpageTitles;
    // Subpage scroll areas, so a card's page is found without walking the stack
    QHash<const QWidget*, QString> mSubpageOfPage;
    // The card on the parent category page that opens each subpage
    QMap<QString, QPointer<QWidget>> mSubpageOpeners;
    QPointer<QGroupBox> mpCard_protocolList;
    QPointer<QGroupBox> mpCard_experiments;
    // Keyed by experiment, e.g. "experiment.3dmap.modernmapper"
    QMap<QString, QPointer<QCheckBox>> mExperimentCheckBoxes;
    // App-wide, unlike the profile's experiments beside it on the card
    QPointer<QCheckBox> mpCheckBox_colouredIcons;
    QPointer<QGroupBox> mpCard_discord;
    QPointer<QPushButton> mpButton_discordSubpage;
    QPointer<QGroupBox> mpCard_securityStatus;
#ifdef INCLUDE_MCPSERVER
    QPointer<QGroupBox> mpCard_mcpAssistant;
    QPointer<QPushButton> mpButton_mcpSubpage;
#endif
    QPointer<QLabel> mpLabel_securityHeadline;
    QPointer<QLabel> mpLabel_securityDetail;
    QPointer<QLabel> mpLabel_securityLink;
    // What every anchor on this dialog currently carries, since the colour is
    // written into the text rather than answered by a palette
    QColor mInkedLinkColour;
    // iconFile is kept because the sidebar item gets a recoloured copy, not the file itself
    struct CategoryPlace
    {
        int row = -1;
        int pageIndex = -1;
        QString iconFile;
    };
    QMap<QString, CategoryPlace> mCategories;
    // Each category's icon as rich text for search headers, recoloured by restyleSidebarIcons()
    QMap<QString, QString> mCategoryIconMarkup;
    QTimer* mpTimer_apply = nullptr;
#ifdef INCLUDE_MCPSERVER
    // What this dialog last put on the MCP result line about the server's state - why it
    // failed to start, or that it is not on yet - so a successful start takes back only that
    QString mMCPServerStateMessage;
    void setMCPConnectResult(const QString& message);
    void refreshMCPAppButtons();
    void updateMCPRestartNotice();
    // The AI apps whose settings this dialog changed; each reads them only at startup
    bool mMCPRestartClaudeDesktop = false;
    bool mMCPRestartChatGpt = false;
#endif
    // The scrollback size the box showed when the maximum was ticked, to put back when it is unticked
    int mBufferSizeBeforeMax = 0;
    // Debounces typing so a part-typed query does not move most cards onto the results page and back
    QTimer* mpTimer_search = nullptr;
    QString mPendingSearch;
    // Set once buildShell() has finished moving controls between cards; wrapping one before then is unsafe
    bool mShellReady = false;
    // Which side of the light/dark line the shell's stylesheet was last mixed
    // for, written by applyShellStyle() from the application's tokens. An appearance
    // change reaches this dialog either from its own combo box or from the
    // application's signal, and by the time the signal arrives the mode has
    // already moved - so what says whether there is anything to redraw is the
    // sheet on the shell rather than a reading taken around the call that
    // changed it.
    bool mShellStyledForDarkPage = false;
    // How many times the shell has been restyled, so that a test can hold an
    // appearance change to one pass: the slot is re-entered through that same
    // signal, and a second pass would be invisible from the outside.
    int mShellStyleApplications = 0;
    // Set while initWithHost()/clearHostDetails() write the controls: suppresses instant apply and re-entry
    bool mPopulating = false;
    // Set for all of closeEvent(): QDialog::closeEvent() calls reject(), which would otherwise start the
    // close again, and nothing needs repopulating on the way out
    bool mClosing = false;
    bool mEditorThemesChecked = false;

    QString mLogDirPath;
    // The profile the keychain was already asked about, so re-reading the settings does not ask again
    QString mSignInTokenCheckedFor;
    // Needed to remember the state on construction so that we can sent the same
    // flag back for Host::mUseSharedDictionary even if we turn-off
    // Host::mEnableUserDictionary: - although, following review THAT has been
    // disallowed...
    bool mUseSharedDictionary = false;
};

#endif // MUDLET_DLGPROFILEPREFERENCES_H
