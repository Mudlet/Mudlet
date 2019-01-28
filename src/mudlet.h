#ifndef MUDLET_MUDLET_H
#define MUDLET_MUDLET_H

/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016 by Chris Leacy - cleacy1972@gmail.com              *
 *   Copyright (C) 2015-2016, 2018 by Stephen Lyons                        *
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
#include "TBuffer.h" // Needed for TChar details

#include "edbee/views/texttheme.h"
#include "ui_main_window.h"
#if defined(INCLUDE_UPDATER)
#include "updater.h"
#endif

#include "discord.h"

#include "pre_guard.h"
#include <QFlags>
#include <QMainWindow>
#include <QMap>
#include <QMediaPlayer>
#include <QPointer>
#include <QProxyStyle>
#include <QQueue>
#include <QSettings>
#include <QTextOption>
#include <QTime>
#include <QTimer>
#include "edbee/models/textautocompleteprovider.h"
#include <QShortcut>
#include <QKeySequence>
#ifdef QT_GAMEPAD_LIB
#include <QGamepad>
#endif
#include "post_guard.h"

#include <assert.h>

class QAction;
class QCloseEvent;
class QMenu;
class QLabel;
class QListWidget;
class QPushButton;
class QTableWidget;
class QTableWidgetItem;
class QTextEdit;
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
class dlgProfilePreferences;

class mudlet : public QMainWindow, public Ui::main_window
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(mudlet)
    mudlet();
    ~mudlet();
    static mudlet* self();
    // This method allows better debugging when mudlet::self() is called inappropriately.
    static void start();
    HostManager& getHostManager() { return mHostManager; }    
    FontManager mFontManager;
    Discord mDiscord;
    QPointer<QSettings> mpSettings;
    void addSubWindow(TConsole* p);
    int getColumnNumber(Host* pHost, QString& name);
    std::pair<bool, int> getLineNumber(Host* pHost, QString& windowName);
    void printSystemMessage(Host* pH, const QString& s);
    void print(Host*, const QString&);
    void addConsoleForNewHost(Host* pH);
    void disableToolbarButtons();
    void enableToolbarButtons();
    Host* getActiveHost();
    void registerTimer(TTimer*, QTimer*);
    void unregisterTimer(QTimer*);
    void forceClose();
    bool saveWindowLayout();
    bool loadWindowLayout();
    void setDockLayoutUpdated(Host*, const QString&);
    void setToolbarLayoutUpdated(Host*, TToolBar*);
    void commitLayoutUpdates();
    bool setWindowFont(Host*, const QString&, const QString&);
    QString getWindowFont(Host*, const QString&);
    bool setWindowFontSize(Host *, const QString &, int);
    int getFontSize(Host*, const QString&);
    QSize calcFontSize(Host* pHost, const QString& windowName);
    bool openWindow(Host*, const QString&, bool loadLayout = true);
    bool createMiniConsole(Host*, const QString&, int, int, int, int);
    bool createLabel(Host* pHost, const QString& name, int x, int y, int width, int height, bool fillBg,
                         bool clickthrough);
    bool echoWindow(Host*, const QString&, const QString&);
    bool echoLink(Host* pHost, const QString& name, const QString& text, QStringList&, QStringList&, bool customFormat = false);
    void insertLink(Host*, const QString&, const QString&, QStringList&, QStringList&, bool customFormat = false);
    bool appendBuffer(Host*, const QString&);
    bool createBuffer(Host*, const QString&);
    bool showWindow(Host*, const QString&);
    bool hideWindow(Host*, const QString&);
    bool paste(Host*, const QString&);
    bool closeWindow(Host*, const QString&);
    bool resizeWindow(Host*, const QString&, int, int);
    bool clearWindow(Host*, const QString&);
    bool pasteWindow(Host* pHost, const QString& name);
    bool setBackgroundColor(Host*, const QString& name, int r, int g, int b, int alpha);
    bool setBackgroundImage(Host*, const QString& name, QString& path);
    bool setTextFormat(Host*, const QString& name, const QColor &bgColor, const QColor &fgColor, const TChar::AttributeFlags attributes = TChar::None);
    bool setDisplayAttributes(Host* pHost, const QString& name, const TChar::AttributeFlags attributes, const bool state);
    bool setLabelClickCallback(Host*, const QString&, const QString&, const TEvent&);
    bool setLabelDoubleClickCallback(Host*, const QString&, const QString&, const TEvent&);
    bool setLabelReleaseCallback(Host*, const QString&, const QString&, const TEvent&);
    bool setLabelMoveCallback(Host*, const QString&, const QString&, const TEvent&);
    bool setLabelWheelCallback(Host*, const QString&, const QString&, const TEvent&);
    bool setLabelOnEnter(Host*, const QString&, const QString&, const TEvent&);
    bool setLabelOnLeave(Host*, const QString&, const QString&, const TEvent&);
    bool moveWindow(Host*, const QString& name, int, int);
    void deleteLine(Host*, const QString& name);
    bool insertText(Host*, const QString& windowName, const QString&);
    void replace(Host*, const QString& name, const QString&);
    int selectString(Host*, const QString& name, const QString& what, int);
    int selectSection(Host*, const QString& name, int, int);
    void setLink(Host* pHost, const QString& name, QStringList& linkFunction, QStringList&);
    std::tuple<bool, QString, int, int> getSelection(Host* pHost, const QString& name);
    void setFgColor(Host*, const QString& name, int, int, int);
    void setBgColor(Host*, const QString& name, int, int, int);
    QString readProfileData(const QString& profile, const QString& item);
    bool setWindowWrap(Host* pHost, const QString& name, int& wrap);
    bool setWindowWrapIndent(Host* pHost, const QString& name, int& wrap);
    bool copy(Host* pHost, const QString& name);
    bool moveCursorEnd(Host*, const QString&);
    bool moveCursor(Host*, const QString&, int, int);
    int getLastLineNumber(Host*, const QString&);
    void readEarlySettings(const QSettings&);
    void readLateSettings(const QSettings&);
    void writeSettings();
    bool openWebPage(const QString& path);
    void checkUpdatesOnStart();
    void processEventLoopHack();
    static const QString scmMudletXmlDefaultVersion;
    static QPointer<TConsole> mpDebugConsole;
    static QPointer<QMainWindow> mpDebugArea;
    static bool debugMode;
    QMap<Host*, TConsole*> mConsoleMap;
    bool isGoingDown() { return mIsGoingDown; }
    int mToolbarIconSize;
    int mEditorTreeWidgetIconSize;
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
    bool setConsoleBufferSize(Host* pHost, const QString& name, int x1, int y1);
    bool setScrollBarVisible(Host* pHost, const QString& name, bool isVisible);
    bool setClickthrough(Host* pHost, const QString& name, bool clickthrough);
    void replayOver();
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    bool resetFormat(Host*, QString& name);
    bool moduleTableVisible();
    bool mWindowMinimized;
    void doAutoLogin(const QString&);
    bool deselect(Host* pHost, const QString& name);
    void stopSounds();
    void playSound(QString s, int);
    int getColumnCount(Host* pHost, QString& name);
    int getRowCount(Host* pHost, QString& name);
    QStringList getAvailableFonts();
    void hideMudletsVariables(Host *pHost);

    static const bool scmIsDevelopmentVersion;
    QTime mReplayTime;
    int mReplaySpeed;
    QToolBar* mpMainToolBar;
    QMap<QTimer*, TTimer*> mTimerMap;
    QMap<Host*, QPointer<dlgIRC>> mpIrcClientMap;
    QString version;
    QPointer<Host> mpCurrentActiveHost;
    bool mAutolog;
    QList<QMediaPlayer*> mMusicBoxList;
    TTabBar* mpTabBar;
    QStringList packagesToInstallList;
    bool mIsLoadingLayout;
    static QVariantHash mLuaFunctionNames;
    bool mHasSavedLayout;
    QMap<Host*, QList<QString>> mHostDockLayoutChangeMap;
    QMap<Host*, QList<TToolBar*>> mHostToolbarLayoutChangeMap;
    QPointer<dlgAboutDialog> mpAboutDlg;
    QPointer<QDialog> mpModuleDlg;
    QPointer<QDialog> mpPackageManagerDlg;
    QMap<Host*, QPointer<dlgProfilePreferences>> mpProfilePreferencesDlgMap;
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
    bool compactInputLine() const { return mCompactInputLine; }
    void setCompactInputLine(const bool state) { mCompactInputLine = state; }
    void createMapper(bool loadDefaultMap = true);
    void setShowIconsOnMenu(const Qt::CheckState);

    static bool unzip(const QString& archivePath, const QString& destination, const QDir& tmpDir);

    // This construct will be very useful for formatting tooltips and by
    // defining a static function/method here we can save using the same
    // QStringLiteral all over the place:
    static QString htmlWrapper(const QString& text) { return QStringLiteral("<html><head/><body>%1</body></html>").arg(text); }

    // From https://stackoverflow.com/a/14678964/4805858 an answer to:
    // "How to find and replace string?" by "Czarek Tomczak":
    static std::string replaceString(std::string subject, const std::string& search, const std::string& replace);

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
        // for the profile game save XML files - ends in a '/':
        profileXmlFilesPath,
        // Takes one extra argument (profile name) that returns the directory
        // for the profile game save maps files - does NOT end in a '/'
        profileMapsPath,
        // Takes two extra arguments (profile name, dataTime stamp) that returns
        // the pathFile name for a dateTime stamped map file:
        profileDateTimeStampedMapPathFileName,
        // Takes two extra arguments (profile name, mapFileName) that returns
        // the pathFile name for any map file:
        profileMapPathFileName,
        // Takes one extra argument (profile name) that returns the pathFile
        // name for the downloaded IRE Server provided XML map:
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
        qtTranslationsPath
    };
    static QString getMudletPath(mudletPathType, const QString& extra1 = QString(), const QString& extra2 = QString());
    // Used to enable "emergency" control recovery action - if Mudlet is
    // operating without either menubar or main toolbar showing.
    bool isControlsVisible() const;
    bool loadReplay(Host*, const QString&, QString* pErrMsg = nullptr);    
    void show_options_dialog(QString tab);
    void setInterfaceLanguage(const QString &languageCode);
    QList<QString> getAvailableTranslationCodes() const { return mTranslatorsMap.keys(); }
    QPair<bool, QStringList> getLines(Host* pHost, const QString& windowName, const int lineFrom, const int lineTo);

#if defined(INCLUDE_UPDATER)
    Updater* updater;
#endif

    void setEnableFullScreenMode(const bool);

    // Currently tracks the "mudlet_option_use_smallscreen" file's existance but
    // may eventually migrate solely to the "EnableFullScreenMode" in the main
    // QSetting file - it is only stored as a file now to maintain backwards
    // compatibility...
    bool mEnableFullScreenMode;

    // approximate max duration that 'Copy as image' is allowed to take (seconds)
    int mCopyAsImageTimeout;

    // Has default form of "en_US" but can be just an ISO langauge code e.g. "fr" for french,
    // without a country designation. Replaces xx in "mudlet_xx.qm" to provide the translation
    // file for GUI translation
    QString mInterfaceLanguage;

    // ISO language code, translated human-readable name w/ English, and percent translated
    // ie: ru_RU, Русский (Russian), 75
    QHash<QString, std::pair<QString, int>> mLanguageCodeMap;
    // translations done high enough will get a gold star to hide the last few percent
    // as well as encourage translators to maintain it;
    const int mTranslationStar = 95;

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
    void slot_module_clicked(QTableWidgetItem*);
    void slot_module_changed(QTableWidgetItem*);
    void slot_multi_view();
    void slot_connection_dlg_finished(const QString& profile, int historyVersion);
    void slot_timer_fires();
    void slot_send_login();
    void slot_send_pass();
    void slot_replay();
    void slot_disconnect();
    void slot_notes();
    void slot_reconnect();
    void slot_close_profile_requested(int);
    void startAutoLogin();
    void slot_irc();
    void slot_uninstall_package();
    void slot_install_package();
    void slot_package_manager();
    void slot_package_exporter();
    void slot_uninstall_module();
    void slot_install_module();
    void slot_module_manager();
    void layoutModules();
    void slot_help_module();
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
    void signal_enableFulScreenModeChanged(bool);
    void signal_showMapAuditErrorsChanged(bool);
    void signal_menuBarVisibilityChanged(const controlsVisibility);
    void signal_toolBarVisibilityChanged(const controlsVisibility);
    void signal_showIconsOnMenusChanged(const Qt::CheckState);

private slots:
    void slot_close_profile();
    void slot_tab_changed(int);
    void show_help_dialog();
    void slot_show_connection_dialog();
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
    void slot_module_manager_destroyed();
#if defined(INCLUDE_UPDATER)
    void slot_update_installed();
#endif

private:
    void initEdbee();
    void goingDown() { mIsGoingDown = true; }
    void loadTranslators();

    QMap<QString, TConsole*> mTabMap;
    QWidget* mainPane;

    QPointer<Host> mpDefaultHost;
    QQueue<QString> tempLoginQueue;
    QQueue<QString> tempPassQueue;
    QQueue<Host*> tempHostQueue;
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

    void check_for_mappingscript();

    QPointer<QAction> mpActionReplay;

    QPointer<QAction> mpActionAbout;
    QPointer<QAction> mpActionAliases;
    QPointer<QAction> mpActionButtons;
    QPointer<QAction> mpActionConnect;
    QPointer<QAction> mpActionDisconnect;
    QPointer<QAction> mpActionFullScreenView;
    QPointer<QAction> mpActionHelp;
    QPointer<QAction> mpActionIRC;
    QPointer<QAction> mpActionKeys;
    QPointer<QAction> mpActionMapper;
    QPointer<QAction> mpActionModuleManager;
    QPointer<QAction> mpActionMultiView;
    QPointer<QAction> mpActionNotes;
    QPointer<QAction> mpActionOptions;
    QPointer<QAction> mpActionPackageManager;
    QPointer<QAction> mpActionReconnect;
    QPointer<QAction> mpActionScripts;
    QPointer<QAction> mpActionTimers;
    QPointer<QAction> mpActionTriggers;
    QPointer<QAction> mpActionVariables;

    QPointer<QListWidget> packageList;
    QPointer<QPushButton> uninstallButton;
    QPointer<QPushButton> installButton;

    QPointer<Host> mpModuleTableHost;
    QPointer<QTableWidget> moduleTable;
    QPointer<QPushButton> moduleUninstallButton;
    QPointer<QPushButton> moduleInstallButton;
    QPointer<QPushButton> moduleHelpButton;

    HostManager mHostManager;

    bool mshowMapAuditErrors;

    bool mCompactInputLine;
    void slot_toggle_compact_input_line();
    void set_compact_input_line();

    QSettings* getQSettings();

    // Argument to QDateTime::toString(...) to format the elapsed time display
    // on the mpToolBarReplay:
    QString mTimeFormat;

    // QMap has key of interface languages (in format of mInterfaceLanguage)
    // value: a QList of QPointers to all the translators needed (mudlet + Qt)
    // for the specific GUI Language, on language change to remove
    // the translators for the old settings and add the ones for
    // the new language
    QMap<QString, QList<QPointer <QTranslator>>> mTranslatorsMap;
    QList<QPointer<QTranslator>> mTranslatorsLoadedList;
    void loadTranslationFile(const QString& translationFileName, const QString &filePath, QString &languageCode);
    void loadLanguagesMap();
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


#endif // MUDLET_MUDLET_H
