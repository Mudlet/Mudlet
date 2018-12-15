#ifndef MUDLET_HOST_H
#define MUDLET_HOST_H

/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2015-2018 by Stephen Lyons - slysven@virginmedia.com    *
 *   Copyright (C) 2016 by Ian Adkins - ieadkins@gmail.com                 *
 *   Copyright (C) 2018 by Huadong Qi - novload@outlook.com                *
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


#include "ActionUnit.h"
#include "AliasUnit.h"
#include "KeyUnit.h"
#include "ScriptUnit.h"
#include "TLuaInterpreter.h"
#include "TimerUnit.h"
#include "TriggerUnit.h"
#include "XMLexport.h"
#include "ctelnet.h"

#include "pre_guard.h"
#include <QColor>
#include <QFile>
#include <QFont>
#include <QPointer>
#include <QTextStream>
#include "post_guard.h"

class QDialog;
class QDockWidget;
class QPushButton;
class QListWidget;

class dlgTriggerEditor;
class TEvent;
class TArea;
class LuaInterface;
class TMap;
class TRoom;
class TConsole;
class dlgNotepad;
class TMap;


class Host : public QObject
{
    Q_OBJECT

    friend class XMLexport;
    friend class XMLimport;
    friend class dlgProfilePreferences;

public:
    Host(int port, const QString& mHostName, const QString& login, const QString& pass, int host_id);
    ~Host();

    enum DiscordOptionFlag {
        DiscordNoOption = 0x0,
        DiscordSetDetail = 0x01,
        DiscordSetState = 0x02,
        DiscordSetLargeIcon = 0x04,
        DiscordSetLargeIconText = 0x08,
        DiscordSetSmallIcon = 0x10,
        DiscordSetSmallIconText = 0x20,
        DiscordSetPartyInfo = 0x80,
        DiscordSetTimeInfo = 0x100,
        DiscordSetSubMask = 0x3ff,
        DiscordLuaAccessEnabled = 0x800
    };
    Q_DECLARE_FLAGS(DiscordOptionFlags, DiscordOptionFlag)


    QString            getName()                        { QMutexLocker locker(& mLock); return mHostName; }
    void               setName(const QString& s )       { QMutexLocker locker(& mLock); mHostName = s; }
    QString            getUrl()                         { QMutexLocker locker(& mLock); return mUrl; }
    void               setUrl(const QString& s )        { QMutexLocker locker(& mLock); mUrl = s; }
    QString            getUserDefinedName()             { QMutexLocker locker(& mLock); return mUserDefinedName; }
    void               setUserDefinedName(const QString& s ) { QMutexLocker locker(& mLock); mUserDefinedName = s; }
    int                getPort()                        { QMutexLocker locker(& mLock); return mPort; }
    void               setPort( int p )                 { QMutexLocker locker(& mLock); mPort = p; }
    QString &          getLogin()                       { QMutexLocker locker(& mLock); return mLogin; }
    void               setLogin(const QString& s )      { QMutexLocker locker(& mLock); mLogin = s; }
    QString &          getPass()                        { QMutexLocker locker(& mLock); return mPass; }
    void               setPass(const QString& s )       { QMutexLocker locker(& mLock); mPass = s; }
    int                getRetries()                     { QMutexLocker locker(& mLock); return mRetries;}
    void               setRetries( int c )              { QMutexLocker locker(& mLock); mRetries=c; }
    int                getTimeout()                     { QMutexLocker locker(& mLock); return mTimeout; }
    void               setTimeout( int seconds )        { QMutexLocker locker(& mLock); mTimeout=seconds; }
    bool               wideAmbiguousEAsianGlyphs() { QMutexLocker locker(& mLock); return mWideAmbigousWidthGlyphs; }
    // Uses PartiallyChecked to set the automatic mode, otherwise Checked/Unchecked means use wide/narrow ambiguous glyphs
    void               setWideAmbiguousEAsianGlyphs(Qt::CheckState state );
    // Is used to set preference dialog control directly:
    Qt::CheckState     getWideAmbiguousEAsianGlyphsControlState() { QMutexLocker locker(& mLock);
                                                                       return mAutoAmbigousWidthGlyphsSetting
                                                                               ? Qt::PartiallyChecked
                                                                               : (mWideAmbigousWidthGlyphs ? Qt::Checked : Qt::Unchecked); }
    void               setDiscordApplicationID(const QString& s);
    const QString&     getDiscordApplicationID();

    void closingDown();
    bool isClosingDown();
    const unsigned int assemblePath();
    const bool checkForMappingScript();

    TriggerUnit* getTriggerUnit() { return &mTriggerUnit; }
    TimerUnit* getTimerUnit() { return &mTimerUnit; }
    AliasUnit* getAliasUnit() { return &mAliasUnit; }
    ActionUnit* getActionUnit() { return &mActionUnit; }
    KeyUnit* getKeyUnit() { return &mKeyUnit; }
    ScriptUnit* getScriptUnit() { return &mScriptUnit; }

    void connectToServer();
    void send(QString cmd, bool wantPrint = true, bool dontExpandAliases = false);

    int getHostID()
    {
        QMutexLocker locker(&mLock);
        return mHostID;
    }

    void setHostID(int id)
    {
        QMutexLocker locker(&mLock);
        mHostID = id;
    }

    TLuaInterpreter* getLuaInterpreter() { return &mLuaInterpreter; }
    LuaInterface* getLuaInterface() { return mLuaInterface.data(); }

    void incomingStreamProcessor(const QString& paragraph, int line);
    void postIrcMessage(const QString&, const QString&, const QString&);
    void enableTimer(const QString&);
    void disableTimer(const QString&);
    void enableTrigger(const QString&);
    void disableTrigger(const QString&);
    void enableKey(const QString&);
    void disableKey(const QString&);
    bool killTimer(const QString&);
    bool killTrigger(const QString&);
    double stopStopWatch(int);
    bool resetStopWatch(int);
    bool startStopWatch(int);
    double getStopWatchTime(int);
    int createStopWatch();
    void startSpeedWalk();
    void saveModules(int sync, bool backup = true);
    void reloadModule(const QString& reloadModuleName);
    bool blockScripts() { return mBlockScriptCompile; }
    void refreshPackageFonts();

    void registerEventHandler(const QString&, TScript*);
    void registerAnonymousEventHandler(const QString& name, const QString& fun);
    void unregisterEventHandler(const QString&, TScript*);
    void raiseEvent(const TEvent& event);
    void resetProfile();
    std::tuple<bool, QString, QString> saveProfile(const QString& saveLocation = QString(), const QString& saveName = QString(), bool syncModules = false);
    std::tuple<bool, QString, QString> saveProfileAs(const QString& fileName);
    void stopAllTriggers();
    void reenableAllTriggers();

    // get Search Engine
    QPair<QString, QString> getSearchEngine();

    void set_USE_IRE_DRIVER_BUGFIX(bool b)
    {
        mUSE_IRE_DRIVER_BUGFIX = b;
        mTelnet.set_USE_IRE_DRIVER_BUGFIX(b);
    }

    void set_LF_ON_GA(bool b)
    {
        mLF_ON_GA = b;
        mTelnet.set_LF_ON_GA(b);
    }

    void adjustNAWS();

    bool installPackage(const QString&, int);
    bool uninstallPackage(const QString&, int);
    bool removeDir(const QString&, const QString&);
    void readPackageConfig(const QString&, QString&);
    void postMessage(const QString message) { mTelnet.postMessage(message); }
    QPair<bool, QString> writeProfileData(const QString&, const QString&);
    QString readProfileData(const QString&);
    void xmlSaved(const QString& xmlName);
    bool currentlySavingProfile();
    void processDiscordGMCP(const QString& packageMessage, const QString& data);
    void waitForProfileSave();
    void clearDiscordData();
    void processDiscordMSDP(const QString& variable, QString value);
    bool discordUserIdMatch(const QString& userName, const QString& userDiscriminator) const;
    void setMmpMapLocation(const QString& data);
    QString getMmpMapLocation() const;

    cTelnet mTelnet;
    QPointer<TConsole> mpConsole;
    TLuaInterpreter mLuaInterpreter;

    int commandLineMinimumHeight;
    bool mAlertOnNewData;
    bool mAllowToSendCommand;
    bool mAutoClearCommandLineAfterSend;
    bool mBlockScriptCompile;
    bool mEchoLuaErrors;
    int mBorderBottomHeight;
    int mBorderLeftWidth;
    int mBorderRightWidth;
    int mBorderTopHeight;
    QFont mCommandLineFont;
    QString mCommandSeparator;
    QFont mDisplayFont;
    bool mEnableGMCP;
    bool mEnableMSDP;
    QTextStream mErrorLogStream;
    QMap<QString, QList<TScript*>> mEventHandlerMap;
    bool mFORCE_GA_OFF;
    bool mFORCE_NO_COMPRESSION;
    bool mFORCE_SAVE_ON_EXIT;
    bool mInsertedMissingLF;
    bool mIsGoingDown;
    bool mIsProfileLoadingSequence;

    bool mLF_ON_GA;
    bool mNoAntiAlias;

    dlgTriggerEditor* mpEditorDialog;
    QScopedPointer<TMap> mpMap;
    dlgNotepad* mpNotePad;

    bool mPrintCommand;

    // To cover the corner case of the user changing the mode
    // whilst a log is being written, this stores the mode of
    // the current log file and is set from
    // mIsNextLogFileInHtmlFormat at the point that a log is started.
    bool mIsCurrentLogFileInHtmlFormat;

    // To cover the corner case of the user changing the mode
    // whilst a log is being written, this stores the mode of
    // future logs file as set in the profile preferences. See
    // also mIsCurrentLogFileInHtmlFormat.
    bool mIsNextLogFileInHtmlFormat;

    bool mIsLoggingTimestamps;

    // Where to put HTML/text logfile (default is the "Logs" under the profile's
    // one):
    QString mLogDir;
    // A user entered name - if blank a language specific default is used:
    QString mLogFileName;
    // The first argument to QDateTime::toString(...) to generate a date/time
    // dependent filename unless it is empty in which case the above value is
    // used - the previously used value of "yyyy-MM-dd#hh-mm-ss" has been
    // changed to "yyyy-MM-dd#HH-mm-ss" and is set as a default in the
    // constructor:
    QString mLogFileNameFormat;

    bool mResetProfile;
    int mScreenHeight;
    int mScreenWidth;

    int mTimeout;

    QString mUrl;

    bool mUSE_FORCE_LF_AFTER_PROMPT;
    bool mUSE_IRE_DRIVER_BUGFIX;
    bool mUSE_UNIX_EOL;
    int mWrapAt;
    int mWrapIndentCount;

    // code editor theme (human-friendly name)
    QString mEditorTheme;
    // code editor theme file on disk for edbee to load
    QString mEditorThemeFile;

    // search engine URL prefix to search query
    QMap<QString, QString> mSearchEngineData;
    QString mSearchEngineName;

    // trigger/alias/script/etc ID whose Lua code to show when previewing a theme
    // remembering this value to show what the user has selected does have its
    // flaws in case of items getting created/deleted, but this is just a
    // convenience feature and if it gets the item wrong, it's no worse
    // than the feature not being there.
    int mThemePreviewItemID;
    // the type of item (a trigger, an alias, etc) that's previewed
    QString mThemePreviewType;

    QColor mBlack;
    QColor mLightBlack;
    QColor mRed;
    QColor mLightRed;
    QColor mLightGreen;
    QColor mGreen;
    QColor mLightBlue;
    QColor mBlue;
    QColor mLightYellow;
    QColor mYellow;
    QColor mLightCyan;
    QColor mCyan;
    QColor mLightMagenta;
    QColor mMagenta;
    QColor mLightWhite;
    QColor mWhite;
    QColor mFgColor;
    QColor mBgColor;
    QColor mCommandBgColor;
    QColor mCommandFgColor;

    QColor mBlack_2;
    QColor mLightBlack_2;
    QColor mRed_2;
    QColor mLightRed_2;
    QColor mLightGreen_2;
    QColor mGreen_2;
    QColor mLightBlue_2;
    QColor mBlue_2;
    QColor mLightYellow_2;
    QColor mYellow_2;
    QColor mLightCyan_2;
    QColor mCyan_2;
    QColor mLightMagenta_2;
    QColor mMagenta_2;
    QColor mLightWhite_2;
    QColor mWhite_2;
    QColor mFgColor_2;
    QColor mBgColor_2;
    bool mMapStrongHighlight;
    QStringList mGMCP_merge_table_keys;
    QString mSpellDic;
    bool mLogStatus;
    bool mEnableSpellCheck;
    QStringList mInstalledPackages;
    QMap<QString, QStringList> mInstalledModules;
    QMap<QString, int> mModulePriorities;
    QMap<QString, QStringList> modulesToWrite;
    QMap<QString, QMap<QString, QString>> moduleHelp;

    // Privacy option to allow the game to set Discord Rich Presence information
    bool mDiscordDisableServerSide;

    // Discord privacy options to give the user control over what data a Server
    // can set over OOB protocols (MSDP & GMCP) and the user via Lua API:
    DiscordOptionFlags mDiscordAccessFlags;

    double mLineSize;
    double mRoomSize;
    bool mShowInfo;
    bool mBubbleMode;
    bool mShowRoomID;
    bool mShowPanel;
    int mServerGUI_Package_version;
    QString mServerGUI_Package_name;
    bool mAcceptServerGUI;
    QColor mCommandLineFgColor;
    QColor mCommandLineBgColor;
    bool mMapperUseAntiAlias;
    bool mFORCE_MXP_NEGOTIATION_OFF;
    QSet<QChar> mDoubleClickIgnore;
    QPointer<QDockWidget> mpDockableMapWidget;
    bool mEnableTextAnalyzer;
    // Set from profile preferences, if the timer interval is less
    // than this then the normal reoccuring debug output of the entire command
    // and script for any timer with a timeout LESS than this is NOT shown
    // - this is so the spammy output from short timeout timers can be
    // suppressed.
    // An invalid/null value is treated as the "show all"/inactive case:
    QTime mTimerDebugOutputSuppressionInterval;

signals:
    // Tells TTextEdit instances for this profile how to draw the ambiguous
    // width characters:
    void signal_changeIsAmbigousWidthGlyphsToBeWide(bool);
    void profileSaveStarted();
    void profileSaveFinished();

private slots:
    void slot_reloadModules();

private:
    void installPackageFonts(const QString &packageName);

    QStringList mModulesToSync;

    QScopedPointer<LuaInterface> mLuaInterface;

    TriggerUnit mTriggerUnit;
    TimerUnit mTimerUnit;
    ScriptUnit mScriptUnit;
    AliasUnit mAliasUnit;
    ActionUnit mActionUnit;
    KeyUnit mKeyUnit;

    QString mBufferIncomingData;
    bool mCodeCompletion;

    bool mDisableAutoCompletion;
    QFile mErrorLogFile;

    QMap<QString, TEvent*> mEventMap;

    int mHostID;
    QString mHostName;

    bool mIsClosingDown;

    QString mLine;
    QMutex mLock;
    QString mLogin;

    int mMXPMode;

    QString mPass;

    int mPort;

    int mRetries;
    bool mSaveProfileOnExit;

    QString mUserDefinedName;

    QMap<int, QTime> mStopWatchMap;

    QMap<QString, QStringList> mAnonymousEventHandlerFunctions;

    QStringList mActiveModules;

    QPushButton* uninstallButton;
    QListWidget* packageList;
    QListWidget* moduleList;
    QPushButton* moduleUninstallButton;
    QPushButton* moduleInstallButton;

    bool mHaveMapperScript;
    // This option makes the control on the preferences tristated so the value
    // used depends - currently - on what the MUD Server encoding is (only set
    // true for GBK and GB18030 ones) - however this is likely to be due for
    // revision once locale/language support is brought in - when it can be
    // made dependent on that instead.
    bool mAutoAmbigousWidthGlyphsSetting;
    // If above is true is the value deduced from the MUD server encoding, if
    // the above is false is the user's direct setting - this is so that changes
    // in the TTextEdit classes are only made when necessary:
    bool mWideAmbigousWidthGlyphs;

    // keeps track of all of the array writers we're currently operating with
    QHash<QString, XMLexport*> writers;

    // Will be null/empty if is to use Mudlet's default/own presence
    QString mDiscordApplicationID;

    // Will be null/empty if we are not concerned to check the use of Discord
    // Rich Presence against the local user currently logged into Discord -
    // these two will be checked against the values from the Discord instance
    // with which we are linked to by the RPC library - and if they do not match
    // we won't use Discord functions.
    QString mRequiredDiscordUserName;
    QString mRequiredDiscordUserDiscriminator;

    void processGMCPDiscordStatus(const QJsonObject& discordInfo);
    void processGMCPDiscordInfo(const QJsonObject& discordInfo);
    void updateModuleZips() const;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Host::DiscordOptionFlags)

#endif // MUDLET_HOST_H
