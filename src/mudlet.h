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

#include "Announcer.h"
#include "MudletInstanceCoordinator.h"
#include "discord.h"
#include "FontManager.h"
#include "HostManager.h"
#include "ShortcutsManager.h"
#include "TMediaData.h"
#include "utils.h"
#include <memory>

#if defined(INCLUDE_UPDATER)
#include "updater.h"
#endif

#include "pre_guard.h"
#include "ui_main_window.h"
#include "edbee/views/texttheme.h"
#include <QAction>
#include <QDir>
#include <QFlags>
#include <QKeySequence>
#include <QMainWindow>
#include <QMap>
#include <QPointer>
#include <QSettings>
#include <QSystemTrayIcon>
#include <QTextOption>
#include <QTime>
#include <QVersionNumber>
#include "edbee/models/textautocompleteprovider.h"
#if defined(INCLUDE_OWN_QT5_KEYCHAIN)
#include <../3rdparty/qtkeychain/keychain.h>
#else
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <qt5keychain/keychain.h>
#else
#include <qt6keychain/keychain.h>
#endif // QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#endif // defined(INCLUDE_OWN_QT5_KEYCHAIN)
#include <optional>
#include <hunspell/hunspell.hxx>
#include <hunspell/hunspell.h>

// for system physical memory info
#if defined(Q_OS_WIN32)
#include <Windows.h>
#include <Psapi.h>
#elif defined(Q_OS_MACOS)
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <array>
#elif defined(Q_OS_HURD)
#include <errno.h>
#include <unistd.h>
#elif defined(Q_OS_OPENBSD)
// OpenBSD doesn't have a sysinfo.h
#include <sys/sysctl.h>
#include <unistd.h>
#elif defined(Q_OS_UNIX)
// Including both GNU/Linux and FreeBSD
#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <unistd.h>
#else
// Any other OS?
#endif
#include "post_guard.h"

class QCloseEvent;
class QMediaPlayer;
class QMenu;
class QLabel;
class QListWidget;
class QPushButton;
class QShortcut;
class QSplitter;
class QTableWidget;
class QTableWidgetItem;
class QTextEdit;
class QToolButton;
class QTimer;

class dlgAboutDialog;
class dlgConnectionProfiles;
class dlgIRC;
class dlgProfilePreferences;
class Host;
class ShortcutManager;
class TConsole;
class TDockWidget;
class TEvent;
class TLabel;
class translation;
class TScrollBox;
class TTabBar;
class TTimer;
class TToolBar;

class mudlet : public QMainWindow, public Ui::main_window
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(mudlet)
    mudlet();
    ~mudlet() override;

    enum Appearance {
        systemSetting = 0,
        light = 1,
        dark = 2
    };

    enum controlsVisibilityFlag {
        visibleNever = 0,
        visibleOnlyWithoutLoadedProfile = 0x1,
        visibleMaskNormally = 0x2,
        visibleAlways = 0x3
    };
    Q_DECLARE_FLAGS(controlsVisibility, controlsVisibilityFlag)

    enum mudletPathType {
        // The root of all mudlet data for the user - does not end in a '/'
        mainPath = 0,
        // Takes one extra argument as a file (or directory) relating to
        // (profile independent) mudlet data - may end with a '/' if the extra
        // argument does:
        mainDataItemPath,
        // (Added for 3.5.0) a revised location to store Mudlet provided fonts:
        mainFontsPath,
        // The directory containing all the saved user's profiles - does not end
        // in '/':
        profilesPath,
        // Takes one extra argument (profile name) that returns the base
        // directory for that profile - does NOT end in a '/' unless the
        // supplied profle name does:
        profileHomePath,
        // Takes one extra argument (profile name) that returns the directory
        // for the profile game save media files - does NOT end in a '/'
        profileMediaPath,
        // Takes two extra arguments (profile name, mediaFileName) that returns
        // the pathFile name for any media file:
        profileMediaPathFileName,
        // Takes one extra argument (profile name) that returns the directory
        // for the profile game save XML files - ends in a '/':
        profileXmlFilesPath,
        // Takes one extra argument (profile name) that returns the directory
        // for the profile game save maps files - does NOT end in a '/'
        profileMapsPath,
        // Takes two extra arguments (profile name, dataTime stamp) that returns
        // the pathFile name for a dateTime stamped map file:
        profileDateTimeStampedMapPathFileName,
        // Takes two extra arguments (profile name, dataTime stamp) that returns
        // the pathFile name for a dateTime stamped JSON map file:
        profileDateTimeStampedJsonMapPathFileName,
        // Takes two extra arguments (profile name, mapFileName) that returns
        // the pathFile name for any map file:
        profileMapPathFileName,
        // Takes one extra argument (profile name) that returns the file
        // location for the downloaded MMP map:
        profileXmlMapPathFileName,
        // Takes two extra arguments (profile name, data item) that gives a
        // path file name for, typically a data item stored as a single item
        // (binary) profile data) file (ideally these can be moved to a per
        // profile QSettings file but that is a future pipe-dream on my part
        // SlySven):
        profileDataItemPath,
        // Takes two extra arguments (profile name, package name) returns the
        // per profile directory used to store (unpacked) package contents
        // - ends with a '/':
        profilePackagePath,
        // Takes two extra arguments (profile name, package name) returns the
        // filename of the XML file that contains the (per profile, unpacked)
        // package mudlet items in that package/module:
        profilePackagePathFileName,
        // Takes one extra argument (profile name) that returns the directory
        // that contains replays (*.dat files) and logs (*.html or *.txt) files
        // for that profile - does NOT end in '/':
        profileReplayAndLogFilesPath,
        // Takes one extra argument (profile name) that returns the pathFileName
        // to the map auditing report file that is appended to each time a
        // map is loaded:
        profileLogErrorsFilePath,
        // Takes two extra arguments (profile name, theme name) that returns the
        // pathFileName of the theme file used by the edbee editor - also
        // handles the special case of the default theme "mudlet.thTheme" that
        // is carried internally in the resource file:
        editorWidgetThemePathFile,
        // Returns the pathFileName to the external JSON file needed to process
        // an edbee edtor widget theme:
        editorWidgetThemeJsonFile,
        // Returns the directory used to store module backups that is used in
        // when saving/resyncing packages/modules - ends in a '/'
        moduleBackupsPath,
        // Returns path to Qt's own translation files
        qtTranslationsPath,
        // Takes one extra argument - a (dictionary) language code that should
        // match a hunspell affix file name e.g. "en_US" in the default case
        // to yield "en_US.aff" that is searched for in one or more OS dependent
        // places - returns the path ending in a '/' to use to get the
        // dictionaries from:
        hunspellDictionaryPath
    };


    static QString getMudletPath(mudletPathType, const QString& extra1 = QString(), const QString& extra2 = QString());
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
    // These have to be "inline" to satisfy the ODR (One Definition Rule):
    inline static bool smDebugMode = false;
    inline static bool smFirstLaunch = false;
    inline static QVariantHash smLuaFunctionNames;
    inline static QPointer<TConsole> smpDebugConsole;
    inline static QPointer<QMainWindow> smpDebugArea;
    // mirror everything shown in any console to stdout. Helpful for CI environments
    inline static bool smMirrorToStdOut = false;
    // adjust Mudlet settings to match Steam's requirements
    inline static bool smSteamMode = false;


    void showEvent(QShowEvent*) override;
    void hideEvent(QHideEvent*) override;


    void init();
    void activateProfile(Host*);
    void takeOwnershipOfInstanceCoordinator(std::unique_ptr<MudletInstanceCoordinator>);
    MudletInstanceCoordinator* getInstanceCoordinator();
    void addConsoleForNewHost(Host*);
    QPair<bool, bool> addWordToSet(const QString&);
    void adjustMenuBarVisibility();
    void adjustToolBarVisibility();
    void announce(const QString& text, const QString& processing = QString());
    void attachDebugArea(const QString&);
    void checkUpdatesOnStart();
    void commitLayoutUpdates(bool flush = false);
    void deleteProfileData(const QString &profile, const QString &item);
    void disableToolbarButtons();
    void doAutoLogin(const QString&);
    void enableToolbarButtons();
    void forceClose();
    void armForceClose();
    Host* getActiveHost();
    QStringList getAvailableFonts();
    QList<QString> getAvailableTranslationCodes() const { return mTranslationsMap.keys(); }
    const QMap<QByteArray, QString>& getEncodingNamesMap() const { return mEncodingNameMap; }
    HostManager& getHostManager() { return mHostManager; }
    std::optional<QSize> getImageSize(const QString&);
    const QString& getInterfaceLanguage() const { return mInterfaceLanguage; }
    int64_t getPhysicalMemoryTotal();
    QSettings* getQSettings();
    const QLocale& getUserLocale() const { return mUserLocale; }
    QSet<QString> getWordSet();
    bool inDarkMode() const { return mDarkMode; }
    // Used to enable "emergency" control recovery action - if Mudlet is
    // operating without either menubar or main toolbar showing.
    bool isControlsVisible() const;
    bool isGoingDown() { return mIsGoingDown; }
    bool loadReplay(Host*, const QString&, QString* pErrMsg = nullptr);
    bool loadWindowLayout();
    controlsVisibility menuBarVisibility() const { return mMenuBarVisibility; }
    bool migratePasswordsToProfileStorage();
    bool migratePasswordsToSecureStorage();
    void onlyShowProfiles(const QStringList&);
    bool openWebPage(const QString&);
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
    // Used by a profile to tell the mudlet class
    // to tell other profiles to reload the updated
    // maps (via signal_profileMapReloadRequested(...))
    void requestProfilesToReloadMaps(QList<QString>);
    void replayOver();
    bool replayStart();
    std::pair<bool, QString> resetProfileIcon(const QString&);
#if defined(Q_OS_WIN32)
    void sanitizeUtf8Path(QString& originalLocation, const QString& fileName) const;
#endif
    // This will save and replace the .dic file with just the words in the
    // supplied second argument and update the .aff file as appropriate. It is
    // to be used at the end of a session to store away the user's changes:
    bool saveDictionary(const QString&, QSet<QString>&);
    bool saveWindowLayout();
    void scanForMudletTranslations(const QString&);
    void scanForQtTranslations(const QString&);
    void setAppearance(Appearance, const bool& loading = false);
    bool setClickthrough(Host*, const QString&, bool);
    void setEditorTextoptions(bool isTabsAndSpacesToBeShown, bool isLinesAndParagraphsToBeShown);
    void setEditorTreeWidgetIconSize(int);
    void setEnableFullScreenMode(const bool);
    void setGlobalStyleSheet(const QString&);
    void setInterfaceLanguage(const QString&);
    void setMenuBarVisibility(controlsVisibility);
    std::pair<bool, QString> setProfileIcon(const QString& profile, const QString& newIconPath);
    void setShowIconsOnMenu(const Qt::CheckState);
    void setShowMapAuditErrors(const bool);
    void setupPreInstallPackages(const QString&);
    void setToolBarIconSize(int);
    void setToolBarVisibility(controlsVisibility);
    void showChangelogIfUpdated();
    void slot_showConnectionDialog();
    bool showMapAuditErrors() const { return mShowMapAuditErrors; }
    // Brings up the preferences dialog and selects the tab whos objectName is
    // supplied:
    void showOptionsDialog(const QString&);
    void startAutoLogin(const QStringList&);
    bool storingPasswordsSecurely() const { return mStorePasswordsSecurely; }
    controlsVisibility toolBarVisibility() const { return mToolbarVisibility; }
    void updateDiscordNamedIcon();
    void updateMultiViewControls();
    QPair<bool, QString> writeProfileData(const QString& profile, const QString& item, const QString& what);
    void writeSettings();
    bool muteAPI() const { return mMuteAPI; }
    bool muteGame() const { return mMuteGame; }
    bool mediaMuted() const { return mMuteAPI && mMuteGame; }
    bool mediaUnmuted() const { return !mMuteAPI && !mMuteGame; }

    Appearance mAppearance = Appearance::systemSetting;
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
    QHash<QString, QString>mDictionaryLanguageCodeMap;
    Discord mDiscord;
    // Used for editor area, but
    // only ::ShowTabsAndSpaces
    // and ::ShowLineAndParagraphSeparators
    // are considered/used/stored
    QTextOption::Flags mEditorTextOptions = QTextOption::Flags();
    int mEditorTreeWidgetIconSize = 0;
    // Currently tracks the "mudlet_option_use_smallscreen" file's existence but
    // may eventually migrate solely to the "EnableFullScreenMode" in the main
    // QSetting file - it is only stored as a file now to maintain backwards
    // compatibility...
    bool mEnableFullScreenMode = false;
    FontManager mFontManager;
    bool mHasSavedLayout = false;
    bool mIsLoadingLayout = false;
    QStringList mOnlyShownPredefinedProfiles;
    QPointer<dlgAboutDialog> mpAboutDlg;
    QStringList mPackagesToInstallList;
    QPointer<dlgConnectionProfiles> mpConnectionDialog;
    QPointer<Host> mpCurrentActiveHost;
    // Options dialog when there's no active host
    QPointer<dlgProfilePreferences> mpDlgProfilePreferences;
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

#if defined(INCLUDE_UPDATER)
    Updater* pUpdater = nullptr;
#endif


public slots:
    void slot_closeCurrentProfile();
    void slot_closeProfileRequested(int);
    void slot_connectionDialogueFinished(const QString&, bool);
    void slot_disconnect();
    void slot_handleToolbarVisibilityChanged(bool);
    void slot_irc();
#if defined(INCLUDE_UPDATER)
    void slot_manualUpdateCheck();
#endif
    void slot_mapper();
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
    void slot_replay();
    void slot_replaySpeedUp();
    void slot_replaySpeedDown();
    void slot_replayTimeChanged();
    void slot_restoreMainMenu() { setMenuBarVisibility(visibleAlways); }
    void slot_restoreMainToolBar() { setToolBarVisibility(visibleAlways); }
    void slot_showAboutDialog();
    void slot_showHelpDialogForum();
// Not used:    void slot_showHelpDialogIrc();
    void slot_showHelpDialogVideo();
    void slot_tabChanged(int);
    void slot_timerFires();
    void slot_toggleFullScreenView();
    void slot_toggleMultiView();


protected:
    void closeEvent(QCloseEvent*) override;


signals:
    void signal_adjustAccessibleNames();
    void signal_appearanceChanged(mudlet::Appearance);
    void signal_editorTextOptionsChanged(QTextOption::Flags);
    void signal_enableFulScreenModeChanged(bool);
    void signal_guiLanguageChanged(const QString&);
    void signal_hostCreated(Host*, quint8);
    void signal_hostDestroyed(Host*, quint8);
    void signal_menuBarVisibilityChanged(const mudlet::controlsVisibility);
    void signal_passwordMigratedToSecure(const QString&);
    void signal_passwordsMigratedToProfiles();
    void signal_passwordsMigratedToSecure();
    void signal_profileActivated(Host *, quint8);
    void signal_profileMapReloadRequested(QList<QString>);
    void signal_setToolBarIconSize(int);
    void signal_setTreeIconSize(int);
    void signal_shortcutsChanged();
    void signal_showIconsOnMenusChanged(const Qt::CheckState);
    void signal_showMapAuditErrorsChanged(bool);
    void signal_tabChanged(const QString&);
    void signal_toolBarVisibilityChanged(const mudlet::controlsVisibility);


private slots:
    void slot_assignShortcutsFromProfile(Host* pHost = nullptr);
    void slot_compactInputLine(const bool);
    void slot_passwordMigratedToPortableStorage(QKeychain::Job*);
    void slot_passwordMigratedToSecureStorage(QKeychain::Job*);
#if defined(INCLUDE_UPDATER)
    void slot_reportIssue();
#endif
    void slot_showActionDialog();
    void slot_showAliasDialog();
    void slot_showEditorDialog();
    void slot_showHelpDialog();
    void slot_showKeyDialog();
    void slot_showPreferencesDialog();
    void slot_showScriptDialog();
    void slot_showTimerDialog();
    void slot_showTriggerDialog();
    void slot_showVariableDialog();
    void slot_tabMoved(const int oldPos, const int newPos);
    void slot_toggleCompactInputLine();
#if defined(INCLUDE_UPDATER)
    void slot_updateAvailable(const int);
    void slot_updateInstalled();
#endif
    void slot_updateShortcuts();


private:
    static bool desktopInDarkMode();


    void assignKeySequences();
    QString autodetectPreferredLanguage();
    void closeHost(const QString&);
    int getDictionaryWordCount(const QString &dictionaryPath);
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
    bool mIsGoingDown = false;
    // Whether multi-view is in effect:
    controlsVisibility mMenuBarVisibility = visibleAlways;
    // Used to ensure that mudlet::slot_updateShortcuts() only runs once each
    // time the main if () logic changes state - will be true if the menu is
    // supposed to be visible, false if not and not have a value initially:
    std::optional<bool> mMenuVisibleState;
    QString mMudletDiscordInvite = qsl("https://www.mudlet.org/chat");
    bool mMultiView = false;
    bool mMuteAPI = false;
    bool mMuteGame = false;
    QPointer<QAction> mpActionAbout;
    QPointer<QAction> mpActionAboutWithUpdates;
    QPointer<QAction> mpActionAliases;
    QPointer<QAction> mpActionButtons;
    QPointer<QAction> mpActionCloseProfile;
    QPointer<QAction> mpActionConnect;
    QPointer<QAction> mpActionDisconnect;
    QPointer<QAction> mpActionDiscord;
    QPointer<QAction> mpActionFullScreenView;
    QPointer<QAction> mpActionHelp;
    QPointer<QAction> mpActionIRC;
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
    QPointer<QAction> mpActionTriggers;
    QPointer<QAction> mpActionVariables;
    Announcer* mpAnnouncer = nullptr;
    // This pair retains the path argument supplied to the corresponding
    // scanForXxxTranslations(...) method so it is available to the subsequent
    // loadTranslators(...) call
    QString mPathNameMudletTranslations;
    QString mPathNameQtTranslations;
    QPointer<QToolButton> mpButtonAbout;
    QPointer<QToolButton> mpButtonConnect;
    QPointer<QToolButton> mpButtonDiscord;
    QPointer<QToolButton> mpButtonMute;
    QPointer<QToolButton> mpButtonPackageManagers;
    QHBoxLayout* mpHBoxLayout_profileContainer = nullptr;
    QPointer<QLabel> mpLabelReplaySpeedDisplay;
    QPointer<QLabel> mpLabelReplayTime;
    // a list of profiles currently being migrated to secure or profile storage
    QStringList mProfilePasswordsToMigrate;
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
    QPointer<QTimer> mpTimerReplay;
    QPointer<QToolBar> mpToolBarReplay;
    QWidget* mpWidget_profileContainer = nullptr;
    // read-only value to see if the interface is light or dark. To set the value,
    // use setAppearance instead
    bool mShowMapAuditErrors = false;
    QSplitter* mpSplitter_profileContainer = nullptr;
    bool mStorePasswordsSecurely = true;
    // Argument to QDateTime::toString(...) to format the elapsed time display
    // on the mpToolBarReplay:
    QString mTimeFormat;
    controlsVisibility mToolbarVisibility = visibleAlways;
    QList<QPointer<QTranslator>> mTranslatorsLoadedList;
    // An encapsulation of the mInterfaceLanguage in a form that Qt uses to
    // hold all the details:
    QLocale mUserLocale;
    QMap<Host*, QToolBar*> mUserToolbarMap;
    // The collection of words in what mpHunspell_sharedDictionary points to:
    QSet<QString> mWordSet_shared;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(mudlet::controlsVisibility)

class TConsoleMonitor : public QObject
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(TConsoleMonitor)
    explicit TConsoleMonitor(QObject* parent)
    : QObject(parent)
    {}

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
    {}

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
