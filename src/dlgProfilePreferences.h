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
#include <QDialog>
#include <QHash>
#include <QList>
#include <QMap>
#include <QVariant>

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


// Control values as last read from the settings, so an apply writes back only what the user changed.
// Both references are to members of the owning dialog, so they outlive this. Keys may dangle: they are
// only compared, never dereferenced, and a control created after the last snapshot reads as dirty.
class SettingsSnapshot
{
public:
    Q_DISABLE_COPY(SettingsSnapshot)
    SettingsSnapshot(const QWidget& owner, const QMap<QString, QKeySequence>& shortcuts);
    // Whether any setting is written from this widget's value
    static bool carriesValue(const QObject* pControl);
    // Call once the controls hold the settings' values; later differences count as user edits
    void take();
    // For one control whose list was rebuilt while the dialog is showing
    void take(const QObject* pControl);
    bool dirty(const QObject* pControl) const;
    bool anyDirty(const QList<const QObject*>& controls) const;
    bool shortcutsDirty() const;
    bool shortcutDirty(const QString& key) const;
    // Edits not yet in the settings: a dirty control, an uncommitted shortcut, a part-typed line edit,
    // or an apply still waiting out its debounce
    bool pendingEdits(const QTimer* pApplyTimer, const QLineEdit* pSearchField) const;
    // Lets a second profile reuse the first one's editors rather than adding another row
    TKeySequenceEdit* editorFor(const QString& key) const;
    void addEditor(const QString& key, TKeySequenceEdit* pEditor);

private:
    const QWidget& mOwner;
    const QMap<QString, QKeySequence>& mCurrentShortcuts;
    QHash<const QObject*, QVariant> mValues;
    QMap<QString, QKeySequence> mShortcuts;
    QMap<QString, QPointer<TKeySequenceEdit>> mEditors;
};

class dlgProfilePreferences : public QDialog, public Ui::profile_preferences
{
    Q_OBJECT

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
    void slot_displayFontSizeChanged();
    void slot_displayFontAliasingChanged();
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

private:
    void setColors();
    void setColors2();
    void setButtonAndProfileColor(QPushButton*, QColor&, bool allowAlpha = false);
    void setPlayerRoomColor(QPushButton*, QColor&);
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
    void cancelShortcutCaptures();
    void updateShortcutConflictWarning();
    void switchEditorTheme(const QString& themeName);
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
    // A muted line under the card's title, ending in a "Learn more" link if learnMoreUrl is given
    void setCardDescription(QGroupBox* pCard, const QString& description, const QString& learnMoreUrl = QString());
    void setCardDescriptions();
    void buildProtocolsSubpage();
    void updateProtocolSummary();
    void buildDiscordSummaryCard();
    void updateDiscordSummary();
    // Shows the current connection's actual security, not what the settings ask for
    void buildSecurityStatusCard();
    void updateSecurityStatus();
    // Strings setupUi() did not create, which retranslateUi() therefore cannot restore on a language change
    void retranslateShell();
    // Search synonyms not in a setting's own words; here rather than in the .ui file so they can carry
    // a translator note.
    void setSearchKeywords();
    void moveIntoCard(QGroupBox* pCard, const QList<QWidget*>& controls);
    void addCardRow(QGroupBox* pCard, QWidget* pLabel, QWidget* pControl);
    void retitleCards();
    void reflowWideCards();
    void reflowDisplayOptionsCard();
    void reflowCompatibilityCard();
    void capColumnWidth(QScrollArea* pScrollArea);
    // Rerun once a profile has filled the controls, which decide the widest card, and after a language change
    void updateColumnWidthCaps();
    // A checkbox never wraps its text, so a long translation makes the page scroll sideways. Offenders
    // get a wrapping label beside them, one at a time and only while that measurably narrows the column.
    void fitCheckBoxesToColumn(QWidget* pColumn);
    void wrapCheckBox(QCheckBox* pCheckBox);
    void unwrapCheckBox(QCheckBox* pCheckBox);
    // Window widths: below collapseBelow the sidebar becomes an icon rail; past fullyExpanded the dialog
    // gains only empty space
    struct SidebarWidths
    {
        int collapseBelow = 0;
        int fullyExpanded = 0;
    };
    SidebarWidths sidebarWidths() const;
    void updateSidebarMode();
    void setSidebarCollapsed(bool collapsed);
    void rebuildTabOrder();
    void guardScrollWheel();
    void buildMigrationBanner();
    void placeBannerOn(QWidget* pColumn);
    void showCategory(const QString& key, QWidget* pSpotlightTarget = nullptr);
    void spotlight(QWidget* pTarget);
    void applyShellStyle();
    void restyleSidebarIcons(const QColor& normal, const QColor& selected);
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
    QPointer<QTemporaryFile> tempThemesArchive;
    QMap<QString, QString> mSearchEngineMap;
    QPointer<QMenu> mpMenu;
    QPointer<QDialog> mpDialogMapGlyphUsage;
    // The map symbol font the glyph usage table was last built from, so that
    // the other symbol settings changing does not cost a rebuild:
    QFont mGlyphDisplayFont;
    QPointer<QDoubleSpinBox> mpDoubleSpinBox_mapSymbolFontFudge;
    std::unique_ptr<QTimer> hidePasswordMigrationLabelTimer;
    QMap<QString, QKeySequence> currentShortcuts;
    // Must be declared after currentShortcuts, which it references
    SettingsSnapshot mSnapshot{*this, currentShortcuts};
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
    QWidget* mpWidget_sidebar = nullptr;
    QWidget* mpWidget_titleRow = nullptr;
    QLabel* mpLabel_wordmark = nullptr;
    QListWidget* mpListWidget_categories = nullptr;
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
    QPointer<QGroupBox> mpCard_discord;
    QPointer<QPushButton> mpButton_discordSubpage;
    QPointer<QGroupBox> mpCard_securityStatus;
    QPointer<QLabel> mpLabel_securityHeadline;
    QPointer<QLabel> mpLabel_securityDetail;
    QPointer<QLabel> mpLabel_securityLink;
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
    // Debounces typing so a part-typed query does not move most cards onto the results page and back
    QTimer* mpTimer_search = nullptr;
    QString mPendingSearch;
    // An icon rail rather than a list of names
    bool mSidebarCollapsed = false;
    // Set once buildShell() has finished moving controls between cards; wrapping one before then is unsafe
    bool mShellReady = false;
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
