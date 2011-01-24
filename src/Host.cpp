
/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn  KoehnHeiko@googlemail.com     *
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



#ifndef _HOST_CPP_
#define _HOST_CPP_

#include <QString>
#include "Host.h"
#include "ctelnet.h"
#include <QDataStream>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include "XMLexport.h"
#include "XMLimport.h"
#include "mudlet.h"
#include "TEvent.h"
#include <QMessageBox>
#include "dlgNotepad.h"


Host::Host( int port, QString hostname, QString login, QString pass, int id )
: mTelnet( this )
, mpConsole( 0 )
, mLuaInterpreter    ( this, id )
, mTriggerUnit       ( this )
, mTimerUnit         ( this )
, mScriptUnit        ( this )
, mAliasUnit         ( this )
, mActionUnit        ( this )
, mKeyUnit           ( this )
, commandLineMinimumHeight( 30 )
, mAlertOnNewData( true )
, mAllowToSendCommand( true )
, mAutoClearCommandLineAfterSend( false )
, mBlockScriptCompile( true )
, mBorderBottomHeight( 0 )
, mBorderLeftWidth( 0 )
, mBorderRightWidth( 0 )
, mBorderTopHeight( 0 )
, mCodeCompletion( true )
, mCommandLineFont   ( QFont("Bitstream Vera Sans Mono", 10, QFont::Courier ) )//( QFont("Monospace", 10, QFont::Courier) )
, mCommandSeparator  ( QString(";") )
, mCommandSeperator  ( QString(";") )
, mDisableAutoCompletion( false )
, mDisplayFont       ( QFont("Bitstream Vera Sans Mono", 10, QFont::Courier ) )//, mDisplayFont       ( QFont("Bitstream Vera Sans Mono", 10, QFont:://( QFont("Monospace", 10, QFont::Courier) ), mPort              ( port )
, mEnableGMCP( false )
, mFORCE_GA_OFF( false )
, mFORCE_NO_COMPRESSION( false )
, mFORCE_SAVE_ON_EXIT( false )
, mHostID( id )
, mHostName( hostname )
, mInsertedMissingLF( false )
, mIsGoingDown( false )
, mLF_ON_GA( true )
, mLogin( login )
, mMainIconSize( 3 )
, mNoAntiAlias( false )
, mPass( pass )
, mpEditorDialog(0)
, mpMap( new TMap( this ) )
, mpNotePad( 0 )
, mPort(port)
, mPrintCommand( true )
, mRawStreamDump( false )
, mResetProfile( false )
, mRetries( 5 )
, mSaveProfileOnExit( false )
, mScreenHeight( 25 )
, mScreenWidth( 90 )
, mTEFolderIconSize( 3 )
, mTimeout( 60 )
, mUSE_FORCE_LF_AFTER_PROMPT( false )
, mUSE_IRE_DRIVER_BUGFIX( true )
, mUSE_UNIX_EOL( false )
, mWrapAt( 100 )
, mWrapIndentCount( 0 )

, mBlack             ( QColor(  0,  0,  0) )
, mLightBlack        ( QColor(128,128,128) )
, mRed               ( QColor(128,  0,  0) )
, mLightRed          ( QColor(255,  0,  0) )
, mLightGreen        ( QColor(  0,255,  0) )
, mGreen             ( QColor(  0,179,  0) )
, mLightBlue         ( QColor(  0,  0,255) )
, mBlue              ( QColor(  0,  0,128) )
, mLightYellow       ( QColor(255,255,  0) )
, mYellow            ( QColor(128,128,  0) )
, mLightCyan         ( QColor(  0,255,255) )
, mCyan              ( QColor(  0,128,128) )
, mLightMagenta      ( QColor(255,  0,255) )
, mMagenta           ( QColor(128,  0,128) )
, mLightWhite        ( QColor(255,255,255) )
, mWhite             ( QColor(192,192,192) )
, mFgColor           ( QColor(192,192,192) )
, mBgColor           ( QColor(  0,  0,  0) )
, mCommandBgColor    ( QColor(  0,  0,  0) )
, mCommandFgColor    ( QColor(113,113,  0) )
, mBlack_2             ( QColor(  0,  0,  0, 255) )
, mLightBlack_2        ( QColor(128,128,128, 255) )
, mRed_2               ( QColor(128,  0,  0, 255) )
, mLightRed_2          ( QColor(255,  0,  0, 255) )
, mLightGreen_2        ( QColor(  0,255,  0, 255) )
, mGreen_2             ( QColor(  0,179,  0, 255) )
, mLightBlue_2         ( QColor(  0,  0, 255, 255) )
, mBlue_2              ( QColor(  0,  0, 128, 255) )
, mLightYellow_2       ( QColor(255,255,  0, 255) )
, mYellow_2            ( QColor(128,128,  0, 255) )
, mLightCyan_2         ( QColor(  0,255,255, 255) )
, mCyan_2              ( QColor(  0,128,128, 255) )
, mLightMagenta_2      ( QColor(255,  0,255, 255) )
, mMagenta_2           ( QColor(128,  0,128, 255) )
, mLightWhite_2        ( QColor(255,255,255, 255) )
, mWhite_2             ( QColor(192,192,192, 255) )
, mFgColor_2           ( QColor(192,192,192, 255) )
, mBgColor_2           ( QColor(  0,  0,  0, 255) )
{
    QString directoryLogFile = QDir::homePath()+"/.config/mudlet/profiles/";
    directoryLogFile.append(mHostName);
    directoryLogFile.append("/log");
    QString logFileName = directoryLogFile + "/errors.txt";
    QDir dirLogFile;
    if( ! dirLogFile.exists( directoryLogFile ) )
    {
        dirLogFile.mkpath( directoryLogFile );
    }
    mErrorLogFile.setFileName( logFileName );
    mErrorLogFile.open( QIODevice::Append );
    mErrorLogStream.setDevice( &mErrorLogFile );
    mpMap->restore();
    mpMap->init( this );
    mMapStrongHighlight = false;
}

Host::Host()
: mTelnet( this )
, mpConsole( 0 )
, mLuaInterpreter    ( this, 0 )
, mTriggerUnit       ( this )
, mTimerUnit         ( this )
, mScriptUnit        ( this )
, mAliasUnit         ( this )
, mActionUnit        ( this )
, mKeyUnit           ( this )
, commandLineMinimumHeight( 30 )
, mAlertOnNewData( true )
, mAllowToSendCommand( true )
, mAutoClearCommandLineAfterSend( false )
, mBlockScriptCompile( true )
, mBorderBottomHeight( 0 )
, mBorderLeftWidth( 0 )
, mBorderRightWidth( 0 )
, mBorderTopHeight( 0 )
, mCodeCompletion( true )
, mCommandLineFont   ( QFont("Bitstream Vera Sans Mono", 10, QFont::Courier ) )//( QFont("Monospace", 10, QFont::Courier) )
, mCommandSeparator  ( QString(";") )
, mCommandSeperator  ( QString(";") )
, mDisableAutoCompletion( false )
, mDisplayFont       ( QFont("Bitstream Vera Sans Mono", 10, QFont::Courier ) )//, mDisplayFont       ( QFont("Bitstream Vera Sans Mono", 10, QFont:://( QFont("Monospace", 10, QFont::Courier) ), mPort              ( port )
, mEnableGMCP( false )
, mFORCE_GA_OFF( false )
, mFORCE_NO_COMPRESSION( false )
, mFORCE_SAVE_ON_EXIT( false )
, mHostID( 0 )
, mHostName( "default-host" )
, mInsertedMissingLF( false )
, mIsGoingDown( false )
, mLF_ON_GA( true )
, mLogin( "" )
, mMainIconSize( 3 )
, mNoAntiAlias( false )
, mPass( "" )
, mpEditorDialog(0)
, mpMap( new TMap( this ) )
, mpNotePad( 0 )
, mPort(23)
, mPrintCommand( true )
, mRawStreamDump( false )
, mResetProfile( false )
, mRetries( 5 )
, mSaveProfileOnExit( false )
, mScreenHeight( 25 )
, mScreenWidth( 90 )
, mTEFolderIconSize( 3 )
, mTimeout( 60 )
, mUSE_FORCE_LF_AFTER_PROMPT( false )
, mUSE_IRE_DRIVER_BUGFIX( true )
, mUSE_UNIX_EOL( false )
, mWrapAt( 100 )
, mWrapIndentCount( 0 )
, mBlack             ( QColor(  0,  0,  0) )
, mLightBlack        ( QColor(128,128,128) )
, mRed               ( QColor(128,  0,  0) )
, mLightRed          ( QColor(255,  0,  0) )
, mLightGreen        ( QColor(  0,255,  0) )
, mGreen             ( QColor(  0,179,  0) )
, mLightBlue         ( QColor(  0,  0,255) )
, mBlue              ( QColor(  0,  0,128) )
, mLightYellow       ( QColor(255,255,  0) )
, mYellow            ( QColor(128,128,  0) )
, mLightCyan         ( QColor(  0,255,255) )
, mCyan              ( QColor(  0,128,128) )
, mLightMagenta      ( QColor(255,  0,255) )
, mMagenta           ( QColor(128,  0,128) )
, mLightWhite        ( QColor(255,255,255) )
, mWhite             ( QColor(192,192,192) )
, mFgColor           ( QColor(192,192,192) )
, mBgColor           ( QColor(  0,  0,  0) )
, mCommandBgColor    ( QColor(  0,  0,  0) )
, mCommandFgColor    ( QColor(113,113,  0) )
, mBlack_2             ( QColor(  0,  0,  0, 255) )
, mLightBlack_2        ( QColor(128,128,128, 255) )
, mRed_2               ( QColor(128,  0,  0, 255) )
, mLightRed_2          ( QColor(255,  0,  0, 255) )
, mLightGreen_2        ( QColor(  0,255,  0, 255) )
, mGreen_2             ( QColor(  0,179,  0, 255) )
, mLightBlue_2         ( QColor(  0,  0, 255, 255) )
, mBlue_2              ( QColor(  0,  0, 128, 255) )
, mLightYellow_2       ( QColor(255,255,  0, 255) )
, mYellow_2            ( QColor(128,128,  0, 255) )
, mLightCyan_2         ( QColor(  0,255,255, 255) )
, mCyan_2              ( QColor(  0,128,128, 255) )
, mLightMagenta_2      ( QColor(255,  0,255, 255) )
, mMagenta_2           ( QColor(128,  0,128, 255) )
, mLightWhite_2        ( QColor(255,255,255, 255) )
, mWhite_2             ( QColor(192,192,192, 255) )
, mFgColor_2           ( QColor(192,192,192, 255) )
, mBgColor_2           ( QColor(  0,  0,  0, 255) )
{
    QString directoryLogFile = QDir::homePath()+"/.config/mudlet/profiles/";
    directoryLogFile.append(mHostName);
    directoryLogFile.append("/log");
    QString logFileName = directoryLogFile + "/errors.txt";
    QDir dirLogFile;
    if( ! dirLogFile.exists( directoryLogFile ) )
    {
        dirLogFile.mkpath( directoryLogFile );
    }
    mErrorLogFile.setFileName( logFileName );
    mErrorLogFile.open( QIODevice::Append );
    mErrorLogStream.setDevice( &mErrorLogFile );
    mpMap->restore();
    mpMap->init( this );
    mMapStrongHighlight = false;

}

Host::~Host()
{
    mErrorLogStream.flush();
    mErrorLogFile.close();
}

void Host::resetProfile()
{
    getTimerUnit()->stopAllTriggers();
    mudlet::self()->mTimerMap.clear();
    getTimerUnit()->removeAllTempTimers();
    getTriggerUnit()->removeAllTempTriggers();



    mTimerUnit.doCleanup();
    mTriggerUnit.doCleanup();
    mpConsole->resetMainConsole();
    mEventHandlerMap.clear();
    mEventMap.clear();
    mLuaInterpreter.initLuaGlobals();
    mLuaInterpreter.loadGlobal();
    mBlockScriptCompile = false;


    getTriggerUnit()->compileAll();
    getAliasUnit()->compileAll();
    getActionUnit()->compileAll();
    getKeyUnit()->compileAll();
    getScriptUnit()->compileAll();
    //getTimerUnit()->compileAll();
    mResetProfile = false;

    mTimerUnit.reenableAllTriggers();

    TEvent event;
    event.mArgumentList.append( "sysLoadEvent" );
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    raiseEvent( & event );
    qDebug()<<"resetProfile() DONE";
}

void Host::assemblePath()
{
    QStringList list;
    for( int i=0; i<mpMap->mPathList.size(); i++ )
    {
        QString n = QString::number( mpMap->mPathList[i]);
        list.append( n );
    }
    QStringList list2;
    for( int i=0; i<mpMap->mDirList.size(); i++ )
    {
        QString n = mpMap->mDirList[i];
        list2.append( n );
    }
    QString t1 = "speedWalkPath";
    mLuaInterpreter.set_lua_table( t1, list );
    QString t2 = "speedWalkDir";
    mLuaInterpreter.set_lua_table( t2, list2 );
}

void Host::startSpeedWalk()
{
    QStringList list;
    for( int i=0; i<mpMap->mPathList.size(); i++ )
    {
        QString n = QString::number( mpMap->mPathList[i]);
        list.append( n );
    }
    QStringList list2;
    for( int i=0; i<mpMap->mDirList.size(); i++ )
    {
        QString n = mpMap->mDirList[i];
        list2.append( n );
    }
    QString t1 = "speedWalkPath";
    mLuaInterpreter.set_lua_table( t1, list );
    QString t2 = "speedWalkDir";
    mLuaInterpreter.set_lua_table( t2, list2 );
    QString f = "doSpeedWalk";
    QString n = "";
    mLuaInterpreter.call( f, n );
}

void Host::adjustNAWS()
{
    mTelnet.setDisplayDimensions();
}

void Host::setReplacementCommand( QString s )
{
    mReplacementCommand = s;
}

void Host::stopAllTriggers()
{
    mTriggerUnit.stopAllTriggers();
    mAliasUnit.stopAllTriggers();
    mTimerUnit.stopAllTriggers();
}

void Host::reenableAllTriggers()
{
    mTriggerUnit.reenableAllTriggers();
    mAliasUnit.reenableAllTriggers();
    mTimerUnit.reenableAllTriggers();
}

void Host::send( QString cmd, bool wantPrint, bool dontExpandAliases )
{
    if( wantPrint && mPrintCommand )
    {
        mInsertedMissingLF = true;
        if( (cmd == "") && ( mUSE_IRE_DRIVER_BUGFIX ) && ( ! mUSE_FORCE_LF_AFTER_PROMPT ) )
        {
            ;
        }
        else
        {
            mpConsole->printCommand( cmd ); // used to print the terminal <LF> that terminates a telnet command
                                            // this is important to get the cursor position right
        }
        mpConsole->update();
    }
    QStringList commandList = cmd.split( QString( mCommandSeparator ), QString::SkipEmptyParts );
    if( ! dontExpandAliases )
    {
        if( commandList.size() == 0 )
        {
            sendRaw( "\n" );//NOTE: damit leerprompt moeglich sind
            return;
        }
    }
    for( int i=0; i<commandList.size(); i++ )
    {
        if( commandList[i].size() < 1 ) continue;
        QString command = commandList[i];
        command.replace("\n", "");
        mReplacementCommand = "";
        if( dontExpandAliases )
        {
            mTelnet.sendData( command );
            continue;
        }
        if( ! mAliasUnit.processDataStream( command ) )
        {
            if( mReplacementCommand.size() > 0 )
            {
                mTelnet.sendData( mReplacementCommand );
            }
            else
            {
                mTelnet.sendData( command );
            }
        }
    }
}

void Host::sendRaw( QString command )
{
    mTelnet.sendData( command );
}


/*QStringList Host::getBufferTable( int from, int to )
{
    QStringList bufList;
    if( (mTextBufferList.size()-1-to<0) || (mTextBufferList.size()-1-from<0) || (mTextBufferList.size()-1-from>=mTextBufferList.size()) || mTextBufferList.size()-1-to>=mTextBufferList.size() )
    {
        return bufList << QString("ERROR: buffer out of range");
    }
    for( int i=mTextBufferList.size()-1-from; i>=0; i-- )
    {
        if( i < mTextBufferList.size()-1-to ) break;
        bufList << mTextBufferList[i];
    }
    return bufList;
}

QString Host::getBufferLine( int line )
{
    QString text;
    if( (line < 0) || (mTextBufferList.size()-1-line>=mTextBufferList.size()) )
    {
        text = "ERROR: buffer out of range";
        return text;
    }
    text = mTextBufferList[mTextBufferList.size()-1-line];
    return text;
} */

int Host::createStopWatch()
{
    int newWatchID = mStopWatchMap.size()+1;
    mStopWatchMap[newWatchID] = QTime(0,0,0,0);
    return newWatchID;
}

double Host::getStopWatchTime( int watchID )
{
    if( mStopWatchMap.contains( watchID ) )
    {
        return static_cast<double>(mStopWatchMap[watchID].elapsed())/1000;
    }
    else
    {
        return -1.0;
    }
}

bool Host::startStopWatch( int watchID )
{
    if( mStopWatchMap.contains( watchID ) )
    {
        mStopWatchMap[watchID].start();
        return true;
    }
    else
        return false;
}

double Host::stopStopWatch( int watchID )
{
    if( mStopWatchMap.contains( watchID ) )
    {
        return static_cast<double>(mStopWatchMap[watchID].elapsed())/1000;
    }
    else
    {
        return -1.0;
    }
}

bool Host::resetStopWatch( int watchID )
{
    if( mStopWatchMap.contains( watchID ) )
    {
        mStopWatchMap[watchID].setHMS(0,0,0,0);
        return true;
    }
    else
        return false;
}

void Host::callEventHandlers()
{

}

void Host::incomingStreamProcessor( QString & data, int line )
{
    mTriggerUnit.processDataStream( data, line );

    mTimerUnit.doCleanup();
    if( mResetProfile )
    {
        resetProfile();
    }
}

void Host::registerEventHandler( QString name, TScript * pScript )
{
    if( mEventHandlerMap.contains( name ) )
    {
        mEventHandlerMap[name].append( pScript );
    }
    else
    {
        QList<TScript *> scriptList;
        scriptList.append( pScript );
        mEventHandlerMap.insert( name, scriptList );
    }
}

void Host::unregisterEventHandler( QString name, TScript * pScript )
{
    if( mEventHandlerMap.contains( name ) )
    {
        mEventHandlerMap[name].removeAll( pScript );
    }
}

void Host::raiseEvent( TEvent * pE )
{
    if( pE->mArgumentList.size() < 1 ) return;
    if( ! mEventHandlerMap.contains( pE->mArgumentList[0] ) ) return;
    QList<TScript *> scriptList = mEventHandlerMap.value( pE->mArgumentList[0] );
    for( int i=0; i<scriptList.size(); i++ )
    {
        scriptList.value( i )->callEventHandler( pE );
    }
}

void Host::enableTimer( QString & name )
{
    mTimerUnit.enableTimer( name );
}

void Host::disableTimer( QString & name )
{
    mTimerUnit.disableTimer( name );
}

bool Host::killTimer( QString & name )
{
    return mTimerUnit.killTimer( name );
}

void Host::enableKey( QString & name )
{
    mKeyUnit.enableKey( name );
}

void Host::disableKey( QString & name )
{
    mKeyUnit.disableKey( name );
}


void Host::enableTrigger( QString & name )
{
    mTriggerUnit.enableTrigger( name );
}

void Host::disableTrigger( QString & name )
{
    mTriggerUnit.disableTrigger( name );
}

bool Host::killTrigger( QString & name )
{
    return mTriggerUnit.killTrigger( name );
}


void Host::connectToServer()
{
    mTelnet.connectIt( mUrl, mPort );
}

bool Host::serialize()
{
    return false;
    if( ! mSaveProfileOnExit )
    {
        return true;
    }
    QString directory_xml = QDir::homePath()+"/.config/mudlet/profiles/"+mHostName+"/current";
    QString filename_xml = directory_xml + "/"+QDateTime::currentDateTime().toString("dd-MM-yyyy#hh-mm-ss")+".xml";
    QDir dir_xml;
    if( ! dir_xml.exists( directory_xml ) )
    {
        dir_xml.mkpath( directory_xml );
    }
    QDir dir_map;
    QString directory_map = QDir::homePath()+"/.config/mudlet/profiles/"+mHostName+"/map";
    QString filename_map = directory_map + "/"+QDateTime::currentDateTime().toString("dd-MM-yyyy#hh-mm-ss")+"map.dat";
    if( ! dir_map.exists( directory_map ) )
    {
        dir_map.mkpath( directory_map );
    }

    QFile file_xml( filename_xml );
    if ( file_xml.open( QIODevice::WriteOnly ) )
    {
        XMLexport writer( this );
        writer.exportHost( & file_xml );
        file_xml.close();
    }
    else
    {
        QMessageBox::critical( 0, "Profile Save Failed", "Failed to save "+mHostName+" to location "+filename_xml+" because of the following error: "+file_xml.errorString() );
    }

    if( mpMap->rooms.size() > 10 )
    {
        QFile file_map( filename_map );
        if ( file_map.open( QIODevice::WriteOnly ) )
        {
            QDataStream out( & file_map );
            mpMap->serialize( out );
            file_map.close();
        }
        else
        {
            QMessageBox::critical( 0, "Profile Save Failed", "Failed to save "+mHostName+" to location "+filename_xml+" because of the following error: "+file_xml.errorString() );
        }
    }
    return true;
}


bool Host::closingDown()
{
    QMutexLocker locker(& mLock);
    bool shutdown = mIsClosingDown;
    return shutdown;
}


void Host::orderShutDown()
{
    QMutexLocker locker(& mLock);
    mIsClosingDown = true;
}



#endif


