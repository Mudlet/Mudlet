
/***************************************************************************
 *   Copyright (C) 2008 by Heiko Koehn                                     *
 *   KoehnHeiko@googlemail.com                                             *
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
, mFgColor           ( QColor(255,255,255) )
, mBgColor           ( QColor(  0,  0,  0) )
, mDisplayFont       ( QFont("Bitstream Vera Sans Mono", 10, QFont::Courier ) )//, mDisplayFont       ( QFont("Bitstream Vera Sans Mono", 10, QFont:://( QFont("Monospace", 10, QFont::Courier) )
, mCommandLineFont   ( QFont("Bitstream Vera Sans Mono", 10, QFont::Courier ) )//( QFont("Monospace", 10, QFont::Courier) )
, mCommandSeperator  ( QString(";") )
, mWrapAt( 100 )
, mWrapIndentCount( 0 )
, mPrintCommand( true )
, mAutoClearCommandLineAfterSend( false )
, mCommandSeparator( ';' )
, mDisableAutoCompletion( false )
, mSaveProfileOnExit( false )
, mUSE_IRE_DRIVER_BUGFIX( false )
, mScreenWidth( 90 )
, mScreenHeight( 25 )
, mUSE_FORCE_LF_AFTER_PROMPT( false )
, mBorderTopHeight( 0 )
, mBorderBottomHeight( 0 )
, mBorderLeftWidth( 0 )
, mBorderRightWidth( 0 )
, mUSE_UNIX_EOL( false )
, mBlockScriptCompile( true )
{
}

Host::~Host()
{
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

void Host::send( QString cmd, bool dontExpandAliases )
{  
    if( mPrintCommand )
    {
        if( (cmd == "") && ( mUSE_IRE_DRIVER_BUGFIX ) && ( ! mUSE_FORCE_LF_AFTER_PROMPT ) )
        {
            ;
        }
        else
        {
            mpConsole->printCommand( cmd ); // used to print the terminal <LF> that terminates a telnet command
                                            // this is important to get the cursor position right
        }
    }
    else
    {
        mpConsole->update();
    }

    QStringList commandList = cmd.split( QString( mCommandSeparator ), QString::SkipEmptyParts );
    if( ! dontExpandAliases )
    {
        if( commandList.size() == 0 ) 
        {
            sendRaw( "" );
            return;
        }
    }

    for( int i=0; i<commandList.size(); i++ )
    {
        QString command = commandList[i].replace(QChar('\n'),"");
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
        return static_cast<double>(mStopWatchMap[watchID].elapsed()/1000);
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
        return static_cast<double>(mStopWatchMap[watchID].elapsed()/1000);
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

void Host::incomingStreamProcessor( QString & data, QString & prompt )
{
    mTriggerUnit.processDataStream( data );
    // TODO: reihenfolge garantieren
    QList<QString> eventList = mEventMap.keys();
    for( int i=0; i<eventList.size(); i++ )
    {
        if( ! mEventHandlerMap.contains( eventList[i] ) ) continue;
        QList<TScript *> scriptList = mEventHandlerMap.value( eventList[i] );
        for( int ii=0; ii<scriptList.size(); ii++ )
        {
            scriptList.value( ii )->callEventHandler( eventList[i], mEventMap.value( eventList[i] ) );
        }
    }
    
    mEventMap.clear();
    mTimerUnit.doCleanup();
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

/*QStringList Host::getLastBuffer()
{
    return mTextBufferList;    
} */

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
    
    return true;
    
    QString directory = QDir::homePath()+"/.config/mudlet/profiles/";
    directory.append( mHostName );
    QString filename = directory + "/Host.dat";
    QDir dir;
    if( ! dir.exists( directory ) )
    {
        dir.mkpath( directory );    
    }
    QFile file( filename );
    file.open( QIODevice::Append );
    QDataStream ofs(&file); 
    ofs << mHostName;
    ofs << mLogin;
    ofs << mPass;
    ofs << mUrl;
    ofs << mTimeout;
    ofs << mRetries;
    ofs << mPort;
    ofs << mUserDefinedName;
    qDebug()<<"SERIALIZING: hostname="<<mHostName<<" url="<<mUrl<<" login="<<mLogin<<" pass="<<mPass;
    
    file.close();
    
    serialize_options2( directory );
    
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
    file.open( QIODevice::Append );
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
    file.close();
    
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
    file.close();
    
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



// returns an empty string as error
QString Host::readProfileData( QString profile, QString item )
{
    QFile file( QDir::homePath()+"/.config/mudlet/profiles/"+profile+"/"+item );
    
    if( ! file.exists() )
        return "";
    
    file.open( QIODevice::ReadOnly );
    
    QDataStream ifs( & file ); 
    QString ret;
    ifs >> ret;
    file.close();
    if( ifs.status() == QDataStream::Ok )
        return ret;
    else
        return "";
}

void Host::writeProfileData( QString profile, QString item, QString what )
{
    QFile file( QDir::homePath()+"/.config/mudlet/profiles/"+profile+"/"+item );
    file.open( QIODevice::WriteOnly );
    QDataStream ofs( & file ); 
    ofs << what;
    file.close();
}

void Host::writeProfileHistory( QString profile, QString item, QString what )
{
    QFile file( QDir::homePath()+"/.config/mudlet/profiles/"+profile+"/"+item );
    file.open( QIODevice::Append );
    QDataStream ofs( & file ); 
    ofs << what;
    file.close();
}

bool Host::restore( QString directory, int selectedHistoryVersion )
{
    return false;
    
    int restorableProfileCount = 0;
    
    if( selectedHistoryVersion > 0 )
    {
        qDebug()<<"\n[ LOADING ] profile history version #"<<selectedHistoryVersion<<"\n";
        restorableProfileCount = loadProfileHistory( directory, selectedHistoryVersion );
    }
    
    if( restorableProfileCount <= 0 ) 
    {
        qDebug()<< "\n[ ANALYSING ] history of "<<directory<<"\n";
        restorableProfileCount = loadProfileHistory( directory, -1 );
    }
    if( restorableProfileCount != -1 )
    {
        qDebug()<<"\n[ RESTORING ] history #"<<restorableProfileCount<<" of profile: "<<directory<<"\n";
        int load = loadProfileHistory( directory, restorableProfileCount );
        if( load == restorableProfileCount-1 )
        {
            QString profile = mHostName;
            writeProfileHistory( profile, "history_version", QDateTime::currentDateTime().toString() ); 
            qDebug()<< "\n[ OK ] restored history #"<<restorableProfileCount<<" of profile: "<<directory<<"\n";
            mScriptUnit.compileAll();
            return true;
        }
        else
        {
            qDebug()<<"\n[ ERROR ] restoring history #"<<restorableProfileCount<<" of profile: "<<directory<<" FAILED."<<"\n"; 
            return false;
        }
    }
    qDebug()<<"\n---> [ RESTORE FAILED ] profile directory:"<<directory<<"\n";
    return 
        false; //this is a new profile
    
 }

int Host::loadProfileHistory( QString directory, int restoreProfileNumber )
{
    QString host = directory + "/Host.dat";
    QFile fileHost( host );
    if( ! fileHost.exists() )
        return -1;
    fileHost.open(QIODevice::ReadOnly);
    QDataStream ifsHost(&fileHost); 
   
    QString triggerUnit = directory + "/Triggers.dat";
    QFile fileTriggerUnit( triggerUnit );
    if( ! fileTriggerUnit.exists() )
        return -1;
    fileTriggerUnit.open( QIODevice::ReadOnly );
    QDataStream ifs_triggerUnit( &fileTriggerUnit ); 
    
    QString timerUnit = directory + "/Timers.dat";
    QFile fileTimerUnit( timerUnit );
    if( ! fileTimerUnit.exists() )
        return -1;
    fileTimerUnit.open( QIODevice::ReadOnly );
    QDataStream ifs_timerUnit( &fileTimerUnit ); 
    
    QString aliasUnit = directory + "/Aliases.dat";
    QFile fileAliasUnit( aliasUnit );
    if( ! fileAliasUnit.exists() )
        return -1;
    fileAliasUnit.open( QIODevice::ReadOnly );
    QDataStream ifs_aliasUnit( &fileAliasUnit ); 
    
    QString scriptUnit = directory + "/Scripts.dat";
    QFile fileScriptUnit( scriptUnit );
    if( ! fileScriptUnit.exists() )
        return -1;
    fileScriptUnit.open( QIODevice::ReadOnly );
    QDataStream ifs_scriptUnit( &fileScriptUnit ); 
    
    QString actionUnit = directory + "/Actions.dat";
    QFile fileActionsUnit( actionUnit );
    if( ! fileActionsUnit.exists() )
        return -1;
    fileActionsUnit.open( QIODevice::ReadOnly );
    QDataStream ifs_actionsUnit( &fileActionsUnit ); 
    
    
    QString keyUnit = directory + "/Keys.dat";
    QFile fileKeyUnit( keyUnit );
    if( ! fileKeyUnit.exists() )
        return -1;
    fileKeyUnit.open( QIODevice::ReadOnly );
    QDataStream ifs_keyUnit( &fileKeyUnit ); 
    
    QString options = directory + "/Options.dat";
    QFile fileOptions( options );
    if( ! fileOptions.exists() )
        return -1;
    fileOptions.open( QIODevice::ReadOnly );
    QDataStream ifsOptions( &fileOptions ); 
    
    QString options_2 = directory + "/Host_options2.dat";
    QFile fileOptions_2( options_2 );
    if( ! fileOptions_2.exists() )
        return -1;
    fileOptions_2.open(QIODevice::ReadOnly);
    QDataStream ifsOptions_2(&fileOptions_2); 
    
    bool isRestorable = true;
    bool isRestorableUnits;
    bool isRestorableHost;
    
    int restorableProfileCount = 0;
    bool initMode = false;
    
    while( isRestorable )
    {
        qDebug() << "analysing history #"<< restorableProfileCount;
        
        // are we going to really load the profile or just analyse if it's restorable?
        if( restorableProfileCount == restoreProfileNumber )
        {
            initMode = true;
        }
        
        ifsHost >> mHostName;
        ifsHost >> mLogin;
        ifsHost >> mPass;
        ifsHost >> mUrl;
        ifsHost >> mTimeout;
        ifsHost >> mRetries;
        ifsHost >> mPort;
        ifsHost >> mUserDefinedName;
        
        qDebug()<<"----------> hostname="<<mHostName<<" url="<<mUrl<<" login="<<mLogin<<" pass="<<mPass<<" port="<<mPort;
        
        isRestorableUnits = mTriggerUnit.restore( ifs_triggerUnit, initMode );
        isRestorableUnits = mTimerUnit.restore( ifs_timerUnit, initMode ); 
        isRestorableUnits = mAliasUnit.restore( ifs_aliasUnit, initMode ); 
        isRestorableUnits = mScriptUnit.restore( ifs_scriptUnit, initMode ); 
        isRestorableUnits = mActionUnit.restore( ifs_actionsUnit, initMode ); 
        isRestorableUnits = mKeyUnit.restore( ifs_keyUnit, initMode ); 
    
        ifsOptions >> mFgColor;
        ifsOptions >> mBgColor;
        ifsOptions >> mBlack;
        ifsOptions >> mLightBlack;
        ifsOptions >> mRed;
        ifsOptions >> mLightRed;
        ifsOptions >> mBlue;
        ifsOptions >> mLightBlue;
        ifsOptions >> mGreen;
        ifsOptions >> mLightGreen;
        ifsOptions >> mYellow;
        ifsOptions >> mLightYellow;
        ifsOptions >> mCyan;
        ifsOptions >> mLightCyan;
        ifsOptions >> mMagenta;
        ifsOptions >> mLightMagenta;
        ifsOptions >> mWhite;
        ifsOptions >> mLightWhite;
        ifsOptions >> mDisplayFont;
        ifsOptions >> mCommandLineFont;
        ifsOptions >> mCommandSeperator;
    
        ifsOptions_2 >> mWrapAt;
        ifsOptions_2 >> mWrapIndentCount;
        ifsOptions_2 >> mPrintCommand;
        ifsOptions_2 >> mAutoClearCommandLineAfterSend;
        ifsOptions_2 >> mCommandSeperator;
        ifsOptions_2 >> mDisableAutoCompletion;
    
        if( ifsHost.status() == QDataStream::Ok )
            isRestorableHost = true;
        
        qDebug()<< "----------> result: isRestorableUnits="<<isRestorableUnits<<" isRestorableHost="<<isRestorableHost<<" RESULT: isRestorable="<<(bool)(isRestorableUnits && isRestorableHost);
        
        isRestorable = isRestorableUnits && isRestorableHost; //FIXME: add both options
        
        if( restorableProfileCount == restoreProfileNumber )
            break;
        
        if( isRestorable ) 
            restorableProfileCount++;
    }
    
    fileTriggerUnit.close();
    fileTimerUnit.close();
    fileAliasUnit.close();
    fileScriptUnit.close();
    fileActionsUnit.close();
    fileKeyUnit.close();
    fileOptions.close();
    fileOptions_2.close();
    
    if( ( restorableProfileCount == 0 ) && ( ! isRestorable ) )
        return -1;
    else    
        return restorableProfileCount-1;
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
    file.open( QIODevice::Append );
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
    QFile file( filename );
    if( file.exists() )
    {
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
    file.open( QIODevice::Append );
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

    mTriggerUnit.restore( ifs, true );
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
    file.open( QIODevice::Append );
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
    
    mTimerUnit.restore( ifs, true );
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
    file.open( QIODevice::Append );
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
    
    mScriptUnit.restore( ifs, true );
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
    file.open( QIODevice::Append );
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
    
    mAliasUnit.restore( ifs, true );
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
    file.open( QIODevice::Append );
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
    
    mKeyUnit.restore( ifs, true );
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
    file.open( QIODevice::Append );
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
    
    mActionUnit.restore( ifs, true );
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


