#ifndef MUDLET_HOST_H
#define MUDLET_HOST_H

/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2015-2016 by Stephen Lyons - slysven@virginmedia.com    *
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
#include "ctelnet.h"
#include "KeyUnit.h"
#include "ScriptUnit.h"
#include "TimerUnit.h"
#include "TLuaInterpreter.h"
#include "TriggerUnit.h"

#include "pre_guard.h"
#include <QColor>
#include <QFile>
#include <QFont>
#include <QPointer>
#include <QTextStream>
#include "post_guard.h"

class QDialog;
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


class Host  : public QObject
{
    friend class XMLexport;
    friend class XMLimport;

public:

                       Host( int port, const QString& mHostName, const QString& login, const QString& pass, int host_id );

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
    bool               closingDown();
    const unsigned int assemblePath();
    const bool         checkForMappingScript();
    TriggerUnit *      getTriggerUnit()                 { return & mTriggerUnit; }
    TimerUnit *        getTimerUnit()                   { return & mTimerUnit; }
    AliasUnit *        getAliasUnit()                   { return & mAliasUnit; }
    ActionUnit *       getActionUnit()                  { return & mActionUnit; }
    KeyUnit *          getKeyUnit()                     { return & mKeyUnit; }
    ScriptUnit *       getScriptUnit()                  { return & mScriptUnit; }
    void               connectToServer();
    void               send( QString cmd, bool wantPrint = true, bool dontExpandAliases = false );
    void               sendRaw( QString s );
    int                getHostID() { QMutexLocker locker(& mLock); return mHostID; }
    void               setHostID( int id ) { QMutexLocker locker(& mLock); mHostID = id; }
    TLuaInterpreter *  getLuaInterpreter() { return & mLuaInterpreter; }
    LuaInterface *     getLuaInterface() { return mLuaInterface.data(); }
    void               incomingStreamProcessor(const QString & paragraph, int line );
    void               postIrcMessage(const QString&, const QString&, const QString& );
    void               enableTimer(const QString & );
    void               disableTimer(const QString & );
    void               enableTrigger(const QString & );
    void               disableTrigger(const QString & );
    void               enableKey(const QString & );
    void               disableKey(const QString & );
    bool               killTimer(const QString & );
    bool               killTrigger(const QString & );
    double             stopStopWatch( int );
    bool               resetStopWatch( int );
    bool               startStopWatch( int );
    double             getStopWatchTime( int );
    int                createStopWatch();
    void               startSpeedWalk();
    //QStringList        getBufferTable( int, int );
    //QString            getBufferLine( int );
    void               saveModules(int);
    void               reloadModule(const QString& moduleName);
    bool               blockScripts() { return mBlockScriptCompile; }

    void               setIsAutologin( bool b ){ mIsAutologin = b; }
    bool               isAutologin(){ return mIsAutologin; }
    void               setReplacementCommand(const QString& );
    void               registerEventHandler(const QString&, TScript * );
    void               registerAnonymousEventHandler(const QString& name, const QString& fun );
    void               unregisterEventHandler(const QString&, TScript * );
    void               raiseEvent( const TEvent & event );
    void               resetProfile();
    void               callEventHandlers();
    void               stopAllTriggers();
    void               reenableAllTriggers();
    void               set_USE_IRE_DRIVER_BUGFIX( bool b ){ mUSE_IRE_DRIVER_BUGFIX = b; mTelnet.set_USE_IRE_DRIVER_BUGFIX( b ); }
    void               set_LF_ON_GA( bool b ){ mLF_ON_GA = b; mTelnet.set_LF_ON_GA( b ); }
    void               adjustNAWS();
    class              Exception_NoLogin{};
    class              Exception_NoConnectionAvailable{};

    bool               installPackage(const QString&, int module);
    bool               uninstallPackage(const QString&, int module);
    bool               removeDir( const QString& dirName, const QString& originalPath);
    void               readPackageConfig(const QString&, QString & );
    void                postMessage( const QString message ) { mTelnet.postMessage( message ); }


    cTelnet            mTelnet;
    QPointer<TConsole> mpConsole;
    TLuaInterpreter    mLuaInterpreter;
    QScopedPointer<LuaInterface> mLuaInterface;
    TriggerUnit        mTriggerUnit;
    TimerUnit          mTimerUnit;
    ScriptUnit         mScriptUnit;
    AliasUnit          mAliasUnit;
    ActionUnit         mActionUnit;
    KeyUnit            mKeyUnit;

    int                commandLineMinimumHeight;
    bool               mAlertOnNewData;
    bool               mAllowToSendCommand;
    bool               mAutoClearCommandLineAfterSend;
    bool               mBlockScriptCompile;
    bool               mEchoLuaErrors; 
    int                mBorderBottomHeight;
    int                mBorderLeftWidth;
    int                mBorderRightWidth;
    int                mBorderTopHeight;
    QString            mBufferIncomingData;
    bool               mCodeCompletion;
    QFont              mCommandLineFont;
    QString            mCommandSeparator;
    bool               mDisableAutoCompletion;
    QFont              mDisplayFont;
    bool               mEnableGMCP;
    bool               mEnableMSDP;
    int                mEncoding;
    QTextStream        mErrorLogStream;
    QFile              mErrorLogFile;
    QMap<QString, QList<TScript *> > mEventHandlerMap;
    QMap<QString, TEvent *> mEventMap;
    bool               mFORCE_GA_OFF;
    bool               mFORCE_NO_COMPRESSION;
    bool               mFORCE_SAVE_ON_EXIT;
    int                mHostID;
    QString            mHostName;
    bool               mInsertedMissingLF;
    bool               mIsAutologin;
    bool               mIsGoingDown;
    bool               mIsProfileLoadingSequence;

    bool               mIsClosingDown;
    bool               mLF_ON_GA;
    QString            mLine;
    QMutex             mLock;
    QString            mLogin;
    int                mMainIconSize;
    QString            mMudOutputBuffer;
    int                mMXPMode;
    bool               mNoAntiAlias;

    QString            mPass;
    dlgTriggerEditor * mpEditorDialog;
    QScopedPointer<TMap> mpMap;
    dlgNotepad *       mpNotePad;
    QStringList        mParagraphList;

    int                mPort;
    bool               mPrintCommand;
    QString            mPrompt;
                       // The following was incorrectly called mRawStreamDump
                       // and caused the log file to be in HTML format rather
                       // then plain text.  To cover the corner case of the user
                       // changing the mode whilst a log is being written it has
                       // been split into:
    bool               mIsNextLogFileInHtmlFormat;
                       // What the user has set as their preference
    bool               mIsCurrentLogFileInHtmlFormat;
                       // What the current file will use, set from the previous
                       // member at the point that logging starts.
                       // Ideally this ought to become a number so that we can
                       // support more than two logging format modes - phpBB
                       // format would be useful for those wanting to post to
                       // MUD forums...!  Problem will be reading and write the
                       // game save file in a compatible way.
    QString            mReplacementCommand;
    QString            mRest;
    bool               mResetProfile;
    int                mRetries;
    bool               mSaveProfileOnExit;
    int                mScreenHeight;
    int                mScreenWidth;
    bool               mShowToolbar;
    int                mTEFolderIconSize;
    QStringList        mTextBufferList;

    int                mTimeout;

    QString            mUrl;

    bool               mUSE_FORCE_LF_AFTER_PROMPT;
    bool               mUSE_IRE_DRIVER_BUGFIX;
    bool               mUSE_UNIX_EOL;
    QString            mUserDefinedName;
    int                mWrapAt;
    int                mWrapIndentCount;

    QColor             mBlack;
    QColor             mLightBlack;
    QColor             mRed;
    QColor             mLightRed;
    QColor             mLightGreen;
    QColor             mGreen;
    QColor             mLightBlue;
    QColor             mBlue;
    QColor             mLightYellow;
    QColor             mYellow;
    QColor             mLightCyan;
    QColor             mCyan;
    QColor             mLightMagenta;
    QColor             mMagenta;
    QColor             mLightWhite;
    QColor             mWhite;
    QColor             mFgColor;
    QColor             mBgColor;
    QColor             mCommandBgColor;
    QColor             mCommandFgColor;

    QMap<int,QTime>    mStopWatchMap;

    QColor             mBlack_2;
    QColor             mLightBlack_2;
    QColor             mRed_2;
    QColor             mLightRed_2;
    QColor             mLightGreen_2;
    QColor             mGreen_2;
    QColor             mLightBlue_2;
    QColor             mBlue_2;
    QColor             mLightYellow_2;
    QColor             mYellow_2;
    QColor             mLightCyan_2;
    QColor             mCyan_2;
    QColor             mLightMagenta_2;
    QColor             mMagenta_2;
    QColor             mLightWhite_2;
    QColor             mWhite_2;
    QColor             mFgColor_2;
    QColor             mBgColor_2;
    bool               mMapStrongHighlight;
    QStringList        mGMCP_merge_table_keys;
    QMap<QString, QStringList> mAnonymousEventHandlerFunctions;
    QString            mSpellDic;
    bool               mLogStatus;
    bool               mEnableSpellCheck;
    QString            mIRCNick;
    QStringList        mInstalledPackages;
    QMap<QString, QStringList> mInstalledModules;
    QMap<QString, int> mModulePriorities;
    QMap<QString, QStringList> modulesToWrite;
    QMap<QString, QMap<QString, QString> > moduleHelp;
    QStringList        mActiveModules;
    bool               mModuleSaveBlock;

    // There was a QDialog *          mpUnzipDialog; but to avoid issues of
    // reentrancy it needed to be made local to the method that used it.
    QPushButton *      uninstallButton;
    QListWidget *      packageList;
    QListWidget *                 moduleList;
    QPushButton *                 moduleUninstallButton;
    QPushButton *                 moduleInstallButton;
    double             mLineSize;
    double             mRoomSize;
    bool               mShowInfo;
    bool               mBubbleMode;
    bool               mShowRoomID;
    bool               mShowPanel;
    int                mServerGUI_Package_version;
    QString            mServerGUI_Package_name;
    bool               mAcceptServerGUI;
    QColor             mCommandLineFgColor;
    QColor             mCommandLineBgColor;
    bool               mMapperUseAntiAlias;
    bool               mFORCE_MXP_NEGOTIATION_OFF;
    bool               mHaveMapperScript;
    QSet<QChar>         mDoubleClickIgnore;
};

#endif // MUDLET_HOST_H
