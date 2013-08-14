/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn (KoehnHeiko@googlemail.com)    *
 *                                                                         *
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



#ifndef _HOST_H_
#define _HOST_H_

class mudlet;
class TLuaInterpreter;
class LuaInterface;


#include <list>
#include <string>
#include <QMutex>
#include <QString>
#include <QMutexLocker>
#include "ctelnet.h"
#include "TriggerUnit.h"
#include "TimerUnit.h"
#include "ScriptUnit.h"
#include "AliasUnit.h"
#include "ActionUnit.h"
#include "TLuaInterpreter.h"
#include <QTextBlock>
#include <QTextStream>
#include <QFile>
#include "dlgTriggerEditor.h"
#include "TEvent.h"
#include "TKey.h"
#include "KeyUnit.h"
#include <QVector3D>
#include "TArea.h"
#include "TRoom.h"
#include "TMap.h"
#include <QListWidget>
#include "LuaInterface.h"

class dlgTriggerEditor;
class TConsole;
class dlgNotepad;
class TMap;





class Host  : public QObject
{
    friend class XMLexport;
    friend class XMLimport;

public:

                       Host( int port, QString mHostName, QString login, QString pass, int host_id );

                       ~Host();

    QString            getName()                        { QMutexLocker locker(& mLock); return mHostName; }
    void               setName( QString s )             { QMutexLocker locker(& mLock); mHostName = s; }
    QString            getUrl()                         { QMutexLocker locker(& mLock); return mUrl; }
    void               setUrl( QString s )              { QMutexLocker locker(& mLock); mUrl = s; }
    QString            getUserDefinedName()             { QMutexLocker locker(& mLock); return mUserDefinedName; }
    void               setUserDefinedName( QString s )  { QMutexLocker locker(& mLock); mUserDefinedName = s; }
    int                getPort()                        { QMutexLocker locker(& mLock); return mPort; }
    void               setPort( int p )                 { QMutexLocker locker(& mLock); mPort = p; }
    QString &          getLogin()                       { QMutexLocker locker(& mLock); return mLogin; }
    void               setLogin( QString s )            { QMutexLocker locker(& mLock); mLogin = s; }
    QString &          getPass()                        { QMutexLocker locker(& mLock); return mPass; }
    void               setPass( QString s )             { QMutexLocker locker(& mLock); mPass = s; }
    int                getRetries()                     { QMutexLocker locker(& mLock); return mRetries;}
    void               setRetries( int c )              { QMutexLocker locker(& mLock); mRetries=c; }
    int                getTimeout()                     { QMutexLocker locker(& mLock); return mTimeout; }
    void               setTimeout( int seconds )        { QMutexLocker locker(& mLock); mTimeout=seconds; }
    bool               closingDown();
    void               assemblePath();
    int                check_for_mappingscript();
    void               orderShutDown();
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
    LuaInterface *     getLuaInterface() { return mLuaInterface; }
    void               incomingStreamProcessor( QString & paragraph, int line );
    void               postIrcMessage( QString, QString, QString );
    void               enableTimer( QString & );
    void               disableTimer( QString & );
    void               enableTrigger( QString & );
    void               disableTrigger( QString & );
    void               enableKey( QString & );
    void               disableKey( QString & );
    bool               killTimer( QString & );
    bool               killTrigger( QString & );
    double             stopStopWatch( int );
    bool               resetStopWatch( int );
    bool               startStopWatch( int );
    double             getStopWatchTime( int );
    int                createStopWatch();
    void               startSpeedWalk();
    //QStringList        getBufferTable( int, int );
    //QString            getBufferLine( int );
    bool               serialize();
    void               saveModules(int);
    void               reloadModule(QString moduleName);
    bool               blockScripts() { return mBlockScriptCompile; }

    void               setIsAutologin( bool b ){ mIsAutologin = b; }
    bool               isAutologin(){ return mIsAutologin; }
    void               setReplacementCommand( QString );
    void               registerEventHandler( QString, TScript * );
    void               registerAnonymousEventHandler( QString name, QString fun );
    void               unregisterEventHandler( QString, TScript * );
    void               raiseEvent( TEvent * event );
    void               resetProfile();
    void               callEventHandlers();
    void               stopAllTriggers();
    void               reenableAllTriggers();
    void               set_USE_IRE_DRIVER_BUGFIX( bool b ){ mUSE_IRE_DRIVER_BUGFIX = b; mTelnet.set_USE_IRE_DRIVER_BUGFIX( b ); }
    void               set_LF_ON_GA( bool b ){ mLF_ON_GA = b; mTelnet.set_LF_ON_GA( b ); }
    void               adjustNAWS();
    class              Exception_NoLogin{};
    class              Exception_NoConnectionAvailable{};

    bool               installPackage( QString, int module);
    bool               uninstallPackage( QString, int module);
    bool               removeDir( const QString dirName, QString originalPath);
    void               readPackageConfig( QString, QString & );

    cTelnet            mTelnet;
    TConsole *         mpConsole;
    TLuaInterpreter    mLuaInterpreter;
    LuaInterface *     mLuaInterface;
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
    int                mBorderBottomHeight;
    int                mBorderLeftWidth;
    int                mBorderRightWidth;
    int                mBorderTopHeight;
    QString            mBufferIncomingData;
    bool               mCodeCompletion;
    QFont              mCommandLineFont;
    QString            mCommandSeparator;
    QString            mCommandSeperator;
    bool               mDisableAutoCompletion;
    QFont              mDisplayFont;
    bool               mEnableGMCP;
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
    TMap *             mpMap;
    dlgNotepad *       mpNotePad;
    QStringList        mParagraphList;

    int                mPort;
    bool               mPrintCommand;
    QString            mPrompt;
    bool               mRawStreamDump;
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

    void               showUnpackingProgress( QString  txt );
    QDialog *          mpUnzipDialog;
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

private:
    Host();




};
#endif


