
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
#include <QUiLoader>
#include "dlgNotepad.h"
#include "zip.h"
#include "zipconf.h"
#include <errno.h>

#ifndef LUA_CPP
extern "C" {
#endif
    #include "lua.h"
    #include "lualib.h"
    #include "lauxlib.h"
#ifndef LUA_CPP
}
#endif


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
, mBlack_2             ( QColor(  36,  36,  36, 255) )
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
, mSpellDic            ( "en_US" )
, mLogStatus           ( false )
, mEnableSpellCheck    ( true )
, mModuleSaveBlock(false)
, mpUnzipDialog        ( 0 )
, mLineSize            ( 5.0 )
, mRoomSize            ( 0.5 )
, mServerGUI_Package_version( -1 )
, mServerGUI_Package_name( "nothing" )
, mAcceptServerGUI     ( true )
, mCommandLineFgColor( QColor(128,128,128) )
, mCommandLineBgColor( QColor(  0,  0,  0) )
, mFORCE_MXP_NEGOTIATION_OFF( false )
, mHaveMapperScript( false )
{
   // mLogStatus = mudlet::self()->mAutolog;
    mLuaInterface = new LuaInterface(this);
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
    mpMap->restore("");
    mpMap->init( this );
    mMapStrongHighlight = false;
    mGMCP_merge_table_keys.append("Char.Status");
    mDoubleClickIgnore.insert('"');
    mDoubleClickIgnore.insert('\'');
}

Host::~Host()
{
    mErrorLogStream.flush();
    mErrorLogFile.close();
}

void Host::saveModules(int sync)
{
    if (mModuleSaveBlock)
    {
        //FIXME: This should generate an error to the user
        return;
    }
    QMapIterator<QString, QStringList> it(modulesToWrite);
    QStringList modulesToSync;
    QString dirName = QDir::homePath()+"/.config/mudlet/moduleBackups/";
    QDir savePath = QDir(dirName);
    if (!savePath.exists())
        savePath.mkpath(dirName);
    while(it.hasNext())
    {
        it.next();
        QStringList entry = it.value();
        QString filename_xml = entry[0];
        QString time = QDateTime::currentDateTime().toString("dd-MM-yyyy#hh-mm-ss");
        QString moduleName = it.key();
        QString tempDir;
        QString zipName;
        zip * zipFile = 0;
        if ( filename_xml.endsWith( "mpackage" ) || filename_xml.endsWith( "zip" ) )
        {
            tempDir = QDir::homePath()+"/.config/mudlet/profiles/"+mHostName+"/"+moduleName;
            filename_xml = tempDir + "/" + moduleName + ".xml";
            int err;
            zipFile = zip_open( entry[0].toStdString().c_str(), 0, &err);
            zipName = filename_xml;
            QDir packageDir = QDir(tempDir);
            if ( !packageDir.exists() ){
                packageDir.mkpath(tempDir);
            }
        }
        else
        {
            savePath.rename(filename_xml,dirName+moduleName+time);//move the old file, use the key (module name) as the file
        }
        QFile file_xml( filename_xml );
        if ( file_xml.open( QIODevice::WriteOnly ) )
        {
            XMLexport writer(this);
            writer.writeModuleXML( & file_xml, it.key() );
            file_xml.close();

            if (entry[1].toInt())
                modulesToSync << it.key();
        }
        else
        {
            file_xml.close();
            //FIXME: Should have an error reported to user
            //qDebug()<<"failed to write xml for module:"<<entry[0]<<", check permissions?";
            mModuleSaveBlock = true;
            return;
        }
        if( !zipName.isEmpty() )
        {
            struct zip_source *s = zip_source_file( zipFile, filename_xml.toStdString().c_str(), 0, 0 );
            QTime t;
            t.start();
//            int err = zip_file_add( zipFile, QString(moduleName+".xml").toStdString().c_str(), s, ZIP_FL_OVERWRITE );
            int err = zip_add( zipFile, QString(moduleName+".xml").toStdString().c_str(), s );
            //FIXME: error checking
            if( zipFile )
            {
                err = zip_close( zipFile );
            }
            //FIXME: error checking
        }
    }
    modulesToWrite.clear();
    if (sync)
    {
        //synchronize modules across sessions
        QMap<Host *, TConsole *> activeSessions = mudlet::self()->mConsoleMap;
        QMapIterator<Host *, TConsole *> it2(activeSessions);
        while (it2.hasNext())
        {
            it2.next();
            Host * host = it2.key();
            if (host->mHostName == mHostName)
                continue;
            QMap<QString, QStringList> installedModules = host->mInstalledModules;
            QMap<QString, int> modulePri = host->mModulePriorities;
            QMapIterator<QString, int> it3(modulePri);
            QMap<int, QStringList> moduleOrder;
            while( it3.hasNext() )
            {
                it3.next();
                //QStringList moduleEntry = moduleOrder[it3.value()];
                //moduleEntry.append(it3.key());
                moduleOrder[it3.value()].append(it3.key());// = moduleEntry;
            }
            QMapIterator<int, QStringList> it4(moduleOrder);
            while(it4.hasNext())
            {
                it4.next();
                QStringList moduleList = it4.value();
                for(int i=0;i<moduleList.size();i++)
                {
                    QString moduleName = moduleList[i];
                    if (modulesToSync.contains(moduleName))
                    {
                        host->reloadModule(moduleName);
                    }
                }
            }
        }
    }
}

void Host::reloadModule(QString moduleName)
{
    QMap<QString, QStringList> installedModules = mInstalledModules;
    QMapIterator<QString, QStringList> it(installedModules);
    while(it.hasNext())
    {
        it.next();
        QStringList entry = it.value();
        if (it.key() == moduleName){
            uninstallPackage(it.key(),2);
            installPackage(entry[0],2);
        }
    }
    //iterate through mInstalledModules again and reset the entry flag to be correct.
    //both the installedModules and mInstalled should be in the same order now as well
    QMapIterator<QString, QStringList> it2(mInstalledModules);
    while(it2.hasNext())
    {
        it2.next();
        QStringList entry = installedModules[it2.key()];
        mInstalledModules[it2.key()] = entry;
    }
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

int Host::check_for_mappingscript()
{
    // the mapper script reminder is only shown once
    // because it is too difficult and error prone (->proper script sequence)
    // to disable this message
    bool ret = (mLuaInterpreter.check_for_mappingscript() || mHaveMapperScript);
    mHaveMapperScript = true;
    return ret;
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
        if( ! mEventHandlerMap[name].contains( pScript ) )
        {
            mEventHandlerMap[name].append( pScript );
        }
    }
    else
    {
        QList<TScript *> scriptList;
        scriptList.append( pScript );
        mEventHandlerMap.insert( name, scriptList );
    }
}
void Host::registerAnonymousEventHandler( QString name, QString fun )
{

    if( mAnonymousEventHandlerFunctions.contains( name ) )
    {
        if( ! mAnonymousEventHandlerFunctions[name].contains( fun ) )
        {
            mAnonymousEventHandlerFunctions[name].push_back( fun );
        }
    }
    else
    {
        QStringList newList;
        newList << fun;
        mAnonymousEventHandlerFunctions[name] = newList;
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
    if( mEventHandlerMap.contains( pE->mArgumentList[0] ) )
    {
        QList<TScript *> scriptList = mEventHandlerMap[pE->mArgumentList[0]];
        for( int i=0; i<scriptList.size(); i++ )
        {
            scriptList[i]->callEventHandler( pE );
        }
    }
    if( mAnonymousEventHandlerFunctions.contains( pE->mArgumentList[0] ) )
    {
        QStringList funList = mAnonymousEventHandlerFunctions[pE->mArgumentList[0]];
        for( int i=0; i<funList.size(); i++ )
        {
            mLuaInterpreter.callEventHandler( funList[i], pE );
        }
    }
}

void Host::postIrcMessage( QString a, QString b, QString c )
{
    TEvent * pE = new TEvent;
    pE->mArgumentList << "sysIrcMessage";
    pE->mArgumentList << a << b << c;
    pE->mArgumentTypeList << ARGUMENT_TYPE_STRING << ARGUMENT_TYPE_STRING << ARGUMENT_TYPE_STRING << ARGUMENT_TYPE_STRING;
    raiseEvent( pE );
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
        modulesToWrite.clear();
        XMLexport writer( this );
        writer.exportHost( & file_xml );
        file_xml.close();
        saveModules(0);
    }
    else
    {
        QMessageBox::critical( 0, "Profile Save Failed", "Failed to save "+mHostName+" to location "+filename_xml+" because of the following error: "+file_xml.errorString() );
    }

    if( mpMap->mpRoomDB->size() > 10 )
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

// this is called by the Lua function unzip() defined in LuaGlobal.lua
void Host::showUnpackingProgress( QString  txt )
{
    return;
    if( ! mpUnzipDialog ) return;
    QStringList l;
    l << txt;
    packageList->addItems( l );
    packageList->scrollToBottom();
    packageList->update();
    QApplication::sendPostedEvents();
}

bool Host::installPackage( QString fileName, int module )
{

//     Module notes:
//     For the module install, a module flag of 0 is a package, a flag
//     of 1 means the module is being installed for the first time via
//     the UI, a flag of 2 means the module is being synced (so it's "installed"
//     already), a flag of 3 means the module is being installed from
//     a script.  This separation is necessary to be able to reuse code
//     while avoiding infinite loops from script installations.

    if( fileName.isEmpty() ) return false;

    QFile file(fileName);
    if( ! file.open(QFile::ReadOnly | QFile::Text) )
    {
        return false;
    }
    QString packageName = fileName.section("/", -1);
    packageName.replace( ".zip" , "" );
    packageName.replace( "trigger", "" );
    packageName.replace( "xml", "" );
    packageName.replace( ".mpackage" , "" );
    packageName.replace( '/' , "" );
    packageName.replace( '\\' , "" );
    packageName.replace( '.' , "" );
    if ( module )
    {
        if( (module == 2) && (mActiveModules.contains( packageName ) ))
        {
            uninstallPackage(packageName, 2);
        }
        else if ( (module == 3) && ( mActiveModules.contains(packageName) ) )
        {
            return false;//we're already installed
        }
    }
    else
    {
        if( mInstalledPackages.contains( packageName ) )
        {
            return false;
        }
    }
    //the extra module check is needed here to prevent infinite loops from script loaded modules
    if( mpEditorDialog && module != 3 )
    {
        mpEditorDialog->doCleanReset();
    }
    QFile file2;
    if( fileName.endsWith(".zip") || fileName.endsWith(".mpackage") )
    {
        QString _home = QDir::homePath();
        _home.append( "/.config/mudlet/profiles/" );
        _home.append( getName() );
        QString _dest = QString( "%1/%2/").arg( _home ).arg( packageName );
        QDir _tmpDir;
        _tmpDir.mkpath(_dest);

        QUiLoader loader;
        QFile file(":/ui/package_manager_unpack.ui");
        file.open(QFile::ReadOnly);
        mpUnzipDialog = dynamic_cast<QDialog *>(loader.load( &file, 0 ) );
        file.close();
        if( ! mpUnzipDialog ) return false;
        QString _title = QString("Unpacking package: %1").arg(fileName);
        mpUnzipDialog->setWindowTitle( _title );
        mpUnzipDialog->show();
        mpUnzipDialog->raise();
        QApplication::sendPostedEvents();

        // At the moment, QuaZip is for Windows only - OSX and Linux use LuaZip as it is more commonly available
        // In the future, QuaZip will be the preferred option with LuaZip as a fallback
//        #ifndef Q_OS_WIN
//            QString _script = QString( "unzip([[%1]], [[%2]])" ).arg( fileName ).arg( _dest );
//            mLuaInterpreter.compileAndExecuteScript( _script );
//        #else
            //JlCompress::extractDir(fileName, _dest );
        int err = 0;
        //from: https://gist.github.com/mobius/1759816
        struct zip_stat zs;
        struct zip_file *zf;
//        int fd;
        long long sum;
        char buf[100];
        zip* archive = zip_open( fileName.toStdString().c_str(), 0, &err);
        if ( err != 0 )
        {
            zip_error_to_str(buf, sizeof(buf), err, errno);
            //FIXME: Tell user error
            return false;
        }
        for (int i=0;i<zip_get_num_entries( archive, 0 );i++ )
        {
            int zsi = zip_stat_index( archive, i, 0, &zs );
            if( zsi == 0 )
            {
                if ( zs.name[strlen( zs.name )-1] == '/' )
                {
                    QDir dir = QDir( _dest );
                    if ( !dir.exists( zs.name ) )
                    {
                        if ( dir.mkdir( zs.name ) == false )
                        {
                            //FIXME: report error to user
                            //qDebug()<<"error creating subdirectory: "<<QString(zs.name);
                        }
                    }
                }
                else
                {
                    zf = zip_fopen_index( archive, i, 0 );
                    if ( !zf )
                    {
                        int sep = 0;
                        zip_error_get( archive, &err, &sep);
                        zip_error_to_str(buf, sizeof(buf), err, errno);
                        //FIXME: report error to user
                        return false;
                    }
                    QFile fd(_dest+QString(zs.name));
                    fd.open(QIODevice::ReadWrite|QIODevice::Truncate);
                    if ( !fd.isOpen() )
                    {
                        //FIXME: report error to user qDebug()<<"error opening"<<_dest+QString(zs.name);
                        return false;
                    }
                    sum = 0;
                    //HEIKO: comparison between signed and unsigned
                    while( static_cast<zip_uint64_t>(sum) != zs.size )
                    {
                        int len = zip_fread( zf, buf, 100 );
                        if ( len < 0 )
                        {
                            //FIXME: report error to user qDebug()<<"zip_fread error"<<len;
                            return false;
                        }
                        fd.write( buf, len );
                        sum += len;
                    }
                    fd.close();
                    zip_fclose( zf );
                }
            }
        }
        err = zip_close( archive );
        if ( err != 0 ){
            zip_error_to_str(buf, sizeof(buf), err, errno);
            //FIXME: report error to user qDebug()<<"close file error"<<buf;
            return false;
        }
//        #endif
        QString xmlPath = _dest+packageName+".xml";
        mpUnzipDialog->close();
        mpUnzipDialog = 0;

        // requirements for zip packages:
        // - packages must be compressed in zip format
        // - file extension should be .mpackage (though .zip is accepted)
        // - there can only be a single xml file per package
        // - the xml file must be located in the root directory of the zip package. example: myPack.zip contains: the folder images and the file myPack.xml

        QDir _dir( _dest );
        // before we start importing xmls in, see if the config.lua manifest file exists
        // - if it does, update the packageName from it
        if (_dir.exists("config.lua"))
        {
            // read in the new packageName from Lua. Should be expanded in future to whatever else config.lua will have
            readPackageConfig(_dir.absoluteFilePath("config.lua"), packageName);
            // now that the packageName changed, redo relevant checks to make sure it's still valid
            if (module)
            {
                if( mActiveModules.contains( packageName ) )
                {
                    uninstallPackage(packageName, 2);
                }
            }
            else
            {
                if( mInstalledPackages.contains( packageName ) )
                {
                    // cleanup and quit if already installed
                    removeDir( _dir.absolutePath(),_dir.absolutePath() );
                    return false;
                }
            }
            // continuing, so update the folder name on disk
            QString newpath(QString( "%1/%2/").arg( _home ).arg( packageName ));
            _dir.rename(_dir.absolutePath(), newpath);
            _dir = QDir( newpath );
        }
        QStringList _filterList;
        _filterList << "*.xml" << "*.trigger";
        QFileInfoList entries = _dir.entryInfoList( _filterList, QDir::Files );
        for( int i=0; i<entries.size(); i++ )
        {
            file2.setFileName( entries[i].absoluteFilePath() );
            file2.open(QFile::ReadOnly | QFile::Text);
            QString profileName = getName();
            QString login = getLogin();
            QString pass = getPass();
            XMLimport reader( this );
            if (module)
            {
                QStringList moduleEntry;
                moduleEntry << fileName;
                moduleEntry << "0";
                mInstalledModules[packageName] = moduleEntry;//.append( packageName );
                mActiveModules.append(packageName);
            }
            else
                mInstalledPackages.append( packageName );
            reader.importPackage( & file2, packageName, module);
            setName( profileName );
            setLogin( login );
            setPass( pass );
            file2.close();
        }
    }
    else
    {
        file2.setFileName( fileName );
        file2.open(QFile::ReadOnly | QFile::Text);
        //mInstalledPackages.append( packageName );
        QString profileName = getName();
        QString login = getLogin();
        QString pass = getPass();
        XMLimport reader( this );
        if (module)
        {
            QStringList moduleEntry;
            moduleEntry << fileName;
            moduleEntry << "0";
            mInstalledModules[packageName] = moduleEntry;//.append( packageName );
            mActiveModules.append(packageName);
        }
        else
            mInstalledPackages.append( packageName );
        reader.importPackage( & file2, packageName, module);
        setName( profileName );
        setLogin( login );
        setPass( pass );
        file2.close();
    }
    if( mpEditorDialog )
    {
       mpEditorDialog->doCleanReset();
    }
    if (!module)
    {
        QString directory_xml = QDir::homePath()+"/.config/mudlet/profiles/"+getName()+"/current";
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
    }
    // reorder permanent and temporary triggers: perm first, temp second
    mTriggerUnit.reorderTriggersAfterPackageImport();
    return true;
}

// credit: http://john.nachtimwald.com/2010/06/08/qt-remove-directory-and-its-contents/
bool Host::removeDir( const QString dirName, QString originalPath )
{
    bool result = true;
    QDir dir(dirName);
    if( dir.exists( dirName ) )
    {
        Q_FOREACH( QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst))
        {
            // prevent recursion outside of the original branch
            if( info.isDir() && info.absoluteFilePath().startsWith( originalPath ) )
            {
                result = removeDir( info.absoluteFilePath(), originalPath );
            }
            else
            {
                result = QFile::remove( info.absoluteFilePath() );
            }

            if( !result )
            {
                return result;
            }
        }
        result = dir.rmdir( dirName );
    }

    return result;
}

bool Host::uninstallPackage( QString packageName, int module)
{

//     As with the installPackage, the module codes are:
//     0=package, 1=uninstall from dialog, 2=uninstall due to module syncing,
//     3=uninstall from a script

    if (module)
    {
        if( ! mInstalledModules.contains( packageName ) ) return false;
    }
    else
    {
        if( ! mInstalledPackages.contains( packageName ) ) return false;
    }
    int dualInstallations=0;
    if (mInstalledModules.contains(packageName) && mInstalledPackages.contains(packageName))
        dualInstallations=1;
    //we check for the module=3 because if we reset the editor, we will re-execute the
    //module uninstall, thus creating an infinite loop.
    if( mpEditorDialog && module != 3 )
    {
        mpEditorDialog->doCleanReset();
    }
    mTriggerUnit.uninstall( packageName );
    mTimerUnit.uninstall( packageName );
    mAliasUnit.uninstall( packageName );
    mActionUnit.uninstall( packageName );
    mScriptUnit.uninstall( packageName );
    mKeyUnit.uninstall( packageName );
    if (module)
    {
        //if module == 2, this is a temporary uninstall for reloading so we exit here
        QStringList entry = mInstalledModules[packageName];
        mInstalledModules.remove( packageName );
        mActiveModules.removeAll(packageName);
        if ( module == 2 )
            return true;
        //if module == 1/3, we actually uninstall it.
        //reinstall the package if it shared a module name.  This is a kludge, but it's cleaner than adding extra arguments/etc imo
        if (dualInstallations)
        {
            //we're a dual install, reinstalling package
            mInstalledPackages.removeAll(packageName); //so we don't get denied from installPackage
            //get the pre package list so we don't get duplicates
            installPackage(entry[0], 0);
        }
    }
    else
    {
        mInstalledPackages.removeAll( packageName );
        if (dualInstallations)
        {
            QStringList entry = mInstalledModules[packageName];
            installPackage(entry[0], 1);
            //restore the module edit flag
            mInstalledModules[packageName] = entry;
        }
    }
    if( mpEditorDialog && module != 3 )
    {
        mpEditorDialog->doCleanReset();
    }

    getActionUnit()->updateToolbar();

    QString _home = QDir::homePath();
    _home.append( "/.config/mudlet/profiles/" );
    _home.append( getName() );
    QString _dest = QString( "%1/%2/").arg( _home ).arg( packageName );
    removeDir( _dest, _dest );
    QString directory_xml = QDir::homePath()+"/.config/mudlet/profiles/"+getName()+"/current";
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
    //NOW we reset if we're uninstalling a module
    if( mpEditorDialog && module == 3 )
    {
        mpEditorDialog->doCleanReset();
    }
    return true;
}

void Host::readPackageConfig( QString luaConfig, QString & packageName )
{

    QFile configFile(luaConfig);
    QStringList strings;
    if (configFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&configFile);
        while (!in.atEnd())
        {
            strings += in.readLine();
        }
    }

    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    int error = luaL_loadstring(L, strings.join("\n").toLatin1().data());

    if( !error )
        error = lua_pcall(L, 0,0,0);

    if( !error )
    {
        // for now, only read the mpackage parameter
        // would be nice to read author, save & version too later
        lua_getglobal(L, "mpackage");
        if (lua_isstring(L, -1))
        {
            packageName = QString(lua_tostring(L, -1));
        }
        lua_pop(L, -1);
        lua_close(L);
        return;
    }
    else // error
    {
        string e = "no error message available from Lua";
        e = lua_tostring( L, -1 );
        string reason;
        switch (error)
        {
            case 4:
                reason = "Out of memory"; break;
            case 3:
                reason = "Syntax error"; break;
            case 2:
                reason = "Runtime error"; break;
            case 1:
                reason = "Yield error"; break;
            default:
                reason = "Unknown error"; break;
        }

        if( mudlet::debugMode ) qDebug()<< reason.c_str() <<" in config.lua:"<<e.c_str();
        // should print error to main display
        QString msg = QString ("%1 in config.lua: %2\n").arg( reason.c_str() ).arg( e.c_str() );
        mpConsole->printSystemMessage(msg);


        lua_pop(L, -1);
        lua_close(L);
    }
}

#endif


