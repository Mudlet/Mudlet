/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2015-2024 by Stephen Lyons - slysven@virginmedia.com    *
 *   Copyright (C) 2016 by Ian Adkins - ieadkins@gmail.com                 *
 *   Copyright (C) 2018 by Huadong Qi - novload@outlook.com                *
 *   Copyright (C) 2023 by Lecker Kebap - Leris@mudlet.org                 *
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

#include "dlgIRC.h"
#include "dlgMapper.h"
#include "dlgModuleManager.h"
#include "dlgNotepad.h"
#include "dlgPackageManager.h"
#include "dlgProfilePreferences.h"
#include "GifTracker.h"
#include "GMCPAuthenticator.h"
#include "LuaInterface.h"
#include "mudlet.h"
#include "TCommandLine.h"
#include "TConsole.h"
#include "TDebug.h"
#include "TDebug.h"
#include "TDockWidget.h"
#include "TEvent.h"
#include "TLabel.h"
#include "TMainConsole.h"
#include "TMap.h"
#include "TMedia.h"
#include "TRoomDB.h"
#include "TScript.h"
#include "TTextEdit.h"
#include "TToolBar.h"
#include "VarUnit.h"
#include "XMLimport.h"

#include "pre_guard.h"
#include <chrono>
#include <QtConcurrent>
#include <QDialog>
#include <QtUiTools>
#include <QNetworkProxy>
#include <QSettings>
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
        return qsl("+:0:0:0:0:000");
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

    qint64 const days = elapsed / std::chrono::milliseconds(24h).count();
    qint64 remainder = elapsed - (days * std::chrono::milliseconds(24h).count());
    quint8 const hours = static_cast<quint8>(remainder / std::chrono::milliseconds(1h).count());
    remainder = remainder - (hours * std::chrono::milliseconds(1h).count());
    quint8 const minutes = static_cast<quint8>(remainder / std::chrono::milliseconds(1min).count());
    remainder = remainder - (minutes * std::chrono::milliseconds(1min).count());
    quint8 const seconds = static_cast<quint8>(remainder / std::chrono::milliseconds(1s).count());
    quint16 const milliSeconds = static_cast<quint16>(remainder - (seconds * std::chrono::milliseconds(1s).count()));
    return qsl("%1:%2:%3:%4:%5:%6").arg((isNegative ? QLatin1String("-") : QLatin1String("+")), QString::number(days), QString::number(hours), QString::number(minutes), QString::number(seconds), QString::number(milliSeconds));
}

Host::Host(int port, const QString& hostname, const QString& login, const QString& pass, int id)
: mTelnet(this, hostname)
, mpConsole(nullptr)
, mLuaInterpreter(this, hostname, id)
, commandLineMinimumHeight(30)
, mAlertOnNewData(true)
, mAllowToSendCommand(true)
, mAutoClearCommandLineAfterSend(false)
, mHighlightHistory(true)
, mBlockScriptCompile(true)
, mBlockStopWatchCreation(true)
, mEchoLuaErrors(false)
, mCommandLineFont(QFont(qsl("Bitstream Vera Sans Mono"), 14, QFont::Normal))
, mCommandSeparator(qsl(";;"))
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
, mIsProfileLoadingSequence(false)
, mNoAntiAlias(false)
, mpEditorDialog(nullptr)
, mpMap(new TMap(this, hostname))
, mpMedia(new TMedia(this, hostname))
, mpAuth(new GMCPAuthenticator(this))
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
, mUSE_IRE_DRIVER_BUGFIX(false)
, mUSE_UNIX_EOL(false)
, mWrapAt(100)
, mWrapIndentCount(0)
, mEditorAutoComplete(true)
, mEditorTheme(QLatin1String("Mudlet"))
, mEditorThemeFile(QLatin1String("Mudlet.tmTheme"))
, mThemePreviewItemID(-1)
, mThemePreviewType(QString())
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
, mMapStrongHighlight(false)
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
, mBufferSearchOptions(TConsole::SearchOption::SearchOptionNone)
, mpDlgIRC(nullptr)
, mpDlgProfilePreferences(nullptr)
, mTutorialForCompactLineAlreadyShown(false)
, mDisplayFont(QFont(qsl("Bitstream Vera Sans Mono"), 14, QFont::Normal))
, mLuaInterface(nullptr)
, mTriggerUnit(this)
, mTimerUnit(this)
, mScriptUnit(this)
, mAliasUnit(this)
, mActionUnit(this)
, mKeyUnit(this)
, mGifTracker()
, mHostID(id)
, mHostName(hostname)
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
    TDebug::addHost(this, mHostName);

    // The "autolog" sentinel file controls whether logging the game's text as
    // plain text or HTML is immediately resumed on profile loading. Do not
    // confuse it with the "autologin" item, which controls whether the profile
    // is automatically started when the Mudlet application is run!
    mLogStatus = QFile::exists(mudlet::getMudletPath(mudlet::profileDataItemPath, mHostName, qsl("autolog")));
    // "autotimestamp" determines if profile loads with timestamps enabled
    mTimeStampStatus = QFile::exists(mudlet::getMudletPath(mudlet::profileDataItemPath, mHostName, qsl("autotimestamp")));
    mLuaInterface.reset(new LuaInterface(this->getLuaInterpreter()->getLuaGlobalState()));

    // Copy across the details needed for the "color_table":
    mLuaInterpreter.updateAnsi16ColorsInTable();
    mLuaInterpreter.updateExtendedAnsiColorsInTable();

    const QString directoryLogFile = mudlet::getMudletPath(mudlet::profileDataItemPath, mHostName, qsl("log"));
    const QString logFileName = qsl("%1/errors.txt").arg(directoryLogFile);
    const QDir dirLogFile;
    if (!dirLogFile.exists(directoryLogFile)) {
        dirLogFile.mkpath(directoryLogFile);
    }
    mErrorLogFile.setFileName(logFileName);
    mErrorLogFile.open(QIODevice::Append);
     /*
     * Mudlet will log messages in ASCII, but force a universal (UTF-8) encoding
     * since user-content can contain anything and someone else reviewing
     * such logs need not have the same default encoding which would be used
     * otherwise - note that this must be done AFTER setDevice(...):
     */
    mErrorLogStream.setDevice(&mErrorLogFile);
    // In Qt6 the default encoding is UTF-8
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    mErrorLogStream.setCodec(QTextCodec::codecForName("UTF-8"));
#endif

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

    // These details are filled in by the dlgConnectionProfile class when that
    // is used to select a profile however when the profile is auto-loaded or
    // selected from a command line argument that does not happen and they need
    // to be populated ASAP from the stored details in the profile's settings.
    mUrl = readProfileData(qsl("url"));
    const QString host_port = readProfileData(qsl("port"));
    if (bool isOk = false; !host_port.isEmpty() && host_port.toInt(&isOk) && isOk) {
        mPort = host_port.toInt();
    }
    mLogin = readProfileData(qsl("login"));

    const QString val = readProfileData(qsl("autoreconnect"));
    setAutoReconnect(!val.isEmpty() && val.toInt() == Qt::Checked);

    // This settings also need to be configured, note that the only time not to
    // save the setting is on profile loading:
    mTelnet.setEncoding(readProfileData(qsl("encoding")).toUtf8(), false);

    auto optin = readProfileData(qsl("discordserveroptin"));
    if (!optin.isEmpty()) {
        mDiscordDisableServerSide = optin.toInt() == Qt::Unchecked ? true : false;
    }

    if (mudlet::self()->storingPasswordsSecurely()) {
        loadSecuredPassword();
    } else {
        QString password{readProfileData(qsl("password"))};
        if (!password.isEmpty()) {
            setPass(password);
            password.clear();
        }
    }

    if (mudlet::self()->publicTestVersion) {
        thankForUsingPTB();
    }

    if (mudlet::self()->smFirstLaunch) {
        QTimer::singleShot(0, this, [this]() {
            mpConsole->mpCommandLine->setPlaceholderText(tr("Text to send to the game"));
        });
    }

    connect(&mTelnet, &cTelnet::signal_disconnected, this, [this](){ purgeTimer.start(1min); });
    connect(&mTelnet, &cTelnet::signal_connected, this, [this](){ purgeTimer.stop(); });
    connect(&purgeTimer, &QTimer::timeout, this, &Host::slot_purgeTemps);

    // enable by default in case of offline connection; if the profile connects - timer will be disabled
    purgeTimer.start(1min);

    auto i = mudlet::self()->mpShortcutsManager->iterator();
    while (i.hasNext()) {
        auto entry = i.next();
        profileShortcuts.insert(entry, new QKeySequence(*mudlet::self()->mpShortcutsManager->getSequence(entry)));
    }

    auto settings = mudlet::self()->getQSettings();
    const auto interval = settings->value("autosaveIntervalMinutes", 2).toInt();
    startMapAutosave(interval);
}

Host::~Host()
{
    if (mpDockableMapWidget) {
        mpDockableMapWidget->deleteLater();
    }
    mErrorLogStream.flush();
    mErrorLogFile.close();
    // Since this is a destructor, it's risky to rely on member variables within the destructor itself.
    // To avoid this, we can pass the profile name as an argument instead of accessing it
    // directly as a member variable. This ensures the destructor doesn't depend on the
    // object's state being valid.

    TDebug::removeHost(this, mHostName);
}

void Host::forceClose()
{
    if (!mpConsole) {
        // The main console has gone away so we must already be dying
        return;
    }

    mForcedClose = true;
    postMessage(tr("[ ALERT ] - This profile will now save and close."));
    // Ensure the above is displayed before proceeding...
    qApp->processEvents();

    // Because we have set the mForcedClose flag above a requestClose()
    // afterwards WILL succeed
}

// Returns true if we are closing down
bool Host::requestClose()
{
    if (!mpConsole) {
        // The main console has gone away so we must already be dying
        return true;
    }

    // This call ends up at the (void) TMainConsole::closeEvent(...) and causes
    // the close() method called here to return a true if the event was
    // accepted:
    if (!mpConsole->close()) {
        // Nope the user doesn't want this to close - and it won't have set its
        // mEnableClose flag:
        return false;
    }

    // The above will have initiated a save of the profile (and its map) if it
    // got a true returned from the TMainConsole::close() call.

    // Get rid of any dialogs we might have open:
    closeChildren();

    // This time this will succeed as mEnableClose is set:
    mpConsole->close();
    return true;
}

void Host::closeChildren()
{
    mIsClosingDown = true;
    const auto hostToolBarMap = getActionUnit()->getToolBarList();
    // disconnect before removing objects from memory as sysDisconnectionEvent needs that stuff.
    if (mSslTsl) {
        mTelnet.abortConnection();
    } else {
        mTelnet.disconnectIt();
    }

    stopAllTriggers();

    if (mpEditorDialog) {
        mpEditorDialog->setAttribute(Qt::WA_DeleteOnClose);
        mpEditorDialog->close();
        mpEditorDialog = nullptr;
    }

    for (const QString& consoleName : mpConsole->mSubConsoleMap.keys()) {
        // Only user-windows will be in this map:
        auto pD = mpConsole->mDockWidgetMap.value(consoleName);
        // All User TConsole's will be in this map:
        auto pC = mpConsole->mSubConsoleMap.value(consoleName);
        if (pD) {
            // This undocks the widget
            mudlet::self()->removeDockWidget(pD);
        }

        // This will remove both what pD and pC point at from their respective
        // QMaps:
        pC->close();
    }

    if (mpNotePad) {
        mpNotePad->save();
        mpNotePad->setAttribute(Qt::WA_DeleteOnClose);
        mpNotePad->close();
        mpNotePad = nullptr;
    }

    for (TToolBar* pTB : hostToolBarMap) {
        if (pTB) {
            pTB->setAttribute(Qt::WA_DeleteOnClose);
            pTB->deleteLater();
        }
    }

    // close IRC client window if it is open.
    if (mpDlgIRC) {
        mpDlgIRC->setAttribute(Qt::WA_DeleteOnClose);
        mpDlgIRC->deleteLater();
        mpDlgIRC = nullptr;
    }
}

void Host::loadMap()
{
    qDebug() << "Host::loadMap() - restore map case 4.";
    if (mpMap->restore(QString(), false)) {
        mpMap->audit();
        if (mpMap->mpMapper) {
            mpMap->mpMapper->mp2dMap->init();
            mpMap->mpMapper->updateAreaComboBox();
            mpMap->mpMapper->resetAreaComboBoxToPlayerRoomArea();
            mpMap->mpMapper->show();
        }
    }
}

void Host::startMapAutosave(const int interval)
{
    if (interval > 0) {
        startTimer(interval * 1min);
    }
}

void Host::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event);

    autoSaveMap();
}

void Host::autoSaveMap()
{
    if (mpMap->isUnsaved()) {
#if defined(DEBUG_MAPAUTOSAVE)
        QString nowString = QDateTime::currentDateTimeUtc().toString("HH:mm:ss.zzz");
#endif
        if (!mIsProfileLoadingSequence) {
#if defined(DEBUG_MAPAUTOSAVE)
            qDebug().nospace().noquote() << "Host::autoSaveMap() INFO - map auto save initiated at:" << nowString << ".";
#endif
            // FIXME: https://github.com/Mudlet/Mudlet/issues/6316 - unchecked return value - we are not handling a failure to save the map!
            mpConsole->saveMap(mudlet::getMudletPath(mudlet::profileMapPathFileName, mHostName, qsl("autosave.dat")));
#if defined(DEBUG_MAPAUTOSAVE)
        } else {
            qDebug().nospace().noquote() << "Host::autoSaveMap() INFO - map auto save requested at:" << nowString << " but declined whilst \"Host::mIsProfileLoadingSequence\" flag set.";
#endif
        }
    }
}

void Host::loadPackageInfo()
{
    const QStringList packages = mInstalledPackages;
    for (int i = 0; i < packages.size(); i++) {
        const QString packagePath{mudlet::self()->getMudletPath(mudlet::profilePackagePath, getName(), packages.at(i))};
        const QDir dir(packagePath);
        if (dir.exists(qsl("config.lua"))) {
            getPackageConfig(dir.absoluteFilePath(qsl("config.lua")));
        }
    }
}

void Host::createModuleBackup(const QString &filename, const QString &saveName)
{
    const QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd#HH-mm-ss");
    QFile::copy(filename, saveName + time);
}

void Host::writeModule(const QString &moduleName, const QString &filename)
{
    QString xml_filename = filename;
    if (filename.endsWith(qsl("mpackage"), Qt::CaseInsensitive) || filename.endsWith(qsl("zip"), Qt::CaseInsensitive)) {
        xml_filename = mudlet::getMudletPath(mudlet::profilePackagePathFileName, mHostName, moduleName);
    }
    auto writer = new XMLexport(this);
    writers.insert(xml_filename, writer);
    writer->writeModuleXML(moduleName, xml_filename);
    updateModuleZips(filename, moduleName);
}

void Host::waitForAsyncXmlSave()
{
    // writers and futures are copied to prevent deletion during for loop (which would mean crash)
    auto myWriters = writers;
    for (auto& writer : myWriters) {
        auto myFutures = writer->saveFutures;
        for (auto& future : myFutures) {
            future.waitForFinished();
        }
    }
}

void Host::saveModules(bool backup)
{
    QMapIterator<QString, QStringList> it(modulesToWrite);
    mModulesToSync.clear();
    const QString savePath = mudlet::getMudletPath(mudlet::moduleBackupsPath);
    auto savePathDir = QDir(savePath);
    if (!savePathDir.exists()) {
        savePathDir.mkpath(savePath);
    }
    while (it.hasNext()) {
        it.next();
        QStringList entry = it.value();
        const QString moduleName = it.key();
        const QString filename = entry[0];

        if (backup) {
            createModuleBackup(filename, savePath + moduleName);
        }
        writeModule(moduleName, filename);
        if (entry[1].toInt()) {
            mModulesToSync << moduleName;
        }
    }
    modulesToWrite.clear();
}

void Host::reloadModules()
{
    //synchronize modules across sessions
    for (auto otherHost : mudlet::self()->getHostManager()) {
        if (otherHost == this || !otherHost->mpConsole) {
            continue;
        }
        const QMap<QString, int>& modulePri = otherHost->mModulePriorities;
        QMap<int, QStringList> moduleOrder;

        auto modulePrioritiesIt = modulePri.constBegin();
        while (modulePrioritiesIt != modulePri.constEnd()) {
            moduleOrder[modulePrioritiesIt.value()].append(modulePrioritiesIt.key());
            ++modulePrioritiesIt;
        }

        QMapIterator<int, QStringList> it(moduleOrder);
        while (it.hasNext()) {
            it.next();
            const QStringList moduleList = it.value();
            for (auto moduleName : moduleList) {
                if (mModulesToSync.contains(moduleName)) {
                    otherHost->reloadModule(moduleName, mHostName);
                }
            }
        }
    }
    mModulesToSync.clear();
}

void Host::updateModuleZips(const QString& zipName, const QString& moduleName)
{
    if (!(zipName.endsWith(qsl("mpackage"), Qt::CaseInsensitive) || zipName.endsWith(qsl("zip"), Qt::CaseInsensitive))) {
        return;
    }
    zip* zipFile = nullptr;
    const QString packagePathName = mudlet::getMudletPath(mudlet::profilePackagePath, mHostName, moduleName);
    const QString filename_xml = mudlet::getMudletPath(mudlet::profilePackagePathFileName, mHostName, moduleName);
    int err = 0;
    zipFile = zip_open(zipName.toStdString().c_str(), ZIP_CREATE, &err);
    if (!zipFile) {
        return;
    }
    const QDir packageDir = QDir(packagePathName);
    if (!packageDir.exists()) {
        packageDir.mkpath(packagePathName);
    }
    const int xmlIndex = zip_name_locate(zipFile, qsl("%1.xml").arg(moduleName).toUtf8().constData(), ZIP_FL_ENC_GUESS);
    zip_delete(zipFile, xmlIndex);
    struct zip_source* s = zip_source_file(zipFile, filename_xml.toUtf8().constData(), 0, -1);
    if (mudlet::smDebugMode && s == nullptr) {
        //: This error message will appear when the xml file inside the module zip cannot be updated for some reason.
        TDebug(QColor(Qt::white), QColor(Qt::red)) << tr("Failed to open xml file \"%1\" inside module %2 to update it. Error message was: \"%3\".")
                                                              .arg(filename_xml, zipName, zip_strerror(zipFile));
    }
    err = zip_file_add(zipFile, qsl("%1.xml").arg(moduleName).toUtf8().constData(), s, ZIP_FL_ENC_UTF_8 | ZIP_FL_OVERWRITE);

    if (zipFile) {
        err = zip_close(zipFile);
    }

    if (mudlet::smDebugMode && err == -1) {
        //: This error message will appear when a module is saved as package but cannot be done for some reason.
        TDebug(QColor(Qt::white), QColor(Qt::red)) << tr("Failed to save \"%1\" to module \"%2\". Error message was: \"%3\".")
                                                              .arg(moduleName, zipName, zip_strerror(zipFile));
    }
}

void Host::reloadModule(const QString& syncModuleName, const QString& syncingFromHost)
{
    //Wait till profile is finished saving
    if (syncingFromHost.isEmpty() && currentlySavingProfile()) {
        //create a dummy object to singleshot connect (disconnect/delete after execution)
        QObject* obj = new QObject(this);
        connect(this, &Host::profileSaveFinished, obj, [=]() {
            reloadModule(syncModuleName);
            obj->deleteLater();
        });
        return;
    }
    QMap<QString, QStringList> installedModules = mInstalledModules;
    QMapIterator<QString, QStringList> moduleIterator(installedModules);
    while (moduleIterator.hasNext()) {
        moduleIterator.next();
        const auto& moduleName = moduleIterator.key();
        const auto& moduleLocation = moduleIterator.value()[0];
        QString fileName = moduleLocation;
        if (moduleName == syncModuleName) {
            if (!syncingFromHost.isEmpty() && (fileName.endsWith(qsl(".zip"), Qt::CaseInsensitive) || fileName.endsWith(qsl(".mpackage"), Qt::CaseInsensitive))) {
                uninstallPackage(moduleName, 2);
                fileName = mudlet::getMudletPath(mudlet::profilePackagePathFileName, syncingFromHost, moduleName);
                installPackage(fileName, 2);
                QStringList moduleEntry;
                moduleEntry << moduleLocation;
                moduleEntry << qsl("0");
                mInstalledModules[moduleName] = moduleEntry;
                //Write the module to the own profile directory to save it on restart
                fileName = mudlet::getMudletPath(mudlet::profilePackagePathFileName, mHostName, moduleName);
                auto writer = new XMLexport(this);
                writers.insert(fileName, writer);
                writer->writeModuleXML(moduleName, fileName, true);
            } else {
                uninstallPackage(moduleName, 2);
                installPackage(fileName, 2);
            }
        }
    }
    //iterate through mInstalledModules again and reset the entry flag to be correct.
    //both the installedModules and mInstalled should be in the same order now as well
    moduleIterator.toFront();
    while (moduleIterator.hasNext()) {
        moduleIterator.next();
        const QStringList entry = installedModules[moduleIterator.key()];
        mInstalledModules[moduleIterator.key()] = entry;
    }
}

std::pair<bool, QString> Host::changeModuleSync(const QString& moduleName, const QLatin1String& value)
{
    if (moduleName.isEmpty()) {
        return {false, qsl("module name cannot be an empty string")};
    }

    if (mInstalledModules.contains(moduleName)) {
        QStringList moduleStringList = mInstalledModules[moduleName];
        moduleStringList[1] = value;
        mInstalledModules[moduleName] = moduleStringList;
        return {true, QString()};
    }
    return {false, qsl("module name '%1' not found").arg(moduleName)};
}

std::pair<bool, QString> Host::getModuleSync(const QString& moduleName)
{
    if (moduleName.isEmpty()) {
        return {false, qsl("module name cannot be an empty string")};
    }

    if (mInstalledModules.contains(moduleName)) {
        QStringList moduleStringList = mInstalledModules[moduleName];
        return {true, moduleStringList[1]};
    }
    return {false, qsl("module name '%1' not found").arg(moduleName)};
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
    getScriptUnit()->compileAll(true);

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
    QString directory_xml;
    if (saveFolder.isEmpty()) {
        directory_xml = mudlet::getMudletPath(mudlet::profileXmlFilesPath, getName());
    } else {
        directory_xml = saveFolder;
    }

    QString filename_xml;
    if (saveName.isEmpty()) {
        filename_xml = qsl("%1/%2.xml").arg(directory_xml, QDateTime::currentDateTime().toString(qsl("yyyy-MM-dd#HH-mm-ss")));
    } else {
        filename_xml = qsl("%1/%2.xml").arg(directory_xml, saveName);
    }

    if (!mLoadedOk) {
        return {false, filename_xml, qsl("profile was not loaded correctly to begin with")};
    }

    if (mIsProfileLoadingSequence) {
        //If we're inside of profile loading sequence modules might not be loaded yet, thus we can accidentally clear their contents
        return {false, filename_xml, qsl("profile loading is in progress")};
    }

    const QDir dir_xml;
    if (!dir_xml.exists(directory_xml)) {
        dir_xml.mkpath(directory_xml);
    }

    if (currentlySavingProfile()) {
        return {false, QString(), qsl("a save is already in progress")};
    }

    if (saveFolder.isEmpty() && saveName.isEmpty()) {
        // This is likely to be the save as the profile is closed
        qDebug().noquote().nospace() << "Host::saveProfile(...) INFO - called with no saveFolder or saveName arguments for profile '"
                                     << mHostName
                                     << "' so assuming it is an end of session save and the TCommandLines' histories need saving...";
        emit signal_saveCommandLinesHistory();
    }

    emit profileSaveStarted();
    qApp->processEvents();

    auto writer = new XMLexport(this);
    writers.insert(qsl("profile"), writer);
    writer->exportHost(filename_xml);
    mWritingHostAndModules = true;
    auto watcher = new QFutureWatcher<void>;
    mModuleFuture = QtConcurrent::run([=]() {
        //wait for the host xml to be ready before starting to sync modules
        waitForAsyncXmlSave();
        saveModules(saveName != qsl("autosave"));
    });
    connect(watcher, &QFutureWatcher<void>::finished, this, [=]() {
        // reload, or queue module reload for when xml is ready
        if (syncModules) {
            reloadModules();
        }
        mWritingHostAndModules = false;
    });
    watcher->setFuture(mModuleFuture);
    return {true, filename_xml, QString()};
}

// exports without the host settings for some reason
std::tuple<bool, QString, QString> Host::saveProfileAs(const QString& file)
{
    emit profileSaveStarted();
    qApp->processEvents();

    if (currentlySavingProfile()) {
        return {false, QString(), qsl("a save is already in progress")};
    }

    auto writer = new XMLexport(this);
    writers.insert(qsl("profile"), writer);
    writer->exportProfile(file);
    return {true, file, QString()};
}

void Host::xmlSaved(const QString& xmlName)
{
    if (writers.contains(xmlName)) {
        auto writer = writers.take(xmlName);
        writer->deleteLater();
    }

    if (writers.empty()) {
        emit profileSaveFinished();
    }
}

bool Host::currentlySavingProfile()
{
    return (mWritingHostAndModules || !writers.empty());
}

void Host::waitForProfileSave()
{
    waitForAsyncXmlSave();
    if (mModuleFuture.isRunning()) {
        mModuleFuture.waitForFinished();
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

    auto urlValue = json.value(qsl("url"));
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

        TEvent event{};
        event.mArgumentList.append(qsl("sysSettingChanged"));
        event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        event.mArgumentList.append(qsl("main window font"));
        event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        event.mArgumentList.append(mDisplayFont.family());
        event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        event.mArgumentList.append(QString::number(mDisplayFont.pointSize()));
        event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
        raiseEvent(event);
    }

    if (mpEditorDialog && mpEditorDialog->mpErrorConsole) {
        mpEditorDialog->mpErrorConsole->setFont(mDisplayFont.family());
        mpEditorDialog->mpErrorConsole->setFontSize(mDisplayFont.pointSize());
    }
    if (mudlet::self()->smpDebugArea) {
        mudlet::self()->smpDebugConsole->setFont(mDisplayFont.family());
        mudlet::self()->smpDebugConsole->setFontSize(mDisplayFont.pointSize());
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
    const QUrl url = QUrl(mediaUrl);

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
    const QUrl url = QUrl(mediaUrl);

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
        return {false, qsl("specified font is invalid (its letters have 0 width)")};
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
    for (const int i : std::as_const(mpMap->mPathList)) {
        const QString n = QString::number(i);
        pathList.append(n);
    }
    QStringList directionList = mpMap->mDirList;
    QStringList weightList;
    for (const int stepWeight : std::as_const(mpMap->mWeightList)) {
        totalWeight += stepWeight;
        const QString n = QString::number(stepWeight);
        weightList.append(n);
    }
    QString tableName = qsl("speedWalkPath");
    mLuaInterpreter.set_lua_table(tableName, pathList);
    tableName = qsl("speedWalkDir");
    mLuaInterpreter.set_lua_table(tableName, directionList);
    tableName = qsl("speedWalkWeight");
    mLuaInterpreter.set_lua_table(tableName, weightList);
    return totalWeight;
}

bool Host::checkForMappingScript()
{
    // the mapper script reminder is only shown once
    // because it is too difficult and error prone (->proper script sequence)
    // to disable this message
    const bool ret = (mLuaInterpreter.check_for_mappingscript() || mHaveMapperScript);
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

        connect(dialog, &QDialog::accepted, mudlet::self(), &mudlet::slot_openMappingScriptsPage);

        dialog->show();
        dialog->raise();
        dialog->activateWindow();
    }
}

bool Host::checkForCustomSpeedwalk()
{
    const bool ret = mLuaInterpreter.check_for_custom_speedwalk();
    return ret;
}

void Host::startSpeedWalk()
{
    const int totalWeight = assemblePath();
    Q_UNUSED(totalWeight);
    const QString f = qsl("doSpeedWalk");
    const QString n = QString();
    mLuaInterpreter.call(f, n);
}

void Host::startSpeedWalk(int sourceRoom, int targetRoom)
{
    const QString sourceName = qsl("speedWalkFrom");
    mLuaInterpreter.set_lua_integer(sourceName, sourceRoom);
    const QString targetName = qsl("speedWalkTo");
    mLuaInterpreter.set_lua_integer(targetName, targetRoom);
    const QString f = qsl("doSpeedWalk");
    const QString n = QString();
    mLuaInterpreter.call(f, n);
}

void Host::updateDisplayDimensions()
{
    mTelnet.checkNAWS();
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
        return qMakePair(qsl("Google"), mSearchEngineData.value(qsl("Google")));
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
        commandList = cmd.split(QString(mCommandSeparator), Qt::SkipEmptyParts);
    } else if (!cmd.isEmpty()) {
        // don't split command if the command separator is blank
        commandList << cmd;
    }

        // allow sending blank commands

    if (!dontExpandAliases && commandList.empty()) {
        QString payload(QChar::LineFeed);
        mTelnet.sendData(payload);
        return;
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
        return qMakePair(0, qsl("unable to create a stopwatch at this time"));
    }

    if (!mStopWatchMap.isEmpty() && !name.isEmpty()) {
        QMapIterator<int, stopWatch*> itStopWatch(mStopWatchMap);
        while (itStopWatch.hasNext()) {
            itStopWatch.next();
            if (itStopWatch.value()->name() == name) {
                return qMakePair(0, qsl("stopwatch with id %1 called '%2' already exists").arg(QString::number(itStopWatch.key()), name));
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
    const int total = stopWatchIdList.size();
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
        return qMakePair(false, qsl("stopwatch with id %1 not found").arg(id));
    }
}

QPair<bool, QString> Host::startStopWatch(const QString& name)
{
    auto watchId = findStopWatchId(name);
    if (!watchId) {
        if (name.isEmpty()) {
            return qMakePair(false, QLatin1String("no unnamed stopwatches found"));
        } else {
            return qMakePair(false, qsl("stopwatch with name '%1' not found").arg(name));
        }
    }

    auto pStopWatch = mStopWatchMap.value(watchId);
    if (Q_LIKELY(pStopWatch)) {
        if (pStopWatch->start()) {
            return qMakePair(true, QString());
        }

        return qMakePair(false, qsl("stopwatch with name '%1' (id:%2) was already running").arg(name, QString::number(watchId)));
    }

    // This should, indeed, be:
    Q_UNREACHABLE();
    return qMakePair(false, qsl("stopwatch with name '%1' (id:%2) not found").arg(name, QString::number(watchId)));
}

QPair<bool, QString> Host::startStopWatch(int id)
{
    auto pStopWatch = mStopWatchMap.value(id);
    if (pStopWatch) {
        if (pStopWatch->start()) {
            return qMakePair(true, QString());
        }

        return qMakePair(false, qsl("stopwatch with id %1 was already running").arg(id));
    }

    return qMakePair(false, qsl("stopwatch with id %1 not found").arg(id));
}

QPair<bool, QString> Host::stopStopWatch(const QString& name)
{
    auto watchId = findStopWatchId(name);
    if (!watchId) {
        if (name.isEmpty()) {
            return qMakePair(false, QLatin1String("no unnamed stopwatches found"));
        } else {
            return qMakePair(false, qsl("stopwatch with name '%1' not found").arg(name));
        }
    }

    auto pStopWatch = mStopWatchMap.value(watchId);
    if (Q_LIKELY(pStopWatch)) {
        if (pStopWatch->stop()) {
            return qMakePair(true, QString());
        }

        return qMakePair(false, qsl("stopwatch with name '%1' (id:%2) was already stopped").arg(name, QString::number(watchId)));
    }

    // This should, indeed, be:
    Q_UNREACHABLE();
    return qMakePair(false, qsl("stopwatch with name '%1' (id:%2) not found").arg(name, QString::number(watchId)));
}

QPair<bool, QString> Host::stopStopWatch(const int id)
{
    auto pStopWatch = mStopWatchMap.value(id);
    if (pStopWatch) {
        if (pStopWatch->stop()) {
            return qMakePair(true, QString());
        }

        return qMakePair(false, qsl("stopwatch with id %1 was already stopped").arg(id));
    }

    return qMakePair(false, qsl("stopwatch with id %1 not found").arg(id));
}

QPair<bool, QString> Host::resetStopWatch(const QString& name)
{
    auto watchId = findStopWatchId(name);
    if (!watchId) {
        if (name.isEmpty()) {
            return qMakePair(false, QLatin1String("no unnamed stopwatches found"));
        } else {
            return qMakePair(false, qsl("stopwatch with name '%1' not found").arg(name));
        }
    }

    auto pStopWatch = mStopWatchMap.value(watchId);
    if (Q_LIKELY(pStopWatch)) {
        if (pStopWatch->reset()) {
            return qMakePair(true, QString());
        }

        if (name.isEmpty()) {
            return qMakePair(false, qsl("the first unnamed stopwatch (id:%1) was already reset").arg(watchId));
        } else {
            return qMakePair(false, qsl("stopwatch with name '%1' (id:%2) was already reset").arg(name, QString::number(watchId)));
        }
    }

    // This should, indeed, be:
    Q_UNREACHABLE();
    return qMakePair(false, qsl("stopwatch with name '%1' (id:%2) not found").arg(name, QString::number(watchId)));
}

QPair<bool, QString> Host::resetStopWatch(const int id)
{
    auto pStopWatch = mStopWatchMap.value(id);
    if (pStopWatch) {
        if (pStopWatch->reset()) {
            return qMakePair(true, QString());
        }

        return qMakePair(false, qsl("stopwatch with id %1 was already reset").arg(id));
    }

    return qMakePair(false, qsl("stopwatch with id %1 not found").arg(id));
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

    return qMakePair(false, qsl("stopwatch with id %1 not found").arg(id));
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
                    return qMakePair(false, qsl("the name '%1' is already in use for another stopwatch (id:%2)").arg(newName, QString::number(itStopWatch.key())));
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

    return qMakePair(false, qsl("stopwatch with id %1 not found").arg(id));
}

QPair<bool, QString> Host::setStopWatchName(const QString& currentName, const QString& newName)
{
    stopWatch* pStopWatch = nullptr;
    // Scan through existing names, in ascending id order
    QList<int> stopWatchIdList = mStopWatchMap.keys();
    const int total = stopWatchIdList.size();
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
            return qMakePair(false, qsl("stopwatch with name '%1' not found").arg(currentName));
        }
    }

    if (isAlreadyUsed) {
        return qMakePair(false, qsl("the name '%1' is already in use for another stopwatch (id:%2)").arg(newName, QString::number(alreadyUsedId)));
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

    static const QString star = qsl("*");

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
        const QStringList functionsList = mAnonymousEventHandlerFunctions.value(pE.mArgumentList.at(0));
        for (int i = 0, total = functionsList.size(); i < total; ++i) {
            mLuaInterpreter.callEventHandler(functionsList.at(i), pE);
        }
    }
    if (mAnonymousEventHandlerFunctions.contains(star)) {
        const QStringList functionsList = mAnonymousEventHandlerFunctions.value(star);
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

std::pair<bool, QString> Host::installPackage(const QString& fileName, int module)
{
    // As the pointer to dialog is only used now WITHIN this method and this
    // method can be re-entered, it is best to use a local rather than a class
    // pointer just in case we accidentally re-enter this method in the future.
    QDialog* pUnzipDialog = nullptr;

    //     Module notes:
    //     For the module install, a module flag of 0 is a package,
    // a flag of 1 means the module is being installed for the first time via the UI,
    // a flag of 2 means the module is being synced (so it's "installed" already),
    // a flag of 3 means the module is being installed from a script.
    //     This separation is necessary to be able to reuse code while avoiding infinite loops from script installations.

    if (fileName.isEmpty()) {
        return {false, qsl("no package file was actually given")};
    }

    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        return {false, qsl("could not open file '%1").arg(fileName)};
    }

    QString packageName = sanitizePackageName(fileName);
    if (module) {
        if ((module == 2) && (mActiveModules.contains(packageName))) {
            uninstallPackage(packageName, 2);
        } else if ((module == 3) && (mActiveModules.contains(packageName))) {
            return {false, qsl("module %1 is already installed").arg(packageName)}; //we're already installed
        }
    } else {
        if (mInstalledPackages.contains(packageName)) {
            return {false, qsl("package %1 is already installed").arg(packageName)};
        }
    }
    //the extra module check is needed here to prevent infinite loops from script loaded modules
    if (mpEditorDialog && module != 3) {
        mpEditorDialog->doCleanReset();
    }
    QFile file2;
    if (fileName.endsWith(qsl(".zip"), Qt::CaseInsensitive) || fileName.endsWith(qsl(".mpackage"), Qt::CaseInsensitive)) {
        const QString _home = mudlet::getMudletPath(mudlet::profileHomePath, getName());
        const QString _dest = mudlet::getMudletPath(mudlet::profilePackagePath, getName(), packageName);
        // home directory for the PROFILE
        const QDir _tmpDir(_home);
        // directory to store the expanded archive file contents
        const bool mkpathSuccessful = _tmpDir.mkpath(_dest);
        if (!mkpathSuccessful) {
            return {false, qsl("could not create destination folder")};
        }

        QUiLoader loader(this);
        QFile uiFile(qsl(":/ui/package_manager_unpack.ui"));
        uiFile.open(QFile::ReadOnly);
        pUnzipDialog = dynamic_cast<QDialog*>(loader.load(&uiFile, nullptr));
        uiFile.close();
        if (!pUnzipDialog) {
            return {false, qsl("could not load unpacking progress dialog")};
        }

        auto * pLabel = pUnzipDialog->findChild<QLabel*>(qsl("label"));
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

        auto unzipSuccessful = mudlet::unzip(fileName, _dest, _tmpDir);
        pUnzipDialog->deleteLater();
        pUnzipDialog = nullptr;
        if (!unzipSuccessful) {
            return {false, qsl("could not unzip package")};
        }

        // requirements for zip packages:
        // - packages must be compressed in zip format
        // - file extension should be .mpackage (though .zip is accepted)
        // - there can only be a single xml file per package
        // - the xml file must be located in the root directory of the zip package. example: myPack.zip contains: the folder images and the file myPack.xml

        QDir _dir(_dest);
        // before we start importing xmls in, see if the config.lua manifest file exists
        // - if it does, update the packageName from it
        if (_dir.exists(qsl("config.lua"))) {
            // read in the new packageName from Lua. Should be expanded in future to whatever else config.lua will have
            readPackageConfig(_dir.absoluteFilePath(qsl("config.lua")), packageName, module > 0);
            // now that the packageName changed, redo relevant checks to make sure it's still valid
            if (module) {
                if (mActiveModules.contains(packageName)) {
                    uninstallPackage(packageName, 2);
                }
            } else {
                if (mInstalledPackages.contains(packageName)) {
                    // cleanup and quit if already installed
                    removeDir(_dir.absolutePath(), _dir.absolutePath());
                    return {false, qsl("package %1 is already installed").arg(packageName)};
                }
            }
            // continuing, so update the folder name on disk
            const QString newpath(qsl("%1/%2").arg(_home, packageName));
            _dir.rename(_dir.absolutePath(), newpath);
            _dir = QDir(newpath);
        }
        QStringList _filterList;
        _filterList << qsl("*.xml") << qsl("*.trigger");
        const QFileInfoList entries = _dir.entryInfoList(_filterList, QDir::Files);
        for (auto& entry : entries) {
            file2.setFileName(entry.absoluteFilePath());
            file2.open(QFile::ReadOnly | QFile::Text);
            XMLimport reader(this);
            if (module) {
                QStringList moduleEntry;
                moduleEntry << fileName;
                moduleEntry << qsl("0");
                mInstalledModules[packageName] = moduleEntry;
                mActiveModules.append(packageName);
            } else {
                mInstalledPackages.append(packageName);
            }
            reader.importPackage(&file2, packageName, module); // TODO: Missing false return value handler
            file2.close();
        }
    } else {
        file2.setFileName(fileName);
        file2.open(QFile::ReadOnly | QFile::Text);
        //mInstalledPackages.append( packageName );
        XMLimport reader(this);
        if (module) {
            QStringList moduleEntry;
            moduleEntry << fileName;
            moduleEntry << qsl("0");
            mInstalledModules[packageName] = moduleEntry;
            mActiveModules.append(packageName);
        } else {
            mInstalledPackages.append(packageName);
        }
        reader.importPackage(&file2, packageName, module); // TODO: Missing false return value handler
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

    if (mpPackageManager) {
        mpPackageManager->resetPackageTable();
    }


    return {true, QString()};
}

QString Host::sanitizePackageName(const QString packageName) const {
    auto tempName = packageName.section(qsl("/"), -1);
    tempName.remove(qsl(".trigger"), Qt::CaseInsensitive);
    tempName.remove(qsl(".xml"), Qt::CaseInsensitive);
    tempName.remove(qsl(".zip"), Qt::CaseInsensitive);
    tempName.remove(qsl(".mpackage"), Qt::CaseInsensitive);
    tempName.remove(QLatin1Char('\\'));
    return tempName;
}

// credit: http://john.nachtimwald.com/2010/06/08/qt-remove-directory-and-its-contents/
bool Host::removeDir(const QString& dirName, const QString& originalPath)
{
    bool result = true;
    const QDir dir(dirName);
    if (dir.exists(dirName)) {
        for (QFileInfo const& info : dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
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

    // block packages/modules from being uninstalled while a profile save is in progress
    // just so the save mechanism doesn't get surprised with something getting removed from memory under its feet
    if (currentlySavingProfile()) {
        return false;
    }

    if (module) {
        if (!mInstalledModules.contains(packageName)) {
            return false;
        }
    } else {
        if (!mInstalledPackages.contains(packageName)) {
            return false;
        }
    }
    //module == 2 seems to be only used for reloading/syncing
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
    mudlet::self()->mFontManager.unloadFonts(packageName);
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

    const QString dest = mudlet::getMudletPath(mudlet::profilePackagePath, getName(), packageName);
    removeDir(dest, dest);

    // ensure only one timer is running in case multiple modules are uninstalled at once
    if (!mSaveTimer.has_value() || !mSaveTimer.value()) {
        mSaveTimer = true;
        // save the profile on the next Qt main loop cycle in order for the asyncronous save mechanism
        // not to try to write to disk a package/module that just got uninstalled and removed from memory
        QTimer::singleShot(0, this, [this]() {
            mSaveTimer = false;
            if (auto [ok, filename, error] = saveProfile(); !ok) {
                qDebug() << qsl("Host::uninstallPackage: Couldn't save '%1' to '%2' because: %3").arg(getName(), filename, error);
            }
        });
    }

    //NOW we reset if we're uninstalling a module
    if (mpEditorDialog && module == 3) {
        mpEditorDialog->doCleanReset();
    }
    if (mpPackageManager) {
        mpPackageManager->resetPackageTable();
    }
    return true;
}

void Host::readPackageConfig(const QString& luaConfig, QString& packageName, bool isModule)
{
    const QString newName = getPackageConfig(luaConfig, isModule);
    if (!newName.isEmpty()) {
        packageName = sanitizePackageName(newName);
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
        /*
         * We also have to explicit set the codec to use whilst reading the file
         * as otherwise QTextCodec::codecForLocale() is used which for Qt5
         * might be a local8Bit codec that thus will not handle all the
         * characters contained in Unicode. In Qt6 the default is UTF-8.
         */
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        in.setCodec(QTextCodec::codecForName("UTF-8"));
#endif
        while (!in.atEnd()) {
            strings += in.readLine();
        }
        configFile.close();
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

    if (mudlet::smDebugMode) {
        TDebug(QColor(Qt::white), QColor(Qt::red)) << "LUA: " << reason.c_str() << " in " << luaConfig << " ERROR:" << e.c_str() << "\n" >> 0;
    }

    lua_pop(L, -1);
    lua_close(L);
    return QString();
}

// writeProfileIniData(...) and readProfileIniData(...) might eventually
// replace writeProfileData(...) and readProfileData(...) but for now are just
// used to store some information about one or more TCommandLine's mHistoryData:
bool Host::writeProfileIniData(const QString& item, const QString& what)
{
    QSettings settings(mudlet::getMudletPath(mudlet::profileDataItemPath, getName(), qsl("profile.ini")), QSettings::IniFormat);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // This will ensure compatibility going forward and backward
    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
#endif
    settings.setValue(item, what);
    settings.sync();
    switch (settings.status()) {
    case QSettings::NoError:
        return true;
    case QSettings::FormatError:
        qWarning().nospace().noquote() << "Host::writeProfileIniData(\"" << item << "\", \"" << what << "\") ERROR - failed to save this detail, reason: \"Format error\".";
        return false;
    case QSettings::AccessError:
        qWarning().nospace().noquote() << "Host::writeProfileIniData(\"" << item << "\", \"" << what << "\") ERROR - failed to save this detail, reason: \"Access error\".";
        return false;
    }
    Q_UNREACHABLE();
}

QString Host::readProfileIniData(const QString& item)
{
    QSettings settings(mudlet::getMudletPath(mudlet::profileDataItemPath, getName(), qsl("profile.ini")), QSettings::IniFormat);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // This will ensure compatibility going forward and backward
    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
#endif
    return settings.value(item).toString();
}

// This function retrieves command line history settings based on the given
// command line type and name. It reads the saveCommands setting from the
// profile.ini file, which is intended to replace all other single data item
// files in the profile's home directory. For the main command line, a
// predefined file name is used, while for any other command line, the function
// first looks for a mapping from a number-suffixed file to a command line
// name. If the mapping is not found, a new file name and default setting for
// saveCommands is created, stored, and returned.
// Because '/' and '\\' are used by the QSettings class in key names for
// special purposes we MUST filter them out in the name, we'll replace them
// with '_'s:
std::tuple<QString, bool> Host::getCmdLineSettings(const TCommandLine::CommandLineType type, const QString& name)
{
    if (type == TCommandLine::MainCommandLine) {
        // This one does not need the name to be kept in a QSettings but we
        // still need to retrieve the other setting:
        auto saveCommands = static_cast<bool>(readProfileIniData(qsl("CommandLines/SaveHistory/main")).compare(qsl("false"), Qt::CaseInsensitive));
        return {qsl("command_history_main"), saveCommands};
    }
    QString localName{name};
    localName.replace(QRegularExpression(qsl("[\\/]")), qsl("_"));
    // We use a '/' in the name as that denotes a grouping (section) within the
    // QSetting's (INI) format with the left-most side of the first '/' as a
    // section header. They actually get converted to `\\` (a single backslash)
    // inside the actual file but are accessed correctly only if given as a
    // forward slash in the code:
    auto fileName = readProfileIniData(qsl("CommandLines/NameMapping/%1").arg(localName));
    // We want this to default to true if the setting doesn't exist, so we will
    // compare it to the opposite (which will be zero for a match) and convert
    // it to a boolean - so that a missing value will give a non-zero value
    // which becomes a true:
    auto saveCommands = static_cast<bool>(readProfileIniData(qsl("CommandLines/SaveHistory/%1").arg(localName)).compare(qsl("false"), Qt::CaseInsensitive));
    if (!fileName.isEmpty()) {
        // Ah, we've used this name before, so return the details:
        return {fileName, saveCommands};
    }

    // Else the name is not in the settings so we will have to create one:
    // Get the highest number used so far:
    bool isOk = false;
    auto usedIndex = readProfileIniData(qsl("CommandLines/UsedIndexes")).toInt(&isOk);
    if (!isOk || !usedIndex) {
        // The value was not found / is null - so force it to be the right one
        // to start with, remembering that it will be incremented before use:
        usedIndex = 0;
    }
    // Increment it and save the new value
    writeProfileIniData(qsl("CommandLines/UsedIndexes"), QString::number(++usedIndex));
    // Generate the name
    fileName = qsl("command_history_%1").arg(usedIndex, 2, 10, QLatin1Char('0'));
    // Save it:
    writeProfileIniData(qsl("CommandLines/NameMapping/%1").arg(localName), fileName);
    // And a default setting:
    writeProfileIniData(qsl("CommandLines/SaveHistory/%1").arg(localName), saveCommands ? qsl("true") : qsl("false"));
    // And return it - with the defaulted other setting:
    return {fileName, saveCommands};
}

void Host::setCmdLineSettings(const TCommandLine::CommandLineType type, const bool saveCommands, const QString& name)
{
    if (type == TCommandLine::MainCommandLine) {
        writeProfileIniData(qsl("CommandLines/SaveHistory/main"), saveCommands ? qsl("true") : qsl("false"));
        return;
    }
    QString localName{name};
    localName.replace(QRegularExpression(qsl("[\\/]")), qsl("_"));
    // We use a '/' in the name as that donotes a grouping (section) within the
    // QSetting's (INI) format with the left-most side of the first '/' as a
    // section header. They actually get converted to `\\` (a single backslash)
    // inside the actual file but are accessed correctly only if given as a
    // forward slash in the code:
    writeProfileIniData(qsl("CommandLines/SaveHistory/%1").arg(localName), saveCommands ? qsl("true") : qsl("false"));
}

// Derived from the one in dlgConnectionProfile class - but it does not need a
// host name argument...
QPair<bool, QString> Host::writeProfileData(const QString& item, const QString& what)
{
    QSaveFile file(mudlet::getMudletPath(mudlet::profileDataItemPath, getName(), item));
    if (file.open(QIODevice::WriteOnly | QIODevice::Unbuffered)) {
        QDataStream ofs(&file);
        if (mudlet::scmRunTimeQtVersion >= QVersionNumber(5, 13, 0)) {
            ofs.setVersion(mudlet::scmQDataStreamFormat_5_12);
        }
        ofs << what;
        if (!file.commit()) {
            qDebug() << "Host::writeProfileData: writing host data: " << file.errorString();
        }
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
    const bool success = file.open(QIODevice::ReadOnly);
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

            mudlet::self()->mFontManager.loadFont(filePath, packageName);
        }
    }
}

// ensures fonts from all installed packages are loaded in Mudlet
void Host::refreshPackageFonts()
{
    for (const auto& package : std::as_const(mInstalledPackages)) {
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
            || encoding == "BIG5-HKSCS"
            || encoding == "EUC-KR") {

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
    default:
        if (ansiCode == TTrigger::scmIgnored) {
            // No-op - corresponds to no setting or ignoring this aspect
            return QColor();
        } else if (ansiCode == TTrigger::scmDefault) {
            return isBackground ? mBgColor : mFgColor;
        } else if (ansiCode >= 16 && ansiCode <= 231) {
            // because color 1-15 behave like normal ANSI colors we need to subtract 16
            // 6x6 RGB color space
            const int r = (ansiCode - 16) / 36;
            const int g = (ansiCode - 16 - (r * 36)) / 6;
            const int b = (ansiCode - 16 - (r * 36)) - (g * 6);
            // Values are scaled according to the standard Xterm color palette
            // http://jonasjacek.github.io/colors/
            return QColor(r == 0 ? 0 : (r - 1) * 40 + 95,
                          g == 0 ? 0 : (g - 1) * 40 + 95,
                          b == 0 ? 0 : (b - 1) * 40 + 95);
        } else if (ansiCode < 256) {
            const int k = (ansiCode - 232) * 10 + 8;
            return QColor(k, k, k);
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
    auto inviteUrl = discordInfo.value(qsl("inviteurl"));
    // Will be of form: "https://discord.gg/#####"
    if (inviteUrl != QJsonValue::Undefined && !inviteUrl.toString().isEmpty() && inviteUrl.toString() != qsl("0")) {
        setDiscordInviteURL(inviteUrl.toString());
        pMudlet->updateDiscordNamedIcon();
        hasInvite = true;
    }

    bool hasApplicationId = false;
    bool hasCustomAppID = false;
    auto appID = discordInfo.value(qsl("applicationid"));
    if (appID != QJsonValue::Undefined) {
        hasApplicationId = true;
        if (appID.toString() == Discord::mMudletApplicationId) {
            pMudlet->mDiscord.setApplicationID(this, QString());
        } else {
            hasCustomAppID = true;
            pMudlet->mDiscord.setApplicationID(this, appID.toString());
            auto image = pMudlet->mDiscord.getLargeImage(this);

            if (image.isEmpty() || image == QLatin1String("mudlet")) {
                pMudlet->mDiscord.setLargeImage(this, qsl("server-icon"));
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
    auto gameName = discordInfo.value(qsl("game"));
    if (gameName != QJsonValue::Undefined) {
        setDiscordGameName(gameName.toString());
        pMudlet->updateDiscordNamedIcon();
        QPair<bool, QString> const richPresenceSupported = pMudlet->mDiscord.gameIntegrationSupported(getUrl());
        if (richPresenceSupported.first && pMudlet->mDiscord.usingMudletsDiscordID(this)) {
            pMudlet->mDiscord.setDetailText(this, tr("Playing %1").arg(richPresenceSupported.second));
            pMudlet->mDiscord.setLargeImage(this, richPresenceSupported.second);
            //: %1 is the game name and %2:%3 is game server address like: mudlet.org:23
            pMudlet->mDiscord.setLargeImageText(this, tr("%1 at %2:%3").arg(gameName.toString(), getUrl(), QString::number(getPort())));
        } else {
            // We are using a custom application id, so the top line is
            // likely to be saying "Playing MudName"
            if (richPresenceSupported.first) {
                pMudlet->mDiscord.setDetailText(this, QString());
                //: %1 is the game name and %2:%3 is game server address like: mudlet.org:23
                pMudlet->mDiscord.setLargeImageText(this, tr("%1 at %2:%3").arg(gameName.toString(), getUrl(), QString::number(getPort())));
                pMudlet->mDiscord.setLargeImage(this, qsl("server-icon"));
            }
        }
    }

    auto details = discordInfo.value(qsl("details"));
    if (details != QJsonValue::Undefined) {
        pMudlet->mDiscord.setDetailText(this, details.toString());
    }

    auto state = discordInfo.value(qsl("state"));
    if (state != QJsonValue::Undefined) {
        pMudlet->mDiscord.setStateText(this, state.toString());
    }

    auto largeImages = discordInfo.value(qsl("largeimage"));
    if (largeImages != QJsonValue::Undefined) {
        auto largeImage = largeImages.toArray().first();
        if (largeImage != QJsonValue::Undefined) {
            pMudlet->mDiscord.setLargeImage(this, largeImage.toString());
        }
    }

    auto largeImageText = discordInfo.value(qsl("largeimagetext"));
    if (largeImageText != QJsonValue::Undefined) {
        pMudlet->mDiscord.setLargeImageText(this, largeImageText.toString());
    }

    auto smallImages = discordInfo.value(qsl("smallimage"));
    if (smallImages != QJsonValue::Undefined) {
        auto smallImage = smallImages.toArray().first();
        if (smallImage != QJsonValue::Undefined) {
            pMudlet->mDiscord.setSmallImage(this, smallImage.toString());
        }
    }

    auto smallImageText = discordInfo.value(qsl("smallimagetext"));
    if ((smallImageText != QJsonValue::Undefined)) {
        pMudlet->mDiscord.setSmallImageText(this, smallImageText.toString());
    }

    // Use -1 so we can detect (at least during debugging) that a value of 0
    // has been seen:
    int64_t timeStamp = -1;
    auto endTimeStamp = discordInfo.value(qsl("endtime"));
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
        auto startTimeStamp = discordInfo.value(qsl("starttime"));
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
    auto partyMax = discordInfo.value(qsl("partymax"));
    auto partySize = discordInfo.value(qsl("partysize"));
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
    writeProfileData(qsl("discordApplicationId"), s);
}

const QString& Host::getDiscordApplicationID()
{
    return mDiscordApplicationID;
}

void Host::setDiscordInviteURL(const QString& s)
{
    mDiscordInviteURL = s;
    writeProfileData(qsl("discordInviteURL"), s);
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

QString  Host::getSpellDic()
{
    if (!mSpellDic.isEmpty()) {
        return mSpellDic;
    }
#if defined(Q_OS_OPENBSD)
    // OpenBSD does not ship a USA dictionary so we will have to use
    // a different starting one to try and locate system ones
    return (qsl("en-GB"));
#else
    return (qsl("en_US"));
#endif
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
    const bool useDictionary = true;
    bool dictionaryChanged {};
    // Copy the value while we have the lock:
    const bool isSpellCheckingEnabled = mEnableSpellCheck;
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
        // This will propagate the changes in the two flags to the main
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
void Host::setName(const QString& name)
{
    if (mHostName == name) {
        return;
    }

    TDebug::changeHostName(this, name);
    int currentPlayerRoom = 0;
    if (mpMap) {
        currentPlayerRoom = mpMap->mRoomIdHash.take(mHostName);
    }

    mHostName = name;

    mTelnet.mProfileName = name;
    if (mpMap) {
        mpMap->mProfileName = name;
        if (currentPlayerRoom) {
            mpMap->mRoomIdHash.insert(name, currentPlayerRoom);
        }
    }

    if (mpConsole) {
        // If skipped they will be taken care of in the TMainConsole constructor:
        mpConsole->setProperty("HostName", name);
        mpConsole->setProfileName(name);
    }
    mTimerUnit.changeHostName(name);
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
    auto *job = new QKeychain::ReadPasswordJob(qsl("Mudlet profile"));
    job->setAutoDelete(false);
    job->setInsecureFallback(false);

    job->setKey(getName());

    connect(job, &QKeychain::ReadPasswordJob::finished, this, [=](QKeychain::Job* task) {
        if (task->error()) {
            const auto error = task->errorString();
            if (error != qsl("Entry not found") && error != qsl("No match")) {
                qDebug().nospace().noquote() << "Host::loadSecuredPassword() ERROR - could not retrieve secure password for \"" << getName() << "\", error is: " << error << ".";
            }

        } else {
            auto readJob = static_cast<QKeychain::ReadPasswordJob*>(task);
            setPass(readJob->textData());
        }

        task->deleteLater();
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

void Host::setBufferSearchOptions(const TConsole::SearchOptions optionsState)
{
    mBufferSearchOptions = optionsState;
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
    if (name.isEmpty() or name == qsl("main")) {
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
        failMessage << qsl("internal error: no main TConsole - please report").arg(windowName);
        return qMakePair(false, failMessage);
    }

    if (windowName.isEmpty() || windowName == QLatin1String("main")) {
        return qMakePair(true, mpConsole->getLines(lineFrom, lineTo));
    }

    auto pC = mpConsole->mSubConsoleMap.value(windowName);
    if (!pC) {
        QStringList failMessage;
        failMessage << qsl("mini console, user window or buffer '%1' not found").arg(windowName);
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
        return {false, qsl("label with the name '%1' already exists").arg(name)};
    }

    auto hostName(getName());
    auto console = mpConsole->mSubConsoleMap.value(name);
    auto dockwidget = mpConsole->mDockWidgetMap.value(name);

    if (!console && !dockwidget) {
        // The name is not used in either the QMaps of all user created TConsole
        // or TDockWidget instances - so we can make a NEW one:
        dockwidget = new TDockWidget(this, name);
        dockwidget->setObjectName(qsl("dockWindow_%1_%2").arg(hostName, name));
        dockwidget->setContentsMargins(0, 0, 0, 0);
        dockwidget->setFeatures(QDockWidget::DockWidgetClosable|QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
        dockwidget->setWindowTitle(name);
        mpConsole->mDockWidgetMap.insert(name, dockwidget);
        // It wasn't obvious but the parent passed to the TConsole constructor
        // is sliced down to a QWidget and is NOT a TDockWidget pointer:
        console = new TConsole(this, name, TConsole::UserWindow, dockwidget->widget());
        console->setObjectName(qsl("dockWindowConsole_%1_%2").arg(hostName, name));
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
        return {false, qsl("userwindow '%1' already exists").arg(name)};
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
            return {false, qsl(R"("docking option "%1" not available. available docking options are "t" top, "b" bottom, "r" right, "l" left and "f" floating")").arg(area)};
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
        // CHECK: The absence of an explicit return statement in this block means that
        // reusing an existing mini console causes the lua function to seem to
        // fail - is this as per Wiki?
        // This part was causing problems with UserWindows
        if (!pW) {
            pC->resize(width, height);
            pC->move(x, y);
            return {false, qsl("miniconsole '%1' already exists, moving/resizing '%1'").arg(name)};
        }
    }
    return {false, qsl("miniconsole/userwindow '%1' already exists").arg(name)};
}

std::pair<bool, QString> Host::createScrollBox(const QString& windowname, const QString& name, int x, int y, int width, int height) const
{
    if (!mpConsole) {
        return {false, QString()};
    }

    auto pS = mpConsole->mScrollBoxMap.value(name);
    if (!pS) {
        pS = mpConsole->createScrollBox(windowname, name, x, y, width, height);
        if (pS) {
            return {true, QString()};
        }
    } else if (pS) {
        pS->resize(width, height);
        pS->move(x, y);
        return {false, qsl("scrollBox '%1' already exists, moving/resizing '%1'").arg(name)};
    }
    return {false, qsl("scrollBox '%1' already exists").arg(name)};
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
        return {false, qsl("label '%1' already exists").arg(name)};
    } else if (pC) {
        return {false, qsl("a miniconsole/userwindow with the name '%1' already exists").arg(name)};
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
    for (const auto& variable : std::as_const(varUnit->savedVars)) {
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
    for (const auto& variable : std::as_const(unhideSavedVars)) {
        varUnit->removeHidden(variable);
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

// Doesn't work on the errors or central debug consoles:
bool Host::clearWindow(const QString& name)
{
    if (!mpConsole) {
        return false;
    }

    return mpConsole->clear(name);
}

bool Host::showWindow(const QString& name)
{
    if (!mpConsole) {
        return false;
    }

    auto pC = mpConsole->mSubConsoleMap.value(name);
    auto pL = mpConsole->mLabelMap.value(name);
    auto pN = mpConsole->mSubCommandLineMap.value(name);
    auto pS = mpConsole->mScrollBoxMap.value(name);
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

    if (pS) {
        pS->show();
        return true;
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
    auto pS = mpConsole->mScrollBoxMap.value(name);

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

    if (pS) {
        pS->hide();
        return true;
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
    auto pS = mpConsole->mScrollBoxMap.value(name);

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

    if (pS) {
        pS->resize(x1, y1);
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
    auto pS = mpConsole->mScrollBoxMap.value(name);

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

    if (pS) {
        pS->move(x1, y1);
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
    //checks - for reasons why the indicated thing might not be moved to the indicated destination
    auto pDCheck = mpConsole->mDockWidgetMap.value(name);
    if (pDCheck) {
        return {false, qsl("element '%1' is the base of a floating/dockable user window and may not be moved").arg(name)};
    }
    if (mpDockableMapWidget) {
        if (!name.compare(QLatin1String("mapper"), Qt::CaseInsensitive)) {
            return {false, qsl("element '%1' is the map in a floating/dockable window and may not be moved").arg(name)};
        }
        if (!windowname.compare(QLatin1String("mapper"), Qt::CaseInsensitive)) {
            return {false, qsl("window '%1' is the map in a floating/dockable window and may not receive other elements").arg(windowname)};
        }
    }

    //children
    auto pL = mpConsole->mLabelMap.value(name);
    auto pC = mpConsole->mSubConsoleMap.value(name);
    auto pM = mpConsole->mpMapper;
    auto pN = mpConsole->mSubCommandLineMap.value(name);
    auto pS = mpConsole->mScrollBoxMap.value(name);
    //parents
    auto pW = mpConsole->mpMainFrame;
    auto pD = mpConsole->mDockWidgetMap.value(windowname);
    auto pSW = mpConsole->mScrollBoxMap.value(windowname);

    if (!pSW && !pD && windowname.compare(QLatin1String("main"), Qt::CaseInsensitive)) {
        // Third argument is non-zero (i.e. true) if the window name is NOT
        // the given string:
        return {false, qsl("window '%1' not found").arg(windowname)};
    }

    if (pD) {
        pW = pD->widget();
    }

    if (pSW) {
        pW = pSW->widget();
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
    } else if (pS) {
        pS->setParent(pW);
        pS->move(x1, y1);
        if (show) {
            pS->show();
        }
        return {true, QString()};
    } else if (pN) {
        pN->setParent(pW);
        pN->move(x1, y1);
        if (show) {
            pN->show();
        }
        return {true, QString()};
    } else if (pM && !name.compare(QLatin1String("mapper"), Qt::CaseInsensitive)) {
        pM->setParent(pW);
        pM->move(x1, y1);
        if (show) {
            pM->show();
        }
        return {true, QString()};
    }

    return {false, qsl("element '%1' not found").arg(name)};
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
        return {false, qsl("cannot create map widget. Do you already use an embedded mapper?")};
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
            return {false, qsl(R"("docking option "%1" not available. available docking options are "t" top, "b" bottom, "r" right, "l" left and "f" floating")").arg(area)};
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
        return {false, qsl("no map widget found to close")};
    }
    if (!pM->isVisible()) {
        return {false, qsl("map widget already closed")};
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

std::pair<bool, QString> Host::setMovie(const QString& name, const QString& moviePath)
{
    if (!mpConsole) {
        return {false, QString()};
    }

    auto pL = mpConsole->mLabelMap.value(name);
    if (!pL) {
        return {false, qsl("label '%1' does not exist").arg(name)};
    }

    auto myMovie = pL->mpMovie;
    if (!myMovie) {
        myMovie = new QMovie();
        mGifTracker.registerGif(myMovie);
        myMovie->setCacheMode(QMovie::CacheAll);
        pL->mpMovie = myMovie;
        myMovie->setParent(pL);
    }

    myMovie->setFileName(moviePath);

    if (!myMovie->isValid()) {
        return {false, qsl("no valid movie found at '%1'").arg(moviePath)};
    }

    myMovie->stop();
    pL->setMovie(myMovie);
    myMovie->start();
    return {true, QString()};

}

QSize Host::calcFontSize(const QString& windowName)
{
    if (!mpConsole) {
        return QSize(-1, -1);
    }

    QFont font;
    if (windowName.isEmpty() || windowName.compare(qsl("main"), Qt::CaseSensitive) == 0) {
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
    if (mpEditorDialog) {
        mpEditorDialog->setStyleSheet(styleSheet);
    }

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
        QString styleSheet = pL->styleSheet();
        QString newColor = QString("background-color: rgba(%1, %2, %3, %4);").arg(r).arg(g).arg(b).arg(alpha);
        if (styleSheet.contains(qsl("background-color"))) {
            QRegularExpression re("background-color: .*;");
            styleSheet.replace(re, newColor);
        } else {
            styleSheet.append(newColor);
        }

        pL->setStyleSheet(styleSheet);
        return true;
    }

    return false;
}

std::optional<QColor> Host::getBackgroundColor(const QString& name) const
{
    if (!mpConsole) {
        return {};
    }

    auto pC = mpConsole->mSubConsoleMap.value(name);
    auto pL = mpConsole->mLabelMap.value(name);
    if (pC) {
        return {pC->mBgColor};
    }

    if (pL) {
        return {pL->palette().color(QPalette::Window)};
    }

    return {};
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

    if (name.isEmpty() || name.compare(qsl("main"), Qt::CaseSensitive) == 0) {
        mpConsole->setConsoleBackgroundImage(imgPath, mode);
        return true;
    }

    auto pL = mpConsole->mLabelMap.value(name);
    if (pL) {
        const QPixmap bgPixmap(imgPath);
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

    if (name.isEmpty() || name.compare(qsl("main"), Qt::CaseSensitive) == 0) {
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

bool Host::setCommandBackgroundColor(const QString& name, int r, int g, int b, int alpha)
{
    if (!mpConsole) {
        return false;
    }

    auto pC = mpConsole->mSubConsoleMap.value(name);
    if (pC) {
        pC->setCommandBgColor(r, g, b, alpha);
        return true;
    }
    return false;
}

bool Host::setCommandForegroundColor(const QString& name, int r, int g, int b, int alpha)
{
    if (!mpConsole) {
        return false;
    }

    auto pC = mpConsole->mSubConsoleMap.value(name);
    if (pC) {
        pC->setCommandFgColor(r, g, b, alpha);
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
    const bool visStatus = mpMap->mpMapper->isVisible();
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
    mpDockableMapWidget->setObjectName(qsl("dockMap_%1").arg(hostName));
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
        const QDateTime now(QDateTime::currentDateTime());
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
            // Needed to set the area selector widget to right area when map is
            // loaded by clicking on Map main toolbar button:
            pMap->mpMapper->updateAreaComboBox();
            pMap->mpMapper->resetAreaComboBoxToPlayerRoomArea();
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
        for (auto dockedConsoleName : std::as_const(mDockLayoutChanges)) {
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
        for (auto pToolBar : std::as_const(mToolbarLayoutChanges)) {
            if (!pToolBar || pToolBar.isNull()) {
                // This can happen when a TToolBar is deleted
                continue;
            }
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

void Host::setupIreDriverBugfix()
{
    // IRE games suffer from unnecessary linebreaks across split packets
    // but other games implementing GA don't. Thus, only enable the workaround
    // for the former only

    const QStringList ireGameUrls{"achaea.com", "lusternia.com", "imperian.com", "aetolia.com", "starmourn.com"};
    if (ireGameUrls.contains(getUrl(), Qt::CaseInsensitive)) {
        set_USE_IRE_DRIVER_BUGFIX(true);
    }
}

void Host::setControlCharacterMode(const ControlCharacterMode mode)
{
    if (Q_UNLIKELY(!(mode == ControlCharacterMode::AsIs
                     || mode == ControlCharacterMode::Picture
                     || mode == ControlCharacterMode::OEM))) {
        return;
    }

    if (mControlCharacter != mode) {
        mControlCharacter = mode;
        emit signal_controlCharacterHandlingChanged(mode);
    }
}

std::optional<QString> Host::windowType(const QString& name) const
{
    if (Q_UNLIKELY(name == QLatin1String("main"))) {
        return {QLatin1String("main")};
    }

    if (mpConsole->mLabelMap.contains(name)) {
        return {qsl("label")};
    }

    auto pWindow = mpConsole->mSubConsoleMap.value(name);
    if (pWindow) {
        switch (pWindow->getType()) {
        case TConsole::UserWindow:
            return {qsl("userwindow")};
        case TConsole::Buffer:
            return {qsl("buffer")};
        case TConsole::SubConsole:
            return {qsl("miniconsole")};
        case TConsole::UnknownType:
            [[fallthrough]];
        case TConsole::CentralDebugConsole:
            [[fallthrough]];
        case TConsole::ErrorConsole:
            [[fallthrough]];
        case TConsole::MainConsole:
            [[fallthrough]];
        default:
            Q_UNREACHABLE();
            return {};
        }
    }

    if (mpConsole->mSubCommandLineMap.contains(name)) {
        return {qsl("commandline")};
    }

    return {};
}

void Host::setLargeAreaExitArrows(const bool state)
{
    if (mLargeAreaExitArrows != state) {
        mLargeAreaExitArrows = state;
        if (mpMap && mpMap->mpMapper && mpMap->mpMapper->mp2dMap) {
            mpMap->mpMapper->mp2dMap->mLargeAreaExitArrows = state;
            mpMap->mpMapper->mp2dMap->update();
        }
    }
}

void Host::setEditorShowBidi(const bool state)
{
    if (mEditorShowBidi != state) {
        mEditorShowBidi = state;
        if (mpEditorDialog) {
            mpEditorDialog->setEditorShowBidi(state);
        }
    }
}

bool Host::caretEnabled() const {
    return mCaretEnabled;
}

void Host::setCaretEnabled(bool enabled) {
    mCaretEnabled = enabled;
    mpConsole->setCaretMode(enabled);
}

void Host::setFocusOnHostActiveCommandLine()
{
    if (mFocusTimerRunning) {
        return;
    }

    mFocusTimerRunning = true;
    QTimer::singleShot(0, this, [this]() {
        auto pCommandLine = activeCommandLine();
        if (pCommandLine) {
            pCommandLine->activateWindow();
            pCommandLine->console()->show();
            pCommandLine->console()->raise();
            pCommandLine->console()->repaint();
            pCommandLine->setFocus(Qt::OtherFocusReason);
        } else {
            mpConsole->mpCommandLine->activateWindow();
            mpConsole->show();
            mpConsole->raise();
            mpConsole->repaint();
            mpConsole->mpCommandLine->setFocus(Qt::OtherFocusReason);
        }
        mFocusTimerRunning = false;
    });
}

void Host::recordActiveCommandLine(TCommandLine* pCommandLine)
{
    mpLastCommandLineUsed.removeAll(QPointer<TCommandLine>(pCommandLine));
    mpLastCommandLineUsed.push(QPointer<TCommandLine>(pCommandLine));
}

void Host::forgetCommandLine(TCommandLine* pCommandLine)
{
    if (pCommandLine) {
        mpLastCommandLineUsed.removeAll(QPointer<TCommandLine>(pCommandLine));
    }
}

// Returns a pointer to the last used TCommandLine for this profile:
TCommandLine* Host::activeCommandLine()
{
    TCommandLine* pCommandLine = nullptr;
    if (mpLastCommandLineUsed.isEmpty()) {
        return nullptr;
    }

    do {
        pCommandLine = mpLastCommandLineUsed.top();
        if (!pCommandLine) {
            mpLastCommandLineUsed.pop();
        }
    } while (!mpLastCommandLineUsed.isEmpty() && !pCommandLine);

    return pCommandLine;
}

QPointer<TConsole> Host::parentTConsole(QObject* start) const
{
    QPointer<TConsole> result;
    auto ptr = start;
    if (!ptr) {
        // Handle pathalogical case:
        return result;
    }
    do {
        ptr = ptr->parent();
    } while (ptr && !ptr->inherits("TConsole"));
    // QObject::inherits(...) uses a const char* - so no need to wrap raw string literal!
    if (!ptr) {
        // Handle not found case:
        return result;
    }
    return qobject_cast<TConsole*>(ptr);
}

void Host::setBorders(QMargins borders)
{
    auto original = mBorders;
    if (borders == original) {
        return;
    }
    mBorders = borders;
    if (mpConsole.isNull()) {
        return;
    }
    auto x = mpConsole->width();
    auto y = mpConsole->height();
    const QSize s = QSize(x, y);
    QResizeEvent event(s, s);
    QApplication::sendEvent(mpConsole, &event);
    mpConsole->raiseMudletSysWindowResizeEvent(x, y);
}

void Host::setCommandLineHistorySaveSize(const int lines)
{
    if (mCommandLineHistorySaveSize != lines) {
        mCommandLineHistorySaveSize = lines;
    }
}
