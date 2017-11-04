#ifndef MUDLET_MUDLET_H
#define MUDLET_MUDLET_H

/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016 by Chris Leacy - cleacy1972@gmail.com              *
 *   Copyright (C) 2015-2016 by Stephen Lyons - slysven@virginmedia.com    *
 *   Copyright (C) 2016-2017 by Ian Adkins - ieadkins@gmail.com            *
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

#include "pre_guard.h"
#include "ui_main_window.h"
#include "edbee/views/texttheme.h"
#include <QFlags>
#include <QMainWindow>
#include <QMap>
#include <QMediaPlayer>
#include <QPointer>
#include <QQueue>
#include <QTextOption>
#include <QTime>
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
    void addSubWindow(TConsole* p);
    int getColumnNumber(Host* pHost, QString& name);
    int getLineNumber(Host* pHost, QString& name);
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
    bool setFontSize(Host*, const QString&, int);
    int getFontSize(Host*, const QString&);
    bool openWindow(Host*, const QString&, bool loadLayout = true);
    bool createMiniConsole(Host*, const QString&, int, int, int, int);
    bool createLabel(Host*, const QString&, int, int, int, int, bool);
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
    bool setTextFormat(Host*, const QString& name, int, int, int, int, int, int, bool, bool, bool, bool);
    bool setLabelClickCallback(Host*, const QString&, const QString&, const TEvent&);
    bool setLabelReleaseCallback(Host*, const QString&, const QString&, const TEvent&);
    bool setLabelOnEnter(Host*, const QString&, const QString&, const TEvent&);
    bool setLabelOnLeave(Host*, const QString&, const QString&, const TEvent&);
    bool moveWindow(Host*, const QString& name, int, int);
    void deleteLine(Host*, const QString& name);
    void insertText(Host*, const QString& name, const QString&);
    void replace(Host*, const QString& name, const QString&);
    int selectString(Host*, const QString& name, const QString& what, int);
    int selectSection(Host*, const QString& name, int, int);
    void setBold(Host*, const QString& name, bool);
    void setLink(Host* pHost, const QString& name, const QString& linkText, QStringList& linkFunction, QStringList&);
    void setItalics(Host*, const QString& name, bool);
    void setUnderline(Host*, const QString& name, bool);
    void setStrikeOut(Host*, const QString& name, bool);
    void setFgColor(Host*, const QString& name, int, int, int);
    void setBgColor(Host*, const QString& name, int, int, int);
    QString readProfileData(const QString& profile, const QString& item);
    bool setWindowWrap(Host* pHost, const QString& name, int& wrap);
    bool setWindowWrapIndent(Host* pHost, const QString& name, int& wrap);
    bool copy(Host* pHost, const QString& name);
    bool moveCursorEnd(Host*, const QString&);
    bool moveCursor(Host*, const QString&, int, int);
    int getLastLineNumber(Host*, const QString&);
    void readSettings();
    void writeSettings();
    bool openWebPage(const QString& path);
    void processEventLoopHack();
    static const QString scmMudletXmlDefaultVersion;
    static QPointer<TConsole> mpDebugConsole;
    static QMainWindow* mpDebugArea;
    static bool debugMode;
    QMap<Host*, TConsole*> mConsoleMap;
    QMap<Host*, QMap<QString, TConsole*>> mHostConsoleMap;
    QMap<Host*, QMap<QString, TDockWidget*>> mHostDockConsoleMap;
    QMap<Host*, QMap<QString, TLabel*>> mHostLabelMap;
    QIcon* testicon;
    bool mShowMenuBar;
    bool mShowToolbar;
    bool isGoingDown() { return mIsGoingDown; }
    int mToolbarIconSize;
    int mEditorTreeWidgetIconSize;
    void setToolBarIconSize(const int);
    void setEditorTreeWidgetIconSize(const int);
    void setToolBarVisible(const bool);
    void setMenuBarVisible(const bool);
    void replayStart();
    bool setConsoleBufferSize(Host* pHost, const QString& name, int x1, int y1);
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
    QTime mReplayTime;
    int mReplaySpeed;
    QToolBar* mpMainToolBar;
    QMap<QTimer*, TTimer*> mTimerMap;
    QMap<Host*, QPointer<dlgIRC>> mpIrcClientMap;
    QString version;
    QPointer<Host> mpCurrentActiveHost;
    bool mAutolog;
    QList<QMediaPlayer*> mMusicBoxList;
    QTabBar* mpTabBar;
    QStringList packagesToInstallList;
    bool mIsLoadingLayout;
    bool mHasSavedLayout;
    QMap<Host*, QList<QString>> mHostDockLayoutChangeMap;
    QMap<Host*, QList<TToolBar*>> mHostToolbarLayoutChangeMap;
    QPointer<dlgAboutDialog> mpAboutDlg;
    QPointer<QDialog> mpModuleDlg;
    QPointer<QDialog> mpPackageManagerDlg;
    QPointer<dlgProfilePreferences> mpProfilePreferencesDlg;

    // Used for editor area, but
    // only ::ShowTabsAndSpaces
    // and ::ShowLineAndParagraphSeparators
    // are considered/used/stored
    QTextOption::Flags mEditorTextOptions;
    void setEditorTextoptions(const bool isTabsAndSpacesToBeShown, const bool isLinesAndParagraphsToBeShown);
    static bool loadEdbeeTheme(const QString &themeName, const QString &themeFile);

    // Used by a profile to tell the mudlet class
    // to tell other profiles to reload the updated
    // maps (via signal_profileMapReloadRequested(...))
    void requestProfilesToReloadMaps(QList<QString>);

    bool showMapAuditErrors() const { return mshowMapAuditErrors; }
    void setShowMapAuditErrors(const bool state) { mshowMapAuditErrors = state; }
    bool compactInputLine() const { return mCompactInputLine; }
    void setCompactInputLine(const bool state) { mCompactInputLine = state; }
    void createMapper(bool loadDefaultMap = true);

    static bool unzip(const QString &archivePath, const QString &destination, const QDir &tmpDir);

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
        moduleBackupsPath
    };
    static QString getMudletPath(const mudletPathType, const QString& extra1 = QString(), const QString& extra2 = QString());

public slots:
    void processEventLoopHack_timerRun();
    void slot_mapper();
    void slot_replayTimeChanged();
    void slot_replaySpeedUp();
    void slot_replaySpeedDown();
    void toggleFullScreenView();
    void slot_userToolBar_orientation_changed(Qt::Orientation);
    void slot_show_about_dialog();
    void slot_show_help_dialog_video();
    void slot_show_help_dialog_forum();
    void slot_show_help_dialog_irc();
    void slot_show_help_dialog_download();
    void slot_open_mappingscripts_page();
    void slot_module_clicked(QTableWidgetItem*);
    void slot_module_changed(QTableWidgetItem*);
    void slot_multi_view();
    void slot_userToolBar_hovered(QAction* pA);
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

protected:
    void closeEvent(QCloseEvent* event) override;

signals:
    void signal_editorTextOptionsChanged(QTextOption::Flags);
    void signal_profileMapReloadRequested(QList<QString>);
    void signal_setToolBarIconSize(const int);
    void signal_setTreeIconSize(const int);

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
    void show_options_dialog();
#ifdef QT_GAMEPAD_LIB
    void slot_gamepadButtonPress(int deviceId, QGamepadManager::GamepadButton button, double value);
    void slot_gamepadButtonRelease(int deviceId, QGamepadManager::GamepadButton button);
    void slot_gamepadConnected(int deviceId);
    void slot_gamepadDisconnected(int deviceId);
    void slot_gamepadAxisEvent(int deviceId, QGamepadManager::GamepadAxis axis, double value);
#endif
    void slot_module_manager_destroyed();

private:
    void initEdbee();

    void goingDown() { mIsGoingDown = true; }
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

    QAction* actionReplaySpeedDown;
    QAction* actionReplaySpeedUp;
    QAction* actionSpeedDisplay;
    QAction* actionReplayTime;
    QLabel* replaySpeedDisplay;
    QLabel* replayTime;
    QTimer* replayTimer;
    QToolBar* replayToolBar;

    QAction* actionReconnect;

    void check_for_mappingscript();

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
};

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
