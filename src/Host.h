#ifndef MUDLET_HOST_H
#define MUDLET_HOST_H

/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2015-2019 by Stephen Lyons - slysven@virginmedia.com    *
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
class TMedia;
class TRoom;
class TConsole;
class dlgNotepad;
class TMap;

class stopWatch {
    friend class XMLimport;

public:
    stopWatch();

    bool start();
    bool stop();
    bool reset();
    bool running() const {return mIsRunning;}
    void adjustMilliSeconds(const qint64);
    qint64 getElapsedMilliSeconds() const;
    QString getElapsedDayTimeString() const;
    void setPersistent(const bool state) {mIsPersistent = state;}
    bool persistent() const {return mIsPersistent;}
    void setName(const QString& name) {mName = name;}
    const QString& name() const {return mName;}

#ifndef QT_NO_DEBUG_STREAM
    // Only used for debugging:
    bool initialised() const {return mIsInitialised;}
    qint64 elapsed() const {return mElapsedTime;}
    QDateTime effectiveStartDateTime() const {return mEffectiveStartDateTime;}
#endif // QT_NO_DEBUG_STREAM

private:
    // true once started the first time - but provides some code short cuts if
    // prior to that:
    bool mIsInitialised;
    // true when RUNNING, false when STOPPED:
    bool mIsRunning;
    // If true this stopwatch is saved with the profile and reloaded - if it is
    // running when saved it will seem to continue to run - so can be used to
    // track real time events outside of the profile, it is intended that the
    // parent Host class that makes use of it will save and restore the id
    // number that that class assigns the stopwatch:
    bool mIsPersistent;
    // When RUNNING this contains the effective point in time when the stop
    // watch was started - so that taking a difference between then and
    // "now" gives the total elapsed time:
    QDateTime mEffectiveStartDateTime;
    // When STOPPED this contains the cumulative elasped time in milliSeconds:
    qint64 mElapsedTime;
    // As the id is generated according to what other ones have been created but
    // persists between saves it is useful for the user or script writer to be
    // able to name a particular stop watch for identification later:
    QString mName;
};

#ifndef QT_NO_DEBUG_STREAM
inline QDebug& operator<<(QDebug& debug, const stopWatch& stopwatch)
{
    QDebugStateSaver saver(debug);
    Q_UNUSED(saver);
    debug.nospace() << QStringLiteral("stopwatch(mIsRunning: %1 mInitialised: %2 mIsPersistent: %3 mEffectiveStartDateTime: %4 mElapsedTime: %5)")
                       .arg((stopwatch.running() ? QLatin1String("true") : QLatin1String("false")),
                            (stopwatch.initialised() ? QLatin1String("true") : QLatin1String("false")),
                            (stopwatch.persistent() ? QLatin1String("true") : QLatin1String("false")),
                            stopwatch.effectiveStartDateTime().toString(),
                            QString::number(stopwatch.elapsed()));
    return debug;
}
#endif // QT_NO_DEBUG_STREAM

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
    QString            getCommandSeparator()            { QMutexLocker locker(& mLock); return mCommandSeparator; }
    void               setName(const QString& s );
    QString            getUrl()                         { QMutexLocker locker(& mLock); return mUrl; }
    void               setUrl(const QString& s )        { QMutexLocker locker(& mLock); mUrl = s; }
    QString            getUserDefinedName()             { QMutexLocker locker(& mLock); return mUserDefinedName; }
    void               setUserDefinedName(const QString& s ) { QMutexLocker locker(& mLock); mUserDefinedName = s; }
    int                getPort()                        { QMutexLocker locker(& mLock); return mPort; }
    void               setPort( int p )                 { QMutexLocker locker(& mLock); mPort = p; }
    void               setAutoReconnect(bool b)         { mTelnet.setAutoReconnect(b); }
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
    void               setHaveColorSpaceId(const bool state) { QMutexLocker locker(& mLock); mSGRCodeHasColSpaceId = state; }
    bool               getHaveColorSpaceId() { QMutexLocker locker(& mLock); return mSGRCodeHasColSpaceId; }
    void               setMayRedefineColors(const bool state) { QMutexLocker locker(& mLock); mServerMayRedefineColors = state; }
    bool               getMayRedefineColors() { QMutexLocker locker(& mLock); return mServerMayRedefineColors; }
    void               setDiscordApplicationID(const QString& s);
    const QString&     getDiscordApplicationID();
    void               setSpellDic(const QString&);
    const QString&     getSpellDic() { QMutexLocker locker(& mLock); return mSpellDic; }
    void               setUserDictionaryOptions(const bool useDictionary, const bool useShared);
    void               getUserDictionaryOptions(bool& useDictionary, bool& useShared) { QMutexLocker locker(& mLock); useDictionary = mEnableUserDictionary; useShared = mUseSharedDictionary; }

    void closingDown();
    bool isClosingDown();
    unsigned int assemblePath();
    bool checkForMappingScript();

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

    QPair<int, QString> createStopWatch(const QString&);
    bool destroyStopWatch(const int);
    bool adjustStopWatch(const int, const qint64 milliSeconds);
    QList<int> getStopWatchIds() const;
    QPair<bool, double> getStopWatchTime(const int) const;
    QPair<bool, QString> getBrokenDownStopWatchTime(const int) const;
    bool makeStopWatchPersistent(const int, const bool);
    QPair<bool, QString> resetStopWatch(const int);
    QPair<bool, QString> resetStopWatch(const QString&);
    QPair<bool, QString> startStopWatch(const int);
    QPair<bool, QString> startStopWatch(const QString&);
    QPair<bool, QString> stopStopWatch(const int);
    QPair<bool, QString> stopStopWatch(const QString&);
    stopWatch* getStopWatch(const int id) const { return mStopWatchMap.value(id); }
    int findStopWatchId(const QString&) const;
    QPair<bool, QString> setStopWatchName(const int, const QString&);
    QPair<bool, QString> setStopWatchName(const QString&, const QString&);
    QPair<bool, QString> resetAndRestartStopWatch(const int);

    void startSpeedWalk();
    void saveModules(int sync, bool backup = true);
    void reloadModule(const QString& reloadModuleName);
    bool blockScripts() { return mBlockScriptCompile; }
    void refreshPackageFonts();

    void registerEventHandler(const QString&, TScript*);
    void registerAnonymousEventHandler(const QString& name, const QString& fun);
    void unregisterEventHandler(const QString&, TScript*);
    void raiseEvent(const TEvent& event);
    // This disables all the triggers/timers/keys in preparation to resetting
    // them - and sets a timer to do resetProfile_phase2() when it is safe to do
    // so. We need to do it this way because a lua script containing the call to
    // produce this action will be purged from the Lua system as part of the
    // reset - which causes nasty existential issues (and crashes) from deleting
    // a script as it is being interpreted!
    void resetProfile_phase1();
    // This actually does the bulk of the reset but must wait until the profile
    // is quiescent:
    void resetProfile_phase2();
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
    QColor getAnsiColor(const int ansiCode, const bool isBackground = false) const;
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
    void setMediaLocationGMCP(const QString& mediaUrl);
    QString getMediaLocationGMCP() const;
    void setMediaLocationMSP(const QString& mediaUrl);
    QString getMediaLocationMSP() const;
    const QFont& getDisplayFont() const { return mDisplayFont; }
    std::pair<bool, QString> setDisplayFont(const QFont& font);
    std::pair<bool, QString> setDisplayFont(const QString& fontName);
    void setDisplayFontFromString(const QString& fontData);
    void setDisplayFontSize(int size);
    void setDisplayFontSpacing(const qreal spacing);
    void setDisplayFontStyle(QFont::StyleStrategy s);
    void setDisplayFontFixedPitch(bool enable);
    void updateProxySettings(QNetworkAccessManager* manager);
    std::unique_ptr<QNetworkProxy>& getConnectionProxy();
    void updateAnsi16ColorsInTable();
    // Store/retrieve all the settings in one call:
    void setPlayerRoomStyleDetails(const quint8 styleCode, const quint8 outerDiameter = 120, const quint8 innerDiameter = 70, const QColor& outerColor = QColor(), const QColor& innerColor = QColor());
    void getPlayerRoomStyleDetails(quint8& styleCode, quint8& outerDiameter, quint8& innerDiameter, QColor& outerColor, QColor& innerColor);

    cTelnet mTelnet;
    QPointer<TConsole> mpConsole;
    TLuaInterpreter mLuaInterpreter;

    int commandLineMinimumHeight;
    bool mAlertOnNewData;
    bool mAllowToSendCommand;
    bool mAutoClearCommandLineAfterSend;
    // Set in constructor and used in (bool) TScript::setScript(const QString&)
    // to prevent compilation of the script that was being set therein, cleared
    // after the main TConsole for a new profile has been created during the
    // period when mIsProfileLoadingSequence has been set:
    bool mBlockScriptCompile;
    bool mBlockStopWatchCreation;
    bool mEchoLuaErrors;
    int mBorderBottomHeight;
    int mBorderLeftWidth;
    int mBorderRightWidth;
    int mBorderTopHeight;
    QFont mCommandLineFont;
    QString mCommandSeparator;
    bool mEnableGMCP;
    bool mEnableMSSP;
    bool mEnableMSP;
    bool mEnableMSDP;
    bool mServerMXPenabled;
    QString mMediaLocationGMCP;
    QString mMediaLocationMSP;
    QTextStream mErrorLogStream;
    QMap<QString, QList<TScript*>> mEventHandlerMap;
    bool mFORCE_GA_OFF;
    bool mFORCE_NO_COMPRESSION;
    bool mFORCE_SAVE_ON_EXIT;
    bool mInsertedMissingLF;

    bool mSslTsl;
    bool mAutoReconnect;
    bool mSslIgnoreExpired;
    bool mSslIgnoreSelfSigned;
    bool mSslIgnoreAll;

    bool mUseProxy;
    QString mProxyAddress;
    quint16 mProxyPort;
    QString mProxyUsername;
    QString mProxyPassword;

    bool mIsGoingDown;
    // Used to force the test compilation of the scripts for TActions ("Buttons")
    // that are pushdown buttons that run when they are "pushed down" during
    // loading even though the buttons start out with themselves NOT being
    // pushed down:
    bool mIsProfileLoadingSequence;

    bool mLF_ON_GA;
    bool mNoAntiAlias;

    dlgTriggerEditor* mpEditorDialog;
    QScopedPointer<TMap> mpMap;
    QScopedPointer<TMedia> mpMedia;
    dlgNotepad* mpNotePad;

    // This is set when we want commands we typed to be shown on the main
    // TConsole:
    bool mPrintCommand;

    /*
     * This is set when the Server is remote echoing what WE send to it,
     * it will have negotiated the ECHO suboption by sending a IAC WILL ECHO and
     * we have agreed to it with a IAC DO ECHO - in this state it is normal
     * Telnet behaviour to NOT echo what we send - to prevent doubled lines.
     * The rationale behind this is so that when we type passwords - WE do not
     * echo what we type and rely on the other end to, but they do not, so as to
     * hide them on our screen (and from logging!) - It should negate the effect
     * of the above mPrintCommand being true...
     */
    bool mIsRemoteEchoingActive;

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

    bool mEditorAutoComplete;

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
    QString mServerGUI_Package_version;
    QString mServerGUI_Package_name;
    bool mAcceptServerGUI;
    bool mAcceptServerMedia;
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
    std::unique_ptr<QNetworkProxy> mpDownloaderProxy;
    QString mProfileStyleSheet;

signals:
    // Tells TTextEdit instances for this profile how to draw the ambiguous
    // width characters:
    void signal_changeIsAmbigousWidthGlyphsToBeWide(bool);
    void profileSaveStarted();
    void profileSaveFinished();
    void signal_changeSpellDict(const QString&);

private slots:
    void slot_reloadModules();

private:
    void installPackageFonts(const QString &packageName);
    void processGMCPDiscordStatus(const QJsonObject& discordInfo);
    void processGMCPDiscordInfo(const QJsonObject& discordInfo);
    void updateModuleZips() const;
    void loadSecuredPassword();
    void removeAllNonPersistentStopWatches();
    void updateConsolesFont();
    void thankForUsingPTB();

    QFont mDisplayFont;
    QStringList mModulesToSync;
    QScopedPointer<LuaInterface> mLuaInterface;

    TriggerUnit mTriggerUnit;
    TimerUnit mTimerUnit;
    ScriptUnit mScriptUnit;
    AliasUnit mAliasUnit;
    ActionUnit mActionUnit;
    KeyUnit mKeyUnit;

    QString mBufferIncomingData;

    QFile mErrorLogFile;

    QMap<QString, TEvent*> mEventMap;

    int mHostID;
    QString mHostName;

    bool mIsClosingDown;

    QString mLine;
    QMutex mLock;
    QString mLogin;
    QString mPass;

    int mPort;

    int mRetries;
    bool mSaveProfileOnExit;

    QString mUserDefinedName;

    // To keep things simple for Lua the first stopwatch will be allocated a key
    // of 1 - and anything less that that will be rejected - and we force
    // createStopWatch() to return 0 during script loading so that we do not get
    // superious stopwatches from being created then (when
    // mIsProfileLoadingSequence is true):
    QMap<int, stopWatch*> mStopWatchMap;

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

    // Handles whether to treat 16M-Colour ANSI SGR codes which only use
    // semi-colons as separator have the initial Colour Space Id parameter
    // (true) or not (false):
    bool mSGRCodeHasColSpaceId;

    // Flag whether the Server can use ANSI OSC "P#RRGGBB\" to redefine the
    // 16 basic colors (and OSC "R\" to reset them).
    bool mServerMayRedefineColors;

    // Was public but hidden to prevent it being changed without going through
    // the process to signal to users that they need to change dictionaries:
    QString mSpellDic;
    // These are hidden to prevent them being changed directly, they are also
    // mirrored/cached in the main TConsole's instance so they do not need to be
    // looked up directly by that class:
    bool mEnableUserDictionary;
    bool mUseSharedDictionary;

    // These hold values that are needed in the TMap clas which are saved with
    // the profile - but which cannot be kept there as that class is not
    // necessarily instantiated when the profile is read.
    // Base color(s) for the player room in the mappers:
    // Mode selected:
    // 0 is closest to original style with adjustable outer diameter
    // 1 is Fixed red color ring with adjustable outer/inner diameter
    // 2 is fixed blue/yellow colors ring with adjustable outer/inner diameter
    // 3 is adjustable outer(primary)/inner(secondary) colors ring with adjustable outer/inner diameter
    quint8 mPlayerRoomStyle;
    QColor mPlayerRoomOuterColor;
    QColor mPlayerRoomInnerColor;
    // Percentage of the room size (actually width) for the outer diameter of
    // the circular marking, integer percentage clamped in the preferences
    // between 200 and 50 - default 120:
    quint8 mPlayerRoomOuterDiameterPercentage;
    // Percentage of the outer size for the inner diameter of the circular
    // marking, integer percentage clamped in the preferences between 83 and 0,
    // with a default of 70. NOT USED FOR "Original" style marking (the 0'th
    // one):
    quint8 mPlayerRoomInnerDiameterPercentage;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Host::DiscordOptionFlags)

#endif // MUDLET_HOST_H
