#ifndef MUDLET_MUDLET_H
#define MUDLET_MUDLET_H

/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016 by Chris Leacy - cleacy1972@gmail.com              *
 *   Copyright (C) 2015-2016, 2018-2019, 2021-2024 by Stephen Lyons        *
 *                                               - slysven@virginmedia.com *
 *   Copyright (C) 2016-2018 by Ian Adkins - ieadkins@gmail.com            *
 *   Copyright (C) 2022 by Thiago Jung Bauermann - bauermann@kolabnow.com  *
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

#include "discord.h"
#include "FontManager.h"
#include "HostManager.h"
#include "ShortcutsManager.h"
#include "utils.h"
#include <memory>

#if defined(INCLUDE_UPDATER)
#include "updater.h"
#endif

#include "ui_main_window.h"
#include <QElapsedTimer>
#include <QHash>
#include <QKeySequence>
#include <QMainWindow>
#include <QMap>
#include <QPointer>
#include <QSystemTrayIcon>
#include <QTextOption>
#include <QTime>
#include <QVersionNumber>

#if defined(INCLUDE_OWN_QT6_KEYCHAIN)
#include <qtkeychain/keychain.h>
#else
#include <qt6keychain/keychain.h>
#endif
#include <array>
#include <optional>
#include <hunspell/hunspell.hxx>
#include <hunspell/hunspell.h>

class QAction;
class QCloseEvent;
class QDateTime;
class QDir;
class QKeyEvent;
class QMediaDevices;
class QMediaPlayer;
class QMenu;
class QLabel;
class QListWidget;
class QPushButton;
class QSettings;
class QShortcut;
class QSplitter;
class QTableWidget;
class QTableWidgetItem;
class QTextEdit;
class QToolButton;
class QTimer;

class dlgAboutDialog;
class dlgConnectionProfiles;
class FileOpenHandler;
class dlgIRC;
class dlgNotepad;
class dlgPackageManager;
class dlgModuleManager;
class dlgPackageExporter;
class dlgProfilePreferences;
class dlgTriggerEditor;
class Host;
class MudletInstanceCoordinator;
class ShortcutManager;
class SpeechRecognizer;
class TConsole;
class TDebugFilterBar;
class TDetachedWindow;
class TDockWidget;
class TEvent;
class TLabel;
class translation;
class TScrollBox;
class TTabBar;
class TTimer;
class TToolBar;
class TUiTour;

class mudlet : public QMainWindow, public Ui::main_window
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(mudlet)
    mudlet();
    ~mudlet() override;

    static QString getMudletPath(enums::mudletPathType, const QString& extra1 = QString(), const QString& extra2 = QString());
    static QSettings* getQSettings();
    // From https://stackoverflow.com/a/14678964/4805858 an answer to:
    // "How to find and replace string?" by "Czarek Tomczak":
    static bool loadEdbeeTheme(const QString& themeName, const QString& themeFile);
    static bool loadLuaFunctionList();
    static std::string replaceString(std::string subject, const std::string& search, const std::string& replace);
    static mudlet* self();
    static void setNetworkRequestDefaults(const QUrl& url, QNetworkRequest& request);
    // This method allows better debugging when mudlet::self() is called inappropriately.
    static void start();
    static bool unzip(const QString& archivePath, const QString& destination, const QDir& tmpDir);
    static QImage getSplashScreen(bool releaseVersion, bool testVersion);


    QString mAppBuild;
    // final, official release
    bool releaseVersion;
    // unofficial "nightly" build - still a type of a release
    bool publicTestVersion;
    // used by developers in everyday coding:
    bool developmentVersion;
    // "scmMudletXmlDefaultVersion" number represents a major (integer part) and minor
    // (1000ths, range 0 to 999) that is used as a "version" attribute number when
    // writing the <MudletPackage ...> element of all (but maps if I ever get around
    // to doing a Map Xml file exporter/writer) Xml files used to export/save Mudlet
    // button/menu/toolbars; aliases. keys, scripts, timers, triggers and variables
    // and collections of these as modules/packages and entire profiles as "game
    // saves".  Mudlet versions up to 3.0.1 never bothered checking the version
    // detail and it had been hard coded as "1.0" back as far as history can
    // determine.  From that version a check was coded to test that the version
    // was less than 2.000f with the intention to loudly and clearly fail if a
    // higher version was encountered. Values above 1.001f have not yet been
    // codified but should be accepted so it should be possible to raise the number
    // a little and to use that to extend the Xml data format in a manner that older
    // versions ignore (possibly with some noise) but which they can still get the
    // details they can handle yet allow a later upgraded version to get extra
    // information they want.
    //
    // Taking this number to 2.000f or more WILL prevent old versions from reading
    // Xml files and should be considered a step associated with a major version
    // number change in the Mudlet application itself and SHOULD NOT BE DONE WITHOUT
    // agreement and consideration from the Project management, even a minor part
    // increment should not be done without justification...!
    // XML version Change history (what and why):
    // 1.001    Added method to allow XML format to permit ASCII control codes
    //          0x01-0x08, 0x0b, 0x0c, 0x0e-0x1f, 0x7f to be stored as part of the
    //          "script" element for a Mudlet "item" (0x09, 0x0a, 0x0d are the only
    //          ones that ARE permitted) - this is wanted so that, for instance
    //          ANSI ESC codes can be included in a Lua script without breaking
    //          the XML format used to store it - prior to this embedding such
    //          codes would break or destroy the script that used it.
    inline static const QString scmMudletXmlDefaultVersion = QString::number(1.001f, 'f', 3);
    // A constant equivalent to QDataStream::Qt_5_12 needed in several places
    // which can't be pulled from Qt as it is not going to be defined for older
    // versions:
    static const int scmQDataStreamFormat_5_12 = 18;
    // The Qt runtime version is needed in various places but as it is a constant
    // during the application run it is easiest to define it as one once:
    inline static const QVersionNumber scmRunTimeQtVersion = QVersionNumber::fromString(QLatin1String(qVersion()));
    // translations done high enough will get a gold star to hide the last few percent
    // as well as encourage translators to maintain it
    static const int scmTranslationGoldStar = 95;
    QString scmVersion;
    QString confPath;
    // These have to be "inline" to satisfy the ODR (One Definition Rule):
    inline static bool smDebugMode = false;
    inline static bool smFirstLaunch = false;
    inline static QVariantHash smLuaFunctionNames;
    inline static QPointer<TConsole> smpDebugConsole;
    inline static QPointer<QMainWindow> smpDebugArea;
    inline static QPointer<TDebugFilterBar> smpDebugFilterBar;
    // mirror everything shown in any console to stdout. Helpful for CI environments
    inline static bool smMirrorToStdOut = false;
    // adjust Mudlet settings to match Steam's requirements
    inline static bool smSteamMode = false;
    // This may need to be localised, it represents the format of the timestamp
    inline static QString smTimeStampFormat = qsl("hh:mm:ss.zzz ");
    // If localised this should be set to the same format and length as the
    // smTimeStampFormat:
    inline static QString smBlankTimeStamp = qsl("------------ ");


    void showEvent(QShowEvent*) override;
    void hideEvent(QHideEvent*) override;


    void init();
    void setupConfig();
    void activateProfile(Host*);
    void switchToProfileTab(int index);
    bool profileSwitchShortcutMatches(const QKeyEvent*) const;
    void takeOwnershipOfInstanceCoordinator(std::unique_ptr<MudletInstanceCoordinator>);
    MudletInstanceCoordinator* getInstanceCoordinator();
    void addConsoleForNewHost(Host*);
    QPair<bool, bool> addWordToSet(const QString&);
    void adjustMenuBarVisibility();
    void adjustToolBarVisibility();
    void announce(const QString& text, const QString& processing = QString(), bool isPlain = false);
    void attachDebugArea(const QString&);
    void checkUpdatesOnStart();
    void commitLayoutUpdates(bool flush = false);
    bool saveFloatingDockGeometries();
    void restoreFloatingDockGeometries();
    void deleteProfileData(const QString& profile, const QString& item);
    void disableToolbarButtons();
    void doAutoLogin(const QString&, bool offline);
    void enableToolbarButtons();
    void updateMainWindowToolbarState();
    void updateMapActionAvailability();
    void updateMainWindowTitle();
    void forceClose();
    void armForceClose();
    Host* getActiveHost();
    QStringList getAvailableFonts();
    QList<QString> getAvailableTranslationCodes() const { return mTranslationsMap.keys(); }
    const QMap<QByteArray, QString>& getEncodingNamesMap() const { return mEncodingNameMap; }
    HostManager& getHostManager() { return mHostManager; }
    ShortcutsManager* shortcutsManager() const { return mpShortcutsManager.data(); }
    // Speech-to-text bridge: creates the single shared recognizer on first use
    // and exposes it to the Lua stt.* API. Recognizer results surface as Lua
    // events; all routing and UI policy lives in packages consuming them.
    void initSpeechRecognition();
    SpeechRecognizer* speechRecognizer() const;
    // Raise one sysSTT* event on the active profile. Public because the stt.*
    // bindings refuse before a recognizer exists - with no engine installed
    // there is no object to emit through, and "refusals speak" has to hold
    // there too or a consumer cannot tell "no engine" from "nothing said yet".
    void raiseSpeechEvent(const QString& name, const QString& value);
    const QMap<QString, QPointer<TDetachedWindow>>& getDetachedWindows() const { return mDetachedWindows; }
    QDockWidget* getMainWindowDockWidget(const QString& mapKey) const { return mMainWindowDockWidgetMap.value(mapKey); }
    std::optional<QSize> getImageSize(const QString&);
    const QString& getInterfaceLanguage() const { return mInterfaceLanguage; }
    int64_t getPhysicalMemoryTotal();
    const QLocale& getUserLocale() const { return mUserLocale; }
    QSet<QString> getWordSet();
    bool inDarkMode() const { return mDarkMode; }
    // Used to enable "emergency" control recovery action - if Mudlet is
    // operating without either menubar or main toolbar showing.
    bool isControlsVisible() const;
    bool isGoingDown() { return mIsGoingDown; }
    bool closeHeldOffByEventPump(Host*) const;
    Host* loadProfile(const QString&, const bool, const QString& saveFileName = QString());
    bool loadReplay(Host*, const QString&, QString* pErrMsg = nullptr);
    bool loadWindowLayout();
    enums::controlsVisibility menuBarVisibility() const { return mMenuBarVisibility; }
    bool canHideToolBar() const { return mMenuBarVisibility != enums::visibleNever; }
    bool migratePasswordsToProfileStorage();
    bool migratePasswordsToSecureStorage();
    // Helper function to check if current version is >= specified version for backward compatibility
    bool isVersionAtLeast(const QString& minVersion);
    void onlyShowProfiles(const QStringList&);
    bool openWebPage(const QString&);

    // Profile validation and orphan detection
    bool hasOrphanedProfiles();
    QStringList getOrphanedProfiles();
    void reattachOrphanedProfiles();
    // Both of these revises the contents of the .aff file and handle a .dic
    // file that has been updated externally/manually (to add or remove words)
    // - the first also puts the contents of the .dic file into the
    // supplied second argument before returning the handle to the dictionary
    // loaded:
    Hunhandle* prepareProfileDictionary(const QString&, QSet<QString>&);
    Hunhandle* prepareSharedDictionary();
    void processEventLoopHack();
    void readEarlySettings(const QSettings&);
    void readLateSettings(const QSettings&);
    QPair<bool, bool> removeWordFromSet(const QString&);
    QString readProfileData(const QString& profile, const QString& item);
    void refreshTabBar();
    void refreshTabBarsAfterStyleChange();
    // Used by a profile to tell the mudlet class
    // to tell other profiles to reload the updated
    // maps (via signal_profileMapReloadRequested(...))
    void requestProfilesToReloadMaps(QList<QString>);
    void replayOver();
    bool replayStart();
    std::pair<bool, QString> resetProfileIcon(const QString&);
#if defined(Q_OS_WINDOWS)
    void sanitizeUtf8Path(QString& originalLocation, const QString& fileName) const;
#endif
    // This will save and replace the .dic file with just the words in the
    // supplied second argument and update the .aff file as appropriate. It is
    // to be used at the end of a session to store away the user's changes:
    bool saveDictionary(const QString&, QSet<QString>&);
    bool saveWindowLayout();
    void scanForMudletTranslations(const QString&);
    void scanForQtTranslations(const QString&);
    void setAppearance(enums::Appearance, const bool& loading = false);
    bool setClickthrough(Host*, const QString&, bool);
    void setEditorTextoptions(bool isTabsAndSpacesToBeShown, bool isLinesAndParagraphsToBeShown);
    void setEditorTreeWidgetIconSize(int);
    void setGlobalStyleSheet(const QString&);
    void setInterfaceLanguage(const QString&);
    void setMenuBarVisibility(enums::controlsVisibility);
    std::pair<bool, QString> setProfileIcon(const QString& profile, const QString& newIconPath);
    void setShowIconsOnMenu(const Qt::CheckState);
    void setShowMapAuditErrors(const bool);
    void setInvertMapZoom(const bool);
    void setShowTabConnectionIndicators(const bool);
    void setupPreInstallPackages(const QString&, const QString&);
    void setToolBarIconSize(int);
    void setToolBarVisibility(enums::controlsVisibility);
    void showChangelogIfUpdated();
    void slot_showConnectionDialog();
    bool showMapAuditErrors() const { return mShowMapAuditErrors; }
    bool invertMapZoom() const { return mInvertMapZoom; }
    bool showTabConnectionIndicators() const { return mShowTabConnectionIndicators; }
    // Addon toolbar button management
    // Surfaces a command can be placed on. A client with different chrome maps
    // these onto whatever it has; one that has only a menu honours Menu alone.
    enum class CommandSurface { Menu, Toolbar, Both };

    struct CommandRequest
    {
        QString name;
        QString icon;
        QString tooltip;
        QString menuPath;
        QString shortcut;
        CommandSurface surfaces = CommandSurface::Both;
    };

    // Why a command could not be placed, so the binding can say which
    int addAddonCommand(const CommandRequest& request, Host* pHost, QString& error);
    bool removeAddonCommand(int commandId, Host* pHost);
    bool setAddonCommandEnabled(int commandId, bool enabled, Host* pHost);
    bool setAddonCommandChecked(int commandId, bool checked, Host* pHost);
    bool setAddonCommandIcon(int commandId, const QString& icon, Host* pHost);
    bool setAddonCommandTooltip(int commandId, const QString& tooltip, Host* pHost);
    bool setAddonCommandPulse(int commandId, bool enabled, const QString& color1, const QString& color2, int interval, Host* pHost, QString& error);
    // Every command a profile placed, dropped when it closes or resets
    void removeAddonCommandsForHost(Host* pHost);
    // Which add-on commands hold this key, named as the player reads them.
    // The clash check only runs when a package asks for a key, and Mudlet's
    // own bindings can appear afterwards - the buffer search is switched on
    // long after a package has taken F3 - at which point Qt disables both.
    // A command belonging to another profile is reported without its name:
    // that is the other package's business and nothing this profile can act
    // on, the same rule addonShortcutUsable() follows.
    QStringList addonCommandsUsingShortcut(const QKeySequence& sequence, const Host* pHost) const;
    void applyToolBarStyleToAddonCommands();

    // Brings up the preferences dialog and selects the tab whos objectName is
    // supplied, for the given Host - or the active one if none is given:
    void showOptionsDialog(const QString&, Host* = nullptr);
    void startAutoLogin(const QStringList&, bool offline = false);
    bool storingPasswordsSecurely() const { return mStorePasswordsSecurely; }
    void setStorePasswordsSecurely(const bool storeSecurely) { mStorePasswordsSecurely = storeSecurely; }
    enums::controlsVisibility toolBarVisibility() const { return mToolbarVisibility; }
    void updateDiscordNamedIcon();
    void updateMultiViewControls();
    QPair<bool, QString> writeProfileData(const QString& profile, const QString& item, const QString& what);
    void writeSettings();
    bool muteAPI() const { return mMuteAPI; }
    bool muteGame() const { return mMuteGame; }
    bool mediaMuted() const { return mMuteAPI && mMuteGame; }
    bool mediaUnmuted() const { return !mMuteAPI && !mMuteGame; }
    bool profileExists(const QString& profileName);
    QString getCanonicalProfileName(const QString& profileName);
    bool showSplitscreenTutorial();
    void showedSplitscreenTutorial();
    bool showMuteAllMediaTutorial();
    void showedMuteAllMediaTutorial();
    bool showCharacterModeWarning();
    void showedCharacterModeWarning();
    // True if the player has used Mudlet long enough not to need the tutorial
    // tips, the interface tour or the starter UI. Memoised.
    bool experiencedMudletPlayer();
    // The two below are public only so they can be tested
    static void rememberFirstLaunch(QSettings& settings, const QString& profilesPath, const QDateTime& now);
    static bool evaluateExperiencedPlayer(const QSettings& settings, const QString& profilesPath, const QDateTime& now);

    // Telnet URI handling
    void handleTelnetUri(const QString& uri);

    enums::Appearance mAppearance = enums::Appearance::systemSetting;
    // 1 (of 2) needed to work around a (Windows/MacOs specific QStyleFactory)
    // issue:
    QString mBG_ONLY_STYLESHEET;
    // approximate max duration that 'Copy as image' is allowed to take
    // (seconds):
    int mCopyAsImageTimeout = 3;

    // A list of potential dictionary languages - probably will cover a much
    // wider range of languages compared to the translations - and is intended
    // for Dictionary identification - there is a request for users to submit
    // entries in their system if they do not appear in this and thus get
    // reported in the dictionary selection as the hunspell dictionary/affix
    // filename (e.g. a "xx" or "xx_YY" form rather than "words"):
    QHash<QString, QString> mDictionaryLanguageCodeMap;
    Discord mDiscord;
    // Used for editor area, but
    // only ::ShowTabsAndSpaces
    // and ::ShowLineAndParagraphSeparators
    // are considered/used/stored
    QTextOption::Flags mEditorTextOptions = QTextOption::Flags();
    int mEditorTreeWidgetIconSize = 0;
    FontManager mFontManager;
    bool mHasSavedLayout = false;
    bool mIsLoadingLayout = false;
    QStringList mOnlyShownPredefinedProfiles;
    QPointer<dlgAboutDialog> mpAboutDlg;
    QStringList mPackagesToInstallList;
    // Test-only: PipelineBenchmark sets this so its profile measures the
    // pipeline rather than the shipped default packages.
    bool mSkipDefaultPackageInstall = false;
    QPointer<dlgConnectionProfiles> mpConnectionDialog;
    QPointer<Host> mpCurrentActiveHost;
    // Options dialog when there's no active host
    QPointer<dlgProfilePreferences> mpDlgProfilePreferences;
    // Flag to prevent connection dialog from opening during telnet:// URI processing
    bool mProcessingTelnetUri = false;
    QToolBar* mpMainToolBar = nullptr;
    QPointer<QSettings> mpSettings;
    QPointer<ShortcutsManager> mpShortcutsManager;
    TTabBar* mpTabBar = nullptr;
    int mReplaySpeed = 1;
    // More modern Desktop styles no longer include icons on the buttons in
    // QDialogButtonBox buttons - but some users are using Desktops (KDE4?) that
    // does use them - use this flag to determine whether we should apply our
    // icons to override some of them:
    QTime mReplayTime;
    bool mShowIconsOnDialogs = true;
    // This is the state for the tri-state control on the preferences and
    // means:
    // Qt::PartiallyChecked = use the previous state set on application start
    //    (set AA_DontShowIconsInMenus to inverse of mShowIconsOnMenuOriginally)
    // Qt::Unchecked = icons are not used on menus (set AA_DontShowIconsInMenus
    //    to false ourselves)
    // Qt::Checked = icons are used on menus (set AA_DontShowIconsInMenus to
    //    true ourselves)
    Qt::CheckState mShowIconsOnMenuCheckedState = Qt::PartiallyChecked;
    // Value of QCoreApplication::testAttribute(Qt::AA_DontShowIconsInMenus) on
    // startup which the user may leave as is or force on or off:
    bool mShowIconsOnMenuOriginally = true;
    // Whether Mudlet was the active application at the last state change, so
    // sysApplicationFocusChangeEvent is raised on a change of that and not on
    // every transition Qt reports between its inactive states
    bool mApplicationActive = true;
    // 2 (of 2) needed to work around a (Windows/MacOs specific QStyleFactory)
    // issue:
    QString mTEXT_ON_BG_STYLESHEET;
    int mToolbarIconSize = 0;
    QMap<QString, translation> mTranslationsMap;
    // This is used to keep track of where the main dictionary files are located
    // will be true if they are ones bundled with Mudlet, false if provided by
    // the system
    QSystemTrayIcon mTrayIcon;
    bool mUsingMudletDictionaries = false;
    bool mWindowMinimized = false;
    std::unique_ptr<MudletInstanceCoordinator> mInstanceCoordinator;
    // How many graphemes do we need before we run the spell checker on a "word" in the command line:
    int mMinLengthForSpellCheck = 3;
    bool mDrawUpperLowerLevels = true;
    bool mShowTabConnectionIndicators = true; // Global preference for showing connection status indicators on tabs

    qreal blinkPulseOpacity(bool isFastBlink) const;
    static qreal computeBlinkPulseOpacity(qreal blinkTimeMs, bool isFastBlink);
    void registerBlinkClient();
    void unregisterBlinkClient();


#if defined(INCLUDE_UPDATER)
    Updater* pUpdater = nullptr;
#endif


public slots:
    void slot_closeCurrentProfile();
    void slot_closeProfileRequested(int);
    void slot_closeProfileByName(const QString& profileName);
    void slot_connectionDialogueFinished(const QString&, bool);
    void slot_disconnect();
    void slot_handleToolbarVisibilityChanged(bool);
    void slot_toolbarToggleActionTriggered(bool);
#if defined(INCLUDE_UPDATER)
    void slot_manualUpdateCheck();
    void slot_updateCheckFailed(const QString& error);
    void slot_showFullChangelog();
#endif
    void slot_mapper();
    void slot_updateShowMapActionText();
    void slot_showMapperDialog(); // Enhanced mapper dialog with per-profile dock widgets
    void slot_moduleManager();
    void slot_mudletDiscord();
    void slot_multiView(const bool);
    void slot_muteMedia();
    void slot_muteAPI(const bool);
    void slot_muteGame(const bool);
    void slot_newDataOnHost(const QString&, bool isLowerPriorityChange = false);
    void slot_notes();
    void slot_openMappingScriptsPage();
    void slot_packageExporter();
    void slot_packageManager();
    void slot_processEventLoopHackTimerRun();
    void slot_profileDiscord();
    void slot_reconnect();
    void slot_reattachAllDetachedWindows();
    void slot_toggleAlwaysOnTop();
    void slot_minimize();
    void slot_newMapWindow();
    void updateWindowMenu();
    void slot_activateMainWindow();
    void slot_activateDetachedWindow();
    void slot_activateMainWindowProfile();
    void slot_activateDetachedWindowProfile();
    void slot_replay();
    void slot_replaySpeedUp();
    void slot_replaySpeedDown();
    void slot_replayTimeChanged();
    void slot_restoreMainMenu() { setMenuBarVisibility(enums::visibleAlways); }
    void slot_restoreMainToolBar() { synchronizeToolBarVisibility(true); }
    void slot_showAboutDialog();
    void slot_showHelpDialogForum();
    void slot_showHelpDialogIrc();
    void slot_showHelpDialogVideo();
    void slot_nextProfile();
    void slot_previousProfile();
    void slot_tabChanged(int);
    void slot_timerFires();
    void slot_toggleFullScreenView();
    void slot_toggleMultiView();
    void slot_toggleTimeStamp();
    void slot_toggleReplay();
    void slot_toggleLogging();
    void slot_toggleEmergencyStop();
    void slot_tabDetachRequested(int index, const QPoint& globalPos);
    void slot_tabReattachRequested(const QString& tabName, int insertIndex = -1);
    void slot_detachedWindowClosed(const QString& profileName);
    void slot_profileDetachToWindow(const QString& profileName, TDetachedWindow* targetWindow);
    void updateDetachedWindowToolbars();
    void updateMainWindowTabIndicators();
    void updateMainWindowTabBarAutoHide();
    void updateTabIndicators();               // Update all tab indicators (main window)
    void updateDetachedWindowTabIndicators(); // Update all detached window tab indicators
    void slot_showActionDialog();
    void slot_showAliasDialog();
    void slot_showEditorDialog();
    void slot_showHelpDialog();
    void slot_showKeyDialog();
    void slot_showPreferencesDialog();
    void slot_showScriptDialog();
    void slot_showUiTour();
    void slot_uiTourClosed();
    static void restoreProfileFocus(const QString& profileName);
    static void setupEditorFocusRestoration(dlgTriggerEditor* pEditor, const QString& profileName, QWidget* targetWindow = nullptr);
    void setupNotepadFocusRestoration(dlgNotepad* pNotepad);
    void setupPackageManagerFocusRestoration(dlgPackageManager* pPackageManager);
    void setupModuleManagerFocusRestoration(dlgModuleManager* pModuleManager);
    void setupPackageExporterFocusRestoration(dlgPackageExporter* pPackageExporter);
    void setupPreferencesFocusRestoration(dlgProfilePreferences* pPreferences);
    void slot_showTimerDialog();
    void slot_showTabContextMenu(const QPoint& position);
    void synchronizeToolBarVisibility(bool visible);
    void slot_showTriggerDialog();
    void slot_showVariableDialog();

protected:
    void closeEvent(QCloseEvent*) override;
    void changeEvent(QEvent*) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;


signals:
    void signal_adjustAccessibleNames();
    void signal_appearanceChanged(enums::Appearance);
    void signal_editorTextOptionsChanged(QTextOption::Flags);
    void signal_enableFulScreenModeChanged(bool);
    void signal_guiLanguageChanged(const QString&);
    void signal_hostCreated(Host*, quint8);
    void signal_hostDestroyed(Host*, quint8);
    void signal_menuBarVisibilityChanged(const enums::controlsVisibility);
    void signal_passwordMigratedToSecure(const QString&);
    void signal_passwordsMigratedToProfiles();
    void signal_passwordsMigratedToSecure();
    void signal_characterPasswordsMigrated();
    void signal_profileActivated(Host*, quint8);
    void signal_profileMapReloadRequested(QList<QString>);
    void signal_setToolBarIconSize(int);
    void signal_setTreeIconSize(int);
    void signal_shortcutsChanged();
    void signal_showIconsOnMenusChanged(const Qt::CheckState);
    void signal_showMapAuditErrorsChanged(bool);
    void signal_tabChanged(const QString&);
    void signal_toolBarVisibilityChanged(const enums::controlsVisibility);
    void signal_windowStateChanged(const Qt::WindowStates);
    void signal_showTabConnectionIndicatorsChanged(bool);
    void signal_blinkStateChanged();
    void signal_profileLoaded();

private slots:
    void slot_assignShortcutsFromProfile(Host* pHost = nullptr);
    void slot_audioOutputDeviceChanged();
    void slot_compactInputLine(const bool);
    void slot_passwordMigratedToPortableStorage(QKeychain::Job*);
    void slot_passwordMigratedToSecureStorage(QKeychain::Job*);
#if defined(INCLUDE_UPDATER)
    void slot_reportIssue();
#endif
    void slot_tabMoved(const int oldPos, const int newPos);
    void slot_toggleCompactInputLine();
#if defined(INCLUDE_UPDATER)
    void slot_updateAvailable(const int);
    void slot_updateInstalled();
#endif
    void slot_updateShortcuts();
    void slot_windowStateChanged(const Qt::WindowStates);
    void slot_applicationStateChanged(const Qt::ApplicationState);
    void slot_refreshTabIndicatorsDelayed();
    void slot_telnetConnectionStateChanged();


private:
    void assignKeySequences();
    QString autodetectPreferredLanguage();
    void showUiTour(const bool skipIntroStep);
    static bool needsCustomDarkTheme();
    void closeHost(const QString&);
    int getDictionaryWordCount(const QString& dictionaryPath);
    void goingDown() { mIsGoingDown = true; }
    void initEdbee();
    void installModulesList(Host*, QStringList);
    void loadMaps();
    void loadTranslators(const QString&);
    void migrateDebugConsole(Host*);
    bool overwriteAffixFile(const QString& affixPath, const QHash<QString, unsigned int>&);
    bool overwriteDictionaryFile(const QString& dictionaryPath, const QStringList&);
    bool scanDictionaryFile(const QString& dictionaryPath, int&, QHash<QString, unsigned int>&, QStringList&);
    int scanWordList(QStringList&, QHash<QString, unsigned int>&);
    void setupTrayIcon();
    void reshowRequiredMainConsoles();
    void toggleMute(bool state, QAction* toolbarAction, QAction* menuAction, bool isAPINotGame, const QString& unmuteText, const QString& muteText);
    dlgTriggerEditor* createMudletEditor();
    static void showEditorRestoringWindowState(QWidget* editor);

    // Profile detachment helper methods
    void moveProfileFromMainToDetachedWindow(const QString& profileName, int tabIndex, TDetachedWindow* targetWindow);
    void moveProfileBetweenDetachedWindows(const QString& profileName, TDetachedWindow* sourceWindow, TDetachedWindow* targetWindow);
    void moveProfileFromDetachedToMainWindow(const QString& profileName, TDetachedWindow* sourceWindow);
    int findTabIndex(const QString& profileName) const;
    void cleanupDetachedWindowsMap(); // Remove null pointers from the map


    inline static QPointer<mudlet> smpSelf = nullptr;


    bool mDarkMode = false;
    QString mDefaultStyle;
    // Stores the translated names for the Encodings for the static and thus
    // const TBuffer::csmEncodingTable:
    QMap<QByteArray, QString> mEncodingNameMap;
    HostManager mHostManager;
    // Points to the common mudlet dictionary handle once a profile has
    // requested it, then gets closed at termination of the application.
    Hunhandle* mpHunspell_sharedDictionary = nullptr;
    // Has default form of "en_US" but can be just an ISO language code e.g. "fr" for french,
    // without a country designation. Replaces xx in "mudlet_xx.qm" to provide the translation
    // file for GUI translation
    QString mInterfaceLanguage;
    QKeySequence mKeySequenceCloseProfile;
    QKeySequence mKeySequenceConnect;
    QKeySequence mKeySequenceDisconnect;
    QKeySequence mKeySequenceInputLine;
    QKeySequence mKeySequenceModules;
    QKeySequence mKeySequenceMultiView;
    QKeySequence mKeySequenceMute;
    QKeySequence mKeySequenceNotepad;
    QKeySequence mKeySequenceOptions;
    QKeySequence mKeySequencePackages;
    QKeySequence mKeySequenceReconnect;
    QKeySequence mKeySequenceShowMap;
    QKeySequence mKeySequenceTriggers;
    QKeySequence mKeySequenceToggleTimeStamp;
    QKeySequence mKeySequenceToggleReplay;
    QKeySequence mKeySequenceToggleLogging;
    QKeySequence mKeySequenceToggleEmergencyStop;
    QKeySequence mKeySequenceNextProfile;
    QKeySequence mKeySequencePreviousProfile;
    std::array<QKeySequence, 9> mKeySequencesSwitchToProfile;
    bool mIsGoingDown = false;
    // Whether multi-view is in effect:
    enums::controlsVisibility mMenuBarVisibility = enums::visibleAlways;
    // Used to ensure that mudlet::slot_updateShortcuts() only runs once each
    // time the main if () logic changes state - will be true if the menu is
    // supposed to be visible, false if not and not have a value initially:
    std::optional<bool> mMenuVisibleState;
    QString mMudletDiscordInvite = qsl("https://www.mudlet.org/chat");
    bool mMultiView = false;
    bool mMuteAPI = false;
    bool mMuteGame = false;
    QMediaDevices* mpMediaDevices = nullptr;
    QPointer<QAction> mpActionAbout;
    QPointer<QAction> mpActionAboutWithUpdates;
    QPointer<QAction> mpActionAliases;
    QPointer<QAction> mpActionButtons;
    QPointer<QAction> mpActionCloseProfile;
    QPointer<QAction> mpActionCloseApplication;
    QPointer<QAction> mpActionConnect;
    QPointer<QAction> mpActionDisconnect;
    QPointer<QAction> mpActionDiscord;
    QPointer<QAction> mpActionFullScreenView;
    QPointer<QAction> mpActionHelp;
    QPointer<QAction> mpActionKeys;
    QPointer<QAction> mpActionMapper;
    QPointer<QAction> mpActionModuleManager;
    QPointer<QAction> mpActionMudletDiscord;
    QPointer<QAction> mpActionMultiView;
    QPointer<QAction> mpActionMuteMedia;
    QPointer<QAction> mpActionMuteAPI;
    QPointer<QAction> mpActionMuteGame;
    QPointer<QAction> mpActionNotes;
    QPointer<QAction> mpActionOptions;
    QPointer<QAction> mpActionPackageExporter;
    QPointer<QAction> mpActionPackageManager;
    QPointer<QAction> mpActionReconnect;
    QPointer<QAction> mpActionReplay;
    QPointer<QAction> mpActionReplaySpeedDown;
    QPointer<QAction> mpActionReplaySpeedUp;
    QPointer<QAction> mpActionReplayTime;
    QPointer<QAction> mpActionReportIssue;
    QPointer<QAction> mpActionScripts;
    QPointer<QAction> mpActionSpeedDisplay;
    QPointer<QAction> mpActionTimers;
    QPointer<QAction> mpActionToggleMainToolBar;
    QPointer<QAction> mpActionTriggers;
    QPointer<QAction> mpActionVariables;
    // This pair retains the path argument supplied to the corresponding
    // scanForXxxTranslations(...) method so it is available to the subsequent
    // loadTranslators(...) call
    QString mPathNameMudletTranslations;
    QString mPathNameQtTranslations;
    QPointer<QToolButton> mpButtonAbout;
    QPointer<QToolButton> mpButtonConnect;
    QPointer<QToolButton> mpButtonDiscord;
    QPointer<QToolButton> mpButtonMute;
    // The single shared speech recognizer (one microphone, one decoder);
    // created lazily by initSpeechRecognition()
    QPointer<SpeechRecognizer> mpSpeechRecognizer;
    QPointer<QToolButton> mpButtonPackageManagers;
    QHBoxLayout* mpHBoxLayout_profileContainer = nullptr;
    QPointer<QLabel> mpLabelReplaySpeedDisplay;
    QPointer<QLabel> mpLabelReplayTime;
    QPointer<QWidget> mpFocusWidgetBeforeDeactivate;
    // a list of profiles currently being migrated to secure or profile storage
    QStringList mProfilePasswordsToMigrate;
    // a list of character passwords currently being migrated to secure storage
    QList<QPair<QString, QString>> mCharacterPasswordsToMigrate;
    QPointer<QShortcut> mpShortcutCloseProfile;
    QPointer<QShortcut> mpShortcutConnect;
    QPointer<QShortcut> mpShortcutDisconnect;
    QPointer<QShortcut> mpShortcutInputLine;
    QPointer<QShortcut> mpShortcutModules;
    QPointer<QShortcut> mpShortcutMultiView;
    QPointer<QShortcut> mpShortcutMute;
    QPointer<QShortcut> mpShortcutNotepad;
    QPointer<QShortcut> mpShortcutOptions;
    QPointer<QShortcut> mpShortcutPackages;
    QPointer<QShortcut> mpShortcutReconnect;
    QPointer<QShortcut> mpShortcutShowMap;
    QPointer<QShortcut> mpShortcutTriggers;
    QPointer<QShortcut> mpShortcutToggleTimeStamp;
    QPointer<QShortcut> mpShortcutToggleReplay;
    QPointer<QShortcut> mpShortcutToggleLogging;
    QPointer<QShortcut> mpShortcutToggleEmergencyStop;
    QPointer<QShortcut> mpShortcutNextProfile;
    QPointer<QShortcut> mpShortcutPreviousProfile;
    std::array<QPointer<QShortcut>, 9> mpShortcutsSwitchToProfile;
    QPointer<QTimer> mpTimerReplay;
    QPointer<QTimer> mpBlinkTimer;
    QElapsedTimer mBlinkElapsedTimer;
    qreal mBlinkTimeMs = 0.0;
    int mBlinkClientCount = 0;
    QPointer<QToolBar> mpToolBarReplay;
    QPointer<TUiTour> mpUiTour;
    QWidget* mpWidget_profileContainer = nullptr;
    // read-only value to see if the interface is light or dark. To set the value,
    // use setAppearance instead
    bool mShowMapAuditErrors = false;
    bool mInvertMapZoom = false; // true = old behavior (inverted), false = modern behavior (non-inverted)
    QSplitter* mpSplitter_profileContainer = nullptr;
    bool mStorePasswordsSecurely = true;
    // Argument to QDateTime::toString(...) to format the elapsed time display
    // on the mpToolBarReplay:
    QString mTimeFormat;
    enums::controlsVisibility mToolbarVisibility = enums::visibleNever;
    QList<QPointer<QTranslator>> mTranslatorsLoadedList;
    // An encapsulation of the mInterfaceLanguage in a form that Qt uses to
    // hold all the details:
    QLocale mUserLocale;
    QMap<Host*, QToolBar*> mUserToolbarMap;
    // The collection of words in what mpHunspell_sharedDictionary points to:
    QSet<QString> mWordSet_shared;

    // Window menu management for multiple windows
    QList<QAction*> mWindowListActions;
    QAction* mWindowListSeparator = nullptr;

    // Addon command management. One command may stand on both surfaces at once -
    // a toolbar button and a menu item that are the same thing to the package
    // that placed it, addressed by one id and raising one event, which is how
    // Mudlet's own commands already behave.
    //
    // closeHost() and a profile reset both drop every command belonging to that
    // profile before its Host goes, so no entry should outlive its owner - the
    // QPointers are what keeps a missed path from turning into a dangling read
    // in the click handlers, which resolve pHost lazily.
    struct AddonCommand
    {
        QPointer<QToolButton> button;
        QPointer<QAction> toolbarAction;
        QPointer<QAction> menuAction;
        QPointer<QTimer> pulseTimer;
        QPointer<Host> pHost;
        bool pulseState = false;
        QString pulseColor1;
        QString pulseColor2;
    };
    QMenu* addonMenuForPath(const QString& menuPath, const Host* pHost, QString& error);
    bool addonShortcutUsable(const QKeySequence& sequence, const Host* pHost, QString& error) const;
    static QString addonTooltip(const QString& tooltip);
    // Qt reads '&' in a QAction's or QToolButton's text as a mnemonic, so a
    // package's "Fish & Chips" draws without the ampersand and steals Alt+Space.
    // The clash checks compare labels after doubling, so a path part is put
    // through this before being matched against what a menu already holds.
    static QString addonLabel(const QString& name);
    // The inverse, for a message rather than a surface: a refusal quoting Qt's
    // mnemonic syntax names a label that appears nowhere on screen.
    static QString addonPlainLabel(const QString& label);
    const Host* addonCommandOwning(const QAction* action) const;
    static void applyAddonIcon(QToolButton* button, QAction* action, const QString& icon);
    void raiseAddonCommandEvent(int commandId);
    // Copy the checked state of the surface the user just activated onto the
    // other one. Qt toggles only the control that was pressed, so without this
    // a command shows a tick in the menu and none on the toolbar.
    void mirrorAddonCommandChecked(int commandId, bool checked);

    QMap<int, AddonCommand> mAddonCommands;
    // One sequence for every command, so an id names one thing or nothing
    int mNextAddonCommandId = 1;
    QAction* mpAddonToolbarSeparator = nullptr;
    QPointer<QMenu> mpAddonsMenu;
    // Which profile a menuPath submenu was built for. Placement has to be
    // decided by the profile's own commands alone: sharing one namespace meant
    // whether a package could place a command depended on which unrelated
    // profiles happened to be open, and on labels it could neither see nor clear.
    QHash<QMenu*, const Host*> mAddonSubmenuOwners;

    // amount of times the shortcut has been shown help educate new users
    int mScrollbackTutorialsShown = 0;   // Cancel split screen
    int mMuteAllMediaTutorialsShown = 0; // Mute all media
    int mCharacterModeWarningsShown = 0; // Character-at-a-time mode detection

    // show the tutorial maximum 3 times on a new Mudlet
    static constexpr int mScrollbackTutorialsMax = 3;   // Split screen
    static constexpr int mMuteAllMediaTutorialsMax = 3; // Mute all media
    static constexpr int mCharacterModeWarningsMax = 3; // Character mode

    // Telnet URI handling structures and methods
    struct TelnetUriData
    {
        QString host;
        int port = 23;
        QString username;
        bool useTls = false;
    };

    std::optional<TelnetUriData> parseTelnetUri(const QString& uri);
    QString findMatchingProfile(const QString& host, int port);
    QString createProfileForUri(const TelnetUriData& uriData);

    // Helper method for detached windows cleanup
    void saveDetachedWindowsGeometry();

    // Detached windows for profiles
    QMap<QString, QPointer<TDetachedWindow>> mDetachedWindows;

    // The map actions' enabled state before the active profile's
    // "mapperButton" setConfig mode is applied on top - the last baseline the
    // toolbar management functions computed
    bool mMapActionBaselineEnabled = false;

    // Dock widget management for main window per-profile widgets
    QMap<QString, QPointer<QDockWidget>> mMainWindowDockWidgetMap;
    QMap<QString, bool> mMainWindowDockWidgetUserPreference; // User's show/hide preference for dock widgets
    QPointer<QDockWidget> mpCurrentMapDockWidget;

    // Helper methods for detached windows
    void closeHostOfClosedDetachedWindow(const QString& profileName);
    void detachTab(int tabIndex, const QPoint& position);
    void reattachTab(const QString& profileName, int insertIndex = -1);
    TMainConsole* removeConsoleFromSplitter(const QString& profileName);
    void addConsoleToSplitter(TMainConsole* console, int index = -1);

    // Helper methods for main window dock widget management
    void updateMainWindowDockWidgetVisibilityForProfile(const QString& profileName);
    void transferDockWidgetToDetachedWindow(const QString& profileName, TDetachedWindow* detachedWindow);
    void transferDockWidgetFromDetachedWindow(const QString& profileName, TDetachedWindow* detachedWindow);
    void transferDockWidgetBetweenDetachedWindows(const QString& profileName, TDetachedWindow* sourceWindow, TDetachedWindow* targetWindow);
};


class TConsoleMonitor : public QObject
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(TConsoleMonitor)
    explicit TConsoleMonitor(QObject* parent)
    : QObject(parent)
    {
    }

protected:
    bool eventFilter(QObject*, QEvent*) override;
};


// A convenience class to keep all the details for the translators for a
// specific locale code (language only "xx" or language/country "xx_YY")
// in one unified structure.
class translation
{
    // The following must have friendship so they can set private members:
    friend void mudlet::scanForMudletTranslations(const QString&);
    friend void mudlet::scanForQtTranslations(const QString&);

public:
    explicit translation(const int translationPercent = -1)
    : mTranslatedPercentage(translationPercent)
    {
    }

    const QString& getNativeName() const { return mNativeName; }
    const QString& getMudletTranslationFileName() const { return mMudletTranslationFileName; }
    const QString& getQtTranslationFileName() const { return mQtTranslationFileName; }
    const int& getTranslatedPercentage() const { return mTranslatedPercentage; }
    bool fromResourceFile() const { return mTranslatedPercentage >= 0; }

private:
    // ONLY if the translation is loaded from an embedded resource file,
    // this is the percentage complete of the translation
    int mTranslatedPercentage = -1;
    // Used for display in the profile preferences and is never translated:
    QString mNativeName;
    // filename translation is loaded from
    QString mMudletTranslationFileName;
    // Qt translation file was found to be, note that in most cases the loaded
    // file will be a "xx" language only file even though it is an "xx_YY" one
    // here:
    QString mQtTranslationFileName;
    // Similar filename locations will require adding for any 3rd party translations
    // we load!
};

#endif // MUDLET_MUDLET_H
