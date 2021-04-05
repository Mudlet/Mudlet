/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2015-2021 by Stephen Lyons - slysven@virginmedia.com    *
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
#include "TDebug.h"
#include "TMainConsole.h"
#include "TCommandLine.h"
#include "TDebug.h"
#include "TDockWidget.h"
#include "TEvent.h"
#include "TLabel.h"
#include "TMap.h"
#include "TMedia.h"
#include "TRoomDB.h"
#include "TScript.h"
#include "TTextEdit.h"
#include "TToolBar.h"
#include "VarUnit.h"
#include "XMLimport.h"
#include "dlgMapper.h"
#include "dlgModuleManager.h"
#include "dlgNotepad.h"
#include "dlgProfilePreferences.h"
#include "dlgIRC.h"
#include "mudlet.h"

#include "pre_guard.h"
#include <chrono>
#include <QDialog>
#include <QtUiTools>
#include <QNetworkProxy>
#include <zip.h>
#include <memory>
#include "post_guard.h"

using namespace std::chrono;

stopWatch::stopWatch()
: mIsInitialised(false)
, mIsRunning(false)
, mIsPersistent(false)
, mEffectiveStartDateTime()
, mElapsedTime()
{
    mEffectiveStartDateTime.setTimeSpec(Qt::UTC);
}

bool stopWatch::start()
{
    if (!mIsInitialised) {
        mIsInitialised = true;
        mEffectiveStartDateTime = QDateTime::currentDateTimeUtc();
        mIsRunning = true;
        return true;
    }

    if (mIsRunning) {
        // Nothing to do, already running
        return false;
    }

    // Is stopped, so subtract elapsed time from current and set that to be the
    // effective start time:
    mEffectiveStartDateTime = QDateTime::currentDateTimeUtc().addMSecs(-mElapsedTime);
    mIsRunning = true;
    return true;
}

bool stopWatch::stop()
{
    if (!mIsInitialised) {
        // Nothing to do, never started
        return false;
    }

    if (!mIsRunning) {
        // Nothing to do, already stopped
        return false;
    }

    // Is running - so stop and note time:
    mElapsedTime = mEffectiveStartDateTime.msecsTo(QDateTime::currentDateTimeUtc());
    mIsRunning = false;
    return true;
}

bool stopWatch::reset()
{
    if (!mIsInitialised) {
        // Nothing to do, never started
        return false;
    }

    if (!mIsRunning) {
        // Not running, so reset elapsed time:
        mElapsedTime = 0;
        // And reset initialised flag:
        mIsInitialised = false;
        return true;
    }

    // Is running so reset effective start time - BUT THIS DOES NOT stop the
    // stopwatch:
    mEffectiveStartDateTime = QDateTime::currentDateTimeUtc();
    return true;
}

void stopWatch::adjustMilliSeconds(const qint64 adjustment)
{
    if (!mIsInitialised) {
        // We can initialise things in this case by setting the flag and falling
        // through into the is not running situation - with the elapsed time
        // being zero up to now we just have to add on the adjustment:
        mIsInitialised = true;
    }

    if (!mIsRunning) {
        // Not running so adjust stored elapsed time:
        mElapsedTime += adjustment;
    }

    // Is running so adjust effective start time - to increase the effective
    // elapsed time we must subtract the adjustment from the effect start time:
    mEffectiveStartDateTime = mEffectiveStartDateTime.addMSecs(-adjustment);
}

qint64 stopWatch::getElapsedMilliSeconds() const
{
    if (!mIsInitialised) {
        // Never started so no elapsed time:
        return 0;
    }

    if (!mIsRunning) {
        // Not running - so return elapsed time:
        return mElapsedTime;
    }

    // Is running so calculate elapsed time:
    return mEffectiveStartDateTime.msecsTo(QDateTime::currentDateTimeUtc());
}

QString stopWatch::getElapsedDayTimeString() const
{
    using namespace std::chrono_literals;

    if (!mIsInitialised) {
        return QStringLiteral("+:0:0:0:0:000");
    }

    qint64 elapsed = 0;
    if (mIsRunning) {
        elapsed = mEffectiveStartDateTime.msecsTo(QDateTime::currentDateTimeUtc());
    } else {
        elapsed = mElapsedTime;
    }

    bool isNegative = false;
    if (elapsed < 0) {
        isNegative = true;
        elapsed *= -1;
    }

    qint64 days = elapsed / std::chrono::milliseconds(24h).count();
    qint64 remainder = elapsed - (days * std::chrono::milliseconds(24h).count());
    quint8 hours = static_cast<quint8>(remainder / std::chrono::milliseconds(1h).count());
    remainder = remainder - (hours * std::chrono::milliseconds(1h).count());
    quint8 minutes = static_cast<quint8>(remainder / std::chrono::milliseconds(1min).count());
    remainder = remainder - (minutes * std::chrono::milliseconds(1min).count());
    quint8 seconds = static_cast<quint8>(remainder / std::chrono::milliseconds(1s).count());
    quint16 milliSeconds = static_cast<quint16>(remainder - (seconds * std::chrono::milliseconds(1s).count()));
    return QStringLiteral("%1:%2:%3:%4:%5:%6").arg((isNegative ? QLatin1String("-") : QLatin1String("+")), QString::number(days), QString::number(hours), QString::number(minutes), QString::number(seconds), QString::number(milliSeconds));
}

Host::Host(int port, const QString& hostname, const QString& login, const QString& pass, int id)
: mTelnet(this, hostname)
, mpConsole(nullptr)
, mpPackageManager(nullptr)
, mpModuleManager(nullptr)
, mLuaInterpreter(this, hostname, id)
, commandLineMinimumHeight(30)
, mAlertOnNewData(true)
, mAllowToSendCommand(true)
, mAutoClearCommandLineAfterSend(false)
, mHighlightHistory(true)
, mBlockScriptCompile(true)
, mBlockStopWatchCreation(true)
, mEchoLuaErrors(false)
, mBorderBottomHeight(0)
, mBorderLeftWidth(0)
, mBorderRightWidth(0)
, mBorderTopHeight(0)
, mCommandLineFont(QFont(QStringLiteral("Bitstream Vera Sans Mono"), 14, QFont::Normal))
, mCommandSeparator(QStringLiteral(";;"))
, mEnableGMCP(true)
, mEnableMSSP(true)
, mEnableMSP(true)
, mEnableMSDP(false)
, mServerMXPenabled(true)
, mMxpClient(this)
, mMxpProcessor(&mMxpClient)
, mFORCE_GA_OFF(false)
, mFORCE_NO_COMPRESSION(false)
, mFORCE_SAVE_ON_EXIT(true)
, mSslTsl(false)
, mSslIgnoreExpired(false)
, mSslIgnoreSelfSigned(false)
, mSslIgnoreAll(false)
, mUseProxy(false)
, mProxyPort(0)
, mIsGoingDown(false)
, mIsProfileLoadingSequence(false)
, mNoAntiAlias(false)
, mpEditorDialog(nullptr)
, mpMap(new TMap(this, hostname))
, mpMedia(new TMedia(this, hostname))
, mpNotePad(nullptr)
, mPrintCommand(true)
, mIsRemoteEchoingActive(false)
, mIsCurrentLogFileInHtmlFormat(false)
, mIsNextLogFileInHtmlFormat(false)
, mIsLoggingTimestamps(false)
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
, mEditorAutoComplete(true)
, mEditorTheme(QLatin1String("Mudlet"))
, mEditorThemeFile(QLatin1String("Mudlet.tmTheme"))
, mThemePreviewItemID(-1)
, mThemePreviewType(QString())
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
, mBlack(QColorConstants::Black)
, mLightBlack(QColorConstants::DarkGray)
, mRed(QColorConstants::DarkRed)
, mLightRed(QColorConstants::Red)
, mLightGreen(QColorConstants::Green)
, mGreen(QColorConstants::DarkGreen)
, mLightBlue(QColorConstants::Blue)
, mBlue(QColorConstants::DarkBlue)
, mLightYellow(QColorConstants::Yellow)
, mYellow(QColorConstants::DarkYellow)
, mLightCyan(QColorConstants::Cyan)
, mCyan(QColorConstants::DarkCyan)
, mLightMagenta(QColorConstants::Magenta)
, mMagenta(QColorConstants::DarkMagenta)
, mLightWhite(QColorConstants::White)
, mWhite(QColorConstants::LightGray)
, mFgColor(QColorConstants::LightGray)
, mBgColor(QColorConstants::Black)
, mCommandBgColor(QColorConstants::Black)
, mCommandFgColor(QColor(113, 113, 0))
, mBlack_2(QColorConstants::Black)
, mLightBlack_2(QColorConstants::DarkGray)
, mRed_2(QColorConstants::DarkRed)
, mLightRed_2(QColorConstants::Red)
, mLightGreen_2(QColorConstants::Green)
, mGreen_2(QColorConstants::DarkGreen)
, mLightBlue_2(QColorConstants::Blue)
, mBlue_2(QColorConstants::DarkBlue)
, mLightYellow_2(QColorConstants::Yellow)
, mYellow_2(QColorConstants::DarkYellow)
, mLightCyan_2(QColorConstants::Cyan)
, mCyan_2(QColorConstants::DarkCyan)
, mLightMagenta_2(QColorConstants::Magenta)
, mMagenta_2(QColorConstants::DarkMagenta)
, mLightWhite_2(QColorConstants::White)
, mWhite_2(QColorConstants::LightGray)
, mFgColor_2(QColorConstants::LightGray)
, mBgColor_2(QColorConstants::Black)
, mRoomBorderColor(QColorConstants::LightGray)
#else
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
, mRoomBorderColor(Qt::lightGray)
#endif
, mMapStrongHighlight(false)
, mLogStatus(false)
, mEnableSpellCheck(true)
, mDiscordDisableServerSide(true)
, mDiscordAccessFlags(DiscordLuaAccessEnabled | DiscordSetSubMask)
, mLineSize(10.0)
, mRoomSize(0.5)
, mMapInfoContributors(QSet<QString>{"Short"})
, mBubbleMode(false)
, mShowRoomID(false)
, mShowPanel(true)
, mServerGUI_Package_version(QLatin1String("-1"))
, mServerGUI_Package_name(QLatin1String("nothing"))
, mAcceptServerGUI(true)
, mAcceptServerMedia(true)
, mCommandLineFgColor(Qt::darkGray)
, mCommandLineBgColor(Qt::black)
, mMapperUseAntiAlias(true)
, mMapperShowRoomBorders(true)
, mFORCE_MXP_NEGOTIATION_OFF(false)
, mFORCE_CHARSET_NEGOTIATION_OFF(false)
, mpDockableMapWidget()
, mEnableTextAnalyzer(false)
, mTimerDebugOutputSuppressionInterval(QTime())
, mSearchOptions(dlgTriggerEditor::SearchOption::SearchOptionNone)
, mpDlgIRC(nullptr)
, mpDlgProfilePreferences(nullptr)
, mDisplayFont(QFont(QStringLiteral("Bitstream Vera Sans Mono"), 14, QFont::Normal))
, mLuaInterface(nullptr)
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
, mSpellDic(QStringLiteral("en_US"))
// DISABLED: - Prevent "None" option for user dictionary - changed to true and not changed anywhere else
, mEnableUserDictionary(true)
, mUseSharedDictionary(false)
, mPlayerRoomStyle(0)
, mPlayerRoomOuterColor(Qt::red)
, mPlayerRoomInnerColor(Qt::white)
, mPlayerRoomOuterDiameterPercentage(120)
, mPlayerRoomInnerDiameterPercentage(70)
, mDebugShowAllProblemCodepoints(false)
, mCompactInputLine(false)
{
    TDebug::addHost(this);

    // mLogStatus = mudlet::self()->mAutolog;
    mLuaInterface.reset(new LuaInterface(this));

    // Copy across the details needed for the "color_table":
    mLuaInterpreter.updateAnsi16ColorsInTable();
    mLuaInterpreter.updateExtendedAnsiColorsInTable();

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

    loadSecuredPassword();

    if (mudlet::scmIsPublicTestVersion) {
        thankForUsingPTB();
    }

    if (mudlet::self()->firstLaunch) {
        QTimer::singleShot(0, this, [this]() {
            mpConsole->mpCommandLine->setPlaceholderText(tr("Text to send to the game"));
        });
    }

    connect(&mTelnet, &cTelnet::signal_disconnected, this, [this](){ purgeTimer.start(1min); });
    connect(&mTelnet, &cTelnet::signal_connected, this, [this](){ purgeTimer.stop(); });
    connect(&purgeTimer, &QTimer::timeout, this, &Host::slot_purgeTemps);

    // enable by default in case of offline connection; if the profile connects - timer will be disabled
    purgeTimer.start(1min);
}

Host::~Host()
{
    if (mpDockableMapWidget) {
        mpDockableMapWidget->deleteLater();
    }
    mIsGoingDown = true;
    mIsClosingDown = true;
    mErrorLogStream.flush();
    mErrorLogFile.close();
    TDebug::removeHost(this);
}

void Host::loadPackageInfo()
{
    QStringList packages = mInstalledPackages;
    for (int i = 0; i < packages.size(); i++) {
        QString packagePath{mudlet::self()->getMudletPath(mudlet::profilePackagePath, getName(), packages.at(i))};
        QDir dir(packagePath);
        if (dir.exists(QStringLiteral("config.lua"))) {
            getPackageConfig(dir.absoluteFilePath(QStringLiteral("config.lua")));
        }
    }
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
            QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd#HH-mm-ss");
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
    for (auto otherHost : mudlet::self()->getHostManager()) {
        if (otherHost == this || !otherHost->mpConsole) {
            continue;
        }
        QMap<QString, int>& modulePri = otherHost->mModulePriorities;
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

std::pair<bool, QString> Host::changeModuleSync(const QString& moduleName, const QLatin1String& value)
{
    if (moduleName.isEmpty()) {
        return {false, QStringLiteral("module name cannot be an empty string")};
    }

    if (mInstalledModules.contains(moduleName)) {
        QStringList moduleStringList = mInstalledModules[moduleName];
        QFileInfo moduleFile = moduleStringList[0];
        QStringList accepted_suffix;
        accepted_suffix << "xml" << "trigger";
        if (!accepted_suffix.contains(moduleFile.suffix().trimmed(), Qt::CaseInsensitive)) {
            return {false, QStringLiteral("module has to be a .xml file")};
        }
        moduleStringList[1] = value;
        mInstalledModules[moduleName] = moduleStringList;
        return {true, QString()};
    }
    return {false, QStringLiteral("module name '%1' not found").arg(moduleName)};
}

std::pair<bool, QString> Host::getModuleSync(const QString& moduleName)
{
    if (moduleName.isEmpty()) {
        return {false, QStringLiteral("module name cannot be an empty string")};
    }

    if (mInstalledModules.contains(moduleName)) {
        QStringList moduleStringList = mInstalledModules[moduleName];
        return {true, moduleStringList[1]};
    }
    return {false, QStringLiteral("module name '%1' not found").arg(moduleName)};
}

void Host::resetProfile_phase1()
{
    mAliasUnit.stopAllTriggers();
    mTriggerUnit.stopAllTriggers();
    mTimerUnit.stopAllTriggers();
    mKeyUnit.stopAllTriggers();
    mResetProfile = true;

    QTimer::singleShot(0, this, [this]() {
        resetProfile_phase2();
    });
}

void Host::resetProfile_phase2()
{
    getAliasUnit()->removeAllTempAliases();
    getTimerUnit()->removeAllTempTimers();
    getTriggerUnit()->removeAllTempTriggers();
    getKeyUnit()->removeAllTempKeys();
    removeAllNonPersistentStopWatches();

    mAliasUnit.doCleanup();
    mTimerUnit.doCleanup();
    mTriggerUnit.doCleanup();
    mKeyUnit.doCleanup();
    mpConsole->resetMainConsole();
    mEventHandlerMap.clear();
    mEventMap.clear();
    mLuaInterpreter.initLuaGlobals();
    mLuaInterpreter.loadGlobal();
    mBlockScriptCompile = false;

    mAliasUnit.reenableAllTriggers();
    mTimerUnit.reenableAllTriggers();
    mTriggerUnit.reenableAllTriggers();
    mKeyUnit.reenableAllTriggers();

    getTimerUnit()->compileAll();
    getTriggerUnit()->compileAll();
    getAliasUnit()->compileAll();
    getActionUnit()->compileAll();
    getKeyUnit()->compileAll();
    getScriptUnit()->compileAll();

    mResetProfile = false;

    // Have to recopy the values into the Lua "color_table"
    mLuaInterpreter.updateAnsi16ColorsInTable();
    mLuaInterpreter.updateExtendedAnsiColorsInTable();

    TEvent event {};
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

    QString filename_xml;
    if (saveName.isEmpty()) {
        filename_xml = QStringLiteral("%1/%2.xml").arg(directory_xml, QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd#HH-mm-ss")));
    } else {
        filename_xml = QStringLiteral("%1/%2.xml").arg(directory_xml, saveName);
    }


    if (mIsProfileLoadingSequence) {
        //If we're inside of profile loading sequence modules might not be loaded yet, thus we can accidetnally clear their contents
        return std::make_tuple(false, filename_xml, QStringLiteral("profile loading is in progress"));
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

// error and debug consoles inherit font of the main console
void Host::updateConsolesFont()
{
    if (mpConsole) {
        mpConsole->refreshView();
    }

    if (mpEditorDialog && mpEditorDialog->mpErrorConsole) {
        mpEditorDialog->mpErrorConsole->setFont(mDisplayFont.family());
        mpEditorDialog->mpErrorConsole->setFontSize(mDisplayFont.pointSize());
    }
    if (mudlet::self()->mpDebugArea) {
        mudlet::self()->mpDebugConsole->setFont(mDisplayFont.family());
        mudlet::self()->mpDebugConsole->setFontSize(mDisplayFont.pointSize());
    }
}

// a little message to make the player feel special for helping us find bugs
void Host::thankForUsingPTB()
{
    const QStringList happyIcons {"😀", "😃", "😄", "😁", "🙂", "🙃", "🤩", "🎉", "🚀", "🤟", "✌️", "👊"};
    const auto randomIcon = happyIcons.at(QRandomGenerator::global()->bounded(happyIcons.size()));
    postMessage(tr(R"([  OK  ]  - %1 Thanks a lot for using the Public Test Build!)", "%1 will be a random happy emoji").arg(randomIcon));
    postMessage(tr(R"([  OK  ]  - %1 Help us make Mudlet better by reporting any problems.)", "%1 will be a random happy emoji").arg(randomIcon));
}

void Host::setMediaLocationGMCP(const QString& mediaUrl)
{
    QUrl url = QUrl(mediaUrl);

    if (!url.isValid()) {
        return;
    }

    mMediaLocationGMCP = mediaUrl;
}

QString Host::getMediaLocationGMCP() const
{
    return mMediaLocationGMCP;
}

void Host::setMediaLocationMSP(const QString& mediaUrl)
{
    QUrl url = QUrl(mediaUrl);

    if (!url.isValid()) {
        return;
    }

    mMediaLocationMSP = mediaUrl;
}

QString Host::getMediaLocationMSP() const
{
    return mMediaLocationMSP;
}

std::pair<bool, QString> Host::setDisplayFont(const QFont& font)
{
    const QFontMetrics metrics(font);
    if (metrics.averageCharWidth() == 0) {
        return {false, QStringLiteral("specified font is invalid (its letters have 0 width)")};
    }

    mDisplayFont = font;
    updateConsolesFont();
    return {true, QString()};
}

std::pair<bool, QString> Host::setDisplayFont(const QString& fontName)
{
    const auto result = setDisplayFont(QFont(fontName));
    updateConsolesFont();
    return result;
}

void Host::setDisplayFontFromString(const QString& fontData)
{
    mDisplayFont.fromString(fontData);
    updateConsolesFont();
}

void Host::setDisplayFontSize(int size)
{
    mDisplayFont.setPointSize(size);
    updateConsolesFont();
}

// Now returns the total weight of the path
unsigned int Host::assemblePath()
{
    unsigned int totalWeight = 0;
    QStringList pathList;
    for (int i : qAsConst(mpMap->mPathList)) {
        QString n = QString::number(i);
        pathList.append(n);
    }
    QStringList directionList = mpMap->mDirList;
    QStringList weightList;
    for (int stepWeight : qAsConst(mpMap->mWeightList)) {
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

bool Host::checkForMappingScript()
{
    // the mapper script reminder is only shown once
    // because it is too difficult and error prone (->proper script sequence)
    // to disable this message
    bool ret = (mLuaInterpreter.check_for_mappingscript() || mHaveMapperScript);
    mHaveMapperScript = true;
    return ret;
}

void Host::check_for_mappingscript()
{
    if (!checkForMappingScript()) {
        QUiLoader loader;

        QFile file(":/ui/lacking_mapper_script.ui");
        file.open(QFile::ReadOnly);

        auto dialog = dynamic_cast<QDialog*>(loader.load(&file, mudlet::self()));
        file.close();
        if (!dialog) {
            // could not load / not a QDialog
            return;
        }

        connect(dialog, &QDialog::accepted, mudlet::self(), &mudlet::slot_open_mappingscripts_page);

        dialog->show();
        dialog->raise();
        dialog->activateWindow();
    }
}

bool Host::checkForCustomSpeedwalk()
{
    bool ret = mLuaInterpreter.check_for_custom_speedwalk();
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

void Host::startSpeedWalk(int sourceRoom, int targetRoom)
{
    QString sourceName = QStringLiteral("speedWalkFrom");
    mLuaInterpreter.set_lua_integer(sourceName, sourceRoom);
    QString targetName = QStringLiteral("speedWalkTo");
    mLuaInterpreter.set_lua_integer(targetName, targetRoom);
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
    if (mSearchEngineData.contains(mSearchEngineName)) {
        return qMakePair(mSearchEngineName, mSearchEngineData.value(mSearchEngineName));
    } else {
        return qMakePair(QStringLiteral("Google"), mSearchEngineData.value(QStringLiteral("Google")));
    }
}

// cmd is UTF-16BE encoded here, but will be transcoded to Server's one by
// cTelnet::sendData(...) call:
void Host::send(QString cmd, bool wantPrint, bool dontExpandAliases)
{
    if (wantPrint && (!mIsRemoteEchoingActive) && mPrintCommand) {
        if (!cmd.isEmpty() || !mUSE_IRE_DRIVER_BUGFIX || mUSE_FORCE_LF_AFTER_PROMPT) {
            // used to print the terminal <LF> that terminates a telnet command
            // this is important to get the cursor position right
            mpConsole->printCommand(cmd);
        }
        //If 3D Mapper is active mpConsole->update(); seems to be superfluous and even cause problems in MacOS
#if defined(INCLUDE_3DMAPPER)
        if (!mpMap->mpMapper || !mpMap->mpMapper->glWidget) {
#else
        if (!mpMap->mpMapper) {
#endif
            mpConsole->update();
        }
    }
    QStringList commandList;
    if (!mCommandSeparator.isEmpty()) {
#if (QT_VERSION) >= (QT_VERSION_CHECK(5, 14, 0))
        commandList = cmd.split(QString(mCommandSeparator), Qt::SkipEmptyParts);
#else
        commandList = cmd.split(QString(mCommandSeparator), QString::SkipEmptyParts);
#endif
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

QPair<int, QString> Host::createStopWatch(const QString& name)
{
    if (mResetProfile || mBlockStopWatchCreation) {
        // Don't create stopwatches when test loading scripts or during a profile reset:
        return qMakePair(0, QStringLiteral("unable to create a stopwatch at this time"));
    }

    if (!mStopWatchMap.isEmpty() && !name.isEmpty()) {
        QMapIterator<int, stopWatch*> itStopWatch(mStopWatchMap);
        while (itStopWatch.hasNext()) {
            itStopWatch.next();
            if (itStopWatch.value()->name() == name) {
                return qMakePair(0, QStringLiteral("stopwatch with id %1 called '%2' already exists").arg(QString::number(itStopWatch.key()), name));
            }
        }
    }
    int newWatchId = 1;
    while (mStopWatchMap.contains(newWatchId)) {
        ++newWatchId;
    }

    // It is hard to imagine a situation in which this will fail - so we won't
    // bother coding for it:
    auto pStopWatch = new stopWatch();
    Q_ASSERT_X(pStopWatch, "Host::createStopWatch", "out of memory, unable to create new stopwatch");
    if (!name.isEmpty()) {
        pStopWatch->setName(name);
    }

    mStopWatchMap.insert(newWatchId, pStopWatch);
    return qMakePair(newWatchId, QString());
}

bool Host::destroyStopWatch(const int id)
{
    auto pStopWatch = mStopWatchMap.take(id);
    if (!pStopWatch) {
        return false;
    }

    delete pStopWatch;
    return true;
}

bool Host::adjustStopWatch(const int id, const qint64 milliSeconds)
{
    auto pStopWatch = mStopWatchMap.value(id);
    if (pStopWatch) {
        pStopWatch->adjustMilliSeconds(milliSeconds);
        return true;
    }

    return false;
}

// Find the first (lowest ID) stopwatch with the given name and return its id,
// returns 0 (not a valid id) if the name is not found. It WILL return the first
// unnamed one if a blank string is given:
int Host::findStopWatchId(const QString& name) const
{
    // Scan through existing names, in ascending id order
    QList<int> stopWatchIdList = mStopWatchMap.keys();
    int total = stopWatchIdList.size();
    if (total > 1) {
        std::sort(stopWatchIdList.begin(), stopWatchIdList.end());
    }
    for (int index = 0, total = stopWatchIdList.size(); index < total; ++index) {
        auto currentId = stopWatchIdList.at(index);
        auto pCurrentStopWatch = mStopWatchMap.value(currentId);
        if (pCurrentStopWatch->name() == name) {
            return currentId;
        }
    }
    return 0;
}

QPair<bool, double> Host::getStopWatchTime(const int id) const
{
    auto pStopWatch = mStopWatchMap.value(id);
    if (pStopWatch) {
        return qMakePair(true, pStopWatch->getElapsedMilliSeconds() / 1000.0);
    } else {
        return qMakePair(false, 0.0);
    }
}

QPair<bool, QString> Host::getBrokenDownStopWatchTime(const int id) const
{
    auto pStopWatch = mStopWatchMap.value(id);
    if (pStopWatch) {
        return qMakePair(true, pStopWatch->getElapsedDayTimeString());
    } else {
        return qMakePair(false, QStringLiteral("stopwatch with id %1 not found").arg(id));
    }
}

QPair<bool, QString> Host::startStopWatch(const QString& name)
{
    auto watchId = findStopWatchId(name);
    if (!watchId) {
        if (name.isEmpty()) {
            return qMakePair(false, QLatin1String("no unnamed stopwatches found"));
        } else {
            return qMakePair(false, QStringLiteral("stopwatch with name '%1' not found").arg(name));
        }
    }

    auto pStopWatch = mStopWatchMap.value(watchId);
    if (Q_LIKELY(pStopWatch)) {
        if (pStopWatch->start()) {
            return qMakePair(true, QString());
        }

        return qMakePair(false, QStringLiteral("stopwatch with name '%1' (id:%2) was already running").arg(name, QString::number(watchId)));
    }

    // This should, indeed, be:
    Q_UNREACHABLE();
    return qMakePair(false, QStringLiteral("stopwatch with name '%1' (id:%2) not found").arg(name, QString::number(watchId)));
}

QPair<bool, QString> Host::startStopWatch(int id)
{
    auto pStopWatch = mStopWatchMap.value(id);
    if (pStopWatch) {
        if (pStopWatch->start()) {
            return qMakePair(true, QString());
        }

        return qMakePair(false, QStringLiteral("stopwatch with id %1 was already running").arg(id));
    }

    return qMakePair(false, QStringLiteral("stopwatch with id %1 not found").arg(id));
}

QPair<bool, QString> Host::stopStopWatch(const QString& name)
{
    auto watchId = findStopWatchId(name);
    if (!watchId) {
        if (name.isEmpty()) {
            return qMakePair(false, QLatin1String("no unnamed stopwatches found"));
        } else {
            return qMakePair(false, QStringLiteral("stopwatch with name '%1' not found").arg(name));
        }
    }

    auto pStopWatch = mStopWatchMap.value(watchId);
    if (Q_LIKELY(pStopWatch)) {
        if (pStopWatch->stop()) {
            return qMakePair(true, QString());
        }

        return qMakePair(false, QStringLiteral("stopwatch with name '%1' (id:%2) was already stopped").arg(name, QString::number(watchId)));
    }

    // This should, indeed, be:
    Q_UNREACHABLE();
    return qMakePair(false, QStringLiteral("stopwatch with name '%1' (id:%2) not found").arg(name, QString::number(watchId)));
}

QPair<bool, QString> Host::stopStopWatch(const int id)
{
    auto pStopWatch = mStopWatchMap.value(id);
    if (pStopWatch) {
        if (pStopWatch->stop()) {
            return qMakePair(true, QString());
        }

        return qMakePair(false, QStringLiteral("stopwatch with id %1 was already stopped").arg(id));
    }

    return qMakePair(false, QStringLiteral("stopwatch with id %1 not found").arg(id));
}

QPair<bool, QString> Host::resetStopWatch(const QString& name)
{
    auto watchId = findStopWatchId(name);
    if (!watchId) {
        if (name.isEmpty()) {
            return qMakePair(false, QLatin1String("no unnamed stopwatches found"));
        } else {
            return qMakePair(false, QStringLiteral("stopwatch with name '%1' not found").arg(name));
        }
    }

    auto pStopWatch = mStopWatchMap.value(watchId);
    if (Q_LIKELY(pStopWatch)) {
        if (pStopWatch->reset()) {
            return qMakePair(true, QString());
        }

        if (name.isEmpty()) {
            return qMakePair(false, QStringLiteral("the first unnamed stopwatch (id:%1) was already reset").arg(watchId));
        } else {
            return qMakePair(false, QStringLiteral("stopwatch with name '%1' (id:%2) was already reset").arg(name, QString::number(watchId)));
        }
    }

    // This should, indeed, be:
    Q_UNREACHABLE();
    return qMakePair(false, QStringLiteral("stopwatch with name '%1' (id:%2) not found").arg(name, QString::number(watchId)));
}

QPair<bool, QString> Host::resetStopWatch(const int id)
{
    auto pStopWatch = mStopWatchMap.value(id);
    if (pStopWatch) {
        if (pStopWatch->reset()) {
            return qMakePair(true, QString());
        }

        return qMakePair(false, QStringLiteral("stopwatch with id %1 was already reset").arg(id));
    }

    return qMakePair(false, QStringLiteral("stopwatch with id %1 not found").arg(id));
}

// Used when emulating past behavior for startStopWatch - there is only one
// which takes a numeric argument as that is what old scripts will be using and
// not a text name:
QPair<bool, QString> Host::resetAndRestartStopWatch(const int id)
{
    auto pStopWatch = mStopWatchMap.value(id);
    if (pStopWatch) {
        pStopWatch->stop();
        pStopWatch->reset();
        pStopWatch->start();
        return qMakePair(true, QString());
    }

    return qMakePair(false, QStringLiteral("stopwatch with id %1 not found").arg(id));
}

bool Host::makeStopWatchPersistent(const int id, const bool state)
{
    auto pStopWatch = mStopWatchMap.value(id);
    if (pStopWatch) {
        pStopWatch->setPersistent(state);
        return true;
    }

    return false;
}

QPair<bool, QString> Host::setStopWatchName(const int id, const QString& newName)
{
    stopWatch* pStopWatch = nullptr;
    if (!newName.isEmpty()) {
        // Scan through existing names
        QMutableMapIterator<int, stopWatch*> itStopWatch(mStopWatchMap);
        while (itStopWatch.hasNext()) {
            itStopWatch.next();
            if (itStopWatch.value()->name() == newName) {
                if (itStopWatch.key() != id) {
                    return qMakePair(false, QStringLiteral("the name '%1' is already in use for another stopwatch (id:%2)").arg(newName, QString::number(itStopWatch.key())));
                } else {
                    // Trivial case - the stopwatch is already called by the NEW name:
                    return qMakePair(true, QString());
                }
            }
        }
    }

    pStopWatch = mStopWatchMap.value(id);
    if (pStopWatch) {
        // Okay found the one we want:
        pStopWatch->setName(newName);
        return qMakePair(true, QString());
    }

    return qMakePair(false, QStringLiteral("stopwatch with id %1 not found").arg(id));
}

QPair<bool, QString> Host::setStopWatchName(const QString& currentName, const QString& newName)
{
    stopWatch* pStopWatch = nullptr;
    // Scan through existing names, in ascending id order
    QList<int> stopWatchIdList = mStopWatchMap.keys();
    int total = stopWatchIdList.size();
    if (total > 1) {
        std::sort(stopWatchIdList.begin(), stopWatchIdList.end());
    }
    int id = 0;
    // Although it would be quicker code to return immediately if we detect the
    // newName is in use the run-time error about it being used will mask a
    // failure to find the current one (perhaps because it has already been set
    // to the new value) - so don't moan about that until we have (or have not)
    // found the current name:
    bool isAlreadyUsed = false;
    int alreadyUsedId = 0;
    // we are looking BOTH for the current name and checking that any other
    // ones WITH names do not match the new name:
    for (int index = 0; index < total; ++index) {
        auto currentId = stopWatchIdList.at(index);
        auto pCurrentStopWatch = mStopWatchMap.value(currentId);
        // This will also pick up the FIRST (lowest id) currently unnamed
        // stopwatch:
        if (!id && pCurrentStopWatch->name() == currentName) {
            pStopWatch = pCurrentStopWatch;
            id = currentId;
        }
        // As zero is never used as an Id it okay to use it here before it
        // is set to the value of the wanted entry:
        if (!newName.isEmpty() && currentId != id && pCurrentStopWatch->name() == newName) {
            isAlreadyUsed = true;
            alreadyUsedId = currentId;
        }
    }

    // if pStopWatch is nullptr then the currentName was not found:
    if (!pStopWatch) {
        if (currentName.isEmpty()) {
            return qMakePair(false, QLatin1String("no unnamed stopwatches found"));
        } else {
            return qMakePair(false, QStringLiteral("stopwatch with name '%1' not found").arg(currentName));
        }
    }

    if (isAlreadyUsed) {
        return qMakePair(false, QStringLiteral("the name '%1' is already in use for another stopwatch (id:%2)").arg(newName, QString::number(alreadyUsedId)));
    }

    if (currentName == newName) {
        // Trivial case that always succeeds
        return qMakePair(true, QString());
    }

    pStopWatch->setName(newName);
    return qMakePair(true, QString());
}

QList<int> Host::getStopWatchIds() const
{
    return mStopWatchMap.keys();
}

void Host::incomingStreamProcessor(const QString& data, int line)
{
    mTriggerUnit.processDataStream(data, line);

    mAliasUnit.doCleanup();
    mTimerUnit.doCleanup();
    mTriggerUnit.doCleanup();
    mKeyUnit.doCleanup();
}

// When Mudlet is running in online mode, deleted temp* objects are cleaned up in bulk
// on every new line. When in offline mode, new lines don't come - so they are
// cleaned up in bulk periodically.
void Host::slot_purgeTemps()
{
    mAliasUnit.doCleanup();
    mTimerUnit.doCleanup();
    mTriggerUnit.doCleanup();
    mKeyUnit.doCleanup();
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

// If a handler matches the event, the Lua stack will be cleared after this function
void Host::raiseEvent(const TEvent& pE)
{
    if (pE.mArgumentList.isEmpty()) {
        return;
    }

    static QString star = QStringLiteral("*");

    if (mEventHandlerMap.contains(pE.mArgumentList.at(0))) {
        QList<TScript*> scriptList = mEventHandlerMap.value(pE.mArgumentList.at(0));
        for (auto& script : scriptList) {
            script->callEventHandler(pE);
        }
    }
    if (mEventHandlerMap.contains(star)) {
        QList<TScript*> scriptList = mEventHandlerMap.value(star);
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
    if (mAnonymousEventHandlerFunctions.contains(star)) {
        QStringList functionsList = mAnonymousEventHandlerFunctions.value(star);
        for (int i = 0, total = functionsList.size(); i < total; ++i) {
            mLuaInterpreter.callEventHandler(functionsList.at(i), pE);
        }
    }

    // After the event has been raised but before 'event' goes out of scope,
    // we need to safely dereference the members of 'event' that point to
    // values in the Lua registry
    mLuaInterpreter.freeAllInLuaRegistry(pE);
}

void Host::postIrcMessage(const QString& a, const QString& b, const QString& c)
{
    TEvent event {};
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
    mIsClosingDown = true;
}

bool Host::isClosingDown()
{
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
            readPackageConfig(_dir.absoluteFilePath(QStringLiteral("config.lua")), packageName, module > 0);
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
            QString newpath(QStringLiteral("%1/%2").arg(_home, packageName));
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
    TEvent genericInstallEvent {};
    genericInstallEvent.mArgumentList.append(QLatin1String("sysInstall"));
    genericInstallEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    genericInstallEvent.mArgumentList.append(packageName);
    genericInstallEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    raiseEvent(genericInstallEvent);

    TEvent detailedInstallEvent {};
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

void Host::removePackageInfo(const QString &packageName, const bool isModule) {
    if (isModule) {
        mModuleInfo.remove(packageName);
    } else {
        mPackageInfo.remove(packageName);
    }
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
    //module == 2 seems to be only used for reloading/syncing, which doesn't work for mpackages anyway
    //No need to remove package info as it can cause the info to be lost
    if (module != 2) {
        removePackageInfo(packageName, module > 0);
    }
    // raise 2 events - a generic one and a more detailed one to serve both
    // a simple need ("I just want the uninstall event") and a more specific need
    // ("I specifically need to know when the module was uninstalled via Lua")
    TEvent genericUninstallEvent {};
    genericUninstallEvent.mArgumentList.append(QLatin1String("sysUninstall"));
    genericUninstallEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    genericUninstallEvent.mArgumentList.append(packageName);
    genericUninstallEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    raiseEvent(genericUninstallEvent);

    TEvent detailedUninstallEvent {};
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

void Host::readPackageConfig(const QString& luaConfig, QString& packageName, bool isModule)
{
    QString newName = getPackageConfig(luaConfig, isModule);
    if (!newName.isEmpty()){
        packageName = newName;
    }
}

QString Host::getPackageConfig(const QString& luaConfig, bool isModule)
{
    QString packageName;
    // We don't use luaL_loadfile here because that breaks on Windows as it won't work if there are accented characters in the path or file name -
    // QFile which can work with whatever local8Bit encoding is used for file names - the luaL_loadfile(...) uses std::iostream which doesn't...
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
        lua_getglobal(L, "mpackage");
        if (lua_isstring(L, -1)) {
            packageName = QString(lua_tostring(L, -1));
        }
        lua_pop(L, -1);
        if (!packageName.isEmpty()) {
            //get rid of lua version
            lua_getglobal(L, "_G");
            lua_pushnil(L);
            lua_setfield(L, -2, "_VERSION");
            QMap<QString, QString> packageInfo;
            lua_pushnil(L);
            while (lua_next(L, -2) != 0) {
                if (lua_isstring(L, -1) && lua_isstring(L, -2)) {
                    packageInfo[lua_tostring(L, -2)] = lua_tostring(L, -1);
                }
                lua_pop(L, 1);
            }

            if (isModule) {
                mModuleInfo[packageName] = packageInfo;
            } else {
                mPackageInfo[packageName] = packageInfo;
            }
        }
        lua_close(L);
        return packageName;
    }

    // error
    std::string e = lua_tostring(L, -1);
    if (e.empty()) {
        e = "no error message available from Lua";
    }

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
        TDebug(QColor(Qt::white), QColor(Qt::red)) << "LUA: " << reason.c_str() << " in " << luaConfig << " ERROR:" << e.c_str() << "\n" >> 0;
    }

    lua_pop(L, -1);
    lua_close(L);
    return QString();
}

// Derived from the one in dlgConnectionProfile class - but it does not need a
// host name argument...
QPair<bool, QString> Host::writeProfileData(const QString& item, const QString& what)
{
    QFile file(mudlet::getMudletPath(mudlet::profileDataItemPath, getName(), item));
    if (file.open(QIODevice::WriteOnly | QIODevice::Unbuffered)) {
        QDataStream ofs(&file);
        if (mudlet::scmRunTimeQtVersion >= QVersionNumber(5, 13, 0)) {
            ofs.setVersion(mudlet::scmQDataStreamFormat_5_12);
        }
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
        if (mudlet::scmRunTimeQtVersion >= QVersionNumber(5, 13, 0)) {
            ifs.setVersion(mudlet::scmQDataStreamFormat_5_12);
        }
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
    for (const auto& package : qAsConst(mInstalledPackages)) {
        installPackageFonts(package);
    }
}

void Host::setWideAmbiguousEAsianGlyphs(const Qt::CheckState state)
{
    bool localState = false;
    bool needToEmit = false;
    const QByteArray encoding(mTelnet.getEncoding());

    if (state == Qt::PartiallyChecked) {
        // Set things automatically
        mAutoAmbigousWidthGlyphsSetting = true;

        if (encoding == "GBK"
            || encoding == "GB18030"
            || encoding == "BIG5"
            || encoding == "BIG5-HKSCS") {

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
        }

    }

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
            return QColor(); // No-op
        }
    }
    // clang-format on
}

// handles out of band (OOB) GMCP/MSDP data for Discord - called whenever GMCP
// Telnet sub-option comes in and starts with "External.Discord.(Status|Info)"
void Host::processDiscordGMCP(const QString& packageMessage, const QString& data)
{
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
    if (inviteUrl != QJsonValue::Undefined && !inviteUrl.toString().isEmpty() && inviteUrl.toString() != QStringLiteral("0")) {
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
            qDebug() << "Game using a custom Discord server. Invite URL:" << inviteUrl.toString();
        } else if (hasApplicationId) {
            qDebug() << "Game using Mudlet's Discord server. Invite URL:" << inviteUrl.toString();
        } else {
            qDebug() << "Discord invite URL: " << inviteUrl.toString();
        }
    } else {
        if (hasCustomAppID) {
            qDebug() << "Game is using custom server Discord application ID";
        } else if (hasApplicationId) {
            qDebug() << "Game is using Mudlet's Discord application ID";
        }
    }
}

void Host::processGMCPDiscordStatus(const QJsonObject& discordInfo)
{
    if (mDiscordDisableServerSide) {
        return;
    }

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
            pMudlet->mDiscord.setLargeImage(this, largeImage.toString());
        }
    }

    auto largeImageText = discordInfo.value(QStringLiteral("largeimagetext"));
    if (largeImageText != QJsonValue::Undefined) {
        pMudlet->mDiscord.setLargeImageText(this, largeImageText.toString());
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
    mDiscordApplicationID = s;
    writeProfileData(QStringLiteral("discordApplicationId"), s);
}

const QString& Host::getDiscordApplicationID()
{
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
    bool isChanged = false;
    if (!newDict.isEmpty() && mSpellDic != newDict) {
        mSpellDic = newDict;
        isChanged = true;
    }
    if (isChanged && mpConsole) {
        mpConsole->setSystemSpellDictionary(newDict);
    }
}

// When called from dlgProfilePreferences the second flag will only be changed
// if necessary:
// DISABLED: - Prevent "None" option for user dictionary - modified to prevent original useDictionary argument from being false:
void Host::setUserDictionaryOptions(const bool _useDictionary, const bool useShared)
{
    Q_UNUSED(_useDictionary);
    bool useDictionary = true;
    bool dictionaryChanged {};
    // Copy the value while we have the lock:
    bool isSpellCheckingEnabled = mEnableSpellCheck;
    if (mEnableUserDictionary != useDictionary) {
        mEnableUserDictionary = useDictionary;
        dictionaryChanged = true;
    }

    if (mUseSharedDictionary != useShared) {
        mUseSharedDictionary = useShared;
        dictionaryChanged = true;
    }

    if (!mpConsole) {
        return;
    }

    if (dictionaryChanged) {
        // This will propogate the changes in the two flags to the main
        // TConsole's copies of them - although setProfileSpellDictionary() is
        // also called in the main TConsole constructor:
        mpConsole->setProfileSpellDictionary();
    }

    // This also needs to handle the spell checking against the system/mudlet
    // bundled dictionary being switched on or off. Given that if it has
    // been disabled the spell checking code won't run we need to clear any
    // highlights in the TCommandLine instance that may have been present when
    // spell checking is turned on or off:
    if (isSpellCheckingEnabled) {
        // Now enabled - so recheck the whole command line with whichever
        // dictionaries are active:
        mpConsole->mpCommandLine->recheckWholeLine();
    } else {
        // Or it is now disabled so clear any spelling marks:
        mpConsole->mpCommandLine->clearMarksOnWholeLine();
    }
}

// This does not take care of any QMaps or other containers that the mudlet
// and HostManager classes have that use the name of this profile as a key,
// however it should ensure that other classes get updated:
void Host::setName(const QString& newName)
{
    if (mHostName == newName) {
        return;
    }

    TDebug::changeHostName(this, newName);
    int currentPlayerRoom = 0;
    if (mpMap) {
        currentPlayerRoom = mpMap->mRoomIdHash.take(mHostName);
    }

    mHostName = newName;

    mTelnet.mProfileName = newName;
    if (mpMap) {
        mpMap->mProfileName = newName;
        if (currentPlayerRoom) {
            mpMap->mRoomIdHash.insert(newName, currentPlayerRoom);
        }
    }

    if (mpConsole) {
        // If skipped they will be taken care of in the TMainConsole constructor:
        mpConsole->setProperty("HostName", newName);
        mpConsole->setProfileName(newName);
    }
    mTimerUnit.changeHostName(newName);
}

void Host::removeAllNonPersistentStopWatches()
{
    QMutableMapIterator<int, stopWatch*> itStopWatch(mStopWatchMap);
    while (itStopWatch.hasNext()) {
        itStopWatch.next();
        auto pStopWatch = itStopWatch.value();
        if (!pStopWatch || pStopWatch->persistent()) {
            continue;
        }

        itStopWatch.remove();
        delete pStopWatch;
    }
}

void Host::updateProxySettings(QNetworkAccessManager* manager)
{
    if (mUseProxy && !mProxyAddress.isEmpty() && mProxyPort != 0) {
        auto& proxy = getConnectionProxy();
        manager->setProxy(*proxy);
    } else {
        manager->setProxy(QNetworkProxy::DefaultProxy);
    }
}

std::unique_ptr<QNetworkProxy>& Host::getConnectionProxy()
{
    if (!mpDownloaderProxy) {
        mpDownloaderProxy = std::make_unique<QNetworkProxy>(QNetworkProxy::Socks5Proxy);
    }
    auto& proxy = mpDownloaderProxy;
    proxy->setHostName(mProxyAddress);
    proxy->setPort(mProxyPort);
    if (!mProxyUsername.isEmpty()) {
        proxy->setUser(mProxyUsername);
    }
    if (!mProxyPassword.isEmpty()) {
        proxy->setPassword(mProxyPassword);
    }

    return mpDownloaderProxy;
}

void Host::setDisplayFontSpacing(const qreal spacing)
{
    mDisplayFont.setLetterSpacing(QFont::AbsoluteSpacing, spacing);
}

void Host::setDisplayFontStyle(QFont::StyleStrategy s)
{
    mDisplayFont.setStyleStrategy(s);
}

void Host::setDisplayFontFixedPitch(bool enable)
{
    mDisplayFont.setFixedPitch(enable);
}

void Host::loadSecuredPassword()
{
    auto *job = new QKeychain::ReadPasswordJob(QStringLiteral("Mudlet profile"));
    job->setAutoDelete(false);
    job->setInsecureFallback(false);

    job->setKey(getName());

    connect(job, &QKeychain::ReadPasswordJob::finished, this, [=](QKeychain::Job* job) {
        if (job->error()) {
            const auto error = job->errorString();
            if (error != QStringLiteral("Entry not found") && error != QStringLiteral("No match")) {
                qDebug() << "Host::loadSecuredPassword ERROR: couldn't retrieve secure password for" << getName() << ", error is:" << error;
            }
        } else {
            auto readJob = static_cast<QKeychain::ReadPasswordJob*>(job);
            setPass(readJob->textData());
        }

        job->deleteLater();
    });

    job->start();
}

// Only needed for places outside of this class:
void Host::updateAnsi16ColorsInTable()
{
    mLuaInterpreter.updateAnsi16ColorsInTable();
}

void Host::setPlayerRoomStyleDetails(const quint8 styleCode, const quint8 outerDiameter, const quint8 innerDiameter, const QColor& outerColor, const QColor& innerColor)
{
    mPlayerRoomStyle = styleCode;
    mPlayerRoomOuterDiameterPercentage = outerDiameter;
    mPlayerRoomInnerDiameterPercentage = innerDiameter;
    mPlayerRoomOuterColor = outerColor;
    mPlayerRoomInnerColor = innerColor;
}

void Host::getPlayerRoomStyleDetails(quint8& styleCode, quint8& outerDiameter, quint8& innerDiameter, QColor& primaryColor, QColor& secondaryColor)
{
    // Now we have the exclusive lock on this class's protected members

    styleCode = mPlayerRoomStyle;
    outerDiameter = mPlayerRoomOuterDiameterPercentage;
    innerDiameter = mPlayerRoomInnerDiameterPercentage;
    primaryColor = mPlayerRoomOuterColor;
    secondaryColor = mPlayerRoomInnerColor;
}

// Used to set the searchOptions here and the one in the editor if present, for
// use by the XMLimporter class:
void Host::setSearchOptions(const dlgTriggerEditor::SearchOptions optionsState)
{
    mSearchOptions = optionsState;
    if (mpEditorDialog) {
        mpEditorDialog->setSearchOptions(optionsState);
    }
}

std::pair<bool, QString> Host::setMapperTitle(const QString& title)
{
    if (!mpDockableMapWidget) {
        return {false, "no floating/dockable type map window found"};
    }

    if (title.isEmpty()) {
        mpDockableMapWidget->setWindowTitle(tr("Map - %1").arg(mHostName));
    } else {
        mpDockableMapWidget->setWindowTitle(title);
    }

    return {true, QString()};
}

void Host::setDebugShowAllProblemCodepoints(const bool state)
{
    if (mDebugShowAllProblemCodepoints != state) {
        mDebugShowAllProblemCodepoints = state;
        emit signal_changeDebugShowAllProblemCodepoints(state);
    }
}

void Host::setCompactInputLine(const bool state)
{
    if (mCompactInputLine != state) {
        mCompactInputLine = state;
        // When the profile is being loaded and the previously saved data is
        // read from the XML file the main TConsole has not been instatiated
        // yet - so must check for it existing first - and ensure the read
        // setting is applied in the constructor for it:
        if (mpConsole && mpConsole->mpButtonMainLayer) {
            mpConsole->mpButtonMainLayer->setVisible(!state);
        }
    }
}

QPointer<TConsole> Host::findConsole(QString name)
{
    if (name.isEmpty() or name == QStringLiteral("main")) {
        // Reason for the deref-plus-ref in the next line: `QPointer`s do not
        // follow inheritance. See https://bugreports.qt.io/browse/QTBUG-2258
        return &*mpConsole;
    } else {
        return mpConsole->mSubConsoleMap.value(name);
    }
}

QPair<bool, QStringList> Host::getLines(const QString& windowName, const int lineFrom, const int lineTo)
{
    if (!mpConsole) {
        QStringList failMessage;
        failMessage << QStringLiteral("internal error: no main TConsole - please report").arg(windowName);
        return qMakePair(false, failMessage);
    }

    if (windowName.isEmpty() || windowName == QLatin1String("main")) {
        return qMakePair(true, mpConsole->getLines(lineFrom, lineTo));
    }

    auto pC = mpConsole->mSubConsoleMap.value(windowName);
    if (!pC) {
        QStringList failMessage;
        failMessage << QStringLiteral("mini console, user window or buffer '%1' not found").arg(windowName);
        return qMakePair(false, failMessage);
    }
    return qMakePair(true, pC->getLines(lineFrom, lineTo));
}

std::pair<bool, QString> Host::openWindow(const QString& name, bool loadLayout, bool autoDock, const QString& area)
{
    if (!mpConsole) {
        return {false, QString()};
    }

    if (name.isEmpty()) {
        return {false, QLatin1String("an userwindow cannot have an empty string as its name")};
    }

    //Dont create Userwindow if there is a Label with the same name already. It breaks the UserWindow
    auto pL = mpConsole->mLabelMap.value(name);
    if (pL) {
        return {false, QStringLiteral("label with the name '%1' already exists").arg(name)};
    }

    auto hostName(getName());
    auto console = mpConsole->mSubConsoleMap.value(name);
    auto dockwidget = mpConsole->mDockWidgetMap.value(name);

    if (!console && !dockwidget) {
        // The name is not used in either the QMaps of all user created TConsole
        // or TDockWidget instances - so we can make a NEW one:
        dockwidget = new TDockWidget(this, name);
        dockwidget->setObjectName(QStringLiteral("dockWindow_%1_%2").arg(hostName, name));
        dockwidget->setContentsMargins(0, 0, 0, 0);
        dockwidget->setFeatures(QDockWidget::DockWidgetClosable|QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
        dockwidget->setWindowTitle(name);
        mpConsole->mDockWidgetMap.insert(name, dockwidget);
        // It wasn't obvious but the parent passed to the TConsole constructor
        // is sliced down to a QWidget and is NOT a TDockWidget pointer:
        console = new TConsole(this, TConsole::UserWindow, dockwidget->widget());
        console->setObjectName(QStringLiteral("dockWindowConsole_%1_%2").arg(hostName, name));
        // Without this the TConsole instance inside the TDockWidget will be
        // left being called the default value of "main":
        console->mConsoleName = name;
        console->setContentsMargins(0, 0, 0, 0);
        dockwidget->setTConsole(console);
        console->layerCommandLine->hide();
        console->mpScrollBar->hide();
        mpConsole->mSubConsoleMap.insert(name, console);
        dockwidget->setStyleSheet(mProfileStyleSheet);
        mudlet::self()->addDockWidget(Qt::RightDockWidgetArea, dockwidget);
        console->setFontSize(10);
    }
    if (!console || !dockwidget) {
        return {false, QStringLiteral("userwindow '%1' already exists").arg(name)};
    }

    // The name is used in BOTH the QMaps of all user created TConsole
    // and TDockWidget instances - so we HAVE an existing user window,
    // Lets confirm this:
    Q_ASSERT_X(console->getType() == TConsole::UserWindow, "host::openWindow(...)", "An existing TConsole was expected to be marked as a User Window type but it isn't");
    dockwidget->update();

    if (loadLayout && !dockwidget->hasLayoutAlready) {
        mudlet::self()->loadWindowLayout();
        dockwidget->hasLayoutAlready = true;
    }
    dockwidget->show();
    dockwidget->setAllowedAreas(autoDock ? Qt::AllDockWidgetAreas : Qt::NoDockWidgetArea);

    if (area.isEmpty()) {
        return {true, QString()};
    }

    if (area == QLatin1String("f") || area == QLatin1String("floating")) {
        if (!dockwidget->isFloating()) {
            dockwidget->setFloating(true);
        }
        return {true, QString()};
    } else {
        if (area == QLatin1String("r") || area == QLatin1String("right")) {
            dockwidget->setFloating(false);
            mudlet::self()->addDockWidget(Qt::RightDockWidgetArea, dockwidget);
            return {true, QString()};
        } else if (area == QLatin1String("l") || area == QLatin1String("left")) {
            dockwidget->setFloating(false);
            mudlet::self()->addDockWidget(Qt::LeftDockWidgetArea, dockwidget);
            return {true, QString()};
        } else if (area == QLatin1String("t") || area == QLatin1String("top")) {
            dockwidget->setFloating(false);
            mudlet::self()->addDockWidget(Qt::TopDockWidgetArea, dockwidget);
            return {true, QString()};
        } else if (area == QLatin1String("b") || area == QLatin1String("bottom")) {
            dockwidget->setFloating(false);
            mudlet::self()->addDockWidget(Qt::BottomDockWidgetArea, dockwidget);
            return {true, QString()};
        } else {
            return {false, QStringLiteral(R"("docking option "%1" not available. available docking options are "t" top, "b" bottom, "r" right, "l" left and "f" floating")").arg(area)};
        }
    }
}

std::pair<bool, QString> Host::createMiniConsole(const QString& windowname, const QString& name, int x, int y, int width, int height)
{
    if (!mpConsole) {
        return {false, QString()};
    }

    auto pC = mpConsole->mSubConsoleMap.value(name);
    auto pW = mpConsole->mDockWidgetMap.value(name);
    if (!pC) {
        pC = mpConsole->createMiniConsole(windowname, name, x, y, width, height);
        if (pC) {
            pC->setFontSize(12);
            return {true, QString()};
        }
    } else if (pC) {
        // CHECK: The absence of an explict return statement in this block means that
        // reusing an existing mini console causes the lua function to seem to
        // fail - is this as per Wiki?
        // This part was causing problems with UserWindows
        if (!pW) {
            pC->resize(width, height);
            pC->move(x, y);
            return {false, QStringLiteral("miniconsole '%1' already exists, moving/resizing '%1'").arg(name)};
        }
    }
    return {false, QStringLiteral("miniconsole/userwindow '%1' already exists").arg(name)};
}

std::pair<bool, QString> Host::createLabel(const QString& windowname, const QString& name, int x, int y, int width, int height, bool fillBg, bool clickthrough)
{
    if (!mpConsole) {
        return {false, QString()};
    }

    auto pL = mpConsole->mLabelMap.value(name);
    auto pC = mpConsole->mSubConsoleMap.value(name);
    if (!pL && !pC) {
        pL = mpConsole->createLabel(windowname, name, x, y, width, height, fillBg, clickthrough);
        if (pL) {
            return {true, QString()};
        }
    } else if (pL) {
        return {false, QStringLiteral("label '%1' already exists").arg(name)};
    } else if (pC) {
        return {false, QStringLiteral("a miniconsole/userwindow with the name '%1' already exists").arg(name)};
    }
    return {false, QString()};

}

bool Host::setClickthrough(const QString& name, bool clickthrough)
{
    if (!mpConsole) {
        return false;
    }

    auto pL = mpConsole->mLabelMap.value(name);
    if (pL) {
        pL->setClickThrough(clickthrough);
        return true;
    }

    return false;
}

void Host::hideMudletsVariables()
{
    auto varUnit = getLuaInterface()->getVarUnit();

    // remember which users' saved variables shouldn't be hidden
    QVector<QString> unhideSavedVars;
    for (const auto& variable : qAsConst(varUnit->savedVars)) {
        if (!varUnit->isHidden(variable)) {
            unhideSavedVars.append(variable);
        }
    }

    // mark all currently loaded Lua variables as hidden
    // this covers entirety of Mudlets API (good!) and but unfortunately
    // user's saved variables as well
    LuaInterface* lI = getLuaInterface();
    lI->getVars(true);

    // unhide user's saved variables
    for (const auto& variable : qAsConst(unhideSavedVars)) {
        varUnit->removeHidden(variable);
    }
}

void Host::close()
{
    // disconnect before removing objects from memory as sysDisconnectionEvent needs that stuff.
    if (mSslTsl) {
        mTelnet.abortConnection();
    } else {
        mTelnet.disconnectIt();
    }

    // close script editor
    if (mpEditorDialog) {
        mpEditorDialog->setAttribute(Qt::WA_DeleteOnClose);
        mpEditorDialog->close();
        mpEditorDialog = nullptr;
    }
    // close notepad
    if (mpNotePad) {
        mpNotePad->save();
        mpNotePad->setAttribute(Qt::WA_DeleteOnClose);
        mpNotePad->close();
        mpNotePad = nullptr;
    }
    // close IRC client window
    if (mpDlgIRC) {
        mpDlgIRC->setAttribute(Qt::WA_DeleteOnClose);
        mpDlgIRC->deleteLater();
        mpDlgIRC = nullptr;
    }
    if (mpConsole) {
        mpConsole->close();
        mpConsole = nullptr;
    }
}

bool Host::createBuffer(const QString& name)
{
    if (!mpConsole) {
        return false;
    }

    auto pC = mpConsole->mSubConsoleMap.value(name);
    if (!pC) {
        pC = mpConsole->createBuffer(name);
        if (pC) {
            return true;
        }
    }
    return false;
}


bool Host::clearWindow(const QString& name)
{
    if (!mpConsole) {
        return false;
    }

    auto pC = mpConsole->mSubConsoleMap.value(name);
    if (pC) {
        pC->mUpperPane->resetHScrollbar();
        pC->buffer.clear();
        pC->mUpperPane->update();
        return true;
    } else {
        return false;
    }
}

bool Host::showWindow(const QString& name)
{
    if (!mpConsole) {
        return false;
    }

    auto pC = mpConsole->mSubConsoleMap.value(name);
    auto pL = mpConsole->mLabelMap.value(name);
    auto pN = mpConsole->mSubCommandLineMap.value(name);
    // check labels first as they are shown/hidden more often
    if (pL) {
        pL->show();
        return true;
    } else if (pC) {
        auto pD = mpConsole->mDockWidgetMap.value(name);
        if (pD) {
            pD->update();
            pD->show();
            // TODO: conside refactoring TConsole::showWindow(name) so that there is a TConsole::showWindow() that can be called directly on the TConsole concerned?
            return mpConsole->showWindow(name);
        } else {
            return mpConsole->showWindow(name);
        }
    }

    if (pN) {
        pN->show();
        return true;
    }

    return false;
}

bool Host::hideWindow(const QString& name)
{
    if (!mpConsole) {
        return false;
    }

    auto pC = mpConsole->mSubConsoleMap.value(name);
    auto pL = mpConsole->mLabelMap.value(name);
    auto pN = mpConsole->mSubCommandLineMap.value(name);

    // check labels first as they are shown/hidden more often
    if (pL) {
        pL->hide();
        return true;
    } else if (pC) {
        auto pD = mpConsole->mDockWidgetMap.value(name);
        if (pD) {
            pD->hide();
            pD->update();
        }
        return mpConsole->hideWindow(name);
    }

    if (pN) {
        pN->hide();
        return true;
    }

    return false;
}

bool Host::resizeWindow(const QString& name, int x1, int y1)
{
    if (!mpConsole) {
        return false;
    }

    auto pL = mpConsole->mLabelMap.value(name);
    auto pC = mpConsole->mSubConsoleMap.value(name);
    auto pD = mpConsole->mDockWidgetMap.value(name);
    auto pN = mpConsole->mSubCommandLineMap.value(name);

    if (pL) {
        pL->resize(x1, y1);
        return true;
    }

    if (pC && !pD) {
        // NOT a floatable/dockable "user window"
        pC->resize(x1, y1);
        return true;
    }

    if (pC && pD) {
        if (!pD->isFloating()) {
            // Undock a docked window
            pD->setFloating(true);
        }

        pD->resize(x1, y1);
        return true;
    }

    if (pN) {
        pN->resize(x1, y1);
        return true;
    }

    return false;
}

bool Host::moveWindow(const QString& name, int x1, int y1)
{
    if (!mpConsole) {
        return false;
    }

    auto pL = mpConsole->mLabelMap.value(name);
    auto pC = mpConsole->mSubConsoleMap.value(name);
    auto pD = mpConsole->mDockWidgetMap.value(name);
    auto pN = mpConsole->mSubCommandLineMap.value(name);

    if (pL) {
        pL->move(x1, y1);
        return true;
    }

    if (pC && !pD) {
        // NOT a floatable/dockable "user window"
        pC->move(x1, y1);
        pC->mOldX = x1;
        pC->mOldY = y1;
        return true;
    }

    if (pC && pD) {
        if (!pD->isFloating()) {
            // Undock a docked window
            pD->setFloating(true);
        }

        pD->move(x1, y1);
        return true;
    }

    if (pN) {
        pN->move(x1, y1);
        return true;
    }

    return false;
}

std::pair<bool, QString> Host::setWindow(const QString& windowname, const QString& name, int x1, int y1, bool show)
{
    if (!mpConsole) {
        return {false, QString()};
    }

    auto pL = mpConsole->mLabelMap.value(name);
    auto pC = mpConsole->mSubConsoleMap.value(name);
    auto pD = mpConsole->mDockWidgetMap.value(windowname);
    auto pW = mpConsole->mpMainFrame;
    auto pM = mpConsole->mpMapper;
    auto pN = mpConsole->mSubCommandLineMap.value(name);

    if (!pD && windowname.toLower() != QLatin1String("main")) {
        return {false, QStringLiteral("window '%1' not found").arg(windowname)};
    }

    if (pD) {
        pW = pD->widget();
    }

    if (pL) {
        pL->setParent(pW);
        pL->move(x1, y1);
        if (show) {
            pL->show();
        }
        return {true, QString()};
    } else if (pC) {
        pC->setParent(pW);
        pC->move(x1, y1);
        pC->mOldX = x1;
        pC->mOldY = y1;
        if (show) {
            pC->show();
        }
        return {true, QString()};
    } else if (pN) {
        pN->setParent(pW);
        pN->move(x1, y1);
        if (show) {
            pN->show();
        }
        return {true, QString()};
    } else if (pM && name.toLower() == QLatin1String("mapper")) {
        pM->setParent(pW);
        pM->move(x1, y1);
        if (show) {
            pM->show();
        }
        return {true, QString()};
    }

    return {false, QStringLiteral("element '%1' not found").arg(name)};
}

std::pair<bool, QString> Host::openMapWidget(const QString& area, int x, int y, int width, int height)
{
    if (!mpConsole) {
        return {false, QString()};
    }

    auto pM = mpDockableMapWidget;
    auto pMapper = mpMap.data()->mpMapper;
    if (!pM && !pMapper) {
        showHideOrCreateMapper(true);
        pM = mpDockableMapWidget;
    }
    if (!pM) {
        return {false, QStringLiteral("cannot create map widget. Do you already use an embedded mapper?")};
    }
    pM->show();
    if (area.isEmpty()) {
        return {true, QString()};
    }

    if (area == QLatin1String("f") || area == QLatin1String("floating")) {
        if (!pM->isFloating()) {
            // Undock a docked window
            // Change of position or size is only possible when floating
            pM->setFloating(true);
        }
        if ((x != -1) && (y != -1)) {
            pM->move(x, y);
        }
        if ((width != -1) && (height != -1)) {
            pM->resize(width, height);
        }
        return {true, QString()};
    } else {
        if (area == QLatin1String("r") || area == QLatin1String("right")) {
            pM->setFloating(false);
            mudlet::self()->addDockWidget(Qt::RightDockWidgetArea, pM);
            return {true, QString()};
        } else if (area == QLatin1String("l") || area == QLatin1String("left")) {
            pM->setFloating(false);
            mudlet::self()->addDockWidget(Qt::LeftDockWidgetArea, pM);
            return {true, QString()};
        } else if (area == QLatin1String("t") || area == QLatin1String("top")) {
            pM->setFloating(false);
            mudlet::self()->addDockWidget(Qt::TopDockWidgetArea, pM);
            return {true, QString()};
        } else if (area == QLatin1String("b") || area == QLatin1String("bottom")) {
            pM->setFloating(false);
            mudlet::self()->addDockWidget(Qt::BottomDockWidgetArea, pM);
            return {true, QString()};
        } else {
            return {false, QStringLiteral(R"("docking option "%1" not available. available docking options are "t" top, "b" bottom, "r" right, "l" left and "f" floating")").arg(area)};
        }
    }
}

std::pair<bool, QString> Host::closeMapWidget()
{
    if (!mpConsole) {
        return {false, QString()};
    }

    auto pM = mpDockableMapWidget;
    if (!pM) {
        return {false, QStringLiteral("no map widget found to close")};
    }
    if (!pM->isVisible()) {
        return {false, QStringLiteral("map widget already closed")};
    }
    pM->hide();
    return {true, QString()};
}

bool Host::closeWindow(const QString& name)
{
    if (!mpConsole) {
        return false;
    }

    auto pC = mpConsole->mSubConsoleMap.value(name);
    if (pC) {
        auto pD = mpConsole->mDockWidgetMap.value(name);
        if (pD) {
            pD->hide();
            pD->update();
        }
        return mpConsole->hideWindow(name);
    }
    return false;
}

bool Host::echoWindow(const QString& name, const QString& text)
{
    if (!mpConsole) {
        return -1;
    }

    auto pL = mpConsole->mLabelMap.value(name);
    auto pC = mpConsole->mSubConsoleMap.value(name);
    if (pC) {
        pC->print(text);
        return true;
    } else if (pL) {
        pL->setText(text);
        return true;
    } else {
        return false;
    }
}

bool Host::pasteWindow(const QString& name)
{
    if (!mpConsole) {
        return -1;
    }

    auto pC = mpConsole->mSubConsoleMap.value(name);
    if (pC) {
        pC->pasteWindow(mpConsole->mClipboard);
        return true;
    } else {
        return false;
    }
}

bool Host::setCmdLineAction(const QString& name, const int func)
{
    if (!mpConsole) {
        return false;
    }
    auto pN = mpConsole->mSubCommandLineMap.value(name);
    if (pN) {
        pN->setAction(func);
        return true;
    }
    return false;
}

bool Host::resetCmdLineAction(const QString& name)
{
    if (!mpConsole) {
        return false;
    }
    auto pN = mpConsole->mSubCommandLineMap.value(name);
    if (pN) {
        pN->resetAction();
        return true;
    }
    return false;
}

bool Host::setLabelClickCallback(const QString& name, const int func)
{
    if (!mpConsole) {
        return false;
    }

    auto pL = mpConsole->mLabelMap.value(name);
    if (pL) {
        pL->setClick(func);
        return true;
    }
    return false;
}

bool Host::setLabelDoubleClickCallback(const QString& name, const int func)
{
    if (!mpConsole) {
        return false;
    }

    auto pL = mpConsole->mLabelMap.value(name);
    if (pL) {
        pL->setDoubleClick(func);
        return true;
    }
    return false;
}

bool Host::setLabelReleaseCallback(const QString& name, const int func)
{
    if (!mpConsole) {
        return false;
    }

    auto pL = mpConsole->mLabelMap.value(name);
    if (pL) {
        pL->setRelease(func);
        return true;
    }
    return false;
}

bool Host::setLabelMoveCallback(const QString& name, const int func)
{
    if (!mpConsole) {
        return false;
    }

    auto pL = mpConsole->mLabelMap.value(name);
    if (pL) {
        pL->setMove(func);
        return true;
    }
    return false;
}

bool Host::setLabelWheelCallback(const QString& name, const int func)
{
    if (!mpConsole) {
        return false;
    }

    auto pL = mpConsole->mLabelMap.value(name);
    if (pL) {
        pL->setWheel(func);
        return true;
    }
    return false;
}

bool Host::setLabelOnEnter(const QString& name, const int func)
{
    if (!mpConsole) {
        return false;
    }

    auto pL = mpConsole->mLabelMap.value(name);
    if (pL) {
        pL->setEnter(func);
        return true;
    }
    return false;
}

bool Host::setLabelOnLeave(const QString& name, const int func)
{
    if (!mpConsole) {
        return false;
    }

    auto pL = mpConsole->mLabelMap.value(name);
    if (pL) {
        pL->setLeave(func);
        return true;
    }
    return false;
}

QSize Host::calcFontSize(const QString& windowName)
{
    if (!mpConsole) {
        return QSize(-1, -1);
    }

    QFont font;
    if (windowName.isEmpty() || windowName.compare(QStringLiteral("main"), Qt::CaseSensitive) == 0) {
        font = mDisplayFont;
    } else {
        auto pC = mpConsole->mSubConsoleMap.value(windowName);
        if (pC) {
            Q_ASSERT_X(pC->mUpperPane, "calcFontSize", "located console does not have the upper pane available");
            font = pC->mUpperPane->mDisplayFont;
        } else {
            return QSize(-1, -1);
        }
    }

    auto fontMetrics = QFontMetrics(font);
    return QSize(fontMetrics.horizontalAdvance(QChar('W')), fontMetrics.height());
}

bool Host::setProfileStyleSheet(const QString& styleSheet)
{
    if (!mpConsole) {
        return false;
    }

    mProfileStyleSheet = styleSheet;
    mpConsole->setStyleSheet(styleSheet);
    mpEditorDialog->setStyleSheet(styleSheet);

    if (mpDlgProfilePreferences) {
        mpDlgProfilePreferences->setStyleSheet(styleSheet);
    }
    if (mpNotePad) {
        mpNotePad->setStyleSheet(styleSheet);
        mpNotePad->notesEdit->setStyleSheet(styleSheet);
    }
    if (mpDockableMapWidget) {
        mpDockableMapWidget->setStyleSheet(styleSheet);
    }

    for (auto& dockWidget : mpConsole->mDockWidgetMap) {
        dockWidget->setStyleSheet(styleSheet);
    }
    if (this == mudlet::self()->mpCurrentActiveHost) {
        mudlet::self()->setGlobalStyleSheet(styleSheet);
    }
    return true;
}


bool Host::setBackgroundColor(const QString& name, int r, int g, int b, int alpha)
{
    if (!mpConsole) {
        return false;
    }

    auto pC = mpConsole->mSubConsoleMap.value(name);
    auto pL = mpConsole->mLabelMap.value(name);
    if (pC) {
        pC->setConsoleBgColor(r, g, b, alpha);
        return true;
    } else if (pL) {
        QPalette mainPalette;
        mainPalette.setColor(QPalette::Window, QColor(r, g, b, alpha));
        pL->setPalette(mainPalette);
        return true;
    }

    return false;
}

bool Host::setBackgroundImage(const QString& name, QString& imgPath, int mode)
{
    if (!mpConsole) {
        return false;
    }

    if (QDir::homePath().contains('\\')) {
        imgPath.replace('/', R"(\)");
    } else {
        imgPath.replace('\\', "/");
    }

    if (name.isEmpty() || name.compare(QStringLiteral("main"), Qt::CaseSensitive) == 0) {
        mpConsole->setConsoleBackgroundImage(imgPath, mode);
        return true;
    }

    auto pL = mpConsole->mLabelMap.value(name);
    if (pL) {
        QPixmap bgPixmap(imgPath);
        pL->setPixmap(bgPixmap);
        return true;
    }

    auto pC = mpConsole->mSubConsoleMap.value(name);
    if (pC) {
        pC->setConsoleBackgroundImage(imgPath, mode);
        return true;
    }
    return false;
}

bool Host::resetBackgroundImage(const QString &name)
{
    if (!mpConsole) {
        return false;
    }

    if (name.isEmpty() || name.compare(QStringLiteral("main"), Qt::CaseSensitive) == 0) {
        mpConsole->resetConsoleBackgroundImage();
        return true;
    }

    auto pL = mpConsole->mLabelMap.value(name);
    if (pL) {
        pL->clear();
        return true;
    }

    auto pC = mpConsole->mSubConsoleMap.value(name);
    if (pC) {
        pC->resetConsoleBackgroundImage();
        return true;
    }

    return false;
}

// Needed to extract into a separate method from mudlet::slot_mapper() so that
// we can use it WITHOUT loading a file - at least for the
// TConsole::importMap(...) case that may need to create a map widget before it
// loads/imports a non-default (last saved map in profile's map directory).
void Host::showHideOrCreateMapper(const bool loadDefaultMap)
{
    auto pMap = mpMap.data();
    if (pMap->mpMapper) {
        toggleMapperVisibility();
        return;
    }

    createMapper(loadDefaultMap);
}

void Host::toggleMapperVisibility()
{
    auto pMap = mpMap.data();
    bool visStatus = mpMap->mpMapper->isVisible();
    if (pMap->mpMapper->isFloatAndDockable()) {
        // If we are using a floating/dockable widget we must show/hide that
        // only and not the mapper widget (otherwise it messes up {shrinks
        // to a minimal size} the mapper inside the container QDockWidget). This
        // is the same as the case for a TConsole inside a TDockWidget in
        // (void) TDockWidget::setVisible(bool).
        pMap->mpMapper->parentWidget()->setVisible(!visStatus);
    } else {
        pMap->mpMapper->setVisible(!visStatus);
    }
}

void Host::createMapper(const bool loadDefaultMap)
{
    auto pMap = mpMap.data();
    auto hostName(getName());
    mpDockableMapWidget = new QDockWidget(tr("Map - %1").arg(hostName));
    mpDockableMapWidget->setObjectName(QStringLiteral("dockMap_%1").arg(hostName));
    // Arrange for TMap member values to be copied from the Host masters so they
    // are in place when the 2D mapper is created:
    getPlayerRoomStyleDetails(pMap->mPlayerRoomStyle,
                                     pMap->mPlayerRoomOuterDiameterPercentage,
                                     pMap->mPlayerRoomInnerDiameterPercentage,
                                     pMap->mPlayerRoomOuterColor,
                                     pMap->mPlayerRoomInnerColor);

    pMap->mpMapper = new dlgMapper(mpDockableMapWidget, this, pMap); //FIXME: mpHost definieren
    pMap->mpMapper->setStyleSheet(mProfileStyleSheet);
    mpDockableMapWidget->setWidget(pMap->mpMapper);

    if (loadDefaultMap && pMap->mpRoomDB->isEmpty()) {
        qDebug() << "Host::create_mapper() - restore map case 3.";
        pMap->pushErrorMessagesToFile(tr("Pre-Map loading(3) report"), true);
        QDateTime now(QDateTime::currentDateTime());
        if (pMap->restore(QString())) {
            pMap->audit();
            pMap->mpMapper->mp2dMap->init();
            pMap->mpMapper->updateAreaComboBox();
            pMap->mpMapper->resetAreaComboBoxToPlayerRoomArea();
            pMap->mpMapper->show();
        }

        pMap->pushErrorMessagesToFile(tr("Loading map(3) at %1 report").arg(now.toString(Qt::ISODate)), true);

    } else {
        if (pMap->mpMapper) {
            pMap->mpMapper->show();
        }
    }
    mudlet::self()->addDockWidget(Qt::RightDockWidgetArea, mpDockableMapWidget);

    // XXX: should this be called multiple times?
    mudlet::self()->loadWindowLayout();

    check_for_mappingscript();
    TEvent mapOpenEvent {};
    mapOpenEvent.mArgumentList.append(QLatin1String("mapOpenEvent"));
    mapOpenEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    raiseEvent(mapOpenEvent);
}

void Host::setDockLayoutUpdated(const QString& name)
{
    if (!mpConsole) {
        return;
    }

    auto pD = mpConsole->mDockWidgetMap.value(name);
    if (Q_LIKELY(pD) && !mDockLayoutChanges.contains(name)) {
        pD->setProperty("layoutChanged", QVariant(true));
        mDockLayoutChanges.append(name);
    }
}

void Host::setToolbarLayoutUpdated(TToolBar* pTB)
{
    if (!mToolbarLayoutChanges.contains(pTB)) {
        pTB->setProperty("layoutChanged", QVariant(true));
        mToolbarLayoutChanges.append(pTB);
    }
}

bool Host::commitLayoutUpdates(bool flush)
{
    bool updated = false;
    if (mpConsole && !flush) {
        // commit changes (or rather clear the layout changed flags) for dockwidget
        // consoles (user windows)
        for (auto dockedConsoleName : mDockLayoutChanges) {
            auto pD = mpConsole->mDockWidgetMap.value(dockedConsoleName);
            if (Q_LIKELY(pD) && pD->property("layoutChanged").toBool()) {
                pD->setProperty("layoutChanged", QVariant(false));
                updated = true;
            }
        }
    }
    mDockLayoutChanges.clear();

    // commit changes (or rather clear the layout changed flags) for
    // dockable/floating toolbars across all profiles:
    if (!flush) {
        for (auto pToolBar : mToolbarLayoutChanges) {
            // Under some circumstances there is NOT a
            // pToolBar->property("layoutChanged") and examining that
            // non-existant variant to see if it was true or false causes seg. faults!
            if (Q_UNLIKELY(!pToolBar->property("layoutChanged").isValid())) {
                qWarning().nospace().noquote() << "host::commitLayoutUpdates() WARNING - was about to check for \"layoutChanged\" meta-property on a toolbar without that property!";
            } else if (pToolBar->property("layoutChanged").toBool()) {
                pToolBar->setProperty("layoutChanged", QVariant(false));
                updated = true;
            }
        }
    }
    mToolbarLayoutChanges.clear();
    return updated;
}
