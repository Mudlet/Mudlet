
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



#ifndef _HOST_CPP_
#define _HOST_CPP_

#include <QString>
#include "Host.h"
#include "ctelnet.h"
#include <QDataStream>
#include <QFile>
#include <QDir>
#include "mudlet.h"
#include "TEvent.h"


Host::Host( int port, QString hostname, QString login, QString pass, int id ) 
: mHostName          ( hostname )
, mLogin             ( login )
, mPass              ( pass )
, mLuaInterpreter    ( this, id )
, mTimeout           ( 60 )
, mRetries           ( 5 )
, mPort              ( port )
, mTriggerUnit       ( this )
, mTimerUnit         ( this )
, mScriptUnit        ( this )
, mAliasUnit         ( this )
, mActionUnit        ( this )
, mKeyUnit           ( this )
, mTelnet            ( this )
, mHostID            ( id )
, mBlack             ( QColor(0,0,0) )
, mLightBlack        ( QColor(50,50,50) )
, mRed               ( QColor(255,0,0) )
, mLightRed          ( QColor(255,100,100) )
, mLightGreen        ( QColor(100,255,100) )
, mGreen             ( QColor(0,255,0) )
, mLightBlue         ( QColor(100,100,255) )
, mBlue              ( QColor(0,0,255) )
, mLightYellow       ( QColor(255,255,100) )
, mYellow            ( QColor(255,255,0) )
, mLightCyan         ( QColor(100,255,255) )
, mCyan              ( QColor(0,255,255) ) 
, mLightMagenta      ( QColor(255,100,255) )
, mMagenta           ( QColor(255,0,255) )
, mLightWhite        ( QColor(155,155,155) )
, mWhite             ( QColor(255,255,255) )
, mFgColor           ( QColor(255,255,255) )
, mBgColor           ( QColor(0,0,0) )
, mDisplayFont       ( QFont("Courier New", 10, QFont::Courier) )
, mCommandLineFont   ( QFont("Courier New", 10, QFont::Courier) )
, mCommandSeperator  ( QString(";") )
, mWrapAt( 80 )    
, mWrapIndentCount( 5 )
, mPrintCommand( true )
, mAutoClearCommandLineAfterSend( false )
, mCommandSeparator( ';' )
, mDisableAutoCompletion( false )
{
}

Host::~Host()
{
}

void Host::setReplacementCommand( QString s )
{
    mReplacementCommand = s;    
}

void Host::send( QString command )
{ 
    mReplacementCommand = "";
    //command.append(QChar('\n'));
    mpConsole->printCommand( command );
    //qDebug()<<"Host<name="<<mHostName<<", ID="<<mHostID<<" sending command:"<<command;
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

void Host::sendRaw( QString command )
{ 
    mTelnet.sendData( command ); 
}


QStringList Host::getBufferTable( int from, int to )
{
    QStringList bufList;
    if( (mTextBufferList.size()-1-to<0) || (mTextBufferList.size()-1-from<0) || (mTextBufferList.size()-1-from>=mTextBufferList.size()) || mTextBufferList.size()-1-to>=mTextBufferList.size() )
    {
        return bufList<<QString("ERROR: buffer out of range");
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
}

void Host::incomingStreamProcessor( QString & data, QString & prompt )
{
    mTextBufferList.append( data );
    if( mTextBufferList.size() > 500 )
    {
        mTextBufferList.pop_front();    
    }
    mTriggerUnit.processDataStream( data );
    
    QList<QString> eventList = mEventMap.keys();
    for( int i=0; i<eventList.size(); i++ )
    {
        if( ! mEventHandlerMap.contains( eventList[i] ) ) continue;
        QList<TScript *> scriptList = mEventHandlerMap.value( eventList[i] );
        for( int ii=0; ii<scriptList.size(); ii++ )
        {
            scriptList.value( ii )->callEventHandler( eventList[i], mEventMap.value( eventList[i] ) );
            
            delete mEventMap.value( eventList[i] );
        }
    }
    
    mEventMap.clear();
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
    mEventMap.insertMulti( pE->mArgumentList[0], pE );    
}

void Host::gotRest( QString & data )
{
    mRest = data; 
    if( mpConsole )
    {
        mpConsole->printOnDisplay( data );
    }
}

void Host::gotLine( QString & data )
{
    if( mpConsole )
    {
        mpConsole->printOnDisplay( data );
    }
}

void Host::gotPrompt( QString & data )
{
    mPrompt = data;
    QString promptVar("prompt");
    mLuaInterpreter.set_lua_string( promptVar, mPrompt ); 
    if( mpConsole )
    {
        mpConsole->printOnDisplay( data );
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

void Host::killTimer( QString & name )
{
    mTimerUnit.killTimer( name );    
}

QStringList Host::getLastBuffer()
{
    return mTextBufferList;    
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

void Host::killTrigger( QString & name )
{
    mTriggerUnit.killTrigger( name );    
}


void Host::connectToServer()
{
    mTelnet.connectIt( mUrl, mPort );     
}

bool Host::serialize()
{
    QString directory = QDir::homePath()+"/.config/mudlet/profiles/";
    directory.append( mHostName );
    QString filename = directory + "/Host.dat";
    QDir dir;
    if( ! dir.exists( directory ) )
    {
        dir.mkpath( directory );    
    }
    QFile file( filename );
    file.open(QIODevice::WriteOnly);
    QDataStream ofs(&file); 
    ofs << mHostName;
    ofs << mLogin;
    ofs << mPass;
    ofs << mUrl;
    ofs << mTimeout;
    ofs << mRetries;
    ofs << mPort;
    ofs << mUserDefinedName;
    //ofs << mHostID;
    file.close();
    
    serialize_options2( directory );
    
    // serialize TriggerUnit
    saveTriggerUnit( directory );    
    saveTimerUnit( directory );
    saveAliasUnit( directory );
    saveScriptUnit( directory );
    saveActionUnit( directory );
    saveKeyUnit( directory );
    saveOptions( directory );    
    return true;
}

void Host::serialize_options2( QString directory )
{
    QString filename = directory + "/Host_options2.dat";
    QDir dir;
    if( ! dir.exists( directory ) )
    {
        dir.mkpath( directory );    
    }
    QFile file( filename );
    file.open(QIODevice::WriteOnly);
    QDataStream ofs(&file); 
    ofs << mWrapAt;
    ofs << mWrapIndentCount;
    ofs << mPrintCommand;
    ofs << mAutoClearCommandLineAfterSend;
    ofs << mCommandSeperator;
    ofs << mDisableAutoCompletion;
    file.close();
}

void Host::restore_options2( QString directory )
{
    QString filename = directory + "/Host_options2.dat";
    QFile file( filename );
    file.open(QIODevice::ReadOnly);
    QDataStream ifs(&file); 
    ifs >> mWrapAt;
    ifs >> mWrapIndentCount;
    ifs >> mPrintCommand;
    ifs >> mAutoClearCommandLineAfterSend;
    ifs >> mCommandSeperator;
    ifs >> mDisableAutoCompletion;
    file.close();
}

bool Host::exportHost( QString userDir )
{
    mHostName.append(QDateTime::currentDateTime().toString());
    mHostName.replace(" ", "");
    mHostName.replace(":","-");
    QString directory = userDir+mHostName;
    QString filename = directory + "/Host.dat";
    QDir dir;
    if( ! dir.exists( directory ) )
    {
        dir.mkpath( directory );    
    }
    QFile file( filename );
    file.open(QIODevice::WriteOnly);
    QDataStream ofs(&file); 
    ofs << mHostName;
    qDebug()<<"saving profile:"<<mHostName;
    ofs << mLogin;
    ofs << mPass;
    ofs << mUrl;
    ofs << mTimeout;
    ofs << mRetries;
    ofs << mPort;
    ofs << mUserDefinedName;
    //ofs << mHostID;
    file.close();
    
    // serialize TriggerUnit
    saveTriggerUnit( directory );    
    saveTimerUnit( directory );
    saveAliasUnit( directory );
    saveScriptUnit( directory );
    saveActionUnit( directory );
    saveKeyUnit( directory );
    saveOptions( directory );
    return true;
}

bool Host::importHost( QString directory )
{
    //FIXME windows
    QString filename = directory + "/Host.dat";
    QFile file( filename );
    file.open(QIODevice::ReadOnly);
    QDataStream ifs(&file); 
    ifs >> mHostName;
    ifs >> mLogin;
    ifs >> mPass;
    ifs >> mUrl;
    ifs >> mTimeout;
    ifs >> mRetries;
    ifs >> mPort;
    ifs >> mUserDefinedName;
    //ifs >> mHostID;
    file.close();
    
    // loading TriggerUnit 
    loadTriggerUnit( directory );    
    loadTimerUnit( directory );
    loadAliasUnit( directory );
    loadScriptUnit( directory );
    loadActionUnit( directory );
    loadKeyUnit( directory );
    loadOptions( directory );
    mScriptUnit.compileAll();
    return true;
}

bool Host::restore( QString directory )
{
    QString filename = directory + "/Host.dat";
    QFile file( filename );
    file.open(QIODevice::ReadOnly);
    QDataStream ifs(&file); 
    ifs >> mHostName;
    ifs >> mLogin;
    ifs >> mPass;
    ifs >> mUrl;
    ifs >> mTimeout;
    ifs >> mRetries;
    ifs >> mPort;
    ifs >> mUserDefinedName;
    //ifs >> mHostID;
    file.close();
    
    // loading TriggerUnit 
    loadTriggerUnit( directory );    
    loadTimerUnit( directory );
    loadAliasUnit( directory );
    loadScriptUnit( directory );
    loadActionUnit( directory );
    loadKeyUnit( directory );
    loadOptions( directory );
    mScriptUnit.compileAll();
    return true;
}

void Host::saveOptions(QString directory )
{
    QString filename = directory + "/Options.dat";
    qDebug()<<"serializing to path: "<<filename;
    QDir dir;
    if( ! dir.exists( directory ) )
    {
        dir.mkpath( directory );    
    }
    QFile file( filename );
    file.open( QIODevice::WriteOnly );
    QDataStream ofs( &file ); 
    ofs << mFgColor;
    ofs << mBgColor;
    ofs << mBlack;
    ofs << mLightBlack;
    ofs << mRed;
    ofs << mLightRed;
    ofs << mBlue;
    ofs << mLightBlue;
    ofs << mGreen;
    ofs << mLightGreen;
    ofs << mYellow;
    ofs << mLightYellow;
    ofs << mCyan;
    ofs << mLightCyan;
    ofs << mMagenta;
    ofs << mLightMagenta;
    ofs << mWhite;
    ofs << mLightWhite;
    ofs << mDisplayFont;
    ofs << mCommandLineFont;
    ofs << mCommandSeperator;
    file.close();
}

void Host::loadOptions( QString directory )
{
    QString filename = directory + "/Options.dat";
    qDebug()<< "restoring data from path:"<<filename;
    QFile file( filename );
    if( file.exists() )
    {
        qDebug()<< "restoring ***OPTIONS*** data from path:"<<filename;
        file.open( QIODevice::ReadOnly );
        QDataStream ifs( &file ); 
        ifs >> mFgColor;
        ifs >> mBgColor;
        ifs >> mBlack;
        ifs >> mLightBlack;
        ifs >> mRed;
        ifs >> mLightRed;
        ifs >> mBlue;
        ifs >> mLightBlue;
        ifs >> mGreen;
        ifs >> mLightGreen;
        ifs >> mYellow;
        ifs >> mLightYellow;
        ifs >> mCyan;
        ifs >> mLightCyan;
        ifs >> mMagenta;
        ifs >> mLightMagenta;
        ifs >> mWhite;
        ifs >> mLightWhite;
        ifs >> mDisplayFont;
        ifs >> mCommandLineFont;
        ifs >> mCommandSeperator;
        file.close();
    }
}


void Host::saveTriggerUnit(QString directory )
{
    QString filename = directory + "/Triggers.dat";
    qDebug()<<"serializing to path: "<<filename;
    QDir dir;
    if( ! dir.exists( directory ) )
    {
        dir.mkpath( directory );    
    }
    QFile file( filename );
    file.open( QIODevice::WriteOnly );
    QDataStream ofs( &file ); 
    mTriggerUnit.serialize( ofs );
    file.close();
}

void Host::loadTriggerUnit( QString directory )
{
    QString filename = directory + "/Triggers.dat";
    qDebug()<< "restoring data from path:"<<filename;
    QFile file( filename );
    file.open( QIODevice::ReadOnly );
    QDataStream ifs( &file ); 

    mTriggerUnit.restore( ifs );
    file.close();
}

void Host::saveTimerUnit(QString directory )
{
    QString filename = directory + "/Timers.dat";
    qDebug()<<"serializing to path: "<<filename;
    QDir dir;
    if( ! dir.exists( directory ) )
    {
        dir.mkpath( directory );    
    }
    QFile file( filename );
    file.open( QIODevice::WriteOnly );
    QDataStream ofs( &file ); 
    mTimerUnit.serialize( ofs );
    file.close();
}

void Host::loadTimerUnit( QString directory )
{
    QString filename = directory + "/Timers.dat";
    qDebug()<< "restoring data from path:"<<filename;
    QFile file( filename );
    file.open( QIODevice::ReadOnly );
    QDataStream ifs( &file ); 
    
    mTimerUnit.restore( ifs );
    file.close();
}

void Host::saveScriptUnit(QString directory )
{
    QString filename = directory + "/Scripts.dat";
    qDebug()<<"serializing to path: "<<filename;
    QDir dir;
    if( ! dir.exists( directory ) )
    {
        dir.mkpath( directory );    
    }
    QFile file( filename );
    file.open( QIODevice::WriteOnly );
    QDataStream ofs( &file ); 
    mScriptUnit.serialize( ofs );
    file.close();
}

void Host::loadScriptUnit( QString directory )
{
    QString filename = directory + "/Scripts.dat";
    qDebug()<< "restoring data from path:"<<filename;
    QFile file( filename );
    file.open( QIODevice::ReadOnly );
    QDataStream ifs( &file ); 
    
    mScriptUnit.restore( ifs );
    file.close();
}

void Host::saveAliasUnit(QString directory )
{
    QString filename = directory + "/Aliases.dat";
    qDebug()<<"serializing to path: "<<filename;
    QDir dir;
    if( ! dir.exists( directory ) )
    {
        dir.mkpath( directory );    
    }
    QFile file( filename );
    file.open( QIODevice::WriteOnly );
    QDataStream ofs( &file ); 
    mAliasUnit.serialize( ofs );
    file.close();
}

void Host::loadAliasUnit( QString directory )
{
    QString filename = directory + "/Aliases.dat";
    qDebug()<< "restoring data from path:"<<filename;
    QFile file( filename );
    file.open( QIODevice::ReadOnly );
    QDataStream ifs( &file ); 
    
    mAliasUnit.restore( ifs );
    file.close();
}

void Host::saveKeyUnit(QString directory )
{
    QString filename = directory + "/Keys.dat";
    qDebug()<<"serializing to path: "<<filename;
    QDir dir;
    if( ! dir.exists( directory ) )
    {
        dir.mkpath( directory );    
    }
    QFile file( filename );
    file.open( QIODevice::WriteOnly );
    QDataStream ofs( &file ); 
    mKeyUnit.serialize( ofs );
    file.close();
}

void Host::loadKeyUnit( QString directory )
{
    QString filename = directory + "/Keys.dat";
    qDebug()<< "restoring data from path:"<<filename;
    QFile file( filename );
    file.open( QIODevice::ReadOnly );
    QDataStream ifs( &file ); 
    
    mKeyUnit.restore( ifs );
    file.close();
}

void Host::saveActionUnit(QString directory )
{
    QString filename = directory + "/Actions.dat";
    qDebug()<<"serializing Actions to path: "<<filename;
    QDir dir;
    if( ! dir.exists( directory ) )
    {
        dir.mkpath( directory );    
    }
    QFile file( filename );
    file.open( QIODevice::WriteOnly );
    QDataStream ofs( &file ); 
    mActionUnit.serialize( ofs );
    file.close();
}

void Host::loadActionUnit( QString directory )
{
    QString filename = directory + "/Actions.dat";
    qDebug()<< "restoring Actions from path:"<<filename;
    QFile file( filename );
    file.open( QIODevice::ReadOnly );
    QDataStream ifs( &file ); 
    
    mActionUnit.restore( ifs );
    file.close();
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


