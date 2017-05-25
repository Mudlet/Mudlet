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


#include "Host.h"


#include "LuaInterface.h"
#include "TConsole.h"
#include "TEvent.h"
#include "TMap.h"
#include "TRoomDB.h"
#include "TScript.h"
#include "XMLexport.h"
#include "XMLimport.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"

#include "pre_guard.h"
#include <QtUiTools>
#include <QApplication>
#include <QDir>
#include <QMessageBox>
#include <QStringBuilder>
#include "post_guard.h"

#include <zip.h>

#include <errno.h>


Host::Host(int port, const QString& hostname, const QString& login, const QString& pass, int id)
: mTelnet(this)
, mpConsole(0)
, mLuaInterpreter(this, id)
, mTriggerUnit(this)
, mTimerUnit(this)
, mScriptUnit(this)
, mAliasUnit(this)
, mActionUnit(this)
, mKeyUnit(this)
, commandLineMinimumHeight(30)
, mAlertOnNewData(true)
, mAllowToSendCommand(true)
, mAutoClearCommandLineAfterSend(false)
, mBlockScriptCompile(true)
, mEchoLuaErrors(false)
, mBorderBottomHeight(0)
, mBorderLeftWidth(0)
, mBorderRightWidth(0)
, mBorderTopHeight(0)
, mCodeCompletion(true)
, mCommandLineFont   ( QFont("Bitstream Vera Sans Mono", 10, QFont::Normal ) )//( QFont("Monospace", 10, QFont::Courier) )
, mCommandSeparator(QString(";"))
, mDisableAutoCompletion(false)
, mDisplayFont       ( QFont("Bitstream Vera Sans Mono", 10, QFont::Normal ) )//, mDisplayFont       ( QFont("Bitstream Vera Sans Mono", 10, QFont:://( QFont("Monospace", 10, QFont::Courier) ), mPort              ( port )
, mEnableGMCP(true)
, mEnableMSDP(false)
, mFORCE_GA_OFF(false)
, mFORCE_NO_COMPRESSION(false)
, mFORCE_SAVE_ON_EXIT(false)
, mHostID(id)
, mHostName(hostname)
, mInsertedMissingLF(false)
, mIsGoingDown(false)
, mLF_ON_GA(true)
, mLogin(login)
, mMainIconSize(3)
, mNoAntiAlias(false)
, mPass(pass)
, mpEditorDialog(0)
, mpMap(new TMap(this))
, mpNotePad(0)
, mPort(port)
, mPrintCommand(true)
, mIsCurrentLogFileInHtmlFormat(false)
, mIsLoggingTimestamps(false)
, mResetProfile(false)
, mRetries(5)
, mSaveProfileOnExit(false)
, mScreenHeight(25)
, mScreenWidth(90)
, mTEFolderIconSize(3)
, mTimeout(60)
, mUSE_FORCE_LF_AFTER_PROMPT(false)
, mUSE_IRE_DRIVER_BUGFIX(true)
, mUSE_UNIX_EOL(false)
, mWrapAt(100)
, mWrapIndentCount(0)
, mBlack(Qt::black)
, mLightBlack(Qt::darkGray)
, mRed(Qt::darkRed)
, mLightRed(Qt::red)
, mLightGreen(Qt::green)
, mGreen(Qt::darkGreen)
, mLightBlue(Qt::blue)
, mBlue(Qt::darkBlue)
, mLightYellow(Qt::yellow)
, mYellow(Qt::darkYellow)
, mLightCyan(Qt::cyan)
, mCyan(Qt::darkCyan)
, mLightMagenta(Qt::magenta)
, mMagenta(Qt::darkMagenta)
, mLightWhite(Qt::white)
, mWhite(Qt::lightGray)
, mFgColor(Qt::lightGray)
, mBgColor(Qt::black)
, mCommandBgColor(Qt::black)
, mCommandFgColor(QColor(113, 113, 0))
, mBlack_2(Qt::black)
, mLightBlack_2(Qt::darkGray)
, mRed_2(Qt::darkRed)
, mLightRed_2(Qt::red)
, mLightGreen_2(Qt::green)
, mGreen_2(Qt::darkGreen)
, mLightBlue_2(Qt::blue)
, mBlue_2(Qt::darkBlue)
, mLightYellow_2(Qt::yellow)
, mYellow_2(Qt::darkYellow)
, mLightCyan_2(Qt::cyan)
, mCyan_2(Qt::darkCyan)
, mLightMagenta_2(Qt::magenta)
, mMagenta_2(Qt::darkMagenta)
, mLightWhite_2(Qt::white)
, mWhite_2(Qt::lightGray)
, mFgColor_2(Qt::lightGray)
, mBgColor_2(Qt::black)
, mSpellDic("en_US")
, mLogStatus(false)
, mEnableSpellCheck(true)
, mModuleSaveBlock(false)
, mLineSize(10.0)
, mRoomSize(0.5)
, mBubbleMode(false)
, mShowRoomID(false)
, mMapperUseAntiAlias(true)
, mServerGUI_Package_version(-1)
, mServerGUI_Package_name("nothing")
, mAcceptServerGUI(true)
, mCommandLineFgColor(Qt::darkGray)
, mCommandLineBgColor(Qt::black)
, mFORCE_MXP_NEGOTIATION_OFF(false)
, mpDockableMapWidget()
, mHaveMapperScript(false)
{
   // mLogStatus = mudlet::self()->mAutolog;
    mLuaInterface.reset(new LuaInterface(this));
    QString directoryLogFile = QDir::homePath() + "/.config/mudlet/profiles/";
    directoryLogFile.append(mHostName);
    directoryLogFile.append("/log");
    QString logFileName = directoryLogFile + "/errors.txt";
    QDir dirLogFile;
    if (!dirLogFile.exists(directoryLogFile)) {
        dirLogFile.mkpath(directoryLogFile);
    }
    mErrorLogFile.setFileName(logFileName);
    mErrorLogFile.open(QIODevice::Append);
    // This is NOW used (for map
    // file auditing and other issues)
    mErrorLogStream.setDevice(&mErrorLogFile);

    // There was a map load attempt made here but it did not seem to be needed
    // and it caused issues being doing in the constructor (some other classes
    // were not fully initialised at this point) so it seemed sensible to remove
    // it - Slysven

    mMapStrongHighlight = false;
    mGMCP_merge_table_keys.append("Char.Status");
    mDoubleClickIgnore.insert('"');
    mDoubleClickIgnore.insert('\'');
}

Host::~Host()
{
    if (mpDockableMapWidget) {
        mpDockableMapWidget->deleteLater();
    }
    mIsGoingDown = true;
    mIsClosingDown = true;
    mTelnet.disconnect();
    mErrorLogStream.flush();
    mErrorLogFile.close();
}

void Host::saveModules(int sync)
{
    if (mModuleSaveBlock) {
        //FIXME: This should generate an error to the user
        return;
    }
    QMapIterator<QString, QStringList> it(modulesToWrite);
    QStringList modulesToSync;
    QString dirName = QDir::homePath() + "/.config/mudlet/moduleBackups/";
    QDir savePath = QDir(dirName);
    if (!savePath.exists())
        savePath.mkpath(dirName);
    while (it.hasNext()) {
        it.next();
        QStringList entry = it.value();
        QString filename_xml = entry[0];
        QString time = QDateTime::currentDateTime().toString("dd-MM-yyyy#hh-mm-ss");
        QString moduleName = it.key();
        QString tempDir;
        QString zipName;
        zip* zipFile = 0;
        // Filename extension tests should be case insensitive to work on MacOS Platforms...! - Slysven
        if(  filename_xml.endsWith( QStringLiteral( "mpackage" ), Qt::CaseInsensitive )
          || filename_xml.endsWith( QStringLiteral( "zip" ), Qt::CaseInsensitive ) )
        {
            tempDir = QDir::homePath() + "/.config/mudlet/profiles/" + mHostName + "/" + moduleName;
            filename_xml = tempDir + "/" + moduleName + ".xml";
            int err;
            zipFile = zip_open(entry[0].toStdString().c_str(), 0, &err);
            zipName = filename_xml;
            QDir packageDir = QDir(tempDir);
            if (!packageDir.exists()) {
                packageDir.mkpath(tempDir);
            }
        } else {
            savePath.rename(filename_xml, dirName + moduleName + time); //move the old file, use the key (module name) as the file
        }
        QFile file_xml(filename_xml);
        if (file_xml.open(QIODevice::WriteOnly)) {
            XMLexport writer(this);
            writer.writeModuleXML(&file_xml, it.key());
            file_xml.close();

            if (entry[1].toInt())
                modulesToSync << it.key();
        } else {
            file_xml.close();
            //FIXME: Should have an error reported to user
            //qDebug()<<"failed to write xml for module:"<<entry[0]<<", check permissions?";
            mModuleSaveBlock = true;
            return;
        }
        if (!zipName.isEmpty()) {
            struct zip_source* s = zip_source_file(zipFile, filename_xml.toStdString().c_str(), 0, 0);
            QTime t;
            t.start();
//            int err = zip_file_add( zipFile, QString(moduleName+".xml").toStdString().c_str(), s, ZIP_FL_OVERWRITE );
            int err = zip_add(zipFile, QString(moduleName + ".xml").toStdString().c_str(), s);
            //FIXME: error checking
            if (zipFile) {
                err = zip_close(zipFile);
            }
            //FIXME: error checking
        }
    }
    modulesToWrite.clear();
    if (sync) {
        //synchronize modules across sessions
        QMap<Host*, TConsole*> activeSessions = mudlet::self()->mConsoleMap;
        QMapIterator<Host*, TConsole*> it2(activeSessions);
        while (it2.hasNext()) {
            it2.next();
            Host* host = it2.key();
            if (host->mHostName == mHostName)
                continue;
            QMap<QString, QStringList> installedModules = host->mInstalledModules;
            QMap<QString, int> modulePri = host->mModulePriorities;
            QMapIterator<QString, int> it3(modulePri);
            QMap<int, QStringList> moduleOrder;
            while (it3.hasNext()) {
                it3.next();
                //QStringList moduleEntry = moduleOrder[it3.value()];
                //moduleEntry.append(it3.key());
                moduleOrder[it3.value()].append(it3.key());// = moduleEntry;
            }
            QMapIterator<int, QStringList> it4(moduleOrder);
            while (it4.hasNext()) {
                it4.next();
                QStringList moduleList = it4.value();
                for (int i = 0; i < moduleList.size(); i++) {
                    QString moduleName = moduleList[i];
                    if (modulesToSync.contains(moduleName)) {
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
    while (it.hasNext()) {
        it.next();
        QStringList entry = it.value();
        if (it.key() == moduleName) {
            uninstallPackage(it.key(), 2);
            installPackage(entry[0], 2);
        }
    }
    //iterate through mInstalledModules again and reset the entry flag to be correct.
    //both the installedModules and mInstalled should be in the same order now as well
    QMapIterator<QString, QStringList> it2(mInstalledModules);
    while (it2.hasNext()) {
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
    event.mArgumentList.append(QLatin1String("sysLoadEvent"));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    raiseEvent(event);
    qDebug() << "resetProfile() DONE";
}

// Saves profile to disk - does not save items dirty in the editor, however.
// takes a directory to save in or an empty string for the default location
// as well as a boolean whenever to sync the modules or not
// returns true+filepath if successful or false+error message otherwise
std::tuple<bool, QString, QString> Host::saveProfile(const QString& saveLocation, bool syncModules)
{
    QString directory_xml;
    if (saveLocation.isEmpty()) {
        directory_xml = QStringLiteral("%1/.config/mudlet/profiles/%2/current").arg(QDir::homePath(), getName());
    } else {
        directory_xml = saveLocation;
    }

    QString filename_xml = QStringLiteral("%1/%2.xml").arg(directory_xml, QDateTime::currentDateTime().toString("dd-MM-yyyy#hh-mm-ss"));
    QDir dir_xml;
    if (!dir_xml.exists(directory_xml)) {
        dir_xml.mkpath(directory_xml);
    }
    QFile file_xml(filename_xml);
    if (file_xml.open(QIODevice::WriteOnly)) {
        XMLexport writer(this);
        writer.exportHost(&file_xml);
        file_xml.close();
        saveModules(syncModules ? 1 : 0);
        return std::make_tuple(true, filename_xml, QString());
    } else {
        return std::make_tuple(false, filename_xml, file_xml.errorString());
    }
}

// Now returns the total weight of the path
const unsigned int Host::assemblePath()
{
    unsigned int totalWeight = 0;
    QStringList pathList;
    for (int i : mpMap->mPathList) {
        QString n = QString::number(i);
        pathList.append(n);
    }
    QStringList directionList = mpMap->mDirList;
    QStringList weightList;
    for (int stepWeight : mpMap->mWeightList) {
        totalWeight += stepWeight;
        QString n = QString::number(stepWeight);
        weightList.append(n);
    }
    QString tableName = QStringLiteral("speedWalkPath");
    mLuaInterpreter.set_lua_table(tableName, pathList);
    tableName = QStringLiteral("speedWalkDir");
    mLuaInterpreter.set_lua_table(tableName, directionList);
    tableName = QStringLiteral("speedWalkWeight");
    mLuaInterpreter.set_lua_table(tableName, weightList);
    return totalWeight;
}

const bool Host::checkForMappingScript()
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
    int totalWeight = assemblePath();
    Q_UNUSED(totalWeight);
    QString f = QStringLiteral("doSpeedWalk");
    QString n = QString();
    mLuaInterpreter.call(f, n);
}

void Host::adjustNAWS()
{
    mTelnet.setDisplayDimensions();
}

void Host::setReplacementCommand(const QString& s)
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

void Host::send(QString cmd, bool wantPrint, bool dontExpandAliases)
{
    if (wantPrint && mPrintCommand) {
        mInsertedMissingLF = true;
        if ((cmd == "") && (mUSE_IRE_DRIVER_BUGFIX) && (!mUSE_FORCE_LF_AFTER_PROMPT)) {
            ;
        } else {
            // used to print the terminal <LF> that terminates a telnet command
            // this is important to get the cursor position right
            mpConsole->printCommand(cmd);
        }
        mpConsole->update();
    }
    QStringList commandList = cmd.split(QString(mCommandSeparator), QString::SkipEmptyParts);
    if (!dontExpandAliases) {
        if (commandList.size() == 0) {
            sendRaw("\n"); //NOTE: damit leerprompt moeglich sind
            return;
        }
    }
    for (int i = 0; i < commandList.size(); i++) {
        if (commandList[i].size() < 1) {
            continue;
        }
        QString command = commandList[i];
        command.remove(QChar::LineFeed);
        mReplacementCommand = "";
        if (dontExpandAliases) {
            mTelnet.sendData(command);
            continue;
        }
        if (!mAliasUnit.processDataStream(command)) {
            if (mReplacementCommand.size() > 0) {
                mTelnet.sendData(mReplacementCommand);
            } else {
                mTelnet.sendData(command);
            }
        }
    }
}

void Host::sendRaw(QString command)
{
    mTelnet.sendData(command);
}

int Host::createStopWatch()
{
    int newWatchID = mStopWatchMap.size() + 1;
    mStopWatchMap[newWatchID] = QTime(0, 0, 0, 0);
    return newWatchID;
}

double Host::getStopWatchTime(int watchID)
{
    if (mStopWatchMap.contains(watchID)) {
        return static_cast<double>(mStopWatchMap[watchID].elapsed()) / 1000;
    } else {
        return -1.0;
    }
}

bool Host::startStopWatch(int watchID)
{
    if (mStopWatchMap.contains(watchID)) {
        mStopWatchMap[watchID].start();
        return true;
    } else
        return false;
}

double Host::stopStopWatch(int watchID)
{
    if (mStopWatchMap.contains(watchID)) {
        return static_cast<double>(mStopWatchMap[watchID].elapsed()) / 1000;
    } else {
        return -1.0;
    }
}

bool Host::resetStopWatch(int watchID)
{
    if (mStopWatchMap.contains(watchID)) {
        mStopWatchMap[watchID].setHMS(0, 0, 0, 0);
        return true;
    } else
        return false;
}

void Host::callEventHandlers()
{
}

void Host::incomingStreamProcessor(const QString& data, int line)
{
    mTriggerUnit.processDataStream(data, line);

    mTimerUnit.doCleanup();
    if (mResetProfile) {
        resetProfile();
    }
}

void Host::registerEventHandler(const QString& name, TScript* pScript)
{
    if (mEventHandlerMap.contains(name)) {
        if (!mEventHandlerMap[name].contains(pScript)) {
            mEventHandlerMap[name].append(pScript);
        }
    } else {
        QList<TScript*> scriptList;
        scriptList.append(pScript);
        mEventHandlerMap.insert(name, scriptList);
    }
}
void Host::registerAnonymousEventHandler(const QString& name, const QString& fun)
{
    if (mAnonymousEventHandlerFunctions.contains(name)) {
        if (!mAnonymousEventHandlerFunctions[name].contains(fun)) {
            mAnonymousEventHandlerFunctions[name].push_back(fun);
        }
    } else {
        QStringList newList;
        newList << fun;
        mAnonymousEventHandlerFunctions[name] = newList;
    }
}

void Host::unregisterEventHandler(const QString& name, TScript* pScript)
{
    if (mEventHandlerMap.contains(name)) {
        mEventHandlerMap[name].removeAll(pScript);
    }
}

void Host::raiseEvent(const TEvent& pE)
{
    if (pE.mArgumentList.isEmpty()) {
        return;
    }

    if (mEventHandlerMap.contains(pE.mArgumentList.at(0))) {
        QList<TScript*> scriptList = mEventHandlerMap.value(pE.mArgumentList.at(0));
        for (auto& script : scriptList) {
            script->callEventHandler(pE);
        }
    }

    if (mAnonymousEventHandlerFunctions.contains(pE.mArgumentList.at(0))) {
        QStringList functionsList = mAnonymousEventHandlerFunctions.value(pE.mArgumentList.at(0));
        for (int i = 0, total = functionsList.size(); i < total; ++i) {
            mLuaInterpreter.callEventHandler(functionsList.at(i), pE);
        }
    }
}

void Host::postIrcMessage(const QString& a, const QString& b, const QString& c)
{
    TEvent event;
    event.mArgumentList << QLatin1String("sysIrcMessage");
    event.mArgumentList << a << b << c;
    event.mArgumentTypeList << ARGUMENT_TYPE_STRING << ARGUMENT_TYPE_STRING << ARGUMENT_TYPE_STRING << ARGUMENT_TYPE_STRING;
    raiseEvent(event);
}

void Host::enableTimer(const QString& name)
{
    mTimerUnit.enableTimer(name);
}

void Host::disableTimer(const QString& name)
{
    mTimerUnit.disableTimer(name);
}

bool Host::killTimer(const QString& name)
{
    return mTimerUnit.killTimer(name);
}

void Host::enableKey(const QString& name)
{
    mKeyUnit.enableKey(name);
}

void Host::disableKey(const QString& name)
{
    mKeyUnit.disableKey(name);
}


void Host::enableTrigger(const QString& name)
{
    mTriggerUnit.enableTrigger(name);
}

void Host::disableTrigger(const QString& name)
{
    mTriggerUnit.disableTrigger(name);
}

bool Host::killTrigger(const QString& name)
{
    return mTriggerUnit.killTrigger(name);
}


void Host::connectToServer()
{
    mTelnet.connectIt(mUrl, mPort);
}

void Host::closingDown()
{
    QMutexLocker locker(&mLock);
    mIsClosingDown = true;
}

bool Host::isClosingDown()
{
    QMutexLocker locker(&mLock);
    return mIsClosingDown;
}

bool Host::installPackage(const QString& fileName, int module)
{
    // As the pointed to dialog is only used now WITHIN this method and this
    // method can be re-entered, it is best to use a local rather than a class
    // pointer just in case we accidently reenter this method in the future.
    QDialog* pUnzipDialog = Q_NULLPTR;

//     Module notes:
//     For the module install, a module flag of 0 is a package, a flag
//     of 1 means the module is being installed for the first time via
//     the UI, a flag of 2 means the module is being synced (so it's "installed"
//     already), a flag of 3 means the module is being installed from
//     a script.  This separation is necessary to be able to reuse code
//     while avoiding infinite loops from script installations.

    if (fileName.isEmpty()) {
        return false;
    }

    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        return false;
    }

    QString packageName = fileName.section(QStringLiteral("/"), -1);
    packageName.remove(QStringLiteral(".trigger"), Qt::CaseInsensitive);
    packageName.remove(QStringLiteral(".xml"), Qt::CaseInsensitive);
    packageName.remove(QStringLiteral(".zip"), Qt::CaseInsensitive);
    packageName.remove(QStringLiteral(".mpackage"), Qt::CaseInsensitive);
    packageName.remove(QLatin1Char('\\'));
    packageName.remove(QLatin1Char('.'));
    if (module) {
        if ((module == 2) && (mActiveModules.contains(packageName))) {
            uninstallPackage(packageName, 2);
        } else if ((module == 3) && (mActiveModules.contains(packageName))) {
            return false; //we're already installed
        }
    } else {
        if (mInstalledPackages.contains(packageName)) {
            return false;
        }
    }
    //the extra module check is needed here to prevent infinite loops from script loaded modules
    if (mpEditorDialog && module != 3) {
        mpEditorDialog->doCleanReset();
    }
    QFile file2;
    if(  fileName.endsWith( QStringLiteral( ".zip" ), Qt::CaseInsensitive )
      || fileName.endsWith( QStringLiteral( ".mpackage"), Qt::CaseInsensitive ) )
    {
        QString _home = QStringLiteral( "%1/.config/mudlet/profiles/%2" )
                        .arg( QDir::homePath(), getName() );
        QString _dest = QStringLiteral( "%1/%2/" )
                        .arg( _home, packageName );
        QDir _tmpDir( _home ); // home directory for the PROFILE
        _tmpDir.mkpath( _dest );

        // TODO: report failure to create destination folder for package/module in profile

        QUiLoader loader(this);
        QFile uiFile(QStringLiteral(":/ui/package_manager_unpack.ui"));
        uiFile.open(QFile::ReadOnly);
        pUnzipDialog = dynamic_cast<QDialog*>(loader.load(&uiFile, 0));
        uiFile.close();
        if (!pUnzipDialog) {
            return false;
        }

        QLabel* pLabel = pUnzipDialog->findChild<QLabel*>(QStringLiteral("label"));
        if (pLabel) {
            if (module) {
                pLabel->setText(tr("Unpacking module:\n\"%1\"\nplease wait...").arg(packageName));
            } else {
                pLabel->setText(tr("Unpacking package:\n\"%1\"\nplease wait...").arg(packageName));
            }
        }
        pUnzipDialog->hide(); // Must hide to change WindowModality
        pUnzipDialog->setWindowTitle(tr("Unpacking"));
        pUnzipDialog->setWindowModality(Qt::ApplicationModal);
        pUnzipDialog->show();
        qApp->processEvents();
        pUnzipDialog->raise();
        pUnzipDialog->repaint(); // Force a redraw
        qApp->processEvents();   // Try to ensure we are on top of any other dialogs and freshly drawn

        int err = 0;
        //from: https://gist.github.com/mobius/1759816
        struct zip_stat zs;
        struct zip_file* zf;
        zip_uint64_t bytesRead = 0;
        char buf[4096]; // Was 100 but that seems unduely stingy...!
        zip* archive = zip_open(fileName.toStdString().c_str(), 0, &err);
        if (err != 0) {
            zip_error_to_str(buf, sizeof(buf), err, errno);
            //FIXME: Tell user error
            if (pUnzipDialog) {
                pUnzipDialog->deleteLater();
                pUnzipDialog = Q_NULLPTR;
            }
            return false;
        }

        // We now scan for directories first, and gather needed ones first, not
        // just relying on (zero length) archive entries ending in '/' as some
        // (possibly broken) archive building libraries seem to forget to
        // include them.
        QMap<QString, QString> directoriesNeededMap;
        //   Key is: relative path stored in archive
        // Value is: absolute path needed when extracting files
        for (zip_int64_t i = 0, total = zip_get_num_entries(archive, 0); i < total; ++i) {
            if (!zip_stat_index(archive, static_cast<zip_uint64_t>(i), 0, &zs)) {
                QString entryInArchive(QString::fromUtf8(zs.name));
                QString pathInArchive(entryInArchive.section(QLatin1Literal("/"), 0, -2));
                // TODO: We are supposed to validate the fields (except the
                // "valid" one itself) in zs before using them:
                // i.e. check that zs.name is valid ( zs.valid & ZIP_STAT_NAME )
                if (entryInArchive.endsWith(QLatin1Char('/'))) {
//                    qDebug() << "Host::installPackage() Scanning archive (for directories) found item:" << i << "called:" << entryInArchive << "this is a DIRECTORY...!";
                    if (!directoriesNeededMap.contains(pathInArchive)) {
                        QString pathInProfile(QStringLiteral("%1/%2").arg(packageName, pathInArchive));
                        directoriesNeededMap.insert(pathInArchive, pathInProfile);
//                        qDebug() << "Added:" << pathInArchive << "to list of sub-directories to be made.";
                    }
//                    else
//                    {
//                        qDebug() << "No need to add:" << pathInArchive << "we have already spotted the need for it!";
//                    }
                } else {
//                    qDebug() << "Host::installPackage() Scanning archive (for directories) found item:" << i << "called:" << entryInArchive << "this is a FILE...!";
                    // Extract needed path from name for archives that do NOT
                    // explicitly list directories
                    if (!pathInArchive.isEmpty() && !directoriesNeededMap.contains(pathInArchive)) {
                        QString pathInProfile(QStringLiteral("%1/%2").arg(packageName, pathInArchive));
                        directoriesNeededMap.insert(pathInArchive, pathInProfile);
//                        qDebug() << "Added:" << pathInArchive << "to list of sub-directories to be made.";
                    }
//                    else
//                    {
//                        qDebug() << "No need to add:" << pathInArchive << "we have already spotted the need for it!";
//                    }
                }
            } else {
                // TODO: Report failure to obtain an archive entry to parse
            }
        }

        // Now create the needed directories:
        QMapIterator<QString, QString> itPath(directoriesNeededMap);
        while (itPath.hasNext()) {
            itPath.next();
//            qDebug() << "Host::installPackage(...)    INFO testing for presence of:"
//                     << itPath.value()
//                     << "relative to:"
//                     << _home;
            if (!_tmpDir.exists(itPath.value())) {
                if (!_tmpDir.mkpath(itPath.value())) {
                    // TODO: report failure to create needed sub-directory
                    // within package destination directory in profile directory

                    zip_close(archive);
                    if (pUnzipDialog) {
                        pUnzipDialog->deleteLater();
                        pUnzipDialog = Q_NULLPTR;
                        // Previously we forgot to close the dialog if we aborted
                    }
                    return false; // Abort reading rest of archive
                }
                _tmpDir.refresh();
            }
        }

        // Now extract the files
        for (zip_int64_t i = 0, total = zip_get_num_entries(archive, 0); i < total; ++i) {
            // No need to check return value as we've already done it first time
            zip_stat_index(archive, static_cast<zip_uint64_t>(i), 0, &zs);
            QString entryInArchive(QString::fromUtf8(zs.name));
            if (!entryInArchive.endsWith(QLatin1Char('/'))) {
                // TODO: check that zs.size is valid ( zs.valid & ZIP_STAT_SIZE )
                zf = zip_fopen_index(archive, static_cast<zip_uint64_t>(i), 0);
                if (!zf) {
                    int sep = 0;
                    zip_error_get(archive, &err, &sep);
                    zip_error_to_str(buf, sizeof(buf), err, errno);
                    // FIXME: report error to user, zip_error_to_str(...) is
                    // already deprecated, if not obsoleted...! - Slysven
                    zip_close(archive);
                    if (pUnzipDialog) {
                        pUnzipDialog->deleteLater();
                        pUnzipDialog = Q_NULLPTR;
                    }
                    return false;
                }

                QFile fd(QStringLiteral("%1%2").arg(_dest, entryInArchive));

                if (!fd.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
                    //FIXME: report error to user
                    qDebug() << "Host::installPackage("
                             << fileName
                             << ","
                             << module
                             << ")\n    ERROR opening:"
                             << QStringLiteral( "%1%2" ).arg(_dest, entryInArchive)
                             << "!\n    Reported error was:"
                             << fd.errorString();
                    zip_fclose(zf);
                    zip_close(archive);
                    if (pUnzipDialog) {
                        pUnzipDialog->deleteLater();
                        pUnzipDialog = Q_NULLPTR;
                    }
                    return false;
                }

                bytesRead = 0;
                zip_uint64_t bytesExpected = zs.size;
                while (bytesRead < bytesExpected && fd.error() == QFileDevice::NoError) {
                    zip_int64_t len = zip_fread(zf, buf, sizeof(buf));
                    if (len < 0) {
                        //FIXME: report error to user qDebug()<<"zip_fread error"<<len;
                        fd.close();
                        zip_fclose(zf);
                        zip_close(archive);
                        if (pUnzipDialog) {
                            pUnzipDialog->deleteLater();
                            pUnzipDialog = Q_NULLPTR;
                        }
                        return false;
                    }

                    if (fd.write(buf, len) == -1) {
                        // TODO: Report failure to write data to actual file
                        fd.close();
                        zip_fclose(zf);
                        zip_close(archive);
                        if (pUnzipDialog) {
                            pUnzipDialog->deleteLater();
                            pUnzipDialog = Q_NULLPTR;
                        }
                        return false;
                    }
                    bytesRead += static_cast<zip_uint64_t>(len);
                }
                fd.close();
                zip_fclose(zf);
            }
        }

        err = zip_close(archive);
        if (err) {
            zip_error_to_str(buf, sizeof(buf), err, errno);
            //FIXME: report error to user qDebug()<<"close file error"<<buf;
            if (pUnzipDialog) {
                pUnzipDialog->deleteLater();
                pUnzipDialog = Q_NULLPTR;
            }
            return false;
        }

        if (pUnzipDialog) {
            pUnzipDialog->deleteLater();
            pUnzipDialog = Q_NULLPTR;
        }

        // requirements for zip packages:
        // - packages must be compressed in zip format
        // - file extension should be .mpackage (though .zip is accepted)
        // - there can only be a single xml file per package
        // - the xml file must be located in the root directory of the zip package. example: myPack.zip contains: the folder images and the file myPack.xml

        QDir _dir(_dest);
        // before we start importing xmls in, see if the config.lua manifest file exists
        // - if it does, update the packageName from it
        if (_dir.exists(QStringLiteral("config.lua"))) {
            // read in the new packageName from Lua. Should be expanded in future to whatever else config.lua will have
            readPackageConfig(_dir.absoluteFilePath(QStringLiteral("config.lua")), packageName);
            // now that the packageName changed, redo relevant checks to make sure it's still valid
            if (module) {
                if (mActiveModules.contains(packageName)) {
                    uninstallPackage(packageName, 2);
                }
            } else {
                if (mInstalledPackages.contains(packageName)) {
                    // cleanup and quit if already installed
                    removeDir(_dir.absolutePath(), _dir.absolutePath());
                    return false;
                }
            }
            // continuing, so update the folder name on disk
            QString newpath(QStringLiteral("%1/%2/").arg(_home, packageName));
            _dir.rename(_dir.absolutePath(), newpath);
            _dir = QDir(newpath);
        }
        QStringList _filterList;
        _filterList << QStringLiteral("*.xml") << QStringLiteral("*.trigger");
        QFileInfoList entries = _dir.entryInfoList(_filterList, QDir::Files);
        for (auto& entry : entries) {
            file2.setFileName(entry.absoluteFilePath());
            file2.open(QFile::ReadOnly | QFile::Text);
            QString profileName = getName();
            QString login = getLogin();
            QString pass = getPass();
            XMLimport reader(this);
            if (module) {
                QStringList moduleEntry;
                moduleEntry << fileName;
                moduleEntry << QStringLiteral("0");
                mInstalledModules[packageName] = moduleEntry;
                mActiveModules.append(packageName);
            } else {
                mInstalledPackages.append(packageName);
            }
            reader.importPackage(&file2, packageName, module); // TODO: Missing false return value handler
            setName(profileName);
            setLogin(login);
            setPass(pass);
            file2.close();
        }
    } else {
        file2.setFileName(fileName);
        file2.open(QFile::ReadOnly | QFile::Text);
        //mInstalledPackages.append( packageName );
        QString profileName = getName();
        QString login = getLogin();
        QString pass = getPass();
        XMLimport reader(this);
        if (module) {
            QStringList moduleEntry;
            moduleEntry << fileName;
            moduleEntry << QStringLiteral("0");
            mInstalledModules[packageName] = moduleEntry;
            mActiveModules.append(packageName);
        } else {
            mInstalledPackages.append(packageName);
        }
        reader.importPackage(&file2, packageName, module); // TODO: Missing false return value handler
        setName(profileName);
        setLogin(login);
        setPass(pass);
        file2.close();
    }
    if (mpEditorDialog) {
        mpEditorDialog->doCleanReset();
    }
    if (!module) {
        saveProfile();
    }
    // reorder permanent and temporary triggers: perm first, temp second
    mTriggerUnit.reorderTriggersAfterPackageImport();

    // raise 2 events - a generic one and a more detailed one to serve both
    // a simple need ("I just want the install event") and a more specific need
    // ("I specifically need to know when the module was synced")
    TEvent genericInstallEvent;
    genericInstallEvent.mArgumentList.append(QLatin1String("sysInstall"));
    genericInstallEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    genericInstallEvent.mArgumentList.append(packageName);
    genericInstallEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    raiseEvent(genericInstallEvent);

    TEvent detailedInstallEvent;
    switch (module) {
    case 0:
        detailedInstallEvent.mArgumentList.append(QLatin1String("sysInstallPackage"));
        break;
    case 1:
        detailedInstallEvent.mArgumentList.append(QLatin1String("sysInstallModule"));
        break;
    case 2:
        detailedInstallEvent.mArgumentList.append(QLatin1String("sysSyncInstallModule"));
        break;
    case 3:
        detailedInstallEvent.mArgumentList.append(QLatin1String("sysLuaInstallModule"));
        break;
    default:
        Q_UNREACHABLE();
    }
    detailedInstallEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    detailedInstallEvent.mArgumentList.append(packageName);
    detailedInstallEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    detailedInstallEvent.mArgumentList.append(fileName);
    detailedInstallEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    raiseEvent(detailedInstallEvent);

    return true;
}

// credit: http://john.nachtimwald.com/2010/06/08/qt-remove-directory-and-its-contents/
bool Host::removeDir(const QString& dirName, const QString& originalPath)
{
    bool result = true;
    QDir dir(dirName);
    if (dir.exists(dirName)) {
        Q_FOREACH (QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
            // prevent recursion outside of the original branch
            if (info.isDir() && info.absoluteFilePath().startsWith(originalPath)) {
                result = removeDir(info.absoluteFilePath(), originalPath);
            } else {
                result = QFile::remove(info.absoluteFilePath());
            }

            if (!result) {
                return result;
            }
        }
        result = dir.rmdir(dirName);
    }

    return result;
}

// This may be called by installPackage(...) in that case however it will have
// module == 2 and in THAT situation it will NOT RE-invoke installPackage(...)
// again - Slysven
bool Host::uninstallPackage(const QString& packageName, int module)
{
//     As with the installPackage, the module codes are:
//     0=package, 1=uninstall from dialog, 2=uninstall due to module syncing,
//     3=uninstall from a script

    if (module) {
        if (!mInstalledModules.contains(packageName)) {
            return false;
        }
    } else {
        if (!mInstalledPackages.contains(packageName)) {
            return false;
        }
    }

    // raise 2 events - a generic one and a more detailed one to serve both
    // a simple need ("I just want the uninstall event") and a more specific need
    // ("I specifically need to know when the module was uninstalled via Lua")
    TEvent genericUninstallEvent;
    genericUninstallEvent.mArgumentList.append(QLatin1String("sysUninstall"));
    genericUninstallEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    genericUninstallEvent.mArgumentList.append(packageName);
    genericUninstallEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    raiseEvent(genericUninstallEvent);

    TEvent detailedUninstallEvent;
    switch (module) {
    case 0:
        detailedUninstallEvent.mArgumentList.append(QLatin1String("sysUninstallPackage"));
        break;
    case 1:
        detailedUninstallEvent.mArgumentList.append(QLatin1String("sysUninstallModule"));
        break;
    case 2:
        detailedUninstallEvent.mArgumentList.append(QLatin1String("sysSyncUninstallModule"));
        break;
    case 3:
        detailedUninstallEvent.mArgumentList.append(QLatin1String("sysLuaUninstallModule"));
        break;
    default:
        Q_UNREACHABLE();
    }
    detailedUninstallEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    detailedUninstallEvent.mArgumentList.append(packageName);
    detailedUninstallEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    raiseEvent(detailedUninstallEvent);

    int dualInstallations = 0;
    if (mInstalledModules.contains(packageName) && mInstalledPackages.contains(packageName)) {
        dualInstallations = 1;
    }
    //we check for the module=3 because if we reset the editor, we will re-execute the
    //module uninstall, thus creating an infinite loop.
    if (mpEditorDialog && module != 3) {
        mpEditorDialog->doCleanReset();
    }
    mTriggerUnit.uninstall(packageName);
    mTimerUnit.uninstall(packageName);
    mAliasUnit.uninstall(packageName);
    mActionUnit.uninstall(packageName);
    mScriptUnit.uninstall(packageName);
    mKeyUnit.uninstall(packageName);
    if (module) {
        //if module == 2, this is a temporary uninstall for reloading so we exit here
        QStringList entry = mInstalledModules[packageName];
        mInstalledModules.remove(packageName);
        mActiveModules.removeAll(packageName);
        if (module == 2) {
            return true;
        }
        //if module == 1/3, we actually uninstall it.
        //reinstall the package if it shared a module name.  This is a kludge, but it's cleaner than adding extra arguments/etc imo
        if (dualInstallations) {
            //we're a dual install, reinstalling package
            mInstalledPackages.removeAll(packageName); //so we don't get denied from installPackage
            //get the pre package list so we don't get duplicates
            installPackage(entry[0], 0);
        }
    } else {
        mInstalledPackages.removeAll(packageName);
        if (dualInstallations) {
            QStringList entry = mInstalledModules[packageName];
            installPackage(entry[0], 1);
            //restore the module edit flag
            mInstalledModules[packageName] = entry;
        }
    }
    if (mpEditorDialog && module != 3) {
        mpEditorDialog->doCleanReset();
    }

    getActionUnit()->updateToolbar();

    QString _home = QDir::homePath();
    _home.append("/.config/mudlet/profiles/");
    _home.append(getName());
    QString _dest = QString("%1/%2/").arg(_home, packageName);
    removeDir(_dest, _dest);
    saveProfile();
    //NOW we reset if we're uninstalling a module
    if (mpEditorDialog && module == 3) {
        mpEditorDialog->doCleanReset();
    }
    return true;
}

void Host::readPackageConfig(const QString& luaConfig, QString& packageName)
{
    QFile configFile(luaConfig);
    QStringList strings;
    if (configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&configFile);
        while (!in.atEnd()) {
            strings += in.readLine();
        }
    }

    lua_State* L = luaL_newstate();
    luaL_openlibs(L);

    int error = luaL_loadstring(L, strings.join("\n").toLatin1().data());

    if (!error) {
        error = lua_pcall(L, 0, 0, 0);
    }

    if (!error) {
        // for now, only read the mpackage parameter
        // would be nice to read author, save & version too later
        lua_getglobal(L, "mpackage");
        if (lua_isstring(L, -1)) {
            packageName = QString(lua_tostring(L, -1));
        }
        lua_pop(L, -1);
        lua_close(L);
        return;
    } else {
        // error
        std::string e = "no error message available from Lua";
        e = lua_tostring(L, -1);
        std::string reason;
        switch (error) {
        case 4:
            reason = "Out of memory";
            break;
        case 3:
            reason = "Syntax error";
            break;
        case 2:
            reason = "Runtime error";
            break;
        case 1:
            reason = "Yield error";
            break;
        default:
            reason = "Unknown error";
            break;
        }

        if (mudlet::debugMode) {
            qDebug() << reason.c_str() << " in config.lua:" << e.c_str();
        }
        // should print error to main display
        QString msg = QString("%1 in config.lua: %2\n").arg(reason.c_str(), e.c_str());
        mpConsole->printSystemMessage(msg);


        lua_pop(L, -1);
        lua_close(L);
    }
}

// Derived from the one in dlgConnectionProfile class - but it does not need a
// host name argument...
void Host::writeProfileData(const QString & item, const QString & what)
{
    QFile file( QStringLiteral( "%1/.config/mudlet/profiles/%2/%3" )
                .arg(QDir::homePath(), getName(), item) );
    file.open( QIODevice::WriteOnly | QIODevice::Unbuffered );
    QDataStream ofs( & file );
    ofs << what;
    file.close();
}
