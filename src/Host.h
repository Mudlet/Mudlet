#ifndef MUDLET_HOST_H
#define MUDLET_HOST_H

/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2015-2017 by Stephen Lyons - slysven@virginmedia.com    *
 *   Copyright (C) 2016 by Ian Adkins - ieadkins@gmail.com                 *
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
    friend class XMLexport;
    friend class XMLimport;

public:
    Host(int port, const QString& mHostName, const QString& login, const QString& pass, int host_id);
    ~Host();

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
    void sendRaw(QString s);

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
    void saveModules(int);
    void reloadModule(const QString& moduleName);
    bool blockScripts() { return mBlockScriptCompile; }

    void setIsAutologin(bool b) { mIsAutologin = b; }
    bool isAutologin() { return mIsAutologin; }
    void setReplacementCommand(const QString&);
    void registerEventHandler(const QString&, TScript*);
    void registerAnonymousEventHandler(const QString& name, const QString& fun);
    void unregisterEventHandler(const QString&, TScript*);
    void raiseEvent(const TEvent& event);
    void resetProfile();
    std::tuple<bool, QString, QString> saveProfile(const QString& saveLocation = QString(), bool syncModules = false);
    void callEventHandlers();
    void stopAllTriggers();
    void reenableAllTriggers();

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

    class Exception_NoLogin
    {
    };

    class Exception_NoConnectionAvailable
    {
    };

    bool installPackage(const QString&, int);
    bool uninstallPackage(const QString&, int);
    bool removeDir(const QString&, const QString&);
    void readPackageConfig(const QString&, QString&);
    void postMessage(const QString message) { mTelnet.postMessage(message); }
    void writeProfileData(const QString&, const QString&);

public:
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

private:
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

    bool mIsAutologin;

    bool mIsClosingDown;

    QString mLine;
    QMutex mLock;
    QString mLogin;
    int mMainIconSize;
    QString mMudOutputBuffer;
    int mMXPMode;

    QString mPass;
    QStringList mParagraphList;

    int mPort;
    QString mPrompt;

    QString mReplacementCommand;

    QString mRest;

    int mRetries;
    bool mSaveProfileOnExit;
    bool mShowToolbar;
    int mTEFolderIconSize;
    QStringList mTextBufferList;

    QString mUserDefinedName;

    QMap<int, QTime> mStopWatchMap;

    QMap<QString, QStringList> mAnonymousEventHandlerFunctions;

    QString mIRCNick;

    QStringList mActiveModules;
    bool mModuleSaveBlock;

    QPushButton* uninstallButton;
    QListWidget* packageList;
    QListWidget* moduleList;
    QPushButton* moduleUninstallButton;
    QPushButton* moduleInstallButton;

    bool mHaveMapperScript;
};

#endif // MUDLET_HOST_H
