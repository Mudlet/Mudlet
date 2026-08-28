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
class QFrame;
class QListWidget;
class QListWidgetItem;
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


signals:
    void signal_themeUpdateCompleted();
    void signal_preferencesSaved();
    void signal_resetMainWindowShortcutsToDefaults();
    void preferencesClosing(const QString& profileName);

protected:
    void closeEvent(QCloseEvent* event) override;
    bool eventFilter(QObject* pObject, QEvent* pEvent) override;

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
    void initWithHost(Host*);
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
    void buildCategoryPage(const QString& key, const QList<QWidget*>& cards);
    QGroupBox* createCard(const QString& objectName);
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
    // The column caps have to be taken again once a profile has filled the
    // controls, because that is what decides how wide the widest card is
    void updateColumnWidthCaps();
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
    QPointer<QMenu> protocolMenu;
    QPointer<QAction> mEnableGMCP;
    QPointer<QAction> mEnableMSDP;
    QPointer<QAction> mEnableMSSP;
    QPointer<QAction> mEnableMSP;
    QPointer<QAction> mEnableMXP;
    QPointer<QAction> mEnableMTTS;
    QPointer<QAction> mEnableMNES;
    QPointer<QAction> mEnableNAWS;
    QPointer<QAction> mEnableCHARSET;
    QPointer<QAction> mEnableNEWENVIRON;

    // One card of one category page: everything it can be found by, and exactly
    // where it goes back to once the search that borrowed it ends
    struct SearchCard
    {
        QPointer<QWidget> pCard;
        QString categoryKey;
        QString text;
        QVBoxLayout* pHomeLayout = nullptr;
        int homeIndex = -1;
        bool onResultsPage = false;
    };

    QWidget* mpWidget_shell = nullptr;
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
    QPointer<QWidget> mpWidget_spotlight;
    int mSearchResultsPageIndex = -1;
    bool mSearchActive = false;
    QString mCategoryBeforeSearch;
    QMap<QString, int> mCategoryPageIndexes;
    QMap<QString, int> mCategoryRows;
    // The sidebar item holds the icon, but a rich-text header needs the
    // resource path it was loaded from
    QMap<QString, QString> mCategoryIconFiles;
    QTimer* mpTimer_apply = nullptr;
    // What every apply-relevant control held the last time the dialog read the
    // settings, keyed by the control (the protocol menu's actions included)
    QHash<const QObject*, QVariant> mValueSnapshot;
    // The shortcut editors write through this map rather than through a control
    // value, so it needs a snapshot of its own
    QMap<QString, QKeySequence> mShortcutsSnapshot;
    // Suppresses the instant apply while initWithHost()/clearHostDetails() are
    // writing the controls rather than the user
    bool mPopulating = false;
    // QDialog::closeEvent() calls reject(), which is where the close this is
    // already inside of would otherwise start again
    bool mClosing = false;
    bool mEditorThemesChecked = false;

    QString mLogDirPath;
    // Needed to remember the state on construction so that we can sent the same
    // flag back for Host::mUseSharedDictionary even if we turn-off
    // Host::mEnableUserDictionary: - although, following review THAT has been
    // disallowed...
    bool mUseSharedDictionary = false;
};

#endif // MUDLET_DLGPROFILEPREFERENCES_H
