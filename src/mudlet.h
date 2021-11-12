#ifndef MUDLET_MUDLET_H
#define MUDLET_MUDLET_H

/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016 by Chris Leacy - cleacy1972@gmail.com              *
 *   Copyright (C) 2015-2016, 2018-2019, 2021 by Stephen Lyons             *
 *                                               - slysven@virginmedia.com *
 *   Copyright (C) 2016-2018 by Ian Adkins - ieadkins@gmail.com            *
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


#include "HostManager.h"
#include "FontManager.h"

#include "edbee/views/texttheme.h"
#include "ui_main_window.h"
#if defined(INCLUDE_UPDATER)
#include "updater.h"
#endif

#include "discord.h"

#include "pre_guard.h"
#include <QDir>
#include <QFlags>
#ifdef QT_GAMEPAD_LIB
#include <QGamepad>
#endif
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
#include <qt5keychain/keychain.h>
#endif

#include <optional>
#include "post_guard.h"

#include <hunspell/hunspell.hxx>
#include <hunspell/hunspell.h>

// how to use: https://github.com/mandeepsandhu/qt-ordered-map/blob/master/tests/functional/testorderedmap.cpp
#include <../3rdparty/qt-ordered-map/src/orderedmap.h>

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
#elif defined(Q_OS_UNIX)
// Including both GNU/Linux and FreeBSD
#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <unistd.h>
#else
// Any other OS?
#endif

class QAction;
class QCloseEvent;
class QMediaPlayer;
class QMenu;
class QLabel;
class QListWidget;
class QPushButton;
class QShortcut;
class QTableWidget;
class QTableWidgetItem;
class QTextEdit;
class QToolButton;
class QTimer;

class Host;
class TConsole;
class TDockWidget;
class TEvent;
class TLabel;
class TTabBar;
class TTimer;
class TToolBar;
class dlgIRC;
class dlgAboutDialog;
class dlgConnectionProfiles;
class dlgProfilePreferences;

class translation;

class mudlet : public QMainWindow, public Ui::main_window
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(mudlet)
    mudlet();
    ~mudlet();

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

    static mudlet* self();
    // This method allows better debugging when mudlet::self() is called inappropriately.
    static void start();
    HostManager& getHostManager() { return mHostManager; }
    void attachDebugArea(const QString& hostname);
    void addConsoleForNewHost(Host* pH);
    void disableToolbarButtons();
    void enableToolbarButtons();
    Host* getActiveHost();
    void forceClose();
    bool saveWindowLayout();
    bool loadWindowLayout();
    void commitLayoutUpdates(bool flush = false);

    std::optional<QSize> getImageSize(const QString& imageLocation);
    QString readProfileData(const QString& profile, const QString& item);
    QPair<bool, QString> writeProfileData(const QString& profile, const QString& item, const QString& what);
    void deleteProfileData(const QString &profile, const QString &item);
    void readEarlySettings(const QSettings&);
    void readLateSettings(const QSettings&);
    void writeSettings();
    bool openWebPage(const QString& path);
    void checkUpdatesOnStart();
    void processEventLoopHack();
    bool isGoingDown() { return mIsGoingDown; }
    void setToolBarIconSize(int);
    void setEditorTreeWidgetIconSize(int);
    enum controlsVisibilityFlag {
        visibleNever = 0,
        visibleOnlyWithoutLoadedProfile = 0x1,
        visibleMaskNormally = 0x2,
        visibleAlways = 0x3
    };
    Q_DECLARE_FLAGS(controlsVisibility, controlsVisibilityFlag)
    void setToolBarVisibility(controlsVisibility);
    void setMenuBarVisibility(controlsVisibility);
    void adjustToolBarVisibility();
    void adjustMenuBarVisibility();
    controlsVisibility menuBarVisibility() const { return mMenuBarVisibility; }
    controlsVisibility toolBarVisibility() const { return mToolbarVisibility; }
    bool replayStart();
    bool setClickthrough(Host* pHost, const QString& name, bool clickthrough);
    void replayOver();
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void doAutoLogin(const QString&);
    void stopSounds();
    void playSound(const QString &s, int);
    QStringList getAvailableFonts();
    std::pair<bool, QString> setProfileIcon(const QString& profile, const QString& newIconPath);
    std::pair<bool, QString> resetProfileIcon(const QString& profile);
#if defined(Q_OS_WIN32)
    void sanitizeUtf8Path(QString& originalLocation, const QString& fileName) const;
#endif
    void activateProfile(Host*);
    void setEditorTextoptions(bool isTabsAndSpacesToBeShown, bool isLinesAndParagraphsToBeShown);
    static bool loadLuaFunctionList();
    static bool loadEdbeeTheme(const QString& themeName, const QString& themeFile);

    // Used by a profile to tell the mudlet class
    // to tell other profiles to reload the updated
    // maps (via signal_profileMapReloadRequested(...))
    void requestProfilesToReloadMaps(QList<QString>);
    void showChangelogIfUpdated();

    bool showMapAuditErrors() const { return mshowMapAuditErrors; }
    void setShowMapAuditErrors(const bool);
    void setShowIconsOnMenu(const Qt::CheckState);
    void setGlobalStyleSheet(const QString& styleSheet);
    void setupPreInstallPackages(const QString& gameUrl);
    static bool unzip(const QString& archivePath, const QString& destination, const QDir& tmpDir);

    // This construct will be very useful for formatting tooltips and by
    // defining a static function/method here we can save using the same
    // QStringLiteral all over the place:
    static QString htmlWrapper(const QString& text) { return QStringLiteral("<html><head/><body>%1</body></html>").arg(text); }

    // From https://stackoverflow.com/a/14678964/4805858 an answer to:
    // "How to find and replace string?" by "Czarek Tomczak":
    static std::string replaceString(std::string subject, const std::string& search, const std::string& replace);

    static QString getMudletPath(mudletPathType, const QString& extra1 = QString(), const QString& extra2 = QString());
    // Used to enable "emergency" control recovery action - if Mudlet is
    // operating without either menubar or main toolbar showing.
    bool isControlsVisible() const;
    bool loadReplay(Host*, const QString&, QString* pErrMsg = nullptr);
    void show_options_dialog(const QString& tab);
    void setInterfaceLanguage(const QString &languageCode);
    const QString& getInterfaceLanguage() const { return mInterfaceLanguage; }
    const QLocale& getUserLocale() const { return mUserLocale; }
    QList<QString> getAvailableTranslationCodes() const { return mTranslationsMap.keys(); }
    void setEnableFullScreenMode(const bool);
    bool migratePasswordsToProfileStorage();
    bool storingPasswordsSecurely() const { return mStorePasswordsSecurely; }
    bool migratePasswordsToSecureStorage();
    static void setNetworkRequestDefaults(const QUrl& url, QNetworkRequest& request);

    // Both of these revises the contents of the .aff file: the first will
    // handle a .dic file that has been updated externally/manually (to add
    // or remove words) - it also puts the contents of the .dic file into the
    // supplied second argument; the second will replace the .dic file with just
    // the words in the supplied second argument and is to be used at the end of
    // a session to store away the user's changes:
    Hunhandle* prepareProfileDictionary(const QString&, QSet<QString>&);
    Hunhandle* prepareSharedDictionary();
    bool saveDictionary(const QString&, QSet<QString>&);
    QPair<bool, bool> addWordToSet(const QString&);
    QPair<bool, bool> removeWordFromSet(const QString&);
    QSet<QString> getWordSet();
    void scanForMudletTranslations(const QString&);
    void scanForQtTranslations(const QString&);
    void startAutoLogin(const QString&);
    int64_t getPhysicalMemoryTotal();
    const QMap<QByteArray, QString>& getEncodingNamesMap() const { return mEncodingNameMap; }
    void refreshTabBar();
    void updateDiscordNamedIcon();

    bool firstLaunch = false;
    // Needed to work around a (likely only Windows) issue:
    QString mBG_ONLY_STYLESHEET;
    QString mTEXT_ON_BG_STYLESHEET;

    FontManager mFontManager;
    Discord mDiscord;
    QPointer<QSettings> mpSettings;
    static const QString scmMudletXmlDefaultVersion;
    static QPointer<TConsole> mpDebugConsole;
    static QPointer<QMainWindow> mpDebugArea;
    static bool debugMode;
    int mToolbarIconSize;
    int mEditorTreeWidgetIconSize;
    bool mWindowMinimized;

    // used by developers in everyday coding
    static const bool scmIsDevelopmentVersion;
    // unofficial "nightly" build - still a type of a release
    static const bool scmIsPublicTestVersion;
    // final, official release
    static const bool scmIsReleaseVersion;

    static const QVersionNumber scmRunTimeQtVersion;
    // A constant equivalent to QDataStream::Qt_5_12 needed in several places
    // which can't be pulled from Qt as it is not going to be defined for older
    // versions:
    static const int scmQDataStreamFormat_5_12;

    QTime mReplayTime;
    int mReplaySpeed;
    QToolBar* mpMainToolBar;
    QString version;
    QPointer<Host> mpCurrentActiveHost;
    bool mAutolog;
    QList<QMediaPlayer*> mMusicBoxList;
    TTabBar* mpTabBar;
    QStringList packagesToInstallList;
    bool mIsLoadingLayout;
    static QVariantHash mLuaFunctionNames;
    bool mHasSavedLayout;
    QPointer<dlgAboutDialog> mpAboutDlg;
    QPointer<dlgConnectionProfiles> mConnectionDialog;
    // More modern Desktop styles no longer include icons on the buttons in
    // QDialogButtonBox buttons - but some users are using Desktops (KDE4?) that
    // does use them - use this flag to determine whether we should apply our
    // icons to override some of them:
    bool mShowIconsOnDialogs;
    // Value of QCoreApplication::testAttribute(Qt::AA_DontShowIconsInMenus) on
    // startup which the user may leave as is or force on or off:
    bool mShowIconsOnMenuOriginally;
    // This is the state for the tri-state control on the preferences and
    // means:
    // Qt::PartiallyChecked = use the previous state set on application start
    //    (set AA_DontShowIconsInMenus to inverse of mShowIconsOnMenuOriginally)
    // Qt::Unchecked = icons are not used on menus (set AA_DontShowIconsInMenus
    //    to false ourselves)
    // Qt::Checked = icons are used on menus (set AA_DontShowIconsInMenus to
    //    true ourselves)
    Qt::CheckState mShowIconsOnMenuCheckedState;

    // Used for editor area, but
    // only ::ShowTabsAndSpaces
    // and ::ShowLineAndParagraphSeparators
    // are considered/used/stored
    QTextOption::Flags mEditorTextOptions;

    QSystemTrayIcon mTrayIcon;

#if defined(INCLUDE_UPDATER)
    Updater* updater;
#endif


    // Currently tracks the "mudlet_option_use_smallscreen" file's existence but
    // may eventually migrate solely to the "EnableFullScreenMode" in the main
    // QSetting file - it is only stored as a file now to maintain backwards
    // compatibility...
    bool mEnableFullScreenMode;

    // approximate max duration that 'Copy as image' is allowed to take (seconds)
    int mCopyAsImageTimeout;

    QMap<QString, translation> mTranslationsMap;

    // translations done high enough will get a gold star to hide the last few percent
    // as well as encourage translators to maintain it;
    const int mTranslationGoldStar = 95;

    // A list of potential dictionary languages - probably will cover a much
    // wider range of languages compared to the translations - and is intended
    // for Dictionary identification - there is a request for users to submit
    // entries in their system if they do not appear in this and thus get
    // reported in the dictionary selection as the hunspell dictionary/affix
    // filename (e.g. a "xx" or "xx_YY" form rather than "words"):
    QHash<QString, QString>mDictionaryLanguageCodeMap;

    // This is used to keep track of where the main dictionary files are located
    // will be true if they are ones bundled with Mudlet, false if provided by
    // the system
    bool mUsingMudletDictionaries;

    // Options dialog when there's no active host
    QPointer<dlgProfilePreferences> mpDlgProfilePreferences;

    enum Appearance {
        system = 0,
        light = 1,
        dark = 2
    };
    Appearance mAppearance = Appearance::system;
    void setAppearance(Appearance, const bool& loading = false);
    bool inDarkMode() { return mDarkMode; };

    // mirror everything shown in any console to stdout. Helpful for CI environments
    inline static bool mMirrorToStdOut;

    struct GameDetails {
        QString hostUrl;
        int port;
        bool tlsEnabled;
        QString websiteInfo;
        QString icon;
    };

    // clang-format off
    inline static const OrderedMap<QString, GameDetails> scmDefaultGames = {
        {"Avalon.de", {"avalon.mud.de", 23, false,
                        "<center><a href='http://avalon.mud.de'>http://avalon.mud.de</a></center>",
                        ":/icons/avalon.png"}},
        {"Achaea", {"achaea.com", 23, false, "<center><a href='http://www.achaea.com/'>http://www.achaea.com</a></center>", ":/icons/achaea_120_30.png"}},
        {"3Kingdoms", {"3k.org", 3200, false, "<center><a href='http://www.3k.org/'>http://www.3k.org</a></center>", ":/icons/3klogo.png"}},
        {"3Scapes", {
            "3k.org",   // address to connect to
            3200,       // port to connect on
            false,      // secure connection possible?
            // game's website
            "<center><a href='http://www.3scapes.org/'>http://www.3scapes.org</a></center>",
            // path to the profile icon
            ":/icons/3slogo.png"
        }},
        {"Lusternia", {"lusternia.com", 23, false, "<center><a href='http://www.lusternia.com/'>http://www.lusternia.com</a></center>", ":/icons/lusternia_120_30.png"}},
        {"BatMUD", {"batmud.bat.org", 23, false, "<center><a href='http://www.bat.org'>http://www.bat.org</a></center>", ":/icons/batmud_mud.png"}},

        {"God Wars II", {"godwars2.org", 3000, false,
                        "<center><a href='http://www.godwars2.org'>http://www.godwars2.org</a></center>",
                        ":/icons/gw2.png"}},
        {"Slothmud", {"slothmud.org", 6101, false, "<center><a href='http://www.slothmud.org/'>http://www.slothmud.org/</a></center>", ":/icons/Slothmud.png"}},
        {"Aardwolf", {"aardmud.org", 4000, false, "<center><a href='http://www.aardwolf.com/'>http://www.aardwolf.com</a></center>", ":/icons/aardwolf_mud.png"}},
        {"Materia Magica", {"materiamagica.com", 23, false,
                        "<center><a href='http://www.materiamagica.com'>http://www.materiamagica.com</a></center>",
                        ":/materiaMagicaIcon"}},
        {"Realms of Despair", {"realmsofdespair.com", 4000, false, "<center><a href='http://www.realmsofdespair.com/'>http://www.realmsofdespair.com</a></center>", ":/icons/120x30RoDLogo.png"}},
        {"ZombieMUD", {"zombiemud.org", 3000, false, "<center><a href='http://www.zombiemud.org/'>http://www.zombiemud.org</a></center>", ":/icons/zombiemud.png"}},
        {"Aetolia", {"aetolia.com", 23, false, "<center><a href='http://www.aetolia.com/'>http://www.aetolia.com</a></center>", ":/icons/aetolia_120_30.png"}},
        {"Imperian", {"imperian.com", 23, false, "<center><a href='http://www.imperian.com/'>http://www.imperian.com</a></center>", ":/icons/imperian_120_30.png"}},
        {"WoTMUD", {"game.wotmud.org", 2224, false, "<center><a href='http://www.wotmud.org/'>Main website</a></center>\n"
                                 "<center><a href='http://www.wotmod.org/'>Forums</a></center>", ":/icons/wotmudicon.png"}},
        {"Midnight Sun 2", {"midnightsun2.org", 3000, false, "<center><a href='http://midnightsun2.org/'>http://midnightsun2.org/</a></center>", ":/icons/midnightsun2.png"}},
        {"Luminari", {"luminarimud.com", 4100, false, "<center><a href='http://www.luminarimud.com/'>http://www.luminarimud.com/</a></center>", ":/icons/luminari_icon.png"}},
        {"StickMUD", {"stickmud.com", 7680, false, "<center><a href='http://www.stickmud.com/'>stickmud.com</a></center>", ":/icons/stickmud_icon.jpg"}},
        {"Clessidra", {"mud.clessidra.it", 4000, false, "<center><a href='http://www.clessidra.it/'>http://www.clessidra.it</a></center>", ":/icons/clessidra.jpg"}},
        {"Reinos de Leyenda", {"reinosdeleyenda.es", 23, false, "<center><a href='https://www.reinosdeleyenda.es/'>Main website</a></center>\n"
                                 "<center><a href='https://www.reinosdeleyenda.es/foro/'>Forums</a></center>\n"
                                 "<center><a href='https://wiki.reinosdeleyenda.es/'>Wiki</a></center>\n", ":/icons/reinosdeleyenda_mud.png"}},
        {"Fierymud", {"fierymud.org", 4000, false, "<center><a href='https://www.fierymud.org/'>https://www.fierymud.org</a></center>", ":/icons/fiery_mud.png"}},
        {"Mudlet self-test", {"mudlet.org", 23, false, "", ""}},
        {"Carrion Fields", {"carrionfields.net", 4449, false, "<center><a href='http://www.carrionfields.net'>www.carrionfields.net</a></center>", ":/icons/carrionfields.png"}},
        {"Cleft of Dimensions", {"cleftofdimensions.net", 4354, false, "<center><a href='https://www.cleftofdimensions.net/'>cleftofdimensions.net</a></center>", ":/icons/cleftofdimensions.png"}},
        {"Legends of the Jedi", {"legendsofthejedi.com", 5656, false, "<center><a href='https://www.legendsofthejedi.com/'>legendsofthejedi.com</a></center>", ":/icons/legendsofthejedi_120x30.png"}},
        {"CoreMUD", {"coremud.org", 4020, true, "<center><a href='https://coremud.org/'>coremud.org</a></center>", ":/icons/coremud_icon.jpg"}},
        {"Multi-Users in Middle-earth", {"mume.org", 4242, true, "<center><a href='https://mume.org/'>mume.org</a></center>", ":/icons/mume.png"}},
    };
    // clang-format on

public slots:
    void processEventLoopHack_timerRun();
    void slot_mapper();
    void slot_replayTimeChanged();
    void slot_replaySpeedUp();
    void slot_replaySpeedDown();
    void toggleFullScreenView();
    void slot_show_about_dialog();
    void slot_show_help_dialog_video();
    void slot_show_help_dialog_forum();
    void slot_show_help_dialog_irc();
    void slot_open_mappingscripts_page();
    void slot_multi_view(const bool);
    void slot_toggle_multi_view();
    void slot_connection_dlg_finished(const QString& profile, bool connectOnLoad);
    void slot_timer_fires();
    void slot_replay();
    void slot_disconnect();
    void slot_notes();
    void slot_reconnect();
    void slot_close_profile_requested(int);
    void slot_irc();
    void slot_discord();
    void slot_mudlet_discord();
    void slot_package_manager();
    void slot_package_exporter();
    void slot_module_manager();
#if defined(INCLUDE_UPDATER)
    void slot_check_manual_update();
#endif
    void slot_restoreMainMenu() { setMenuBarVisibility(visibleAlways); }
    void slot_restoreMainToolBar() { setToolBarVisibility(visibleAlways); }
    void slot_handleToolbarVisibilityChanged(bool);
    void slot_newDataOnHost(const QString&, bool isLowerPriorityChange = false);


protected:
    void closeEvent(QCloseEvent* event) override;

signals:
    void signal_editorTextOptionsChanged(QTextOption::Flags);
    void signal_profileMapReloadRequested(QList<QString>);
    void signal_tabChanged(const QString& hostName);
    void signal_setToolBarIconSize(int);
    void signal_setTreeIconSize(int);
    void signal_hostCreated(Host*, quint8);
    void signal_hostDestroyed(Host*, quint8);
    void signal_appearanceChanged(Appearance);
    void signal_enableFulScreenModeChanged(bool);
    void signal_showMapAuditErrorsChanged(bool);
    void signal_menuBarVisibilityChanged(const controlsVisibility);
    void signal_toolBarVisibilityChanged(const controlsVisibility);
    void signal_showIconsOnMenusChanged(const Qt::CheckState);
    void signal_guiLanguageChanged(const QString&);
    void signal_passwordsMigratedToSecure();
    void signal_passwordMigratedToSecure(const QString&);
    void signal_passwordsMigratedToProfiles();


private slots:
    void slot_tab_changed(int);
    void show_help_dialog();
    void slot_show_connection_dialog();
    void show_editor_dialog();
    void show_trigger_dialog();
    void show_alias_dialog();
    void show_script_dialog();
    void show_timer_dialog();
    void show_action_dialog();
    void show_key_dialog();
    void show_variable_dialog();
    void slot_update_shortcuts();
    void slot_show_options_dialog();
#ifdef QT_GAMEPAD_LIB
    void slot_gamepadButtonPress(int deviceId, QGamepadManager::GamepadButton button, double value);
    void slot_gamepadButtonRelease(int deviceId, QGamepadManager::GamepadButton button);
    void slot_gamepadConnected(int deviceId);
    void slot_gamepadDisconnected(int deviceId);
    void slot_gamepadAxisEvent(int deviceId, QGamepadManager::GamepadAxis axis, double value);
#endif
#if defined(INCLUDE_UPDATER)
    void slot_update_installed();
    void slot_updateAvailable(const int);
    void slot_report_issue();
#endif
    void slot_toggle_compact_input_line();
    void slot_compact_input_line(const bool);
    void slot_password_migrated_to_secure(QKeychain::Job *job);
    void slot_password_migrated_to_profile(QKeychain::Job *job);
    void slot_tabMoved(const int oldPos, const int newPos);


private:
    void initEdbee();
    void goingDown() { mIsGoingDown = true; }
    bool scanDictionaryFile(QFile&, int&, QHash<QString, unsigned int>&, QStringList&);
    int scanWordList(QStringList&, QHash<QString, unsigned int>&);
    bool overwriteDictionaryFile(QFile&, const QStringList&);
    bool overwriteAffixFile(QFile&, QHash<QString, unsigned int>&);
    int getDictionaryWordCount(QFile&);
    QSettings* getQSettings();
    void loadTranslators(const QString &languageCode);
    void loadMaps();
    void migrateDebugConsole(Host* currentHost);
    QString autodetectPreferredLanguage();
    void installModulesList(Host*, QStringList);
    void setupTrayIcon();
    static bool desktopInDarkMode();

    QWidget* mpWidget_profileContainer;
    QHBoxLayout* mpHBoxLayout_profileContainer;

    static QPointer<mudlet> _self;
    QMap<Host*, QToolBar*> mUserToolbarMap;
    QMenu* restoreBar;
    bool mIsGoingDown;
    controlsVisibility mMenuBarVisibility;
    controlsVisibility mToolbarVisibility;

    QPointer<QAction> mpActionReplaySpeedDown;
    QPointer<QAction> mpActionReplaySpeedUp;
    QPointer<QAction> mpActionSpeedDisplay;
    QPointer<QAction> mpActionReplayTime;
    QPointer<QLabel> mpLabelReplaySpeedDisplay;
    QPointer<QLabel> mpLabelReplayTime;
    QPointer<QTimer> mpTimerReplay;
    QPointer<QToolBar> mpToolBarReplay;

    QPointer<QShortcut> triggersShortcut;
    QPointer<QShortcut> showMapShortcut;
    QPointer<QShortcut> inputLineShortcut;
    QPointer<QShortcut> optionsShortcut;
    QPointer<QShortcut> notepadShortcut;
    QPointer<QShortcut> packagesShortcut;
    QPointer<QShortcut> modulesShortcut;
    QPointer<QShortcut> multiViewShortcut;
    QPointer<QShortcut> connectShortcut;
    QPointer<QShortcut> disconnectShortcut;
    QPointer<QShortcut> reconnectShortcut;
    QKeySequence triggersKeySequence;
    QKeySequence showMapKeySequence;
    QKeySequence inputLineKeySequence;
    QKeySequence optionsKeySequence;
    QKeySequence notepadKeySequence;
    QKeySequence packagesKeySequence;
    QKeySequence modulesKeySequence;
    QKeySequence multiViewKeySequence;
    QKeySequence connectKeySequence;
    QKeySequence disconnectKeySequence;
    QKeySequence reconnectKeySequence;

    QPointer<QAction> mpActionReplay;

    QPointer<QAction> mpActionAbout;
    QPointer<QAction> mpActionAboutWithUpdates;
    QPointer<QToolButton> mpButtonAbout;
    QPointer<QAction> mpActionAliases;
    QPointer<QAction> mpActionButtons;
    QPointer<QToolButton> mpButtonConnect;
    QPointer<QAction> mpActionConnect;
    QPointer<QAction> mpActionDisconnect;
    QPointer<QAction> mpActionFullScreenView;
    QPointer<QAction> mpActionHelp;
    QPointer<QAction> mpActionDiscord;
    QPointer<QAction> mpActionMudletDiscord;
    QPointer<QAction> mpActionIRC;
    QPointer<QToolButton> mpButtonDiscord;
    QPointer<QAction> mpActionKeys;
    QPointer<QAction> mpActionMapper;
    QPointer<QAction> mpActionMultiView;
    QPointer<QAction> mpActionReportIssue;
    QPointer<QAction> mpActionNotes;
    QPointer<QAction> mpActionOptions;
    QPointer<QToolButton> mpButtonPackageManagers;
    QPointer<QAction> mpActionPackageManager;
    QPointer<QAction> mpActionModuleManager;
    QPointer<QAction> mpActionPackageExporter;
    QPointer<QAction> mpActionReconnect;
    QPointer<QAction> mpActionScripts;
    QPointer<QAction> mpActionTimers;
    QPointer<QAction> mpActionTriggers;
    QPointer<QAction> mpActionVariables;

    HostManager mHostManager;

    bool mshowMapAuditErrors;

    // Argument to QDateTime::toString(...) to format the elapsed time display
    // on the mpToolBarReplay:
    QString mTimeFormat;

    QString mDefaultStyle;

    // Has default form of "en_US" but can be just an ISO language code e.g. "fr" for french,
    // without a country designation. Replaces xx in "mudlet_xx.qm" to provide the translation
    // file for GUI translation
    QString mInterfaceLanguage {};
    // An encapsulation of the above in a form that Qt uses to hold all the
    // details:
    QLocale mUserLocale {};

    // The next pair retains the path argument supplied to the corresponding
    // scanForXxxTranslations(...) method so it is available to the subsequent
    // loadTranslators(...) call
    QString mQtTranslationsPathName;
    QString mMudletTranslationsPathName;
    QList<QPointer<QTranslator>> mTranslatorsLoadedList;

    // Points to the common mudlet dictionary handle once a profile has
    // requested it, then gets closed at termination of the application.
    Hunhandle* mHunspell_sharedDictionary;
    // The collection of words in the above:
    QSet<QString> mWordSet_shared;

    QString mMudletDiscordInvite = QStringLiteral("https://www.mudlet.org/chat");

    // a list of profiles currently being migrated to secure or profile storage
    QStringList mProfilePasswordsToMigrate {};

    bool mStorePasswordsSecurely {true};
    // Stores the translated names for the Encodings for the static and thus
    // const TBuffer::csmEncodingTable:
    QMap<QByteArray, QString> mEncodingNameMap;

    // Whether multi-view is in effect:
    bool mMultiView;

    // read-only value to see if the interface is light or dark. To set the value,
    // use setAppearance instead
    bool mDarkMode = false;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(mudlet::controlsVisibility)

class TConsoleMonitor : public QObject
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(TConsoleMonitor)
    TConsoleMonitor(QObject* parent) : QObject(parent) {}
protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
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
    translation(const int translationPercent = -1) : mTranslatedPercentage(translationPercent) {}

    const QString& getNativeName() const { return mNativeName; }
    const QString& getMudletTranslationFileName() const { return mMudletTranslationFileName; }
    const QString& getQtTranslationFileName() const { return mQtTranslationFileName; }
    const int& getTranslatedPercentage() const { return mTranslatedPercentage; }
    bool fromResourceFile() const { return mTranslatedPercentage >= 0; }

private:
    // Used for display in the profile preferences and is never translated:
    QString mNativeName;
    // ONLY if the translation is loaded from an embedded resource file,
    // this is the percentage complete of the translation
    int mTranslatedPercentage;
    // filename translation is loaded from
    QString mMudletTranslationFileName;
    // Qt translation file was found to be, note that in most
    // cases the loaded file will be a "xx" language only file even though it
    // is an "xx_YY" one here:
    QString mQtTranslationFileName;
    // Similar filename locations will require adding for any 3rd party translations we load
};

#endif // MUDLET_MUDLET_H
