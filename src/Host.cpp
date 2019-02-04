/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2015-2019 by Stephen Lyons - slysven@virginmedia.com    *
 *   Copyright (C) 2016 by Ian Adkins - ieadkins@gmail.com                 *
 *   Copyright (C) 2018 by Huadong Qi - novload@outlook.com                *
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
#include "XMLimport.h"
#include "dlgMapper.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"

#include "pre_guard.h"
#include <QtUiTools>
#include <zip.h>
#include "post_guard.h"

Host::Host(int port, const QString& hostname, const QString& login, const QString& pass, int id)
: mTelnet(this)
, mpConsole(nullptr)
, mLuaInterpreter(this, id)
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
, mCommandLineFont(QFont("Bitstream Vera Sans Mono", 10, QFont::Normal))
, mCommandSeparator(QLatin1String(";"))
, mDisplayFont(QFont("Bitstream Vera Sans Mono", 10, QFont::Normal))
, mEnableGMCP(true)
, mEnableMSDP(false)
, mServerMXPenabled(true)
, mFORCE_GA_OFF(false)
, mFORCE_NO_COMPRESSION(false)
, mFORCE_SAVE_ON_EXIT(false)
, mInsertedMissingLF(false)
, mIsGoingDown(false)
, mIsProfileLoadingSequence(false)
, mLF_ON_GA(true)
, mNoAntiAlias(false)
, mpEditorDialog(nullptr)
, mpMap(new TMap(this))
, mpNotePad(nullptr)
, mPrintCommand(true)
, mIsRemoteEchoingActive(false)
, mIsCurrentLogFileInHtmlFormat(false)
, mIsNextLogFileInHtmlFormat(false)
, mIsLoggingTimestamps(false)
, mLogDir(QString())
, mLogFileName(QString())
, mLogFileNameFormat(QLatin1String("yyyy-MM-dd#HH-mm-ss")) // In the past we have used "yyyy-MM-dd#hh-mm-ss" but we always want a 24-hour clock
, mResetProfile(false)
, mScreenHeight(25)
, mScreenWidth(90)
, mTimeout(60)
, mUSE_FORCE_LF_AFTER_PROMPT(false)
, mUSE_IRE_DRIVER_BUGFIX(true)
, mUSE_UNIX_EOL(false)
, mWrapAt(100)
, mWrapIndentCount(0)
, mEditorTheme(QLatin1String("Mudlet"))
, mEditorThemeFile(QLatin1String("Mudlet.tmTheme"))
, mThemePreviewItemID(-1)
, mThemePreviewType(QString())
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
, mMapStrongHighlight(false)
, mSpellDic(QLatin1String("en_US"))
, mLogStatus(false)
, mEnableSpellCheck(true)
, mDiscordDisableServerSide(true)
, mDiscordAccessFlags(DiscordLuaAccessEnabled | DiscordSetSubMask)
, mLineSize(10.0)
, mRoomSize(0.5)
, mShowInfo(true)
, mBubbleMode(false)
, mShowRoomID(false)
, mShowPanel(true)
, mServerGUI_Package_version(QLatin1String("-1"))
, mServerGUI_Package_name(QLatin1String("nothing"))
, mAcceptServerGUI(true)
, mCommandLineFgColor(Qt::darkGray)
, mCommandLineBgColor(Qt::black)
, mMapperUseAntiAlias(true)
, mFORCE_MXP_NEGOTIATION_OFF(false)
, mpDockableMapWidget()
, mEnableTextAnalyzer(false)
, mTimerDebugOutputSuppressionInterval(QTime())
, mTriggerUnit(this)
, mTimerUnit(this)
, mScriptUnit(this)
, mAliasUnit(this)
, mActionUnit(this)
, mKeyUnit(this)
, mHostID(id)
, mHostName(hostname)
, mIsClosingDown(false)
, mLogin(login)
, mPass(pass)
, mPort(port)
, mRetries(5)
, mSaveProfileOnExit(false)
, mHaveMapperScript(false)
, mAutoAmbigousWidthGlyphsSetting(true)
, mWideAmbigousWidthGlyphs(false)
, mSGRCodeHasColSpaceId(false)
, mServerMayRedefineColors(false)
{
    // mLogStatus = mudlet::self()->mAutolog;
    mLuaInterface.reset(new LuaInterface(this));
    QString directoryLogFile = mudlet::getMudletPath(mudlet::profileDataItemPath, mHostName, QStringLiteral("log"));
    QString logFileName = QStringLiteral("%1/errors.txt").arg(directoryLogFile);
    QDir dirLogFile;
    if (!dirLogFile.exists(directoryLogFile)) {
        dirLogFile.mkpath(directoryLogFile);
    }
    mErrorLogFile.setFileName(logFileName);
    mErrorLogFile.open(QIODevice::Append);
    // This is NOW used (for map
    // file auditing and other issues)
    mErrorLogStream.setDevice(&mErrorLogFile);

    QTimer::singleShot(0, this, [this]() {
        qDebug() << "Host::Host() - restore map case 4 {QTimer::singleShot(0)} lambda.";
        if (mpMap->restore(QString(), false)) {
            mpMap->audit();
            if (mpMap->mpMapper) {
                mpMap->mpMapper->mp2dMap->init();
                mpMap->mpMapper->updateAreaComboBox();
                mpMap->mpMapper->resetAreaComboBoxToPlayerRoomArea();
                mpMap->mpMapper->show();
            }
        }
    });

    mGMCP_merge_table_keys.append("Char.Status");
    mDoubleClickIgnore.insert('"');
    mDoubleClickIgnore.insert('\'');

    // search engine load entries
    mSearchEngineData = QMap<QString, QString>(
    {
                    {"Bing",       "https://www.bing.com/search?q="},
                    {"DuckDuckGo", "https://duckduckgo.com/?q="},
                    {"Google",     "https://www.google.com/search?q="}
    });

    auto optin = readProfileData(QStringLiteral("discordserveroptin"));
    if (!optin.isEmpty()) {
        mDiscordDisableServerSide = optin.toInt() == Qt::Unchecked ? true : false;
    }
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

void Host::saveModules(int sync, bool backup)
{
    QMapIterator<QString, QStringList> it(modulesToWrite);
    mModulesToSync.clear();
    QString savePath = mudlet::getMudletPath(mudlet::moduleBackupsPath);
    auto savePathDir = QDir(savePath);
    if (!savePathDir.exists()) {
        savePathDir.mkpath(savePath);
    }
    while (it.hasNext()) {
        it.next();
        QStringList entry = it.value();
        QString moduleName = it.key();
        QString filename_xml = entry[0];

        if (backup) {
            // CHECKME: Consider changing datetime spec to more "sortable" "yyyy-MM-dd#HH-mm-ss" (1 of 6)
            QString time = QDateTime::currentDateTime().toString("dd-MM-yyyy#hh-mm-ss");
            savePathDir.rename(filename_xml, savePath + moduleName + time); //move the old file, use the key (module name) as the file
        }

        auto writer = new XMLexport(this);
        writers.insert(filename_xml, writer);
        writer->writeModuleXML(moduleName, filename_xml);

        if (entry[1].toInt()) {
            mModulesToSync << moduleName;
        }
    }
    modulesToWrite.clear();

    if (sync) {
        connect(this, &Host::profileSaveFinished, this, &Host::slot_reloadModules);
    }
}

void Host::slot_reloadModules()
{
    // update the module zips
    updateModuleZips();

    //synchronize modules across sessions
    QMap<Host*, TConsole*> activeSessions = mudlet::self()->mConsoleMap;
    QMapIterator<Host*, TConsole*> sessionIterator(activeSessions);
    while (sessionIterator.hasNext()) {
        sessionIterator.next();
        Host* otherHost = sessionIterator.key();
        if (otherHost->getName() == mHostName) {
            continue;
        }
        QMap<QString, int> modulePri = otherHost->mModulePriorities;
        QMap<int, QStringList> moduleOrder;

        auto modulePrioritiesIt = modulePri.constBegin();
        while (modulePrioritiesIt != modulePri.constEnd()) {
            moduleOrder[modulePrioritiesIt.value()].append(modulePrioritiesIt.key());
            ++modulePrioritiesIt;
        }

        QMapIterator<int, QStringList> it(moduleOrder);
        while (it.hasNext()) {
            it.next();
            QStringList moduleList = it.value();
            for (int i = 0, total = moduleList.size(); i < total; ++i) {
                QString moduleName = moduleList[i];
                if (mModulesToSync.contains(moduleName)) {
                    otherHost->reloadModule(moduleName);
                }
            }
        }
    }

    // disconnect the one-time event so we're not always reloading modules whenever a profile save happens
    mModulesToSync.clear();
    QObject::disconnect(this, &Host::profileSaveFinished, this, &Host::slot_reloadModules);
}

void Host::updateModuleZips() const
{
    QMapIterator<QString, QStringList> it(modulesToWrite);
    while (it.hasNext()) {
        it.next();
        QStringList entry = it.value();
        QString moduleName = it.key();
        QString filename_xml = entry[0];

        QString zipName;
        zip* zipFile = nullptr;
        if (filename_xml.endsWith(QStringLiteral("mpackage"), Qt::CaseInsensitive) || filename_xml.endsWith(QStringLiteral("zip"), Qt::CaseInsensitive)) {
            QString packagePathName = mudlet::getMudletPath(mudlet::profilePackagePath, mHostName, moduleName);
            filename_xml = mudlet::getMudletPath(mudlet::profilePackagePathFileName, mHostName, moduleName);
            int err;
            zipFile = zip_open(entry[0].toStdString().c_str(), ZIP_CREATE, &err);
            zipName = filename_xml;
            QDir packageDir = QDir(packagePathName);
            if (!packageDir.exists()) {
                packageDir.mkpath(packagePathName);
            }

            struct zip_source* s = zip_source_file(zipFile, filename_xml.toStdString().c_str(), 0, 0);
            err = zip_add(zipFile, QString(moduleName + ".xml").toStdString().c_str(), s);
            //FIXME: error checking
            if (zipFile) {
                err = zip_close(zipFile);
                //FIXME: error checking
            }
        }
    }
}


void Host::reloadModule(const QString& reloadModuleName)
{
    QMap<QString, QStringList> installedModules = mInstalledModules;
    QMapIterator<QString, QStringList> moduleIterator(installedModules);
    while (moduleIterator.hasNext()) {
        moduleIterator.next();
        const auto& moduleName = moduleIterator.key();
        const auto& moduleLocation = moduleIterator.value()[0];

        if (moduleName == reloadModuleName) {
            uninstallPackage(moduleName, 2);
            installPackage(moduleLocation, 2);
        }
    }
    //iterate through mInstalledModules again and reset the entry flag to be correct.
    //both the installedModules and mInstalled should be in the same order now as well
    moduleIterator.toFront();
    while (moduleIterator.hasNext()) {
        moduleIterator.next();
        QStringList entry = installedModules[moduleIterator.key()];
        mInstalledModules[moduleIterator.key()] = entry;
    }
}

void Host::resetProfile()
{
    getTimerUnit()->stopAllTriggers();
    mudlet::self()->mTimerMap.clear();
    getTimerUnit()->removeAllTempTimers();
    getTriggerUnit()->removeAllTempTriggers();
    getKeyUnit()->removeAllTempKeys();


    mTimerUnit.doCleanup();
    mTriggerUnit.doCleanup();
    mKeyUnit.doCleanup();
    mpConsole->resetMainConsole();
    mEventHandlerMap.clear();
    mEventMap.clear();
    mLuaInterpreter.initLuaGlobals();
    mLuaInterpreter.loadGlobal();
    mLuaInterpreter.initIndenterGlobals();
    mBlockScriptCompile = false;


    getTriggerUnit()->compileAll();
    getAliasUnit()->compileAll();
    getActionUnit()->compileAll();
    getKeyUnit()->compileAll();
    getScriptUnit()->compileAll();
    // All the Timers are NOT compiled here;
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
std::tuple<bool, QString, QString> Host::saveProfile(const QString& saveFolder, const QString& saveName, bool syncModules)
{
    emit profileSaveStarted();
    qApp->processEvents();

    QString directory_xml;
    if (saveFolder.isEmpty()) {
        directory_xml = mudlet::getMudletPath(mudlet::profileXmlFilesPath, getName());
    } else {
        directory_xml = saveFolder;
    }

    // CHECKME: Consider changing datetime spec to more "sortable" "yyyy-MM-dd#HH-mm-ss" (2 of 6)
    QString filename_xml;
    if (saveName.isEmpty()) {
        filename_xml = QStringLiteral("%1/%2.xml").arg(directory_xml, QDateTime::currentDateTime().toString(QStringLiteral("dd-MM-yyyy#hh-mm-ss")));
    } else {
        filename_xml = QStringLiteral("%1/%2.xml").arg(directory_xml, saveName);
    }

    QDir dir_xml;
    if (!dir_xml.exists(directory_xml)) {
        dir_xml.mkpath(directory_xml);
    }

    if (currentlySavingProfile()) {
        return std::make_tuple(false, QString(), QStringLiteral("a save is already in progress"));
    }

    auto writer = new XMLexport(this);
    writers.insert(QStringLiteral("profile"), writer);
    writer->exportHost(filename_xml);
    saveModules(syncModules ? 1 : 0, saveName == QStringLiteral("autosave") ? false : true);
    return std::make_tuple(true, filename_xml, QString());
}

// exports without the host settings for some reason
std::tuple<bool, QString, QString> Host::saveProfileAs(const QString& file)
{
    emit profileSaveStarted();
    qApp->processEvents();

    if (currentlySavingProfile()) {
        return std::make_tuple(false, QString(), QStringLiteral("a save is already in progress"));
    }

    auto writer = new XMLexport(this);
    writers.insert(QStringLiteral("profile"), writer);
    writer->exportProfile(file);
    return std::make_tuple(true, file, QString());
}

void Host::xmlSaved(const QString& xmlName)
{
    qDebug() << "saved" << xmlName;
    if (writers.contains(xmlName)) {
        auto writer = writers.take(xmlName);
        delete writer;
    }

    if (writers.empty()) {
        emit profileSaveFinished();
    }
}

bool Host::currentlySavingProfile()
{
    return !writers.empty();
}

void Host::waitForProfileSave()
{
    for (auto& writer : writers) {
        for (auto& future: writer->saveFutures) {
            future.waitForFinished();
        }
    }
}

void Host::setMmpMapLocation(const QString& data)
{
    auto document = QJsonDocument::fromJson(data.toUtf8());
    if (!document.isObject()) {
        return;
    }
    auto json = document.object();
    if (json.isEmpty()) {
        return;
    }

    auto urlValue = json.value(QStringLiteral("url"));
    if (urlValue == QJsonValue::Undefined) {
        return;
    }
    auto url = QUrl(urlValue.toString());
    if (!url.isValid()) {
        return;
    }

    mpMap->setMmpMapLocation(urlValue.toString());
}

QString Host::getMmpMapLocation() const
{
    return mpMap->getMmpMapLocation();
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

void Host::stopAllTriggers()
{
    mTriggerUnit.stopAllTriggers();
    mAliasUnit.stopAllTriggers();
    mTimerUnit.stopAllTriggers();
    mKeyUnit.stopAllTriggers();
}

void Host::reenableAllTriggers()
{
    mTriggerUnit.reenableAllTriggers();
    mAliasUnit.reenableAllTriggers();
    mTimerUnit.reenableAllTriggers();
    mKeyUnit.reenableAllTriggers();
}

QPair<QString, QString> Host::getSearchEngine()
{
    if (mSearchEngineData.contains(mSearchEngineName))
        return qMakePair(mSearchEngineName, mSearchEngineData.value(mSearchEngineName));
    else
        return qMakePair(QStringLiteral("Google"), mSearchEngineData.value(QStringLiteral("Google")));
}

// cmd is UTF-16BE encoded here, but will be transcoded to Server's one by
// cTelnet::sendData(...) call:
void Host::send(QString cmd, bool wantPrint, bool dontExpandAliases)
{
    if (wantPrint && (! mIsRemoteEchoingActive) && mPrintCommand) {
        mInsertedMissingLF = true;
        if (!cmd.isEmpty() || !mUSE_IRE_DRIVER_BUGFIX || mUSE_FORCE_LF_AFTER_PROMPT) {
            // used to print the terminal <LF> that terminates a telnet command
            // this is important to get the cursor position right
            mpConsole->printCommand(cmd);
        }
        mpConsole->update();
    }
    QStringList commandList;
    if (!mCommandSeparator.isEmpty()) {
        commandList = cmd.split(QString(mCommandSeparator), QString::SkipEmptyParts);
    } else {
        // don't split command if the command separator is blank
        commandList << cmd;
    }

    if (!dontExpandAliases) {
        // allow sending blank commands
        if (commandList.empty()) {
            QString payload(QChar::LineFeed);
            mTelnet.sendData(payload);
            return;
        }
    }

    for (int i = 0, total = commandList.size(); i < total; ++i) {
        if (commandList.at(i).isEmpty()) {
            continue;
        }
        QString command = commandList.at(i);
        command.remove(QChar::LineFeed);
        if (dontExpandAliases) {
            mTelnet.sendData(command);
            continue;
        }

        if (!mAliasUnit.processDataStream(command)) {
            mTelnet.sendData(command);
        }
    }
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
    } else {
        return false;
    }
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
    } else {
        return false;
    }
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
    // pointer just in case we accidentally reenter this method in the future.
    QDialog* pUnzipDialog = Q_NULLPTR;

    //     Module notes:
    //     For the module install, a module flag of 0 is a package,
    // a flag of 1 means the module is being installed for the first time via the UI,
    // a flag of 2 means the module is being synced (so it's "installed" already),
    // a flag of 3 means the module is being installed from a script.
    //     This separation is necessary to be able to reuse code while avoiding infinite loops from script installations.

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
    if (fileName.endsWith(QStringLiteral(".zip"), Qt::CaseInsensitive) || fileName.endsWith(QStringLiteral(".mpackage"), Qt::CaseInsensitive)) {
        QString _home = mudlet::getMudletPath(mudlet::profileHomePath, getName());
        QString _dest = mudlet::getMudletPath(mudlet::profilePackagePath, getName(), packageName);
        // home directory for the PROFILE
        QDir _tmpDir(_home);
        // directory to store the expanded archive file contents
        _tmpDir.mkpath(_dest);

        // TODO: report failure to create destination folder for package/module in profile

        QUiLoader loader(this);
        QFile uiFile(QStringLiteral(":/ui/package_manager_unpack.ui"));
        uiFile.open(QFile::ReadOnly);
        pUnzipDialog = dynamic_cast<QDialog*>(loader.load(&uiFile, nullptr));
        uiFile.close();
        if (!pUnzipDialog) {
            return false;
        }

        auto * pLabel = pUnzipDialog->findChild<QLabel*>(QStringLiteral("label"));
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

        auto successful = mudlet::unzip(fileName, _dest, _tmpDir);
        pUnzipDialog->deleteLater();
        pUnzipDialog = Q_NULLPTR;
        if (!successful) {
            return false;
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

    // make any fonts in the package available to Mudlet for use
    if (module != 2) {
        installPackageFonts(packageName);
    }

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

    QString dest = mudlet::getMudletPath(mudlet::profilePackagePath, getName(), packageName);
    removeDir(dest, dest);
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

    int error = luaL_loadstring(L, strings.join("\n").toUtf8().constData());

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
QPair<bool, QString> Host::writeProfileData(const QString& item, const QString& what)
{
    QFile file(mudlet::getMudletPath(mudlet::profileDataItemPath, getName(), item));
    if (file.open(QIODevice::WriteOnly | QIODevice::Unbuffered)) {
        QDataStream ofs(&file);
        ofs << what;
        file.close();
    }

    if (file.error() == QFile::NoError) {
        return qMakePair(true, QString());
    } else {
        return qMakePair(false, file.errorString());
    }
}

// Similar to the above, a convenience for reading profile data for this host.
QString Host::readProfileData(const QString& item)
{
    QFile file(mudlet::getMudletPath(mudlet::profileDataItemPath, getName(), item));
    bool success = file.open(QIODevice::ReadOnly);
    QString ret;
    if (success) {
        QDataStream ifs(&file);
        ifs >> ret;
        file.close();
    }

    return ret;
}

// makes fonts in a given package/module be available for Mudlet scripting
// does not install font system-wide
void Host::installPackageFonts(const QString &packageName)
{
    auto packagePath = mudlet::getMudletPath(mudlet::profilePackagePath, getName(), packageName);

    QDirIterator it(packagePath, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        auto filePath = it.next();

        if (filePath.endsWith(QLatin1String(".otf"), Qt::CaseInsensitive) || filePath.endsWith(QLatin1String(".ttf"), Qt::CaseInsensitive) ||
            filePath.endsWith(QLatin1String(".ttc"), Qt::CaseInsensitive) || filePath.endsWith(QLatin1String(".otc"), Qt::CaseInsensitive)) {

            mudlet::self()->mFontManager.loadFont(filePath);
        }
    }
}

// ensures fonts from all installed packages are loaded in Mudlet
void Host::refreshPackageFonts()
{
    for (const auto& package : mInstalledPackages) {
        installPackageFonts(package);
    }
}

void Host::setWideAmbiguousEAsianGlyphs(const Qt::CheckState state)
{
    bool localState = false;
    bool needToEmit = false;
    const QString encoding(mTelnet.getEncoding());

    QMutexLocker locker(& mLock);
    if (state == Qt::PartiallyChecked) {
        // Set things automatically
        mAutoAmbigousWidthGlyphsSetting = true;

        if ( encoding == QLatin1String("GBK")
             || encoding == QLatin1String("GB18030")
             || encoding == QLatin1String("Big5")) {

            // Need to use wide width for ambiguous characters
            if (!mWideAmbigousWidthGlyphs) {
                // But the last setting was narrow - so we need to change
                mWideAmbigousWidthGlyphs = true;
                localState = true;
                needToEmit = true;
            }

        } else {
            // Need to use narrow width for ambiguous characters
            if (mWideAmbigousWidthGlyphs) {
                // But the last setting was wide - so we need to change
                mWideAmbigousWidthGlyphs = false;
                localState = false;
                needToEmit = true;
            }

        }

    } else {
        // Set things manually:
        mAutoAmbigousWidthGlyphsSetting = false;
        if (mWideAmbigousWidthGlyphs != (state == Qt::Checked)) {
            // The last setting is the opposite to what we want:

            mWideAmbigousWidthGlyphs = (state == Qt::Checked);
            localState = (state == Qt::Checked);
            needToEmit = true;
        };

    }

    locker.unlock();
    // We do not need to keep the mutex any longer as we have a local copy to
    // work with whilst the connected methods react to the signal:
    if (needToEmit) {
        emit signal_changeIsAmbigousWidthGlyphsToBeWide(localState);
    }
}

QColor Host::getAnsiColor(const int ansiCode, const bool isBackground) const
{
    // clang-format off
    switch (ansiCode) {
    case 0:         return mBlack;
    case 1:         return mRed;
    case 2:         return mGreen;
    case 3:         return mYellow;
    case 4:         return mBlue;
    case 5:         return mMagenta;
    case 6:         return mCyan;
    case 7:         return mWhite;
    case 8:         return mLightBlack;
    case 9:         return mLightRed;
    case 10:        return mLightGreen;
    case 11:        return mLightYellow;
    case 12:        return mLightBlue;
    case 13:        return mLightMagenta;
    case 14:        return mLightCyan;
    case 15:        return mLightWhite;
    // Grey scale divided into 24 values:
    case 232:       return QColor(  0,   0,   0); //   0.000
    case 233:       return QColor( 11,  11,  11); //  11.087
    case 234:       return QColor( 22,  22,  22); //  22.174
    case 235:       return QColor( 33,  33,  33); //  33.261
    case 236:       return QColor( 44,  44,  44); //  44.348
    case 237:       return QColor( 55,  55,  55); //  55.435
    case 238:       return QColor( 67,  67,  67); //  66.522
    case 239:       return QColor( 78,  78,  78); //  77.609
    case 240:       return QColor( 89,  89,  89); //  88.696
    case 241:       return QColor(100, 100, 100); //  99.783
    case 242:       return QColor(111, 111, 111); // 110.870
    case 243:       return QColor(122, 122, 122); // 121.957
    case 244:       return QColor(133, 133, 133); // 133.043
    case 245:       return QColor(144, 144, 144); // 144.130
    case 246:       return QColor(155, 155, 155); // 155.217
    case 247:       return QColor(166, 166, 166); // 166.304
    case 248:       return QColor(177, 177, 177); // 177.391
    case 249:       return QColor(188, 188, 188); // 188.478
    case 250:       return QColor(200, 200, 200); // 199.565
    case 251:       return QColor(211, 211, 211); // 210.652
    case 252:       return QColor(222, 222, 222); // 221.739
    case 253:       return QColor(233, 233, 233); // 232.826
    case 254:       return QColor(244, 244, 244); // 243.913
    case 255:       return QColor(255, 255, 255); // 255.000
    default:
        if (ansiCode == TTrigger::scmIgnored) {
            // No-op - corresponds to no setting or ignoring this aspect
            return QColor();
        } else if (ansiCode == TTrigger::scmDefault) {
            return isBackground ? mBgColor : mFgColor;
        } else if (ansiCode >= 16 && ansiCode <= 231) {
            // because color 1-15 behave like normal ANSI colors we need to subtract 16
            // 6x6 RGB color space
            int r = (ansiCode - 16) / 36;
            int g = (ansiCode - 16 - (r * 36)) / 6;
            int b = (ansiCode - 16 - (r * 36)) - (g * 6);
            // The following WERE using 42 as factor but that does not reflect
            // changes already made in TBuffer::translateToPlainText a while ago:
            return QColor(r * 51, g * 51, b * 51);
        } else {
            return QColor(); // Noop
        }
    }
    // clang-format on
}

// handles out of band (OOB) GMCP/MSDP data for Discord - called whenever GMCP
// Telnet sub-option comes in and starts with "External.Discord.(Status|Info)"
void Host::processDiscordGMCP(const QString& packageMessage, const QString& data)
{
    if (mDiscordDisableServerSide) {
        return;
    }

    auto document = QJsonDocument::fromJson(data.toUtf8());
    if (!document.isObject()) {
        return;
    }

    auto json = document.object();
    if (json.isEmpty()) {
        return;
    }

    if (packageMessage == QLatin1String("External.Discord.Status")) {
        processGMCPDiscordStatus(json);
    } else if (packageMessage == QLatin1String("External.Discord.Info")) {
        processGMCPDiscordInfo(json);
    }
}

void Host::processGMCPDiscordInfo(const QJsonObject& discordInfo)
{
    mudlet* pMudlet = mudlet::self();
    bool hasInvite = false;
    auto inviteUrl = discordInfo.value(QStringLiteral("inviteurl"));
    // Will be of form: "https://discord.gg/#####"
    if (inviteUrl != QJsonValue::Undefined) {
        hasInvite = true;
    }

    bool hasApplicationId = false;
    bool hasCustomAppID = false;
    auto appID = discordInfo.value(QStringLiteral("applicationid"));
    if (appID != QJsonValue::Undefined) {
        hasApplicationId = true;
        if (appID.toString() == Discord::mMudletApplicationId) {
            pMudlet->mDiscord.setApplicationID(this, QString());
        } else {
            hasCustomAppID = true;
            pMudlet->mDiscord.setApplicationID(this, appID.toString());
            auto image = pMudlet->mDiscord.getLargeImage(this);

            if (image.isEmpty() || image == QLatin1String("mudlet")) {
                pMudlet->mDiscord.setLargeImage(this, QStringLiteral("server-icon"));
            }
        }
    }

    if (hasInvite) {
        if (hasCustomAppID) {
            qDebug() << "Game using a custom Discord server. Invite URL: " << inviteUrl.toString();
        } else if (hasApplicationId) {
            qDebug() << "Game using Mudlets Discord server. Invite URL: " << inviteUrl.toString();
        } else {
            qDebug() << "Discord invite URL: " << inviteUrl.toString();
        }
    } else {
        if (hasCustomAppID) {
            qDebug() << "Game is using custom server Discord application ID";
        } else if (hasApplicationId) {
            qDebug() << "Game is using Mudlets Discord application ID";
        }
    }
}

void Host::processGMCPDiscordStatus(const QJsonObject& discordInfo)
{
    auto pMudlet = mudlet::self();
    auto gameName = discordInfo.value(QStringLiteral("game"));
    if (gameName != QJsonValue::Undefined) {
        QPair<bool, QString> richPresenceSupported = pMudlet->mDiscord.gameIntegrationSupported(getUrl());
        if (richPresenceSupported.first && pMudlet->mDiscord.usingMudletsDiscordID(this)) {
            pMudlet->mDiscord.setDetailText(this, tr("Playing %1").arg(richPresenceSupported.second));
            pMudlet->mDiscord.setLargeImage(this, richPresenceSupported.second);
            pMudlet->mDiscord.setLargeImageText(this, tr("%1 at %2:%3", "%1 is the game name and %2:%3 is game server address like: mudlet.org:23").arg(gameName.toString(), getUrl(), QString::number(getPort())));
        } else {
            // We are using a custom application id, so the top line is
            // likely to be saying "Playing MudName"
            if (richPresenceSupported.first) {
                pMudlet->mDiscord.setDetailText(this, QString());
                pMudlet->mDiscord.setLargeImageText(this, tr("%1 at %2:%3", "%1 is the game name and %2:%3 is game server address like: mudlet.org:23").arg(gameName.toString(), getUrl(), QString::number(getPort())));
                pMudlet->mDiscord.setLargeImage(this, QStringLiteral("server-icon"));
            }
        }
    }

    auto details = discordInfo.value(QStringLiteral("details"));
    if (details != QJsonValue::Undefined) {
        pMudlet->mDiscord.setDetailText(this, details.toString());
    }

    auto state = discordInfo.value(QStringLiteral("state"));
    if (state != QJsonValue::Undefined) {
        pMudlet->mDiscord.setStateText(this, state.toString());
    }

    auto largeImages = discordInfo.value(QStringLiteral("largeimage"));
    if (largeImages != QJsonValue::Undefined) {
        auto largeImage = largeImages.toArray().first();
        if (largeImage != QJsonValue::Undefined) {
            pMudlet->mDiscord.setSmallImage(this, largeImage.toString());
        }
    }

    auto largeImageText = discordInfo.value(QStringLiteral("largeimagetext"));
    if (largeImageText != QJsonValue::Undefined) {
        pMudlet->mDiscord.setSmallImageText(this, largeImageText.toString());
    }

    auto smallImages = discordInfo.value(QStringLiteral("smallimage"));
    if (smallImages != QJsonValue::Undefined) {
        auto smallImage = smallImages.toArray().first();
        if (smallImage != QJsonValue::Undefined) {
            pMudlet->mDiscord.setSmallImage(this, smallImage.toString());
        }
    }

    auto smallImageText = discordInfo.value(QStringLiteral("smallimagetext"));
    if ((smallImageText != QJsonValue::Undefined)) {
        pMudlet->mDiscord.setSmallImageText(this, smallImageText.toString());
    }

    // Use -1 so we can detect (at least during debugging) that a value of 0
    // has been seen:
    int64_t timeStamp = -1;
    auto endTimeStamp = discordInfo.value(QStringLiteral("endtime"));
    if (endTimeStamp.isDouble()) {
        // It is not entirely clear from the proposed specification
        // whether the integral seconds since epoch is a string or a
        // double, so handle both:
        // This only works properly when the value is less than
        // 9007199254740992 but since when I last checked it was
        //       1533042027 second since beginning of 1970 it should be
        // good enough!
        timeStamp = static_cast<int64_t>(endTimeStamp.toDouble());
        pMudlet->mDiscord.setEndTimeStamp(this, timeStamp);
    } else if (endTimeStamp.isString()) {
        timeStamp = endTimeStamp.toString().toLongLong();
        pMudlet->mDiscord.setEndTimeStamp(this, timeStamp);
    } else {
        auto startTimeStamp = discordInfo.value(QStringLiteral("starttime"));
        if (startTimeStamp.isDouble()) {
            timeStamp = static_cast<int64_t>(startTimeStamp.toDouble());
            pMudlet->mDiscord.setStartTimeStamp(this, timeStamp);
        } else if (endTimeStamp.isString()) {
            timeStamp = endTimeStamp.toString().toLongLong();
            pMudlet->mDiscord.setStartTimeStamp(this, timeStamp);
        }
    }

    // Use -1 so we can detect (at least during debugging) that a value of 0
    // has been seen:
    int partySizeValue = -1;
    int partyMaxValue = -1;
    auto partyMax = discordInfo.value(QStringLiteral("partymax"));
    auto partySize = discordInfo.value(QStringLiteral("partysize"));
    if (partyMax.isDouble()) {
        partyMaxValue = static_cast<int>(partyMax.toDouble());
        if (partyMaxValue > 0 && partySize.isDouble()) {
            partySizeValue = static_cast<int>(partySize.toDouble());
            pMudlet->mDiscord.setParty(this, partySizeValue, partyMaxValue);
        } else {
            // Switches off the party detail from the RP
            pMudlet->mDiscord.setParty(this, 0, 0);
        }
    } else {
        if (partySize.isDouble()) {
            partySizeValue = static_cast<int>(partySize.toDouble());
            pMudlet->mDiscord.setParty(this, partySizeValue);
        } else {
            pMudlet->mDiscord.setParty(this, 0, 0);
        }
    }
}

void Host::clearDiscordData()
{
    mudlet* pMudlet = mudlet::self();
    pMudlet->mDiscord.setDetailText(this, QString());
    pMudlet->mDiscord.setStateText(this, QString());
    pMudlet->mDiscord.setLargeImage(this, QString());
    pMudlet->mDiscord.setLargeImageText(this, QString());
    pMudlet->mDiscord.setSmallImage(this, QString());
    pMudlet->mDiscord.setSmallImageText(this, QString());
    pMudlet->mDiscord.setStartTimeStamp(this, 0);
    pMudlet->mDiscord.setParty(this, 0, 0);
}


void Host::processDiscordMSDP(const QString& variable, QString value)
{
    if (mDiscordDisableServerSide) {
        return;
    }

    Q_UNUSED(variable)
    Q_UNUSED(value)
// TODO:
//    if (!(variable == QLatin1String("SERVER_ID") || variable == QLatin1String("AREA_NAME"))) {
//        return;
//    }

//    // MSDP value comes padded with quotes - strip them (from the local copy of
//    // the supplied argument):
//    if (value.startsWith(QLatin1String("\""))) {
//        value = value.mid(1);
//    }

//    if (value.endsWith(QLatin1String("\""))) {
//        value.chop(1);
//    }

//    if (variable == QLatin1String("SERVER_ID")) {
//        mudlet::self()->mDiscord.setGame(this, value);
//    } else if (variable == QLatin1String("AREA_NAME")) {
//        mudlet::self()->mDiscord.setArea(this, value);
//    }
}

void Host::setDiscordApplicationID(const QString& s)
{
    QMutexLocker locker(& mLock);
    mDiscordApplicationID = s;
    locker.unlock();

    writeProfileData(QStringLiteral("discordApplicationId"), s);
}

const QString& Host::getDiscordApplicationID()
{
    QMutexLocker locker(&mLock);
    return mDiscordApplicationID;
}

// Compares the current discord username and discriminator against the non-empty
// arguments. Returns true if neither match, otherwise false.
bool Host::discordUserIdMatch(const QString& userName, const QString& userDiscriminator) const
{
    if (!userName.isEmpty() && !mRequiredDiscordUserName.isEmpty() && userName != mRequiredDiscordUserName) {
        return false;
    }

    if (!userDiscriminator.isEmpty() && !mRequiredDiscordUserDiscriminator.isEmpty() && userDiscriminator != mRequiredDiscordUserDiscriminator) {
        return false;
    } else {
        return true;
    }
}

void Host::setSpellDic(const QString& newDict)
{
    QMutexLocker locker(& mLock);
    bool isChanged = false;
    if (!newDict.isEmpty() && mSpellDic != newDict) {
        mSpellDic = newDict;
        isChanged = true;
    }
    locker.unlock();
    if (isChanged) {
        emit signal_changeSpellDict(mSpellDic);
    }
}
