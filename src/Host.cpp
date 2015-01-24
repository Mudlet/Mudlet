/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
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


#include "Host.h"


#include "mudlet.h"
#include "TConsole.h"
#include "TEvent.h"
#include "TMap.h"
#include "TRoomDB.h"
#include "XMLexport.h"
#include "XMLimport.h"

#include "pre_guard.h"
#include <QtUiTools>
#include <QDir>
#include <QMessageBox>
#include "post_guard.h"

#include "zip.h"
#include "zipconf.h"

#include <errno.h>


Host::Host( int port, const QString& hostname, const QString& login, const QString& pass, int id )
: mTelnet( this )
, mpConsole( 0 )
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
, mEnableMSDP( false )
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
, mBlack             (Qt::black)
, mLightBlack        (Qt::darkGray)
, mRed               (Qt::darkRed)
, mLightRed          (Qt::red)
, mLightGreen        (Qt::green)
, mGreen             (Qt::darkGreen)
, mLightBlue         (Qt::blue)
, mBlue              (Qt::darkBlue)
, mLightYellow       (Qt::yellow)
, mYellow            (Qt::darkYellow)
, mLightCyan         (Qt::cyan)
, mCyan              (Qt::darkCyan)
, mLightMagenta      (Qt::magenta)
, mMagenta           (Qt::darkMagenta)
, mLightWhite        (Qt::white)
, mWhite             (Qt::lightGray)
, mFgColor           (Qt::lightGray)
, mBgColor           (Qt::black)
, mCommandBgColor    (Qt::black)
, mCommandFgColor    (QColor(113, 113, 0))
, mBlack_2             (Qt::black)
, mLightBlack_2        (Qt::darkGray)
, mRed_2               (Qt::darkRed)
, mLightRed_2          (Qt::red)
, mLightGreen_2        (Qt::green)
, mGreen_2             (Qt::darkGreen)
, mLightBlue_2         (Qt::blue)
, mBlue_2              (Qt::darkBlue)
, mLightYellow_2       (Qt::yellow)
, mYellow_2            (Qt::darkYellow)
, mLightCyan_2         (Qt::cyan)
, mCyan_2              (Qt::darkCyan)
, mLightMagenta_2      (Qt::magenta)
, mMagenta_2           (Qt::darkMagenta)
, mLightWhite_2        (Qt::white)
, mWhite_2             (Qt::lightGray)
, mFgColor_2           (Qt::lightGray)
, mBgColor_2           (Qt::black)
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
, mCommandLineFgColor(Qt::darkGray)
, mCommandLineBgColor(Qt::black)
, mFORCE_MXP_NEGOTIATION_OFF( false )
, mHaveMapperScript( false )
, mLogErrorOnConsole( false )
, java(this,hostname)
{
   // mLogStatus = mudlet::self()->mAutolog;
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
    mIsGoingDown = true;
    mTelnet.disconnect();
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

void Host::reloadModule(const QString& moduleName)
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

    mpConsole->resetMainConsole();
    mEventHandlerMap.clear();
    mEventMap.clear();
    mBlockScriptCompile = false;

    getKeyUnit()->compileAll();
    mResetProfile = false;

    TEvent event;
    event.mArgumentList.append( "sysLoadEvent" );
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    raiseEvent( event );
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
}

void Host::adjustNAWS()
{
    mTelnet.setDisplayDimensions();
}

void Host::setReplacementCommand(const QString& s )
{
    mReplacementCommand = s;
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

    for( int i=0; i<commandList.size(); i++ )
    {
        if( commandList[i].size() < 1 ) continue;
        QString command = commandList[i];
        command.replace("\n", "");
        mReplacementCommand = "";
        mTelnet.sendData( command );
    }
}

void Host::sendRaw( QString command )
{
    mTelnet.sendData( command );
}

void Host::incomingStreamProcessor(const QString & data, int line )
{
    java.handleLine(data,line);

    if( mResetProfile )
    {
        resetProfile();
    }
}

void Host::registerEventHandler(const QString& name, TScript * pScript )
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
void Host::registerAnonymousEventHandler(const QString& name, const QString& fun )
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

void Host::unregisterEventHandler(const QString & name, TScript * pScript )
{
    if( mEventHandlerMap.contains( name ) )
    {
        mEventHandlerMap[name].removeAll( pScript );
    }
}

void Host::raiseEvent( const TEvent & pE )
{

}

void Host::enableKey(const QString & name )
{
    mKeyUnit.enableKey( name );
}

void Host::disableKey(const QString & name )
{
    mKeyUnit.disableKey( name );
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


// credit: http://john.nachtimwald.com/2010/06/08/qt-remove-directory-and-its-contents/
bool Host::removeDir( const QString& dirName, const QString& originalPath )
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
