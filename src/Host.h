/***************************************************************************
 *   Copyright (C) 2008 by Heiko Koehn   *
 *   KoehnHeiko@googlemail.com   *
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
#include "dlgTriggerEditor.h"
#include "TEvent.h"
#include "TKey.h"
#include "KeyUnit.h"

class dlgTriggerEditor;
class TConsole;
class dlgNotepad;


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
    void               orderShutDown();
    TriggerUnit *      getTriggerUnit()                 { return & mTriggerUnit; }
    TimerUnit *        getTimerUnit()                   { return & mTimerUnit; }
    AliasUnit *        getAliasUnit()                   { return & mAliasUnit; }
    ActionUnit *       getActionUnit()                  { return & mActionUnit; }
    KeyUnit *          getKeyUnit()                     { return & mKeyUnit; }
    ScriptUnit *       getScriptUnit()                  { return & mScriptUnit; }
    void               connectToServer();
    void               send( QString cmd, bool dontExpandAliases = false );
    void               sendRaw( QString s );
    int                getHostID() { QMutexLocker locker(& mLock); return mHostID; }
    void               setHostID( int id ) { QMutexLocker locker(& mLock); mHostID = id; }
    TLuaInterpreter *  getLuaInterpreter() { return & mLuaInterpreter; }    
    void               incomingStreamProcessor( QString & paragraph, QString & prompt, int line );
    void               gotRest( QString & );
    void               gotLine( QString & );
    void               gotPrompt( QString & );
    void               enableTimer( QString & );
    void               disableTimer( QString & );
    void               enableTrigger( QString & );
    void               disableTrigger( QString & );
    void               enableKey( QString & );
    void               disableKey( QString & );
    void               killTimer( QString & );
    void               killTrigger( QString & );
    double             stopStopWatch( int );
    bool               resetStopWatch( int );
    bool               startStopWatch( int );
    double             getStopWatchTime( int );
    int                createStopWatch();

    //QStringList        getBufferTable( int, int );
    //QString            getBufferLine( int );
    bool               serialize();
    bool               blockScripts() { return mBlockScriptCompile; }
    
    void               setIsAutologin( bool b ){ mIsAutologin = b; }
    bool               isAutologin(){ return mIsAutologin; }
    void               setReplacementCommand( QString );
    void               registerEventHandler( QString, TScript * );
    void               unregisterEventHandler( QString, TScript * );
    void               raiseEvent( TEvent * event );
    void               callEventHandlers();
    void               stopAllTriggers();
    void               reenableAllTriggers();
    void               set_USE_IRE_DRIVER_BUGFIX( bool b ){ mUSE_IRE_DRIVER_BUGFIX = b; mTelnet.set_USE_IRE_DRIVER_BUGFIX( b ); }
    void               adjustNAWS();
    class              Exception_NoLogin{};
    class              Exception_NoConnectionAvailable{};
    TConsole *         mpConsole;
    dlgTriggerEditor * mpEditorDialog;
    
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
    QFont              mDisplayFont;
    int                mScreenHeight;
    int                mScreenWidth;
    QFont              mCommandLineFont;
    QString            mCommandSeperator;
    bool               mSaveProfileOnExit;    
    //////////////////////////////////////////
    // this is serialized into hostOptions_2
    int mWrapAt;    
    int mWrapIndentCount;
    bool mPrintCommand;
    bool mAutoClearCommandLineAfterSend;
    QString mCommandSeparator;
    bool mDisableAutoCompletion;
    //////////////////////////////////////////
    bool               mUSE_IRE_DRIVER_BUGFIX;
    bool               mUSE_FORCE_LF_AFTER_PROMPT;
    bool               mNoAntiAlias;
    bool               mRawStreamDump;
    bool               mCodeCompletion;
    dlgNotepad *       mpNotePad;
    
    //private:
   
    QStringList        mTextBufferList;
    QString            mRest; 
    QString            mPrompt;
    QString            mLine; 
    QStringList        mParagraphList; 
    QString            mHostName;
    QString            mUrl;
    QString            mLogin;
    QString            mPass;
    QMutex             mLock;
    TLuaInterpreter    mLuaInterpreter;
    int                mTimeout;
    int                mRetries;
    bool               mIsClosingDown;
    int                mPort;
    QString            mUserDefinedName;
    
    TriggerUnit        mTriggerUnit;
    TimerUnit          mTimerUnit;
    ScriptUnit         mScriptUnit;
    AliasUnit          mAliasUnit;
    ActionUnit         mActionUnit;
    KeyUnit            mKeyUnit;
    
    cTelnet            mTelnet;
    int                mHostID;
    QString            mMudOutputBuffer;
    QString            mBufferIncomingData;
    QString            mReplacementCommand;
    QMap<QString, QList<TScript *> > mEventHandlerMap;
    QMap<QString, TEvent *> mEventMap;
    bool               mIsAutologin;
    QMap<int,QTime>    mStopWatchMap;
    int                mBorderTopHeight;
    int                mBorderBottomHeight;
    int                mBorderLeftWidth;
    int                mBorderRightWidth;
    bool               mUSE_UNIX_EOL;
    bool               mBlockScriptCompile;
    int                mMainIconSize;
    int                mTEFolderIconSize;
    bool               mIsGoingDown;
};
#endif


