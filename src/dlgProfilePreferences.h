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


#include "mudlet.h"

#include "ui_profile_preferences.h"
#include <QDialog>
#include <QHash>
#include <QList>
#include <QMap>
#include <QVariant>

class Host;
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

    // Save.
    void slot_saveAndClose();

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
    // Named rather than lambdas so that initWithHost() can make every one of
    // its connections with Qt::UniqueConnection - see the comment on that
    // function for why running it twice has to be harmless
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
    // Writes every control a profile decides the value of. Safe to run again on
    // a dialog that is already showing a profile: everything it builds is built
    // once and re-read afterwards, every list it fills is emptied first, and
    // every connection it makes is either Qt::UniqueConnection or inside one of
    // those build-once blocks. refreshFromSettings() depends on all three.
    void initWithHost(Host*);
    // ...and its counterpart for the settings that belong to the application
    // rather than to any profile
    void populateApplicationSettings();
    // Re-reads the settings into a dialog that has been left open, so that a
    // change made from Lua or from another dialog is what the user comes back
    // to. Refuses whenever the dialog is holding an edit of its own, because
    // re-reading would throw that edit away - see pendingEdits().
    void refreshFromSettings();
    // Anything the user has changed that the settings do not know about yet: a
    // control differing from its snapshot, a shortcut editor holding a sequence
    // that has not been committed, a line edit part-way through a word, or an
    // apply still waiting out its debounce
    bool pendingEdits() const;
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
    bool updateDisplayFont();
    void cancelShortcutCaptures();
    void updateShortcutConflictWarning();
    void switchEditorTheme(const QString& themeName);
    static QString findThemeCounterpart(const QString& themeName, const QComboBox* themeComboBox, bool toDark);

    // The sidebar-and-cards shell that replaces the .ui file's tab widget at
    // runtime:
    void buildShell();
    QWidget* buildSidebar();
    void addCategory(const QString& key, const QString& iconFile);
    void addSidebarSeparator();
    // The scrolling column every page is, whether the sidebar leads to it or a
    // card does
    QScrollArea* buildPage(const QString& objectSuffix, const QList<QWidget*>& cards);
    void buildCategoryPage(const QString& key, const QList<QWidget*>& cards);
    // A page reached by drilling into a card rather than from the sidebar: the
    // sidebar stays on the parent category, and a breadcrumb with a back
    // chevron leads back out of it
    void addSubpage(const QString& categoryKey, const QString& subKey, QWidget* pOpenerCard, const QList<QWidget*>& cards);
    void showSubpage(const QString& categoryKey, const QString& subKey, QWidget* pSpotlightTarget = nullptr);
    void leaveSubpage();
    // "Category › Subpage", the widest thing the title row is ever asked to show
    QString breadcrumbFor(const QString& subpageKey) const;
    // Which subpage, if any, a widget lives on - "category/sub", or empty for
    // anything on a category page
    QString subpageHolding(const QWidget* pWidget) const;
    QGroupBox* createCard(const QString& objectName);
    // One muted line under a card's title saying what the card is for, ending
    // in a "Learn more" link where the wiki has a page about it
    void setCardDescription(QGroupBox* pCard, const QString& description, const QString& learnMoreUrl = QString());
    void setCardDescriptions();
    // The ten telnet protocols, as a page of checkboxes reached from the
    // Connection page's protocols card
    void buildProtocolsSubpage();
    void updateProtocolSummary();
    void buildDiscordSummaryCard();
    void updateDiscordSummary();
    // The one status hero: what the current connection's security actually is,
    // rather than what the settings below it ask for
    void buildSecurityStatusCard();
    void updateSecurityStatus();
    // Every string the shell shows that setupUi() did not make, and that
    // retranslateUi() therefore cannot put back on a language change. Called
    // once as the shell is built and again from slot_guiLanguageChanged(), so
    // each of those strings is written in exactly one place.
    void retranslateShell();
    // Synonyms a player might type for a setting whose own words do not
    // include them. Written here rather than in the .ui file so that they can
    // carry a translator note, and re-read on a language change.
    void setSearchKeywords();
    void moveIntoCard(QGroupBox* pCard, const QList<QWidget*>& controls);
    void addCardRow(QGroupBox* pCard, QWidget* pLabel, QWidget* pControl);
    void retitleCards();
    void reflowWideCards();
    // A column narrower than its contents clips them rather than scrolling, so
    // the cap is the reading width or whatever the widest card needs once the
    // checkboxes on it have been fitted to that width
    void capColumnWidth(QScrollArea* pScrollArea);
    // The column caps have to be taken again once a profile has filled the
    // controls, because that is what decides how wide the widest card is - and
    // again after a language change, which is what decides how wide the text is
    void updateColumnWidthCaps();
    // A checkbox draws its label on one line however long it is, so a
    // translation longer than the reading column is what makes a page scroll
    // sideways. Any that do are turned into an indicator with a wrapping label
    // beside it, one at a time and only while it measurably narrows the column,
    // so a language whose text fits keeps native checkboxes throughout.
    void fitCheckBoxesToColumn(QWidget* pColumn);
    void wrapCheckBox(QCheckBox* pCheckBox);
    void unwrapCheckBox(QCheckBox* pCheckBox);
    // Below the width the sidebar needs to stand beside a full reading column,
    // it collapses to a rail of icons. Driven by the window's size alone -
    // there is no preference to get out of step with it.
    int widthNeededForFullSidebar() const;
    void updateSidebarMode();
    void setSidebarCollapsed(bool collapsed);
    void rebuildTabOrder();
    void guardScrollWheel();
    void buildMigrationBanner();
    // The banner belongs to no one category: it is lent to the top of whichever
    // page is showing, and taken off every page while the search has the stack
    void placeBannerOn(QWidget* pColumn);
    void showCategory(const QString& key, QWidget* pSpotlightTarget = nullptr);
    void spotlight(QWidget* pTarget);
    void applyShellStyle();
    // "Find in settings" - an index over the real widget tree, and a results
    // page the matching cards are lent to for as long as the query stands:
    void buildSearchResultsPage();
    void buildSearchIndex();
    void runSearch(const QString& query);
    void invalidateSearch();
    void exitSearchMode();
    void returnSearchedCardsHome();
    void clearSearchHighlights();
    void highlightMatches(QWidget* pCard, const QStringList& needles);
    QLabel* searchCategoryHeader(const QString& key);
    QPushButton* searchSubpageLink(const QString& subpageKey, QWidget* pCard);
    void connectApplyTriggers();
    void snapshotValues();
    bool dirty(const QObject* pControl) const;
    bool anyDirty(const QList<const QObject*>& controls) const;
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
    // The editor showing each of those, so that a second profile re-reads the
    // editors the first one left behind rather than adding a second row of them
    QMap<QString, QPointer<TKeySequenceEdit>> mShortcutEditors;
    // The ten telnet protocols, on the Connection page's protocols subpage
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

    // One card of one page: everything it can be found by, and exactly where it
    // goes back to once the search that borrowed it ends. A card on a subpage
    // is never borrowed - the results offer the way into its subpage instead.
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
    // The sidebar and the row the page title sits in: both are measured when
    // the window is resized, to decide whether the sidebar still fits
    QWidget* mpWidget_sidebar = nullptr;
    QWidget* mpWidget_titleRow = nullptr;
    QLabel* mpLabel_wordmark = nullptr;
    QListWidget* mpListWidget_categories = nullptr;
    // The one sidebar row that is a link rather than a category
    QListWidgetItem* mpItem_support = nullptr;
    QStackedWidget* mpStackedWidget_categories = nullptr;
    QLineEdit* mpLineEdit_search = nullptr;
    // The search field's leading glyph, recoloured for the theme in
    // applyShellStyle() rather than added again on every appearance change
    QPointer<QAction> mpAction_searchIcon;
    // Leads out of the search results, back to the category they interrupted
    QToolButton* mpButton_searchBack = nullptr;
    // ...and its counterpart on a subpage, leading up to the parent category
    QToolButton* mpButton_subpageBack = nullptr;
    QLabel* mpLabel_pageTitle = nullptr;
    QLabel* mpLabel_pageTitleIcon = nullptr;
    QFrame* mpFrame_migrationBanner = nullptr;
    QScrollArea* mpScrollArea_searchResults = nullptr;
    QVBoxLayout* mpLayout_searchResults = nullptr;
    QLabel* mpLabel_searchEmpty = nullptr;
    // Thrown away whenever the cards' contents change under it - a profile
    // appearing brings controls of its own
    QList<SearchCard> mSearchCards;
    QList<QPointer<QWidget>> mHighlightedWidgets;
    QMap<QString, QLabel*> mSearchCategoryHeaders;
    // The way into a subpage whose contents matched but whose opener card did
    // not, one per subpage, kept between searches like the headers are
    QMap<QString, QPushButton*> mSearchSubpageLinks;
    QPointer<QWidget> mpWidget_spotlight;
    int mSearchResultsPageIndex = -1;
    bool mSearchActive = false;
    QString mCategoryBeforeSearch;
    // Which subpage the search interrupted, so that leaving the results by any
    // door comes back to the page the query was typed on
    QString mSubpageBeforeSearch;
    // "category/sub" while a subpage is showing, empty on a category page
    QString mCurrentSubpage;
    QMap<QString, int> mSubpageIndexes;
    // What the breadcrumb calls each subpage - written by retranslateShell()
    QMap<QString, QString> mSubpageTitles;
    // The scroll area of each subpage, so that a card can be asked which page
    // it is on without walking the stack
    QHash<const QWidget*, QString> mSubpageOfPage;
    // The card on the parent category page that drills into each subpage
    QMap<QString, QPointer<QWidget>> mSubpageOpeners;
    QPointer<QGroupBox> mpCard_protocolList;
    QPointer<QGroupBox> mpCard_discord;
    QPointer<QPushButton> mpButton_discordSubpage;
    QPointer<QGroupBox> mpCard_securityStatus;
    QPointer<QLabel> mpLabel_securityHeadline;
    QPointer<QLabel> mpLabel_securityDetail;
    QPointer<QLabel> mpLabel_securityLink;
    QMap<QString, int> mCategoryPageIndexes;
    QMap<QString, int> mCategoryRows;
    // The sidebar item holds the icon, but a rich-text header needs the
    // resource path it was loaded from
    QMap<QString, QString> mCategoryIconFiles;
    QTimer* mpTimer_apply = nullptr;
    // What every apply-relevant control held the last time the dialog read the
    // settings, keyed by the control
    QHash<const QObject*, QVariant> mValueSnapshot;
    // The shortcut editors write through this map rather than through a control
    // value, so it needs a snapshot of its own
    QMap<QString, QKeySequence> mShortcutsSnapshot;
    // The sidebar is a rail of icons rather than a list of names
    bool mSidebarCollapsed = false;
    // Set once buildShell() has finished moving controls between cards, which
    // is when it becomes safe to wrap one that does not fit - see
    // fitCheckBoxesToColumn()
    bool mShellReady = false;
    // Suppresses the instant apply while initWithHost()/clearHostDetails() are
    // writing the controls rather than the user - and, since it is raised for
    // the whole of a repopulation, what makes re-entering one impossible
    bool mPopulating = false;
    // Raised for the whole of closeEvent(): QDialog::closeEvent() calls
    // reject(), which is where the close this is already inside of would
    // otherwise start again, and nothing on the way out is worth repopulating
    bool mClosing = false;
    bool mEditorThemesChecked = false;

    QString mLogDirPath;
    // Which profile the keychain has already been asked about for the "forget
    // saved sign-in" button, so that re-reading the settings does not ask again
    QString mSignInTokenCheckedFor;
    // Needed to remember the state on construction so that we can sent the same
    // flag back for Host::mUseSharedDictionary even if we turn-off
    // Host::mEnableUserDictionary: - although, following review THAT has been
    // disallowed...
    bool mUseSharedDictionary = false;
};

#endif // MUDLET_DLGPROFILEPREFERENCES_H
